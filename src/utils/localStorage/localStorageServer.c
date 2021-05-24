



#include "sack_ucb_networking.h"
#include "systray/systray.h"
#include "object-storage-remote.h"
#include "nexus1.h" 
#include "jsox.h" 

struct connection {
	struct jsox_parse_state* state;
};

static uintptr_t my_web_socket_opened( PCLIENT pc, uintptr_t psv )
{
	lprintf( "Connection opened... %p %p", pc, psv );
	struct connection* c = New( struct connection );
	c->state = jsox_begin_parse();


	return psv;
}

static void my_web_socket_closed( PCLIENT pc, uintptr_t psv, int code, const char *reason )
{
	lprintf( "Connection closed... %p %p", pc, psv );
}

static void my_web_socket_error( PCLIENT pc, uintptr_t psv, int error )
{
	/* no errors are implemented yet*/
	lprintf( "Connection error... %p %p", pc, psv );
}

static void my_web_socket_event( PCLIENT pc, uintptr_t psv, LOGICAL binary, CPOINTER buffer, size_t msglen )
{
	struct connection* c = (struct connection*)psv;
	int status;
	for( status = jsox_parse_add_data( c->state, buffer, msglen );
		status > 0;
		status = jsox_parse_add_data( c->state, NULL, 0 ) ) {
		PDATALIST msg = jsox_parse_get_data( c->state );
		INDEX idx;
		struct jsox_value_container* val;
		DATA_FORALL( msg, idx, struct jsox_value_container*, val ) {
			if( val->value_type == JSOX_VALUE_OBJECT ) {
				INDEX fidx;
				struct jsox_value_container* field;
				DATA_FORALL( val->contains, fidx, struct jsox_value_container*, field ) {
					if( StrCaseCmpEx( field->name, "op", 2 ) == 0 ) {
						lprintf( "Handle message: %.*s", field->stringLen, field->string );
					} else if( StrCaseCmpEx( field->name, "data", 4 ) == 0 ) {
					}
				}
			}
		}
	}

}

static uintptr_t request( PCLIENT pc, uintptr_t psv )
{
	HTTPState state = GetWebSocketHttpState( pc );
	PTEXT tmp = GetWebSocketResource( pc );
	//lprintf
	// how to respond?
	lprintf( "Extra request...:%s", GetText( tmp ) );
	// abort.

	PVARTEXT pvt = VarTextCreate();
	if( StrCmp( GetText( tmp ), "/favicon.ico" ) == 0 ) {

		vtprintf( pvt, "HTTP/1.1 200 OK\r\n" );
		vtprintf( pvt, "Content-Type:image/jpeg\r\n" );
		vtprintf( pvt, "\r\n" );
		PTEXT header = VarTextPeek( pvt );

		SendTCP( pc, GetText( header ), GetTextSize( header ) );
		SendTCP( pc, nexus, sizeof( nexus ) );

	}  else if( StrCmp( GetText( tmp ), "/js" ) == 0 ) {
		vtprintf( pvt, "HTTP/1.1 200 OK\r\n" );
		vtprintf( pvt, "Content-Type:text/javascript\r\n" );
		vtprintf( pvt, "\r\n" );
		PTEXT header = VarTextPeek( pvt );

		SendTCP( pc, GetText( header ), GetTextSize( header ) );
		SendTCP( pc, storage_js, sizeof( storage_js ) );
	} else if( StrCmp( GetText( tmp ), "/jsox.mjs" ) == 0 ) {
		vtprintf( pvt, "HTTP/1.1 200 OK\r\n" );
		vtprintf( pvt, "Content-Type:text/javascript\r\n" );
		vtprintf( pvt, "\r\n" );
		PTEXT header = VarTextPeek( pvt );

		SendTCP( pc, GetText( header ), GetTextSize( header ) );
		SendTCP( pc, jsox, sizeof( jsox ) );

	} else {
		vtprintf( pvt, "HTTP/1.1 200 OK\r\n" );
		vtprintf( pvt, "Content-Type:text/html\r\n" );
		vtprintf( pvt, "\r\n" );
		vtprintf( pvt, "<html><head><title>localStorage</title></head><body></body>" );
		vtprintf( pvt, "<script src=\"js\" type=\"module\"></script>" );
		vtprintf( pvt, "</html>" );
		PTEXT header = VarTextPeek( pvt );
		SendTCP( pc, GetText( header ), GetTextSize( header ) );
	}
	VarTextDestroy( &pvt );

	RemoveClient( pc );
	return psv;
}

int main( void )
{
	RegisterIcon( NULL );
	NetworkStart();
	{
		PCLIENT socket = WebSocketCreate( "ws://[::1]:9314/"
											  , my_web_socket_opened
											  , my_web_socket_event
											  , my_web_socket_closed
											  , my_web_socket_error
											  , 0
											  );
		SetWebSocketHttpCallback( socket, request );
	}
	while( 1 )
		WakeableSleep( 10000 );

	return 0;
}

