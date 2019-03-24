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

Outfit& ArmorAddonOverrideService::getOutfit(const char* name) {
   return this->outfits.at(name);
}

void ArmorAddonOverrideService::addOutfit(const char* name) {
   auto created = Outfit();
   created.name = name;
   this->outfits[name] = created;
}
void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
   auto created = Outfit();
   created.name = name;
   for (auto it = armors.begin(); it != armors.end(); ++it) {
      auto armor = *it;
      if (armor)
         created.armors.insert(armor);
   }
   this->outfits[name] = created;
}
void ArmorAddonOverrideService::modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing) {
   try {
      Outfit& target = this->getOutfit(name);
      for (auto it = add.begin(); it != add.end(); ++it) {
         auto armor = *it;
         if (armor)
            target.armors.insert(armor);
      }
      for (auto it = remove.begin(); it != remove.end(); ++it) {
         auto armor = *it;
         if (armor)
            target.armors.erase(armor);
      }
   } catch (std::out_of_range) {
      if (createIfMissing) {
         this->addOutfit(name);
         this->modifyOutfit(name, add, remove);
      }
   }
}
void ArmorAddonOverrideService::setOutfit(const char* name) {
   try {
      Outfit& target = this->getOutfit(name);
      this->currentOutfit = target;
   } catch (std::out_of_range) {
      this->currentOutfit = g_noOutfit; // no outfit
   }
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out) {
   out.clear();
   auto& list = this->outfits;
   out.reserve(list.size());
   for (auto it = list.begin(); it != list.end(); ++it)
      out.push_back(it->first);
}
void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
   this->enabled = flag;
}