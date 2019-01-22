#ifndef SACK_HTML5_WEBSOCKET_COMMON_DEFINED
#define SACK_HTML5_WEBSOCKET_COMMON_DEFINED

#include <network.h>
#include <html5.websocket.client.h>
#ifdef HAVE_ZLIB
#  include <zlib.h>
#else
#  define __NO_WEBSOCK_COMPRESSION__
#endif
#include <http.h>

#ifndef WEBSOCKET_COMMON_SOURCE
#define EXTERN extern
#else
#define EXTERN
#endif

typedef struct web_socket_input_state *WebSocketInputState;

struct web_socket_input_state
{
	struct web_socket_common_flags
	{
		BIT_FIELD closed : 1;
		BIT_FIELD received_pong : 1;
		BIT_FIELD sent_ping : 1;
		BIT_FIELD deflate : 1; // set on server if client requested permessage-deflate; can be overridden by SetWebSocketDeflate() during accept
		BIT_FIELD do_not_deflate : 1;  // do not deflate outbound messages; inbound message might still be inflated
		BIT_FIELD sent_type : 1;
		// apparently clients did not implement back masking??
		// I get a close; probably because of the length exception
		BIT_FIELD expect_masking : 1;
		BIT_FIELD use_ssl : 1;
	} flags;
	uint32_t last_reception; // (last message tick) for automatic ping/keep alive/idle death

#ifndef __NO_WEBSOCK_COMPRESSION__
	int client_max_bits;  // max bits used on (deflater if server, inflater if client)
	int server_max_bits;  // max bits used on (inflater if server, deflater if client)
	z_stream deflater;
	POINTER deflateBuf;
	size_t deflateBufLen;
	z_stream inflater;
	POINTER inflateBuf;
	size_t inflateBufLen;
	size_t inflateBufUsed;
#endif
	// expandable buffer for collecting input messages from client.
	size_t fragment_collection_avail;
	size_t fragment_collection_length;
	size_t fragment_collection_index;  // used for selecting mask byte
	size_t fragment_collection_buffer_size;
	uint8_t* fragment_collection;

	LOGICAL final;
	LOGICAL mask;
	uint8_t mask_key[4];
	int opcode;
	int RSV1;  // input bit of RSV1 bit on each message
	int _RSV1; // input bit of the first RSV1 bit on a fragmented packet; used for permessage-deflate
	int input_msg_state;
	int input_type; // text or binary
	size_t frame_length;

	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_http_close on_http_close;
	web_socket_opened on_open;
	web_socket_error on_error;
	web_socket_accept on_accept;  // server socket event
	web_socket_http_request on_request;
	web_socket_completion on_fragment_done;
	uintptr_t psv_on;
	uintptr_t psv_open; // result of the open, to pass to read
	int close_code;
	char *close_reason;
};

EXTERN void SendWebSocketMessage( PCLIENT pc, int opcode, int final, int do_mask, const uint8_t* payload, size_t length, int use_ssl );
EXTERN void ProcessWebSockProtocol( WebSocketInputState websock, PCLIENT pc, const uint8_t* msg, size_t length );


struct html5_web_socket {
	uint32_t Magic; // this value must be 0x20130912
	struct web_socket_flags
	{
		BIT_FIELD initial_handshake_done : 1;
		BIT_FIELD rfc6455 : 1;
		BIT_FIELD accepted : 1;
		BIT_FIELD http_request_only : 1;
		BIT_FIELD in_open_event : 1; // set when sent to client, which can write and close before return; no further read must be done.
		BIT_FIELD closed : 1; // was already closed (during in_read_event)
	} flags;
	HTTPState http_state;
	PCLIENT pc;
	POINTER buffer;
	char *protocols;

	struct web_socket_input_state input_state;
};

struct web_socket_client
{
	uint32_t Magic; // this value must be 0x20130911
	struct web_socket_client_flags
	{
		BIT_FIELD connected : 1; // if not connected, then parse data as http, otherwise process as websock protocol.
		BIT_FIELD want_close : 1; // schedule to close
		//BIT_FIELD use_ssl : 1;
	} flags;
	PCLIENT pc;

	CTEXTSTR host;
	CTEXTSTR address_url;
	struct url_data *url;
	CTEXTSTR protocols;

	POINTER buffer;
	HTTPState pHttpState;

	uint32_t ping_delay; // when set by enable auto_ping is the delay between packets to generate a ping

	struct web_socket_input_state input_state;
};


#endif
