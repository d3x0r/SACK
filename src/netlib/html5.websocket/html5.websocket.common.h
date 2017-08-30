#ifndef SACK_HTML5_WEBSOCKET_COMMON_DEFINED
#define SACK_HTML5_WEBSOCKET_COMMON_DEFINED

#include <network.h>
#include <html5.websocket.client.h>
#include <zlib.h>

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

	int client_max_bits;  // max bits used on (deflater if server, inflater if client)
	int server_max_bits;  // max bits used on (inflater if server, deflater if client)
	z_stream deflater;
	POINTER deflateBuf;
	size_t deflateBufLen;
	z_stream inflater;
	POINTER inflateBuf;
	size_t inflateBufLen;
	size_t inflateBufUsed;

	// expandable buffer for collecting input messages from client.
	size_t fragment_collection_avail;
	size_t fragment_collection_length;
	size_t fragment_collection_index;  // used for selecting mask byte
	uint8_t* fragment_collection;

	uint32_t last_reception; // (last message tick) for automatic ping/keep alive/idle death

	LOGICAL final;
	LOGICAL mask;
	uint8_t mask_key[4];
	int opcode;
	int RSV1;  // input bit of RSV1 bit on each message
	int _RSV1; // input bit of the first RSV1 bit on a fragmented packet; used for permessage-deflate
	size_t frame_length;
	int input_msg_state;
	int input_type; // text or binary

	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_opened on_open;
	web_socket_error on_error;
	web_socket_accept on_accept;  // server socket event
	uintptr_t psv_on;
	uintptr_t psv_open; // result of the open, to pass to read

};

EXTERN void SendWebSocketMessage( PCLIENT pc, int opcode, int final, int do_mask, const uint8_t* payload, size_t length, int use_ssl );
EXTERN void ProcessWebSockProtocol( WebSocketInputState websock, PCLIENT pc, const uint8_t* msg, size_t length );

#endif
