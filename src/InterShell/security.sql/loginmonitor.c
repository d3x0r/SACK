///////////////////////////////////////////////////////////////////////////
//
//   login_monitor.c
//   (C) Copyright 2009 
//   Crafted by d3x0r
//                   
////////////////////////////////////////////////////////////////////////////

// process api
// make sure it links against psapi, and not kernel32 (windows7++)
#define PSAPI_VERSION 1

#define DO_LOGGING
#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <service_hook.h>
#include <logging.h>
#include <idle.h>
#include "../widgets/include/banner.h"
#include "../intershell_registry.h"
#include "global.h"
#include <Psapi.h>

//--------------------------------------------------------------------------------
// Local Struct
static struct local_login_mon 
{
	// Hook Handles
	HHOOK gMouseHook;
	HHOOK gKeyboardHook;

	// Process info
	_32 pidList1[1024];
	_32 pidListLength1;
	_32 pidList2[1024];
	_32 pidListLength2;

	// Timer data
	_32 iTimerHandle;

	// Database settings	
	_32 iIdleTime;	
	_32 iScreenSaverWaitTime;
	_32 last_event_time;

	// Flags
	struct local_login_mon_flags 
	{
		// INI Option Flags
		unsigned bLog          : 1; // Set to 1 to turn on logging
		unsigned bLogSleeps    : 1; // Set to 1 to turn on logging

		// Flags
		unsigned countingDown : 1;			
	} flags;

	TEXTCHAR option_dsn[64];


} l;

//----------------------------------------------------------------------------------------
// Callback for Mouse Event
static LRESULT CALLBACK LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	if ( nCode >= 0 )
	{
		PMSLLHOOKSTRUCT pmll = (PMSLLHOOKSTRUCT)lParam;

		//lprintf( WIDE(" Mouse Activity Hook: %lu, x:%ld, y:%ld"), wParam, pmll->pt.x, pmll->pt.y );		 

		l.flags.countingDown = 0;
		l.last_event_time = timeGetTime();
	}	

	return CallNextHookEx( l.gMouseHook, nCode, wParam, lParam );
}

//----------------------------------------------------------------------------------------
// Callback for Keyboard Event
static LRESULT CALLBACK LowLevelKeyboardProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	if ( nCode >= 0 )
	{
		PMSLLHOOKSTRUCT pmll = (PMSLLHOOKSTRUCT)lParam;

		//lprintf( WIDE(" Keyboard Activity Hook: %lu, x:%ld, y:%ld"), wParam, pmll->pt.x, pmll->pt.y );		

		l.flags.countingDown = 0;
		l.last_event_time = timeGetTime();
	}	

	return CallNextHookEx( l.gKeyboardHook, nCode, wParam, lParam );
}

//----------------------------------------------------------------------------------------
// Hook Thread
static PTRSZVAL CPROC HandleHook( PTHREAD thread )
{	
	MSG msg;

	l.gMouseHook = SetWindowsHookEx( WH_MOUSE_LL, (HOOKPROC)LowLevelMouseProc, GetModuleHandle( _WIDE(TARGETNAME) ), 0 );
	l.gKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, GetModuleHandle( _WIDE(TARGETNAME) ), 0 );

	while( GetMessage( &msg, NULL, 0, 0 ) )
		DispatchMessage( &msg );

	return 0;
}


//----------------------------------------------------------------------------------------
// Output handler for system call
void CPROC LogOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( WIDE("%s"), buffer );
}

static LOGICAL HaveCurrentUser( PODBC odbc )
{
	CTEXTSTR *results;

	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE("select count(*) from login_history")
	                                             WIDE(" where logout_whenstamp=11111111111111")
	                                             WIDE(" and system_id=%d")
						     , g.system_id );

	if( results )
	{		
		if( results[0] )
		{			
			if( atoi( results[0] ) > 0 )
			{				
				SQLEndQuery( odbc );
				return TRUE;
			}
		}
	}
	SQLEndQuery( odbc );
	return FALSE;
}

