// much core wayland interface code from...
// https://jan.newmarch.name/Wayland/ProgrammingClient/
// https://jan.newmarch.name/Wayland/WhenCanIDraw/
//

//#define DEBUG_DUMP_SURFACE_IMAGES
//#define DEBUG_COMMIT_ATTACH

// last phase of dirty logging
//#define DEBUG_DIRTY_DRAW

//#define DEBUG_SURFACE_ATTACH
// general debug enable...
//#define DEBUG_COMMIT
// tracks the allocation, locking and unlocking of buffers
//#define DEBUG_COMMIT_BUFFER
//#define DEBUG_COMMIT_BUFFER_DETAILS
// subset of just canCommit, not buffer requests and other flows...
//#define DEBUG_COMMIT_STATE
// events related to keys.
//#define DEBUG_KEY_EVENTS
//#define DEBUG_REDRAW

// individual update portion and update call logging 
//#define DEBUG_UPDATE_DISPLAY

// there is a crash (sometimes) when opening a window
//#define DEBUG_SURFACE_INIT

#define USE_IMAGE_INTERFACE wl.pii
#include <stdhdrs.h>
#include <render.h>
#include <image.h>
#include <idle.h>

#include <sys/mman.h>
#include <fcntl.h>

#include "local.h"
//#define ALLOW_KDE

// these(name and version) should be merged into a struct.
//static int supportedVersions[max_interface_versions] = {4,1,1,1,7,1,1,1};
static struct interfaceVersionInfo {
	char const *name;
	int const version;
} interfaces [max_interface_versions] = {
 [wis_compositor]={  "wl_compositor", 4 }
 ,[wis_output]={ "wl_output", 3 }
 ,[wis_subcompositor]={ "wl_subcompositor", 1 }
 ,[wis_shell]={ "wl_shell", 1 }
 ,[wis_seat]={ "wl_seat", 7 }
,[wis_shm]= { "wl_shm", 1 }
,[wis_xdg_shell]={ "zxdg_shell_v6", 1 }
,[	wis_xdg_base]={NULL,3 }
,[	wis_xdg_base]={NULL,1 }
#ifdef ALLOW_KDE
 ,[wis_kde_shell]={ "org_kde_plasma_shell", 1 }
#endif
};


struct wayland_local_tag wl;

static void InitWayland( void );
 static void sack_wayland_BeginMoveDisplay(  PRENDERER renderer );
 static void sack_wayland_BeginSizeDisplay(  PRENDERER renderer, enum sizeDisplayValues mode );
static void sack_wayland_Redraw( PRENDERER renderer );
static void sack_wayland_Redraw_( PXPANEL renderer, int noCallback, volatile int *redrawState_ );
static LOGICAL attachNewBuffer( PXPANEL r, int req, int locked );

static void postFocusEvent( PXPANEL r, LOGICAL focus );
static void postSizeEvent( PXPANEL r, uint32_t w, uint32_t h );

static volatile int redrawState = -1;

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

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
    int32_t y, int32_t physical_width, int32_t physical_height,
    int32_t subpixel, const char *make, const char *model, int32_t transform)
{
  struct output_data *output_data = wl_output_get_user_data(wl_output);
  (void)data;
  (void)wl_output;
  (void)subpixel;
  (void)make;
  (void)model;
  (void)transform;
  // on gWSL 0, 0, 0, 0
  // not sure what real wayland looks like...
  //lprintf( "Screen geometry: %d %d  %d %d", x, y, physical_height, physical_width );
	output_data->x = x;
	output_data->y = y;
	output_data->w_mm = physical_width;
	output_data->h_mm = physical_height;
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
    int32_t width, int32_t height, int32_t refresh)
{
  struct output_data *output_data = wl_output_get_user_data(wl_output);
  (void)data;
  (void)wl_output;
  (void)refresh;
  // refresh/1000 = hz
  if( flags ) {
	  if( flags & WL_OUTPUT_MODE_CURRENT ) {
		  output_data->w = width;
		  output_data->h = height;
	  }
  	//lprintf( "Screen mode:%d %d %d  %d ", flags, width, height, refresh );
  }
}

static void output_done(void *data, struct wl_output *wl_output)
{
  (void)data;
  struct output_data *output_data = wl_output_get_user_data(wl_output);
  output_data->scale = output_data->pending_scale;
}

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
  (void)data;
  struct output_data *output_data = wl_output_get_user_data(wl_output);
  output_data->pending_scale = factor;
  lprintf( "Scale? %d", factor );
}


static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	char *s;
	switch (format) {
	case WL_SHM_FORMAT_ARGB8888: s = "ARGB8888";
		wl.canDraw = 1;
		wl.registering = 0;
		 break;
	case WL_SHM_FORMAT_XRGB8888: s = "XRGB8888"; break;
	case WL_SHM_FORMAT_RGB565: s = "RGB565"; break;
	default: s = "other format"; break;
	}
	//lprintf( "Possible shmem format %s", s);
}



static struct wl_shm_listener shm_listener = {
	shm_format
};

static const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

static void pointer_enter(void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial, struct wl_surface *surface,
    wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct pointer_data *pointer_data;
	if( !surface )  {
		lprintf( "pointer enter No Surface.... not sure how we can handle that" );
		return;
	}
	EnterCriticalSec( &wl.cs_wl );
	pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	pointer_data->target_surface = surface;
	/*
	wl_surface_attach(pointer_data->surface,
		pointer_data->buffer, 0, 0);
	wl_surface_commit(pointer_data->surface);
	*/
	wl_pointer_set_cursor(wl_pointer, serial,
	wl.cursor_surface, pointer_data->hot_spot_x,
	    pointer_data->hot_spot_y);

	wl.mouse_.x = surface_x >> 8;
	wl.mouse_.y = surface_y >> 8;
	//lprintf( "Mouse Enter %d %d" );

	PXPANEL r = wl_surface_get_user_data(
		pointer_data->target_surface);
	//if( !GetPixel( GetDipslaySurface( r ), wl.mouse.x, wl.mouse.y ))
		//wl_seat_pointer_forward( wl.seat, serial );
	LeaveCriticalSec( &wl.cs_wl );

	wl.mouse_.r = r;
	EnqueData( &wl.pdqMouseEvents, &wl.mouse_ );
	WakeThread( wl.drawThread );
}

static void pointer_leave(void *data,
	struct wl_pointer *wl_pointer, uint32_t serial,
	struct wl_surface *wl_surface) {
	//lprintf( "pointer_leave");
	if( !wl_surface ) return; // closed surface..
	EnterCriticalSec( &wl.cs_wl );
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	PXPANEL r = wl_surface_get_user_data(
		pointer_data->target_surface);
	LeaveCriticalSec( &wl.cs_wl );
	//lprintf( "Mouse Leave" );
	wl.mouse_.b = 0;
	wl.mouse_.r = r;
	EnqueData( &wl.pdqMouseEvents, &wl.mouse_ );
	WakeThread( wl.drawThread );

 }

static void pointer_motion(void *data,

	struct wl_pointer *wl_pointer, uint32_t time,
	wl_fixed_t surface_x, wl_fixed_t surface_y) {
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	//lprintf( "pointer motion... ");
	if( !pointer_data->target_surface ) {
		lprintf( "no target for mouse??" ); 
		return;
	}
	PXPANEL r = wl_surface_get_user_data( pointer_data->target_surface);


	{
		int n;
		int found;
		struct mouseEvent *event;
		struct mouseEvent *lastEvent = &wl.mouse_;
		for( n = 0; event = PeekDataInQueueEx( &wl.pdqMouseEvents, n); n++ ) {
			if( event->r == r) {
				lastEvent = event;
			}
		}
		event = lastEvent;
		event->x = surface_x>>8;
		event->y = surface_y>>8;
		if( lastEvent == &wl.mouse_ ) {
			wl.mouse_.r = r;
			EnqueData( &wl.pdqMouseEvents, &wl.mouse_ );
			WakeThread( wl.drawThread );
		}
	}
  	//lprintf( "mouse motion:%d %d", wl.mouse_.x, wl.mouse_.y );
	//if( !GetPixel( GetDipslaySurface( r ), wl.mouse.x, wl.mouse.y ))
		 //wl_seat_pointer_forward( wl.seat, serial );


}

static void pointer_button(void *data,
    struct wl_pointer *wl_pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state)
{
	
	struct pointer_data *pointer_data;
	PXPANEL r;
	void (*callback)(uint32_t);
	wl.mouseSerial = serial;
	//lprintf( "pointer button:%d %d %d", button, state, serial);
	pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	if( button == BTN_LEFT ) {
		if( state )
		wl.mouse_.b |= MK_LBUTTON;
	else
		wl.mouse_.b &= ~MK_LBUTTON;
	}
	if( button == BTN_RIGHT ) {
		if( state )
		wl.mouse_.b |= MK_RBUTTON;
	else
		wl.mouse_.b &= ~MK_RBUTTON;
	}
	if( button == BTN_MIDDLE ) {
		if( state )
		wl.mouse_.b |= MK_MBUTTON;
	else
		wl.mouse_.b &= ~MK_MBUTTON;
	}
	r = wl_surface_get_user_data(
		pointer_data->target_surface);

	wl.mouse_.r = r;
	EnqueData( &wl.pdqMouseEvents, &wl.mouse_ );
	WakeThread( wl.drawThread );

	 	//xdg_toplevel_resize( r->xdg_toplevel, wl.seat, wl.mouseSerial, 12 );

	/*
	if (r != NULL && r->mouse ) {
		r->mouse( r->psvMouse, wl.mouse_.x, wl.mouse_.y, wl.mouse_.b );
		// callback(button);
	}
	*/
}

static void pointer_axis(void *data,
    struct wl_pointer *wl_pointer, uint32_t time,
    uint32_t axis, wl_fixed_t value) {
		 // 0 is default scroll wheel
		//lprintf( "Axis discrete:%d %d.%02x", axis, value>>8, value&0xFF );


	  }

static void pointer_frame( void *data,
    struct wl_pointer *wl_pointer) {
		 // WHY?!
	  }

	static void pointer_axis_source(void *data,
			    struct wl_pointer *wl_pointer,
			    uint32_t axis_source){
					 //lprintf( "Axis source:%d", axis_source );
				 }
	static void pointer_axis_stop(void *data,
			  struct wl_pointer *wl_pointer,
			  uint32_t time,
			  uint32_t axis){
					 //lprintf( "Axis stop:%d %d", time, axis );
			  }
	static void pointer_axis_discrete(void *data,
			      struct wl_pointer *wl_pointer,
			      uint32_t axis,
			      int32_t discrete){
					 //lprintf( "Axis discrete:%d %d", axis, discrete );

					}


static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
	 .frame = pointer_frame,
	 .axis_source =pointer_axis_source,
	 .axis_stop =pointer_axis_stop,
	 .axis_discrete =pointer_axis_discrete,
	 //.release = pointer_release
};


