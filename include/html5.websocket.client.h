#ifndef HTML5_WEBSOCKET_CLIENT_INCLUDED
#define HTML5_WEBSOCKET_CLIENT_INCLUDED

/*****************************************************

so... what does the client provide?

websocket protocol is itself wrapped in a frame, so messages are described with exact
length, and what is received will be exactly like the block that was sent.







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

// the result returned from the web_socket_opened event will
// become the new value used for future uintptr_t parameters to other events.
typedef uintptr_t (*web_socket_opened)( PCLIENT pc, uintptr_t psv );
typedef void (*web_socket_closed)( PCLIENT pc, uintptr_t psv );
typedef void (*web_socket_error)( PCLIENT pc, uintptr_t psv, int error );
typedef void (*web_socket_event)( PCLIENT pc, uintptr_t psv, POINTER buffer, int msglen );


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
															 , uintptr_t psv );

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

WEBSOCKET_EXPORT void WebSocketEnableAutoPing( PCLIENT websock, uint32_t delay );

WEBSOCKET_EXPORT void WebSocketPing( PCLIENT websock, uint32_t timeout );

#endif
