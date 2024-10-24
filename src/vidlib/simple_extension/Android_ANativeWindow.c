
#include <imglib/imagestruct.h>

#include <render.h>
#include <render3d.h>
#include <image3d.h>
#include <sqlgetoption.h>
#include "server_local.h"



static IMAGE_INTERFACE ProxyImageInterface;


static void ClearDirtyFlag( PVPImage image )
{
	//lprintf( "Clear dirty on %p  (%p) %08x", image, (image)?image->image:NULL, image?image->image->flags:0 );
	for( ; image; image = image->next )
	{
		if( image->image )
		{
			image->image->flags &= ~IF_FLAG_UPDATED;
			//lprintf( "Clear dirty on %08x", (image)?(image)->image->flags:0 );
		}
		if( image->child )
			ClearDirtyFlag( image->child );
		if( image->flage & IF_FLAG_FINAL_RENDER )
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
	return FALSE;
}

static void CPROC AndroidANW_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = StrDup( title );
}

static void CPROC AndroidANW_GetDisplaySizeEx( int nDisplay
														  , int32_t *x, int32_t *y
														  , uint32_t *width, uint32_t *height)
{
	if( x )
		(*x) = 0;
	if( y )
		(*y) = 0;
   lprintf( "If you haven't fixed it; size comes from the static android display we got" );
	if( width )
		(*width) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Width", 1024 );
	if( height )
		(*height) = SACK_GetProfileInt( "SACK/Vidlib", "Default Display Height", 768 );

}

static void CPROC AndroidANW_GetDisplaySize( uint32_t *width, uint32_t *height )
{
   AndroidANW_GetDisplaySizeEx( 0, NULL, NULL, width, height );
}

static void CPROC AndroidANW_SetDisplaySize		( uint32_t width, uint32_t height )
{
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Width", width );
	//SACK_WriteProfileInt( "SACK/Vidlib", "Default Display Height", height );
}


static PVPImage Internal_MakeImageFileEx ( INDEX iRender, uint32_t Width, uint32_t Height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = Width;
	image->h = Height;
	image->render_id = iRender;
	image->image = l.real_interface->_MakeImageFileEx( Width, Height DBG_RELAY );
	image->image->reverse_interface = &ProxyImageInterface;
	image->image->reverse_interface_instance = image;
	if( iRender != INVALID_INDEX )
		image->image->flags |= IF_FLAG_FINAL_RENDER;
	AddLink( &l.images, image );
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make proxy image %p %d(%d,%d)", image, image->id, Width, Height );
	return image;
}

static PVPImage WrapImageFile( Image native )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = native->width;
	image->h = native->height;
	image->render_id = INVALID_INDEX;
	image->image = native;
	image->image->reverse_interface = &ProxyImageInterface;
	image->image->reverse_interface_instance = image;
	AddLink( &l.images, image );
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make wrapped proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	return image;
}


static Image CPROC AndroidANW_MakeImageFileEx (uint32_t Width, uint32_t Height DBG_PASS)
{
	return (Image)Internal_MakeImageFileEx( INVALID_INDEX, Width, Height DBG_RELAY );
}


static PRENDERER CPROC AndroidANW_OpenDisplayAboveUnderSizedAt( uint32_t attributes, uint32_t width, uint32_t height, int32_t x, int32_t y, PRENDERER above, PRENDERER under )
{

	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );
	AddLink( &l.renderers, Renderer );
	Renderer->id = FindLink( &l.renderers, Renderer );
	Renderer->x = x;
	Renderer->y = y;
	Renderer->w = width;
	Renderer->h = height;
	Renderer->attributes = attributes;
	Renderer->above = (PVPRENDER)above;
	Renderer->under = (PVPRENDER)under;
	Renderer->image = Internal_MakeImageFileEx( Renderer->id, width, height DBG_SRC );
	
	return (PRENDERER)Renderer;
}

static PRENDERER CPROC AndroidANW_OpenDisplayAboveSizedAt( uint32_t attributes, uint32_t width, uint32_t height, int32_t x, int32_t y, PRENDERER above )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, above, NULL );
}

static PRENDERER CPROC AndroidANW_OpenDisplaySizedAt	  ( uint32_t attributes, uint32_t width, uint32_t height, int32_t x, int32_t y )
{
	return AndroidANW_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, NULL, NULL );
}


/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
	
	Internal
	Interface index 10																	*/  IMAGE_PROC_PTR( Image,AndroidANW_LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
static  void CPROC AndroidANW_UnmakeImageFileEx( Image pif DBG_PASS )
{
	if( pif )
	{
		PVPImage image = (PVPImage)pif;
		//lprintf( "UNMake proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
		SetLink( &l.images, ((PVPImage)pif)->id, NULL );
		if( ((PVPImage)pif)->image->reverse_interface )
		{
			((PVPImage)pif)->image->reverse_interface = NULL;
			((PVPImage)pif)->image->reverse_interface_instance = 0;
			l.real_interface->_UnmakeImageFileEx( ((PVPImage)pif)->image DBG_RELAY );
		}
		Release( pif );
	}
}

static void CPROC  AndroidANW_CloseDisplay ( PRENDERER Renderer )
{
	AndroidANW_UnmakeImageFileEx( (Image)(((PVPRENDER)Renderer)->image) DBG_SRC );
	DeleteLink( &l.renderers, Renderer );
	Release( Renderer );
}

static void CPROC AndroidANW_UpdateDisplayPortionEx( PRENDERER r, int32_t x, int32_t y, uint32_t width, uint32_t height DBG_PASS )
{
	// no-op; it will ahve already displayed(?)
}

static void CPROC AndroidANW_UpdateDisplayEx( PRENDERER r DBG_PASS)
{
	// no-op; it will ahve already displayed(?)

}

static void CPROC AndroidANW_GetDisplayPosition ( PRENDERER r, int32_t *x, int32_t *y, uint32_t *width, uint32_t *height )
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

static void CPROC AndroidANW_MoveSizeDisplay( PRENDERER r
													 , int32_t x, int32_t y
													 , int32_t w, int32_t h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->x = x;
	pRender->y = y;
	pRender->w = w;
	pRender->h = h;
}

static void CPROC AndroidANW_MoveDisplay		  ( PRENDERER r, int32_t x, int32_t y )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								x,
								y,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_MoveDisplayRel( PRENDERER r, int32_t delx, int32_t dely )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w,
								pRender->h
								);
}

