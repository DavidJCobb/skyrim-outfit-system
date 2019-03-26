Scriptname SkyrimOutfitSystemNativeFuncs Hidden

Int Function GetOutfitNameMaxLength() Global Native

Armor[] Function GetWornItems    (Actor akSubject) Global Native
        Function RefreshArmorFor (Actor akSubject) Global Native

         Function PrepOutfitBodySlotListing           (String asOutfitName) Global Native
Armor[]  Function GetOutfitBodySlotListingArmorForms  () Global Native
String[] Function GetOutfitBodySlotListingArmorNames  () Global Native
Int[]    Function GetOutfitBodySlotListingSlotIndices () Global Native
         Function ClearOutfitBodySlotListing          () Global Native

         Function AddArmorToOutfit  (String asOutfitName, Armor akArmor) Global Native
Bool     Function ArmorConflictsWithOutfit (Armor akTest, String asOutfitName) Global Native
         Function CreateOutfit      (String asOutfitName) Global Native
         Function DeleteOutfit      (String asOutfitName) Global Native
Armor[]  Function GetOutfitContents (String asOutfitName) Global Native
String   Function GetSelectedOutfit () Global Native
Bool     Function IsEnabled         () Global Native
String[] Function ListOutfits       () Global Native
         Function RemoveConflictingArmorsFrom (Armor akTest, String asOutfitName) Global Native
Bool     Function RenameOutfit      (String asOutfitName, String asRenameTo) Global Native
Bool     Function OutfitExists      (String asOutfitName) Global Native
         Function OverwriteOutfit   (String asOutfitName, Armor[] akArmors) Global Native
         Function SetEnabled        (Bool abEnabled) Global Native
         Function SetSelectedOutfit (String asOutfitName) Global Native