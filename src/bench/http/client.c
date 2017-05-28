#define CONSOLE_SHELL
#include <stdhdrs.h>
#include <network.h>
#include <http.h>
#include <deadstart.h>

PRELOAD( initNetwork ) {
   NetworkStart();
   SetSystemLog( SYSLOG_FILE, stdout );
}

SaneWinMain( argc, argv )
{
	PTEXT req = SegCreateFromText( "192.168.173.2:8080" );
	PTEXT req2 = SegCreateFromText( "localhost:8080" );
	PTEXT req3 = SegCreateFromText( "127.0.0.1:6180" );
	PTEXT res = SegCreateFromText( "/simple.html" );
	uint32_t now = GetTickCount();
	uint32_t end;
	uint32_t _end = now;
	int n;
	for( n = 0; n < 10000; n++ ) {
		PTEXT result = GetHttp( req3, res, FALSE );
		end = GetTickCount();
		if( end - _end > 300 )
			break;
		_end = end;
		if( (end - now) > 5000 )
			break;
		//lprintf( "tick" );
	}
	lprintf( "only did:%d", n );
	end = GetTickCount();
	lprintf( "Total in %d : %g",  end - now, ((double)n)/((end-now)/1000.0) );
	
	//lprintf( "Got:%s", GetText( result ) );
}
EndSaneWinMain();

