
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
	PLIST connecting;
	CRITICALSECTION csLock;
} local_ssh_layer_data;

static LIBSSH2_LISTENER_CONNECT_FUNC( listener_connect_relay );
static void CPROC ForwardSocketRead( PCLIENT pc, POINTER buffer, size_t length );
static void CPROC ForwardSocketClose( PCLIENT pc );
static void ForwardChannelData( uintptr_t psv, int stream, const uint8_t* data, size_t length );
static void ForwardChannelEOF( uintptr_t psv );
static void ForwardChannelClose( uintptr_t psv );
static int OpenForwardDirector( struct ssh_director* director );

#ifdef __cplusplus
#define set_pending_state(ps,s,...) { pending_state tmp = {s,##__VA_ARGS__}; ps= tmp; }
#else
#define set_pending_state(ps,s,...) ps = (struct pending_state){s,##__VA_ARGS__}
#endif

PRELOAD( initSSH ) {
	InitializeCriticalSection( &local_ssh_layer_data.csLock );
}

static void* alloc_callback( size_t size, void** abstract ) {
	return AllocateEx( size DBG_SRC );
}

static void free_callback( void *data, void** abstract ) {
	ReleaseEx( data DBG_SRC );
}
static void* realloc_callback( void* p, size_t size, void** abstract ) {
	return ReallocateEx( p, size DBG_SRC );
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
	if( cs->pc )
		SendTCP( cs->pc, buffer, length );
	else if( cs->write )
		cs->write( cs->psv, (uint8_t*)buffer, length );
	return length;
}


// move the buffer at `at` (just fully consumed) to the end of the list, keeping the
// rest in order: filled buffers stay at the front in arrival order, free ones behind.
static void moveBuffers( PDATALIST *pdl, INDEX at ) {
	struct data_buffer* bufs;
	struct data_buffer moved;
	INDEX idx;
	if( !(*pdl) || (*pdl)->Cnt < 2 || at >= (*pdl)->Cnt - 1 ) return;
	bufs = (struct data_buffer*)(*pdl)->data;
	moved = bufs[at];
	for( idx = at + 1; idx < (*pdl)->Cnt; idx++ )
		bufs[idx-1] = bufs[idx];
	bufs[(*pdl)->Cnt-1] = moved;
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
	// flushing output (possibly on the application's thread) must not process
	// incoming packets; their callbacks belong on the network thread.
	if( cs->block_recv ) return -EAGAIN;
	while( !cs->buffers || !cs->buffers->Cnt ) {
		return -EAGAIN;
	}
	// Fill as much of the request as the queued reads allow.  libssh2 treats a short
	// read as "wait for the socket"; here nothing else re-triggers it until more TCP
	// data arrives, so bytes left in these buffers would sit unprocessed (the last
	// packets of a stream - data, EOF, close - would never be seen).
	size_t total = 0;
	while( total < length ) {
		struct data_buffer* buf;
		INDEX idx;
		LOGICAL found = FALSE;
		DATA_FORALL( cs->buffers, idx, struct data_buffer*, buf ) {
			if( buf->length ) {
				size_t avail = buf->length - buf->used;
				size_t filled = ( avail < ( length - total ) ) ? avail : ( length - total );
				MemCpy( (uint8_t*)buffer + total, buf->buffer + buf->used, filled );
				buf->used += filled;
				total += filled;
				if( buf->used == buf->length ) {
					buf->used = 0;
					buf->length = 0;
					moveBuffers( &cs->buffers, idx );
				}
				found = TRUE;
				break; // the list may have been reordered; rescan from the front
			}
		}
		if( !found ) break;
	}
	return total ? (ssize_t)total : -EAGAIN;
}

// bytes received from the socket that libssh2 hasn't taken yet
static size_t ssh_unconsumed( struct ssh_session* cs ) {
	size_t left = 0;
	INDEX idx;
	struct data_buffer* db;
	DATA_FORALL( cs->buffers, idx, struct data_buffer*, db )
		if( db->length ) left += db->length - db->used;
	return left;
}


static void pwChange( LIBSSH2_SESSION *session, char** newpw, int* newpw_len, void **abstract ) {
	struct ssh_session* cs = (struct ssh_session*)abstract[0];
	if( cs->pw_change )
		cs->pw_change( cs->psv, newpw, newpw_len );
	//RemoveClient( cs->pc );
}

//----------------------------- Output queue / flow control -----------------------

static void ssh_list_flush( struct ssh_channel* ch ) {
	// csOut held
	struct ssh_session* cs = ch->session;
	if( !ch->flush_listed ) {
		ch->flush_listed = 1;
		ch->flush_next = NULL;
		if( cs->flush_tail ) cs->flush_tail->flush_next = ch;
		else cs->flush_head = ch;
		cs->flush_tail = ch;
	}
	cs->out_seq = cs->out_seq + 1;
}

static void ssh_drop_output( struct ssh_channel* ch ) {
	// csOut held
	struct ssh_out_buffer* buf;
	while( ( buf = ch->out_head ) ) {
		ch->out_head = buf->next;
		ReleaseEx( buf DBG_SRC );
	}
	ch->out_tail = NULL;
	ch->out_pending = 0;
}

