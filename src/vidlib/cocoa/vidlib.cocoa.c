// macOS (Cocoa) video backend for SACK's render interface.
//
// This is the framebuffer/window layer (the equivalent of vidlib.c on win32 or
// vidlib.wayland.c on linux).  The application draws into an Image (software
// framebuffer); this backend owns the NSWindow, blits that framebuffer to a
// layer-backed NSView via a CGImage, and translates Cocoa input/window events
// back into the SACK render callbacks.
//
// All AppKit access goes through the Objective-C runtime from plain C - there
// is intentionally no Objective-C (.m) source.  AppKit/Cocoa requires its event
// loop to run on the main thread; see cocoa_RunApplication()/the notes at the
// bottom for how the host is expected to drive it.

#define USE_IMAGE_INTERFACE cocoa_l.pii
#include <stdhdrs.h>
#include <render.h>
#include <image.h>
#include <idle.h>
#include <keybrd.h>

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <pthread.h>

#include "local.h"

RENDER_NAMESPACE

struct cocoa_local_tag cocoa_l;

//----------------------------------------------------------------------------
// Objective-C runtime helpers (typed objc_msgSend wrappers).
// On arm64 objc_msgSend MUST be called through a prototype that matches the
// real method signature, so each shape gets its own cast macro.
//----------------------------------------------------------------------------

#define CLS(n)   ((id)objc_getClass(n))
#define SELP(n)  sel_registerName(n)

#define msgVoid(r,s)             (((void(*)(id,SEL))objc_msgSend)((id)(r), SELP(s)))
#define msgId(r,s)               (((id(*)(id,SEL))objc_msgSend)((id)(r), SELP(s)))
#define msgLong(r,s)             (((long(*)(id,SEL))objc_msgSend)((id)(r), SELP(s)))
#define msgVoidId(r,s,a)         (((void(*)(id,SEL,id))objc_msgSend)((id)(r), SELP(s), (id)(a)))
#define msgVoidBool(r,s,a)       (((void(*)(id,SEL,BOOL))objc_msgSend)((id)(r), SELP(s), (BOOL)(a)))
#define msgVoidUL(r,s,a)         (((void(*)(id,SEL,unsigned long))objc_msgSend)((id)(r), SELP(s), (unsigned long)(a)))
#define msgIdId(r,s,a)           (((id(*)(id,SEL,id))objc_msgSend)((id)(r), SELP(s), (id)(a)))
#define msgIdCStr(r,s,a)         (((id(*)(id,SEL,const char*))objc_msgSend)((id)(r), SELP(s), (a)))
#define msgVoidLLLL(r,s,a,b,c,d) (((void(*)(id,SEL,double,double,double,double))objc_msgSend)((id)(r), SELP(s), (double)(a),(double)(b),(double)(c),(double)(d)))

// NSWindow style mask / backing-store constants (from AppKit headers).
#define NSWindowStyleMaskTitled        1
#define NSWindowStyleMaskClosable      2
#define NSWindowStyleMaskMiniaturizable 4
#define NSWindowStyleMaskResizable     8
#define NSWindowStyleMaskBorderless    0
#define NSBackingStoreBuffered         2

//----------------------------------------------------------------------------
// PXPANEL <-> Cocoa object association (so event IMPs can find their window)
//----------------------------------------------------------------------------
static const void *kPanelKey = &cocoa_l; // any stable unique address

static void SetPanel( id obj, PXPANEL r ) {
	objc_setAssociatedObject( obj, kPanelKey, (id)r, OBJC_ASSOCIATION_ASSIGN );
}
static PXPANEL GetPanel( id obj ) {
	return (PXPANEL)objc_getAssociatedObject( obj, kPanelKey );
}

//----------------------------------------------------------------------------
// CoreGraphics blit: push the framebuffer pixels into the view's layer.
// SACK COLOR is 0xAARRGGBB in-word => little-endian 32-bit ARGB, which matches
// kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little.
//----------------------------------------------------------------------------
static void PresentFramebuffer( PXPANEL r ) {
	if( !r || !r->caLayer || !r->framebuffer || !r->pImageDraw )
		return;
	{
		uint32_t w = r->w, h = r->h;
		size_t stride = (size_t)r->pImageDraw->pwidth * 4;
		CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
		CGDataProviderRef prov = CGDataProviderCreateWithData( NULL, r->framebuffer
		                                                      , stride * h, NULL );
		CGImageRef img = CGImageCreate( w, h, 8, 32, stride, cs
		                              , kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little
		                              , prov, NULL, FALSE, kCGRenderingIntentDefault );
		// CALayer setContents: accepts a CGImageRef (as id)
		msgVoidId( r->caLayer, "setContents:", (id)img );
		CGImageRelease( img );
		CGDataProviderRelease( prov );
		CGColorSpaceRelease( cs );
	}
}

