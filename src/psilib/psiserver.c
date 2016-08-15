
#include <controls.h>

#include <msgserver.h>

typedef struct server_data_tracking_tag
{
	uint32_t pid; // pid of the owner - so destruction can happen on exit
	uintptr_t thing; // object on the client side that it wants...
   struct server_data_tracking_tag *next, **me;
} DATA_TRACKING, *PDATATRACKING;

typedef struct global_data_tag
{
	PDATATRACKING pTracking;
   uint32_t MsgBase;
} GLOBAL, *PGLOBAL;

static GLOBAL g;

static int ControlClientClosed(  uint32_t *params, uint32_t length
										, uint32_t *result, uint32_t *result_length )
{
   // client has gone away - clean up all his resources....
   uint32_t pid = params[0];

   return TRUE;
}

#define SERVER_PROC(type,name) static int Server##name ( uint32_t *params, uint32_t param_length    \
                                            , uint32_t *result, uint32_t *result_length )

SERVER_PROC( void, SetControlInterface)
	//( PRENDER_INTERFACE DisplayInterface );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetControlImageInterface )//( PIMAGE_INTERFACE DisplayInterface );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( void, AlignBaseToWindows)//( void );
{
   AlignBaseToWindows();
   *result_length = INVALID_INDEX;
   return TRUE;
}

// see indexes above.
SERVER_PROC( void, SetBaseColor )//( INDEX idx, CDATA c );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( CDATA, GetBaseColor )//CDATA ( INDEX idx );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//-------- Frame and generic control functions --------------
SERVER_PROC( PFRAME, CreateFrame)//( char *caption, int x, int y
						  // 			, int w, int h
						  // 			, uint32_t BorderFlags
						 ///  			, PFRAME hAbove );
{
	if( *result_length >= 4 )
	{
		result[0] = CreateFrame( (char*)(params + 6)
									 , params[0], params[1]
									 , params[2], params[3]
									 , params[4]
                            , (PFRAME)params[5] );
		*result_length = 4;
		return TRUE;
	}
	*result_length = 0;
	return FALSE;
}

