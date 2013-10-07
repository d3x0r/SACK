
#include <rpc.h>
#include <stdio.h>
#include <logging.h>

int main( void )
{
	UUID tmp;
	RPC_STATUS result = UuidCreate( &tmp );
	SetSystemLog( SYSLOG_FILE, stdout );
   LogBinary( &tmp, sizeof( tmp ) );
	printf( "result was %d\n", result );
   return 0;
}
