#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include "xdg-shell-client-protocol.h"

#include "local.h"

/* Shared memory support code */
static void
randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

static int
create_shm_file(void)
{
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int
allocate_shm_file(size_t size)
{
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* Wayland code */

struct client_state state = { 0 };

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    /* Sent by the compositor when it's no longer using this buffer */
	printf( "Destroy buffer?\n" );
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

uint32_t* buffer_data;
struct wl_buffer* buffers[2];
int last_buffer;

static struct wl_buffer *
draw_frame(struct client_state *state, int mode)
{
	static int last_mode;
    const int width = 640, height = 480;
    int stride = width * 4;
    int size = stride * height;
	 if( mode == 0 ) mode = last_mode;
	 else last_mode = mode;
	 uint32_t* data;
	 struct wl_buffer* buffer;
	 if( !buffer_data ) {
		 int fd = allocate_shm_file(size);
		 if (fd == -1) {
			  return NULL;
		 }

		 data = mmap(NULL, size,
					PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		 buffer_data = data;
		 if (data == MAP_FAILED) {
			  close(fd);
			  return NULL;
		 }

		 struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
		 buffer = wl_shm_pool_create_buffer(pool, 0,
					width, height, stride, WL_SHM_FORMAT_XRGB8888);
		 buffers[0] = buffer;
		 wl_shm_pool_destroy(pool);
		 close(fd);
		 wl_buffer_add_listener( buffer, &wl_buffer_listener, NULL );
	 } else {
		 data = buffer_data;
		 buffer = buffers[0];
	 }
	 switch( mode ) {
	 case 0:
		 /* Draw checkerboxed background */
		 for (int y = 0; y < height; ++y) {
			  for (int x = 0; x < width; ++x) {
					if ((x + y / 8 * 8) % 16 < 8)
						 data[y * width + x] = 0xFF666666;
					else
						 data[y * width + x] = 0xFFEEEEEE;
			  }
		 }
		 break;
	 case 1:
		 {
			 uint32_t * here = data;
			 for( int y = 0; y < height; ++y ) {
				 for( int x = 0; x < width; ++x ) {
					 (here++)[0] = 0xa0FF0000;
				 }
			 }
		 }
		 break;
	 case 2:
		 {
			 uint32_t* here = data;
			 for( int y = 0; y < height; ++y ) {
				 for( int x = 0; x < width; ++x ) {
					 ( here++ )[0] = 0xa000FF00;
				 }
			 }
		 }
		 break;

	 }

    //munmap(data, size);
	 printf( "done..\n" );
    return buffer;
}

static void
xdg_surface_configure(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    struct wl_buffer *buffer = draw_frame(state, 0);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

void drawGreen( void ) {
	printf( "draw green\n" );
	struct wl_buffer* buffer = draw_frame( &state, 1 );
	wl_surface_damage_buffer( state.wl_surface, 0, 0, INT32_MAX, INT32_MAX );
	wl_surface_attach( state.wl_surface, buffer, 0, 0 );
	wl_surface_commit( state.wl_surface );
	wl_display_roundtrip( state.wl_display );

}

void drawRed( void ) {
	printf( "draw red\n" );
	wl_surface_damage_buffer( state.wl_surface, 0, 0, INT32_MAX, INT32_MAX );
	struct wl_buffer* buffer = draw_frame( &state, 2 );
	wl_surface_attach( state.wl_surface, buffer, 0, 0 );
	wl_surface_commit( state.wl_surface );
	wl_display_roundtrip( state.wl_display );

}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void
registry_global(void *data, struct wl_registry *wl_registry,
        uint32_t name, const char *interface, uint32_t version)
{
    struct client_state *state = data;

	 if( strcmp( interface, "wl_seat" ) == 0 ) {
		 extern const struct wl_keyboard_listener keyboard_listener;
		 extern const struct wl_pointer_listener pointer_listener;

		 state->wl_seat = wl_registry_bind( wl_registry, name
			 , &wl_seat_interface, version > 2 ? version : version );
		 //wl_proxy_set_queue( (struct wl_proxy*)state->wl_seat, state->wl_queue );
		 state->wl_pointer = wl_seat_get_pointer( state->wl_seat );
		 //wl_proxy_set_queue( (struct wl_proxy*)state->wl_pointer, state->wl_queue );
		 state->pointer_data.surface = wl_compositor_create_surface( state->wl_compositor );
		 wl_pointer_add_listener( state->wl_pointer, &pointer_listener, state );
		 state->wl_keyboard = wl_seat_get_keyboard( state->wl_seat );
		 //wl_proxy_set_queue( (struct wl_proxy*)state->wl_keyboard, state->wl_queue );
		 wl_keyboard_add_listener( state->wl_keyboard, &keyboard_listener, state );
	 }

    if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->wl_shm = wl_registry_bind(
                wl_registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor = wl_registry_bind(
                wl_registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(
                wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base,
                &xdg_wm_base_listener, state);
    }
}

static void
registry_global_remove(void *data,
        struct wl_registry *wl_registry, uint32_t name)
{
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

int
main(int argc, char *argv[])
{
    state.wl_display = wl_display_connect(NULL);
    state.wl_registry = wl_display_get_registry(state.wl_display);
    wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    wl_display_roundtrip(state.wl_display);

    state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(
            state.xdg_wm_base, state.wl_surface);
    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Example client");
    wl_surface_commit(state.wl_surface);

		state.wl_cursor_theme = wl_cursor_theme_load( NULL, 24, state.wl_shm );
		if( state.wl_cursor_theme ) {
			struct wl_cursor *cursor = wl_cursor_theme_get_cursor( state.wl_cursor_theme, "left_ptr");
			state.wl_cursor_image = cursor->images[0];
			struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer( state.wl_cursor_image );
			state.wl_cursor_surface = wl_compositor_create_surface( state.wl_compositor );
#if defined( DEBUG_SURFACE_ATTACH )
			lprintf( "Cursor surface attach." );
#endif
			wl_surface_attach( state.wl_cursor_surface, cursor_buffer, 0, 0 );
#if defined( DEBUG_SURFACE_ATTACH )
			lprintf( "Commiting cursor attach." );
#endif
			wl_surface_commit( state.wl_cursor_surface );
			wl_display_flush( state.wl_display );
		} else printf( "Naive theme check failed...\n" );


    while (wl_display_dispatch(state.wl_display)) {
        /* This space deliberately left blank */
    }

    return 0;
}
