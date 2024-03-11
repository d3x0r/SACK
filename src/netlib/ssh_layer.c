
#include <stdhdrs.h>
#include <network.h>
#include <sack_ssh.h>
#include <html5.websocket.h>
#include <html5.websocket.client.h>

SACK_NETWORK_NAMESPACE
#ifdef __cplusplus
namespace ssh {
#endif

#include "ssh_layer.h"

static struct local_ssh_layer_data {
	struct ssh_session* connecting_session;
} local_ssh_layer_data;

static LIBSSH2_LISTERNER_CONNECT_FUNC( listener_connect_relay );

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
	if( buf ) buf[idx] = buf0;
	
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
				buf->used = buf->length = 0;
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
		//lprintf( "Received %d", length );

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
						int err; CTEXTSTR errmsg; int errmsglen;
						err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
						if( cs->error ) {
							cs->error( cs->psv, err, errmsg, errmsglen );
						} else {
							lprintf( "Session Error(%d):%s", err, errmsg );
						}
						cs->pending.state = SSH_STATE_RESET; // allow a different connect?
						while( !IsDataQueueEmpty( &cs->pdqStates ) ) {
							// remove all other events too...
							DequeData( &cs->pdqStates, NULL );
						}
						RemoveClient( pc );
						return;
					}
					else {
						CTEXTSTR fingerprint = libssh2_hostkey_hash( cs->session, LIBSSH2_HOSTKEY_HASH_SHA1 );
						cs->pending.state = SSH_STATE_RESET;
						cs->handshake_complete( cs->psv, (const uint8_t*)fingerprint );
					}
				}
				break;
			case SSH_STATE_AUTH_PK:
				//libssh2_userauth_publickey_fromfile( cs->session, cs->user, NULL, "rsa", cs->pass );
				rc = libssh2_userauth_publickey_frommemory( cs->session, cs->user, cs->user_len
														, (const char*)cs->pubKey, cs->pubKey_len
														, (const char*)cs->privKey, cs->privKey_len
														, cs->pass );
				if( rc == LIBSSH2_ERROR_EAGAIN ) break;
				else if( rc ) {
					// error
					int err; CTEXTSTR errmsg; int errmsglen;
					err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
					if( cs->error ) {
						cs->error( cs->psv, err, errmsg, errmsglen );
					} else if( cs->auth_complete ) {
						cs->auth_complete( cs->psv, FALSE );
					} else {
						lprintf( "Session Error(%d):%s", err, errmsg );
					}
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
			case SSH_STATE_LISTEN:
				{
					struct ssh_session* session = (struct ssh_session*)cs->pending.state_data[0];
					CTEXTSTR remotehost = (CTEXTSTR)cs->pending.state_data[1];
					int remoteport = (int)cs->pending.state_data[2];
					int bound_port;// (int*)cs->pending.state_data[3];
					LIBSSH2_LISTENER* listener = libssh2_channel_forward_listen_ex( session->session, remotehost, remoteport, &bound_port, 4096 );
					if( !listener ) {
						rc = libssh2_session_last_error( session->session, NULL, NULL, 0 );
						if( rc == LIBSSH2_ERROR_EAGAIN ) break;
						else if( rc ) {
							// error
							if( session->forward_listen_cb )
								session->forward_listen_cb( session->psv, NULL, 0 );
							cs->pending.state = SSH_STATE_RESET;
							//RemoveClient( pc );
						} else {
							lprintf( "no listener, no error?" );
						}
					} else {
						struct ssh_listener *ssh_listener = NewArray( struct ssh_listener, 1);
						ssh_listener->listener = listener;
						ssh_listener->session = session;
						ssh_listener->listen_connect_cb = (ssh_forward_listen_accept_cb)cs->pending.state_data[3];
						if( ssh_listener->listen_connect_cb ) {
							libssh2_listener_callback_set( listener, LIBSSH2_CALLBACK_LISTENER_ACCEPT, (libssh2_cb_generic*)listener_connect_relay );
							libssh2_listener_abstract( listener)[0] = (void*)ssh_listener;
						}

						if( session->forward_listen_cb )
							ssh_listener->psv = session->forward_listen_cb( session->psv, ssh_listener, bound_port );
						cs->pending.state = SSH_STATE_RESET;
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
							int err; CTEXTSTR errmsg; int errmsglen;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
							if( cs->error ) {
								cs->error( cs->psv, err, errmsg, errmsglen );
							} else {
								lprintf( "Session Error(%d):%s", err, errmsg );
							}
						}
						else {
							lprintf( "no channel, no error?");
						}
					}else {
						struct ssh_channel* ssh_channel = NewArray( struct ssh_channel, 1);
						MemSet( ssh_channel, 0, sizeof( struct ssh_channel ) );
						((uintptr_t*)libssh2_channel_abstract( channel ))[0] = (uintptr_t)ssh_channel;
						Deallocate( CTEXTSTR, type );
						Deallocate( CTEXTSTR, message );
						ssh_channel->session = cs;
						ssh_channel->channel = channel;
						ssh_open_cb cb = (ssh_open_cb)cs->pending.state_data[6];
						if( cb )
							ssh_channel->psv = cb( cs->psv, ssh_channel );
						else
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
						if( channel->error ) {
							int err; CTEXTSTR errmsg; int errmsglen;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
							channel->error( channel->psv, err, errmsg, errmsglen );
						} else if( channel->pty_open ) {
							channel->pty_open( cs->psv, FALSE );
						} else {
							int err; CTEXTSTR errmsg;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, NULL, 0 );
							lprintf( "PTY Error(%d):%s", err, errmsg );
						}
						cs->pending.state = SSH_STATE_RESET;
					}
					else {
						if( cs->pending.state_data[2] )
							( (ssh_pty_cb)cs->pending.state_data[2] )( channel->psv, TRUE );
						else if( channel->pty_open )
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
						if( channel->error ) {
							int err; CTEXTSTR errmsg; int errmsglen;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
							channel->error( channel->psv, err, errmsg, errmsglen );
						} else if( cs->pending.state_data[3] )
							( (ssh_pty_cb)cs->pending.state_data[3] )( channel->psv, FALSE );
						else {
							int err;
							char* msg;
							int msglen;
							err = libssh2_session_last_error( cs->session, &msg, &msglen, 0 );
							lprintf( "error setting env:%d $d %s", rc, err, msg );
						}
						cs->pending.state = SSH_STATE_RESET;
						Deallocate( CTEXTSTR, key );
						Deallocate( CTEXTSTR, value );
						//RemoveClient( pc );
					}
					else {
						if( cs->pending.state_data[3] )
							( (ssh_pty_cb)cs->pending.state_data[3] )( channel->psv, TRUE );

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
						if( channel->error ) {
							int err; CTEXTSTR errmsg; int errmsglen;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
							channel->error( channel->psv, err, errmsg, errmsglen );
						} else if( channel->shell_open ) {
							channel->shell_open( channel->psv, FALSE );
						} else {
							int err;
							char* msg;
							int msglen;
							err = libssh2_session_last_error( cs->session, &msg, &msglen, 0 );
							lprintf( "Error starting Shell:%d %s", err, msg );
						}
					}
					else {
						if( cs->pending.state_data[1] )
							( (ssh_shell_cb)cs->pending.state_data[1] )( channel->psv, TRUE );
						else 
							if( channel->shell_open )
							channel->shell_open( channel->psv, TRUE );
						cs->pending.state = SSH_STATE_RESET;
					}
				}
				break;
			case SSH_STATE_EXEC:
				{
					struct ssh_channel* channel = (struct ssh_channel*)cs->pending.state_data[0];
					CTEXTSTR shell = (CTEXTSTR)cs->pending.state_data[1];
					rc = libssh2_channel_exec( channel->channel, shell );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						if( channel->error ) {
							int err; CTEXTSTR errmsg; int errmsglen;
							err = libssh2_session_last_error( cs->session, (char**)&errmsg, &errmsglen, 0 );
							channel->error( channel->psv, err, errmsg, errmsglen );
						} else if( channel->exec_done ) {
							channel->exec_done( cs->psv, FALSE );
						} else {
							int err;
							char* msg;
							int msglen;
							err = libssh2_session_last_error( cs->session, &msg, &msglen, 0 );
							lprintf( "Error Exec:%d %s", err, msg );
						}
			}
					else {
						if( cs->pending.state_data[2] )
							( (ssh_exec_cb)cs->pending.state_data[2] )( channel->psv, TRUE );
						else if( channel->exec_done )
							channel->exec_done( cs->psv, TRUE );
						cs->pending.state = SSH_STATE_RESET;
						Deallocate( CTEXTSTR, shell );
					}
				}
				break;
			case SSH_STATE_SFTP:
				{
					LIBSSH2_SFTP *sftp = libssh2_sftp_init( cs->session );
					if( !sftp ) {
						rc = libssh2_session_last_error( cs->session, NULL, NULL, 0 );
						if( rc == LIBSSH2_ERROR_EAGAIN ) break;
						else if( rc ) {
							// error
							//RemoveClient( pc );
							//return;
						}
						else {
							lprintf( "no sftp, no error?");
						}
					}
					else {
						struct ssh_sftp* ssh_sftp = NewArray( struct ssh_sftp, 1);
						MemSet( ssh_sftp, 0, sizeof( struct ssh_sftp ) );
						//((uintptr_t*)libssh2_sftp_abstract( sftp ))[0] = (uintptr_t)ssh_sftp;
						ssh_sftp->psv = cs->sftp_open( cs->psv, ssh_sftp );
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
		if( !db->length ) {
			// only queue 1 more read, not every more read.
			ReadTCP( pc, db->buffer, 4096 );
			return;
		}
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
	if( !pc )
		session = local_ssh_layer_data.connecting_session;
	if( error ) {
		// error

#ifdef WIN32
		char msgbuf[256];   // for a message up to 255 bytes.
		msgbuf[0] = '\0';    // Microsoft doesn't guarantee this on man page.

		DWORD errmsglen = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
			NULL,                // lpsource
			error,               // message id
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),    // languageid
			msgbuf,              // output buffer
			sizeof( msgbuf ),     // size of msgbuf, bytes
			NULL );               // va_list of arguments

		CTEXTSTR errmsg = msgbuf;
#else
		CTEXTSTR errmsg = strerror( error ); int errmsglen = strlen( errmsg );
#endif
		//err = libssh2_session_last_error( session->session, (char*)&errmsg, NULL, 0 );
		if( session->error ) {
			session->error( session->psv, error, errmsg, errmsglen );
		} else {
			lprintf( "Connection Error(%d):%s", error, errmsg );
		}
		// could have queued authorization and other events already...
		while( !IsDataQueueEmpty( &session->pdqStates ) ) {
			// remove all other events too...
			DequeData( &session->pdqStates, NULL );
		}
		session->pending.state = SSH_STATE_RESET; // allow a different connect?

	}
	else {
		// success
		set_pending_state( session->pending, SSH_STATE_HANDSHAKE );
		libssh2_session_handshake( session->session, 1/*sock*/ );
	}
	local_ssh_layer_data.connecting_session = NULL;
}

