// much core wayland interface code from...
// https://jan.newmarch.name/Wayland/ProgrammingClient/
// https://jan.newmarch.name/Wayland/WhenCanIDraw/
//
//#define DEBUG_COMMIT

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
,[	wis_xdg_base]={NULL,1 }
#ifdef ALLOW_KDE
 ,[wis_kde_shell]={ "org_kde_plasma_shell", 1 }
#endif
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

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
    int32_t y, int32_t physical_width, int32_t physical_height,
    int32_t subpixel, const char *make, const char *model, int32_t transform)
{
  struct output_data *output_data = wl_output_get_user_data(wl_output);
  (void)data;
  (void)wl_output;
  (void)x;
  (void)y;
  (void)physical_width;
  (void)physical_height;
  (void)subpixel;
  (void)make;
  (void)model;
  (void)transform;
  lprintf( "Screen geometry: %d %d  %d %d", x, y, physical_height, physical_width );
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
  (void)flags;
  (void)width;
  (void)height;
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

	pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	pointer_data->target_surface = surface;
	/*
	wl_surface_attach(pointer_data->surface,
		pointer_data->buffer, 0, 0);
	wl_surface_commit(pointer_data->surface);
	*/
	//wl_pointer_set_cursor(wl_pointer, serial,
	//    pointer_data->surface, pointer_data->hot_spot_x,
	//    pointer_data->hot_spot_y);

	wl.mouse_.x = surface_x >> 8;
	wl.mouse_.y = surface_y >> 8;
	//lprintf( "Mouse Enter %d %d" );

	PXPANEL r = wl_surface_get_user_data(
		pointer_data->target_surface);
	//if( !GetPixel( GetDipslaySurface( r ), wl.mouse.x, wl.mouse.y ))
		//wl_seat_pointer_forward( wl.seat, serial );
	if (r != NULL && r->mouse ) {
		r->mouse( r->psvMouse, wl.mouse_.x, wl.mouse_.y, wl.mouse_.b );
		// callback(b);
	}

}

static void pointer_leave(void *data,
	struct wl_pointer *wl_pointer, uint32_t serial,
	struct wl_surface *wl_surface) {
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	PXPANEL r = wl_surface_get_user_data(
		pointer_data->target_surface);
	//lprintf( "Mouse Leave" );
	wl.mouse_.b = 0;
	if (r != NULL && r->mouse ) {
		r->mouse( r->psvMouse, wl.mouse_.x, wl.mouse_.y, MK_NO_BUTTON );
		// callback(b);
	}

 }

static void pointer_motion(void *data,
	struct wl_pointer *wl_pointer, uint32_t time,
	wl_fixed_t surface_x, wl_fixed_t surface_y) {
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	wl.mouse_.x = surface_x>>8;
	wl.mouse_.y = surface_y>>8;
  	//lprintf( "mouse motion:%d %d", wl.mouse_.x, wl.mouse_.y );


   PXPANEL r = wl_surface_get_user_data(
        pointer_data->target_surface);

	//if( !GetPixel( GetDipslaySurface( r ), wl.mouse.x, wl.mouse.y ))
		 //wl_seat_pointer_forward( wl.seat, serial );

    if (r != NULL && r->mouse ) {
		 r->mouse( r->psvMouse, wl.mouse_.x, wl.mouse_.y, wl.mouse_.b );
       // callback(b);
	 }

}

static void pointer_button(void *data,
    struct wl_pointer *wl_pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state)
{
	struct pointer_data *pointer_data;
	PXPANEL r;
	void (*callback)(uint32_t);
	wl.mouseSerial = serial;
	//lprintf( "pointer button:%d %d", button, state);
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
	if (r != NULL && r->mouse ) {
		r->mouse( r->psvMouse, wl.mouse_.x, wl.mouse_.y, wl.mouse_.b );
		// callback(button);
	}
}

static void pointer_axis(void *data,
    struct wl_pointer *wl_pointer, uint32_t time,
    uint32_t axis, wl_fixed_t value) { }