static void CPROC AndroidANW_SizeDisplay( PRENDERER r, uint32_t w, uint32_t h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	AndroidANW_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								w,
								h
								);
}

static void CPROC AndroidANW_SizeDisplayRel( PRENDERER r, int32_t delw, int32_t delh )
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
																 , int32_t delx, int32_t dely
																 , int32_t delw, int32_t delh )
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

static void CPROC AndroidANW_SetCloseHandler	 ( PRENDERER r, CloseCallback c, uintptr_t p )
{
}

static void CPROC AndroidANW_SetMouseHandler  ( PRENDERER r, MouseCallback c, uintptr_t p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->mouse_callback = c;
	render->psv_mouse_callback = p;
}

static void CPROC AndroidANW_SetRedrawHandler  ( PRENDERER r, RedrawCallback c, uintptr_t p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->redraw = c;
	render->psv_redraw = p;
}

static void CPROC AndroidANW_SetKeyboardHandler	( PRENDERER r, KeyProc c, uintptr_t p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->key_callback = c;
	render->psv_key_callback = p;
}

static void CPROC AndroidANW_SetLoseFocusHandler  ( PRENDERER r, LoseFocusCallback c, uintptr_t p )
{
}

static void CPROC AndroidANW_GetMousePosition	( int32_t *x, int32_t *y )
{
}

static void CPROC AndroidANW_SetMousePosition  ( PRENDERER r, int32_t x, int32_t y )
{
}

static LOGICAL CPROC AndroidANW_HasFocus		 ( PRENDERER  r )
{
	return TRUE;
}

static TEXTCHAR CPROC AndroidANW_GetKeyText		 ( int key )
{ 
	int c;
	char ch[5];
#ifdef __LINUX__
	{
		int used = 0;
		CTEXTSTR text = SACK_Vidlib_GetKeyText( IsKeyPressed( key ), KEY_CODE( key ), &used );
		if( used && text )
		{
			return text[0];
		}
	}
   return 0;
#else
	if( key & KEY_MOD_DOWN )
		return 0;
	key ^= 0x80000000;
	c =  
#  ifndef UNDER_CE
		ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
					l.key_states, (unsigned short *) ch, 0);
#  else
		key;
#  endif
	if (!c)
	{
		// check prior key bindings...
		//printf( "no translation\n" );
		return 0;
	}
	else if (c == 2)
	{
		//printf( "Key Translated: %d %d\n", ch[0], ch[1] );
		return 0;
	}
	else if (c < 0)
	{
		//printf( "Key Translation less than 0\n" );
		return 0;
	}
	//printf( "Key Translated: %d(%c)\n", ch[0], ch[0] );
	return ch[0];
#endif
}

static uint32_t CPROC AndroidANW_IsKeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static uint32_t CPROC AndroidANW_KeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static LOGICAL CPROC AndroidANW_DisplayIsValid ( PRENDERER r )
{
	return (r != NULL);
}

static void CPROC AndroidANW_OwnMouseEx ( PRENDERER r, uint32_t Own DBG_PASS)
{

}

static int CPROC AndroidANW_BeginCalibration ( uint32_t points )
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

static int CPROC  AndroidANW_BindEventToKey( PRENDERER pRenderer, uint32_t scancode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv )
{
	return 0;
}

static int CPROC AndroidANW_UnbindKey( PRENDERER pRenderer, uint32_t scancode, uint32_t modifier )
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
	return 0;
}

static void CPROC AndroidANW_GetMouseState( int32_t *x, int32_t *y, uint32_t *b )
{
}

static PSPRITE_METHOD CPROC AndroidANW_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h ), uintptr_t psv )
{
	return NULL;
}

static void CPROC AndroidANW_WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, uintptr_t psvUser )
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

