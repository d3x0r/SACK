// much core wayland interface code from...
// https://jan.newmarch.name/Wayland/ProgrammingClient/
// https://jan.newmarch.name/Wayland/WhenCanIDraw/
//
#define FALSE 0
#define TRUE (!FALSE)

typedef int INDEX;
typedef int LOGICAL;
#define lprintf(f,...)  do{ printf(f "\n",##__VA_ARGS__); fflush( stdout ); } while(0)

// general debug enable...
#define DEBUG_COMMIT
// tracks the allocation, locking and unlocking of buffers
#define DEBUG_COMMIT_BUFFER
// subset of just canCommit, not buffer requests and other flows...
#define DEBUG_COMMIT_STATE
// events related to keys.
//#define DEBUG_KEY_EVENTS

#define DEBUG_ATTACH_SURFACE

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include <sys/mman.h>
#include <fcntl.h>

#include "local.h"
//#define ALLOW_KDE

typedef struct wvideo_tag *PRENDERER;
typedef struct image *Image;

#define DBG_PASS
#define DBG_SRC
#define DBG_RELAY
#define New(type) (type*)malloc(sizeof(type))
struct renderer {
	int data;
};

struct image {
    int data;
};

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
//,[wis_xdg_shell3]={ "zxdg_shell_v6", 3 }
,[	wis_xdg_base]={"xdg_wm_base",1 }
,[	wis_xdg_shell3]={"xdg_wm_base",3 }
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
  printf( "Screen geometry: %d %d  %d %d\n", x, y, physical_height, physical_width );
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
	if( !surface )  {
		lprintf( "pointer enter No Surface.... not sure how we can handle that" );
		return;
	}
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

	wl.mouse_.r = r;
	//EnqueData( &wl.pdlMouseEvents, &wl.mouse_ );
	//WakeThread( wl.drawThread );
}

static void pointer_leave(void *data,
	struct wl_pointer *wl_pointer, uint32_t serial,
	struct wl_surface *wl_surface) {
	if( !wl_surface ) return; // closed surface..
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);
	PXPANEL r = wl_surface_get_user_data(
		pointer_data->target_surface);
	//lprintf( "Mouse Leave" );
	wl.mouse_.b = 0;
	wl.mouse_.r = r;
	//EnqueData( &wl.pdlMouseEvents, &wl.mouse_ );
	//WakeThread( wl.drawThread );

 }

static void pointer_motion(void *data,
	struct wl_pointer *wl_pointer, uint32_t time,
	wl_fixed_t surface_x, wl_fixed_t surface_y) {
	struct pointer_data* pointer_data = data;//wl_pointer_get_user_data(wl_pointer);

   PXPANEL r = wl_surface_get_user_data(
        pointer_data->target_surface);


	{
	}
  	//lprintf( "mouse motion:%d %d", wl.mouse_.x, wl.mouse_.y );
	//if( !GetPixel( GetDipslaySurface( r ), wl.mouse.x, wl.mouse.y ))
		 //wl_seat_pointer_forward( wl.seat, serial );


}

static void pointer_button(void *data,
    struct wl_pointer *wl_pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state)
{
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
}

static void keyboard_leave(void *data,
		      struct wl_keyboard *wl_keyboard,
		      uint32_t serial,
		      struct wl_surface *surface){
}



static void keyboard_key(void *data,
		    struct wl_keyboard *wl_keyboard,
		    uint32_t serial,
		    uint32_t time,
		    uint32_t key,
		    uint32_t state)
{
	//lprintf( "KEY: %p %d %d %d %d", wl_keyboard, serial, time, key, state );

}


