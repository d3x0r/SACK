// much core wayland interface code from...
// https://jan.newmarch.name/Wayland/ProgrammingClient/
// https://jan.newmarch.name/Wayland/WhenCanIDraw/
//

#define USE_IMAGE_INTERFACE wl.pii
#include <stdhdrs.h>
#include <render.h>
#include <image.h>
#include <idle.h>

#include <sys/mman.h>
#include <fcntl.h>

#include <wayland-client.h>
#include <linux/input-event-codes.h>

typedef struct wvideo_tag
{
	struct {
		uint32_t bShown : 1;
		uint32_t bDestroy : 1;
		uint32_t bFocused : 1;
		uint32_t dirty : 1;
		uint32_t canCommit : 1;
		uint32_t wantDraw : 1;
	} flags;

	int32_t x, y;
	uint32_t w, h;

	struct wvideo_tag* under;  // this window is under this reference
	struct wvideo_tag* above;  // this window is above the one pointed here

	struct wl_surface* surface;
	struct wl_shell_surface *shell_surface;
	struct wl_subsurface *sub_surface;
	PCOLOR shm_data;
	struct wl_buffer *buff;
	int curBuffer;
	uint32_t bufw, bufh;
	struct wl_buffer * buffers[2];
	PCOLOR  color_buffers[2];
	size_t buffer_sizes[2];
	Image buffer_images[2];
	struct wl_shm_pool *pool;
	struct wl_callback *frame_callback;  // request for WM_PAINT
	Image pImage;
	RedrawCallback pRedrawCallback;
	intptr_t dwRedrawData;

	MouseCallback mouse; uintptr_t psvMouse;
	LoseFocusCallback pLoseFocus;
	uintptr_t dwLoseFocus;

	CloseCallback pWindowClose;
	uintptr_t dwCloseData;

	KeyProc pKeyProc;
	uintptr_t dwKeyData;

	HideAndRestoreCallback pHideCallback;
	uintptr_t  dwHideData;

} XPANEL, *PXPANEL;

struct pointer_data {
    struct wl_surface *surface;
    struct wl_buffer *buffer;
    int32_t hot_spot_x;
    int32_t hot_spot_y;
    struct wl_surface *target_surface;
};

struct output_data {
	struct wl_output *wl_output;
	int32_t scale;
	int32_t pending_scale;
};

struct wayland_local_tag
{
	PTHREAD waylandThread;
	PIMAGE_INTERFACE pii;
	struct {
		volatile uint32_t bInited : 1;
	} flags;
	PLIST damaged;
	int dirty;
	int commit;

	struct wl_event_queue* queue;
	struct wl_display* display;
	struct wl_compositor* compositor;
	struct wl_subcompositor* subcompositor;
	struct wl_shm *shm;
	struct wl_shell *shell;
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct pointer_data pointer_data ;
	PLIST outputSurfaces;

	int screen_width;
	int screen_height;
	int screen_num;
/* these variables will be used to store the IDs of the black and white */
/* colors of the given screen. More on this will be explained later.    */
	unsigned long white_pixel;
	unsigned long black_pixel;
	struct mouse {
		int32_t x, y;
		uint32_t b;
	}mouse_;
	PXPANEL hVidFocused; // keyboard events go here
	PXPANEL hCaptured; // send all mouse events here (probalby should-no-op this)
	PLIST pActiveList;
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
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
    int32_t width, int32_t height, int32_t refresh)
{
  (void)data;
  (void)wl_output;
  (void)flags;
  (void)width;
  (void)height;
  (void)refresh;
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
}


static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    char *s;
    switch (format) {
    case WL_SHM_FORMAT_ARGB8888: s = "ARGB8888"; break;
    case WL_SHM_FORMAT_XRGB8888: s = "XRGB8888"; break;
    case WL_SHM_FORMAT_RGB565: s = "RGB565"; break;
    default: s = "other format"; break;
    }
    lprintf( "Possible shmem format %s", s);
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

	wl.mouse_.x = surface_x;
	wl.mouse_.y = surface_y;

    PXPANEL r = wl_surface_get_user_data(
        pointer_data->target_surface);
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

	 lprintf( "pointer button:%d %d", button, state);
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


