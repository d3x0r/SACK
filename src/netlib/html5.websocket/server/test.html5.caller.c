
#include <stdhdrs.h>
#include <network.h>

#include <html5.websocket.h>


uintptr_t my_web_socket_opened( PCLIENT pc, uintptr_t psv )
{
   return psv;
}

void my_web_socket_closed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
}

void my_web_socket_error( PCLIENT pc, uintptr_t psv, int error )
{
}

void my_web_socket_event( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen )
{
   WebSocketSendText( pc, buffer, msglen );
}


int main( void )
{
	PCLIENT socket = WebSocketCreate( WIDE("0.0.0.0:9998")
											  , my_web_socket_opened
											  , my_web_socket_event
											  , my_web_socket_closed
											  , my_web_socket_error
											  , (uintptr_t)0
											  );

	while( 1 )
      WakeableSleep( 10000 );

   return 0;
}

