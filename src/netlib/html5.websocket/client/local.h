
#include <http.h>
#include <html5.websocket.client.h>

struct web_socket_client
{
	struct web_socket_client_flags
	{
		BIT_FIELD connected : 1; // if not connected, then parse data as http, otherwise process as websock protocol.
		BIT_FIELD closed : 1; // may get link connected, and be waiting for a connected status, but may not get connected
		BIT_FIELD want_close : 1; // schedule to close

		BIT_FIELD fragment_collecting : 1;
		BIT_FIELD sent_type : 1;
		BIT_FIELD received_pong : 1;
		BIT_FIELD sent_ping : 1;
	} flags;
	PCLIENT pc;

	CTEXTSTR host;
	CTEXTSTR address_url;
	struct url_data *url;

	web_socket_opened on_open;
	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_error on_error;
	PTRSZVAL psv_on;

	POINTER buffer;
	HTTPState pHttpState;

	size_t fragment_collection_avail;
	size_t fragment_collection_length;
	size_t fragment_collection_index;  // used for selecting mask byte
	P_8 fragment_collection;

	_32 last_reception; // for automatic ping/keep alive/idle death
   _32 ping_delay; // when set by enable auto_ping is the delay between packets to generate a ping

	LOGICAL final;
	LOGICAL mask;
	_8 mask_key[4];
	int opcode;
	size_t frame_length;
	int input_msg_state;
	int input_type; // text or binary
};


struct web_socket_client_local
{
   _32 timer;
	PLIST clients;
   CRITICALSECTION cs_opening;
	struct web_socket_client *opening_client;
   int newmask;
} wsc_local;
