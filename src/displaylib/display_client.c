#include <stdhdrs.h>
#include <sys/types.h>

#include <sack_types.h>
#include <idle.h>
#include <sharemem.h>

#define g global_display_client_data
#define GLOBAL_STRUCTURE_DEFINED
#define DEFINE_RENDER_PROTOCOL

#define MyInterface render_interface_tag
#define BASE_MESSAGE_ID g.MsgBase
#define DISPLAY_CLIENT
#include "client.h"


#define OPTION_FLUSH_ON_UPDATE
//#include <render.h>

#include <msgclient.h>

RENDER_NAMESPACE

// This module is all the functions of displaylib native linking methods

static int CPROC DisplayEventProcessor( _32 EventMsg, _32 *data, _32 length )
{
   //Log1( WIDE("Event received... %d"), EventMsg );
	switch( EventMsg )
	{
	case MSG_DispatchPending: // any pending message can be dipatched now.
		{
			PDISPLAY display;
         INDEX idx;
			LIST_FORALL( g.pDisplayList, idx, PDISPLAY, display )
			{
				if( display->flags.mouse_pending )
				{
					display->flags.mouse_pending = 0;
					// mouse events are dispatched in screen coordinates
					// and are modified according to the local specs...
					// this SHOULD counteract the drift observed in X
               // with client implementations...
					display->MouseMethod( display->MouseData
											  , display->mouse_pending.x - display->x
											  , display->mouse_pending.y - display->y
											  , display->mouse_pending.b );
               if( !display->flags.draw_pending )
						SendServerMessage( MSG_OkaySyncRender, &display->hDisplay, sizeof( display->hDisplay ) );
				}
				if( display->flags.draw_pending )
				{
					display->flags.draw_pending = 0;
               display->flags.redraw_dispatched = 1;
               g.flags.redraw_dispatched = 1;
					//Log( WIDE("Calling Redraw Method (client)") );
               //lprintf( WIDE("Redraw to be performed by the application, resulting with a SyncRender.") );
					display->RedrawMethod( display->RedrawData, (PRENDERER)display );
					//lprintf( WIDE("Redraw performed to the application, resulting with a SyncRender.") );
					{
						SendServerMessage( MSG_OkaySyncRender, &display->hDisplay, sizeof( display->hDisplay ) );
					   //SyncRender( display );
					}
               display->flags.redraw_dispatched = 0;
               g.flags.redraw_dispatched = 0;
				}
			}
		}
      break;
	case MSG_RedrawMethod:
		{
			PDISPLAY display = ((PDISPLAY*)(data+0))[0];
			if( FindLink( &g.pDisplayList, display ) == INVALID_INDEX )
            break;
			display->flags.draw_pending = 1;
         //Log( WIDE("Enable a delayed redraw - collect events...") );
			// data[1] would be psvSelf (unused)
			// psvself is different on this side anyhow so....
         if( display->DisplayImage )
				MemCpy( display->DisplayImage
						, (data+((sizeof( PTRSZVAL )*2)/sizeof(*data)))
						, sizeof( IMAGE_RECTANGLE ) );
			else
				lprintf( WIDE("Display image surface was never retrieved...") );
			return EVENT_WAIT_DISPATCH;

		}
      break;
	case MSG_MouseMethod:
		//Log4( WIDE("Mouse method: %p %ld,%ld %08lx"), (POINTER)data[0], data[1], data[2], data[3] );
		{
			PDISPLAY display = ((PDISPLAY*)(data+0))[0];
			if( FindLink( &g.pDisplayList, display ) == INVALID_INDEX )
            break;
			if( display->MouseMethod )
			{
				// if already pending, check to see if the buttons have changed.
				if( display->flags.mouse_pending
					&&( ( data[3] != display->mouse_pending.b )
					   || ( data[3] & MK_OBUTTON ) ) )
				{
					// any mouse button change MUST be dispatched.
               // (dispatch prior, collect current)
					display->MouseMethod( display->MouseData
											 , display->mouse_pending.x
											 , display->mouse_pending.y
											 , display->mouse_pending.b );
				}
				if( data[3] & MK_OBUTTON )
				{
					MemCpy( display->KeyboardState
						   , data+4
						   , sizeof( display->KeyboardState ) );
				}
				display->flags.mouse_pending = 1;
				display->mouse_pending.x = data[1];
				display->mouse_pending.y = data[2];
				display->mouse_pending.b = data[3];
            return EVENT_WAIT_DISPATCH;
			}
			else
			{
            Log( WIDE("Not dispatched!") );
			}
		}
		break;
	case MSG_KeyMethod:
		//Log3( WIDE("Key Method: %p %08lX (%d)"), (POINTER)data[0], data[1], length );
      // now we should have the full keyboard state....
		{
			_32 key = data[1];
         _32 scancode = ( key & 0xFF0000 ) >> 16;
			_32 keymod = KEY_MOD( key );
			PDISPLAY display = ((PDISPLAY*)(data+0))[0];
			if( FindLink( &g.pDisplayList, display ) == INVALID_INDEX )
            break;
			MemCpy( display->KeyboardState, data+2, sizeof( display->KeyboardState ) );

         //lprintf( WIDE("Checking for key at %d %d"), scancode, keymod, g.KeyDefs[scancode].mod[keymod].flags.bFunction );
			if( ( key & KEY_PRESSED ) && g.KeyDefs[scancode].mod[keymod].flags.bFunction )
			{
				INDEX idx;
				PKEY_FUNCTION keyfunc;
				//lprintf( WIDE("Invokeing registered key trigger function.") );
				LIST_FORALL( g.KeyDefs[scancode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
				{
					keyfunc->data.trigger( keyfunc->psv
												, key );
				}
			}
			else if( display )
			{
			   //lprintf( WIDE("checking for a local function...") );
				//lprintf( WIDE("Checking for key at %d %d"), scancode, keymod, g.KeyDefs[scancode].mod[keymod].flags.bFunction );
				if( ( key & KEY_PRESSED ) &&
					display->KeyDefs[scancode].mod[keymod].flags.bFunction )
				{
					INDEX idx;
					PKEY_FUNCTION keyfunc;
               //lprintf( WIDE("Dispatch event...") );
					LIST_FORALL( g.KeyDefs[scancode].mod[keymod].key_procs, idx, PKEY_FUNCTION, keyfunc )
					{
						keyfunc->data.trigger( keyfunc->psv, key );
					}
				}
				else
				{
					PPANEL panel = display;
					//lprintf( WIDE("No predfined events associated... dispatch event...") );
					if( display->KeyMethod )
					{
						panel->KeyMethod( panel->KeyData, key );
					}
					else if( panel->MouseMethod )
					{
						panel->MouseMethod( panel->MouseData
												, display->mouse_pending.x
												, display->mouse_pending.y
												, display->mouse_pending.b | MK_OBUTTON ); // other button.
					}
					// okay- wonder what this does that's so magical...
					SendServerMessage( MSG_OkaySyncRender, &display->hDisplay, sizeof( display->hDisplay ) );
				}
			}
		}
		break;
	case MSG_LoseFocusMethod:
		{
			PDISPLAY display = ((PDISPLAY*)(data+0))[0];
         PRENDERER displaygain;
			if( FindLink( &g.pDisplayList, display ) == INVALID_INDEX )
            break;
			//Log3( WIDE("Generating a focus event? %p %p %p")
			//	 , display
			//	 , display->LoseFocusData
			//	 , data[1]
			//	 );
			{
				PDISPLAY tmp;
				INDEX idx;
				if( data[1] )
				{
               displaygain = (PRENDERER)1;
					LIST_FORALL( g.pDisplayList, idx, PDISPLAY, tmp )
					{
						if( tmp->hDisplay == *(PRENDERER*)(data+(sizeof(PDISPLAY)/sizeof(_32))) )
						{
							displaygain = (PRENDERER)tmp;
							break;
						}
					}
				}
				else
               displaygain = NULL; // SET focus.
			}
			if( display->LoseFocusMethod )
            display->LoseFocusMethod( display->LoseFocusData, (PRENDERER)displaygain );
		}
      break;
	default:
		Log2( WIDE("Unknown event message: %") _32f WIDE(" (%") _32f WIDE(" bytes)"), EventMsg + g.MsgBase, length );
		break;
	}

	// no result ever.
   return 0;
}

static void DisconnectFromServer( void )
{
	if( g.flags.connected )
	{
      Log( WIDE("Disconnecting from server (display)") );
		UnloadService( g.MsgBase );
		g.flags.connected = 0;
      g.MsgBase = 0;
	}
}

RENDER_PROC( int, ProcessDisplayMessages )( void )
{
	// this needs to call over to the client message receiver thread thing
	// since that thread may invoke an event which in turn causes a thing
	// which waits and must call this get message dispatch.
   //Log( WIDE("Process client messages..") );
   return ProcessClientMessages(0);
}

static int CPROC ConnectToServer( void )
{
	if( g.flags.disconnected )
      return FALSE;
   AddIdleProc( (IdleProc)ProcessDisplayMessages, 0 );
	if( !g.flags.connected )
	{
		if( InitMessageService() )
		{
			g.MsgBase = LoadService( WIDE("display"), DisplayEventProcessor );
			if( g.MsgBase != INVALID_INDEX )
			{
				g.flags.connected = 1;
            g.KeyDefs = CreateKeyBinder();
			}
		}
	}
	if( !g.flags.connected )
		Log( WIDE("Failed to connect") );
   return g.flags.connected;
}


RENDER_PROC( void, SetApplicationTitle )(const char *title )
{
	if( ConnectToServer() )
	{
		if( TransactServerMessage( MSG_SetApplicationTitle, (_32*)title, strlen( title ) + 1
										 , NULL, NULL, NULL ) )
		{
         Log( WIDE("Application Set Title True...") );
		}
	}

   return; // has no meaning yet. .
}

RENDER_PROC (void, SetRendererTitle) (PRENDERER hVideo, const TEXTCHAR *pTitle)
{
	//local_vidlib.gpTitle = pTitle;
	//if (local_vidlib.hWndInstance)
lprintf( "Set the arenderer's title so it shows in X?" );
}        



RENDER_PROC( void, SetApplicationIcon  )(Image Icon)
{
   return; // has no meaning yet...
}

RENDER_PROC( void, GetDisplaySize  )( _32 *width, _32 *height )
{
	_32 Responce;
	_32 data[2];
   _32 len = 8;
	if( ConnectToServer() )
	{
		TransactServerMessage( MSG_GetDisplaySize, NULL, 0
							      , &Responce, data, &len );
		if( len == 8 )
		{
         if( width )
				*width = data[0];
         if( height )
				*height = data[1];
		}
	}
}

RENDER_PROC( void, SetDisplaySize )( _32 width, _32 height )
{
   SendServerMessage( MSG_SetDisplaySize, &width, 8 );
}


RENDER_PROC( PRENDERER, OpenDisplaySizedAt )   ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
	if( ConnectToServer() )
	{
		_32 Responce;
		_32 data[5];
      _32 len = 20;
		if( TransactServerMessage( MSG_OpenDisplaySizedAt, &attributes, 5 * sizeof( _32 )
										 , &Responce, data, &len ) &&
			Responce == (MSG_OpenDisplaySizedAt|SERVER_SUCCESS) )
		{
			PDISPLAY display = (PDISPLAY)Allocate( sizeof( DISPLAY ) );
			MemSet( display, 0, sizeof( DISPLAY ) );
			display->hDisplay = *(PRENDERER*)(data+0);
         MemCpy( &display->x, data + 1, 16 );
         AddLink( &g.pDisplayList, display );
			return (PRENDERER)display;
		}
	}
   return (PRENDERER)NULL;
}

RENDER_PROC( PRENDERER, OpenDisplayAboveSizedAt )   ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER parent )
{
	if( ConnectToServer() )
	{
		_32 Responce;
		_32 data[5];
		_32 len = 20;
		if( parent )
		{
			PDISPLAY display = (PDISPLAY)parent;
			parent = display->hDisplay;
		}
		if( TransactServerMessage( MSG_OpenDisplayAboveSizedAt, &attributes, 6 * sizeof( _32 )
										 , &Responce, data, &len ) &&
			Responce == (MSG_OpenDisplayAboveSizedAt|SERVER_SUCCESS) )
		{
			PDISPLAY display = (PDISPLAY)Allocate( sizeof( DISPLAY ) );
         Log( WIDE("Server returned susccess open display above sized at...") );
         MemSet( display, 0, sizeof( DISPLAY ) );
			display->hDisplay = *(PRENDERER*)(data+0);
         MemCpy( &display->x, data + 1, 16 );
         AddLink( &g.pDisplayList, display );
			return (PRENDERER)display;
		}
	}
   return (PRENDERER)NULL;
}

