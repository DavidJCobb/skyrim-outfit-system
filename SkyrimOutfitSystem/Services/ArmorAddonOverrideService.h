#pragma once
#include <set>
#include <unordered_map>
#include <vector>

namespace RE {
   class TESObjectARMO;
}

struct Outfit {
   std::string name;
   std::set<RE::TESObjectARMO*> armors;

   bool hasShield() const;
};
static Outfit g_noOutfit;

class ArmorAddonOverrideService {
   public:
      typedef Outfit Outfit;
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
      Outfit& getOutfit(const char* name); // throws std::out_of_range if not found
      //
      void addOutfit(const char* name);
      void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors);
      void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false);
      void setOutfit(const char* name);
      //
      void getOutfitNames(std::vector<std::string>& out);
      void setEnabled(bool) noexcept;
};