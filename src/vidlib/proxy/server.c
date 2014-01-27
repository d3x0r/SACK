
#include <imglib/imagestruct.h>

#include <render.h>
#include <render3d.h>
#include <image3d.h>
#include <sqlgetoption.h>
#include <html5.websocket.h>
#include <json_emitter.h>
#include "server_local.h"

static struct json_context_object *WebSockInitJson( enum proxy_message_id message )
{
	struct json_context_object *cto;
	struct json_context_object *cto_data;
	int ofs = 4;  // first thing is length, but that is not encoded..
	if( !l.json_context )
		l.json_context = json_create_context();
	cto = json_create_object( l.json_context, 0 );
	SetLink( &l.messages, (int)message, cto );
	json_add_object_member( cto, WIDE("MsgID"), 0, JSON_Element_Unsigned_Integer_8, 0 );
	cto_data = json_add_object_member( cto, WIDE( "data" ), 1, JSON_Element_Object, 0 );

	ofs = 0;
	switch( message )
	{
	case PMID_Version:
		json_add_object_member( cto_data, WIDE("version"), 0, JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_SetApplicationTitle:
		json_add_object_member( cto_data, WIDE("title"), 0, JSON_Element_CharArray, 0 );
		break;
	case PMID_OpenDisplayAboveUnderSizedAt:
		json_add_object_member( cto_data, WIDE("x"), ofs = 0, JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("attrib"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = ofs + sizeof(_32), JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("over_render_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL_BLANK_0, 0 );
		json_add_object_member( cto_data, WIDE("under_render_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL_BLANK_0, 0 );
		break;
	case PMID_CloseDisplay:
		json_add_object_member( cto_data, WIDE("client_render_id"), 0, JSON_Element_PTRSZVAL, 0 );
		break;
	}
	return cto;
}


static void SendTCPMessage( PCLIENT pc, INDEX idx, LOGICAL websock, enum proxy_message_id message, va_list args )
{
	TEXTSTR json_msg;
	struct json_context_object *cto;
	size_t sendlen;
	// often used; sometimes unused...
	PVPRENDER render;
	_8 *msg;
	if( websock )
	{
		cto = (struct json_context_object *)GetLink( &l.messages, message );
		if( !cto )
			cto = WebSockInitJson( message );
	}
	else
		cto = NULL;
	switch( message )
	{
	case PMID_Version:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
			StrCpy( msg + 1, l.application_title );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( websock )
			{
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_SetApplicationTitle:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
			StrCpy( msg + 1, l.application_title );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( websock )
			{
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_OpenDisplayAboveUnderSizedAt:
		{
			render = va_arg( args, PVPRENDER );
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct opendisplay_data ) ) );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;

			((struct opendisplay_data*)(msg+5))->x = render->x;
			((struct opendisplay_data*)(msg+5))->y = render->y;
			((struct opendisplay_data*)(msg+5))->w = render->w;
			((struct opendisplay_data*)(msg+5))->h = render->h;
			((struct opendisplay_data*)(msg+5))->attr = render->attributes;
			((struct opendisplay_data*)(msg+5))->server_display_id = (PTRSZVAL)render;

			if( render->above )
				((struct opendisplay_data*)(msg+5))->over = (PTRSZVAL)(GetLink( &render->above->remote_render_id, idx ) );
			else
				((struct opendisplay_data*)(msg+5))->over = 0;
			if( render->under )
				((struct opendisplay_data*)(msg+5))->under = (PTRSZVAL)(GetLink( &render->under->remote_render_id, idx ) );
			else
				((struct opendisplay_data*)(msg+5))->under = 0;

			if( websock )
			{

			}

				json_msg = json_build_message( cto, msg + 4 );
				WebSocketSendText( pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_CloseDisplay:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( POINTER ) ) );
			render = va_arg( args, PVPRENDER );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( ((POINTER*)(msg+5))[0] = GetLink( &render->remote_render_id, idx ) )
				if( websock )
				{
					json_msg = json_build_message( cto, msg + 4 );
					WebSocketSendText( pc, json_msg, StrLen( json_msg ) );
					Release( json_msg );
				}
				else
					SendTCP( pc, msg, sendlen );
			Release( msg );
		}
		break;
	}
}

static void SendTCPMessageV( PCLIENT pc, INDEX idx, LOGICAL websock, enum proxy_message_id message, ... )
{
	va_list args;
	va_start( args, message );
	SendTCPMessage( pc, idx, websock, message, args );
}

static void SendClientMessage( enum proxy_message_id message, ... )
{
	INDEX idx;
	struct server_proxy_client *client;
	LIST_FORALL( l.clients, idx, struct server_proxy_client*, client )
	{
		va_list args;
		va_start( args, message );
		SendTCPMessage( client->pc, idx, client->websock, message, args );
	}
}


static void CPROC SocketRead( PCLIENT pc, POINTER buffer, int size )
{
	if( !buffer )
	{
		buffer = NewArray( _8, 1024 );
		size = 1024;
	}
	else
	{

	}
	ReadTCP( pc, buffer, size );
}

static void CPROC Connected( PCLIENT pcServer, PCLIENT pcNew )
{
	struct server_proxy_client *client = New( struct server_proxy_client );
	INDEX idx;
	client->pc = pcNew;
	client->websock = TRUE;
	AddLink( &l.clients, client );
	idx = FindLink ( &l.clients, client );
	if( l.application_title )
		SendTCPMessageV( pcNew, idx, FALSE, PMID_SetApplicationTitle );
	{
		INDEX idx;
		PVPRENDER render;
		LIST_FORALL( l.renderers, idx, PVPRENDER, render )
		{
			SendTCPMessageV( pcNew, idx, FALSE, PMID_OpenDisplayAboveUnderSizedAt, render );
		}
	}
}

static PTRSZVAL WebSockOpen( PCLIENT pc, PTRSZVAL psv )
{
	struct server_proxy_client *client = New( struct server_proxy_client );
	INDEX idx;
	client->pc = pc;
	client->websock = TRUE;
	AddLink( &l.clients, client );
	idx = FindLink( &l.clients, client );
	if( l.application_title )
		SendTCPMessageV( pc, idx, TRUE, PMID_SetApplicationTitle );
	{
		INDEX idx;
		PVPRENDER render;
		LIST_FORALL( l.renderers, idx, PVPRENDER, render )
		{
			SendTCPMessageV( pc, idx, TRUE, PMID_OpenDisplayAboveUnderSizedAt, render );
		}
	}
	return (PTRSZVAL)1;
}

static void WebSockClose( PCLIENT pc, PTRSZVAL psv )
{
}

static void WebSockError( PCLIENT pc, PTRSZVAL psv, int error )
{
}

static void WebSockEvent( PCLIENT pc, PTRSZVAL psv, POINTER buffer, int msglen )
{
	POINTER *msg;
	if( json_parse_message( l.json_context, buffer, &msg ) )
	{
		lprintf( "Success" );
	}
}


static void InitService( void )
{
	if( !l.listener )
	{
		NetworkStart();
		l.listener = OpenTCPListenerAddrEx( CreateSockAddress( "0.0.0.0", 4241 ), Connected );
		l.web_listener = WebSocketCreate( "ws://0.0.0.0:4240/Sack/Vidlib/Proxy"
													, WebSockOpen
													, WebSockEvent
													, WebSockClose
													, WebSockError
													, 0 );
	}
}

static int CPROC VidlibProxy_InitDisplay( void )
{
	return TRUE;
}

static void CPROC VidlibProxy_SetApplicationIcon( Image icon )
{
	// no support
}

static LOGICAL CPROC VidlibProxy_RequiresDrawAll( void )
{
	return TRUE;
}

static void VidlibProxy_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = StrDup( title );
	SendClientMessage( PMID_SetApplicationTitle );
}

static void VidlibProxy_GetDisplaySize( _32 *width, _32 *height )
{
	if( width )
		(*width) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Width", 1024 );
	if( height )
		(*height) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Height", 768 );
}

static void VidlibProxy_SetDisplaySize		( _32 width, _32 height )
{
	SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Width", width );
	SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Height", height );
}


static Image CPROC VidlibProxy_MakeImageFileEx (_32 Width, _32 Height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = Width;
	image->h = Height;
	image->image = l.real_interface->_MakeImageFileEx( Width, Height DBG_RELAY );
	SendClientMessage( PMID_MakeImageFile, image );
	AddLink( &l.images, image );
	return (Image)image;
}


static PRENDERER VidlibProxy_OpenDisplayAboveUnderSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under )
{

	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );
	Renderer->x = x;
	Renderer->y = y;
	Renderer->w = width;
	Renderer->h = height;
	Renderer->attributes = attributes;
	Renderer->above = (PVPRENDER)above;
	Renderer->under = (PVPRENDER)under;
	Renderer->image = VidlibProxy_MakeImageFileEx( width, height DBG_SRC );
	Renderer->image->flags |= IF_FLAG_FINAL_RENDER;
	AddLink( &l.renderers, Renderer );
	SendClientMessage( PMID_OpenDisplayAboveUnderSizedAt, Renderer );
	return (PRENDERER)Renderer;
}

static PRENDERER VidlibProxy_OpenDisplayAboveSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above )
{
	return VidlibProxy_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, above, NULL );
}

