#include <network.h>

/*
* Initialize a session
*/
NETWORK_PROC( struct ssh_session *, sack_ssh_session_init)( uintptr_t psv );
/*
* connect a session
* if port is 0, the default port is used (22)
* if the host string includes a port, it will be used instead of the port parameter
*/
NETWORK_PROC( void, sack_ssh_session_connect )( struct ssh_session* session, CTEXTSTR host, int port );
/*
* close a session
*/
NETWORK_PROC( void, sack_ssh_session_close)(struct ssh_session *session);


/*
* set a callback for when a session is connected (or fails to connect)
*/
typedef void ( *ssh_connect_callback )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_connect_callback, sack_ssh_set_connect )( struct ssh_session* session, ssh_connect_callback );
/*
* set a callback for when a session negotiates initial keys
* returns the previous callback
*/
typedef void ( *ssh_handshake_cb )( uintptr_t psv, CTEXTSTR fingerprint );
NETWORK_PROC( ssh_handshake_cb, sack_ssh_set_handshake_complete )(struct ssh_session *session, ssh_handshake_cb );
/*
* set a callback for when a session is authenticated
* returns the previous callback
*/
typedef void ( *ssh_auth_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_auth_cb, sack_ssh_set_auth_complete)(struct ssh_session *session, ssh_auth_cb );

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
NETWORK_PROC( ssh_open_cb, sack_ssh_set_channel_open)(struct ssh_session *session, ssh_open_cb );
/*
* set a callback for when a pty is opened
* returns the previous callback
*/
typedef void( *ssh_pty_cb )( uintptr_t psv, LOGICAL success );
NETWORK_PROC( ssh_pty_cb, sack_ssh_set_pty_open)(struct ssh_channel*channel, ssh_pty_cb );

/*
* set a callback for when data is received on a channel
* returns the previous callback
*/
typedef void( *ssh_channel_data_cb )( uintptr_t psv, struct ssh_channel *channel, int stream, const uint8_t *data, size_t len );
NETWORK_PROC( ssh_channel_data_cb, sack_ssh_set_channel_data)(struct ssh_channel *channel, ssh_channel_data_cb );
/*
* set a callback for when a channel is eof (end of file)
* returns the previous callback
*/
typedef void( *ssh_channel_eof_cb )( uintptr_t psv, struct ssh_channel *channel );
NETWORK_PROC( ssh_channel_eof_cb, sack_ssh_set_channel_eof)(struct ssh_channel *channel, ssh_channel_eof_cb );
/*
* set a callback for when a channel is closed
* returns the previous callback
*/
typedef void( *ssh_channel_close_cb )( uintptr_t psv, struct ssh_channel *channel );
NETWORK_PROC( ssh_channel_close_cb, sack_ssh_set_channel_close)( struct ssh_channel* channel, ssh_channel_close_cb );


/*
* authenticate a user with a password
*/
NETWORK_PROC( void, sack_auth_user_password )( struct ssh_session *session, const char *user, const char *password );
/*
* authenticate a user with a public key
*/
NETWORK_PROC( void, sack_auth_user_cert )( struct ssh_session*session, CTEXTSTR user
                                         , CTEXTSTR pubkey
                                         , CTEXTSTR privkey
                                         , CTEXTSTR pass );

/*
* open a channel on the session
*/
NETWORK_PROC( void, sack_ssh_channel_open )( struct ssh_session* session );
/*
* open a channel on the session with extra parameters.
*/
NETWORK_PROC( void, sack_ssh_channel_open_v2 )( struct ssh_session* session, CTEXTSTR type, size_t type_len, uint32_t window_size, uint32_t packet_size,
	CTEXTSTR message, size_t message_len );
/*
* set an environment variable on the channel 
*/
NETWORK_PROC( void, sack_ssh_channel_setenv )( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value );

/*
* request a pty on the channel
* pty comes back in a callback set by sack_ssh_set_pty_open
*/
NETWORK_PROC( void, sack_ssh_channel_request_pty )( struct ssh_channel *channel, CTEXTSTR term );

/*
* open a shell on the channel
*/
NETWORK_PROC( void, sack_ssh_channel_shell )( struct ssh_channel* channel );