static void keyboard_keymap(void *data,
		       struct wl_keyboard *wl_keyboard,
		       uint32_t format,
		       int32_t fd,
		       uint32_t size)
{

	switch( format ) {
	case WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP:
		break;
	case WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1:
		;

		char *keymap_string = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
		//lprintf( "keystring maps to keyboard :%p %s %d", keymap_string, keymap_string, errno );
		xkb_keymap_unref (wl.keymap);
		wl.keymap = xkb_keymap_new_from_string (wl.xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		munmap (keymap_string, size);
		close (fd);
		xkb_state_unref (wl.xkb_state);
		wl.xkb_state = xkb_state_new (wl.keymap);
		//lprintf( "done in keymap event..");
		break;
	}
}

static void keyboard_enter(void *data,
		      struct wl_keyboard *wl_keyboard,
		      uint32_t serial,
		      struct wl_surface *surface,
		      struct wl_array *keys)
{
	PXPANEL r = (PXPANEL) wl_surface_get_user_data( surface );
	postFocusEvent( r, TRUE );
	uint32_t *key;
	wl_array_for_each( key, keys ){
		char buf[128];
		char buf2[128];
		xkb_keysym_t sym = xkb_state_key_get_one_sym( wl.xkb_state, key[0]+8);
		xkb_keysym_get_name( sym, buf, sizeof( buf) );

		xkb_state_key_get_utf8( wl.xkb_state, key[0]+8, buf2, sizeof( buf2 ) );
#ifdef DEBUG_KEY_EVENTS
		lprintf( "Key enter gets keys? %d %s %s", key[0]+8, buf, buf2 );
#endif
	}
}

static void keyboard_leave(void *data,
                           struct wl_keyboard *wl_keyboard,
                           uint32_t serial,
                           struct wl_surface *surface){
	if( surface ) {
		PXPANEL r = (PXPANEL) wl_surface_get_user_data( surface );
		postFocusEvent( r, FALSE );

		struct pendingKey *key;
		INDEX idx;
		LIST_FORALL( wl.keyRepeat.pendingKeys, idx, struct pendingKey *, key){
			if( key->tick ){
				if( r->pKeyProc ){
#ifdef DEBUG_KEY_EVENTS
					lprintf( "Auto releasing key as key %08x %08x", key->rawKey, key->key );
#endif
					r->pKeyProc( r->dwKeyData, key->key & ~KEY_PRESSED );
				}
				xkb_state_update_key( wl.xkb_state, key->rawKey, XKB_KEY_UP );
			}
		}
		EmptyList( &wl.keyRepeat.pendingKeys );
	}
}



static void keyboard_key(void *data,
                         struct wl_keyboard *wl_keyboard,
                         uint32_t serial,
                         uint32_t time,
                         uint32_t key,
                         uint32_t state)
{
	PXPANEL r = wl.hVidFocused;
	if( !r )  {
		lprintf( "No focused window....");
		return;
	}
	//lprintf( "KEY: %p %d %d %d %d", wl_keyboard, serial, time, key, state );
	wl.key_.r = r;
	xkb_state_update_key( wl.xkb_state, key+8, state == WL_KEYBOARD_KEY_STATE_PRESSED?XKB_KEY_DOWN:XKB_KEY_UP );
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym (wl.xkb_state, key+8);
#ifdef DEBUG_KEY_EVENTS		
		lprintf( "key is still keysym home:%x(%d) pad9:%x(%d) pgup:%x(%d) symchar:%c %04x key:%02x"
					, KEY_HOME, KEY_HOME
					, KEY_PAD_9, KEY_PAD_9
					, KEY_PGUP, KEY_PGUP
					, keysym&0x7F, keysym, key);
#endif			
		uint32_t keycode;
		int extended = ((keysym&0xff00)==0xFF00);
		if( ( ( keysym & 0xFFFFFF00 ) != 0xFF00 ) || (( keysym&0xFF00) && !extended ) ) 
			lprintf( "This keycode was bad: %04x key:%x(%d)", keysym, key, key );
		keycode = KEY_PRESSED;
		keycode |= key;
		keycode |= (keysym&0xFF)<<16;
		keycode |= wl.keyMods | (extended?KEY_MOD_EXTENDED:0);
		wl.key_.keycode = keycode;
		wl.key_.utfKeyCode = xkb_keysym_to_utf32 (keysym);
#ifdef DEBUG_KEY_EVENTS
		lprintf( "Keysym:%d(%04x) %d %d %c(%08x)(%08x)", keysym, keysym, key, state, wl.key_.utfKeyCode, wl.key_.utfKeyCode, wl.keyMods );
#endif
		//wl.key_.tick = timeGetTime(); // record first press time.
		//wl.key_.rawKey = key;
		EnqueData( &wl.pdqKeyEvents, &wl.key_ );

		struct pendingKey *newKey = New(struct pendingKey );//&wl.keyRepeat.pendingKey;
		// and press this new key.

		newKey->r = r;
		newKey->rawKey = key;
		newKey->keysym = keysym;
		newKey->key = keycode;
		newKey->tick = timeGetTime(); // record first press time.
		newKey->repeating = 0;
		AddLink( &wl.keyRepeat.pendingKeys, newKey );

		WakeThread( wl.drawThread ); // let it setup a shorter timer, and handle stopping repeats?
		return;

		if( !wl_HandleKeyEvents( r->pKeyDefs, keycode ) ) {
			if( r->pKeyProc ){
	#ifdef DEBUG_KEY_EVENTS
				lprintf( "Sending as key %08x %08x", key, keycode );
	#endif
				r->pKeyProc( r->dwKeyData, keycode );
			}
			#if 0
			if (wl.utfKeyCode) {
				if (wl.utfKeyCode >= 0x21 && wl.utfKeyCode <= 0x7E) {
					lprintf ("the key %c was pressed", (char)wl.utfKeyCode);
					//if (utf32 == 'q') running = 0;
				}
				else {
					lprintf ("the key U+%04X was pressed", wl.utfKeyCode);
				}
			}
			else {
				char name[64];
				xkb_keysym_get_name (keysym, name, 64);
				lprintf ("the key %s was pressed", name);
			}
			#endif
		}
	}
	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		uint32_t keycode;
		// auto release any previous key.  There's no repeat, and
		// key down won't retrigger after a release of a different key...
		// A S D  and then release A doesn't matter.
		INDEX idx;
		struct pendingKey *check;
		LIST_FORALL( wl.keyRepeat.pendingKeys, idx, struct pendingKey*, check ) {
			if( check->rawKey == key ){
				keycode = check->key;
				xkb_state_update_key( wl.xkb_state, check->rawKey, XKB_KEY_UP );
				SetLink( &wl.keyRepeat.pendingKeys, idx, NULL );
				Release( check );
				break;
			}
		}
		if( !check ) {
			lprintf( "A release happened without a press??" );
			keycode = 0;
			keycode |= ( key << 16 );
			keycode |= key;
		}
		wl.key_.keycode = keycode & 0x7FFFFFF;
		EnqueData( &wl.pdqKeyEvents, &wl.key_ );
		WakeThread( wl.drawThread ); // let it setup a shorter timer, and handle stopping repeats?
	}
}


static void keyboard_modifiers(void *data,
			  struct wl_keyboard *wl_keyboard,
			  uint32_t serial,
			  uint32_t mods_depressed,
			  uint32_t mods_latched,
			  uint32_t mods_locked,
			  uint32_t group)
{
	wl.keyMods = 0;
	if( mods_depressed & 1 ) {
		wl.keyMods |= KEY_SHIFT_DOWN;
	}
	if( mods_depressed & 4 ) {
		wl.keyMods |= KEY_CONTROL_DOWN;
	}
	if( mods_depressed & 8 ) {
		wl.keyMods |= KEY_ALT_DOWN;
	}
	// I get an initial state of nil,0,0,0,N
	//lprintf( "key MOD: %p %d %x %x %d",  mods_depressed, mods_latched, mods_locked, group );
	xkb_state_update_mask (wl.xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);


}

static const TEXTCHAR * sack_wayland_GetKeyText           ( int key ){
	static char buf[6];
	//lprintf( "assuming key was the one dispatched....");
	ConvertToUTF8( buf, wl.key_sent.utfKeyCode );
	return buf;
}


static void keyboard_repeat_info(void *data,
			    struct wl_keyboard *wl_keyboard,
			    int32_t rate,
			    int32_t delay){
					 //lprintf( "Why do I care about the repeat? %d %d", rate, delay );
					 wl.keyRepeat.repeat = 1000/rate;
					 wl.keyRepeat.delay = delay;
				 }


static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};


static void xdg_surface_configure ( void *data, struct xdg_surface* xdg_surface, uint32_t serial ){
	PXPANEL r= (PXPANEL)data;
	r->last_xdg_serial = serial;
#ifdef DEBUG_SURFACE_INIT
	lprintf( "xdg_surface_ack_configure %p", xdg_surface );
#endif	
	xdg_surface_ack_configure( xdg_surface, serial );

}

static struct xdg_surface_listener const xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

typedef enum WaylandWindowState {

    TOPLEVEL_STATE_NONE = 0,
    TOPLEVEL_STATE_MAXIMIZED = 1,
    TOPLEVEL_STATE_FULLSCREEN = 2,
    TOPLEVEL_STATE_RESIZING = 4,
    TOPLEVEL_STATE_ACTIVATED = 8,
    TOPLEVEL_STATE_TILED_LEFT = 16,
    TOPLEVEL_STATE_TILED_RIGHT = 32,
    TOPLEVEL_STATE_TILED_TOP = 64,
    TOPLEVEL_STATE_TILED_BOTTOM = 128,
    TOPLEVEL_STATE_SUSPENDED = 256,
} WaylandWindowState;

static void xdgTopLeveLHandleConfigure(void* data,
                                       struct xdg_toplevel* toplevel,
                                       int32_t width,
                                       int32_t height,
                                       struct wl_array* states)
{
	PXPANEL r = (PXPANEL)data;
	// _GLFWwindow* window = data;
    float aspectRatio;
    float targetRatio;
    enum xdg_toplevel_state* state;
    uint32_t new_states = 0;
	wl_array_for_each(state, states) {
		  switch (state[0]) {
#define C(x) case XDG_##x: new_states |= x; /*lprintf("%s ", #x);*/ break
            C(TOPLEVEL_STATE_RESIZING);
            C(TOPLEVEL_STATE_MAXIMIZED);
            C(TOPLEVEL_STATE_FULLSCREEN);
            C(TOPLEVEL_STATE_ACTIVATED);
            C(TOPLEVEL_STATE_TILED_LEFT);
            C(TOPLEVEL_STATE_TILED_RIGHT);
            C(TOPLEVEL_STATE_TILED_TOP);
            C(TOPLEVEL_STATE_TILED_BOTTOM);
#ifdef XDG_TOPLEVEL_STATE_SUSPENDED_SINCE_VERSION
            C(TOPLEVEL_STATE_SUSPENDED);
#endif
#undef C
		  }
	}
	//lprintf( "xdg_toplevel_configure...%08x", new_states );

	if( new_states == 0 ){
		if( !r->freeBuffer[r->curBuffer] && r->buffer_images[r->curBuffer]) {
			postFocusEvent( r, FALSE );
			//lprintf( "Sent lose focus?" );
		} else lprintf( "window doesn't even rally exist yet..." );
	}
	//xdg_surface_ack_configure((struct xdg_surface* )r->shell_surface, r->last_xdg_serial);
	if( new_states & TOPLEVEL_STATE_ACTIVATED ){
		//lprintf( "top level activate : Alternate focus event?");
		postFocusEvent( r, TRUE );
		/*
		if( r != wl.hVidFocused ){
			if( wl.hVidFocused && wl.hVidFocused->pLoseFocus )
				wl.hVidFocused->pLoseFocus( wl.hVidFocused->dwLoseFocus, (PRENDERER)r );
			wl.hVidFocused = r;
			lprintf( "Send focus as if key enter...");
			if( r && r->pLoseFocus )
				r->pLoseFocus( r->dwLoseFocus, NULL );
		}
		*/
	}
	if (new_states & TOPLEVEL_STATE_RESIZING) {
		postSizeEvent( r, width, height );
		if (width) r->bufw = width;
		if (height) r->bufh = height;
		//lprintf( "xdg_toplevel_configure...%d %d  %d %d", width, height, r->flags.canCommit, !r->flags.dirty);
		/*
		if( width && height ) {

			lprintf( "xdg_toplevel_configure...%d %d  %d %d", width, height, r->flags.canCommit, !r->flags.dirty);
			//if (!(r->wl.current.toplevel_states & TOPLEVEL_STATE_RESIZING)) 
			//	;//report_live_resize(window, true);
			if( r->flags.canCommit && !r->flags.dirty){
				lprintf( "Surface didn't want to draw already, swap buffer, and then do draw callback");
				EnterCriticalSec( &wl.cs_wl );
				attachNewBuffer( r, 0, 0 );
				LeaveCriticalSec( &wl.cs_wl );
			}
			sack_wayland_Redraw_( r, 0, NULL );
		}
		*/
    }
    if (width != 0 && height != 0)
    {
        //if (!(new_states & TOPLEVEL_STATE_DOCKED))
        {
			#if 0
            if (window->numer != GLFW_DONT_CARE && window->denom != GLFW_DONT_CARE)
            {
                aspectRatio = (float)width / (float)height;
                targetRatio = (float)window->numer / (float)window->denom;
                if (aspectRatio < targetRatio)
                    height = (int32_t)((float)width / targetRatio);
                else if (aspectRatio > targetRatio)
                    width = (int32_t)((float)height * targetRatio);
            }
			#endif
        }
    }