//----------------------------------------------------------------------------
// Runtime-built NSView subclass: forwards mouse / key events to SACK
//----------------------------------------------------------------------------

static void DispatchMouse( PXPANEL r ) {
	if( r && r->mouse )
		r->mouse( r->psvMouse, r->mouse_x, r->mouse_y, r->mouse_b );
}

// NSEvent helpers
static int EventButton( id ev, uint32_t bit, uint32_t down, PXPANEL r ) {
	if( down ) r->mouse_b |= bit; else r->mouse_b &= ~bit;
	return 0;
}
static void UpdateMouseFromEvent( PXPANEL r, id ev ) {
	// locationInWindow -> view coords; we approximate with window coords flipped
	CGPoint p = (((CGPoint(*)(id,SEL))objc_msgSend)( ev, SELP("locationInWindow") ));
	r->mouse_x = (int32_t)p.x;
	r->mouse_y = (int32_t)( (double)r->h - p.y ); // flip to top-left origin
}

static void imp_mouseDown( id self, SEL _cmd, id ev ) {
	PXPANEL r = GetPanel( self ); if( !r ) return;
	UpdateMouseFromEvent( r, ev ); r->mouse_b |= MK_LBUTTON; DispatchMouse( r );
}
static void imp_mouseUp( id self, SEL _cmd, id ev ) {
	PXPANEL r = GetPanel( self ); if( !r ) return;
	UpdateMouseFromEvent( r, ev ); r->mouse_b &= ~MK_LBUTTON; DispatchMouse( r );
}
static void imp_rightMouseDown( id self, SEL _cmd, id ev ) {
	PXPANEL r = GetPanel( self ); if( !r ) return;
	UpdateMouseFromEvent( r, ev ); r->mouse_b |= MK_RBUTTON; DispatchMouse( r );
}
static void imp_rightMouseUp( id self, SEL _cmd, id ev ) {
	PXPANEL r = GetPanel( self ); if( !r ) return;
	UpdateMouseFromEvent( r, ev ); r->mouse_b &= ~MK_RBUTTON; DispatchMouse( r );
}
static void imp_mouseMoved( id self, SEL _cmd, id ev ) {
	PXPANEL r = GetPanel( self ); if( !r ) return;
	UpdateMouseFromEvent( r, ev ); DispatchMouse( r );
}
static void imp_keyEvent( id self, SEL _cmd, id ev, uint32_t pressed ) {
	PXPANEL r = GetPanel( self ); if( !r || !r->pKeyProc ) return;
	{
		unsigned short kc = (((unsigned short(*)(id,SEL))objc_msgSend)( ev, SELP("keyCode") ));
		uint32_t key = kc | (pressed ? KEY_PRESSED : 0);
		r->pKeyProc( r->dwKeyData, key );
	}
}
static void imp_keyDown( id self, SEL _cmd, id ev ) { imp_keyEvent( self, _cmd, ev, 1 ); }
static void imp_keyUp  ( id self, SEL _cmd, id ev ) { imp_keyEvent( self, _cmd, ev, 0 ); }
static BOOL imp_acceptsFirstResponder( id self, SEL _cmd ) { return YES; }
static BOOL imp_isFlipped( id self, SEL _cmd ) { return YES; } // top-left origin

