#include <stdhdrs.h>
#include <timers.h>
#include <network.h>

PCLIENT gpc[65536];

int nOpen;
TEXTCHAR *pAddress;

void ConnectProc( PCLIENT pc, int n )
{
	uintptr_t i = GetNetworkLong( pc, 0 );

	if( n != 10049 )
	{
		if( n != 10061 ) // refused...
		{
			printf( WIDE("\rConnect on %d = %d\n"), i, n );
 		}
	}
	else
	{
		printf( WIDE("Bad Address: %s\n"), pAddress );
		exit(0);
	}
	if( !n )
   {
      fprintf( stderr, WIDE("\rSuccessful on %d\n"), i );
   }
   //else
      // should be noted that an error will result in the socket closing
      // upon return from here automagically...
	RemoveClient( pc );
            
	gpc[i] = NULL;
	nOpen--;
}

SaneWinMain( argc, argv )
{
	int i, start, stop;
	if( argc < 2 ) 
   {
   	printf( WIDE("Usage: %s <IP> [port] [range]\n"), argv[0] );
   	return 0;
   }
   pAddress = argv[1];
   start = 1;
   stop = 65535;
   if( argc > 2 )
      start = atoi( argv[2] );
   if( argc > 3 )
      stop = start + atoi( argv[3] );
   NetworkWait(NULL,2000,4);
	for( i = start; i < stop; i++ )
	{
		int bLogged;
//		gpc[i] = OpenTCPClientExx( WIDE("65.0.7.180"), i, NULL, NULL, NULL, (cNotifyCallback)ConnectProc );
		gpc[i] = OpenTCPClientExx( argv[1], (uint16_t)i, NULL, NULL, NULL, (cConnectCallback)ConnectProc );
		if( !gpc[i] )
		{
			fprintf( stderr,WIDE("\rBad Failure: %d\n"), WSAGetLastError() ) ;
		}
		else
		{
			SetNetworkLong( gpc[i], 0, i );
			nOpen++;
		}
		bLogged = FALSE;
		while( nOpen > 20 )
		{
			if( !bLogged )
			{
				fprintf( stderr, WIDE("\rSockets: %d (%d)"), i, nOpen );
				bLogged = TRUE;
			}	
			Sleep(0);
		}	
		fprintf( stderr, WIDE("\rSockets: %d (%d)"), i, nOpen );
	}
	{
		int bDone;
		do
		{
			bDone = TRUE;
			for( i = 0; i < (sizeof(gpc)/sizeof(PCLIENT)) ; i++ )
			{
				if( gpc[i] )
				{
					bDone = FALSE;
					break;
				}				
			}
			Sleep(0);	
		}while( !bDone );
	}
   return 0;
}
EndSaneWinMain()

// $Log: scanner.c,v $
// Revision 1.8  2005/01/27 07:37:11  panther
// Linux cleaned.
//
// Revision 1.7  2003/07/24 08:19:36  panther
// Typecast expressions to make watcom happy
//
// Revision 1.6  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
