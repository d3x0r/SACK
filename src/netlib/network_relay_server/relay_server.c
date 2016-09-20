#include <stdhdrs.h>
#include <network.h>
#include <deadstart.h>
#include <pssql.h>

struct pending_buf {
	POINTER data;
	size_t datalen;
};

typedef struct RelayConnection {
	int readstate;
	POINTER buffer;
	uint8_t* use_key;
	size_t keylen;
	size_t keyidx;
	size_t read_keyidx;
	PCLIENT pc;
	struct RelayConnection *member_of;  // group this connection is a member of (so we don't have to find it)
	PLIST members;  // list of PCLIENT connected to this (group/contact)
	CTEXTSTR conn_key;
	TEXTCHAR sendbuf[256];
	PLINKQUEUE pending;
   PTHREAD delay_send;
} RELAY_CONNECTION, *PRELAY_CONNECTION;

static struct {
	PCLIENT server;
	PLIST connections;	 // list of PRELAY_CONNECTION
} l;

static void RelayServerClose( PCLIENT pc )
{
	PRELAY_CONNECTION conn = ( PRELAY_CONNECTION )GetNetworkLong( pc, 0 );
	if( conn )
	{
		INDEX idx;
		PRELAY_CONNECTION peer;
		SetNetworkLong( pc, 0, 0 );
		LIST_FORALL( conn->members, idx, PRELAY_CONNECTION, peer )
		{
			peer->member_of = NULL;
			RemoveClient( peer->pc );
		}
		if( conn->member_of )
		{
			DeleteLink( &conn->member_of->members, conn );
			LIST_FORALL( conn->member_of->members, idx, PRELAY_CONNECTION, peer )
			{
				break;
			}
			if( !peer )
				RemoveClient( conn->member_of->pc );
		}
		DeleteLink( &l.connections, conn );
		Release( conn->buffer );
		Release( conn->use_key );
		Release( conn );
	}
}

static void DecodeBuffer( PRELAY_CONNECTION pc, TEXTSTR message, size_t message_length )
{
	int n;
	/* encrypt message here */
	for( n = 0; n < message_length; n++ )
		message[n] ^= pc->use_key[ ( pc->read_keyidx + n ) % pc->keylen];
	pc->read_keyidx += n;
	pc->read_keyidx %= pc->keylen;
}

static void SendRelayTCP( PRELAY_CONNECTION pc, TEXTSTR message, size_t message_length )
{
	int n;
	/* encrypt message here */
	SendTCP( pc->pc, &message_length, 4 );
	for( n = 0; n < message_length; n++ )
		message[n] ^= pc->use_key[ ( pc->keyidx + n ) % pc->keylen];
	pc->keyidx += n;
	pc->keyidx %= pc->keylen;
	SendTCP( pc->pc, message, message_length );
}

static uintptr_t CPROC DelaySend( PTHREAD thread )
{
	struct RelayConnection *conn = (struct RelayConnection *)GetThreadParam( thread );
	struct pending_buf *pending;
	Relinquish(); // this thread will never be able to run immediately.
	while( !NetworkLock( conn->pc ) )
	{
		lprintf( "Waiting to lock socket..." );
		Relinquish();
	}
	while( pending = DequeLink( &conn->pending ) )
	{
		lprintf( "sending pended socket..." );
		SendRelayTCP( conn, pending->data, pending->datalen );
		Release( pending->data );
		Release( pending );
	}
	conn->delay_send = NULL;
	NetworkUnlock( conn->pc );
	lprintf( "Done with delayed sending..." );
	return 0;
}