// csLock held.  Returns TRUE if the channel still has output it couldn't send.
static LOGICAL ssh_flush_channel( struct ssh_channel* ch ) {
	struct ssh_session* cs = ch->session;
	struct ssh_out_buffer* buf;
	LOGICAL drained = FALSE, send_eof = FALSE, do_close = FALSE;

	if( !ch->want_hold )
		libssh2_channel_window_hold( ch->channel, 0 ); // refund anything held back

	while( 1 ) {
		EnterCriticalSec( &cs->csOut );
		buf = ch->out_head;
		if( buf && ch->remote_closed ) {
			lprintf( "ssh channel closed by peer; dropping %zu queued bytes", ch->out_pending );
			ssh_drop_output( ch );
			buf = NULL;
		}
		if( !buf ) {
			// The queue is empty: decide drain/EOF/close in the same step, so output
			// another thread queues after this point can't be overtaken by them (the
			// peer discards data that arrives after EOF).
			drained = ch->out_blocked;
			ch->out_blocked = 0;
			send_eof = ch->eof_pending && !ch->eof_sent;
			if( send_eof ) ch->eof_sent = 1;
			do_close = ch->close_pending;
			LeaveCriticalSec( &cs->csOut );
			break;
		}
		LeaveCriticalSec( &cs->csOut );
		ssize_t rc = libssh2_channel_write_ex( ch->channel, buf->stream, (const char*)buf->data + buf->used, buf->length - buf->used );
		if( rc == LIBSSH2_ERROR_EAGAIN || rc == 0 ) {
			ch->out_blocked = 1;
			return TRUE; // peer's window is full; retried after the next packet arrives
		}
		EnterCriticalSec( &cs->csOut );
		if( rc < 0 ) {
			lprintf( "ssh channel write failed (%zd); dropping %zu queued bytes", rc, ch->out_pending );
			ssh_drop_output( ch );
		} else {
			buf->used += rc;
			ch->out_pending -= rc;
			if( buf->used == buf->length ) {
				ch->out_head = buf->next;
				if( !ch->out_head ) ch->out_tail = NULL;
				ReleaseEx( buf DBG_SRC );
			}
		}
		LeaveCriticalSec( &cs->csOut );
	}
	// report drain if a write left data waiting (the window was full, or another
	// thread had the session)
	if( drained && ch->drain && !do_close )
		ch->drain( ch->psv );
	if( send_eof )
		libssh2_channel_send_eof( ch->channel );
	if( do_close ) {
		libssh2_channel_abstract( ch->channel )[0] = NULL;
		libssh2_channel_close( ch->channel );
		ReleaseEx( ch DBG_SRC );
	}
	return FALSE;
}

static void _readCallback( struct ssh_session* cs, POINTER buffer, size_t length );

// csLock held.  Start requests deferred by ssh_api_queue, then write whatever queued
// output the peers' windows allow.
// csLock held.  Move requests deferred by ssh_api_queue into the state queue (behind
// what's already there), and start the next one if nothing is in progress.
static void ssh_start_deferred( struct ssh_session* cs ) {
	struct pending_state state;
	LOGICAL moved = FALSE;
	EnterCriticalSec( &cs->csOut );
	while( DequeData( &cs->pdqDeferred, &state ) ) {
		EnqueData( &cs->pdqStates, &state );
		moved = TRUE;
	}
	LeaveCriticalSec( &cs->csOut );
	if( moved && !cs->busy && cs->pending.state == SSH_STATE_RESET ) {
		// run the state machine to send the next request; reading is blocked, so
		// replies (and their callbacks) are left for the network thread.
		cs->block_recv++;
		_readCallback( cs, NULL, 0 );
		cs->block_recv--;
	}
}

static void ssh_flush_session( struct ssh_session* cs ) {
	struct ssh_channel* list;
	struct ssh_channel* ch;
	struct ssh_channel* blocked = NULL;
	struct ssh_channel* blocked_tail = NULL;
	ssh_start_deferred( cs );
	cs->busy++;
	cs->block_recv++;
	EnterCriticalSec( &cs->csOut );
	list = cs->flush_head;
	cs->flush_head = cs->flush_tail = NULL;
	for( ch = list; ch; ch = ch->flush_next ) ch->flush_listed = 0;
	LeaveCriticalSec( &cs->csOut );
	while( ( ch = list ) ) {
		list = ch->flush_next;
		if( ssh_flush_channel( ch ) ) {
			ch->flush_next = NULL;
			if( blocked_tail ) blocked_tail->flush_next = ch;
			else blocked = ch;
			blocked_tail = ch;
		}
	}
	// still waiting on a window adjust; keep them listed (without bumping out_seq)
	EnterCriticalSec( &cs->csOut );
	while( ( ch = blocked ) ) {
		blocked = ch->flush_next;
		if( !ch->flush_listed ) {
			ch->flush_listed = 1;
			ch->flush_next = NULL;
			if( cs->flush_tail ) cs->flush_tail->flush_next = ch;
			else cs->flush_head = ch;
			cs->flush_tail = ch;
		}
	}
	LeaveCriticalSec( &cs->csOut );
	cs->block_recv--;
	cs->busy--;
}

// Called (without csLock) after queueing work.  If the session is busy on another
// thread, that thread flushes before it lets go (it re-checks out_seq after leaving).
static void ssh_try_flush( struct ssh_session* cs ) {
	uint32_t seq;
	int32_t r;
	do {
		seq = cs->out_seq;
		while( ( r = EnterCriticalSecNoWait( &cs->csLock, NULL ) ) < 0 )
			Relinquish();
		if( !r ) return;
		if( cs->busy ) {
			// re-entered from a callback on the thread already inside the session;
			// it flushes on the way out.
			LeaveCriticalSec( &cs->csLock );
			return;
		}
		ssh_flush_session( cs );
		LeaveCriticalSec( &cs->csLock );
	} while( seq != cs->out_seq );
}

// leave csLock after flushing; picks up work queued by threads that found it locked
static void ssh_leave_session( struct ssh_session* cs ) {
	uint32_t seq = cs->out_seq;
	ssh_flush_session( cs );
	LeaveCriticalSec( &cs->csLock );
	if( seq != cs->out_seq )
		ssh_try_flush( cs );
}