static void BuildViewClass( void ) {
	Class sup = objc_getClass( "NSView" );
	Class c = objc_allocateClassPair( sup, "SackCocoaView", 0 );
	class_addMethod( c, SELP("mouseDown:"),        (IMP)imp_mouseDown,      "v@:@" );
	class_addMethod( c, SELP("mouseUp:"),          (IMP)imp_mouseUp,        "v@:@" );
	class_addMethod( c, SELP("mouseDragged:"),     (IMP)imp_mouseMoved,     "v@:@" );
	class_addMethod( c, SELP("rightMouseDown:"),   (IMP)imp_rightMouseDown, "v@:@" );
	class_addMethod( c, SELP("rightMouseUp:"),     (IMP)imp_rightMouseUp,   "v@:@" );
	class_addMethod( c, SELP("rightMouseDragged:"),(IMP)imp_mouseMoved,     "v@:@" );
	class_addMethod( c, SELP("mouseMoved:"),       (IMP)imp_mouseMoved,     "v@:@" );
	class_addMethod( c, SELP("keyDown:"),          (IMP)imp_keyDown,        "v@:@" );
	class_addMethod( c, SELP("keyUp:"),            (IMP)imp_keyUp,          "v@:@" );
	class_addMethod( c, SELP("acceptsFirstResponder"), (IMP)imp_acceptsFirstResponder, "B@:" );
	class_addMethod( c, SELP("isFlipped"),         (IMP)imp_isFlipped,      "B@:" );
	objc_registerClassPair( c );
	cocoa_l.viewClass = (void*)c;
}

//----------------------------------------------------------------------------
// Runtime-built window delegate: close / resize
//----------------------------------------------------------------------------
static BOOL imp_windowShouldClose( id self, SEL _cmd, id sender ) {
	PXPANEL r = GetPanel( self );
	if( r && r->pWindowClose )
		r->pWindowClose( r->dwCloseData );
	return YES;
}
static void imp_windowDidResize( id self, SEL _cmd, id note ) {
	PXPANEL r = GetPanel( self );
	if( !r ) return;
	// query the content view bounds and resize the framebuffer
	{
		id view = (id)r->nsView;
		CGRect b = (((CGRect(*)(id,SEL))objc_msgSend)( view, SELP("bounds") ));
		uint32_t nw = (uint32_t)b.size.width, nh = (uint32_t)b.size.height;
		if( nw && nh && ( nw != r->w || nh != r->h ) ) {
			r->w = nw; r->h = nh;
			r->framebuffer = (PCOLOR)Reallocate( r->framebuffer, (size_t)nw * nh * 4 );
			r->pImageDraw = RemakeImage( r->pImageDraw, r->framebuffer, nw, nh );
			if( r->pRedrawCallback )
				r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r );
		}
	}
}
static void BuildDelegateClass( void ) {
	Class sup = objc_getClass( "NSObject" );
	Class c = objc_allocateClassPair( sup, "SackCocoaWindowDelegate", 0 );
	class_addMethod( c, SELP("windowShouldClose:"), (IMP)imp_windowShouldClose, "B@:@" );
	class_addMethod( c, SELP("windowDidResize:"),   (IMP)imp_windowDidResize,   "v@:@" );
	objc_registerClassPair( c );
	cocoa_l.delegateClass = (void*)c;
}

//----------------------------------------------------------------------------
// One-time backend init
//----------------------------------------------------------------------------
static int InitializeDisplay( void ) {
	if( cocoa_l.flags.bInited )
		return TRUE;
	InitializeCriticalSec( &cocoa_l.cs );
	cocoa_l.pii = GetImageInterface();
	cocoa_l.default_display_x = 1280;
	cocoa_l.default_display_y = 1024;

	// NSApplication must exist before any window is created.
	cocoa_l.nsApp = (void*)msgId( CLS("NSApplication"), "sharedApplication" );
	// NSApplicationActivationPolicyRegular == 0
	(((void(*)(id,SEL,long))objc_msgSend)( (id)cocoa_l.nsApp, SELP("setActivationPolicy:"), 0 ));
	cocoa_l.flags.bAppInited = 1;

	BuildViewClass();
	BuildDelegateClass();
	cocoa_l.flags.bInited = 1;
	return TRUE;
}

