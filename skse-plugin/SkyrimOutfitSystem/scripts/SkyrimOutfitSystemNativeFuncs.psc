Scriptname SkyrimOutfitSystemNativeFuncs Hidden

;
; Information on the outfit system:
;
;  - Outfit names are not sorted; functions to retrieve a list of outfits 
;    will return an unsorted list. List order will remain consistent until 
;    the list changes (i.e. the std::unordered_map's iterators invalidate).
;
;  - Inventory lists are unsorted but likely to remain similarly consistent.
;
;  - Armor search results are not intentionally sorted, but will most 
;    likely be returned in form ID order.
;
;  - Body part listings are sorted by body part index.
;
;  - Outfit names shouldn't be longer than the constant returned by a call 
;    to GetOutfitNameMaxLength(); currently this limit is due to how SKSE 
;    serializes and loads strings in the co-save.
;
;  - Outfit names shouldn't be blank; a blank outfit name refers to "not 
;    using an outfit."
;

Int Function GetOutfitNameMaxLength() Global Native

Armor[] Function GetCarriedArmor (Actor akSubject) Global Native
Armor[] Function GetWornItems    (Actor akSubject) Global Native
        Function RefreshArmorFor (Actor akSubject) Global Native ; force akSubject to update their ArmorAddons

;
; Search through all armor forms defined in the game (excluding templated ones). 
; Filter by name or require the "playable" flag.
;
         Function PrepArmorSearch           (String asNameFilter = "", Bool abMustBePlayable = True) Global Native
Armor[]  Function GetArmorSearchResultForms () Global Native
String[] Function GetArmorSearchResultNames () Global Native
         Function ClearArmorSearch          () Global Native

;
; Given an outfit, generate string and form arrays representing which body slots 
; are taken by which armors.
;
         Function PrepOutfitBodySlotListing           (String asOutfitName) Global Native
Armor[]  Function GetOutfitBodySlotListingArmorForms  () Global Native
String[] Function GetOutfitBodySlotListingArmorNames  () Global Native
Int[]    Function GetOutfitBodySlotListingSlotIndices () Global Native
         Function ClearOutfitBodySlotListing          () Global Native

;
; String functions, to be synchronized with CobbAPI:
;
String[] Function NaturalSort_ASCII          (String[] asStrings, Bool abDescending = False) Global Native
String[] Function NaturalSortPairArmor_ASCII (String[] asStrings, Armor[] akForms, Bool abDescending = False) Global Native ; modifies akForms; returns sorted copy of asStrings
;
Int      Function HexToInt32 (String asHex) Global Native
String   Function ToHex      (Int aiValue, Int aiDigits) Global Native

;
; Functions for working with the armor-addon override service.
;
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