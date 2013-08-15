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


#ifdef UNDER_CE
#define LockedExchange InterlockedExchange
#endif

SACK_DEADSTART_NAMESPACE

#undef PRELOAD

EXPORT_METHOD void RunDeadstart( void );

typedef struct startup_proc_tag {
	DeclareLink( struct startup_proc_tag );
	int bUsed;
   int priority;
	void (CPROC*proc)(void);
   CTEXTSTR func;
	CTEXTSTR file;
   int line;
} STARTUP_PROC, *PSTARTUP_PROC;

typedef struct shutdown_proc_tag {
	DeclareLink( struct shutdown_proc_tag );
	int bUsed;
   int priority;
	void (CPROC*proc)(void);
   CTEXTSTR func;
	CTEXTSTR file;
   int line;
} SHUTDOWN_PROC, *PSHUTDOWN_PROC;

struct deadstart_local_data_
{
	// this is a lot of procs...
	int nShutdownProcs;
#define nShutdownProcs l.nShutdownProcs
	SHUTDOWN_PROC shutdown_procs[512];
#define shutdown_procs l.shutdown_procs
	int bInitialDone;
#define bInitialDone l.bInitialDone
	LOGICAL bInitialStarted;
#define bInitialStarted l.bInitialStarted
   int bSuspend;
#define bSuspend l.bSuspend
	int bDispatched;
#define bDispatched l.bDispatched

	PSHUTDOWN_PROC shutdown_proc_schedule;
#define shutdown_proc_schedule l.shutdown_proc_schedule

	int nProcs; // count of used procs...
#define nProcs l.nProcs
	STARTUP_PROC procs[1024];
#define procs l.procs
	PSTARTUP_PROC proc_schedule;
#define proc_schedule l.proc_schedule
	struct
	{
		BIT_FIELD bInitialized : 1;
		BIT_FIELD bLog : 1;
	} flags;
};

#ifdef UNDER_CE
#  ifndef __STATIC_GLOBALS__
#    define __STATIC_GLOBALS__
#  endif
#endif

#ifndef __STATIC_GLOBALS__
static struct deadstart_local_data_ *deadstart_local_data;
#define l (*deadstart_local_data)
#else
static struct deadstart_local_data_ deadstart_local_data;
#define l (deadstart_local_data)
#endif

EXPORT_METHOD void RunExits( void )
{
   InvokeExits();
}

static void InitLocal( void )
{
#ifndef __STATIC_GLOBALS__
	if( !deadstart_local_data )
	{
		SimpleRegisterAndCreateGlobal( deadstart_local_data );
	}
#endif
	if( !l.flags.bInitialized )
	{
		//atexit( RunExits );
		l.flags.bInitialized = 1;
	}
}