static void closeCallback( PCLIENT pc ) {
	struct ssh_session* session = (struct ssh_session*)GetNetworkLong( pc, 0 );
	libssh2_session_free( session->session );
	ReleaseEx( session DBG_SRC );
}

//----------------------------- Utility Functions -----------------------

static void ReleaseCert( struct keyparts* bincert ) {
	ReleaseEx( bincert->leadin DBG_SRC );
	ReleaseEx( bincert DBG_SRC );
}

//----------------------------- Session -----------------------

struct ssh_session * sack_ssh_session_init(uintptr_t psv){
	struct ssh_session * session = NewArray( struct ssh_session, 1);
	MemSet( session, 0, sizeof( struct ssh_session ) );
	session->buffers = NULL;
	session->pdqStates = CreateDataQueue( sizeof( struct pending_state ) );
	session->psv = psv;
	session->session = libssh2_session_init_ex(alloc_callback, free_callback, realloc_callback, session);
	libssh2_session_banner_set( session->session, "SSH-2.0-sack_2.0+libssh2_1.9.0" );

	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_SEND, (libssh2_cb_generic*)SendCallback );
	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_RECV, (libssh2_cb_generic*)RecvCallback );
	libssh2_session_set_blocking( session->session, 0 );

	return session;
}

void sack_ssh_session_connect( struct ssh_session* session, CTEXTSTR host, int port ) {
	if( session->pending.state != SSH_STATE_RESET ) {
		lprintf( "session is already in progress" );
	}
	while( local_ssh_layer_data.connecting_session ) {
		// wait for previous connection to complete...
		WakeableSleep( 10 );	
	}
	local_ssh_layer_data.connecting_session = session;
	session->pc = OpenTCPClientExxx( host, port?port:22, readCallback, closeCallback, NULL, connectCallback, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	if( session->pc ) {
		// make sure to block other connections until handshake resolves...
		session->pending.state = SSH_STATE_CONNECTING;
		SetNetworkLong( session->pc, 0, (uintptr_t)session );
		NetworkConnectTCP( session->pc );
	}
}

void sack_ssh_trace( struct ssh_session* session, int bitmask ) {
	libssh2_trace( session->session, bitmask );
}

void sack_ssh_get_error( struct ssh_session* session, int* err, CTEXTSTR* errmsg ) {
	err[0] = libssh2_session_last_error( session->session, (char**)errmsg, NULL, 0 );
}

void sack_ssh_session_close( struct ssh_session*session ){
	// prevent callback from being triggered
	SetNetworkCloseCallback( session->pc, NULL );
	SetNetworkReadComplete( session->pc, NULL );

	RemoveClient( session->pc );
	if( session->bincert )
		ReleaseCert( session->bincert );
	if( session->user ) ReleaseEx( (POINTER)session->user DBG_SRC );
	if( session->pass ) ReleaseEx( (POINTER)session->pass DBG_SRC );
	if( session->pdqStates ) DeleteDataQueue( &session->pdqStates );
	if( session->session ) libssh2_session_free( session->session );

	// might ave a pending read with a buffer reference....
	if( session->buffers ) {
		INDEX idx;
		struct data_buffer* buffer;
		DATA_FORALL( session->buffers, idx, struct data_buffer*, buffer ) {
			ReleaseEx( buffer->buffer DBG_SRC );
		}
		DeleteDataList( &session->buffers );
	}

}

//----------------------------- Callbacks -----------------------

ssh_session_error_cb sack_ssh_set_session_error( struct ssh_session* session, ssh_session_error_cb error_cb ) {
	ssh_session_error_cb cb = session->error;
	session->error = error_cb;
	return cb;
}

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

ssh_forward_listen_cb sack_ssh_set_forward_listen( struct ssh_session*session, ssh_forward_listen_cb forward_listen_cb ){
	ssh_forward_listen_cb cb = session->forward_listen_cb;
	session->forward_listen_cb = forward_listen_cb;
	return cb;
}

ssh_forward_connect_cb sack_ssh_set_forward_connect( struct ssh_session* session, ssh_forward_connect_cb forward_connect_cb ) {
	ssh_forward_connect_cb cb = session->forward_connect_cb;
	session->forward_connect_cb = forward_connect_cb;
	return cb;
}

static LIBSSH2_LISTERNER_CONNECT_FUNC( listener_connect_relay ) {
	struct ssh_listener* ssh_listener = (struct ssh_listener*)listener_abstract[0];
	struct ssh_channel* ssh_channel = NewArray( struct ssh_channel, 1 );
	MemSet( ssh_channel, 0, sizeof( *ssh_channel ) );
	libssh2_channel_abstract( channel )[0] = (void*)ssh_channel;

	ssh_channel->session = (struct ssh_session*)session_abstract[0];
	//channel->channel = libssh2_channel_accept_ex( listener->listener, (void*)channel );
	ssh_channel->channel = channel;
	ssh_channel->psv = ssh_listener->listen_connect_cb( ssh_listener->psv, ssh_channel );
}

ssh_forward_listen_accept_cb sack_ssh_set_forward_listen_accept( struct ssh_listener* listener, ssh_forward_listen_accept_cb connect_cb ) {
	ssh_forward_listen_accept_cb cb = listener->listen_connect_cb;
	libssh2_listener_callback_set( listener->listener, LIBSSH2_CALLBACK_LISTENER_ACCEPT, (libssh2_cb_generic*)listener_connect_relay );
	listener->listen_connect_cb = connect_cb;
	return cb;
}

ssh_pty_cb sack_ssh_set_pty_open( struct ssh_channel*channel, void(*pty_open)(uintptr_t,LOGICAL) ){
	ssh_pty_cb cb = channel->pty_open;
	channel->pty_open = pty_open;
	return cb;
}

ssh_shell_cb sack_ssh_set_shell_open( struct ssh_channel*channel, void(*shell_open)(uintptr_t,LOGICAL) ){
	ssh_shell_cb cb = channel->shell_open;
	channel->shell_open = shell_open;
	return cb;
}

ssh_exec_cb sack_ssh_set_exec_done( struct ssh_channel* channel, void( *exec_done )( uintptr_t, LOGICAL ) ) {
	ssh_exec_cb cb = channel->exec_done;
	channel->exec_done = exec_done;
	return cb;
}

static LIBSSH2_CHANNEL_EOF_FUNC( channel_eof_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_eof( ssh_channel->psv );
}

static LIBSSH2_CHANNEL_CLOSE_FUNC( channel_close_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_close( ssh_channel->psv );
}

static LIBSSH2_CHANNEL_DATA_FUNC( channel_data_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	ssh_channel->channel_data( ssh_channel->psv, stream, buffer, length );
}

ssh_channel_data_cb sack_ssh_set_channel_data( struct ssh_channel*channel, ssh_channel_data_cb channel_data ){
	ssh_channel_data_cb cb = channel->channel_data;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_DATA, (libssh2_cb_generic*)channel_data_relay );
	channel->channel_data = channel_data;
	return cb;
}