static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{

	//lprintf("Got a registry event for %s id %d", interface, id);
	if( strcmp(interface, "wl_compositor") == 0 ) {
		wl.compositor = wl_registry_bind(registry,
				      id,
				      &wl_compositor_interface,
				      version);
	} else if( strcmp(interface, "wl_subcompositor") == 0 ) {
		wl.subcompositor = wl_registry_bind(registry,
				      id,
				      &wl_subcompositor_interface,
				      version);
	} else if( strcmp(interface, "zwp_linux_dmabuf_v1") == 0 ) {
		//lprintf( "Well, fancy that,we get DMA buffers?");
		// can video fullscreen
	} else if( strcmp(interface, "wl_shell") == 0 ) {
       wl.shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, version);
	} else if( strcmp(interface, "wl_seat") == 0 ) {
		wl.seat = wl_registry_bind(registry, id,
			&wl_seat_interface,  version>2?version:version);
		wl.pointer = wl_seat_get_pointer(wl.seat);
		wl.pointer_data.surface = wl_compositor_create_surface(wl.compositor);

		wl_pointer_add_listener(wl.pointer, &pointer_listener, &wl.pointer_data);

	} else if( strcmp(interface, "wl_shm") == 0 ) {
      wl.shm = wl_registry_bind(registry, id,
                                 &wl_shm_interface, version);
		wl_shm_add_listener(wl.shm, &shm_listener, NULL);
/*	} else if( strcmp(interface, "wl_output") == 0 ) {
		struct output_data *out = New( struct output_data );
		out->wl_output = wl_registry_bind( registry, id, &wl_output_interface, version );
		// pending scaling
		out->pending_scale = 1;
		wl_output_set_user_data( out->wl_output, out );
		wl_output_add_listener( out->wl_output, &output_listener, NULL )
		AddLink( &wl.outputSurfaces, out );
*/
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

}


