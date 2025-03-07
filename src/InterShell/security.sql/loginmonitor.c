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
	uint32_t pidList1[1024];
	uint32_t pidListLength1;
	uint32_t pidList2[1024];
	uint32_t pidListLength2;

	// Timer data
	uint32_t iTimerHandle;

	// Database settings	
	uint32_t iIdleTime;	
	uint32_t iScreenSaverWaitTime;
	uint32_t last_event_time;

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

		//lprintf( " Mouse Activity Hook: %lu, x:%ld, y:%ld", wParam, pmll->pt.x, pmll->pt.y );		 

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

		//lprintf( " Keyboard Activity Hook: %lu, x:%ld, y:%ld", wParam, pmll->pt.x, pmll->pt.y );		

		l.flags.countingDown = 0;
		l.last_event_time = timeGetTime();
	}	

	return CallNextHookEx( l.gKeyboardHook, nCode, wParam, lParam );
}

//----------------------------------------------------------------------------------------
// Hook Thread
static uintptr_t CPROC HandleHook( PTHREAD thread )
{	
	MSG msg;

	l.gMouseHook = SetWindowsHookEx( WH_MOUSE_LL, (HOOKPROC)LowLevelMouseProc, GetModuleHandle( TARGETNAME ), 0 );
	l.gKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, GetModuleHandle( TARGETNAME ), 0 );

	while( GetMessage( &msg, NULL, 0, 0 ) )
		DispatchMessage( &msg );

	return 0;
}


//----------------------------------------------------------------------------------------
// Output handler for system call
void CPROC LogOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( "%s", buffer );
}

static LOGICAL HaveCurrentUser( PODBC odbc )
{
	CTEXTSTR *results;

	SQLRecordQueryf( odbc, NULL, &results, NULL, "select count(*) from login_history"
	                                             " where logout_whenstamp=11111111111111"
	                                             " and system_id=%d"
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
static uintptr_t CPROC countDown( PTHREAD thread )
{
	TEXTCHAR buf[45];
	uint8_t found = 0;	
	uint32_t startTime;   
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
			lprintf( "Sleep for... %d:%02d.%03d", x / 60000, (x / 1000 ) %60, x % 1000 );
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
			lprintf( "woke up and user is %p (%d)", HaveCurrentUser( odbc ), ( startTime - l.last_event_time ) );
		// If there is current user, else there is no active logins or processes to kill

		if( HaveCurrentUser( odbc ) )
		{
			uint32_t target_tick = startTime + ( l.iScreenSaverWaitTime * 1000 );
			int32_t remaining = ( target_tick - startTime );
			// Get time

			// Check last event, if timeout not hit go back to sleep until timeout will be hit
			if( remaining < 0 )
			{
				if( l.flags.bLogSleeps )
					lprintf( "Not enough time, go back and wait..." );

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
				snprintf( buf, sizeof( buf ), " The system will logout in %d.%03d seconds..."
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
				uint32_t x;
				uint32_t y;
				// Logout users
				SQLCommandf( odbc , "update login_history set logout_whenstamp=now() where logout_whenstamp=11111111111111 and system_id=%d", g.system_id );

				// Get current process list
				if ( !EnumProcesses( l.pidList2, sizeof( l.pidList2 ), &l.pidListLength2 ) )					
						lprintf( " Getting Process List 2 Failed!" );

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
						snprintf( cmd, sizeof( cmd ), "pskill %d", l.pidList2[x] );
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
 
static void OnTaskLaunchComplete( "Login Monitor" )( void )
{
	// RE-Get Process List
	if ( !EnumProcesses( l.pidList1, sizeof( l.pidList1 ), &l.pidListLength1 ) )
		lprintf( " Getting Process List 1 Failed(2)!" );
}

//----------------------------------------------------------------------------------------
// Pre-Main Event, Initialize variables
PRELOAD( InitLoginMon )
{
	if( g.flags.bInitializeLogins )
	{
		PODBC odbc;

		// Check if logging is to be turned on
		l.flags.bLog = SACK_GetPrivateProfileInt( GetProgramName(), "SECURITY/Do Logging", 0, NULL );
		l.flags.bLogSleeps = SACK_GetPrivateProfileInt( GetProgramName(), "SECURITY/SQL Password/Do Logging (Sleeps)", 0, NULL );
		lprintf( " Inactivity Service has been started." );

		// Set up for pulling options
		SACK_GetProfileString( "SECURITY/SQL Passwords", "password DSN", GetDefaultOptionDatabaseDSN(), l.option_dsn, sizeof( l.option_dsn ) );
		odbc = GetOptionODBC( l.option_dsn, 0 );

		// Get Idle Time ( Minutes )
		l.iIdleTime = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Timeouts", "idle", 30, NULL );
		//lprintf( " Idle Time: %d minutes", l.iIdleTime );

		// If option enabled
		if( l.iIdleTime )
		{
			// Get Screen Saver Wait Time ( Seconds )
			l.iScreenSaverWaitTime = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Timeouts", "message time", 10, NULL );
			//lprintf( " Screen Saver Wait Time: %d seconds", l.iScreenSaverWaitTime );

			// Get Process List
			if ( !EnumProcesses( l.pidList1, sizeof( l.pidList1 ), &l.pidListLength1 ) )
				lprintf( " Getting Process List 1 Failed!" );

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
		lprintf( " Inactivity Service has stopped." );

		// Unhook hooks
		UnhookWindowsHookEx( l.gMouseHook );
		UnhookWindowsHookEx( l.gKeyboardHook );
	}
}

