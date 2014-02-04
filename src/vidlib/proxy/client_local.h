
#include <network.h>
#include <image3d.h>
#include "protocol.h"

struct client_proxy_render
{
	PCLIENT pc;
	PRENDERER render;
	INDEX id;
	struct client_proxy_render_flags
	{
		BIT_FIELD opening : 1;  // mostly used when still opening to skip redraw event generation.
	} flags;
	PLIST images;
};

struct client_proxy_image
{
	Image image;
	INDEX render_id;

	P_8 buffer;
	size_t sendlen;
	size_t buf_avail;
};

struct client_socket_state
{
	struct client_socket_flags {
		BIT_FIELD get_length : 1;
	} flags;
	POINTER buffer;
	int read_length;
};

struct vidlib_proxy_local
{
	PCLIENT service;
	PTHREAD main;
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;
	PRENDER3D_INTERFACE pr3i;
	PIMAGE_3D_INTERFACE pi3i;
	PLIST renderers;  // list of PRENDERER
	PLIST images;  // list of struct client_socket_state
} proxy_client_local;
#define l proxy_client_local

