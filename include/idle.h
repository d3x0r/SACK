
#ifndef IDLE_FUNCTIONS_DEFINED
#define IDLE_FUNCTIONS_DEFINED
#include <sack_types.h>
#if !defined(__STATIC__) && !defined( __UNIX__ )
# ifdef IDLE_SOURCE
#  define IDLE_PROC(type,name) EXPORT_METHOD type CPROC name
# else
#  define IDLE_PROC(type,name) IMPORT_METHOD type CPROC name
# endif
#else
# ifdef IDLE_LIBRARY_SOURCE
#  define IDLE_PROC(type,name) type name
# else
#  define IDLE_PROC(type,name) extern type name
# endif
#endif
#ifdef __cplusplus
namespace sack {
	namespace timers {
#endif
// return -1 if not the correct thread
// return 0 if no events processed
// return 1 if events were processed
typedef int (CPROC *IdleProc)(PTRSZVAL);

IDLE_PROC( void, AddIdleProc )( IdleProc Proc, PTRSZVAL psvUser );
IDLE_PROC( int, RemoveIdleProc )( IdleProc Proc );

IDLE_PROC( int, Idle )( void );
IDLE_PROC( int, IdleFor )( _32 dwMilliseconds );
#ifdef __cplusplus
	};//	namespace timers {
};//namespace sack {
using namespace sack::timers;
#endif

#endif
