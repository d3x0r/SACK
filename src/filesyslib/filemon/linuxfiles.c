
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE  199309L
#endif
//#include <windows.h>
#include <stdhdrs.h>

// fcntl, open
//#include <unistd.h>
//#define _ASM_GENERIC_FCNTL_H
//#define HAVE_ARCH_STRUCT_FLOCK
//#undef __USE_GNU
//#include <linux/fcntl.h>
#include <fcntl.h>

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

#ifdef __LINUX__

#ifndef __QNX__
#include <sys/inotify.h>
#endif

#define INVALID_HANDLE_VALUE -1
#define SCAN_DELAY 1500

#include "monitor.h"

FILEMON_NAMESPACE

//-------------------------------------------------------------------------

void ScanDirectory( PMONITOR monitor, PCHANGECALLBACK Change )
{
    DIR *dir;
    dir = opendir( monitor->directory );
    if( dir )
    {
		 struct dirent *dirent;
		 if( monitor->flags.bLogFilesFound )
			 lprintf( "DIR OPEN" );
		 while( ( dirent = readdir( dir ) ) )
		 {
				if( monitor->flags.bLogFilesFound )
					Log1( "Found file %s", dirent->d_name );
				{
					if( !Change->mask ||
					    CompareMask( Change->mask
										, dirent->d_name
										, FALSE ) ) {
						if( monitor->flags.bLogFilesFound )
							lprintf( "And the mask matched.", Change->mask );
						AddMonitoredFile( Change, dirent->d_name );
					}
				}
        }
        closedir( dir );
	 } else
		 lprintf( "Fatality; failed to open directory %d %s", errno, monitor->directory );
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
    monitor->flags.bLogFilesFound  = 0;
    monitor->flags.bIntelligentUpdates = 1;
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
		//Log( "Monitor already closing..." );
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
		Log1( "Closing monitor handle: %d", monitor->fdMon );
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
#ifndef __QNX__
			  if( si->si_fd == cur->fdMon )
#endif
            {
                Log1( "Setting to scan due to event.. %s", cur->directory );
					 if( !cur->DoScanTime )
						 cur->DoScanTime = GetTickCount() - 1;
                break;
            }
        }
        if( !cur )
        {
#ifndef __QNX__
           Log1( "Signal on handle which did not exist?! %d Invoking failure scanall!", si->si_fd );
			  close( si->si_fd );
#endif
		for( cur = Monitors; cur; cur = cur->next )
   	     {
				  if( !cur->DoScanTime )
					  cur->DoScanTime = GetTickCount() - 1;
           }
        }
    }
}

