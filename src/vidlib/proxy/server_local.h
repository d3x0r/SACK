
#include <network.h>
#include "protocol.h"

struct vidlib_proxy_image
{
	int x, y, w, h;
	int string_mode;
	int blot_method;
	PCDATA custom_buffer;
	PLIST remote_image_id;
	struct vidlib_proxy_image *parent;
	struct vidlib_proxy_image *child;
	struct vidlib_proxy_image *next;
	struct vidlib_proxy_image *prior;
	PLINKQUEUE draw_commands;

	size_t available;
	size_t used;
	P_8 draw_commands;

} *PVPImage;

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
	PCLIENT web_listener;
	PLIST clients;
	TEXTSTR application_title;
	PLIST renderers;
	PLIST web_renderers;
	PLIST images;
	struct json_context *json_context;
} l;


