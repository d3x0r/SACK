/* Sack Layer on libssh2. This provides a event based interface. */
#include <network.h>
#include <html5.websocket.h>
#include <html5.websocket.client.h>

SACK_NETWORK_NAMESPACE
#ifdef __cplusplus
namespace ssh {
#endif


typedef void ( *ssh_handshake_cb )( uintptr_t psv, const uint8_t* fingerprint );

// ----------------- SESSIONS ---------------------

/*
* Initialize a session
*/
NETWORK_PROC( struct ssh_session*, sack_ssh_session_init )( uintptr_t psv );
/*
* connect a session
* if port is 0, the default port is used (22)
* if the host string includes a port, it will be used instead of the port parameter
*/
NETWORK_PROC( void, sack_ssh_session_connect )( struct ssh_session* session, CTEXTSTR host, int port, ssh_handshake_cb cb );
/*
* enable debugging on a session
*/
NETWORK_PROC( void, sack_ssh_trace )( struct ssh_session* session, int bitmask );
/*
* get the error code and message from a session
*/
NETWORK_PROC( void, sack_ssh_get_error )( struct ssh_session* session, int* err, CTEXTSTR* errmsg );
/*
* close a session
*/
NETWORK_PROC( void, sack_ssh_session_close )( struct ssh_session* session );

//----------------- CALLBACKS ---------------------


typedef void (*ssh_session_error_cb)( uintptr_t psv, int err, CTEXTSTR errmsg, int errmsglen );
NETWORK_PROC( ssh_session_error_cb, sack_ssh_set_session_error )( struct ssh_session* session, ssh_session_error_cb );

typedef void (*ssh_channel_error_cb)( uintptr_t psv, int err, CTEXTSTR errmsg, int errmsglen );
NETWORK_PROC( ssh_channel_error_cb, sack_ssh_set_channel_error )( struct ssh_channel* channel, ssh_channel_error_cb );

typedef void ( *ssh_listen_error_cb )( uintptr_t psv, int err, CTEXTSTR errmsg, int errmsglen );
NETWORK_PROC( ssh_listen_error_cb, sack_ssh_set_listener_error )( struct ssh_listener* listener, ssh_listen_error_cb );

typedef void ( *ssh_setenv_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_setenv_cb, sack_ssh_set_setenv )( struct ssh_listener* listener, ssh_setenv_cb );


typedef uintptr_t( *ssh_forward_connect_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_forward_connect_cb, sack_ssh_set_forward_connect )( struct ssh_session* session, ssh_forward_connect_cb );

/*
* set a callback for when a session is connected (or fails to connect)
*/
typedef void ( *ssh_connect_callback )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_connect_callback, sack_ssh_set_connect )( struct ssh_session* session, ssh_connect_callback );

/*
* set a callback when a listener is set up
* returns the previous callback
*
* This is called when a listener is set up, and the callback should return a pointer to a structure that will be used as the psv for the listener
*/
typedef uintptr_t( *ssh_forward_listen_cb )( uintptr_t psv, struct ssh_listener*, int bound_port );
NETWORK_PROC( ssh_forward_listen_cb, sack_ssh_set_forward_listen )( struct ssh_session* session, ssh_forward_listen_cb );

/*
* set a callback for when a listener gets a connection
* returns the previous callback
*
* This is called when a listener gets a connection, and the callback should return a pointer to a structure that will be used as the psv for the channel
*/
typedef uintptr_t( *ssh_forward_listen_accept_cb ) ( uintptr_t psv, struct ssh_listener*listener, struct ssh_channel* channel );
NETWORK_PROC( ssh_forward_listen_accept_cb, sack_ssh_set_forward_listen_accept )( struct ssh_listener* listen, ssh_forward_listen_accept_cb );