static void pointer_frame( void *data,
    struct wl_pointer *wl_pointer) { }

	static void pointer_axis_source(void *data,
			    struct wl_pointer *wl_pointer,
			    uint32_t axis_source){}
	static void pointer_axis_stop(void *data,
			  struct wl_pointer *wl_pointer,
			  uint32_t time,
			  uint32_t axis){}
	static void pointer_axis_discrete(void *data,
			      struct wl_pointer *wl_pointer,
			      uint32_t axis,
			      int32_t discrete){}


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
		//lprintf( "Well here's a map...");

		char *keymap_string = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
		//lprintf( "mmap:%p %d", keymap_string, errno );
		xkb_keymap_unref (wl.keymap);
		wl.keymap = xkb_keymap_new_from_string (wl.xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
		munmap (keymap_string, size);
		close (fd);
		xkb_state_unref (wl.xkb_state);
		wl.xkb_state = xkb_state_new (wl.keymap);

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
	if( wl.hVidFocused && wl.hVidFocused->pLoseFocus )
		wl.hVidFocused->pLoseFocus( wl.hVidFocused->dwLoseFocus, (PRENDERER)r );
	wl.hVidFocused = r;
	if( r && r->pLoseFocus )
		r->pLoseFocus( r->dwLoseFocus, NULL );
}

static void keyboard_leave(void *data,
		      struct wl_keyboard *wl_keyboard,
		      uint32_t serial,
		      struct wl_surface *surface){
	if( surface ) {
		PXPANEL r = (PXPANEL) wl_surface_get_user_data( surface );
		if( wl.hVidFocused && wl.hVidFocused->pLoseFocus ){
			//lprintf( "on leave lose focus?");
			wl.hVidFocused->pLoseFocus( wl.hVidFocused->dwLoseFocus, (PRENDERER)1 );
		}
	}
	wl.hVidFocused = NULL;
}


static int xkbToWindows[] = {
	[65288] = KEY_BACKSPACE,
	[65307] = KEY_ESCAPE,
	[65289] = KEY_TAB,
	[65056] = KEY_TAB,
	[65293] = KEY_ENTER,
};

static void initKeys( void ) {
	int n;
	for( n = 32 ; n <= 127; n++ ){
		xkbToWindows[n] = n;
	}

	for( n = 'A' ; n <= 'Z'; n++ ){
		xkbToWindows[n+32] = n;
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
	if( !xkbToWindows[32] ) initKeys();
	//lprintf( "KEY: %p %d %d %d %d", wl_keyboard, serial, time, key, state );

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym (wl.xkb_state, key+8);
		wl.utfKeyCode = xkb_keysym_to_utf32 (keysym);

		uint32_t keycode;
		uint8_t keyval = xkbToWindows[keysym];
		lprintf( "Keysym:%d(%04x) %d %d %d", keysym, keysym, key, state, keyval );
		keycode = KEY_PRESSED;
		keycode |= keyval << 16;
		keycode |= keyval;
		keycode |= wl.keyMods;

		if( r->pKeyProc ){
			lprintf( "Sending as key %08x", keycode );
			r->pKeyProc( r->dwKeyData, keycode );
		}
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
	}
	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		uint32_t keycode;
		keycode = 0;
		keycode |= ( wl.keysym << 16 ) | wl.keysym;
		keycode |= wl.keyMods;

		if( r->pKeyProc ){
			r->pKeyProc( r->dwKeyData, keycode );
		}
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
		//wl.keyMods |= KEY_MOD_SHIFT;
		wl.keyMods |= KEY_SHIFT_DOWN;
	}
	if( mods_depressed & 4 ) {
		wl.keyMods |= KEY_CONTROL_DOWN;
	}
	if( mods_depressed & 8 ) {
		wl.keyMods |= KEY_ALT_DOWN;
	}
	// I get an initial state of nil,0,0,0,N
	//lprintf( "MOD: %p %d %x %x %d",  mods_depressed, mods_latched, mods_locked, group );
	xkb_state_update_mask (wl.xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);


}

