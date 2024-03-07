
#include <stdhdrs.h>
#include <network.h>
#include <sack_ssh.h>
#include <libssh2.h>

#include "ssh_layer.h"

#ifdef __cplusplus
#define set_pending_state(ps,s,...) { pending_state tmp = {s,##__VA_ARGS__}; ps= tmp; }
#else
#define set_pending_state(ps,s,...) ps = (struct pending_state){s,##__VA_ARGS__}
#endif


static void* alloc_callback( size_t size, void** abstract ) {
	return AllocateEx( size DBG_SRC );
}

static void free_callback( void *data, void** abstract ) {
	ReleaseEx( data DBG_SRC );
}
static void* realloc_callback( void* p, size_t size, void** abstract ) {
	return Reallocate( p, size DBG_SRC );
}

static LIBSSH2_SEND_FUNC( SendCallback ) {
	// return ssize_t
	/* int socket \
		, const void* buffer
		, size_t length \
		, int flags
		, void** abstract)
	*/
	struct ssh_session* cs = (struct ssh_session*)abstract[0];
	SendTCP( cs->pc, buffer, length );
	return length;
}


static void moveBuffers( PDATALIST pdl ) {
	struct data_buffer* buf;
	INDEX idx;
	struct data_buffer buf0;


	DATA_FORALL( pdl, idx, struct data_buffer*, buf ) {
		if( !idx ) buf0 = buf[0];
		else {
			buf[-1] = buf[0];
		}
	}
	if( buf )
	buf[idx] = buf0;

}

static LIBSSH2_RECV_FUNC( RecvCallback ) {
	// return ssize_t
	/* int socket \
		, const void* buffer
		, size_t length \
		, int flags
		, void** abstract)
	*/
	struct ssh_session* cs = (struct ssh_session*)abstract[0];
	while( !cs->buffers || !cs->buffers->Cnt ) {
		return -EAGAIN;
	}
	struct data_buffer* buf;
	INDEX idx;
	//cs->waiter = MakeThread();
	DATA_FORALL( cs->buffers, idx, struct data_buffer*, buf ) {
		if( buf->length ) {
			ssize_t filled;
			MemCpy( buffer, buf->buffer+buf->used, filled = ((( buf->length-buf->used)< length)?(buf->length-buf->used):length) );
			buf->used += filled;
			if( buf->used == buf->length ) {
				buf->length = 0;
				moveBuffers( cs->buffers );
			}
			return filled;
		}
	}
	return -EAGAIN;
}


static void pwChange( LIBSSH2_SESSION *session, char** newpw, int* newpw_len, void **abstract ) {
	struct ssh_session* cs = (struct ssh_session*)abstract[0];
	if( cs->pw_change )
		cs->pw_change( cs->psv, newpw, newpw_len );
	//RemoveClient( cs->pc );
}

