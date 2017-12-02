
#include <stdhdrs.h>
#include <network.h>

#include <html5.websocket.h>


uintptr_t my_web_socket_opened( PCLIENT pc, uintptr_t psv )
{
	lprintf( WIDE("Connection opened... %p %p"), pc, psv );
	return psv;
}

void my_web_socket_closed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	lprintf( WIDE("Connection closed... %p %p"), pc, psv );
}

void my_web_socket_error( PCLIENT pc, uintptr_t psv, int error )
{
	/* no errors are implemented yet*/
	lprintf( WIDE("Connection error... %p %p"), pc, psv );
}

void my_web_socket_event( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, int msglen )
{
	lprintf( WIDE("Recieved event") );
	LogBinary( buffer, msglen );
	WebSocketSendText( pc, buffer, msglen );
}

int main( void )
{
	PCLIENT socket = WebSocketCreate( WIDE("ws://0.0.0.0:9998/echo")
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

