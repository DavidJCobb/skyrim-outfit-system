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
               // NOTE: This hook also affects shields, so if we don't want to 
               // reskin the shield, then we should have this hook cancel if it's 
               // running on a shield.
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
            namespace CustomSkinPlayer {
               //
               // Here, we're wrapping the call that iterates over the actor's inventory 
               // to identify all worn items and forward them to the weight model data. 
               // After that call runs, we'll send a few extra items.
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