void CPROC NewScanTimer( uintptr_t unused )
{

	PMONITOR monitor = (PMONITOR)unused;//  Monitors;
					 //for( cur = Monitors; cur; cur = cur->next )
   while( 1 )
	{
		static uint8_t buf[4096];
		struct inotify_event *event;
		int len;
      int used;
      //lprintf( "new scan timer." );
		while( ( len = read( monitor->fdMon, &buf, sizeof( buf ) ) ) > 0 )
		{
			PFILEMON filemon;
			for( used = 0; used < len; used += sizeof( struct inotify_event ) + event->len ) {
				PCHANGEHANDLER Change;
				event = (struct inotify_event*)buf;
				//lprintf( "Events: %d %d %08x (%d)%s", len, used, event->mask, event->len, event->name );

				if( event->len ) {


					for( Change = monitor->ChangeHandlers; Change; Change = Change->next ) {
						filemon = (PFILEMON)FindInBinaryTree( Change->filelist, (uintptr_t)event->name );
						if( filemon )
							break;
						if( event->mask & IN_CREATE ) {
							if( !Change->mask ||
							    CompareMask( Change->mask
										, event->name
										, FALSE ) ) {
								filemon = AddMonitoredFile( Change, event->name );
								break;
							}
						} else {
						}
					}


				}
				if( filemon ) {
					int updated = 1;
					if( event->mask & IN_CREATE ) {
						//lprintf( "Creating event, adding file" );
						//AddMonitoredFile( Change, event->name );
						//continue;
					}
					if( event->mask & IN_ACCESS )
						updated = 0;;
					if( event->mask & IN_ATTRIB ){
					        updated = 1;
					}
					if( event->mask & IN_CLOSE_WRITE );
					if( event->mask & IN_CLOSE_NOWRITE )
						updated = 0;
					if( event->mask & IN_DELETE ) {
						//lprintf( "Deletion event" );
						filemon->flags.bToDelete = 1;
					}
					if( event->mask & IN_DELETE_SELF );
					if( event->mask & IN_MODIFY );
					if( event->mask & IN_MOVE_SELF );
					if( event->mask & IN_MOVED_FROM );
					if( event->mask & IN_MOVED_TO );
					if( event->mask & IN_OPEN ) {
						updated = 0;
					}

														 //event->wd;
														 //event->name;
					if( updated ) {
						if( !filemon->flags.bPending ) {
                     filemon->flags.bPending = TRUE;
							EnqueLink( &Change->PendingChanges, filemon );
						}
						if( !monitor->DoScanTime ) {
							monitor->DoScanTime =
								filemon->ScannedAt = timeGetTime() + monitor->scan_delay;
						}
						if( monitor->parent_monitor )
							monitor->parent_monitor->DoScanTime = monitor->DoScanTime;
                  //lprintf( "Set pending scan %p", monitor );
						monitor->flags.bPendingScan = 1;
					}
					filemon->flags.bScanned = 0;
				}
			}
		}
	}
}


uintptr_t threadScanTimer( PTHREAD thread ) {
	NewScanTimer( GetThreadParam( thread ) );
   return 0;
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
			Log( "Failed to set signal handler" );
			return NULL;
		}
#endif
	}

	{
		int fdMon;
		//Log2( "Attempting to monitor:(%d) %s", getpid(), dirname );
	// if I include <fcntl.h> then <linux/fcntl.h> is broken
	// or vice-versa.  Please suffer with no definition on these lines.
#if 0
		fdMon = open( dirname, O_RDONLY );
#else
#ifndef __QNX__
		fdMon = inotify_init();
#endif
		//fcntl(fdMon, F_SETFL, O_NONBLOCK);
#endif
		if( fdMon >= 0 )
		{
			PMONITOR monitor;
#if 0
			fcntl( fdMon, F_SETSIG, SIGRTMIN );
			fcntl( fdMon, F_NOTIFY, DN_CREATE|DN_DELETE|DN_RENAME|DN_MODIFY|DN_MULTISHOT );
#else
#ifndef __QNX__
         // this should work for QNX (qnx car?)
			inotify_add_watch( fdMon, dirname, IN_ALL_EVENTS & ~(IN_OPEN|IN_CLOSE_NOWRITE) );
#endif
#endif
			monitor = CreateMonitor( fdMon, (char*)dirname );
			Monitoring = 1;
			//monitor->Client = Client;
			monitor->DoScanTime = GetTickCount() - 1; // do first scan NOW
         ThreadTo( threadScanTimer, (uintptr_t)monitor );
			monitor->timer = AddTimerEx( 0, SCAN_DELAY/2, (void(*)(uintptr_t))ScanTimer, (uintptr_t)monitor );
			//Log2( "Created timer: %" _32f " Monitor handle: %d", monitor->timer, monitor->fdMon );
			return monitor;
		}
		else
			Log( "Failed to open directory to monitor" );
	}
	return NULL;
}

FILEMONITOR_PROC( PMONITOR, MonitorFiles )( CTEXTSTR dirname, int scan_delay )
{
    return MonitorFilesEx( dirname, scan_delay, 0 );
}

FILEMON_NAMESPACE_END

#endif
