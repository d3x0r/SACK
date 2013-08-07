#include <stdhdrs.h>
#include <network.h>
#include <sharemem.h>

_64 rcv;

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size, SOCKADDR *saFrom )
{
	if( !buffer )
	{
      buffer = Allocate( 4096 );
	}
	else
	{
      static _32 last;
		rcv++;
		if( !last )
			last = GetTickCount();
		else
		{
			if( (GetTickCount() - last) > 50 )
			{
            last = GetTickCount();
				printf( "Received %Ld\r", rcv );
			}
		}
		//printf( "Received." );
	}
	ReadUDP( pc, buffer, 4096 );
}

int main( int argc, char **argv )
{
	PCLIENT pcServe;
	NetworkStart();
	if( argc > 1 )
	{
		if( StrCaseCmp( argv[1], "listen" ) == 0 )
		{
			pcServe = ServeUDP( "127.0.0.1", 5123, ReadComplete, NULL );
		}
		else
		{
			int n = 0;
         //SOCKADDR *saSend = CreateSockAddress( "255.255.255.255", 6595 );
         SOCKADDR *saSend = CreateSockAddress( "127.0.0.1", 5123 );
			for( n = 0; n < 50; n++ )
			{
				//pcServe = ServeUDP( "172.20.2.84", 5124+n, ReadComplete, NULL );
				pcServe = ServeUDP( "127.0.0.1", 5124+n, ReadComplete, NULL );
            UDPEnableBroadcast( pcServe, TRUE );
				if( pcServe )
					break;
			}
			if( pcServe )
			{
            _64 cnt;
            _32 tick = GetTickCount();
				while(1) 
				{
               cnt++;
					SendUDPEx( pcServe, "test", 625, saSend);
					if( ( ( tick + 250 ) < GetTickCount() ) && ( tick = GetTickCount() ) )
						printf( "Sent %Ld packets\r", cnt );
               Relinquish();
				}
            return 0;
			}
		}
		while( 1 )
			WakeableSleep( 500 );
	}
	else
	{
		printf( "Usage: %s <listen/send>\n", argv[0] );
	}
	return 1;
}