static void readCallback( PCLIENT pc, POINTER buffer, size_t length ) {
	struct ssh_session* cs = (struct ssh_session*)GetNetworkLong( pc, 0 );
	if( !buffer ) {
		cs->buffers = CreateDataList( sizeof( struct data_buffer ) );
	}
	else {
		// got data from socket, need to stuff it into SSH.
		int rc = 0;
		int did_one = 0;
		INDEX idx;
		struct data_buffer* db;
		DATA_FORALL( cs->buffers, idx, struct data_buffer*, db ) {
			if( !db->length && db->buffer == buffer ) {
				db->length = length;
				//WakeThread( cs->waiter );
				break;
			}
		}
		while( 1 ) {
			if( did_one && cs->pending.state != SSH_STATE_RESET ) break;
			did_one = 1;
			if( cs->pending.state == SSH_STATE_RESET )
				DequeData( &cs->pdqStates, &cs->pending );
			switch( cs->pending.state ) {
			case SSH_STATE_HANDSHAKE:{
					rc = libssh2_session_handshake( cs->session, 1/*sock*/ );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						RemoveClient( pc );
						return;
					}
					else {
						CTEXTSTR fingerprint = libssh2_hostkey_hash( cs->session, LIBSSH2_HOSTKEY_HASH_SHA1 );
						cs->pending.state = SSH_STATE_RESET;
						cs->handshake_complete( cs->psv, fingerprint );
					}
				}
				break;
			case SSH_STATE_AUTH_PK:
				rc = libssh2_userauth_publickey_frommemory( cs->session, cs->user, cs->user_len
														, cs->privKey, cs->privKey_len
														, cs->pubKey, cs->pubKey_len
														, cs->pass );
				if( rc == LIBSSH2_ERROR_EAGAIN ) break;
				else if( rc ) {
					// error
					cs->auth_complete( cs->psv, FALSE );
					RemoveClient( pc );
					return;
				}
				else {
					if( cs->user ) ReleaseEx( (POINTER)cs->user DBG_SRC );
					if( cs->privKey ) ReleaseEx( (POINTER)cs->privKey DBG_SRC );
					if( cs->pubKey ) ReleaseEx( (POINTER)cs->pubKey DBG_SRC );
					if( cs->pass ) ReleaseEx( (POINTER)cs->pass DBG_SRC );
					cs->pending.state = SSH_STATE_RESET;
					cs->auth_complete( cs->psv, TRUE );
				}
				break;
			case SSH_STATE_AUTH_PW:
				{
					CTEXTSTR user = (CTEXTSTR)cs->pending.state_data[0];
					CTEXTSTR pass = (CTEXTSTR)cs->pending.state_data[2];
					rc = libssh2_userauth_password_ex( cs->session, user, (unsigned int)cs->pending.state_data[1], pass, (unsigned int)cs->pending.state_data[3], pwChange );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						cs->auth_complete( cs->psv, FALSE );
						RemoveClient( pc );
						return;
					} else {
						if( user ) ReleaseEx( (POINTER)user DBG_SRC );
						if( pass ) ReleaseEx( (POINTER)pass DBG_SRC );
						cs->pending.state = SSH_STATE_RESET;
						cs->auth_complete( cs->psv, TRUE );
					}
				}
				break;
			case SSH_STATE_OPEN_CHANNEL:{
					CTEXTSTR type = (CTEXTSTR)cs->pending.state_data[0];
					CTEXTSTR message = (CTEXTSTR)cs->pending.state_data[4];
					LIBSSH2_CHANNEL *channel = libssh2_channel_open_ex( cs->session, type, (unsigned int)cs->pending.state_data[1], (unsigned int)cs->pending.state_data[2], (unsigned int)cs->pending.state_data[3], message, (unsigned int)cs->pending.state_data[5] );
					if( !channel ) {
						rc = libssh2_session_last_error( cs->session, NULL, NULL, 0 );
						if( rc == LIBSSH2_ERROR_EAGAIN ) break;
						else if( rc ) {
							// error
							RemoveClient( pc );
							return;
						}
						else {
							lprintf( "no channel, no error??");
						}
					}else {
						struct ssh_channel* ssh_channel = NewArray( struct ssh_channel, 1);
						MemSet( ssh_channel, 0, sizeof( struct ssh_channel ) );
						((uintptr_t*)libssh2_channel_abstract( channel ))[0] = (uintptr_t)ssh_channel;
						Deallocate( CTEXTSTR, type );
						Deallocate( CTEXTSTR, message );
						ssh_channel->session = cs;
						ssh_channel->channel = channel;
						ssh_channel->psv = cs->channel_open( cs->psv, ssh_channel );
						cs->pending.state = SSH_STATE_RESET;
					}
				}
				break;
			case SSH_STATE_REQUEST_PTY:
				{
					struct ssh_channel *channel = (struct ssh_channel*)cs->pending.state_data[0];
					CTEXTSTR stream = (CTEXTSTR)cs->pending.state_data[1];
					rc = libssh2_channel_request_pty( channel->channel, stream );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						channel->pty_open( cs->psv, FALSE );
						RemoveClient( pc );
						return;
					}
					else {
						channel->pty_open( cs->psv, TRUE );
						cs->pending.state = SSH_STATE_RESET;
					}
					break;
				}
			case SSH_STATE_SETENV:
				{
					CTEXTSTR key = (CTEXTSTR)cs->pending.state_data[0];
					CTEXTSTR value = (CTEXTSTR)cs->pending.state_data[1];
					struct ssh_channel* channel = (struct ssh_channel*)cs->pending.state_data[2];
					rc = libssh2_channel_setenv( channel->channel, key, value );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						lprintf( "error setting env:%d", rc );
						char* msg;
						int msglen;
						libssh2_session_last_error( cs->session, &msg, &msglen, 0 );
						lprintf( "error setting env:%s", msg );
						cs->pending.state = SSH_STATE_RESET;
						Deallocate( CTEXTSTR, key );
						Deallocate( CTEXTSTR, value );
						//RemoveClient( pc );
					}
					else {
						cs->pending.state = SSH_STATE_RESET;
						Deallocate( CTEXTSTR, key );
						Deallocate( CTEXTSTR, value );
					}
				}
				break;
			case SSH_STATE_SHELL:
				{
					struct ssh_channel* channel = (struct ssh_channel*)cs->pending.state_data[0];
					rc = libssh2_channel_shell( channel->channel );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						RemoveClient( pc );
						return;
					}
					else {
						cs->pending.state = SSH_STATE_RESET;
					}
				}
				break;
			case SSH_STATE_RESET:
				rc = libssh2_read( cs->session );
				break;
			}
			if( rc == LIBSSH2_ERROR_EAGAIN ) break;
		}
		//case 
	}
	INDEX idx;
	struct data_buffer* db;
	DATA_FORALL( cs->buffers, idx, struct data_buffer*, db ) {
		if( !db->length )
			ReadTCP( pc, db->buffer, 4096 );
	}
	struct data_buffer new_db;
	new_db.buffer = (uint8_t*)AllocateEx( 4096 DBG_SRC );
	new_db.length = 0;
	new_db.used = 0;
	AddDataItem( &cs->buffers, &new_db );
	ReadTCP( pc, new_db.buffer, 4096 );
}