RENDER_PROC( void, CloseDisplay ) (PRENDERER renderer )
{
   PDISPLAY display = (PDISPLAY)renderer;
	if( ConnectToServer() )
	{
		if( TransactServerMessage( MSG_CloseDisplay, &display->hDisplay, 1 * sizeof( _32 )
										 , NULL, NULL, NULL ) )
		{
         DeleteLink( &g.pDisplayList, display );
			Release( display->DisplayImage );
			Release( display );
         lprintf( WIDE("Display closed.") );
			return;
		}
		else
         Log( WIDE("CloseDisplay failed - transaction to server failed, or responce was bad") );
	}
}



RENDER_PROC( void, UpdateDisplayPortionEx  )( PRENDERER renderer, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	PDISPLAY display = (PDISPLAY)renderer;
#ifdef OPTION_FLUSH_ON_UPDATE
	_32 Responce;
#endif
						  // might have created another window...
						  // we buffer updates together all over now, this is
   // prolly redundant.
	if( !display->flags.redraw_dispatched && ConnectToServer() )
	{
		if( TransactServerMultiMessage( MSG_UpdateDisplayPortionEx
#ifdef _DEBUG
												, 4
#else
                                    , 2
#endif
												, &Responce, NULL, 0
												,&display->hDisplay, 1 * sizeof( _32 )
												, &x, 4 * sizeof( _32 )
#ifdef _DEBUG
												, &nLine, sizeof( nLine )
												, pFile, strlen( pFile ) + 1
#endif
												)
#ifdef OPTION_FLUSH_ON_UPDATE
			&& ( Responce == ( MSG_UpdateDisplayPortionEx | SERVER_SUCCESS ) )
#endif
		  )
		{
			return;
		}
		else
         Log( WIDE("UpdateDisplay failed - transaction to server failed, or responce was bad") );
	}

}

