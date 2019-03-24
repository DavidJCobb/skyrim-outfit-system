#include "ArmorAddonOverrideService.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"

bool Outfit::hasShield() const {
   auto& list = this->armors;
   for (auto it = list.cbegin(); it != list.cend(); ++it) {
      RE::TESObjectARMO* armor = *it;
      if (armor) {
         if (armor->IsShield())
            return true;
      }
   }
   return false;
};

void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
   auto created = Outfit();
   created.name   = name;
   created.armors = armors;
   this->outfits.push_back(created);
}
void ArmorAddonOverrideService::setOutfit(const char* name) {
   for (auto it = this->outfits.cbegin(); it != this->outfits.cend(); ++it) {
      if (it->name == name) {
         this->currentOutfit = *it;
         return;
      }
   }
   this->currentOutfit = g_noOutfit; // no outfit
};