static void CPROC AndroidANW_Redraw( PRENDERER r )
{
	PVPRENDER render = (PVPRENDER)r;
	if( render->redraw )
		render->redraw( render->psv_redraw, (PRENDERER)render );
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

static void CPROC AndroidANW_IssueUpdateLayeredEx( PRENDERER r, LOGICAL bContent, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS )
{
}


#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@uintptr_t>
		 
		 \ \																									  */
static void CPROC AndroidANW_SetTouchHandler  ( PRENDERER r, TouchCallback c, uintptr_t p )
{
}
#endif

static void CPROC AndroidANW_MarkDisplayUpdated( PRENDERER r  )
{
}

static void CPROC AndroidANW_SetHideHandler		( PRENDERER r, HideAndRestoreCallback c, uintptr_t p )
{
}

static void CPROC AndroidANW_SetRestoreHandler  ( PRENDERER r, HideAndRestoreCallback c, uintptr_t p )
{
}

static void CPROC AndroidANW_RestoreDisplayEx ( PRENDERER r DBG_PASS )
{
	((PVPRENDER)r)->redraw( ((PVPRENDER)r)->psv_redraw, r );
}

// android extension
PUBLIC( void, SACK_Vidlib_ShowInputDevice )( void )
{
}

PUBLIC( void, SACK_Vidlib_HideInputDevice )( void )
{
}


static RENDER_INTERFACE ProxyInterface = {
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
	 /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@uintptr_t>
		 
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

static void InitProxyInterface( void )
{
	ProxyInterface._RequiresDrawAll = AndroidANW_RequiresDrawAll;
}

static RENDER3D_INTERFACE Proxy3dInterface = {
	NULL
};

static void CPROC AndroidANW_SetStringBehavior( Image pImage, uint32_t behavior )
{

}
static void CPROC AndroidANW_SetBlotMethod	  ( uint32_t method )
{

}

static Image CPROC AndroidANW_BuildImageFileEx ( PCOLOR pc, uint32_t width, uint32_t height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	//lprintf( "CRITICAL; BuildImageFile is not possible" );
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_BuildImageFileEx( pc, width, height DBG_RELAY );
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	AddLink( &l.images, image );
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make built proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	return (Image)image;
}

static Image CPROC AndroidANW_MakeSubImageEx  ( Image pImage, int32_t x, int32_t y, uint32_t width, uint32_t height DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->x = x;
	image->y = y;
	image->w = width;
	image->h = height;
	image->render_id = ((PVPImage)pImage)->render_id;
	if( ((PVPImage)pImage)->image )
	{
		image->image = l.real_interface->_MakeSubImageEx( ((PVPImage)pImage)->image, x, y, width, height DBG_RELAY );
		image->image->reverse_interface = &ProxyImageInterface;
		image->image->reverse_interface_instance = image;
	}
	image->parent = (PVPImage)pImage;
	if( image->next = ((PVPImage)pImage)->child )
		image->next->prior = image;
	((PVPImage)pImage)->child = image;

	AddLink( &l.images, image );
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make sub proxy image %p %d(%d,%d)  on %p %d", image, image->id, image->w, image->w, ((PVPImage)pImage), ((PVPImage)pImage)->id  );
	// don't really need to make this; if it needs to be updated to the client it will be handled later

	return (Image)image;
}

static Image CPROC AndroidANW_RemakeImageEx	 ( Image pImage, PCOLOR pc, uint32_t width, uint32_t height DBG_PASS)
{
	PVPImage image;
	if( !(image = (PVPImage)pImage ) )
	{
		image = New( struct vidlib_proxy_image );
		MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
		image->render_id = INVALID_INDEX;
		AddLink( &l.images, image );
	}
	//lprintf( "CRITICAL; RemakeImageFile is not possible" );
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_RemakeImageEx( image->image, pc, width, height DBG_RELAY );
	image->image->reverse_interface = &ProxyImageInterface;
	image->image->reverse_interface_instance = image;
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	return (Image)image;
}

static Image CPROC AndroidANW_LoadImageFileFromGroupEx( INDEX group, CTEXTSTR filename DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->filegroup = group;
	image->filename = StrDup( filename );
	image->image = l.real_interface->_LoadImageFileFromGroupEx( group, filename DBG_RELAY );
	if( image->image )
	{
		image->w = image->image->actual_width;
		image->h = image->image->actual_height;
	}
	image->render_id = INVALID_INDEX;
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	AddLink( &l.images, image );
	image->id = FindLink( &l.images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "loaded proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	return (Image)image;
}

static Image CPROC AndroidANW_LoadImageFileEx( CTEXTSTR filename DBG_PASS )
{
	return AndroidANW_LoadImageFileFromGroupEx( GetFileGroup( "Images", "./images" ), filename DBG_RELAY );
}



static void CPROC AndroidANW_ResizeImageEx	  ( Image pImage, int32_t width, int32_t height DBG_PASS)
{
	PVPImage image = (PVPImage)pImage;
	image->w = width;
	image->h = height;
	l.real_interface->_ResizeImageEx( image->image, width, height DBG_RELAY );
}

static void CPROC AndroidANW_MoveImage			( Image pImage, int32_t x, int32_t y )
{
	PVPImage image = (PVPImage)pImage;
	if( image->x != x || image->y != y )
	{
		image->x = x;
		image->y = y;
		l.real_interface->_MoveImage( image->image, x, y );
	}
}

uint8_t* GetMessageBuf( PVPImage image, size_t size )
{
	uint8_t* resultbuf;
	if( ( image->buf_avail ) <= ( size + image->sendlen ) )
	{
		uint8_t* newbuf;
		image->buf_avail += size + 256;
		newbuf = NewArray( uint8_t, image->buf_avail );
		if( image->buffer )
		{
			MemCpy( newbuf, image->buffer, image->sendlen );
			Release( image->buffer );
		}
		image->buffer = newbuf;
	}
	resultbuf = image->buffer + image->sendlen;
	((uint32_t*)resultbuf)[0] = size - 4;
	image->sendlen += size;

	return resultbuf + 4;
}


static LOGICAL FixArea( IMAGE_RECTANGLE *result, IMAGE_RECTANGLE *source, PVPImage pifDest )
{
	IMAGE_RECTANGLE r2;
	// build rectangle of what we want to show
	// build rectangle which is presently visible on image
	r2.x		= 0;
	r2.y		= 0;
	r2.height = pifDest->h;
	r2.width  = pifDest->w;
	return l.real_interface->_IntersectRectangle( result, source, &r2 );
}

static void CPROC AndroidANW_BlatColor	  ( Image pifDest, int32_t x, int32_t y, uint32_t w, uint32_t h, CDATA color )
{
	if( ((PVPImage)pifDest)->render_id != INVALID_INDEX )
	{
		PVPImage image = (PVPImage)pifDest;
		struct common_message *outmsg;
		size_t sendlen;
		IMAGE_RECTANGLE r, r1;
 		// build rectangle of what we want to show
		r1.x		= x;
		r1.width  = w;
		r1.y		= y;
		r1.height = h;
		if( !FixArea( &r, &r1, image ) )
		{
			return;
		}

		EnterCriticalSec( &l.message_formatter );

		if( w == image->w && h == image->h )
		{
			image->websock_sendlen = 0;
			image->sendlen = 0;
		}

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen=( 4 + 1 + sizeof( struct blatcolor_data ) ) );
		outmsg->message_id = PMID_BlatColor;
		outmsg->data.blatcolor.x = r.x;
		outmsg->data.blatcolor.y = r.y;
		outmsg->data.blatcolor.w = r.width;
		outmsg->data.blatcolor.h = r.height;
		outmsg->data.blatcolor.color = color;
		outmsg->data.blatcolor.server_image_id = image->id;
		if( w == image->w && h == image->h )
		{
			//lprintf( "clear buffer; full color : %08x", color );
		}
		else
		{
			//lprintf( "w:%d w:%d h:%d h:%d full color : %08x", w, image->w, h, image->h, color );
		}
		LeaveCriticalSec( &l.message_formatter );

	}
	else
	{
		l.real_interface->_BlatColor( ((PVPImage)pifDest)->image, x, y, w, h, color );
	}

}

static void CPROC AndroidANW_BlatColorAlpha( Image pifDest, int32_t x, int32_t y, uint32_t w, uint32_t h, CDATA color )
{
	if( ((PVPImage)pifDest)->render_id != INVALID_INDEX )
	{
		IMAGE_RECTANGLE r, r1;
		PVPImage image = (PVPImage)pifDest;
		struct common_message *outmsg;
		size_t sendlen;

		// build rectangle of what we want to show
		r1.x		= x;
		r1.width  = w;
		r1.y		= y;
		r1.height = h;
		// build rectangle which is presently visible on image
		if( !FixArea( &r, &r1, image ) )
		{
			return;
		}
		EnterCriticalSec( &l.message_formatter );
		if( w == image->w && h == image->h )
		{
			image->websock_sendlen = 0;
			image->sendlen = 0;
		}

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blatcolor_data ) ) );
		outmsg->message_id = PMID_BlatColorAlpha;
		outmsg->data.blatcolor.x = r.x;
		outmsg->data.blatcolor.y = r.y;
		outmsg->data.blatcolor.w = r.width;
		outmsg->data.blatcolor.h = r.height;
		outmsg->data.blatcolor.color = color;
		outmsg->data.blatcolor.server_image_id = image->id;
		if( w == image->w && h == image->h )
		{
			//lprintf( "clear buffer; full color : %08x", color );
		}
		else
		{
			//lprintf( "w:%d w:%d h:%d h:%d full color : %08x", w, image->w, h, image->h, color );
		}
		{
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		l.real_interface->_BlatColorAlpha( ((PVPImage)pifDest)->image, x, y, w, h, color );
	}
}

