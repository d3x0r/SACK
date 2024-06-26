#ifdef _WIN32

#include <stdhdrs.h>
#include <time.h> // gmtime
// --- unix like files --
#include <io.h>
#include <fcntl.h>
// ----------------------
#include <string.h>
#include <stdio.h>

#include <network.h>

#include <sack_types.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <filesys.h>
#include "monitor.h"

FILEMON_NAMESPACE

//-------------------------------------------------------------------------

void ScanDirectory( PMONITOR monitor, PCHANGECALLBACK Change )
{
	HANDLE hFile;
	TEXTCHAR match[256];
	WIN32_FIND_DATA FindFileData;
	tnprintf( match, sizeof(match), "%s/*.*", monitor->directory );
	hFile = FindFirstFile( match, &FindFileData );
	if( local_filemon.flags.bLog )
		lprintf( "Scan directory: %s", match );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		do
		{
			if(!( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ))
			{
				//PCHANGECALLBACK Change;
				if( monitor->flags.bLogFilesFound )
					if( local_filemon.flags.bLog )
						Log1( "Found file %s", FindFileData.cFileName );
				// invoke change on this and it's parent (if there was a parent)
				//for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
				{
					if( !Change->mask ||
						CompareMask( Change->mask
										, FindFileData.cFileName
										, FALSE ) )
					{
						if( monitor->flags.bLogFilesFound )
							if( local_filemon.flags.bLog ) Log( "And the mask matched." );
						AddMonitoredFile( Change, FindFileData.cFileName );
					}
				}
			}
		} while( FindNextFile( hFile, &FindFileData ) );
		FindClose( hFile );
	}
	else
		if( local_filemon.flags.bLog ) Log( "FindFirstFile returned an invalid find handle" );
	if( local_filemon.flags.bLog ) lprintf( "Scanned directory: %s", match );
}


//-------------------------------------------------------------------------

PMONITOR Monitors;

//-------------------------------------------------------------------------

FILEMONITOR_PROC( void, EndMonitor )( PMONITOR monitor )
{
	if( !monitor )
		return;
	if( monitor->flags.bDispatched || monitor->flags.bScanning )
	{
		monitor->flags.bEnd = 1;
		return;
	}
	if( monitor->flags.bClosing )
	{
		if( local_filemon.flags.bLog ) Log( "Monitor already closing..." );
		return;
	}
	EnterCriticalSec( &monitor->cs );
	monitor->flags.bClosing = 1;
	monitor->flags.bRemoveFromEvents = 1;

	if( !monitor->flags.bRemovedFromEvents )
	{
		SetEvent( local_filemon.hMonitorThreadControlEvent );
		while( !monitor->flags.bRemovedFromEvents )
			Relinquish();
	}
	//Log1( "Closing the monitor on %s and killing thread..."
	//    , monitor->directory );
	if( monitor->hChange != INVALID_HANDLE_VALUE )
	{
		lprintf( "close ntoification (wakes thread?" );
		FindCloseChangeNotification( monitor->hChange );
		lprintf( "and then we wait..." );
	}
	monitor->hChange = INVALID_HANDLE_VALUE;
	{
		uint32_t tick = timeGetTime();
		while( monitor->pThread && ( ( tick+50 ) > timeGetTime() ) )
			Relinquish();
	}
	if( monitor->pThread )
	{
		EndThread( monitor->pThread );
	}
	//else
	//	Log( "Thread already left..." );
	CloseFileMonitors( monitor );
	RemoveTimer( monitor->timer );
	UnlinkThing( monitor );
	LeaveCriticalSec( &monitor->cs );
	Release( monitor );
}

//-------------------------------------------------------------------------

