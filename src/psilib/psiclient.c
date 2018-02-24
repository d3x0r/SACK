#include <stdhdrs.h>
#define BASE_CONTROL_MESSAGE_ID g.MsgBase
#include <controls.h>
#include <msgclient.h>
#include <sharemem.h>


#define CONTROL_PROC_DEF( name )                  \
	PSI_CONTROL vConfig##name( PSI_CONTROL pControl, va_list args );  \
	PSI_CONTROL Config##name( PSI_CONTROL pc, ... ); \
	PSI_CONTROL Make##name( PFRAME pFrame, int attr \
	              , int x, int y, int w, int h \
	, uintptr_t nID, ... ) \
	{    \
   return NULL;  \
}                                   \
	PSI_CONTROL Config##name( PSI_CONTROL pControl, ... )           \
	{                                                         \
     va_list ap;                                             \
     va_start(ap, pControl );                                \
	  return vConfig##name( pControl, ap );                   \
	}                                                          \
	PSI_CONTROL vConfig##name( PSI_CONTROL pControl, va_list args )

typedef struct global_tag
{
	struct {
		uint32_t connected : 1;
	} flags;
   uint32_t MsgBase;
} GLOBAL;

static GLOBAL g;

typedef struct client_commmon_control_frame {
   // the handle which the server desires...
	union {
      PFRAME   frame;
		PSI_CONTROL control;
      PSI_CONTROL  common;
	} server;
	// events which will be dispatched potentially to the client...
	// although the client itself may not have these referenced
   // on controls...
   CTEXTSTR DrawThySelfName;
	void CPROC (*DrawThySelf)( uintptr_t, struct client_commmon_control_frame * );
	uintptr_t psvDraw;

   CTEXTSTR MouseMethodName;
	void CPROC (*MouseMethod)( uintptr_t psv, int32_t x, int32_t y, uint32_t b );
	uintptr_t psvMouse;

   CTEXTSTR KeyProcName;
	void CPROC (*KeyProc)( uintptr_t psv, uint32_t );
   uintptr_t psvKey;

   CTEXTSTR SaveProcName;
	int CPROC (*SaveProc)( struct client_commmon_control_frame *, FILE *out );

   CTEXTSTR LoadProcName;
	void CPROC (*LoadProc)( struct client_commmon_control_frame *, FILE *in );

} CLIENT_COMMON, *PCLIENT_COMMON;


typedef struct client_control_tag {
   CLIENT_COMMON common;
	struct {
		uint32_t bInitComplete : 1;
	} flags;
   uintptr_t psvServerControl;
	uint32_t ID;
	ControlInitProc InitProc;
	uintptr_t psvInit;
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

static void ControlEventProcessor( uint32_t EventMsg, uint32_t *data, uint32_t length )
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
			pcc->InitResult = pcc->InitProc( pcc->psvInit, (PSI_CONTROL)pcc, pcc->ID );
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
		//if( InitMessageService() )
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
						   			, uint32_t BorderFlags
						   			, PFRAME hAbove )
{
	PCLIENT_FRAME pcf = Allocate( sizeof( CLIENT_FRAME ) );
	PCLIENT_FRAME pcfAbove = (PCLIENT_FRAME)hAbove;
	uint32_t result;
   uint32_t resultlen = sizeof( pcf->common.server.frame );;
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

PSI_PROC( void, SizeCommon)( PSI_CONTROL pf, uint32_t w, uint32_t h )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_SizeCommon, &pf, ParamLength( pf, h ) );
}

PSI_PROC( void, MoveCommon)( PSI_CONTROL pf, int32_t x, int32_t y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveCommon, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeMoveCommon)( PSI_CONTROL pf, int32_t x, int32_t y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveSizeCommon, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeCommonRel)( PSI_CONTROL pf, uint32_t w, uint32_t h )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_SizeCommonRel, &pf, ParamLength( pf, h ) );
}