static void connectCallback( PCLIENT pc, int error ){
	struct ssh_session* session = (struct ssh_session*)GetNetworkLong( pc, 0 );
	if( error ) {
		// error
	}
	else {
		// success
		set_pending_state( session->pending , SSH_STATE_HANDSHAKE );
		libssh2_session_handshake( session->session, 1/*sock*/ );
	}
}

static void closeCallback( PCLIENT pc ) {
	struct ssh_session* session = (struct ssh_session*)GetNetworkLong( pc, 0 );
	libssh2_session_free( session->session );
	ReleaseEx( session DBG_SRC );
}

//----------------------------- Session

struct ssh_session * sack_ssh_session_init(uintptr_t psv){
	struct ssh_session * session = NewArray( struct ssh_session, 1);
	MemSet( session, 0, sizeof( struct ssh_session ) );
	session->buffers = NULL;
	session->pdqStates = CreateDataQueue( sizeof( struct pending_state ) );
	session->psv = psv;
	session->session = libssh2_session_init_ex(alloc_callback, free_callback, realloc_callback, session);

	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_SEND, (libssh2_cb_generic*)SendCallback );
	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_RECV, (libssh2_cb_generic*)RecvCallback );
	libssh2_session_set_blocking( session->session, 0 );

	return session;
}

void sack_ssh_session_connect( struct ssh_session* session, CTEXTSTR host, int port ) {
	session->pc = OpenTCPClientExxx( host, port?port:22, readCallback, closeCallback, NULL, connectCallback, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	SetNetworkLong( session->pc, 0, (uintptr_t)session );
	NetworkConnectTCP( session->pc );


}

void sack_ssh_session_close( struct ssh_session*session ){
	RemoveClient( session->pc );
}

//----------------------------- Callbacks
                 
ssh_handshake_cb sack_ssh_set_handshake_complete( struct ssh_session*session, ssh_handshake_cb handshake_complete ){
	ssh_handshake_cb cb = session->handshake_complete;
	session->handshake_complete = handshake_complete;
	return cb;
}
ssh_auth_cb sack_ssh_set_auth_complete( struct ssh_session*session, void(*auth_complete)(uintptr_t,LOGICAL) ){
	ssh_auth_cb cb = session->auth_complete;
	session->auth_complete = auth_complete;
	return cb;
}

ssh_open_cb sack_ssh_set_channel_open( struct ssh_session*session, uintptr_t(*channel_open)(uintptr_t,struct ssh_channel*) ){
	ssh_open_cb cb = session->channel_open;
	session->channel_open = channel_open;
	return cb;
}

ssh_pty_cb sack_ssh_set_pty_open( struct ssh_channel*channel, void(*pty_open)(uintptr_t,LOGICAL) ){
	ssh_pty_cb cb = channel->pty_open;
	channel->pty_open = pty_open;
	return cb;
}

static LIBSSH2_CHANNEL_EOF_FUNC( channel_eof_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_eof( ssh_channel->psv, ssh_channel );
}

static LIBSSH2_CHANNEL_CLOSE_FUNC( channel_close_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_close( ssh_channel->psv, ssh_channel );
}

static LIBSSH2_CHANNEL_DATA_FUNC( channel_data_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_data( ssh_channel->psv, ssh_channel, stream, buffer, length );
}

ssh_channel_data_cb sack_ssh_set_channel_data( struct ssh_channel*channel, void(*channel_data)(uintptr_t,struct ssh_channel*,int stream, const uint8_t*,size_t) ){
	ssh_channel_data_cb cb = channel->channel_data;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_DATA, (libssh2_cb_generic*)channel_data_relay );
	channel->channel_data = channel_data;
	return cb;
}

