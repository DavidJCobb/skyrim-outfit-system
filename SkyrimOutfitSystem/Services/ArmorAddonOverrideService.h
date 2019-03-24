#pragma once
#include <vector>

namespace RE {
   class TESObjectARMO;
}

struct Outfit {
   std::string name;
   std::vector<RE::TESObjectARMO*> armors;

   bool hasShield() const;
};
static Outfit g_noOutfit;

class ArmorAddonOverrideService {
   public:
      typedef Outfit Outfit;
   public:
      bool enabled = false;
      Outfit currentOutfit = g_noOutfit; // ideally this should be const, but unlike pointers, references suck and don't distinguish between "can't modify" and "can't reassign;" you get both or none
      std::vector<Outfit> outfits;
      //
      static ArmorAddonOverrideService& GetInstance() {
         static ArmorAddonOverrideService instance;
         return instance;
      };
      //
      void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors);
      void setOutfit(const char* name);
};