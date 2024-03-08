#include <network.h>

SACK_NETWORK_NAMESPACE
#ifdef __cplusplus
namespace ssh {
#endif

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
	NETWORK_PROC( void, sack_ssh_session_connect )( struct ssh_session* session, CTEXTSTR host, int port );
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
	typedef uintptr_t( *ssh_listen_cb )( uintptr_t psv, struct ssh_listener*, int bound_port );
	NETWORK_PROC( ssh_listen_cb, sack_ssh_set_listen )( struct ssh_session* session, ssh_listen_cb );

	/*
	* set a callback for when a listener gets a connection
	* returns the previous callback
	*
	* This is called when a listener gets a connection, and the callback should return a pointer to a structure that will be used as the psv for the channel
	*/
	typedef uintptr_t( *ssh_listen_connect_cb )( uintptr_t psv, struct ssh_channel* );
	NETWORK_PROC( ssh_listen_connect_cb, sack_ssh_set_listen_connect )( struct ssh_listener* listener, ssh_listen_connect_cb );


	/*
	* set a callback for when a session negotiates initial keys
	* returns the previous callback
	* 
	* fingerprint is 20 bytes long
	*/
	typedef void ( *ssh_handshake_cb )( uintptr_t psv, const uint8_t *fingerprint );
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
	NETWORK_PROC( void, sack_ssh_auth_user_password )( struct ssh_session* session, const char* user, const char* password );
	/*
	* authenticate a user with a public key
	*/
	NETWORK_PROC( void, sack_ssh_auth_user_cert )( struct ssh_session* session, CTEXTSTR user
		, CTEXTSTR pubkey
		, CTEXTSTR privkey
		, CTEXTSTR pass );

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
	NETWORK_PROC( void, sack_ssh_channel_setenv )( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value );

	/*
	* request a pty on the channel
	* pty comes back in a callback set by sack_ssh_set_pty_open
	*/
	NETWORK_PROC( void, sack_ssh_channel_request_pty )( struct ssh_channel* channel, CTEXTSTR term );

	/*
	* open a shell on the channel
	*/
	NETWORK_PROC( void, sack_ssh_channel_shell )( struct ssh_channel* channel );

	/*
	* exec a command on channel
	*/
	NETWORK_PROC( void, sack_ssh_channel_exec )( struct ssh_channel* channel, CTEXTSTR shell );

	/*
	* Send data on a channel
	* stream is 0 for stdin, 1 for stderr
	*/
	NETWORK_PROC( void, sack_ssh_channel_write )( struct ssh_channel* channel, int stream, const uint8_t* buffer, size_t length );

	NETWORK_PROC( void, sack_ssh_channel_close )( struct ssh_channel* channel );

	/*
	LIBSSH2_API int libssh2_channel_send_eof(LIBSSH2_CHANNEL *channel);
	LIBSSH2_API int libssh2_channel_eof(LIBSSH2_CHANNEL *channel);
	LIBSSH2_API int libssh2_channel_wait_eof(LIBSSH2_CHANNEL *channel);
	LIBSSH2_API int libssh2_channel_close(LIBSSH2_CHANNEL *channel);
	LIBSSH2_API int libssh2_channel_wait_closed(LIBSSH2_CHANNEL *channel);
	LIBSSH2_API int libssh2_channel_free(LIBSSH2_CHANNEL *channel);
	*/

	//----------------- Port Forwarding ---------------------

	/*
	* request a port forward
	*/
	NETWORK_PROC( void, sack_ssh_channel_forward_listen )( struct ssh_session* session, CTEXTSTR remotehost, uint16_t remoteport, ssh_listen_connect_cb cb );

	//----------------- SFTP ---------------------

	NETWORK_PROC( void, sack_ssh_sftp_init )( struct ssh_session* session );
	NETWORK_PROC( void, sack_ssh_sftp_shutdown )( struct ssh_sftp* session );



#ifdef __cplusplus
}
#endif
SACK_NETWORK_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::network::ssh;
#endif
