
#include <network.h>
#include "protocol.h"

struct vidlib_proxy_renderer
{
	_32 w, h;
	S_32 x, y;
	_32 attributes;
   PLIST remote_render_id;  // this is synced with same index as l.clients
} *PVPRENDER;

struct vidlib_proxy_local
{
	PCLIENT listener;
	PLIST clients;
	TEXTSTR application_title;
   PLIST renderers;
} l;


