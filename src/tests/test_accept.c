#include <stdhdrs.h>
#include <stdio.h>
#include <sharemem.h>
#include <network.h>

void CPROC ReadCallback( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
		buffer = Allocate( 256 );
	}
	else
	{
		lprintf( WIDE("Received data on %p"), pc );
		LogBinary( buffer, len );
	}
	ReadTCP( pc, buffer, 256 );
}

void CPROC ReadCallback3( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
		RemoveClient( pc );
		return;
	}
	else
	{
		lprintf( WIDE("Received data on %p"), pc );
		LogBinary( buffer, len );
		RemoveClient( pc );
		return;
	}
	ReadTCP( pc, buffer, 256 );
}

void CPROC ReadCallback2( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
		buffer = Allocate( 256 );
	}
	else
	{
		lprintf( WIDE("Received data on %p"), pc );
		LogBinary( buffer, len );
		RemoveClient( pc );
	}
	ReadTCP( pc, buffer, 256 );
}

void CPROC WriteCallback( PCLIENT pc )
{
	lprintf( WIDE("disconnect client after write") );
	RemoveClient( pc );
}

void CPROC WriteCallback2( PCLIENT pc )
{
	lprintf( WIDE("disconnect client after write") );
	RemoveClient( pc );
}

void CPROC ConnectCallback( PCLIENT pc, int error )
{
	if( error )
		lprintf( WIDE("Connect event error : %d"), error );
	else
	{
		lprintf( WIDE("Connect completed (remove client)") );
		RemoveClient( pc );
	}
	return;
}

void CPROC ConnectCallback2( PCLIENT pc, int error )
{
	if( error )
	{
		lprintf( WIDE("Connect event error : %d"), error );
	}
	else
	{
		lprintf( WIDE("Connect completed") );
		SendTCP( pc, WIDE("network event thread send"), sizeof( WIDE("network event thread send") ) );
	}
	return;
}

int main( void )
{
	int n;
	int d = 0;
	NetworkStart();
	for( n = 0; n < 10000; n++ )
	{
		PCLIENT pc = OpenTCPClient( WIDE("127.0.0.1"), 23, NULL );
		lprintf( WIDE("1) result %p"), pc );
		if( pc )
		{
			RemoveClient( pc );
			d++;
		}


		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, NULL, NULL, WriteCallback, ConnectCallback );
		lprintf( WIDE("2) result %p"), pc );
		if( pc )
		{
			d++;
		}

		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, ReadCallback, NULL, WriteCallback2, ConnectCallback );
		lprintf( WIDE("3) result %p"), pc );
		if( pc )
		{
			d++;
		}

		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, NULL, NULL, NULL, NULL );
		lprintf( WIDE("4) result %p"), pc );
		if( pc )
		{
			SendTCP( pc, WIDE("other thread send"), sizeof( WIDE("other thread send") ) );
			RemoveClient( pc );
			d++;
		}

		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, ReadCallback, NULL, NULL, NULL );
		lprintf( WIDE("5) result %p"), pc );
		if( pc )
		{
			SendTCP( pc, WIDE("other thread send"), sizeof( WIDE("other thread send") ) );
			RemoveClient( pc );
			d++;
		}

		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, ReadCallback2, NULL, NULL, NULL );
		lprintf( WIDE("6) result %p"), pc );
		if( pc )
		{
			SendTCP( pc, WIDE("other thread send"), sizeof( WIDE("other thread send") ) );
			RemoveClient( pc );
			d++;
		}
		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, ReadCallback3, NULL, NULL, NULL );
		lprintf( WIDE("7) result %p"), pc );
		if( pc )
		{
			SendTCP( pc, WIDE("other thread send"), sizeof( WIDE("other thread send") ) );
			RemoveClient( pc );
			d++;
		}


		pc = OpenTCPClientExx( WIDE("127.0.0.1"), 23, NULL, NULL, NULL, ConnectCallback2 );
		lprintf( WIDE("8) result %p"), pc );
		if( pc )
		{
			RemoveClient( pc );
			d++;
		}

		//if( ( n % 100 ) == 0 )
		{
			printf( "\rconections: %d of %d", d, n );
			fflush( stdout );
		}
	}
	return 0;
}
