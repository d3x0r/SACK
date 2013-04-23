#include <stdhdrs.h>
#define BASE_CONTROL_MESSAGE_ID g.MsgBase
#include <controls.h>
#include <msgclient.h>
#include <sharemem.h>


#define CONTROL_PROC_DEF( name )                  \
	PCONTROL vConfig##name( PCONTROL pControl, va_list args );  \
	PCONTROL Config##name( PCONTROL pc, ... ); \
	PCONTROL Make##name( PFRAME pFrame, int attr \
	              , int x, int y, int w, int h \
	, PTRSZVAL nID, ... ) \
	{    \
   return NULL;  \
}                                   \
	PCONTROL Config##name( PCONTROL pControl, ... )           \
	{                                                         \
     va_list ap;                                             \
     va_start(ap, pControl );                                \
	  return vConfig##name( pControl, ap );                   \
	}                                                          \
	PCONTROL vConfig##name( PCONTROL pControl, va_list args )

typedef struct global_tag
{
	struct {
		_32 connected : 1;
	} flags;
   _32 MsgBase;
} GLOBAL;

static GLOBAL g;

typedef struct client_commmon_control_frame {
   // the handle which the server desires...
	union {
      PFRAME   frame;
		PCONTROL control;
      PCOMMON  common;
	} server;
	// events which will be dispatched potentially to the client...
	// although the client itself may not have these referenced
   // on controls...
   CTEXTSTR DrawThySelfName;
	void CPROC (*DrawThySelf)( PTRSZVAL, struct client_commmon_control_frame * );
	PTRSZVAL psvDraw;

   CTEXTSTR MouseMethodName;
	void CPROC (*MouseMethod)( PTRSZVAL psv, S_32 x, S_32 y, _32 b );
	PTRSZVAL psvMouse;

   CTEXTSTR KeyProcName;
	void CPROC (*KeyProc)( PTRSZVAL psv, _32 );
   PTRSZVAL psvKey;

   CTEXTSTR SaveProcName;
	int CPROC (*SaveProc)( struct client_commmon_control_frame *, FILE *out );

   CTEXTSTR LoadProcName;
	void CPROC (*LoadProc)( struct client_commmon_control_frame *, FILE *in );

} CLIENT_COMMON, *PCLIENT_COMMON;


typedef struct client_control_tag {
   CLIENT_COMMON common;
	struct {
		_32 bInitComplete : 1;
	} flags;
   PTRSZVAL psvServerControl;
	_32 ID;
	ControlInitProc InitProc;
	PTRSZVAL psvInit;
   int InitResult;
   struct control_tag *next, **me;
} CLIENT_CONTROL, *PCLIENT_CONTROL;

typedef struct client_frame_tag
{
   CLIENT_COMMON common;
   PCLIENT_CONTROL controls;
   struct client_frame_tag *next, **me;
} CLIENT_FRAME, *PCLIENT_FRAME;

PCLIENT_FRAME pClientFrames;

static void ControlEventProcessor( _32 EventMsg, _32 *data, _32 length )
{
   Log1( WIDE("Event received... %ld"), EventMsg );
	switch( EventMsg )
	{
	case MSG_MateEnded:  // server closed - all client resources defunct.
		{
		}
		break;
	case MSG_DispatchPending: // any pending message can be dipatched now.
		{
		}
		break;
	case MSG_ControlInit:
		{
			PCLIENT_CONTROL pcc = (PCLIENT_CONTROL)data[0];
			pcc->InitResult = pcc->InitProc( pcc->psvInit, (PCONTROL)pcc, pcc->ID );
         pcc->flags.bInitComplete = 1;
		}
      break;
	case MSG_ButtonDraw:
      break;
	case MSG_ButtonClick:
      break;
	default:
		Log2( WIDE("Unknown event message: %ld (%ld bytes)"), EventMsg + g.MsgBase, length );
		break;
	}
}

static int ConnectToServer( void )
{
	if( !g.flags.connected )
	{
		if( InitMessageService() )
		{
			g.MsgBase = LoadService( WIDE("controls"), ControlEventProcessor );
			if( g.MsgBase )
				g.flags.connected = 1;
		}
	}
	if( !g.flags.connected )
		Log( WIDE("Failed to connect") );
   return g.flags.connected;
}