PSI_PROC( void, MoveCommonRel)( PSI_CONTROL pf, int32_t x, int32_t y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveCommonRel, &pf, ParamLength( pf, y ) );
}

PSI_PROC( void, SizeMoveCommonRel)( PSI_CONTROL pf, int32_t x, int32_t y )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
   pf = pcf->common.server.frame;
	SendServerMessage( MSG_MoveSizeCommonRel, &pf, ParamLength( pf, y ) );
}
#undef GetControl
PSI_PROC( PSI_CONTROL, GetControl)( PSI_CONTROL pf, int ID )
{
	PCLIENT_FRAME pcf = (PCLIENT_FRAME)pf;
	uint32_t responce
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
PSI_PROC( PFRAME, GetFrame )( PSI_CONTROL pc )
#define GetFrame(c) GetFrame((PSI_CONTROL)pc)
{
   return NULL;
}

PSI_PROC( PSI_CONTROL, GetNearControl)( PSI_CONTROL pc, int ID )
{
   return NULL;
}

PSI_PROC( void, GetControlTextEx)( PSI_CONTROL pc, char *buffer, int buflen, int bCString )
{
}

PSI_PROC( void, SetControlText)( PSI_CONTROL pc, CTEXTSTR text )
{

}

PSI_PROC( void, SetControlFocus)( PFRAME pf, PSI_CONTROL pc )
{
}

PSI_PROC( void, EnableControl)( PSI_CONTROL pc, int bEnable )
{
}

PSI_PROC( int, IsControlEnabled)( PSI_CONTROL pc )
{
   return FALSE;
}

PSI_PROC( PSI_CONTROL, CreateControlExx)( PFRAME pFrame
                                     , uint32_t attr  // attributes (custom use per type)
												 , int x, int y
												 , int w, int h
												 , int nID
												 , int BorderType
												 , int extra
												 , ControlInitProc InitProc
												 , uintptr_t psvInit
												  DBG_PASS)
{
	uint32_t result;
	uint32_t resultlen = sizeof( result );
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
PSI_PROC( PSI_CONTROL, CreateControl)( PFRAME pFrame
                      , int nID
                      , int x, int y
                      , int w, int h
                      , int BorderType
                      , int extra )
{
   return NULL;
}

PSI_PROC( Image,GetControlSurface)( PSI_CONTROL pc )
{
   return NULL;
}

PSI_PROC( void, SetCommonDraw)( PSI_CONTROL pc, void (*Draw)(uintptr_t psv, PSI_CONTROL pc ), uintptr_t psv )
{
}

PSI_PROC( void, SetCommonMouse)( PSI_CONTROL pc, void (*MouseMethod)(uintptr_t pc, int32_t x, int32_t y, uint32_t b ), uintptr_t psv )
{
}

PSI_PROC( void, SetCommonKey )( PSI_CONTROL pc, void (*KeyMethod)( uintptr_t pc, uint32_t key ), uintptr_t psv )
{
}

PSI_PROC( void, UpdateControl)( PSI_CONTROL pc )
{
}

PSI_PROC( int, GetControlID)( PSI_CONTROL pc )
{
   return 0;
}


PSI_PROC( void, DestroyControlEx)( PSI_CONTROL pc DBG_PASS )
{
}

PSI_PROC( void, SetNoFocus)( PSI_CONTROL pc )
{
}

PSI_PROC( void *, ControlExtraData)( PSI_CONTROL pc )
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
					 , void (*PushMethod)(uintptr_t psv, PSI_CONTROL pc)
					 , uintptr_t Data) )
{
   return NULL;
}

CONTROL_PROC_DEF( ImageButton, Image pImage
                  , void (*PushMethod)(uintptr_t psv, PSI_CONTROL pc)
                  , uintptr_t Data )
{
   return NULL;
}

CONTROL_PROC_DEF( CustomDrawnButton, void (*DrawMethod)(uintptr_t, PSI_CONTROL pc)
                  , void (*PushMethod)(uintptr_t psv, PSI_CONTROL pc), uintptr_t Data )
{
   return NULL;
}

