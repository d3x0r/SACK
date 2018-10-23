#include "sack_typelib.h"


int done;
PTHREAD main_thread;

// -------------- WebSocket Client ------------------------


uintptr_t my_web_socket_opened( PCLIENT pc, uintptr_t psv )
{
	lprintf( "Connection opened... %p %p", pc, psv );
   WebSocketSendText( pc, "Test Message", 12 );
	return psv;
}

void my_web_socket_closed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
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

void my_web_socket_event( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen )
{
	lprintf( "Recieved event" );
	LogBinary( buffer, msglen );
	//WebSocketSendText( pc, buffer, msglen );
}

int webSocketClient( void )
{
	PCLIENT socket;
	NetworkStart();
	socket = WebSocketOpen( "ws://localhost:9998/echo", 0
								 , my_web_socket_opened
								 , my_web_socket_event
								 , my_web_socket_closed
								 , my_web_socket_error
								 , 0
								, "echo"
								 );
	if( socket )
	{
		main_thread = MakeThread();
		while( !done )
			WakeableSleep( 10000 );
	}
	return 0;
}



// -------------- WebSocket Server ------------------------

uintptr_t my_web_socket_server_opened( PCLIENT pc, uintptr_t psv )
{
	lprintf( WIDE("Connection opened... %p %p"), pc, psv );
	return psv;
}

void my_web_socket_server_closed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	lprintf( WIDE("Connection closed... %p %p"), pc, psv );
}

void my_web_socket_server_error( PCLIENT pc, uintptr_t psv, int error )
{
	/* no errors are implemented yet*/
	lprintf( WIDE("Connection error... %p %p"), pc, psv );
}

void my_web_socket_server_event( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen )
{
	lprintf( WIDE("Recieved event") );
	LogBinary( buffer, msglen );
	WebSocketSendText( pc, buffer, msglen );
}

int webSocketServer( void )
{
	PCLIENT socket = WebSocketCreate( WIDE("ws://0.0.0.0:9998/echo")
											  , my_web_socket_server_opened
											  , my_web_socket_server_event
											  , my_web_socket_server_closed
											  , my_web_socket_server_error
											  , 0 );


	return 0;
}

//-------------------------------- BEGIN TEST MAIN ----------------

int main( void ) {
	PLIST list = NULL;
	INDEX idx;
	char *name;
	AddLink( &list, "asdf" );
	LIST_FORALL( list, idx, char *, name ) {
		printf( "list has: %d = %s\n", idx, name );
	}

	webSocketServer();
	webSocketClient();
	
	return 0;
}

