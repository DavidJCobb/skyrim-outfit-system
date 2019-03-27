Scriptname SkyOutSysQuickslotEffect extends activemagiceffect

Message Property SkyrimOutfitSystemQuickslotMenuMessage Auto

Event OnEffectStart(Actor akCaster, Actor akTarget)
   Int iSelection = SkyrimOutfitSystemQuickslotMenuMessage.Show()
   ;
   ; The first option is "Cancel."
   ;
   If iSelection
      iSelection = iSelection - 1
      SkyOutSysQuickslotManager kManager = Quest.GetQuest("SkyrimOutfitSystemQuickslotManager") as SkyOutSysQuickslotManager
      Int iCount = kManager.GetQuickslotCount()
      If iSelection < iCount
         String sOutfit = kManager.GetQuickslot(iSelection)
         SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit(sOutfit)
      Else
         ;
         ; The option after the last outfit is "No Outfit."
         ;
         SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit("")
      EndIf
      SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   EndIf
EndEvent