PSI_PROC( PRENDER_INTERFACE, SetControlInterface)( PRENDER_INTERFACE DisplayInterface )
{
	// hmm - server side controls does not use this message
	// same kinda class as 'process messages'
	return DisplayInterface;
}

PSI_PROC( PIMAGE_INTERFACE, SetControlImageInterface )( PIMAGE_INTERFACE DisplayInterface )
{
	// hmm - server side controls does not use this message
	// same kinda class as 'process messages'
   return DisplayInterface;
}


PSI_PROC( void, AlignBaseToWindows)( void )
{
   SendServerMessage( MSG_AlignBaseToWindows, NULL, 0 );
}

// see indexes above.
PSI_PROC( void, SetBaseColor )( INDEX idx, CDATA c )
{
   SendServerMessage( MSG_SetBaseColor, &idx, ParamLength( idx, c ) );
}


//-------- Frame and generic control functions --------------
PSI_PROC( PFRAME, CreateFrame)( CTEXTSTR caption, int x, int y
						   			, int w, int h
						   			, _32 BorderFlags
						   			, PFRAME hAbove )
{
	PCLIENT_FRAME pcf = Allocate( sizeof( CLIENT_FRAME ) );
	PCLIENT_FRAME pcfAbove = (PCLIENT_FRAME)hAbove;
	_32 result;
   _32 resultlen = sizeof( pcf->common.server.frame );;
	if( TransactServerMultiMessage( MSG_CreateFrame, 3
											, &result, &pcf->common.server.frame, &resultlen
											, &x, ParamLength( x, BorderFlags )
											, &pcfAbove->common.server.frame, sizeof( pcfAbove->common.server.frame )
											, caption, strlen( caption ) + 1 ) &&
		( result == (MSG_CreateFrame | SERVER_SUCCESS) ) )
	{
		if( ( pcf->next = pClientFrames ) )
         pcf->next->me = &pcf->next;
		pcf->me = &pClientFrames;
		pClientFrames = pcf;
      return pcf;
	}
	else
      Release( pcf );
   return NULL;
}


PSI_PROC( void, DestroyFrameEx)( PFRAME pf DBG_PASS )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
	SendServerMessage( MSG_DestroyFrameEx, &pcf->common.server.frame, sizeof( pcf->common.server.frame ) );
	if( ( (*pcf->me) = pcf->next ) )
      pcf->next->me = pcf->me;
}


PSI_PROC( void, DisplayFrame)( PFRAME pf )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
	SendServerMessage( MSG_DisplayFrame, &pcf->common.server.frame, sizeof( pcf->common.server.frame ) );
}

PSI_PROC( void, SizeCommon)( PCOMMON pf, _32 w, _32 h )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_SizeCommon, &pf, ParamLength( pf, h ) );
}

PSI_PROC( void, MoveCommon)( PCOMMON pf, S_32 x, S_32 y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveCommon, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeMoveCommon)( PCOMMON pf, S_32 x, S_32 y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveSizeCommon, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeCommonRel)( PCOMMON pf, _32 w, _32 h )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_SizeCommonRel, &pf, ParamLength( pf, h ) );
}

PSI_PROC( void, MoveCommonRel)( PCOMMON pf, S_32 x, S_32 y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveCommonRel, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeMoveCommonRel)( PCOMMON pf, S_32 x, S_32 y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveSizeCommonRel, &pf, ParamLength( pf, y ) );
}
#undef GetControl
PSI_PROC( PCONTROL, GetControl)( PCOMMON pf, int ID )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
	_32 responce
, control
, result_length = sizeof( control );
	pf = pcf->common.server.frame;
	if( TransactServerMessage( MSG_GetControl, &pf, ParamLength( pf, ID )
									 , &responce, &control, &result_length ) &&
		responce == ( MSG_GetControl | SERVER_SUCCESS ) )
	{
		return control;
	}
   return NULL;
}

