Scriptname SkyrimOutfitSystemNativeFuncs Hidden

Int Function GetOutfitNameMaxLength() Global Native

Function RefreshArmorFor(Actor akSubject) Global Native

Bool     Function ArmorConflictsWithOutfit (Armor akTest, String asOutfitName) Global Native
         Function DeleteOutfit      (String asOutfitName) Global Native
Armor[]  Function GetOutfitContents (String asOutfitName) Global Native
String   Function GetSelectedOutfit () Global Native
Bool     Function IsEnabled         () Global Native
String[] Function ListOutfits       () Global Native
         Function OverwriteOutfit   (String asOutfitName, Armor[] akArmors) Global Native
         Function SetEnabled        (Bool abEnabled) Global Native
         Function SetSelectedOutfit (String asOutfitName) Global Native