//----------------------------------------------------------------------------------------
// Timer for checking the timestamp
static PTRSZVAL CPROC countDown( PTHREAD thread )
{
	TEXTCHAR buf[45];
	_8 found = 0;	
	_32 startTime;   
	PODBC odbc;
	
	// Loop forever
	while( 1 )
	{
		// Get Current time
		startTime = timeGetTime();

		// Sleep to next timeout
		if( l.flags.bLogSleeps )
		{
			long x =  ( l.iIdleTime*(60)*(1000) ) - ( startTime - l.last_event_time );
			lprintf( WIDE("Sleep for... %d:%02d.%03d"), x / 60000, (x / 1000 ) %60, x % 1000 );
		}

		if( ( l.iIdleTime*(60)*(1000) ) > ( startTime - l.last_event_time ) )
			WakeableSleep( ( l.iIdleTime*(60)*(1000) ) - ( startTime - l.last_event_time ) );

		startTime = timeGetTime();
		// sleep some more if we got a new event

		if( ( l.iIdleTime*(60)*(1000) ) > ( startTime - l.last_event_time ) )
			continue;

		// Make connection to database
		odbc = SQLGetODBC( l.option_dsn );

		if( l.flags.bLogSleeps )
			lprintf( WIDE("woke up and user is %p (%d)"), HaveCurrentUser( odbc ), ( startTime - l.last_event_time ) );
		// If there is current user, else there is no active logins or processes to kill

		if( HaveCurrentUser( odbc ) )
		{
			_32 target_tick = startTime + ( l.iScreenSaverWaitTime * 1000 );
			S_32 remaining = ( target_tick - startTime );
			// Get time

			// Check last event, if timeout not hit go back to sleep until timeout will be hit
			if( remaining < 0 )
			{
				if( l.flags.bLogSleeps )
					lprintf( WIDE("Not enough time, go back and wait...") );

				// Close Connection to database
				SQLDropODBC( odbc );

				// not enough time has really passed... sleep more
				continue;
			}

			// Let hook know to update activity flag
			l.flags.countingDown = 1;			

			// Start countdown
			do
			{
				// Update time to check against
				remaining = ( target_tick - startTime );

				// Update Banner Message
				snprintf( buf, sizeof( buf ), WIDE(" The system will logout in %d.%03d seconds...")
						  , remaining / 1000, remaining % 1000 );
				BannerTopNoWait( buf );

				// Don't eat to much cpu time
				IdleFor( 200 );
				startTime = timeGetTime();
			}
			while( l.flags.countingDown && ( target_tick > startTime ) );

			// If countdown made it to 0
			if( ( target_tick < startTime ) )
			{
				_32 x;
				_32 y;
				// Logout users
				SQLCommandf( odbc , WIDE("update login_history set logout_whenstamp=now() where logout_whenstamp=11111111111111 and system_id=%d"), g.system_id );

				// Get current process list
				if ( !EnumProcesses( l.pidList2, sizeof( l.pidList2 ), &l.pidListLength2 ) )					
						lprintf( WIDE(" Getting Process List 2 Failed!") );

				// Kill Processes
				for( x = 0; x < l.pidListLength2; x++ )
				{
					// Check if process existed originally
					for( y = 0; y < l.pidListLength1; y++ )
					{
						if( l.pidList2[x] == l.pidList1[y] )
						{
							found = 1;
							break;
						}
					}

					// If did not exist originally kill the process
					if( !found )
					{
						TEXTCHAR cmd[25];
						snprintf( cmd, sizeof( cmd ), WIDE("pskill %d"), l.pidList2[x] );
						System( cmd, LogOutput, 0 );
					}

					found = 0;
				}

				// Give extra time to see Banner
				IdleFor( 500 );

				// Reset Data				
				l.flags.countingDown  = 0;				
			}			

			// Remove Banner
			RemoveBanner2Ex( NULL DBG_SRC  );
		}
		else
		{
			// fake an event, there was no user, so didn't do anything, just sleep more
			l.last_event_time = startTime;
		}

		// Close Connection to database
		SQLDropODBC( odbc );
	}

	return 0;
}
 
static void OnTaskLaunchComplete( WIDE("Login Monitor") )( void )
{
	// RE-Get Process List
	if ( !EnumProcesses( l.pidList1, sizeof( l.pidList1 ), &l.pidListLength1 ) )
		lprintf( WIDE(" Getting Process List 1 Failed(2)!") );
}

//----------------------------------------------------------------------------------------
// Pre-Main Event, Initialize variables
PRELOAD( InitLoginMon )
{
	if( g.flags.bInitializeLogins )
	{
		PODBC odbc;

		// Check if logging is to be turned on
		l.flags.bLog = SACK_GetPrivateProfileInt( GetProgramName(), WIDE( "SECURITY/Do Logging" ), 0, NULL );
		l.flags.bLogSleeps = SACK_GetPrivateProfileInt( GetProgramName(), WIDE( "SECURITY/SQL Password/Do Logging (Sleeps)" ), 0, NULL );
		lprintf( WIDE(" Inactivity Service has been started.") );

		// Set up for pulling options
		SACK_GetProfileString( WIDE("SECURITY/SQL Passwords"), WIDE( "password DSN" ), GetDefaultOptionDatabaseDSN(), l.option_dsn, sizeof( l.option_dsn ) );
		odbc = GetOptionODBC( l.option_dsn, 0 );

		// Get Idle Time ( Minutes )
		l.iIdleTime = SACK_GetPrivateOptionInt( odbc, WIDE("SECURITY/Login/Timeouts"), WIDE("idle"), 30, NULL );
		lprintf( WIDE(" Idle Time: %d minutes"), l.iIdleTime );

		// If option enabled
		if( l.iIdleTime )
		{
			// Get Screen Saver Wait Time ( Seconds )
			l.iScreenSaverWaitTime = SACK_GetPrivateOptionInt( odbc, WIDE("SECURITY/Login/Timeouts"), WIDE("message time"), 10, NULL );
			lprintf( WIDE(" Screen Saver Wait Time: %d seconds"), l.iScreenSaverWaitTime );

			// Get Process List
			if ( !EnumProcesses( l.pidList1, sizeof( l.pidList1 ), &l.pidListLength1 ) )
				lprintf( WIDE(" Getting Process List 1 Failed!") );

			// Setup Timer Thread
			l.last_event_time = timeGetTime();
			ThreadTo( countDown, 0 );

			// Setup Hook Thread
			ThreadTo( HandleHook, 0 );
		}
		DropOptionODBC( odbc );
	}
}


//----------------------------------------------------------------------------------------
// Post-Main Event
ATEXIT( ExitLoginMon )
{
	if( g.flags.bInitializeLogins )
	{
		// Log Service Stopped
		lprintf( WIDE(" Inactivity Service has stopped.") );

		// Unhook hooks
		UnhookWindowsHookEx( l.gMouseHook );
		UnhookWindowsHookEx( l.gKeyboardHook );
	}
}

