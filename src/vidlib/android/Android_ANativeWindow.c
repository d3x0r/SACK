

#include "Android_local.h"



static IMAGE_INTERFACE AndroidANWImageInterface;


static void ClearDirtyFlag( Image image )
{
	//lprintf( "Clear dirty on %p  (%p) %08x", image, (image)?image->image:NULL, image?image->image->flags:0 );
	for( ; image; image = image->pElder )
	{
		if( image )
		{
			image->flags &= ~IF_FLAG_UPDATED;
			//lprintf( "Clear dirty on %08x", (image)?(image)->image->flags:0 );
		}
		if( image->pChild )
			ClearDirtyFlag( image->pChild );
		if( image->flags & IF_FLAG_FINAL_RENDER )
		{
         // unlock(?)
		}
	}
}


static int CPROC AndroidANW_InitDisplay( void )
{
	return TRUE;
}

static void CPROC AndroidANW_SetApplicationIcon( Image icon )
{
	// no support
}

static LOGICAL CPROC AndroidANW_RequiresDrawAll( void )
{
	// force application to mostly draw itself...
	return FALSE;
}

static LOGICAL CPROC AndroidANW_AllowsAnyThreadToUpdate( void )
{
	// doesn't matter what thread updates to the render surface?
   // does matter who outputs?
	return TRUE;
}

static void CPROC AndroidANW_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = StrDup( title );
}

static void CPROC AndroidANW_GetDisplaySizeEx( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height)
{
	if( x )
		(*x) = 0;
	if( y )
		(*y) = 0;
	if( width )
		(*width) = l.default_display_x;
	if( height )
		(*height) = l.default_display_y;

}

static void CPROC AndroidANW_GetDisplaySize( _32 *width, _32 *height )
{
   AndroidANW_GetDisplaySizeEx( 0, NULL, NULL, width, height );
}

static void CPROC AndroidANW_SetDisplaySize		( _32 width, _32 height )
{
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Width", width );
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Height", height );
}



static PRENDERER CPROC AndroidANW_OpenDisplayAboveUnderSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under )
{

	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );
	AddLink( &l.renderers, Renderer );
	Renderer->id = FindLink( &l.renderers, Renderer );
	Renderer->x = (x == -1)?0:x;
	Renderer->y = (y == -1)?0:x;
	Renderer->w = width;
	Renderer->h = height;
	Renderer->attributes = attributes;
	if( !l.bottom )
		l.bottom = Renderer;

	if( Renderer->above = (PVPRENDER)above )
		Renderer->above->under = Renderer;

	if( Renderer->under = (PVPRENDER)under )
      Renderer->under->above = Renderer;
	Renderer->image = MakeImageFileEx( width, height DBG_SRC );
   ClearImageTo( Renderer->image, 0 );
	return (PRENDERER)Renderer;
}

static PRENDERER CPROC AndroidANW_OpenDisplayAboveSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, above, NULL );
}

static PRENDERER CPROC AndroidANW_OpenDisplaySizedAt	  ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, NULL, NULL );
}


static void CPROC  AndroidANW_CloseDisplay ( PRENDERER Renderer )
{
	UnmakeImageFileEx( (Image)(((PVPRENDER)Renderer)->image) DBG_SRC );
	DeleteLink( &l.renderers, Renderer );
	Release( Renderer );
}