//----------------------------------------------------------------------------
// Display open / close
//----------------------------------------------------------------------------
static PRENDERER sack_cocoa_OpenDisplayAboveUnderSizedAt( uint32_t attr, uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above, PRENDERER under ) {
	PXPANEL r;
	InitializeDisplay();

	r = New( struct cocoa_video_tag );
	MemSet( r, 0, sizeof( *r ) );
	r->above = (PXPANEL)above;
	r->under = (PXPANEL)under;

	if( x < 0 ) x = cocoa_l.opens * 25;
	if( y < 0 ) y = cocoa_l.opens * 25;
	cocoa_l.opens++;
	if( (int)w <= 0 ) w = 800;
	if( (int)h <= 0 ) h = 600;
	r->x = x; r->y = y; r->w = w; r->h = h;

	r->framebuffer = (PCOLOR)Allocate( (size_t)w * h * 4 );
	MemSet( r->framebuffer, 0, (size_t)w * h * 4 );
	r->pImageDraw = RemakeImage( NULL, r->framebuffer, w, h );

	EnterCriticalSec( &cocoa_l.cs );
	{
		CGRect frame = CGRectMake( x, y, w, h );
		unsigned long style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
		                    | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
		id win = msgId( CLS("NSWindow"), "alloc" );
		win = (((id(*)(id,SEL,CGRect,unsigned long,unsigned long,BOOL))objc_msgSend)
		       ( win, SELP("initWithContentRect:styleMask:backing:defer:")
		       , frame, style, NSBackingStoreBuffered, NO ));
		r->nsWindow = (void*)win;

		// content view: our input-handling subclass, layer-backed
		{
			id view = msgId( (id)cocoa_l.viewClass, "alloc" );
			view = (((id(*)(id,SEL,CGRect))objc_msgSend)( view, SELP("initWithFrame:"), CGRectMake(0,0,w,h) ));
			r->nsView = (void*)view;
			msgVoidBool( view, "setWantsLayer:", YES );
			r->caLayer = (void*)msgId( view, "layer" );
			r->flags.bLayerReady = 1;
			SetPanel( view, r );
			msgVoidId( win, "setContentView:", view );
		}
		// delegate for close/resize
		{
			id del = msgId( msgId( (id)cocoa_l.delegateClass, "alloc" ), "init" );
			r->windowDelegate = (void*)del;
			SetPanel( del, r );
			msgVoidId( win, "setDelegate:", del );
		}
		msgVoidId( win, "makeKeyAndOrderFront:", (id)0 );
		msgVoidBool( win, "setReleasedWhenClosed:", NO );
		r->flags.bShown = 1;
	}
	AddLink( &cocoa_l.pActiveList, r );
	LeaveCriticalSec( &cocoa_l.cs );

	if( r->pRedrawCallback )
		r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r );
	return (PRENDERER)r;
}

static PRENDERER sack_cocoa_OpenDisplayAboveSizedAt( uint32_t attr, uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above ) {
	return sack_cocoa_OpenDisplayAboveUnderSizedAt( attr, w, h, x, y, above, NULL );
}
static PRENDERER sack_cocoa_OpenDisplaySizedAt( uint32_t attr, uint32_t w, uint32_t h, int32_t x, int32_t y ) {
	return sack_cocoa_OpenDisplayAboveUnderSizedAt( attr, w, h, x, y, NULL, NULL );
}

static void sack_cocoa_CloseDisplay( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	if( !r ) return;
	if( cocoa_l.hVidFocused == r ) cocoa_l.hVidFocused = NULL;
	if( cocoa_l.hCaptured == r )   cocoa_l.hCaptured = NULL;
	EnterCriticalSec( &cocoa_l.cs );
	if( r->nsWindow ) {
		msgVoidId( r->nsWindow, "setDelegate:", (id)0 );
		msgVoid( r->nsWindow, "close" );
	}
	DeleteLink( &cocoa_l.pActiveList, r );
	LeaveCriticalSec( &cocoa_l.cs );
	if( r->pImageDraw ) UnmakeImageFile( r->pImageDraw );
	if( r->framebuffer ) Deallocate( PCOLOR, r->framebuffer );
	Release( r );
}

//----------------------------------------------------------------------------
// Update (blit) / image
//----------------------------------------------------------------------------
static void sack_cocoa_UpdateDisplayPortionEx( PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS ) {
	PXPANEL r = (PXPANEL)renderer;
	if( !r ) return;
	r->flags.hidden = 0;
	// the framebuffer already holds what the app drew (pImageDraw is over it);
	// push it to the layer.  CALayer mutation must happen on the main thread.
	PresentFramebuffer( r );
}
static void sack_cocoa_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	PXPANEL r = (PXPANEL)renderer;
	if( r ) sack_cocoa_UpdateDisplayPortionEx( renderer, 0, 0, r->w, r->h DBG_RELAY );
}
static Image sack_cocoa_GetDisplayImage( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	return r ? r->pImageDraw : NULL;
}