    //window->wl.pending.toplevel_states = new_states;
    //window->wl.pending.width = width;
    //window->wl.pending.height = height;
    //window->wl.pending_state |= PENDING_STATE_TOPLEVEL;
}

static void xdgTopLevelHandleClose(void* data,
                                   struct xdg_toplevel* toplevel){
	
}

static struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdgTopLeveLHandleConfigure,
	xdgTopLevelHandleClose
};

static void xdg_wm_base_ping( void*data, struct xdg_wm_base*base, uint32_t serial){
#ifdef DEBUG_SURFACE_INIT	
	lprintf( "xdg_wm_base_ping pong...");
#endif
	xdg_wm_base_pong( base,serial);
}
static struct xdg_wm_base_listener const xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};


static void
global_registry_handler( void *data, struct wl_registry *registry, uint32_t id
                       , const char *interface, uint32_t version )
{
	static volatile int processing;
	while( processing ) Relinquish();
	processing = 1;

	int n;
	//lprintf( "Registry handler:%s %d", interface, version );
	for( n = 0; n < sizeof( interfaces )/sizeof(interfaces[0] ); n++ ){
		if( strcmp( interface, interfaces[n].name ) == 0 ){
			//lprintf( "Is: %s %s ", interface, interfaces[n]);
			wl.versions[n] = version;
			if( version < interfaces[n].version ){
				lprintf( "Interface %d:%s is version %d and library expects %d",
				      n, interfaces[n].name, version, interfaces[n].version );
			}
			if( interfaces[n].version < version ){
				lprintf( "Interface %d:%s is version %d and library only supports %d",
				      n, interfaces[n].name, version, interfaces[n].version );
				version = interfaces[n].version;
			}

			break;
		}
	}

	//lprintf("Got a registry event for %s id %d %d", interface, id, n);
	if( n == wis_compositor ) {
		wl.compositor = wl_registry_bind(registry,
				      id,
				      &wl_compositor_interface,
				      version);
	} else if( n == wis_subcompositor ) {
		wl.subcompositor = wl_registry_bind(registry,
				      id,
				      &wl_subcompositor_interface,
				      version);
	//} else if( n == wis_zwp_linux_dmabuf_v1] ) {
		//lprintf( "Well, fancy that,we get DMA buffers?");
		// can video fullscreen
	} else if( n == wis_shell  ) {
		wl.shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, version);
#ifdef ALLOW_KDE
	} else if( n == wis_kde_shell ) {
		wl.shell = wl_registry_bind(registry, id,
                                 &org_kde_plasma_shell_interface, version);
#endif
	} else if( n == wis_xdg_shell ) {
       //wl.xdg_shell = wl_registry_bind(registry, id,
       //                          &, version);
	} else if( n == wis_xdg_base ) {

		wl.xdg_wm_base = wl_registry_bind(registry, id
		                                 , &xdg_wm_base_interface, version);
		wl.xdg_wm_base = wl_registry_bind(registry, id
		                                 , &xdg_wm_base_interface, version);
		xdg_wm_base_add_listener( wl.xdg_wm_base, &xdg_wm_base_listener, NULL );
		wl_proxy_set_queue( (struct wl_proxy*)wl.xdg_wm_base, wl.queue );

	} else if( n == wis_seat ) {
		wl.seat = wl_registry_bind( registry, id
		                          , &wl_seat_interface,  version>2?version:version);
		wl_proxy_set_queue( (struct wl_proxy*)wl.seat, wl.queue );
		wl.pointer = wl_seat_get_pointer(wl.seat);
		wl_proxy_set_queue( (struct wl_proxy*)wl.pointer, wl.queue );
		wl.pointer_data.surface = wl_compositor_create_surface(wl.compositor);
		lprintf( "----- Setup Pointer Listener....");
		wl_pointer_add_listener(wl.pointer, &pointer_listener, &wl.pointer_data);
		wl.keyboard = wl_seat_get_keyboard(wl.seat );
		wl_proxy_set_queue( (struct wl_proxy*)wl.keyboard, wl.queue );
		wl_keyboard_add_listener(wl.keyboard, &keyboard_listener, NULL );
	} else if( n == wis_shm ) {
		wl.shm = wl_registry_bind(registry, id
		                         , &wl_shm_interface, version);
		wl_shm_add_listener(wl.shm, &shm_listener, NULL);
		wl_proxy_set_queue( (struct wl_proxy*)wl.shm, wl.queue );
	} else if( n == wis_output ) {
		struct output_data *out = New( struct output_data );
		//lprintf( "Display");
		out->wl_output = wl_registry_bind( registry, id, &wl_output_interface, version );
		// pending scaling
		out->pending_scale = 1;
		wl_proxy_set_queue( (struct wl_proxy*)out->wl_output, wl.queue );
		wl_output_set_user_data( out->wl_output, out );
		wl_output_add_listener( out->wl_output, &output_listener, out );
		AddLink( &wl.outputSurfaces, out );

	}
	processing = 0;
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


static const struct wl_interface interface_listener = {

};

static void initConnections( void ) {
	InitializeCriticalSec( &wl.cs_wl );
	wl.pii = GetImageInterface();
	wl.pdqMouseEvents = CreateDataQueue( sizeof( struct mouseEvent ));
	wl.pdqKeyEvents = CreateDataQueue( sizeof( struct keyEvent ));
	wl.display = wl_display_connect(NULL);
	if (wl.display == NULL) {
		lprintf( "Display connect failed: %m" );
		wl.flags.bFailed = 1;
		//lprintf( "Can't connect to display");
		return;
	}
	wl.queue = wl_display_create_queue( wl.display );
	//wl_proxy_create( NULL, )
	wl.flags.bInited = 1;

	interfaces[wis_xdg_base].name = xdg_wm_base_interface.name;

}


static void commitSurface( PXPANEL r ){
	struct damageInfo damage;
	if( r->flags.commited ) {
		//lprintf( "Don't do another damage and commit until released...");
		return;
	}
	while( DequeData( &r->damageQueue, &damage )){
		lprintf( "Dispatching some damage to a surface...");
		wl_surface_damage( r->surface, damage.x, damage.y, damage.w, damage.h );
		r->flags.dirty = 1; // was damaged, needs commit.
	}

	wl_surface_commit( r->surface );
	wl_display_flush( wl.display );
	//lprintf( "Committed surface propr... (commit, wantAttach)" );
	r->flags.commited = 1;
	//lprintf( "display_flush after surface_commit and maybe damage? %d", r->flags.dirty);
	// no more damage, but we need a buffer too...
	r->flags.dirty = 0;
	//lprintf( "Needs to be replaced now... dont 'need' attach until a damage...");
	r->flags.wantAttach = 1;  // I do want a new attach.
}

static void finishInitConnections( void ) {
	wl.xkb_context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);
	wl.registering = 1;
	struct wl_registry *registry = wl_display_get_registry(wl.display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_proxy_set_queue( (struct wl_proxy*)registry, wl.queue );

	wl_display_roundtrip_queue(wl.display, wl.queue);

	while( wl.registering )Relinquish(); // wait one more time, the last prior thing might take a bit.

	wl_registry_destroy(registry);
	//lprintf( "and we finish with %p %d", wl.compositor, wl.canDraw );

#if 0
	/// dump all the versions found, so supportedVersions can be udpated.
	{
		int n;
		for( n = 0; n < sizeof( interfaces )/sizeof(interfaces[0] ); n++ ){
			lprintf( "interface %d %s v %d", n, interfaces[n], wl.versions[n] );
		}
	}
#endif
	if( !wl.shell&& !wl.xdg_wm_base ){
		lprintf( "Can't find wl_shell.");
		DebugBreak();
	}
	if (!wl.compositor || !wl.canDraw ) {
		DebugBreak();
		lprintf( "Can't find compositor or no supported pixel format found.");
		return;
	} else {
		//lprintf( "Found compositor");
		
		// load a cursor
		wl.cursor_theme = wl_cursor_theme_load( NULL, 24, wl.shm );
		if( wl.cursor_theme ) {
			struct wl_cursor *cursor = wl_cursor_theme_get_cursor( wl.cursor_theme, "left_ptr");
			wl.cursor_image = cursor->images[0];
			struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer( wl.cursor_image );
			wl.cursor_surface = wl_compositor_create_surface( wl.compositor );
#if defined( DEBUG_SURFACE_ATTACH )
			lprintf( "Cursor surface attach." );
#endif
			wl_surface_attach( wl.cursor_surface, cursor_buffer, 0, 0 );
#if defined( DEBUG_SURFACE_ATTACH )
			lprintf( "Commiting cursor attach." );
#endif
			wl_surface_commit( wl.cursor_surface );
			wl_display_flush( wl.display );
		} else lprintf( "Naive theme check failed..." );
		//wl_display_flush(wl.display);
		//wl_display_roundtrip_queue(wl.display, wl.queue);

	}
}



static int
set_cloexec_or_close(int fd)
{
        long flags;

        if (fd == -1)
                return -1;

        flags = fcntl(fd, F_GETFD);
        if (flags == -1)
                goto err;

        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
                goto err;

        return fd;

err:
        close(fd);
        return -1;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
        int fd;

#ifdef HAVE_MKOSTEMP
        fd = mkostemp(tmpname, O_CLOEXEC);
        if (fd >= 0)
                unlink(tmpname);
#else
        fd = mkstemp(tmpname);
        if (fd >= 0) {
                fd = set_cloexec_or_close(fd);
                unlink(tmpname);
        }
#endif

        return fd;
}
/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 */
int os_create_anonymous_file(off_t size)
{
        static const char template[] = "/weston-shared-XXXXXX";
        const char *path;
        char *name;
        int fd;

        path = getenv("XDG_RUNTIME_DIR");
        if (!path) {
                errno = ENOENT;
                return -1;
        }

        name = malloc(strlen(path) + sizeof(template));
        if (!name)
                return -1;
        strcpy(name, path);
        strcat(name, template);

        fd = create_tmpfile_cloexec(name);
		//lprintf( "create temp file...%s", name );
        free(name);

        if (fd < 0)
                return -1;

        if (ftruncate(fd, size) < 0) {
                close(fd);
                return -1;
        }

        return fd;
}


static void surfaceFrameCallback( void *data, struct wl_callback* callback, uint32_t time );
static const struct wl_callback_listener frame_listener = {
	surfaceFrameCallback
};

static void releaseBuffer( void*data, struct wl_buffer*wl_buffer );
static const struct wl_buffer_listener  buffer_listener = {
	.release = releaseBuffer,
};