static void CPROC AndroidANW_UpdateDisplayPortionEx( PRENDERER r, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	// no-op; it will ahve already displayed(?)
	ARect bounds;
	ANativeWindow_Buffer buffer;
	S_32 out_x = ((PVPRENDER)r)->x;
	S_32 out_y = ((PVPRENDER)r)->y;
	S_32 ofs_x = 0;
	S_32 ofs_y = 0;
	//lprintf( "Update %d,%d to %d,%d on %d,%d %d,%d",
	//		  x, y, width, height
   //        , out_x, out_y, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h );
	if( out_x < 0 )
	{
		x +=  out_x;
      ofs_x = -out_x;
      out_x = 0;
		if( x < 0 )
		{
			if( width < -x )
            return;
			width += x;
         // this is resulting as 0
			x = 0;
		}
	}
	else
      ofs_x = 0;
	if( out_y < 0 )
	{
		y +=  out_y;
      ofs_y = -out_y;
      out_y = 0;
		if( y < 0 )
		{
			if( height < -y )
            return;
			height += y;
			y = 0;
		}
	}
	else
      ofs_y = 0;

	if( width != ((PVPRENDER)r)->w && height != ((PVPRENDER)r)->h
      || width != l.default_display_x || height != l.default_display_y
		|| x || y )
	{
		ofs_x += x;
		ofs_y += y;

		bounds.left = out_x + x;
		bounds.top = out_y + y;
		bounds.right = bounds.left + width;
		bounds.bottom = bounds.top + height;

		lprintf(" Updating from %d,%d %d,%d to %d,%d  %d,%d"
				 , ofs_x, ofs_y, width, height
				 , bounds.left, bounds.top, l.default_display_x, l.default_display_y );
		ANativeWindow_lock( l.displayWindow, &buffer, &bounds );
      if( bounds.right - bounds.left < width )
			width = ( bounds.right - bounds.left );
      if( ( bounds.bottom - bounds.top ) < height )
			height = bounds.bottom - bounds.top;
      // output bounds is updates to be the size actually needed.

		{
			int row;
         lprintf( "buffer is %d %d buffer stride is %d  pwidth is %d width is %d", bounds.top, bounds.left, buffer.stride, ((PVPRENDER)r)->image->pwidth, width );
			for( row = 0; row < height; row++ )
			{
				memcpy(((_32*)buffer.bits) + buffer.stride * ( bounds.top + row ) + bounds.left
						, ((PVPRENDER)r)->image->image + ((PVPRENDER)r)->image->pwidth * ( ofs_y + row ) + ofs_x
						, width * sizeof( CDATA ) );
			}
		}
	}
	else
	{
      //lprintf( "update full image..." );
		ANativeWindow_lock( l.displayWindow, &buffer, NULL );
		memcpy(buffer.bits , ((PVPRENDER)r)->image->image, height * width * 4 );
	}
   //_lprintf(DBG_RELAY)( "unlock... ( did we not lock?" );
	ANativeWindow_unlockAndPost(l.displayWindow);
}

static void CPROC AndroidANW_UpdateDisplayEx( PRENDERER r DBG_PASS)
{
	// no-op; it will ahve already displayed(?)
   AndroidANW_UpdateDisplayPortionEx( r, 0, 0, ((PVPRENDER)r)->w, ((PVPRENDER)r)->h DBG_RELAY );
}

static void CPROC AndroidANW_GetDisplayPosition ( PRENDERER r, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	PVPRENDER pRender = (PVPRENDER)r;
	if( r )
	{
      lprintf( "Get display pos of %p into %p %p %p %p", r, x, y, width, height );
		if( x )
			(*x) = pRender->x;
		if( y )
			(*y) = pRender->y;
		if( width )
			(*width) = pRender->w;
		if( height )
			(*height) = pRender->h;
	}
	else
      lprintf( WIDE( "WHy are you getting position of NULL?") );
}

static void CPROC AndroidANW_MoveSizeDisplay( PRENDERER r
													 , S_32 x, S_32 y
													 , S_32 w, S_32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
   //lprintf( "move size %d %d   %d %d", x, y, w, h );
	pRender->x = x;
	pRender->y = y;
	pRender->w = w;
	pRender->h = h;

   AndroidANW_UpdateDisplayEx( r DBG_SRC );
}

static void CPROC AndroidANW_MoveDisplay		  ( PRENDERER r, S_32 x, S_32 y )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								x,
								y,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_MoveDisplayRel( PRENDERER r, S_32 delx, S_32 dely )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_SizeDisplay( PRENDERER r, _32 w, _32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								w,
								h
								);
}

