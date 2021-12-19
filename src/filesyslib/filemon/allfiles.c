#define PRIMARY_FILEMON_SOURCE
#include <stdhdrs.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#ifdef WIN32
//#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <sys/stat.h>
//#include <linux/fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#  ifndef O_BINARY
#    define O_BINARY 0
#  endif
//#include <windunce.h>
#endif

#if !defined( WIN32 ) && !defined( S_IFDIR )
#include <sys/stat.h>
#endif

#include <time.h>

#include <signal.h>


#include <string.h>
#include <stdio.h>

#include <stdhdrs.h>

#include <sack_types.h>
#include <sharemem.h>
#include <filesys.h>
#include <logging.h>

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE -1
#endif

#include "monitor.h"

FILEMON_NAMESPACE

#ifdef __cplusplus
using namespace sack::containers::queue;
#endif
//-------------------------------------------------------------------------


static void InitFileMonitor( void )
{
	if( !local_filemon.flags.bInit )
	{
#ifndef __NO_OPTIONS__
		local_filemon.flags.bLog = SACK_GetProfileIntEx( "SACK/File Monitor", "Enable Logging", 0, TRUE );
#endif
		//local_filemon.flags.bLog = 1;
		local_filemon.flags.bInit = 1;
	}
}

PRELOAD( InitFileMon)
{
	InitFileMonitor();
}

int CPROC CompareName( uintptr_t psv1, uintptr_t psv2 )
{
	CTEXTSTR s1 = (CTEXTSTR)psv1;
	CTEXTSTR s2 = (CTEXTSTR)psv2;
	return StrCmp( s1, s2 );
}

//-------------------------------------------------------------------------

void CloseFileMonitor( PCHANGEHANDLER Change, PFILEMON filemon )
{
	if( !Change || !filemon )
		return;
	//Log1( "Closing %p", filemon );
	RemoveBinaryNode( Change->filelist, (POINTER)filemon, (uintptr_t)filemon->filename );
	//DeleteLink( &Change->filelist, filemon );
	if( Change->currentchange == filemon )
		Change->currentchange = NULL;
	DeleteFromSet( FILEMON, local_filemon.filemon_set, filemon );
}

//-------------------------------------------------------------------------

void CloseFileMonitors( PMONITOR monitor )
{
	PFILEMON filemon;
	//INDEX idx;
	PCHANGEHANDLER Change;
	if( !monitor )
		return;
	for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
	{
		for( filemon = (PFILEMON)GetLeastNode(Change->filelist); filemon; filemon = (PFILEMON)GetGreaterNode(Change->filelist) )
		{
			CloseFileMonitor( Change, filemon );
		}
		Change->currentchange = NULL;
		DeleteLinkQueue( &Change->PendingChanges );
		DestroyBinaryTree( Change->filelist );
		Change->filelist = NULL;
		//DeleteList( &Change->filelist );
	}
	if( local_filemon.flags.bLog ) Log( "Closed all" );
}


//-------------------------------------------------------------------------