static void SetImageUsed( PVPImage image )
{
	while( image  )
	{
		image->flags.bUsed = 1;
		image = image->parent;
	}
}

static void CPROC AndroidANW_BlotImageSizedEx( Image pDest, Image pIF, int32_t x, int32_t y, int32_t xs, int32_t ys, uint32_t wd, uint32_t ht, uint32_t nTransparent, uint32_t method, ... )
{
	PVPImage image = (PVPImage)pDest;
	if( !((PVPImage)pIF)->image )
		return;
	if( ((PVPImage)pDest)->render_id != INVALID_INDEX )
	{
		IMAGE_RECTANGLE r;
		struct common_message *outmsg;
		size_t sendlen;
		EnterCriticalSec( &l.message_formatter );

		{
			IMAGE_RECTANGLE r1;
			// build rectangle of what we want to show
			r1.x		= x;
			r1.width  = wd;
			r1.y		= y;
			r1.height = ht;
			// build rectangle which is presently visible on image
			if( !FixArea( &r, &r1, image ) )
			{
				LeaveCriticalSec( &l.message_formatter );
				return;
			}
		}
		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blot_image_data ) ) );
		outmsg->message_id = PMID_BlotImageSizedTo;
		if( x != r.x )
		{
			xs += (r.x-x);
		}
		if( y != r.y )
		{
			ys += (r.y-y);
		}
		outmsg->data.blot_image.x = r.x;
		outmsg->data.blot_image.y = r.y;
		outmsg->data.blot_image.w = r.width;
		outmsg->data.blot_image.h = r.height;
		switch( method & 3 )
		{
		case BLOT_COPY:
			// sending this clears the flag.
			SetImageUsed( (PVPImage)pIF );
			outmsg->data.blot_image.image_id = ((PVPImage)pIF)->id;
			break;
		case BLOT_SHADED:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );
				while( ((PVPImage)pIF)->parent )
				{
					xs += ((PVPImage)pIF)->x;
					ys += ((PVPImage)pIF)->y;
					pIF = (Image)(((PVPImage)pIF)->parent);
				}
				shaded_image = l.real_interface->_GetTintedImage( ((PVPImage)pIF)->image, va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_image.image_id = actual_image->id;
			}
			break;
		case BLOT_MULTISHADE:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );

				while( ((PVPImage)pIF)->parent )
				{
					xs += ((PVPImage)pIF)->x;
					ys += ((PVPImage)pIF)->y;
					pIF = (Image)(((PVPImage)pIF)->parent);
				}
				shaded_image = l.real_interface->_GetShadedImage( ((PVPImage)pIF)->image, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_image.image_id = actual_image->id;
			}
			break;
		}
		outmsg->data.blot_image.xs = xs;
		outmsg->data.blot_image.ys = ys;
		outmsg->data.blot_image.server_image_id = image->id;
		{
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		va_list args;
		va_start( args, method );
		l.real_interface->_BlotImageEx( image->image, ((PVPImage)pIF)->image, x, y, xs, ys, wd, ht
					, nTransparent
					, method
					, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
	}
}

static void CPROC AndroidANW_BlotImageEx	  ( Image pDest, Image pIF, int32_t x, int32_t y, uint32_t nTransparent, uint32_t method, ... )
{
	va_list args;
	va_start( args, method );
	AndroidANW_BlotImageSizedEx( pDest, pIF, x, y, 0, 0
					, pIF->width, pIF->height
					, nTransparent
					, method, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) ); 
}