static void CPROC AndroidANW_SizeDisplayRel( PRENDERER r, S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC AndroidANW_MoveSizeDisplayRel( PRENDERER r
																 , S_32 delx, S_32 dely
																 , S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC AndroidANW_PutDisplayAbove		( PRENDERER r, PRENDERER above )
{
	lprintf( "window ordering is not implemented" );
}

static Image CPROC AndroidANW_GetDisplayImage( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return (Image)pRender->image;
}

static void CPROC AndroidANW_SetCloseHandler	 ( PRENDERER r, CloseCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_SetMouseHandler  ( PRENDERER r, MouseCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->mouse_callback = c;
	render->psv_mouse_callback = p;
}

static void CPROC AndroidANW_Redraw( PRENDERER r )
{
	PVPRENDER render = (PVPRENDER)r;
	lprintf( "Sending application draw.... %p %p", render?render->redraw:0, render );
	if( render->redraw )
		render->redraw( render->psv_redraw, (PRENDERER)render );
   lprintf( "updating to the displa" );
   AndroidANW_UpdateDisplayEx( r DBG_SRC );
}

static void CPROC AndroidANW_SetRedrawHandler  ( PRENDERER r, RedrawCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->redraw = c;
	render->psv_redraw = p;
}

static void CPROC AndroidANW_SetKeyboardHandler	( PRENDERER r, KeyProc c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->key_callback = c;
	render->psv_key_callback = p;
}

static void CPROC AndroidANW_SetLoseFocusHandler  ( PRENDERER r, LoseFocusCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_GetMousePosition	( S_32 *x, S_32 *y )
{
}

static void CPROC AndroidANW_SetMousePosition  ( PRENDERER r, S_32 x, S_32 y )
{
}

static LOGICAL CPROC AndroidANW_HasFocus		 ( PRENDERER  r )
{
	return TRUE;
}

static _32 CPROC AndroidANW_IsKeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static _32 CPROC AndroidANW_KeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static LOGICAL CPROC AndroidANW_DisplayIsValid ( PRENDERER r )
{
	return (r != NULL);
}

static void CPROC AndroidANW_OwnMouseEx ( PRENDERER r, _32 Own DBG_PASS)
{

}

static int CPROC AndroidANW_BeginCalibration ( _32 points )
{
	return 0;
}

static void CPROC AndroidANW_SyncRender( PRENDERER pDisplay )
{
}

static void CPROC AndroidANW_MakeTopmost  ( PRENDERER r )
{
}

static void CPROC AndroidANW_HideDisplay	 ( PRENDERER r )
{
}

static void CPROC AndroidANW_RestoreDisplay  ( PRENDERER r )
{
   AndroidANW_Redraw( r );
}

static void CPROC AndroidANW_ForceDisplayFocus ( PRENDERER r )
{
}

static void CPROC AndroidANW_ForceDisplayFront( PRENDERER r )
{
}

static void CPROC AndroidANW_ForceDisplayBack( PRENDERER r )
{
}

static int CPROC  AndroidANW_BindEventToKey( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	return 0;
}

static int CPROC AndroidANW_UnbindKey( PRENDERER pRenderer, _32 scancode, _32 modifier )
{
	return 0;
}

static int CPROC AndroidANW_IsTopmost( PRENDERER r )
{
	return 0;
}

static void CPROC AndroidANW_OkaySyncRender( void )
{
	// redundant thing?
}

static int CPROC AndroidANW_IsTouchDisplay( void )
{
	return 1;
}

static void CPROC AndroidANW_GetMouseState( S_32 *x, S_32 *y, _32 *b )
{
}

static PSPRITE_METHOD CPROC AndroidANW_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	return NULL;
}

static void CPROC AndroidANW_WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
}

static void CPROC AndroidANW_PutDisplayIn(PRENDERER r, PRENDERER hContainer)
{
}

static void CPROC AndroidANW_SetRendererTitle( PRENDERER render, const TEXTCHAR *title )
{
}

static void CPROC AndroidANW_DisableMouseOnIdle(PRENDERER r, LOGICAL bEnable )
{
}

static void CPROC AndroidANW_SetDisplayNoMouse( PRENDERER r, int bNoMouse )
{
}

static void CPROC AndroidANW_MakeAbsoluteTopmost(PRENDERER r)
{
}

static void CPROC AndroidANW_SetDisplayFade( PRENDERER r, int level )
{
}

static LOGICAL CPROC AndroidANW_IsDisplayHidden( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return pRender->flags.hidden;
}

#ifdef WIN32
static HWND CPROC AndroidANW_GetNativeHandle( PRENDERER r )
{
}
#endif

static void CPROC AndroidANW_LockRenderer( PRENDERER render )
{
}

static void CPROC AndroidANW_UnlockRenderer( PRENDERER render )
{
}

static void CPROC AndroidANW_IssueUpdateLayeredEx( PRENDERER r, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
}


static void CPROC AndroidANW_SetTouchHandler  ( PRENDERER r, TouchCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->touch_callback = c;
	render->psv_touch_callback = p;
}

static void CPROC AndroidANW_MarkDisplayUpdated( PRENDERER r  )
{
}

static void CPROC AndroidANW_SetHideHandler		( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_SetRestoreHandler  ( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC AndroidANW_RestoreDisplayEx ( PRENDERER r DBG_PASS )
{
	((PVPRENDER)r)->redraw( ((PVPRENDER)r)->psv_redraw, r );
}



static RENDER_INTERFACE AndroidANWInterface = {
	AndroidANW_InitDisplay
													  , AndroidANW_SetApplicationTitle
													  , AndroidANW_SetApplicationIcon
													  , AndroidANW_GetDisplaySize
													  , AndroidANW_SetDisplaySize
													  , AndroidANW_OpenDisplaySizedAt
													  , AndroidANW_OpenDisplayAboveSizedAt
													  , AndroidANW_CloseDisplay
													  , AndroidANW_UpdateDisplayPortionEx
													  , AndroidANW_UpdateDisplayEx
													  , AndroidANW_GetDisplayPosition
													  , AndroidANW_MoveDisplay
													  , AndroidANW_MoveDisplayRel
													  , AndroidANW_SizeDisplay
													  , AndroidANW_SizeDisplayRel
													  , AndroidANW_MoveSizeDisplayRel
													  , AndroidANW_PutDisplayAbove
													  , AndroidANW_GetDisplayImage
													  , AndroidANW_SetCloseHandler
													  , AndroidANW_SetMouseHandler
													  , AndroidANW_SetRedrawHandler
													  , AndroidANW_SetKeyboardHandler
	 /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
		 
		 \ \																												 */
													  , AndroidANW_SetLoseFocusHandler
			 ,  0  //POINTER junk1;
													  , AndroidANW_GetMousePosition
													  , AndroidANW_SetMousePosition
													  , AndroidANW_HasFocus

													  , AndroidANW_GetKeyText
													  , AndroidANW_IsKeyDown
													  , AndroidANW_KeyDown
													  , AndroidANW_DisplayIsValid
													  , AndroidANW_OwnMouseEx
													  , AndroidANW_BeginCalibration
													  , AndroidANW_SyncRender

													  , AndroidANW_MoveSizeDisplay
													  , AndroidANW_MakeTopmost
													  , AndroidANW_HideDisplay
													  , AndroidANW_RestoreDisplay
													  , AndroidANW_ForceDisplayFocus
													  , AndroidANW_ForceDisplayFront
													  , AndroidANW_ForceDisplayBack
													  , AndroidANW_BindEventToKey
													  , AndroidANW_UnbindKey
													  , AndroidANW_IsTopmost
													  , AndroidANW_OkaySyncRender
													  , AndroidANW_IsTouchDisplay
													  , AndroidANW_GetMouseState
													  , AndroidANW_EnableSpriteMethod
													  , AndroidANW_WinShell_AcceptDroppedFiles
													  , AndroidANW_PutDisplayIn
													  , NULL // make renderer from native handle (junk4)
													  , AndroidANW_SetRendererTitle
													  , AndroidANW_DisableMouseOnIdle
													  , AndroidANW_OpenDisplayAboveUnderSizedAt
													  , AndroidANW_SetDisplayNoMouse
													  , AndroidANW_Redraw
													  , AndroidANW_MakeAbsoluteTopmost
													  , AndroidANW_SetDisplayFade
													  , AndroidANW_IsDisplayHidden
#ifdef WIN32
													, NULL // get native handle from renderer
#endif
													  , AndroidANW_GetDisplaySizeEx

													  , AndroidANW_LockRenderer
													  , AndroidANW_UnlockRenderer
													  , AndroidANW_IssueUpdateLayeredEx
													  , AndroidANW_RequiresDrawAll
#ifndef NO_TOUCH
													  , AndroidANW_SetTouchHandler
#endif
													  , AndroidANW_MarkDisplayUpdated
													  , AndroidANW_SetHideHandler
													  , AndroidANW_SetRestoreHandler
													  , AndroidANW_RestoreDisplayEx
												, SACK_Vidlib_ShowInputDevice
												, SACK_Vidlib_HideInputDevice
												, AndroidANW_AllowsAnyThreadToUpdate
};

static void InitAndroidANWInterface( void )
{
	AndroidANWInterface._RequiresDrawAll = AndroidANW_RequiresDrawAll;
}


static POINTER CPROC GetAndroidANWDisplayInterface( void )
{
	// open server socket
	return &AndroidANWInterface;
}
static void CPROC DropAndroidANWDisplayInterface( POINTER i )
{
	// close connections
}

PRIORITY_PRELOAD( RegisterAndroidNativeWindowInterface, VIDLIB_PRELOAD_PRIORITY )
{
	LoadFunction( "libbag.image.so", NULL );
	l.real_interface = (PIMAGE_INTERFACE)GetInterface( "sack.image" );
	RegisterInterface( WIDE( "sack.render.android" ), GetAndroidANWDisplayInterface, DropAndroidANWDisplayInterface );
	InitAndroidANWInterface();
}


//-------------- Android Interface ------------------

static void HostSystem_InitDisplayInfo(void )
{
	// this is passed in from the external world; do nothing, but provide the hook.
	// have to wait for this ....
	//default_display_x	ANativeWindow_getFormat( camera->displayWindow)

}

void SACK_Vidlib_SetNativeWindowHandle( ANativeWindow *displayWindow )
{
   //lprintf( "Setting native window handle... (shouldn't this do something else?)" );
	l.displayWindow = displayWindow;

	l.default_display_x = ANativeWindow_getWidth( l.displayWindow);
	l.default_display_y = ANativeWindow_getHeight( l.displayWindow);

	ANativeWindow_setBuffersGeometry( displayWindow,l.default_display_x,l.default_display_y,WINDOW_FORMAT_RGBA_8888);

	lprintf( "Format is :%dx%d %d", l.default_display_x, l.default_display_y, ANativeWindow_getFormat( displayWindow ) );

}


void SACK_Vidlib_DoFirstRender( void )
{
	/* no render pass; should return FALSE or somethig to stop animating... */
	INDEX idx;
	PVPRENDER render;
	LIST_FORALL( l.renderers, idx, PVPRENDER, render )
	{
      lprintf( "RENDER PASS UPDATE..." );
		AndroidANW_UpdateDisplayEx( (PRENDERER)render DBG_SRC );
	}
}


void SACK_Vidlib_OpenCameras( void )
{
   /* no cameras to open; this is on screen rotation; maybe re-set the window handle (see above)*/
}


int SACK_Vidlib_SendTouchEvents( int nPoints, PINPUT_POINT points )
{
	INDEX idx;
	PVPRENDER render;
   //lprintf( "Received touch %d", nPoints );
	LIST_FORALL( l.renderers, idx, PVPRENDER, render )
	{
		if( !HandleTouches( render, points, nPoints ) )
			if( render->touch_callback )
			{
				//lprintf( "And somenoe can handle them.." );
				if( render->touch_callback( render->psv_touch_callback, points, nPoints ) )
					return;
			}
	}
}

void SACK_Vidlib_CloseDisplay( void )
{
   // not much to do...
}