RENDER_PROC( void, UpdateDisplayEx )( PRENDERER renderer DBG_PASS )
{
	PDISPLAY display = (PDISPLAY)renderer;
   _32 Responce;
	if( !display->flags.redraw_dispatched && ConnectToServer() )
	{
		if( TransactServerMessage( MSG_UpdateDisplay, &display->hDisplay, 1 * sizeof( _32 )
										, &Responce, NULL, 0 )
#ifdef OPTION_FLUSH_ON_UPDATE
			&& ( Responce == ( MSG_UpdateDisplay | SERVER_SUCCESS ) )
#endif
		  )
		{
			return;
		}
		else
         Log( WIDE("UpdateDisplay failed - transaction to server failed, or responce was bad") );
	}

}
                             
RENDER_PROC( void, ClearDisplay)              ( PRENDERER renderer ) // ClearTo(0), Update
{
	PDISPLAY display = (PDISPLAY)renderer;
	TransactServerMessage( MSG_ClearDisplay, &display->hDisplay, sizeof( display->hDisplay )
								, NULL, NULL, NULL );
}

RENDER_PROC( void, GetDisplayPosition )       ( PRENDERER renderer, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	if( !renderer )
		return;
	{
	PDISPLAY display = (PDISPLAY)renderer;
	if( width )
      *width = display->width;
	if( height )
      *height = display->height;
	if( x )
      *x = display->x;
	if( y )
      *y = display->y;
   }
}