ssh_channel_eof_cb sack_ssh_set_channel_eof( struct ssh_channel*channel, ssh_channel_eof_cb channel_eof ){
	ssh_channel_eof_cb cb = channel->channel_eof;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_EOF, (libssh2_cb_generic*)channel_eof_relay );
	channel->channel_eof = channel_eof;
	return cb;
}

ssh_channel_close_cb sack_ssh_set_channel_close( struct ssh_channel*channel, ssh_channel_close_cb channel_close ){
	ssh_channel_close_cb cb = channel->channel_close;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_CLOSE, (libssh2_cb_generic*)channel_close_relay );
	channel->channel_close = channel_close;
	return cb;
}

ssh_channel_error_cb sack_ssh_set_channel_error( struct ssh_channel* channel, ssh_channel_error_cb error ){
	ssh_channel_error_cb cb = channel->error;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_CLOSE, (libssh2_cb_generic*)channel_close_relay );
	channel->error = error;
	return cb;


}

ssh_sftp_open_cb sack_ssh_set_sftp_open( struct ssh_session* session, ssh_sftp_open_cb sftp_open ) {
	ssh_sftp_open_cb cb = session->sftp_open;
	session->sftp_open = sftp_open;
	return cb;
}

//----------------------------- Actions -----------------------

static struct keyparts* CertToBinary( const char* cert, size_t* result ) {
	char* outbuf = NewArray( char, StrLen( cert ) );
	int nibble = 0;
	int byte = 0;
	size_t bytes = 0;
	int line_eol = 0;
	int skip_to_end = 0;
	char* outp = outbuf;
	const char* p = cert;
	while( p[0] ) {
		if( !skip_to_end && p[0] == '-' ) {
			skip_to_end = 1;
			if( line_eol ) {
				p++;
				continue;
			}
		}
		if( p[0] == '\r' || p[0] == '\n' ) {
			line_eol = 1;
			skip_to_end = 0;
		} else line_eol = 0;
		if( skip_to_end ) {
			if( !line_eol ) {
				p++;  continue;
			} else skip_to_end = 0;
		}
		if( line_eol ) {
			p++; continue;
		}
		outp[0] = p[0];
		outp++; p++;
	}
	bytes = outp - outbuf;
	uint8_t* newbuf = DecodeBase64Ex( (const char*)outbuf, bytes, result, NULL );
	// done with the temp buffer to decode
	ReleaseEx( outbuf DBG_SRC );

