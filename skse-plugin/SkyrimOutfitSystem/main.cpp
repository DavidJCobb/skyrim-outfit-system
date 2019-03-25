#include "skse/PluginAPI.h"		// super
#include "skse/skse_version.h"	// What version of SKSE is running?
#include "skse/SafeWrite.h"
#include "skse/GameAPI.h"
#include <shlobj.h>				// CSIDL_MYCODUMENTS
#include <psapi.h>  // MODULEINFO, GetModuleInformation
#pragma comment( lib, "psapi.lib" ) // needed for PSAPI to link properly
#include <string>

//#include "Services/INI.h"
#include "Services/ArmorAddonOverrideService.h"
#include "Services/StorableObject.h"
#include "Patches/Exploratory.h"
#include "Patches/OverridePlayerSkinning.h"
#include "Papyrus/OutfitSystem.h"

#include "skse/GameRTTI.h"
#include "skse/GameObjects.h"

PluginHandle			       g_pluginHandle = kPluginHandle_Invalid;
SKSEPapyrusInterface*       g_papyrus            = nullptr;
SKSEMessagingInterface*     g_ISKSEMessaging     = nullptr;
SKSESerializationInterface* g_ISKSESerialization = nullptr;

static UInt32 g_pluginSerializationSignature = 'cOft'; // TODO: Confirm with SKSE team

static const char* g_pluginName    = "SkyrimOutfitSystem";
const UInt32       g_pluginVersion = 0x01000000; // 0xAABBCCDD = AA.BB.CC.DD with values converted to decimal // major.minor.update.internal-build-or-zero

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message* message);
void Callback_Serialization_Save(SKSESerializationInterface * intfc);
void Callback_Serialization_Load(SKSESerializationInterface * intfc);

