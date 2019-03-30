Scriptname SkyOutSysMCM extends SKI_ConfigBase Hidden

Int      _iOutfitBrowserPage   = 0
Int      _iOutfitNameMaxBytes = 256 ; should never change at run-time; can change if the DLL is revised appropriately
String[] _sOutfitNames
String   _sSelectedOutfit = ""

String   _sEditingOutfit = ""
String   _sOutfitShowingContextMenu = ""
Int      _iOutfitEditorBodySlotPage = 0

String[] _sOutfitSlotNames
String[] _sOutfitSlotArmors
Armor[]  _kOutfitSlotArmors
Armor[]  _kOutfitContents

String[] _sOutfitEditor_AddCandidates
Armor[]  _kOutfitEditor_AddCandidates

String[] _sOutfitEditor_AddFromListCandidates
Armor[]  _kOutfitEditor_AddFromListCandidates
String   _sOutfitEditor_AddFromList_Filter   = ""
Bool     _bOutfitEditor_AddFromList_Playable = True

String[] Property pkQuickslotOutfits Auto

Int Function GetModVersion() Global ; static method; therefore, safely callable by outside parties even before/during OnInit
	Return 0x01000000
EndFunction
Int Function GetVersion()
	Return GetModVersion()
EndFunction

SkyOutSysQuickslotManager Function GetQuickslotManager() Global
   Return Quest.GetQuest("SkyrimOutfitSystemQuickslotManager") as SkyOutSysQuickslotManager
EndFunction

Event OnGameReload()
	Parent.OnGameReload()
	;
	GetQuickslotManager().UpdateAllSlots()
EndEvent
Event OnConfigInit()
   pkQuickslotOutfits = new String[4]
EndEvent
Event OnConfigOpen()
   _iOutfitNameMaxBytes = SkyrimOutfitSystemNativeFuncs.GetOutfitNameMaxLength()
   ResetOutfitBrowser()
   ResetOutfitEditor()
   RefreshCache()
EndEvent
Event OnConfigClose()
   SkyrimOutfitSystemNativeFuncs.RefreshArmorFor(Game.GetPlayer())
   ResetOutfitBrowser()
   ResetOutfitEditor()
EndEvent
Event OnPageReset(String asPage)
   If asPage == "$SkyOutSys_MCM_Options"
      ResetOutfitEditor()
      ShowOptions()
   ElseIf asPage == "$SkyOutSys_MCM_OutfitList"
      If _sEditingOutfit
         ShowOutfitEditor()
      Else
         ResetOutfitEditor()
         ShowOutfitList()
      EndIf
   EndIf
EndEvent

Function RefreshCache()
   _sOutfitNames    = SkyrimOutfitSystemNativeFuncs.ListOutfits()
   _sSelectedOutfit = SkyrimOutfitSystemNativeFuncs.GetSelectedOutfit()
   ;
   _sOutfitNames = SkyrimOutfitSystemNativeFuncs.NaturalSort_ASCII(_sOutfitNames)
EndFunction

Function ResetOutfitBrowser()
   _iOutfitBrowserPage   = 0
   _iOutfitEditorBodySlotPage = 0
   _sEditingOutfit       = ""
   _sOutfitShowingContextMenu = ""
   _sOutfitNames = new String[1]
EndFunction
Function ResetOutfitEditor()
   _sOutfitSlotNames  = new String[1]
   _sOutfitSlotArmors = new String[1]
   _kOutfitSlotArmors = new Armor[1]
   _kOutfitContents   = new Armor[1]
   _sOutfitEditor_AddCandidates = new String[1]
   _kOutfitEditor_AddCandidates = new Armor[1]
   ;
   _sOutfitEditor_AddFromListCandidates = new String[1]
   _kOutfitEditor_AddFromListCandidates = new Armor[1]
   _sOutfitEditor_AddFromList_Filter   = ""
   _bOutfitEditor_AddFromList_Playable = True
EndFunction