struct filemon_peer_thread_info
{
	HANDLE *phNextThread;
	PMONITOR resume_from;
	INDEX resume_from_sub;
	struct filemon_peer_thread_info *parent_peer;
	struct filemon_peer_thread_info *child_peer;
	PLIST monitor_list;
	PDATALIST event_list;
	HANDLE *phMyThread;
	HANDLE hNextThread;
	int nEvents;
	struct {
		BIT_FIELD bProcessing : 1;
		BIT_FIELD bBuildingList : 1;
	} flags;
	INDEX iEvent; // used to get back monitor info
};
static uintptr_t CPROC MonitorFileThread( PTHREAD pThread );

static void ClearThreadEvents( struct filemon_peer_thread_info *info )
{
	struct filemon_peer_thread_info *first_peer = info;
	struct filemon_peer_thread_info *peer;
	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		EmptyList( &peer->monitor_list );
		EmptyDataList( &peer->event_list );
		SetLink( &peer->monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
		SetDataItem( &peer->event_list, 0, peer->phMyThread );
		peer->nEvents = 1;
	}
	{
		PMONITOR monitor;
		for( monitor = Monitors; monitor; monitor = NextThing( monitor ) )
		{
			PMONITOR sub_monitor;
			INDEX idx;
			monitor->flags.bAddedToEvents = 0;

			LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
			{
				monitor->flags.bAddedToEvents = 0;
			}
		}
	}
}

static void FileMonAddThreadEvent( PMONITOR monitor, struct filemon_peer_thread_info *info )
{
	struct filemon_peer_thread_info *first_peer;
	struct filemon_peer_thread_info *last_peer;
	struct filemon_peer_thread_info *peer;

	if( monitor->flags.bRemoveFromEvents )
	{
		return;
	}

	for( first_peer = info; first_peer && first_peer->parent_peer; first_peer = first_peer->parent_peer );
	for( last_peer = info; last_peer && last_peer->child_peer; last_peer = last_peer->parent_peer );
	for( peer = first_peer; peer->nEvents >= 60; peer = peer->child_peer );
	if( !peer )
	{
		if( local_filemon.flags.bLog )
			lprintf( "Now at event capacity, this is where next should resume from" );
		ThreadTo( MonitorFileThread, (uintptr_t)last_peer );
		while( !last_peer->child_peer )
			Relinquish();
		last_peer = last_peer->child_peer;
		peer = last_peer; // since we needed to create some more monitor space; this is where we should add this monitor;
	}


	{
		// make sure to only add this handle when the first peer will also be added.
		// this means the list can be 61 and at this time no more.
		if( !monitor->flags.bAddedToEvents )
		{
			AddLink( &peer->monitor_list, monitor );
			AddDataItem( &peer->event_list, &monitor->hChange );
			if( local_filemon.flags.bLog )
			{
				lprintf( "Added handle %d on %s", monitor->hChange, monitor->directory );
				//LogBinary( event_list->data, (nEvents +1)* sizeof( HANDLE ));
			}
			peer->nEvents++;
			monitor->flags.bAddedToEvents = 1;
		}
	}
}