int IsDirectory( CTEXTSTR name )
{
#ifdef __cplusplus_cli
	System::String^ tmp = gcnew System::String(name);	
	if( System::IO::Directory::Exists( tmp ) )		
		return 1;
	return 0;
#else
#ifdef WIN32
	{
		uint32_t dwAttr = GetFileAttributes( name );
		if( dwAttr == -1 ) // uncertainty about what it really is, return ti's not a directory
			return 0;
		if( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
			return 1;
		return 0;
	}
#else
	struct stat statbuf;
	if( 
	#ifdef __cplusplus
	
	#endif
	stat( (const char*)name, &statbuf ) >= 0 && S_ISDIR(statbuf.st_mode) )
		return 1;
	return 0;
#endif
#endif
}

//-------------------------------------------------------------------------

PFILEMON WatchingFile( PCHANGEHANDLER monitor, CTEXTSTR name )
{
	PFILEMON filemon;
	//INDEX idx;
	// this check could be moved
	// but then I'd have to track a filemonitor structure
	// for each directory... well this is fairly efficient
	// so I'm not significantly worried at this moment...
	if( name[0] == '.' && ( strcmp( name, "." ) == 0 ||
		strcmp( name, ".." ) == 0 ) )
	{
		//if( local_filemon.flags.bLog ) Log1( "%s is a root file path", name );
		return (PFILEMON)2; // claim we already know about these  - stops actual updates
	}
	filemon = (PFILEMON)FindInBinaryTree( monitor->filelist, (uintptr_t)name );
	if( !filemon && IsDirectory( name ) )
	{
		//if( local_filemon.flags.bLog ) Log1( "%s is a directory - probably skipping..", name );
		return (PFILEMON)1;
	}
	return filemon;
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PFILEMON, AddMonitoredFile )( PCHANGEHANDLER Change, CTEXTSTR name )
{
	if( Change )
	{
		PFILEMON newfile;
		PMONITOR monitor = Change->monitor;
							  //uint64_t tick = GetCPUTick();
		if( local_filemon.flags.bLog ) lprintf( "Watch file: %s", name );
		if( !(newfile=WatchingFile( Change, name ) ) )
		{
			size_t pos;
			CTEXTSTR base;
			base = monitor->directory;
			newfile = GetFromSet( FILEMON, &local_filemon.filemon_set );
			//Log2( "Making a new file (not watched): %s/%s", base, name );
			tnprintf( newfile->name, sizeof(newfile->name), "%s/", base );
			pos = strlen( newfile->name );
			newfile->filename = newfile->name + pos;
			StrCpyEx( newfile->filename, name, (sizeof( newfile->name )/sizeof(TEXTCHAR)) - pos );
#ifdef __cplusplus_cli
			newfile->lastmodifiedtime = gcnew System::DateTime();
#else
#ifdef WIN32
			(*(uint64_t*)&newfile->lastmodifiedtime) = 0;
#else
			newfile->lastmodifiedtime = 0;
#endif
#endif
			newfile->flags.bScanned         = 0;
			newfile->flags.bToDelete        = 0;
			newfile->flags.bDirectory       = 0;
			newfile->flags.bPending         = 0;
			newfile->flags.bCreated         = Change->flags.bInitial?0:1;
			newfile->lastknownsize    = 0;
			AddBinaryNode( Change->filelist, newfile, (uintptr_t)newfile->filename );
			BalanceBinaryTree( Change->filelist );
			//AddLink( &Change->filelist, newfile );
		}
		else
		{
			if( newfile != (PFILEMON)1 && // iS NOT a directory name
				newfile != (PFILEMON)2 )  // iS NOT . or .. directories
			{
				// IS a file.
				newfile->flags.bScanned = FALSE; // okay need to get new information about this.
				newfile->flags.bToDelete = FALSE; // present - not deleted.
			}
		}
		if( newfile != (PFILEMON)1 && // iS NOT a directory name
			newfile != (PFILEMON)2 )  // iS NOT . or .. directories
		{
         if( local_filemon.flags.bLog ) lprintf( "SET MONITOR SCAN" );
			monitor->DoScanTime =
				newfile->ScannedAt = timeGetTime() + monitor->scan_delay;
			if( monitor->parent_monitor )
				monitor->parent_monitor->DoScanTime = monitor->DoScanTime;
			monitor->flags.bPendingScan = 1;
		}
		//{
		//	uint64_t tick2 = GetCPUTick();
		//   lprintf( "Delta %Ld (%d)", tick2-tick, ConvertTickToMicrosecond( tick2-tick ) );
		//}
		return newfile;
	}
	return NULL;
}

//-------------------------------------------------------------------------

uintptr_t CPROC ScanFile( uintptr_t psv, INDEX idx, POINTER *item )
{
	PCHANGEHANDLER Change = (PCHANGEHANDLER)psv;
	PFILEMON filemon = (PFILEMON)(*item);
	uint32_t dwSize;
#ifdef WIN32
#else
	struct stat statbuf;
#endif
	// if this file is already pending a change
	// do not re-enque... if it's changing THIS often
	// such that a idle time has elapsed between changes
	// and just when we start to queue it for event dispatch
	// it changes AGAIN ... whatever... we don't like these
	// kind of logging stream files - but they'll work....
	// eventually a free scan will happen even if we end up
	// losing the very last change announcement... but
	// even then ... blah it'll all be good... the clock
	// mechanism may get extra wait states, but the notice will
	// be moved forward...
	if( filemon->flags.bPending )
	{
		if( local_filemon.flags.bLog ) lprintf( "%s Already pending.... do not scan yet.", filemon->name );
		return 0;
	}
	if( !filemon->flags.bScanned )
	{
		FILETIME lastmodified;
		filemon->flags.bScanned = TRUE;
		if( filemon->flags.bDirectory ) {
			dwSize = 0xFFFFFFFF;
#ifdef WIN32
			lastmodified.dwHighDateTime = 0;
			lastmodified.dwLowDateTime = 0;
#else
			lastmodified = 0;
#endif
		} else {
			dwSize = GetFileTimeAndSize( filemon->name, NULL, NULL, &lastmodified, NULL );
		}
#ifdef WIN32
		// this 
		if( local_filemon.flags.bLog )
		lprintf( "File change stats: %s(%s) %lu %lu, %lu %lu, %s"
				 , filemon->name
				 , filemon->filename
				 , dwSize
				 , filemon->lastknownsize
		  	 , lastmodified, filemon->lastmodifiedtime
		  	 , filemon->flags.bToDelete?"delete":"" );
#endif
		if( dwSize != filemon->lastknownsize
			|| (*(uint64_t*)&lastmodified) != (*(uint64_t*)&filemon->lastmodifiedtime)
			|| filemon->flags.bToDelete
		  )
		{
			filemon->lastknownsize = dwSize;
#ifdef WIN32
			(*(uint64_t*)&filemon->lastmodifiedtime) = (*(uint64_t*)&lastmodified);
#else
			filemon->lastmodifiedtime = lastmodified;
#endif
			if( !filemon->flags.bToDelete && !filemon->flags.bCreated )
			{
				if( dwSize == 0xFFFFFFFF )
				{
					//Log1( "Directory %s changed", filemon->name );
				}
				else
				{
					//Log1( "File %s changed", filemon->name );
				}
				filemon->ScannedAt = timeGetTime() + Change->monitor->scan_delay;
				if( local_filemon.flags.bLog )
					lprintf( "file changed Setting monitor do scan time - new file monitor" );
				Change->monitor->DoScanTime = filemon->ScannedAt;
				Change->monitor->flags.bPendingScan = 1;
				//Log( "A file changed... setting file's change time at now plus delay" );
				//filemon->lastknownsize = dwSize;
				//filemon->lastmodifiedtime = statbuf.st_mtime;
				return 0;
			}
			else
			{
				// if created or deleted, immediatly pend the change.
				if( local_filemon.flags.bLog ) Log1( "Enque (pend) filemon for a change...%s", filemon->name );
				filemon->flags.bPending = 1;
				EnqueLink( &Change->PendingChanges, filemon );
			}
		}
	}

	if( !filemon->flags.bPending &&
		 !Change->flags.bInitial &&
	    !filemon->flags.bToDelete &&
		 filemon->ScannedAt &&
		 filemon->ScannedAt < timeGetTime() )
	{
		//Log(" File didn't change - but it did before..." );
		if( local_filemon.flags.bLog ) Log1( "Enque (pend) filemon for a change...%s", filemon->name );
		filemon->flags.bPending = 1;
		EnqueLink( &Change->PendingChanges, filemon );
	}
	return 0;
}


//-------------------------------------------------------------------------

static void ScanMonitorFiles( PMONITOR monitor )
{
	PCHANGEHANDLER Change;
	for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
	{
		PFILEMON filemon;
		lprintf( "Scanning mask: %s in %s", Change->mask, Change->monitor->directory );
		for( filemon = (PFILEMON)GetLeastNode( Change->filelist )
			 ; filemon
			  ; filemon = (PFILEMON)GetGreaterNode( Change->filelist ) )
		{
			ScanFile( (uintptr_t)Change, 0, (POINTER*)&filemon );
		}
		//while( ForAllLinks( &Change->filelist, ScanFile, (uintptr_t)Change ) );
	}
}

//-------------------------------------------------------------------------

// really this just sets on each file holder already known
// that the file has been scaned, is not deleted, clear the size and modified time.
static void DoForgetAll( PCHANGEHANDLER Change )
{
	PFILEMON filemon;
	for( filemon = (PFILEMON)GetLeastNode( Change->filelist ); filemon; filemon = (PFILEMON)GetGreaterNode( Change->filelist ) )
	{
		if( !filemon->flags.bPending )
		{
			filemon->flags.bScanned = FALSE;
			filemon->flags.bToDelete = 1;
			filemon->lastknownsize = 0;
#ifdef __cplusplus_cli
			filemon->lastmodifiedtime = gcnew System::DateTime();
#else
#ifdef WIN32
			(*(uint64_t*)&filemon->lastmodifiedtime) = 0;
#else
			filemon->lastmodifiedtime = 0;
#endif
#endif
		}
	}
}

FILEMONITOR_PROC( void, MonitorForgetAll )( PMONITOR monitor )
{
	PCHANGEHANDLER Change;
	EnterCriticalSec( &monitor->cs );
	for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
	{
		DoForgetAll( Change );
		// okay also if there are sub_monitors make sure they forget everything too.
		if( monitor->monitors )
		{
			INDEX idx;
			PMONITOR sub_monitor;
			LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
			{
				for( Change = sub_monitor->ChangeHandlers; Change; Change = Change->next )
				{
					DoForgetAll( Change );
				}
			}
		}
	}
	if( local_filemon.flags.bLog ) lprintf( "Setting monitor do scan time - forget all files" );

	monitor->DoScanTime = timeGetTime() - 1;
	monitor->flags.bPendingScan = 1;
	LeaveCriticalSec( &monitor->cs );
}

//-------------------------------------------------------------------------

static void DeleteAll( PMONITOR monitor )
{
	// called while in critical section
	PCHANGEHANDLER Change;
	for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
	{
		PFILEMON filemon;
		//INDEX idx;
		for( filemon = (PFILEMON)GetLeastNode( Change->filelist ); filemon; filemon = (PFILEMON)GetGreaterNode( Change->filelist ) )
			//LIST_FORALL( Change->filelist, idx, PFILEMON, filemon )
		{
			// if a change is already pending, do not mark it
			// deleted... the dispatch might happen just then.
			if( !filemon->flags.bPending )
			{
				//Log1( "Marking delete: %s", filemon->filename );
				filemon->flags.bScanned = 0;
				filemon->flags.bToDelete = 1;
				// if the file has already been in this list once,
				// it is no longer created, otherwise it will not be here,
				// but will be added in the next stage of scanning as created.
				// created must then immediatly pend that change.
				if( !Change->flags.bInitial )
				{
					filemon->flags.bCreated = 0;
					//lprintf( "File %s is no longer created.", filemon->name );
				}
			}
		}
	}
}

//-------------------------------------------------------------------------

static int DispatchChangesInternal( PMONITOR monitor, LOGICAL bExternalSource )
{
	if( IsThisThread( local_filemon.timer_thread ) )
	{
		uint32_t now = timeGetTime();
		int changed = 0;
		PCHANGEHANDLER Change;
		if( monitor->flags.bClosing )
			return 0;
		EnterCriticalSec( &monitor->cs );
		monitor->flags.bDispatched = 1;
		for( Change = monitor->ChangeHandlers; Change && !monitor->flags.bUserInterruptedChanges; Change = Change->next )
		{
			while( ( Change->currentchange = (PFILEMON)DequeLink( &Change->PendingChanges ) ) )
			{
				TEXTCHAR fullname[256];
				if( Change->currentchange->ScannedAt > now )
				{
					PrequeLink( &Change->PendingChanges, Change->currentchange );
					Change->currentchange = NULL;
					break;
				}
				changed++;
				Change->currentchange->ScannedAt = 0;
				tnprintf( fullname, sizeof( fullname ), "%s" SYSPATHCHAR "%s", monitor->directory, Change->currentchange->filename );
				if( local_filemon.flags.bLog ) lprintf( "Report %s for a change (%s,%s)"
												  , fullname
												  , Change->currentchange->flags.bToDelete?"deleted":""
												  , Change->currentchange->flags.bCreated?"created":""
												  );
				if( Change->HandleChange )
				{
					if( Change->flags.bExtended )
					{
						if( !Change->HandleChangeEx( Change->psv
															, fullname
															, Change->currentchange->lastknownsize
#ifdef __cplusplus_cli
															, Change->currentchange->lastmodifiedtime->Ticks
#else
															, (*(uint64_t*)&Change->currentchange->lastmodifiedtime)
#endif
															, Change->currentchange->flags.bCreated
															, Change->currentchange->flags.bDirectory
															, Change->currentchange->flags.bToDelete ) )
						{
							monitor->flags.bUserInterruptedChanges = 1;
							break;
						}
					}
					else
					{
						if( !Change->HandleChange( Change->psv
														 , fullname
														 , Change->currentchange->flags.bToDelete ) )
						{
							monitor->flags.bUserInterruptedChanges = 1;
							break;
						}
					}
				}
				if( Change->currentchange->flags.bToDelete )
					CloseFileMonitor( Change, Change->currentchange );
				else
				{
					Change->currentchange->flags.bScanned = FALSE;
					if( local_filemon.flags.bLog ) lprintf( "Done dispatching that change... no longer pending." );
					Change->currentchange->flags.bPending = 0;
				}
			}
			if( changed && !Change->currentchange )
			{
				// no further changes expected.
				if( Change->flags.bExtended )
					Change->HandleChangeEx( Change->psv, NULL, 0, 0, 0, 0, 0 );
				else
					Change->HandleChange( Change->psv, NULL, 0 );
			}
			if( local_filemon.flags.bLog ) lprintf( "----------- NOW Files can be mared with CREATE ---------------" );
			Change->flags.bInitial = 0;
		}
		monitor->flags.bDispatched = 0;
		LeaveCriticalSec( &monitor->cs );
		if( monitor->flags.bEnd )
			EndMonitor( monitor );
		return changed;
	}
	else
	{
		PCHANGEHANDLER Change;
		// first, find if there are any changes
		for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
		{
			if( !IsQueueEmpty( &Change->PendingChanges ) )
				break;
		}
		// only if there are changes
		if( Change )
		{
			// wake the timer to send out changes (don't do it in a non timer thread, otherwise we'll be stuck a long time)
			RescheduleTimerEx( monitor->timer, 0 );
			return 1;
		}
	}
	return 0;
}

FILEMONITOR_PROC( int, DispatchChanges )( PMONITOR monitor )
{
	monitor->flags.bUserInterruptedChanges = 0;
	return DispatchChangesInternal( monitor, TRUE );
}

//-------------------------------------------------------------------------

void EndMonitorFiles( void )
{
	extern PMONITOR Monitors;
	while( Monitors )
		EndMonitor( Monitors );
}

//-------------------------------------------------------------------------

static void InvokeScan( PMONITOR monitor )
{
	if( local_filemon.flags.bLog ) lprintf( "Doing a scan... %d", monitor->DoScanTime );
	monitor->DoScanTime = 0;
	monitor->flags.bPendingScan = 0;
	monitor->flags.bUserInterruptedChanges = 0;
	//Log( "Delete all on monitor..." );
	if( !monitor->flags.bIntelligentUpdates )
	{
		DeleteAll( monitor ); // mark everything deleted
	//Log( "Scan directory..." );
		{
			PCHANGECALLBACK Change;

			for( Change = monitor->ChangeHandlers; Change; Change = Change->next )
				ScanDirectory( monitor, Change ); // look for all files which match mask(s)
		}
		//Log( "Scan files for status changes..." );
	}
#ifndef __LINUX__
	ScanMonitorFiles( monitor ); // check for changed size/time
#endif
	//Log( "Dispatch Changes... " );
	if( !monitor->flags.bUserInterruptedChanges )
	{
		DispatchChangesInternal( monitor, FALSE ); // announce any modified files..
	}
	else
	{
		if( local_filemon.flags.bLog ) 
			Log( "Can't report auto update - user interuppted..." );
	}
	if( local_filemon.flags.bLog )
		lprintf( "Did a scan... %d", monitor->DoScanTime );
}

void DoScan( PMONITOR monitor )
{
	uint32_t now = timeGetTime();
	// in critical section...
	if( local_filemon.flags.bLog )
		lprintf( "Tick...%s %p %d %d", monitor->directory, monitor, timeGetTime(), monitor->DoScanTime );
	if( monitor->DoScanTime && ( now > monitor->DoScanTime ) )
	{
		if( local_filemon.flags.bLog )
			lprintf( "... %p", monitor->monitors, monitor->flags.bPendingScan );
		if( monitor->flags.bPendingScan ) {
			if( local_filemon.flags.bLog )
				lprintf( "pending scan on monitor" );
			InvokeScan( monitor );
		}
		if( monitor->monitors )
		{
			PMONITOR sub_monitor;
			INDEX idx;
			LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
			{
				if( local_filemon.flags.bLog )
					lprintf( "Scan:%d", sub_monitor->DoScanTime );
				if( sub_monitor->DoScanTime )
				{
					if( now > sub_monitor->DoScanTime )
					{
						InvokeScan( sub_monitor );
					}
					else
					{
						if( monitor->DoScanTime < sub_monitor->DoScanTime )
						{
							// setting the do scan time here should not also set bPendingScan
							if( local_filemon.flags.bLog )
								lprintf( "updating monitor scantime to sub scan time" );
							monitor->DoScanTime = sub_monitor->DoScanTime;
						}
					}
				}
			}
			// there are sub-monitors which might actually be the ones scanning.
		}
	}
}

//-------------------------------------------------------------------------

void CPROC ScanTimer( uintptr_t monitor )
{
	if( !local_filemon.timer_thread )
		local_filemon.timer_thread = MakeThread();
	if( EnterCriticalSecNoWait( &((PMONITOR)monitor)->cs, NULL ) == 1 )
	{
		((PMONITOR)monitor)->flags.bScanning = 1;
		DoScan( (PMONITOR)monitor );
		((PMONITOR)monitor)->flags.bScanning = 0;
		if( ((PMONITOR)monitor)->flags.bEnd )
			EndMonitor( ((PMONITOR)monitor) );
		else
			LeaveCriticalSec( &((PMONITOR)monitor)->cs );
	}
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( void, EverybodyScan )( void )
{
	extern PMONITOR Monitors;
	PMONITOR monitor;
	for( monitor = Monitors; monitor; monitor = monitor->next )
	{
		MonitorForgetAll( monitor );
	}
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PCHANGEHANDLER, AddExtendedFileChangeCallback )( PMONITOR monitor
																		, CTEXTSTR mask
																		, EXTENDEDCHANGEHANDLER HandleChange
																		, uintptr_t psv )
{
	if( monitor && HandleChange )
	{
		PCHANGECALLBACK Change = GetFromSet( CHANGECALLBACK, &local_filemon.change_callback_set );
		EnterCriticalSec( &monitor->cs );
		Change->mask           = StrDup( mask );
		Change->currentchange  = NULL;
		Change->PendingChanges = NULL;
		Change->filelist       = CreateBinaryTreeExtended( BT_OPT_NODUPLICATES
																		 , CompareName
																		 , NULL
                                                       DBG_SRC);
		Change->psv            = psv;
		Change->flags.bExtended = 1;
		Change->flags.bInitial = 1; // created flag set on file monitors is cleared
		Change->HandleChangeEx = HandleChange;
		Change->monitor        = monitor;
		LinkThing( monitor->ChangeHandlers, Change );
		if( local_filemon.flags.bLog )
			lprintf( "Setting scan time to %" _32fs, timeGetTime() - 1 );
		monitor->DoScanTime = timeGetTime() + monitor->scan_delay;
		monitor->flags.bPendingScan = 1;
		{
			PMONITOR sub_monitor;
			INDEX idx;
			LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
				AddExtendedFileChangeCallback( sub_monitor, mask, HandleChange, psv );
		}
      ScanDirectory( monitor, Change ); // initial scan.
		LeaveCriticalSec( &monitor->cs );
		return Change;
	}
	return NULL;
}

//-------------------------------------------------------------------------

FILEMONITOR_PROC( PCHANGEHANDLER, AddFileChangeCallback )( PMONITOR monitor
                                                         , CTEXTSTR mask
                                              , CHANGEHANDLER HandleChange
                                              , uintptr_t psv )
{
	if( local_filemon.flags.bLog )
		lprintf("add change handler is %p %p", monitor, mask );
	if( monitor && HandleChange )
	{
		PCHANGECALLBACK Change = GetFromSet( CHANGECALLBACK, &local_filemon.change_callback_set );
		EnterCriticalSec( &monitor->cs );
		if( local_filemon.flags.bLog )
			lprintf( "adding change handler %s", mask?mask: "(null-mask)" );
		Change->mask           = StrDup( mask );
		Change->currentchange  = NULL;
		Change->PendingChanges = NULL;
		Change->filelist       = CreateBinaryTreeExtended( BT_OPT_NODUPLICATES
		                                                 , CompareName
		                                                 , NULL
                                                       DBG_SRC
                                                       );
		Change->psv            = psv;
		Change->flags.bExtended = 0;
		Change->flags.bInitial = 1; // created flag set on file monitors is cleared
		Change->HandleChange   = HandleChange;
		Change->monitor        = monitor;
		LinkThing( monitor->ChangeHandlers, Change );
		monitor->DoScanTime = timeGetTime() + monitor->scan_delay;
		monitor->flags.bPendingScan = 1;
		{
			PMONITOR sub_monitor;
			INDEX idx;
			LIST_FORALL( monitor->monitors, idx, PMONITOR, sub_monitor )
				AddFileChangeCallback( sub_monitor, mask, HandleChange, psv );
		}
      ScanDirectory( monitor, Change ); // initial scan.
		LeaveCriticalSec( &monitor->cs );
		return Change;
	}
	return NULL;
}

//-------------------------------------------------------------------

FILEMONITOR_PROC( void, SetFileLogging )( PMONITOR monitor, int enable )
{
	monitor->flags.bLogFilesFound = 1;
}

FILEMONITOR_PROC( void, SetFMonitorForceScanTime )( PMONITOR monitor, uint32_t delay )
{
	if( monitor )
		monitor->free_scan_delay = delay;
}

FILEMON_NAMESPACE_END