static struct wl_buffer * allocateBuffer( PXPANEL r )
{
	int stride = r->bufw * 4; // 4 bytes per pixel
	int size = stride * r->bufh;
	int fd;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		lprintf( "creating a buffer file for %d B failed: %m\n",
			size);
		return FALSE;
	}
	r->buffer_states[r->curBuffer].size = size;
	r->buffer_states[r->curBuffer].w = r->bufw;
	r->buffer_states[r->curBuffer].h = r->bufh;
	r->buffer_states[r->curBuffer].dirty = 1;
	

	r->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (r->shm_data == MAP_FAILED) {
		lprintf( "mmap failed: %m\n");
		close(fd);
		return FALSE;
	}

	//static struct wl_shm_pool *pool; if( !pool ) pool = wl_shm_create_pool(wl.shm, fd, size);
	struct wl_shm_pool *pool = wl_shm_create_pool(wl.shm, fd, size);
#ifdef DEBUG_COMMIT_BUFFER_DETAILS	
	lprintf( "Allocating a buffer: %d %d %d", r->bufw, r->bufh, r->bufw*r->bufh*4);
#endif	
	r->buff = wl_shm_pool_create_buffer(pool, 0, /* starting offset */
					r->bufw, r->bufh,
					stride,
					WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener( r->buff, &buffer_listener, r );
	//lprintf( "created a pool and got a buffer, destroy pool now..." );
	//wl_shm_pool_destroy(pool);

	return r->buff;
}

static void clearBuffer( PXPANEL r ) {
	//if( r->flags.hidden ) return;
	wl_surface_attach( r->surface, NULL, 0, 0);
}

static int canCommit( PXPANEL r ) {
	if( r->freeBuffer[r->curBuffer]  
		&& ( r->buffer_states[r->curBuffer].w == r->bufw 
		&& r->buffer_states[r->curBuffer].h == r->bufh ) ) {
#if defined( DEBUG_COMMIT )
		lprintf( "Can just use the current image... it's already attached %d", r->curBuffer);
#endif
		return 1;
	}
	if( r->buffers[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES]
	  && !r->freeBuffer[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES] ) {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "(can commit)buffers in use and nothing free, (no more commit!!) want buffer %d", r->curBuffer);
#endif
		return 0;
	}
	return 1; // there's a free buffer or a free image next.
}

static struct wl_buffer * nextBuffer( PXPANEL r, int attach ) {
	int priorBuffer = r->curBuffer;//?r->curBuffer-1:(MAX_OUTSTANDING_FRAMES-1);
	int nextBuffer = (r->curBuffer+1)%MAX_OUTSTANDING_FRAMES;

	if( r->freeBuffer[r->curBuffer]  
		&& ( r->buffer_states[r->curBuffer].w == r->bufw 
		&& r->buffer_states[r->curBuffer].h == r->bufh ) ) {
#if defined( DEBUG_COMMIT )
		lprintf( "Can just use the current image... it's already attached %d", r->curBuffer);
#endif
		return r->buff;
	}
	if( r->buffers[nextBuffer]
	  && !r->freeBuffer[nextBuffer] ) {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "buffers in use and nothing free, (no more commit!!) want buffer %d", r->curBuffer);
#endif
		return NULL;
	}
	r->curBuffer = nextBuffer;
	int curBuffer = r->curBuffer;
#if defined( DEBUG_SURFACE_ATTACH )
	lprintf( "using image to a new image.... %d sizeChange???",curBuffer );
#endif
	if( r->buffer_states[curBuffer].w != r->bufw 
	   || r->buffer_states[curBuffer].h != r->bufh ) {
		// if the buffer has to change size, we need a new one....
		// maybe can keep this around to fill a copy of the prior window?
		if( r->color_buffers[curBuffer] ) {
			lprintf( "Remove Old Buffers %d,%d  %d,%d", r->buffer_states[curBuffer].w, r->buffer_states[curBuffer].h, r->bufw, r->bufh );
			// one of these is in use...
			munmap( r->color_buffers[curBuffer], r->buffer_states[curBuffer].size );
			UnmakeImageFile( r->buffer_images[curBuffer] );
			if( r->buffers[curBuffer] )
				wl_buffer_destroy( r->buffers[curBuffer] );
		}

		r->buffer_images[curBuffer] = NULL;
		r->buffers[curBuffer] = NULL;
		r->color_buffers[curBuffer] = NULL;
	}

	// setup the current values to this buffer...
	r->shm_data = r->color_buffers[curBuffer];
	r->buff = r->buffers[curBuffer];

	if( !r->buff ) {
#if defined(DEBUG_COMMIT_ATTACH )
		lprintf( "Allocate NEW buffer" );
#endif		
		// haven't actually allocated a bufer yet... so do so...
		// allocate buffer updates the size of the image
		allocateBuffer(r);
		r->buffer_images[curBuffer] = RemakeImage( r->buffer_images[curBuffer], r->shm_data, r->bufw, r->bufh );
		r->buffers[curBuffer] = r->buff;
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "Setting buffer (free) %d  %p", curBuffer, r->buff );
#endif
		r->color_buffers[curBuffer] = r->shm_data;
		r->freeBuffer[curBuffer] = 1; // initialize this as 'free' as in not commited(in use)
	}
	
	{
		// update previous frame to current frame.
#if defined(DEBUG_COMMIT_ATTACH_DETAILS )
		lprintf( "Copying old buffer to current buffer....%d %d", curBuffer, (curBuffer+(MAX_OUTSTANDING_FRAMES-1))%MAX_OUTSTANDING_FRAMES );
#endif
		// copy just the damaged portions?
		if( r->buffer_images[priorBuffer] )
			if( r->buffer_states[priorBuffer].w == r->bufw
			  &&  r->buffer_states[priorBuffer].h == r->bufh ){
				r->buffer_states[r->curBuffer].dirty = 0;
				BlotImage( r->buffer_images[curBuffer]
				         , r->buffer_images[priorBuffer]
				         , 0, 0 );
			}
	}
	// update image internals of renderer... 
	//lprintf( "(nextBuffer) Resizing Output image %p %p %d %d", r->pImageOutput, r->shm_data, r->bufw, r->bufh );
	r->flags.commited = 0;
	r->pImageOutput = RemakeImage( r->pImageOutput, r->shm_data, r->bufw, r->bufh );

	if( !r->pImageDraw || r->pImageDraw->width != r->bufw || r->pImageDraw->height != r->bufh ) {
		//lprintf( "This is making a draw image too - can't we just use the old one?");
		Image tmp = r->pImageDraw;//UnmakeImageFile( r->pImageDraw );
		r->pImageDraw = MakeImageFile( r->bufw, r->bufh );
		if( tmp ) {
			BlotImageSizedTo( r->pImageDraw, tmp, 0, 0, 0, 0, r->bufw, r->bufh );
			UnmakeImageFile( tmp );
		}
		r->pImageOutput->flags |= IF_FLAG_FINAL_RENDER|IF_FLAG_IN_MEMORY;
	}
	return r->buff;
}


static LOGICAL attachNewBuffer( PXPANEL r, int req, int locked ) {
#if defined( DEBUG_SURFACE_ATTACH )
	lprintf( "attachNewBuffer... req %d,  locked %d", req, locked);
#endif	
	if( !r->surface ) return FALSE; 
#ifdef DEBUG_DIRTY_DRAW
	lprintf( "attachNewBuffer2... (have a surface) %d %p", r->freeBuffer[r->curBuffer], r->color_buffers[r->curBuffer] );
#endif	
	if( !r->freeBuffer[r->curBuffer] && r->color_buffers[r->curBuffer]) {
		if( r->flags.dirty ) {
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "existing damage... (flush was needed after commit)");
#endif			
			commitSurface( r );
		} else {
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "(not dirty, no commit) NOT Attaching over a buffer and there was no damage to it? %d",r->freeBuffer[r->curBuffer] );
#endif		
			return FALSE;
		}
		// any new draws are going to 
	}
	//else lprintf( "surface cannot have damage nothing there");

	//lprintf( "Attach New Buffer should Always have 'Want Attach!?' %d", r->flags.wantAttach );
	if( !r->flags.wantAttach ) return TRUE; // it IS attached

	struct wl_buffer *next = nextBuffer(r, 0);
	//lprintf( "NextBuffer %p", next );
	if( req && !next ) {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "waiting for buffer...----------------%d %d", canCommit( r ), r->flags.canCommit );
#endif
		if( canCommit( r ) ) 
		{
			r->flags.canCommit = 0; // am commiting, do not commit
			//r->flags.dirty = 0;
			//r->flags.commited = 1; // unused?
			//r->flags.dirty = 0; // 
			r->flags.canDamage = 1; // still have existing buffer, collect damage
			r->flags.wantBuffer = 1;
			r->bufferWaiter = MakeThread();
#if defined( DEBUG_COMMIT_ATTACH )
			lprintf( "Commiting my surface, flushing, and waiting for new surface" );
#endif
#if defined( DEBUG_DUMP_SURFACE_IMAGES )
			{
				uint8_t *buf; size_t len;
				PngImageFile( r->pImage, &buf, &len );
				char name[12];
				snprintf( name, 12, "im-%d.png", wl.frame++ );
				FILE *out= fopen( name, "wb" );
				fwrite( buf, 1, len, out );
				fclose( out );
				Release( buf );
			}
#endif
			do {
//#if defined( DEBUG_COMMIT_BUFFER )
				lprintf( "to loop around, waiting for hidden or a next buffer available..." );
//#endif
				if( locked )
					LeaveCriticalSec( &wl.cs_wl );
				if( r->bufferWaiter == wl.waylandThread ) {
					lprintf( "Is the wayland thread, not the draw thread?" );
					wl_display_roundtrip_queue(wl.display, wl.queue);
				}
				while( r->flags.wantBuffer ) {
					//lprintf( "SLEEP?" ); 
					IdleFor( 250 ); 
					//lprintf( "Slept?%d", r->flags.wantBuffer );
				} 
				if( locked )
					EnterCriticalSec( &wl.cs_wl );
				lprintf( "AttachNewbuffer Another next buffer");
				next = nextBuffer(r,0);
//#if defined( DEBUG_COMMIT_BUFFER_DETAILS )
				lprintf( "called next buffer again %p", next );
//#endif						
			} while( !r->flags.hidden && !next );
			lprintf( "Finally have another buffer - so issue the commit");
			commitSurface( r );
			// no longer dirty
			r->buffer_states[r->curBuffer].dirty = 0;
			// now have a buffer, do not want it.
			r->bufferWaiter = NULL;
			r->flags.canCommit = 1; // was commiting, can now commit sub-updates
		}else {
			//lprintf( "Attach New Buffer is really still waiting for a new buffer !!!!!!!!!!!!!!!! ");
		}
	}
	if( next ) {
#if defined( DEBUG_DUMP_SURFACE_IMAGES )
		{
			uint8_t *buf; size_t len;
			PngImageFile( r->pImage, &buf, &len );
			char name[12];
			snprintf( name, 12, "nx-%d.png", wl.frame++ );
			FILE *out= fopen( name, "wb" );
			fwrite( buf, 1, len, out );
			fclose( out );
			Release( buf );
		}
#endif
		if( r->flags.wantBuffer ){
#if defined( DEBUG_COMMIT_BUFFER )
			lprintf( "Buffers: %d %d %p %p", r->freeBuffer[0], r->freeBuffer[1], r->buffers[0], r->buffers[1]);
			lprintf( "NEED TO WAIT FOR A BUFFER or commit won't finish with a good attached buffer... right now there is a good buffer." );
#endif
			lprintf( "Have a buffer -and want buffer, why return false??");
			return FALSE;
		}
		if( !next ) lprintf( "auto clean can't be done... backing buffer is still in use...");
		r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
		// already wants to commit again...
		// clear dirty, still can't commit.
		
		if( r->surface ) { // by this point, thse surce COULD have closed... 
#if defined( DEBUG_SURFACE_ATTACH )
			lprintf( "Attach new surface, such that can damage (wake someone?)(clears want!)" );
#endif
			r->flags.wantAttach = 0;
			wl_surface_attach( r->surface, next, 0, 0 );
			r->flags.canDamage = 1;
			return TRUE;
		}
	}else {
#ifdef DEBUG_DIRTY_DRAW		
		lprintf( "(attachNewBuffer)Leaving without attaching a buffer... (next release call this) %d %d %d", r->flags.dirty, r->flags.wantAttach, r->flags.canCommit );
#endif		
		if( r->flags.dirty )
			r->flags.wantAttach = 1;
	}
	return FALSE;
}

