Scriptname SkyOutSysMCM extends SKI_ConfigBase Hidden

Bool _bEditingOutfit = false

Int      _iOutfitBrowserPage   = 0
Int      _iOutfitNameMaxLength = 256 ; should never change at run-time; can change if the DLL is revised appropriately
String[] _sOutfitNames
String   _sSelectedOutfit = ""
String   _sEditingOutfit = ""
String   _sOutfitShowingContextMenu = ""
Int      _iOutfitEditorBodySlotPage = 0

String[] _sOutfitSlotNames
String[] _sOutfitSlotArmors
Form[]   _kOutfitSlotArmors ; must be a Form array so we can resize it

Int Function GetVersion()
	return 0x01000000
EndFunction

Event OnConfigInit()
EndEvent
Event OnConfigOpen()
   _iOutfitNameMaxLength = SkyrimOutfitSystemNativeFuncs.GetOutfitNameMaxLength()
   ResetOutfitBrowser()
   RefreshCache()
EndEvent
Event OnConfigClose()
EndEvent
Event OnPageReset(String asPage)
   If asPage == "$Options"
      ResetOutfitBrowser()
      ShowOptions()
   ElseIf asPage == "$SkyOutSys_MCM_OutfitList"
      If _bEditingOutfit
         ShowOutfitEditor()
      Else
         ShowOutfitList()
      EndIf
   EndIf
EndEvent

Function RefreshCache()
   _sOutfitNames    = SkyrimOutfitSystemNativeFuncs.ListOutfits()
   _sSelectedOutfit = SkyrimOutfitSystemNativeFuncs.GetSelectedOutfit()
EndFunction

Function ResetOutfitBrowser()
   _bEditingOutfit       = false
   _iOutfitBrowserPage   = 0
   _iOutfitEditorBodySlotPage = 0
   _sEditingOutfit       = ""
   _sOutfitShowingContextMenu = ""
EndFunction

Int Function BodySlotToMask(Int aiSlot) Global
   Return Math.LeftShift(1, aiSlot - 30)
EndFunction
String Function BodySlotName(Int aiSlot) Global
   Return "$SkyOutSys_BodySlot" + aiSlot
EndFunction

Function SetupSlotDataForOutfit(String asOutfitName)
   ;
   ; TODO: This process incurs a significant performance penalty; 
   ; we should move it to the DLL.
   ;
   _sOutfitSlotNames  = new String[32]
   _sOutfitSlotArmors = new String[32]
   _kOutfitSlotArmors = new Form[32] ; must be a Form array so we can resize it
   Int iArraySize = 0
   ;
   Armor[] kArmors = SkyrimOutfitSystemNativeFuncs.GetOutfitContents(asOutfitName)
   Int iIteratorBS = 30
   While iIteratorBS < 62
      Int    iMask      = BodySlotToMask(iIteratorBS)
      String sMask      = BodySlotName(iIteratorBS)
      Int    iIteratorA = 0
      While iIteratorA < kArmors.Length
         Armor kCurrent   = kArmors[iIteratorA]
         Int   iArmorMask = kCurrent.GetSlotMask()
         If Math.LogicalAnd(iArmorMask, iMask)
            _sOutfitSlotNames[iArraySize]  = sMask
            _sOutfitSlotArmors[iArraySize] = kCurrent.GetName()
            _kOutfitSlotArmors[iArraySize] = kCurrent
            ;
            iArraySize = iArraySize + 1
            If iArraySize >= _sOutfitSlotNames.Length
               Int iNewSize = _sOutfitSlotNames.Length + 8
               ;
               ; Reallocate.
               ;
               _sOutfitSlotNames  = Utility.ResizeStringArray(_sOutfitSlotNames, iNewSize)
               _sOutfitSlotArmors = Utility.ResizeStringArray(_sOutfitSlotArmors, iNewSize)
               _kOutfitSlotArmors = Utility.ResizeFormArray(_kOutfitSlotArmors, iNewSize)
            EndIf
         EndIf
         iIteratorA = iIteratorA + 1
      EndWhile
      iIteratorBS = iIteratorBS + 1
   EndWhile
   ;
   ; Shrink arrays to fit:
   ;
   _sOutfitSlotNames  = Utility.ResizeStringArray(_sOutfitSlotNames,  iArraySize)
   _sOutfitSlotArmors = Utility.ResizeStringArray(_sOutfitSlotArmors, iArraySize)
   _kOutfitSlotArmors = Utility.ResizeFormArray  (_kOutfitSlotArmors, iArraySize)