//PSI_PROC void SetDefaultOkayID( PFRAME pFrame, int nID )
//PSI_PROC void SetDefaultCancelID( PFRAME pFrame, int nID )

//-------- Generic control functions --------------
#undef GetFrame
PSI_PROC( PFRAME, GetFrame )( PCOMMON pc )
#define GetFrame(c) GetFrame((PCOMMON)pc)
{
   return NULL;
}

PSI_PROC( PCONTROL, GetNearControl)( PCONTROL pc, int ID )
{
   return NULL;
}

PSI_PROC( void, GetControlTextEx)( PCONTROL pc, char *buffer, int buflen, int bCString )
{
}

PSI_PROC( void, SetControlText)( PCONTROL pc, CTEXTSTR text )
{

}

PSI_PROC( void, SetControlFocus)( PFRAME pf, PCONTROL pc )
{
}

PSI_PROC( void, EnableControl)( PCONTROL pc, int bEnable )
{
}

PSI_PROC( int, IsControlEnabled)( PCONTROL pc )
{
   return FALSE;
}

PSI_PROC( PCONTROL, CreateControlExx)( PFRAME pFrame
                                     , _32 attr  // attributes (custom use per type)
												 , int x, int y
												 , int w, int h
												 , int nID
												 , int BorderType
												 , int extra
												 , ControlInitProc InitProc
												 , PTRSZVAL psvInit
												  DBG_PASS)
{
	_32 result;
	_32 resultlen = sizeof( result );
	PCLIENT_CONTROL pcc = Allocate( sizeof( CLIENT_CONTROL ) );
   MemSet( &pcc, 0, sizeof( CLIENT_CONTROL ) );
	pcc->InitProc = InitProc;
	pcc->psvInit = 0;
	if( TransactServerMultiMessage( MSG_CreateControlExx, 3
										   , &result, &pcc->common.server.frame, &resultlen
											, &attr, ParamLength( attr, BorderType )
											, &psvInit, ParamLength( psvInit, psvInit ) ) &&
		( result == (MSG_CreateFrame | SERVER_SUCCESS) ) )
	{
		while( !pcc->flags.bInitComplete )
			Relinquish();
      // and now - what about pcc->InitResult?
	}
   return NULL;
}

#undef CreateControl
PSI_PROC( PCONTROL, CreateControl)( PFRAME pFrame
                      , int nID
                      , int x, int y
                      , int w, int h
                      , int BorderType
                      , int extra )
{
   return NULL;
}

PSI_PROC( Image,GetControlSurface)( PCONTROL pc )
{
   return NULL;
}

PSI_PROC( void, SetCommonDraw)( PCOMMON pc, void (*Draw)(PTRSZVAL psv, PCONTROL pc ), PTRSZVAL psv )
{
}

PSI_PROC( void, SetCommonMouse)( PCOMMON pc, void (*MouseMethod)(PTRSZVAL pc, S_32 x, S_32 y, _32 b ), PTRSZVAL psv )
{
}

PSI_PROC( void, SetCommonKey )( PCOMMON pc, void (*KeyMethod)( PTRSZVAL pc, _32 key ), PTRSZVAL psv )
{
}

PSI_PROC( void, UpdateControl)( PCONTROL pc )
{
}

PSI_PROC( int, GetControlID)( PCONTROL pc )
{
   return 0;
}


PSI_PROC( void, DestroyControlEx)( PCONTROL pc DBG_PASS )
{
}

PSI_PROC( void, SetNoFocus)( PCONTROL pc )
{
}

PSI_PROC( void *, ControlExtraData)( PCONTROL pc )
{
   return NULL;
}


//------ General Utilities ------------
// adds OK and Cancel buttons based off the 
// bottom right of the frame, and when pressed set
// either done (cancel) or okay(OK) ... 
// could do done&ok or just done - but we're bein cheesey
PSI_PROC( void, AddCommonButtonsEx)( PFRAME pf
                                , int *done, char *donetext
                                , int *okay, char *okaytext )
{
}

PSI_PROC( void, AddCommonButtons)( PFRAME pf, int *done, int *okay )
{
}


PSI_PROC( void, CommonLoop)( int *done, int *okay ) // perhaps give a callback for within the loop?
{
}

