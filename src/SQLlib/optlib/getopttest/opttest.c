#include <stdio.h>
#include <stdhdrs.h>
//#include <windows.h>

#define DEFAULT_OPTION_INTERFACE Interface
#include <sqloptint.h>
POPTION_INTERFACE Interface;

#include <logging.h>

int main( void )
{
   int n;
	char buffer[256];
   SystemLogTime( SYSLOG_TIME_CPU | SYSLOG_TIME_DELTA );
	SetSystemLog( SYSLOG_FILE, stdout );

   lprintf( "begin time sync..." );
	{
		_32 tick = GetTickCount() + 2000;
		while( tick > GetTickCount() );
	}
   lprintf( "2 seconds." );


	Interface = (POPTION_INTERFACE)GetInterface( WIDE("SACK_SQL_options") );

	for( n = 0; n < 5; n++ )
	{
		lprintf( "Begin read..." );
		OptGetProfileString( WIDE("aims/settings/receipt printer/enabled"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "read" );
		//lprintf( "Begin write..." );
		//OptWriteProfileString( WIDE("aims/settings/receipt printer/enabled"), WIDE("enabled"), WIDE("value goes here") );
		//lprintf( "Written" );
		lprintf( "Begin read1..." );
		OptGetProfileString( WIDE("aims/settings/receipt printer/test1"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "read1" );
		//lprintf( "Begin write1..." );
		//OptWriteProfileString( WIDE("aims/settings/receipt printer/test1"), WIDE("enabled"), WIDE("value goes here") );
		//lprintf( "Written1" );
		lprintf( "Begin read2..." );
		OptGetProfileString( WIDE("aims/settings/receipt printer/test2"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "read2" );
		lprintf( "2Begin read..." );
		OptGetProfileString( WIDE("aims2/settings/receipt printer/enabled"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "2read" );
		//lprintf( "Begin write..." );
		//OptWriteProfileString( WIDE("aims/settings/receipt printer/enabled"), WIDE("enabled"), WIDE("value goes here") );
		//lprintf( "Written" );
		lprintf( "2Begin read1..." );
		OptGetProfileString( WIDE("aims2/settings/receipt printer/test1"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "2read1" );
		//lprintf( "Begin write1..." );
		//OptWriteProfileString( WIDE("aims/settings/receipt printer/test1"), WIDE("enabled"), WIDE("value goes here") );
		//lprintf( "Written1" );
		lprintf( "2Begin read2..." );
		OptGetProfileString( WIDE("aims2/settings/receipt printer/test2"), WIDE("enabled"), WIDE("default"), buffer, 256 );
		//OptWriteProfileInt( WIDE("aims/settings/receipt printer"), WIDE("test2"), 13 );
		lprintf( "2read2" );
		//lprintf( "Begin write2..." );
		//OptWriteProfileString( WIDE("aims/settings/receipt printer/test2"), WIDE("enabled"), WIDE("value goes here") );
		//lprintf( "Written2" );
	}
	//OptGetProfileString( WIDE("SecTion1"), WIDE("test"), WIDE("default"), buffer, 256 );
	//printf( WIDE("Result buffer: %s\n"), buffer );
	//OptGetProfileString( WIDE("SecTion1"), WIDE("test3"), WIDE("default"), buffer, 256 );
	//printf( WIDE("Result buffer: %s\n"), buffer );
	//printf( WIDE("Result value: %ld\n"), OptGetProfileInt( WIDE("SeCTION1"), WIDE("test2"), 111 ) );
	//printf( WIDE("Result value: %ld\n"), OptGetProfileInt( WIDE("SeCTION1"), WIDE("test4"), 333 ) );
   return 0;
}