//----------------------------------------------------------------------------
// Geometry
//----------------------------------------------------------------------------
static void sack_cocoa_GetDisplaySize( uint32_t *w, uint32_t *h ) {
	if( w ) *w = cocoa_l.default_display_x;
	if( h ) *h = cocoa_l.default_display_y;
}
static void sack_cocoa_SetDisplaySize( uint32_t w, uint32_t h ) { }
static void sack_cocoa_GetDisplayPosition( PRENDERER renderer, int32_t* x, int32_t* y, uint32_t* w, uint32_t* h ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return;
	if( x ) *x = r->x; if( y ) *y = r->y;
	if( w ) *w = r->w; if( h ) *h = r->h;
}
static void sack_cocoa_MoveDisplay( PRENDERER renderer, int32_t x, int32_t y ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow ) return;
	r->x = x; r->y = y;
	(((void(*)(id,SEL,CGPoint))objc_msgSend)( (id)r->nsWindow, SELP("setFrameTopLeftPoint:"), CGPointMake(x,y) ));
}
static void sack_cocoa_MoveDisplayRel( PRENDERER renderer, int32_t dx, int32_t dy ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return;
	sack_cocoa_MoveDisplay( renderer, r->x + dx, r->y + dy );
}
static void sack_cocoa_SizeDisplay( PRENDERER renderer, uint32_t w, uint32_t h ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow ) return;
	r->w = w; r->h = h;
	(((void(*)(id,SEL,CGSize))objc_msgSend)( (id)r->nsWindow, SELP("setContentSize:"), CGSizeMake(w,h) ));
}
static void sack_cocoa_SizeDisplayRel( PRENDERER renderer, int32_t dw, int32_t dh ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return;
	sack_cocoa_SizeDisplay( renderer, r->w + dw, r->h + dh );
}
static void sack_cocoa_MoveSizeDisplayRel( PRENDERER renderer, int32_t dx, int32_t dy, int32_t dw, int32_t dh ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return;
	sack_cocoa_MoveDisplay( renderer, r->x + dx, r->y + dy );
	sack_cocoa_SizeDisplay( renderer, r->w + dw, r->h + dh );
}
void GetDisplaySizeEx( int nDisplay, int32_t *x, int32_t *y, uint32_t *width, uint32_t *height ) {
	if( x ) *x = 0; if( y ) *y = 0;
	if( width ) *width = cocoa_l.default_display_x;
	if( height ) *height = cocoa_l.default_display_y;
}

//----------------------------------------------------------------------------
// Show / hide
//----------------------------------------------------------------------------
static void sack_cocoa_HideDisplay( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow ) return;
	r->flags.hidden = 1;
	msgVoidId( r->nsWindow, "orderOut:", (id)0 );
}
static void sack_cocoa_RestoreDisplayEx( PRENDERER renderer DBG_PASS ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow ) return;
	r->flags.hidden = 0;
	msgVoidId( r->nsWindow, "makeKeyAndOrderFront:", (id)0 );
}
static void sack_cocoa_RestoreDisplay( PRENDERER renderer ) {
	sack_cocoa_RestoreDisplayEx( renderer DBG_SRC );
}
static void sack_cocoa_ForceDisplayFocus( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow ) return;
	msgVoidId( r->nsWindow, "makeKeyAndOrderFront:", (id)0 );
}

//----------------------------------------------------------------------------
// Handlers
//----------------------------------------------------------------------------
static void sack_cocoa_SetCloseHandler( PRENDERER renderer, CloseCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pWindowClose = cb; r->dwCloseData = psv;
}
static void sack_cocoa_SetMouseHandler( PRENDERER renderer, MouseCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->mouse = cb; r->psvMouse = psv;
}
static void sack_cocoa_SetRedrawHandler( PRENDERER renderer, RedrawCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pRedrawCallback = cb; r->dwRedrawData = psv;
}
static void sack_cocoa_SetKeyboardHandler( PRENDERER renderer, KeyProc cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pKeyProc = cb; r->dwKeyData = psv;
}
static void sack_cocoa_SetLoseFocusHandler( PRENDERER renderer, LoseFocusCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pLoseFocus = cb; r->dwLoseFocus = psv;
}
static void sack_cocoa_SetHideHandler( PRENDERER renderer, HideAndRestoreCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pHideCallback = cb; r->dwHideData = psv;
}
static void sack_cocoa_SetRestoreHandler( PRENDERER renderer, HideAndRestoreCallback cb, uintptr_t psv ) {
	PXPANEL r = (PXPANEL)renderer; if( !r ) return; r->pRestoreCallback = cb; r->dwRestoreData = psv;
}

