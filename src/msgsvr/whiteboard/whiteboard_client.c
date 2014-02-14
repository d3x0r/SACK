// if you don't want a psi control registered for
// whiteboard - such as registering your own, comment
// this out, and the control portion goes away, leaving
// all whiteboard access methods...
#define IMPLEMENT_DEFAULT_WHITEBOARD_CONTROL
#include <stdhdrs.h>
#include <sharemem.h>
#define USE_IMAGE_INTERFACE l.pii
#include <psi.h>
#include <image.h>
#include <msgclient.h>

#include "whiteboard.h"

#ifdef IMPLEMENT_DEFAULT_WHITEBOARD_CONTROL
   static CONTROL_REGISTRATION whiteboard;
#  define WHITEBOARD_CONTROL_ID whiteboard.TypeID
#  define WHITEBOARD_CONTROL(pc,pcw) ValidatedControlData( PWHITEBOARD_CONTROL, WHITEBOARD_CONTROL_ID, pcw, pc )
#endif

typedef struct local_tag
{
   PIMAGE_INTERFACE pii;
	_32 ServerBaseMsgID;
   BOARD_CLIENTSET clients; // people who are in this system
   WHITEBOARDSET boards;
} LOCAL;

typedef struct control_data_tag
{
	_32 BoardID;
	_32 _b;
   enum SHAPE_TYPE iCurrentShape;
} *PWHITEBOARD_CONTROL, WHITEBOARD_CONTROL;

static LOCAL l;

static int CPROC HandleBoardEvents( _32 MsgID, _32 *params, _32 paramlen )
{
	switch( MsgID )
	{
	case WB_LOGIN:
		// a new user has joined the cluster...
		// this notification could be disabled I suppose... and only
		// provided on-demand by user... or ... shrug...
		{
			PBOARD_CLIENT client = GetSetMember( BOARD_CLIENT, &l.clients, params[0] );
         client->name = StrDup( (TEXTSTR)(params + 1) );
		}
      break;
	case WB_CREATE_LAYER:
		{

		}
      break;
	case WB_ADD_POINT:
      break;
	case WB_ADD_SEGMENT:
      break;
	default:
		lprintf( WIDE("Received unknown event from whiteboard %ld ... params follow"), MsgID );
		LogBinary( (POINTER)params, paramlen );
      break;
	}
   return EVENT_HANDLED;
}

static int Init( void )
{
	if( !l.ServerBaseMsgID )
		l.ServerBaseMsgID = LoadService( WIDE("Whiteboard Service"), HandleBoardEvents );
	if( !l.ServerBaseMsgID )
		return FALSE;
   l.pii = GetImageInterface();
   return TRUE;
}

INDEX OpenBoard( char *name )
{
   _32 result;
	_32 data[1];
	_32 datalen = sizeof( data );
	if( !Init() )
		return INVALID_INDEX;
	// as soon as open board is called, and
	// remotely possible that before we get the responce
	// that data will already be coming in for the board.
	// the board must 1> exist. 2> be ready.  events MUST wait
   // until the board is marked ready.
	if( TransactServerMessage( l.ServerBaseMsgID + WB_OPEN_BOARD
											 , name, strlen( name )
											 , &result, &data, &datalen )
	  && ( result == ((WB_OPEN_BOARD+l.ServerBaseMsgID)|SERVER_SUCCESS) ) )
	{
		PWHITEBOARD board = GetSetMember( WHITEBOARD, &l.boards, data[0] );
		MemSet( board, 0, sizeof( WHITEBOARD ) );
		board->flags.bReady = 1;
		if( board->flags.bWaiting )
         Relinquish();
      return data[0];
	}
   return INVALID_INDEX;
}

void CPROC WhiteboardBeginShape( INDEX iBoard, enum SHAPE_TYPE type, S_32 x, S_32 y )
{
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD, &l.boards, iBoard );
	if( board )
	{
		SendServerMultiMessage( l.ServerBaseMsgID
											 + WB_FIRST_VECTOR
											 , 1
											 , &iBoard, ParamLength( iBoard, y ) );
	}
}

void CPROC WhiteboardContinueShape( INDEX iBoard, S_32 x, S_32 y )
{
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD, &l.boards, iBoard );
	if( board )
	{
		SendServerMultiMessage( l.ServerBaseMsgID
											 + WB_ADD_VECTOR
											 , 1
											 , &iBoard, ParamLength( iBoard, y ) );
	}
}

void CPROC WhiteboardEndShape( INDEX iBoard, S_32 x, S_32 y )
{
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD, &l.boards, iBoard );
	if( board )
	{
		SendServerMultiMessage( l.ServerBaseMsgID
											 + WB_END_VECTOR
											 , 1
											 , &iBoard, ParamLength( iBoard, y ) );
	}
}

