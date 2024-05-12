#include <stdhdrs.h>
#include <network.h>


void PingResult( SOCKADDR* dwIP, CTEXTSTR name
						, int min, int max, int avg
						, int drop, int hops )
{
	if( dwIP ) { // else was a timeout.
		CTEXTSTR str = GetAddrString( dwIP );
		printf( "Result: %25s(%12s) %d %d %d\n", name, str
                 							, min, max, avg );
		FreeAddrString( str );
	}
}

SaneWinMain( argc, argv )
{
   int ttl = 0;
	if( argc < 2 )
	{
		printf( "Please enter a IP - class c will be scanned\n" );
		return 0;
	}
	if( argc > 2 ) {
      ttl = atoi( argv[2] );
	}
   NetworkWait(NULL,2000,4);
  		printf( "Trying %s...TTL:%d\n", argv[1], ttl );
			 DoPing( argv[1],
      	         ttl,  // no ttl - just ping
         	    150,  // short timeout
            	 3,    // just one time
	             NULL, 
   	          TRUE, // no RDNS for now
      	       PingResult );
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
