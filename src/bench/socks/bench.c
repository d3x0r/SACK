#include <stdhdrs.h>
#include <network.h>
#include <sharemem.h>
#define PSOCKADDR SOCKADDR *
typedef struct local_tag
{
	struct {
		_32 bServer : 1;
		_32 bClient : 1;
		_32 bUnix : 1;
		_32 bUDP : 1;
	} flags;
	PCLIENT pClient;
   PSOCKADDR pAddr;
   _32 waiting;
	_64 start;
	_64 end;
	_64 accum;
	_64 passes;
	_64 min;
	_64 max;
	_64 bytes;

   // buffer to send from...
   _32 data[4096];
} LOCAL;
static LOCAL l;

#ifdef __WATCOMC__
extern _64 CPROC GetCPUTicks();
#pragma aux GetCPUTicks = "rdtsc"

#define SetTick(var)   ((var) = GetCPUTicks())
#else
#define SetTick(var) 	asm( WIDE("rdtsc\n") : "=A"(var) );
#endif

void CPROC UDPClientRead( PCLIENT pc, POINTER buffer, int len, PSOCKADDR sa )
{
	if( !buffer )
	{
      buffer = Allocate( 4096 );
	}
	else
	{
      _64 del;
		SetTick( l.end );
		l.accum += (del = l.end - l.start);
		l.passes++;
		l.bytes += len;
		if( (!l.min) || (del < l.min) )
			l.min = del;
		if( del > l.max )
			l.max = del;
      l.waiting = 0;
	}
   ReadUDP( pc, buffer, 4096 );
}

void CPROC ClientReadComplete( PCLIENT pc, POINTER buffer, int len )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	else
	{
      _64 del;
		SetTick( l.end );
		l.accum += (del = l.end - l.start);
		l.passes++;
		l.bytes += len;
		if( (!l.min) || ( del < l.min ) )
			l.min = del;
		if( del > l.max )
			l.max = del;
      l.waiting = 0;
	}
   ReadTCP( pc, buffer, 4096 );
}

void OpenClient( void )
{
	PSOCKADDR pAddr;
	if( l.flags.bUDP )
	{
		pAddr = CreateSockAddress( WIDE("127.0.0.1:10005"), 10005 );
      l.pClient = ConnectUDPAddr( pAddr, l.pAddr, UDPClientRead, NULL );
	}
	else
	{
		if( l.flags.bUnix )
			pAddr = CreateRemote( WIDE("./TestSocket"), 0 );
		else
			pAddr = CreateSockAddress( WIDE("127.0.0.1:10000"), 10000 );
		l.pClient = OpenTCPClientAddrEx( pAddr, ClientReadComplete, NULL, NULL );
      SetTCPNoDelay( l.pClient, 1 );
	}
}

void CPROC UDPServerRead( PCLIENT pc, POINTER buffer, _32 len, PSOCKADDR sa )
{
	if( !buffer )
	{
      buffer = Allocate( 4096 );
	}
   else
		SendUDPEx( pc, buffer, len, sa );
   ReadUDP( pc, buffer, 4096 );
}

void CPROC ServerReadComplete( PCLIENT pc, POINTER buffer, int len )
{
	if( !buffer )
	{
      buffer = Allocate( 4096 );
	}
	else
	{
      SendTCP( pc, buffer, len );
	}
   ReadTCP( pc, buffer, 4096 );
}

void CPROC Connected( PCLIENT pServer, PCLIENT pClient )
{
   SetNetworkReadComplete( pClient, ServerReadComplete );
	SetTCPNoDelay( pClient, 1 );
}

void OpenServer( void )
{
	PCLIENT pServer;
	PSOCKADDR pAddr;
	if( l.flags.bUDP )
	{
		l.pAddr = CreateSockAddress( WIDE("127.0.0.1:10001"), 10001 );
      pServer = ServeUDPAddr( l.pAddr, UDPServerRead, NULL );
	}
	else
	{
		if( l.flags.bUnix )
			pAddr = CreateRemote( WIDE("./TestSocket"), 0 );
		else
			pAddr = CreateSockAddress( WIDE("127.0.0.1:10000"), 10000 );
		pServer = OpenTCPListenerAddrEx( pAddr, Connected );
      SetTCPNoDelay( pServer, 1 );
	}
}