// API calls (usually from the application's thread) must not wait for csLock: the
// network thread holding it may itself be blocked in a callback until the application
// thread returns.  ssh_api_enter takes the lock only if it's free (or already ours);
// otherwise the caller hands its request to ssh_api_queue, and the holder starts it.
// While an API call holds the lock, packets are not read: replies, and the callbacks
// they cause, stay on the network thread.
static int ssh_api_enter( struct ssh_session* cs ) {
	int32_t r;
	while( ( r = EnterCriticalSecNoWait( &cs->csLock, NULL ) ) < 0 )
		Relinquish();
	if( !r ) return 0;
	cs->block_recv++;
	// requests deferred earlier go first (channel opens complete in request order)
	ssh_start_deferred( cs );
	cs->busy++;
	return 1;
}

static void ssh_api_leave( struct ssh_session* cs ) {
	cs->block_recv--;
	cs->busy--;
	ssh_leave_session( cs );
}

static void ssh_api_queue( struct ssh_session* cs, struct pending_state* state, int locked ) {
	if( locked ) {
		// another request is in progress; this one runs after it
		EnqueData( &cs->pdqStates, state );
		ssh_api_leave( cs );
		return;
	}
	EnterCriticalSec( &cs->csOut );
	EnqueData( &cs->pdqDeferred, state );
	cs->out_seq = cs->out_seq + 1;
	LeaveCriticalSec( &cs->csOut );
	ssh_try_flush( cs );
}

static void _readCallback( struct ssh_session* cs, POINTER buffer, size_t length ) {
	{
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
		EnterCriticalSec( &cs->csLock );
		cs->busy++; // callbacks below may write; they queue, and we flush on the way out

		size_t pass_start;
		int idle_retries = 0;
		while( 1 ) {
			if( did_one && cs->pending.state != SSH_STATE_RESET ) break;
			did_one = 1;
			pass_start = ssh_unconsumed( cs );
			if( cs->pending.state == SSH_STATE_RESET )
				DequeData( &cs->pdqStates, &cs->pending );
			switch( cs->pending.state ) {
			default:
				lprintf( "Unhandled ssh_layer state:%d", cs->pending.state );
				break;
			/*
			case SSH_STATE_CONNECTING:
				break;
			case SSH_STATE_CONNECT:
				break;
			case SSH_STATE_FORWARD:
				break;
			*/
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
						if( cs->pc )
							RemoveClient( cs->pc );
						else if( cs->close ) cs->close( cs->psv );
						cs->busy--;
						LeaveCriticalSec( &cs->csLock );
						return;
					}
					else {
						CTEXTSTR fingerprint = libssh2_hostkey_hash( cs->session, LIBSSH2_HOSTKEY_HASH_SHA1 );
						if( cs->pending.state_data[0] )
							( (ssh_handshake_cb)cs->pending.state_data[0] )( cs->psv, (const uint8_t*)fingerprint );
						else
							cs->handshake_complete( cs->psv, (const uint8_t*)fingerprint );
					}
					cs->pending.state = SSH_STATE_RESET;
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
					} else if( cs->pending.state_data[0] ) {
						((ssh_auth_cb) cs->pending.state_data[0])( cs->psv, FALSE );
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
					if( cs->pending.state_data[0] ) {
						((ssh_auth_cb) cs->pending.state_data[0])( cs->psv, TRUE );
					}  else if( cs->auth_complete ) {
						cs->auth_complete( cs->psv, TRUE );
					}
				}
				cs->pending.state = SSH_STATE_RESET;
				break;
			case SSH_STATE_AUTH_PW:
				{
					CTEXTSTR user = (CTEXTSTR)cs->pending.state_data[0];
					CTEXTSTR pass = (CTEXTSTR)cs->pending.state_data[2];
					rc = libssh2_userauth_password_ex( cs->session, user, (unsigned int)cs->pending.state_data[1], pass, (unsigned int)cs->pending.state_data[3], pwChange );
					if( rc == LIBSSH2_ERROR_EAGAIN ) break;
					else if( rc ) {
						// error
						if( cs->pending.state_data[4] ) {
							( (ssh_auth_cb)cs->pending.state_data[4] )( cs->psv, FALSE );
						} else if( cs->auth_complete ) {
							cs->auth_complete( cs->psv, FALSE );
						}
						cs->pending.state = SSH_STATE_RESET;
						cs->busy--;
						ssh_leave_session( cs );
						return;
					} else {
						if( user ) ReleaseEx( (POINTER)user DBG_SRC );
						if( pass ) ReleaseEx( (POINTER)pass DBG_SRC );
						if( cs->pending.state_data[4] ) {
							( (ssh_auth_cb)cs->pending.state_data[4] )( cs->psv, TRUE );
						} else if( cs->auth_complete ) {
							cs->auth_complete( cs->psv, TRUE );
						}
						cs->pending.state = SSH_STATE_RESET;
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
							ssh_forward_listen_cb cb = (ssh_forward_listen_cb)cs->pending.state_data[3];
							cs->pending.state = SSH_STATE_RESET;
							if( cb )
								cb( session->psv, NULL, 0 );
							else if( session->forward_listen_cb )
								session->forward_listen_cb( session->psv, NULL, 0 );
						} else {
							lprintf( "no listener, no error?" );
						}
					} else {
						struct ssh_listener *ssh_listener = NewArray( struct ssh_listener, 1);
						ssh_listener->listener = listener;
						ssh_listener->session = session;
						ssh_forward_listen_cb cb = (ssh_forward_listen_cb)cs->pending.state_data[3];
						libssh2_listener_callback_set( listener, LIBSSH2_CALLBACK_LISTENER_ACCEPT, (libssh2_cb_generic*)listener_connect_relay );
						libssh2_listener_abstract( listener )[0] = (void*)ssh_listener;
						if( cb ) {
							ssh_listener->psv = cb( session->psv, ssh_listener, bound_port );
						}
						else if( session->forward_listen_cb )
							ssh_listener->psv = session->forward_listen_cb( session->psv, ssh_listener, bound_port );
						cs->pending.state = SSH_STATE_RESET;
					}
				}
				break;
			case SSH_STATE_FORWARD:
				{
					struct ssh_director* director = (struct ssh_director*)cs->pending.state_data[0];
					if( OpenForwardDirector( director ) )
						cs->pending.state = SSH_STATE_RESET;
					else
						break;
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
							// tell the requester it failed (NULL channel), and let the next state run.
							Deallocate( CTEXTSTR, type );
							Deallocate( CTEXTSTR, message );
							cs->pending.state = SSH_STATE_RESET;
							ssh_open_cb cb = (ssh_open_cb)cs->pending.state_data[6];
							if( cb )
								cb( cs->psv, NULL );
							else if( cs->channel_open )
								cs->channel_open( cs->psv, NULL );
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
							lprintf( "error setting env:%d %d %s", rc, err, msg );
						}
						cs->pending.state = SSH_STATE_RESET;
						Deallocate( CTEXTSTR, key );
						Deallocate( CTEXTSTR, value );
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
				rc = libssh2_session_read( cs->session );
				if( rc < 0 && rc != LIBSSH2_ERROR_EAGAIN ) {
					// the session is unusable (decrypt/MAC failure, disconnect...);
					// retrying would only spin on the same data.
					lprintf( "ssh session read failed (%d)", rc );
					rc = LIBSSH2_ERROR_EAGAIN;
				}
				break;
			}
			if( rc == LIBSSH2_ERROR_EAGAIN ) {
				// libssh2 may stop with received bytes still queued here (it parses the
				// next packet's header, then waits for the socket before reading the
				// rest).  Nothing else re-triggers it until more TCP data arrives, so
				// keep going while it makes progress (one retry without progress
				// covers that header case).
				size_t now = ssh_unconsumed( cs );
				if( !now ) break;
				if( now < pass_start ) idle_retries = 0;
				else if( idle_retries++ ) break;
				did_one = 0; // run the same pending state again
				continue;
			}
		}
		//case
		cs->busy--;
		// window adjusts that arrived above may let queued output go
		ssh_leave_session( cs );
	}
}


