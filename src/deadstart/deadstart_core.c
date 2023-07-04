#ifdef _WIN64
#ifndef __64__
#define __64__
#endif
#endif
#ifdef WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#endif

// debugging only gets you the ordering(priority) logging and something else...
// useful logging is now controlled with l.flags.bLog
#define DISABLE_DEBUG_REGISTER_AND_DISPATCH
//#define DEBUG_SHUTDOWN
#define LOG_ALL 0

//
// core library load
//    all procs scheduled, initial = 0
// Application starts, invokes preloads
//    additional libraries load, scheduling because of suspend
//    library load completes by invoking the newly registered list
// final core application schedulging happens, after initial preload completes
//    additional preload scheduligin happens( not suspended, is initial)
//#define DEBUG_CYGWIN_START
//#ifndef __LINUX__
#define IS_DEADSTART
#ifdef __LINUX__
#include <signal.h>
#endif

#include <stdhdrs.h>
#ifdef WIN32
#include <wincon.h> // GetConsoleWindow()
#endif
#include <sack_types.h>
#include <logging.h>
#include <deadstart.h>
#include <procreg.h>
#include <sqlgetoption.h>
#ifdef __NO_BAG__
#undef lprintf
#define lprintf printf
#define BAG_Exit exit
#else
#endif

#include "deadstart_core.h"

//#define lprintf(f,...) printf(f "\n",##__VA_ARGS__)
//#define _lprintf(n) lprintf

#ifdef UNDER_CE
#define LockedExchange InterlockedExchange
#endif

#ifdef __cplusplus
extern "C" {
#endif
	IMPORT_METHOD struct deadstart_local_data_* GetDeadstartSharedGlobal( void );
#ifdef __cplusplus
}
#endif

SACK_DEADSTART_NAMESPACE

//#undef PRELOAD

EXPORT_METHOD void RunDeadstart( void );



#ifdef __STATIC_GLOBALS__
extern 
#ifdef __cplusplus
  "C"
#endif
	 struct deadstart_local_data_ deadstart_local_data;
#define l (deadstart_local_data)
#else
static struct deadstart_local_data_ *deadstart_local_data;
#define l (*deadstart_local_data)
#endif

EXPORT_METHOD void RunExits( void )
{
	//fprintf( stderr, "Run Exits InvokeExits()\n" );
	InvokeExits();
}

static void InitLocal( void )
{
#ifndef __STATIC_GLOBALS__
	if( !deadstart_local_data ) deadstart_local_data = GetDeadstartSharedGlobal();
#endif
	if( !l.flags.bInitialized )
	{
		l.flags.bInitialized = 1;
	}
}

#ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
#define ENQUE_STARTUP_DBG_SRC DBG_SRC
static void EnqueStartupProc( PSTARTUP_PROC *root, PSTARTUP_PROC proc DBG_PASS )
#else
#define ENQUE_STARTUP_DBG_SRC
static void EnqueStartupProc( PSTARTUP_PROC *root, PSTARTUP_PROC proc )
#endif
{
	PSTARTUP_PROC check;
	PSTARTUP_PROC last;

		if( proc->next || proc->me )
		{
			if( ( (*proc->me) = proc->next ) )
				proc->next->me = proc->me;
		}
		for( last = check = (*root); check; check = check->next )
		{
			// if the current one being added is less then the one in the list
			// then the one in the list becomes the new one's next...
			if( proc->priority < check->priority )
			{
#ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
				_lprintf(DBG_RELAY)( "%s(%d) is to run before %s and after %s first is %s"
						 , proc->func
						 , proc - procs
						 , check->func
						 , (check->me==root)?"Is First":((PSTARTUP_PROC)check->me)->func
						 , (*root)?(*root)->func:"First"
						 );
#endif
				proc->next = check;
				proc->me = check->me;
				(*check->me) = proc;
				check->me = &proc->next;
				break;
			}
			last = check;
		}
		if( !check )
		{
#ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
			lprintf( "%s(%d) is to run after all"
					 , proc->func
					 , proc - procs
					 );
#endif
			proc->next = NULL;
			if( last )
			{
				last->next = proc;
				proc->me = &last->next;
			}
			else
			{
				(*root) = proc;
				proc->me = root;
			}
		}
}