EndFunction

;/Block/; ; Default handlers
   Event OnSelectST()
      String sState = GetState()
      If StringUtil.Substring(sState, 0, 16) == "OutfitList_Item_"
         String sOutfitName = StringUtil.Substring(sState, 16)
         _sOutfitShowingContextMenu = sOutfitName
         ;
         ; TODO: pop context menu for this outfit
         ;
         ForcePageReset()
         Return
      EndIf
      If StringUtil.Substring(sState, 0, 22) == "OutfitEditor_BodySlot_"
         Int iEntryIndex = StringUtil.Substring(sState, 22) as Int
         ;
         ; TODO: Take action on _kOutfitSlotArmors[iEntryIndex], i.e. 
         ; offer to remove it from the outfit
         ;
         Return
      EndIf
   EndEvent
;/EndBlock/;

;/Block/; ; Options
   Function ShowOptions()
      AddToggleOptionST("OPT_Enabled", "$Enabled", SkyrimOutfitSystemNativeFuncs.IsEnabled())
   EndFunction
   
   State OPT_Enabled
      Event OnSelectST()
         Bool bToggle = !SkyrimOutfitSystemNativeFuncs.IsEnabled()
         SkyrimOutfitSystemNativeFuncs.SetEnabled(bToggle)
         SetToggleOptionValueST(bToggle)
      EndEvent
   EndState
;/EndBlock/;