/*
* set a callback for when a session negotiates initial keys
* returns the previous callback
* 
* fingerprint is 20 bytes long
*/
NETWORK_PROC( ssh_handshake_cb, sack_ssh_set_handshake_complete )( struct ssh_session* session, ssh_handshake_cb );
/*
* set a callback for when a session is authenticated
* returns the previous callback
*/
typedef void ( *ssh_auth_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_auth_cb, sack_ssh_set_auth_complete )( struct ssh_session* session, ssh_auth_cb );

/*
* set a callback for when a password change is requested
* returns the previous callback
*/
typedef void ( *pw_change_cb )( uintptr_t, char** newpw, int* newpw_len );
NETWORK_PROC( pw_change_cb, sack_ssh_set_password_change_callback )( struct ssh_session* session, pw_change_cb );

/*
* set a callback for when a channel is opened
* returns the previous callback
*
* This is called when a channel is opened, and the callback should return a pointer to a structure that will be used as the psv for the channel
*/
typedef uintptr_t( *ssh_open_cb )( uintptr_t psv, struct ssh_channel* channel );
NETWORK_PROC( ssh_open_cb, sack_ssh_set_channel_open )( struct ssh_session* session, ssh_open_cb );
/*
* set a callback for when a pty is opened
* returns the previous callback
*/
typedef void( *ssh_pty_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_pty_cb, sack_ssh_set_pty_open )( struct ssh_channel* channel, ssh_pty_cb );

/*
* set a callback for when a pty is opened
* returns the previous callback
*/
typedef void( *ssh_shell_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_shell_cb, sack_ssh_set_shell_open )( struct ssh_channel* channel, ssh_shell_cb );

/*
* set a callback for when a command is executed
* returns the previous callback
*/
typedef void( *ssh_exec_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_exec_cb, sack_ssh_set_exec_done )( struct ssh_channel* channel, ssh_exec_cb );

/*
* set a callback for when data is received on a channel
* returns the previous callback
*/
typedef void( *ssh_channel_data_cb )( uintptr_t psv, int stream, const uint8_t* data, size_t len );
NETWORK_PROC( ssh_channel_data_cb, sack_ssh_set_channel_data )( struct ssh_channel* channel, ssh_channel_data_cb );
/*
* set a callback for when a channel is eof (end of file)
* returns the previous callback
*/
typedef void( *ssh_channel_eof_cb )( uintptr_t psv );
NETWORK_PROC( ssh_channel_eof_cb, sack_ssh_set_channel_eof )( struct ssh_channel* channel, ssh_channel_eof_cb );
/*
* set a callback for when a channel is closed
* returns the previous callback
*/
typedef void( *ssh_channel_close_cb )( uintptr_t psv );
NETWORK_PROC( ssh_channel_close_cb, sack_ssh_set_channel_close )( struct ssh_channel* channel, ssh_channel_close_cb );

/*
* set a callback for when a sftp session is opened
*/
typedef uintptr_t( *ssh_sftp_open_cb )( uintptr_t psv, struct ssh_sftp* channel );
NETWORK_PROC( ssh_sftp_open_cb, sack_ssh_set_sftp_open )( struct ssh_session* session, ssh_sftp_open_cb );


/*
* authenticate a user with a password
*/
NETWORK_PROC( void, sack_ssh_auth_user_password )( struct ssh_session* session, const char* user, const char* password, ssh_auth_cb cb );
/*
* authenticate a user with a public key
*/
NETWORK_PROC( void, sack_ssh_auth_user_cert )( struct ssh_session* session, CTEXTSTR user
	, uint8_t* pubkey, size_t pubkeylen
	, uint8_t* privkey, size_t privkeylen
	, CTEXTSTR pass, ssh_auth_cb cb );

//----------------- Port Forwarding ---------------------
//------------------------ More Session Interface; requires callback def --------------------
/*
* setup a local socket to accept connections, which gets
* forwarded to a remote connetion.  if remotePort == -1 remoteAddress
* specifies a unix socket name.
*
* cb is called when a new connection is made, and is treated like a channel open callback
*/
NETWORK_PROC( PCLIENT, sack_ssh_forward_connect )( struct ssh_session* session
	, CTEXTSTR localAddress, int localPort
	, CTEXTSTR remoteAddress, int remotePort
	, ssh_forward_connect_cb cb );