static void readCallback( PCLIENT pc, POINTER buffer, size_t length ) {
	struct ssh_session* cs = (struct ssh_session*)GetNetworkLong( pc, 0 );
	if( !buffer ) {
		cs->buffers = CreateDataList( sizeof( struct data_buffer ) );
	}
	else {
		_readCallback( cs, buffer, length );
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
	if( !pc ) {
		INDEX idx;
		struct ssh_session* connecting;
		EnterCriticalSec( &local_ssh_layer_data.csLock );

		LIST_FORALL( local_ssh_layer_data.connecting, idx, struct ssh_session*, connecting ) {
			if( connecting->pc == pc ) {
				SetLink( &local_ssh_layer_data.connecting, idx, NULL );
				session = connecting;
				break;
			}
		}
		LeaveCriticalSec( &local_ssh_layer_data.csLock );
	}
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
		uintptr_t cb = GetNetworkLong( session->pc, 1 );
		set_pending_state( session->pending, SSH_STATE_HANDSHAKE, { (uintptr_t)cb } );
		libssh2_session_handshake( session->session, 1/*sock*/ );
	}
}

static void closeCallback( PCLIENT pc ) {
	struct ssh_session* session = (struct ssh_session*)GetNetworkLong( pc, 0 );
	DeleteCriticalSec( &session->csLock );
	DeleteCriticalSec( &session->csOut );
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
	InitializeCriticalSection( &session->csLock );
	InitializeCriticalSection( &session->csOut );

	session->pdqStates = CreateDataQueue( sizeof( struct pending_state ) );
	session->psv = psv;
	session->buffers = CreateDataList( sizeof( struct data_buffer ) );

	session->session = libssh2_session_init_ex(alloc_callback, free_callback, realloc_callback, session);
	libssh2_session_banner_set( session->session, "SSH-2.0-sack_2.0+libssh2_1.9.0" );

	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_SEND, (libssh2_cb_generic*)SendCallback );
	libssh2_session_callback_set2( session->session, LIBSSH2_CALLBACK_RECV, (libssh2_cb_generic*)RecvCallback );
	libssh2_session_set_blocking( session->session, 0 );

	return session;
}

void sack_ssh_session_open( struct ssh_session* session, ssh_session_read_cb *read, ssh_session_write_cb write, ssh_session_close_cb close, ssh_handshake_cb cb ) {
	EnterCriticalSec( &session->csLock );
	if( session->pending.state != SSH_STATE_RESET ) {
		lprintf( "session is already in progress" );
	}
	session->pc = NULL;//OpenTCPClientExxx( host, port?port:22, readCallback, closeCallback, NULL, connectCallback, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	read[0] = (ssh_session_read_cb)_readCallback;
	session->write = write;
	session->close = close;
	session->pending.state = SSH_STATE_CONNECTING;

	LeaveCriticalSec( &session->csLock );
}