static void CPROC AndroidANW_BlotScaledImageSizedEx( Image pifDest, Image pifSrc
											  , int32_t xd, int32_t yd
											  , uint32_t wd, uint32_t hd
											  , int32_t xs, int32_t ys
											  , uint32_t ws, uint32_t hs
											  , uint32_t nTransparent
											  , uint32_t method, ... )
{
	PVPImage image = (PVPImage)pifDest;
	if( image->render_id != INVALID_INDEX )
	{
		struct common_message *outmsg;
		size_t sendlen;
		uint32_t dhd, dwd, dhs, dws;
		int errx, erry;
		if( !((PVPImage)pifSrc)->image )
			return;
		if( ( xd > ( pifDest->x + pifDest->width ) )
			|| ( yd > ( pifDest->y + pifDest->height ) )
			|| ( ( xd + (signed)wd ) < 0/*pifDest->x*/ )
			|| ( ( yd + (signed)hd ) < 0/*pifDest->y*/ ) )
		{
			return;
		}
		dhd = hd;
		dhs = hs;
		dwd = wd;
		dws = ws;

		// ok - how to figure out how to do this
		// need to update the position and width to be within the 
		// the bounds of pifDest....
		//	lprintf(" begin scaled output..." );
		errx = -(signed)dwd;
		erry = -(signed)dhd;

		if( ( xd < 0 ) || ( (xd+(int32_t)(wd&0x7FFFFFFF)) > image->w ) )
		{
			int x = xd;
			int w = 0;
			while( x < pifDest->x )
			{
				errx += (signed)dws;
				while( errx >= 0 )
				{
					errx -= (signed)dwd;
					ws--;
					xs++;
				}
				wd--;
				x++;
			}
			xd = x;

			while( x < pifDest->width && SUS_GT( w, int, wd, uint32_t ) )
			{
				errx += (signed)dws;
				while( errx >= 0 )
				{
					errx -= (signed)dwd;
					w++;
				}
				x++;
			}
			wd = w;
		}
		//Log8( "Blot scaled params: %d %d %d %d / %d %d %d %d ", 
		//		 xs, ys, ws, hs, xd, yd, wd, hd );
		if( ( yd < 0 ) || ( yd +(int32_t)(hd&0x7FFFFFFF) ) > pifDest->height )
		{
			int y = yd;
			int h = 0;
			while( yd < pifDest->y )
			{
				erry += (signed)dhs;
				while( erry >= 0 )
				{
					erry -= (signed)dhd;
					hs--;
					yd++;
				}
				hd--;
				y++;
			}
			while( y < pifDest->height && h < (int)hd )
			{
				erry += (signed)dhs;
				while( erry >= 0 )
				{
					erry -= (signed)dhd;
					h++;
				}
				y++;
			}
			hd = h;
		}

		EnterCriticalSec( &l.message_formatter );

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blot_scaled_image_data ) ) );
		outmsg->message_id = PMID_BlotScaledImageSizedTo;

		switch( method & 3 )
		{
		case BLOT_COPY:
			// sending this clears the flag.
			SetImageUsed( (PVPImage)pifSrc );
			outmsg->data.blot_scaled_image.image_id = ((PVPImage)pifSrc)->id;
			break;
		case BLOT_SHADED:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );
				while( ((PVPImage)pifSrc)->parent )
				{
					xs += ((PVPImage)pifSrc)->x;
					ys += ((PVPImage)pifSrc)->y;
					pifSrc = (Image)(((PVPImage)pifSrc)->parent);
				}
				shaded_image = l.real_interface->_GetTintedImage( ((PVPImage)pifSrc)->image, va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					lprintf( "wrap shaded image for %p %d", ((PVPImage)pifSrc), ((PVPImage)pifSrc)->id );
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_scaled_image.image_id = actual_image->id;
			}
			break;
		case BLOT_MULTISHADE:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );

				while( ((PVPImage)pifSrc)->parent )
				{
					xs += ((PVPImage)pifSrc)->x;
					ys += ((PVPImage)pifSrc)->y;
					pifSrc = (Image)(((PVPImage)pifSrc)->parent);
				}
				shaded_image = l.real_interface->_GetShadedImage( ((PVPImage)pifSrc)->image, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_scaled_image.image_id = actual_image->id;
			}
			break;
		}
		outmsg->data.blot_scaled_image.x = xd;
		outmsg->data.blot_scaled_image.y = yd;
		outmsg->data.blot_scaled_image.w = wd;
		outmsg->data.blot_scaled_image.h = hd;
		outmsg->data.blot_scaled_image.xs = xs;
		outmsg->data.blot_scaled_image.ys = ys;
		outmsg->data.blot_scaled_image.ws = ws;
		outmsg->data.blot_scaled_image.hs = hs;
		//outmsg->data.blot_scaled_image.image_id = ((PVPImage)pifSrc)->id;
		outmsg->data.blot_scaled_image.server_image_id = image->id;
		{
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		va_list args;
		va_start( args, method );
		l.real_interface->_BlotScaledImageSizedEx( image->image, ((PVPImage)pifSrc)->image, xd, yd, wd, hd, xs, ys, ws, hs
					, nTransparent
					, method
					, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
	}
}

static void CPROC AndroidANW_MarkImageDirty ( Image pImage )
{
	// this library tracks the IF_FLAG_UPDATED which is set by native routine;native routine marks child and all parents.
	//lprintf( "Mark image %p dirty", pImage );
	l.real_interface->_MarkImageDirty( ((PVPImage)pImage)->image );
	if( 0 )
	{
		size_t outlen;
		uint8_t* encoded_image;
	
		if( ((PVPImage)pImage)->parent )
			encoded_image = EncodeImage( ((PVPImage)pImage)->parent->image, FALSE, &outlen );
		else
			encoded_image = EncodeImage( ((PVPImage)pImage)->image, FALSE, &outlen );
		Release( encoded_image );
	}
}

#define DIMAGE_DATA_PROC(type,name,args)  static type (CPROC VidlibProxy2_##name)args;static type (CPROC* AndroidANW_##name)args = VidlibProxy2_##name; type (CPROC VidlibProxy2_##name)args

DIMAGE_DATA_PROC( void,plot,		( Image pi, int32_t x, int32_t y, CDATA c ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		l.real_interface->_plot[0]( ((PVPImage)pi)->image, x, y, c );
		AndroidANW_MarkImageDirty( pi );
	}
}

DIMAGE_DATA_PROC( void,plotalpha, ( Image pi, int32_t x, int32_t y, CDATA c ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		l.real_interface->_plot[0]( ((PVPImage)pi)->image, x, y, c );
	}
}

DIMAGE_DATA_PROC( CDATA,getpixel, ( Image pi, int32_t x, int32_t y ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		PVPImage my_image = (PVPImage)pi;
		if( my_image )
		{
			return (*l.real_interface->_getpixel)( my_image->image, x, y );
		}
	}
	return 0;
}

DIMAGE_DATA_PROC( void,do_line,	  ( Image pifDest, int32_t x, int32_t y, int32_t xto, int32_t yto, CDATA color ))
{
	PVPImage image = (PVPImage)pifDest;
	if( image->render_id != INVALID_INDEX )
	{
		struct common_message *outmsg;
		size_t sendlen;
		EnterCriticalSec( &l.message_formatter );

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct line_data ) ) );
		outmsg->message_id = PMID_DrawLine;
		outmsg->data.line.server_image_id = image->id;
		outmsg->data.line.x1 = x;
		outmsg->data.line.y1 = y;
		outmsg->data.line.x2 = xto;
		outmsg->data.line.y2 = yto;
		outmsg->data.line.color = color;
		{
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		l.real_interface->_do_line[0]( image->image, x, y, xto, yto, color );
	}
}

