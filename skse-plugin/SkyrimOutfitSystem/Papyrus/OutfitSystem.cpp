#include "OutfitSystem.h"

#include "skse/PapyrusNativeFunctions.h"
#include "skse/PapyrusObjects.h"
#include "skse/PapyrusVM.h"

#include "skse/GameRTTI.h"

#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "ReverseEngineered/Forms/Actor.h"
#include "Services/ArmorAddonOverrideService.h"

namespace CobbPapyrus {
   namespace OutfitSystem {
      SInt32 GetOutfitNameMaxLength(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
         return ArmorAddonOverrideService::ce_outfitNameMaxLength;
      }
      VMResultArray<RE::TESObjectARMO*> GetWornItems(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::Actor* target) {
         VMResultArray<RE::TESObjectARMO*> result;
         if (target == nullptr) {
            registry->LogError("Cannot retrieve data for a None actor.", stackId);
            return result;
         }
         //
         class _Visitor : public RE::ExtraContainerChanges::InventoryVisitor {
            //
            // If the player has a shield equipped, and if we're not overriding that 
            // shield, then we need to grab the equipped shield's worn-flags.
            //
            private:
               virtual BOOL Visit(RE::InventoryEntryData* data) override {
                  auto form = data->type;
                  if (form && form->formType == kFormType_Armor)
                     this->list.push_back((RE::TESObjectARMO*) form);
                  return true;
               };
            public:
               VMResultArray<RE::TESObjectARMO*>& list;
               //
               _Visitor(VMResultArray<RE::TESObjectARMO*>& l) : list(l) {};
         };
         auto inventory = RE::GetExtraContainerChangesData(target);
         if (inventory) {
            _Visitor visitor(result);
            CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
         }
         return result;
      }
      void RefreshArmorFor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::Actor* target) {
         ERROR_AND_RETURN_IF(target == nullptr, "Cannot refresh armor on a None actor.", registry, stackId);
         auto pm = target->processManager;
         if (pm) {
            CALL_MEMBER_FN(pm, SetEquipFlag)(true);
            CALL_MEMBER_FN(pm, UpdateEquipment)(target);
         }
      }
      //
      namespace BodySlotListing {
         enum {
            kBodySlotMin = 30,
            kBodySlotMax = 61,
         };
         static struct {
            std::vector<SInt32>             bodySlots;
            std::vector<std::string>        armorNames;
            std::vector<RE::TESObjectARMO*> armors;
         } data;
         //
         void Clear(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
         }
         void Prep(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
            //
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
               auto& outfit = service.getOutfit(name.data);
               auto& armors = outfit.armors;
               for (UInt8 i = kBodySlotMin; i <= kBodySlotMax; i++) {
                  UInt32 mask = 1 << (i - kBodySlotMin);
                  for (auto it = armors.begin(); it != armors.end(); it++) {
                     RE::TESObjectARMO* armor = *it;
                     if (armor && (armor->bipedObject.data.parts & mask)) {
                        data.bodySlots.push_back(i);
                        data.armors.push_back(armor);
                        { // name
                           TESFullName* pFullName = DYNAMIC_CAST(armor, TESObjectARMO, TESFullName);
                           if (pFullName)
                              data.armorNames.push_back(pFullName->name.data);
                           else
                              data.armorNames.push_back("");
                        }
                     }
                  }
               }
            } catch (std::out_of_range) {
               registry->LogWarning("The specified outfit does not exist.", stackId);
            }
         }
         VMResultArray<RE::TESObjectARMO*> GetArmorForms(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<RE::TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
               result.push_back(*it);
            return result;
         }
         VMResultArray<BSFixedString> GetArmorNames(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<BSFixedString> result;
            auto& list = data.armorNames;
            for (auto it = list.begin(); it != list.end(); it++)
               result.push_back(it->c_str());
            return result;
         }
         VMResultArray<SInt32> GetSlotIndices(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<SInt32> result;
            auto& list = data.bodySlots;
            for (auto it = list.begin(); it != list.end(); it++)
               result.push_back(*it);
            return result;
         }
      }
      //
      bool ArmorConflictsWithOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::TESObjectARMO* armor, BSFixedString name) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOutfit(name.data);
            return outfit.conflictsWith(armor);
         } catch (std::out_of_range) {
            registry->LogWarning("The specified outfit does not exist.", stackId);
            return false;
         }
      }
      void CreateOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            service.addOutfit(name.data);
         } catch (ArmorAddonOverrideService::bad_name) {
            registry->LogError("Invalid outfit name specified.", stackId);
            return;
         }
      }
      void DeleteOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         service.deleteOutfit(name.data);
      }
      VMResultArray<RE::TESObjectARMO*> GetOutfitContents(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         VMResultArray<RE::TESObjectARMO*> result;
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOutfit(name.data);
            auto& armors = outfit.armors;
            for (auto it = armors.begin(); it != armors.end(); ++it)
               result.push_back(*it);
         } catch (std::out_of_range) {
            registry->LogWarning("The specified outfit does not exist.", stackId);
         }
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
      bool OutfitExists(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         return service.hasOutfit(name.data);
      }
      void OverwriteOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, VMArray<RE::TESObjectARMO*> armors) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOrCreateOutfit(name.data);
            outfit.armors.clear();
            auto count = armors.Length();
            for (UInt32 i = 0; i < count; i++) {
               RE::TESObjectARMO* ptr = nullptr;
               armors.Get(&ptr, i);
               if (ptr)
                  outfit.armors.insert(ptr);
            }
         } catch (ArmorAddonOverrideService::bad_name) {
            registry->LogError("Invalid outfit name specified.", stackId);
            return;
         }
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
   registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, SInt32>(
      "GetOutfitNameMaxLength",
      "SkyrimOutfitSystemNativeFuncs",
      GetOutfitNameMaxLength,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<RE::TESObjectARMO*>, RE::Actor*>(
      "GetWornItems",
      "SkyrimOutfitSystemNativeFuncs",
      GetWornItems,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, RE::Actor*>(
      "RefreshArmorFor",
      "SkyrimOutfitSystemNativeFuncs",
      RefreshArmorFor,
      registry
   ));
   //
   {  // body slot data
      registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
         "PrepOutfitBodySlotListing",
         "SkyrimOutfitSystemNativeFuncs",
         BodySlotListing::Prep,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<RE::TESObjectARMO*>>(
         "GetOutfitBodySlotListingArmorForms",
         "SkyrimOutfitSystemNativeFuncs",
         BodySlotListing::GetArmorForms,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<BSFixedString>>(
         "GetOutfitBodySlotListingArmorNames",
         "SkyrimOutfitSystemNativeFuncs",
         BodySlotListing::GetArmorNames,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<SInt32>>(
         "GetOutfitBodySlotListingSlotIndices",
         "SkyrimOutfitSystemNativeFuncs",
         BodySlotListing::GetSlotIndices,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>(
         "ClearOutfitBodySlotListing",
         "SkyrimOutfitSystemNativeFuncs",
         BodySlotListing::Clear,
         registry
      ));
   }
   //
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, RE::TESObjectARMO*, BSFixedString>(
      "ArmorConflictsWithOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      ArmorConflictsWithOutfit,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
      "CreateOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      CreateOutfit,
      registry
   ));
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
      "DeleteOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      DeleteOutfit,
      registry
   ));
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
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>(
      "OutfitExists",
      "SkyrimOutfitSystemNativeFuncs",
      OutfitExists,
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