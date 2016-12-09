#include <stdhdrs.h>
#include <network.h>
#include <http.h>

int secure = FALSE;

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
		if( secure ) {
#define REQ "GET / HTTP/1.1\r\n"   \
				"\r\n"


			ssl_Send( pc, REQ, sizeof(REQ)-1 );
		}
		//SendTCP( pc, "Yes, I've connected", 12 );
	}
	ReadTCP( pc, buf, 4096 );
}

PCLIENT pc_user;

void CPROC Closed( PCLIENT pc )
{
	pc_user = NULL;
}

static void CPROC Connected( PCLIENT pc, int err ) {
	if( secure )
		if( !ssl_BeginClientSession( pc, NULL, 0 ) ) {
			SystemLog( WIDE( "Failed to create client ssl session" ) );
			RemoveClient( pc );
			return FALSE;
		}
	{
		POINTER key;
		size_t keylen;
		ssl_GetPrivateKey( pc, &key, &keylen );
	}
}

SaneWinMain( argc, argv )
{
	SOCKADDR *sa;
	int arg;
	SetSystemLog( SYSLOG_FILE, stderr );
	if( argc < 2 )
	{
		printf( "usage: %s [-s] <Telnet IP[:port]>\n", argv[0] );
		printf( "	-s : enable sssl\n" );
		return 0;
	}
	NetworkStart();
	for( arg = 1; arg < argc; arg++ ) {
		if( argv[arg][0] == '-' ) {
			switch( argv[arg][1] ) {
				
			case 'r':
				{
					PTEXT del1, del2;

					PTEXT result = GetHttp( del1 = SegCreateFromText( argv[arg]+2 )						, del2 = SegCreateFromText( argv[arg+1] )						, TRUE					);
					lprintf( "result is [%s]", GetText( result ) );
					//, del3 = SegCreateFromText( "w=get_version" )					arg++;
				}
				break;
				
			case 's':
				if( pc_user ) {
					// this should also sort of work...
					// there's a buffer leak of sorts, that we'll get a NULL
					// read callback twice this way...
					if( !ssl_BeginClientSession( pc_user, NULL, 0 ) ) {
						SystemLog( WIDE( "Failed to create client ssl session" ) );
						return FALSE;
					}
					{
						POINTER key;
						size_t keylen;
						ssl_GetPrivateKey( pc_user, &key, &keylen );
					}
				}
				secure = TRUE;
				break;
			}
		} else {
			sa = CreateSockAddress( argv[arg], 23 );
			pc_user = OpenTCPClientAddrExx( sa, ReadComplete, Closed, NULL, Connected );
		}
	}

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
EndSaneWinMain()

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
