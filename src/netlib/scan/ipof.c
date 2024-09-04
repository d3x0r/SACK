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
	if( NetworkWait( NULL, 1, 0 ) ) {
		int i;
		for( i = 0; i < 2; i++ ) {
			SOCKADDR* sa = CreateRemoteV2( argv[1], 5555, i?NETWORK_ADDRESS_FLAG_PREFER_V6:NETWORK_ADDRESS_FLAG_PREFER_V4 );
			uint32_t IP;
			if( sa->sa_family == AF_INET ) {
				GetAddressParts( sa, &IP, NULL );

				printf( "%"_32f ".%"_32f ".%"_32f ".%"_32f "\n"
				      , ( IP & 0xFF )
				      , ( IP & 0xFF00 ) >> 8
				      , ( IP & 0xFF0000 ) >> 16
				      , ( IP & 0xFF000000 ) >> 24
				);
			}
			if( sa->sa_family == AF_INET6 ) {
				//uint32_t IP[4];
			//GetAddressParts( sa, IP, NULL );
				printf( "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n"
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+8))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+10))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+12))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+14))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+16))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+18))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+20))))
				      , ntohs(*(((unsigned short *)((unsigned char*)sa+22))))
				      );
				//printf( "%
			}
		}
	}
	else
		printf( "Failed to enable network." );
	return 0;
}
EndSaneWinMain()

//
//
