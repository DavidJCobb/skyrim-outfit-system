#include "ArmorAddonOverrideService.h"
#include "ReverseEngineered/Forms/BaseForms/TESObjectARMO.h"
#include "skse/GameAPI.h"
#include "skse/GameRTTI.h"
#include "skse/Serialization.h"

void _assertWrite(bool result, const char* err) {
   if (!result)
      throw ArmorAddonOverrideService::save_error(err);
}
void _assertRead(bool result, const char* err) {
   if (!result)
      throw ArmorAddonOverrideService::load_error(err);
}

bool Outfit::conflictsWith(RE::TESObjectARMO* test) const {
   if (!test)
      return false;
   auto mask = test->bipedObject.data.parts;
   for (auto it = this->armors.cbegin(); it != this->armors.cend(); ++it) {
      RE::TESObjectARMO* armor = *it;
      if (armor)
         if (mask & armor->bipedObject.data.parts)
            return true;
   }
   return false;
}
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
//
void Outfit::load(SKSESerializationInterface* intfc, UInt32 version) {
   using namespace Serialization;
   //
   UInt32 size = 0;
   _assertRead(ReadData(intfc, &size), "Failed to read an outfit's armor count.");
   for (UInt32 i = 0; i < size; i++) {
      UInt32 formID = 0;
      _assertRead(ReadData(intfc, &formID), "Failed to read an outfit's armor.");
      UInt32 fixedID;
      if (intfc->ResolveFormId(formID, &fixedID)) {
         auto armor = (RE::TESObjectARMO*) DYNAMIC_CAST(LookupFormByID(fixedID), TESForm, TESObjectARMO);
         if (armor)
            this->armors.insert(armor);
      }
   }
}
void Outfit::save(SKSESerializationInterface* intfc) const {
   using namespace Serialization;
   //
   UInt32 size = this->armors.size();
   _assertWrite(WriteData(intfc, &size), "Failed to write the outfit's armor count.");
   for (auto it = this->armors.cbegin(); it != this->armors.cend(); ++it) {
      UInt32 formID = 0;
      auto   armor  = *it;
      if (armor)
         formID = armor->formID;
      _assertWrite(WriteData(intfc, &formID), "Failed to write an outfit's armors.");
   }
}

Outfit& ArmorAddonOverrideService::getOutfit(const char* name) {
   return this->outfits.at(name);
}
Outfit& ArmorAddonOverrideService::getOrCreateOutfit(const char* name) {
   if (name == g_noOutfitName)
      throw bad_name("You cannot create an outfit with a blank name.");
   //
   // TODO: Cap the outfit name length, or modify the serialization code so that there is no length limit.
   //
   return this->outfits.emplace(name, name).first->second;
}

void ArmorAddonOverrideService::addOutfit(const char* name) {
   if (name == g_noOutfitName)
      throw bad_name("You cannot create an outfit with a blank name.");
   //
   // TODO: Cap the outfit name length, or modify the serialization code so that there is no length limit.
   //
   this->outfits.emplace(name, name);
}
void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
   if (name == g_noOutfitName)
      throw bad_name("You cannot create an outfit with a blank name.");
   //
   // TODO: Cap the outfit name length, or modify the serialization code so that there is no length limit.
   //
   auto& created = this->outfits.emplace(name, name).first->second;
   for (auto it = armors.begin(); it != armors.end(); ++it) {
      auto armor = *it;
      if (armor)
         created.armors.insert(armor);
   }
}
bool ArmorAddonOverrideService::hasOutfit(const char* name) const {
   try {
      this->outfits.at(name);
      return true;
   } catch (std::out_of_range) {
      return false;
   }
}
void ArmorAddonOverrideService::deleteOutfit(const char* name) {
   this->outfits.erase(name);
   if (this->currentOutfit.name == name)
      this->currentOutfit = g_noOutfit;
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
void ArmorAddonOverrideService::renameOutfit(const char* oldName, const char* newName) {
   try {
      this->outfits.at(newName);
      throw name_conflict("");
   } catch (std::out_of_range) {
      try {
         auto old = this->outfits.at(oldName);
         this->outfits.emplace(newName, old);
         this->outfits.erase(oldName);
      } catch (std::out_of_range) {}
   }
}
void ArmorAddonOverrideService::setOutfit(const char* name) {
   if (strcmp(name, g_noOutfitName) == 0) {
      this->currentOutfit = g_noOutfit;
      return;
   }
   try {
      Outfit& target = this->getOutfit(name);
      this->currentOutfit = target;
   } catch (std::out_of_range) {
      _MESSAGE("ArmorAddonOverrideService: Tried to set non-existent outfit %s as active. Switching the system off for now.", name);
      this->currentOutfit = g_noOutfit; // no outfit
   }
}
bool ArmorAddonOverrideService::shouldOverride() const noexcept {
   if (!this->enabled)
      return false;
   if (this->currentOutfit.name == g_noOutfitName)
      return false;
   return true;
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out) {
   out.clear();
   auto& list = this->outfits;
   out.reserve(list.size());
   for (auto it = list.begin(); it != list.end(); ++it)
      out.push_back(it->second.name);
}
void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
   this->enabled = flag;
}
//
void ArmorAddonOverrideService::reset() {
   _MESSAGE("ArmorAddonOverrideService::reset();");
   this->enabled = false;
   this->currentOutfit = g_noOutfit;
   this->outfits.clear();
}
void ArmorAddonOverrideService::load(SKSESerializationInterface* intfc, UInt32 version) {
   using namespace Serialization;
   //
   this->reset();
   //
   std::string selectedOutfitName;
   _assertWrite(ReadData(intfc, &this->enabled),      "Failed to read the enable state.");
   _assertWrite(ReadData(intfc, &selectedOutfitName), "Failed to read the selected outfit name.");
   UInt32 size;
   _assertRead(ReadData(intfc, &size), "Failed to read the outfit count.");
   for (UInt32 i = 0; i < size; i++) {
      std::string name;
      _assertRead(ReadData(intfc, &name), "Failed to read an outfit's name.");
      auto& outfit = this->getOrCreateOutfit(name.c_str());
      outfit.load(intfc, version);
   }
   this->setOutfit(selectedOutfitName.c_str());
}
void ArmorAddonOverrideService::save(SKSESerializationInterface* intfc) {
   using namespace Serialization;
   //
   _assertWrite(WriteData(intfc, &this->enabled), "Failed to write the enable state.");
   {  // current outfit
      auto& outfit = this->currentOutfit;
      _assertWrite(WriteData(intfc, &outfit.name), "Failed to write the selected outfit name.");
   }
   UInt32 size = this->outfits.size();
   _assertWrite(WriteData(intfc, &size), "Failed to write the outfit count.");
   for (auto it = this->outfits.cbegin(); it != this->outfits.cend(); ++it) {
      auto& name = it->first;
      _assertWrite(WriteData(intfc, &name), "Failed to write an outfit's name.");
      //
      auto& outfit = it->second;
      outfit.save(intfc);
   }
}