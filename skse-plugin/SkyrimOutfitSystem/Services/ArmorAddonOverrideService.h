#pragma once
#include "skse/PluginAPI.h"
#include <set>
#include <unordered_map>
#include <vector>

namespace RE {
   class TESObjectARMO;
}

struct Outfit {
   Outfit(const char* n) : name(n) {};
   Outfit(const char* n, const Outfit& other) : name(n) {
      this->armors = other.armors;
   }
   //
   // NOTE: SKSE caps serialized std::strings and const char*s to 256 bytes.
   //
   // TODO: Cap names to 256 bytes; make names const
   //
   std::string name; // can't be const; prevents assigning Objects
   std::set<RE::TESObjectARMO*> armors;

   bool conflictsWith(RE::TESObjectARMO*) const;
   bool hasShield() const;

   void load(SKSESerializationInterface* intfc, UInt32 version); // can throw ArmorAddonOverrideService::load_error
   void save(SKSESerializationInterface*) const; // can throw ArmorAddonOverrideService::save_error
};
constexpr char* g_noOutfitName = "";
static const Outfit g_noOutfit(g_noOutfitName);

class ArmorAddonOverrideService {
   public:
      typedef Outfit Outfit;
      static constexpr UInt32 signature = 'AAOS';
      enum { kSaveVersion = 1 };
      //
      static constexpr UInt32 ce_outfitNameMaxLength = 256;
      //
      struct load_error : public std::runtime_error {
         explicit load_error(const std::string& what_arg) : runtime_error(what_arg) {};
      };
      struct name_conflict : public std::runtime_error {
         explicit name_conflict(const std::string& what_arg) : runtime_error(what_arg) {};
      };
      struct save_error : public std::runtime_error {
         explicit save_error(const std::string& what_arg) : runtime_error(what_arg) {};
      };
      //
   public:
      bool enabled = false;
      Outfit currentOutfit = g_noOutfit; // ideally this should be const, but unlike pointers, references suck and don't distinguish between "can't modify" and "can't reassign;" you get both or none
      std::unordered_map<std::string, Outfit> outfits;
      //
      static ArmorAddonOverrideService& GetInstance() {
         static ArmorAddonOverrideService instance;
         return instance;
      };
      //
      // TODO: throw exception when taking any action that would create an outfit named g_noOutfit.name
      //
      Outfit& getOutfit(const char* name); // throws std::out_of_range if not found
      Outfit& getOrCreateOutfit(const char* name);
      //
      void addOutfit(const char* name);
      void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors);
      void deleteOutfit(const char* name);
      void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false);
      void renameOutfit(const char* oldName, const char* newName); // throws name_conflict if the new name is already taken
      void setOutfit(const char* name);
      //
      bool shouldOverride() const noexcept;
      void getOutfitNames(std::vector<std::string>& out);
      void setEnabled(bool) noexcept;
      //
      void reset();
      void load(SKSESerializationInterface* intfc, UInt32 version); // can throw load_error
      void save(SKSESerializationInterface* intfc); // can throw save_error
};