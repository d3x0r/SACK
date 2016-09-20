#include <stdio.h>
#include <sharemem.h>
#include <network.h>




void ReadServer( PCLIENT pc, POINTER buffer, int size )
{
	int toread = 4;
	if( !buffer )
		buffer = Allocate( 4096 );
	else
	{
		if( size == 4 )
			toread = *(uint32_t*)buffer;
		else
		{
			printf( " server %d byte packet read...", size );
			fflush( stdout );
		}
	}
	    
	ReadTCPMsg( pc, buffer, toread );
}

void ReadClient( PCLIENT pc, POINTER buffer, int size )
{
	int toread = 4;
	if( !buffer )
		buffer = Allocate( 4096 );
	else
	{
    		if( size == 4 )
    			toread = *(uint32_t*)buffer;
		else
		{
			printf( " client %d byte packet read...", size );
			fflush( stdout );
		}
	}
	ReadTCPMsg( pc, buffer, toread );
}

void CloseServer( PCLIENT pc )
{
        printf( "\n server side close" );
	fflush( stdout );
}


void Connected( PCLIENT pcServer, PCLIENT pcNew )
{
	printf( "\n server size received connect." );
	fflush( stdout );
	SetNetworkCloseCallback( pcNew, CloseServer );
	SetNetworkReadComplete( pcNew, ReadServer );
}

void CloseClient( PCLIENT pc )
{
	printf( "\n client side close" );
	fflush( stdout );
}


int main( int argc, char **argv )
{

	PCLIENT pcListen;
	PCLIENT pcSend;
	CTEXTSTR connect_to;

	connect_to = NULL;
	NetworkWait( NULL, 16, 16 );
	
	if( argc < 2 )
	{
		printf( "Command line arguments [-c address] or [-s] \n" );
		return -1;
	}
	
	if( strcmp( argv[1], "-s" ) == 0 )
	{
		pcListen = OpenTCPListener( 6555 );
	}

	
	if( strcmp( argv[1], "-c" ) == 0 )
	{
	    if( argv[1][2] )
		connect_to = argv[1] + 2;
	    else
		connect_to = argv[2];
	}
	if( connect_to )
	{
		int m; 
		for( m = 0; m < 100; m++ )
		{
		int n;
		for( n = 0; n < 10; n++ )
		{
			pcSend = OpenTCPClientEx( connect_to, 6555
				, ReadClient
				, CloseClient, NULL );
			if( pcSend )
			{
				uint32_t len = 32;
				int buf[128];
				SetTCPNoDelay( pcSend, TRUE );
				SendTCP( pcSend, &len, sizeof( len ) );
				SendTCP( pcSend, buf, len );
			}
			RemoveClient( pcSend );
		}
WakeableSleep( 11000 );
}
	}
	else
	{
		while( 1 )
			WakeableSleep( 100000 );
	}

	
	return 0;
}