static void RelayServerRead( PCLIENT pc, POINTER buffer, size_t buflen )
{
	size_t toread;
	PRELAY_CONNECTION conn;
	if( !buffer )
	{
		PRELAY_CONNECTION conn = New( RELAY_CONNECTION );
		memset( conn, 0, sizeof( RELAY_CONNECTION ) );
		conn->pending = CreateLinkQueue();
		buffer = conn->buffer = Allocate( 4096 );
		conn->pc = pc;
		SetNetworkLong( pc, 0, (uintptr_t)conn );
		toread = 4;
		AddLink( &l.connections, conn );
	}
	else
	{
		conn = (PRELAY_CONNECTION)GetNetworkLong( pc, 0 );
		switch( conn->readstate )
		{
		default:
			DebugBreak();
			RemoveClient( pc );
			return;
		case 0:
			toread = ((uint32_t*)buffer)[0];
			conn->readstate = 1;
			break;
		case 1:
			conn->keylen = buflen;
			conn->use_key = NewArray( uint8_t, buflen );
			MemCpy( conn->use_key, buffer, buflen );
			conn->readstate = 2;
			toread = 4;
			break;
		case 2:
			toread = ((uint32_t*)buffer)[0];
			conn->readstate = 3;
			break;
		case 3:
			DecodeBuffer( conn, buffer, buflen );
			if( StrCmpEx( (CTEXTSTR)buffer, "Conference:", 11 ) == 0 )
			{
				if( StrCmp( (CTEXTSTR)buffer + 11, "New" ) == 0 )
				{
					size_t len;
					conn->conn_key = GetGUID();
					len = snprintf( conn->sendbuf, 256, "Key:%s", conn->conn_key );
					SendRelayTCP( conn, conn->sendbuf, len + 1 );
				}
				else
				{
					INDEX idx;
					PRELAY_CONNECTION find_conn;
					LIST_FORALL( l.connections, idx, PRELAY_CONNECTION, find_conn )
					{
						if( find_conn->conn_key )
							if( StrCmp( find_conn->conn_key, (TEXTSTR)buffer+11 ) == 0 )
							{
								conn->member_of = find_conn;
								AddLink( &find_conn->members, conn );
							}
					}
				}
			}
			else
			{
				TEXTSTR sendbuf = NewArray( TEXTCHAR, buflen );
				PRELAY_CONNECTION sendto;
				if( conn->member_of )
				{
			
					PRELAY_CONNECTION root = conn->member_of;
					INDEX idx;
					MemCpy( sendbuf, buffer, buflen );
					if( root->delay_send || !NetworkLock( root->pc ) )
					{
						struct pending_buf *pend = New( struct pending_buf );
						pend->data = sendbuf;
						pend->datalen = buflen;
						sendbuf = NewArray( uint8_t, buflen );
						if( !root->delay_send )
							root->delay_send = ThreadTo( DelaySend, (uintptr_t)root );
					}
					else
					{
						SendRelayTCP( root, sendbuf, buflen );
						NetworkUnlock( root->pc );
					}
					LIST_FORALL( root->members, idx, PRELAY_CONNECTION, sendto )
					{
						if( sendto != conn )
						{
							MemCpy( sendbuf, buffer, buflen );
							if( sendto->delay_send || !NetworkLock( sendto->pc ) )
							{
								struct pending_buf *pend = New( struct pending_buf );
								pend->data = sendbuf;
								pend->datalen = buflen;
								sendbuf = NewArray( uint8_t, buflen );
								if( !sendto->delay_send )
									sendto->delay_send = ThreadTo( DelaySend, (uintptr_t)sendto );
							}
							else
							{
								SendRelayTCP( sendto, sendbuf, buflen );
								NetworkUnlock( sendto->pc );
							}
						}
					}
					Release( sendbuf );
				}
				else
				{
					INDEX idx;
					LIST_FORALL( conn->members, idx, PRELAY_CONNECTION, sendto )
					{
						MemCpy( sendbuf, buffer, buflen );
						if( sendto->delay_send || !NetworkLock( sendto->pc ) )
						{
							struct pending_buf *pend = New( struct pending_buf );
							pend->data = sendbuf;
							pend->datalen = buflen;
							sendbuf = NewArray( uint8_t, buflen );
							if( !sendto->delay_send )
								sendto->delay_send = ThreadTo( DelaySend, (uintptr_t)sendto );
						}
						else
						{
							SendRelayTCP( sendto, sendbuf, buflen );
							NetworkUnlock( sendto->pc );
						}
					}
				}
			}
			toread = 4;
			conn->readstate = 2;
			break;
		}
	}
	ReadTCPMsg( pc, buffer, toread );
}

static void RelayServerConnect( PCLIENT pc_server, PCLIENT pc_new )
{
	SetTCPNoDelay( pc_new, TRUE );
	SetNetworkReadComplete( pc_new, RelayServerRead );
	SetNetworkCloseCallback( pc_new, RelayServerClose );
}

PRELOAD( InitServer )
{
   // expect a lot of sockets.
   NetworkWait( 0, 65000, 1 );
   l.server = OpenTCPListenerEx( 5732, RelayServerConnect );
}


SaneWinMain( argc, argv )
{
	while( 1 )
		WakeableSleep( 1000000 );
}
EndSaneWinMain();