DIMAGE_DATA_PROC( void,do_lineAlpha,( Image pBuffer, int32_t x, int32_t y, int32_t xto, int32_t yto, CDATA color))
{
	VidlibProxy2_do_line( pBuffer, x, y, xto, yto, color );
}


DIMAGE_DATA_PROC( void,do_hline,	  ( Image pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, xfrom, y, xto, y, color );
}

DIMAGE_DATA_PROC( void,do_vline,	  ( Image pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, x, yfrom, x, yto, color );
}

DIMAGE_DATA_PROC( void,do_hlineAlpha,( Image pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, xfrom, y, xto, y, color );
}

DIMAGE_DATA_PROC( void,do_vlineAlpha,( Image pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, x, yfrom, x, yto, color );
}

static SFTFont CPROC AndroidANW_GetDefaultFont ( void )
{
	return l.real_interface->_GetDefaultFont( );
}

static uint32_t CPROC AndroidANW_GetFontHeight  ( SFTFont font )
{
	return l.real_interface->_GetFontHeight( font );
}

static uint32_t CPROC AndroidANW_GetStringSizeFontEx( CTEXTSTR pString, size_t len, uint32_t *width, uint32_t *height, SFTFont UseFont )
{
	return l.real_interface->_GetStringSizeFontEx( pString, len, width, height, UseFont );
}

static void CPROC AndroidANW_PutCharacterFont		  ( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
}

static void CPROC AndroidANW_PutCharacterVerticalFont( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterVerticalFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
}

static void CPROC AndroidANW_PutCharacterInvertFont  ( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterInvertFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
}

static void CPROC AndroidANW_PutCharacterVerticalInvertFont( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterVerticalInvertFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
}

static void CPROC AndroidANW_PutStringFontEx  ( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background
												, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
}

static void CPROC AndroidANW_PutStringVerticalFontEx		( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringVerticalFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
}

static void CPROC AndroidANW_PutStringInvertFontEx		  ( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringInvertFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
}

static void CPROC AndroidANW_PutStringInvertVerticalFontEx( Image pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringInvertVerticalFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
}

static uint32_t CPROC AndroidANW_GetMaxStringLengthFont( uint32_t width, SFTFont UseFont )
{
	return l.real_interface->_GetMaxStringLengthFont( width, UseFont );
}

static void CPROC AndroidANW_GetImageSize ( Image pImage, uint32_t *width, uint32_t *height )
{
	if( width )
		(*width) = ((PVPImage)pImage)->w;
	if( height )
		(*height) = ((PVPImage)pImage)->h;
}

static SFTFont CPROC AndroidANW_LoadFont ( SFTFont font )
{
}
			/* <combine sack::image::UnloadFont@SFTFont>
				
				\ \												*/
			IMAGE_PROC_PTR( void, UnloadFont )					  ( SFTFont font );

/* Internal
	Interface index 44
	
	This is used by internal methods to transfer image and font
	data to the render agent.											  */	IMAGE_PROC_PTR( DataState, BeginTransferData )	 ( uint32_t total_size, uint32_t segsize, CDATA data );
/* Internal
	Interface index 45
	
	Used internally to transfer data to render agent. */	IMAGE_PROC_PTR( void, ContinueTransferData )		( DataState state, uint32_t segsize, CDATA data );
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

static PCDATA CPROC AndroidANW_GetImageSurface 		 ( Image pImage )
{
	if( pImage )
	{
		if( ((PVPImage)pImage)->render_id == INVALID_INDEX )
			return l.real_interface->_GetImageSurface( ((PVPImage)pImage)->image );
	}
	return NULL;
}

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

static void CPROC AndroidANW_OrphanSubImage ( Image pImage )
{
	PVPImage image = (PVPImage)pImage;
	if( image )
	{
		//if( !image->parent
		//	|| ( pImage->flags & IF_FLAG_OWN_DATA ) )
		//	return;
		if( image->prior )
			image->prior->next = image->next;
		else
			image->parent->child = image->next;

		if( image->next )
			image->next->prior = image->prior;

		image->parent = NULL;
		image->next = NULL; 
		image->prior = NULL; 
		
		if( image->image )
			l.real_interface->_OrphanSubImage( image->image );
	}
}

static void SmearRenderFlag( PVPImage image )
{
	for( ; image; image = image->next )
	{
		if( image->image && ( ( image->render_id = image->parent->render_id ) != INVALID_INDEX ) )
			image->image->flags |= IF_FLAG_FINAL_RENDER;
		image->image->reverse_interface = &ProxyImageInterface;
		image->image->reverse_interface_instance = image;
		SmearRenderFlag( image->child );
	}
}


static void CPROC AndroidANW_AdoptSubImage ( Image pFoster, Image pOrphan )
{
	PVPImage foster = (PVPImage)pFoster;
	PVPImage orphan = (PVPImage)pOrphan;
	if( foster->id == 1 )
	{
		int a =3 ;
	}
	if( foster && orphan )
	{
		if( ( orphan->next = foster->child ) )
			orphan->next->prior = orphan;
		orphan->parent = foster;
		foster->child = orphan;
		orphan->prior = NULL; // otherwise would be undefined
		SmearRenderFlag( orphan );

		if( foster->image && orphan->image )
			l.real_interface->_AdoptSubImage( foster->image, orphan->image );
	}
}

