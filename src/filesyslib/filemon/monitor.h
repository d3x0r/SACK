
#ifndef MONITOR_HEADER_INCLUDED
#define MONITOR_HEADER_INCLUDED

#ifndef _WIN32
#define MAX_PATH 256 // protocol limit...
#endif

#include <stdhdrs.h>
#include <sack_types.h>
#include <timers.h>
#include <filemon.h>
#include <sys/types.h> // time_t

FILEMON_NAMESPACE

typedef struct filemonitor_tag {
	TEXTCHAR   name[320]; // MAX_PATH is inconsistantly defined
	TEXTSTR  filename; // just the filename in the name
	struct {
		BIT_FIELD   bDirectory : 1;
		BIT_FIELD   bToDelete : 1;
		BIT_FIELD   bScanned : 1; // if set - don't requeue, set when added to the changed queue, and cleared when done...
		BIT_FIELD   bCreated : 1;
		BIT_FIELD   bPending : 1; // set when enqueued in pending list, and cleared after event dispatched.
	} flags;
	// if GetTickCount() - ScannedAt > monitor->scan_delay queue for change
	uint32_t ScannedAt; // time this file was scanned
#ifdef __cplusplus_cli
	gcroot<System::DateTime^> lastmodifiedtime;
#else
#  ifdef WIN32
	FILETIME lastmodifiedtime;
#  else
	time_t lastmodifiedtime;
#  endif
#endif
	uint64_t    lastknownsize;
	CRITICALSECTION cs;
} FILEMON;

#define MAXFILEMONSPERSET 256
DeclareSet( FILEMON );

typedef struct mytime_tag {
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} MYTIME, *PMYTIME;

typedef struct filechangecallback_tag
{
	CTEXTSTR mask;
	PFILEMON currentchange;
	PLINKQUEUE PendingChanges;
	PTREEROOT filelist;
	struct {
		BIT_FIELD bExtended : 1;
		BIT_FIELD bInitial : 1; // first time files are scanned, they are not created.
	} flags;
	struct monitor_tag *monitor;
	union {
		CHANGEHANDLER HandleChange;
		EXTENDEDCHANGEHANDLER HandleChangeEx;
	};
	uintptr_t psv;
	DeclareLink( struct filechangecallback_tag );
} CHANGECALLBACK, *PCHANGECALLBACK;

#define MAXCHANGECALLBACKSPERSET 64
DeclareSet( CHANGECALLBACK );

typedef struct monitor_tag
{
	CRITICALSECTION cs;
	PTHREAD pThread;
	struct {
		BIT_FIELD bDispatched : 1;
		BIT_FIELD bEnd : 1;
		BIT_FIELD bClosing : 1;
		BIT_FIELD bUserInterruptedChanges : 1;
		BIT_FIELD bScanning : 1;
		BIT_FIELD bLogFilesFound : 1;
		BIT_FIELD bAddedToEvents : 1;
		BIT_FIELD bRemoveFromEvents : 1;
		BIT_FIELD bRemovedFromEvents : 1;
		BIT_FIELD bPendingScan : 1; // also set - added when montitors have sub-monitors; the parent monitor gets a tick, but might not scan.
		BIT_FIELD bRecurseSubdirs : 1;
		BIT_FIELD bIntelligentUpdates : 1;
	} flags;
#ifdef _WIN32
	HANDLE hChange;
#else
	int fdMon;
#endif
	PLIST monitors; // other monitors being tracked if SUBCURSE was used
	PMONITOR parent_monitor; // if this is in the list, it also references its root

    // this includes updates to directories and files
	 TEXTCHAR directory[MAX_PATH];
    uint32_t DoScanTime;
    uint32_t timer;
	 uint32_t scan_delay;
    uint32_t free_scan_delay;
	 PCHANGECALLBACK ChangeHandlers;
	 DeclareLink( struct monitor_tag );
    PFILEMON last_rename_file;
} MONITOR;

#ifndef PRIMARY_FILEMON_SOURCE
extern
#endif
struct local_filemon {
	struct {
		BIT_FIELD bInit : 1;
		BIT_FIELD bLog : 1;
		BIT_FIELD bAddedMonitor : 1;
	} flags;
	PTHREAD timer_thread;
	PTHREAD directory_monitor_thread;
#ifdef _WIN32
	HANDLE hMonitorThreadControlEvent;
#endif
	PCHANGECALLBACKSET change_callback_set;
	PFILEMONSET filemon_set;
} local_filemon;
#define l local_filemon


void CPROC ScanTimer( uintptr_t monitor );

PFILEMON NewFile( PCHANGEHANDLER, CTEXTSTR name );
void CloseFileMonitors( PMONITOR monitor );
void ScanDirectory( PMONITOR monitor );
PFILEMON WatchingFile( PCHANGEHANDLER monitor, CTEXTSTR name );

FILEMON_NAMESPACE_END


#endif
//-------------------------------------------------------------------
// Branched from bingocore.  Also console barbingo/relay (a little more robust)
// $Log: monitor.h,v $
// Revision 1.11  2005/02/23 13:01:35  panther
// Fix scrollbar definition.  Also update vc projects
//
// Revision 1.10  2004/12/01 23:15:58  panther
// Extend file monitor to make forced scan timeout settable by the application
//
// Revision 1.9  2004/01/16 16:57:48  d3x0r
// Added logging, and an extern anable per monitor for file logging
//
// Revision 1.8  2004/01/07 09:46:18  panther
// Cleanup warnings, missing prototypes...
//
// Revision 1.7  2003/11/09 22:30:09  panther
// Stablity fixes... don't End while scanning for instance
//
// Revision 1.6  2003/11/04 11:40:00  panther
// Mark per file updates - a directory may continually get updates
//
// Revision 1.5  2003/11/04 09:20:49  panther
// Modify call method to watch directory and supply seperate masks per change routine
//
// Revision 1.4  2003/11/04 02:08:17  panther
// Reduced messages, investigated close.  If EndMonitor is called, the thread will be awoken and killed
//
// Revision 1.3  2003/11/04 01:46:43  panther
// Remove some unused members, remove tree function
//
// Revision 1.2  2003/11/04 01:28:51  panther
// Fixed most holes with the windows change monitoring system
//
// Revision 1.1  2003/11/03 23:01:43  panther
// Initial commit of librarized filemonitor
//
//
