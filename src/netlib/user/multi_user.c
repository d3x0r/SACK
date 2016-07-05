/*
 This is a test program that opens 1000 connections to a server.
 no data is transferred to server... just whatever the server sends is dumped.
 */

#include <stdhdrs.h>
#include <stdio.h>
#include <logging.h>
#include <network.h>
#include <sharemem.h>

static SOCKADDR *sa;
PCLIENT conns[1000];
PTHREAD main_thread;
int awake;
int closed_pending;

static void CPROC ReadComplete( PCLIENT pc, void *bufptr, size_t sz )
{
	char *buf = (char*)bufptr;
	if( buf )
	{
		lprintf( "Socket %p recieved....", pc );
		LogBinary( bufptr, sz );
	}
	else
	{
		buf = (char*)Allocate( 4097 );
		//SendTCP( pc, "Yes, I've connected", 12 );
	}
	ReadTCP( pc, buf, 4096 );
}

PCLIENT pc_user;

void CPROC Closed( PCLIENT pc )
{
	int sock = GetNetworkLong( pc, 0 );
	lprintf( "connection %p %d closed... reopen", pc, sock );
	if( sock )
	{
		conns[sock-1] = NULL;
		closed_pending = 1;
		if( !awake )
		{
			awake = 1;
			WakeThread( main_thread );
		}
	}
}



int main( int argc, char** argv )
{
	int n;
	if( argc < 2 )
	{
		printf( "usage: %s <Telnet IP[:port]>\n", argv[0] );
		return 0;
	}
	SystemLog( WIDE("Starting the network") );
	NetworkWait( NULL, 3000, 5 );
	SystemLog( WIDE("Started the network") );
	sa = CreateSockAddress( DupCharToText( argv[1] ), 23 );
	main_thread = MakeThread();
	//if( argc >= 3 ) port = atoi( argv[2] ); else port = 23;
	awake = 1;
	while( 1 )
	{
		closed_pending = 0; // will handle one or more closed sockets.
		for( n = 0; n < 1000; n++ )
		{
			if( conns[n] == NULL )
			{
				conns[n] = OpenTCPClientAddrEx( sa, ReadComplete, Closed, NULL );
				if( !conns[n] )
					lprintf( "Connection %d failed in open", n );
				else
				{
					SetNetworkLong( conns[n], 0, n + 1 );
					lprintf( "connection %d success", n );
				}
			}
		}
		if( !closed_pending )
		{
			awake = 0;
			lprintf( "waiting for closed socks." );
			WakeableSleep( 10000 );
		}
	}
	return -1;
}


