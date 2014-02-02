
#include <network.h>
#include <image3d.h>
#include "protocol.h"

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
	PLIST renderers;
	PLIST images;
} proxy_client_local;