RENDER_PROC( void, MoveDisplay)               ( PRENDERER renderer, S_32 x, S_32 y )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_MoveDisplay, &renderer, ParamLength( renderer, y )
								, NULL, NULL, NULL );
	display->x = x;
   display->y = y;
}

RENDER_PROC( void, MoveDisplayRel)            ( PRENDERER renderer, S_32 delx, S_32 dely )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_MoveDisplayRel, &renderer, ParamLength( renderer, dely )
								, NULL, NULL, NULL );
	display->x += delx;
   display->y += dely;
}

RENDER_PROC( void, SizeDisplay)               ( PRENDERER renderer, _32 w, _32 h )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_SizeDisplay, &renderer, ParamLength( renderer, h )
								, NULL, NULL, NULL );
	display->width = w;
   display->height = h;
}

RENDER_PROC( void, SizeDisplayRel)            ( PRENDERER renderer, S_32 delw, S_32 delh )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_SizeDisplayRel, &renderer, ParamLength( renderer, delh )
								, NULL, NULL, NULL );
	display->width += delw;
   display->height += delh;
}

RENDER_PROC( void, MoveSizeDisplayRel)        ( PRENDERER renderer
															 , S_32 delx, S_32 dely
															 , S_32 delw, S_32 delh )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_MoveSizeDisplayRel, &renderer, ParamLength( renderer, delh )
								, NULL, NULL, NULL );
	display->x += delx;
   display->y += dely;
	display->width += delw;
   display->height += delh;
}

