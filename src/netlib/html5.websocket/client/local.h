
#include <http.h>
#include <html5.websocket.client.h>

typedef struct web_socket_client *WebSocketClient;

struct web_socket_client
{
   _32 Magic; // this value must be 0x20130911
	struct web_socket_client_flags
	{
		BIT_FIELD connected : 1; // if not connected, then parse data as http, otherwise process as websock protocol.
		BIT_FIELD want_close : 1; // schedule to close
	} flags;
	PCLIENT pc;

	CTEXTSTR host;
	CTEXTSTR address_url;
	struct url_data *url;

	POINTER buffer;
	HTTPState pHttpState;

	_32 ping_delay; // when set by enable auto_ping is the delay between packets to generate a ping

	struct web_socket_input_state input_state;
   struct web_socket_output_state output_state;
};


struct web_socket_client_local
{
   _32 timer;
	PLIST clients;
   CRITICALSECTION cs_opening;
	struct web_socket_client *opening_client;
} wsc_local;
