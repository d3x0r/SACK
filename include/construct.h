/* Defines interface for Construct API.
   Description
   This API is for distributed process tracking. A launching
   program will receive notifications to cause certain events to
   happen. Applications built for use by this execution tracking
   program will register that they are loading while they are
   loading, and before the application Main() is invoked. the
   application should then call LoadComplete() once they have
   initialized and are ready to process. This allows a
   quick-wait to wait for the process to register that it is
   loading, and a longer wait for process completion. Certain
   processes may not require others to be completely loaded, but
   maybe just loading. (Two peer processes that have to
   coordinate together to have either one complete
   initialization).                                              */
#include <sack_types.h>

/* Define the procedure call type for construct API methods. */
#define CONSTRUCT_API CPROC
#ifdef CONSTRUCT_SOURCE
#define CONSTRUCT_PROC EXPORT_METHOD
#else
/* Library linkage specification. */
#define CONSTRUCT_PROC IMPORT_METHOD
#endif

#ifdef __cplusplus
/* Defines TASK namespace (unused?) */
#define _TASK_NAMESPACE namespace task {
/* Define Construct namespace. Construct is for distributed
   process tracking project. Applications will register on-load
   that they are loading, and should register load completed
   when they are done loading, or exit.                         */
#define _CONSTRUCT_NAMESPACE namespace construct {
/* Defines TASK namespace ending.(unused?) */
#define _TASK_NAMESPACE_END }
/* Define Construct namespace end. Construct is for distributed
   process tracking project. Applications will register on-load
   that they are loading, and should register load completed
   when they are done loading, or exit.                         */
#define _CONSTRUCT_NAMESPACE_END }
#else
#define _TASK_NAMESPACE 
#define _CONSTRUCT_NAMESPACE 
#define _TASK_NAMESPACE_END
#define _CONSTRUCT_NAMESPACE_END
#endif

/* Define a symbol to specify full sack::task::construct
   namespace.                                            */
#define CONSTRUCT_NAMESPACE SACK_NAMESPACE _TASK_NAMESPACE _CONSTRUCT_NAMESPACE
/* Define a symbol to specify full sack::task::construct
   namespace ending.                                     */
#define CONSTRUCT_NAMESPACE_END _CONSTRUCT_NAMESPACE_END _TASK_NAMESPACE_END SACK_NAMESPACE_END

	SACK_NAMESPACE
	_TASK_NAMESPACE
	/* Registers with message service, assuming the summoner message service is active.
	 Provides communication methods with a task manager, so the application can notify,
	 start has completed.   The service is ready to work.*/
_CONSTRUCT_NAMESPACE

/* Called to indicate that a process is done initializing and is
   ready to process. Notifies summoner service of Loading
   completed. If enabled, there is also a library component that
   will run at deadstart to just confirm initializing, this
   would actually indicate the service is now ready to serve.    */
CONSTRUCT_PROC void CONSTRUCT_API LoadComplete( void );

CONSTRUCT_NAMESPACE_END

#ifdef __cplusplus
	using namespace sack::task::construct;
#endif