	//size_t outlen;
	//char* testbuf = EncodeBase64Ex( newbuf, *result, &outlen, NULL );
	struct keyparts* data_ = NewArray( struct keyparts, 1 );
#define data (data_[0])
	data.leadin = (char*)newbuf;
	{
		int nul = 0;
		int ofs;
		int p;
		while( nul < result[0] && data.leadin[nul] != 0 ) nul++;
		ofs = nul + 1;
		if( StrCmp( data.leadin, "openssh-key-v1" ) ) {
			lprintf( "Certificate type not supported: %s, expected openssh-key-v1", data.leadin );
			Deallocate( struct keyparts*, data_ );
			Deallocate( uint8_t*, newbuf );
			return NULL;
		}
		DebugBreak();
		for( p = 0; p < 3; p++ ) {
			data.parts[p].textlen = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.parts[p].text = (uint8_t*)&data.leadin[ofs + 4];
			ofs += 4 + data.parts[p].textlen;
			if( !data.parts[p].textlen ) break;
		}
		data.numkeys = NTOHL( *(uint32_t*)&data.leadin[ofs] );
		ofs += 4;
		for( p = 0; p < data.numkeys; p++ ) {
			data.keys[p].pubkeylen = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].pubkey = (uint8_t*)(data.leadin + ofs + 4);
			ofs += data.keys[p].pubkeylen + 4;

			data.keys[p].privkeylen = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].privkey = (uint8_t*)( data.leadin + ofs + 4 );
			ofs += data.keys[p].privkeylen + 4;