SERVER_PROC( PFRAME, CreateFrameFromRenderer)//( char *caption, int x, int y
						  // 			, int w, int h
						  // 			, uint32_t BorderFlags
						 ///  			, PFRAME hAbove );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, DestroyFrameEx)//( PFRAME pf );
{
   DestroyFrame( (PFRAME)params[0] );
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( void, DisplayFrame)//( PFRAME pf );
{
   DisplayFrame( (PFRAME)params[0] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SizeCommon)//( PFRAME pf, uint32_t w, uint32_t h );
{
   SizeCommon( (PCOMMON)params[0], params[1], params[2] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SizeCommonRel)//( PFRAME pf, uint32_t w, uint32_t h );
{
   SizeCommonRel( (PCOMMON)params[0], params[1], params[2] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, MoveCommon)//( PFRAME pf, int32_t x, int32_t y );
{
   MoveCommon( (PCOMMON)params[0], params[1], params[2] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, MoveCommonRel)//( PFRAME pf, int32_t x, int32_t y );
{
   MoveCommonRel( (PCOMMON)params[0], params[1], params[2] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, MoveSizeCommon)//( PFRAME pf, int32_t x, int32_t y, w, h );
{
   MoveSizeCommon( (PCOMMON)params[0], params[1], params[2], params[3], params[4]);
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, MoveSizeCommonRel)//( PFRAME pf, int32_t x, int32_t y, w, y );
{
   MoveSizeCommonRel( (PCOMMON)params[0], params[1], params[2], params[3], params[4] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, GetControl)//( PFRAME pf, int ID );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, FrameBorderX)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, FrameBorderY)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, FrameBorderXOfs)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, FrameBorderYOfs)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}
SERVER_PROC( PCONTROL, OrphanCommon)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, AdoptCommon)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, GetFrameUserData)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}
SERVER_PROC( PCONTROL, SetFrameUserData)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}
SERVER_PROC( PCONTROL, UpdateFrame)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}
SERVER_PROC( PCONTROL, SetFrameMousePosition)
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

//SERVER_PROC void SetDefaultOkayID( PFRAME pFrame, int nID );
//SERVER_PROC void SetDefaultCancelID( PFRAME pFrame, int nID );

//-------- Generic control functions --------------
SERVER_PROC( PFRAME, GetFrame)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, GetNearControl)//( PCONTROL pc, int ID );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, GetCommonTextEx)//( PCONTROL pc, char *buffer, int buflen, int bCString );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetControlText)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetControlFocus)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, EnableControl)//( PCONTROL pc, int bEnable );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( int, IsControlEnabled)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

typedef struct client_init_tag {
   uint32_t pid;
	uintptr_t psvClient;
} CLIENT_INIT, *PCLIENT_INIT;

static int ClientEventInitControl( uintptr_t psv, PCONTROL pc, uint32_t ID )
{
	PCLIENT_INIT init = (PCLIENT_INIT)psv;
   psv = init->psvClient;
	SendServiceEvent( psv, g.MsgBase + MSG_ControlInit
						 , &psv, ParamLength( psv, ID )
						 );
}

SERVER_PROC( PCONTROL, CreateControlExx )//( PFRAME pFrame
                   ////   , int nID
                   //   , int x, int y
                   ///   , int w, int h
                    ///  , int BorderType
                    //  , int extra );
{
	CLIENT_INIT ci;
	ci.pid = params[-1];
	ci.psvClient = params[9];

	result[0] = (uint32_t)CreateControlExx( (PFRAME)params[0] // frame
						, params[1] // attr
						, params[2], params[3] // x, y
						, params[4], params[5] // w, h
						, params[6], params[7] // id, border
                  , params[8] // extra
						, ClientEventInitControl
						, (uintptr_t)&ci
                  DBG_SRC
						 );
   
   *result_length = 4;
   return TRUE;
}

SERVER_PROC( Image,GetControlSurface)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetCommonDraw)//( PCONTROL pc, void (*Draw)//(PCONTROL pc ) );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetCommonMouse)//( PCONTROL pc, void (*MouseMethod)//(PCONTROL pc, int x, int y, int b ) );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetCommonKey) //( PCONTROL pc, void (*KeyMethod)//( PCONTROL pc, int key ) );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, UpdateControl)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( int, GetControlID)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( void, DestroyControlEx)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetNoFocus)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void *, ControlExtraData)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------ General Utilities ------------
// adds OK and Cancel buttons based off the 
// bottom right of the frame, and when pressed set
// either done (cancel) or okay(OK) ... 
// could do done&ok or just done - but we're bein cheesey
SERVER_PROC( void, AddCommonButtonsEx)//( PFRAME pf
                  ///              , int *done, char *donetext
                   //             , int *okay, char *okaytext );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, AddCommonButtons)//( PFRAME pf, int *done, int *okay );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( void, CommonLoop)//( int *done, int *okay ); // perhaps give a callback for within the loop?
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, ProcessControlMessages)//(void);
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

