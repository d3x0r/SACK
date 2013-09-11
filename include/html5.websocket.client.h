

/*****************************************************

so... what does the client provide?

websocket protocol is itself wrapped in a frame, so messages are described with exact
length, and what is received will be exactly like the block that was sent.


typedef void (*web_socket_opened)( PTRSZVAL psv );
typedef void (*web_socket_closed)( PTRSZVAL psv );
typedef void (*web_socket_error)( PTRSZVAL psv, int error );
typedef void (*web_socket_event)( PTRSZVAL psv, POINTER buffer, int msglen );
typedef struct web_socket_client *WebSocketClient;

 WebSocketClient OpenWebSocket( address, socket_opened, event_received, socket_closed, error_recieved, psv );





*****************************************************/

#include <network.h>

#ifdef __cplusplus
#else
#endif

#ifdef SACK_WEBSOCKET_CLIENT_SOURCE
#define WEBSOCKET_EXPORT EXPORT_METHOD
#else
#define WEBSOCKET_EXPORT IMPORT_METHOD
#endif

typedef void (*web_socket_opened)( PTRSZVAL psv );
typedef void (*web_socket_closed)( PTRSZVAL psv );
typedef void (*web_socket_error)( PTRSZVAL psv, int error );
typedef void (*web_socket_event)( PTRSZVAL psv, POINTER buffer, int msglen );


// create a websocket connection.
//  If web_socket_opened is passed as NULL, this function will wait until the negotiation has passed.
//  since these packets are collected at a lower layer, buffers passed to receive event are allocated for
//  the application, and the application does not need to setup an  initial read.
WEBSOCKET_EXPORT PCLIENT WebSocketOpen( CTEXTSTR address
															 , int options
															 , web_socket_opened
															 , web_socket_event
															 , web_socket_closed
															 , web_socket_error
															 , PTRSZVAL psv );

// end a websocket connection nicely.
WEBSOCKET_EXPORT void WebSocketClose( PCLIENT );


// there is a control bit for whether the content is text or binary or a continuation
WEBSOCKET_EXPORT void WebSocketBeginSendText( PCLIENT, POINTER, size_t ); // UTF8 RFC3629

// literal binary sending; this may happen to be base64 encoded too
WEBSOCKET_EXPORT void WebSocketBeginSendBinary( PCLIENT, POINTER, size_t );

// there is a control bit for whether the content is text or binary or a continuation
WEBSOCKET_EXPORT void WebSocketSendText( PCLIENT, POINTER, size_t ); // UTF8 RFC3629

// literal binary sending; this may happen to be base64 encoded too
WEBSOCKET_EXPORT void WebSocketSendBinary( PCLIENT, POINTER, size_t ); 

WEBSOCKET_EXPORT void WebSocketEnableAutoPing( PCLIENT websock, _32 delay );

WEBSOCKET_EXPORT void WebSocketPing( PCLIENT websock, _32 timeout );