static const TEXTCHAR * sack_wayland_GetKeyText           ( int key ){
	static char buf[6];
	lprintf( "assuming key was the one dispatched....");
	ConvertToUTF8( buf, wl.utfKeyCode );
	return buf;
}


static void keyboard_repeat_info(void *data,
			    struct wl_keyboard *wl_keyboard,
			    int32_t rate,
			    int32_t delay){
					 lprintf( "Why do I care about the repeat? %d %d", rate, delay );
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
	xdg_surface_ack_configure( xdg_surface, serial );

}

static struct xdg_surface_listener const xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_wm_base_ping( void*data, struct xdg_wm_base*base, uint32_t serial){
	xdg_wm_base_pong( base,serial);
}
static struct xdg_wm_base_listener const xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};


static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
	static volatile int processing;
	while( processing ) Relinquish();
	processing = 1;

	int n;
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

      wl.xdg_wm_base = wl_registry_bind(registry, id,
                                 &xdg_wm_base_interface, version);
		xdg_wm_base_add_listener( wl.xdg_wm_base, &xdg_wm_base_listener, NULL );

	} else if( n == wis_seat ) {
		wl.seat = wl_registry_bind(registry, id,
			&wl_seat_interface,  version>2?version:version);
		wl.pointer = wl_seat_get_pointer(wl.seat);
		wl.pointer_data.surface = wl_compositor_create_surface(wl.compositor);
		wl_pointer_add_listener(wl.pointer, &pointer_listener, &wl.pointer_data);
		wl.keyboard = wl_seat_get_keyboard(wl.seat );
		wl_keyboard_add_listener(wl.keyboard, &keyboard_listener, NULL );
	} else if( n == wis_shm ) {
      wl.shm = wl_registry_bind(registry, id,
                                 &wl_shm_interface, version);
		wl_shm_add_listener(wl.shm, &shm_listener, NULL);
	} else if( n == wis_output ) {
		struct output_data *out = New( struct output_data );
		//lprintf( "Display");
		out->wl_output = wl_registry_bind( registry, id, &wl_output_interface, version );
		// pending scaling
		out->pending_scale = 1;
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
	wl.pii = GetImageInterface();
	wl.display = wl_display_connect(NULL);
	if (wl.display == NULL) {
		lprintf( "Can't connect to display");
		return;
	}
	wl.queue = wl_display_create_queue( wl.display );
   //wl_proxy_create( NULL, )
	wl.flags.bInited = 1;

	interfaces[wis_xdg_base].name = xdg_wm_base_interface.name;

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
	}
	wl.dirty = 0;
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

        free(name);

        if (fd < 0)
                return -1;

        if (ftruncate(fd, size) < 0) {
                close(fd);
                return -1;
        }

        return fd;
}



static void redraw( void *data, struct wl_callback* callback, uint32_t time );
static const struct wl_callback_listener frame_listener = {
	redraw
};

static void releaseBuffer( void*data, struct wl_buffer*wl_buffer );
static const struct wl_buffer_listener  buffer_listener = {
	.release = releaseBuffer,
};

