#include <stdio.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "local.h"

static void pointer_enter(void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial, struct wl_surface *surface,
    wl_fixed_t surface_x, wl_fixed_t surface_y)
{

    struct client_state *state = data;
	printf( "setting pointer cursor?\n" );
	wl_pointer_set_cursor(wl_pointer, serial,
	state->wl_cursor_surface, state->pointer_data.hot_spot_x,
	    state->pointer_data.hot_spot_y);

}

static void pointer_leave( void* data,
	struct wl_pointer* wl_pointer, uint32_t serial,
	struct wl_surface* wl_surface ) {

 }

static void pointer_motion( void* data,

	struct wl_pointer* wl_pointer, uint32_t time,
	wl_fixed_t surface_x, wl_fixed_t surface_y ) {



}

static void pointer_button(void *data,
    struct wl_pointer *wl_pointer, uint32_t serial,
    uint32_t time, uint32_t button, uint32_t state)
{
	
}

static void pointer_axis(void *data,
    struct wl_pointer *wl_pointer, uint32_t time,
    uint32_t axis, wl_fixed_t value) {

	  }

static void pointer_frame( void *data,
    struct wl_pointer *wl_pointer) {
	  }

	static void pointer_axis_source(void *data,
			    struct wl_pointer *wl_pointer,
			    uint32_t axis_source){
				 }
	static void pointer_axis_stop(void *data,
			  struct wl_pointer *wl_pointer,
			  uint32_t time,
			  uint32_t axis){
			  }
	static void pointer_axis_discrete(void *data,
			      struct wl_pointer *wl_pointer,
			      uint32_t axis,
			      int32_t discrete){

					}


const struct wl_pointer_listener pointer_listener = {
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
