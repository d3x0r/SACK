#include <stdio.h>
#include <logging.h>
#include <network.h>
#include <sharemem.h>

void CPROC ReadComplete( PCLIENT pc, void *bufptr, size_t sz )
{
   char *buf = (char*)bufptr;
	if( buf )
	{
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

void CPROC Closed( PCLIENT pc )
{
   pc_user = NULL;
}

int main( int argc, char** argv )
{
   SOCKADDR *sa;
	if( argc < 2 )
	{
		printf( "usage: %s <Telnet IP[:port]>\n", argv[0] );
		return 0;
	}
	SystemLog( WIDE("Starting the network") );
	NetworkStart();
	SystemLog( WIDE("Started the network") );
   sa = CreateSockAddress( DupCharToText( argv[1] ), 23 );
	//if( argc >= 3 ) port = atoi( argv[2] ); else port = 23;
	pc_user = OpenTCPClientAddrEx( sa, ReadComplete, Closed, NULL );
	if( !pc_user )
	{
		SystemLog( WIDE("Failed to open some port as telnet") );
		printf( "failed to open %s%s\n", argv[1], strchr(argv[1],':')?"":":telnet[23]" );
		return 0;
	}
   //SendTCP( pc_user, "Some data here...", 12 );
	while( pc_user )
	{
		char buf[256];
		if( !fgets( buf, 256, stdin ) )
		{
			RemoveClient( pc_user );
			return 0;
		}
		SendTCP( pc_user, buf, strlen( buf ) );
	}
	return -1;
}


//-----------------------------------------------
// $Log: user.c,v $
// Revision 1.7  2005/05/26 23:18:39  jim
// Use fancy CreateSockAddress so that the command line param may specify a unix domain socket as well as a TCP address...
//
// Revision 1.6  2005/05/26 23:17:09  jim
// - Modified Log -
//  Updated to use CreateSockAddress to build the address... allowing us to
// open UNIX sockets as well as TCP sockets.
//
// Revision 1.5  2005/01/27 07:37:11  panther
// Linux cleaned.
//
// Revision 1.4  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