static struct wl_buffer * allocateBuffer( PXPANEL r )
{
	int stride = r->w * 4; // 4 bytes per pixel
	int size = stride * r->h;
	int fd;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		lprintf( "creating a buffer file for %d B failed: %m\n",
			size);
		return FALSE;
	}
	r->buffer_sizes[r->curBuffer] = size;
	r->shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (r->shm_data == MAP_FAILED) {
		lprintf( "mmap failed: %m\n");
		close(fd);
		return FALSE;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(wl.shm, fd, size);
	r->buff = wl_shm_pool_create_buffer(pool, 0, /* starting offset */
					r->w, r->h,
					stride,
					WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener( r->buff, &buffer_listener, r );
	wl_shm_pool_destroy(pool);

	return r->buff;
}

static void clearBuffer( PXPANEL r ) {
	//if( r->flags.hidden ) return;
	wl_surface_attach( r->surface, NULL, 0, 0);
}

static struct wl_buffer * nextBuffer( PXPANEL r, int attach ) {
	if( r->flags.hidden ) {
		lprintf( "window is hidden... returning a fault.");
		return NULL;
	}
	if( r->freeBuffer[r->curBuffer]  && ( r->bufw == r->w && r->bufh == r->h ) ) {
		//lprintf( "Can just use the current image... it's already attached", r->buff);
		return r->buff;
	}

	r->curBuffer=1-r->curBuffer;
	int curBuffer = r->curBuffer;
	lprintf( "using image to a new image.... %d",curBuffer );
	if( r->bufw != r->w || r->bufh != r->h ) {
		// if the buffer has to change size, we need a new one....
		// maybe can keep this around to fill a copy of the prior window?
		//lprintf( "Remove Old Buffer");
		if( r->color_buffers[curBuffer] ) {
			munmap( r->color_buffers[curBuffer], r->buffer_sizes[curBuffer] );
			UnmakeImageFile( r->buffer_images[curBuffer] );
			if( r->buffers[curBuffer] )
				wl_buffer_destroy( r->buffers[curBuffer] );
		}
		r->w = r->bufw;
		r->h = r->bufh;
		r->buffer_images[curBuffer] = NULL;
		r->buffers[curBuffer] = NULL;
		r->color_buffers[curBuffer] = NULL;
	}

	// setup the current values to this buffer...
	r->shm_data = r->color_buffers[curBuffer];
	r->buff = r->buffers[curBuffer];

	if( !r->buff ) {
		//lprintf( "Allocate NEW buffer" );
		// haven't actually allocated a bufer yet... so do so...
		allocateBuffer(r);
		r->buffer_images[curBuffer] = RemakeImage( r->buffer_images[curBuffer], r->shm_data, r->w, r->h );
		if( r->buffer_images[1-curBuffer] ) {
			//lprintf( "Copy old buffer to new current buffer...");
			BlotImage( r->buffer_images[curBuffer], r->buffer_images[1-curBuffer], 0, 0 );
		}

		r->buffers[curBuffer] = r->buff;
		lprintf( "Setting buffer %d  %p", curBuffer, r->buff );
		r->color_buffers[curBuffer] = r->shm_data;
		r->freeBuffer[curBuffer] = 1;
	}
	r->pImage = RemakeImage( r->pImage, r->shm_data, r->w, r->h );
	r->pImage->flags |= IF_FLAG_FINAL_RENDER;
	wl_surface_attach( r->surface, r->buff, 0, 0);
	return r->buff;
}


 void redraw( void *data, struct wl_callback* callback, uint32_t time ) {
	PXPANEL r = (PXPANEL)data;
	if( r->frame_callback ) wl_callback_destroy( r->frame_callback );
	if( !r->flags.bDestroy ) {
		// schedule another callback.
		r->frame_callback = wl_surface_frame( r->surface );
		wl_callback_add_listener( r->frame_callback, &frame_listener, r );
		//lprintf( "frame callback - check dirty %d %d %d", r->flags.dirty, r->flags.canCommit );

		if( r->flags.dirty ){
			r->flags.canCommit = 0;
			r->flags.dirty = 0;
			r->flags.commited = 1;
#if defined( DEBUG_COMMIT )
			lprintf( "Window is dirty, do commit");
#endif
			struct wl_buffer *next = nextBuffer(r, 0);

			wl_surface_commit( r->surface );
			wl_surface_attach( r->surface, next, 0, 0 );
			// wait until we actually NEED the buffer, maybe we can use the same one.
		}else {
			//lprintf( "Allow window to be commited at will.");
			r->flags.canCommit = 1;
		}
	}else
		r->flags.canCommit = 1;

}

static void sack_wayland_Redraw( PRENDERER renderer );

static uintptr_t waylandThread( PTHREAD thread ) {
	wl.waylandThread = thread;
	if( wl.display) DebugBreak();
	initConnections();
	while( wl_display_dispatch_queue(wl.display, wl.queue) != -1 ) {
		//lprintf( "Handled an event." );
		{
			INDEX idx;
			//int flush = 0;
			PXPANEL r;
			//wl.commit = 0;
			//if(0)
			LIST_FORALL( wl.wantDraw, idx, PXPANEL, r ){
				lprintf( "Sending a want draw window...");
				sack_wayland_Redraw( r );
#if 0
				//lprintf( "Damage found... send %p", r );
				if( !r->flags.bDestroy ){
					if( r->flags.canCommit ) {
						flush++;
						r->flags.dirty = 0;
						r->flags.canCommit = 0;
						r->flags.commited = 1;
						r->freeBuffer[r->curBuffer] = 0; // the image is NOT free
						struct wl_buffer *next = nextBuffer(r, 0);
						wl_surface_commit( r->surface );
						wl_surface_attach( r->surface, next, 0, 0 );
						// wait until we NEED a buffer...

					}else {
						//lprintf( "frame callback still waiting?");
						// still waiting... next commit event will catch this.
					}
				}
#endif
			}
			EmptyList( &wl.wantDraw );
		}
	}
	lprintf( "Thread exiting?" );
	//wl_event_queue_destroy( wl.queue );
	wl_display_disconnect( wl.display );
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
	r->bufw = width;
	r->bufh = height;
	sack_wayland_Redraw((PRENDERER)r);
	//wl_surface_commit( r->surface );
	//r->changedEdge = edges;
	lprintf( "shell configure %d %d", width, height );
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
	PXPANEL r = (PXPANEL)data;
	int n;
	for( n = 0; n < 2; n++ )  {
		lprintf( "is %p %p?", r->buffers[n] , wl_buffer );
		if( r->buffers[n] == wl_buffer ) {
			//lprintf( "Buffer %d is free again", n );
			r->freeBuffer[n] = 1;
			break;
		}
	}
	if( n == 2 ) {
		lprintf( "Released buffer isn't on this surface?");
	}
	//lprintf( "Buffer is released, you can draw now..." );
	//if( !)
	//r->surface
	//wl_surface_attach(r->surface, r->buff, 0, 0);

}

LOGICAL CreateWindowStuff(PXPANEL r, PXPANEL parent )
{
	//lprintf( "-----Create WIndow Stuff----- %s %s", hVideo->flags.bLayeredWindow?"layered":"solid"
	//		 , hVideo->flags.bChildWindow?"Child(tool)":"user-selectable" );
	//DebugBreak();

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
		return FALSE;
   }
   wl_surface_set_user_data(r->surface, r);

	if( parent ) {
		r->sub_surface = wl_subcompositor_get_subsurface( wl.subcompositor, r->surface, parent->surface );
		wl_subsurface_set_user_data(r->sub_surface, r);
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
			//xdg_toplevel_set_title(  r->xdg_toplevel, "I DOn't want a title");
		   xdg_surface_set_user_data((struct xdg_surface*)r->shell_surface, r);
			// must commit to get a config
			wl_surface_commit( r->surface );
			// must also wait to get config.
			wl_display_roundtrip_queue(wl.display, wl.queue);
		}

		if (r->shell_surface == NULL) {
			lprintf("Can't create shell surface");
			return FALSE;
		}
	}


	return TRUE;
}