static void CPROC AndroidANW_TransferSubImages( Image pImageTo, Image pImageFrom )
{
	PVPImage tmp;
	while( tmp = ((PVPImage)pImageFrom)->child )
	{
		// moving a child allows it to keep all of it's children too?
		// I think this is broken in that case; Orphan removes from the family entirely?
		AndroidANW_OrphanSubImage( (Image)tmp );
		AndroidANW_AdoptSubImage( (Image)pImageTo, (Image)tmp );
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
	 /* <combine sack::image::DecodeMemoryToImage@uint8_t*@uint32_t>
		 
		 \ \																*/
static Image CPROC AndroidANW_DecodeMemoryToImage ( uint8_t* buf, uint32_t size )
{
	Image real_image = l.real_interface->_DecodeMemoryToImage( buf, size );
	return (Image)WrapImageFile( real_image );
}

/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@uint32_t *>
	
	\ \																			  */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, int32_t x, int32_t y );
/* <combine sack::image::SetSpritePosition@PSPRITE@int32_t@int32_t>
	
	\ \																		  */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, int32_t x, int32_t y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
		
		\ \																*/
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
	
	\ \											  */

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@uint32_t@uint32_t *@uint32_t *@uint32_t *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@uint32_t *@uint32_t *@uint32_t *@SFTFont>
	
	\ \																																																							*/
IMAGE_PROC_PTR( uint32_t, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, uint32_t *width, uint32_t *height, uint32_t *charheight, SFTFont UseFont );

IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags );
IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );


static Image CPROC AndroidANW_GetNativeImage( Image pImage )
{
	return ((PVPImage)pImage)->image;
}

IMAGE_PROC_PTR( void, DumpFontCache )( void );
IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, int32_t width, int32_t height, PFRACTION width_scale, PFRACTION height_scale );
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
	AndroidANW_SetStringBehavior,
		AndroidANW_SetBlotMethod,
		AndroidANW_BuildImageFileEx,
		AndroidANW_MakeImageFileEx,
		AndroidANW_MakeSubImageEx,
		AndroidANW_RemakeImageEx,
		AndroidANW_LoadImageFileEx,
		AndroidANW_UnmakeImageFileEx,
		AndroidANW_ResizeImageEx,
		AndroidANW_MoveImage,
		AndroidANW_BlatColor
		, AndroidANW_BlatColorAlpha
		, AndroidANW_BlotImageEx
		, AndroidANW_BlotImageSizedEx
		, AndroidANW_BlotScaledImageSizedEx
		, &AndroidANW_plot
		, &AndroidANW_plotalpha
		, &AndroidANW_getpixel
		, &AndroidANW_do_line
		, &AndroidANW_do_lineAlpha
		, &AndroidANW_do_hline
		, &AndroidANW_do_vline
		, &AndroidANW_do_hlineAlpha
		, &AndroidANW_do_vlineAlpha
		, AndroidANW_GetDefaultFont
		, AndroidANW_GetFontHeight
		, AndroidANW_GetStringSizeFontEx
		, AndroidANW_PutCharacterFont
		, AndroidANW_PutCharacterVerticalFont
		, AndroidANW_PutCharacterInvertFont
		, AndroidANW_PutCharacterVerticalInvertFont
		, AndroidANW_PutStringFontEx
		, AndroidANW_PutStringVerticalFontEx
		, AndroidANW_PutStringInvertFontEx
		, AndroidANW_PutStringInvertVerticalFontEx
		, AndroidANW_GetMaxStringLengthFont
		, AndroidANW_GetImageSize

		, NULL//AndroidANW_LoadFont
		, NULL//AndroidANW_UnloadFont
		, NULL//AndroidANW_BeginTransferData
		, NULL//AndroidANW_ContinueTransferData
		, NULL//AndroidANW_DecodeTransferredImage
		, NULL//AndroidANW_AcceptTransferredFont
		, &AndroidANW_ColorAverage
		, NULL//AndroidANW_SyncImage
		, AndroidANW_GetImageSurface
		, NULL//AndroidANW_IntersectRectangle
		, NULL//AndroidANW_MergeRectangle
		, NULL// ** AndroidANW_GetImageAuxRect
		, NULL// ** AndroidANW_SetImageAuxRect
		, AndroidANW_OrphanSubImage
		, AndroidANW_AdoptSubImage
		, NULL // *****	AndroidANW_MakeSpriteImageFileEx
		, NULL // *****	AndroidANW_MakeSpriteImageEx
		, NULL // *****	AndroidANW_rotate_scaled_sprite
		, NULL // *****	AndroidANW_rotate_sprite
		, NULL // *****	AndroidANW_BlotSprite
		, AndroidANW_DecodeMemoryToImage
		, NULL//AndroidANW_InternalRenderFontFile
		, NULL//AndroidANW_InternalRenderFont
		, NULL//AndroidANW_RenderScaledFontData
		, NULL//AndroidANW_RenderFontFileScaledEx
		, NULL//AndroidANW_DestroyFont
		, NULL
		, NULL//AndroidANW_GetFontRenderData
		, NULL//AndroidANW_SetFontRendererData
		, NULL //AndroidANW_SetSpriteHotspot
		, NULL //AndroidANW_SetSpritePosition
		, NULL //AndroidANW_UnmakeSprite
		, NULL //AndroidANW_struct font_global_tag *, GetGlobalFonts)( void );

, NULL //IMAGE_PROC_PTR( uint32_t, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, uint32_t *width, uint32_t *height, uint32_t *charheight, SFTFont UseFont );

, NULL //IMAGE_PROC_PTR( Image, LoadImageFileFromGroupEx )( INDEX group, CTEXTSTR filename DBG_PASS );

, NULL //IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags );
, NULL //IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags, size_t *pnFontDataSize, POINTER *pFontData );

, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetGreenValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetBlueValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
, NULL //IMAGE_PROC_PTR( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
, NULL //IMAGE_PROC_PTR( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
, NULL //IMAGE_PROC_PTR( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
, NULL //IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
, NULL //IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );

, NULL //IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
, NULL //IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
, NULL //IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
, NULL //IMAGE_PROC_PTR( void, MarkImageDirty )( Image pImage );

, NULL //IMAGE_PROC_PTR( void, DumpFontCache )( void );
, NULL //IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, int32_t width, int32_t height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

, NULL //IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
, NULL //IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
, NULL // IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
, NULL //IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );
};

static CDATA CPROC AndroidANW_SetRedValue( CDATA color, COLOR_CHANNEL r )
{
	return ( ((color)&0xFFFFFF00) | ( ((r)&0xFF)<<0 ) );
}
static CDATA CPROC AndroidANW_SetGreenValue( CDATA color, COLOR_CHANNEL green )
{
	return ( ((color)&0xFFFF00FF) | ( ((green)&0xFF)<<8 ) );
}
static CDATA CPROC AndroidANW_SetBlueValue( CDATA color, COLOR_CHANNEL b )
{
	return ( ((color)&0xFF00FFFF) | ( ((b)&0xFF)<<16 ) );
}
static CDATA CPROC AndroidANW_SetAlphaValue( CDATA color, COLOR_CHANNEL a )
{
	return ( ((color)&0xFFFFFF) | ( (a)<<24 ) );
}