static void keyboard_modifiers(void *data,
			  struct wl_keyboard *wl_keyboard,
			  uint32_t serial,
			  uint32_t mods_depressed,
			  uint32_t mods_latched,
			  uint32_t mods_locked,
			  uint32_t group)
{
	xkb_state_update_mask (wl.xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);


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
global_registry_handler( void *data, struct wl_registry *registry, uint32_t id
                       , const char *interface, uint32_t version )
{
	static volatile int processing;
	//while( processing ) Relinquish();
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
				lprintf( "Interface %d:%s is version %d and library only supports %d (trying again)",
				      n, interfaces[n].name, version, interfaces[n].version );
				continue;
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
	} else if( n == wis_xdg_shell) {
       //wl.xdg_shell = wl_registry_bind(registry, id,
       //                          &, version);
	} else if( n == wis_xdg_base  || n == wis_xdg_shell3) {

		wl.xdg_wm_base = wl_registry_bind(registry, id
		                                 , &xdg_wm_base_interface, version);
		xdg_wm_base_add_listener( wl.xdg_wm_base, &xdg_wm_base_listener, NULL );

	} else if( n == wis_seat ) {
		wl.seat = wl_registry_bind( registry, id
		                          , &wl_seat_interface,  version>2?version:version);
		wl.pointer = wl_seat_get_pointer(wl.seat);
		wl.pointer_data.surface = wl_compositor_create_surface(wl.compositor);
		wl_pointer_add_listener(wl.pointer, &pointer_listener, &wl.pointer_data);
		wl.keyboard = wl_seat_get_keyboard(wl.seat );
		wl_keyboard_add_listener(wl.keyboard, &keyboard_listener, NULL );
	} else if( n == wis_shm ) {
		wl.shm = wl_registry_bind(registry, id
		                         , &wl_shm_interface, version);
		wl_shm_add_listener(wl.shm, &shm_listener, NULL);
	} else if( n == wis_output ) {
		struct output_data *out = New( struct output_data );
		//lprintf( "Display");
		out->wl_output = wl_registry_bind( registry, id, &wl_output_interface, version );
		// pending scaling
		out->pending_scale = 1;
		wl_output_set_user_data( out->wl_output, out );
		wl_output_add_listener( out->wl_output, &output_listener, out );
		wl.outputSurfaces[wl.nOutputSurfaces++] = out;
		//AddLink( &wl.outputSurfaces, out );

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
	wl.display = wl_display_connect(NULL);
	if (wl.display == NULL) {
		wl.flags.bFailed = 1;
		//lprintf( "Can't connect to display");
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

	//while( wl.registering )Relinquish(); // wait one more time, the last prior thing might take a bit.

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
//		*((int*)0)=0;
		//DebugBreak();
	}
	while( wl.registering )
	wl_display_roundtrip_queue(wl.display, wl.queue);

	if (!wl.compositor || !wl.canDraw ) {
		//*((int*)0)=0;
//		DebugBreak();
		lprintf( "Can't find compositor or no supported pixel format found.");
		return;
	} else {
		//lprintf( "Found compositor");
		
		// load a cursor
		wl.cursor_theme = wl_cursor_theme_load( NULL, 24, wl.shm );
		struct wl_cursor *cursor = wl_cursor_theme_get_cursor( wl.cursor_theme, "left_ptr");
		wl.cursor_image = cursor->images[0];
		struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer( wl.cursor_image );
		wl.cursor_surface = wl_compositor_create_surface( wl.compositor );
		lprintf( "Surface Attach (cursor)" );
		wl_surface_attach( wl.cursor_surface, cursor_buffer, 0, 0 );
		wl_surface_commit( wl.cursor_surface );
		wl_display_flush(wl.display);
		//wl_surface_flush( wl.cursor_surface);
	wl_display_roundtrip_queue(wl.display, wl.queue);
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

	static struct wl_shm_pool *pool;
	if( !pool ) pool  = wl_shm_create_pool(wl.shm, fd, size);
	r->buff = wl_shm_pool_create_buffer(pool, 0, /* starting offset */
					r->w, r->h,
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

static struct wl_buffer * nextBuffer( PXPANEL r, int attach ) {
	if( r->flags.hidden ) {
		lprintf( "window is hidden... returning a fault.");
		return NULL;
	}
	if( r->freeBuffer[r->curBuffer]  && ( r->bufw == r->w && r->bufh == r->h ) ) {
#if defined( DEBUG_COMMIT )
		lprintf( "Can just use the current image... it's already attached %d", r->curBuffer);
#endif
		return r->buff;
	}
	if( r->buffers[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES]
	  && !r->freeBuffer[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES] ) {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "buffers in use and nothing free, (no more commit!!) want buffer %d", r->curBuffer);
#endif
		r->flags.canCommit = 0;
		r->flags.wantBuffer = 1;
		return NULL;
	}
	r->curBuffer=(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES;
	int curBuffer = r->curBuffer;
#if defined( DEBUG_COMMIT )
	lprintf( "using image to a new image.... %d",curBuffer );
#endif
	if( r->bufw != r->w || r->bufh != r->h ) {
		// if the buffer has to change size, we need a new one....
		// maybe can keep this around to fill a copy of the prior window?
		//lprintf( "Remove Old Buffer");
		if( r->color_buffers[curBuffer] ) {
			munmap( r->color_buffers[curBuffer], r->buffer_sizes[curBuffer] );
			//UnmakeImageFile( r->buffer_images[curBuffer] );
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
		//r->buffer_images[curBuffer] = RemakeImage( r->buffer_images[curBuffer], r->shm_data, r->w, r->h );
		
		r->buffers[curBuffer] = r->buff;
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "Setting buffer (free) %d  %p", curBuffer, r->buff );
#endif
		r->color_buffers[curBuffer] = r->shm_data;
		r->freeBuffer[curBuffer] = 1; // initialize this as 'free' as in not commited(in use)
	}
	//r->pImage = RemakeImage( r->pImage, r->shm_data, r->w, r->h );
	//r->pImage->flags |= IF_FLAG_FINAL_RENDER|IF_FLAG_IN_MEMORY;
	return r->buff;
}


 void surfaceFrameCallback( void *data, struct wl_callback* callback, uint32_t time ) {
	PXPANEL r = (PXPANEL)data;
	// internal usage doesn't re-add callback... was stalled on buffer
	// after allocating a callback that's still valid.
#if defined( DEBUG_COMMIT )
	lprintf( "----------- surfaceFrameCallback (after surface is freed? no...) %d %p",r->curBuffer, r->buffers[r->curBuffer] );
#endif
	if( callback && r->frame_callback ) wl_callback_destroy( r->frame_callback );
	if( !r->flags.bDestroy ) {
		if( callback ) {
			// schedule another callback.
			// otherwise, this is just using the common 'r->flags.dirty' handler.
			r->frame_callback = wl_surface_frame( r->surface );
			wl_callback_add_listener( r->frame_callback, &frame_listener, r );
		}

		if( r->flags.dirty ){
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
			lprintf( "lock commit;LOCK BUFFER %d", r->curBuffer );
#endif
			struct wl_buffer *next = nextBuffer(r, 0);
			if( next ) {
				r->flags.canCommit = 0;
				if( r->flags.wantBuffer ){
#if defined( DEBUG_COMMIT_BUFFER )
					lprintf( "Buffers: %d %d %p %p", r->freeBuffer[0], r->freeBuffer[1], r->buffers[0], r->buffers[1]);
					lprintf( "NEED TO WAIT FOR A BUFFER or commit won't finish with a good attached buffer... right now there is a good buffer." );
#endif
					return;
				}
				if( !next ) lprintf( "auto clean can't be done... backing buffer is still in use...");
				r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
				// already wants to commit again...
				// clear dirty, still can't commit.
				r->flags.dirty = 0;
				r->flags.commited = 1;
				wl_surface_commit( r->surface );
				//wl_display_flush(wl.display);
#if defined( DEBUG_COMMIT )
				lprintf( "Window is (already) dirty, do commit");
#endif
		lprintf( "Surface Attach (frame)" );
				wl_surface_attach( r->surface, next, 0, 0 );
				//wl_surface_commit( r->surface );
			}
			//else wl_surface_commit( r->surface );

			//else lprintf( "No next buffer, can't commit already dirty thing yet." );
			// wait until we actually NEED the buffer, maybe we can use the same one.
		}else {
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
			lprintf( "Only Allow window to be commited at will.");
#endif
			r->flags.canCommit = 1;
		}
	}else {
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
		lprintf( "(DESTROYED)Allow window to be commited at will.");
#endif
		r->flags.canCommit = 1;
	}

}

static void sack_wayland_Redraw( PRENDERER renderer );
#if 0
static uintptr_t waylandDrawThread( PTHREAD thread ) {
	// need draws to happen on a 'draw' thread, which is NOT the application thread.
	//
	wl.drawThread = thread;
	while(1)
	{
		int sleepTime = 100;
		int newSleepTime;
		INDEX idx;
		struct drawRequest *req;
		struct pendingKey *key;
		PXPANEL r;
		uint32_t now = timeGetTime();
		struct mouseEvent event;

	}
}
#endif

static uintptr_t waylandThread( void )  {
//	wl.waylandThread = thread;
	//if( wl.display) DebugBreak();
	initConnections();
	if( wl.display ) {
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
	//lprintf( "shell configure %d %d", width, height );
	#if 0
	if( edges & wrsdv_left ){

	}
	if( edges & wrsdv_right ){

	}
	#endif
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
#if defined( DEBUG_COMMIT_BUFFER )
	lprintf( "------------------------ RELEASE BUFFER ----------------------------" );
#endif
	for( n = 0; n < MAX_OUTSTANDING_FRAMES; n++ )  {
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "RELEASE BUFFER FINDING BUFFER is %p %p?", r->buffers[n] , wl_buffer );
#endif
		if( r->buffers[n] == wl_buffer ) {
#if defined( DEBUG_COMMIT_BUFFER )
			lprintf( "UNLOCK Buffer %d is free again", n );
#endif
			if( r->freeBuffer[n] )
				lprintf( "!!!!!!!!!!!!! MULTIPLE EVENT ON SAME BUFFER ");
			if( n == ( (r->curBuffer+(MAX_OUTSTANDING_FRAMES-1))%MAX_OUTSTANDING_FRAMES ) )
			{
				INDEX idx;
				int minx = r->w;
				int miny = r->h;
				int maxx = 0;
				int maxy = 0;
				if( maxx  ) {
					lprintf( "update next damaged area %d %d %d %d", minx, miny, maxx-minx, maxy-miny );
					//BlotImageSizedTo( r->buffer_images[(r->curBuffer+1)%MAX_OUTSTANDING_FRAMES], r->buffer_images[r->curBuffer]
					//			, minx, miny, minx, miny, maxx-minx, maxy-miny );
				}
			}
			r->freeBuffer[n] = 1;
			if( r->flags.wantBuffer ){
				r->flags.wantBuffer = 0;
#if defined( DEBUG_COMMIT_BUFFER )
				lprintf( "~~~ This should be rare? a pending dirty commit couldn't get a buffer, it now has a buffer...");
#endif
				surfaceFrameCallback( r, NULL, 0 );
			}
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
	//wl_buffer_add_listener( r->buff, &buffer_listener, r );

#if defined( DEBUG_COMMIT_BUFFER )
	lprintf( "--- end of unlock buffer--");
#endif
}

LOGICAL CreateWindowStuff(PXPANEL r, PXPANEL parent )
{
	//lprintf( "-----Create WIndow Stuff----- %s %s", hVideo->flags.bLayeredWindow?"layered":"solid"
	//		 , hVideo->flags.bChildWindow?"Child(tool)":"user-selectable" );
	//DebugBreak();

	if (r->x == -1 || r->y == -1 )
	{
		if( r->x == -1 )
			r->x = 256;
		if( r->y == -1 )
			r->y = 128;
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
		wl_subsurface_set_position( r->sub_surface, r->x, r->y );
		wl_subsurface_set_desync( r->sub_surface );
		lprintf( "Created subsurface and attached it...(commit) %d %d", r->x, r->y );
		wl_surface_commit( parent->surface );

	} else {
		if(wl.shell && !wl.xdg_wm_base) {
			r->shell_surface = wl_shell_get_shell_surface(wl.shell, r->surface);
			wl_shell_surface_add_listener( r->shell_surface, &shell_surface_listener, r );
			wl_shell_surface_set_toplevel(r->shell_surface);
			wl_shell_surface_set_user_data(r->shell_surface, r);
		}

		if(wl.xdg_wm_base) {
			lprintf( "DID get XDG wm_base" );
			r->shell_surface = (struct wl_shell_surface*)xdg_wm_base_get_xdg_surface( wl.xdg_wm_base, r->surface );
			xdg_surface_add_listener( (struct xdg_surface*)r->shell_surface, &xdg_surface_listener, r );
			r->xdg_toplevel = xdg_surface_get_toplevel( (struct xdg_surface*)r->shell_surface );
			//xdg_toplevel_set_title(  r->xdg_toplevel, "I DOn't want a title");
			xdg_surface_set_user_data((struct xdg_surface*)r->shell_surface, r);
			// must commit to get a config
			// must also wait to get config.
			lprintf( "XDG? %p", r->shell_surface );
		}

		if (r->shell_surface == NULL) {
			lprintf("Can't create shell surface");
			return FALSE;
		}
	}
	{
		struct wl_buffer *next = nextBuffer(r, 0);
		if( next ) {
			r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
			r->flags.dirty = 0; // don't need a commit.
			wl_surface_commit( r->surface );
			wl_display_roundtrip_queue(wl.display, wl.queue);
        		wl_surface_attach( r->surface, next, 0, 0 );
			//lprintf( "DUring update display, do a commit right now" );
		}else {
			lprintf( "Don't have another surface ready to commit" );
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
	struct drawRequest* req = New( struct drawRequest );
	//PTHREAD thisThread = MakeThread();
	//if( wl.waylandThread != thisThread && wl.drawThread != thisThread )
	{
#if defined( DEBUG_COMMIT )
		lprintf( "(get new buffer)Issue redraw on renderer %p", renderer );
#endif
		r->flags.commited = 0;
		if( !r->flags.commited )
			if( r->flags.canCommit ) {
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
				lprintf( "Commiting redraw... ");
#endif

#if defined( DEBUG_COMMIT_BUFFER )
				lprintf( "LOCK BUFFER %d", r->curBuffer );
#endif
				r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
				struct wl_buffer *next = nextBuffer(r, 0);
				//r->flags.canCommit = 0;
				if( next ) {
					r->flags.dirty = 0;
					r->flags.commited = 1;
					lprintf( "In Redraw; which is what, a global callback? commit surface here.", r->surface );
                                        if( r->surface ) wl_surface_commit( r->surface );
                                        r->flags.canCommit = 0;
					//wl_display_flush(wl.display);
#if defined( DEBUG_ATTACH_SURFACE )
					lprintf( "attach %p", next );
#endif
					if( r->surface ) {
		lprintf( "Surface Attach (redraw)" );
						wl_surface_attach( r->surface, next, 0, 0 );
						//wl_surface_commit( r->surface );
						//wl_display_flush( wl.display );
					}
				}else {
					lprintf( "No Next buffer was available, we shouldn't actually commit yet..." );
				}
			}else {
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
				lprintf( "Can't commit, mark dirty" );
#endif
				r->flags.dirty = 1;
			}
		else {
			lprintf( "Was commited already" );
		}
		//if( r->flags.commited ){
			//lprintf( "A commit happened during redraw, wait for that to flush.");
			//wl_display_flush( wl.display );
			//wl_display_roundtrip_queue(wl.display, wl.queue);
		//}
	}
}


//#if 0
static void sack_wayland_SetApplicationTitle( char const *title ){

	//xdg_toplevel_set_title(  r->xdg_toplevel, "I DOn't want a title");

}


static void sack_wayland_SetDisplaySize( uint32_t w, uint32_t h ) {

}

static PRENDERER sack_wayland_OpenDisplayAboveUnderSizedAt(uint32_t attr , uint32_t w, uint32_t h, int32_t x, int32_t y, PRENDERER above, PRENDERER under ){
	struct wvideo_tag *rAbove = (struct wvideo_tag*)above;
	struct wvideo_tag *rUnder = (struct wvideo_tag*)under;
	struct wvideo_tag *r;
	r = New( struct wvideo_tag );
	memset( r, 0, sizeof( *r ) );
#if defined( DEBUG_COMMIT )
	lprintf( "Initialize with canCommit %d %d %d %d", x, y, w, h );
#endif
	r->flags.canCommit = 1;
	r->above = rAbove;
	r->under = rUnder;
	r->bufw = r->w = w;
	r->bufh = r->h = h;
	{
		struct wvideo_tag *parent = r->above;
		while( (parent ) && parent->sub_surface ) {
			x -= parent->x;
			y -= parent->y;
			lprintf( "adjust position:%d %d %d %d", x, y, parent->x, parent->y );
			parent = parent->above;
		}
	}
	r->x = x;
	r->y = y;

	// eventually we'll need a commit callback.

	//AddLink( &wl.pActiveList, r );
	#if 0
	while( !wl.flags.bInited && !wl.flags.bFailed ) {
		//lprintf( "Waiting for wayland setup...");
		Relinquish();
	}
	#endif
	if( wl.flags.bFailed ) return NULL;
 lprintf( "----------------- BEGIN CREATE WINDOW -----------------------" );
	if( !CreateWindowStuff( r, rAbove ) ) {
		lprintf( "Failed to create drawing surface... falling back to offscreen rendering?");
	}else {
		r->frame_callback = wl_surface_frame( r->surface );
		wl_callback_add_listener( r->frame_callback, &frame_listener, r );
		//wl_surface_commit( r->surface );
	}
	lprintf( "did we ever do a initial draw? Probably no" );

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
	if( r->sub_surface ) {
		wl_subsurface_destroy( r->sub_surface );
		lprintf( "destroy sub_surface... " );
	}
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
	r->surface = NULL;
	//DeleteLink( &wl.pActiveList, r );
	if( r->above ) {
		struct wvideo_tag *a = r->above;
		while( a->above ) a = a->above;
		wl_surface_commit( a->surface );
	}
}

static void sack_wayland_UpdateDisplayPortionEx(PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
//	lprintf( "UpdateDisplayProtionEx %p", r->surface );
	if( !r->surface ) return;
	wl_surface_damage( r->surface, x, y, w, h );
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
	lprintf( "Damaged area, and can we commit? %d", r->flags.canCommit );
#endif
	if( r->flags.canCommit ){
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
		lprintf( "Updating now - getting new buffer, commit, flush, roundtrip, and attach the new buffer");
#endif
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "LOCK BUFFER %d", r->curBuffer, r->buffers[r->curBuffer] );
#endif
		struct wl_buffer *next = nextBuffer(r, 0);
		if( next ) {
			r->flags.canCommit = 0;
			r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
			r->flags.dirty = 0; // don't need a commit.
			r->flags.commited = 1;
			if( !r->surface ) return;
			//lprintf( "DUring update display, do a commit right now" );
			wl_surface_commit( r->surface );
			wl_display_flush( wl.display );
			//if( wl.xdg_wm_base )
			//	wl_display_roundtrip_queue(wl.display, wl.queue);
#if defined( DEBUG_COMMIT )
			lprintf( "This should have a NEW buffer." );
			lprintf( "attach %p", next );
#endif
		lprintf( "Surface Attach (portion)" );

			wl_surface_attach( r->surface, next, 0, 0 );
			//wl_surface_commit( r->surface );
		}else {
			//wl_surface_commit( r->surface );
			lprintf( "Don't have another surface ready to commit" );
		}
	}else {
		r->flags.dirty = 1;
		lprintf( "update portion, was not ready yet, wait for callback");
	}
}

static void sack_wayland_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
//	lprintf( "update DIpslay Ex" );
	wl_surface_damage_buffer( r->surface, 0, 0, r->w, r->h );
	if( r->flags.canCommit ){
#if defined( DEBUG_COMMIT ) || defined( DEBUG_COMMIT_STATE )
		lprintf( "Updating now - getting new buffer, commit, flush, roundtrip, and attach the new buffer");
#endif
#if defined( DEBUG_COMMIT_BUFFER )
		lprintf( "LOCK BUFFER %d %p", r->curBuffer, r->buffers[r->curBuffer] );
#endif
		struct wl_buffer *next = nextBuffer(r, 0);
		if( next ) {
			r->flags.canCommit = 0;
			r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
			r->flags.dirty = 0; // don't need a commit.
			r->flags.commited = 1;
			if( !r->surface ) return;
			lprintf("Update commit" );
			wl_surface_commit( r->surface );
			//wl_display_flush( wl.display );
			//if( wl.xdg_wm_base )
			//	wl_display_roundtrip_queue(wl.display, wl.queue);

			lprintf( "attach (update) %p", next );
			wl_surface_attach( r->surface, next, 0, 0 );
			//wl_surface_commit( r->surface );
		}else {
			r->flags.dirty = 1;  // still needs a commit, when there's a replacement buffer available.
			//wl_surface_commit( r->surface );
			//lprintf( "couldn't commit yet, no next surface available" );
		}
	}else {
		r->flags.dirty = 1;
		lprintf( "update portion, was not ready yet, wait for callback");
	}
}


Image GetDisplayImage(PRENDERER renderer) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r ) {
		return NULL;
	}
	return NULL;
}


//#endif



int main( void ) {
    waylandThread( ) ;
     finishInitConnections( ) ;

    if(0)
    {
		while( wl_display_dispatch_queue(wl.display, wl.queue) != -1 ){
			//lprintf( ".... did some messages...");
		}

		lprintf( "Thread exiting?" );
		//wl_event_queue_destroy( wl.queue );
		wl_display_disconnect( wl.display );
    }
	 

    PRENDERER r = sack_wayland_OpenDisplaySizedAt( 0, 400,400, -1, -1 );
            wl_display_roundtrip_queue(wl.display, wl.queue);

        while( !r->flags.canCommit ) {
            usleep( 1000 );
            wl_display_roundtrip_queue(wl.display, wl.queue);
        }

        		wl_surface_damage( r->surface, 10, 10, 50, 50 );

    for( int i = 0; i < 5; i++ ) {
		printf( "------------------------ BEGIN %d FRAME ---------------------\n", i );
        r->freeBuffer[r->curBuffer] = 0; // lock this buffer.
        struct wl_buffer *next = nextBuffer(r, 0);
        //r->flags.canCommit = 0;
        if( next ) {
        	r->flags.dirty = 0;
        	r->flags.commited = 1;
                lprintf( "Draw Loop; commit surface here. %p", r->surface );
                //if( i )
                    r->flags.canCommit = 0;
                wl_surface_commit( r->surface );
        	//wl_display_flush(wl.display);
        	lprintf( "attach %p", next );
        	{
        		lprintf( "Surface Attach (redraw)" );
        		wl_surface_attach( r->surface, next, 0, 0 );
        		//wl_surface_commit( r->surface );
        		//if(i>2)
        		wl_surface_damage( r->surface, 20+i, 20, 50, 50 );
        		//wl_surface_commit( r->surface );
        		//wl_display_flush( wl.display );
        	}
        }else {
        	lprintf( "No Next buffer was available, we shouldn't actually commit yet..." );
        }
//        if(i)
        while( !r->flags.canCommit ) {
            wl_display_roundtrip_queue(wl.display, wl.queue);
            usleep( 10000 );
        }
    }
    
    return 0;
}
