#define USE_IMAGE_INTERFACE wl.pii
#include <stdhdrs.h>
#include <render.h>
#include <image.h>

#include <wayland-client.h>

//struct wl_display *display = NULL;
//struct HVIDEO_tag

typedef struct wvideo_tag
{
	struct {
		uint32_t bShown : 1;
	} flags;

	int32_t x, y;
	uint32_t w, h;

	struct wvideo_tag* under;  // this window is under this reference
	struct wvideo_tag* above;  // this window is above the one pointed here

	struct wl_surface* surface;
	Image pImage;
} XPANEL, *PXPANEL;

struct wayland_local_tag
{
	PIMAGE_INTERFACE pii;
	struct {
		uint32_t bInited : 1;
	} flags;
	struct wl_display* display;
	struct wl_compositor *compositor;
	struct wl_surface *surface;
	struct wl_shell *shell;
	struct wl_shell_surface *shell_surface;
	int screen_width;
	int screen_height;
	int screen_num;
/* these variables will be used to store the IDs of the black and white */
/* colors of the given screen. More on this will be explained later.    */
	unsigned long white_pixel;
	unsigned long black_pixel;

};
struct wayland_local_tag wl;

/*
 wl_compositor id 1
 wl_subcompositor id 2
 wp_viewporter id 3
 zxdg_output_manager_v1 id 4
 wp_presentation id 5
 zwp_relative_pointer_manager_v1 id 6
 zwp_pointer_constraints_v1 id 7
 zwp_input_timestamps_manager_v1 id 8
 wl_data_device_manager id 9
 wl_shm id 10
 wl_drm id 11
 wl_seat id 12
 zwp_linux_dmabuf_v1 id 13
 weston_direct_display_v1 id 14
 zwp_linux_explicit_synchronization_v1 id 15
 weston_content_protection id 16
 wl_output id 17
 wl_output id 18
 zwp_input_panel_v1 id 19
 zwp_input_method_v1 id 20
 zwp_text_input_manager_v1 id 21
 xdg_wm_base id 22
 zxdg_shell_v6 id 23
 wl_shell id 24
 weston_desktop_shell id 25
 weston_screenshooter id 26
*/

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
	lprintf("Got a registry event for %s id %d", interface, id);
	if( strcmp(interface, "wl_compositor") == 0 ) {
		wl.compositor = wl_registry_bind(registry,
				      id,
				      &wl_compositor_interface,
				      1);
	} else if( strcmp(interface, "wl_shell") == 0 ) {
        wl.shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    lprintf("Got a registry losing event for %d", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};




static void initConnections( void ) {
	wl.pii = GetImageInterface();
	wl.display = wl_display_connect(NULL);
	if (wl.display == NULL) {
		lprintf( "Can't connect to display");
		return;
	}

	struct wl_registry *registry = wl_display_get_registry(wl.display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	wl_display_dispatch(wl.display);
	wl_display_roundtrip(wl.display);

	if (!wl.compositor ) {
		lprintf( "Can't find compositor");
		return;
	} else {
		lprintf( "Found compositor");
	}

   wl.surface = wl_compositor_create_surface(wl.compositor);
   if (wl.surface == NULL) {
		lprintf( "Can't create surface");
		return;
   }

   wl.shell_surface = wl_shell_get_shell_surface(wl.shell, wl.surface);
   if (wl.shell_surface == NULL) {
		lprintf("Can't create shell surface");
		return;
   }

	wl_shell_surface_set_toplevel(wl.shell_surface);


}

static int InitializeDisplay( void ) {
	return 1;
}

static void shutdownDisplay( void ) {
	wl_display_disconnect(wl.display);

}

void DoDestroy (PRENDERER hVideo) {

}

void GetDisplaySizeEx( int display, int* x, int* y, uint32_t* w, uint32_t* h ){
	int j1;
	uint32_t j2;
	if( !x ) x = &j1;
	if( !y ) y = &j1;
	if( !w ) w = &j2;
	if( !h ) h = &j2;


}


LOGICAL CreateWindowStuff(PXPANEL r )
{
	//lprintf( "-----Create WIndow Stuff----- %s %s", hVideo->flags.bLayeredWindow?"layered":"solid"
	//		 , hVideo->flags.bChildWindow?"Child(tool)":"user-selectable" );
	//DebugBreak();
	r->pImage = MakeImageFile( r->w, r->h );

	if (r->x == -1 || r->y == -1 )
	{
		uint32_t w, h;
		GetDisplaySizeEx ( 0, NULL, NULL, &w, &h);
		if( r->x == -1 )
			r->x = w * 7 / 10;
		if( r->y == -1 )
			r->y = h * 7 / 10;
	}
	//SetParent( hVideo->hWndOutput, l.hWndInstance );
	return TRUE;
}


RENDER_PROC (void, UpdateDisplayPortionEx)( PRENDERER pPanel_
                                          , int32_t x, int32_t y
                                          , uint32_t w, uint32_t h DBG_PASS)
{
}


int InitDisplay( void )
{
	if( !wl.flags.bInited )
	{
		char *display_name = getenv( "WAYLAND_DISPLAY" );
		initConnections();
		wl.flags.bInited = 1;

	}
}

LOGICAL CreateDrawingSurface( PXPANEL *pPanel )
{
   /* create the window, as specified earlier. */
}

void ShutdownVideo( void )
{
	if( wl.flags.bInited )
	{
	}
}


static int CPROC ProcessDisplayMessages(void )
{
}

static void sack_wayland_Redraw( PRENDERER renderer ) {
	lprintf( "Issue redraw on renderer %p", renderer );
}

static void sack_wayland_SetApplicationTitle( char const *title ){

}

static void sack_wayland_SetApplicationIcon( Image icon ) {

}

static void sack_wayland_GetDisplaySize( uint32_t *w, uint32_t *h ){

}

static void sack_wayland_SetDisplaySize( uint32_t w, uint32_t h ) {

}

static PRENDERER sack_wayland_OpenDisplayAboveUnderSizedAt(uint32_t attr , uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above, PRENDERER under ){
	struct wvideo_tag *rAbove = (struct wvideo_tag*)above;
	struct wvideo_tag *rUnder = (struct wvideo_tag*)under;
	struct wvideo_tag *r;
	r = New( struct wvideo_tag );
	memset( r, 0, sizeof( *r ) );

	r->w = w;
	r->h = h;
	r->x = x;
	r->y = y;

	CreateWindowStuff( r );

	return (PRENDERER)r;
}


static PRENDERER sack_wayland_OpenDisplayAboveSizedAt(uint32_t attr , uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above ){
	return sack_wayland_OpenDisplayAboveUnderSizedAt( attr, w, h, x, y, above, NULL );

}

static PRENDERER sack_wayland_OpenDisplaySizedAt(uint32_t attr , uint32_t w, uint32_t h, int32_t x, int32_t y){
	return sack_wayland_OpenDisplayAboveUnderSizedAt( attr, w, h, x, y, NULL, NULL );
}

static void sack_wayland_CloseDisplay( PRENDERER renderer ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_UpdateDisplayPortionEx(PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_GetDisplayPosition( PRENDERER renderer, int32_t* x, int32_t* y, uint32_t* w, uint32_t* h ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_MoveDisplay(PRENDERER renderer, int32_t x, int32_t y){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}
static void sack_wayland_MoveDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}


static void sack_wayland_SizeDisplay(PRENDERER renderer, uint32_t w, uint32_t h){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}
static void sack_wayland_SizeDisplayRel(PRENDERER renderer, int32_t dw, int32_t dh){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_MoveSizeDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy, int32_t dw, int32_t dh){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_SetCloseHandler( PRENDERER renderer, CloseCallback closeCallback, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_SetMouseHandler( PRENDERER renderer, MouseCallback mouse, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}
static void sack_wayland_SetRedrawHandler( PRENDERER renderer, RedrawCallback redraw, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_SetKeyboardHandler(PRENDERER renderer, KeyProc keyproc, uintptr_t psv){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_SetLoseFocusHandler( PRENDERER renderer, LoseFocusCallback lfCallback, uintptr_t psv ) {

}

static void sack_wayland_SetHideHandler( PRENDERER renderer, HideAndRestoreCallback hideCallback, uintptr_t psv ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_SetRestoreHandler( PRENDERER renderer, HideAndRestoreCallback hideCallback, uintptr_t psv ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static LOGICAL sack_wayland_RequiresDrawAll( void ) {
	return FALSE;
}


static  int sack_wayland_IsTouchDisplay ( void ){
	return FALSE;
}


static void sack_wayland_SetRendererTitle( PRENDERER render, char const* title ){

}

static void sack_wayland_RestoreDisplayEx( PRENDERER renderer DBG_PASS ){

}

static void sack_wayland_RestoreDisplay( PRENDERER renderer ){
	return sack_wayland_RestoreDisplayEx( renderer DBG_SRC );
}

static void sack_wayland_HideDisplay( PRENDERER renderer ){

}

Image GetDisplayImage(PRENDERER renderer) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r )
		return r->pImage;
	return NULL;
}

static RENDER_INTERFACE VidInterface = { InitializeDisplay

													, sack_wayland_SetApplicationTitle
													, sack_wayland_SetApplicationIcon
													, sack_wayland_GetDisplaySize
													, sack_wayland_SetDisplaySize
													, sack_wayland_OpenDisplaySizedAt
													, sack_wayland_OpenDisplayAboveSizedAt
													, sack_wayland_CloseDisplay
													, sack_wayland_UpdateDisplayPortionEx
													, sack_wayland_UpdateDisplayEx
													, sack_wayland_GetDisplayPosition
													, sack_wayland_MoveDisplay
													, sack_wayland_MoveDisplayRel
													, sack_wayland_SizeDisplay
													, sack_wayland_SizeDisplayRel
													, sack_wayland_MoveSizeDisplayRel
													, NULL //(void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
													, (Image (CPROC*)(PRENDERER)) GetDisplayImage
													, sack_wayland_SetCloseHandler
													, sack_wayland_SetMouseHandler
													, sack_wayland_SetRedrawHandler
													, sack_wayland_SetKeyboardHandler
													, sack_wayland_SetLoseFocusHandler
													, NULL
													,NULL// (void (CPROC*)(int32_t *, int32_t *)) GetMousePosition
													, NULL//(void (CPROC*)(PRENDERER, int32_t, int32_t)) SetMousePosition
													, NULL//HasFocus  // has focus
													, NULL//GetKeyText
													, NULL//IsKeyDown
													, NULL//KeyDown
													, NULL//DisplayIsValid
													, NULL//OwnMouseEx
													, NULL//BeginCalibration
													, NULL//SyncRender	// sync
													, NULL//MoveSizeDisplay
													, NULL//MakeTopmost
													, sack_wayland_HideDisplay
													, sack_wayland_RestoreDisplay
													, NULL//ForceDisplayFocus
													, NULL//ForceDisplayFront
													, NULL//ForceDisplayBack
													, NULL//BindEventToKey
													, NULL//UnbindKey
													, NULL//IsTopmost
													, NULL // OkaySyncRender is internal.
													, sack_wayland_IsTouchDisplay
													, NULL//GetMouseState
													, NULL//EnableSpriteMethod
													, NULL//WinShell_AcceptDroppedFiles
													, NULL//PutDisplayIn
													, NULL//MakeDisplayFrom
													, sack_wayland_SetRendererTitle
													, NULL//DisableMouseOnIdle
													, sack_wayland_OpenDisplayAboveUnderSizedAt
													, NULL//SetDisplayNoMouse
													, sack_wayland_Redraw
													, NULL//MakeAbsoluteTopmost
													, NULL//SetDisplayFade
													, NULL//IsDisplayHidden
													, NULL
#ifdef WIN32
													, NULL//GetNativeHandle
#endif
													, NULL//GetDisplaySizeEx
													, NULL//LockRenderer
													, NULL//UnlockRenderer
													//, NULL//IssueUpdateLayeredEx
													, sack_wayland_RequiresDrawAll
#ifndef NO_TOUCH
													, NULL//SetTouchHandler
#endif
													, NULL//MarkDisplayUpdated
													, sack_wayland_SetHideHandler
													, sack_wayland_SetRestoreHandler
													, sack_wayland_RestoreDisplayEx
													, NULL // show input device
													, NULL // hide input device
													, NULL//AllowsAnyThreadToUpdate
													, NULL//Vidlib_SetDisplayFullScreen
													, NULL//Vidlib_SuspendSystemSleep
													, NULL /* is instanced */
													, NULL /* render allows copy (not remote network render) */
													, NULL//SetDisplayCursor
													, NULL//IsDisplayRedrawForced
};

static POINTER GetWaylandDisplayInterface(void)
{
	InitDisplay();
	return (POINTER)&VidInterface;
}

static void DropWaylandDisplayInterface(POINTER p)
{
}

PRIORITY_PRELOAD( WaylandRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	RegisterInterface( "sack.render.wayland", GetWaylandDisplayInterface, DropWaylandDisplayInterface );
}