RENDER_PROC( void, MoveSizeDisplay)        ( PRENDERER renderer
															 , S_32 x, S_32 y
															 , S_32 w, S_32 h )
{
	PDISPLAY display = (PDISPLAY)renderer;
	renderer = (PRENDERER)display->hDisplay;
	TransactServerMessage( MSG_MoveSizeDisplay, &renderer, ParamLength( renderer, h )
								, NULL, NULL, NULL );
	display->x = x;
   display->y = y;
	display->width = w;
   display->height = h;
}

RENDER_PROC( void, PutDisplayAbove)           ( PRENDERER renderer, PRENDERER renderer2 ) // this that - put this above that
{
	PDISPLAY display = (PDISPLAY)renderer
		, display2 = (PDISPLAY)renderer2;
	renderer = (PRENDERER)display->hDisplay;
   renderer2 = (PRENDERER)display2->hDisplay;
	TransactServerMessage( MSG_PutDisplayAbove, &renderer, ParamLength( renderer, renderer2 )
								, NULL, NULL, NULL );
}

RENDER_PROC( Image, GetDisplayImage)     ( PRENDERER renderer)
{
	PDISPLAY display = (PDISPLAY)renderer;
	if( display->DisplayImage )
      return (Image)display->DisplayImage;
	if( ConnectToServer() )
	{
		_32 Responce;
		_32 data[5];
		_32 len = 20;
		PMyImage image = (PMyImage)Allocate( sizeof( MyImage ) );
		if( TransactServerMultiMessage( MSG_GetDisplayImage, 2
												, &Responce, data, &len
												, &display->hDisplay, sizeof( display->hDisplay )
												, &image, sizeof( image )
												) &&
			Responce == (MSG_GetDisplayImage|SERVER_SUCCESS) )
		{
			Log2( WIDE("Result image is : %p (into %p)"), *(Image*)(data+4), image );
			image->x = data[0];
			image->y = data[1];
			image->width = data[2];
			image->height = data[3];
			image->RealImage = *(Image*)(data+4);
			display->DisplayImage = image;
			return (Image)image;
		}
		else
		{
			Release( image );
		}
	}
   return (Image)NULL;
}