ssh_channel_eof_cb sack_ssh_set_channel_eof( struct ssh_channel*channel, void(*channel_eof)(uintptr_t,struct ssh_channel*) ){
	ssh_channel_eof_cb cb = channel->channel_eof;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_EOF, (libssh2_cb_generic*)channel_eof_relay );
	channel->channel_eof = channel_eof;
	return cb;
}

ssh_channel_close_cb sack_ssh_set_channel_close( struct ssh_channel*channel, void(*channel_close)(uintptr_t,struct ssh_channel*) ){
	ssh_channel_close_cb cb = channel->channel_close;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_CLOSE, (libssh2_cb_generic*)channel_close_relay );
	channel->channel_close = channel_close;
	return cb;
}

//----------------------------- Actions

void sack_auth_user_password( struct ssh_session*session, CTEXTSTR user, CTEXTSTR pass ){
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen(user), (uintptr_t)StrDup( pass ), StrLen( pass )}};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen( user ), (uintptr_t)StrDup( pass ), StrLen(pass) });
	libssh2_userauth_password_ex( session->session, user, (unsigned int)session->pending.state_data[1], pass, (unsigned int)session->pending.state_data[3], pwChange );
}

void sack_auth_user_cert( struct ssh_session*session, CTEXTSTR user
                        , CTEXTSTR pubkey
                        , CTEXTSTR privkey
                        , CTEXTSTR pass ){
	session->user = StrDup( user );
	session->user_len = StrLen( user );
	session->pubKey = StrDup( pubkey );
	session->pubKey_len = StrLen( pubkey );
	session->privKey = StrDup( privkey );
	session->privKey_len = StrLen( privkey );
	session->pass = StrDup( pass );
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_AUTH_PK};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PK );
	libssh2_userauth_publickey_frommemory( session->session
	                                     , user, strlen( user )
	                                     , pubkey, strlen( pubkey )
	                                     , privkey, strlen( privkey )
	                                     , pass );

}

void sack_ssh_channel_open_v2( struct ssh_session* session, CTEXTSTR type, size_t type_len, uint32_t window_size, uint32_t packet_size,
		CTEXTSTR message, size_t message_len ) {
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = { SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)StrDup( message ), message_len } };
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)StrDup( message ), message_len } );
	libssh2_channel_open_ex( session->session, type, (unsigned int)type_len, window_size, packet_size, message, (unsigned int)message_len  );
}

void sack_ssh_channel_open( struct ssh_session *session ) {
	sack_ssh_channel_open_v2( session, "session", 7, 0x4000, 0x1000, NULL, 0 );
}


void sack_ssh_channel_request_pty( struct ssh_channel* channel, CTEXTSTR term ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_REQUEST_PTY, {(uintptr_t)channel, (uintptr_t)StrDup(term)}};
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_REQUEST_PTY, { (uintptr_t)channel, (uintptr_t)StrDup( term ) } );
	libssh2_channel_request_pty( channel->channel, term );
}

void sack_ssh_channel_setenv( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel}};
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel});
	libssh2_channel_setenv( channel->channel, key, value );
}

void sack_ssh_channel_shell( struct ssh_channel* channel ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SHELL, { (uintptr_t)channel } };
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SHELL, { (uintptr_t)channel } );
	libssh2_channel_shell( channel->channel );
}
