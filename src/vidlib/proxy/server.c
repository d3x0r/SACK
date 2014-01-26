
#include <render.h>
#include <render3d.h>
#include <html5.websocket.h>
#include <json_emitter.h>
#include "server_local.h"

static struct json_context_object *WebSockInitJson( enum proxy_message_id message )
{
	struct json_create_object *cto;
	struct json_create_object *cto_data;
	if( !l.json_context )
		l.json_context = json_create_context();
	cto = json_create_object( l.json_context, 0 );
	SetLink( &l.messages, (int)message, cto );
	json_add_object_member( cto, WIDE("MsgID"), 0, JSON_Element_Unsigned_Integer_8, 0 );
	cto_data = json_add_object_member( cto, WIDE( "data" ), 0, JSON_Element_Object, 0 );
	switch( message )
	{
	case PMID_Version:
		json_add_object_member( cto_data, WIDE("version"), 0, JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_SetApplicationTitle:
		json_add_object_member( cto_data, WIDE("title"), 0, JSON_Element_CharArray, 0 );
		break;
	}
	return cto;
}

static void WebSockSendMessage( enum proxy_message_id message, ... )
{
	INDEX idx;
	PCLIENT client;
	TEXTSTR json_msg;
	struct json_context_object *cto = GetLink( &l.messages, message );
	if( !cto )
		cto = WebSockInitJson( message );
	LIST_FORALL( l.web_clients, idx, PCLIENT, client )
	{
		switch( message )
		{
		case PMID_Version:
			{
				int sendlen;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
				StrCpy( msg + 1, l.application_title );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( client, json_msg, StrLen( json_msg ) );
				Release( msg );
				Release( json_msg );
			}
			break;
		case PMID_SetApplicationTitle:
			{
				int sendlen;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
				StrCpy( msg + 1, l.application_title );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( client, json_msg, StrLen( json_msg ) );
				Release( msg );
				Release( json_msg );
			}
			break;
		case PMID_CloseDisplay:
			{
				int sendlen;
				va_list args;
				PVPRENDER render;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( POINTER ) ) );
				va_start( args, essage );
				render = va_start( args, PVPRENDER );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				if( ((POINTER*)(msg+5))[0] = GetLink( &render, idx ) )
					SendTCP( client, msg, sendlen );
				Release( msg );
			}
			break;
		}
	}
}

static void SendMessage( enum proxy_message_id message, ... )
{
	INDEX idx;
	PCLIENT client;
	LIST_FORALL( l.clients, idx, PCLIENT, client )
	{
		switch( message )
		{
		case PMID_Version:
			{
				int sendlen;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
				StrCpy( msg + 1, l.application_title );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				SendTCP( client, msg, sendlen );
				Release( msg );

			}
			break;
		case PMID_SetApplicationTitle:
			{
				int sendlen;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
				StrCpy( msg + 1, l.application_title );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				SendTCP( client, msg, sendlen );
				Release( msg );
			}
			break;
		case PMID_CloseDisplay:
			{
				int sendlen;
				va_list args;
				PVPRENDER render;
				_8 *msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( POINTER ) ) );
				va_start( args, essage );
				render = va_start( args, PVPRENDER );
				((_32*)msg)[0] = sendlen - 4;
				msg[4] = message;
				if( ((POINTER*)(msg+5))[0] = GetLink( &render, idx ) )
					SendTCP( client, msg, sendlen );
				Release( msg );
			}
			break;
		}
	}
}

static void CPROC SocketRead( PCLIENT pc, POINTER buffer, int size )
{
	if( !buffer )
	{
		buffer = NewArray( _8, 1024 );
      len = 1024;
	}
	else
	{

	}
   ReadTCP( pc, buffer, len );
}

static void CPROC Connected( PCLIENT pcServer, PCLIENT pcNew )
{
	AddLink( &l.clients, pcNew );
	if( l.application_id )
		SendMessage( PMID_SetApplicationTitle );
	{
		INDEX idx;
		PVPRENDER render;
		LIST_FORALL( l.renderers, idx, PVPRENDERER, render )
		{
			SendMessage( PMID_OpenDisplayAboveSizeAt, render );
		}
	}
}

static PTRSZVAL WebSockOpen( PCLIENT pc, PTRSZVAL psv )
{
	AddLink( &l.web_clients, pc );

	if( l.application_id )
		WebSockSendMessage( PMID_SetApplicationTitle );
	{
		INDEX idx;
		PVPRENDER render;
		LIST_FORALL( l.renderers, idx, PVPRENDERER, render )
		{
			WebSockSendMessage( PMID_OpenDisplayAboveSizeAt, render );
		}
	}
}

static void WebSockClose( PCLIENT pc, PTRSZVAL psv )
{
}

static void WebSockError( PCLIENT pc, PTRSZVAL psv, int error )
{
}

static void WebSockEvent( PCLIENT pc, PTRSZVAL psv, POINTER buffer, int msglen )
{
}


static void InitService( void )
{
	if( !l.listener )
	{
		NetworkStart();
		l.listener = OpenTCPListenerAddrEx( CreateSockAddress( "0.0.0.0", 4241 ), Connected, 0 );
		l.web_listener = WebSocketCreate( "0.0.0.0:4240/Sack/Vidlib/Proxy"
													, WebSockOpen
													, WebSockEvent
													, WebSockClose
													, WebSockError
													, 0 );
      SetNetworkReadComplete(
	}
}

static int VidlibProxy_InitDisplay( void )
{
	return TRUE;
}


static void VidlibProxy_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = title;
   SendMessage( PMID_SetApplicationTitle );
}

static void VidlibProxy_SetApplicationTitle( Image image )
{

}

static void VidlibProxy_GetDisplaySize( _32 *width, _32 *height )
{
	if( width )
		(*width) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Width", 1024 );
	if( height )
		(*height) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Height", 768 );
}

static void VidlibProxy_SetDisplaySize      ( _32 width, _32 height )
{
   SACK_SetProfileInt( "SACK/Vidlib", "Default Display Width", width );
   SACK_SetProfileInt( "SACK/Vidlib", "Default Display Height", height );
}