void EnqueStartupProc( PSTARTUP_PROC *root, PSTARTUP_PROC proc )
{
	PSTARTUP_PROC check;
	PSTARTUP_PROC last;

		if( proc->next || proc->me )
		{
			if( (*proc->me) = proc->next )
				proc->next->me = proc->me;
		}
		for( last = check = (*root); check; check = check->next )
		{
		// if the current one being added is less then the one in the list
         // then the one in the list becomes the new one's next...
			if( proc->priority < check->priority )
			{
#ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
				lprintf( WIDE("%s(%d) is to run before %s and after %s first is %s")
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
			lprintf( WIDE("%s(%d) is to run after all")
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
void RegisterPriorityStartupProc( void (CPROC*proc)(void), CTEXTSTR func,int priority, void *use_label, CTEXTSTR file,int line )
{
   int use_proc;
	InitLocal();
	if( LOG_ALL ||
#ifndef __STATIC_GLOBALS__
		 (deadstart_local_data 
#else 
		(1
#endif
		&& l.flags.bLog ))
		lprintf( WIDE("Register %s@%s(%d) %d"), func,file,line, priority);
	if( nProcs == 1024 )
	{
		for( use_proc = 0; use_proc < 1024; use_proc++ )
			if( !procs[use_proc].bUsed )
            break;
		if( use_proc == 1024 )
		{
			lprintf( WIDE( "Used all 1024, and, have 1024 startups total scheduled." ) );
         DebugBreak();
		}
	}
	else
      use_proc = nProcs;

	procs[use_proc].proc = proc;
	procs[use_proc].func = func;
	procs[use_proc].file = file;
	procs[use_proc].line = line;
	procs[use_proc].priority = priority;
	procs[use_proc].bUsed = 1;
	procs[use_proc].next = NULL; // initialize so it doesn't try unlink in requeue common routine.
	procs[use_proc].me = NULL; // initialize so it doesn't try unlink in requeue common routine.

	EnqueStartupProc( &proc_schedule, procs + use_proc );

   if( nProcs < 1024 )
		nProcs++;
   /*
	if( nProcs == 1024 )
	{
		lprintf( WIDE( "Excessive number of startup procs!" ) );
		DebugBreak();
	}
	*/
	if( bInitialDone && !bSuspend )
	{
#define ONE_MACRO(a,b) a,b
#ifdef _DEBUG
		_xlprintf(LOG_NOISE,file,line)( WIDE( "Initial done, not suspended, dispatch immediate." ) );
#endif
      InvokeDeadstart();
	}
   //lprintf( WIDE("Total procs %d"), nProcs );
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
#    ifndef __cplusplus_cli
static BOOL WINAPI CtrlC( DWORD dwCtrlType )
{
	switch( dwCtrlType )
	{
	case CTRL_BREAK_EVENT:
	case CTRL_C_EVENT:
		InvokeExits();
		// allow C api to exit, whatever C api we're using
      // (allows triggering atexit functions)
		exit(3);
		return TRUE;
	case CTRL_CLOSE_EVENT:
		break;
	case CTRL_LOGOFF_EVENT:
		break;
	case CTRL_SHUTDOWN_EVENT:
		break;
	}
   // default... return not processed.
   return FALSE;
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

//#ifdef WIN32

// wow no such thing as static-izing this... it's
// always retrieved with dynamic function loading, therefore
// MUST be exported if at all possible.
// !defined( __STATIC__ ) &&
// this one is used when a library is loaded.
void InvokeDeadstart( void )
{
	PSTARTUP_PROC proc;
	//if( !bInitialDone /*|| bDispatched*/ )
	//   return;
	InitLocal();
	bInitialStarted = 1;
   // allowing initial start to be set lets final resume do this invoke.
	if( bSuspend )
	{
		if( l.flags.bLog )
			lprintf( "Suspended, first proc is %s", proc_schedule?proc_schedule->func:"No First" );
		return;
	}
#ifdef WIN32
	if( !bInitialDone && !bDispatched )
	{
#  ifndef UNDER_CE
		if( GetConsoleWindow() )
		{
#    ifndef __cplusplus_cli
			//MessageBox( NULL, "!!--!! CtrlC", "blah", MB_OK );
			SetConsoleCtrlHandler( CtrlC, TRUE );
#    endif
		}
		else
		{
			//MessageBox( NULL, "!!--!! NO CtrlC", "blah", MB_OK );
			; // do nothing if we're no actually a console window. this should fix ctrl-c not working in CMD prompts launched by MILK/InterShell
		}
#  endif
	}
#endif

#ifdef __64__
	while( ( proc = (PSTARTUP_PROC)LockedExchange64( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
#else
		while( ( proc = (PSTARTUP_PROC)LockedExchange( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
#endif
	{
		// need to set this to point to new head of list... it's not in proc_schedule anymore
		//proc->me = &proc;
		if( LOG_ALL ||
#ifndef __STATIC_GLOBALS__
		 (deadstart_local_data 
#else 
		(1
#endif
		&& l.flags.bLog ))
		{
			lprintf( WIDE("Dispatch %s@%s(%d)p:%d "), proc->func,proc->file,proc->line, proc->priority );
		}
		{
			bDispatched = 1;
			if( proc->proc
#ifndef __LINUX__
#if  __WATCOMC__ >= 1280
				&& !IsBadCodePtr( (int(STDCALL*)(void))proc->proc )
#elif defined( __64__ )
				&& !IsBadCodePtr( (FARPROC)proc->proc )
#else
//				&& !IsBadCodePtr( (int STDCALL(*)(void))proc->proc )
#endif
#endif
			  )
			{
				proc->proc();
			}
			proc->bUsed = 0;
			bDispatched = 0;
		}
      // look to see if anything new was scheduled.  Grab the list, add it to the one's we're processing.
		{
			{
				PSTARTUP_PROC newly_scheduled_things;
            proc->me = &proc;
#ifdef __64__
				if( ( newly_scheduled_things = (PSTARTUP_PROC)LockedExchange64( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
#else
				if( ( newly_scheduled_things = (PSTARTUP_PROC)LockedExchange( (PVPTRSZVAL)&proc_schedule, 0 ) ) != NULL )
#endif
				{
					newly_scheduled_things->me = &newly_scheduled_things;
					while( newly_scheduled_things )
					{
						EnqueStartupProc( &proc, newly_scheduled_things );
					}
				}
			}
			proc_schedule = proc;
			proc_schedule->me = &proc_schedule;
		}
		UnlinkThing( proc );
	}
}

void MarkRootDeadstartComplete( void )
{
	bInitialDone = 1;
}

#ifndef __NO_OPTIONS__
// options initializes at SQL+1
PRIORITY_PRELOAD( InitDeadstartOptions, SQL_PRELOAD_PRIORITY+2 )
{
	l.flags.bLog = SACK_GetProfileIntEx( WIDE( "SACK/Deadstart" ), WIDE( "Logging Enabled?" ), 0, TRUE );
}
#endif

#if defined( __GNUC__ )

#ifndef __cplusplus
static void RegisterStartups( void ) __attribute__((constructor));

// this one is used when the library loads.  (there is only one of these.)
// and constructors are run every time a library is loaded....
// I wonder whose fault that is....
void RegisterStartups( void )
{
#  ifndef paste
#    define paste(a,b) a##b
#  endif
#  define paste2(a,b) paste(a,b)
#  define DeclareList(n) paste2(n,TARGET_LABEL)
   extern struct rt_init DeclareList( begin_deadstart_ );
   extern struct rt_init DeclareList( end_deadstart_ );
	struct rt_init *begin = &DeclareList( begin_deadstart_ );
	struct rt_init *end = &DeclareList( end_deadstart_ );
	struct rt_init *current;
#  ifdef __NO_BAG__
   printf( WIDE( "Not doing deadstarts\n" ) );
	return;
#  endif
   //cygwin_dll_init();
	if( begin[0].scheduled )
      return;
	if( (begin+1) < end )
	{
#  ifdef __CYGWIN__
		void (*MyRegisterPriorityStartupProc)( void (CPROC*proc)(void), CTEXTSTR func,int priority, CTEXTSTR file,int line );
		char myname[256];
      HMODULE mod;
      GetModuleFileName(NULL,myname,sizeof(myname));
		mod = LoadLibrary( myname );GetModuleFileName(NULL,myname,sizeof(myname));
		MyRegisterPriorityStartupProc = (void(*)( void(CPROC*)(void),CTEXTSTR,int,CTEXTSTR,int))GetProcAddress( mod, WIDE( "RegisterPriorityStartupProc" ) );
#    ifdef DEBUG_CYGWIN_START
		fprintf( stderr, WIDE( "mod is %p proc  is %p %s\n" ), mod, MyRegisterPriorityStartupProc, TARGETNAME );
#    endif
		if( MyRegisterPriorityStartupProc )
		{
#  endif
		for( current = begin + 1; current < end; current++ )
		{
			if( !current[0].scheduled )
			{
				if( ( LOG_ALL
#ifndef __STATIC_GLOBALS__
					|| deadstart_local_data
#endif
					 )	&& l.flags.bLog )
					lprintf( WIDE("Register %d %s@%s(%d)"), current->priority, current->funcname, current->file, current->line );
#  ifdef __CYGWIN__
#    ifdef DEBUG_CYGWIN_START
				fprintf( stderr, WIDE("Register %d %s@%s(%d)\n"), current->priority, current->funcname, current->file, current->line );
#    endif
				MyRegisterPriorityStartupProc( current->routine, current->funcname, current->priority, current->file, current->line );
#  else
				RegisterPriorityStartupProc( current->routine, current->funcname, current->priority, NULL, current->file, current->line );
#  endif
            current[0].scheduled = 1;
			}
			else
			{
#  ifndef  DISABLE_DEBUG_REGISTER_AND_DISPATCH
				lprintf( WIDE("Not Register(already did this once) %d %s@%s(%d)"), current->priority, current->funcname, current->file, current->line );
#  endif
			}
#  ifdef __CYGWIN__
		}
#  endif
		}
	}
   // should be setup in such a way that this ignores all external invokations until the core app runs.
	//InvokeDeadstart();
}
#endif  //__cplusplus

#endif // #if __GNUC__

// parameter 4 is just used so the external code is not killed
// we don't actually do anything with this?
void RegisterPriorityShutdownProc( void (CPROC*proc)(void), CTEXTSTR func, int priority,void *use_label, CTEXTSTR file,int line )
{
	InitLocal();
	if( LOG_ALL ||
#ifndef __STATIC_GLOBALS__
		(deadstart_local_data
#else
		 (1
#endif
		  && l.flags.bLog ))
		lprintf( WIDE("Exit Proc %s(%p) from %s(%d) registered...")
				 , func
				 , proc
				 , file
				 , line );
	shutdown_procs[nShutdownProcs].proc = proc;
	shutdown_procs[nShutdownProcs].func = func;
	shutdown_procs[nShutdownProcs].file = file;
	shutdown_procs[nShutdownProcs].line = line;
	shutdown_procs[nShutdownProcs].bUsed = 1;
	shutdown_procs[nShutdownProcs].priority = priority;
	{
		PSHUTDOWN_PROC check;
		for( check = shutdown_proc_schedule; check; check = check->next )
		{
			if( shutdown_procs[nShutdownProcs].priority >= check->priority )
			{
#ifdef DEBUG_SHUTDOWN
				lprintf( WIDE("%s(%d) is to run before %s(%d) %s")
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
		//lprintf( WIDE("first routine is %s(%d)")
	  //		 , shutdown_proc_schedule->func
		//		 , shutdown_proc_schedule->line );
	}
	nShutdownProcs++;
   //lprintf( WIDE("Total procs %d"), nProcs );
}

//id RunExits( void )
void InvokeExits( void )
{
// okay well since noone previously scheduled exits...
// this runs a prioritized list of exits - all within
// a single moment of exited-ness.
	PSHUTDOWN_PROC proc;
// shutdown is much easier than startup cause more
// procedures shouldn't be added as a property of shutdown.
   //bugBreak();
	while(
#ifdef __64__
			( proc = (PSHUTDOWN_PROC)LockedExchange64( (PV_64)&shutdown_proc_schedule, 0 ) ) != NULL
#else
			( proc = (PSHUTDOWN_PROC)LockedExchange( (PV_32)&shutdown_proc_schedule, 0 ) ) != NULL
#endif
		  )
	{
      PSHUTDOWN_PROC proclist = proc;
      // link list to myself..
		proc->me = &proclist;
		while( ( proc = proclist ) )
		{
#ifdef DEBUG_SHUTDOWN
			lprintf( WIDE("Exit Proc %s(%p)(%d) priority %d from %s(%d)...")
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
			UnlinkThing( proc );
#ifdef DEBUG_SHUTDOWN
			lprintf( WIDE("Okay and that's done... next is %p %p"), proclist, shutdown_proc_schedule );
#endif
		}
      //shutdown_proc_schedule = proc;
	}
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
EXPORT_METHOD	void BAG_Exit( int code )
{
#ifndef __cplusplus_cli
	InvokeExits();
#endif
#undef exit
   exit( code );
}

// legacy linking code - might still be usin this for linux...
int is_deadstart_complete( void )
{
	//extern _32 deadstart_complete;
#ifndef __STATIC_GLOBALS__
   if( deadstart_local_data )
		return bInitialDone;//deadstart_complete;
#endif
	return 0;
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


SACK_DEADSTART_NAMESPACE_END