/*
* request a port forward
*/
NETWORK_PROC( void, sack_ssh_forward_listen )( struct ssh_session* session, CTEXTSTR remotehost, uint16_t remoteport, ssh_forward_listen_cb cb );

//------------------ CHANNELS ---------------------
/*
* open a channel on the session
*/
NETWORK_PROC( void, sack_ssh_channel_open )( struct ssh_session* session );
/*
* free a channel; closes channel if still open
*/
NETWORK_PROC( void, sack_ssh_channel_free )( struct ssh_channel* channel );
/*
* open a channel on the session with extra parameters.
*/
NETWORK_PROC( void, sack_ssh_channel_open_v2 )( struct ssh_session* session, CTEXTSTR type, size_t type_len, uint32_t window_size, uint32_t packet_size,
	CTEXTSTR message, size_t message_len, ssh_open_cb cb );
/*
* set an environment variable on the channel
*/
NETWORK_PROC( void, sack_ssh_channel_setenv )( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value, ssh_setenv_cb cb );

/*
* request a pty on the channel
* pty comes back in a callback set by sack_ssh_set_pty_open
*/
NETWORK_PROC( void, sack_ssh_channel_request_pty )( struct ssh_channel* channel, CTEXTSTR term, ssh_pty_cb cb );

/*
* open a shell on the channel
*/
NETWORK_PROC( void, sack_ssh_channel_shell )( struct ssh_channel* channel, ssh_shell_cb );

/*
* exec a command on channel
*/
NETWORK_PROC( void, sack_ssh_channel_exec )( struct ssh_channel* channel, CTEXTSTR shell, ssh_exec_cb );

/*
* Send data on a channel
* stream is 0 for stdin, 1 for stderr
*/
NETWORK_PROC( void, sack_ssh_channel_write )( struct ssh_channel* channel, int stream, const uint8_t* buffer, size_t length );

NETWORK_PROC( void, sack_ssh_channel_close )( struct ssh_channel* channel );  // send Close on channel (which also sends EOF on channel)

NETWORK_PROC( void, sack_ssh_channel_eof )( struct ssh_channel* channel ); // send EOF on channel
/*
LIBSSH2_API int libssh2_channel_send_eof(LIBSSH2_CHANNEL *channel);
LIBSSH2_API int libssh2_channel_eof(LIBSSH2_CHANNEL *channel);
LIBSSH2_API int libssh2_channel_wait_eof(LIBSSH2_CHANNEL *channel);
LIBSSH2_API int libssh2_channel_close(LIBSSH2_CHANNEL *channel);
LIBSSH2_API int libssh2_channel_wait_closed(LIBSSH2_CHANNEL *channel);
LIBSSH2_API int libssh2_channel_free(LIBSSH2_CHANNEL *channel);
*/

//----------------- SFTP ---------------------

NETWORK_PROC( void, sack_ssh_sftp_init )( struct ssh_session* session );
NETWORK_PROC( void, sack_ssh_sftp_shutdown )( struct ssh_sftp* session );

//----------------- Websocket ---------------------

/*
* serve websocket on an accepting channel. 
* the channel has to be opening from a forwarded listener 
* (before any data from the remote is processed)
*/
// a lot of this client work side is done in the websocket client library (sack.vfs)
NETWORK_PROC( struct ssh_websocket*, sack_ssh_channel_serve_websocket )( struct ssh_channel* channel,
	web_socket_opened ws_open,
	web_socket_event ws_event,
	web_socket_closed ws_close,
	web_socket_error ws_error,
	web_socket_accept ws_accept,  // server socket event
	web_socket_http_request ws_http,
	web_socket_http_close ws_http_close,
	web_socket_completion ws_completion,
	uintptr_t psv
	);


#ifdef __cplusplus
}
#endif
SACK_NETWORK_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::network::ssh;
#endif
