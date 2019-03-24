#include "OutfitSystem.h"

#include "skse/PapyrusNativeFunctions.h"
#include "skse/PapyrusObjects.h"
#include "skse/PapyrusVM.h"

#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "Services/ArmorAddonOverrideService.h"

namespace CobbPapyrus {
   namespace OutfitSystem {
      VMResultArray<RE::TESObjectARMO*> GetOutfitContents(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         VMResultArray<RE::TESObjectARMO*> result;
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto  outfit = service.getOutfit(name.data);
            auto& armors = outfit.armors;
            for (auto it = armors.begin(); it != armors.end(); ++it)
               result.push_back(*it);
         } catch (std::out_of_range) {}
         return result;
      }
      BSFixedString GetSelectedOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         return service.currentOutfit.name.c_str();
      }
      bool IsEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         return service.enabled;
      }
      VMResultArray<BSFixedString> ListOutfits(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         VMResultArray<BSFixedString> result;
         std::vector<std::string> intermediate;
         service.getOutfitNames(intermediate);
         result.reserve(intermediate.size());
         for (auto it = intermediate.begin(); it != intermediate.end(); ++it)
            result.push_back(it->c_str());
         return result;
      }
      void OverwriteOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, VMArray<RE::TESObjectARMO*> armors) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         //
         std::vector<RE::TESObjectARMO*> list;
         auto count = armors.Length();
         list.reserve(count);
         for (UInt32 i = 1; i < count; i++) {
            RE::TESObjectARMO* ptr = nullptr;
            armors.Get(&ptr, i);
            if (ptr)
               list.push_back(ptr);
         }
         service.addOutfit(name.data, list);
      }
      void SetEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, bool state) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         service.setEnabled(state);
      }
      void SetSelectedOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         service.setOutfit(name.data);
      }
   }
}

bool CobbPapyrus::OutfitSystem::Register(VMClassRegistry* registry) {
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<RE::TESObjectARMO*>, BSFixedString>(
      "GetOutfitContents",
      "SkyrimOutfitSystemNativeFuncs",
      GetOutfitContents,
      registry
   ));
   registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>(
      "IsEnabled",
      "SkyrimOutfitSystemNativeFuncs",
      IsEnabled,
      registry
   ));
   registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>(
      "GetSelectedOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      GetSelectedOutfit,
      registry
   ));
   registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<BSFixedString>>(
      "ListOutfits",
      "SkyrimOutfitSystemNativeFuncs",
      ListOutfits,
      registry
   ));
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, VMArray<RE::TESObjectARMO*>>(
      "OverwriteOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      OverwriteOutfit,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>(
      "SetEnabled",
      "SkyrimOutfitSystemNativeFuncs",
      SetEnabled,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
      "SetSelectedOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      SetSelectedOutfit,
      registry
   ));
   return true;
}