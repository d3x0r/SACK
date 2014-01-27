
#include <network.h>
#include "protocol.h"

typedef struct vidlib_proxy_image
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
	P_8 draw_command_buffer;
	
	INDEX filegroup;
	TEXTSTR filename;
	Image image;
} *PVPImage;

typedef struct vidlib_proxy_renderer
{
	_32 w, h;
	S_32 x, y;
	_32 attributes;
	struct vidlib_proxy_renderer *above, *under;
	PLIST remote_render_id;  // this is synced with same index as l.clients
	Image image;  // representation of the output surface
	struct vidlib_proxy_renderer_flags
	{
		BIT_FIELD hidden : 1;
	} flags;
} *PVPRENDER;

struct server_proxy_client
{
	PCLIENT pc;
	LOGICAL websock;
};

struct vidlib_proxy_local
{
	PCLIENT listener;
	PCLIENT web_listener;
	PLIST clients; // list of struct server_proxy_client
	TEXTSTR application_title;
	PLIST renderers;
	PLIST web_renderers;
	PLIST images;
	struct json_context *json_context;
	PLIST messages;
	PIMAGE_INTERFACE real_interface;
} l;


