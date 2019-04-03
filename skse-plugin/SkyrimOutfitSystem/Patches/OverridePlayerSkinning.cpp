#include "OverridePlayerSkinning.h"
//
#include "ReverseEngineered/ExtraData.h"
#include "ReverseEngineered/Forms/Actor.h"
#include "ReverseEngineered/Forms/TESObjectREFR.h"
#include "ReverseEngineered/Forms/BaseForms/TESNPC.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "ReverseEngineered/Systems/EquipManager.h"
#include "skse/GameAPI.h"
#include "skse/GameRTTI.h"
#include "skse/SafeWrite.h"
//
#include "Services/ArmorAddonOverrideService.h"

namespace SkyrimOutfitSystem {
   namespace Patches {
      namespace OverridePlayerSkinning {
         bool _stdcall ShouldOverrideSkinning(RE::TESObjectREFR* target) {
            if (!ArmorAddonOverrideService::GetInstance().shouldOverride())
               return false;
            return (target == (RE::TESObjectREFR*) *g_thePlayer);
         }
         const std::set<RE::TESObjectARMO*>& GetOverrideArmors() {
            auto& svc = ArmorAddonOverrideService::GetInstance();
            return svc.currentOutfit().armors;
         }
         //
         // The normal process for equipping armor works as follows:
         //
         //  - Iterate over the player's inventory and total all of the body 
         //    part flags on all worn items. Then, take these flags and hide 
         //    the relevant body parts (i.e. hide hair for helmets).
         //
         //  - Iterate over the player's inventory. For each worn armor, 
         //    call a function to add the armor's ArmorAddons to the player's 
         //    ActorWeightModel::unk00.
         //
         // We need to override both processes in order to override the armor 
         // addons that are used. We do so as follows:
         //
         //  - Override the first process: supply the flags for our outfit.
         //
         //  - Override the second process:
         //
         //     - Tamper with the iterator/functor: don't allow it to act 
         //       on any armors.
         //
         //     - Wrap the caller: after it runs, manually register the armors 
         //       we want to show.
         //
         namespace FixGetters {
            //
            // Several of the game's getters access the ActorWeightData rather 
            // than the inventory.
            //
            namespace Actor_GetDominantArmorSkill {
               class _Visitor : public RE::ExtraContainerChanges::InventoryVisitor {
                  private:
                     virtual BOOL Visit(RE::InventoryEntryData* data) override {
                        auto form = data->type;
                        if (form && form->formType == kFormType_Armor) {
                           auto armor = (RE::TESObjectARMO*) form;
                           //
                           // Mimic processing for the IdentifyDominantArmorTypeVisitor.
                           //
                           if (armor->IsShield()) // shields don't count
                              return true;
                           if (armor->bipedObject.data.weightClass == 2)
                              return true;
                           if (0.0F >= ((float)armor->armorValTimes100 / 100.0F))
                              return true;
                           UInt32 mod = 1;
                           if (armor->bipedObject.data.parts & armor->bipedObject.kPart_Body)
                              mod = 2;
                           SInt32 skill = CALL_MEMBER_FN(armor, GetArmorSkillAVIndex)();
//_MESSAGE("GetDominantArmorType functor: found eligible armor with form ID %08X and skill %d.", armor->formID, skill);
                           if (skill == 0xB) {
                              this->shimmed.foundHeavy += mod;
                           } else if (skill == 0xC) {
                              this->shimmed.foundLight += mod;
                           }
                        }
                        return true;
                     };
                     RE::ActorWeightData::IdentifyDominantArmorTypeVisitor shimmed;
                  public:
                     SInt32 GetResult() { // call after executing the visitor
//_MESSAGE("GetDominantArmorType functor done... heavy: %d; light: %d", this->shimmed.foundHeavy, this->shimmed.foundLight);
                        return CALL_MEMBER_FN(&this->shimmed, GetResultAVIndex)();
                     };
               };
               SInt32 _stdcall Inner(RE::Actor* actor) {
                  auto inventory = RE::GetOrCreateExtraContainerChangesDataFor(actor);
                  if (inventory) {
                     _Visitor visitor;
//_MESSAGE("GetDominantArmorType: running functor...");
                     CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
                     auto result = visitor.GetResult();
//_MESSAGE("GetDominantArmorType: final result should be %d.", result);
                     return result;
                  }
                  _MESSAGE("OverridePlayerSkinning: Actor::GetDominantArmorType shim failed: no inventory!");
                  return -1;
               }
               __declspec(naked) void Outer() {
                  _asm {
                     push ecx; // protect
                     push ecx;
                     call ShouldOverrideSkinning; // stdcall
                     test al, al;
                     pop  ecx; // restore
                     jz   lVanilla;
                     push ecx;
                     call Inner; // stdcall
                     retn;
                  lVanilla:
                     //
                     // reproduce vanilla code:
                     //
                     mov  eax, dword ptr [ecx];
                     mov  edx, dword ptr [eax + 0x1F8];
                     sub  esp, 0x10;
                     push esi;
                     mov  esi, 0x006E1B7C;
                     jmp  esi;
                  }
               }
               void Apply() {
                  WriteRelJump(0x006E1B70, (UInt32)&Outer);
               }
            }
            namespace Actor_GetEquippedShield {
               class _Visitor : public RE::ExtraContainerChanges::InventoryVisitor {
                  private:
                     virtual BOOL Visit(RE::InventoryEntryData* data) override {
                        auto form = data->type;
                        if (form && form->formType == kFormType_Armor) {
                           auto armor = (RE::TESObjectARMO*) form;
                           if (armor->IsShield()) {
                              shield = armor;
                              return false; // halt visitor early
                           }
                        }
                        return true;
                     };
                  public:
                     RE::TESObjectARMO* shield = nullptr;
               };
               RE::TESObjectARMO* _stdcall Inner(RE::Actor* actor) {
                  auto inventory = RE::GetOrCreateExtraContainerChangesDataFor(actor);
                  if (inventory) {
                     _Visitor visitor;
                     CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
                     return visitor.shield;
                  }
                  _MESSAGE("OverridePlayerSkinning: Actor::GetEquippedShield shim failed: no inventory!");
                  return nullptr;
               }
               _declspec(naked) void Outer() {
                  _asm {
                     push ecx; // protect
                     push ecx;
                     call ShouldOverrideSkinning; // stdcall
                     test al, al;
                     pop  ecx; // restore
                     jz   lVanilla;
                     push ecx;
                     call Inner; // stdcall
                     retn;
                  lVanilla:
                     //
                     // reproduce vanilla code:
                     //
                     mov  eax, dword ptr [ecx];
                     mov  edx, dword ptr [eax + 0x1FC];
                     push esi;
                     xor  esi, esi;
                     call edx;
                     mov  ecx, 0x006E1BED;
                     jmp  ecx;
                  }
               }
               void Apply() {
                  WriteRelJump(0x006E1BE0, (UInt32)&Outer);
               }
            }
            //
            void Apply() {
               Actor_GetDominantArmorSkill::Apply();
               Actor_GetEquippedShield::Apply();
            }
         }
         namespace DontVanillaSkinPlayer {
            //
            // Prevent the vanilla code from skinning the player; we'll do it 
            // ourselves.
            //
            // NOTE: This hook also affects shields, so if we don't want to 
            // reskin the shield, then we should have this hook cancel if it's 
            // running on a shield.
            //
            // TODO: There's another call to ApplyArmorAddon in a method we 
            // don't patch: TESNPC::Subroutine00561FF0. When is this other 
            // method called? Do we need to patch it?
            //
            bool _stdcall ShouldOverride(RE::TESObjectREFR* target, RE::TESObjectARMO* armor) {
               if (!ShouldOverrideSkinning(target))
                  return false;
               if (armor->IsShield()) {
                  auto& svc = ArmorAddonOverrideService::GetInstance();
                  auto& outfit = svc.currentOutfit();
                  if (!outfit.hasShield()) {
                     return false;
                  }
               }
               return true;
            }
            _declspec(naked) void Outer() {
               _asm {
                  push ecx; // protect
                  mov  eax, dword ptr [esp + 0x20]; // esp + 0x10, offset by our push and three args
                  push ecx;
                  push eax;
                  call ShouldOverride; // stdcall
                  pop  ecx; // restore
                  test al, al;
                  jnz  lSuppressVanilla;
                  mov  eax, 0x004A21C0; // reproduce patched-over call to TESObjectARMO::ApplyArmorAddon
                  call eax;             // 
                  jmp  lExit;
               lSuppressVanilla:
                  add  esp, 0xC; // remove arguments for the call we've skipped
               lExit:
                  mov  ecx, 0x0056238B;
                  jmp  ecx;
               }
            }
            void Apply() {
               //
               // The function we're patching in the middle of is called by InitWornVisitor::Unk_01.
               //
               WriteRelJump(0x00562386, (UInt32)&Outer);
            }
         }
         namespace ShimWornFlags {
            //
            // To find the patch site, look for the subroutine which creates a 
            // GetWornMaskVisitor object and uses it... and then find that 
            // subroutine's caller.
            //
            class _ShieldVisitor : public RE::ExtraContainerChanges::InventoryVisitor {
               //
               // If the player has a shield equipped, and if we're not overriding that 
               // shield, then we need to grab the equipped shield's worn-flags.
               //
               private:
                  virtual BOOL Visit(RE::InventoryEntryData* data) override {
                     auto form = data->type;
                     if (form && form->formType == kFormType_Armor) {
                        auto armor = (RE::TESObjectARMO*) form;
                        if (armor->IsShield()) {
                           this->mask |= armor->bipedObject.data.parts;
                           this->hasShield = true;
                        }
                     }
                     return true;
                  };
               public:
                  UInt32 mask = 0;
                  bool   hasShield = false;
            };
            //
            UInt32 _stdcall OverrideWornFlags(RE::ExtraContainerChanges::Data* inventory) {
               UInt32 mask = 0;
               //
               auto& svc    = ArmorAddonOverrideService::GetInstance();
               auto& outfit = svc.currentOutfit();
               auto& armors = outfit.armors;
               bool  shield = false;
               if (!outfit.hasShield()) {
                  _ShieldVisitor visitor;
                  CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
                  mask |= visitor.mask;
                  shield = visitor.hasShield;
               }
               for (auto it = armors.cbegin(); it != armors.cend(); ++it) {
                  RE::TESObjectARMO* armor = *it;
                  if (armor) {
                     if (armor->IsShield()) {
                        if (!shield)
                           continue;
                     }
                     mask |= armor->bipedObject.data.parts;
                  }
               }
               //
               return mask;
            }
            __declspec(naked) void Outer() {
               _asm {
                  push ebx;
                  call ShouldOverrideSkinning; // stdcall
                  test al, al;
                  jnz  lSuppressVanilla;
                  mov  eax, 0x004768D0; // reproduce patched-over call to GetBodyPartFlagsOnAllWorn(ExtraContainerChanges::Data*)
                  call eax;             // 
                  jmp  lExit;
               lSuppressVanilla:
                  //
                  // the call we're replacing was non-member, so we shouldn't clean 
                  // up the stack; the caller will do that itself.
                  //
                  mov  eax, dword ptr [esp];
                  push eax;
                  call OverrideWornFlags; // stdcall
               lExit:
                  mov  ecx, 0x00566519;
                  jmp  ecx;
               }
            }
            void Apply() {
               WriteRelJump(0x00566514, (UInt32)&Outer);
            }
         }
         namespace CustomSkinPlayer {
            //
            // Here, we're wrapping the call that iterates over the actor's inventory 
            // to identify all worn items and forward them to the weight model data. 
            // After that call runs, we'll send a few extra items.
            //
            // The process we're patching creates a WornItemVisitor, so look for that 
            // vtbl to find the call we want to wrap.
            //
            class _ShieldVisitor : public RE::ExtraContainerChanges::InventoryVisitor {
               //
               // If the player has a shield equipped, and if we're not overriding that 
               // shield, then we need to grab the equipped shield's worn-flags.
               //
               private:
                  virtual BOOL Visit(RE::InventoryEntryData* data) override {
                     auto form = data->type;
                     if (form && form->formType == kFormType_Armor) {
                        auto armor = (RE::TESObjectARMO*) form;
                        if (armor->IsShield()) {
                           this->result = true;
                           return false; // halt visitor early
                        }
                     }
                     return true;
                  };
               public:
                  bool result = false;
            };
            //
            void _stdcall Custom(RE::Actor* target, void* actorWeightModelData) {
               if (!actorWeightModelData)
                  return;
               auto base = (RE::TESNPC*) DYNAMIC_CAST(target->baseForm, TESForm, TESNPC);
               if (!base)
                  return;
               auto race     = (RE::TESRace*) base->race.race;
               BOOL isFemale = base->actorData.flags & base->kFlag_Female;
               //
               void** toPass = &actorWeightModelData;
               auto& armors = GetOverrideArmors();
               for (auto it = armors.cbegin(); it != armors.cend(); ++it) {
                  RE::TESObjectARMO* armor = *it;
                  if (armor) {
                     if (armor->IsShield()) {
                        //
                        // We should only apply a shield's armor-addons if the player has 
                        // a shield equipped.
                        //
                        auto inventory = RE::GetExtraContainerChangesData(target);
                        if (inventory) {
                           _ShieldVisitor visitor;
                           CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
                           if (!visitor.result)
                              continue;
                        } else {
                           _MESSAGE("OverridePlayerSkinning: Outfit has a shield; unable to check whether the player has a shield equipped.");
                        }
                     }
                     CALL_MEMBER_FN(armor, ApplyArmorAddon)(race, toPass, isFemale);
                  }
               }
            }
            __declspec(naked) void Outer() {
               _asm {
                  mov  eax, 0x00475D50; // reproduce patched-over call
                  call eax;             // 
                  push ebp;
                  call ShouldOverrideSkinning; // stdcall
                  test al, al;
                  jz   lExit;
               lCustom:
                  push esi;
                  push ebp;
                  call Custom; // stdcall
               lExit:
                  mov  eax, 0x0056678E;
                  jmp  eax;
               }
            }
            void Apply() {
               WriteRelJump(0x00566789, (UInt32)&Outer);
            }
         }
         namespace FixEquipConflictCheck {
            //
            // When you try to equip an item, the game loops over the armors in your ActorWeightModel 
            // rather than your other worn items. Because we're tampering with what goes into the AWM, 
            // this means that conflict checks are run against your outfit instead of your equipment, 
            // unless we patch in a fix. (For example, if your outfit doesn't include a helmet, then 
            // you'd be able to stack helmets endlessly without this patch.)
            //
            // The loop in question is performed in Actor::Unk_120, which is also generally responsible 
            // for equipping items at all.
            //
            class _Visitor : public RE::ExtraContainerChanges::InventoryVisitor {
               //
               // Bethesda used a visitor to add armor-addons to the ActorWeightModel in the first 
               // place (see call stack for DontVanillaSkinPlayer patch), so why not use a similar 
               // visitor to check for conflicts?
               //
               private:
                  virtual BOOL Visit(RE::InventoryEntryData* data) override {
                     auto form = data->type;
                     if (form && form->formType == kFormType_Armor) {
                        auto armor = (RE::TESObjectARMO*) form;
                        if (CALL_MEMBER_FN(armor, TestBodyPartByIndex)(this->conflictIndex)) {
                           auto em = RE::EquipManager::GetSingleton();
                           //
                           // TODO: The third argument to this call is meant to be a BaseExtraList*, 
                           // and Bethesda supplies one when calling from Unk_120. Can we get away 
                           // with a nullptr here, or do we have to find the BaseExtraList that 
                           // contains an ExtraWorn?
                           //
                           // I'm not sure how to investigate this, but I did run one test, and that 
                           // works properly: I gave myself ten Falmer Helmets and applied different 
                           // enchantments to two of them (leaving the others unenchanted). In tests, 
                           // I was unable to stack the helmets with each other or with other helmets, 
                           // suggesting that the BaseExtraList may not be strictly necessary.
                           //
                           CALL_MEMBER_FN(em, UnequipItem)(this->target, (RE::TESForm*)form, nullptr, 1, nullptr, false, false, true, false, nullptr);
                        }
                     }
                     return true;
                  };
               public:
                  RE::Actor* target;
                  UInt32     conflictIndex = 0;
            };
            void _stdcall Inner(UInt32 bodyPartForNewItem, RE::Actor* target) {
               auto inventory = RE::GetOrCreateExtraContainerChangesDataFor(target);
               if (inventory) {
                  _Visitor visitor;
                  visitor.conflictIndex = bodyPartForNewItem;
                  visitor.target = target;
                  CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
               } else {
                  _MESSAGE("OverridePlayerSkinning: Conflict check failed: no inventory!");
               }
            }
            bool _stdcall ShouldOverride(RE::TESForm* item) {
               //
               // We only hijack equipping for armors, so I'd like for this patch to only 
               // apply to armors as well. It shouldn't really matter -- before I added 
               // this check, weapons and the like tested in-game with no issues, likely 
               // because they're handled very differently -- but I just wanna be sure. 
               // We should use vanilla code whenever we don't need to NOT use it.
               //
               return (item->formType == kFormType_Armor);
            }
            __declspec(naked) void Outer() {
               _asm {
                  mov  eax, 0x00447C20; // reproduce patched-over call to BGSBipedObjectForm::TestBodyPartByIndex(edi);
                  call eax;
                  test al, al;
                  jz   lExit;
                  mov  eax, dword ptr [esp + 0x3C]; // Arg1
                  push eax;
                  call ShouldOverride; // stdcall
                  test al, al;
                  mov  eax, 1; // let the vanilla check run as originally coded
                  jz   lVanilla;
                  push ebx;
                  push edi;
                  call Inner; // stdcall
               lExit:
                  xor  al, al; // force the vanilla check to always skip, since we're handling this now
               lVanilla:
                  mov  ecx, 0x006B60D8;
                  jmp  ecx;
               }
            }
            void Apply() {
               WriteRelJump(0x006B60D3, (UInt32)&Outer);
            }
         }
         //
         void Apply() {
            DontVanillaSkinPlayer::Apply();
            ShimWornFlags::Apply();
            CustomSkinPlayer::Apply();
            FixEquipConflictCheck::Apply();
            FixGetters::Apply();
         }
      }
   }
}