static void ReadChanges( PMONITOR monitor )
{
	static uint8_t buffer[4096];
	DWORD dwResultSize;

	if( local_filemon.flags.bLog )
		lprintf( "Begin getting changes on %p (%d)", monitor, monitor->hChange );

	if( ReadDirectoryChangesW( monitor->hChange
	                        , buffer
	                        , sizeof( buffer )
	                        , monitor->flags.bRecurseSubdirs
	                        , FILE_NOTIFY_CHANGE_FILE_NAME
	                         | FILE_NOTIFY_CHANGE_DIR_NAME
	                         | FILE_NOTIFY_CHANGE_ATTRIBUTES
	                         | FILE_NOTIFY_CHANGE_SIZE
	                         | FILE_NOTIFY_CHANGE_LAST_WRITE
	                         | FILE_NOTIFY_CHANGE_LAST_ACCESS
	                         | FILE_NOTIFY_CHANGE_CREATION
	                         | FILE_NOTIFY_CHANGE_SECURITY
	                        , &dwResultSize
	                        , NULL
	                        , NULL ) 
	                        )
	{
		if( dwResultSize )
		{

			PFILE_NOTIFY_INFORMATION pni;
			DWORD dwOffset = 0;
			do
			{
				wchar_t *dupname;
				CTEXTSTR a_name;
				pni = (PFILE_NOTIFY_INFORMATION)(buffer + dwOffset );
				dupname = NewArray( wchar_t, ( pni->FileNameLength + sizeof( wchar_t ) ) / sizeof( wchar_t ) );
				MemCpy( dupname, pni->FileName, pni->FileNameLength );
				dupname[pni->FileNameLength/2] = '\0';
				if( local_filemon.flags.bLog )
					lprintf( "offset %d, next %d", dwOffset, pni->NextEntryOffset );
				if( !pni->NextEntryOffset )
					dwOffset = (DWORD)INVALID_INDEX;
				else
					dwOffset += pni->NextEntryOffset;
				if( local_filemon.flags.bLog )
				{
					LogBinary( (const uint8_t*)pni, sizeof( (*pni ) ) );
					LogBinary( (const uint8_t*)pni->FileName, 64 );
				}
				a_name = DupWideToText( dupname );

				if( local_filemon.flags.bLog )
					lprintf( "File change was on %s", a_name );
				switch( pni->Action )
				{
				case FILE_ACTION_MODIFIED:
					if( local_filemon.flags.bLog )
						lprintf( "File modified" );
					if( 0 )
					{
				case FILE_ACTION_ADDED:
					if( local_filemon.flags.bLog )
						lprintf( "File added" );
					}
					{
						PCHANGECALLBACK Change;
						if( local_filemon.flags.bLog )
							lprintf( "checking handlers %p", monitor->ChangeHandlers );
						for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
						{
							if( local_filemon.flags.bLog )
								lprintf( "checking handlers %p %s %s", Change->mask, Change->mask?Change->mask: "*", a_name );
							if( !Change->mask ||
								CompareMask( Change->mask
											  , a_name
											  , FALSE ) )
							{
								if( monitor->flags.bLogFilesFound )
									if( local_filemon.flags.bLog ) Log( "And the mask matched." );
								AddMonitoredFile( Change, a_name );
							}
						}
					}
					break;
				case FILE_ACTION_REMOVED:
					if( local_filemon.flags.bLog )
						lprintf( "File removed" );
					{
						PCHANGECALLBACK Change;
						for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
						{
							if( !Change->mask ||
								CompareMask( Change->mask
											  , a_name
											  , FALSE ) )
							{
								PFILEMON filemon = WatchingFile( Change, a_name );
								// deleted file that we're not watching already?
								if( filemon )
								{
									filemon->flags.bToDelete = 1;
									filemon->ScannedAt = timeGetTime() + monitor->scan_delay;
									filemon->flags.bPending = 1;
									EnqueLink( &Change->PendingChanges, filemon );
								}
							}
						}
					}
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					if( local_filemon.flags.bLog )
						lprintf( "old rename" );
					{
						PCHANGECALLBACK Change;
						for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
						{
							if( !Change->mask ||
								CompareMask( Change->mask
											  , a_name
											  , FALSE ) )
							{
								PFILEMON filemon = WatchingFile( Change, a_name );
								if( filemon )
								{
									filemon->flags.bToDelete = 1;
									filemon->ScannedAt = timeGetTime() + monitor->scan_delay;
									filemon->flags.bPending = 1;
									EnqueLink( &Change->PendingChanges, filemon );
								}
							}
						}
					}
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					if( local_filemon.flags.bLog )
						lprintf( "new rename" );
					{
						PCHANGECALLBACK Change;
						for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
						{
							if( !Change->mask ||
								CompareMask( Change->mask
											  , a_name
											  , FALSE ) )
							{
								AddMonitoredFile( Change, a_name );
							}
						}
					}
					break;
				}
#ifndef _UNICODE
				Deallocate( CTEXTSTR, a_name );
#endif
				Deallocate( wchar_t *, dupname );
			}
			while( dwOffset != (DWORD)INVALID_INDEX );
		}
	}
}

