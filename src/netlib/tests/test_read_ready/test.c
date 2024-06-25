
#include <stdhdrs.h>
#include <network.h>

struct local {
	PCLIENT server;
   PCLIENT client;
} l;


static void serverRead( PCLIENT pc, POINTER buffer, size_t length ) {
	if( !buffer ) {
		buffer = Allocate( 1024 );
	} else {
		lprintf( "Read: %d", length );
	}
	ReadTCP( pc, buffer, length );
}


static void connected( PCLIENT pcServer, PCLIENT pcNew ) {
	SetNetworkReadComplete( pcNew, serverRead );
	SetNetworkWriteComplete( pcNew, serverWrite );
}

void Init( void ) {
	NetworkStart();
	l.server = OpenTCPListener( 1234 );
   l.client = OpenTCPClient( "localhost", 1234, NULL );
}


void Test1( void ) {
	int i;
	for( i = 0; i < 10; i++ ) {
		SendTCP( l.client, "1234", 4 );
		WakeableSleep( 1000 );
	}
}

int main( void ) {
	Init();
	Test1();
   WakeableSleep( SLEEP_FOREVER );
}