			uint32_t rand1 = ( (uint32_t*)data.keys[p].privkey )[0];
			uint32_t rand2 = ( (uint32_t*)data.keys[p].privkey )[1];
			data.keys[p].privkey += 8;
			/* pubkey parts

			data.keys[p].textlen = *(uint32_t*)&data.leadin[ofs + 4];
			data.keys[p].textlen = NTOHL( data.keys[p].textlen );
			data.keys[p].text = &data.leadin[ofs + 4 + 4];
			ofs += data.keys[p].textlen + 4 + 4;
			data.keys[p].rndlen = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].rnd = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].rndlen + 4;
			data.keys[p].pubkeylen1 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].pubkey1 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].pubkeylen1 + 4;


			ofs = ( data.keys[p].text - data.leadin ) + data.keys[p].textlen + ( ( data.keys[p].keylen + 7 ) & 0xFFFF8 );
			*/

			/* privkey parts

			data.keys[p].textlen2 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].text2 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].textlen2 + 4;
			data.keys[p].datalen2 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].data2 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].datalen2 + 4;

			data.keys[p].rndlen2 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].rnd2 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].rndlen2 + 4;

			data.keys[p].datalen3 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].data3 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].datalen3 + 4;

			data.keys[p].datalen4 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].data4 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].datalen4 + 4;

			data.keys[p].datalen5 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].data5 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].datalen5 + 4;

			data.keys[p].datalen6 = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].data6 = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].datalen6 + 4;

			data.keys[p].commentlen = NTOHL( *(uint32_t*)&data.leadin[ofs] );
			data.keys[p].comment = (uint8_t*)&data.leadin[ofs + 4];
			ofs += data.keys[p].commentlen + 4;
			data.keys[p].padlen = ( ( ( ofs - nul ) + 7 ) & 0xFFFF8 ) - ( ofs - nul );
			data.keys[p].finalpad = (uint8_t*)( data.leadin + ofs );
			ofs += data.keys[p].padlen;
			*/