;/Block/; ; Helpers
   Int Function BodySlotToMask(Int aiSlot) Global
      Return Math.LeftShift(1, aiSlot - 30)
   EndFunction
   String Function BodySlotName(Int aiSlot) Global
      Return "$SkyOutSys_BodySlot" + aiSlot
   EndFunction
   Function LogStringArrayForDebugging(String asPrefix, String[] asArray) Global
      Debug.Trace(asPrefix)
      Int iIterator = 0
      While iIterator < asArray.Length
         Debug.Trace("[" + iIterator + "] == " + asArray[iIterator])
         iIterator = iIterator + 1
      EndWhile
   EndFunction
   String[] Function PrependStringToArray(String[] asMenu, String asPrepend)
      Int      iCount = asMenu.Length
      String[] kOut   = Utility.CreateStringArray(iCount + 1)
      kOut[0] = asPrepend
      Int iIterator = 0
      While iIterator < iCount
         Int iTemporary = iIterator + 1
         kOut[iTemporary] = asMenu[iIterator]
         iIterator = iTemporary
      EndWhile
      Return kOut
   EndFunction
;/EndBlock/;

Function SetupSlotDataForOutfit(String asOutfitName)
   _kOutfitContents = SkyrimOutfitSystemNativeFuncs.GetOutfitContents(asOutfitName)
   ;
   ; This process is doable in Papyrus, but not without a significant 
   ; performance hit. It's fast in the DLL.
   ;
   SkyrimOutfitSystemNativeFuncs.PrepOutfitBodySlotListing(asOutfitName)
   Int[] iSlots = SkyrimOutfitSystemNativeFuncs.GetOutfitBodySlotListingSlotIndices()
   _sOutfitSlotNames = Utility.CreateStringArray(iSlots.Length)
   Int iIterator = 0
   While iIterator < iSlots.Length
      _sOutfitSlotNames[iIterator] = BodySlotName(iSlots[iIterator])
      iIterator = iIterator + 1
   EndWhile
   _sOutfitSlotArmors = SkyrimOutfitSystemNativeFuncs.GetOutfitBodySlotListingArmorNames()
   _kOutfitSlotArmors = SkyrimOutfitSystemNativeFuncs.GetOutfitBodySlotListingArmorForms()
   SkyrimOutfitSystemNativeFuncs.ClearOutfitBodySlotListing()
EndFunction

;/Block/; ; Default handlers
   Event OnSelectST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 16) == "OutfitList_Item_"
         String sOutfitName = StringUtil.Substring(sState, 16)
         _sOutfitShowingContextMenu = sOutfitName
         ForcePageReset()
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 22) == "OutfitEditor_BodySlot_"
         Int iEntryIndex = StringUtil.Substring(sState, 22) as Int
         String sArmorName = _sOutfitSlotArmors[iEntryIndex]
         Armor  kArmorForm = _kOutfitSlotArmors[iEntryIndex]
         Bool bDelete = ShowMessage("$SkyOutSys_Confirm_RemoveArmor_Text{" + sArmorName + "}", True, "$SkyOutSys_Confirm_RemoveArmor_Yes", "$SkyOutSys_Confirm_RemoveArmor_No")
         If bDelete
            SkyrimOutfitSystemNativeFuncs.RemoveArmorFromOutfit(_sEditingOutfit, kArmorForm)
            SetupSlotDataForOutfit(_sEditingOutfit)
            ForcePageReset()
         EndIf
         Return
      EndIf
   EndEvent
   Event OnMenuOpenST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 18) == "OPT_QuickslotEntry"
         Int iQuickslotIndex = StringUtil.Substring(sState, 18) as Int
         String[] sMenu = PrependStringToArray(_sOutfitNames, "$SkyOutSys_QuickslotEdit_Cancel")
         SetMenuDialogOptions(sMenu)
         SetMenuDialogStartIndex(0)
         SetMenuDialogDefaultIndex(0)
         Return
      EndIf
   EndEvent
   Event OnMenuAcceptST(Int aiIndex)
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 18) == "OPT_QuickslotEntry"
         aiIndex = aiIndex - 1
         If aiIndex < 0 ; user canceled
            Return
         EndIf
         Int iQuickslotIndex = StringUtil.Substring(sState, 18) as Int
         SkyOutSysQuickslotManager kQM = GetQuickslotManager()
         String sOutfitName = _sOutfitNames[aiIndex]
         kQM.SetQuickslot(iQuickslotIndex, sOutfitName)
         SetMenuOptionValueST(kQM.GetQuickslot(iQuickslotIndex))
         Return
      EndIf
   EndEvent
   Event OnHighlightST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 18) == "OPT_QuickslotEntry"
         SetInfoText("$SkyOutSys_Desc_Quickslot")
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 16) == "OutfitList_Item_"
         SetInfoText("$SkyOutSys_MCMInfoText_Outfit")
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 22) == "OutfitEditor_BodySlot_"
         SetInfoText("$SkyOutSys_MCMInfoText_BodySlot")
         Return
      EndIf
   EndEvent
   Event OnDefaultST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 18) == "OPT_QuickslotEntry"
         Int iQuickslotIndex = StringUtil.Substring(sState, 18) as Int
         SkyOutSysQuickslotManager kQM = GetQuickslotManager()
         Bool bDelete = ShowMessage("$SkyOutSys_Confirm_UnsetQuickslot_Text{" + _sOutfitShowingContextMenu + "}", True, "$SkyOutSys_Confirm_UnsetQuickslot_Yes", "$SkyOutSys_Confirm_UnsetQuickslot_No")
         If bDelete
            kQM.SetQuickslot(iQuickslotIndex, "")
            SetMenuOptionValueST("")
         EndIf
         Return
      EndIf
   EndEvent