void sack_ssh_session_connect( struct ssh_session* session, CTEXTSTR host, int port, ssh_handshake_cb cb ) {
	EnterCriticalSec( &session->csLock );
	if( session->pending.state != SSH_STATE_RESET ) {
		lprintf( "session is already in progress" );
	}
	EnterCriticalSec( &local_ssh_layer_data.csLock );
	AddLink( &local_ssh_layer_data.connecting, session );
	LeaveCriticalSec( &local_ssh_layer_data.csLock );
	session->pc = OpenTCPClientExxx( host, port?port:22, readCallback, closeCallback, NULL, connectCallback, OPEN_TCP_FLAG_DELAY_CONNECT DBG_SRC );
	session->write = NULL;
	session->close = NULL;
	if( session->pc ) {
		SetNetworkLong( session->pc, 1, (uintptr_t)cb );
		// make sure to block other connections until handshake resolves...
		session->pending.state = SSH_STATE_CONNECTING;
		SetNetworkLong( session->pc, 0, (uintptr_t)session );
		NetworkConnectTCP( session->pc );
	}
	LeaveCriticalSec( &session->csLock );
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
	if( session->pc )
		RemoveClient( session->pc );
	else if( session->close ) session->close( session->psv );
	if( session->bincert )
		ReleaseCert( session->bincert );
	if( session->user ) ReleaseEx( (POINTER)session->user DBG_SRC );
	if( session->pass ) ReleaseEx( (POINTER)session->pass DBG_SRC );
	if( session->pdqStates ) DeleteDataQueue( &session->pdqStates );
	if( session->pdqDeferred ) DeleteDataQueue( &session->pdqDeferred );
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

ssh_open_cb sack_ssh_set_channel_open( struct ssh_session*session, ssh_open_cb channel_open ){
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

static LIBSSH2_LISTENER_CONNECT_FUNC( listener_connect_relay ) {
	struct ssh_listener* ssh_listener = (struct ssh_listener*)listener_abstract[0];
	struct ssh_channel* ssh_channel = NewArray( struct ssh_channel, 1 );
	MemSet( ssh_channel, 0, sizeof( *ssh_channel ) );
	libssh2_channel_abstract( channel )[0] = (void*)ssh_channel;

	ssh_channel->session = (struct ssh_session*)session_abstract[0];
	//channel->channel = libssh2_channel_accept_ex( listener->listener, (void*)channel );
	ssh_channel->channel = channel;
	ssh_channel->psv = ssh_listener->listen_connect_cb( ssh_listener->psv, ssh_listener, ssh_channel );
}

ssh_forward_listen_accept_cb sack_ssh_set_forward_listen_accept( struct ssh_listener* listener, ssh_forward_listen_accept_cb connect_cb ) {
	ssh_forward_listen_accept_cb cb = listener->listen_connect_cb;
	libssh2_listener_callback_set( listener->listener, LIBSSH2_CALLBACK_LISTENER_ACCEPT, (libssh2_cb_generic*)listener_connect_relay );
	listener->listen_connect_cb = connect_cb;
	return cb;
}

ssh_pty_cb sack_ssh_set_pty_open( struct ssh_channel*channel, ssh_pty_cb pty_open ){
	ssh_pty_cb cb = channel->pty_open;
	channel->pty_open = pty_open;
	return cb;
}

ssh_shell_cb sack_ssh_set_shell_open( struct ssh_channel*channel, ssh_shell_cb shell_open ){
	ssh_shell_cb cb = channel->shell_open;
	channel->shell_open = shell_open;
	return cb;
}

ssh_exec_cb sack_ssh_set_exec_done( struct ssh_channel* channel, void( *exec_done )( uintptr_t, LOGICAL ) ) {
	ssh_exec_cb cb = channel->exec_done;
	channel->exec_done = exec_done;
	return cb;
}

// channel_abstract[0] is cleared by sack_ssh_channel_close(), so late callbacks are dropped.
// A channel the user closed (close_pending) is only finishing its output; its user
// callbacks are not called any more.
static LIBSSH2_CHANNEL_EOF_FUNC( channel_eof_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	if( ssh_channel && !ssh_channel->close_pending && ssh_channel->channel_eof ) ssh_channel->channel_eof( ssh_channel->psv );
}

static LIBSSH2_CHANNEL_CLOSE_FUNC( channel_close_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	if( !ssh_channel ) return;
	// queued output can't go anywhere now; the next flush drops it (and finishes a pending close)
	EnterCriticalSec( &ssh_channel->session->csOut );
	ssh_channel->remote_closed = 1;
	ssh_list_flush( ssh_channel );
	LeaveCriticalSec( &ssh_channel->session->csOut );
	if( !ssh_channel->close_pending && ssh_channel->channel_close ) ssh_channel->channel_close( ssh_channel->psv );
}

static LIBSSH2_CHANNEL_DATA_FUNC( channel_data_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	if( ssh_channel && !ssh_channel->close_pending && ssh_channel->channel_data ) ssh_channel->channel_data( ssh_channel->psv, stream, buffer, length );
}

static LIBSSH2_CHANNEL_REQUEST_FUNC( channel_request_relay ) {
	struct ssh_channel* ssh_channel = (struct ssh_channel*)channel_abstract[0];
	if( ssh_channel && !ssh_channel->close_pending && ssh_channel->channel_request ) ssh_channel->channel_request( ssh_channel->psv, request, request_len );
}

ssh_channel_drain_cb sack_ssh_set_channel_drain( struct ssh_channel* channel, ssh_channel_drain_cb drain ) {
	ssh_channel_drain_cb cb = channel->drain;
	channel->drain = drain;
	return cb;
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

ssh_channel_request_cb sack_ssh_set_channel_request( struct ssh_channel*channel, ssh_channel_request_cb channel_request ){
	ssh_channel_request_cb cb = channel->channel_request;
	libssh2_channel_callback_set( channel->channel, LIBSSH2_CALLBACK_CHANNEL_REQUEST, (libssh2_cb_generic*)channel_request_relay );
	channel->channel_request = channel_request;
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

			//uint32_t rand1 = ( (uint32_t*)data.keys[p].privkey )[0];
			//uint32_t rand2 = ( (uint32_t*)data.keys[p].privkey )[1];
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

void sack_ssh_auth_user_password( struct ssh_session*session, CTEXTSTR user, CTEXTSTR pass, ssh_auth_cb cb ){
	int locked = ssh_api_enter( session );
	if( !locked || session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen(user), (uintptr_t)StrDup( pass ), StrLen( pass ), (uintptr_t)cb}};
		ssh_api_queue( session, &state, locked );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PW, {(uintptr_t)StrDup( user ), StrLen( user ), (uintptr_t)StrDup( pass ), StrLen(pass), (uintptr_t)cb });
	libssh2_userauth_password_ex( session->session, user, (unsigned int)session->pending.state_data[1], pass, (unsigned int)session->pending.state_data[3], pwChange );
	ssh_api_leave( session );
}

void sack_ssh_auth_user_cert( struct ssh_session*session, CTEXTSTR user
                        , uint8_t* pubkey, size_t pubkeylen
                        , uint8_t* privkey, size_t privkeylen
                        , CTEXTSTR pass, ssh_auth_cb cb ){
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
	int locked = ssh_api_enter( session );
	if( !locked || session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = { SSH_STATE_AUTH_PK, {(uintptr_t)cb} };
		ssh_api_queue( session, &state, locked );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_AUTH_PK, {(uintptr_t)cb});
	//libssh2_userauth_publickey_fromfile( session->session, user, NULL, "rsa", pass );

	libssh2_userauth_publickey_frommemory( session->session
	                                     , user, strlen( user )
	                                     , (const char*)session->pubKey, session->pubKey_len
	                                     , (const char*)session->privKey, session->privKey_len
	                                     , pass );
	ssh_api_leave( session );

}

// channel-open messages are binary (direct-tcpip has embedded NULs), so copy by length, not StrDup.
static char* ssh_message_dup( CTEXTSTR message, size_t message_len ) {
	if( !message ) return NULL;
	char* copy = NewArray( char, message_len + 1 );
	MemCpy( copy, message, message_len );
	copy[message_len] = 0;
	return copy;
}

void sack_ssh_channel_open_v2( struct ssh_session* session, CTEXTSTR type, size_t type_len, uint32_t window_size, uint32_t packet_size,
	CTEXTSTR message, size_t message_len, ssh_open_cb cb ) {
	int locked = ssh_api_enter( session );
	if( !locked || session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = { SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)ssh_message_dup( message, message_len ), message_len, (uintptr_t)cb }};
		ssh_api_queue( session, &state, locked );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_OPEN_CHANNEL, {(uintptr_t)StrDup( type ), type_len, window_size, packet_size, (uintptr_t)ssh_message_dup( message, message_len ), message_len, (uintptr_t)cb } );
	libssh2_channel_open_ex( session->session, type, (unsigned int)type_len, window_size, packet_size, message, (unsigned int)message_len );
	ssh_api_leave( session );
}