			//data.leadin + ofs + 4 + data.keys[p].textlen + data.keys[p].keylen
		}
	}
	return data_;
#undef data
}

void sack_ssh_auth_user_password( struct ssh_session*session, CTEXTSTR user, CTEXTSTR pass ){
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen(user), (uintptr_t)StrDup( pass ), StrLen( pass )}};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen( user ), (uintptr_t)StrDup( pass ), StrLen(pass) });
	libssh2_userauth_password_ex( session->session, user, (unsigned int)session->pending.state_data[1], pass, (unsigned int)session->pending.state_data[3], pwChange );
}

void sack_ssh_auth_user_cert( struct ssh_session*session, CTEXTSTR user
                        , uint8_t* pubkey, size_t pubkeylen
                        , uint8_t* privkey, size_t privkeylen
                        , CTEXTSTR pass ){
	session->user = StrDup( user );
	session->user_len = StrLen( user );
	
	if( privkey ) {
		if( privkey[0] == '\0' ) {
			struct keyparts* bincert = CertToBinary( (const char*)privkey, &privkeylen );
			if( !bincert ) {
				return;
			}
			session->bincert = bincert;
			session->pubKey = bincert->keys[0].pubkey; // some part of keyparts?
			session->pubKey_len = bincert->keys[0].pubkeylen;
			session->privKey = bincert->keys[0].privkey;
			session->privKey_len = bincert->keys[0].privkeylen;
		}
		else {
			if( 0 && !pubkey ) {
				static const char msg[] = "If you're passing the binary parts of a certificate, need to supply pubkey also";
				if( session->error ) {
					session->error( session->psv, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, msg, sizeof( msg ) );
				} else
					lprintf( msg );
				return;
			}

			session->privKey = privkeylen ? NewArray( uint8_t, privkeylen ) : NULL;
			if( privkeylen )
				MemCpy( session->privKey, privkey, privkeylen );
			session->privKey_len = privkeylen;

			session->pubKey = pubkeylen ? NewArray( uint8_t, pubkeylen ) : NULL;
			if( pubkeylen )
				MemCpy( session->pubKey, pubkey, pubkeylen );
			session->pubKey_len = pubkeylen;
		}
	} else {
		static const char msg[] = "auth_user_cert: Must specify private key to use.";
		if( session->error ) {
			session->error( session->psv, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, msg, sizeof( msg ) );
		} else
			lprintf( msg );
		return;
	}

	session->pass = StrDup( pass );
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_AUTH_PK};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PK );
	//libssh2_userauth_publickey_fromfile( session->session, user, NULL, "rsa", pass );

	libssh2_userauth_publickey_frommemory( session->session
	                                     , user, strlen( user )
	                                     , (const char*)session->pubKey, session->pubKey_len
	                                     , (const char*)session->privKey, session->privKey_len
	                                     , pass );

}

