Scriptname SkyrimOutfitSystemNativeFuncs Hidden

Int Function GetOutfitNameMaxLength() Global Native

Armor[] Function GetCarriedArmor (Actor akSubject) Global Native
Armor[] Function GetWornItems    (Actor akSubject) Global Native
        Function RefreshArmorFor (Actor akSubject) Global Native

         Function PrepArmorSearch           (String asNameFilter = "", Bool abMustBePlayable = True) Global Native
Armor[]  Function GetArmorSearchResultForms () Global Native
String[] Function GetArmorSearchResultNames () Global Native
         Function ClearArmorSearch          () Global Native

         Function PrepOutfitBodySlotListing           (String asOutfitName) Global Native
Armor[]  Function GetOutfitBodySlotListingArmorForms  () Global Native
String[] Function GetOutfitBodySlotListingArmorNames  () Global Native
Int[]    Function GetOutfitBodySlotListingSlotIndices () Global Native
         Function ClearOutfitBodySlotListing          () Global Native

;
; String functions copied from CobbAPI:
;
String[] Function NaturalSort_ASCII          (String[] asStrings, Bool abDescending = False) Global Native
String[] Function NaturalSortPairArmor_ASCII (String[] asStrings, Armor[] akForms, Bool abDescending = False) Global Native ; modifies akForms; returns sorted copy of asStrings
;
Int      Function HexToInt32 (String asHex) Global Native
String   Function ToHex      (Int aiValue, Int aiDigits) Global Native

         Function AddArmorToOutfit  (String asOutfitName, Armor akArmor) Global Native
Bool     Function ArmorConflictsWithOutfit (Armor akTest, String asOutfitName) Global Native
         Function CreateOutfit      (String asOutfitName) Global Native
         Function DeleteOutfit      (String asOutfitName) Global Native
Armor[]  Function GetOutfitContents (String asOutfitName) Global Native
String   Function GetSelectedOutfit () Global Native
Bool     Function IsEnabled         () Global Native
String[] Function ListOutfits       () Global Native
         Function RemoveArmorFromOutfit (String asOutfitName, Armor akArmor) Global Native
         Function RemoveConflictingArmorsFrom (Armor akTest, String asOutfitName) Global Native
Bool     Function RenameOutfit      (String asOutfitName, String asRenameTo) Global Native
Bool     Function OutfitExists      (String asOutfitName) Global Native
         Function OverwriteOutfit   (String asOutfitName, Armor[] akArmors) Global Native
         Function SetEnabled        (Bool abEnabled) Global Native
         Function SetSelectedOutfit (String asOutfitName) Global Native