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
#include "Patches/Exploratory.h"
#include "Patches/OverridePlayerSkinning.h"

#include "skse/GameRTTI.h"
#include "skse/GameObjects.h"

PluginHandle			       g_pluginHandle = kPluginHandle_Invalid;
SKSEPapyrusInterface*       g_papyrus            = nullptr;
SKSEMessagingInterface*     g_ISKSEMessaging     = nullptr;
SKSESerializationInterface* g_ISKSESerialization = nullptr;

static const char*  g_pluginName = "SkyrimOutfitSystem";
const UInt32 g_pluginVersion = 0x01000000; // 0xAABBCCDD = AA.BB.CC.DD with values converted to decimal // major.minor.update.internal-build-or-zero

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message* message);

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
      g_papyrus = (SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);
      return true;
   }
};

void Callback_Messaging_SKSE(SKSEMessagingInterface::Message* message) {
   if (message->type == SKSEMessagingInterface::kMessage_PostLoad) {
   } else if (message->type == SKSEMessagingInterface::kMessage_PostPostLoad) {
   } else if (message->type == SKSEMessagingInterface::kMessage_DataLoaded) {
      //
      // TEST TEST TEST TEST TEST
      //
      auto& svc = ArmorAddonOverrideService::GetInstance();
      std::vector<RE::TESObjectARMO*> armors;
      //
      auto greybeardRobes = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A44), TESForm, TESObjectARMO);
      auto greybeardBoots = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A46), TESForm, TESObjectARMO);
      if (greybeardRobes)
         armors.push_back(greybeardRobes);
      if (greybeardBoots)
         armors.push_back(greybeardBoots);
      svc.addOutfit("Test outfit", armors);
      svc.setOutfit("Test outfit");
      svc.enabled = true;
   } else if (message->type == SKSEMessagingInterface::kMessage_NewGame) {
   } else if (message->type == SKSEMessagingInterface::kMessage_PreLoadGame) {
   }
};