void sack_ssh_channel_free( struct ssh_channel* channel ){
	libssh2_channel_close( channel->channel );
}

void sack_ssh_channel_open( struct ssh_session *session ) {
	sack_ssh_channel_open_v2( session, "session", 7, 0x4000, 0x1000, NULL, 0, NULL );
}

// Close happens after queued output is sent (or dropped, if the peer closes first).
// From here on no callbacks reach the caller, and the channel must not be used again;
// the flush that sends the close releases it.
void sack_ssh_channel_close( struct ssh_channel* channel ) {
	struct ssh_session* cs = channel->session;
	EnterCriticalSec( &cs->csOut );
	channel->close_pending = 1;
	ssh_list_flush( channel );
	LeaveCriticalSec( &cs->csOut );
	ssh_try_flush( cs );
}

// EOF goes after queued output.
void sack_ssh_channel_eof( struct ssh_channel* channel ) {
	struct ssh_session* cs = channel->session;
	EnterCriticalSec( &cs->csOut );
	channel->eof_pending = 1;
	ssh_list_flush( channel );
	LeaveCriticalSec( &cs->csOut );
	ssh_try_flush( cs );
}

size_t sack_ssh_channel_pending( struct ssh_channel* channel ) {
	return channel->out_pending;
}

void sack_ssh_channel_pause( struct ssh_channel* channel, LOGICAL paused ) {
	struct ssh_session* cs = channel->session;
	channel->want_hold = paused ? 1 : 0;
	if( paused ) {
		// only sets a flag in libssh2 (no I/O); takes effect for the next packet
		libssh2_channel_window_hold( channel->channel, 1 );
		return;
	}
	// releasing sends a window adjust, which needs the session lock
	EnterCriticalSec( &cs->csOut );
	ssh_list_flush( channel );
	LeaveCriticalSec( &cs->csOut );
	ssh_try_flush( cs );
}

void sack_ssh_channel_request_pty( struct ssh_channel* channel, CTEXTSTR term, ssh_pty_cb cb ) {
	int locked = ssh_api_enter( channel->session );
	if( !locked || channel->session->pending.state != SSH_STATE_RESET ) {
		// error
		struct pending_state state = {SSH_STATE_REQUEST_PTY, {(uintptr_t)channel, (uintptr_t)StrDup(term), (uintptr_t)cb}};
		ssh_api_queue( channel->session, &state, locked );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_REQUEST_PTY, { (uintptr_t)channel, (uintptr_t)StrDup( term ), (uintptr_t)cb } );
	libssh2_channel_request_pty( channel->channel, term );
	ssh_api_leave( channel->session );
}

void sack_ssh_channel_setenv( struct ssh_channel* channel, CTEXTSTR key, CTEXTSTR value, ssh_setenv_cb cb ) {
	int locked = ssh_api_enter( channel->session );
	if( !locked || channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel, (uintptr_t)cb}};
		ssh_api_queue( channel->session, &state, locked );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SETENV, {(uintptr_t)StrDup( key),(uintptr_t)StrDup( value ), (uintptr_t)channel, (uintptr_t)cb });
	libssh2_channel_setenv( channel->channel, key, value );
	ssh_api_leave( channel->session );
}