//------ BUTTONS ------------
SERVER_PROC( PCONTROL, MakeButton)//( PFRAME pFrame, int attr, int x, int y, int w, int h
                ///  , uintptr_t nID
                ///  , char *caption
                 // , void (*PushMethod)//(PCONTROL pc, uintptr_t psv)
                 // , uintptr_t Data );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, MakeImageButton)//( PFRAME pFrame, int attr, int x, int y, int w, int h
                //  , Image pImage
                //  , uintptr_t nID
                //  , void (*PushMethod)//(PCONTROL pc, uintptr_t psv)
                //  , uintptr_t Data );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, MakeCustomDrawnButton)//( PFRAME pFrame, int attr, int x, int y, int w, int h
                 // , void (*DrawMethod)//(PCONTROL pc)
                 // , uintptr_t nID
                 /// , void (*PushMethod)//(PCONTROL pc, uintptr_t psv), uintptr_t Data );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, PressButton)//( PCONTROL pc, int bPressed );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( int, IsButtonPressed)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( PCONTROL, MakeCheckButton)//( PFRAME pFrame, int attr, int x, int y, int w, int h
                   ///     , uintptr_t nID, char *text
                   //     , void (*CheckProc)//(PCONTROL pc, uintptr_t psv )
                   //     , uintptr_t psv );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, MakeRadioButton)//( PFRAME pFrame, int attr, int x, int y, int w, int h
						 //  	, uintptr_t nID, uint32_t GroupID, char *text
						 //  	, void (*CheckProc)//(PCONTROL pc, uintptr_t psv )
						 ///  	, uintptr_t psv );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( int, GetCheckState)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetCheckState)//( PCONTROL pc, int nState );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------ Static Text -----------
SERVER_PROC( PCONTROL, MakeTextControl)//( PFRAME pf, int flags, int x, int y, int w, int h
                   //     , uintptr_t nID, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}
SERVER_PROC( void, SetTextControlColors )//( PCONTROL pc, CDATA fore, CDATA back )
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

//------- Edit Control ---------
SERVER_PROC( PCONTROL, MakeEditControl)//( PFRAME pf, int options, int x, int y, int w, int h
                   //     , uintptr_t nID, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

// Use GetControlText/SetControlText

//------- Slider Control --------
SERVER_PROC( PCONTROL, MakeSlider)//( PFRAME pf, int flags, int x, int y, int w, int h, uintptr_t nID, void (*SliderUpdated)//(PCONTROL pc, int val) );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetSliderValues)//( PCONTROL pc, int min, int current, int max );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------- Color Control --------
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
SERVER_PROC( int, PickColor)//( CDATA *result, CDATA original, PFRAME pAbove );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------- ListBox Control --------
SERVER_PROC( PCONTROL, MakeListBox)//( PFRAME pf, int options, int x, int y, int w, int h, uintptr_t nID );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC( void, ResetList)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, AddListItem)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, AddListItemEx)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, InsertListItem)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, InsertListItemEx)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, DeleteListItem)//( PCONTROL pc, PLISTITEM hli );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetItemData)//( PLISTITEM hli, uintptr_t psv );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( uintptr_t, GetItemData)//( PLISTITEM hli );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, GetListItemText)//( PLISTITEM hli, int bufsize, char *buffer );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, GetSelectedItem)//( PCONTROL pc );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetSelectedItem)//( PCONTROL pc, PLISTITEM hli );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetCurrentItem)//( PCONTROL pc, PLISTITEM hli );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, FindListItem)//( PCONTROL pc, char *text );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PLISTITEM, GetNthItem )//( PCONTROL pc, int idx );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC(PFONT, SetSelChangeHandler)//()
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetDoubleClickHandler)//( PCONTROL pc, DoubleClicker proc, uintptr_t psvUser );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------- GridBox Control --------
#ifdef __LINUX__
SERVER_PROC(PCONTROL, MakeGridBox)//( PFRAME pf, int options, int x, int y, int w, int h,
                  //               int viewport_x, int viewport_y, int total_x, int total_y,
                     //            int row_thickness, int column_thickness, uintptr_t nID );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

#endif
//------- Popup Menus ------------
// popup interface.... these are mimics of windows... 
// and at some point should internally alias to popup code - 
//    if I ever get it back

SERVER_PROC( PMENU, CreatePopup)//( void );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, DestroyPopup)//( PMENU pm );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

