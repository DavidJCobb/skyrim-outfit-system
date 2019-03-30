Scriptname SkyOutSysQuickslotManager extends Quest Hidden

Bool     Property pbEnabled = False Auto Hidden
String[] Property psOutfitNames Auto Hidden

String Property psShowForEmptyOutfit = "[None]" Auto
Spell  Property SkyrimOutfitSystemQuickslotSpell Auto ; auto-fill

;
; We offer the user a spell that can be cast in order to show a pop-up 
; menu, allowing them to quickswap between some of their outfits. In 
; order to show the outfit names in this menu, we need to resort to a 
; bit of trickery: we can't have dynamic menu buttons, and menu text 
; can only be dynamic using quest aliases.
;
; Accordingly, we have a quest, with aliases pointing to predefined 
; persistent refs that the menu can pull text from; and this system, 
; to manage those references' names and, consequently, the text that 
; the menu pulls.
;
; In order to add more quickslots, we must add:
;
;  - A new alias, with an internal ID contiguous with the existing 
;    ones. The Creation Kit is horrible about this; if you at any point 
;    delete aliases, you'll need to use xEdit to fix up the IDs.
;
;  - A new persistent reference.
;
;  - A new base form for that reference. We can't rename individual 
;    references; we can only rename base forms.
;
;  - Code to restart this quest so that the new alias fills.
;

Event OnInit()
   psOutfitNames = new String[3]
EndEvent

Bool Function GetEnabled()
   Return pbEnabled
EndFunction
Function SetEnabled(Bool abState)
   pbEnabled = abState
   If abState
      Game.GetPlayer().AddSpell(SkyrimOutfitSystemQuickslotSpell, False)
   Else
      Game.GetPlayer().RemoveSpell(SkyrimOutfitSystemQuickslotSpell)
   EndIf
EndFunction

Int Function GetQuickslotCount()
   Return psOutfitNames.Length
EndFunction
String Function GetQuickslot(Int aiIndex)
   If aiIndex < 0 || aiIndex > psOutfitNames.Length
      Debug.TraceStack("Bad quickslot index.")
      Return ""
   EndIf
   Return psOutfitNames[aiIndex]
EndFunction
Function SetQuickslot(Int aiIndex, String asOutfitName)
   If aiIndex < 0 || aiIndex > psOutfitNames.Length
      Debug.TraceStack("Bad quickslot index.")
      Return
   EndIf
   psOutfitNames[aiIndex] = asOutfitName
   UpdateSlot(aiIndex)
EndFunction
Int Function IndexOfQuickslot(String asOutfitName)
   Return psOutfitNames.Find(asOutfitName)
EndFunction

Function UpdateSlot(Int aiSlot)
   ReferenceAlias kSlot = Self.GetAlias(aiSlot) as ReferenceAlias
   If !kSlot
      Debug.Trace("Skyrim Outfit System Quickslot Manager: Failed to find alias #" + aiSlot + "!")
      Return
   EndIf
   ObjectReference kRef = kSlot.GetReference()
   If !kRef
      Debug.Trace("Skyrim Outfit System Quickslot Manager: Failed to find ref #" + aiSlot + "!")
      Return
   EndIf
   String sDisplay = psOutfitNames[aiSlot]
   If !sDisplay
      sDisplay = psShowForEmptyOutfit
   EndIf
   kRef.GetBaseObject().SetName(sDisplay)
EndFunction
Function UpdateAllSlots()
   Int iIterator = 0
   Int iCount    = psOutfitNames.Length
   While iIterator < iCount
      UpdateSlot(iIterator)
      iIterator = iIterator + 1
   EndWhile
EndFunction