void sack_ssh_channel_shell( struct ssh_channel* channel, ssh_shell_cb cb ) {
	int locked = ssh_api_enter( channel->session );
	if( !locked || channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SHELL, { (uintptr_t)channel, (uintptr_t)cb } };
		ssh_api_queue( channel->session, &state, locked );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_SHELL, { (uintptr_t)channel, (uintptr_t)cb } );
	int rc = libssh2_channel_shell( channel->channel );
	if( !rc ) {
		if( cb )
			cb( channel->psv, TRUE );
		else
			if( channel->shell_open )
				channel->shell_open( channel->psv, TRUE );
		channel->session->pending.state = SSH_STATE_RESET;
	} else if( rc != LIBSSH2_ERROR_EAGAIN ) {
		if( cb )
			cb( channel->psv, FALSE );
		else
			if( channel->shell_open )
				channel->shell_open( channel->psv, FALSE );
		channel->session->pending.state = SSH_STATE_RESET;
	}
	ssh_api_leave( channel->session );
}

void sack_ssh_channel_exec( struct ssh_channel* channel, CTEXTSTR shell, ssh_exec_cb cb ) {
	int locked = ssh_api_enter( channel->session );
	if( !locked || channel->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_EXEC, { (uintptr_t)channel, (uintptr_t)StrDup( shell ), (uintptr_t)cb}};
		ssh_api_queue( channel->session, &state, locked );
		return;
	}
	set_pending_state( channel->session->pending, SSH_STATE_EXEC, { (uintptr_t)channel, (uintptr_t)StrDup( shell ), (uintptr_t)cb } );
	libssh2_channel_exec( channel->channel, (CTEXTSTR)channel->session->pending.state_data[1] );
	ssh_api_leave( channel->session );
}

// Queue the data and write as much as the peer's window allows now; the rest goes
// as window adjusts arrive.  sack_ssh_channel_pending() tells how much is waiting.
void sack_ssh_channel_write( struct ssh_channel* channel, int stream, const uint8_t* buffer, size_t length ) {
	struct ssh_session* cs = channel->session;
	struct ssh_out_buffer* buf;
	if( !length ) return;
	buf = (struct ssh_out_buffer*)AllocateEx( sizeof( struct ssh_out_buffer ) + length DBG_SRC );
	buf->next = NULL;
	buf->stream = stream;
	buf->length = length;
	buf->used = 0;
	MemCpy( buf->data, buffer, length );
	EnterCriticalSec( &cs->csOut );
	if( channel->close_pending || channel->eof_pending ) {
		LeaveCriticalSec( &cs->csOut );
		ReleaseEx( buf DBG_SRC );
		lprintf( "ssh channel write after eof/close; %zu bytes dropped", length );
		return;
	}
	if( channel->out_tail ) channel->out_tail->next = buf;
	else channel->out_head = buf;
	channel->out_tail = buf;
	channel->out_pending += length;
	ssh_list_flush( channel );
	LeaveCriticalSec( &cs->csOut );
	ssh_try_flush( cs );
	// anything still queued is the caller's bufferedAmount; make sure whoever sends
	// it reports drain (checked with the queue under csOut, so it can't be missed).
	EnterCriticalSec( &cs->csOut );
	if( channel->out_head ) channel->out_blocked = 1;
	LeaveCriticalSec( &cs->csOut );
}

//------------------------- SOCKET FORWARDING ----------------------------

void sack_ssh_forward_listen( struct ssh_session* session, CTEXTSTR remotehost, uint16_t remoteport, ssh_forward_listen_cb cb ) {
	int locked = ssh_api_enter( session );
	if( !locked || session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_LISTEN, { (uintptr_t)session, (uintptr_t)StrDup( remotehost ), remoteport, (uintptr_t)cb}};
		ssh_api_queue( session, &state, locked );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_LISTEN, { (uintptr_t)session, (uintptr_t)StrDup( remotehost ), remoteport, (uintptr_t)cb } );
	// bound_port can't be used until much later... this will be a wait for completion.
	libssh2_channel_forward_listen_ex( session->session, remotehost, remoteport, NULL, 4096 );
	ssh_api_leave( session );
}

static void AcceptedClient( PCLIENT pc, PCLIENT pcNew ) {
	struct ssh_director_listener* listen = (struct ssh_director_listener*)GetNetworkLong( pc, 0 );

	struct ssh_director* director = NewArray( struct ssh_director, 1 );
	MemSet( director, 0, sizeof( struct ssh_director ) );
	director->pc = pcNew;
	director->director = listen;

	int locked = ssh_api_enter( listen->session );
	if( !locked || listen->session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_FORWARD, { (uintptr_t)director } };
		ssh_api_queue( listen->session, &state, locked );
		return;
	}	

	set_pending_state( listen->session->pending, SSH_STATE_FORWARD, { (uintptr_t)director } );
	if( OpenForwardDirector( director ) )
		listen->session->pending.state = SSH_STATE_RESET;
	ssh_api_leave( listen->session );

}

static void CPROC ForwardSocketRead( PCLIENT pc, POINTER buffer, size_t length ) {
	struct ssh_director* director = (struct ssh_director*)GetNetworkLong( pc, 0 );
	if( director && director->channel )
		sack_ssh_channel_write( director->channel, 0, (const uint8_t*)buffer, length );
}

static void CPROC ForwardSocketClose( PCLIENT pc ) {
	struct ssh_director* director = (struct ssh_director*)GetNetworkLong( pc, 0 );
	if( director ) {
		director->pc = NULL;
		SetNetworkLong( pc, 0, 0 );
	}
	if( director && director->channel )
		sack_ssh_channel_eof( director->channel );
}

