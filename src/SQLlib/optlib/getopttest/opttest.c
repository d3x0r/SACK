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
		uint32_t tick = GetTickCount() + 2000;
		while( tick > GetTickCount() );
	}
   lprintf( "2 seconds." );


	Interface = (POPTION_INTERFACE)GetInterface( "SACK_SQL_options" );

	for( n = 0; n < 5; n++ )
	{
		lprintf( "Begin read..." );
		OptGetProfileString( "aims/settings/receipt printer/enabled", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "read" );
		//lprintf( "Begin write..." );
		//OptWriteProfileString( "aims/settings/receipt printer/enabled", "enabled", "value goes here" );
		//lprintf( "Written" );
		lprintf( "Begin read1..." );
		OptGetProfileString( "aims/settings/receipt printer/test1", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "read1" );
		//lprintf( "Begin write1..." );
		//OptWriteProfileString( "aims/settings/receipt printer/test1", "enabled", "value goes here" );
		//lprintf( "Written1" );
		lprintf( "Begin read2..." );
		OptGetProfileString( "aims/settings/receipt printer/test2", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "read2" );
		lprintf( "2Begin read..." );
		OptGetProfileString( "aims2/settings/receipt printer/enabled", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "2read" );
		//lprintf( "Begin write..." );
		//OptWriteProfileString( "aims/settings/receipt printer/enabled", "enabled", "value goes here" );
		//lprintf( "Written" );
		lprintf( "2Begin read1..." );
		OptGetProfileString( "aims2/settings/receipt printer/test1", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "2read1" );
		//lprintf( "Begin write1..." );
		//OptWriteProfileString( "aims/settings/receipt printer/test1", "enabled", "value goes here" );
		//lprintf( "Written1" );
		lprintf( "2Begin read2..." );
		OptGetProfileString( "aims2/settings/receipt printer/test2", "enabled", "default", buffer, 256 );
		//OptWriteProfileInt( "aims/settings/receipt printer", "test2", 13 );
		lprintf( "2read2" );
		//lprintf( "Begin write2..." );
		//OptWriteProfileString( "aims/settings/receipt printer/test2", "enabled", "value goes here" );
		//lprintf( "Written2" );
	}
	//OptGetProfileString( "SecTion1", "test", "default", buffer, 256 );
	//printf( "Result buffer: %s\n", buffer );
	//OptGetProfileString( "SecTion1", "test3", "default", buffer, 256 );
	//printf( "Result buffer: %s\n", buffer );
	//printf( "Result value: %ld\n", OptGetProfileInt( "SeCTION1", "test2", 111 ) );
	//printf( "Result value: %ld\n", OptGetProfileInt( "SeCTION1", "test4", 333 ) );
   return 0;
}