static void finishInitConnections( void ) {


	struct wl_registry *registry = wl_display_get_registry(wl.display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_proxy_set_queue( (struct wl_proxy*)registry, wl.queue );


    wl_display_roundtrip_queue(wl.display, wl.queue);
    wl_registry_destroy(registry);

	if (!wl.compositor ) {
		lprintf( "Can't find compositor");
		return;
	} else {
		lprintf( "Found compositor");
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
	}
	return r->buff;
}

static void nextBuffer( PXPANEL r ) {

	r->curBuffer=1-r->curBuffer;
	if( r->bufw != r->w || r->bufh != r->h ) {
		if( r->color_buffers[r->curBuffer] ) {
			munmap( r->color_buffers[r->curBuffer], r->buffer_sizes[r->curBuffer] );
			UnmakeImageFile( r->buffer_images[r->curBuffer] );
			if( r->buffers[r->curBuffer] )
				wl_buffer_destroy( r->buffers[r->curBuffer] );
		}
		r->buffer_images[r->curBuffer] = NULL;
		r->buffers[r->curBuffer] = NULL;
		r->color_buffers[r->curBuffer] = NULL;
	}
	r->shm_data = r->color_buffers[r->curBuffer];
	r->buff = r->buffers[r->curBuffer];

	if( !r->buff ) {
		allocateBuffer(r);
		r->buffer_images[r->curBuffer] = RemakeImage( NULL, r->shm_data, r->w, r->h );
		r->buffers[r->curBuffer] = r->buff;
		r->color_buffers[r->curBuffer] = r->shm_data;
	}
	r->pImage = RemakeImage( r->pImage, r->shm_data, r->w, r->h );
	wl_surface_attach( r->surface, r->buff, 0, 0);
}


 void redraw( void *data, struct wl_callback* callback, uint32_t time ) {
	PXPANEL r = (PXPANEL)data;

	//lprintf( "And a commit after redraw callback... setup for next completion" );

	if( r->frame_callback ) wl_callback_destroy( r->frame_callback );
	r->frame_callback = wl_surface_frame( r->surface );
	wl_callback_add_listener( r->frame_callback, &frame_listener, r );

	if( r->flags.dirty ){
		r->flags.canCommit = 0;
		r->flags.dirty = 0;
		lprintf( "Window is dirty, do commit");

		wl_surface_commit( r->surface );
		nextBuffer(r);
	}else {
		lprintf( "Allow window to be commited at will.");
		r->flags.canCommit = 1;
	}
}


static uintptr_t waylandThread( PTHREAD thread ) {
	wl.waylandThread = thread;
	lprintf( "Start event thread on display?");
	initConnections();
	lprintf( "-------- WAIT ------------");
	while( wl_display_dispatch_queue(wl.display, wl.queue) != -1 ) {
		//lprintf( "Handled an event." );
		if( wl.commit ) {
			INDEX idx;
			int flush = 0;
			PXPANEL r;
			wl.commit = 0;
			LIST_FORALL( wl.damaged, idx, PXPANEL, r ){
				lprintf( "Damage found... send %p", r );
				if( !r->flags.bDestroy ){
					if( r->flags.canCommit ) {
						flush++;
						r->flags.dirty = 0;
						r->flags.canCommit = 0;
						wl_surface_commit( r->surface );
					}else {
						lprintf( "frame callback still waiting?");
						// still waiting... next commit event will catch this.
					}
				}
			}
			EmptyList( &wl.damaged );
			if( flush ) {
				lprintf( "And then even try flush?" );
				wl_display_flush( wl.display );
			}
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
	if( !x ) x = &j1;
	if( !y ) y = &j1;
	if( !w ) w = &j2;
	if( !h ) h = &j2;


}


static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
							uint32_t serial)
{
	//PXPANEL r = (PXPANEL)data;
   wl_shell_surface_pong(shell_surface, serial);
   lprintf("Pinged and ponged");
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
		 uint32_t edges, int32_t width, int32_t height)
{
	PXPANEL r = (PXPANEL)data;
	lprintf( "shell configure");
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
	lprintf( "Buffer is released, you can draw now..." );
	//if( !)
	//r->surface
	wl_surface_attach(r->surface, r->buff, 0, 0);

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
	lprintf( "Okay still need a actual drawing surface here:" );
   r->surface = wl_compositor_create_surface(wl.compositor);
   if (r->surface == NULL) {
		lprintf( "Can't create surface");
		return FALSE;
   }

	if( parent ) {
		r->sub_surface = wl_subcompositor_get_subsurface( wl.subcompositor, r->surface, parent->surface );
	}else {
		r->shell_surface = wl_shell_get_shell_surface(wl.shell, r->surface);
		if (r->shell_surface == NULL) {
			lprintf("Can't create shell surface");
			return FALSE;
		}
		wl_shell_surface_add_listener( r->shell_surface, &shell_surface_listener, r );
		wl_shell_surface_set_toplevel(r->shell_surface);
	}

   wl_shell_surface_set_user_data(r->shell_surface, r);
   wl_surface_set_user_data(r->surface, r);

	nextBuffer(r);

	return TRUE;
}

int dispatchMessages( uintptr_t psv ){
	if( MakeThread() != wl.waylandThread ) return -1; // not for this thread...
	int n = wl_display_dispatch(wl.display);
	if( n == 0 ) return 0;
	if( n > 1 ) return 1;
}

int InitDisplay( void )
{
	if( !wl.flags.bInited ) {
		//AddIdleProc( dispatchMessages, 0 );
		//wl.waylandThread = MakeThread();
		//initConnections();
		//wl.flags.bInited = 1;
		ThreadTo( waylandThread, 0 );
		while( !wl.flags.bInited) {
			Relinquish();
		}
		finishInitConnections();

	}
}

void ShutdownVideo( void )
{
	if( wl.flags.bInited )
	{
	}
}


static void sack_wayland_Redraw( PRENDERER renderer ) {
	PXPANEL r = (PXPANEL)renderer;
	lprintf( "Issue redraw on renderer %p", renderer );
	r->pRedrawCallback( r->dwRedrawData, (PRENDERER)r  );

	wl_display_roundtrip_queue(wl.display, wl.queue);
	lprintf( "------ TICK-------------");
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
	r->flags.canCommit = 1;
	r->above = rAbove;
	r->under = rUnder;
	r->w = w;
	r->h = h;
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
	r->flags.bDestroy = 1;
	if( r->sub_surface )
		wl_subsurface_destroy( r->sub_surface );
	if( r->shell_surface )
		wl_shell_surface_destroy( r->shell_surface );
	wl_surface_destroy( r->surface );
	DeleteLink( &wl.pActiveList, r );

}

static void sack_wayland_UpdateDisplayPortionEx(PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h DBG_PASS){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	//LogBinary( r->buff, 1024 );
	r->flags.dirty = 1;
	//lprintf( "Update whole surface %d %d", r->w, r->h );
	//wl_surface_damage_buffer( r->surface, 0, 0, r->w, r->h );

	//lprintf( "Update portion: %d %d  %d %d", x, y, w, h );
	//wl_surface_damage( r->surface, x, y, w, h );
	if( r->buffer_images[r->curBuffer] && r->buffer_images[1-r->curBuffer] ) {
		BlotImageSizedTo( r->buffer_images[1-r->curBuffer], r->buffer_images[r->curBuffer], x, y, x, y, w, h );
	}

	{
		INDEX i = FindLink( &wl.damaged, r );
		if( i == INVALID_INDEX )
			AddLink( &wl.damaged, r );
		 wl.commit= 1;
	}
}

static void sack_wayland_UpdateDisplayEx( PRENDERER renderer DBG_PASS ) {
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	lprintf( "Update whole surface %d %d", r->w, r->h );
	wl_surface_damage_buffer( r->surface, 0, 0, r->w, r->h );
	if( r->buffer_images[r->curBuffer] && r->buffer_images[1-r->curBuffer] ) {
		BlotImage( r->buffer_images[1-r->curBuffer], r->buffer_images[r->curBuffer], 0, 0 );
	}
	r->flags.dirty = 1;
	{
		INDEX i = FindLink( &wl.damaged, r );
		if( i == INVALID_INDEX )
			AddLink( &wl.damaged, r );
		 wl.commit= 1;
	}
}

static void sack_wayland_GetDisplayPosition( PRENDERER renderer, int32_t* x, int32_t* y, uint32_t* w, uint32_t* h ){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
}

static void sack_wayland_MoveDisplay(PRENDERER renderer, int32_t x, int32_t y){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
	if( r->above ){
		lprintf( "MOVE SUBSURFACE" );
		wl_subsurface_set_position( r->sub_surface, x, y );
	}
}
static void sack_wayland_MoveDisplayRel(PRENDERER renderer, int32_t dx, int32_t dy){
	struct wvideo_tag *r = (struct wvideo_tag*)renderer;
		lprintf( "MOVE DISPLAYREL" );

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
	lprintf( "CLOSE HANDLER FIX");
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
	if( r ) {
		if( !r->pImage )
			r->pImage = RemakeImage( r->pImage, NULL, 0, 0 );

		return r->pImage;
	}
	return NULL;
}

static void sack_wayland_OwnMouseEx ( PRENDERER display, uint32_t bOwn DBG_PASS ){
	struct wvideo_tag *r = (struct wvideo_tag*)display;
	_lprintf( DBG_RELAY )( "Own Mouse - probably for move/drag... %p %d", display, bOwn );
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



static void sack_wayland_GetMousePosition ( int32_t *x, int32_t *y ){
	(*x) = wl.mouse_.x;
	(*y) = wl.mouse_.y;
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
													, sack_wayland_GetMousePosition// (void (CPROC*)(int32_t *, int32_t *)) GetMousePosition
													, NULL//(void (CPROC*)(PRENDERER, int32_t, int32_t)) SetMousePosition
													, NULL//HasFocus  // has focus
													, NULL//GetKeyText
													, NULL//IsKeyDown
													, NULL//KeyDown
													, NULL//DisplayIsValid
													, sack_wayland_OwnMouseEx
													, NULL//BeginCalibration
													, NULL//SyncRender	// sync
													, NULL//MoveSizeDisplay
													, NULL//MakeTopmost
													, sack_wayland_HideDisplay
													, sack_wayland_RestoreDisplay
													, sack_wayland_ForceDisplayFocus
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