static  void surfaceFrameCallback( void *data, struct wl_callback* callback, uint32_t time ) {
	EnterCriticalSec( &wl.cs_wl );
	PXPANEL r = (PXPANEL)data;
	// internal usage doesn't re-add callback... was stalled on buffer
	// after allocating a callback that's still valid.
	if( wl.flags.shellInit ) {
		lprintf( "Completed Shell Init... wait for draw %p", r );
		wl.flags.shellInit = 0;
	}//= 1;

#if defined( DEBUG_COMMIT )
	lprintf( "----------- surfaceFrameCallback (after surface is freed? no...) %d %p",r->curBuffer, r->buffers[r->curBuffer] );
#endif
	if( callback && r->frame_callback ) 
		wl_callback_destroy( r->frame_callback );
		r->frame_callback = NULL;
		{
			r->flags.drawing = 1;
			if( r->pRedrawCallback )
				r->drawResult = r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r  );
			r->flags.drawing = 0;
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "dispatched redraw...%d", r->drawResult );
#endif			
			if( !r->flags.dirty && !r->flags.commited ) {
				// it wasn't a smart thing, and didn't select a sub-portion to smudge
				lprintf( "Just damage the surface here, not dirty after draw?");
				wl_surface_damage( r->surface, 0, 0, r->bufw, r->bufh );
				r->flags.dirty = 1; // was damaged, needs commit.
			}
		}
#if defined( DEBUG_REDRAW )		
		lprintf( "surface frame callback... Dirty? before atttach %d", r->flags.dirty );
#endif		
		// commit any old damage, and get a new, fresh drawable buffer.
		if( !attachNewBuffer( r, 1, 1 ) ) {
			//lprintf( "Required and locked failed to attach ------");
		}else
			r->flags.canCommit = 1; // allow sub-damages

	/*
	r->frame_callback = wl_surface_frame( r->surface );
	wl_callback_add_listener( r->frame_callback, &frame_listener, r );
	*/
	LeaveCriticalSec( &wl.cs_wl );

}

static void postDirt( PXPANEL r, int x, int y, int w, int h ) {
	struct damageInfo damage;
	damage.x = x;
	damage.y = y;
	damage.w = w;
	damage.h = h;
	EnqueData( &r->damageQueue, &damage );
}

static void postDrawEvent( PXPANEL r, int noCallback ) {
	INDEX idx;
	struct drawRequest * check;
	if( r->flags.hidden ) {
		return;
	}
	LIST_FORALL( wl.wantDraw, idx, struct drawRequest*, check ){
		if( check->r == r ) {
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "draw is already queued... updating: %d %d", check->noCallback, noCallback );
#endif			
			// turn off the 1 if there isn't a block for the callback
			if( !noCallback ) check->noCallback = 0;
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "redrawState: %d", redrawState);
#endif			
			WakeThread( wl.drawThread );
			break;
		}
	}
	if( !check ) {
		struct drawRequest* req = New( struct drawRequest );
		req->r = r;
		req->noCallback = noCallback;
		req->setFocus = 0;
		req->setSize = 0;
		//req->thread = thisThread;
		AddLink( &wl.wantDraw, req );
#if defined( DEBUG_COMMIT )
		lprintf( "^^^^^^^^^^^ draw waking draw thread, and resulting ^^^^^^^^^^^^^^^^");
#endif
		WakeThread( wl.drawThread );
	}
}

static void postFocusEvent( PXPANEL r, LOGICAL focus ) {
	INDEX idx;
	struct drawRequest * check;
	if( r->flags.hidden ) {
		return;
	}
	LIST_FORALL( wl.wantDraw, idx, struct drawRequest*, check ){
		if( check->r == r ) {
			check->setFocus = 1;
			check->hasFocus = focus;
			//lprintf( "Adding focus to existing draw event?");
			break;
		}
	}
	if( !check ) {
		struct drawRequest* req = New( struct drawRequest );
		req->r = r;
		req->noCallback = 1;
		req->setSize = 0;
		req->setFocus = 1;
		req->hasFocus = focus;
		//req->thread = thisThread;
		AddLink( &wl.wantDraw, req );
#if defined( DEBUG_COMMIT )
		lprintf( "^^^^^^^^^^^ focus waking draw thread, and resulting ^^^^^^^^^^^^^^^^");
#endif
		WakeThread( wl.drawThread );
	}
}


static void postSizeEvent( PXPANEL r, uint32_t w, uint32_t h ) {
	INDEX idx;
	struct drawRequest * check;
	if( r->flags.hidden ) {
		return;
	}
	LIST_FORALL( wl.wantDraw, idx, struct drawRequest*, check ){
		if( check->r == r ) {
			check->setSize = 1;
			check->noCallback = 0;
			check->w = w;
			check->h = h;
			break;
		}
	}
	if( !check ) {
		struct drawRequest* req = New( struct drawRequest );
		req->r = r;
		req->noCallback = 0;
		req->setFocus = 0;
		req->setSize = 1;
		req->w = w;
		req->h = h;
		//req->thread = thisThread;
		AddLink( &wl.wantDraw, req );
//#if defined( DEBUG_COMMIT )
		lprintf( "^^^^^^^^^^^ size waking draw thread, and resulting ^^^^^^^^^^^^^^^^");
//#endif
		WakeThread( wl.drawThread );
	}
}

static int do_waylandDrawThread( uintptr_t psv ) {
	static volatile int sleeping = 0;
	static volatile int drawing = 0;
	static volatile int sleepTime = 0;
	int drewSome = 0;
	// need draws to happen on a 'draw' thread, which is NOT the application thread.
	//
	PTHREAD checkThread = MakeThread();
	if( checkThread == wl.drawThread )
	{
		int newSleepTime = -1;
		INDEX idx;
		int did_event = 0;
		struct drawRequest *req;
		struct pendingKey *key;
		//PXPANEL r;
		uint32_t now = timeGetTime();
		struct mouseEvent event;
		while( DequeData( &wl.pdqKeyEvents, &wl.key_sent )){
			redrawState = 100;
			did_event = 1;
			if( !wl_HandleKeyEvents( wl.key_sent.r->pKeyDefs, wl.key_sent.keycode ) ) {
				if( wl.key_sent.r->pKeyProc ){
		#ifdef DEBUG_KEY_EVENTS
					lprintf( "2) Sending as key %08x %08x", wl.key_sent.utfKeyCode, wl.key_sent.keycode );
		#endif
					wl.key_sent.r->pKeyProc( wl.key_sent.r->dwKeyData, wl.key_sent.keycode );
				}
				#if 0
				if (wl.utfKeyCode) {
					if (wl.utfKeyCode >= 0x21 && wl.utfKeyCode <= 0x7E) {
						lprintf ("the key %c was pressed", (char)wl.utfKeyCode);
						//if (utf32 == 'q') running = 0;
					}
					else {
						lprintf ("the key U+%04X was pressed", wl.utfKeyCode);
					}
				}
				else {
					char name[64];
					xkb_keysym_get_name (keysym, name, 64);
					lprintf ("the key %s was pressed", name);
				}
				#endif
			}
			redrawState = 101;
		}
		sleepTime = 500000;
		while( DequeData( &wl.pdqMouseEvents, &event ) ) {
			redrawState = 200;
			did_event = 1;
			if( wl.hCaptured ) {
				/*
					struct wvideo_tag *r = wl.hCaptured;
					while( r && r->above ){
						lprintf( "adjust mouse event by %d %d   %d %d", r->x, r->y, event.x, event.y );
						//event.x -= r->x; // root x/y is not reliable... we're sourced to that parent.
						//event.y -= r->y;
						r = r->above;
					}
					*/
					wl.hCaptured->mouse( wl.hCaptured->psvMouse, event.x, event.y, event.b );

			}else {
				if (event.r != NULL && event.r->mouse ) {
					event.r->mouse( event.r->psvMouse, event.x, event.y, event.b );
				}
			}
			if( event.r->flags.dirty ) {
				//postDrawEvent( event.r );
			}
			sleepTime = 0;
			redrawState = 201;
		}
		if( sleepTime ) { // if mouse happened, ignore keys this pass...
			//lprintf( "Consult repeating keys.... " );
			LIST_FORALL( wl.keyRepeat.pendingKeys, idx, struct pendingKey*, key ) {
				if( key->tick ) {
					//lprintf( "key tick:%fd %d", key->tick, key->repeating, now, key->tick-now );
					if( !key->repeating ) {
						if( ( now > key->tick )
							&& (now - key->tick) > wl.keyRepeat.delay ){
							key->tick += wl.keyRepeat.delay;
							key->r->pKeyProc( key->r->dwKeyData, key->key );
							key->repeating = 1;
						}
						newSleepTime = wl.keyRepeat.delay - (key->tick - now) ;
					}
					if( key->repeating ) {
						while( ( now > key->tick )
							&& (now - key->tick) > wl.keyRepeat.repeat ){
							key->r->pKeyProc( key->r->dwKeyData, key->key );
							key->tick += wl.keyRepeat.repeat;
						}
						newSleepTime = wl.keyRepeat.repeat - (key->tick - now) ;
					}
				}
			}
			if( newSleepTime >= 0 && newSleepTime < sleepTime )
				sleepTime = newSleepTime;
			//lprintf( "Set sleep time? %d", sleepTime );
		}
		
		if( !drawing ) {
			drawing = 1;
			LIST_FORALL( wl.wantDraw, idx, struct drawRequest *, req ){
				drewSome = 1;
				int oldCallback = req->noCallback;

				if( req->setSize ) {
					req->r->bufw = req->w;
					req->r->bufh = req->h;
					//lprintf( "Set Size request in wayland draw thread...");
				}
				if( req->setFocus ) {
					//lprintf( "Set Focus in draw event... %d", req->hasFocus );
					if( req->hasFocus ) {
						if( wl.hVidFocused && wl.hVidFocused->pLoseFocus )
							wl.hVidFocused->pLoseFocus( wl.hVidFocused->dwLoseFocus, (PRENDERER)req->r );
						wl.hVidFocused = req->r;
						if( req->r && req->r->pLoseFocus )
							req->r->pLoseFocus( req->r->dwLoseFocus, NULL );
					} else {
						if( wl.hVidFocused && wl.hVidFocused->pLoseFocus ){
							//lprintf( "on leave lose focus?");
							wl.hVidFocused->pLoseFocus( wl.hVidFocused->dwLoseFocus, (PRENDERER)1 );
						}
						wl.hVidFocused = NULL;
					}
				}

				SetLink( &wl.wantDraw, idx, NULL ); // remove it from the list... 
#if defined( DEBUG_COMMIT ) || defined( DEBUG_REDRAW )
				lprintf( " ------------------------  cleared want draw state...");
#endif
				redrawState = 300;
				sack_wayland_Redraw_( req->r, req->noCallback, &redrawState );
				// assume the above took some time...
				if( oldCallback && !req->noCallback ) {
					// the drawer found the request before it was removed
					// AND they have added a full redraw
					redrawState = 302;
#if defined( DEBUG_REDRAW )
					lprintf( " --------- And we do a fuller draw");
#endif
					sack_wayland_Redraw_( req->r, req->noCallback, &redrawState );
				}
				redrawState = 301;
#if defined( DEBUG_REDRAW )
				lprintf( " ------------------------  how does ready state not become 301?" );
#endif				
				req->r = NULL; // clear draw request.
				//WakeThread( req->thread );
				Release( req );
			}
			drawing = 0;
		}
#if defined( DEBUG_REDRAW )		
		 else {
			lprintf( "Draw is already dispatched ");
		}
#endif		
		if( sleepTime > 0 ) // mouse events might want to tick off quickly
		{
			if( wl.display ) {
				//lprintf( "---------  Send wl_display_flush....");
				redrawState = 10;
				//wl_display_flush( wl.display );
				redrawState = 11;
			}
			//lprintf( "Sleeping" );
			if( sleeping ) {
				//lprintf( "already sleeping just return (idle callback)");
				return 0;
			} else {
				redrawState = 12;
							
				sleeping = TRUE;
				IdleFor(sleepTime);
				sleeping = FALSE;
				redrawState = 13;
			}
			//lprintf( "not sleeping(slept) here... %d", psv );
			return 1;
		} else {
			if( drewSome ) {
#if defined( DEBUG_REDRAW )		
				lprintf( "don't sleep here... %d", psv );
#endif				
				return 1;
			}
			//lprintf( "sleeping here... %d", psv );
			return 0;
		}
	} else {
		//lprintf( "Idle call?  bad thread?");
		return -1;
	}
	lprintf( "Return to sleep?");
	return 0;
}

