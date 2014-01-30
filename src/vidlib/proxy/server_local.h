
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
	INDEX id;

	P_8 buffer;
	size_t sendlen;
	size_t buf_avail;
	P_8 websock_buffer;
	size_t websock_sendlen;
	size_t websock_buf_avail;
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
	INDEX id;
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


