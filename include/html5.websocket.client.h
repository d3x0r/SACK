#ifndef HTML5_WEBSOCKET_CLIENT_INCLUDED
#define HTML5_WEBSOCKET_CLIENT_INCLUDED

/*****************************************************

so... what does the client provide?

websocket protocol is itself wrapped in a frame, so messages are described with exact
length, and what is received will be exactly like the block that was sent.







*****************************************************/

#include <network.h>
#include <http.h>
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
typedef void (*web_socket_closed)( PCLIENT pc, uintptr_t psv, int code, const char *reason );
typedef void (*web_socket_error)( PCLIENT pc, uintptr_t psv, int error );
typedef void (*web_socket_event)( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen );
// protocolsAccepted value set can be released in opened callback, or it may be simply assigned as protocols passed...
typedef LOGICAL ( *web_socket_accept )(PCLIENT pc, uintptr_t psv, const char *protocols, const char *resource, char **protocolsAccepted);

typedef uintptr_t ( *web_socket_http_request )(PCLIENT pc, uintptr_t psv); // passed psv used in server create; since it is sort of an open, return a psv for next states(if any)

// these should be a combination of bit flags
// options used for WebSocketOpen
enum WebSocketOptions {
	WS_DELAY_OPEN = 1,  
};

//enum WebSockClientOptions {
//   WebSockClientOption_Protocols
//};

// create a websocket connection.
//  If web_socket_opened is passed as NULL, this function will wait until the negotiation has passed.
//  since these packets are collected at a lower layer, buffers passed to receive event are allocated for
//  the application, and the application does not need to setup an  initial read.
//  if protocols is NULL none are specified, otherwise the list of
//  available protocols is sent to the server.
WEBSOCKET_EXPORT PCLIENT WebSocketOpen( CTEXTSTR address
                                      , enum WebSocketOptions options
                                      , web_socket_opened
                                      , web_socket_event
                                      , web_socket_closed
                                      , web_socket_error
                                      , uintptr_t psv
                                      , const char *protocols );

// if WS_DELAY_OPEN is used, WebSocketOpen does not do immediate connect.  
// calling this begins the connection sequence.
WEBSOCKET_EXPORT void WebSocketConnect( PCLIENT );

// end a websocket connection nicely.
// code must be 1000, or 3000-4999, and reason must be less than 123 characters (125 bytes with code)
WEBSOCKET_EXPORT void WebSocketClose( PCLIENT, int code, const char *reason );


// there is a control bit for whether the content is text or binary or a continuation
WEBSOCKET_EXPORT void WebSocketBeginSendText( PCLIENT, const char *, size_t ); // UTF8 RFC3629

// literal binary sending; this may happen to be base64 encoded too
WEBSOCKET_EXPORT void WebSocketBeginSendBinary( PCLIENT, const uint8_t *, size_t );

// there is a control bit for whether the content is text or binary or a continuation
WEBSOCKET_EXPORT void WebSocketSendText( PCLIENT, const char *, size_t ); // UTF8 RFC3629

// literal binary sending; this may happen to be base64 encoded too
WEBSOCKET_EXPORT void WebSocketSendBinary( PCLIENT, const uint8_t *, size_t );

WEBSOCKET_EXPORT void WebSocketEnableAutoPing( PCLIENT websock, uint32_t delay );

WEBSOCKET_EXPORT void WebSocketPing( PCLIENT websock, uint32_t timeout );


WEBSOCKET_EXPORT void SetWebSocketAcceptCallback( PCLIENT pc, web_socket_accept callback );
WEBSOCKET_EXPORT void SetWebSocketReadCallback( PCLIENT pc, web_socket_event callback );
WEBSOCKET_EXPORT void SetWebSocketCloseCallback( PCLIENT pc, web_socket_closed callback );
WEBSOCKET_EXPORT void SetWebSocketErrorCallback( PCLIENT pc, web_socket_error callback );
WEBSOCKET_EXPORT void SetWebSocketHttpCallback( PCLIENT pc, web_socket_http_request callback );

// if set in server accept callback, this will return without extension set
// on client socket (default), does not request permessage-deflate
#define WEBSOCK_DEFLATE_DISABLE 0
// if set in server accept callback (or if not set, default); accept client request to deflate per message
// if set on client socket, sends request for permessage-deflate to server.
#define WEBSOCK_DEFLATE_ENABLE 1
// if set in server accept callback; accept client request to deflate per message, but do not deflate outbound messages
// if set on client socket, sends request for permessage-deflate to server, but does not deflate outbound messages(?)
#define WEBSOCK_DEFLATE_ALLOW 2

// set permessage-deflate option for client requests.
// allow server side to disable this when responding to a client.
WEBSOCKET_EXPORT void SetWebSocketDeflate( PCLIENT pc, int enable_flags );

// default is client masks, server does not
// this can be used to disable masking on client or enable on server
// (masked output from server to client is not supported by browsers)
WEBSOCKET_EXPORT void SetWebSocketMasking( PCLIENT pc, int enable );

#endif
