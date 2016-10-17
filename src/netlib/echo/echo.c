#include <stdhdrs.h>
#include <sharemem.h>
#include <timers.h>
#include <network.h>
#include <http.h>

void CPROC ServerRecieve( PCLIENT pc, POINTER buf, size_t size )
{
	//int bytes;
	if( !buf )
	{
		buf = Allocate( 4096 );
		// SendTCP( pc, (void*)"Hi, welccome to...", 15 );
	}
	else
		SendTCP( pc, buf, size );

	// test for waitread support...
	// read will not result until the data is read.
	//bytes = WaitReadTCP( pc, buf, 4096 );
	//if( bytes > 0 )
	//   SendTCP( pc, buf, bytes );

	ReadTCP( pc, buf, 4095 );
	// buffer does not have anything in it....
}

void CPROC ClientConnected( PCLIENT pListen, PCLIENT pNew )
{
	SetNetworkReadComplete( pNew, ServerRecieve );
}

static LOGICAL OnHttpGet( "AppClass", "test" )(uintptr_t psv, PCLIENT pc, struct HttpState *httpState, PTEXT httpBody ) {

	return FALSE;
}

LOGICAL onRequestCallback( uintptr_t psv
	, HTTPState pHttpState ) {
	// default request handling failing finding a OnHttpGet()

	return FALSE;
}

int main( int argc, char **argv )
{
	PCLIENT pcListen;
	SOCKADDR *port;
	NetworkWait( 0, 6000, 16 );
	if( argc < 2 )
	{
		printf( WIDE("usage: %s <listen port> (defaulting to telnet)\n"), DupCharToText( argv[0] ) );
		port = CreateSockAddress( WIDE("localhost:23"), 23 );
	}
	else {
		if( argv[1][0] == '-' ) {
			switch( argv[1][1] ) {
			case 's': {
				struct HttpServer *server = CreateHttpsServerEx( argv[1] + 2
					, "AppClass", argv[3], onRequestCallback, 0 );
				pcListen = (PCLIENT)-1;
				break;
			}
			}
		}
		else {
			port = CreateSockAddress( DupCharToText( argv[1] ), 23 );
			pcListen = OpenTCPListenerAddrEx( port, ClientConnected );
		}
	}
	if(pcListen)
	{
		while(1)
		{
			WakeableSleep(500);
		}
	}
	else
		printf( WIDE("Failed to listen on port %s\n"), DupCharToText( argv[1] ) );
	return 0;
}
// $Log: echo.c,v $
// Revision 1.9  2005/05/26 23:18:47  jim
// Use fancy CreateSockAddress so that the command line param may specify a unix domain socket as well as a TCP address...
//
// Revision 1.8  2005/05/26 23:17:09  jim
// - Modified Log -
//  Updated to use CreateSockAddress to build the address... allowing us to
// open UNIX sockets as well as TCP sockets.
//
// Revision 1.7  2005/05/23 17:26:00  jim
// Update echo server test to test reading TCP sockets with a wait mode read...
//
// Revision 1.6  2005/01/27 07:37:11  panther
// Linux cleaned.
//
// Revision 1.5  2003/11/09 03:32:15  panther
// Added some address functions to set port and override default port
//
// Revision 1.4  2003/07/24 22:50:10  panther
// Updates to make watcom happier
//
// Revision 1.3  2003/04/08 08:42:31  panther
// Fix inclusion of stdhdrs
//
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
