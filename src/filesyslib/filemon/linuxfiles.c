//#include <windows.h>
#include <stdhdrs.h>

#ifdef __LINUX__
// fcntl, open
//#include <unistd.h>
//#define _ASM_GENERIC_FCNTL_H
//#define HAVE_ARCH_STRUCT_FLOCK
//#undef __USE_GNU
//#include <linux/fcntl.h>
#include <fcntl.h>
#include <sys/inotify.h>
#endif

#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __cplusplus
//#include <unistd.h>
#endif
#include <signal.h>
#include <dirent.h>


#include <string.h>
#include <stdio.h>

#include <network.h>
#include <timers.h>
#include <filesys.h>

#include <sack_types.h>
#include <sharemem.h>
#define DO_LOGGING
#include <logging.h>


#define INVALID_HANDLE_VALUE -1
#define SCAN_DELAY 1500

#include "monitor.h"

FILEMON_NAMESPACE

//-------------------------------------------------------------------------

void ScanDirectory( PMONITOR monitor )
{
    DIR *dir;
    dir = opendir( monitor->directory );
    if( dir )
    {
        struct dirent *dirent;
        while( ( dirent = readdir( dir ) ) )
        {
				PCHANGECALLBACK Change;
				if( monitor->flags.bLogFilesFound )
					Log1( WIDE("Found file %s"), dirent->d_name );
				for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
				{
					if( !Change->mask ||
                   CompareMask( Change->mask
										, dirent->d_name
										, FALSE ) )
						if( monitor->flags.bLogFilesFound )
                     Log( WIDE("And the mask matched.") );
                  AddMonitoredFile( Change, dirent->d_name );
				}
        }
        closedir( dir );
    }
    return;
}

//-------------------------------------------------------------------------

static int Monitoring;

// perhaps these two shall be grouped?
// then I can watch multiple directories and maintain
// their names...
PMONITOR Monitors;

//-------------------------------------------------------------------------

PMONITOR CreateMonitor( int fdMon, char *directory )
{
    PMONITOR monitor = (PMONITOR)Allocate( sizeof( MONITOR ) );
    MemSet( monitor, 0, sizeof( MONITOR ) );
    monitor->fdMon = fdMon;
	 strcpy( monitor->directory, directory );
 //   strcpy( monitor->mask, mask );
    monitor->next = Monitors;
    if( Monitors )
        Monitors->me = &monitor->next;
    monitor->me = &Monitors;
    Monitors = monitor;
    return monitor;
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( void, EndMonitor )( PMONITOR monitor )
{
	if( monitor->flags.bDispatched )
	{
		monitor->flags.bEnd = 1;
		return;
	}
	if( monitor->flags.bClosing )
	{
		//Log( WIDE("Monitor already closing...") );
		return;
	}
	EnterCriticalSec( &monitor->cs );
	if( monitor )
	{
		if( monitor->timer )
			RemoveTimer( monitor->timer );
		// clear signals on this handle?
	// wouldn't just closing it be enough!?!?!
	// if I include <fcntl.h> then <linux/fcntl.h> is broken
	// or vice-versa.  Please suffer with no definition on these lines.
#if 0
		fcntl( monitor->fdMon, F_SETSIG, 0 );
		fcntl( monitor->fdMon, F_NOTIFY, 0 );
#endif
		Log1( WIDE("Closing monitor handle: %d"), monitor->fdMon );
		close( monitor->fdMon );
		UnlinkThing( monitor );
		Release( monitor );
		if( !Monitors )
		{
			signal( SIGRTMIN, SIG_IGN );
		}
	}
}

//-------------------------------------------------------------------------

static void handler( int sig, siginfo_t *si, void *data )
{
    if( si )
    {
        PMONITOR cur = Monitors;
        for( cur = Monitors; cur; cur = cur->next )
        {
            if( si->si_fd == cur->fdMon )
            {
                Log1( WIDE("Setting to scan due to event.. %s"), cur->directory );
					 if( !cur->DoScanTime )
						 cur->DoScanTime = GetTickCount() - 1;
                break;
            }
        }
        if( !cur )
        {
           Log1( WIDE("Signal on handle which did not exist?! %d Invoking failure scanall!"), si->si_fd );
           close( si->si_fd );
	        for( cur = Monitors; cur; cur = cur->next )
   	     {
				  if( !cur->DoScanTime )
					  cur->DoScanTime = GetTickCount() - 1;
           }
        }
    }
}

void CPROC NewScanTimer( PTRSZVAL unused )
{
	
        PMONITOR cur = Monitors;
        for( cur = Monitors; cur; cur = cur->next )
        {
		static _8 buf[4096];
		struct inotify_event *event;
		int len;	

		while( ( len = read( cur->fdMon, &buf, sizeof( buf ) ) ) > 0 )
		{
			event = (struct inotify_event*)buf;

                        //event->wd;
			//event->name;
			 if( !cur->DoScanTime )
				 cur->DoScanTime = GetTickCount() - 1;
		}
        }
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PMONITOR, MonitorFilesEx )( CTEXTSTR dirname, int scan_delay, int flags )
{
	if( !Monitors )
	{
#if 0
		struct sigaction act;
		act.sa_sigaction = handler;
		sigemptyset( &act.sa_mask );
		act.sa_flags = SA_SIGINFO;
		if( sigaction( SIGRTMIN, &act, NULL ) != 0 )
		{
			Log( WIDE("Failed to set signal handler") );
			return NULL;
		}
#endif
	}

	{
		int fdMon;
		Log2( WIDE("Attempting to monitor:(%d) %s"), getpid(), dirname );
	// if I include <fcntl.h> then <linux/fcntl.h> is broken
	// or vice-versa.  Please suffer with no definition on these lines.
#if 0
		fdMon = open( dirname, O_RDONLY );
#else
		fdMon = inotify_init();
		fcntl(fdMon, F_SETFL, O_NONBLOCK); 
#endif
		if( fdMon >= 0 )
		{
			PMONITOR monitor;
#if 0
			fcntl( fdMon, F_SETSIG, SIGRTMIN );
			fcntl( fdMon, F_NOTIFY, DN_CREATE|DN_DELETE|DN_RENAME|DN_MODIFY|DN_MULTISHOT );
#else
			inotify_add_watch( fdMon, dirname, IN_ALL_EVENTS );
#endif
			monitor = CreateMonitor( fdMon, (char*)dirname );
			Monitoring = 1;
			//monitor->Client = Client;
			monitor->DoScanTime = GetTickCount() - 1; // do first scan NOW
			monitor->timer = AddTimerEx( 0, SCAN_DELAY/2, (void(*)(PTRSZVAL))ScanTimer, (PTRSZVAL)monitor );
			Log2( WIDE("Created timer: %") _32f WIDE(" Monitor handle: %d"), monitor->timer, monitor->fdMon );
			return monitor;
		}
		else
			Log( WIDE("Failed to open directory to monitor") );
	}
	return NULL;
}

FILEMONITOR_PROC( PMONITOR, MonitorFiles )( CTEXTSTR dirname, int scan_delay )
{
    return MonitorFilesEx( dirname, scan_delay, 0 );
}

FILEMON_NAMESPACE_END

