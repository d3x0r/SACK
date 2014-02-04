
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
	INDEX render_id;
	INDEX id;

	struct vidlib_proxy_image_flags {
		BIT_FIELD bUsed : 1;
	} flags;

	P_8 buffer;
	size_t sendlen;
	size_t buf_avail;
	P_8 websock_buffer;
	size_t websock_sendlen;
	size_t websock_buf_avail;
} *PVPImage;

struct server_socket_state
{
	struct server_socket_flags {
		BIT_FIELD get_length : 1;
	} flags;
	POINTER buffer;
	int read_length;
};

typedef struct vidlib_proxy_renderer
{
	_32 w, h;
	S_32 x, y;
	_32 attributes;
	struct vidlib_proxy_renderer *above, *under;
	PLIST remote_render_id;  // this is synced with same index as l.clients
	struct vidlib_proxy_image *image;  // representation of the output surface
	struct vidlib_proxy_renderer_flags
	{
		BIT_FIELD hidden : 1;
	} flags;
	INDEX id;
	MouseCallback mouse_callback;
	PTRSZVAL psv_mouse_callback;
	KeyProc key_callback;
	PTRSZVAL psv_key_callback;
	RedrawCallback redraw;
	PTRSZVAL psv_redraw;
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
	struct json_context *json_reply_context; // shorter list to search for input messages
	PLIST messages;
	PIMAGE_INTERFACE real_interface;
	_8 key_states[256];
} l;


