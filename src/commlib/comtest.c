
#include <stdhdrs.h>
#include <sackcomm.h>

void CPROC callback( uintptr_t psv, int com, POINTER buffer, int len )
{
	lprintf( WIDE("received %p %d\n"), buffer, len );
	LogBinary( buffer, len );
}


SaneWinMain( argc, argv )
{
	int port = SackOpenComm( argv[1], 0, 0 );
	SetSystemLog( SYSLOG_FILE, stdout );
	SackSetReadCallback( port, callback, 0 );
	while( 1 )
		WakeableSleep( 100000 );
	return 0;
}
EndSaneWinMain()
