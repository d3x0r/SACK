#ifndef SERVICE_HOOK_DEFINED
#define SERVICE_HOOK_DEFINED

#include <sack_system.h>

#ifdef SERVICE_SOURCE
#define SERVICE_EXPORT EXPORT_METHOD
#else
#define SERVICE_EXPORT IMPORT_METHOD
#endif

#ifdef __cplusplus
#define _SERVICE_NAMESPACE namespace service {
#define _SERVICE_NAMESPACE_END }
#else
#define _SERVICE_NAMESPACE
#define _SERVICE_NAMESPACE_END
#endif
#define SERVICE_NAMESPACE SACK_NAMESPACE _SYSTEM_NAMESPACE _SERVICE_NAMESPACE
#define SERVICE_NAMESPACE_END SACK_NAMESPACE_END _SYSTEM_NAMESPACE_END _SERVICE_NAMESPACE_END


SACK_NAMESPACE
   _SYSTEM_NAMESPACE
	_SERVICE_NAMESPACE

// setup a service of a certain name.  The start routine is called when the service begins.
// the start routine should be treated as an initialization routine, and return in a short amount of time.
SERVICE_EXPORT void SetupService( TEXTSTR name, void (CPROC*Start)(void) );

// setup a service of a certain name.  The start routine is called when the service begins.
// the stop routine is called when the service is notified for shutdown.
	// the start routine should be treated as an initialization routine, and return in a short amount of time.
SERVICE_EXPORT void SetupServiceEx( TEXTSTR name, void (CPROC*Start)( void ), void (CPROC*Stop)( void ) );

// setup a service of a certain name.  The startthread begins when the service starts.
// the start routine thread can wait indefinately.
SERVICE_EXPORT void SetupServiceThread( TEXTSTR name, PTRSZVAL (CPROC*Start)( PTHREAD ), PTRSZVAL psv );

// install the current executable as a service of the specified name.  (and starts the service)
SERVICE_EXPORT void ServiceInstall( CTEXTSTR ServiceName );

// uninstall the current executable as a service of the specified name.  (and stops the service)
SERVICE_EXPORT void ServiceUninstall( CTEXTSTR ServiceName );

// This is a method attempting to activate processes under other user contexts using impersonation.
// this is really a function that belongs in system.h; but is here because it's utility is inherantly
// for services.
SERVICE_EXPORT PTASK_INFO LaunchUserProcess( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
									 , int flags
									 , TaskOutput OutputHandler
									 , TaskEnd EndNotice
									 , PTRSZVAL psv
									  DBG_PASS
														 );

// This returns TRUE if we started as a service.
// This should be valid a short time after the process starts... but probably not in PRELOAD states.
SERVICE_EXPORT LOGICAL IsThisAService( void );

	_SERVICE_NAMESPACE_END
   _SYSTEM_NAMESPACE_END
SACK_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::system::service;
#endif

#endif
