#if 0
    ALIAS_WRAPPER(SetControlInterface)
   ,ALIAS_WRAPPER(SetControlImageInterface)

   ,ALIAS_WRAPPER(AlignBaseToWindows)
   ,ALIAS_WRAPPER(SetBaseColor)
   ,ALIAS_WRAPPER(GetBaseColor)

//-------- Frame and generic control functions --------------
,ALIAS_WRAPPER(CreateFrame)
,ALIAS_WRAPPER(CreateFrameFromRenderer)
   
   ,NULL //ALIAS_WRAPPER(DestroyFrameEx)
				  ,ALIAS_WRAPPER( FrameBorderX )
				  ,ALIAS_WRAPPER( FrameBorderXOfs )
					,ALIAS_WRAPPER(FrameBorderY)
					,ALIAS_WRAPPER(FrameBorderYOfs)
   ,ALIAS_WRAPPER(DisplayFrame)
   ,ALIAS_WRAPPER(SizeCommon)
   ,ALIAS_WRAPPER(SizeCommonRel)
   ,ALIAS_WRAPPER(MoveCommon)
   ,ALIAS_WRAPPER(MoveCommonRel)
   ,ALIAS_WRAPPER(MoveSizeCommon)
   ,ALIAS_WRAPPER(MoveSizeCommonRel)
,ALIAS_WRAPPER(GetControl)
,NULL //ALIAS_WRAPPER(GetFrameUserData)
,NULL //ALIAS_WRAPPER(SetFrameUserData)
,ALIAS_WRAPPER(UpdateFrameEx)
,ALIAS_WRAPPER(SetFrameMousePosition)
//PSI_PROC_PTR void SetDefaultOkayID( PFRAME pFrame, int nID );
//PSI_PROC_PTR void SetDefaultCancelID( PFRAME pFrame, int nID );

//-------- Generic control functions --------------
   ,ALIAS_WRAPPER(GetFrame)
   ,ALIAS_WRAPPER(GetNearControl)
   ,ALIAS_WRAPPER(GetCommonTextEx)
   ,NULL //ALIAS_WRAPPER(SetControlText)
   ,ALIAS_WRAPPER(SetCommonFocus)
   ,ALIAS_WRAPPER(EnableControl)
   ,ALIAS_WRAPPER(IsControlEnabled)
   ,NULL //ALIAS_WRAPPER(CreateControlExx)
,ALIAS_WRAPPER(GetControlSurface)
,NULL //ALIAS_WRAPPER(SetCommonDraw)
,NULL //ALIAS_WRAPPER(SetCommonKey)
,NULL //ALIAS_WRAPPER(SetCommonMouse)
   ,NULL //ALIAS_WRAPPER(UpdateControlEx)
   ,ALIAS_WRAPPER(GetControlID)

   ,NULL //ALIAS_WRAPPER(DestroyControlEx)
   ,ALIAS_WRAPPER(SetNoFocus)
   ,ALIAS_WRAPPER(ControlExtraData)

   ,ALIAS_WRAPPER(OrphanCommon)
   ,ALIAS_WRAPPER(AdoptCommon)

//------ General Utilities ------------
   ,ALIAS_WRAPPER(AddCommonButtonsEx)
   ,ALIAS_WRAPPER(AddCommonButtons)

   ,ALIAS_WRAPPER(CommonLoop)
   ,ALIAS_WRAPPER(ProcessControlMessages)
//------ BUTTONS ------------
   ,NULL//ALIAS_WRAPPER(MakeButton)
   ,NULL//ALIAS_WRAPPER(MakeImageButton)
   ,NULL//ALIAS_WRAPPER(MakeCustomDrawnButton)
   ,ALIAS_WRAPPER(PressButton)
   ,ALIAS_WRAPPER(IsButtonPressed)

   ,NULL//ALIAS_WRAPPER(MakeCheckButton)
   ,NULL//ALIAS_WRAPPER(MakeRadioButton)
   ,ALIAS_WRAPPER(GetCheckState)
   ,ALIAS_WRAPPER(SetCheckState)

//------ Static Text -----------
   ,ALIAS_WRAPPER(MakeTextControl)
   ,ALIAS_WRAPPER(SetTextControlColors)

//------- Edit Control ---------
   ,ALIAS_WRAPPER(MakeEditControl)
// Use GetContrcolText/SetControlText

//------- Slider Control --------
   ,ALIAS_WRAPPER(MakeSlider)
   ,ALIAS_WRAPPER(SetSliderValues)

//------- Color Control --------
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
   ,ALIAS_WRAPPER(PickColor)

//------- Font Control --------
,ALIAS_WRAPPER(PickFont)
,ALIAS_WRAPPER(RenderFont)
//------- ListBox Control --------
   ,ALIAS_WRAPPER(MakeListBox)
   
   ,ALIAS_WRAPPER(ResetList)
   ,ALIAS_WRAPPER(InsertListItem)
   ,ALIAS_WRAPPER(InsertListItemEx)
   ,ALIAS_WRAPPER(AddListItem)
   ,ALIAS_WRAPPER(AddListItemEx)
   ,ALIAS_WRAPPER(DeleteListItem)
   ,ALIAS_WRAPPER(SetItemData)
   ,ALIAS_WRAPPER(GetItemData)
   ,ALIAS_WRAPPER(GetItemText)
   ,ALIAS_WRAPPER(GetSelectedItem)
   ,ALIAS_WRAPPER(SetSelectedItem)
   ,ALIAS_WRAPPER(SetCurrentItem)
   ,ALIAS_WRAPPER(FindListItem)
,ALIAS_WRAPPER(GetNthItem)
, ALIAS_WRAPPER(SetSelChangeHandler)
   ,ALIAS_WRAPPER(SetDoubleClickHandler)
//------- GridBox Control --------
#ifdef __LINUX__
   , ALIAS_WRAPPER(NotNULL) //ALIAS_WRAPPER(MakeGridBox)
#endif
	//------- Popup Menus ------------
// popup interface.... these are mimics of windows... 
   ,ALIAS_WRAPPER(CreatePopup)
   ,ALIAS_WRAPPER(DestroyPopup)
   ,ALIAS_WRAPPER(GetPopupData)
   ,ALIAS_WRAPPER(AppendPopupItem)
   ,ALIAS_WRAPPER(CheckPopupItem)
   ,ALIAS_WRAPPER(DeletePopupItem)
   ,ALIAS_WRAPPER(TrackPopup)
//------- Color Control --------
// these are basic basic file selection dialogs... 
// the concept is valid, and they should be common like controls...
   , ALIAS_WRAPPER(PSI_OpenFile)
// this may be used for save I think....
   , ALIAS_WRAPPER(PSI_OpenFileEx)
//------- Scroll Control --------
   , ALIAS_WRAPPER(SetScrollParams)
   , ALIAS_WRAPPER(MakeScrollBar)
, ALIAS_WRAPPER(SetScrollUpdateMethod)
, ALIAS_WRAPPER(MoveScrollBar)

//------- Misc Controls (and widgets) --------
																																														  , ALIAS_WRAPPER(SimpleMessageBox)
, ALIAS_WRAPPER(HideFrame)
#else
   NULL
#endif