static uintptr_t waylandDrawThread( PTHREAD thread ) {
	// need draws to happen on a 'draw' thread, which is NOT the application thread.
	//
	lprintf( "!!!!!!!!!! START DRAW THREAD" );
	wl.drawThread = thread;
	while(1)
	{
		if( do_waylandDrawThread(0) == 0 ) {
			WakeableSleep(SLEEP_FOREVER);
		}
	}
}


static int do_waylandThread( uintptr_t isThread ) {
	if( MakeThread() != wl.waylandThread ) return -1;
	if( wl.display ) {
		lprintf( " --- Sending pending? ");
		wl_display_dispatch_queue_pending ( wl.display, wl.queue );
		//wl_display_dispatch_queue(wl.display, wl.queue);
	}
	return 0;
}
static uintptr_t waylandThread( PTHREAD thread ) {
	wl.waylandThread = thread;
	if( wl.display) DebugBreak();
	lprintf( "!!!!!!!!!! START WAYLAND THREAD" );
	initConnections();
	if( wl.display ) {
		while( wl_display_dispatch_queue(wl.display, wl.queue) != -1 ){
			PTHREAD *ppWaiter; INDEX idx;
			LIST_FORALL( wl.shellWaits, idx, PTHREAD*, ppWaiter ) {
				PTHREAD waiter = ppWaiter[0]; ppWaiter[0] = NULL;
				SetLink( &wl.shellWaits, idx, NULL );
				WakeThread( waiter );
			}
			//lprintf( ".... did some messages...");
		}

		lprintf( "!*!*!*!*!*! Thread exiting? !*!*!*!*!*!" );
		//wl_event_queue_destroy( wl.queue );
		wl_display_disconnect( wl.display );
	} else {
		lprintf( "Failed to connect to wayland display" );
	}
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
	INDEX idx;
	if( !display ) {
		// default to this monitor...
		display = 1;
	}
	display--;
	if( !x ) x = &j1;
	if( !y ) y = &j1;
	if( !w ) w = &j2;
	if( !h ) h = &j2;
	 struct output_data* output = GetLink( &wl.outputSurfaces,display );
	 if( output ) {
	 	x[0] = output->x;
	 	y[0] = output->y;
	 	w[0] = output->w;
	 	h[0] = output->h;
	 }else {
	 	x[0] = y[0] = w[0] = h[0] = 0;
	 }
}


static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
							uint32_t serial)
{
	//PXPANEL r = (PXPANEL)data;
   wl_shell_surface_pong(shell_surface, serial);
   //lprintf("Pinged and ponged");
}


static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
		 uint32_t edges, int32_t width, int32_t height)
{
	PXPANEL r = (PXPANEL)data;

	postSizeEvent( r, r->bufw, r->bufh );
	lprintf( "psot size event (should result in something)");
	//wl_surface_commit( r->surface );
	//r->changedEdge = edges;
	lprintf( "(old?) shell configure %d %d", width, height );
	if( edges & wrsdv_left ){

	}
	if( edges & wrsdv_right ){

	}
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
	PXPANEL r = (PXPANEL)data;
	lprintf( "shell exit");
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done,
};




void releaseBuffer( void*data, struct wl_buffer*wl_buffer ){
	EnterCriticalSec( &wl.cs_wl );
	PXPANEL r = (PXPANEL)data;
	int n;
#if defined( DEBUG_COMMIT_BUFFER )
	lprintf( "------------------------ RELEASE BUFFER ---------------------------- %p %p", data, wl_buffer );
#endif
	r->flags.commited = 0;
	r->flags.wantAttach = 1;

	for( n = 0; n < MAX_OUTSTANDING_FRAMES; n++ )  {
#if defined( DEBUG_COMMIT_BUFFER_DETAILS )
		lprintf( "RELEASE BUFFER FINDING BUFFER is %p %p?", r->buffers[n] , wl_buffer );
#endif
		if( r->buffers[n] == wl_buffer ) {
#if defined( DEBUG_COMMIT_BUFFER_DETAILS )
			lprintf( "UNLOCK Buffer %d %p is free again", n, r->color_buffers[n] );
#endif
			if( MAX_OUTSTANDING_FRAMES < 3 ) 
			
				if( n == ( (r->curBuffer+(MAX_OUTSTANDING_FRAMES-1))%MAX_OUTSTANDING_FRAMES ) )
				{
					INDEX idx;
					int minx = r->bufw;
					int miny = r->bufh;
					int maxx = 0;
					int maxy = 0;
					struct damageInfo *damage;
					LIST_FORALL( r->damage, idx, struct damageInfo *, damage ) {
						int newminx, newminy, newmaxx, newmaxy;
						int grew;
						lprintf( "Recovering a damaged area %d %d %d %d", damage->x, damage->y, damage->w, damage->h );

						if( damage->x < minx )
							minx = damage->x;
						if( damage->y < miny )
							miny = damage->y;

						if( (damage->x+damage->w) > maxx )
							maxx = damage->x+damage->w;
						if( (damage->y+damage->h) > maxy )
							maxy = damage->y+damage->h;

						Release( damage );
					}
					EmptyList( &r->damage );
					if( maxx  ) {
						lprintf( "update next damaged area %d %d %d %d", minx, miny, maxx-minx, maxy-miny );
						BlotImageSizedTo( r->buffer_images[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES], r->buffer_images[r->curBuffer]
									, minx, miny, minx, miny, maxx-minx, maxy-miny );
					}
				};
				
			r->freeBuffer[n] = 1;
			if( r->flags.wantBuffer ){
				lprintf( "This wakes the thread waiting for a buffer to be able to commit.");
				r->flags.wantBuffer = 0;
				if( r->bufferWaiter ) WakeThread( r->bufferWaiter );
			} else {
				// else noone wanted the buffer right now... so just let it be free?
				//lprintf( "Buffer was freed, but maybe there's no event?");
				if( r->flags.wantAttach ) {
					//lprintf( "wanted to attach but didn't want the buffer??");
					// if not required, lock state doesn't matter
					if( !attachNewBuffer( r, 0, 0 ) ) {
						//lprintf( "Still (didn't need to?)couldn't attach!? ------");

					}
				}
			}
			break;
		}
	}

	if( n == MAX_OUTSTANDING_FRAMES ) {
		lprintf( "Released buffer isn't on this surface?");
	}
#if defined( DEBUG_COMMIT_BUFFER )
	lprintf( "--- end of unlock buffer--");
#endif
	LeaveCriticalSec( &wl.cs_wl );
}

LOGICAL CreateWindowStuff(PXPANEL r, PXPANEL parent )
{
	lprintf( "-----Create WIndow Stuff----- " );
	//DebugBreak();
	EnterCriticalSec( &wl.cs_wl );

	if (r->x == -1 || r->y == -1 )
	{
		uint32_t w, h;
		GetDisplaySizeEx ( 0, NULL, NULL, &w, &h);
		if( r->x == -1 )
			r->x = w * 7 / 10;
		if( r->y == -1 )
			r->y = h * 7 / 10;
	}

	r->surface = wl_compositor_create_surface(wl.compositor);
	if (r->surface == NULL) {
		lprintf( "Can't create surface");
		LeaveCriticalSec( &wl.cs_wl );
		return FALSE;
	}
	wl_surface_set_user_data(r->surface, r);

	if( parent ) {
		r->sub_surface = wl_subcompositor_get_subsurface( wl.subcompositor, r->surface, parent->surface );
		wl_subsurface_set_user_data(r->sub_surface, r);
		wl_subsurface_set_position( r->sub_surface, r->x, r->y );
		wl_subsurface_set_desync( r->sub_surface );
		lprintf( "Created subsurface and attached it...(commit) %d %d", r->x, r->y );
		wl_surface_commit( parent->surface );
		wl_display_flush( wl.display );
	} else {
		if(wl.shell && !wl.xdg_wm_base) {
			r->shell_surface = wl_shell_get_shell_surface(wl.shell, r->surface);
			wl_shell_surface_add_listener( r->shell_surface, &shell_surface_listener, r );
			wl_shell_surface_set_toplevel(r->shell_surface);
			wl_shell_surface_set_user_data(r->shell_surface, r);
		}
		if(wl.xdg_wm_base) {
			r->shell_surface = (struct wl_shell_surface*)xdg_wm_base_get_xdg_surface( wl.xdg_wm_base, r->surface );
			xdg_surface_add_listener( (struct xdg_surface*)r->shell_surface, &xdg_surface_listener, r );
			r->xdg_toplevel = xdg_surface_get_toplevel( (struct xdg_surface*)r->shell_surface );
			//
			if( r->xdg_toplevel && r->pending_title ){
				xdg_toplevel_set_title(  r->xdg_toplevel, r->pending_title);
				ReleaseEx( r->pending_title DBG_SRC );
				r->pending_title = NULL;
			}
#ifdef DEBUG_SURFACE_INIT
			lprintf( "xdg_surface_init... %p", r->shell_surface );
#endif			

			xdg_surface_set_user_data((struct xdg_surface*)r->shell_surface, r);
			xdg_toplevel_add_listener( r->xdg_toplevel, &xdg_toplevel_listener, r);
			// must commit to get a config
//#ifdef DEBUG_SURFACE_INIT
			lprintf( "Commiting shell initalization (with flush)" );
//#endif			
			wl_surface_commit( r->surface );
			// must also wait to get config.
			wl_display_flush( wl.display );
			if( r->bufferWaiter == wl.waylandThread ) {
				lprintf( "Is the wayland thread, not the draw thread? (roundtrip)" );
				wl_display_roundtrip_queue(wl.display, wl.queue);
			} else {
				//lprintf( "This has to e called anyway ( with flush, after commit) " );
				LeaveCriticalSec( &wl.cs_wl );
				PTHREAD waiting = MakeThread();
				AddLink( &wl.shellWaits, &waiting );				
				while(waiting) IdleFor(1000);//wl_display_roundtrip_queue(wl.display, wl.queue);
				EnterCriticalSec( &wl.cs_wl );
			}
		}

		if (r->shell_surface == NULL) {
			lprintf("Can't create shell surface");
			LeaveCriticalSec( &wl.cs_wl );
			return FALSE;
		}
	}
//#ifdef DEBUG_SURFACE_INIT
	lprintf( " ---- surface frame is setup...");
//#endif	
	r->flags.canDamage = 1;
	LeaveCriticalSec( &wl.cs_wl );

	return TRUE;
}


void ShutdownVideo( void )
{
	if( wl.flags.bInited )
	{
	}
}

