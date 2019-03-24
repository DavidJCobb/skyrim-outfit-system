#include "OverridePlayerSkinning.h"
//
#include "ReverseEngineered/ExtraData.h"
#include "ReverseEngineered/Forms/Actor.h"
#include "ReverseEngineered/Forms/TESObjectREFR.h"
#include "ReverseEngineered/Forms/BaseForms/TESNPC.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "skse/GameAPI.h"
#include "skse/GameRTTI.h"
#include "skse/SafeWrite.h"
//
#include "Services/ArmorAddonOverrideService.h"

namespace SkyrimOutfitSystem {
   namespace Patches {
      namespace OverridePlayerSkinning {
         bool _stdcall ShouldOverrideSkinning(RE::TESObjectREFR* target) {
            if (!ArmorAddonOverrideService::GetInstance().enabled)
               return false;
            return (target == (RE::TESObjectREFR*) *g_thePlayer);
         }
         const std::set<RE::TESObjectARMO*>& GetOverrideArmors() {
            auto& svc = ArmorAddonOverrideService::GetInstance();
            return svc.currentOutfit.armors;
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
                  auto& outfit = svc.currentOutfit;
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
               private:
                  virtual BOOL Visit(RE::InventoryEntryData* data) override {
                     auto form = data->type;
                     if (form && form->formType == kFormType_Armor) {
                        auto armor = (RE::TESObjectARMO*) form;
                        if (armor->IsShield())
                           this->mask |= armor->bipedObject.data.parts;
                     }
                     return true;
                  };
               public:
                  UInt32 mask = 0;
            };
            //
            UInt32 _stdcall OverrideWornFlags(RE::ExtraContainerChanges::Data* inventory) {
               UInt32 mask = 0;
               //
               auto& svc    = ArmorAddonOverrideService::GetInstance();
               auto& outfit = svc.currentOutfit;
               auto& armors = outfit.armors;
               for (auto it = armors.cbegin(); it != armors.cend(); ++it) {
                  RE::TESObjectARMO* armor = *it;
                  if (armor)
                     mask |= armor->bipedObject.data.parts;
               }
               if (!outfit.hasShield()) {
                  _ShieldVisitor visitor;
                  CALL_MEMBER_FN(inventory, ExecuteVisitorOnWorn)(&visitor);
                  mask |= visitor.mask;
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
                  if (armor)
                     CALL_MEMBER_FN(armor, ApplyArmorAddon)(race, toPass, isFemale);
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
         //
         void Apply() {
            DontVanillaSkinPlayer::Apply();
            ShimWornFlags::Apply();
            CustomSkinPlayer::Apply();
         }
      }
   }
}