static COLOR_CHANNEL CPROC AndroidANW_GetRedValue( CDATA color )
{
	return (color & 0xFF ) >> 0;
}

static COLOR_CHANNEL CPROC AndroidANW_GetGreenValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF00 ) >> 8);
}

static COLOR_CHANNEL CPROC AndroidANW_GetBlueValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0x00FF0000 ) >> 16);
}

static COLOR_CHANNEL CPROC AndroidANW_GetAlphaValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF000000 ) >> 24);
}

static CDATA CPROC AndroidANW_MakeAlphaColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b, COLOR_CHANNEL alpha )
{
#  ifdef _WIN64
#	 define _AND_FF &0xFF
#  else
/* This is a macro to cure a 64bit warning in visual studio. */
#	 define _AND_FF
#  endif
#define _AColor( r,g,b,a ) (((uint32_t)( ((uint8_t)((b)_AND_FF))|((uint16_t)((uint8_t)((g))_AND_FF)<<8))|(((uint32_t)((uint8_t)((r))_AND_FF)<<16)))|(((a)_AND_FF)<<24))
	return _AColor( r, grn, b, alpha );
}

static CDATA CPROC AndroidANW_MakeColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b )
{
	return AndroidANW_MakeAlphaColor( r,grn,b, 0xFF );
}

static LOGICAL CPROC AndroidANW_IsImageTargetFinal( Image image )
{
	if( image )
		return ( ((PVPImage)image)->render_id != INVALID_INDEX );
	return 0;
}


static void InitImageInterface( void )
{
	ProxyImageInterface._GetRedValue = AndroidANW_GetRedValue;
	ProxyImageInterface._GetGreenValue = AndroidANW_GetGreenValue;
	ProxyImageInterface._GetBlueValue = AndroidANW_GetBlueValue;
	ProxyImageInterface._GetAlphaValue = AndroidANW_GetAlphaValue;
	ProxyImageInterface._SetRedValue = AndroidANW_SetRedValue;
	ProxyImageInterface._SetGreenValue = AndroidANW_SetGreenValue;
	ProxyImageInterface._SetBlueValue = AndroidANW_SetBlueValue;
	ProxyImageInterface._SetAlphaValue = AndroidANW_SetAlphaValue;
	ProxyImageInterface._MakeColor = AndroidANW_MakeColor;
	ProxyImageInterface._MakeAlphaColor = AndroidANW_MakeAlphaColor;
	ProxyImageInterface._LoadImageFileFromGroupEx = AndroidANW_LoadImageFileFromGroupEx;
	ProxyImageInterface._GetStringSizeFontEx = AndroidANW_GetStringSizeFontEx;
	ProxyImageInterface._GetFontHeight = AndroidANW_GetFontHeight;
	ProxyImageInterface._OrphanSubImage = AndroidANW_OrphanSubImage;
	ProxyImageInterface._AdoptSubImage = AndroidANW_AdoptSubImage;
	ProxyImageInterface._TransferSubImages = AndroidANW_TransferSubImages;
	ProxyImageInterface._MarkImageDirty = AndroidANW_MarkImageDirty;
	ProxyImageInterface._GetNativeImage = AndroidANW_GetNativeImage;
	ProxyImageInterface._IsImageTargetFinal = AndroidANW_IsImageTargetFinal;

	// ============= FONT Support ============================
	// these should go through real_interface
	ProxyImageInterface._RenderFontFileScaledEx = l.real_interface->_RenderFontFileScaledEx;
	ProxyImageInterface._RenderScaledFont = l.real_interface->_RenderScaledFont;
	ProxyImageInterface._RenderScaledFontData = l.real_interface->_RenderScaledFontData;
	ProxyImageInterface._RerenderFont = l.real_interface->_RerenderFont;
	ProxyImageInterface._RenderScaledFontEx = l.real_interface->_RenderScaledFontEx;
	ProxyImageInterface._DumpFontCache = l.real_interface->_DumpFontCache;
	ProxyImageInterface._DumpFontFile = l.real_interface->_DumpFontFile;

	ProxyImageInterface._GetFontHeight = l.real_interface->_GetFontHeight;

	ProxyImageInterface._GetFontRenderData = l.real_interface->_GetFontRenderData;
	ProxyImageInterface._GetStringRenderSizeFontEx = l.real_interface->_GetStringRenderSizeFontEx;
	ProxyImageInterface._GetStringSizeFontEx = l.real_interface->_GetStringSizeFontEx;

	// this is part of the old interface; and isn't used anymore
	//ProxyImageInterface._UnloadFont = l.real_interface->_UnloadFont;
	ProxyImageInterface._DestroyFont = l.real_interface->_DestroyFont;
	ProxyImageInterface._global_font_data = l.real_interface->_global_font_data;
	ProxyImageInterface._GetGlobalFonts = l.real_interface->_GetGlobalFonts;
	ProxyImageInterface._InternalRenderFontFile = l.real_interface->_InternalRenderFontFile;
	ProxyImageInterface._InternalRenderFont = l.real_interface->_InternalRenderFont;
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
	InitializeCriticalSec( &l.message_formatter );
	RegisterInterface( "sack.image.proxy.server", GetProxyImageInterface, DropProxyImageInterface );
	RegisterInterface( "sack.image.3d.proxy.server", Get3dProxyImageInterface, Drop3dProxyImageInterface );
	RegisterInterface( "sack.render.proxy.server", GetProxyDisplayInterface, DropProxyDisplayInterface );
	RegisterInterface( "sack.render.3d.proxy.server", Get3dProxyDisplayInterface, Drop3dProxyDisplayInterface );
#ifdef _WIN32
	LoadFunction( "bag.image.dll", NULL );
#endif
	l.real_interface = (PIMAGE_INTERFACE)GetInterface( "sack.image" );

	InitProxyInterface();
	// needs sack.image loaded before; fonts are passed to this
	InitImageInterface();

}