;/Block/; ; Outfit editing
   ;/Block/; ; Outfit browser
      Function ShowOutfitList()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitList")
            If _sOutfitNames.Length > 11 ; too many to fit on one screen
               Int iCount     = _sOutfitNames.Length
               Int iPageCount = iCount / 9
               If iPageCount * 9 < iCount
                  iPageCount = iPageCount + 1
               EndIf
               If _iOutfitBrowserPage >= iPageCount
                  _iOutfitBrowserPage = iPageCount - 1
               EndIf
               Int iOffset    = _iOutfitBrowserPage * 9
               Int iIterator  = 0
               Int iMax       = iCount - _iOutfitBrowserPage
               If iMax > 9 ; (visible - 2) -- make room for page buttons
                  iMax = 9
               EndIf
               While iIterator < iMax
                  String sName = _sOutfitNames[iIterator + iOffset]
                  AddTextOptionST("OutfitList_Item_" + sName, sName, "")
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
               AddTextOptionST("OutfitBrowser_Prev", "$SkyOutSys_MCMText_OutfitListPageNumber{" + _iOutfitBrowserPage + "}{" + iPageCount + "}", "$SkyOutSys_MCMText_OutfitListButtonPagePrev", iFlagsPrev)
               AddTextOptionST("OutfitBrowser_Next", "", "$SkyOutSys_MCMText_OutfitListButtonPageNext", iFlagsNext)
            Else
               Int iIterator = 0
               While iIterator < _sOutfitNames.Length
                  String sName = _sOutfitNames[iIterator]
                  AddTextOptionST("OutfitList_Item_" + sName, sName, "")
                  iIterator = iIterator + 1
               EndWhile
            EndIf
         ;/EndBlock/;
         ;/Block/; ; Right column
            SetCursorPosition(1)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitActions")
            AddTextOptionST("OutfitContext_New", "$SkyOutSys_OContext_New", "")
            ;
            Int iContextFlags = OPTION_FLAG_HIDDEN
            If _sOutfitShowingContextMenu
               iContextFlags = OPTION_FLAG_NONE
            EndIf
            AddTextOptionST("OutfitContext_Edit", "$SkyOutSys_OContext_Edit", "", iContextFlags)
            AddTextOptionST("OutfitContext_Delete", "$SkyOutSys_OContext_Delete", "", iContextFlags)
            ;
            ; TODO: context menu items, initially hidden
            ;
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
         Event OnSelectST()
            ;
            ; TODO
            ;
            ; RELEVANT CONSIDERATIONS:
            ;
            ;  - limit name length to GetOutfitNameMaxLength()
            ;
         EndEvent
      EndState
      State OutfitContext_Edit
         Event OnSelectST()
            _sEditingOutfit = _sOutfitShowingContextMenu
            _bEditingOutfit = True
            SetupSlotDataForOutfit(_sEditingOutfit)
            _iOutfitEditorBodySlotPage = 0
            ForcePageReset()
         EndEvent
      EndState
      State OutfitContext_Delete
         Event OnSelectST()
            ;
            ; TODO
            ;
         EndEvent
      EndState
   ;/EndBlock/;
   ;/Block/; ; Outfit editor
      Function ShowOutfitEditor()
         SetCursorFillMode(TOP_TO_BOTTOM)
         ;/Block/; ; Left column
            SetCursorPosition(0)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitEditor{" + _sEditingOutfit + "}")
            ;
            ; TODO:
            ;
            ;  - Rename
            ;  - Add item from current equipment
            ;     - pop a menu of the player's equipment
            ;  - Add item by form ID
            ;  - Add item by name
            ;     - a menu of every armor form in the game
            ;        - can we filter out the ones used as "skins?"
            ;           - do skins have names? if not, we can filter those out. 
            ;             i mean, we should filter nameless armors out anyway...
            ;     - a text field to filter that list by substring
            ;     - maybe a bool to filter out non-playable armor
            ;
         ;/EndBlock/;
         ;/Block/; ; Right column
            SetCursorPosition(1)
            AddHeaderOption("$SkyOutSys_MCMHeader_OutfitSlots")
            Int iSlotCount = _sOutfitSlotNames.Length
            If iSlotCount > 11
               Int iPageCount = iSlotCount / 9
               If iPageCount * 9 < iSlotCount
                  iPageCount = iPageCount + 1
               EndIf
               If _iOutfitEditorBodySlotPage >= iPageCount
                  _iOutfitEditorBodySlotPage = iPageCount - 1
               EndIf
               Int iOffset    = _iOutfitEditorBodySlotPage * 9
               Int iIterator  = 0
               Int iMax       = iSlotCount - _iOutfitEditorBodySlotPage
               If iMax > 9 ; (visible - 2) -- make room for page buttons
                  iMax = 9
               EndIf
               While iIterator < iMax
                  String sSlot  = _sOutfitSlotNames [iIterator + iOffset]
                  String sArmor = _sOutfitSlotArmors[iIterator + iOffset]
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
               AddTextOptionST("OutfitEditor_BodySlotsPrev", "$SkyOutSys_MCMText_OutfitSlotsPageNumber{" + _iOutfitBrowserPage + "}{" + iPageCount + "}", "$SkyOutSys_MCMText_OutfitSlotsButtonPagePrev", iFlagsPrev)
               AddTextOptionST("OutfitEditor_BodySlotsNext", "", "$SkyOutSys_MCMText_OutfitSlotsButtonPageNext", iFlagsNext)
            ElseIf iSlotCount
               Int iIterator = 0
               While iIterator < iSlotCount
                  String sSlot  = _sOutfitSlotNames[iIterator]
                  String sArmor = _sOutfitSlotArmors[iIterator]
                  AddTextOptionST("OutfitEditor_BodySlot_" + iIterator, sSlot, sArmor)
                  iIterator = iIterator + 1
               EndWhile
            Else
               ;
               ; TODO: Empty outfit
               ;
            EndIf
         ;/EndBlock/;
      EndFunction
   ;/EndBlock/;
;/EndBlock/;