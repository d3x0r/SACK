
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE  199309L
#endif
#include <stdhdrs.h>

#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>

#include <network.h>
#include <timers.h>
#include <filesys.h>

#include <sack_types.h>
#include <sharemem.h>
#define DO_LOGGING
#include <logging.h>

#ifdef __MAC__

// FSEvents lives in CoreServices; dispatch is used to deliver the
// stream callbacks onto a serial queue (no CFRunLoop thread needed).
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>

#define INVALID_HANDLE_VALUE -1
#define SCAN_DELAY 1500

#include "monitor.h"

FILEMON_NAMESPACE

//-------------------------------------------------------------------------

// Identical directory enumeration to the linux/windows back ends; used for
// the initial scan when a change callback is added, and for full re-scans
// when intelligent (event driven) updates are disabled.
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
			if( !Change->mask ||
			    CompareMask( Change->mask
			               , dirent->d_name
			               , FALSE ) ) {
				if( monitor->flags.bLogFilesFound )
					lprintf( "And the mask matched." );
				AddMonitoredFile( Change, dirent->d_name );
			}
		}
		closedir( dir );
	} else
		lprintf( "Fatality; failed to open directory %d %s", errno, monitor->directory );
	return;
}

//-------------------------------------------------------------------------

// list of all active monitors (shared name used by allfiles.c)
PMONITOR Monitors;

// every FSEventStream is serviced from this one serial queue; created lazily
// on the first monitor and left alive for the life of the process.
static dispatch_queue_t fsevents_queue;

//-------------------------------------------------------------------------

// FSEvents reports canonical absolute paths.  The rest of the file monitor
// keys everything off the simple name within the watched directory, so reduce
// the reported path back to a name relative to monitor->directory.  When the
// path doesn't sit under the (possibly symlink-resolved) directory, fall back
// to the final path component, which is what the non-recursive watch sees.
static CTEXTSTR RelativeName( PMONITOR monitor, const char *path )
{
	size_t dirlen = strlen( monitor->directory );
	const char *slash;
	while( dirlen && monitor->directory[dirlen - 1] == '/' )
		dirlen--;
	if( dirlen
	    && strncmp( path, monitor->directory, dirlen ) == 0
	    && path[dirlen] == '/' )
		return (CTEXTSTR)( path + dirlen + 1 );
	slash = strrchr( path, '/' );
	return (CTEXTSTR)( slash ? slash + 1 : path );
}

//-------------------------------------------------------------------------