extern "C" {
   //
   // SKSEPlugin_Query: Called by SKSE to learn about this plug-in and check that it's safe to load.
   //
   bool SKSEPlugin_Query(const SKSEInterface* skse, PluginInfo* info) {
      gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim\\SKSE\\SkyrimOutfitSystem.log");
      gLog.SetPrintLevel(IDebugLog::kLevel_Error);
      gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);
      //
      _MESSAGE("Query");
      //
      // Populate info structure.
      //
      info->infoVersion = PluginInfo::kInfoVersion;
      info->name        = g_pluginName;
      info->version     = g_pluginVersion;
      {  // Log version number
         auto  v     = info->version;
         UInt8 main  = v >> 0x18;
         UInt8 major = v >> 0x10;
         UInt8 minor = v >> 0x08;
         _MESSAGE("Current version is %d.%d.%d.", main, major, minor);
      }
      {  // Get run-time information
         HMODULE    baseAddr = GetModuleHandle("SkyrimOutfitSystem"); // DLL filename
         MODULEINFO info;
         if (baseAddr && GetModuleInformation(GetCurrentProcess(), baseAddr, &info, sizeof(info)))
            _MESSAGE("We're loaded to the span of memory at %08X - %08X.", info.lpBaseOfDll, (UInt32)info.lpBaseOfDll + info.SizeOfImage);
      }
      //
      // Store plugin handle so we can identify ourselves later.
      //
      g_pluginHandle = skse->GetPluginHandle();
      //
      //g_SKSEVersionSupported = (skse->skseVersion >= 0x01070300); // 1.7.3.0
      //
      if (skse->isEditor) {
         _MESSAGE("We've been loaded in the Creation Kit. Marking as incompatible.");
         return false;
      } else if (skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0) {
         _MESSAGE("Unsupported Skyrim version: %08X.", skse->runtimeVersion);
         return false;
      }
      {  // Get the messaging interface and query its version.
         g_ISKSEMessaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
         if (!g_ISKSEMessaging) {
            _MESSAGE("Couldn't get messaging interface.");
            return false;
         } else if (g_ISKSEMessaging->interfaceVersion < SKSEMessagingInterface::kInterfaceVersion) {
            _MESSAGE("Messaging interface too old (%d; we expected %d).", g_ISKSEMessaging->interfaceVersion, SKSEMessagingInterface::kInterfaceVersion);
            return false;
         }
      }
      {  // Get the serialization interface and query its version.
         g_ISKSESerialization = (SKSESerializationInterface *)skse->QueryInterface(kInterface_Serialization);
         if (!g_ISKSESerialization) {
            _MESSAGE("Couldn't get serialization interface.");
            return false;
         } else if (g_ISKSESerialization->version < SKSESerializationInterface::kVersion) {
            _MESSAGE("Serialization interface too old (%d; we expected %d).", g_ISKSESerialization->version, SKSESerializationInterface::kVersion);
            return false;
         }
      }
      //
      // This plug-in supports the current Skyrim and SKSE versions:
      //
      return true;
   }
   //
   // SKSEPlugin_Load: Called by SKSE to load this plug-in.
   //
   void _RegisterAndEchoPapyrus(SKSEPapyrusInterface::RegisterFunctions callback, char* module) {
      bool status = g_papyrus->Register(callback);
      if (status)
         _MESSAGE("Papyrus registration %s for %s.", "succeeded", module);
      else
         _MESSAGE("Papyrus registration %s for %s.", "FAILED", module);
   };
   bool SKSEPlugin_Load(const SKSEInterface* skse) {
      _MESSAGE("Load.");
      //SkyrimOutfitSystem::INISettingManager::GetInstance().Load();
      {  // Patches:
         SkyrimOutfitSystem::Patches::Exploratory::Apply();
         SkyrimOutfitSystem::Patches::OverridePlayerSkinning::Apply();
      }
      {  // Messaging callbacks.
         g_ISKSEMessaging->RegisterListener(g_pluginHandle, "SKSE", Callback_Messaging_SKSE);
      }
      {  // Serialization callbacks.
         g_ISKSESerialization->SetUniqueID(g_pluginHandle, g_pluginSerializationSignature);
         //g_ISKSESerialization->SetRevertCallback  (g_pluginHandle, Callback_Serialization_Revert);
         g_ISKSESerialization->SetSaveCallback(g_pluginHandle, Callback_Serialization_Save);
         g_ISKSESerialization->SetLoadCallback(g_pluginHandle, Callback_Serialization_Load);
      }
      {  // Papyrus registrations
         g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
         _RegisterAndEchoPapyrus(CobbPapyrus::OutfitSystem::Register, "SkyrimOutfitSystemNativeFuncs");
      }
      return true;
   }
};

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message* message) {
   if (message->type == SKSEMessagingInterface::kMessage_PostLoad) {
   } else if (message->type == SKSEMessagingInterface::kMessage_PostPostLoad) {
   } else if (message->type == SKSEMessagingInterface::kMessage_DataLoaded) {
   } else if (message->type == SKSEMessagingInterface::kMessage_NewGame) {
      ArmorAddonOverrideService::GetInstance().reset();
   } else if (message->type == SKSEMessagingInterface::kMessage_PreLoadGame) {
      ArmorAddonOverrideService::GetInstance().reset(); // AAOS::load resets as well, but this is needed in case the save we're about to load doesn't have any AAOS data.
   }
};
void Callback_Serialization_Save(SKSESerializationInterface* intfc) {
   _MESSAGE("Saving...");
   //
   if (intfc->OpenRecord(ArmorAddonOverrideService::signature, ArmorAddonOverrideService::kSaveVersion)) {
      try {
         auto& service = ArmorAddonOverrideService::GetInstance();
         service.save(intfc);
      } catch (const ArmorAddonOverrideService::save_error& exception) {
         _MESSAGE("Save FAILED for ArmorAddonOverrideService.");
         _MESSAGE(" - Exception string: %s", exception.what());
      }
   } else
      _MESSAGE("Save FAILED for ArmorAddonOverrideService. Record didn't open.");
   //
   _MESSAGE("Saving done!");
}
void Callback_Serialization_Load(SKSESerializationInterface* intfc) {
   _MESSAGE("Loading...");
   //
   UInt32 type;    // This IS correct. A UInt32 and a four-character ASCII string have the same length (and can be read interchangeably, it seems).
   UInt32 version;
   UInt32 length;
   bool   error = false;
   //
   while (!error && intfc->GetNextRecordInfo(&type, &version, &length)) {
      switch (type) {
         case ArmorAddonOverrideService::signature:
            try {
               auto& service = ArmorAddonOverrideService::GetInstance();
               service.load(intfc, version);
            } catch (const ArmorAddonOverrideService::load_error& exception) {
               _MESSAGE("Load FAILED for ArmorAddonOverrideService.");
               _MESSAGE(" - Exception string: %s", exception.what());
            }
            break;
         default:
            _MESSAGE("Loading: Unhandled type %c%c%c%c", (char)(type >> 0x18), (char)(type >> 0x10), (char)(type >> 0x8), (char)type);
            error = true;
            break;
      }
   }
   //
   _MESSAGE("Loading done!");
}