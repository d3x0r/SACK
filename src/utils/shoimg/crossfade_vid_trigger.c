#include <stdhdrs.h>
#include <network.h>


static PCLIENT pcCommand;

static int BeginNetwork( void )
{
	if( !NetworkStart() )
		return 0;
	pcCommand = ConnectUDP( NULL, 3021, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
		pcCommand = ConnectUDP( NULL, 3022, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
		pcCommand = ConnectUDP( NULL, 3023, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
		pcCommand = ConnectUDP( NULL, 3024, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
		pcCommand = ConnectUDP( NULL, 3025, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
		pcCommand = ConnectUDP( NULL, 3026, "255.255.255.255", 3020, NULL, NULL );
	if( !pcCommand )
	{
		srand( GetTickCount() );
		pcCommand = ConnectUDP( NULL, (_16)(3000 + rand() % 10000), "255.255.255.255", 3020, NULL, NULL );
	}


	if( !pcCommand )
	{
		lprintf( "Failed to bind to any port!\n" );
		return 0;
	}
	return 1;
}


SaneWinMain( argc, argv )
{
	if( argc > 1 )
		if( BeginNetwork() )
		{
			_32 packet[3];
			packet[0] = timeGetTime();
			packet[1] = 0;
			packet[2] = atoi( argv[1] );
			SendUDP( pcCommand, packet, 12 );
			Sleep( 25 );
			SendUDP( pcCommand, packet, 12 );
			Sleep( 25 );
			SendUDP( pcCommand, packet, 12 );
		}
   return 0;
}
EndSaneWinMain()