// Called on fsevents_queue whenever the watched directory changes.  We don't
// dispatch user callbacks here (that only happens on the timer thread); we just
// reconcile the file list and flag the monitor so the next ScanTimer tick will
// pick up sizes/times and announce the changes - mirroring the windows back end.
static void FileMonStreamCallback( ConstFSEventStreamRef streamRef
                                 , void *clientCallBackInfo
                                 , size_t numEvents
                                 , void *eventPaths
                                 , const FSEventStreamEventFlags eventFlags[]
                                 , const FSEventStreamEventId eventIds[] )
{
	PMONITOR monitor = (PMONITOR)clientCallBackInfo;
	char **paths = (char**)eventPaths;
	size_t n;

	(void)streamRef;
	(void)eventFlags;
	(void)eventIds;

	if( monitor->flags.bClosing )
		return;

	EnterCriticalSec( &monitor->cs );
	for( n = 0; n < numEvents; n++ )
	{
		CTEXTSTR name = RelativeName( monitor, paths[n] );
		// FSEvents can coalesce create/modify/remove into a single event, so
		// trust the filesystem: if the entry still exists it's a create/modify,
		// otherwise it's a remove/rename-away.
		int exists = ( access( paths[n], F_OK ) == 0 );
		PCHANGECALLBACK Change;

		if( monitor->flags.bLogFilesFound )
			lprintf( "FSEvent on %s (%s)", paths[n], exists ? "present" : "gone" );

		for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
		{
			if( Change->mask &&
			    !CompareMask( Change->mask, name, FALSE ) )
				continue;

			if( exists )
			{
				// new or modified - AddMonitoredFile adds it if unknown and
				// clears bScanned so ScanFile picks up the size/time change.
				AddMonitoredFile( Change, name );
			}
			else
			{
				PFILEMON filemon = WatchingFile( Change, name );
				// (PFILEMON)1 == directory, (PFILEMON)2 == "."/".." - ignore both.
				if( filemon
				    && filemon != (PFILEMON)1
				    && filemon != (PFILEMON)2 )
				{
					filemon->flags.bToDelete = 1;
					filemon->ScannedAt = timeGetTime() + monitor->scan_delay;
					if( !filemon->flags.bPending )
					{
						filemon->flags.bPending = 1;
						EnqueLink( &Change->PendingChanges, filemon );
					}
				}
			}
		}
	}

	if( !monitor->DoScanTime )
		monitor->DoScanTime = timeGetTime() - 1;
	monitor->flags.bPendingScan = 1;
	if( monitor->parent_monitor && !monitor->parent_monitor->DoScanTime )
		monitor->parent_monitor->DoScanTime = monitor->DoScanTime;
	LeaveCriticalSec( &monitor->cs );
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( void, EndMonitor )( PMONITOR monitor )
{
	if( !monitor )
		return;
	// can't tear down while the timer thread is inside this monitor; defer.
	if( monitor->flags.bDispatched || monitor->flags.bScanning )
	{
		monitor->flags.bEnd = 1;
		return;
	}
	if( monitor->flags.bClosing )
		return;
	EnterCriticalSec( &monitor->cs );
	monitor->flags.bClosing = 1;

	if( monitor->fsStream )
	{
		FSEventStreamRef stream = (FSEventStreamRef)monitor->fsStream;
		// Stop/Invalidate guarantees no further callbacks once Invalidate
		// returns; Release drops our reference.
		FSEventStreamStop( stream );
		FSEventStreamInvalidate( stream );
		FSEventStreamRelease( stream );
		monitor->fsStream = NULL;
	}

	if( monitor->timer )
	{
		RemoveTimer( monitor->timer );
		monitor->timer = 0;
	}

	CloseFileMonitors( monitor );
	UnlinkThing( monitor );
	LeaveCriticalSec( &monitor->cs );
	Release( monitor );
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PMONITOR, MonitorFilesEx )( CTEXTSTR directory, int scan_delay, int flags )
{
	PMONITOR monitor;
	FSEventStreamRef stream;
	FSEventStreamContext context;
	CFStringRef cfpath;
	CFArrayRef pathsToWatch;

	if( !scan_delay )
		scan_delay = 1000;

	if( local_filemon.flags.bLog )
		Log1( "Going to start monitoring changes to: %s", directory );

	for( monitor = Monitors; monitor; monitor = NextThing( monitor ) )
	{
		if( StrCaseCmp( monitor->directory, directory ) == 0 )
		{
			if( local_filemon.flags.bLog )
				lprintf( "already monitoring this directory; should check flags...." );
			return monitor;
		}
	}

	monitor = (PMONITOR)Allocate( sizeof( MONITOR ) );
	MemSet( monitor, 0, sizeof( MONITOR ) );
	InitializeCriticalSec( &monitor->cs );
	StrCpyEx( monitor->directory, directory, sizeof( monitor->directory ) / sizeof( TEXTCHAR ) );
	monitor->flags.bIntelligentUpdates = 1;
	monitor->scan_delay = scan_delay;
	monitor->free_scan_delay = 5000;
	monitor->DoScanTime = timeGetTime() - 1; // scan next tick
	monitor->flags.bPendingScan = 1;
	monitor->flags.bRecurseSubdirs = ( flags & SFF_SUBCURSE ) ? TRUE : FALSE;

	if( !fsevents_queue )
		fsevents_queue = dispatch_queue_create( "org.sack.filemon", DISPATCH_QUEUE_SERIAL );

	cfpath = CFStringCreateWithCString( NULL, monitor->directory, kCFStringEncodingUTF8 );
	pathsToWatch = CFArrayCreate( NULL, (const void **)&cfpath, 1, &kCFTypeArrayCallBacks );
	CFRelease( cfpath );

	MemSet( &context, 0, sizeof( context ) );
	context.info = monitor;

	// kFSEventStreamCreateFlagFileEvents -> per-file granularity (10.7+)
	// kFSEventStreamCreateFlagNoDefer    -> deliver the first event of an
	//                                       idle burst immediately.
	stream = FSEventStreamCreate( NULL
	                            , (FSEventStreamCallback)FileMonStreamCallback
	                            , &context
	                            , pathsToWatch
	                            , kFSEventStreamEventIdSinceNow
	                            , (CFTimeInterval)( scan_delay / 1000.0 )
	                            , kFSEventStreamCreateFlagFileEvents
	                            | kFSEventStreamCreateFlagNoDefer );
	CFRelease( pathsToWatch );

	if( !stream )
	{
		Log1( "Failed to create FSEventStream to monitor directory: %s", monitor->directory );
		Release( monitor );
		return NULL;
	}

	monitor->fsStream = stream;
	FSEventStreamSetDispatchQueue( stream, fsevents_queue );
	if( !FSEventStreamStart( stream ) )
	{
		Log1( "Failed to start FSEventStream to monitor directory: %s", monitor->directory );
		FSEventStreamInvalidate( stream );
		FSEventStreamRelease( stream );
		monitor->fsStream = NULL;
		Release( monitor );
		return NULL;
	}

	if( local_filemon.flags.bLog )
		lprintf( "Started FSEvent monitor on %s", monitor->directory );

	LinkLast( Monitors, PMONITOR, monitor );

	// the FSEvents callback only flags DoScanTime; the timer thread does the
	// actual stat/diff and dispatches changes to the user callbacks.
	monitor->timer = AddTimer( scan_delay / 3, ScanTimer, (uintptr_t)monitor );

	return monitor;
}

FILEMONITOR_PROC( PMONITOR, MonitorFiles )( CTEXTSTR dirname, int scan_delay )
{
	return MonitorFilesEx( dirname, scan_delay, 0 );
}

FILEMON_NAMESPACE_END

#endif
