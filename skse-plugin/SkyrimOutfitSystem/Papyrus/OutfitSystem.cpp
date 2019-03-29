#include "OutfitSystem.h"

#include "skse/PapyrusNativeFunctions.h"
#include "skse/PapyrusObjects.h"
#include "skse/PapyrusVM.h"

#include "skse/GameRTTI.h"

#include "ReverseEngineered/Systems/GameData.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "ReverseEngineered/Forms/Actor.h"
#include "Services/ArmorAddonOverrideService.h"

#include "Miscellaneous/strings.h"
#include "Miscellaneous/utf8string.h"
#include "Miscellaneous/utf8naturalsort.h"

#include <algorithm>

namespace CobbPapyrus {
   namespace OutfitSystem {
      SInt32 GetOutfitNameMaxLength(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
         return ArmorAddonOverrideService::ce_outfitNameMaxLength;
      }
      VMResultArray<RE::TESObjectARMO*> GetCarriedArmor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::Actor* target) {
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
            CALL_MEMBER_FN(inventory, ExecuteVisitor)(&visitor);
         }
         return result;
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
            //
            // "SetEquipFlag" tells the process manager that the actor's 
            // equipment has changed, and that their ArmorAddons should 
            // be updated. If you need to find it in Skyrim Special, you 
            // should see a call near the start of EquipManager's func-
            // tion to equip an item.
            //
            CALL_MEMBER_FN(pm, SetEquipFlag)(true);
            CALL_MEMBER_FN(pm, UpdateEquipment)(target);
         }
      }
      //
      namespace ArmorFormSearchUtils {
         static struct {
            std::vector<std::string>    names;
            std::vector<TESObjectARMO*> armors;
            //
            void setup(std::string nameFilter, bool mustBePlayable) {
               auto  data = RE::TESDataHandler::GetSingleton();
               auto& list = data->armors;
               auto  size = list.count;
               this->names.reserve(size);
               this->armors.reserve(size);
               for (UInt32 i = 0; i < size; i++) {
                  auto armor = list[i];
                  if (armor && armor->formType == kFormType_Armor) {
                     if (armor->templateArmor) // filter out predefined enchanted variants, to declutter the list
                        continue;
                     if (mustBePlayable && !!(armor->flags & 4))
                        continue;
                     std::string armorName;
                     {  // get name
                        TESFullName* tfn = DYNAMIC_CAST(armor, TESObjectARMO, TESFullName);
                        if (tfn)
                           armorName = tfn->name.data;
                     }
                     if (armorName.empty()) // skip nameless armor
                        continue;
                     if (!nameFilter.empty()) {
                        auto it = std::search(
                           armorName.begin(),  armorName.end(),
                           nameFilter.begin(), nameFilter.end(),
                           [](char a, char b) { return toupper(a) == toupper(b); }
                        );
                        if (it == armorName.end())
                           continue;
                     }
                     this->armors.push_back(armor);
                     this->names.push_back(armorName.c_str());
                  }
               }
            }
            void clear() {
               this->names.clear();
               this->armors.clear();
            }
         } data;
         //
         //
         void Prep(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString filter, bool mustBePlayable) {
            data.setup(filter.data, mustBePlayable);
         }
         VMResultArray<TESObjectARMO*> GetForms(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
               result.push_back(*it);
            return result;
         }
         VMResultArray<BSFixedString> GetNames(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<BSFixedString> result;
            auto& list = data.names;
            for (auto it = list.begin(); it != list.end(); it++)
               result.push_back(it->c_str());
            return result;
         }
         void Clear(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            data.clear();
         }
      }
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
      namespace StringSorts {
         VMResultArray<BSFixedString> NaturalSort_ASCII(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, VMArray<BSFixedString> arr, bool descending) {
            VMResultArray<BSFixedString> result;
            {  // Copy input array into output array
               UInt32 size = arr.Length();
               result.reserve(size);
               for (UInt32 i = 0; i < size; i++) {
                  BSFixedString x;
                  arr.Get(&x, i);
                  result.push_back(x);
               }
            }
            std::sort(
               result.begin(),
               result.end(),
               [descending](const BSFixedString& x, const BSFixedString& y) {
                  //cobb::lowerstring a(x.data); // TODO: lowerstring is busted and doesn't actually change case on insert
                  //cobb::lowerstring b(y.data);
                  std::string a(x.data);
                  std::string b(y.data);
                  std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) -> unsigned char { return tolower(c); });
                  std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) -> unsigned char { return tolower(c); });
                  if (descending)
                     std::swap(a, b);
                  return cobb::utf8_naturalcompare(a, b) > 0;
               }
            );
            return result;
         }
         template<typename T> VMResultArray<BSFixedString> NaturalSortPair_ASCII(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, VMArray<BSFixedString> arr, VMArray<T> second, bool descending) {
            UInt32 size = arr.Length();
            if (size != second.Length()) {
               registry->LogError("The two arrays must be the same length.", stackId);
               //
               VMResultArray<BSFixedString> result;
               result.reserve(size);
               for (UInt32 i = 0; i < size; i++) {
                  BSFixedString x;
                  arr.Get(&x, i);
                  result.push_back(x);
               }
               return result;
            }
            //
            typedef std::pair<BSFixedString, T> _pair;
            std::vector<_pair> pairs;
            //
            VMResultArray<BSFixedString> result;
            {  // Copy input array into output array
               result.reserve(size);
               for (UInt32 i = 0; i < size; i++) {
                  BSFixedString x;
                  T y;
                  arr.Get(&x, i);
                  second.Get(&y, i);
                  pairs.emplace_back(x, y);
               }
            }
            std::sort(
               pairs.begin(),
               pairs.end(),
               [descending](const _pair& x, const _pair& y) {
                  auto result = cobb::utf8_naturalcompare(cobb::lowerstring(x.first.data), cobb::lowerstring(y.first.data));
                  if (descending)
                     result = -result;
                  return result > 0;
               }
            );
            for (UInt32 i = 0; i < size; i++) {
               result.push_back(pairs[i].first);
               second.Set(&pairs[i].second, i);
            }
            return result;
         }
      }
      namespace Utility {
         UInt32 HexToInt32(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString str) {
            const char* s = str.data;
            char* discard;
            return strtoul(s, &discard, 16);
         }
         BSFixedString ToHex(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, UInt32 value, SInt32 length) {
            if (length < 1) {
               registry->LogWarning("Cannot format a hexadecimal valueinteger to a negative number of digits. Defaulting to eight.", stackId);
               length = 8;
            } else if (length > 8) {
               registry->LogWarning("Cannot format a hexadecimal integer longer than eight digits.", stackId);
               length = 8;
            }
            char hex[9];
            memset(hex, '0', sizeof(hex));
            hex[length] = '\0';
            while (value > 0 && length--) {
               UInt8 digit = value % 0x10;
               value /= 0x10;
               if (digit < 0xA) {
                  hex[length] = digit + '0';
               } else {
                  hex[length] = digit + 0x37;
               }
            }
            return hex; // passes through BSFixedString constructor, which I believe caches the string, so returning local vars should be fine
         }
      }
      //
      void AddArmorToOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, RE::TESObjectARMO* armor) {
         ERROR_AND_RETURN_IF(armor == nullptr, "Cannot add a None armor to an outfit.", registry, stackId);
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOutfit(name.data);
            outfit.armors.insert(armor);
         } catch (std::out_of_range) {
            registry->LogWarning("The specified outfit does not exist.", stackId);
         }
      }
      bool ArmorConflictsWithOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::TESObjectARMO* armor, BSFixedString name) {
         if (armor == nullptr) {
            registry->LogWarning("A None armor can't conflict with anything in an outfit.", stackId);
            return false;
         }
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
         return service.currentOutfit().name.c_str();
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
      void RemoveArmorFromOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, RE::TESObjectARMO* armor) {
         ERROR_AND_RETURN_IF(armor == nullptr, "Cannot remove a None armor from an outfit.", registry, stackId);
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOutfit(name.data);
            outfit.armors.erase(armor);
         } catch (std::out_of_range) {
            registry->LogWarning("The specified outfit does not exist.", stackId);
         }
      }
      void RemoveConflictingArmorsFrom(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, RE::TESObjectARMO* armor, BSFixedString name) {
         ERROR_AND_RETURN_IF(armor == nullptr, "A None armor can't conflict with anything in an outfit.", registry, stackId);
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            auto& outfit = service.getOutfit(name.data);
            auto& armors = outfit.armors;
            std::vector<RE::TESObjectARMO*> conflicts;
            UInt32 candidateMask = armor->bipedObject.data.parts;
            for (auto it = armors.begin(); it != armors.end(); ++it) {
               RE::TESObjectARMO* existing = *it;
               if (existing) {
                  UInt32 mask = existing->bipedObject.data.parts;
                  if (mask & candidateMask)
                     conflicts.push_back(existing);
               }
            }
            for (auto it = conflicts.begin(); it != conflicts.end(); ++it)
               armors.erase(*it);
         } catch (std::out_of_range) {
            registry->LogError("The specified outfit does not exist.", stackId);
            return;
         }
      }
      bool RenameOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, BSFixedString changeTo) {
         auto& service = ArmorAddonOverrideService::GetInstance();
         try {
            service.renameOutfit(name.data, changeTo.data);
         } catch (ArmorAddonOverrideService::bad_name) {
            registry->LogError("The desired name is invalid.", stackId);
            return false;
         } catch (ArmorAddonOverrideService::name_conflict) {
            registry->LogError("The desired name is taken.", stackId);
            return false;
         } catch (std::out_of_range) {
            registry->LogError("The specified outfit does not exist.", stackId);
            return false;
         }
         return true;
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
   registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "GetOutfitNameMaxLength", VMClassRegistry::kFunctionFlag_NoWait);
   registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<RE::TESObjectARMO*>, RE::Actor*>(
      "GetCarriedArmor",
      "SkyrimOutfitSystemNativeFuncs",
      GetCarriedArmor,
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
   {  // armor form search utils
      registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>(
         "PrepArmorSearch",
         "SkyrimOutfitSystemNativeFuncs",
         ArmorFormSearchUtils::Prep,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<TESObjectARMO*>>(
         "GetArmorSearchResultForms",
         "SkyrimOutfitSystemNativeFuncs",
         ArmorFormSearchUtils::GetForms,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<BSFixedString>>(
         "GetArmorSearchResultNames",
         "SkyrimOutfitSystemNativeFuncs",
         ArmorFormSearchUtils::GetNames,
         registry
      ));
      registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>(
         "ClearArmorSearch",
         "SkyrimOutfitSystemNativeFuncs",
         ArmorFormSearchUtils::Clear,
         registry
      ));
   }
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
   {  // string sorts
      registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMResultArray<BSFixedString>, VMArray<BSFixedString>, bool>(
         "NaturalSort_ASCII",
         "SkyrimOutfitSystemNativeFuncs",
         StringSorts::NaturalSort_ASCII,
         registry
      ));
      registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "NaturalSort_ASCII", VMClassRegistry::kFunctionFlag_NoWait);
      registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, VMResultArray<BSFixedString>, VMArray<BSFixedString>, VMArray<RE::TESObjectARMO*>, bool>(
         "NaturalSortPairArmor_ASCII",
         "SkyrimOutfitSystemNativeFuncs",
         StringSorts::NaturalSortPair_ASCII<RE::TESObjectARMO*>,
         registry
      ));
      registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "NaturalSortPairArmor_ASCII", VMClassRegistry::kFunctionFlag_NoWait);
   }
   {  // Utility
      registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, BSFixedString>(
         "HexToInt32",
         "SkyrimOutfitSystemNativeFuncs",
         Utility::HexToInt32,
         registry
      ));
      registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "HexToInt32", VMClassRegistry::kFunctionFlag_NoWait);
      registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, BSFixedString, UInt32, SInt32>(
         "ToHex",
         "SkyrimOutfitSystemNativeFuncs",
         Utility::ToHex,
         registry
      ));
      registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "ToHex", VMClassRegistry::kFunctionFlag_NoWait);
   }
   //
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, RE::TESObjectARMO*>(
      "AddArmorToOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      AddArmorToOutfit,
      registry
   ));
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
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, RE::TESObjectARMO*>(
      "RemoveArmorFromOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      RemoveArmorFromOutfit,
      registry
   ));
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, RE::TESObjectARMO*, BSFixedString>(
      "RemoveConflictingArmorsFrom",
      "SkyrimOutfitSystemNativeFuncs",
      RemoveConflictingArmorsFrom,
      registry
   ));
   registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, BSFixedString, BSFixedString>(
      "RenameOutfit",
      "SkyrimOutfitSystemNativeFuncs",
      RenameOutfit,
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