
#include <stdhdrs.h>
#include <network.h>

struct local {
	PCLIENT server;
	PCLIENT client;
	PCLIENT connected;
} l;


static void serverRead( PCLIENT pc, POINTER buffer, size_t length ) {
	if( !buffer ) {
		buffer = Allocate( 1024 );
	} else {
		lprintf( "Server Read: %d", length );
	}
	ReadTCP( pc, buffer, 1024 );
}

static void serverWrite( PCLIENT pc, CPOINTER buffer, size_t length ) {
	lprintf( "Server Write event callback..." );
}

static void connected( PCLIENT pcServer, PCLIENT pcNew ) {
	l.connected = pcNew;
	SetNetworkReadComplete( pcNew, serverRead );
	SetNetworkWriteComplete( pcNew, serverWrite );
	lprintf( "Client connected" );
}

static void clientRead( PCLIENT pc, POINTER buffer, size_t length ) {
	if( !buffer ) {
		buffer = Allocate( 1024 );
	} else {
		lprintf( "Client Read: %d", length );
	}
	ReadTCP( pc, buffer, 1024 );
}

void Init( void ) {
	NetworkStart();
	l.server = OpenTCPListenerEx( 4321, connected );
	l.client = OpenTCPClient( "localhost", 4321, clientRead );
}


void Test1( void ) {
	int i;
	for( i = 0; i < 10; i++ ) {
		SendTCP( l.client, "1234", 4 );
		WakeableSleep( 1000 );
	}
}


void Test2( void ) {
	int i;
	for( i = 0; i < 10; i++ ) {
		SendTCP( l.connected, "1234", 4 );
		WakeableSleep( 1000 );
	}
}

int main( void ) {
	Init();
	Test1();
	Test2();
	WakeableSleep( SLEEP_FOREVER );
}