;/EndBlock/;

;/Block/; ; Options
   Function ShowOptions()
      SetCursorFillMode(TOP_TO_BOTTOM)
      AddToggleOptionST("OPT_Enabled", "$Enabled", SkyrimOutfitSystemNativeFuncs.IsEnabled())
      AddEmptyOption()
      ;
      ; Quickslots:
      ;
      SkyOutSysQuickslotManager kQM = GetQuickslotManager()
      AddHeaderOption("$SkyOutSys_MCMHeader_Quickslots")
      Int iCount = kQM.GetQuickslotCount()
      AddToggleOptionST("OPT_QuickslotsEnabled", "$SkyOutSys_Text_EnableQuickslots", kQM.GetEnabled())
      Int iIterator = 0
      While iIterator < iCount
         String sQuickslotted = kQM.GetQuickslot(iIterator)
         AddMenuOptionST("OPT_QuickslotEntry" + iIterator, "$SkyOutSys_Text_Quickslot{" + (iIterator + 1) + "}", sQuickslotted)
         iIterator = iIterator + 1
      EndWhile
   EndFunction
   ;
   State OPT_Enabled
      Event OnSelectST()
         Bool bToggle = !SkyrimOutfitSystemNativeFuncs.IsEnabled()
         SkyrimOutfitSystemNativeFuncs.SetEnabled(bToggle)
         SetToggleOptionValueST(bToggle)
      EndEvent
   EndState
   State OPT_QuickslotsEnabled
      Event OnSelectST()
         SkyOutSysQuickslotManager kQM = GetQuickslotManager()
         kQM.SetEnabled(!kQM.GetEnabled())
         SetToggleOptionValueST(kQM.GetEnabled())
      EndEvent
      Event OnHighlightST()
         SetInfoText("$SkyOutSys_Desc_EnableQuickslots")
      EndEvent
   EndState
;/EndBlock/;