PSI_PROC( void, ProcessControlMessages)(void)
{
}

//------ BUTTONS ------------
CONTROL_PROC_DEF( Button, (char *caption
					 , void (*PushMethod)(PTRSZVAL psv, PCONTROL pc)
					 , PTRSZVAL Data) )
{
   return NULL;
}

CONTROL_PROC_DEF( ImageButton, Image pImage
                  , void (*PushMethod)(PTRSZVAL psv, PCONTROL pc)
                  , PTRSZVAL Data )
{
   return NULL;
}

CONTROL_PROC_DEF( CustomDrawnButton, void (*DrawMethod)(PTRSZVAL, PCONTROL pc)
                  , void (*PushMethod)(PTRSZVAL psv, PCONTROL pc), PTRSZVAL Data )
{
   return NULL;
}

PSI_PROC( void, PressButton)( PCONTROL pc, int bPressed )
{
}

PSI_PROC( int, IsButtonPressed)( PCONTROL pc )
{
   return 0;
}


CONTROL_PROC_DEF( CheckButton, void (*CheckProc)(PTRSZVAL psv, PCONTROL pc)
                        , PTRSZVAL psv )
{
   return NULL;
}

CONTROL_PROC_DEF( RadioButton, _32 GroupID, char *text
						   	, void (*CheckProc)(PTRSZVAL psv, PCONTROL pc)
						   	, PTRSZVAL psv )
{
   return NULL;
}

PSI_PROC( int, GetCheckState)( PCONTROL pc )
{
   return 0;
}

PSI_PROC( void, SetCheckState)( PCONTROL pc, int nState )
{
   return;
}


//------ Static Text -----------
CONTROL_PROC_DEF( TextControl, CTEXTSTR text )
{
   return NULL;
}


//------- Edit Control ---------
CONTROL_PROC_DEF( EditControl, CTEXTSTR text )
{
   return NULL;
}

// Use GetControlText/SetControlText

//------- Slider Control --------
CONTROL_PROC_DEF( Slider, void (*SliderUpdated)(PTRSZVAL, PCONTROL pc, int val), PTRSZVAL psv )
{
   return NULL;
}

PSI_PROC( void, SetSliderValues)( PCONTROL pc, int min, int current, int max )
{
}


//------- Color Control --------
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
PSI_PROC( int, PickColor)( CDATA *result, CDATA original, PFRAME pAbove )
{
   return 0;
}


//------- ListBox Control --------
CONTROL_PROC_DEF( ListBox )
{
   return NULL;
}


PSI_PROC( void, ResetList)( PCONTROL pc )
{
}

PSI_PROC( PLISTITEM, AddListItem)( PCONTROL pc, const char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, AddListItemEx)( PCONTROL pc, int nLevel, const char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, InsertListItem)( PCONTROL pc, PLISTITEM prior, char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, InsertListItemEx)( PCONTROL pc, PLISTITEM prior, int nLevel, char *text )
{
   return NULL;
}