RENDER_PROC( void, SetCloseHandler)    ( PRENDERER renderer, CloseCallback CloseMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->CloseMethod = CloseMethod;
   display->CloseData = data;
	TransactServerMultiMessage( MSG_SetCloseHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &data, sizeof( PTRSZVAL ) );
}

RENDER_PROC( void, SetMouseHandler)    ( PRENDERER renderer, MouseCallback MouseMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->MouseMethod = MouseMethod;
	display->MouseData = data;
	Log3( WIDE("Setting mouse handler, informing server of our information %p %p %") PTRSZVALfx
		 , display, MouseMethod, data );
	TransactServerMultiMessage( MSG_SetMouseHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &display, sizeof( PTRSZVAL ) );
}

RENDER_PROC( void, SetRedrawHandler)   ( PRENDERER renderer, RedrawCallback RedrawMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->RedrawMethod = RedrawMethod;
   display->RedrawData = data;
	TransactServerMultiMessage( MSG_SetRedrawHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &display, sizeof( PTRSZVAL ) );
}

RENDER_PROC( void, SetKeyboardHandler) ( PRENDERER renderer, KeyProc KeyMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->KeyMethod = KeyMethod;
   display->KeyData = data;
	TransactServerMultiMessage( MSG_SetKeyboardHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &display, sizeof( PTRSZVAL ) );
}

RENDER_PROC( void, SetLoseFocusHandler)( PRENDERER renderer, LoseFocusCallback LoseFocusMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->LoseFocusMethod = LoseFocusMethod;
	display->LoseFocusData = data;
   Log2( WIDE("Setting focus handler %p %p"), display->hDisplay, renderer );
	TransactServerMultiMessage( MSG_SetLoseFocusHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &renderer, sizeof( PTRSZVAL ) );
}

#ifdef ACTIVE_MESSAGE_IMPLEMENTED
RENDER_PROC( void, SetDefaultHandler)  ( PRENDERER renderer, GeneralCallback GeneralMethod, PTRSZVAL data )
{
	PDISPLAY display = (PDISPLAY)renderer;
	display->GeneralMethod = GeneralMethod;
	display->GeneralData = data;
	TransactServerMultiMessage( MSG_SetDefaultHandler, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &display, sizeof( PTRSZVAL )
									  );
}
#endif

RENDER_PROC( void, GetMousePosition)   ( S_32 *x, S_32 *y )
{
	_32 Responce;
	_32 data[2];
	_32 len = 8;
	if( TransactServerMessage( MSG_GetMousePosition, NULL, 0
									 , &Responce, data, &len ) &&
		Responce == (MSG_GetMousePosition|SERVER_SUCCESS) )
	{
		if( x )
			*x = data[0];
		if( y )
			*y = data[1];
	}
	else
	{
      if( x )
			*x = 0;
		if( y )
			*y = 0;
	}
}
//----------------------------------------------------------------------------

void CPROC GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
	_32 Responce;
	_32 data[2];
	_32 len = 8;
	if( TransactServerMessage( MSG_GetMousePosition, NULL, 0
									 , &Responce, data, &len ) &&
		Responce == (MSG_GetMousePosition|SERVER_SUCCESS) )
	{
		if( x )
			*x = data[0];
		if( y )
			*y = data[1];
		if( b )
         (*b) = data[2];
	}
	else
	{
      if( x )
			*x = 0;
		if( y )
			*y = 0;
		if( b )
         (*b) = 0;
	}
}


RENDER_PROC( void, SetMousePosition)   ( PRENDERER renderer, S_32 x, S_32 y )
{
	PDISPLAY display = (PDISPLAY)renderer;
	TransactServerMultiMessage( MSG_SetMousePosition, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &x, ParamLength( x, y )
									  );
}

