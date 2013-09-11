
#include <stdhdrs.h>
#include <network.h>

#include <html5.websocket.h>


int main( void )
{
	PCLIENT socket = WebSocketCreate( "0.0.0.0:9998" );

	while( 1 )
      WakeableSleep( 10000 );

   return 0;
}

