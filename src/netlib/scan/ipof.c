#include <stdhdrs.h>
#include <network.h>

#ifdef __LINUX__
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


SaneWinMain( argc, argv )
{
	if( argc < 1 )
	{
		printf( "Usage: %s <name>\n", argv[0] );
      return 1;
	}
   if( NetworkWait(NULL,1,0) )
	{
		SOCKADDR *sa = CreateRemote( argv[1], 5555 );
		uint32_t IP;
		GetAddressParts( sa, &IP, NULL );

		printf( "%"_32f ".%"_32f ".%"_32f ".%"_32f ""
				, (IP & 0xFF)
				, (IP & 0xFF00) >> 8
				, (IP & 0xFF0000) >> 16
				, (IP & 0xFF000000) >> 24
				);
	}
	else
      printf( "Failed to enable network." );
	return 0;
}
EndSaneWinMain()

//
//