// parameter 4 is just used so the external code is not killed
// we don't actually do anything with this?
void RegisterPriorityStartupProc( void (CPROC*proc)(void), CTEXTSTR func,int priority, void *use_label DBG_PASS )
{
	int use_proc;
	InitLocal();
	if( LOG_ALL || (
#ifndef __STATIC_GLOBALS__
		 deadstart_local_data && 
#endif
		  l.flags.bLog ))
		lprintf( "Register %s@" DBG_FILELINEFMT_MIN " %d", func DBG_RELAY, priority);
	if( nProcs == 1024 )
	{
		for( use_proc = 0; use_proc < 1024; use_proc++ )
			if( !procs[use_proc].bUsed )
				break;
		if( use_proc == 1024 )
		{
			lprintf( "Used all 1024, and, have 1024 startups total scheduled." );
			DebugBreak();
		}
	}
	else
		use_proc = nProcs;

	procs[use_proc].proc = proc;
	procs[use_proc].func = func;
#ifdef _DEBUG
	procs[use_proc].file = pFile;
	procs[use_proc].line = nLine;
#endif
	procs[use_proc].priority = priority;
	procs[use_proc].bUsed = 1;
	procs[use_proc].next = NULL; // initialize so it doesn't try unlink in requeue common routine.
	procs[use_proc].me = NULL; // initialize so it doesn't try unlink in requeue common routine.

	EnqueStartupProc( &proc_schedule, procs + use_proc ENQUE_STARTUP_DBG_SRC );

	if( nProcs < 1024 )
		nProcs++;
	/*
	if( nProcs == 1024 )
	{
		lprintf( "Excessive number of startup procs!" );
		DebugBreak();
	}
	*/
	if( bInitialDone && !bSuspend )
	{
#ifdef _DEBUG
		_xlprintf(LOG_NOISE,pFile,nLine)( "Initial done, not suspended, dispatch immediate." );
#endif
		InvokeDeadstart();
	}
	//lprintf( "Total procs %d", nProcs );
}

#ifdef __LINUX__
// this handles the peculiarities of fork() and exit()
void ClearDeadstarts( void )
{
	// this is reserved for the sole use of
	// fork() success and then exec() failing...
	// when oh wait - __attribute__((destructor))
	// if( registered_pid != getppid() )
	shutdown_proc_schedule = NULL;
	// be rude - yes we lose resources. but everything goes away cause
	// this is just a clone..
}
#endif

#ifndef UNDER_CE
#  if defined( WIN32 )

static int ignoreBreak;
void IgnoreBreakHandler( int ignore) {
	ignoreBreak = ignore;
}

#    ifndef __cplusplus_cli
static BOOL WINAPI CtrlC( DWORD dwCtrlType )
{
	if( ignoreBreak & ( 1 << dwCtrlType ) ) return TRUE;
	fprintf( stderr, "Received ctrlC Event %08x %d\n", ignoreBreak, dwCtrlType );
	switch( dwCtrlType )
	{
	case CTRL_BREAK_EVENT:
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		InvokeExits();
		// allow C api to exit, whatever C api we're using
		// (allows triggering atexit functions)
		//exit(3);
		break;
	}
	// default... return not processed.
	return FALSE; // allow others to process this too
}
#    endif
#  endif

#  ifndef WIN32
static void CtrlC( int signal )
{
	exit(3);
}
#  endif
#endif