//----------------------------------------------------------------------------
// Misc / queries
//----------------------------------------------------------------------------
static void sack_cocoa_GetMouseState( int32_t *x, int32_t *y, uint32_t *b ) {
	PXPANEL r = cocoa_l.hVidFocused ? cocoa_l.hVidFocused : (PXPANEL)( cocoa_l.pActiveList ? GetLink( &cocoa_l.pActiveList, 0 ) : NULL );
	if( r ) { if( x ) *x = r->mouse_x; if( y ) *y = r->mouse_y; if( b ) *b = r->mouse_b; }
	else    { if( x ) *x = 0; if( y ) *y = 0; if( b ) *b = 0; }
}
static const TEXTCHAR *sack_cocoa_GetKeyText( int key ) {
	// TODO: map virtual keycode -> typed text via a keymap (keymap_mac.c).
	return NULL;
}
static int  sack_cocoa_IsTouchDisplay( void ) { return FALSE; }
static LOGICAL sack_cocoa_RequiresDrawAll( void ) { return FALSE; }
static void sack_cocoa_SetRendererTitle( PRENDERER renderer, char const *title ) {
	PXPANEL r = (PXPANEL)renderer; if( !r || !r->nsWindow || !title ) return;
	{
		id ns = msgIdCStr( CLS("NSString"), "stringWithUTF8String:", title );
		msgVoidId( r->nsWindow, "setTitle:", ns );
	}
}
static void sack_cocoa_SetApplicationTitle( char const *title ) { }
static void sack_cocoa_SetApplicationIcon( Image icon ) { }
static void sack_cocoa_Redraw( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	if( r && r->pRedrawCallback )
		r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r );
	if( r ) PresentFramebuffer( r );
}

//----------------------------------------------------------------------------
// Interface table (same layout/order as the wayland driver; unimplemented
// entries are NULL).
//----------------------------------------------------------------------------
static RENDER_INTERFACE VidInterface = { InitializeDisplay
	, sack_cocoa_SetApplicationTitle
	, sack_cocoa_SetApplicationIcon
	, sack_cocoa_GetDisplaySize
	, sack_cocoa_SetDisplaySize
	, sack_cocoa_OpenDisplaySizedAt
	, sack_cocoa_OpenDisplayAboveSizedAt
	, sack_cocoa_CloseDisplay
	, sack_cocoa_UpdateDisplayPortionEx
	, sack_cocoa_UpdateDisplayEx
	, sack_cocoa_GetDisplayPosition
	, sack_cocoa_MoveDisplay
	, sack_cocoa_MoveDisplayRel
	, sack_cocoa_SizeDisplay
	, sack_cocoa_SizeDisplayRel
	, sack_cocoa_MoveSizeDisplayRel
	, NULL // PutDisplayAbove
	, (Image (CPROC*)(PRENDERER)) sack_cocoa_GetDisplayImage
	, sack_cocoa_SetCloseHandler
	, sack_cocoa_SetMouseHandler
	, sack_cocoa_SetRedrawHandler
	, sack_cocoa_SetKeyboardHandler
	, sack_cocoa_SetLoseFocusHandler
	, NULL
	, NULL // GetMousePosition
	, NULL // SetMousePosition
	, NULL // HasFocus
	, sack_cocoa_GetKeyText
	, NULL // IsKeyDown
	, NULL // KeyDown
	, NULL // DisplayIsValid
	, NULL // OwnMouseEx
	, NULL // BeginCalibration
	, NULL // SyncRender
	, NULL // MoveSizeDisplay
	, NULL // MakeTopmost
	, sack_cocoa_HideDisplay
	, sack_cocoa_RestoreDisplay
	, sack_cocoa_ForceDisplayFocus
	, NULL // ForceDisplayFront
	, NULL // ForceDisplayBack
	, NULL // BindEventToKey
	, NULL // UnbindKey
	, NULL // IsTopmost
	, NULL // OkaySyncRender
	, sack_cocoa_IsTouchDisplay
	, sack_cocoa_GetMouseState
	, NULL // EnableSpriteMethod
	, NULL // WinShell_AcceptDroppedFiles
	, NULL // PutDisplayIn
	, NULL // MakeDisplayFrom
	, sack_cocoa_SetRendererTitle
	, NULL // DisableMouseOnIdle
	, sack_cocoa_OpenDisplayAboveUnderSizedAt
	, NULL // SetDisplayNoMouse
	, sack_cocoa_Redraw
	, NULL // MakeAbsoluteTopmost
	, NULL // SetDisplayFade
	, NULL // IsDisplayHidden
	, GetDisplaySizeEx
	, NULL
	, NULL // LockRenderer
	, NULL // UnlockRenderer
	, sack_cocoa_RequiresDrawAll
#ifndef NO_TOUCH
	, NULL // SetTouchHandler
#endif
	, NULL // MarkDisplayUpdated
	, sack_cocoa_SetHideHandler
	, sack_cocoa_SetRestoreHandler
	, sack_cocoa_RestoreDisplayEx
	, NULL // show input device
	, NULL // hide input device
	, NULL // AllowsAnyThreadToUpdate
	, NULL // SetDisplayFullScreen
	, NULL // SuspendSystemSleep
	, NULL // is instanced
	, NULL // render allows copy
	, NULL // SetDisplayCursor
	, NULL // IsDisplayRedrawForced
	, NULL // ReplyCloseDisplay
	, NULL // SetClipboardEventCallback
	, NULL // BeginMoveDisplay
	, NULL // BeginSizeDisplay
	, NULL // WillUpdatePortions
};

