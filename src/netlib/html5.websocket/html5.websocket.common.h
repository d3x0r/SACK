#ifndef SACK_HTML5_WEBSOCKET_COMMON_DEFINED
#define SACK_HTML5_WEBSOCKET_COMMON_DEFINED

#include <network.h>
#include <html5.websocket.client.h>

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
	} flags;


	size_t fragment_collection_avail;
	size_t fragment_collection_length;
	size_t fragment_collection_index;  // used for selecting mask byte
	uint8_t* fragment_collection;

	uint32_t last_reception; // for automatic ping/keep alive/idle death

	LOGICAL final;
	LOGICAL mask;
	uint8_t mask_key[4];
	int opcode;
	size_t frame_length;
	int input_msg_state;
	int input_type; // text or binary


	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_opened on_open;
	web_socket_error on_error;
	uintptr_t psv_on;
	uintptr_t psv_open; // result of the open, to pass to read

};

struct web_socket_output_state
{
	struct web_socket_output_flags
	{
		BIT_FIELD sent_type : 1;
		// apparently clients did not implement back masking??
      // I get a close; probably because of the length exception
		BIT_FIELD expect_masking : 1;
		BIT_FIELD use_ssl : 1;
	} flags;
};


EXTERN void SendWebSocketMessage( PCLIENT pc, int opcode, int final, int do_mask, const uint8_t* payload, size_t length, int use_ssl );
EXTERN void ProcessWebSockProtocol( WebSocketInputState websock, PCLIENT pc, const uint8_t* msg, size_t length );

#endif