PSI_PROC( void, DeleteListItem)( PCONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( void, SetItemData)( PLISTITEM hli, PTRSZVAL psv )
{
}

PSI_PROC( PTRSZVAL, GetItemData)( PLISTITEM hli )
{
   return 0;
}

PSI_PROC( void, GetListItemText)( PLISTITEM hli, int bufsize, char *buffer )
{
}

PSI_PROC( PLISTITEM, GetSelectedItem)( PCONTROL pc )
{
   return 0;
}

PSI_PROC( void, SetSelectedItem)( PCONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( void, SetCurrentItem)( PCONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( PLISTITEM, FindListItem)( PCONTROL pc, char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, GetNthItem )( PCONTROL pc, int idx )
{
   return NULL;
}


PSI_PROC( void, SetDoubleClickHandler)( PCONTROL pc, DoubleClicker proc, PTRSZVAL psvUser )
{
}


//------- GridBox Control --------
#ifdef __LINUX__
PSI_PROC(PCONTROL, MakeGridBox)( PFRAME pf, int options, int x, int y, int w, int h
                               , int viewport_x, int viewport_y, int total_x, int total_y
                               , int row_thickness, int column_thickness, PTRSZVAL nID )
{
   return NULL;
}

#endif
//------- Popup Menus ------------
// popup interface.... these are mimics of windows... 
// and at some point should internally alias to popup code - 
//    if I ever get it back

PSI_PROC( PMENU, CreatePopup)( void )
{
   return (PMENU)NULL;
}

PSI_PROC( void, DestroyPopup)( PMENU pm )
{
}

// get sub-menu data...
PSI_PROC( void *,GetPopupData)( PMENU pm, int item )
{
}

PSI_PROC( PMENUITEM, AppendPopupItem)( PMENU pm, int type, PTRSZVAL dwID, POINTER pData )
{
   return (PMENUITEM)NULL;
}

PSI_PROC( PMENUITEM, CheckPopupItem)( PMENU pm, _32 dwID, _32 state )
{
   return (PMENUITEM)NULL;
}

PSI_PROC( PMENUITEM, DeletePopupItem)( PMENU pm, _32 dwID, _32 state )
{
   return (PMENUITEM)NULL;
}

PSI_PROC( int, TrackPopup)( PMENU hMenuSub, PFRAME parent )
{
   return 0;
}


//------- Color Control --------
// these are basic basic file selection dialogs... 
// the concept is valid, and they should be common like controls...
PSI_PROC( int, PSI_OpenFile)( char *basepath, char *types, char *result )
{
   return 0;
}

// this may be used for save I think....
PSI_PROC( int, PSI_OpenFileEx)( char *basepath, char *types, char *result, int Create )
{
   return 0;
}


//------- Scroll Control --------
#define SCROLL_HORIZONTAL 1
#define SCROLL_VERITCAL   0
PSI_PROC( void, SetScrollParams)( PCONTROL pc, int min, int cur, int range, int max )
{
}

CONTROL_PROC_DEF( ScrollBar  )
{
   return NULL;
}

PSI_PROC( void, SetScrollUpdateMethod)( PCONTROL pc
					, void (*UpdateProc)(PTRSZVAL psv, int type, int current)
					, PTRSZVAL data )
{
}





// $Log: psiclient.c,v $
// Revision 1.1  2004/09/19 19:22:32  d3x0r
// Begin version 2 psilib...
//
// Revision 1.23  2004/09/13 09:12:40  d3x0r
// Simplify procregsitration, standardize registration, cleanups...
//
// Revision 1.22  2004/09/02 10:22:53  d3x0r
// tweaks for linux build
//
// Revision 1.21  2004/06/16 10:27:36  d3x0r
// Added key events to display library...
//
// Revision 1.20  2003/10/26 23:48:49  panther
// Linux fixes to interface libs
//
// Revision 1.19  2003/10/12 02:47:05  panther
// Cleaned up most var-arg stack abuse ARM seems to work.
//
// Revision 1.18  2003/09/25 08:49:31  panther
// Update for compilation for service mode
//
// Revision 1.17  2003/09/19 13:56:43  panther
// Fix typos...
//
// Revision 1.16  2003/09/19 13:42:25  panther
// Well - work on PSI client/server interface...
//
// Revision 1.15  2003/09/18 15:23:00  panther
// Update client/server interface
//
// Revision 1.14  2003/09/18 07:42:48  panther
// Changes all across the board...image support, display support, controls editable in psi...
//
// Revision 1.13  2003/09/11 13:09:25  panther
// Looks like we maintained integrety while overhauling the Make/Create/Init/Config interface for controls
//
// Revision 1.12  2003/05/01 21:55:48  panther
// Update server/client interfaces
//
// Revision 1.11  2003/04/24 16:32:50  panther
// Initial calibration support in display (incomplete)...
// mods to support more features in controls... (returns set interface)
// Added close service support to display_server and display_image_server
//
// Revision 1.10  2003/03/31 23:39:53  panther
// Clean up some passing of string fucntions to be CTEXTSTR
//
// Revision 1.9  2003/03/30 21:38:54  panther
// Fix MSG_ definitions.  Fix lack of anonymous unions
//
// Revision 1.8  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
