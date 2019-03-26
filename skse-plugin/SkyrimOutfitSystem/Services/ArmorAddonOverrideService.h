#pragma once
#include "skse/PluginAPI.h"
#include <set>
#include <unordered_map>
#include <vector>

#include "Miscellaneous/strings.h"

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
   // TODO: Cap names to 256 bytes
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
      struct bad_name : public std::runtime_error {
         explicit bad_name(const std::string& what_arg) : runtime_error(what_arg) {};
      };
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
      std::unordered_map<cobb::istring, Outfit> outfits;
      //
      static ArmorAddonOverrideService& GetInstance() {
         static ArmorAddonOverrideService instance;
         return instance;
      };
      //
      Outfit& getOutfit(const char* name); // throws std::out_of_range if not found
      Outfit& getOrCreateOutfit(const char* name); // can throw bad_name
      //
      void addOutfit(const char* name); // can throw bad_name
      void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors); // can throw bad_name
      bool hasOutfit(const char* name) const;
      void deleteOutfit(const char* name);
      void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false); // can throw bad_name if (createIfMissing)
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