// wow no such thing as static-izing this... it's
// always retrieved with dynamic function loading, therefore
// MUST be exported if at all possible.
// this one is used when a library is loaded.
void InvokeDeadstart( void )
{
	PSTARTUP_PROC proc;
	PSTARTUP_PROC resumed_proc;
#ifndef __STATIC_GLOBALS__
	if( !deadstart_local_data ) return; // nothing was registerd to run.
#endif

	if( bInitialStarted )
		return;
	bInitialStarted = 1;
	// allowing initial start to be set lets final resume do this invoke.
	if( bSuspend )
	{
		if( l.flags.bLog )
			lprintf( "Suspended, first proc is %s", proc_schedule?proc_schedule->func:"No First" ); //-V595
		return;
	}
#ifdef WIN32
	if( !bInitialDone && !l.bDispatched )
	{
#  ifndef UNDER_CE
		if( GetConsoleWindow() )
		{
			if( !SetConsoleCtrlHandler( CtrlC, TRUE ) ) fprintf( stderr, "failed to SetConsoleCtrlHandler? %d\n", GetLastError() );
		}
		else
		{
			//MessageBox( NULL, "!!--!! NO CtrlC", "blah", MB_OK );
			// do nothing if we're no actually a console window. this should fix ctrl-c not working in CMD prompts launched by MILK/InterShell
		}
#  endif
	}
#endif

	while( ( proc = (PSTARTUP_PROC)LockedExchangePtrSzVal( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
	{
		// need to set this to point to new head of list... it's not in proc_schedule anymore
		//proc->me = &proc;
		if( LOG_ALL || (
#ifndef __STATIC_GLOBALS__
		   deadstart_local_data  &&
#endif
		   l.flags.bLog ))
		{
#ifdef _DEBUG
			lprintf( "Dispatch %s@%s(%d)p:%d ", proc->func,proc->file,proc->line, proc->priority );
#else
			lprintf( "Dispatch %s@p:%d ", proc->func, proc->priority );
#endif
		}
		{
			l.bDispatched = 1;
#ifdef _DEBUG
			if( proc->proc
#  ifndef __LINUX__
#    if  __WATCOMC__ >= 1280
				&& !IsBadCodePtr( (int(STDCALL*)(void))proc->proc )
#    elif defined( __64__ )
				&& !IsBadCodePtr( (FARPROC)proc->proc )
#    else
//				&& !IsBadCodePtr( (int STDCALL(*)(void))proc->proc )
#    endif
#  endif
			  )
#endif
			{
				proc->proc();
			}
			proc->bUsed = 0;
			l.bDispatched = 0;
		}
		// look to see if anything new was scheduled.  Grab the list, add it to the one's we're processing.
		{
			{
				PSTARTUP_PROC newly_scheduled_things;
				proc->me = &proc;
				resumed_proc = proc;
				if( ( newly_scheduled_things = (PSTARTUP_PROC)LockedExchangePtrSzVal( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
				{
					newly_scheduled_things->me = &newly_scheduled_things;
					//lprintf( "------------------  newly scheduled startups; requeue old startups into new list ------------------ " );
					while( newly_scheduled_things )
					{
						EnqueStartupProc( &proc, newly_scheduled_things ENQUE_STARTUP_DBG_SRC );
					}
				}
				else
					resumed_proc = NULL;
			}
			proc_schedule = proc;
			proc_schedule->me = &proc_schedule;
		}
		if( resumed_proc )
			UnlinkThing( resumed_proc );
		else
			UnlinkThing( proc );
	}
	bInitialStarted = 0;
}

void MarkRootDeadstartComplete( void )
{
	bInitialDone = 1;
}

#ifndef __NO_OPTIONS__
// options initializes at SQL+1
PRIORITY_PRELOAD( InitDeadstartOptions, NAMESPACE_PRELOAD_PRIORITY+1 )
{
#ifdef DISABLE_DEBUG_REGISTER_AND_DISPATCH
#  ifndef __NO_OPTIONS
	l.flags.bLog = SACK_GetProfileIntEx( "SACK/Deadstart", "Logging Enabled?", 0, TRUE );
#  else
	l.flags.bLog = 0;
#  endif
#else
	l.flags.bLog = 1;
#endif
}
#endif

// parameter 4 is just used so the external code is not killed
// we don't actually do anything with this?
void RegisterPriorityShutdownProc( void (CPROC*proc)(void), CTEXTSTR func, int priority,void *use_label DBG_PASS )
{
	InitLocal();
	if( LOG_ALL || (
#ifndef __STATIC_GLOBALS__
		deadstart_local_data &&
#endif
		   l.flags.bLog ))
		lprintf( "Exit Proc %s(%p) from " DBG_FILELINEFMT_MIN " registered..."
				 , func
				 , proc DBG_RELAY );
	shutdown_procs[nShutdownProcs].proc = proc;
	shutdown_procs[nShutdownProcs].func = func;
#ifdef _DEBUG
	shutdown_procs[nShutdownProcs].file = pFile;
	shutdown_procs[nShutdownProcs].line = nLine;
#endif
	shutdown_procs[nShutdownProcs].bUsed = 1;
	shutdown_procs[nShutdownProcs].priority = priority;
	{
		PSHUTDOWN_PROC check;
		for( check = shutdown_proc_schedule; check; check = check->next )
		{
			if( shutdown_procs[nShutdownProcs].priority >= check->priority )
			{
#ifdef DEBUG_SHUTDOWN
				lprintf( "%s(%d) is to run before %s(%d) %s"
						 , shutdown_procs[nShutdownProcs].func
						 , nShutdownProcs
						 , check->file
						 , check->line
						 , check->func );
#endif
				shutdown_procs[nShutdownProcs].next = check;
				shutdown_procs[nShutdownProcs].me = check->me;
				(*check->me) = shutdown_procs + nShutdownProcs;
				check->me = &shutdown_procs[nShutdownProcs].next;
				break;
			}
		}
		if( !check )
			LinkLast( shutdown_proc_schedule, PSHUTDOWN_PROC, shutdown_procs + nShutdownProcs );
		//lprintf( "first routine is %s(%d)"
		//		 , shutdown_proc_schedule->func
		//		 , shutdown_proc_schedule->line );
	}
	nShutdownProcs++;
	//lprintf( "Total procs %d", nProcs );
}

void InvokeExits( void )
{
	// okay well since noone previously scheduled exits...
	// this runs a prioritized list of exits - all within
	// a single moment of exited-ness.
	PSHUTDOWN_PROC proc;
	// shutdown is much easier than startup cause more
	// procedures shouldn't be added as a property of shutdown.
	//fprintf( stderr, "InvokeExits()\n" );
	// don't allow shutdown procs to schedule more shutdown procs...
	// although in theory we could; if the first list contained
	// ReleaseAllMemory(); then there is no memory.
	if( 
#ifndef __STATIC_GLOBALS__
		deadstart_local_data &&
#endif
			( proc = (PSHUTDOWN_PROC)LockedExchangePtrSzVal( (PVPTRSZVAL)&shutdown_proc_schedule, 0 ) ) != NULL
		  )
	{
		// just before all memory goes away
		// global memory goes away (including mine) so deadstart_local_data is invalidated.
#ifndef __STATIC_GLOBALS__
		struct deadstart_local_data_ *local_pointer = (struct deadstart_local_data_*)(((uintptr_t)deadstart_local_data)-sizeof(PLIST));
#endif
		PSHUTDOWN_PROC proclist = proc;
		// link list to myself..
#ifndef __STATIC_GLOBALS__
		Hold( local_pointer );
#endif
		proc->me = &proclist;
		while( ( proc = proclist ) )
		{
#if defined( DEBUG_SHUTDOWN )
			lprintf( "Exit Proc %s(%p)(%d) priority %d from %s(%d)..."
			       , proc->func
			       , proc->proc
			       , proc - shutdown_procs
			       , proc->priority
			       , proc->file
			       , proc->line );
#endif
			if( proc->priority == 0 )
			{
				//atexit( proc->proc );
				//continue;
			}
			// don't release this stuff... memory might be one of the autoexiters.
			UnlinkThing( proc );
			if( proc->proc
#ifndef __LINUX__
				&& !IsBadCodePtr( (FARPROC)proc->proc )
#endif
			  )
			{
#ifdef DEBUG_SHUTDOWN
				lprintf( "Dispatching..." );
#endif
				proc->proc();
			}
			// okay I have the whol elist... so...
#ifdef DEBUG_SHUTDOWN
			lprintf( "Okay and that's done... next is %p %p", proclist, shutdown_proc_schedule );
#endif
		}
		// nope by this time memory doesn't exist anywhere.
		//Release( local_pointer );
		//shutdown_proc_schedule = proc;
	}
#ifndef __STATIC_GLOBALS__
	deadstart_local_data = (struct deadstart_local_data_*)NULL;
#endif
	//shutdown_proc_schedule = NULL;
}

void DispelDeadstart( void )
{
	shutdown_proc_schedule = NULL;
}

#ifdef __cplusplus

ROOT_ATEXIT(AutoRunExits)
{
	InvokeExits();
}

#endif


void SuspendDeadstart( void )
{
	bSuspend++;
}
void ResumeDeadstart( void )
{
	bSuspend--;
	if( !bSuspend )
	{
		if( bInitialDone )
			InvokeDeadstart();
	}
}

SACK_DEADSTART_NAMESPACE_END
SACK_NAMESPACE

// linked into BAG to provide a common definition for function Exit()
// this then invokes an exit in the mainline program (if available)
void BAG_Exit( int code )
{
	fprintf( stderr, "BAG_Exit();" );
	InvokeExits();
#undef exit
	exit( code );
}

// legacy linking code - might still be usin this for linux...
LOGICAL is_deadstart_complete( void )
{
	//extern uint32_t deadstart_complete;
#ifndef __STATIC_GLOBALS__
	if( deadstart_local_data )
		return bInitialDone;//deadstart_complete;
#endif
	return FALSE;
}

SACK_NAMESPACE_END
SACK_DEADSTART_NAMESPACE

LOGICAL IsRootDeadstartStarted( void )
{
#ifndef __STATIC_GLOBALS__
	if( deadstart_local_data )
		return bInitialStarted;
	return 0;
#else
	return bInitialStarted;
#endif
}

LOGICAL IsRootDeadstartComplete( void )
{
#ifndef __STATIC_GLOBALS__
	if( deadstart_local_data )
		return bInitialDone;
	return 0;
#else
	return bInitialDone;
#endif
}

#ifndef __STATIC__
#  ifndef __WATCOMC__
#    if !defined( NO_DEADSTART_DLLMAIN ) && !defined( BUILD_PORTABLE_EXECUTABLE )
#      if !defined( __LINUX__ ) && !defined( __GNUC__ )
#        ifdef __cplusplus
extern "C"
#        endif
__declspec(dllexport) 
	BOOL WINAPI DllMain(  HINSTANCE hinstDLL,
   DWORD fdwReason,
   LPVOID lpvReserved
  		 )
{

	if( fdwReason == DLL_PROCESS_DETACH ) {
		fprintf( stderr, "DLL_DETACH\n" );
		InvokeExits();
	}
	return TRUE;
}
#      else
void RootDestructor(void) __attribute__((destructor));
void RootDestructor( void )
{
	fprintf( stderr, "RootDestructor\n" );
	InvokeExits();
}
#      endif
#    endif
#  endif
#endif

#undef l

SACK_DEADSTART_NAMESPACE_END