static POINTER GetCocoaDisplayInterface( void ) { return (POINTER)&VidInterface; }
static void DropCocoaDisplayInterface( POINTER p ) { }

//----------------------------------------------------------------------------
// Main-thread event servicing.
//
// AppKit requires its event loop to run on the OS main thread.  In an embedded
// host (e.g. a Node/libuv process, where V8/libuv own the main thread) we must
// NOT take the thread over with [NSApp run]; instead the host drains pending
// Cocoa events cooperatively, once per main-thread turn, by calling
// cocoa_PumpEvents() (e.g. from a libuv uv_check_t/uv_prepare_t handle, or from
// an IdleFor() loop running on the main thread).  This is non-blocking.
//----------------------------------------------------------------------------
void cocoa_PumpEvents( void ) {
	id pool;
	id distantPast;
	id mode;
	if( !cocoa_l.flags.bAppInited )
		return;
	// AppKit calls are only valid from the OS main thread.
	if( !pthread_main_np() )
		return;

	pool = msgId( msgId( CLS("NSAutoreleasePool"), "alloc" ), "init" );
	distantPast = msgId( CLS("NSDate"), "distantPast" );
	mode = msgIdCStr( CLS("NSString"), "stringWithUTF8String:", "kCFRunLoopDefaultMode" );
	for( ;; ) {
		id ev = (((id(*)(id,SEL,unsigned long long,id,id,BOOL))objc_msgSend)
		        ( (id)cocoa_l.nsApp, SELP("nextEventMatchingMask:untilDate:inMode:dequeue:")
		        , ~0ull /* NSEventMaskAny */, distantPast, mode, YES ));
		if( !ev )
			break;
		msgVoidId( cocoa_l.nsApp, "sendEvent:", ev );
	}
	msgVoid( cocoa_l.nsApp, "updateWindows" );
	msgVoid( pool, "drain" );
}

// SACK idle hook: lets a pure-SACK app whose main thread runs Idle()/IdleFor()
// drive the pump automatically.  Self-filters to the OS main thread (returns -1
// otherwise) exactly like the wayland driver's do_waylandThread.  In a Node
// host this proc will simply no-op on worker threads; the host drives the pump
// from its own main-thread loop instead.
static int do_cocoaPump( uintptr_t unused ) {
	if( !pthread_main_np() )
		return -1;
	if( !cocoa_l.flags.bAppInited )
		return -1;
	cocoa_PumpEvents();
	return -1; // never reports "work pending" - don't busy-spin the idle loop
}

// Standalone (non-embedded) convenience: take over the main thread with the
// AppKit run loop.  Do NOT use this when another runtime (Node/libuv) owns the
// main thread - use cocoa_PumpEvents() cooperatively instead.
void cocoa_RunApplication( void ) {
	InitializeDisplay();
	msgVoid( cocoa_l.nsApp, "run" );
}

PRIORITY_PRELOAD( CocoaRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	RegisterInterface( "sack.render.cocoa", GetCocoaDisplayInterface, DropCocoaDisplayInterface );
	AddIdleProc( do_cocoaPump, 0 );
}

RENDER_NAMESPACE_END
