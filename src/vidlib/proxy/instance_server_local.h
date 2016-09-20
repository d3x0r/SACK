
#include <network.h>
#include "protocol.h"



typedef struct vidlib_proxy_image
{
	int x, y, w, h;
	int string_mode;
	int blot_method;
	struct vidlib_proxy_image *parent;
	struct vidlib_proxy_image *child;
	struct vidlib_proxy_image *next;
	struct vidlib_proxy_image *prior;

	INDEX filegroup;
	TEXTSTR filename;
	Image image;
	INDEX render_id;
	INDEX id;

	struct vidlib_proxy_image_flags {
		BIT_FIELD bUsed : 1;
	} flags;

	uint8_t* buffer;
	size_t sendlen;
	size_t buf_avail;
	uint8_t* websock_buffer;
	size_t websock_sendlen;
	size_t websock_buf_avail;
} *PVPImage;

typedef struct vidlib_proxy_application
{
	PLIST renderers;
	PLIST images;
	TEXTSTR client_id; // client identification... arbitrary length data
	PLIST local_instances;
	struct server_socket_state *network_state;
} *PVP_APPLICATION;

typedef struct vidlib_proxy_application_instance
{
	struct vidlib_proxy_application_local **ppLocal;
	struct vidlib_proxy_application_local *pLocal; 
} *PVP_APPLICATION_INSTANCE;

struct server_socket_state
{
	struct server_socket_flags {
		BIT_FIELD get_length : 1; // read is for next message length
	} flags;
	POINTER buffer;
	int read_length;
	LOGICAL websock;
	PCLIENT pc;
	PLINKQUEUE pending_operations; // hold draws until the frame is shown for early draw
	PVP_APPLICATION application_instance;
};


#if HAS_TLS
DeclareThreadLocal struct my_network_state_info {
	struct server_socket_state *app;
	struct {
		BIT_FIELD during_connect : 1;
	} flags;
	PTHREAD pThread;
	THREAD_ID nThread;
} ThreadNetworkState;
#else
#error requires thread local storage.
#endif

typedef struct vidlib_proxy_renderer
{
	uint32_t w, h;
	int32_t x, y;
	uint32_t attributes;
	struct vidlib_proxy_renderer *above, *under;
	PLIST remote_render_id;  // this is synced with same index as l.clients
	struct vidlib_proxy_image *image;  // representation of the output surface
	struct vidlib_proxy_renderer_flags
	{
		BIT_FIELD hidden : 1;
		BIT_FIELD open : 1;
		BIT_FIELD did_first_mouse : 1;
		BIT_FIELD pending_mouse : 1;
		BIT_FIELD consume_mouse : 1;  // set to consume mouse events... otherwise we have to dispatch them.
		BIT_FIELD closed : 1;
	} flags;
	INDEX id;
	MouseCallback mouse_callback;
	uintptr_t psv_mouse_callback;
	KeyProc key_callback;
	uintptr_t psv_key_callback;
	RedrawCallback redraw;
	uintptr_t psv_redraw;
	uint32_t _b;
	int32_t mouse_x, mouse_y;
} *PVPRENDER;



#define l vidlib_proxy_server_local
struct vidlib_proxy_local
{
	PCLIENT listener;
	PCLIENT web_listener;
	PLIST clients; // list of struct server_proxy_client
	PLIST applications; // application instance may be reconnected
	TEXTSTR application_title;
	struct json_context *json_context;
	struct json_context *json_reply_context; // shorter list to search for input messages
	PLIST messages;   // json message formats
	PIMAGE_INTERFACE real_interface;
	uint8_t key_states[256];
	CRITICALSECTION message_formatter;
} l;

void InvokeNewDisplay( struct vidlib_proxy_application * );


CTEXTSTR SACK_Vidlib_GetKeyText( int pressed, int key_index, int *used );
void SACK_Vidlib_ProcessKeyState( int pressed, int key_index, int *used );


