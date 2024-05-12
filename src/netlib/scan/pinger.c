#define NO_UNICODE_C
#include <stdhdrs.h>
#include <network.h>


void PingResult( SOCKADDR* dwIP, CTEXTSTR name
						, int min, int max, int avg
						, int drop, int hops )
{
	if( dwIP ) // else was a timeout.
	printf( "Result: %25s(%12s) %d %d %d\n", name, GetAddrString( dwIP )
                 							, min, max, avg );
}

SaneWinMain( argc, argv )
{
	if( argc < 2 )
	{
		printf( "Please enter a IP - class c will be scanned\n" );
		return 0;
	}
   NetworkWait(NULL,2000,4);
	{
		TEXTCHAR address[256];
		uint32_t IP;
		IP = inet_addr(CStrDup(argv[1]));
		IP = ntohl( IP );
		for( ; ( IP & 0xFFff ) != 0xFFFF ; IP++ )
		{
			uint32_t junk = htonl(IP);
			StrCpy( address, DupCStr( inet_ntoa( *(struct in_addr*)&junk ) ) );
			//printf( "Trying %s...\n", address );
			 DoPing( address, 
      	         0,  // no ttl - just ping
         	    50,  // short timeout
            	 1,    // just one time
	             NULL, 
   	          TRUE, // no RDNS for now
      	       PingResult );
		}
	}
	return 0;
}
EndSaneWinMain()


// $Log: pinger.c,v $
// Revision 1.3  2005/01/27 07:37:11  panther
// Linux cleaned.
//
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
