#pragma once
#include "skse/PluginAPI.h"
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Miscellaneous/strings.h"

namespace RE {
   class TESObjectARMO;
}

struct Outfit {
   Outfit() {}; // we shouldn't need this, really, but std::unordered_map is a brat
   Outfit(const char* n) : name(n) {};
   Outfit(const Outfit& other) = default;
   Outfit(const char* n, const Outfit& other) : name(n) {
      this->armors = other.armors;
   }
   std::string name; // can't be const; prevents assigning to Outfit vars
   std::set<RE::TESObjectARMO*> armors;

   bool conflictsWith(RE::TESObjectARMO*) const;
   bool hasShield() const;

   void load(SKSESerializationInterface* intfc, UInt32 version); // can throw ArmorAddonOverrideService::load_error
   void save(SKSESerializationInterface*) const; // can throw ArmorAddonOverrideService::save_error
};
constexpr char* g_noOutfitName = "";
static Outfit g_noOutfit(g_noOutfitName); // can't be const; prevents us from assigning it to Outfit&s

class ArmorAddonOverrideService {
   public:
      typedef Outfit Outfit;
      static constexpr UInt32 signature = 'AAOS';
      enum { kSaveVersion = 1 };
      //
      static constexpr UInt32 ce_outfitNameMaxLength = 256; // SKSE caps serialized std::strings and const char*s to 256 bytes.
      //
      static void _validateNameOrThrow(const char* outfitName);
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
   private:
      struct OutfitReferenceByName : public cobb::istring {
         OutfitReferenceByName(const value_type* s)       : cobb::istring(s) {};
         OutfitReferenceByName(const basic_string& other) : cobb::istring(other) {};
         //
         operator Outfit&() {
            return ArmorAddonOverrideService::GetInstance().outfits.at(*this);
         }
      };
   public:
      bool enabled = true;
      cobb::istring currentOutfitName = g_noOutfitName;
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
      Outfit& currentOutfit();
      bool hasOutfit(const char* name) const;
      void deleteOutfit(const char* name);
      void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false); // can throw bad_name if (createIfMissing)
      void renameOutfit(const char* oldName, const char* newName); // throws name_conflict if the new name is already taken; can throw bad_name; throws std::out_of_range if the oldName doesn't exist
      void setOutfit(const char* name);
      //
      bool shouldOverride() const noexcept;
      void getOutfitNames(std::vector<std::string>& out) const;
      void setEnabled(bool) noexcept;
      //
      void refreshCurrentIfChanged(const char* testName);
      //
      void reset();
      void load(SKSESerializationInterface* intfc, UInt32 version); // can throw load_error
      void save(SKSESerializationInterface* intfc); // can throw save_error
      //
      void dump() const;
};