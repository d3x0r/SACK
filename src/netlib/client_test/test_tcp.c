#include <stdhdrs.h>
#include <stdio.h>
#include <idle.h>
#include <logging.h>
#include <network.h>
#include <sharemem.h>
#include <timers.h>

void CPROC ReadComplete( PCLIENT pc, void *bufptr, int sz )
{
   char *buf = (char*)bufptr;
	if( buf )
	{
		IdleFor( 500 );
		buf[sz] = 0;
		printf( "%s", buf );
		fflush( stdout );
	}
	else
	{
		buf = (char*)Allocate( 4097 );
      //SendTCP( pc, "Yes, I've connected", 12 );
	}
	ReadTCP( pc, buf, 4096 );
}

PCLIENT pc_user;
int connected = 0;

void CPROC Closed( PCLIENT pc )
{
   connected--;
   pc_user = NULL;
   lprintf( "disconnect." );
   printf( "-" );
}

void CPROC Connected( PCLIENT pc, int error )
{
	lprintf( "connect." );
   printf( "+" );
   connected++;
}

int main( int argc, char** argv )
{
	SOCKADDR *sa;
   int n;
	if( argc < 2 )
	{
		printf( "usage: %s <Telnet IP[:port]>\n", argv[0] );
		return 0;
	}
	SystemLog( "Starting the network" );
	NetworkStart();
	SystemLog( "Started the network" );
   sa = CreateSockAddress( argv[1], 23 );
	//if( argc >= 3 ) port = atoi( argv[2] ); else port = 23;
	pc_user = OpenTCPClientAddrExx( sa, ReadComplete, Closed, NULL, Connected );

	if( !pc_user )
	{
		SystemLog( "Failed to open some port as telnet" );
		printf( "failed to open %s%s\n", argv[1], strchr(argv[1],':')?"":":telnet[23]" );
		return 0;
	}
	for( n = 0; n < 1000; n++ )
	{
		if( (rand() & 3) != 0 )
		{
         WakeableSleep( rand() %1000 );
		}
      lprintf( "..." );
		RemoveClient( pc_user );
		pc_user = OpenTCPClientAddrExx( sa, ReadComplete, Closed, NULL, Connected );
		lprintf( "..." );
		if( (rand() &3) < 3 )
		{
         fflush( stdout );
			while( !connected )
				Relinquish();
		}
		if( connected )
		{
         lprintf( "send." );
			SendTCP( pc_user, "test", 4 );
		}
	}
	return -1;
}