void sack_ssh_channel_open_v2( struct ssh_session* session, CTEXTSTR type, size_t type_len, uint32_t window_size, uint32_t packet_size,
	CTEXTSTR message, size_t message_len, ssh_open_cb cb ) {
	if( session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = { SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)StrDup( message ), message_len, (uintptr_t)cb }};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)StrDup( message ), message_len, (uintptr_t)cb } );
	libssh2_channel_open_ex( session->session, type, (unsigned int)type_len, window_size, packet_size, message, (unsigned int)message_len );
}

void sack_ssh_channel_free( struct ssh_channel* channel ){
	libssh2_channel_close( channel->channel );
}

void sack_ssh_channel_open( struct ssh_session *session ) {
	sack_ssh_channel_open_v2( session, "session", 7, 0x4000, 0x1000, NULL, 0, NULL );
}

void sack_ssh_channel_close( struct ssh_channel* channel ) {
	libssh2_channel_close( channel->channel );
	ReleaseEx( channel DBG_SRC );
}

void sack_ssh_channel_request_pty( struct ssh_channel* channel, CTEXTSTR term, ssh_pty_cb cb ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_REQUEST_PTY, {(uintptr_t)channel, (uintptr_t)StrDup(term), (uintptr_t)cb}};
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_REQUEST_PTY, { (uintptr_t)channel, (uintptr_t)StrDup( term ), (uintptr_t)cb } );
	libssh2_channel_request_pty( channel->channel, term );
}

void sack_ssh_channel_setenv( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value, ssh_setenv_cb cb ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel, (uintptr_t)cb}};
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel, (uintptr_t)cb });
	libssh2_channel_setenv( channel->channel, key, value );
}

void sack_ssh_channel_shell( struct ssh_channel* channel, ssh_shell_cb cb ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SHELL, { (uintptr_t)channel, (uintptr_t)cb } };
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SHELL, { (uintptr_t)channel, (uintptr_t)cb } );
	libssh2_channel_shell( channel->channel );
}

void sack_ssh_channel_exec( struct ssh_channel* channel, CTEXTSTR shell, ssh_exec_cb cb ) {
	if( channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_EXEC, { (uintptr_t)channel, (uintptr_t)StrDup( shell ), (uintptr_t)cb}};
		EnqueData( &channel->session->pdqStates, &state );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_EXEC, { (uintptr_t)channel, (uintptr_t)StrDup( shell ), (uintptr_t)cb } );
	libssh2_channel_exec( channel->channel, (CTEXTSTR)channel->session->pending.state_data[1] );
}

void sack_ssh_channel_write( struct ssh_channel* channel, int stream, const uint8_t* buffer, size_t length ) {
	libssh2_channel_write_ex( channel->channel, stream, (const char*)buffer, length );
}

//------------------------- SOCKET FORWARDING ----------------------------

void sack_ssh_channel_forward_listen( struct ssh_session* session, CTEXTSTR remotehost, uint16_t remoteport, ssh_forward_listen_cb cb ) {
	if( session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_LISTEN, { (uintptr_t)session, (uintptr_t)StrDup( remotehost ), remoteport, (uintptr_t)cb}};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_LISTEN, { (uintptr_t)session, (uintptr_t)StrDup( remotehost ), remoteport, (uintptr_t)cb } );
	// bound_port can't be used until much later... this will be a wait for completion.
	libssh2_channel_forward_listen_ex( session->session, remotehost, remoteport, NULL, 4096 );
}

