
#include <stdhdrs.h>
#include <network.h>

#include <html5.websocket.h>


PTRSZVAL my_web_socket_opened( PCLIENT pc, PTRSZVAL psv )
{
   return psv;
}

void my_web_socket_closed( PCLIENT pc, PTRSZVAL psv )
{
}

void my_web_socket_error( PCLIENT pc, PTRSZVAL psv, int error )
{
}

void my_web_socket_event( PCLIENT pc, PTRSZVAL psv, POINTER buffer, int msglen )
{
   WebSocketSendText( pc, buffer, msglen );
}


int main( void )
{
	PCLIENT socket = WebSocketCreate( "0.0.0.0:9998"
											  , my_web_socket_opened
											  , my_web_socket_event
											  , my_web_socket_closed
											  , my_web_socket_error
											  , 0
											  );

	while( 1 )
      WakeableSleep( 10000 );

   return 0;
}