// get sub-menu data...
SERVER_PROC( void *,GetPopupData)//( PMENU pm, int item );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PMENUITEM, AppendPopupItem)//( PMENU pm, int type, uintptr_t dwID, POINTER pData );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PMENUITEM, CheckPopupItem)//( PMENU pm, uint32_t dwID, uint32_t state );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PMENUITEM, DeletePopupItem)//( PMENU pm, uint32_t dwID, uint32_t state );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( int, TrackPopup)//( PMENU hMenuSub );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------- Color Control --------
// these are basic basic file selection dialogs... 
// the concept is valid, and they should be common like controls...
SERVER_PROC( int, PSI_OpenFile)//( char *basepath, char *types, char *result );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

// this may be used for save I think....
SERVER_PROC( int, PSI_OpenFileEx)//( char *basepath, char *types, char *result, int Create );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


//------- Scroll Control --------
SERVER_PROC( void, SetScrollParams)//( PCONTROL pc, int min, int cur, int range, int max );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( PCONTROL, MakeScrollBar)//( PFRAME pf, int flags, int x, int y, int w, int h, uintptr_t nID  );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SetScrollUpdateMethod)//( PCONTROL pc
					//, void (*UpdateProc)(uintptr_t psv, int type, int current)
					///, uintptr_t data );
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, MoveScrollBar) // ( PCONTROL pc, int method ) - evertthing but drag.
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC(PFONT, PickFont)//()
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC(PFONT, RenderFont)//()
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, SimpleMessageBox ) // PFRAME, char *, char *
{
   *result_length = INVALID_INDEX;
   return TRUE;
}

SERVER_PROC( void, HideFrame ) // PFRAME, LOGICAL bHide
{
   *result_length = INVALID_INDEX;
   return TRUE;
}


SERVER_PROC(PFONT, NotNULL)//()
{
   // ServerNULL function - interesting.
   *result_length = INVALID_INDEX;
   return TRUE;
}


#undef ALIAS_WRAPPER
#define ALIAS_WRAPPER(name) ServerFunctionEntry(Server##name)
#define NUM_FUNCTIONS (sizeof(ControlMessageInterface)/sizeof( server_function))

#define TABLE_SIZE ( sizeof( PSIServerTable ) / sizeof( server_function_table ) )

static SERVER_FUNCTION PSIServerTable[] = {
   ServerFunctionEntry( ControlClientClosed )

  , [MSG_EventUser] =
#include "interface_data.h"
};


int GetControlFunctionTable(server_function_table *ppTable
									,int *nEntries
									,uint32_t MsgBase)
{
	// connect to display, image...
   // set our interfaces to the appropriate interfaces there....
	*ppTable = PSIServerTable;
   *nEntries = TABLE_SIZE;
   g.MsgBase = MsgBase;
   return TRUE;
}

// $Log: psiserver.c,v $
// Revision 1.1  2004/09/19 19:22:32  d3x0r
// Begin version 2 psilib...
//
// Revision 1.13  2004/02/18 09:50:56  d3x0r
// Made mods for better linux compat
//
// Revision 1.12  2003/12/03 16:31:03  panther
// Restore some fixes for dynamic builds
//
// Revision 1.11  2003/10/26 23:48:49  panther
// Linux fixes to interface libs
//
// Revision 1.10  2003/09/25 08:49:31  panther
// Update for compilation for service mode
//
// Revision 1.9  2003/09/19 13:56:44  panther
// Fix typos...
//
// Revision 1.8  2003/09/19 13:42:25  panther
// Well - work on PSI client/server interface...
//
// Revision 1.7  2003/09/18 15:23:00  panther
// Update client/server interface
//
// Revision 1.6  2003/09/11 13:09:25  panther
// Looks like we maintained integrety while overhauling the Make/Create/Init/Config interface for controls
//
// Revision 1.5  2003/05/18 16:22:32  panther
// Handle missing make grid box better
//
// Revision 1.4  2003/05/01 21:55:48  panther
// Update server/client interfaces
//
// Revision 1.3  2003/04/10 00:31:03  panther
// Working on PSI client/server/interface
//
// Revision 1.2  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
