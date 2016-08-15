// force our logging on.
#define DO_LOGGING

#include <stdhdrs.h> // blah, I hate that I MUST include this.

#include <network.h>
#include <systray.h>
#include <sqlgetoption.h>
#include <../src/intershell/widgets/include/banner.h>
#include "myping.h"
#ifdef PROGRAM
int main( void )
{
	// yeah, I like this main :)
   TerminateIcon();
   RegisterIcon( NULL );
	while( 1 ) WakeableSleep( 100000 );
	return 1;
}
#endif

static struct {
	TEXTCHAR address[256];
	int successes;
   SYSTEMTIME ok_time;
   uint32_t successtime;
	int failures;
   SYSTEMTIME not_ok_time;
   uint32_t failtime;
	PBANNER banner;
} l;

static PTHREAD Monitor;
static int bSetTime; // skip ping
static uint32_t sleep_time;


static void DoLogNow( int forced )
{
	if( l.successes || forced )
	{
		if( l.successtime )
		{
			uint32_t del = GetTickCount() - l.successtime;
			lprintf( "Succeses: %ld (for %02d:%02d:%02d.%03d)", l.successes
					 , (del / (1000*60*60))
					 , (del / (1000*60))%60
					 , (del / (1000))%60
					 , (del % 1000)
					 );
		}
		else
			lprintf( "Succeses: %ld (for ever)", l.successes );

      if( !forced )
			l.successes = 0;
	}
	if( l.failures || forced )
	{
		if( l.failtime )
		{
			uint32_t del = GetTickCount() - l.failtime;
			lprintf( "Failures: %ld (for %02d:%02d:%02d.%03d)", l.failures
					 , (del / (1000*60*60))
					 , (del / (1000*60))%60
					 , (del / (1000))%60
					 , (del % 1000)
					 );
		}
      else
			lprintf( "Failures: %ld (for ever)", l.failures );

		if( !forced )
			l.failures = 0;
	}
}


ATEXIT( FinalLog )
{
   DoLogNow( TRUE );
}

static void CPROC LogNow( void )
{
   DoLogNow( TRUE );
}

static void Result( uint32_t dwIP, CTEXTSTR name, int min, int max, int avg, int drop, int hops );

static void Count( int drop )
{
	if( drop )
	{
		if( l.successes )
		{
			DoLogNow( FALSE );
         GetLocalTime( &l.not_ok_time );
			l.failtime = GetTickCount();
		}
		else
		{
			if( l.failures > 10 )
			{
				static int step;
				if( l.failures == 21 )
					step = 0;

				SetBannerText(l.banner
								, step==0?"Network is really bad...."
								 : step==1?"Still waiting..."
								 : step==2?"Still waiting....\nErr..."
								 : step==3?"Still waiting....."
								 : step==4?"Still waiting......"
								 : step==5?"Still waiting.......\nPlease Reconnect..."
								 : "Still waiting........"
								);
            step++;
				if( step > 6 )
               step = 0;
				sleep_time = 5000;
			}
			else if( l.failures > 5 )
			{
				SetBannerText(l.banner
								, "Network is still failing..." );

				sleep_time = 1000;
			}
		}
		l.failures++;

		if( l.failures == 1 )
		{
			lprintf( "Failing!" );
			{
				CreateBannerEx( NULL, &l.banner
								  , "Network Has Failed..."
								  , BANNER_ABSOLUTE_TOP|BANNER_DEAD|BANNER_NOWAIT
								  , 0 );
			}
         //lprintf( "First drop, retry..." );
			sleep_time = 500;
			WakeThread( Monitor );
		}
	}
	else
	{
		if( l.failures )
		{
			DoLogNow( FALSE );
         GetLocalTime( &l.ok_time );
         l.successtime = GetTickCount();
			sleep_time = 1000;
			bSetTime = 1;
			WakeThread( Monitor );
		}
		else
		{
			if( l.successes > 5 )
            sleep_time = 10000;
		}
		l.successes++;
		if( l.successes == 1 )
		{
         RemoveBanner( l.banner );
			lprintf( "Succeeding!" );
		}
	}
   //lprintf( "Result is %d %d %d %d %d", min, max, avg, drop, hops );
}

static uintptr_t CPROC NetworkMonitor( PTHREAD thread )
{
   Monitor = thread;
   AddSystrayMenuFunction( "Force Log Now", LogNow );
	while( 1 )
	{
		if( !bSetTime )
		{
			//DoPing( l.address, 0, 100, 1, NULL, FALSE, Result );
         // result is number of successes. (non drops)
			Count( 1-MyDoPing( ) );
		}
		bSetTime = 0;
		WakeableSleep( sleep_time );
	}
}


PRELOAD( network_monitor_startup )
{
	NetworkStart();
   sleep_time = 10000;
	SACK_GetPrivateProfileString( "Network Safety Monitor", "Monitor Address", "172.17.2.201", l.address, sizeof( l.address ), GetProgramName() );
   InitPing( l.address );

	SetSystemLog( SYSLOG_AUTO_FILE, NULL );
   SetSystemLoggingLevel( 10000 );
	SystemLogTime( SYSLOG_TIME_ENABLE|SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
	{
		FLAGSET( options, SYSLOG_OPT_MAX );
		int n;
		for( n = 0; n < SYSLOG_OPT_MAX; n++ )
		{
         RESETFLAG( options, n );
		}
      SETFLAG( options, SYSLOG_OPT_OPEN_BACKUP );
		SetSyslogOptions( options );
	}
   lprintf( "succeses is spelled wrong.  This forces it to align with 'failures'" );
	lprintf( "... the last line is NOT the current... " );
	lprintf( "if it says 'failure' it's probably succeeding..." );
	lprintf( "if it says 'success' it's probably failing...(right click on icon to update)" );
   ThreadTo( NetworkMonitor, 0 );
}