static PRENDERER VidlibProxy_OpenDisplaySizedAt	  ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
	return VidlibProxy_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, NULL, NULL );
}

static void  VidlibProxy_CloseDisplay ( PRENDERER Renderer )
{
	SendClientMessage( PMID_CloseDisplay, Renderer );
	DeleteLink( &l.renderers, Renderer );
	Release( Renderer );
}

static void VidlibProxy_UpdateDisplayPortionEx( PRENDERER r, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	// no-op; it will ahve already displayed(?)
}

static void VidlibProxy_UpdateDisplayEx( PRENDERER r DBG_PASS)
{
	// no-op; it will ahve already displayed(?)
}

static void VidlibProxy_GetDisplayPosition ( PRENDERER r, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	PVPRENDER pRender = (PVPRENDER)r;
	if( x )
		(*x) = pRender->x;
	if( y )
		(*y) = pRender->y;
	if( width )
		(*width) = pRender->w;
	if( height ) 
		(*height) = pRender->h;
}

static void CPROC VidlibProxy_MoveDisplay		  ( PRENDERER r, S_32 x, S_32 y )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->x = x;
	pRender->y = y;
}

static void CPROC VidlibProxy_MoveDisplayRel( PRENDERER r, S_32 delx, S_32 dely )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->x += delx;
	pRender->y += dely;
}