static void AcceptedClient( PCLIENT pc, PCLIENT pcNew ) {
	struct ssh_director_listener* listen = (struct ssh_director_listener*)GetNetworkLong( pc, 0 );

	struct ssh_director* director = NewArray( struct ssh_director, 1 );
	director->pc = pcNew;
	director->director = listen;

	if( listen->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_FORWARD, { (uintptr_t)director } };
		EnqueData( &listen->session->pdqStates, &state );
		return;
	}	

	set_pending_state( listen->session->pending, SSH_STATE_FORWARD, { (uintptr_t)director } );
	libssh2_channel_direct_tcpip_ex( listen->session->session, listen->remoteAddr,
		listen->remotePort, listen->localAddr, listen->localPort );

}



PCLIENT sack_ssh_forward_connect( struct ssh_session* session
		, CTEXTSTR localAddress, int localPort
		, CTEXTSTR remoteAddress, int remotePort
		, ssh_forward_connect_cb cb ) {
	PSOCKADDR addr = CreateRemote( localAddress, localPort );
	if( !addr ) {
		if( session->error )
			session->error( session->psv, -EINVAL, "Bad address for local connection", 32 );
		return NULL;
	}
	PCLIENT pc = OpenTCPListenerAddr_v2( addr, AcceptedClient, TRUE );
	if( !pc ) {
		ReleaseAddress( addr );
		if( session->error )
			session->error( session->psv, -EINVAL, "Failed to open listen socket", 28 );
		return NULL;

	}
	struct ssh_director_listener* director = NewArray( struct ssh_director_listener, 1 );
	SetNetworkLong( pc, 0, (uintptr_t)director );
	director->pc = pc;
	director->remoteAddr = StrDup( remoteAddress );
	director->remotePort = remotePort;
	director->localAddr = StrDup( localAddress );
	director->localPort = localPort;

	ReleaseAddress( addr );
	return pc;
}
// libssh2_channel_direct_tcpip_ex

//void sack_ssh_channel_

//------------------------- SFTP Interface ----------------------------

void sack_ssh_sftp_init( struct ssh_session* session ) {
	if( session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SFTP, { (uintptr_t)session }};
		EnqueData( &session->pdqStates, &state );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_SFTP, { (uintptr_t)session } );
	// can never complete immediately...
	libssh2_sftp_init( session->session );
}

//------------------------- Websocket Interface ----------------------------

static void WebSocketData( uintptr_t psv, int stream, const uint8_t* data, size_t len ) {
	WebSocketWrite( ( (struct ssh_websocket*)psv )->ws, data, len );
}

static int websocketSendSSH( uintptr_t psv, CPOINTER buffer, size_t length ) {
	struct ssh_websocket* ws = (struct ssh_websocket*)psv;
	sack_ssh_channel_write( ws->channel, 0, (const uint8_t *)buffer, length );
	return 0;
}

static void ssh_ws_open( uintptr_t psv ) {
	struct ssh_websocket* ws = (struct ssh_websocket*)psv;
	
}

/*
* SSH channel to serve websocket
* This can be called when a SSH channel is in the open callback from a remote TCP connection
* This will also fallback to serve HTTP requests to the socket.
*/
struct ssh_websocket* sack_ssh_channel_serve_websocket( struct ssh_channel* channel,
	web_socket_opened ws_open,
	web_socket_event ws_event,
	web_socket_closed ws_close,
	web_socket_error ws_error,
	web_socket_http_request ws_http,
	web_socket_http_close ws_http_close,
	web_socket_completion ws_completion,
	uintptr_t psv
	) {
	struct ssh_websocket* ws = NewArray( struct ssh_websocket, 1 );
	ws->channel = channel;
	channel->psv = (uintptr_t)ws;
	channel->channel_data = WebSocketData;
	ws->ws = WebSocketCreateServerPipe( websocketSendSSH, (uintptr_t)ws
		, ws_open, ws_event, ws_close, ws_error
		, ws_http, ws_http_close, ws_completion
		, psv );
	return ws;
}

#ifdef __cplusplus
}
#endif
SACK_NETWORK_NAMESPACE_END