//--------------------------------------------------------------------
// control library
//--------------------------------------------------------------------
#ifdef IMPLEMENT_DEFAULT_WHITEBOARD_CONTROL

static PTRSZVAL CPROC DrawSegment( void *pointer, PTRSZVAL psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	WHITEBOARD_CONTROL( pc, pcw );
	PWHITEBOARD board = pcw?GetUsedSetMember( WHITEBOARD
														 , &l.boards
														 , pcw->BoardID )
		: NULL;
   PBOARD_SEGMENT segment = (PBOARD_SEGMENT)pointer;
	if( pcw && board )
	{
		Image surface = GetControlSurface( pc );
		PBOARD_POINT point, _point = NULL;
      INDEX idx;
		LIST_FORALL( segment->points, idx, PBOARD_POINT, point )
		{
         if( _point )
				do_line( surface
						 , _point->x, _point->y
						 , point->x, point->y
						 , segment->color?segment->color:BASE_COLOR_WHITE );
         _point = point;
		}
	}
   return 0;
}

static PTRSZVAL CPROC DrawALayer( void *pointer, PTRSZVAL psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	WHITEBOARD_CONTROL( pc, pcw );
	PWHITEBOARD board = pcw?GetUsedSetMember( WHITEBOARD
														 , &l.boards
														 , pcw->BoardID )
		: NULL;
	PBOARD_LAYER layer = (PBOARD_LAYER)pointer;
	if( pcw && board )
	{
      ForAllInSet( BOARD_SEGMENT, layer->segments, DrawSegment, (PTRSZVAL)pc );
	}
   return 0;
}

static int OnDrawCommon( "Whiteboard Client Surface" ) /*CPROC DrawWhiteboardSurface*/( PSI_CONTROL pc )
{
	WHITEBOARD_CONTROL( pc, pcw );
	if( pcw )
	{
		PWHITEBOARD board = GetUsedSetMember( WHITEBOARD
														, &l.boards, pcw->BoardID );
		if( board )
		{
         ForAllInSet( BOARD_LAYER, &board->layers, DrawALayer, (PTRSZVAL)pc );
		}
      //EnumerateVectors( pcw->BoardID );
	}
   return 1;
}

static int OnMouseCommon( "Whiteboard Client Surface" )/*CPROC MouseWhiteboardSurface*/( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	WHITEBOARD_CONTROL( pc, pcw );
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD
													, &l.boards, pcw->BoardID );
	if( pcw )
	{
      _32 _b = pcw->_b;
		if( b == MK_NO_BUTTON )
		{
		// unselect, drop stuff, etc...
		// the mouse was down, and as it leaves the surface,
		// this event is given...
			return 0;
		}
#define BUTTON_DOWN(which) (((b)&(which)) && !((_b)&(which)))
#define BUTTON_UP(which) (((_b)&(which)) && !((b)&(which)))
#define BUTTON_IS_DOWN(which) (((b)&(which)) && ((_b)&(which)))
		if( board )
		{
			if( BUTTON_DOWN( MK_LBUTTON ) )
			{
			// flop the y axis so that - is down and + is up
				WhiteboardBeginShape( pcw->BoardID
										  , pcw->iCurrentShape
										  , x - board->org_x
										  , board->org_y - y );
			}
			else if( BUTTON_UP( MK_LBUTTON ) )
			{
				WhiteboardContinueShape( pcw->BoardID
											  , x - board->org_x, board->org_y - y );
			}
			else if( BUTTON_IS_DOWN( MK_LBUTTON ) )
			{
				WhiteboardEndShape( pcw->BoardID
										, x - board->org_x, board->org_y - y );
			}
		}
		pcw->_b = b;
	}
   return 1;
}

static int OnCreateCommon( "Whiteboard Client Surface" )( PSI_CONTROL pc )
//CPROC InitWhiteboardSurface( PSI_CONTROL pc )
{
	WHITEBOARD_CONTROL( pc, pcw );
	if( pcw )
	{
		// need a way to specify this....
      // maybe display a user choice box?
      pcw->BoardID = OpenBoard( WIDE("A Board") );
		if( pcw->BoardID == INVALID_INDEX )
         return FALSE;
		return TRUE;
	}
   return FALSE;
}

static CONTROL_REGISTRATION whiteboard = { "Whiteboard Client Surface", { { 512, 420 }, sizeof( WHITEBOARD_CONTROL ), BORDER_NORMAL }
};

PRELOAD( RegisterWhiteboardControl )
{
   DoRegisterControl( &whiteboard );
}


#endif