void ShutdownVideo( void )
{
	if( wl.flags.bInited )
	{
	}
}


static void sack_wayland_Redraw( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	struct wl_buffer *b = nextBuffer(r,1);
	if( wl.waylandThread != MakeThread() ){
		AddLink( &wl.wantDraw, r );
#if defined( DEBUG_COMMIT )
		lprintf( "Dispatching roundtrip to trigger draw in thread...");
#endif
		wl_display_roundtrip_queue(wl.display, wl.queue);
	}else {
#if defined( DEBUG_COMMIT )
		lprintf( "(get new buffer)Issue redraw on renderer %p %p", renderer, b );
#endif
		r->flags.commited = 0;
		r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r  );
		if( !r->flags.commited )
			if( r->flags.canCommit ) {
#if defined( DEBUG_COMMIT )
				lprintf( "Commiting redraw... ");
#endif
				struct wl_buffer *next = nextBuffer(r, 0);
				r->flags.canCommit = 0;
				r->flags.dirty = 0;
				r->flags.commited = 1;
				wl_surface_commit( r->surface );
				wl_surface_attach( r->surface, next, 0, 0 );
			}else {
#if defined( DEBUG_COMMIT )
				lprintf( "Can't commit, mark dirty" );
#endif
				r->flags.dirty = 1;

			}
		//wl_display_flush( wl.display );
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
	r = New( struct wvideo_tag );
	memset( r, 0, sizeof( *r ) );

	r->flags.canCommit = 1;
	r->above = rAbove;
	r->under = rUnder;
	r->bufw = r->w = w;
	r->bufh = r->h = h;
	r->x = x;
	r->y = y;

	// eventually we'll need a commit callback.

	AddLink( &wl.pActiveList, r );
	while( !wl.flags.bInited ) {
		lprintf( "Waiting for wayland setup...");
		Relinquish();
	}

	if( !CreateWindowStuff( r, rAbove ) ) {
		lprintf( "Failed to create drawing surface... falling back to offscreen rendering?");
	}else {
		r->frame_callback = wl_surface_frame( r->surface );
		wl_callback_add_listener( r->frame_callback, &frame_listener, r );
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
	if( r->sub_surface )
		wl_subsurface_destroy( r->sub_surface );
	if( r->shell_surface ) {
		if( wl.xdg_wm_base ) {
		if( r->xdg_toplevel )
				xdg_toplevel_destroy( r->xdg_toplevel );
			xdg_surface_destroy( (struct xdg_surface*)r->shell_surface );
		}else
			wl_shell_surface_destroy( r->shell_surface );
	}
	lprintf( "Destroy surface:%p %p", r, r->surface );
	wl_surface_destroy( r->surface );
	DeleteLink( &wl.pActiveList, r );
}

static void sack_wayland_UpdateDisplayPortionEx(PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;

	if( r->buffer_images[r->curBuffer] && r->buffer_images[1-r->curBuffer] ) {
		BlotImageSizedTo( r->buffer_images[1-r->curBuffer], r->buffer_images[r->curBuffer], x, y, x, y, w, h );
	}

	wl_surface_damage( r->surface, x, y, w, h );

	if( r->flags.canCommit ){
#if defined( DEBUG_COMMIT )
		lprintf( "Updating now - getting new buffer, commit, flush, roundtrip, and attach the new buffer");
#endif
		r->flags.canCommit = 0;
		struct wl_buffer *next = nextBuffer(r, 0);
		r->flags.dirty = 0; // don't need a commit.
		r->flags.commited = 1;
		wl_surface_commit( r->surface );
		wl_display_flush( wl.display );
		//if( wl.xdg_wm_base )
		//	wl_display_roundtrip_queue(wl.display, wl.queue);
		wl_surface_attach( r->surface, next, 0, 0 );
	}else {
		r->flags.dirty = 1;
		//lprintf( "update portion, was not ready yet, wait for callback");
	}
}

static void sack_wayland_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	//lprintf( "Update whole surface %d %d", r->w, r->h );
	if( r->buffer_images[r->curBuffer] && r->buffer_images[1-r->curBuffer] ) {
		BlotImage( r->buffer_images[1-r->curBuffer], r->buffer_images[r->curBuffer], 0, 0 );
	}
	wl_surface_damage_buffer( r->surface, 0, 0, r->w, r->h );
	if( r->flags.canCommit ){
#if defined( DEBUG_COMMIT )
		lprintf( "Updating now - getting new buffer, commit, flush, roundtrip, and attach the new buffer");
#endif
		struct wl_buffer *next = nextBuffer(r, 0);
		r->flags.canCommit = 0;
		r->flags.dirty = 0; // don't need a commit.
		r->flags.commited = 1;
		wl_surface_commit( r->surface );
		wl_display_flush( wl.display );
		//if( wl.xdg_wm_base )
		//	wl_display_roundtrip_queue(wl.display, wl.queue);

		wl_surface_attach( r->surface, next, 0, 0 );
	}else {
		r->flags.dirty = 1;
		//lprintf( "update portion, was not ready yet, wait for callback");
	}
}