static PRENDERER VidlibProxy_OpenDisplayAboveSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above )
{
	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );
	Renderer->x = x;
   Renderer->y = y;
	Renderer->w = width;
	Renderer->h = height;
	Renderer->attributes = attributes;
	Renderer->above = above;
	AddLink( &l.renderers, Renderer );
   SendMessage( PMID_OpenDisplayAboveSizedAt, Renderer );
	return (PRENDERER)Renderer;
}

static PRENDERER VidlibProxy_OpenDisplaySizedAt     ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
   return VidlibProxy_OpenDisplayAboveSizedAt( attributes, width, height, x, y, NULL );
}

static void  VidlibProxy_CloseDisplay ( PRENDERER Renderer )
{
   SendMessage( PMID_CloseDisplay, Renderer );
	DeleteLink( &l.renderers, Renderer );
   Release( Renderer );
}

static void VidlibProxy_UpdateDisplayPortionEx( PRENDERER, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
}

    RENDER_PROC_PTR( void, VidlibProxy_UpdateDisplayEx)        ( PRENDERER DBG_PASS);
                             
    RENDER_PROC_PTR( void, VidlibProxy_ClearDisplay)         ( PRENDERER ); /* <combine sack::image::render::ClearDisplay@PRENDERER>
                                                   
                                                   \ \                                                   */
   
    /* <combine sack::image::render::GetDisplayPosition@PRENDERER@S_32 *@S_32 *@_32 *@_32 *>
       
       \ \                                                                                   */
    RENDER_PROC_PTR( void, VidlibProxy_GetDisplayPosition)   ( PRENDERER, S_32 *x, S_32 *y, _32 *width, _32 *height );
    /* <combine sack::image::render::MoveDisplay@PRENDERER@S_32@S_32>
       
       \ \                                                            */
    RENDER_PROC_PTR( void, VidlibProxy_MoveDisplay)          ( PRENDERER, S_32 x, S_32 y );
    /* <combine sack::image::render::MoveDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, VidlibProxy_MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely );
    /* <combine sack::image::render::SizeDisplay@PRENDERER@_32@_32>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, VidlibProxy_SizeDisplay)          ( PRENDERER, _32 w, _32 h );
    /* <combine sack::image::render::SizeDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, VidlibProxy_SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh );
    /* <combine sack::image::render::MoveSizeDisplayRel@PRENDERER@S_32@S_32@S_32@S_32>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, VidlibProxy_MoveSizeDisplayRel )  ( PRENDERER hVideo
                                                 , S_32 delx, S_32 dely
                                                 , S_32 delw, S_32 delh );
    RENDER_PROC_PTR( void, VidlibProxy_PutDisplayAbove)      ( PRENDERER, PRENDERER ); /* <combine sack::image::render::PutDisplayAbove@PRENDERER@PRENDERER>
                                                              
                                                              \ \                                                                */
 
    /* <combine sack::image::render::GetDisplayImage@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( Image, VidlibProxy_GetDisplayImage)     ( PRENDERER );

    /* <combine sack::image::render::SetCloseHandler@PRENDERER@CloseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, VidlibProxy_SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetMouseHandler@PRENDERER@MouseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, VidlibProxy_SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRedrawHandler@PRENDERER@RedrawCallback@PTRSZVAL>
       
       \ \                                                                               */
    RENDER_PROC_PTR( void, VidlibProxy_SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL );
    /* <combine sack::image::render::SetKeyboardHandler@PRENDERER@KeyProc@PTRSZVAL>
       
       \ \                                                                          */
    RENDER_PROC_PTR( void, VidlibProxy_SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
       
       \ \                                                                                     */
    RENDER_PROC_PTR( void, VidlibProxy_SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
    /* <combine sack::image::render::SetDefaultHandler@PRENDERER@GeneralCallback@PTRSZVAL>
       
       \ \                                                                                 */
    RENDER_PROC_PTR( void, VidlibProxy_GetMousePosition)     ( S_32 *x, S_32 *y );
    /* <combine sack::image::render::SetMousePosition@PRENDERER@S_32@S_32>
       
       \ \                                                                 */
    RENDER_PROC_PTR( void, VidlibProxy_SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );

    /* <combine sack::image::render::HasFocus@PRENDERER>
       
       \ \                                               */
    RENDER_PROC_PTR( LOGICAL, VidlibProxy_HasFocus)          ( PRENDERER );

    RENDER_PROC_PTR( TEXTCHAR, VidlibProxy_GetKeyText)           ( int key );
    /* <combine sack::image::render::IsKeyDown@PRENDERER@int>
       
       \ \                                                    */
    RENDER_PROC_PTR( _32, VidlibProxy_IsKeyDown )        ( PRENDERER display, int key );
    /* <combine sack::image::render::KeyDown@PRENDERER@int>
       
       \ \                                                  */
    RENDER_PROC_PTR( _32, VidlibProxy_KeyDown )         ( PRENDERER display, int key );
    /* <combine sack::image::render::DisplayIsValid@PRENDERER>
       
       \ \                                                     */
    RENDER_PROC_PTR( LOGICAL, VidlibProxy_DisplayIsValid )  ( PRENDERER display );
    /* <combine sack::image::render::OwnMouseEx@PRENDERER@_32 bOwn>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, VidlibProxy_OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS);
    /* <combine sack::image::render::BeginCalibration@_32>
       
       \ \                                                 */
    RENDER_PROC_PTR( int, VidlibProxy_BeginCalibration )       ( _32 points );
    /* <combine sack::image::render::SyncRender@PRENDERER>
       
       \ \                                                 */
    RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, EnableOpenGL )           ( PRENDERER hVideo );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, SetActiveGLDisplay )     ( PRENDERER hDisplay );

	 /* <combine sack::image::render::MoveSizeDisplay@PRENDERER@S_32@S_32@S_32@S_32>
	    
	    \ \                                                                          */
	 RENDER_PROC_PTR( void, VidlibProxy_MoveSizeDisplay )( PRENDERER hVideo
                                        , S_32 x, S_32 y
                                        , S_32 w, S_32 h );
   /* <combine sack::image::render::MakeTopmost@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, MakeTopmost )    ( PRENDERER hVideo );
   /* <combine sack::image::render::HideDisplay@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, HideDisplay )      ( PRENDERER hVideo );
   /* <combine sack::image::render::RestoreDisplay@PRENDERER>
      
      \ \                                                     */
   RENDER_PROC_PTR( void, RestoreDisplay )   ( PRENDERER hVideo );

	/* <combine sack::image::render::ForceDisplayFocus@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFocus )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayFront@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFront )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayBack@PRENDERER>
	   
	   \ \                                                       */
	RENDER_PROC_PTR( void, ForceDisplayBack )( PRENDERER display );

	/* <combine sack::image::render::BindEventToKey@PRENDERER@_32@_32@KeyTriggerHandler@PTRSZVAL>
	   
	   \ \                                                                                        */
	RENDER_PROC_PTR( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
	/* <combine sack::image::render::UnbindKey@PRENDERER@_32@_32>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier );
	/* <combine sack::image::render::IsTopmost@PRENDERER>
	   
	   \ \                                                */
	RENDER_PROC_PTR( int, IsTopmost )( PRENDERER hVideo );
	/* Used as a point to sync between applications and the message
	   display server; Makes sure that all draw commands which do
	   not have a response are done.
	   
	   
	   
	   Waits until all commands are processed; which is wait until
	   this command is processed.                                   */
	RENDER_PROC_PTR( void, OkaySyncRender )            ( void );
   /* <combine sack::image::render::IsTouchDisplay>
      
      \ \                                           */
   RENDER_PROC_PTR( int, IsTouchDisplay )( void );
	/* <combine sack::image::render::GetMouseState@S_32 *@S_32 *@_32 *>
	   
	   \ \                                                              */
	RENDER_PROC_PTR( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b );
	/* <combine sack::image::render::EnableSpriteMethod@PRENDERER@void__cdecl*RenderSpritesPTRSZVAL psv\, PRENDERER renderer\, S_32 x\, S_32 y\, _32 w\, _32 h@PTRSZVAL>
	   
	   \ \                                                                                                                                                               */
	RENDER_PROC_PTR ( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv );
	/* <combine sack::image::render::WinShell_AcceptDroppedFiles@PRENDERER@dropped_file_acceptor@PTRSZVAL>
	   
	   \ \                                                                                                 */
	RENDER_PROC_PTR( void, WinShell_AcceptDroppedFiles )( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser );
	/* <combine sack::image::render::PutDisplayIn@PRENDERER@PRENDERER>
	   
	   \ \                                                             */
	RENDER_PROC_PTR(void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer);
#ifdef WIN32
	/* <combine sack::image::render::MakeDisplayFrom@HWND>
	   
	   \ \                                                 */
			RENDER_PROC_PTR (PRENDERER, MakeDisplayFrom) (HWND hWnd) ;
#else
      POINTER junk4;
#endif
	/* <combine sack::image::render::SetRendererTitle@PRENDERER@TEXTCHAR *>
	   
	   \ \                                                                  */
	RENDER_PROC_PTR( void , SetRendererTitle) ( PRENDERER render, const TEXTCHAR *title );
	/* <combine sack::image::render::DisableMouseOnIdle@PRENDERER@LOGICAL>
	   
	   \ \                                                                 */
	RENDER_PROC_PTR (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable );
	/* <combine sack::image::render::OpenDisplayAboveUnderSizedAt@_32@_32@_32@S_32@S_32@PRENDERER@PRENDERER>
	   
	   \ \                                                                                                   */
	RENDER_PROC_PTR( PRENDERER, OpenDisplayAboveUnderSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under );
	/* <combine sack::image::render::SetDisplayNoMouse@PRENDERER@int>
	   
	   \ \                                                            */
	RENDER_PROC_PTR( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse );
	/* <combine sack::image::render::Redraw@PRENDERER>
	   
	   \ \                                             */
	RENDER_PROC_PTR( void, Redraw )( PRENDERER hVideo );
	/* <combine sack::image::render::MakeAbsoluteTopmost@PRENDERER>
	   
	   \ \                                                          */
	RENDER_PROC_PTR(void, MakeAbsoluteTopmost) (PRENDERER hVideo);
	/* <combine sack::image::render::SetDisplayFade@PRENDERER@int>
	   
	   \ \                                                         */
	RENDER_PROC_PTR( void, SetDisplayFade )( PRENDERER hVideo, int level );
	/* <combine sack::image::render::IsDisplayHidden@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( LOGICAL, IsDisplayHidden )( PRENDERER video );
#ifdef WIN32
	/* <combine sack::image::render::GetNativeHandle@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( HWND, GetNativeHandle )( PRENDERER video );
#endif
		 /* <combine sack::image::render::GetDisplaySizeEx@int@S_32 *@S_32 *@_32 *@_32 *>
		    
		    \ \                                                                           */
		 RENDER_PROC_PTR (void, GetDisplaySizeEx) ( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height);

	/* Locks a video display. Applications shouldn't be locking
	   this, but if for some reason they require it; use this
	   function.                                                */
	RENDER_PROC_PTR( void, LockRenderer )( PRENDERER render );
	/* Release renderer lock critical section. Applications
	   shouldn't be locking this surface.                   */
	RENDER_PROC_PTR( void, UnlockRenderer )( PRENDERER render );
	/* Provides a way for applications to cause the window to flush
	   to the display (if it's a transparent window)                */
	RENDER_PROC_PTR( void, IssueUpdateLayeredEx )( PRENDERER hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS );
	/* Check to see if the render mode is always redraw; changes how
	   smudge works in PSI. If always redrawn, then the redraw isn't
	   done during the smudge, and instead is delayed until a draw
	   is triggered at which time all controls are drawn.
	   
	   
	   
	   
	   Returns
	   TRUE if full screen needs to be drawn during a draw,
	   otherwise partial updates may be done.                        */
	RENDER_PROC_PTR( LOGICAL, RequiresDrawAll )( void );
#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
       
       \ \                                                                             */
			RENDER_PROC_PTR( void, SetTouchHandler)      ( PRENDERER, TouchCallback, PTRSZVAL );
#endif
    RENDER_PROC_PTR( void, MarkDisplayUpdated )( PRENDERER );
    /* <combine sack::image::render::SetHideHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetHideHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRestoreHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetRestoreHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
		 RENDER_PROC_PTR( void, RestoreDisplayEx )   ( PRENDERER hVideo DBG_PASS );
		 /* added for android extensions; call to enable showing the keyboard in the correct thread
        ; may have applications for windows tablets 
		  */
       RENDER_PROC_PTR( void, SACK_Vidlib_ShowInputDevice )( void );
		 /* added for android extensions; call to enable hiding the keyboard in the correct thread
		  ; may have applications for windows tablets */
       RENDER_PROC_PTR( void, SACK_Vidlib_HideInputDevice )( void );



static RENDER_INTERFACE ProxyInterface = {
	VidlibProxy_InitDisplay
													  , VidlibProxy_SetApplicationTitle
													  , VidlibProxy_SetApplicationIcon
                                         , VidlibProxy_GetDisplaySize
													  , VidlibProxy_SetDisplaySize
													  , VidlibProxy_ProcessDisplayMessages
													  , VidlibProxy_OpenDisplaySizedAt
													  , VidlibProxy_OpenDisplayAboveSizedAt
													  , VidlibProxy_CloseDisplay
													  , VidlibProxy_UpdateDisplayPortionEx
													  , VidlibProxy_UpdateDisplayEx

													  , VidlibProxy_ClearDisplay
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
    RENDER_PROC_PTR( void, SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
       
       \ \                                                                                     */
    RENDER_PROC_PTR( void, SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
			 ,  0  //POINTER junk1;
    RENDER_PROC_PTR( void, GetMousePosition)     ( S_32 *x, S_32 *y );
    RENDER_PROC_PTR( void, SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );
    RENDER_PROC_PTR( LOGICAL, HasFocus)          ( PRENDERER );

	 , 0// POINTER junk2;
	 , 0// POINTER junk3;

    RENDER_PROC_PTR( TEXTCHAR, GetKeyText)           ( int key );
    RENDER_PROC_PTR( _32, IsKeyDown )        ( PRENDERER display, int key );
    RENDER_PROC_PTR( _32, KeyDown )         ( PRENDERER display, int key );
    RENDER_PROC_PTR( LOGICAL, DisplayIsValid )  ( PRENDERER display );
    RENDER_PROC_PTR( void, OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS);
    RENDER_PROC_PTR( int, BeginCalibration )       ( _32 points );
    RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay );
    , 0 //RENDER_PROC_PTR( int, EnableOpenGL )           ( PRENDERER hVideo );
    , 0 //RENDER_PROC_PTR( int, SetActiveGLDisplay )     ( PRENDERER hDisplay );

	 RENDER_PROC_PTR( void, MoveSizeDisplay )( PRENDERER hVideo
                                        , S_32 x, S_32 y
                                        , S_32 w, S_32 h );
   RENDER_PROC_PTR( void, MakeTopmost )    ( PRENDERER hVideo );
   RENDER_PROC_PTR( void, HideDisplay )      ( PRENDERER hVideo );
   RENDER_PROC_PTR( void, RestoreDisplay )   ( PRENDERER hVideo );
	RENDER_PROC_PTR( void, ForceDisplayFocus )( PRENDERER display );
	RENDER_PROC_PTR( void, ForceDisplayFront )( PRENDERER display );
	RENDER_PROC_PTR( void, ForceDisplayBack )( PRENDERER display );
	RENDER_PROC_PTR( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
	RENDER_PROC_PTR( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier );
	RENDER_PROC_PTR( int, IsTopmost )( PRENDERER hVideo );
	RENDER_PROC_PTR( void, OkaySyncRender )            ( void );
   RENDER_PROC_PTR( int, IsTouchDisplay )( void );
	RENDER_PROC_PTR( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b );
	RENDER_PROC_PTR ( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv );
	RENDER_PROC_PTR( void, WinShell_AcceptDroppedFiles )( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser );
	RENDER_PROC_PTR(void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer);
			RENDER_PROC_PTR (PRENDERER, MakeDisplayFrom) (HWND hWnd) ;
	RENDER_PROC_PTR( void , SetRendererTitle) ( PRENDERER render, const TEXTCHAR *title );
	RENDER_PROC_PTR (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable );
	RENDER_PROC_PTR( PRENDERER, OpenDisplayAboveUnderSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under );
	RENDER_PROC_PTR( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse );
	RENDER_PROC_PTR( void, Redraw )( PRENDERER hVideo );
	RENDER_PROC_PTR(void, MakeAbsoluteTopmost) (PRENDERER hVideo);
	RENDER_PROC_PTR( void, SetDisplayFade )( PRENDERER hVideo, int level );
	RENDER_PROC_PTR( LOGICAL, IsDisplayHidden )( PRENDERER video );
#ifdef WIN32
	RENDER_PROC_PTR( HWND, GetNativeHandle )( PRENDERER video );
#endif
		 RENDER_PROC_PTR (void, GetDisplaySizeEx) ( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height);

	RENDER_PROC_PTR( void, LockRenderer )( PRENDERER render );
	RENDER_PROC_PTR( void, UnlockRenderer )( PRENDERER render );
	/* Provides a way for applications to cause the window to flush
	   to the display (if it's a transparent window)                */
	RENDER_PROC_PTR( void, IssueUpdateLayeredEx )( PRENDERER hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS );
	/* Check to see if the render mode is always redraw; changes how
	   smudge works in PSI. If always redrawn, then the redraw isn't
	   done during the smudge, and instead is delayed until a draw
	   is triggered at which time all controls are drawn.
	   
	   
	   
	   
	   Returns
	   TRUE if full screen needs to be drawn during a draw,
	   otherwise partial updates may be done.                        */
	RENDER_PROC_PTR( LOGICAL, RequiresDrawAll )( void );
#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
       
       \ \                                                                             */
			RENDER_PROC_PTR( void, SetTouchHandler)      ( PRENDERER, TouchCallback, PTRSZVAL );
#endif
    RENDER_PROC_PTR( void, MarkDisplayUpdated )( PRENDERER );
    /* <combine sack::image::render::SetHideHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetHideHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRestoreHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetRestoreHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
		 RENDER_PROC_PTR( void, RestoreDisplayEx )   ( PRENDERER hVideo DBG_PASS );
		 /* added for android extensions; call to enable showing the keyboard in the correct thread
        ; may have applications for windows tablets 
		  */
       RENDER_PROC_PTR( void, SACK_Vidlib_ShowInputDevice )( void );
		 /* added for android extensions; call to enable hiding the keyboard in the correct thread
		  ; may have applications for windows tablets */
       RENDER_PROC_PTR( void, SACK_Vidlib_HideInputDevice )( void );

};

static RENDER3D_INTERFACE Proxy3dInterface = {
};

staic void CPROC VidlibProxy_SetStringBehavior( Image pImage, _32 behavior )
{

}
static void CPROC SetBlotMethod     ( _32 method )
{
}

static Image CPROC BuildImageFileEx ( PCOLOR pc, _32 width, _32 height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = Width;
	image->h = Height;
	SendMessage( PMID_MakeImageFile );
	AddLink( &l.images, image );
}

static Image CPROC VidlibProxy_MakeImageFileEx (_32 Width, _32 Height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = Width;
	image->h = Height;
	SendMessage( PMID_MakeImageFile );
	AddLink( &l.images, image );
}

static Image CPROC VidlibProxy_MakeSubImageEx  ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->x = x;
	image->y = y;
	image->w = Width;
	image->h = Height;

	image->parent = pImage;
	if( image->next = pImage->child )
		image->next->prior = image;
	pImage->child = image;

	SendMessage( PMID_MakeSubImageFile );

	return (Image)PVPImage;
}

static Image CPROC VidlibProxy_RemakeImageEx    ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS)
{
}
/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
   
   Internal
   Interface index 10                                                   */  IMAGE_PROC_PTR( Image,VidlibProxy_LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
/* <combine sack::image::UnmakeImageFileEx@Image pif>
   
   Internal
   Interface index 11                                                 */  IMAGE_PROC_PTR( void,VidlibProxy_UnmakeImageFileEx) ( Image pif DBG_PASS );
/* <combine sack::image::SetImageBound@Image@P_IMAGE_RECTANGLE>
   
   Internal
   Interface index 12                                                           */  IMAGE_PROC_PTR( void ,VidlibProxy_SetImageBound)    ( Image pImage, P_IMAGE_RECTANGLE bound );
/* <combine sack::image::FixImagePosition@Image>
   
   Internal
   Interface index 13
   
   reset clip rectangle to the full image (subimage part ) Some
   operations (move, resize) will also reset the bound rect,
   this must be re-set afterwards. ALSO - one SHOULD be nice and
   reset the rectangle when done, otherwise other people may not
   have checked this.
                                                                 */  IMAGE_PROC_PTR( void ,FixImagePosition) ( Image pImage );

//-----------------------------------------------------

/* <combine sack::image::ResizeImageEx@Image@S_32@S_32 height>
   
   Internal
   Interface index 14                                                          */  IMAGE_PROC_PTR( void,ResizeImageEx)     ( Image pImage, S_32 width, S_32 height DBG_PASS);
/* <combine sack::image::MoveImage@Image@S_32@S_32>
   
   Internal
   Interface index 15                                               */   IMAGE_PROC_PTR( void,MoveImage)         ( Image pImage, S_32 x, S_32 y );

//-----------------------------------------------------

/* <combine sack::image::BlatColor@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 16                                                             */   IMAGE_PROC_PTR( void,BlatColor)     ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
/* <combine sack::image::BlatColorAlpha@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 17                                                                  */   IMAGE_PROC_PTR( void,BlatColorAlpha)( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

/* <combine sack::image::BlotImageEx@Image@Image@S_32@S_32@_32@_32@...>
                                                                                                                   
   Internal
	Interface index 18*/   IMAGE_PROC_PTR( void,BlotImageEx)     ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... );

 /* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>

   Internal
	Interface index 19*/   IMAGE_PROC_PTR( void,BlotImageSizedEx)( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... );

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
  Internal
   Interface index  20                                                                                                        */   IMAGE_PROC_PTR( void,BlotScaledImageSizedEx)( Image pifDest, Image pifSrc
                                   , S_32 xd, S_32 yd
                                   , _32 wd, _32 hd
                                   , S_32 xs, S_32 ys
                                   , _32 ws, _32 hs
                                   , _32 nTransparent
                                   , _32 method, ... );


/*Internal
   Interface index 21*/   DIMAGE_PROC_PTR( void,plot)      ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 22*/   DIMAGE_PROC_PTR( void,plotalpha) ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 23*/   DIMAGE_PROC_PTR( CDATA,getpixel) ( Image pi, S_32 x, S_32 y );
/*Internal
   Interface index 24*/   DIMAGE_PROC_PTR( void,do_line)     ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  // d is color data...
/*Internal
   Interface index 25*/   DIMAGE_PROC_PTR( void,do_lineAlpha)( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color);  // d is color data...

/*Internal
   Interface index 26*/   DIMAGE_PROC_PTR( void,do_hline)     ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 27*/   DIMAGE_PROC_PTR( void,do_vline)     ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
/*Internal
   Interface index 28*/   DIMAGE_PROC_PTR( void,do_hlineAlpha)( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 29*/   DIMAGE_PROC_PTR( void,do_vlineAlpha)( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );

/* <combine sack::image::GetDefaultFont>
   
   Internal
   Interface index 30                    */   IMAGE_PROC_PTR( SFTFont,GetDefaultFont) ( void );
/* <combine sack::image::GetFontHeight@SFTFont>
   
   Internal
   Interface index 31                                        */   IMAGE_PROC_PTR( _32 ,GetFontHeight)  ( SFTFont );
/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@SFTFont>
   
   Internal
   Interface index 32                                                          */   IMAGE_PROC_PTR( _32 ,GetStringSizeFontEx)( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont );

/* <combine sack::image::PutCharacterFont@Image@S_32@S_32@CDATA@CDATA@_32@SFTFont>
   
   Internal
   Interface index 33                                                           */   IMAGE_PROC_PTR( void,PutCharacterFont)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 34                                                                                        */   IMAGE_PROC_PTR( void,PutCharacterVerticalFont)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 35                                                                                      */   IMAGE_PROC_PTR( void,PutCharacterInvertFont)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 36                                                                                              */   IMAGE_PROC_PTR( void,PutCharacterVerticalInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );

/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 37                                                                                   */   IMAGE_PROC_PTR( void,PutStringFontEx)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 38                                                                                           */   IMAGE_PROC_PTR( void,PutStringVerticalFontEx)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 39                                                                                         */   IMAGE_PROC_PTR( void,PutStringInvertFontEx)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 40                                                                                                 */   IMAGE_PROC_PTR( void,PutStringInvertVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

/* <combine sack::image::GetMaxStringLengthFont@_32@SFTFont>
   
   Internal
   Interface index 41                                     */   IMAGE_PROC_PTR( _32, GetMaxStringLengthFont )( _32 width, SFTFont UseFont );

/* <combine sack::image::GetImageSize@Image@_32 *@_32 *>
   
   Internal
   Interface index 42                                                    */   IMAGE_PROC_PTR( void, GetImageSize)                ( Image pImage, _32 *width, _32 *height );
/* <combine sack::image::LoadFont@SFTFont>
   
   Internal
   Interface index 43                                   */   IMAGE_PROC_PTR( SFTFont, LoadFont )                   ( SFTFont font );
         /* <combine sack::image::UnloadFont@SFTFont>
            
            \ \                                    */
         IMAGE_PROC_PTR( void, UnloadFont )                 ( SFTFont font );

/* Internal
   Interface index 44
   
   This is used by internal methods to transfer image and font
   data to the render agent.                                   */   IMAGE_PROC_PTR( DataState, BeginTransferData )    ( _32 total_size, _32 segsize, CDATA data );
/* Internal
   Interface index 45
   
   Used internally to transfer data to render agent. */   IMAGE_PROC_PTR( void, ContinueTransferData )      ( DataState state, _32 segsize, CDATA data );
/* Internal
   Interface index 46
   
   Command issues at end of data transfer to decode the data
   into an image.                                            */   IMAGE_PROC_PTR( Image, DecodeTransferredImage )    ( DataState state );
/* After a data transfer decode the information as a font.
   Internal
   Interface index 47                                      */   IMAGE_PROC_PTR( SFTFont, AcceptTransferredFont )     ( DataState state );
/*Internal
   Interface index 48*/   DIMAGE_PROC_PTR( CDATA, ColorAverage )( CDATA c1, CDATA c2
                                              , int d, int max );
/* <combine sack::image::SyncImage>
   
   Internal
   Interface index 49               */   IMAGE_PROC_PTR( void, SyncImage )                 ( void );
         /* <combine sack::image::GetImageSurface@Image>
            
            \ \                                          */
         IMAGE_PROC_PTR( PCDATA, GetImageSurface )       ( Image pImage );
         /* <combine sack::image::IntersectRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
            
            \ \                                                                                             */
         IMAGE_PROC_PTR( int, IntersectRectangle )      ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::MergeRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
      
      \ \                                                                                         */
   IMAGE_PROC_PTR( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::GetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, GetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::SetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, SetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::OrphanSubImage@Image>
      
      \ \                                         */
   IMAGE_PROC_PTR( void, OrphanSubImage )  ( Image pImage );
   /* <combine sack::image::AdoptSubImage@Image@Image>
      
      \ \                                              */
   IMAGE_PROC_PTR( void, AdoptSubImage )   ( Image pFoster, Image pOrphan );
	/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
	   
	   \ \                                                         */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	/* <combine sack::image::MakeSpriteImageEx@Image image>
	   
	   \ \                                                  */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	/* <combine sack::image::rotate_scaled_sprite@Image@PSPRITE@fixed@fixed@fixed>
	   
	   \ \                                                                         */
	IMAGE_PROC_PTR( void   , rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	/* <combine sack::image::rotate_sprite@Image@PSPRITE@fixed>
	   
	   \ \                                                      */
	IMAGE_PROC_PTR( void   , rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
 /* <combine sack::image::BlotSprite@Image@PSPRITE>
	                                                  
	 Internal
   Interface index 61                                              */
		IMAGE_PROC_PTR( void   , BlotSprite )( Image pdest, PSPRITE ps );
    /* <combine sack::image::DecodeMemoryToImage@P_8@_32>
       
       \ \                                                */
    IMAGE_PROC_PTR( Image, DecodeMemoryToImage )( P_8 buf, _32 size );

   /* <combine sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32>
      
      \returns a SFTFont                                                      */
	IMAGE_PROC_PTR( SFTFont, InternalRenderFontFile )( CTEXTSTR file
																 , S_32 nWidth
																 , S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																 , _32 flags 
																 );
   /* <combine sack::image::InternalRenderFont@_32@_32@_32@S_32@S_32@_32>
      
      requires knowing the font cache....                                 */
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
   
   \ \                                                                       */
IMAGE_PROC_PTR( SFTFont, RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
/* <combine sack::image::RenderFontFileEx@CTEXTSTR@_32@_32@_32@P_32@POINTER *>
   
   \ \                                                                         */
IMAGE_PROC_PTR( SFTFont, RenderFontFileScaledEx )( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *size, POINTER *pFontData );
/* <combine sack::image::DestroyFont@SFTFont *>
   
   \ \                                       */
IMAGE_PROC_PTR( void, DestroyFont)( SFTFont *font );
/* <combine sack::image::GetGlobalFonts>
   
   global_font_data in interface is really a global font data. Don't
   have to call GetGlobalFont to get this.                           */
struct font_global_tag *_global_font_data;
/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@_32 *>
   
   \ \                                                           */
IMAGE_PROC_PTR( int, GetFontRenderData )( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
/* <combine sack::image::SetFontRendererData@SFTFont@POINTER@_32>
   
   \ \                                                         */
IMAGE_PROC_PTR( void, SetFontRendererData )( SFTFont font, POINTER pResult, size_t size );
/* <combine sack::image::SetSpriteHotspot@PSPRITE@S_32@S_32>
   
   \ \                                                       */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
/* <combine sack::image::SetSpritePosition@PSPRITE@S_32@S_32>
   
   \ \                                                        */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
	   
	   \ \                                                */
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
   
   \ \                                   */
IMAGE_PROC_PTR( struct font_global_tag *, GetGlobalFonts)( void );

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
   
   \ \                                                                                                                                                                     */
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


static IMAGE_INTERFACE ProxyImageInterface = {
/* <combine sack::image::SetStringBehavior@Image@_32>
   
   Internal
   Interface index 4                                  */ IMAGE_PROC_PTR( void, SetStringBehavior) ( Image pImage, _32 behavior );
/* <combine sack::image::SetBlotMethod@_32>
   
   \ \ 
   Internal
   Interface index 5                        */ IMAGE_PROC_PTR( void, SetBlotMethod)     ( _32 method );

/*
   Internal
   Interface index 6*/   IMAGE_PROC_PTR( Image,BuildImageFileEx) ( PCOLOR pc, _32 width, _32 height DBG_PASS);
/* <combine sack::image::MakeImageFileEx@_32@_32 Height>
   
   Internal
   Interface index 7*/  IMAGE_PROC_PTR( Image,MakeImageFileEx)  (_32 Width, _32 Height DBG_PASS);
/* <combine sack::image::MakeSubImageEx@Image@S_32@S_32@_32@_32 height>
   
   Internal
   Interface index 8                                                                    */   IMAGE_PROC_PTR( Image,MakeSubImageEx)   ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
/* <combine sack::image::RemakeImageEx@Image@PCOLOR@_32@_32 height>
   
   \ \ 
   
   <b>Internal</b>
   
   Interface index 9                                                */   IMAGE_PROC_PTR( Image,RemakeImageEx)    ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS);
/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
   
   Internal
   Interface index 10                                                   */  IMAGE_PROC_PTR( Image,LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
/* <combine sack::image::UnmakeImageFileEx@Image pif>
   
   Internal
   Interface index 11                                                 */  IMAGE_PROC_PTR( void,UnmakeImageFileEx) ( Image pif DBG_PASS );
#define UnmakeImageFile(pif) UnmakeImageFileEx( pif DBG_SRC )
/* <combine sack::image::SetImageBound@Image@P_IMAGE_RECTANGLE>
   
   Internal
   Interface index 12                                                           */  IMAGE_PROC_PTR( void ,SetImageBound)    ( Image pImage, P_IMAGE_RECTANGLE bound );
/* <combine sack::image::FixImagePosition@Image>
   
   Internal
   Interface index 13
   
   reset clip rectangle to the full image (subimage part ) Some
   operations (move, resize) will also reset the bound rect,
   this must be re-set afterwards. ALSO - one SHOULD be nice and
   reset the rectangle when done, otherwise other people may not
   have checked this.
                                                                 */  IMAGE_PROC_PTR( void ,FixImagePosition) ( Image pImage );

//-----------------------------------------------------

/* <combine sack::image::ResizeImageEx@Image@S_32@S_32 height>
   
   Internal
   Interface index 14                                                          */  IMAGE_PROC_PTR( void,ResizeImageEx)     ( Image pImage, S_32 width, S_32 height DBG_PASS);
/* <combine sack::image::MoveImage@Image@S_32@S_32>
   
   Internal
   Interface index 15                                               */   IMAGE_PROC_PTR( void,MoveImage)         ( Image pImage, S_32 x, S_32 y );

//-----------------------------------------------------

/* <combine sack::image::BlatColor@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 16                                                             */   IMAGE_PROC_PTR( void,BlatColor)     ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
/* <combine sack::image::BlatColorAlpha@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 17                                                                  */   IMAGE_PROC_PTR( void,BlatColorAlpha)( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

/* <combine sack::image::BlotImageEx@Image@Image@S_32@S_32@_32@_32@...>
                                                                                                                   
   Internal
	Interface index 18*/   IMAGE_PROC_PTR( void,BlotImageEx)     ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... );

 /* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>

   Internal
	Interface index 19*/   IMAGE_PROC_PTR( void,BlotImageSizedEx)( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... );

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
  Internal
   Interface index  20                                                                                                        */   IMAGE_PROC_PTR( void,BlotScaledImageSizedEx)( Image pifDest, Image pifSrc
                                   , S_32 xd, S_32 yd
                                   , _32 wd, _32 hd
                                   , S_32 xs, S_32 ys
                                   , _32 ws, _32 hs
                                   , _32 nTransparent
                                   , _32 method, ... );


/*Internal
   Interface index 21*/   DIMAGE_PROC_PTR( void,plot)      ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 22*/   DIMAGE_PROC_PTR( void,plotalpha) ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 23*/   DIMAGE_PROC_PTR( CDATA,getpixel) ( Image pi, S_32 x, S_32 y );
/*Internal
   Interface index 24*/   DIMAGE_PROC_PTR( void,do_line)     ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  // d is color data...
/*Internal
   Interface index 25*/   DIMAGE_PROC_PTR( void,do_lineAlpha)( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color);  // d is color data...

/*Internal
   Interface index 26*/   DIMAGE_PROC_PTR( void,do_hline)     ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 27*/   DIMAGE_PROC_PTR( void,do_vline)     ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
/*Internal
   Interface index 28*/   DIMAGE_PROC_PTR( void,do_hlineAlpha)( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 29*/   DIMAGE_PROC_PTR( void,do_vlineAlpha)( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );

/* <combine sack::image::GetDefaultFont>
   
   Internal
   Interface index 30                    */   IMAGE_PROC_PTR( SFTFont,GetDefaultFont) ( void );
/* <combine sack::image::GetFontHeight@SFTFont>
   
   Internal
   Interface index 31                                        */   IMAGE_PROC_PTR( _32 ,GetFontHeight)  ( SFTFont );
/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@SFTFont>
   
   Internal
   Interface index 32                                                          */   IMAGE_PROC_PTR( _32 ,GetStringSizeFontEx)( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont );

/* <combine sack::image::PutCharacterFont@Image@S_32@S_32@CDATA@CDATA@_32@SFTFont>
   
   Internal
   Interface index 33                                                           */   IMAGE_PROC_PTR( void,PutCharacterFont)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 34                                                                                        */   IMAGE_PROC_PTR( void,PutCharacterVerticalFont)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 35                                                                                      */   IMAGE_PROC_PTR( void,PutCharacterInvertFont)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 36                                                                                              */   IMAGE_PROC_PTR( void,PutCharacterVerticalInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font );

/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 37                                                                                   */   IMAGE_PROC_PTR( void,PutStringFontEx)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 38                                                                                           */   IMAGE_PROC_PTR( void,PutStringVerticalFontEx)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 39                                                                                         */   IMAGE_PROC_PTR( void,PutStringInvertFontEx)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 40                                                                                                 */   IMAGE_PROC_PTR( void,PutStringInvertVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

/* <combine sack::image::GetMaxStringLengthFont@_32@SFTFont>
   
   Internal
   Interface index 41                                     */   IMAGE_PROC_PTR( _32, GetMaxStringLengthFont )( _32 width, SFTFont UseFont );

/* <combine sack::image::GetImageSize@Image@_32 *@_32 *>
   
   Internal
   Interface index 42                                                    */   IMAGE_PROC_PTR( void, GetImageSize)                ( Image pImage, _32 *width, _32 *height );
/* <combine sack::image::LoadFont@SFTFont>
   
   Internal
   Interface index 43                                   */   IMAGE_PROC_PTR( SFTFont, LoadFont )                   ( SFTFont font );
         /* <combine sack::image::UnloadFont@SFTFont>
            
            \ \                                    */
         IMAGE_PROC_PTR( void, UnloadFont )                 ( SFTFont font );

/* Internal
   Interface index 44
   
   This is used by internal methods to transfer image and font
   data to the render agent.                                   */   IMAGE_PROC_PTR( DataState, BeginTransferData )    ( _32 total_size, _32 segsize, CDATA data );
/* Internal
   Interface index 45
   
   Used internally to transfer data to render agent. */   IMAGE_PROC_PTR( void, ContinueTransferData )      ( DataState state, _32 segsize, CDATA data );
/* Internal
   Interface index 46
   
   Command issues at end of data transfer to decode the data
   into an image.                                            */   IMAGE_PROC_PTR( Image, DecodeTransferredImage )    ( DataState state );
/* After a data transfer decode the information as a font.
   Internal
   Interface index 47                                      */   IMAGE_PROC_PTR( SFTFont, AcceptTransferredFont )     ( DataState state );
/*Internal
   Interface index 48*/   DIMAGE_PROC_PTR( CDATA, ColorAverage )( CDATA c1, CDATA c2
                                              , int d, int max );
/* <combine sack::image::SyncImage>
   
   Internal
   Interface index 49               */   IMAGE_PROC_PTR( void, SyncImage )                 ( void );
         /* <combine sack::image::GetImageSurface@Image>
            
            \ \                                          */
         IMAGE_PROC_PTR( PCDATA, GetImageSurface )       ( Image pImage );
         /* <combine sack::image::IntersectRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
            
            \ \                                                                                             */
         IMAGE_PROC_PTR( int, IntersectRectangle )      ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::MergeRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
      
      \ \                                                                                         */
   IMAGE_PROC_PTR( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::GetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, GetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::SetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, SetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::OrphanSubImage@Image>
      
      \ \                                         */
   IMAGE_PROC_PTR( void, OrphanSubImage )  ( Image pImage );
   /* <combine sack::image::AdoptSubImage@Image@Image>
      
      \ \                                              */
   IMAGE_PROC_PTR( void, AdoptSubImage )   ( Image pFoster, Image pOrphan );
	/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
	   
	   \ \                                                         */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	/* <combine sack::image::MakeSpriteImageEx@Image image>
	   
	   \ \                                                  */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	/* <combine sack::image::rotate_scaled_sprite@Image@PSPRITE@fixed@fixed@fixed>
	   
	   \ \                                                                         */
	IMAGE_PROC_PTR( void   , rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	/* <combine sack::image::rotate_sprite@Image@PSPRITE@fixed>
	   
	   \ \                                                      */
	IMAGE_PROC_PTR( void   , rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
 /* <combine sack::image::BlotSprite@Image@PSPRITE>
	                                                  
	 Internal
   Interface index 61                                              */
		IMAGE_PROC_PTR( void   , BlotSprite )( Image pdest, PSPRITE ps );
    /* <combine sack::image::DecodeMemoryToImage@P_8@_32>
       
       \ \                                                */
    IMAGE_PROC_PTR( Image, DecodeMemoryToImage )( P_8 buf, _32 size );

   /* <combine sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32>
      
      \returns a SFTFont                                                      */
	IMAGE_PROC_PTR( SFTFont, InternalRenderFontFile )( CTEXTSTR file
																 , S_32 nWidth
																 , S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																 , _32 flags 
																 );
   /* <combine sack::image::InternalRenderFont@_32@_32@_32@S_32@S_32@_32>
      
      requires knowing the font cache....                                 */
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
   
   \ \                                                                       */
IMAGE_PROC_PTR( SFTFont, RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
/* <combine sack::image::RenderFontFileEx@CTEXTSTR@_32@_32@_32@P_32@POINTER *>
   
   \ \                                                                         */
IMAGE_PROC_PTR( SFTFont, RenderFontFileScaledEx )( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *size, POINTER *pFontData );
/* <combine sack::image::DestroyFont@SFTFont *>
   
   \ \                                       */
IMAGE_PROC_PTR( void, DestroyFont)( SFTFont *font );
/* <combine sack::image::GetGlobalFonts>
   
   global_font_data in interface is really a global font data. Don't
   have to call GetGlobalFont to get this.                           */
struct font_global_tag *_global_font_data;
/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@_32 *>
   
   \ \                                                           */
IMAGE_PROC_PTR( int, GetFontRenderData )( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
/* <combine sack::image::SetFontRendererData@SFTFont@POINTER@_32>
   
   \ \                                                         */
IMAGE_PROC_PTR( void, SetFontRendererData )( SFTFont font, POINTER pResult, size_t size );
/* <combine sack::image::SetSpriteHotspot@PSPRITE@S_32@S_32>
   
   \ \                                                       */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
/* <combine sack::image::SetSpritePosition@PSPRITE@S_32@S_32>
   
   \ \                                                        */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
	   
	   \ \                                                */
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
   
   \ \                                   */
IMAGE_PROC_PTR( struct font_global_tag *, GetGlobalFonts)( void );

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
   
   \ \                                                                                                                                                                     */
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
};

static IMAGE_3D_INTERFACE Proxy3dImageInterface = {
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

PRELOAD( RegisterProxyInterface )
{
   	RegisterInterface( WIDE( "sack.image.proxy.server" ), GetProxyImageInterface, DropProxyImageInterface );
   	RegisterInterface( WIDE( "sack.image.3d.proxy.server" ), Get3dProxyImageInterface, Drop3dProxyImageInterface );
   	RegisterInterface( WIDE( "sack.render.proxy.server" ), GetProxyDisplayInterface, DropProxyDisplayInterface );
   	RegisterInterface( WIDE( "sack.render.3d.proxy.server" ), Get3dProxyDisplayInterface, Drop3dProxyDisplayInterface );

}