static LOGICAL PeerIsBuilding( struct filemon_peer_thread_info *info )
{
	struct filemon_peer_thread_info *first_peer = info;
	struct filemon_peer_thread_info *peer;
	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		if( peer->flags.bBuildingList )
			return TRUE;
	}
	return FALSE;
}

static LOGICAL PeerIsBusy( struct filemon_peer_thread_info *info )
{
	struct filemon_peer_thread_info *first_peer = info;
	struct filemon_peer_thread_info *peer;
	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		if( peer->flags.bProcessing )
			return TRUE;
	}
	return FALSE;
}

static void WakePeers( struct filemon_peer_thread_info *info )
{
	struct filemon_peer_thread_info *first_peer = info;
	struct filemon_peer_thread_info *peer;
	while( first_peer && first_peer->parent_peer )
		first_peer = first_peer->parent_peer;
	for( peer = first_peer; peer; peer = peer->child_peer )
	{
		if( peer->hNextThread != INVALID_HANDLE_VALUE )
			SetEvent( peer->hNextThread );
	}
}

static uintptr_t CPROC MonitorFileThread( PTHREAD pThread )
{
	struct filemon_peer_thread_info this_thread;
	struct filemon_peer_thread_info *peer_thread = (struct filemon_peer_thread_info*)GetThreadParam( pThread );
	LOGICAL rebuild_events = peer_thread?FALSE:TRUE;
	PMONITOR monitor;
	DWORD dwResult;

	this_thread.nEvents = 0;
	this_thread.hNextThread = INVALID_HANDLE_VALUE;
	this_thread.phMyThread = peer_thread?peer_thread->phNextThread:&local_filemon.hMonitorThreadControlEvent;
	this_thread.monitor_list = NULL;
	this_thread.event_list = CreateDataList( sizeof( HANDLE ) );
	this_thread.phNextThread = &this_thread.hNextThread;
	this_thread.resume_from = NULL;
	this_thread.resume_from_sub = INVALID_INDEX;
	this_thread.parent_peer = peer_thread;
	this_thread.child_peer = NULL;

	if( peer_thread )
		peer_thread->child_peer = &this_thread;

	(*this_thread.phMyThread) = CreateEvent( NULL, TRUE, FALSE, NULL );

	if( local_filemon.flags.bLog )
		lprintf( "thread control handle %p", (*this_thread.phMyThread) );

	SetLink( &this_thread.monitor_list, 0, (POINTER)1 ); // has to be a non zero value.  monitor is not referenced for wait event 0
	SetDataItem( &this_thread.event_list, 0, this_thread.phMyThread );
	this_thread.nEvents = 1;

	do
	{
		if( rebuild_events )
		{
			if( local_filemon.flags.bLog )
				lprintf( "rebuilding events." );
			this_thread.flags.bBuildingList = 1;
			this_thread.flags.bProcessing = 0;

			while( PeerIsBusy( &this_thread ) )
			{
				lprintf( "a peer is busy with an event?" );
				Relinquish();
			}

			ClearThreadEvents( &this_thread );

			// this is the routine 'buildthreadevents'
			//lprintf( "...%p %p %p", peer_thread, peer_thread?peer_thread->resume_from:NULL, next_monitor );
			//lprintf( "first monitor is %p", next_monitor );
			for( monitor = Monitors; monitor; monitor = NextThing( monitor ) )
			{
				PMONITOR sub_monitor;
				INDEX idx;
				FileMonAddThreadEvent( monitor, &this_thread );

				LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
				{
					FileMonAddThreadEvent( sub_monitor, &this_thread );
				}
			}

			this_thread.flags.bBuildingList = 0;
			WakePeers( &this_thread );
			rebuild_events = FALSE;
		}


		if( local_filemon.flags.bLog )
		{
			lprintf( "begin wait on %d events", this_thread.nEvents );
			//LogBinary( event_list->data, this_thread. * sizeof( HANDLE ));
		}


		this_thread.flags.bProcessing = 0;
		dwResult = WaitForMultipleObjects( this_thread.nEvents
												  , (const HANDLE *)this_thread.event_list->data			//Log( "Waiting for a change..." );
												  , FALSE
												  , INFINITE
													);
		this_thread.flags.bProcessing = 1;

		if( PeerIsBuilding( &this_thread ) )
		{
			if( local_filemon.flags.bLog )
				lprintf( "Work on %ld but peer is building my lists, so I can't use them", dwResult );
			continue;
		}

		if( local_filemon.flags.bLog )
			lprintf( "Result of wait was %ld", dwResult );
		//dwResult = WaitForSingleObject( monitor->hChange, monitor->free_scan_delay /*900000*/ /*3600000*/ );
		if( dwResult == WAIT_FAILED )
		{
			if( GetLastError() == 6 )
			{
				rebuild_events = TRUE;
				if( local_filemon.flags.bLog )
				{
					LogBinary( (const uint8_t*)this_thread.event_list->data, this_thread.nEvents * sizeof( HANDLE ));
					lprintf( "Wait failed on %d... %d", this_thread.nEvents, GetLastError() );
				}
				continue;
			}
			if( local_filemon.flags.bLog )
			{
				LogBinary( (const uint8_t*)this_thread.event_list->data, this_thread.nEvents * sizeof( HANDLE ));
				lprintf( "Wait failed on %d... %d", this_thread.nEvents, GetLastError() );
			}
			break;
		}
		else if( dwResult == WAIT_ABANDONED )
		{
			if( local_filemon.flags.bLog ) lprintf( "Wait abandoned - have to leave..." );
			//MessageBox( NULL, "Wait returned bad error", "Monitor Failed", MB_OK );
			break;
		}
		else if( dwResult == WAIT_OBJECT_0 )
		{
			if( local_filemon.flags.bLog ) 
				lprintf( "signaled to rebuild events..." );
			ResetEvent( (*this_thread.phMyThread) );

			// if not the root thread, our lists will be built for us.
			if( this_thread.parent_peer == NULL )
			{
				if( local_filemon.flags.bLog ) 
					lprintf( "this is the root thread... " );
				rebuild_events = TRUE;
			}
			else
				lprintf( "Nevermind, we're nto the root thread." );
		}
		else if( dwResult >= (WAIT_OBJECT_0+1) && dwResult <= (WAIT_OBJECT_0+this_thread.nEvents ) )
		{
			monitor = (PMONITOR)GetLink( &this_thread.monitor_list, dwResult - WAIT_OBJECT_0 );
			ReadChanges( monitor );
			if( !FindNextChangeNotification( monitor->hChange ) )
			{
				DWORD dwError = GetLastError();
				lprintf( "Find next change failed...%d %s", dwError, monitor->directory );
				// bad things happened
				//MessageBox( NULL, "Find change notification failed", "Monitor Failed", MB_OK );
				if( dwError == ERROR_TOO_MANY_CMDS )
				{
					WakeableSleep( 50 );
					continue;
				}
				monitor->hChange = INVALID_HANDLE_VALUE;
				break;
			}

			// keep pushing this forward for every good scan
			// if a LOT of files are being scanned over a slow network
			// we need to keep pushing off the physical scan of changes.
			if( monitor->parent_monitor )
				monitor->parent_monitor->DoScanTime = timeGetTime() - 1;

			if( !monitor->DoScanTime )
				monitor->DoScanTime = timeGetTime() - 1;
			monitor->flags.bPendingScan = 1;
		}
 
	} while( 1 );
	if( local_filemon.flags.bLog ) Log( "Leaving the thread..." );

	for( monitor = Monitors; monitor; monitor = Monitors )
	{
		monitor->flags.bRemovedFromEvents = 1;
		EndMonitor( monitor );
	}
	return 0; // something....
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PMONITOR, MonitorFilesEx )( CTEXTSTR directory, int scan_delay, int flags )
{
	PMONITOR monitor;
	if( !scan_delay )
		scan_delay = 1000;
	if( local_filemon.flags.bLog )
		Log1( "Going to start monitoring changes to: %s", directory );

	{
		for( monitor = Monitors; monitor; monitor = NextThing( monitor ) )
		{
			if( StrCaseCmp( monitor->directory, directory ) == 0 )
			{
				lprintf( "already monitoring this directory; should check flags...." );
				return monitor;
			}
		}
	}

	monitor = (PMONITOR)Allocate( sizeof( MONITOR ) );
	MemSet( monitor, 0, sizeof( MONITOR ) );
	InitializeCriticalSec( &monitor->cs );
	StrCpyEx( monitor->directory, directory, sizeof( monitor->directory )/sizeof(TEXTCHAR) );
	//strcpy( monitor->mask, mask );
	monitor->flags.bIntelligentUpdates = 1;
	monitor->scan_delay = scan_delay;
	monitor->free_scan_delay = 5000;
	monitor->DoScanTime = timeGetTime() - 1; // scan next tick
	monitor->flags.bPendingScan = 1;
	monitor->flags.bRecurseSubdirs = (flags&SFF_SUBCURSE)?TRUE:FALSE;
	monitor->hChange = FindFirstChangeNotification( monitor->directory
	                                              , monitor->flags.bRecurseSubdirs
	                                              , FILE_NOTIFY_CHANGE_FILE_NAME
	                                               | FILE_NOTIFY_CHANGE_DIR_NAME
	                                               | FILE_NOTIFY_CHANGE_SIZE
	                                               | FILE_NOTIFY_CHANGE_LAST_WRITE
	                                               | FILE_NOTIFY_CHANGE_ATTRIBUTES
	                                               | FILE_NOTIFY_CHANGE_SIZE
	                                               | FILE_NOTIFY_CHANGE_LAST_WRITE
	                                               | FILE_NOTIFY_CHANGE_CREATION
	                                               | FILE_NOTIFY_CHANGE_SECURITY
	                                              );

	if( local_filemon.flags.bLog ) lprintf( "Opened handle %d on %s", monitor->hChange, monitor->directory );
	if( monitor->hChange == INVALID_HANDLE_VALUE )
	{
		TEXTCHAR msg[128];
		tnprintf( msg, sizeof(msg), "Cannot monitor directory: %s", monitor->directory );
		// probably log something like we didn't have a good directory?
		// this needs to be a visible failure - likely a
		// configuration error...
		// and being in windows this is allowed?
		MessageBox( NULL, msg, "Monitor Failed", MB_OK );
	}

	if( !local_filemon.directory_monitor_thread )
	{
		local_filemon.directory_monitor_thread = ThreadTo( MonitorFileThread, (uintptr_t)0 );
	}

	// wait for thread to finish initialization
	while( !local_filemon.hMonitorThreadControlEvent )
		Relinquish();

	// This could be added boefre the thread is ready; but then we're not guaranteed that this will be in the list of objects to wait on
	LinkLast( Monitors, PMONITOR, monitor );

	if( local_filemon.flags.bLog )
		lprintf( "Signal monitor to wake on %d", local_filemon.hMonitorThreadControlEvent );
	SetEvent( local_filemon.hMonitorThreadControlEvent );

	if( local_filemon.flags.bLog )
		Log1( "Adding timer %d", scan_delay / 3 );
	monitor->timer = AddTimer( scan_delay/3, ScanTimer, (uintptr_t)monitor );

	return monitor;
}

FILEMONITOR_PROC( PMONITOR, MonitorFiles )( CTEXTSTR directory, int scan_delay )
{
	return MonitorFilesEx( directory, scan_delay, 0 );
}


FILEMON_NAMESPACE_END

#endif