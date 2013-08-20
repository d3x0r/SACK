
#include <stdhdrs.h> // Sleep
#include <stdio.h>
#include <timers.h>
#include <stdio.h>
#include <logging.h>

void TimerProc( PTRSZVAL unused )
{
   lprintf( WIDE("Timer %d\n"), unused );
}


int main( int argc, char **argv )
{
	int timers;
   fprintf( stderr, " Sending logging to UDP local port 514\n" );
	SetSystemLog( SYSLOG_UDP, stdout );
	//SetSystemLog( SYSLOG_FILE, stdout );
	if( argc < 2 )
	{
		  AddTimer( 731, TimerProc, 731 );
        AddTimerEx( 1000, 0, TimerProc, 1 );
        AddTimerEx( 0, 3000, TimerProc, 3 );
        AddTimer( 7000, TimerProc, 7 );
        AddTimer( 13000, TimerProc, 13 );
		  AddTimer( 17000, TimerProc, 17 );
		  while( 1 )
           WakeableSleep( SLEEP_FOREVER );
        fgetc( stdin );
        RemoveTimer( 1003 );
        fgetc( stdin );
        RemoveTimer( 1002 );
        fgetc( stdin );
        RemoveTimer( 1001 );
        fgetc( stdin );
        RemoveTimer( 1000 );
        fgetc( stdin );
	}
	else
	{
		int i;
		for( i = 1; i < argc; i++ )
		{
			AddTimer( atoi( argv[i] ), TimerProc, atoi( argv[i] ) );
                }
                fgetc(stdin);
		for( i = 1; i < argc; i++ )
		{
			RemoveTimer( 999 + i );
                }
                fgetc(stdin);
		for( i = 1; i < argc; i++ )
		{
			AddTimer( atoi( argv[i] ), TimerProc, atoi( argv[i] ) );
                }
                fgetc(stdin);
		for( i = 1; i < argc; i++ )
		{
			RemoveTimer( 999 + i );
                }
                fgetc(stdin);
	}
  //while( !getch() );
   //while( !kbhit() ) Sleep( 100 );
}
// $Log: $
