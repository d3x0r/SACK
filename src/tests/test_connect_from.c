#include <stdhdrs.h>

#include <network.h>

static struct local_tag {
	PCLIENT listener;
   PCLIENT connection;
} local;

void ListenerConnect( PCLIENT pc_server, PCLIENT pc_new )
{

}

void ClientConnect( PCLIENT pc, int err )
{

}

void ReadComplete( PCLIENT pc, POINTER buf, size_t len )
{
	if( !buf )
	{
		buf = Allocate( 1024 );
	}
	else
	{

	}
	ReadTCP( pc, buf, 1024 );
}

void ClientClose( PCLIENT pc )
{
}

SaneWinMain( argc, argv )
{
	SOCKADDR *addr;
	PLIST addresses;
	INDEX idx;
	CTEXTSTR a;
	NetworkStart();
	addresses = GetLocalAddresses();
	LIST_FORALL( addresses, idx, CTEXTSTR, a )
		DumpAddr( "address", a );

	local.listener = OpenTCPListenerEx( 16666, ListenerConnect );
	//addr = CreateSockAddress( "127.0.0.1", 16666 );

	LIST_FORALL( addresses, idx, CTEXTSTR, a )
	{
		addr = DuplicateAddress( a );
		SetAddressPort( addr, 16666 );
		lprintf( "Connecting to %s", GetAddrName( addr ) );
		if( IsAddressV6( addr ) )
			local.connection = OpenTCPClientAddrExx( addr, ReadComplete, ClientClose, NULL, ClientConnect );
		else
			local.connection = OpenTCPClientAddrFrom( addr, 16666, ReadComplete, ClientClose, NULL, ClientConnect );
	}

	while( 1 )
		WakeableSleep( 10000 );
}
EndSaneWinMain()

