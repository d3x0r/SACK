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
	P_8 fragment_collection;

	_32 last_reception; // for automatic ping/keep alive/idle death

	LOGICAL final;
	LOGICAL mask;
	_8 mask_key[4];
	int opcode;
	size_t frame_length;
	int input_msg_state;
	int input_type; // text or binary


	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_opened on_open;
	web_socket_error on_error;
	PTRSZVAL psv_on;
	PTRSZVAL psv_open; // result of the open, to pass to read

};

struct web_socket_output_state
{
	struct web_socket_output_flags
	{
		BIT_FIELD sent_type : 1;
		// apparently clients did not implement back masking??
      // I get a close; probably because of the length exception
		BIT_FIELD expect_masking : 1;
	} flags;
};


EXTERN void SendWebSocketMessage( PCLIENT pc, int opcode, int final, int do_mask, P_8 payload, size_t length );
EXTERN void ProcessWebSockProtocol( WebSocketInputState websock, PCLIENT pc, P_8 msg, size_t length );

