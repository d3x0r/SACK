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
		printf( WIDE("Usage: %s <IP>\n"), argv[0] );
      return 1;
	}
   if( NetworkWait(NULL,1,0) )
	{
		unsigned long dwIP = inet_addr( argv[1] );
		struct hostent *phe;

		phe = gethostbyaddr( (char*)&dwIP, 4, AF_INET );
		if( phe )
		{
         char **alias = phe->h_aliases;
			printf( WIDE("%s %s"), argv[1], phe->h_name );
			while( alias[0] )
			{
				printf( WIDE("or %s"), alias[0] );
				alias++;
			}
			printf( WIDE("\n") );
         alias = phe->h_addr_list;
			while( alias[0] )
			{
				printf( WIDE("%d.%d.%d.%d ")
						, alias[0][0]
						, alias[0][1]
						, alias[0][2]
						, alias[0][3]
						);
				alias++;
			}
		}
		else
         printf( WIDE("%s has no name\n"), argv[1] );
	}
	else
      printf( WIDE("Failed to enable network.") );
	return 0;
}


//
//
