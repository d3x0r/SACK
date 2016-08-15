#include <stdhdrs.h>

#include <html5.websocket.client.h>

int done;
PTHREAD main_thread;

uintptr_t my_web_socket_opened( PCLIENT pc, uintptr_t psv )
{
	lprintf( "Connection opened... %p %p", pc, psv );
   WebSocketSendText( pc, "Test Message", 12 );
	return psv;
}

void my_web_socket_closed( PCLIENT pc, uintptr_t psv )
{
	lprintf( "Connection closed... %p %p", pc, psv );
	done = 1;
	WakeThread( main_thread );
}

void my_web_socket_error( PCLIENT pc, uintptr_t psv, int error )
{
	/* no errors are implemented yet*/
	lprintf( "Connection error... %p %p", pc, psv );
}

void my_web_socket_event( PCLIENT pc, uintptr_t psv, POINTER buffer, int msglen )
{
	lprintf( "Recieved event" );
	LogBinary( buffer, msglen );
	//WebSocketSendText( pc, buffer, msglen );
}

int main( void )
{
	PCLIENT socket;
	NetworkStart();
	socket = WebSocketOpen( "ws://localhost:9998/echo", 0
								 , my_web_socket_opened
								 , my_web_socket_event
								 , my_web_socket_closed
								 , my_web_socket_error
								 , 0
								 );
	if( socket )
	{
		main_thread = MakeThread();
		while( !done )
			WakeableSleep( 10000 );
	}
	return 0;
}

