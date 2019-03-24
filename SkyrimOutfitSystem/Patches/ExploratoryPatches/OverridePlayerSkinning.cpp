#include "OverridePlayerSkinning.h"
//
#include "ReverseEngineered/Forms/Actor.h"
#include "ReverseEngineered/Forms/TESObjectREFR.h"
#include "ReverseEngineered/Forms/BaseForms/TESNPC.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "skse/GameAPI.h"
#include "skse/GameRTTI.h"
#include "skse/SafeWrite.h"

namespace SkyrimOutfitSystem {
   namespace Patches {
      namespace Exploratory {
         bool _stdcall ShouldOverrideSkinning(RE::TESObjectREFR* target) {
            return (target == (RE::TESObjectREFR*) *g_thePlayer);
         }
         //
         namespace OverridePlayerSkinning {
            namespace DontVanillaSkinPlayer {
               //
               // Prevent the vanilla code from skinning the player; we'll do it 
               // ourselves.
               //
               // BUG: When using Bethesda's default test character, his hair is 
               // missing -- probably because some other part of the armor code 
               // hides it in anticipation of the helmet armor-addon being shown.
               //
               //  - Yep. I think Actor::Unk_120 contains the code that shows or 
               //    hides body parts based on the body part flags on the armor.
               //
               // NOTE: This hook also affects shields, so if we don't want to 
               // reskin the shield, then we should have this hook cancel if it's 
               // running on a shield.
               //
               // TODO: There's another call to ApplyArmorAddon in a method we 
               // don't patch: TESNPC::Subroutine00561FF0. When is this other 
               // method called? Do we need to patch it?
               //
               _declspec(naked) void Outer() {
                  _asm {
                     push ecx; // protect
                     mov  eax, dword ptr [esp + 0x20]; // esp + 0x10, offset by our push and three args
                     push eax;
                     call ShouldOverrideSkinning; // stdcall
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
               UInt32 _stdcall OverrideWornFlags() {
                  UInt32 mask = 0;
                  //
                  auto greybeardRobes = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A44), TESForm, TESObjectARMO);
                  if (greybeardRobes)
                     mask |= greybeardRobes->bipedObject.data.parts;
                  //
                  auto greybeardBoots = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A46), TESForm, TESObjectARMO);
                  if (greybeardBoots)
                     mask |= greybeardBoots->bipedObject.data.parts;
                  //
                  return mask;
               }
               __declspec(naked) void Outer() {
                  _asm {
                     push ebx;
                     call ShouldOverrideSkinning;
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
                     call OverrideWornFlags;
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
                  auto greybeardRobes = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A44), TESForm, TESObjectARMO);
                  auto greybeardBoots = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(0x00036A46), TESForm, TESObjectARMO);
                  if (!greybeardRobes || !greybeardBoots)
                     return;
                  void** toPass = &actorWeightModelData;
                  CALL_MEMBER_FN(greybeardRobes, ApplyArmorAddon)(race, toPass, isFemale);
                  CALL_MEMBER_FN(greybeardBoots, ApplyArmorAddon)(race, toPass, isFemale);
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
               CustomSkinPlayer::Apply();
            }
         }
      }
   }
}