void DumpStats( void )
{
	printf( WIDE("Accumulated ticks: %Ld\n"), l.accum );
	printf( WIDE("Packets          : %Ld\n"), l.passes );
	printf( WIDE("Bytes            : %Ld\n"), l.bytes );
	printf( WIDE("Min ticks        : %Ld\n"), l.min );
	printf( WIDE("Max ticks        : %Ld\n"), l.max );
	printf( WIDE("Avg ticks        : %Ld\n"), l.passes?(l.accum / l.passes):0);
	l.min = 0;
	l.max = 0;
	l.accum = 0;
	l.bytes = 0;
   l.passes = 0;
}

void TCPTest1Byte( void )
{
	int n;
   _32 sec = time(NULL);
	//for( n = 0; n < 10000; n++ )
   while( (sec+5) > time(NULL) )
	{
		l.waiting = 1;

		SetTick( l.start );
		SendTCP( l.pClient, l.data, 1 );
	// have to wait here until the responce is received.
		while( l.waiting )
			Relinquish();
      //lprintf( WIDE("Got back 1 byte.") );
	}
   DumpStats();
}

void UDPTest1Byte( void )
{
	int n;
   _32 sec = time(NULL);
	while( (sec+5) > time(NULL) )
	//for( n = 0; n < 10000; n++ )
	{
		l.waiting = 1;

		SetTick( l.start );
		SendUDP( l.pClient, l.data, 1 );
	// have to wait here until the responce is received.
		while( l.waiting )
         Relinquish();
	}
   DumpStats();
}


void UDPTest1000Byte( void )
{
	int n;
   _32 sec = time(NULL);
	while( (sec+5) > time(NULL) )
	//for( n = 0; n < 10000; n++ )
	{
		l.waiting = 1;

		SetTick( l.start );
		SendUDP( l.pClient, l.data, 1000 );
	// have to wait here until the responce is received.
		while( l.waiting )
         Relinquish();
	}
   DumpStats();
}

void TCPTest1000Byte( void )
{
	int n;
   _32 sec = time(NULL);
	while( (sec+5) > time(NULL) )
	//for( n = 0; n < 10000; n++ )
	{
		l.waiting = 1;

		SetTick( l.start );
		SendTCP( l.pClient, l.data, 1000 );
	// have to wait here until the responce is received.
		while( l.waiting )
         Relinquish();
	}
   DumpStats();
}


int main( int argc, char **argv )
{
   NetworkStart();
	if( argc < 2 )
	{
		printf( WIDE("Usage: %s [scUu]\n"), argv[0] );
		printf( WIDE("  s - server\n") );
		printf( WIDE("  c - client\n") );
		printf( WIDE("  U - use a unix socket instead of tcp\n") );
      printf( WIDE("  u - use a UDP socket instead of tcp\n") );
		printf( WIDE(" s and c may be specified together to test single-process\n") );
      return 0;
	}
	while( argc > 1 )
	{
		char *p = argv[1];
		while( p[0] )
		{
			switch( p[0]  )
			{
			case 's':
			case 'S':
            l.flags.bServer = 1;
            break;
			case 'c':
			case 'C':
            l.flags.bClient = 1;
				break;
			case 'U':
				l.flags.bUnix = 1;
            l.flags.bUDP = 0;
				break;
			case 'u':
				l.flags.bUDP = 1;
            l.flags.bUnix = 0;
            break;
			}
         p++;
		}
		argv++;
      argc--;
	}
	if( l.flags.bServer )
	{
      OpenServer();
	}
	if( l.flags.bClient )
	{
      OpenClient();
	}
	if( l.flags.bClient )
	{
	// all tests are client based.
		if( l.flags.bUDP )
		{
         UDPTest1Byte();
		}
		else
			TCPTest1Byte();
	// all tests are client based.
		if( l.flags.bUDP )
		{
         UDPTest1000Byte();
		}
		else
			TCPTest1000Byte();
	}
	else
      WakeableSleep( 10000 );
   return 0;
}
