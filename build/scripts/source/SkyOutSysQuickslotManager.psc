Scriptname SkyOutSysQuickslotManager extends Quest Hidden

Bool     Property pbEnabled = False Auto Hidden
String[] Property psOutfitNames Auto Hidden

String Property psShowForEmptyOutfit = "[None]" Auto
Spell  Property SkyrimOutfitSystemQuickslotSpell Auto

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