static void CPROC VidlibProxy_SizeDisplay( PRENDERER r, _32 w, _32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->w = w;
	pRender->h = h;
}

static void CPROC VidlibProxy_SizeDisplayRel( PRENDERER r, S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->w += delw;
	pRender->h += delh;
}

static void CPROC VidlibProxy_MoveSizeDisplayRel( PRENDERER r
																 , S_32 delx, S_32 dely
																 , S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->w += delw;
	pRender->h += delh;
	pRender->x += delx;
	pRender->y += dely;
}

static void CPROC VidlibProxy_MoveSizeDisplay( PRENDERER r
													 , S_32 x, S_32 y
													 , S_32 w, S_32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->x = x;
	pRender->y = y;
	pRender->w = w;
	pRender->h = h;
}

static void CPROC VidlibProxy_PutDisplayAbove		( PRENDERER r, PRENDERER above )
{
	lprintf( "window ordering is not implemented" );
}

static Image CPROC VidlibProxy_GetDisplayImage( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return pRender->image;
}

static void CPROC VidlibProxy_SetCloseHandler	 ( PRENDERER r, CloseCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetMouseHandler  ( PRENDERER r, MouseCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetRedrawHandler  ( PRENDERER r, RedrawCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetKeyboardHandler	( PRENDERER r, KeyProc c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetLoseFocusHandler  ( PRENDERER r, LoseFocusCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_GetMousePosition	( S_32 *x, S_32 *y )
{
}

static void CPROC VidlibProxy_SetMousePosition  ( PRENDERER r, S_32 x, S_32 y )
{
}

static LOGICAL CPROC VidlibProxy_HasFocus		 ( PRENDERER  r )
{
	return TRUE;
}

static TEXTCHAR CPROC VidlibProxy_GetKeyText		 ( int key )
{ 
	return 0;
}

static _32 CPROC VidlibProxy_IsKeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static _32 CPROC VidlibProxy_KeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static LOGICAL CPROC VidlibProxy_DisplayIsValid ( PRENDERER r )
{
	return (r != NULL);
}

static void CPROC VidlibProxy_OwnMouseEx ( PRENDERER r, _32 Own DBG_PASS)
{

}

static int CPROC VidlibProxy_BeginCalibration ( _32 points )
{
	return 0;
}

static void CPROC VidlibProxy_SyncRender( PRENDERER pDisplay )
{
}

static void CPROC VidlibProxy_MakeTopmost  ( PRENDERER r )
{
}

static void CPROC VidlibProxy_HideDisplay	 ( PRENDERER r )
{
}

static void CPROC VidlibProxy_RestoreDisplay  ( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayFocus ( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayFront( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayBack( PRENDERER r )
{
}

static int CPROC  VidlibProxy_BindEventToKey( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	return 0;
}

static int CPROC VidlibProxy_UnbindKey( PRENDERER pRenderer, _32 scancode, _32 modifier )
{
	return 0;
}

static int CPROC VidlibProxy_IsTopmost( PRENDERER r )
{
	return 0;
}

static void CPROC VidlibProxy_OkaySyncRender( void )
{
	// redundant thing?
}

static int CPROC VidlibProxy_IsTouchDisplay( void )
{
	return 0;
}

static void CPROC VidlibProxy_GetMouseState( S_32 *x, S_32 *y, _32 *b )
{
}

static PSPRITE_METHOD CPROC VidlibProxy_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	return NULL;
}

static void CPROC VidlibProxy_WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
}

static void CPROC VidlibProxy_PutDisplayIn(PRENDERER r, PRENDERER hContainer)
{
}

static void CPROC VidlibProxy_SetRendererTitle( PRENDERER render, const TEXTCHAR *title )
{
}

static void CPROC VidlibProxy_DisableMouseOnIdle(PRENDERER r, LOGICAL bEnable )
{
}

static void CPROC VidlibProxy_SetDisplayNoMouse( PRENDERER r, int bNoMouse )
{
}

static void CPROC VidlibProxy_Redraw( PRENDERER r )
{
}

static void CPROC VidlibProxy_MakeAbsoluteTopmost(PRENDERER r)
{
}

static void CPROC VidlibProxy_SetDisplayFade( PRENDERER r, int level )
{
}

static LOGICAL CPROC VidlibProxy_IsDisplayHidden( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return pRender->flags.hidden;
}

#ifdef WIN32
static HWND CPROC VidlibProxy_GetNativeHandle( PRENDERER r )
{
}
#endif

static void CPROC VidlibProxy_GetDisplaySizeEx( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height)
{
}

static void CPROC VidlibProxy_LockRenderer( PRENDERER render )
{
}

static void CPROC VidlibProxy_UnlockRenderer( PRENDERER render )
{
}

static void CPROC VidlibProxy_IssueUpdateLayeredEx( PRENDERER r, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
}


#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
		 
		 \ \																									  */
static void CPROC VidlibProxy_SetTouchHandler  ( PRENDERER r, TouchCallback c, PTRSZVAL p )
{
}
#endif

static void CPROC VidlibProxy_MarkDisplayUpdated( PRENDERER r  )
{
}

static void CPROC VidlibProxy_SetHideHandler		( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetRestoreHandler  ( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_RestoreDisplayEx ( PRENDERER r DBG_PASS )
{
}

// android extension
PUBLIC( void, SACK_Vidlib_ShowInputDevice )( void )
{
}

PUBLIC( void, SACK_Vidlib_HideInputDevice )( void )
{
}


static RENDER_INTERFACE ProxyInterface = {
	VidlibProxy_InitDisplay
													  , VidlibProxy_SetApplicationTitle
													  , VidlibProxy_SetApplicationIcon
													  , VidlibProxy_GetDisplaySize
													  , VidlibProxy_SetDisplaySize
													  , VidlibProxy_OpenDisplaySizedAt
													  , VidlibProxy_OpenDisplayAboveSizedAt
													  , VidlibProxy_CloseDisplay
													  , VidlibProxy_UpdateDisplayPortionEx
													  , VidlibProxy_UpdateDisplayEx
													  , VidlibProxy_GetDisplayPosition
													  , VidlibProxy_MoveDisplay
													  , VidlibProxy_MoveDisplayRel
													  , VidlibProxy_SizeDisplay
													  , VidlibProxy_SizeDisplayRel
													  , VidlibProxy_MoveSizeDisplayRel
													  , VidlibProxy_PutDisplayAbove
													  , VidlibProxy_GetDisplayImage
													  , VidlibProxy_SetCloseHandler
													  , VidlibProxy_SetMouseHandler
													  , VidlibProxy_SetRedrawHandler
													  , VidlibProxy_SetKeyboardHandler
	 /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
		 
		 \ \																												 */
													  , VidlibProxy_SetLoseFocusHandler
			 ,  0  //POINTER junk1;
													  , VidlibProxy_GetMousePosition
													  , VidlibProxy_SetMousePosition
													  , VidlibProxy_HasFocus

													  , VidlibProxy_GetKeyText
													  , VidlibProxy_IsKeyDown
													  , VidlibProxy_KeyDown
													  , VidlibProxy_DisplayIsValid
													  , VidlibProxy_OwnMouseEx
													  , VidlibProxy_BeginCalibration
													  , VidlibProxy_SyncRender

													  , VidlibProxy_MoveSizeDisplay
													  , VidlibProxy_MakeTopmost
													  , VidlibProxy_HideDisplay
													  , VidlibProxy_RestoreDisplay
													  , VidlibProxy_ForceDisplayFocus
													  , VidlibProxy_ForceDisplayFront
													  , VidlibProxy_ForceDisplayBack
													  , VidlibProxy_BindEventToKey
													  , VidlibProxy_UnbindKey
													  , VidlibProxy_IsTopmost
													  , VidlibProxy_OkaySyncRender
													  , VidlibProxy_IsTouchDisplay
													  , VidlibProxy_GetMouseState
													  , VidlibProxy_EnableSpriteMethod
													  , VidlibProxy_WinShell_AcceptDroppedFiles
													  , VidlibProxy_PutDisplayIn
#ifdef WIN32
													  , NULL // make renderer from native handle
#endif
													  , VidlibProxy_SetRendererTitle
													  , VidlibProxy_DisableMouseOnIdle
													  , VidlibProxy_OpenDisplayAboveUnderSizedAt
													  , VidlibProxy_SetDisplayNoMouse
													  , VidlibProxy_Redraw
													  , VidlibProxy_MakeAbsoluteTopmost
													  , VidlibProxy_SetDisplayFade
													  , VidlibProxy_IsDisplayHidden
#ifdef WIN32
													, NULL // get native handle from renderer
#endif
													  , VidlibProxy_GetDisplaySizeEx

													  , VidlibProxy_LockRenderer
													  , VidlibProxy_UnlockRenderer
													  , VidlibProxy_IssueUpdateLayeredEx
													  , VidlibProxy_RequiresDrawAll
#ifndef NO_TOUCH
													  , VidlibProxy_SetTouchHandler
#endif
													  , VidlibProxy_MarkDisplayUpdated
													  , VidlibProxy_SetHideHandler
													  , VidlibProxy_SetRestoreHandler
													  , VidlibProxy_RestoreDisplayEx
												, SACK_Vidlib_ShowInputDevice
												, SACK_Vidlib_HideInputDevice
};

static void InitProxyInterface( void )
{
	ProxyInterface._RequiresDrawAll = VidlibProxy_RequiresDrawAll;

}

static RENDER3D_INTERFACE Proxy3dInterface = {
	NULL
};

static void CPROC VidlibProxy_SetStringBehavior( Image pImage, _32 behavior )
{

}
static void CPROC VidlibProxy_SetBlotMethod	  ( _32 method )
{

}

static Image CPROC VidlibProxy_BuildImageFileEx ( PCOLOR pc, _32 width, _32 height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	lprintf( "CRITICAL; BuildImageFile is not possible" );
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_BuildImageFileEx( pc, width, height DBG_RELAY );
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	//SendClientMessage( PMID_MakeImageFile, image );
	AddLink( &l.images, image );
	return (Image)image;
}

static Image CPROC VidlibProxy_MakeSubImageEx  ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->x = x;
	image->y = y;
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_MakeSubImageEx( ((PVPImage)pImage)->image, x, y, width, height DBG_RELAY );

	image->parent = (PVPImage)pImage;
	if( image->next = ((PVPImage)pImage)->child )
		image->next->prior = image;
	((PVPImage)pImage)->child = image;

	// don't really need to make this; if it needs to be updated to the client it will be handled later
	SendClientMessage( PMID_MakeSubImageFile, image );

	return (Image)image;
}

static Image CPROC VidlibProxy_RemakeImageEx	 ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS)
{
	PVPImage image;
	if( !(image = (PVPImage)pImage ) )
	{
		image = New( struct vidlib_proxy_image );
		MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	}
	lprintf( "CRITICAL; RemakeImageFile is not possible" );
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_RemakeImageEx( image->image, pc, width, height DBG_RELAY );

	AddLink( &l.images, image );
	return (Image)image;
}

static Image CPROC VidlibProxy_LoadImageFileFromGroupEx( INDEX group, CTEXTSTR filename DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->filegroup = group;
	image->filename = StrDup( filename );
	image->image = l.real_interface->_LoadImageFileFromGroupEx( group, filename DBG_RELAY );
	image->w = image->image->actual_width;
	image->h = image->image->actual_height;
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	SendClientMessage( PMID_LoadImageFileFromGroup, image );
	AddLink( &l.images, image );
	return (Image)image;
}

static Image CPROC VidlibProxy_LoadImageFileEx( CTEXTSTR filename DBG_PASS )
{
	return VidlibProxy_LoadImageFileFromGroupEx( 0, filename DBG_RELAY );
}



/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
	
	Internal
	Interface index 10																	*/  IMAGE_PROC_PTR( Image,VidlibProxy_LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
static  void CPROC VidlibProxy_UnmakeImageFileEx( Image pif DBG_PASS )
{
	SendClientMessage( PMID_UnmakeImageFile, pif );
	Release( pif );
}

static void CPROC VidlibProxy_ResizeImageEx	  ( Image pImage, S_32 width, S_32 height DBG_PASS)
{
	PVPImage image = (PVPImage)pImage;
	image->w = width;
	image->h = height;
	l.real_interface->_ResizeImageEx( image->image, width, height DBG_RELAY );
}

static void CPROC VidlibProxy_MoveImage			( Image pImage, S_32 x, S_32 y )
{
	PVPImage image = (PVPImage)pImage;
	if( image->x != x || image->y != y )
	{
		image->x = x;
		image->y = y;
		l.real_interface->_MoveImage( image->image, x, y );
	}
}

static void CPROC VidlibProxy_BlatColor	  ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{

}

static void CPROC VidlibProxy_BlatColorAlpha( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
}

static void CPROC VidlibProxy_BlotImageEx	  ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... )
{
}

static void CPROC VidlibProxy_BlotImageSizedEx( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... )
{
}

static void CPROC VidlibProxy_BlotScaledImageSizedEx( Image pifDest, Image pifSrc
											  , S_32 xd, S_32 yd
											  , _32 wd, _32 hd
											  , S_32 xs, S_32 ys
											  , _32 ws, _32 hs
											  , _32 nTransparent
											  , _32 method, ... )
{
}


#define DIMAGE_DATA_PROC(type,name,args)  static type (CPROC VidlibProxy2_##name)args;static type (CPROC* VidlibProxy_##name)args = VidlibProxy2_##name; type (CPROC VidlibProxy2_##name)args

DIMAGE_DATA_PROC( void,plot,		( Image pi, S_32 x, S_32 y, CDATA c ))
{
}

DIMAGE_DATA_PROC( void,plotalpha, ( Image pi, S_32 x, S_32 y, CDATA c ))
{
}

DIMAGE_DATA_PROC( CDATA,getpixel, ( Image pi, S_32 x, S_32 y ))
{
	PVPImage my_image = (PVPImage)pi;
	if( my_image )
	{
		return (*l.real_interface->_getpixel)( my_image->image, x, y );
	}
	return 0;
}

DIMAGE_DATA_PROC( void,do_line,	  ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color ))
{
}

DIMAGE_DATA_PROC( void,do_lineAlpha,( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color))
{
}


DIMAGE_DATA_PROC( void,do_hline,	  ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color ))
{
}

DIMAGE_DATA_PROC( void,do_vline,	  ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color ))
{
}

DIMAGE_DATA_PROC( void,do_hlineAlpha,( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color ))
{
}

DIMAGE_DATA_PROC( void,do_vlineAlpha,( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color ))
{
}

static SFTFont CPROC VidlibProxy_GetDefaultFont ( void )
{
	return NULL;
}

static _32 CPROC VidlibProxy_GetFontHeight  ( SFTFont font )
{
	return 12;
}

static _32 CPROC VidlibProxy_GetStringSizeFontEx( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont )
{
	if( width )
		(*width) = (_32)(len*12);
	if( height )
		(*height) = 12;
	return 12;
}

static void CPROC VidlibProxy_PutCharacterFont		  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
}

/* <combine sack::image::PutCharacterVerticalFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
	
	Internal
	Interface index 34																													 */	IMAGE_PROC_PTR( void,PutCharacterVerticalFont)		( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
	
	Internal
	Interface index 35																												  */	IMAGE_PROC_PTR( void,PutCharacterInvertFont)		  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
	
	Internal
	Interface index 36																															 */	IMAGE_PROC_PTR( void,PutCharacterVerticalInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );

/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
	
	Internal
	Interface index 37																											  */	IMAGE_PROC_PTR( void,PutStringFontEx)				  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
	
	Internal
	Interface index 38																														 */	IMAGE_PROC_PTR( void,PutStringVerticalFontEx)		( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
	
	Internal
	Interface index 39																													  */	IMAGE_PROC_PTR( void,PutStringInvertFontEx)		  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
	
	Internal
	Interface index 40																																 */	IMAGE_PROC_PTR( void,PutStringInvertVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

/* <combine sack::image::GetMaxStringLengthFont@_32@SFTFont>
	
	Internal
	Interface index 41												 */	IMAGE_PROC_PTR( _32, GetMaxStringLengthFont )( _32 width, SFTFont UseFont );

/* <combine sack::image::GetImageSize@Image@_32 *@_32 *>
	
	Internal
	Interface index 42																	 */	IMAGE_PROC_PTR( void, GetImageSize)					 ( Image pImage, _32 *width, _32 *height );
/* <combine sack::image::LoadFont@SFTFont>
	
	Internal
	Interface index 43											  */	IMAGE_PROC_PTR( SFTFont, LoadFont )						 ( SFTFont font );
			/* <combine sack::image::UnloadFont@SFTFont>
				
				\ \												*/
			IMAGE_PROC_PTR( void, UnloadFont )					  ( SFTFont font );

/* Internal
	Interface index 44
	
	This is used by internal methods to transfer image and font
	data to the render agent.											  */	IMAGE_PROC_PTR( DataState, BeginTransferData )	 ( _32 total_size, _32 segsize, CDATA data );
/* Internal
	Interface index 45
	
	Used internally to transfer data to render agent. */	IMAGE_PROC_PTR( void, ContinueTransferData )		( DataState state, _32 segsize, CDATA data );
/* Internal
	Interface index 46
	
	Command issues at end of data transfer to decode the data
	into an image.														  */	IMAGE_PROC_PTR( Image, DecodeTransferredImage )	 ( DataState state );
/* After a data transfer decode the information as a font.
	Internal
	Interface index 47												  */	IMAGE_PROC_PTR( SFTFont, AcceptTransferredFont )	  ( DataState state );

DIMAGE_DATA_PROC( CDATA, ColorAverage,( CDATA c1, CDATA c2
													, int d, int max ))
{
	return c1;
}
/* <combine sack::image::SyncImage>
	
	Internal
	Interface index 49					*/	IMAGE_PROC_PTR( void, SyncImage )					  ( void );
			/* <combine sack::image::GetImageSurface@Image>
				
				\ \														*/
			IMAGE_PROC_PTR( PCDATA, GetImageSurface )		 ( Image pImage );
			/* <combine sack::image::IntersectRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
				
				\ \																															*/
			IMAGE_PROC_PTR( int, IntersectRectangle )		( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
	/* <combine sack::image::MergeRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
		
		\ \																													  */
	IMAGE_PROC_PTR( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
	/* <combine sack::image::GetImageAuxRect@Image@P_IMAGE_RECTANGLE>
		
		\ \																				*/
	IMAGE_PROC_PTR( void, GetImageAuxRect )	( Image pImage, P_IMAGE_RECTANGLE pRect );
	/* <combine sack::image::SetImageAuxRect@Image@P_IMAGE_RECTANGLE>
		
		\ \																				*/
	IMAGE_PROC_PTR( void, SetImageAuxRect )	( Image pImage, P_IMAGE_RECTANGLE pRect );

static void CPROC VidlibProxy_OrphanSubImage ( Image pImage )
{
	PVPImage image = (PVPImage)pImage;
	if( image )
	{
		if( image->image )
			l.real_interface->_OrphanSubImage( image->image );
	}
}

static void CPROC VidlibProxy_AdoptSubImage ( Image pFoster, Image pOrphan )
{
	PVPImage foster = (PVPImage)pFoster;
	PVPImage orphan = (PVPImage)pOrphan;
	if( foster && orphan )
	{
		if( foster->image && orphan->image )
			l.real_interface->_AdoptSubImage( foster->image, orphan->image );
	}
}
	/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
		
		\ \																			*/
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	/* <combine sack::image::MakeSpriteImageEx@Image image>
		
		\ \																  */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	/* <combine sack::image::rotate_scaled_sprite@Image@PSPRITE@fixed@fixed@fixed>
		
		\ \																								 */
	IMAGE_PROC_PTR( void	, rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	/* <combine sack::image::rotate_sprite@Image@PSPRITE@fixed>
		
		\ \																		*/
	IMAGE_PROC_PTR( void	, rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
 /* <combine sack::image::BlotSprite@Image@PSPRITE>
																	  
	 Internal
	Interface index 61															 */
		IMAGE_PROC_PTR( void	, BlotSprite )( Image pdest, PSPRITE ps );
	 /* <combine sack::image::DecodeMemoryToImage@P_8@_32>
		 
		 \ \																*/
	 IMAGE_PROC_PTR( Image, DecodeMemoryToImage )( P_8 buf, _32 size );

	/* <combine sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32>
		
		\returns a SFTFont																		*/
	IMAGE_PROC_PTR( SFTFont, InternalRenderFontFile )( CTEXTSTR file
																 , S_32 nWidth
																 , S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																 , _32 flags 
																 );
	/* <combine sack::image::InternalRenderFont@_32@_32@_32@S_32@S_32@_32>
		
		requires knowing the font cache....											*/
	IMAGE_PROC_PTR( SFTFont, InternalRenderFont )( _32 nFamily
															, _32 nStyle
															, _32 nFile
															, S_32 nWidth
															, S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
															, _32 flags
															);
/* <combine sack::image::RenderScaledFontData@PFONTDATA@PFRACTION@PFRACTION>
	
	\ \																							  */
IMAGE_PROC_PTR( SFTFont, RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
/* <combine sack::image::RenderFontFileEx@CTEXTSTR@_32@_32@_32@P_32@POINTER *>
	
	\ \																								 */
IMAGE_PROC_PTR( SFTFont, RenderFontFileScaledEx )( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *size, POINTER *pFontData );
/* <combine sack::image::DestroyFont@SFTFont *>
	
	\ \													*/
IMAGE_PROC_PTR( void, DestroyFont)( SFTFont *font );
/* <combine sack::image::GetGlobalFonts>
	
	global_font_data in interface is really a global font data. Don't
	have to call GetGlobalFont to get this.									*/
struct font_global_tag *_global_font_data;
/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@_32 *>
	
	\ \																			  */
IMAGE_PROC_PTR( int, GetFontRenderData )( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
/* <combine sack::image::SetFontRendererData@SFTFont@POINTER@_32>
	
	\ \																			*/
IMAGE_PROC_PTR( void, SetFontRendererData )( SFTFont font, POINTER pResult, size_t size );
/* <combine sack::image::SetSpriteHotspot@PSPRITE@S_32@S_32>
	
	\ \																		 */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
/* <combine sack::image::SetSpritePosition@PSPRITE@S_32@S_32>
	
	\ \																		  */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
		
		\ \																*/
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
	
	\ \											  */
IMAGE_PROC_PTR( struct font_global_tag *, GetGlobalFonts)( void );

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
	
	\ \																																																							*/
IMAGE_PROC_PTR( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_PTR( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
IMAGE_PROC_PTR( COLOR_CHANNEL, GetGreenValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetBlueValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
IMAGE_PROC_PTR( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
IMAGE_PROC_PTR( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
IMAGE_PROC_PTR( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
IMAGE_PROC_PTR( void, MarkImageDirty )( Image pImage );

IMAGE_PROC_PTR( void, DumpFontCache )( void );
IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );


static IMAGE_INTERFACE ProxyImageInterface = {
	VidlibProxy_SetStringBehavior,
		VidlibProxy_SetBlotMethod,
		VidlibProxy_BuildImageFileEx,
		VidlibProxy_MakeImageFileEx,
		VidlibProxy_MakeSubImageEx,
		VidlibProxy_RemakeImageEx,
		VidlibProxy_LoadImageFileEx,
		VidlibProxy_UnmakeImageFileEx,
		VidlibProxy_ResizeImageEx,
		VidlibProxy_MoveImage,
		VidlibProxy_BlatColor
		, VidlibProxy_BlatColorAlpha
		, VidlibProxy_BlotImageEx
		, VidlibProxy_BlotImageSizedEx
		, VidlibProxy_BlotScaledImageSizedEx
		, &VidlibProxy_plot
		, &VidlibProxy_plotalpha
		, &VidlibProxy_getpixel
		, &VidlibProxy_do_line
		, &VidlibProxy_do_lineAlpha
		, &VidlibProxy_do_hline
		, &VidlibProxy_do_vline
		, &VidlibProxy_do_hlineAlpha
		, &VidlibProxy_do_vlineAlpha
		, VidlibProxy_GetDefaultFont
		, VidlibProxy_GetFontHeight
		, VidlibProxy_GetStringSizeFontEx
		, VidlibProxy_PutCharacterFont
#if 0

		, VidlibProxy_PutCharacterVerticalFont
		, VidlibProxy_PutCharacterInvertFont
		, VidlibProxy_PutCharacterVerticalInvertFont
		, VidlibProxy_PutStringFontEx
		, VidlibProxy_PutStringVerticalFontEx
		, VidlibProxy_PutStringInvertFontEx
		, VidlibProxy_PutStringInvertVerticalFontEx
		, VidlibProxy_GetMaxStringLengthFont
		, VidlibProxy_GetImageSize
		, VidlibProxy_LoadFont
		, VidlibProxy_UnloadFont
		, VidlibProxy_BeginTransferData
		, VidlibProxy_ContinueTransferData
		, VidlibProxy_DecodeTransferredImage
		, VidlibProxy_AcceptTransferredFont
		, &VidlibProxy_ColorAverage
		, VidlibProxy_SyncImage
		, VidlibProxy_GetImageSurface
		, VidlibProxy_IntersectRectangle
		, VidlibProxy_MergeRectangle
		, VidlibProxy_GetImageAuxRect
		, VidlibProxy_SetImageAuxRect
		, VidlibProxy_OrphanSubImage
		, VidlibProxy_AdoptSubImage
		, VidlibProxy_MakeSpriteImageFileEx
		, VidlibProxy_MakeSpriteImageEx
		, VidlibProxy_rotate_scaled_sprite
		, VidlibProxy_rotate_sprite
		, VidlibProxy_BlotSprite
		, VidlibProxy_DecodeMemoryToImage
		, VidlibProxy_InternalRenderFontFile
		, VidlibProxy_InternalRenderFont
		, VidlibProxy_RenderScaledFontData
		, VidlibProxy_RenderFontFileScaledEx
		, VidlibProxy_DestroyFont
		, NULL
		, VidlibProxy_GetFontRenderData
		, VidlibProxy_SetFontRendererData
		, VidlibProxy_SetSpriteHotspot
		, VidlibProxy_SetSpritePosition
		, VidlibProxy_UnmakeSprite
		, NULL //VidlibProxy_struct font_global_tag *, GetGlobalFonts)( void );

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
	
	\ \																																																							*/
IMAGE_PROC_PTR( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

IMAGE_PROC_PTR( Image, LoadImageFileFromGroupEx )( INDEX group, CTEXTSTR filename DBG_PASS );

IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_PTR( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
IMAGE_PROC_PTR( COLOR_CHANNEL, GetGreenValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetBlueValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
IMAGE_PROC_PTR( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
IMAGE_PROC_PTR( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
IMAGE_PROC_PTR( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
IMAGE_PROC_PTR( void, MarkImageDirty )( Image pImage );

IMAGE_PROC_PTR( void, DumpFontCache )( void );
IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );
#endif
};

static COLOR_CHANNEL CPROC VidlibProxy_GetRedValue( CDATA color )
{
	return (color & 0xFF ) >> 0;
}

static COLOR_CHANNEL CPROC VidlibProxy_GetGreenValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF00 ) >> 8);
}

static COLOR_CHANNEL CPROC VidlibProxy_GetBlueValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0x00FF0000 ) >> 16);
}

static COLOR_CHANNEL CPROC VidlibProxy_GetAlphaValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF000000 ) >> 24);
}

static CDATA CPROC VidlibProxy_MakeAlphaColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b, COLOR_CHANNEL alpha )
{
#  ifdef _WIN64
#	 define _AND_FF &0xFF
#  else
/* This is a macro to cure a 64bit warning in visual studio. */
#	 define _AND_FF
#  endif
#define _AColor( r,g,b,a ) (((_32)( ((_8)((b)_AND_FF))|((_16)((_8)((g))_AND_FF)<<8))|(((_32)((_8)((r))_AND_FF)<<16)))|(((a)_AND_FF)<<24))
	return _AColor( r, grn, b, alpha );
}

static CDATA CPROC VidlibProxy_MakeColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b )
{
	return VidlibProxy_MakeAlphaColor( r,grn,b, 0xFF );
}

static void InitImageInterface( void )
{
	ProxyImageInterface._GetRedValue = VidlibProxy_GetRedValue;
	ProxyImageInterface._GetGreenValue = VidlibProxy_GetGreenValue;
	ProxyImageInterface._GetBlueValue = VidlibProxy_GetBlueValue;
	ProxyImageInterface._GetAlphaValue = VidlibProxy_GetAlphaValue;
	ProxyImageInterface._MakeColor = VidlibProxy_MakeColor;
	ProxyImageInterface._MakeAlphaColor = VidlibProxy_MakeAlphaColor;
	ProxyImageInterface._LoadImageFileFromGroupEx = VidlibProxy_LoadImageFileFromGroupEx;
	ProxyImageInterface._GetStringSizeFontEx = VidlibProxy_GetStringSizeFontEx;
	ProxyImageInterface._GetFontHeight = VidlibProxy_GetFontHeight;
	ProxyImageInterface._OrphanSubImage = VidlibProxy_OrphanSubImage;
	ProxyImageInterface._AdoptSubImage = VidlibProxy_AdoptSubImage;

}

static IMAGE_3D_INTERFACE Proxy3dImageInterface = {
	NULL
};

static POINTER CPROC GetProxyDisplayInterface( void )
{
	// open server socket
	return &ProxyInterface;
}
static void CPROC DropProxyDisplayInterface( POINTER i )
{
	// close connections
}
static POINTER CPROC Get3dProxyDisplayInterface( void )
{
	// open server socket
	return &Proxy3dInterface;
}
static void CPROC Drop3dProxyDisplayInterface( POINTER i )
{
	// close connections
}

static POINTER CPROC GetProxyImageInterface( void )
{
	// open server socket
	return &ProxyImageInterface;
}
static void CPROC DropProxyImageInterface( POINTER i )
{
	// close connections
}
static POINTER CPROC Get3dProxyImageInterface( void )
{
	// open server socket
	return &Proxy3dImageInterface;
}
static void CPROC Drop3dProxyImageInterface( POINTER i )
{
	// close connections
}

PRIORITY_PRELOAD( RegisterProxyInterface, VIDLIB_PRELOAD_PRIORITY )
{
	InitProxyInterface();
	InitImageInterface();
	LoadFunction( "bag.image.dll", NULL );
	l.real_interface = (PIMAGE_INTERFACE)GetInterface( WIDE( "sack.image" ) );
	RegisterInterface( WIDE( "sack.image.proxy.server" ), GetProxyImageInterface, DropProxyImageInterface );
	RegisterInterface( WIDE( "sack.image.3d.proxy.server" ), Get3dProxyImageInterface, Drop3dProxyImageInterface );
	RegisterInterface( WIDE( "sack.render.proxy.server" ), GetProxyDisplayInterface, DropProxyDisplayInterface );
	RegisterInterface( WIDE( "sack.render.3d.proxy.server" ), Get3dProxyDisplayInterface, Drop3dProxyDisplayInterface );

	// wanted to delay-init; until a renderer is actually open..
	InitService();
}