PSI_PROC( void, PressButton)( PSI_CONTROL pc, int bPressed )
{
}

PSI_PROC( int, IsButtonPressed)( PSI_CONTROL pc )
{
   return 0;
}


CONTROL_PROC_DEF( CheckButton, void (*CheckProc)(uintptr_t psv, PSI_CONTROL pc)
                        , uintptr_t psv )
{
   return NULL;
}

CONTROL_PROC_DEF( RadioButton, uint32_t GroupID, char *text
						   	, void (*CheckProc)(uintptr_t psv, PSI_CONTROL pc)
						   	, uintptr_t psv )
{
   return NULL;
}

PSI_PROC( int, GetCheckState)( PSI_CONTROL pc )
{
   return 0;
}

PSI_PROC( void, SetCheckState)( PSI_CONTROL pc, int nState )
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
CONTROL_PROC_DEF( Slider, void (*SliderUpdated)(uintptr_t, PSI_CONTROL pc, int val), uintptr_t psv )
{
   return NULL;
}

PSI_PROC( void, SetSliderValues)( PSI_CONTROL pc, int min, int current, int max )
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


PSI_PROC( void, ResetList)( PSI_CONTROL pc )
{
}

PSI_PROC( PLISTITEM, AddListItem)( PSI_CONTROL pc, const char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, AddListItemEx)( PSI_CONTROL pc, int nLevel, const char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, InsertListItem)( PSI_CONTROL pc, PLISTITEM prior, char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, InsertListItemEx)( PSI_CONTROL pc, PLISTITEM prior, int nLevel, char *text )
{
   return NULL;
}

PSI_PROC( void, DeleteListItem)( PSI_CONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( void, SetItemData)( PLISTITEM hli, uintptr_t psv )
{
}

PSI_PROC( uintptr_t, GetItemData)( PLISTITEM hli )
{
   return 0;
}

PSI_PROC( void, GetListItemText)( PLISTITEM hli, int bufsize, char *buffer )
{
}

PSI_PROC( PLISTITEM, GetSelectedItem)( PSI_CONTROL pc )
{
   return 0;
}

PSI_PROC( void, SetSelectedItem)( PSI_CONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( void, SetCurrentItem)( PSI_CONTROL pc, PLISTITEM hli )
{
}

PSI_PROC( PLISTITEM, FindListItem)( PSI_CONTROL pc, char *text )
{
   return NULL;
}

PSI_PROC( PLISTITEM, GetNthItem )( PSI_CONTROL pc, int idx )
{
   return NULL;
}


PSI_PROC( void, SetDoubleClickHandler)( PSI_CONTROL pc, DoubleClicker proc, uintptr_t psvUser )
{
}


//------- GridBox Control --------
#ifdef __LINUX__
PSI_PROC(PSI_CONTROL, MakeGridBox)( PFRAME pf, int options, int x, int y, int w, int h
                               , int viewport_x, int viewport_y, int total_x, int total_y
                               , int row_thickness, int column_thickness, uintptr_t nID )
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

PSI_PROC( PMENUITEM, AppendPopupItem)( PMENU pm, int type, uintptr_t dwID, POINTER pData )
{
   return (PMENUITEM)NULL;
}

PSI_PROC( PMENUITEM, CheckPopupItem)( PMENU pm, uint32_t dwID, uint32_t state )
{
   return (PMENUITEM)NULL;
}

PSI_PROC( PMENUITEM, DeletePopupItem)( PMENU pm, uint32_t dwID, uint32_t state )
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
PSI_PROC( void, SetScrollParams)( PSI_CONTROL pc, int min, int cur, int range, int max )
{
}

CONTROL_PROC_DEF( ScrollBar  )
{
   return NULL;
}

PSI_PROC( void, SetScrollUpdateMethod)( PSI_CONTROL pc
					, void (*UpdateProc)(uintptr_t psv, int type, int current)
					, uintptr_t data )
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