//    LOGICAL,_HasFocus)        ( PRENDERER renderer );

//    int,_SendActiveMessage)         ( PRENDERER renderer dest, PACTIVEMESSAGE msg );
//    PACTIVEMESSAGE,_CreateActiveMessage) ( int ID, int size, ... );


//RENDER_PROC( _32, IsKeyDown )              ( PRENDERER display, int key ) { return FALSE; }
//RENDER_PROC( _32, KeyDown )                ( PRENDERER display, int key ) { return FALSE; }

    //IsKeyDown
    //KeyDown
    //KeyDouble
//RENDER_PROC( char, GetKeyText)             ( int key ){ return 0; }
    //GetKeyText

RENDER_PROC( LOGICAL, DisplayIsValid ) ( PRENDERER renderer )
{
	return TRUE;
}

RENDER_PROC( void, OwnMouseEx ) ( PRENDERER renderer, _32 bOwn DBG_PASS )
{
	PDISPLAY display = (PDISPLAY)renderer;
	Log2( DBG_FILELINEFMT "Own Mouse: %p %"_32fs"" DBG_RELAY, renderer, bOwn );
	TransactServerMultiMessage( MSG_OwnMouseEx, 2
									  ,  NULL, NULL, NULL
									  , &display->hDisplay, sizeof( display->hDisplay )
									  , &bOwn, ParamLength( bOwn, bOwn )
									  );
}

RENDER_PROC( void, SyncRender )( PRENDERER renderer )
{
	_32 Responce;
	PDISPLAY display = (PDISPLAY)renderer;
   lprintf( WIDE("__________ Going to sync display %p %p"), display, display->hDisplay );
	if( !ConnectToServer() ||
		!TransactServerMessage( MSG_SyncRender
									 , &display->hDisplay, sizeof( display->hDisplay )
									 , &Responce, NULL, NULL ) ||
		 Responce != (MSG_SyncRender|SERVER_SUCCESS) )
	{
      Log( WIDE("Failed to sync render.") );
	}
}

RENDER_PROC( int, BeginCalibration ) ( _32 nPoints )
{
   _32 Msg;
	TransactServerMessage( MSG_BeginCalibration, &nPoints, sizeof( nPoints )
									 , &Msg, NULL, 0 );
   return 0;
}

RENDER_PROC( void, HideDisplay )( PRENDERER renderer )
{
	PDISPLAY display = (PDISPLAY)renderer;
   SendServerMessage( MSG_HideDisplay, &display->hDisplay, 4 );
}

RENDER_PROC( void, RestoreDisplayEx )( PRENDERER renderer DBG_PASS )
{
	PDISPLAY display = (PDISPLAY)renderer;
   SendServerMessage( MSG_RestoreDisplay, &display->hDisplay, 4 );
}

RENDER_PROC( int, SetActiveGLDisplay )( PRENDERER pRender )
{
#ifdef __LINUX__
	// not sure we can do this with SDL...
   // maybe it's just a matter of viewport...
#endif
   return 0;
}

RENDER_PROC( int, EnableOpenGL )( PRENDERER pRender )
{
   return 1;
}

void DoNothing( void )
{
}

RENDER_PROC( void, ForceDisplayFocus )( PRENDERER pRender )
{
}
RENDER_PROC( void, ForceDisplayFront )( PRENDERER pRender )
{
}
RENDER_PROC( void, ForceDisplayBack )( PRENDERER pRender )
{
}
RENDER_PROC( void, MakeTopmost )( PRENDERER hVideo )
{
}
RENDER_PROC( int, IsTopmost )( PRENDERER hVideo )
{
   return 0;
}
//----------------------------------------------------------------------------