static void sack_wayland_Redraw( PRENDERER renderer ) {
	sack_wayland_Redraw_( (PXPANEL)renderer, 0, NULL );
}
static void sack_wayland_Redraw_( PXPANEL r, int noCallback, volatile int *redrawState_ ) {
	//static volatile int redrawState__ = -3;
	//#define redrawState ((redrawState_?redrawState_:&redrawState__)[0])
	if( r->flags.hidden ) return;
	PTHREAD thisThread = MakeThread();

	if( redrawState != 5 ) {
		//if( redrawState == 300 )DebugBreak();
#ifdef DEBUG_REDRAW
		lprintf( "Redraw dispatched to display...%d  %d", redrawState, noCallback );
#endif
	} else {
		// this is just the application being generous, it is in
		// its own redraw callback; and can't draw yet
#ifdef DEBUG_DIRTY_DRAW		
		lprintf( "state 5...");
#endif		
		//return;
	}
	if( wl.drawThread != thisThread ){
		//r->flags.dirty = 1;  // This must be dirty?  (Will be dirty?)
		postDrawEvent( r, noCallback );
	} else {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "********* BEGIN REDRAW THREAD ******************* " );
		lprintf( "(get new buffer)Issue redraw on renderer %p", r );
#endif
		redrawState = 0;
		int couldCommit; couldCommit = r->flags.canCommit;
		while( wl.flags.shellInit ) {
			lprintf( "Wait for shell..." );
			redrawState = 1;
			IdleFor( 200 );
			redrawState = 2;
			break;
		}
		while( !r->flags.canDamage ) {
			
			//r->flags.wantBuffer = 1;
			redrawState = 3;
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "Waiting in redraw to be able to draw; if I can't commit, then there's no good buffer anyway" );
#endif			
			IdleFor( 1000 );
			redrawState = 4;
			//Relinquish();
		}
		EnterCriticalSec( &wl.cs_wl );
		
		r->flags.canCommit = 0; // don't allow sub-daamages to commit... just damage  (if this gets a frame callback it'll reset and flush early)
		//lprintf( "maybe redraw callback? should have forced wantattach?");
		// dirty happens in state after a resize.
		if( ( r->buffer_states[r->curBuffer].dirty || !noCallback ) && r->pRedrawCallback ) {
			redrawState = 5;
#if defined( DEBUG_REDRAW )		
			lprintf( "dispatch redraw...%d %d", r->buffer_states[r->curBuffer].dirty, noCallback );
#endif			
			r->flags.drawing = 1;
			r->drawResult = r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r  );
			r->flags.drawing = 0;
#if defined( DEBUG_REDRAW )		
			//lprintf( "Draw Result %d %d", r->drawResult, r->flags.dirty );
#endif	
			// autoDraw might be set if the
			// redraw callback doesn't post any sort of dirty update status...
			if(r->flags.autoDraw )
				if( !r->flags.dirty ) {
					BlotImage( r->pImageOutput, r->pImageDraw, 0,0 );
					wl_surface_damage( r->surface, 0, 0, r->bufw, r->bufh );
					r->flags.dirty = 1; // was damaged, needs commit.
				} else {
					// lost frame?
					//lprintf( "Still waiting for the previous output to write..");
				}
			redrawState = 6;
		}
#if defined( DEBUG_REDRAW )		
		lprintf( "wayland draw... Dirty? after draw %d", r->flags.dirty );
#endif		
		if( !attachNewBuffer( r, 1, 1 ) ) {
			//lprintf( "normal draw callback thread wanted the buffer... and it didn't getone.");
		}else
			r->flags.canCommit = 1; // allow sub-damages

		LeaveCriticalSec( &wl.cs_wl );
		redrawState = -1;
		//if( r->flags.commited ){
			//lprintf( "A commit happened during redraw, wait for that to flush.");
			//wl_display_flush( wl.display );
			//wl_display_roundtrip_queue(wl.display, wl.queue);
		//}
	}
}

static void sack_wayland_SetApplicationTitle( char const *title ){

	//xdg_toplevel_set_title(  r->xdg_toplevel, "I DOn't want a title");

}

static void sack_wayland_SetApplicationIcon( Image icon ) {

}

static void sack_wayland_GetDisplaySize( uint32_t *w, uint32_t *h ){
	return GetDisplaySizeEx( 0, NULL, NULL, w, h );
}

static void sack_wayland_SetDisplaySize( uint32_t w, uint32_t h ) {

}

static PRENDERER sack_wayland_OpenDisplayAboveUnderSizedAt(uint32_t attr , uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above, PRENDERER under ){
	struct wvideo_tag *rAbove = (struct wvideo_tag*)above;
	struct wvideo_tag *rUnder = (struct wvideo_tag*)under;
	struct wvideo_tag *r;
	InitWayland();
	
	r = New( struct wvideo_tag );
	memset( r, 0, sizeof( *r ) );
	r->flags.autoDraw = 1;
	r->flags.wantAttach = 1;

	r->above = rAbove;
	r->under = rUnder;

	if( x < 0 ) x = wl.opens * 25;
	if( y < 0 ) y = wl.opens * 25;
	wl.opens++;
	if( (int)w < 0 ) w = 800;
	if( (int)h < 0 ) h = 600;

	lprintf( "Initial window create a window size set bufw bufh");
	r->bufw = w;
	r->bufh = h;
	r->pImageDraw = MakeImageFile( w, h );
	{
		struct wvideo_tag *parent = r->above;
		while( (parent ) && parent->sub_surface ) {
			x -= parent->x;
			y -= parent->y;
			parent = parent->above;
		}
	}
	r->x = x;
	r->y = y;
	r->flags.hidden = 1;

	// eventually we'll need a commit callback.

	AddLink( &wl.pActiveList, r );
	while( !wl.flags.bInited && !wl.flags.bFailed ) {
		//lprintf( "Waiting for wayland setup...");
		Relinquish();
	}
	if( wl.flags.bFailed ) return NULL;

	if( !CreateWindowStuff( r, rAbove ) ) {
		lprintf( "Failed to create drawing surface... falling back to offscreen rendering?");
	}
	
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
	if( wl.hVidFocused == r ) wl.hVidFocused = NULL;
	if( wl.hCaptured == r ) wl.hCaptured = NULL;
	r->flags.bDestroy = 1;
	EnterCriticalSec( &wl.cs_wl );
	if( r->sub_surface ) {
		wl_subsurface_destroy( r->sub_surface );
		lprintf( "destroy sub_surface... " );
	}
	if( r->shell_surface ) {
		if( wl.xdg_wm_base ) {
			if( r->xdg_toplevel )
				xdg_toplevel_destroy( r->xdg_toplevel );
			lprintf( "xdg_surface_destroy... %p", r->shell_surface );
			xdg_surface_destroy( (struct xdg_surface*)r->shell_surface );
		}else
			wl_shell_surface_destroy( r->shell_surface );
	}
	lprintf( "Destroy surface:%p %p", r, r->surface );
	wl_surface_destroy( r->surface );
	r->surface = NULL;
	DeleteLink( &wl.pActiveList, r );
	if( r->above ) {
		struct wvideo_tag *a = r->above;
		while( a->above ) a = a->above;
		lprintf( "Destroyed surface commit here" );
		wl_surface_commit( a->surface );
	}
	LeaveCriticalSec( &wl.cs_wl );
	
}

static void sack_wayland_UpdateDisplayPortionEx(PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( (int)w < 0 ) w = r->bufw;
	if( (int)h < 0 ) h = r->bufh;
	r->flags.hidden = 0; // ShowWindow( SW_SHOW );
#ifdef DEBUG_UPDATE_DISPLAY
	_lprintf( DBG_RELAY )( "UpdateDisplayPortionEx %p %d %d %d %d", r->surface, x, y, w, h );
#endif	
	THREAD_ID prior = 0;
	BlotImageSizedEx( r->pImageOutput, r->pImageDraw, x, y, x, y, w, h, 0, BLOT_COPY );
	if( r->flags.commited ) {
		lprintf( "Still waiting for commit on buffer to release the buffer?");
		postDirt( r, x, y, w, h );	
		return;
	}

	int e = EnterCriticalSecNoWait( &wl.cs_wl, NULL );
	if( e > 0 ) {
#ifdef DEBUG_DIRTY_DRAW		
		lprintf( "Locked, able to do update");
#endif
		if( r->surface ) {
			//_lprintf(DBG_RELAY)( "Do surface damage.." );
			wl_surface_damage( r->surface, x, y, w, h );
			// this doesn't directly commit
			// so maybe canCommit isn't useful?
			r->flags.dirty = 1;
#ifdef DEBUG_DIRTY_DRAW		
			lprintf( "Set dirty, and is it drawing? %d %p", r->flags.drawing, r->frame_callback);
#endif		
			//lprintf( "doing frame callback after this too?")	;
			if( !r->flags.drawing ) {
				if( !r->frame_callback ) {
					r->frame_callback = wl_surface_frame( r->surface );
					wl_callback_add_listener( r->frame_callback, &frame_listener, r );
				}
				postDrawEvent( r, 1 );
			}
		}
		LeaveCriticalSec( &wl.cs_wl );
	} else {
		// it's locked because it's in use... 
		// it will get around to damaging before commit.(?)
#ifdef DEBUG_DIRTY_DRAW		
		lprintf( "This posted the dirt to draw which might be eager?");
#endif		
		postDirt( r, x, y, w, h );	
	}
}

static void sack_wayland_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	_lprintf( DBG_RELAY )( "Who calls update the WHOLE display anyway? %d %d", r->buffer_states[r->curBuffer].w, r->buffer_states[r->curBuffer].h);
	sack_wayland_UpdateDisplayPortionEx( renderer
		, 0, 0
		, r->buffer_states[r->curBuffer].w
		, r->buffer_states[r->curBuffer].h DBG_RELAY );
}

static void sack_wayland_GetDisplayPosition( PRENDERER renderer, int32_t* x, int32_t* y, uint32_t* w, uint32_t* h ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	// wayland displays are always at '0'
	if( x ) x[0] = 0;
	if( y ) y[0] = 0;
	if( w ) w[0] = r->bufw;
	if( h ) h[0] = r->bufh;
}

static void sack_wayland_MoveDisplay(PRENDERER renderer, int32_t x, int32_t y){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	//lprintf( "MOVE DISPLAY %d %d", x, y);
	if( r->sub_surface ){
		struct wvideo_tag *parent = r->above;
		while( (parent) && parent->sub_surface ) {
			x -= parent->x;
			y -= parent->y;
			lprintf( "adjust position:%d %d %d %d", x, y, parent->x, parent->y );
			parent = parent->above;
		}
		lprintf( "MOVE SUBSURFACE %d %d", x, y );
		r->x = x;
		r->y = y;
		EnterCriticalSec( &wl.cs_wl );
		wl_subsurface_set_position( r->sub_surface, x, y );
		LeaveCriticalSec( &wl.cs_wl );

	}
	else if( wl.hCaptured == r ){
		lprintf( "Fabricting system drag");
		sack_wayland_BeginMoveDisplay(renderer);

	}
}
static void sack_wayland_MoveDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	lprintf( "MOVE DISPLAYREL %d %d", dx, dy);
	if( r->sub_surface ){
		r->x += dx;
		r->y += dy;
		EnterCriticalSec( &wl.cs_wl );
		wl_subsurface_set_position( r->sub_surface, r->x, r->y );
		LeaveCriticalSec( &wl.cs_wl );

	}
}


static void sack_wayland_SizeDisplay(PRENDERER renderer, uint32_t w, uint32_t h){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}
static void sack_wayland_SizeDisplayRel(PRENDERER renderer, int32_t dw, int32_t dh){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

}

static void sack_wayland_MoveSizeDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy, int32_t dw, int32_t dh){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	lprintf( "MOVESIZE" );
}

