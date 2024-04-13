/* \File Monitor interfaces. This monitors the file system for
   changes to files.                                           */
#ifndef FILE_MONITOR_LIBRARY_DEFINED
#define FILE_MONITOR_LIBRARY_DEFINED

#ifdef FILEMONITOR_SOURCE
#define FILEMONITOR_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define FILEMONITOR_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

// filemon will require system features, so pull stdhdrs instead of
// just sack_tyeps
#include <stdhdrs.h>

// for namespace definitions
#include <filesys.h>  

#ifdef __cplusplus
namespace sack {
	namespace filesys {
		namespace monitor {
#endif

typedef struct monitor_tag *PMONITOR;
typedef struct filechangecallback_tag *PCHANGEHANDLER;
typedef struct filemonitor_tag *PFILEMON;

// bTree monitors this path and all sub-paths...
// (in theory)
// mask is a file mask to match - supports DOS style * and ?
FILEMONITOR_PROC(PMONITOR, MonitorFiles )( CTEXTSTR directory, int scan_delay );

// if monitor_file_flag_subcurse is used, every subdirectory gets a notification for every file in it.
// the default scan of '*' is used.
// make sure we use the same symbol as the filescan so we can just pass that on.
#define MONITOR_FILE_FLAG_SUBCURSE SFF_SUBCURSE
FILEMONITOR_PROC( PMONITOR, MonitorFilesEx )( CTEXTSTR directory, int scan_delay, int flags );

// end a single monitor....
FILEMONITOR_PROC( void, EndMonitor )( PMONITOR monitor );
// close all file monitors....
FILEMONITOR_PROC(void, EndMonitorFiles )( void );

// Log files read in the directory for this monitor.  Log matches per handler
FILEMONITOR_PROC( void, SetFileLogging )( PMONITOR monitor, int enable );

// return TRUE if okay to get next, return FALSE to
// stop processing and wait until next...
typedef int (CPROC *CHANGEHANDLER)(uintptr_t psv
											 , CTEXTSTR filepath
											 , int bDeleted);

FILEMONITOR_PROC( PCHANGEHANDLER, AddFileChangeCallback )( PMONITOR monitor
                                               , CTEXTSTR mask
															  , CHANGEHANDLER HandleChange
															  , uintptr_t psv );

typedef int (CPROC *EXTENDEDCHANGEHANDLER)( uintptr_t psv
														, CTEXTSTR filepath
														, uint64_t size
														, uint64_t time
														, LOGICAL bCreated // file has just now been created.
                                          , LOGICAL bDirectory // it's a directory (add another monitor?)
														, LOGICAL bDeleted); // file was just now deleted.

FILEMONITOR_PROC( PCHANGEHANDLER, AddExtendedFileChangeCallback )( PMONITOR monitor
																		, CTEXTSTR mask
																		, EXTENDEDCHANGEHANDLER HandleChange
																		, uintptr_t psv );


FILEMONITOR_PROC( PFILEMON, AddMonitoredFile )( PCHANGEHANDLER Change, CTEXTSTR name );
FILEMONITOR_PROC( void, EverybodyScan )( void );
FILEMONITOR_PROC( void, MonitorForgetAll )( PMONITOR monitor );
FILEMONITOR_PROC( void, SetFMonitorForceScanTime )( PMONITOR monitor, uint32_t delay );

// returns 0 if no changed were pending, else returns number
// of changes dispatched (not nessecarily handled)
FILEMONITOR_PROC( int, DispatchChanges )( PMONITOR monitor );


FILEMON_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::filesys::monitor;
#endif


#endif