RENDER_PROC( int, IsTouchDisplay )( void )
{
	_32 Responce;
	_32 data[1];
	_32 len = 4;
	if( TransactServerMultiMessage( MSG_IsTouchDisplay, 0
											,  &Responce, data, &len
											) )
	{
      if( Responce == ( MSG_IsTouchDisplay|SERVER_SUCCESS ) )
      return data[0];
	}

   return 0;
}

RENDER_INTERFACE MyDisplayInterface = {
  ConnectToServer
, SetApplicationTitle
, SetApplicationIcon
, GetDisplaySize
, SetDisplaySize
, ProcessDisplayMessages
, OpenDisplaySizedAt
, OpenDisplayAboveSizedAt
, CloseDisplay
, UpdateDisplayPortionEx
, UpdateDisplayEx
, ClearDisplay
, GetDisplayPosition
, MoveDisplay
, MoveDisplayRel
, SizeDisplay
, SizeDisplayRel
, MoveSizeDisplayRel
, PutDisplayAbove
, GetDisplayImage
, SetCloseHandler
, SetMouseHandler
, SetRedrawHandler
, SetKeyboardHandler
, SetLoseFocusHandler
#ifdef ACTIVE_MESSAGE_IMPLEMENTED
												  , SetDefaultHandler
#endif
												  , (void(CPROC *)(PS_32,PS_32))GetMousePosition
												  , (void(CPROC *)(PRENDERER,S_32,S_32))SetMousePosition
												  , (LOGICAL(CPROC *)(PRENDERER))DoNothing // hasfocus
#ifdef ACTIVE_MESSAGE_IMPLEMENTED
												  , (int(CPROC *)(PRENDERER,PACTIVEMESSAGE))DoNothing //sendactmsg
												  , (PACTIVEMESSAGE(CPROC *)(int,int,...))DoNothing
#endif
												  , GetKeyText
												  , IsKeyDown
												  , KeyDown
												  , DisplayIsValid
												  , OwnMouseEx
												  , BeginCalibration
												 , SyncRender
												 , (int(CPROC *)(PRENDERER))DoNothing // enable GL
												 , SetActiveGLDisplay // SetActiveGLDisplay
												 , MoveSizeDisplay
												 , MakeTopmost // MakeTopMost
												 , HideDisplay
												  , RestoreDisplayEx
												  , ForceDisplayFocus
												  , ForceDisplayFront
												  , ForceDisplayBack
												  , BindEventToKey
												  , UnbindKey
												  , IsTopmost
                                      , NULL /* OkaySyncRender */
												  , IsTouchDisplay
                                      , GetMouseState
};

//RENDER_DATA( PRENDER_INTERFACE, RenderInterface );
#undef GetDisplayInterface
#undef DropDisplayInterface
static int nReferences;


static POINTER CPROC _GetClientDisplayInterface( void )
{
	if( ConnectToServer() )
	{
      //atexit( (void(*)(void))DropDisplayInterface );
		nReferences++;
		return (POINTER)&MyDisplayInterface;
	}
   return NULL;
}
RENDER_PROC( PRENDER_INTERFACE, GetDisplayInterface )( void )
{
   return (PRENDER_INTERFACE)_GetClientDisplayInterface();
}

void CPROC _DropClientDisplayInterface(POINTER p)
{
   Log1( WIDE("Dropping service registration %d interfaces retreived...."), nReferences );
	nReferences--;
	//if( !nReferences )
	{
		DisconnectFromServer();
      g.flags.disconnected = 1;
	}
}

ATEXIT( ____DropClientDisplayInterface )
{
   _DropClientDisplayInterface( NULL );
}

RENDER_PROC( void, DropDisplayInterface )( PRENDER_INTERFACE p )
{
   _DropClientDisplayInterface( p );
}

PRELOAD( RegisterDisplayInterface )
{
	//lprintf( WIDE("Registering client display interface") );
   RegisterInterface( WIDE("sack.msgsvr.render"), _GetClientDisplayInterface, _DropClientDisplayInterface );
}

RENDER_NAMESPACE_END