static void sack_wayland_SetCloseHandler( PRENDERER renderer, CloseCallback closeCallback, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->pWindowClose = closeCallback;
	r->dwCloseData = psv;
}

static void sack_wayland_SetMouseHandler( PRENDERER renderer, MouseCallback mouse, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->mouse = mouse; r->psvMouse = psv;
}
static void sack_wayland_SetRedrawHandler( PRENDERER renderer, RedrawCallback redraw, uintptr_t psv ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->pRedrawCallback = redraw;
	r->dwRedrawData = psv;
}

static void sack_wayland_SetKeyboardHandler(PRENDERER renderer, KeyProc keyproc, uintptr_t psv){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->pKeyProc = keyproc;
	r->dwKeyData = psv;
}

static void sack_wayland_SetLoseFocusHandler( PRENDERER renderer, LoseFocusCallback lfCallback, uintptr_t psv ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->dwLoseFocus = psv;
	r->pLoseFocus = lfCallback;

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
	struct wvideo_tag *r = (struct wvideo_tag*)render;
	if( title ) {
		if( r->xdg_toplevel )
			xdg_toplevel_set_title(  r->xdg_toplevel, title );
		r->pending_title = StrDup( title );
	}

}

static void sack_wayland_RestoreDisplayEx( PRENDERER renderer DBG_PASS ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	r->flags.hidden = 0;
	lprintf( "REDRAW - this is probabaly what wakes it?" );
	sack_wayland_Redraw( renderer );
}

static void sack_wayland_RestoreDisplay( PRENDERER renderer ){
	return sack_wayland_RestoreDisplayEx( renderer DBG_SRC );
}

static void sack_wayland_HideDisplay( PRENDERER renderer ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( !r->flags.hidden ) {
		r->flags.hidden = 1;
		clearBuffer(r);
		EnterCriticalSec( &wl.cs_wl );
		lprintf( "Hide display commit" );
		wl_surface_commit( r->surface );
		wl_display_flush( wl.display );
		LeaveCriticalSec( &wl.cs_wl );
	}
}

static void sack_wayland_GetMouseState    ( int32_t *x, int32_t *y, uint32_t *b ){
	if( x ) x[0] = wl.mouse_.x;
	if( y ) y[0] = wl.mouse_.y;
	if( b ) b[0] = wl.mouse_.b;
}

static Image sack_wayland_GetDisplayImage(PRENDERER renderer) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r ) {
		if( !r->pImageDraw ) {
			lprintf( "draw getting display image...(do attachnewbuffer (initialized))");
			attachNewBuffer( r, 1, 0 );
		}
		//lprintf( "GetDisplayImage:Result with image: %p %p", r->pImage );
		return r->pImageDraw;
	}
	return NULL;
}

static void sack_wayland_OwnMouseEx ( PRENDERER display, uint32_t bOwn DBG_PASS ){
	struct wvideo_tag *r = (struct wvideo_tag*)display;
	//_lprintf( DBG_RELAY )( "Own Mouse - probably for move/drag... %p %d", display, bOwn );
	if( bOwn )
		wl.hCaptured = r;
	else if( wl.hCaptured == r || !r )
		wl.hCaptured = NULL;
}

static void sack_wayland_ForceDisplayFocus( PRENDERER display ) {
	struct wvideo_tag *r = (struct wvideo_tag*)display;
	if( r->flags.bDestroy )
	{
		// in the process of being destroyed... don't focus it.
		return;
	}
	//lprintf( "Forcing Focus!!!");
	wl.hVidFocused = (PXPANEL)display;

	if( !r->flags.bFocused )
	{
		r->flags.bFocused = 1;
		{
			if( FindLink( &wl.pActiveList, r ) != INVALID_INDEX )
			{
				//lprintf( "Dispatching focus." );
				if( r && r->pLoseFocus )
					r->pLoseFocus (r->dwLoseFocus, NULL );
			}
		}
	}
}

static PSPRITE_METHOD sack_wayland_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h ), uintptr_t psv ){
	PXPANEL r = (PXPANEL)render;
	//r->drawSprites = RenderSprites;
	//r->psvSprites = psv;
}

static void sack_wayland_SyncRender( PRENDERER r ) {
	EnterCriticalSec( &wl.cs_wl );
	wl_display_flush( wl.display);
	wl_display_roundtrip_queue(wl.display, wl.queue);
	LeaveCriticalSec( &wl.cs_wl );

}

static void sack_wayland_GetMousePosition ( int32_t *x, int32_t *y ){
	(*x) = wl.mouse_.x;
	(*y) = wl.mouse_.y;
}


static int sack_wayland_BindEventToKey( PRENDERER pRenderer
	, uint32_t scancode, uint32_t modifier
	, KeyTriggerHandler trigger, uintptr_t psv ){
	PXPANEL r = (PXPANEL)pRenderer;
	if( r ) {
		if( !r->pKeyDefs )
			r->pKeyDefs = wl_CreateKeyBinder();
		wl_BindEventToKey( r->pKeyDefs, scancode, modifier, trigger, psv );
	}
	else
		wl_BindEventToKey( NULL, scancode, modifier, trigger, psv );
}

static int sack_wayland_UnbindKey( PRENDERER pRenderer, uint32_t scancode, uint32_t modifier ){
	PXPANEL r = (PXPANEL)pRenderer;
	if( r && r->pKeyDefs ) {
		wl_UnbindKey( r->pKeyDefs, scancode, modifier );
	}
	else
		wl_UnbindKey( NULL, scancode, modifier );
}

void sack_wayland_BeginMoveDisplay(  PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	//lprintf( "Issue Mouse Move %d", wl.mouseSerial );
 	wl.mouse_.b &= ~MK_LBUTTON;
	EnterCriticalSec( &wl.cs_wl );

	if( r->sub_surface ) {
		//wl_subsurface_set_position( r->sub_surface, 0, 0 );
	} else if( wl.xdg_wm_base ) {
		if( r->xdg_toplevel) xdg_toplevel_move( r->xdg_toplevel, wl.seat, wl.mouseSerial );
	} else
		wl_shell_surface_move( r->shell_surface, wl.seat, wl.mouseSerial );
	LeaveCriticalSec( &wl.cs_wl );
 }
 //static void sack_wayland_EndMoveDisplay(  PRENDERER pRenderer )

int sd_to_xdg[] = {
	[0]=0,
	[wrsdv_top] = 1,
	[wrsdv_top|wrsdv_left] = 5,
	[wrsdv_top|wrsdv_right]=9,
	[wrsdv_left]=4,
	[wrsdv_right]=8,
	[wrsdv_bottom] = 2,
	[wrsdv_bottom|wrsdv_left] = 6,
	[wrsdv_bottom|wrsdv_right]=10
	
};

 void sack_wayland_BeginSizeDisplay(  PRENDERER renderer, enum sizeDisplayValues mode ) {
	PXPANEL r = (PXPANEL)renderer;
	int xdg_mode = sd_to_xdg[mode];
 	wl.mouse_.b &= ~MK_LBUTTON;
	lprintf( "Issue Mouse SizeBegin %d %x %x", wl.mouseSerial, mode, xdg_mode );
	EnterCriticalSec( &wl.cs_wl );
	if( wl.xdg_wm_base ) {
	 	xdg_toplevel_resize( r->xdg_toplevel, wl.seat, wl.mouseSerial, xdg_mode );
		
	} else{
		wl_shell_surface_resize( r->shell_surface, wl.seat, wl.mouseSerial, mode );
	}
	wl_display_flush( wl.display );
	LeaveCriticalSec( &wl.cs_wl );
 }
 //static void sack_wayland_EndSizeDisplay(  PRENDERER pRenderer )

static LOGICAL sack_wayland_AllowsAnyThreadToUpdate( void ) {
    return TRUE;
}

static void sack_wayland_WillUpdatePortions( PRENDERER renderer, LOGICAL disableAutoDraw ){
	PXPANEL r = (PXPANEL)renderer;
	r->flags.autoDraw = !disableAutoDraw;
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
													, (Image (CPROC*)(PRENDERER)) sack_wayland_GetDisplayImage
													, sack_wayland_SetCloseHandler
													, sack_wayland_SetMouseHandler
													, sack_wayland_SetRedrawHandler
													, sack_wayland_SetKeyboardHandler
													, sack_wayland_SetLoseFocusHandler
													, NULL
													, sack_wayland_GetMousePosition// (void (CPROC*)(int32_t *, int32_t *)) GetMousePosition
													, NULL//(void (CPROC*)(PRENDERER, int32_t, int32_t)) SetMousePosition
													, NULL//HasFocus  // has focus
													, sack_wayland_GetKeyText
													, NULL//IsKeyDown
													, NULL//KeyDown
													, NULL//DisplayIsValid
													, sack_wayland_OwnMouseEx
													, NULL//BeginCalibration
													, sack_wayland_SyncRender	// sync
													, NULL//MoveSizeDisplay
													, NULL//MakeTopmost
													, sack_wayland_HideDisplay
													, sack_wayland_RestoreDisplay
													, sack_wayland_ForceDisplayFocus
													, NULL//ForceDisplayFront
													, NULL//ForceDisplayBack
													, sack_wayland_BindEventToKey
													, sack_wayland_UnbindKey
													, NULL//IsTopmost
													, NULL // OkaySyncRender is internal.
													, sack_wayland_IsTouchDisplay
													, sack_wayland_GetMouseState
													, sack_wayland_EnableSpriteMethod
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
#ifdef WIN32
													, NULL//GetNativeHandle
#endif
													, GetDisplaySizeEx
													, NULL
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
													, sack_wayland_AllowsAnyThreadToUpdate
													, NULL//Vidlib_SetDisplayFullScreen
													, NULL//Vidlib_SuspendSystemSleep
													, NULL /* is instanced */
													, NULL /* render allows copy (not remote network render) */
													, NULL//SetDisplayCursor
													, NULL//IsDisplayRedrawForced
														, NULL//RENDER_PROC_PTR( void, ReplyCloseDisplay )( void ); // only valid during a headless display event....

		/* Clipboard Callback */
													, NULL//	RENDER_PROC_PTR( void, SetClipboardEventCallback )(PRENDERER pRenderer, ClipboardCallback callback, uintptr_t psv);


	// where ever the current mouse is, lock the mouse to the window, and allow the mouse to move it.
	//
													, sack_wayland_BeginMoveDisplay
													, sack_wayland_BeginSizeDisplay
													, sack_wayland_WillUpdatePortions
};

static void InitWayland( void ) {
	if( !wl.flags.bInited ) {
		//wl.waylandThread = MakeThread();
		//initConnections();
		//wl.flags.bInited = 1;
		ThreadTo( waylandThread, 0 );
		ThreadTo( waylandDrawThread, 0 );
		AddIdleProc( do_waylandThread, 1 );
		AddIdleProc( do_waylandDrawThread, 1 );
		while( !wl.flags.bInited && !wl.flags.bFailed) {
			Relinquish();
		}
		if( wl.flags.bInited )
			finishInitConnections();
	}

}

static POINTER GetWaylandDisplayInterface(void)
{
	if(0)
	if( !wl.flags.bInited ) {
		//wl.waylandThread = MakeThread();
		//initConnections();
		//wl.flags.bInited = 1;
		ThreadTo( waylandThread, 0 );
		ThreadTo( waylandDrawThread, 0 );
		AddIdleProc( do_waylandThread, 1 );
		AddIdleProc( do_waylandDrawThread, 1 );
		while( !wl.flags.bInited && !wl.flags.bFailed) {
			Relinquish();
		}
		if( wl.flags.bInited )
			finishInitConnections();
	}
	return (POINTER)&VidInterface;
}

static void DropWaylandDisplayInterface(POINTER p)
{
}

PRIORITY_PRELOAD( WaylandRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	RegisterInterface( "sack.render.wayland", GetWaylandDisplayInterface, DropWaylandDisplayInterface );
}

