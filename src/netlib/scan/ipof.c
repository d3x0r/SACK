#include <stdhdrs.h>
#include <network.h>

#ifdef __LINUX__
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


int main( int argc, char **argv )
{
	if( argc < 1 )
	{
		printf( WIDE("Usage: %s <name>\n"), argv[0] );
      return 1;
	}
   if( NetworkWait(NULL,1,0) )
	{
		SOCKADDR *sa = CreateRemote( argv[1], 5555 );
		_32 IP;
		GetAddressParts( sa, &IP, NULL );

		printf( WIDE("%")_32f WIDE(".%")_32f WIDE(".%")_32f WIDE(".%")_32f WIDE("")
				, (IP & 0xFF)
				, (IP & 0xFF00) >> 8
				, (IP & 0xFF0000) >> 16
				, (IP & 0xFF000000) >> 24
				);
	}
	else
      printf( WIDE("Failed to enable network.") );
	return 0;
}


//
//
