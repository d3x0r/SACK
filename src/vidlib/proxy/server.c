
#include <render.h>
#include <render3d.h>

#include "server_local.h"

static void SendMessage( enum proxy_message_id message, ... )
{
	INDEX idx;
	PCLIENT client;
	LIST_FORALL( l.clients, idx, PCLIENT, client )
	{
		switch( message )
		{
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

static void InitService( void )
{
	if( !l.listener )
	{
		NetworkStart();
		l.listener = OpenTCPListenerAddrEx( CreateSockAddress( "0.0.0.0", 4241 ), Connected, 0 );
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
    RENDER_PROC_PTR( void, MoveDisplay)          ( PRENDERER, S_32 x, S_32 y );
    RENDER_PROC_PTR( void, MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely );
    RENDER_PROC_PTR( void, SizeDisplay)          ( PRENDERER, _32 w, _32 h );
    RENDER_PROC_PTR( void, SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh );
    RENDER_PROC_PTR( void, MoveSizeDisplayRel )  ( PRENDERER hVideo
                                                 , S_32 delx, S_32 dely
                                                 , S_32 delw, S_32 delh );
    RENDER_PROC_PTR( void, PutDisplayAbove)      ( PRENDERER, PRENDERER ); /* <combine sack::image::render::PutDisplayAbove@PRENDERER@PRENDERER>
                                                              
                                                              \ \                                                                */
 
    /* <combine sack::image::render::GetDisplayImage@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( Image, GetDisplayImage)     ( PRENDERER );

    /* <combine sack::image::render::SetCloseHandler@PRENDERER@CloseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetMouseHandler@PRENDERER@MouseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRedrawHandler@PRENDERER@RedrawCallback@PTRSZVAL>
       
       \ \                                                                               */
    RENDER_PROC_PTR( void, SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL );
    /* <combine sack::image::render::SetKeyboardHandler@PRENDERER@KeyProc@PTRSZVAL>
       
       \ \                                                                          */
    RENDER_PROC_PTR( void, SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
       
       \ \                                                                                     */
    RENDER_PROC_PTR( void, SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
    /* <combine sack::image::render::SetDefaultHandler@PRENDERER@GeneralCallback@PTRSZVAL>
       
       \ \                                                                                 */
#if ACTIVE_MESSAGE_IMPLEMENTED
			 RENDER_PROC_PTR( void, SetDefaultHandler)    ( PRENDERER, GeneralCallback, PTRSZVAL );
#else
       POINTER junk1;
#endif
    /* <combine sack::image::render::GetMousePosition@S_32 *@S_32 *>
       
		 \ \                                                           */
    RENDER_PROC_PTR( void, GetMousePosition)     ( S_32 *x, S_32 *y );
    /* <combine sack::image::render::SetMousePosition@PRENDERER@S_32@S_32>
       
       \ \                                                                 */
    RENDER_PROC_PTR( void, SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );

    /* <combine sack::image::render::HasFocus@PRENDERER>
       
       \ \                                               */
    RENDER_PROC_PTR( LOGICAL, HasFocus)          ( PRENDERER );

#if ACTIVE_MESSAGE_IMPLEMENTED
    RENDER_PROC_PTR( int, SendActiveMessage)     ( PRENDERER dest, PACTIVEMESSAGE msg );
    RENDER_PROC_PTR( PACTIVEMESSAGE , CreateActiveMessage) ( int ID, int size, ... );
#else
	 /* Just a place holder value. Some function used to be here. */
	 POINTER junk2;
	 /* Place Holder value - depricated functions in interface. */
	 POINTER junk3;
#endif
    /* <combine sack::image::render::GetKeyText@int>
       
       \ \                                           */
    RENDER_PROC_PTR( TEXTCHAR, GetKeyText)           ( int key );
    /* <combine sack::image::render::IsKeyDown@PRENDERER@int>
       
       \ \                                                    */
    RENDER_PROC_PTR( _32, IsKeyDown )        ( PRENDERER display, int key );
    /* <combine sack::image::render::KeyDown@PRENDERER@int>
       
       \ \                                                  */
    RENDER_PROC_PTR( _32, KeyDown )         ( PRENDERER display, int key );
    /* <combine sack::image::render::DisplayIsValid@PRENDERER>
       
       \ \                                                     */
    RENDER_PROC_PTR( LOGICAL, DisplayIsValid )  ( PRENDERER display );
    /* <combine sack::image::render::OwnMouseEx@PRENDERER@_32 bOwn>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS);
    /* <combine sack::image::render::BeginCalibration@_32>
       
       \ \                                                 */
    RENDER_PROC_PTR( int, BeginCalibration )       ( _32 points );
    /* <combine sack::image::render::SyncRender@PRENDERER>
       
       \ \                                                 */
    RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, EnableOpenGL )           ( PRENDERER hVideo );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, SetActiveGLDisplay )     ( PRENDERER hDisplay );

	 /* <combine sack::image::render::MoveSizeDisplay@PRENDERER@S_32@S_32@S_32@S_32>
	    
	    \ \                                                                          */
	 RENDER_PROC_PTR( void, MoveSizeDisplay )( PRENDERER hVideo
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

};

static RENDER3D_INTERFACE Proxy3dInterface = {
};

static IMAGE_INTERFACE ProxyImageInterface = {
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