static void sack_wayland_GetDisplayPosition( PRENDERER renderer, int32_t* x, int32_t* y, uint32_t* w, uint32_t* h ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
}

static void sack_wayland_MoveDisplay(PRENDERER renderer, int32_t x, int32_t y){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	//lprintf( "MOVE DISPLAY %d %d", x, y);
	if( r->above ){
		lprintf( "MOVE SUBSURFACE" );
		wl_subsurface_set_position( r->sub_surface, x, y );
	}
}
static void sack_wayland_MoveDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	lprintf( "MOVE DISPLAYREL %d %d", dx, dy);

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
	xdg_toplevel_set_title(  r->xdg_toplevel, title );

}

static void sack_wayland_RestoreDisplayEx( PRENDERER renderer DBG_PASS ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r->flags.hidden ) {
		r->flags.hidden = 0;
		lprintf( "RESTORE AND REDRAW" );
		nextBuffer(r, 1);
	}
	//lprintf( "REDRAW" );
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
	}
}

static void sack_wayland_GetMouseState    ( int32_t *x, int32_t *y, uint32_t *b ){
	if( x ) x[0] = wl.mouse_.x;
	if( y ) y[0] = wl.mouse_.y;
	if( b ) b[0] = wl.mouse_.b;
}

Image GetDisplayImage(PRENDERER renderer) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r ) {
		if( !r->pImage ) {
			lprintf( "Get the display iamge, which uses next buffer");
			nextBuffer(r,1 );
		}
		return r->pImage;
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
	wl.hVidFocused = (PXPANEL)display;

	if( !r->flags.bFocused )
	{
		r->flags.bFocused = 1;
		{
			if( FindLink( &wl.pActiveList, r ) != INVALID_INDEX )
			{
				lprintf( "Dispatching focus." );
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
	wl_display_flush( wl.display);
	wl_display_roundtrip_queue(wl.display, wl.queue);

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

 static void sack_wayland_BeginMoveDisplay(  PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	lprintf( "Issue Mouse Move %d", wl.mouseSerial );
 	wl.mouse_.b &= ~MK_LBUTTON;

	if( wl.xdg_wm_base )
		xdg_toplevel_move( r->xdg_toplevel, wl.seat, wl.mouseSerial );
	else
		wl_shell_surface_move( r->shell_surface, wl.seat, wl.mouseSerial );
 }
 //static void sack_wayland_EndMoveDisplay(  PRENDERER pRenderer )

 static void sack_wayland_BeginSizeDisplay(  PRENDERER renderer, enum sizeDisplayValues mode ) {
	PXPANEL r = (PXPANEL)renderer;
 	wl.mouse_.b &= ~MK_LBUTTON;
	lprintf( "Issue Mouse SizeBgin %d", wl.mouseSerial );
	if( wl.xdg_wm_base )
	 	xdg_toplevel_resize( r->xdg_toplevel, wl.seat, wl.mouseSerial, mode );
	else
		wl_shell_surface_resize( r->shell_surface, wl.seat, wl.mouseSerial, mode );
 }
 //static void sack_wayland_EndSizeDisplay(  PRENDERER pRenderer )

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
													, NULL//AllowsAnyThreadToUpdate
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

};

static POINTER GetWaylandDisplayInterface(void)
{
	if( !wl.flags.bInited ) {
		//wl.waylandThread = MakeThread();
		//initConnections();
		//wl.flags.bInited = 1;
		ThreadTo( waylandThread, 0 );
		while( !wl.flags.bInited) {
			Relinquish();
		}
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