static void ForwardChannelData( uintptr_t psv, int stream, const uint8_t* data, size_t length ) {
	struct ssh_director* director = (struct ssh_director*)psv;
	if( director && director->pc )
		SendTCP( director->pc, data, length );
}

static void ForwardChannelEOF( uintptr_t psv ) {
	struct ssh_director* director = (struct ssh_director*)psv;
	if( director && director->pc ) {
		PCLIENT pc = director->pc;
		director->pc = NULL;
		SetNetworkLong( pc, 0, 0 );
		RemoveClient( pc );
	}
}

static void ForwardChannelClose( uintptr_t psv ) {
	ForwardChannelEOF( psv );
}

static int OpenForwardDirector( struct ssh_director* director ) {
	int rc;
	struct ssh_director_listener* listen = director->director;
	LIBSSH2_CHANNEL *channel = libssh2_channel_direct_tcpip_ex( listen->session->session
		, listen->remoteAddr, listen->remotePort
		, listen->localAddr, listen->localPort );
	if( !channel ) {
		rc = libssh2_session_last_error( listen->session->session, NULL, NULL, 0 );
		if( rc == LIBSSH2_ERROR_EAGAIN )
			return 0;
		if( listen->connected )
			listen->connected( listen->session->psv, FALSE );
		else if( listen->session->forward_connect_cb )
			listen->session->forward_connect_cb( listen->session->psv, FALSE );
		SetNetworkLong( director->pc, 0, 0 );
		RemoveClient( director->pc );
		ReleaseEx( director DBG_SRC );
		return 1;
	} else {
		struct ssh_channel* ssh_channel = NewArray( struct ssh_channel, 1);
		MemSet( ssh_channel, 0, sizeof( struct ssh_channel ) );
		libssh2_channel_abstract( channel )[0] = (void*)ssh_channel;
		ssh_channel->session = listen->session;
		ssh_channel->channel = channel;
		ssh_channel->psv = (uintptr_t)director;
		ssh_channel->channel_data = ForwardChannelData;
		ssh_channel->channel_eof = ForwardChannelEOF;
		ssh_channel->channel_close = ForwardChannelClose;
		director->session = listen->session;
		director->channel = ssh_channel;
		SetNetworkLong( director->pc, 0, (uintptr_t)director );
		SetNetworkReadComplete( director->pc, ForwardSocketRead );
		SetNetworkCloseCallback( director->pc, ForwardSocketClose );
		sack_ssh_set_channel_data( ssh_channel, ForwardChannelData );
		sack_ssh_set_channel_eof( ssh_channel, ForwardChannelEOF );
		sack_ssh_set_channel_close( ssh_channel, ForwardChannelClose );
		if( listen->connected )
			listen->connected( listen->session->psv, TRUE );
		else if( listen->session->forward_connect_cb )
			listen->session->forward_connect_cb( listen->session->psv, TRUE );
		return 1;
	}
}


PCLIENT sack_ssh_forward_connect( struct ssh_session* session
		, CTEXTSTR localAddress, int localPort
		, CTEXTSTR remoteAddress, int remotePort
		, ssh_forward_connect_cb cb ) {
	SOCKADDR* addr = CreateRemote( localAddress, localPort );
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
	MemSet( director, 0, sizeof( struct ssh_director_listener ) );
	SetNetworkLong( pc, 0, (uintptr_t)director );
	director->pc = pc;
	director->session = session;
	director->connected = cb;
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
	int locked = ssh_api_enter( session );
	if( !locked || session->pending.state != SSH_STATE_RESET ) {
		struct pending_state state = { SSH_STATE_SFTP, { (uintptr_t)session }};
		ssh_api_queue( session, &state, locked );
		return;
	}
	set_pending_state( session->pending, SSH_STATE_SFTP, { (uintptr_t)session } );
	// can never complete immediately...
	libssh2_sftp_init( session->session );
	ssh_api_leave( session );
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

static void websocketCloseSSH( uintptr_t psv ) {
	struct ssh_websocket* ws = (struct ssh_websocket*)psv;
	sack_ssh_channel_close( ws->channel );
}

/*
static void ssh_ws_open( uintptr_t psv ) {
	//struct ssh_websocket* ws = (struct ssh_websocket*)psv;
	
}
*/

static void ssh_ws_eof( uintptr_t psv ) {
	struct ssh_websocket* ws = (struct ssh_websocket*)psv;
	// this side was given an EOF
	WebSocketPipeClose( ws->ws, 1006, "Channel at EOF" );
	sack_ssh_channel_close( ws->channel );
}

static void ssh_ws_close( uintptr_t psv ) {
	// close has already happened, turns out to be a noop?
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
	web_socket_accept ws_accept,  // server socket event
	web_socket_http_request ws_http,
	web_socket_http_close ws_http_close,
	web_socket_completion ws_completion,
	uintptr_t psv
	) {
	struct ssh_websocket* ws = NewArray( struct ssh_websocket, 1 );
	ws->channel = channel;
	sack_ssh_set_channel_eof( channel, ssh_ws_eof );
	sack_ssh_set_channel_close( channel, ssh_ws_close );
	channel->psv = (uintptr_t)ws;
	channel->channel_data = WebSocketData;
	ws->ws = WebSocketCreateServerPipe( ws_open, ws_event, ws_close, ws_error
		, ws_accept
		, ws_http, ws_http_close, ws_completion
		, psv );
	WebSocketPipeSetSend( ws->ws, websocketSendSSH, (uintptr_t)ws );
	WebSocketPipeSetClose( ws->ws, websocketCloseSSH, (uintptr_t)ws );
	return ws;
}

#ifdef __cplusplus
}
#endif
SACK_NETWORK_NAMESPACE_END
