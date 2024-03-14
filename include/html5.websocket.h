/*
 * SACK extension to define methods to render to javascript/HTML5 WebSocket event interface
 *
 * Crafted by: Jim Buckeyne
 *
 * Purpose: Provide a well defined, concise structure to
 *   provide websocket server support to C applications.
 *   
 *
 *
 * (c)Freedom Collective, Jim Buckeyne 2012+; SACK Collection.
 *
 */

#ifndef HTML5_WEBSOCKET_STUFF_DEFINED
#define HTML5_WEBSOCKET_STUFF_DEFINED

#include <procreg.h>
//#include <controls.h>

// should consider merging these headers(?)
#include <html5.websocket.client.h>

#ifdef __cplusplus
#define _HTML5_WEBSOCKET_NAMESPACE namespace Html5WebSocket {
#define HTML5_WEBSOCKET_NAMESPACE SACK_NAMESPACE _NETWORK_NAMESPACE _HTML5_WEBSOCKET_NAMESPACE
#define HTML5_WEBSOCKET_NAMESPACE_END } _NETWORK_NAMESPACE_END SACK_NAMESPACE_END
#define USE_HTML5_WEBSOCKET_NAMESPACE using namespace sack::network::Html5WebSocket;
#else
#define _HTML5_WEBSOCKET_NAMESPACE
#define HTML5_WEBSOCKET_NAMESPACE
#define HTML5_WEBSOCKET_NAMESPACE_END
#define USE_HTML5_WEBSOCKET_NAMESPACE
#endif

HTML5_WEBSOCKET_NAMESPACE


#ifdef HTML5_WEBSOCKET_SOURCE
#define HTML5_WEBSOCKET_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define HTML5_WEBSOCKET_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

// need some sort of other methods to work with an HTML5WebSocket...
// server side.
HTML5_WEBSOCKET_PROC( PCLIENT, WebSocketCreate_v2 )(CTEXTSTR hosturl
	, web_socket_opened on_open
	, web_socket_event on_event
	, web_socket_closed on_closed
	, web_socket_error on_error
	, uintptr_t psv
	, int webSocketOptions
);

/*
* Creates a server side websocket/http request socket for an accepted network connection.
* 
*/
HTML5_WEBSOCKET_PROC( struct html5_web_socket*, WebSocketCreateServerPipe )( web_socket_opened on_open
                                        , web_socket_event on_event
                                        , web_socket_closed on_closed
                                        , web_socket_error on_error
                                        , web_socket_accept on_accept
                                        , web_socket_http_request ws_http
                                        , web_socket_http_close ws_http_close
                                        , web_socket_completion ws_completion
                                        , uintptr_t psv
);

// Option Wait 
#define WEBSOCK_SERVER_OPTION_WAIT 1

HTML5_WEBSOCKET_PROC( PCLIENT, WebSocketCreate )( CTEXTSTR server_url
																	, web_socket_opened on_open
																	, web_socket_event on_event
																	, web_socket_closed on_closed
																	, web_socket_error on_error
																	, uintptr_t psv
																	);

/*
* Write new data to a html5_web_socket pipe connection. (should be accepted socket, not the listener/server socket);
*/
HTML5_WEBSOCKET_PROC( void, WebSocketWrite )( struct html5_web_socket* socket, CPOINTER buffer, size_t length );
/*
* Set the send callback for a pipe connection.
*/
HTML5_WEBSOCKET_PROC( void, WebSocketPipeSetSend )( struct html5_web_socket* pipe, int ( *on_send )( uintptr_t psv, CPOINTER buffer, size_t length ), uintptr_t psv_send );
HTML5_WEBSOCKET_PROC( void, WebSocketPipeSetClose )( struct html5_web_socket* pipe, void ( *do_close )( uintptr_t psv ), uintptr_t psv_close );
HTML5_WEBSOCKET_PROC( void, WebSocketPipeSend )( struct html5_web_socket* socket, CPOINTER buffer, size_t length );
HTML5_WEBSOCKET_PROC( void, WebSocketPipeClose )( struct html5_web_socket* socket );
/*
* A new pipe connection has been accepted, this performs the same operation
* as accepting a socket internally
*/
HTML5_WEBSOCKET_PROC( struct html5_web_socket*, WebSocketPipeConnect )( struct html5_web_socket* pipe, uintptr_t psvNew );

// during open, server may need to switch behavior based on protocols
// this can be used to return the protocols requested by the client.
HTML5_WEBSOCKET_PROC( const char *, WebSocketGetProtocols )( PCLIENT pc );

HTML5_WEBSOCKET_PROC( const char *, WebSocketPipeGetProtocols )( struct html5_web_socket* );

// after examining protocols, this is a reply to the client which protocol has been accepted.
HTML5_WEBSOCKET_PROC( PCLIENT, WebSocketSetProtocols )( PCLIENT pc, const char *protocols );
/* define a callback which uses a HTML5WebSocket collector to build javascipt to render the control.
 * example:
 *       static int OnDrawToHTML("Control Name")(CONTROL, HTML5WebSocket ){ }
 */
//#define OnDrawToHTML(name)  \
//	__DefineRegistryMethodP(PRELOAD_PRIORITY,ROOT_REGISTRY,_OnDrawCommon,"control",name "/rtti","draw_to_canvas",int,(CONTROL, HTML5WebSocket ), __LINE__)


/* a server side utility to get the request headers that came in.
this is for going through proxy agents mostly where the header might have x-forwarded-for
*/
HTML5_WEBSOCKET_PROC( PLIST, GetWebSocketHeaders )( PCLIENT pc );
/* for server side sockets, get the requested resource path from the client request.
*/
HTML5_WEBSOCKET_PROC( PTEXT, GetWebSocketResource )( PCLIENT pc );

HTML5_WEBSOCKET_PROC( HTTPState, GetWebSocketHttpState )( PCLIENT pc );

HTML5_WEBSOCKET_PROC( PLIST, GetWebSocketPipeHeaders )( struct html5_web_socket* socket );
HTML5_WEBSOCKET_PROC( PTEXT, GetWebSocketPipeResource )( struct html5_web_socket* socket );
HTML5_WEBSOCKET_PROC( HTTPState, GetWebSocketPipeHttpState )( struct html5_web_socket* socket );


HTML5_WEBSOCKET_PROC( void, ResetWebsocketRequestHandler )( PCLIENT pc_client );

HTML5_WEBSOCKET_PROC( uintptr_t, WebSocketGetServerData )( PCLIENT pc );



HTML5_WEBSOCKET_NAMESPACE_END
USE_HTML5_WEBSOCKET_NAMESPACE

#endif