;/Block/; ; Outfit editing
   Function StartEditingOutfit(String asOutfitName)
      _sEditingOutfit = asOutfitName
      SetupSlotDataForOutfit(_sEditingOutfit)
      _iOutfitEditorBodySlotPage = 0
      ForcePageReset()
   EndFunction
   Function StopEditingOutfit()
      _sOutfitShowingContextMenu = _sEditingOutfit
      _sEditingOutfit = ""
      ForcePageReset()
   EndFunction
   
   ;/Block/; ; Outfit browser
      Function ShowOutfitList()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitList")
            If _sOutfitNames.Length > 11 ; too many to fit on one screen
               Int iCount     = _sOutfitNames.Length
               Int iPageCount = iCount / 8
               If iPageCount * 8 < iCount
                  iPageCount = iPageCount + 1
               EndIf
               If _iOutfitBrowserPage >= iPageCount
                  _iOutfitBrowserPage = iPageCount - 1
               EndIf
               Int iOffset    = _iOutfitBrowserPage * 8
               Int iIterator  = 0
               Int iMax       = iCount - _iOutfitBrowserPage
               If iMax > 8 ; (visible - 3) -- make room for page separator and buttons
                  iMax = 8
               EndIf
               While iIterator < iMax
                  String sName = _sOutfitNames[iIterator + iOffset]
                  String sMark = " "
                  If sName == _sSelectedOutfit
                     sMark = "$SkyOutSys_OutfitBrowser_ActiveMark"
                     If sName == _sOutfitShowingContextMenu
                        sMark = "$SkyOutSys_OutfitBrowser_ContextActiveMark"
                     EndIf
                  ElseIf sName == _sOutfitShowingContextMenu
                     sMark = "$SkyOutSys_OutfitBrowser_ContextMark"
                  EndIf
                  AddTextOptionST("OutfitList_Item_" + sName, sName, sMark)
                  iIterator = iIterator + 1
               EndWhile
               Int iFlagsPrev = OPTION_FLAG_NONE
               Int iFlagsNext = OPTION_FLAG_NONE
               If _iOutfitBrowserPage < 1
                  iFlagsPrev = OPTION_FLAG_DISABLED
               EndIf
               If _iOutfitBrowserPage == iPageCount - 1
                  iFlagsNext = OPTION_FLAG_DISABLED
               EndIf
               SetCursorPosition(19)
               AddHeaderOption("")
               AddTextOptionST("OutfitBrowser_Prev", "$SkyOutSys_MCMText_OutfitListPageNumber{" + (_iOutfitBrowserPage + 1) + "}{" + iPageCount + "}", "$SkyOutSys_MCMText_OutfitListButtonPagePrev", iFlagsPrev)
               AddTextOptionST("OutfitBrowser_Next", "", "$SkyOutSys_MCMText_OutfitListButtonPageNext", iFlagsNext)
            Else
               Int iIterator = 0
               While iIterator < _sOutfitNames.Length
                  String sName = _sOutfitNames[iIterator]
                  String sMark = " "
                  If sName == _sSelectedOutfit
                     sMark = "$SkyOutSys_OutfitBrowser_ActiveMark"
                     If sName == _sOutfitShowingContextMenu
                        sMark = "$SkyOutSys_OutfitBrowser_ContextActiveMark"
                     EndIf
                  ElseIf sName == _sOutfitShowingContextMenu
                     sMark = "$SkyOutSys_OutfitBrowser_ContextMark"
                  EndIf
                  AddTextOptionST("OutfitList_Item_" + sName, sName, sMark)
                  iIterator = iIterator + 1
               EndWhile
            EndIf
         ;/EndBlock/;
         ;/Block/; ; Right column
            SetCursorPosition(1)
            AddHeaderOption("$SkyOutSys_MCMHeader_GeneralActions")
            AddInputOptionST("OutfitContext_New", "$SkyOutSys_OContext_New", "")
            AddInputOptionST("OutfitContext_NewFromWorn", "$SkyOutSys_OContext_NewFromWorn", "")
            AddEmptyOption()
            ;
            Int iContextFlags = OPTION_FLAG_HIDDEN
            If _sOutfitShowingContextMenu
               iContextFlags = OPTION_FLAG_NONE
            EndIf
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitActions{" + _sOutfitShowingContextMenu + "}", iContextFlags)
            If _sSelectedOutfit == _sOutfitShowingContextMenu
               AddTextOptionST("OutfitContext_Toggle", "$SkyOutSys_OContext_ToggleOff", "", iContextFlags)
            Else
               AddTextOptionST("OutfitContext_Toggle", "$SkyOutSys_OContext_ToggleOn", "", iContextFlags)
            EndIf
            AddTextOptionST ("OutfitContext_Edit",   "$SkyOutSys_OContext_Edit",   "", iContextFlags)
            AddInputOptionST("OutfitContext_Rename", "$SkyOutSys_OContext_Rename", "", iContextFlags)
            AddTextOptionST ("OutfitContext_Delete", "$SkyOutSys_OContext_Delete", "", iContextFlags)
         ;/EndBlock/;
      EndFunction
      State OutfitBrowser_Prev
         Event OnSelectST()
            _iOutfitBrowserPage = _iOutfitBrowserPage - 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitBrowser_Next
         Event OnSelectST()
            _iOutfitBrowserPage = _iOutfitBrowserPage + 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_New
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutSys_Err_OutfitNameTooLong", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutSys_Err_OutfitNameTaken", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            SkyrimOutfitSystemNativeFuncs.CreateOutfit(asTextEntry)
            RefreshCache()
            StartEditingOutfit(asTextEntry)
         EndEvent
      EndState
      State OutfitContext_NewFromWorn
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutSys_Err_OutfitNameTooLong", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutSys_Err_OutfitNameTaken", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            Armor[] kList = SkyrimOutfitSystemNativeFuncs.GetWornItems(Game.GetPlayer())
            SkyrimOutfitSystemNativeFuncs.OverwriteOutfit(asTextEntry, kList)
            RefreshCache()
            StartEditingOutfit(asTextEntry)
         EndEvent
      EndState
      State OutfitContext_Toggle
         Event OnSelectST()
            If _sSelectedOutfit == _sOutfitShowingContextMenu
               SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit("")
            Else
               SkyrimOutfitSystemNativeFuncs.SetSelectedOutfit(_sOutfitShowingContextMenu)
            EndIf
            RefreshCache()
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_Edit
         Event OnSelectST()
            StartEditingOutfit(_sOutfitShowingContextMenu)
         EndEvent
      EndState
      State OutfitContext_Rename
         Event OnInputOpenST()
            SetInputDialogStartText("outfit name or blank to cancel")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            If asTextEntry == _sOutfitShowingContextMenu
               Return
            EndIf
            If StringUtil.GetLength(asTextEntry) > _iOutfitNameMaxBytes
               ShowMessage("$SkyOutSys_Err_OutfitNameTooLong", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            If SkyrimOutfitSystemNativeFuncs.OutfitExists(asTextEntry)
               ShowMessage("$SkyOutSys_Err_OutfitNameTaken", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            Bool bSuccess = SkyrimOutfitSystemNativeFuncs.RenameOutfit(_sOutfitShowingContextMenu, asTextEntry)
            If bSuccess
               SkyOutSysQuickslotManager kQM = GetQuickslotManager()
               Int iIndex = kQM.IndexOfQuickslot(_sOutfitShowingContextMenu)
               If iIndex >= 0
                  kQM.SetQuickslot(iIndex, asTextEntry)
               EndIf
               ;
               _sOutfitShowingContextMenu = asTextEntry
               RefreshCache()
               ForcePageReset()
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_RenameOutfit{" + _sOutfitShowingContextMenu + "}")
         EndEvent
      EndState
      State OutfitContext_Delete
         Event OnSelectST()
            If !_sOutfitShowingContextMenu
               Return
            EndIf
            Bool bDelete = ShowMessage("$SkyOutSys_Confirm_Delete_Text{" + _sOutfitShowingContextMenu + "}", True, "$SkyOutSys_Confirm_Delete_Yes", "$SkyOutSys_Confirm_Delete_No")
            If bDelete
               SkyrimOutfitSystemNativeFuncs.DeleteOutfit(_sOutfitShowingContextMenu)
               ;
               SkyOutSysQuickslotManager kQM = GetQuickslotManager()
               Int iIndex = kQM.IndexOfQuickslot(_sOutfitShowingContextMenu)
               If iIndex >= 0
                  kQM.SetQuickslot(iIndex, "")
               EndIf
               ;
               RefreshCache()
               StopEditingOutfit()
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_DeleteOutfit{" + _sOutfitShowingContextMenu + "}")
         EndEvent
      EndState
   ;/EndBlock/;
   ;/Block/; ; Outfit editor
      Function ShowOutfitEditor()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption ("$SkyOutSys_MCMHeader_OutfitEditor{" + _sEditingOutfit + "}")
            AddTextOptionST ("OutfitEditor_Back",           "$SkyOutSys_OEdit_Back", "")
            AddMenuOptionST ("OutfitEditor_AddFromCarried", "$SkyOutSys_OEdit_AddFromCarried", "")
            AddMenuOptionST ("OutfitEditor_AddFromWorn",    "$SkyOutSys_OEdit_AddFromWorn", "")
            AddInputOptionST("OutfitEditor_AddByID",        "$SkyOutSys_OEdit_AddByID", "")
            AddEmptyOption()
            AddHeaderOption  ("$SkyOutSys_OEdit_AddFromList_Header")
            AddMenuOptionST  ("OutfitEditor_AddFromList_Menu",     "$SkyOutSys_OEdit_AddFromList_Search", "")
            AddInputOptionST ("OutfitEditor_AddFromList_Filter",   "$SkyOutSys_OEdit_AddFromList_Filter_Name", _sOutfitEditor_AddFromList_Filter)
            AddToggleOptionST("OutfitEditor_AddFromList_Playable", "$SkyOutSys_OEdit_AddFromList_Filter_Playable", _bOutfitEditor_AddFromList_Playable)
            ;
            ; All add functions must fail if the armor already exists 
            ; in the item (though that shouldn't cause problems on the 
            ; DLL side of things; we use std::set rather than vector, 
            ; so redundant entries are impossible anyway).
            ;
         ;/EndBlock/;
         ;/Block/; ; Right column
            SetCursorPosition(1)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitSlots")
            ;
            ; The goal here:
            ;
            ;  - Show only the body slots that the outfit uses
            ;
            ;  - If a body slot is covered by multiple different armors 
            ;    in the outfit (i.e. the user has chosen to enable the 
            ;    use of conflicting armors), then show the slot multiple 
            ;    times, once for each armor.
            ;
            ;     - As of this writing, conflicting armors don't actually 
            ;       work in our patch; the last armor for a slot "wins." 
            ;       However, there are no real engine limitations on how 
            ;       many ArmorAddons can cover a given body part; if we 
            ;       were to reconfigure our patch, then we could allow 
            ;       users to enable conflicts on a per-armor basis. As 
            ;       such, this approach is prep work for that.
            ;
            ;       (During early development, it wasn't immediately 
            ;       clear whether our patch supported conflicting slots; 
            ;       random tinkering in the R&D stage proved that the 
            ;       innermost bits of the armor system allow conflicts, 
            ;       but I didn't know whether my patch was low-level 
            ;       enough to take advantage of that. This approach to 
            ;       listing the outfit contents was designed at that 
            ;       stage of development.)
            ;
            Int iSlotCount = _sOutfitSlotNames.Length
            If iSlotCount > 11
               Int iPageCount = iSlotCount / 8
               If iPageCount * 8 < iSlotCount
                  iPageCount = iPageCount + 1
               EndIf
               If _iOutfitEditorBodySlotPage >= iPageCount
                  _iOutfitEditorBodySlotPage = iPageCount - 1
               EndIf
               Int iOffset    = _iOutfitEditorBodySlotPage * 8
               Int iIterator  = 0
               Int iMax       = iSlotCount - iOffset
               If iMax > 8 ; (visible - 3) -- make room for page separator and buttons
                  iMax = 8
               EndIf
               While iIterator < iMax
                  String sSlot  = _sOutfitSlotNames [iIterator + iOffset]
                  String sArmor = _sOutfitSlotArmors[iIterator + iOffset]
                  If !sArmor
                     sArmor = "$SkyOutSys_NamelessArmor"
                  EndIf
                  AddTextOptionST("OutfitEditor_BodySlot_" + iIterator, sSlot, sArmor)
                  iIterator = iIterator + 1
               EndWhile
               Int iFlagsPrev = OPTION_FLAG_NONE
               Int iFlagsNext = OPTION_FLAG_NONE
               If _iOutfitEditorBodySlotPage < 1
                  iFlagsPrev = OPTION_FLAG_DISABLED
               EndIf
               If _iOutfitEditorBodySlotPage == iPageCount - 1
                  iFlagsNext = OPTION_FLAG_DISABLED
               EndIf
               SetCursorPosition(19)
               AddHeaderOption("")
               AddTextOptionST("OutfitEditor_BodySlotsPrev", "$SkyOutSys_MCMText_OutfitSlotsPageNumber{" + (_iOutfitEditorBodySlotPage + 1) + "}{" + iPageCount + "}", "$SkyOutSys_MCMText_OutfitSlotsButtonPagePrev", iFlagsPrev)
               AddTextOptionST("OutfitEditor_BodySlotsNext", "", "$SkyOutSys_MCMText_OutfitSlotsButtonPageNext", iFlagsNext)
            ElseIf iSlotCount
               Int iIterator = 0
               While iIterator < iSlotCount
                  String sSlot  = _sOutfitSlotNames[iIterator]
                  String sArmor = _sOutfitSlotArmors[iIterator]
                  If !sArmor
                     sArmor = "$SkyOutSys_NamelessArmor"
                  EndIf
                  AddTextOptionST("OutfitEditor_BodySlot_" + iIterator, sSlot, sArmor)
                  iIterator = iIterator + 1
               EndWhile
            Else
               AddTextOption("$SkyOutSys_OutfitEditor_OutfitIsEmpty", "")
            EndIf
         ;/EndBlock/;
      EndFunction
      Function AddArmorToOutfit(Armor kAdd)
         If !kAdd || !_sEditingOutfit
            Return
         EndIf
         If SkyrimOutfitSystemNativeFuncs.ArmorConflictsWithOutfit(kAdd, _sEditingOutfit)
            Bool bSwap = ShowMessage("$SkyOutSys_Confirm_BodySlotConflict_Text", True, "$SkyOutSys_Confirm_BodySlotConflict_Yes", "$SkyOutSys_Confirm_BodySlotConflict_No")
            If bSwap
               SkyrimOutfitSystemNativeFuncs.RemoveConflictingArmorsFrom(kAdd, _sEditingOutfit)
            Else
               Return
            EndIf
         EndIf
         SkyrimOutfitSystemNativeFuncs.AddArmorToOutfit(_sEditingOutfit, kAdd)
         SetupSlotDataForOutfit(_sEditingOutfit)
         ForcePageReset()
      EndFunction
      ;
      State OutfitEditor_BodySlotsPrev
         Event OnSelectST()
            _iOutfitEditorBodySlotPage = _iOutfitEditorBodySlotPage - 1
            ForcePageReset()
         EndEvent
      EndState
      State OutfitEditor_BodySlotsNext
         Event OnSelectST()
            _iOutfitEditorBodySlotPage = _iOutfitEditorBodySlotPage + 1
            ForcePageReset()
         EndEvent
      EndState
      ;
      State OutfitEditor_Back
         Event OnSelectST()
            StopEditingOutfit()
            ForcePageReset()
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_BackToOutfitList")
         EndEvent
      EndState
      State OutfitEditor_AddFromCarried
         Event OnMenuOpenST()
            _kOutfitEditor_AddCandidates = SkyrimOutfitSystemNativeFuncs.GetCarriedArmor(Game.GetPlayer())
            Int iCount = _kOutfitEditor_AddCandidates.Length
            _sOutfitEditor_AddCandidates = Utility.CreateStringArray(iCount)
            Int iIterator = 0
            While iIterator < iCount
               Armor  kCurrent = _kOutfitEditor_AddCandidates[iIterator]
               String sCurrent = ""
               If kCurrent
                  sCurrent = kCurrent.GetName()
               EndIf
               If !sCurrent
                  sCurrent = "$SkyOutSys_NamelessArmor"
               EndIf
               _sOutfitEditor_AddCandidates[iIterator] = sCurrent
               iIterator = iIterator + 1
            EndWhile
            ;
            _sOutfitEditor_AddCandidates = SkyrimOutfitSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddCandidates, _kOutfitEditor_AddCandidates)
            ;
            String[] sMenu = PrependStringToArray(_sOutfitEditor_AddCandidates, "$SkyOutSys_OEdit_AddCancel")
            ;
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
         EndEvent
         Event OnMenuAcceptST(Int aiIndex)
            aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
            If aiIndex < 0 ; user canceled
               _sOutfitEditor_AddCandidates = new String[1]
               _kOutfitEditor_AddCandidates = new Armor[1]
               Return
            EndIf
            Armor kCurrent = _kOutfitEditor_AddCandidates[aiIndex]
            _sOutfitEditor_AddCandidates = new String[1]
            _kOutfitEditor_AddCandidates = new Armor[1]
            If kCurrent
               AddArmorToOutfit(kCurrent)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_AddToOutfitFromCarried")
         EndEvent
      EndState
      State OutfitEditor_AddFromWorn
         Event OnMenuOpenST()
            _kOutfitEditor_AddCandidates = SkyrimOutfitSystemNativeFuncs.GetWornItems(Game.GetPlayer())
            Int iCount = _kOutfitEditor_AddCandidates.Length
            _sOutfitEditor_AddCandidates = Utility.CreateStringArray(iCount)
            Int iIterator = 0
            While iIterator < iCount
               Armor  kCurrent = _kOutfitEditor_AddCandidates[iIterator]
               String sCurrent = ""
               If kCurrent
                  sCurrent = kCurrent.GetName()
               EndIf
               If !sCurrent
                  sCurrent = "$SkyOutSys_NamelessArmor"
               EndIf
               _sOutfitEditor_AddCandidates[iIterator] = sCurrent
               iIterator = iIterator + 1
            EndWhile
            ;
            _sOutfitEditor_AddCandidates = SkyrimOutfitSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddCandidates, _kOutfitEditor_AddCandidates)
            ;
            String[] sMenu = PrependStringToArray(_sOutfitEditor_AddCandidates, "$SkyOutSys_OEdit_AddCancel")
            ;
            SetMenuDialogOptions(sMenu)
            SetMenuDialogStartIndex(0)
            SetMenuDialogDefaultIndex(0)
         EndEvent
         Event OnMenuAcceptST(Int aiIndex)
            aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
            If aiIndex < 0 ; user canceled
               _sOutfitEditor_AddCandidates = new String[1]
               _kOutfitEditor_AddCandidates = new Armor[1]
               Return
            EndIf
            Armor kCurrent = _kOutfitEditor_AddCandidates[aiIndex]
            _sOutfitEditor_AddCandidates = new String[1]
            _kOutfitEditor_AddCandidates = new Armor[1]
            If kCurrent
               AddArmorToOutfit(kCurrent)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_AddToOutfitFromWorn")
         EndEvent
      EndState
      State OutfitEditor_AddByID
         Event OnInputOpenST()
            SetInputDialogStartText("0x00000000")
         EndEvent
         Event OnInputAcceptST(String asTextEntry)
            If !asTextEntry
               Return
            EndIf
            Int iFormID = SkyrimOutfitSystemNativeFuncs.HexToInt32(asTextEntry)
            If !iFormID
               Return
            EndIf
            Form  kForm  = Game.GetForm(iFormID)
            Armor kArmor = Game.GetForm(iFormID) as Armor
            If !kArmor
               If !kForm
                  ShowMessage("$SkyOutSys_Err_FormDoesNotExist", False, "$SkyOutSys_ErrDismiss")
                  Return
               EndIf
               ShowMessage("$SkyOutSys_Err_FormIsNotArmor", False, "$SkyOutSys_ErrDismiss")
               Return
            EndIf
            String sName = kArmor.GetName()
            If !sName
               sName = "$SkyOutSys_NamelessArmor"
            EndIf
            Bool bConfirm = ShowMessage("$SkyOutSys_Confirm_AddByID_Text{" + sName + "}", True, "$SkyOutSys_Confirm_AddByID_Yes", "$SkyOutSys_Confirm_AddByID_No")
            If bConfirm
               AddArmorToOutfit(kArmor)
            EndIf
         EndEvent
         Event OnHighlightST()
            SetInfoText("$SkyOutSys_MCMInfoText_AddToOutfitByID")
         EndEvent
      EndState
      ;
      ;/Block/; ; Add-from-list
         Function UpdateArmorSearch()
            SkyrimOutfitSystemNativeFuncs.PrepArmorSearch(_sOutfitEditor_AddFromList_Filter, _bOutfitEditor_AddFromList_Playable)
            _sOutfitEditor_AddFromListCandidates = SkyrimOutfitSystemNativeFuncs.GetArmorSearchResultNames()
            _kOutfitEditor_AddFromListCandidates = SkyrimOutfitSystemNativeFuncs.GetArmorSearchResultForms()
            SkyrimOutfitSystemNativeFuncs.ClearArmorSearch()
            ;
            _sOutfitEditor_AddFromListCandidates = SkyrimOutfitSystemNativeFuncs.NaturalSortPairArmor_ASCII(_sOutfitEditor_AddFromListCandidates, _kOutfitEditor_AddFromListCandidates)
         EndFunction
         ;
         State OutfitEditor_AddFromList_Menu
            Event OnMenuOpenST()
               UpdateArmorSearch()
               String[] sMenu = PrependStringToArray(_sOutfitEditor_AddFromListCandidates, "$SkyOutSys_OEdit_AddCancel")
               ;
               SetMenuDialogOptions(sMenu)
               SetMenuDialogStartIndex(0)
               SetMenuDialogDefaultIndex(0)
            EndEvent
            Event OnMenuAcceptST(Int aiIndex)
               aiIndex = aiIndex - 1 ; first menu item is a "cancel" option
               If aiIndex < 0 ; user canceled
                  Return
               EndIf
               Armor kCurrent = _kOutfitEditor_AddFromListCandidates[aiIndex]
               If kCurrent
                  AddArmorToOutfit(kCurrent)
               EndIf
            EndEvent
         EndState
         State OutfitEditor_AddFromList_Filter
            Event OnInputOpenST()
               SetInputDialogStartText(_sOutfitEditor_AddFromList_Filter)
            EndEvent
            Event OnInputAcceptST(String asTextEntry)
               If asTextEntry == _sOutfitEditor_AddFromList_Filter
                  Return
               EndIf
               _sOutfitEditor_AddFromList_Filter = asTextEntry
               SetInputOptionValueST(asTextEntry)
            EndEvent
         EndState
         State OutfitEditor_AddFromList_Playable
            Event OnSelectST()
               _bOutfitEditor_AddFromList_Playable = !_bOutfitEditor_AddFromList_Playable
               SetToggleOptionValueST(_bOutfitEditor_AddFromList_Playable)
            EndEvent
         EndState
      ;/EndBlock/;
   ;/EndBlock/;
;/EndBlock/;