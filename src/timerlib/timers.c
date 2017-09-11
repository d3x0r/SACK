/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   Adds functionality of timers that run dispatched from a single thread
 *   timer delay is trackable to provide self adjusting reliable frequency dispatch.
 *
 *   RemoveTimer( AddTimer( tick_frequency, timer_callback, user_data ) );
 *
 */
//#define ENABLE_CRITICALSEC_LOGGING

// this is a method replacement to use PIPEs instead of SEMAPHORES
// replacement code only affects linux.
#if __ANDROID__
#define USE_PIPE_SEMS
#endif

#if defined( __QNX__ ) || defined( __MAC__)
#define USE_PIPE_SEMS
// no semtimedop; no semctl, etc
//#include <sys/sem.h>
#endif

#define NO_UNICODE_C

#ifdef USE_PIPE_SEMS
#define _NO_SEMTIMEDOP_
#endif

// this is a cheat to get the critical section
// object... otherwise we'd have had circular
// linking reference between this and sharemem
// which would prefer to implement wakeablesleep
// for critical section waiting...
// must be included before memlib..

//#undef UNICODE
#define MEMORY_STRUCT_DEFINED
#define DEFINE_MEMORY_STRUCT

#define THREAD_STRUCTURE_DEFINED

#include <stdhdrs.h> // Sleep()
#include <procreg.h> // SimpleRegisterAndCreateGlobal
// sorry if this causes problems...
// maybe promote this include into stdhdrs when this fails to compile
#ifdef __WATCOMC__
// _beginthread
#undef exit
#undef getenv
// process.h redefines exit
#include <process.h>
#endif


#include <sack_types.h>
#include <deadstart.h>
#include <sharemem.h>
#include <idle.h>
#ifndef __NO_OPTIONS__
#include <sqlgetoption.h>
#endif
#define DO_LOGGING
#include <logging.h>

// display pause/resume support.
#include <render.h>

#include <timers.h>

#ifndef _SHARED_MEMORY_LIBRARY
#  include "../memlib/sharestruc.h"
#endif
#ifdef __cplusplus
namespace sack {
	namespace timers {
		using namespace sack::containers;
		using namespace sack::memory;
		using namespace sack::logging;
#endif


//#define LOG_CREATE_EVENT_OBJECT
//#define LOG_THREAD
//#define LOG_SLEEPS

// - define this to log when timers were delayed in scheduling...
//198#define LOG_LATENCY_LIGHT
//#define LOG_LATENCY

//#define LOG_INSERTS
//#define LOG_DISPATCH
//#define DEBUG_PIPE_USAGE

typedef struct thread_event THREAD_EVENT;
typedef struct thread_event *PTHREAD_EVENT;

struct timer_tag
{
// putting next as first thing in structure
   // allows me to reference also prior
	struct timer_tag *next;
	union {
		struct timer_tag **me;
		struct timer_tag *prior;
	};
	struct {
		BIT_FIELD bRescheduled : 1;
	} flags;
	uint32_t frequency;
	int32_t delta;
	uint32_t ID;
	void (CPROC*callback)(uintptr_t user);
	uintptr_t userdata;
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
	CTEXTSTR pFile;
	int nLine;
#endif
};
typedef struct timer_tag TIMER, *PTIMER;

#define MAXTIMERSPERSET 32
DeclareSet( TIMER );

struct threads_tag
{
	// these first two items MUST
	// be declared publically, and MUST be visible
	// to the thread created.
	uintptr_t param;
	uintptr_t (CPROC*proc)( struct threads_tag * );
	uintptr_t (CPROC*simple_proc)( POINTER );
	TEXTSTR thread_event_name; // might be not a real thread.
	THREAD_ID thread_ident;
	PTHREAD_EVENT thread_event;
#ifdef _WIN32
	//HANDLE hEvent;
	HANDLE hThread;
#else
#ifdef USE_PIPE_SEMS
	int pipe_ends[2]; // file handles that are the pipe's ends. 0=read 1=write
#endif
	int semaphore; // use this as a status of pipes if USE_PIPE_SEMS is used...; otherwise it's a ipcsem
	pthread_t hThread;
#endif
	struct {
		//BIT_FIELD bLock : 1;
		//BIT_FIELD bSleeping : 1;
		//BIT_FIELD bWakeWhileRunning : 1;
		BIT_FIELD bRemovedWhileRunning : 1;
		BIT_FIELD bLocal : 1;
		BIT_FIELD bReady : 1;
		BIT_FIELD bStarted : 1;
	} flags;
	//struct threads_tag *next, **me;
	CTEXTSTR pFile;
	uint32_t nLine;
};

typedef struct threads_tag THREAD;
#define MAXTHREADSPERSET 16
DeclareSet( THREAD );

struct thread_event
{
	TEXTSTR name;
#ifdef _WIN32
	HANDLE hEvent;
#endif
};


static struct {
	uint32_t timerID;
	PTIMERSET timer_pool;
	PTIMER timers;
	PTIMER add_timer; // this timer is scheduled to be added...
	PTIMER current_timer;
	struct {
		BIT_FIELD away_in_timer : 1;
		BIT_FIELD insert_while_away : 1;
		BIT_FIELD set_timer_signal : 1;
		BIT_FIELD bExited : 1;
#ifdef ENABLE_CRITICALSEC_LOGGING
		BIT_FIELD bLogCriticalSections : 1;
#endif
		BIT_FIELD bLogSleeps : 1;
		BIT_FIELD bLogTimerDispatch : 1;
		BIT_FIELD bLogThreadCreate : 1;
		BIT_FIELD bHaltTimers : 1;
	} flags;
	uint32_t del_timer; // this timer is scheduled to be removed...
	uint32_t tick_bias; // should somehow end up equating to sleep overhead...
	uint32_t last_tick; // last known time that a timer could have fired...
	uint32_t this_tick; // the current moment up to which we fire all timers.
	PTHREAD pTimerThread;
	PTHREADSET threadset;
	PTHREAD threads;
	uint32_t lock_timers;
	CRITICALSECTION cs_timer_change;
	//uint32_t pending_timer_change;
	uint32_t remove_timer;
	uint32_t CurrentTimerID;
	int32_t last_sleep;

#define globalTimerData (*global_timer_structure)
	uintptr_t lock_thread_create;
	// should be a short list... 10 maybe 15...
	PLIST thread_events;

	CRITICALSECTION csGrab;
#if defined( WIN32 )
	DWORD my_thread_info_tls;
#elif defined( __LINUX__ )
	pthread_key_t my_thread_info_tls;
#endif
} *global_timer_structure;// = { 1000 };


#if HAS_TLS
struct my_thread_info {
	PTHREAD pThread;
	THREAD_ID nThread;
};
#define MyThreadInfo (*_MyThreadInfo)
#endif

#ifdef _WIN32
#else
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/ipc.h>

	 // hmm wonder why this has to be defined....
	 // semtimedop is a wonderful wonderful thing...
	 // but yet /usr/include/sys/sem.h only defines it if
// __USE_GNU is defined....
#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifdef __ANDROID__
#include <linux/sem.h>
#else
#include <sys/sem.h>
#endif
#include <signal.h>

#endif

void  RemoveTimerEx( uint32_t ID DBG_PASS );

static struct my_thread_info* GetThreadTLS( void )
{
	struct my_thread_info* _MyThreadInfo;
#if defined( WIN32 )
	if( !global_timer_structure )
		SimpleRegisterAndCreateGlobal( global_timer_structure );
	if( !( _MyThreadInfo = (struct my_thread_info*)TlsGetValue( global_timer_structure->my_thread_info_tls ) ) )
	{
		int old = SetAllocateLogging( FALSE );
		TlsSetValue( global_timer_structure->my_thread_info_tls, _MyThreadInfo = New( struct my_thread_info ) );
		SetAllocateLogging( old );
		_MyThreadInfo->nThread = 0;
		_MyThreadInfo->pThread = 0;
	}
#elif defined( __LINUX__ )
	if( !( _MyThreadInfo = (struct my_thread_info*)pthread_getspecific( global_timer_structure->my_thread_info_tls ) ) )
	{
		pthread_setspecific( global_timer_structure->my_thread_info_tls, _MyThreadInfo = New( struct my_thread_info ) );
		_MyThreadInfo->nThread = 0;
		_MyThreadInfo->pThread = 0;
	}
#endif
	return _MyThreadInfo;
}

// this priorirty is also relative to a secondary init for procreg/names.c
// if you change this, need to change when that is scheduled also
PRIORITY_PRELOAD( LowLevelInit, CONFIG_SCRIPT_PRELOAD_PRIORITY-1 )
{
	// there is a small chance the local is already initialized.
	if( !global_timer_structure )
		SimpleRegisterAndCreateGlobal( global_timer_structure );
	if( !globalTimerData.timerID )
	{
#if defined( WIN32 )
		globalTimerData.my_thread_info_tls = TlsAlloc();
#elif defined( __LINUX__ )
		pthread_key_create( &globalTimerData.my_thread_info_tls, NULL );
#endif
		InitializeCriticalSec( &globalTimerData.csGrab );
		// this may have initialized early?
		globalTimerData.timerID = 1000;
		//lprintf( "thread global will be %p %p", global_timer_structure, &global_timer_structure );
	}
}

PRELOAD( ConfigureTimers )
{
#ifndef __NO_OPTIONS__
#  ifdef ENABLE_CRITICALSEC_LOGGING
	globalTimerData.flags.bLogCriticalSections = SACK_GetProfileInt( GetProgramName(), WIDE( "SACK/Memory Library/Log critical sections" ), 0 );
#  endif
	globalTimerData.flags.bLogThreadCreate = SACK_GetProfileInt( GetProgramName(), WIDE( "SACK/Timers/Log Thread Create" ), 0 );
	globalTimerData.flags.bLogSleeps = SACK_GetProfileInt( GetProgramName(), WIDE( "SACK/Timers/Log Sleeps" ), 0 );
	globalTimerData.flags.bLogTimerDispatch = SACK_GetProfileInt( GetProgramName(), WIDE( "SACK/Timers/Log Timer Dispatch" ), 0 );
#endif
}

//--------------------------------------------------------------------------
#ifdef __LINUX__
#ifdef __LINUX__
uint32_t  GetTickCount( void )
{
	struct timeval time;
	gettimeofday( &time, 0 );
	return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

uint32_t  timeGetTime( void )
{
	struct timeval time;
	gettimeofday( &time, 0 );
	return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

void  Sleep( uint32_t ms )
{
	(usleep((ms)*1000));
}
#endif
uintptr_t closesem( POINTER p, uintptr_t psv )
{
	PTHREAD thread = (PTHREAD)p;
#ifdef USE_PIPE_SEMS
	//lprintf( "CLOSE PIPES %s %"_64fx, thread->thread_event_name, thread->thread_ident );
	close( thread->pipe_ends[0] );
	close( thread->pipe_ends[1] );
	thread->pipe_ends[0] = -1;
	thread->pipe_ends[1] = -1;
	thread->semaphore = -1;
#else
	if( semctl( thread->semaphore, 0, IPC_RMID ) == -1 )
	{
		lprintf( WIDE( "Error: %08x %s" ), thread->semaphore, strerror( errno ) );
	}
	thread->semaphore = -1;
#endif
	return 0;
}

static uintptr_t threadrunning( POINTER p, uintptr_t psv )
{
	PTHREAD thread = (PTHREAD)p;
	if( thread->hThread && thread->flags.bStarted )
		return 1;
	return 0;
}

// sharemem exit priority +1 (exit after everything else, except emmory; globals at memory+1)
PRIORITY_ATEXIT( CloseAllWakeups, ATEXIT_PRIORITY_THREAD_SEMS )
{
	//pid_t mypid = getppid();
	// not sure if mypid is needed...
	while( ForAllInSet( THREAD, globalTimerData.threadset, threadrunning, 0 ) )
		Relinquish();
	lprintf( WIDE("Destroy thread semaphores...") );
	ForAllInSet( THREAD, globalTimerData.threadset, closesem, (uintptr_t)0 );
	DeleteSet( (GENERICSET**)&globalTimerData.threadset );
	globalTimerData.pTimerThread = NULL;
	//globalTimerData.threads = NULL;
	globalTimerData.timers = NULL;
}
#endif

// sharemem exit priority +1 (exit after everything else, except emmory)
PRIORITY_ATEXIT( StopTimers, ATEXIT_PRIORITY_TIMERS )
{
	int tries = 0;
	//pid_t mypid = getppid();
	// not sure if mypid is needed...
	if( global_timer_structure ) {
		globalTimerData.flags.bExited = 1;
		if( globalTimerData.pTimerThread )
			WakeThread( globalTimerData.pTimerThread );
		while( globalTimerData.pTimerThread )
		{
			tries++;
			if( tries > 10 )
				return;
			WakeThread( globalTimerData.pTimerThread );
			Relinquish();
		}
	}
}
//--------------------------------------------------------------------------

static void InitWakeup( PTHREAD thread, CTEXTSTR event_name )
{
	if( !event_name )
		event_name = WIDE("ThreadSignal");
	thread->thread_event_name = StrDup( event_name );
#ifdef _WIN32
	if( !thread->thread_event )
	{
		PTHREAD_EVENT thread_event;
		TEXTCHAR name[64];
		tnprintf( name, 64, WIDE("%s:%08lX:%08lX"), event_name, (uint32_t)(thread->thread_ident >> 32)
				  , (uint32_t)(thread->thread_ident & 0xFFFFFFFF) );
		name[sizeof(name)/sizeof(name[0])-1] = 0;
#ifdef LOG_CREATE_EVENT_OBJECT
		lprintf( WIDE("Thread Event created is: %s everyone should use this..."), name );
#endif
		thread_event = New( THREAD_EVENT );
		thread_event->name = StrDup( name );
		thread_event->hEvent = CreateEvent( NULL, TRUE, FALSE, name );
		AddLink( &globalTimerData.thread_events, thread_event );
		thread->thread_event = thread_event;
	}
#else
#ifdef USE_PIPE_SEMS
	// store status of pipe() in semaphore... it's not really a semaphore..
#  ifdef DEBUG_PIPE_USAGE
	lprintf( "Init wakeup %p %s", thread, event_name );
#  endif

	if( ( thread->semaphore = pipe( thread->pipe_ends ) )  == -1 )
	{
		lprintf( WIDE("Failed to get pipe! %d:%s"), errno, strerror( errno ) );
	}
	else
	{
		char buf;
		int success = 0;
		do
		{
			int stat;
			int n;
			fd_set set;
			struct timeval timeout;
			FD_ZERO(&set); /* clear the set */
			FD_SET( thread->pipe_ends[0], &set); /* add our file descriptor to the set */
			timeout.tv_sec = 0;
			timeout.tv_usec = 100;

#  ifdef DEBUG_PIPE_USAGE
			lprintf(" Begin select-flush on thread %p", thread );
#  endif
			stat = select(thread->pipe_ends[0] + 1, &set, NULL, NULL, &timeout);
			if(stat == -1)
			{
				lprintf( WIDE("select error %d %d"), errno, thread->pipe_ends[0]); /* an error accured */
			}
			else if(stat == 0)
			{
				success = 1;
#  ifdef DEBUG_PIPE_USAGE
				lprintf("timeout"); /* a timeout occured */
#  endif
			}
			else
			{
#  ifdef DEBUG_PIPE_USAGE
				lprintf(" immediate return?" );
#  endif
				stat = read( thread->pipe_ends[0], &buf, 1 );
#  ifdef DEBUG_PIPE_USAGE
				lprintf( "Stat is now %d", stat );
#  endif
			}
		}
		while( !success );
	}

#else
	thread->semaphore = semget( IPC_PRIVATE
									  , 1, IPC_CREAT | 0600 );
	if( thread->semaphore == -1 )
	{
		// basically this can't really happen....
		if( errno ==  EEXIST )
		{
			thread->semaphore = semget( IPC_PRIVATE
											  , 1, 0 );
			if( thread->semaphore == -1 )
				lprintf( WIDE("FAILED TO CREATE SEMAPHORE! : %d"), errno );
		}
		if( errno == ENOSPC )
		{
			lprintf( WIDE("Hmm Need to cleanup some semaphore objects!!!") );
		}
		else
			lprintf( WIDE("Failed to get semaphore! %d"), errno );
	}
	if( thread->semaphore != -1 )
	{
		//union semun ctl;
		//ctl.val = 0;
		//lprintf( WIDE("Setting thread semaphore to 0 (locked).") );
		if( semctl( thread->semaphore, 0, SETVAL, 0 ) < 0 )
		{
			lprintf( WIDE("Errro setting semaphre value: %d"), errno );
		}
		//lprintf( WIDE("after semctl = %d %08lx"), semctl( thread->semaphore, 0, GETVAL ), thread->semaphore );
	}
#endif
#endif
}

//--------------------------------------------------------------------------


uintptr_t CPROC check_thread_name( POINTER p, uintptr_t psv )
{
	PTHREAD thread = (PTHREAD)p;
	if( StrCaseCmp( thread->thread_event_name, (CTEXTSTR)psv ) == 0 )
		return (uintptr_t)p;
	return 0;
}

static PTHREAD FindWakeup( CTEXTSTR name )
{
	PTHREAD check;
	if( global_timer_structure )
	{
		// don't need locks if init didn't finish, there's now way to have threads in loader lock.
		while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
			Relinquish();
	}
	else
	{
		if( IsRootDeadstartStarted() )
			SimpleRegisterAndCreateGlobal( global_timer_structure );
	}

	check = (PTHREAD)ForAllInSet( THREAD, globalTimerData.threadset, check_thread_name, (uintptr_t)name );
	if( !check )
	{
#ifdef _DEBUG
		//lprintf( DBG_FILELINEFMT "Failed to find the thread - so let's add it" );
#endif
		check = GetFromSet( THREAD, &globalTimerData.threadset );
		MemSet( check, 0, sizeof( THREAD ) );
		check->thread_ident = GetMyThreadID();
		InitWakeup( check, name );
		check->flags.bReady = 1;
	}
	globalTimerData.lock_thread_create = 0;
	return check;
}
//--------------------------------------------------------------------------

struct name_and_id_params
{
	CTEXTSTR name;
	THREAD_ID thread;
};

uintptr_t CPROC check_thread_name_and_id( POINTER p, uintptr_t psv )
{
	struct name_and_id_params *params = (struct name_and_id_params*)psv;
	PTHREAD thread = (PTHREAD)p;
	if( thread->thread_ident == params->thread
		&& StrCaseCmp( thread->thread_event_name, params->name ) == 0 )
		return (uintptr_t)p;
	return 0;
}

static PTHREAD FindThreadWakeup( CTEXTSTR name, THREAD_ID thread )
{
	PTHREAD check;
	struct name_and_id_params params;
	params.name = name;
	params.thread = thread;
	if( global_timer_structure )
	{
		// don't need locks if init didn't finish, there's now way to have threads in loader lock.
		while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
			Relinquish();
	}
	else
	{
		if( IsRootDeadstartStarted() )
			SimpleRegisterAndCreateGlobal( global_timer_structure );
	}

	check = (PTHREAD)ForAllInSet( THREAD, globalTimerData.threadset, check_thread_name_and_id, (uintptr_t)&params );
	if( !check )
	{
#ifdef _DEBUG
		//lprintf( DBG_FILELINEFMT "Failed to find the thread - so let's add it" );
#endif
		check = GetFromSet( THREAD, &globalTimerData.threadset );
		MemSet( check, 0, sizeof( THREAD ) );
		check->thread_ident = thread;
		InitWakeup( check, name );
		check->flags.bReady = 1;
	}
	globalTimerData.lock_thread_create = 0;
	return check;
}

//--------------------------------------------------------------------------

uintptr_t CPROC check_thread( POINTER p, uintptr_t psv )
{
	PTHREAD thread = (PTHREAD)p;
	THREAD_ID ID = *((THREAD_ID*)psv);
	//lprintf( "Check thread %016llx %016llx %s", thread->thread_ident, ID, thread->thread_event_name );
	if( ( thread->thread_ident == ID )
		&& ( StrCmp( thread->thread_event_name, WIDE("ThreadSignal") ) == 0 ) )
		return (uintptr_t)p;
	return 0;
}

static PTHREAD FindThread( THREAD_ID thread )
{
	PTHREAD check;
	if( global_timer_structure )
	{
		// don't need locks if init didn't finish, there's now way to have threads in loader lock.
		while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
			Relinquish();
	}
	else
	{
		if( IsRootDeadstartStarted() )
			SimpleRegisterAndCreateGlobal( global_timer_structure );
	}
	check = (PTHREAD)ForAllInSet( THREAD, globalTimerData.threadset, check_thread, (uintptr_t)&thread );
	if( !check )
	{
#ifdef _DEBUG
		//lprintf( DBG_FILELINEFMT "Failed to find the thread - so let's add it" );
#endif
		check = GetFromSet( THREAD, &globalTimerData.threadset );
		MemSet( check, 0, sizeof( THREAD ) );
		check->thread_ident = thread;
		InitWakeup( check, NULL );
		check->flags.bReady = 1;
	}
	globalTimerData.lock_thread_create = 0;
	return check;
}

//--------------------------------------------------------------------------

void  WakeThreadEx( PTHREAD thread DBG_PASS )
{
	if( !thread ) // can't wake nothing
	{
		//_lprintf(DBG_RELAY)( WIDE("Failed to find thread to wake...") );
		return;
	}
	//_xlprintf( 0 DBG_RELAY )( WIDE("Waking a thread: %p"), thread );
	//while( thread->flags.bLock )
	//{
	//	Log( WIDE("Waiting for thread to go to sleep") );
	//	Relinquish();
	//}
	//if( thread->flags.bLocal && !thread->flags.bSleeping )
	//{
	//	//Log( WIDE("Waking thread which is already awake") );
	//  thread->flags.bWakeWhileRunning = 1;
	//  Relinquish(); // wake implies that we want the other thing to run.
	//  lprintf( DBG_FILELINEFMT "Thread is not sleeping... woke it before it slept" );
	//  return;
	//}
#ifdef _WIN32
	//	lprintf( WIDE("setting event.") );
	{
		PTHREAD_EVENT thread_event;
		INDEX idx;
		TEXTCHAR name[64];
		if( !(thread_event = thread->thread_event ) )
		{
			tnprintf( name, sizeof(name), WIDE("%s:%08lX:%08lX")
					  , thread->thread_event_name, (uint32_t)(thread->thread_ident >> 32)
					  , (uint32_t)(thread->thread_ident & 0xFFFFFFFF));
			name[sizeof(name)/sizeof(name[0])-1] = 0;
			LIST_FORALL( globalTimerData.thread_events, idx, PTHREAD_EVENT, thread_event )
			{
				if( StrCmp( thread_event->name, name ) == 0 )
					break;
			}
#ifdef LOG_CREATE_EVENT_OBJECT
			lprintf( WIDE("Event opened is: %s"), name );
#endif
		}
#ifdef LOG_CREATE_EVENT_OBJECT
		else
		{
			lprintf( WIDE("Event opened is thread.") );
		}
#endif
		if( !thread_event )
		{
			thread_event = New( THREAD_EVENT );
			thread_event->name = StrDup( name );
			thread_event->hEvent = OpenEvent( EVENT_ALL_ACCESS /*EVENT_MODIFY_STATE */, FALSE, name );
			AddLink( &globalTimerData.thread_events, thread_event );
			thread->thread_event = thread_event;
		}
		if( thread_event->hEvent )
		{
			//lprintf( WIDE("event opened successfully... %d"), WaitForSingleObject( hEvent, 0 ) );
#ifndef NO_LOGGING
			if( globalTimerData.flags.bLogSleeps )
				_xlprintf(1 DBG_RELAY )( WIDE("About to wake on %d Thread event created...%016llx")
											  , thread->thread_event->hEvent
											  , thread->thread_ident );
#endif
			if( !SetEvent( thread_event->hEvent ) )
				 lprintf( WIDE("Set event FAILED..%d"), GetLastError() );
			Relinquish(); // may or may not execute other thread before this...
		}
		else
		{
			lprintf( WIDE("Failed to open that event! %d"), GetLastError() );
			// thread to wake is not ready to be
			// woken, does not exist, or some other
			// BAD problem.
		}
	}
#else
#ifdef USE_PIPE_SEMS
	if( thread->semaphore != -1 )
	{
#  ifdef DEBUG_PIPE_USAGE
		_lprintf(DBG_RELAY)( "(wakethread)wil write pipe... %p", thread );
#  endif
		write( thread->pipe_ends[1], "G", 1 );
		//lprintf( "did write pipe..." );
		Relinquish();
	}
#else
	if( thread->semaphore != -1 )
	{
		int stat;
		int val;
		struct sembuf semdo;
		semdo.sem_num = 0;
		semdo.sem_op = 1;
		semdo.sem_flg = 0;
		//_xlprintf( 1 DBG_RELAY )( WIDE("Resetting event on %08x %016"_64fx"x"), thread->semaphore, thread->thread_ident );
		//lprintf( WIDE("Before semval = %d %08lx"), semctl( thread->semaphore, 0, GETVAL ), thread->semaphore );
		stat = semop( thread->semaphore, &semdo, 1 );
		if( stat == -1 )
		{
			if( errno != ERANGE )
				lprintf( WIDE("semop error (wake) : %d"), errno );
		}
		//lprintf( WIDE("After semval = %d %08lx"), val = semctl( thread->semaphore, 0, GETVAL ), thread->semaphore );
		if( !val )
		{
			//DebugBreak();
			//lprintf( WIDE("Did we fail the semop?!") );
		}
		Relinquish(); // may or may not execute other thread before this...
	}
#endif
#  if 0
	if( thread->thread ) // thread creation might not be complete yet...
	{
		pthread_kill( thread->thread, SIGUSR1 );
	}
#  endif
#endif
}


void  WakeNamedThreadSleeperEx( CTEXTSTR name, THREAD_ID thread DBG_PASS )
{
	PTHREAD sleeper = FindThreadWakeup( name, thread );
	if( sleeper )
		WakeThreadEx( sleeper DBG_RELAY );
}

void  WakeNamedSleeperEx( CTEXTSTR name DBG_PASS )
{
	PTHREAD sleeper = FindWakeup( name );
	if( sleeper )
		WakeThreadEx( sleeper DBG_RELAY );
}
//#undef WakeThread
//void  WakeThread( PTHREAD thread )
//{
//	WakeThreadEx( thread DBG_SRC );
//}

//--------------------------------------------------------------------------

void  WakeThreadIDEx( THREAD_ID thread DBG_PASS )
{
	PTHREAD pThread = FindThread( thread );
	WakeThreadEx( pThread DBG_RELAY );
}

//--------------------------------------------------------------------------
#undef WakeThreadID
void  WakeThreadID( THREAD_ID thread )
{
	WakeThreadIDEx( thread DBG_SRC );
}

//--------------------------------------------------------------------------
#ifdef _NO_SEMTIMEDOP_
#ifndef _WIN32
static void CPROC TimerWake( uintptr_t psv )
{
	WakeThreadEx( (PTHREAD)psv DBG_SRC );
}
#endif
#endif
//--------------------------------------------------------------------------
static void  InternalWakeableNamedSleepEx( CTEXTSTR name, uint32_t n, LOGICAL threaded DBG_PASS )
{
	PTHREAD pThread;
	if( name && threaded )
		pThread = FindThreadWakeup( name, GetMyThreadID() );
	else if( name )
		pThread = FindWakeup( name );
	else
	{
#ifdef HAS_TLS
		struct my_thread_info* _MyThreadInfo = GetThreadTLS();
		pThread = MyThreadInfo.pThread;
		//lprintf( "thread will be %p %p", pThread, &MyThreadInfo );
		//lprintf( "pthread is %p", pThread );
		if( !pThread )
		{
			//lprintf( WIDE("had to init thread...") );
			MakeThread();
			pThread = MyThreadInfo.pThread;
		}
#  ifdef DEBUG_PIPE_USAGE
		lprintf( "Sleeping on threadsignal... %p", pThread );
#  endif
#else
		pThread = FindThread( GetMyThreadID() );
#endif
	}
	if( pThread )
	{
#ifdef _WIN32
#ifndef NO_LOGGING
		if( globalTimerData.flags.bLogSleeps )
			_xlprintf(1 DBG_RELAY )( WIDE("About to sleep on %d Thread event created...%s:%016llx")
										  , pThread->thread_event->hEvent
										  , pThread->thread_event_name
										  , pThread->thread_ident );
#endif
		if( WaitForSingleObject( pThread->thread_event->hEvent
									  , n==SLEEP_FOREVER?INFINITE:(n) ) != WAIT_TIMEOUT )
		{
#ifdef LOG_LATENCY
			_lprintf(DBG_RELAY)( WIDE("Woke up- reset event") );
#endif
			ResetEvent( pThread->thread_event->hEvent );
			//if( n == SLEEP_FOREVER )
			//   DebugBreak();
		}
#ifdef LOG_LATENCY
		else
			_lprintf(DBG_RELAY)( WIDE("Timed out from %d"), n );
#endif
#else
		{
#ifndef USE_PIPE_SEMS
#ifdef _NO_SEMTIMEDOP_
			int nTimer = 0;
			if( n != SLEEP_FOREVER )
			{
				//lprintf( WIDE("Wakeable sleep in %ld (oneshot, no frequency)"), n );
				nTimer = AddTimerExx( n, 0, TimerWake, (uintptr_t)pThread DBG_RELAY );
			}
#endif
#endif
			if( pThread->semaphore == -1 )
			{
				//lprintf( WIDE("Invalid semaphore...fixing?") );
				InitWakeup( pThread, name );
			}
			if( pThread->semaphore != -1 )
			{
#ifdef USE_PIPE_SEMS
#else
				struct sembuf semdo[2];
				semdo[0].sem_num = 0;
				semdo[0].sem_op = -1;
				semdo[0].sem_flg = 0;
#endif
				while(1)
				{
					int stat;
					//lprintf( WIDE("Lock on semop on semdo... %08x %016"_64fx"x"), pThread->semaphore, pThread->thread_ident );
					//lprintf( WIDE("Before semval = %d %08lx"), semctl( pThread->semaphore, 0, GETVAL ), pThread->semaphore );
					if( n != SLEEP_FOREVER )
					{
#ifdef USE_PIPE_SEMS
						char buf;
						{
							fd_set set;
							struct timeval timeout;
							FD_ZERO(&set); /* clear the set */
							FD_SET( pThread->pipe_ends[0], &set); /* add our file descriptor to the set */

							timeout.tv_sec = n / 1000;
							timeout.tv_usec = ( n % 1000 ) * 1000;


#  ifdef DEBUG_PIPE_USAGE
							lprintf(" Begin select-read on thread %p %d ", pThread, n );
                     //_lprintf(DBG_RELAY)( "Select  %p %d  %d  %d", pThread, pThread->pipe_ends[0], pThread->pipe_ends[1],n );
#  endif
							stat = select(pThread->pipe_ends[0] + 1, &set, NULL, NULL, &timeout);
							if(stat == -1)
							{
								lprintf(WIDE("select error %d %d"), errno, pThread->pipe_ends[0]); /* an error accured */
							}
							else if(stat == 0)
							{
#  ifdef DEBUG_PIPE_USAGE
								lprintf("timeout"); /* a timeout occured */
#  endif
							}
							else
							{
#  ifdef DEBUG_PIPE_USAGE
								lprintf(" immediate return?" );
#  endif
								stat = read( pThread->pipe_ends[0], &buf, 1 );
								// 1 = success
								// -1 will be an error (errno handled later)
                        // 0 would be end of file...
#  ifdef DEBUG_PIPE_USAGE
								lprintf( "Stat is now %d", stat );
#endif
							}
						}
#  ifdef DEBUG_PIPE_USAGE
						lprintf( "end read" );
#  endif
#else
# ifdef _NO_SEMTIMEDOP_
						stat = semop( pThread->semaphore, semdo, 1 );
# else
						struct timespec timeout;
						timeout.tv_nsec = ( n % 1000 ) * 1000000L;
						timeout.tv_sec = n / 1000;
						stat = semtimedop( pThread->semaphore, semdo, 1, &timeout );
# endif
#endif
					}
					else
					{
#ifdef USE_PIPE_SEMS
                  char buf;
#  ifdef DEBUG_PIPE_USAGE
						_lprintf(DBG_RELAY)(" Begin read on thread %p", pThread );
#  endif
						stat = read( pThread->pipe_ends[0], &buf, 1 );
#  ifdef DEBUG_PIPE_USAGE
						lprintf( "end read" );
#  endif
#else
						stat = semop( pThread->semaphore, semdo, 1 );
#endif
					}
					//lprintf( WIDE("After semval = %d %08lx"), semctl( pThread->semaphore, 0, GETVAL ), pThread->semaphore );
					//lprintf( WIDE("Lock passed.") );
					if( stat < 0 )
					{
						if( errno == EINTR )
						{
							//lprintf( WIDE("EINTR") );
							break;
							//continue;
						}
						if( errno == EAGAIN )
						{
						//lprintf( WIDE("EAGAIN?") );
						// timeout elapsed on semtimedop - or IPC_NOWAIT was specified
						// but since it's not, it must be the timeout condition.
							break;
						}
						if( errno == EIDRM )
						{
							lprintf( WIDE("Semaphore has been removed on us!?") );
							pThread->semaphore = -1;
							break;
						}
						if( errno == EINVAL )
						{
							lprintf( WIDE("Semaphore is no longer valid on this thread object... %d")
									 , pThread->semaphore );
							// this probably means that it has gone away..
							pThread->semaphore = -1;
							break;
						}
						lprintf( WIDE("stat from sempop on thread semaphore %p = %d (%d)")
								 , pThread
								 , stat
								 , stat<0?errno:0 );
						break;
					}
					else
					{
						// reset semaphore to nothing.... might
						// have been woken up MANY times.
							//lprintf( WIDE("Resetting our lock count from %d to 0....")
						//		 , semctl( pThread->semaphore, 0, GETVAL ));
#ifdef USE_PIPE_SEMS
						// flush? empty the pipe?
#else
						semctl( pThread->semaphore, 0, SETVAL, 0 );
#endif
						break;
					}
				}
			}
			else
			{
				lprintf( WIDE("Still an invalid semaphore? Dang.") );
				fprintf( stderr, WIDE("Out of semaphores.") );
				BAG_Exit(0);
			}
		}
#endif
		//pThread->flags.bSleeping = 0;
	}
	else
	{
		lprintf( WIDE("You, as a thread, do not exist, sorry.") );
	}
}

void  WakeableNamedThreadSleepEx( CTEXTSTR name, uint32_t n DBG_PASS )
{
	InternalWakeableNamedSleepEx( name, n, TRUE DBG_RELAY );
}

void  WakeableNamedSleepEx( CTEXTSTR name, uint32_t n DBG_PASS )
{
	InternalWakeableNamedSleepEx( name, n, FALSE DBG_RELAY );
}
void  WakeableSleepEx( uint32_t n DBG_PASS )
{
	InternalWakeableNamedSleepEx( NULL, n, FALSE DBG_RELAY );
}


#undef WakeableSleep
void  WakeableSleep( uint32_t n )
#define WakeableSleep(n) WakeableSleepEx(n DBG_SRC)
{
	WakeableSleepEx(n DBG_SRC);
}

//--------------------------------------------------------------------------

#ifdef __LINUX__
static void ContinueSignal( int sig )
{
	lprintf( WIDE("Sigusr1") );
}

// network is at GLOBAL_INIT_PRIORITY
PRIORITY_PRELOAD( IgnoreSignalContinue, GLOBAL_INIT_PRELOAD_PRIORITY-1 )
{
	//lprintf( "register handler for sigusr1" );
#if defined __ANDROID_OLD_PLATFORM_SUPPORT__
	bsd_signal( SIGUSR1, ContinueSignal );
#else
	signal( SIGUSR1, ContinueSignal );
#endif
}

static void AlarmSignal( int sig )
{
	//lprintf( "Received alarm" );
	WakeThread( globalTimerData.pTimerThread );
}

static void TimerWakeableSleep( uint32_t n )
{
	if( globalTimerData.pTimerThread )
	{
#ifndef USE_PIPE_SEMS
		if( !globalTimerData.flags.set_timer_signal )
		{
#  if defined __ANDROID_OLD_PLATFORM_SUPPORT__
			bsd_signal( SIGALRM, AlarmSignal );
#  else
			signal( SIGALRM, AlarmSignal );
#  endif
			globalTimerData.flags.set_timer_signal = 1;
		}
		if( n != SLEEP_FOREVER )
		{
			struct itimerval val;
			//lprintf( WIDE("Wakeable sleep in %") _32f WIDE(""), n );
			val.it_value.tv_sec = n / 1000;
			val.it_value.tv_usec = (n % 1000) * 1000;
			val.it_interval.tv_sec = 0;
			val.it_interval.tv_usec = 0;
			//lprintf( "setitimer %d %d", val.it_value.tv_sec, val.it_value.tv_usec );
			setitimer( ITIMER_REAL, &val, NULL );
		}
#endif
		if( globalTimerData.pTimerThread && globalTimerData.pTimerThread->semaphore != -1 )
		{
#ifdef USE_PIPE_SEMS
			InternalWakeableNamedSleepEx( NULL, n, FALSE DBG_SRC );
#else
			struct sembuf semdo;
			semdo.sem_num = 0;
			semdo.sem_op = -1;
			semdo.sem_flg = 0;
			//lprintf( WIDE("Before semval = %d %08lx")
			//		 , semctl( globalTimerData.pTimerThread->semaphore, 0, GETVAL )
			//		 , globalTimerData.pTimerThread->semaphore );
			while( semop( globalTimerData.pTimerThread->semaphore, &semdo, 1 ) < 0 )
			{
				if( !globalTimerData.pTimerThread )
					return;
				if( errno == EIDRM )
				{
					globalTimerData.pTimerThread->semaphore = -1; // closed.
					return;
				}
				//lprintf( WIDE("Before semval = %d"), semctl( globalTimerData.pTimerThread->semaphore, 0, GETVAL ) );
				if( errno == EINTR )
				{
					//lprintf( WIDE("Before semval = %d"), semctl( globalTimerData.pTimerThread->semaphore, 0, GETVAL ) );
					continue;
				}
				else
				{
					lprintf( WIDE("Semop failed: %d %08x"), errno, globalTimerData.pTimerThread->semaphore );
					break;
				}
			}
#endif
			//lprintf( WIDE("After semval = %d %08lx")
			//		 , semctl( globalTimerData.pTimerThread->semaphore, 0, GETVAL )
			//		 , globalTimerData.pTimerThread->semaphore );
		}
	}
}

#endif

//--------------------------------------------------------------------------
uintptr_t CPROC ThreadProc( PTHREAD pThread );
// results if the timer

int  IsThisThreadEx( PTHREAD pThreadTest DBG_PASS )
{
	PTHREAD pThread;
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	pThread
#ifdef HAS_TLS
		= MyThreadInfo.pThread;
#else
		= FindThread( GetMyThreadID() );
#endif
//   lprintf( WIDE("Found thread; %p is it %p?"), pThread, pThreadTest );
	if( pThread == pThreadTest )
		return TRUE;
	//lprintf( WIDE("Found thread; %p is not  %p?"), pThread, pThreadTest );
	return FALSE;
}

static int NotTimerThread( void )
{
	PTHREAD pThread;
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	pThread
#ifdef HAS_TLS
		= MyThreadInfo.pThread;
#else
		= FindThread( GetMyThreadID() );
#endif
	if( pThread && ( pThread->proc == ThreadProc ) )
		return FALSE;
	return TRUE;
}

//--------------------------------------------------------------------------

void  UnmakeThread( void )
{
	PTHREAD pThread;
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, (uintptr_t)_MyThreadInfo->nThread ) )
		Relinquish();
	pThread
#ifdef HAS_TLS
		= MyThreadInfo.pThread;
#else
		= FindThread( GetMyThreadID() );
#endif
	if( pThread )
	{
#ifdef _WIN32
		//lprintf( WIDE("Unmaking thread event! on thread %016"_64fx"x"), pThread->thread_ident );
		CloseHandle( pThread->thread_event->hEvent );
		{
			struct my_thread_info* _MyThreadInfo = GetThreadTLS();
			Deallocate( struct my_thread_info*, _MyThreadInfo );
			TlsSetValue( global_timer_structure->my_thread_info_tls, NULL );
		}
#else
		closesem( (POINTER)pThread, 0 );
#endif
		// unlink from globalTimerData.threads list.
		//if( ( (*pThread->me)=pThread->next ) )
		//	pThread->next->me = pThread->me;
		{
			int tmp = SetAllocateLogging( FALSE );
			Deallocate( TEXTSTR, pThread->thread_event_name );
#ifdef _WIN32
			Deallocate( TEXTSTR, pThread->thread_event->name );
			if( global_timer_structure )
				DeleteLink( &globalTimerData.thread_events, pThread->thread_event );
			Deallocate( PTHREAD_EVENT, pThread->thread_event );
#endif
			if( global_timer_structure )
				DeleteFromSet( THREAD, globalTimerData.threadset, pThread ) /*Release( pThread )*/;
			SetAllocateLogging( tmp );
		}
	}
	globalTimerData.lock_thread_create = 0;
}

//--------------------------------------------------------------------------
#ifdef __WATCOMC__
static void *ThreadWrapper( PTHREAD pThread )
#else
static uintptr_t CPROC ThreadWrapper( PTHREAD pThread )
#endif
{
	uintptr_t result = 0;
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
#ifdef __LINUX__
	//lprintf( "register handler for sigusr1 (for thread)" );
#  if defined __ANDROID_OLD_PLATFORM_SUPPORT__
	bsd_signal( SIGUSR1, ContinueSignal );
#  else
	signal( SIGUSR1, ContinueSignal );
#  endif
#endif
#ifdef _WIN32
	while( !pThread->hThread )
		Relinquish();
#endif
	pThread->flags.bStarted = 1;
	//DeAttachThreadToLibraries( TRUE );
	while( !pThread->flags.bReady )
		Relinquish();
#ifdef HAS_TLS
#  ifdef LOG_THREAD
	lprintf( "thread will be %p %p", MyThreadInfo.pThread, &MyThreadInfo );
	lprintf( "thread will be %p %p", pThread, &MyThreadInfo.pThread );
#  endif
	MyThreadInfo.pThread = pThread;
	MyThreadInfo.nThread =
#endif
		pThread->thread_ident = _GetMyThreadID();
	//DebugBreak();
	InitWakeup( pThread, NULL );
#ifdef LOG_THREAD
	Log1( WIDE("Set thread ident: %016"_64fx""), pThread->thread_ident );
#endif
	if( pThread->proc )
		 result = pThread->proc( pThread );
	//lprintf( WIDE("%s(%d):Thread is exiting... "), pThread->pFile, pThread->nLine );
	//DeAttachThreadToLibraries( FALSE );
	UnmakeThread();
#ifdef __LINUX__
	pThread->hThread = 0;
#else
	pThread->hThread = NULL;
#endif
	//lprintf( WIDE("%s(%d):Thread is exiting... "), pThread->pFile, pThread->nLine );
#ifdef __WATCOMC__
	return (void*)result;
#else
	return result;
#endif
}

//--------------------------------------------------------------------------
#ifdef __WATCOMC__
static void *SimpleThreadWrapper( PTHREAD pThread )
#else

static uintptr_t CPROC SimpleThreadWrapper( PTHREAD pThread )
#endif
{
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	uintptr_t result = 0;
#ifdef _WIN32
	while( !pThread->hThread )
	{
		Log( WIDE("wait for main thread to process...") );
		Relinquish();
	}
#endif
	pThread->flags.bStarted = 1;
	while( !pThread->flags.bReady )
		Relinquish();
#ifdef HAS_TLS
	MyThreadInfo.pThread = pThread;
	MyThreadInfo.nThread =
#endif
		pThread->thread_ident = GetMyThreadID();
	InitWakeup( pThread, NULL );
#ifdef LOG_THREAD
	Log1( WIDE("Set thread ident: %016") _64fx, pThread->thread_ident );
#endif
	if( pThread->proc )
		 result = pThread->simple_proc( (POINTER)GetThreadParam( pThread ) );
	//lprintf( WIDE("%s(%d):Thread is exiting... "), pThread->pFile, pThread->nLine );
	UnmakeThread();
	//lprintf( WIDE("%s(%d):Thread is exiting... "), pThread->pFile, pThread->nLine );
#ifdef __WATCOMC__
	return (void*)result;
#else
	return result;
#endif
}

//--------------------------------------------------------------------------

PTHREAD  MakeThread( void )
{
#ifdef HAS_TLS
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	if( MyThreadInfo.pThread )
		return MyThreadInfo.pThread;
	MyThreadInfo.nThread = _GetMyThreadID();
#endif
	{
		PTHREAD pThread;
		THREAD_ID thread_ident = _GetMyThreadID();
#ifndef HAS_TLS
		if( !(pThread = FindThread( thread_ident ) ) )
#endif
		{
			uintptr_t oldval;
			LOGICAL dontUnlock = FALSE;
			while( ( oldval = LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, (uintptr_t)thread_ident ) ) && oldval != thread_ident )
			{
				if( oldval != thread_ident )
					globalTimerData.lock_thread_create = oldval;
				Relinquish();
			}
			if( oldval == thread_ident )
				dontUnlock = TRUE;
			pThread = GetFromSet( THREAD, &globalTimerData.threadset ); /*Allocate( sizeof( THREAD ) )*/;
			//lprintf( WIDE("Get Thread %p"), pThread );
			MemSet( pThread, 0, sizeof( THREAD ) );
			pThread->flags.bLocal = TRUE;
			pThread->proc = NULL;
			pThread->param = 0;
			pThread->thread_ident = thread_ident;
			pThread->flags.bReady = 1;
			//if( ( pThread->next = globalTimerData.threads ) )
			//	globalTimerData.threads->me = &pThread->next;
			//pThread->me = &globalTimerData.threads;
			//globalTimerData.threads = pThread;

			InitWakeup( pThread, NULL );
			if( !dontUnlock )
				globalTimerData.lock_thread_create = 0;
#ifdef LOG_THREAD
			Log3( WIDE("Created thread address: %p %" PRIxFAST64 " at %p")
				 , pThread->proc, pThread->thread_ident, pThread );
#endif
		}
#ifdef HAS_TLS
		MyThreadInfo.pThread = pThread;
#endif
		return pThread;
	}
}

THREAD_ID GetThreadID( PTHREAD thread )
{
	if( thread )
		return thread->thread_ident;
	return 0;
}
THREAD_ID GetThisThreadID( void )
{
#if HAS_TLS
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	if( !MyThreadInfo.nThread )
	{
		MyThreadInfo.nThread = _GetMyThreadID();
	}
	return MyThreadInfo.nThread;
#else
	return MakeThread()->thread_ident;
#endif
}
uintptr_t GetThreadParam( PTHREAD thread )
{
	if( thread )
		return thread->param;
	return 0;
}

//--------------------------------------------------------------------------

PTHREAD  ThreadToEx( uintptr_t (CPROC*proc)(PTHREAD), uintptr_t param DBG_PASS )
{
	int success;
	PTHREAD pThread;
	while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
		Relinquish();
	do
	{
		pThread = GetFromSet( THREAD, &globalTimerData.threadset );
		if( !pThread )
			xlprintf(LOG_ALWAYS)( WIDE( "Thread to pThread allocation failed!" ) );
	} while( !pThread );
	/*AllocateEx( sizeof( THREAD ) DBG_RELAY );*/
	if( globalTimerData.flags.bLogThreadCreate )
		_lprintf(DBG_RELAY)( WIDE("Create New thread %p"), pThread );
	MemSet( pThread, 0, sizeof( THREAD ) );
	pThread->flags.bLocal = TRUE;
	pThread->proc = proc;
	pThread->param = param;
	pThread->thread_ident = 0;
#if DBG_AVAILABLE
	pThread->pFile = pFile;
	pThread->nLine = nLine;
#endif
   globalTimerData.lock_thread_create = 0;
#ifdef LOG_THREAD
	Log( WIDE("Begin Create Thread") );
#endif
#ifdef _WIN32
#if defined( __WATCOMC__ ) || defined( __WATCOM_CPLUSPLUS__ )
	pThread->hThread = (HANDLE)_beginthread( (void(*)(void*))ThreadWrapper, 8192, pThread );
#else
	{
		DWORD dwJunk;
		pThread->hThread = CreateThread( NULL, 1024
												 , (LPTHREAD_START_ROUTINE)(ThreadWrapper)
												 , pThread
												 , 0
												 , &dwJunk );
	}
#endif
	success = (int)(pThread->hThread!=NULL);
#else
	//lprintf( "Create thread..." );
	success = !pthread_create( &pThread->hThread, NULL, (void*(*)(void*))ThreadWrapper, pThread );
#endif
	if( success )
	{
		// link into list... it's a valid thread
		// the system claims that it can start one.
		//if( ( ( pThread->next = globalTimerData.threads ) ) )
		//   globalTimerData.threads->me = &pThread->next;
		//pThread->me = &globalTimerData.threads;
		//globalTimerData.threads = pThread;
		pThread->flags.bReady = 1;
		{
			uint32_t now = GetTickCount();
			while( !pThread->thread_event && ( now + 250 ) > GetTickCount()  )
				Relinquish();
		}
#ifdef LOG_THREAD
		Log3( WIDE("Created thread address: %p %016"_64fx" at %p")
		    , pThread->proc, pThread->thread_ident, pThread );
#endif
	}
	else
	{
		// unlink from globalTimerData.threads list.
		while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
			Relinquish();
		DeleteFromSet( THREAD, &globalTimerData.threadset, pThread ) /*Release( pThread )*/;
		globalTimerData.lock_thread_create = 0;
		pThread = NULL;
	}
	return pThread;
}

//--------------------------------------------------------------------------

PTHREAD  ThreadToSimpleEx( uintptr_t (CPROC*proc)(POINTER), POINTER param DBG_PASS )
{
	int success;
	PTHREAD pThread;
	while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
		Relinquish();
	pThread = GetFromSet( THREAD, &globalTimerData.threadset );
	/*AllocateEx( sizeof( THREAD ) DBG_RELAY );*/
#ifdef LOG_THREAD
	Log( WIDE("Creating a new thread... ") );
	lprintf( WIDE("New thread %p"), pThread );
#endif
	MemSet( pThread, 0, sizeof( THREAD ) );
	pThread->flags.bLocal = TRUE;
	pThread->simple_proc = proc;
	pThread->param = (uintptr_t)param;
	pThread->thread_ident = 0;
#if DBG_AVAILABLE
	pThread->pFile = pFile;
	pThread->nLine = nLine;
#endif
	globalTimerData.lock_thread_create = 0;
#ifdef LOG_THREAD
	Log( WIDE("Begin Create Thread") );
#endif
#ifdef _WIN32
#if defined( __WATCOMC__ ) || defined( __WATCOM_CPLUSPLUS__ )
	pThread->hThread = (HANDLE)_beginthread( (void(*)(void*))SimpleThreadWrapper, 8192, pThread );
#else
	{
		DWORD dwJunk;
		pThread->hThread = CreateThread( NULL, 1024
												 , (LPTHREAD_START_ROUTINE)(SimpleThreadWrapper)
												 , pThread
												 , 0
												 , &dwJunk );
	}
#endif
	success = (int)(pThread->hThread!=NULL);
#else
	//lprintf( "Create thread" );
	 success = !pthread_create( &pThread->hThread, NULL, (void*(*)(void*))SimpleThreadWrapper, pThread );
#endif
	if( success )
	{
		// link into list... it's a valid thread
		// the system claims that it can start one.
		//if( ( ( pThread->next = globalTimerData.threads ) ) )
		//   globalTimerData.threads->me = &pThread->next;
		//pThread->me = &globalTimerData.threads;
		//globalTimerData.threads = pThread;
		pThread->flags.bReady = 1;
		while( !pThread->thread_ident )
			Relinquish();
#ifdef LOG_THREAD
		lprintf( WIDE("Created thread address: %p %016"_64fx" at %p")
		       , pThread->proc, pThread->thread_ident, pThread );
#endif
	}
	else
	{
		// unlink from globalTimerData.threads list.
		while( LockedExchangePtrSzVal( &globalTimerData.lock_thread_create, 1 ) )
			Relinquish();
		DeleteFromSet( THREAD, &globalTimerData.threadset, pThread ) /*Release( pThread )*/;
		globalTimerData.lock_thread_create = 0;
		pThread = NULL;
	}
	return pThread;
}

//--------------------------------------------------------------------------

void  EndThread( PTHREAD thread )
{
	if( thread )
	{
#ifdef __LINUX__
#  ifndef __ANDROID__
		pthread_cancel( thread->hThread );
#  endif
#else
		TerminateThread( thread->hThread, 0xD1E );
#ifdef LOG_THREAD
		lprintf( WIDE("Killing thread...") );
#endif
		CloseHandle( thread->thread_event->hEvent );
#endif
	}
}

#if _WIN32
HANDLE GetThreadHandle( PTHREAD thread )
{
	if( thread )
		return thread->hThread;
	return INVALID_HANDLE_VALUE;
}
#endif

//--------------------------------------------------------------------------

static void DoInsertTimer( PTIMER timer )
{
	PTIMER check;
#ifdef ENABLE_CRITICALSEC_LOGGING
	BIT_FIELD bLock = globalTimerData.flags.bLogCriticalSections;
	SetCriticalLogging( 0 );
	globalTimerData.flags.bLogCriticalSections = 0;
#endif
	EnterCriticalSec( &globalTimerData.csGrab );
#ifdef ENABLE_CRITICALSEC_LOGGING
	globalTimerData.flags.bLogCriticalSections = bLock;
	SetCriticalLogging( bLock );
#endif
	if( !(check = globalTimerData.timers) )
	{
#ifdef LOG_INSERTS
		Log( WIDE("First(only known) timer!") );
#endif
		// subtract already existing time... (ONLY if first timer)
		//timer->delta -= ( globalTimerData.this_tick - globalTimerData.last_tick );
		(*(timer->me = &globalTimerData.timers))=timer;
#ifdef LOG_INSERTS
		Log( WIDE("Done with addition") );
#endif
#ifdef ENABLE_CRITICALSEC_LOGGING
		SetCriticalLogging( 0 );
		globalTimerData.flags.bLogCriticalSections = 0;
#endif
		LeaveCriticalSec( &globalTimerData.csGrab );
#ifdef ENABLE_CRITICALSEC_LOGGING
		globalTimerData.flags.bLogCriticalSections = bLock;
		SetCriticalLogging( bLock );
#endif
		return;
	}
	while( check )
	{
		// was previously <= which would schedule equal timers at the
		// head of the queue constantly.
#ifdef LOG_INSERTS
		lprintf( WIDE("Timer to store %d freq: %d delta: %d check delta: %d"), timer->ID, timer->frequency, timer->delta, check->delta );
#endif
		if( timer->delta < check->delta )
		{
			check->delta -= timer->delta;
#ifdef LOG_INSERTS
			Log3( WIDE("Storing before timer: %d delta %d next %d"), check->ID, timer->delta, check->delta );
#endif
			timer->next = check;
			(*(timer->me = check->me))=timer;
			check->me = &timer->next;
			break;
		}
		else
		{
			timer->delta -= check->delta;
		}
		if( !check->next )
		{
#ifdef LOG_INSERTS
			Log1( WIDE("Storing after last timer. Delta %d"), timer->delta );
#endif
			(*(timer->me = &check->next))=timer;
			break;
		}
		check = check->next;
	}
#ifdef LOG_INSERTS
	Log( WIDE("Done with addition") );
#endif
	if( !check )
		Log( WIDE("Fatal! Didn't add the timer!") );
#ifdef ENABLE_CRITICALSEC_LOGGING
	SetCriticalLogging( 0 );
	globalTimerData.flags.bLogCriticalSections = 0;
#endif
	LeaveCriticalSec( &globalTimerData.csGrab );
#ifdef ENABLE_CRITICALSEC_LOGGING
	globalTimerData.flags.bLogCriticalSections = bLock;
	SetCriticalLogging( bLock );
#endif
}

//--------------------------------------------------------------------------

static uintptr_t CPROC find_timer( POINTER p, uintptr_t psvID )
{
	uint32_t timerID = (uint32_t)psvID;
	PTIMER timer = (PTIMER)p;
	//lprintf( "Find to remove test %d==%d", timer->ID, timerID );
	if( timer->ID == timerID )
		return (uintptr_t)p;
	return 0;
}

static void  DoRemoveTimer( uint32_t timerID DBG_PASS )
{
	EnterCriticalSec( &globalTimerData.csGrab );
	{
		PTIMER timer = globalTimerData.timers;
		uintptr_t psvTimerResult = ForAllInSet( TIMER, &globalTimerData.timer_pool, find_timer, (uintptr_t)timerID );
		if( psvTimerResult )
			timer = (PTIMER)psvTimerResult;
		else
		{
			while( timer )
			{
				if( timer->ID == timerID )
					break;
				timer = timer->next;
			}
		}
		if( timer )
		{
			PTIMER tmp;
			if( ( tmp = ( (*timer->me) = timer->next ) ) )
			{
				// if I had a next - his refernece of thing that points at him is mine.
				tmp->delta += timer->delta;
				tmp->me = timer->me;
			}
			DeleteFromSet( TIMER, &globalTimerData.timer_pool, timer );
		}
		else
			_lprintf(DBG_RELAY)( WIDE("Failed to find timer to grab") );
	}
	LeaveCriticalSec( &globalTimerData.csGrab );
}

//--------------------------------------------------------------------------

static void InsertTimer( PTIMER timer DBG_PASS )
{
	if( NotTimerThread() )
	{
		if( globalTimerData.flags.away_in_timer )
		{ // if it's away - should be safe to add a new timer
			globalTimerData.flags.insert_while_away = 1;
			// set that we're adding a timer while away
			if( globalTimerData.flags.away_in_timer )
			{
				// if the thread is still away - we can add the timer...
#ifdef LOG_SLEEPS
				lprintf( "Timer is away, just add this new timer back in.." );
#endif
				DoInsertTimer( timer );
				globalTimerData.flags.insert_while_away = 0;
				return;
			}
			// otherwise he came back before we set our addin
			// therefore it should be safe to schedule.
			globalTimerData.flags.insert_while_away = 0;
		}
#ifdef LOG_INSERTS
		Log( WIDE("Inserting timer...to wait for change allow") );
#endif
											  // lockout multiple additions...
		EnterCriticalSec( &globalTimerData.cs_timer_change );
#ifdef LOG_INSERTS
		Log( WIDE("Inserting timer...to wait for free add") );
#endif
		// don't add a timer while there's one being added...
		while( globalTimerData.add_timer )
		{
			WakeThread(globalTimerData.pTimerThread);
			//Relinquish();
		}
#ifdef LOG_INSERTS
		Log( WIDE("Inserting timer...setup dataa") );
#endif
		globalTimerData.add_timer = timer;
		LeaveCriticalSec( &globalTimerData.cs_timer_change );
		// it might be sleeping....
#ifdef LOG_INSERTS
		Log( WIDE("Inserting timer...wake and done") );
#endif

#ifdef LOG_SLEEPS
		lprintf( "Wake timer thread." );
#endif
		// wake this thread because it's current scheduled delta (ex 1000ms)
		// may put it's sleep beyond the frequency of this timer (ex 10ms)
		WakeThreadEx(globalTimerData.pTimerThread DBG_RELAY);
	}
	else
	{
		EnterCriticalSec( &globalTimerData.csGrab );
		// have to assume that we're away in callback
		// in order to get here... there's no other way
		// for this routine to be called and BE the timer thread.
		// therefore - safe to add it.
		DoInsertTimer( timer );
#ifdef LOG_SLEEPS
		lprintf( "Insert timer not dispatched." );
#endif
		if( globalTimerData.timers == timer )
		{
#ifdef LOG_SLEEPS
			lprintf( "Wake timer thread." );
#endif
			WakeThread(globalTimerData.pTimerThread);
		}
		LeaveCriticalSec( &globalTimerData.csGrab );
	}
}

//--------------------------------------------------------------------------

static PTIMER GrabTimer( PTIMER timer )
{
	// if a timer has been grabbed, it won't be grabbed...
	// but if a timer is being grabbed, it might get grabbed twice.
	if( timer && timer->me )
	{
		// the thing that points at me points at my next....
#ifdef LOG_INSERTS
		Log1( WIDE("Grab Timer: %d"), timer->ID );
#endif
		if( ( (*timer->me) = timer->next ) )
		{
			// if I had a next - his refernece of thing that points at him is mine.
			timer->next->me = timer->me;
		}
		timer->next = NULL;
		timer->me = NULL;
		return timer;
	}
	return NULL;
}

//--------------------------------------------------------------------------

static int CPROC ProcessTimers( uintptr_t psvForce )
{
	if( global_timer_structure )
	{
	PTIMER timer;
#ifdef ENABLE_CRITICALSEC_LOGGING
	BIT_FIELD bLock = globalTimerData.flags.bLogCriticalSections;
#endif
	uint32_t newtick;

	if( globalTimerData.flags.bExited )
		return -1;
	if( !psvForce && !IsThisThread( globalTimerData.pTimerThread ) )
	{
		//Log( WIDE("Unknown thread attempting to process timers...") );
		return -1;
	}
#ifndef _WIN32
	//nice( -3 ); // allow ourselves a bit more priority...
#endif
	{
		// there are timers - and there's one which wants to be added...
		// if there's no timers - just sleep here...
		while( ( !globalTimerData.add_timer && !globalTimerData.timers ) || globalTimerData.flags.bHaltTimers )
		{
			if( !psvForce )
				return 1;
#ifdef LOG_SLEEPS
			lprintf( WIDE("Timer thread sleeping forever...") );
#endif
#ifdef __LINUX__
			if( globalTimerData.pTimerThread )
				TimerWakeableSleep( SLEEP_FOREVER );
#else
			WakeableSleep( SLEEP_FOREVER );
#endif
			// had no timers - but NOW either we woke up by default...
			// OR - we go kicked awake - so mark the beginning of known time.
#ifdef LOG_LATENCY
			Log( WIDE("Re-synch first tick...") );
#endif
			globalTimerData.last_tick = timeGetTime();//GetTickCount();
		}
		// add and delete new/old timers here...
		// should be the next event after sleeping (low var-sleep top const-sleep)
		if( globalTimerData.add_timer )
		{
#ifdef LOG_INSERTS
			Log( WIDE("Adding timer really...") );
#endif
			DoInsertTimer( globalTimerData.add_timer );
			globalTimerData.add_timer = NULL;
		}
		if( globalTimerData.del_timer )
		{
#ifdef LOG_INSERTS
			Log( WIDE("Scheduled remove timer...") );
#endif
			DoRemoveTimer( globalTimerData.del_timer DBG_SRC );
			globalTimerData.del_timer = 0;
		}
		// get the time now....
		newtick = globalTimerData.this_tick = timeGetTime();//GetTickCount();
#ifdef LOG_LATENCY
		Log3( WIDE("total - Tick: %u Last: %u  delta: %u"), globalTimerData.this_tick, globalTimerData.last_tick, globalTimerData.this_tick-globalTimerData.last_tick );
#endif
		//if( globalTimerData.timers )
		//	 delay_skew = globalTimerData.this_tick-globalTimerData.last_tick - globalTimerData.timers->delta;
		// delay_skew = 0; // already chaotic...
		//if( timers )
		//	Log1( WIDE("timer delta: %ud"), timers->delta );

#ifdef ENABLE_CRITICALSEC_LOGGING
		SetCriticalLogging( 0 );
		globalTimerData.flags.bLogCriticalSections = 0;
#endif
		while( ( EnterCriticalSec( &globalTimerData.csGrab ), timer = globalTimerData.timers ) &&
				( (int32_t)( newtick - globalTimerData.last_tick ) >= timer->delta ) )
		{
#ifdef ENABLE_CRITICALSEC_LOGGING
			globalTimerData.flags.bLogCriticalSections = bLock;
			SetCriticalLogging( bLock );
#endif
#ifdef LOG_LATENCY
#ifdef _DEBUG
			_xlprintf( 1, timer->pFile, timer->nLine )( WIDE("Tick: %u Last: %u  delta: %u Timerdelta: %u")
																	, globalTimerData.this_tick, globalTimerData.last_tick, globalTimerData.this_tick-globalTimerData.last_tick, timer->delta );
#else
			lprintf( WIDE("Tick: %u Last: %u  delta: %u Timerdelta: %u")
					 , globalTimerData.this_tick, globalTimerData.last_tick, globalTimerData.this_tick-globalTimerData.last_tick, timer->delta );
#endif
#endif
			// also enters csGrab... should be ok.
			GrabTimer( timer );
#ifdef ENABLE_CRITICALSEC_LOGGING
			SetCriticalLogging( 0 );
			globalTimerData.flags.bLogCriticalSections = 0;
#endif
			LeaveCriticalSec( &globalTimerData.csGrab );
#ifdef ENABLE_CRITICALSEC_LOGGING
			globalTimerData.flags.bLogCriticalSections = bLock;
			SetCriticalLogging( bLock );
#endif
			globalTimerData.last_tick += timer->delta;
			if( timer->callback )
			{
#ifdef _WIN32
#if PARANOID
				if( IsBadCodePtr( (FARPROC)timer->callback ) )
				{
					Log1( WIDE("Timer %d proc has been unloaded! kiling timer"), timer->ID );
					timer->frequency = 0;
				}
				else
#endif
#endif
				{
					//#ifdef LOG_DISPATCH
					static int level;
					if( globalTimerData.flags.bLogTimerDispatch )
					{
						level++;
#ifdef _DEBUG
						lprintf( WIDE("%d Dispatching timer %")_32fs WIDE(" freq %")_32fs WIDE(" %s(%d)"), level, timer->ID, timer->frequency
								 , timer->pFile, timer->nLine );
#else
						lprintf( WIDE("%d Dispatching timer %") _32fs WIDE(" freq %") _32fs, level, timer->ID, timer->frequency );
#endif
					}
					//#endif
					globalTimerData.current_timer = timer;
					timer->flags.bRescheduled = 0;
					globalTimerData.flags.away_in_timer = 1;
					globalTimerData.CurrentTimerID = timer->ID;
					timer->callback( timer->userdata );
					if( globalTimerData.flags.bLogTimerDispatch )
					{
						level--;
						lprintf( WIDE("timer done. (%d)"), level );
					}
					globalTimerData.flags.away_in_timer = 0;
					while( globalTimerData.flags.insert_while_away )
					{
						// request for insert while away... allow it to
						// get scheduled...
						Relinquish();
					}
					globalTimerData.current_timer = NULL;
				}
				// allow timers to be added while away in this
				// timer's callback... so wait for it to finish.
				// but do - clear away status so that ANOTHER
				// timer will be held waiting...
			}
			// reset timer to frequency here

			// if a VERY long time has elapsed, next timer occurs its
			//  frequency after now.  Otherwise we may NEVER get out
			//  of processing this timer.
			// this point should be optioned whether the timer is
			// a guaranteed tick, or whether it's sloppy.
			if( timer->frequency || timer->flags.bRescheduled )
			{
				if( timer->flags.bRescheduled )
				{
					timer->flags.bRescheduled = 0;
					// delta will have been set for next run...
					// therefore do not schedule it ourselves.
				}
				else
				{
					if( ( newtick - globalTimerData.last_tick ) > timer->frequency )
					{
#ifdef LOG_LATENCY_LIGHT
						lprintf( WIDE("Timer used more time than its frequency.  Scheduling at 1 ms.") );
#endif
						timer->delta = ( newtick - globalTimerData.last_tick ) + 1;
					}
					else
					{
#ifdef LOG_LATENCY_LIGHT
						// timer alwyas goes +1 frequency from its base tick.
						lprintf( WIDE("Scheduling timer at 1 frequency.") );
#endif
						timer->delta = timer->frequency;
					}
				}
				DoInsertTimer( timer );
			}
			else
			{
#ifdef LOG_INSERTS
				lprintf( WIDE("Removing one shot timer. %d"), timer->ID );
#endif
				// was a one shot timer.
				DeleteFromSet( TIMER, &globalTimerData.timer_pool, timer );
				timer = NULL;
			}
		}
#ifdef ENABLE_CRITICALSEC_LOGGING
		SetCriticalLogging( 0 );
		globalTimerData.flags.bLogCriticalSections = 0;
#endif
		LeaveCriticalSec( &globalTimerData.csGrab );
#ifdef ENABLE_CRITICALSEC_LOGGING
		globalTimerData.flags.bLogCriticalSections = bLock;
		SetCriticalLogging( bLock );
#endif
		if( timer )
		{
#ifdef LOG_LATENCY
			lprintf( WIDE("Pending timer in: %d Sleeping %d (%d) [%d]")
					 , timer->delta
					 , timer->delta - (newtick-globalTimerData.last_tick)
					 , timer->delta - (globalTimerData.this_tick-globalTimerData.last_tick)
					 , newtick - globalTimerData.this_tick
					 );
#endif
			globalTimerData.last_sleep = ( timer->delta - ( globalTimerData.this_tick - globalTimerData.last_tick ) );
			if( globalTimerData.last_sleep < 0 )
			{
				lprintf( WIDE( "next pending sleep is %d" ), globalTimerData.last_sleep );
				globalTimerData.last_sleep = 1;
			}
#ifdef LOG_LATENCY
			Log1( WIDE("Sleeping %d"), globalTimerData.last_sleep );
#endif
			if( !psvForce )
				return 1;
			if( globalTimerData.last_sleep )
			{
#ifdef __LINUX__
				TimerWakeableSleep( globalTimerData.last_sleep );
#else
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
				WakeableSleepEx( globalTimerData.last_sleep, timer->pFile, timer->nLine );
#else
				WakeableSleepEx( globalTimerData.last_sleep );
#endif
#endif
			}
			if( !global_timer_structure || globalTimerData.flags.bExited )
				return -1;
		}
		// else no timers - go back up to the top - where we sleep.
	}
	//Log( WIDE("Timer thread is exiting...") );
	return 1;
	}
	return -1;
}


//--------------------------------------------------------------------------

uintptr_t CPROC ThreadProc( PTHREAD pThread )
{
	InitializeCriticalSec( &globalTimerData.cs_timer_change );
	globalTimerData.pTimerThread = pThread;
#ifndef __NO_IDLE__
	AddIdleProc( ProcessTimers, (uintptr_t)0 );
#endif
#ifndef _WIN32
	nice( -3 ); // allow ourselves a bit more priority...
#endif
	//Log( WIDE("Permanently lock timers - indicates that thread is running...") );
	globalTimerData.lock_timers = 1;
	//Log( WIDE("Get first tick") );
	globalTimerData.last_tick = timeGetTime();//GetTickCount();
	while( ProcessTimers( 1 ) > 0 );
	//Log( WIDE("Timer thread is exiting...") );
	globalTimerData.pTimerThread = NULL;
	return 0;
}

//--------------------------------------------------------------------------
#if 0
// this would really be a good thing to impelment someday.
static void *WatchdogProc( void *unused )
{
	// this checks the running status of the main thread(s)
// if there is a paused thread, then a new thread is created.
// yeah see dekware( syscore/nexus.c WakeAThread() )
	return 0;
}
#endif
//--------------------------------------------------------------------------

uint32_t  AddTimerExx( uint32_t start, uint32_t frequency, TimerCallbackProc callback, uintptr_t user DBG_PASS )
{
	PTIMER timer = GetFromSet( TIMER, &globalTimerData.timer_pool );
					 //timer = AllocateEx( sizeof( TIMER ) DBG_RELAY );
	MemSet( timer, 0, sizeof( TIMER ) );
	if( start && !frequency )
	{
		//"Creating one shot timer %d long", start );
	}
	timer->delta	 = (int32_t)start; // first time for timer to fire... may be 0
	timer->frequency = frequency;
	timer->callback = callback;
	timer->ID = globalTimerData.timerID++;
	timer->userdata = user;
#ifdef _DEBUG
	timer->pFile = pFile;
	timer->nLine = nLine;
#endif
	if( !globalTimerData.pTimerThread )
	{

		//Log( WIDE("Starting \"a\" timer thread!!!!" ) );
		if( !( ThreadTo( ThreadProc, 0 ) ) )
		{
			//Log1( WIDE("Failed to start timer ThreadProc... %d"), GetLastError() );
			return 0;
		}
		while( !globalTimerData.lock_timers )
			Relinquish();
		//Log1( WIDE("Thread started successfully? %d"), GetLastError() );

		 // make sure that the thread is running, and had setup its
		 // locks, and tick reference
	}
	//_xlprintf(1 DBG_RELAY)( WIDE("Inserting newly created timer.") );
	InsertTimer( timer DBG_RELAY );
	// don't need to sighup here, cause we MUST have permission
	// from the idle thread to add the timer, which means we issue it
	// a sighup to make it wake up and allow us to post.
#ifdef LOG_INSERTS
	_lprintf( DBG_RELAY )( "Resulting timer ID: %d", timer->ID );
#endif
	return timer->ID;
}

#undef AddTimerEx
uint32_t  AddTimerEx( uint32_t start, uint32_t frequency, void (CPROC*callback)(uintptr_t user), uintptr_t user )
{
	return AddTimerExx( start, frequency, callback, user DBG_SRC );
}

//--------------------------------------------------------------------------

void  RemoveTimerEx( uint32_t ID DBG_PASS )
{
	// Lockout multiple changes at a time...
	if( !NotTimerThread() && // IS timer thread..
		( ID != globalTimerData.CurrentTimerID ) ) // and not in THIS timer...
	{
		// is timer thread itself... safe to remove the timer....
		DoRemoveTimer( ID DBG_SRC );
		return;
	}
	EnterCriticalSec( &globalTimerData.cs_timer_change );
	// only allow one delete at a time...
	while( globalTimerData.del_timer )
	{
#ifdef LOG_INSERTS
		lprintf( "pending timer delete, wait." );
#endif
		if( !NotTimerThread() ) // IS timer thread...
		{
#ifdef LOG_INSERTS
			lprintf( "is not the timer." );
#endif
			if( globalTimerData.del_timer != globalTimerData.CurrentTimerID )
			{
#ifdef LOG_INSERTS
				lprintf( "schedule timer is not the current timer..." );
#endif
				DoRemoveTimer( globalTimerData.del_timer DBG_SRC );
				globalTimerData.del_timer = 0;
			}
			if( ID != globalTimerData.CurrentTimerID )
			{
#ifdef LOG_INSERTS
				lprintf( "removing timer is not the current timer" );
#endif
				DoRemoveTimer( ID DBG_SRC );
				return;
			}
		}
		else
			Relinquish();
	}
	// now how to set del_timer to a valid timer?!
#ifdef LOG_INSERTS
	_lprintf(DBG_RELAY)( "Set del_timer to schedule delete." );
#endif
	globalTimerData.del_timer = ID;
	LeaveCriticalSec( &globalTimerData.cs_timer_change );
	if( NotTimerThread() )
	{
#ifdef LOG_INSERTS
		lprintf( "wake thread, scheduled delete" );
#endif
		//Log( WIDE("waking timer thread to indicate deletion...") );
		WakeThread( globalTimerData.pTimerThread );
	}
}


#undef RemoveTimer
void  RemoveTimer( uint32_t ID )
{
   RemoveTimerEx( ID DBG_SRC );
}

//--------------------------------------------------------------------------

static void InternalRescheduleTimerEx( PTIMER timer, uint32_t delay )
{
	if( timer )
	{
		PTIMER bGrabbed = GrabTimer( timer );
		timer->flags.bRescheduled = 1;
		timer->delta = (int32_t)delay;  // should never pass a negative value here, but delta can be negative.
#ifdef LOG_SLEEPS
		lprintf( "Reschedule at %d  %p", timer->delta, bGrabbed );
#endif
		if( bGrabbed )
		{
			//lprintf( WIDE("Rescheduling timer...") );
			DoInsertTimer( timer );
			if( timer == globalTimerData.timers )
			{
#ifdef LOG_SLEEPS
				lprintf( "We cheated to insert - so create a wake." );
#endif
				WakeThread( globalTimerData.pTimerThread );
			}
		}
	}
}

//--------------------------------------------------------------------------

// should lock this...
void  RescheduleTimerEx( uint32_t ID, uint32_t delay )
{
	PTIMER timer;
	EnterCriticalSec( &globalTimerData.csGrab );
	if( !ID )
	{
		timer =globalTimerData.current_timer;
	}
	else
	{
		timer = globalTimerData.timers;
		while( timer && timer->ID != ID )
			timer = timer->next;

		if( !timer )
		{
			// this timer is not part of the list if it's
			// dispatched and we get here (timer itself rescheduling itself)
			if( globalTimerData.current_timer && globalTimerData.current_timer->ID == ID )
				timer = globalTimerData.current_timer;
		}
	}
	InternalRescheduleTimerEx( timer, delay );
	LeaveCriticalSec( &globalTimerData.csGrab );
}

//--------------------------------------------------------------------------

void  RescheduleTimer( uint32_t ID )
{
	PTIMER timer = globalTimerData.timers;
	EnterCriticalSec( &globalTimerData.csGrab );
	while( timer && timer->ID != ID )
		timer = timer->next;
	if( !timer )
	{
		if( globalTimerData.current_timer && globalTimerData.current_timer->ID == ID )
			timer = globalTimerData.current_timer;
	}
	if( timer )
	{
		InternalRescheduleTimerEx( timer, timer->frequency );
	}
	LeaveCriticalSec( &globalTimerData.csGrab );
}

//--------------------------------------------------------------------------

static void OnDisplayPause( WIDE("@Internal Timers") _WIDE(TARGETNAME) )( void )
{
	globalTimerData.flags.bHaltTimers = 1;
}

//--------------------------------------------------------------------------
static void OnDisplayResume( WIDE("@Internal Timers") _WIDE(TARGETNAME))( void )
{
	globalTimerData.flags.bHaltTimers = 0;
	if( globalTimerData.pTimerThread )
		WakeThread( globalTimerData.pTimerThread );
}

//--------------------------------------------------------------------------

void  ChangeTimerEx( uint32_t ID, uint32_t initial, uint32_t frequency )
{
	PTIMER timer = globalTimerData.timers;
	EnterCriticalSec( &globalTimerData.csGrab );
	while( timer && timer->ID != ID )
		timer = timer->next;
	if( timer )
	{
		timer->frequency = frequency;
		InternalRescheduleTimerEx( timer, initial );
	}
	LeaveCriticalSec( &globalTimerData.csGrab );
}

//--------------------------------------------------------------------------

#ifndef USE_NATIVE_CRITICAL_SECTION
LOGICAL  EnterCriticalSecEx( PCRITICALSECTION pcs DBG_PASS )
{
	int d;
	THREAD_ID prior = 0;
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
#ifdef _DEBUG
	if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
		_lprintf( DBG_RELAY )( WIDE("Enter critical section %p (owner) %")_64fx, pcs, pcs->dwThreadID );
#endif
#endif
	do
	{
		d=EnterCriticalSecNoWaitEx( pcs, &prior DBG_RELAY );
		if( d < 0 )
			Relinquish();
		else if( d == 0 )
		{
			if( pcs->dwThreadID )
			{
#ifdef ENABLE_CRITICALSEC_LOGGING
#  ifdef _DEBUG
				if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
					lprintf( WIDE("Failed to enter section... sleeping (forever)...") );
#  endif
#endif
				WakeableNamedThreadSleepEx( WIDE("sack.critsec"), SLEEP_FOREVER DBG_RELAY );
#ifdef ENABLE_CRITICALSEC_LOGGING
#  ifdef _DEBUG
				if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
					lprintf( WIDE("Allowed to retry section entry, woken up...") );
#  endif
#endif
			}
#ifdef ENABLE_CRITICALSEC_LOGGING
#  ifdef _DEBUG
			else
			{
				if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
					lprintf( WIDE("Lock Released while we logged?") );
			}
#  endif
#endif
		}
		else {
			if( prior ) {
#ifdef ENABLE_CRITICALSEC_LOGGING
#  ifdef _DEBUG
				if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
					lprintf( WIDE("Entered section, restore prior waiting thread. %") _64fx  WIDE(" %" ) _64fx, prior, pcs->dwThreadWaiting );
#  endif
#endif
			}
		}
		// after waking up, this will re-aquire a lock, and
		// set the prior waiting ID into the criticalsection
		// this will then wake that process when this lock is left.
	}
	while( (d <= 0) );
	return TRUE;
}
#endif
//-------------------------------------------------------------------------

#ifndef USE_NATIVE_CRITICAL_SECTION
LOGICAL  LeaveCriticalSecEx( PCRITICALSECTION pcs DBG_PASS )
{
	THREAD_ID dwCurProc;
#ifdef _DEBUG
	uint32_t curtick = timeGetTime();//GetTickCount();
#endif
#ifdef ENABLE_CRITICALSEC_LOGGING
	if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
		_xlprintf( LOG_NOISE DBG_RELAY )( WIDE("Begin leave critical section %p %") _64fx, pcs, pcs->dwThreadWaiting );
#endif
	while( LockedExchange( &pcs->dwUpdating, 1 )
#ifdef _DEBUG
			&& ( (curtick+2000) > timeGetTime() )//GetTickCount() )
#endif
	)
	{
#ifdef ENABLE_CRITICALSEC_LOGGING
		if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
			_lprintf( DBG_RELAY )( WIDE("On leave - section is updating, wait...") );
#endif
		Relinquish();
	}
	dwCurProc = GetMyThreadID();
#ifdef _DEBUG
	if( (curtick+2000) <= timeGetTime() )//GetTickCount() )
	{
		lprintf( WIDE( "Timeout during critical section wait for lock.  No lock should take more than 1 task cycle" ) );
		DebugBreak();
		return FALSE;
	}
#endif
	if( !( pcs->dwLocks & ~SECTION_LOGGED_WAIT ) )
	{
#ifdef ENABLE_CRITICALSEC_LOGGING
		if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
			_lprintf( DBG_RELAY )( WIDE("Leaving a blank critical section") );
#endif
		DebugBreak();
		pcs->dwUpdating = 0;
		return FALSE;
	}

	if( pcs->dwThreadID == dwCurProc )
	{
#ifdef DEBUG_CRITICAL_SECTIONS
#  ifdef _DEBUG
		pcs->pFile[pcs->nPrior] = pFile;
		pcs->nLine[pcs->nPrior] = nLine;
#  else
		pcs->pFile[pcs->nPrior] = __FILE__;
		pcs->nLine[pcs->nPrior] = __LINE__;
#  endif
		pcs->nLineCS[pcs->nPrior] = __LINE__;
		pcs->isLock[pcs->nPrior] = 0;
		pcs->dwThreadPrior[pcs->nPrior] = dwCurProc;
		pcs->nPrior = (pcs->nPrior + 1) % MAX_SECTION_LOG_QUEUE;
#endif
		pcs->dwLocks--;
#ifdef ENABLE_CRITICALSEC_LOGGING
		if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
			lprintf( WIDE("Remaining locks... %08") _32fx, pcs->dwLocks );
#endif
		if( !( pcs->dwLocks & ~(SECTION_LOGGED_WAIT) ) )
		{
			pcs->dwLocks = 0;
			pcs->dwThreadID = 0;
			// wake the prior (if there is one sleeping)

			if( pcs->dwThreadWaiting )
			{
				pcs->dwUpdating = 0;
#ifdef ENABLE_CRITICALSEC_LOGGING
				if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
					_lprintf( DBG_RELAY )( WIDE("%8")_64fx WIDE(" Waking a thread which is waiting..."), pcs->dwThreadWaiting );
#endif
				// don't clear waiting... so the proper thread can
				// allow itself to claim section...
				WakeNamedThreadSleeperEx( WIDE("sack.critsec"), pcs->dwThreadWaiting DBG_RELAY );
				//WakeThreadIDEx( wake DBG_RELAY);
			}
			else
				pcs->dwUpdating = 0;
			return TRUE;
		}
	}
	else {
#ifdef ENABLE_CRITICALSEC_LOGGING
		if( global_timer_structure && globalTimerData.flags.bLogCriticalSections )
		{
			_lprintf( DBG_RELAY )(WIDE( "Sorry - you can't leave a section owned by %016" )_64fx WIDE( " locks:%08" )_32fx
#  ifdef DEBUG_CRITICAL_SECTIONS
				WIDE( "%s(%d)..." )
#  endif
				, pcs->dwThreadID
				, pcs->dwLocks
#  ifdef DEBUG_CRITICAL_SECTIONS
				, (pcs->pFile[(pcs->nPrior + 15) % MAX_SECTION_LOG_QUEUE]) ? (pcs->pFile[(pcs->nPrior + 15) % MAX_SECTION_LOG_QUEUE]) : WIDE( "Unknown" ), pcs->nLine[(pcs->nPrior + 15) % MAX_SECTION_LOG_QUEUE]
#  endif
				);
		}
#endif
		DebugBreak();
	}
	pcs->dwUpdating = 0;
	return FALSE;
}
#endif
//--------------------------------------------------------------------------

void DeleteCriticalSec( PCRITICALSECTION pcs )
{
	// ya I don't have anything to do here...
	return;
}

#ifdef _WIN32
HANDLE  GetWakeEvent( void )
{
#if HAS_TLS
	struct my_thread_info* _MyThreadInfo = GetThreadTLS();
	if( !MyThreadInfo.pThread )
		MakeThread();
	return MyThreadInfo.pThread->thread_event->hEvent;
#else
	return MakeThread()->thread_event->hEvent;
#endif
}
#endif

#ifdef __cplusplus
};//	namespace timers {
};//namespace sack {
#endif
//--------------------------------------------------------------------------

// $Log: timers.c,v $
// Revision 1.140  2005/06/22 23:13:51  jim
// Differentiate the normal logging of 'entered, left section' but leave in notable exception case logging when enabling critical section debugging.
//
// Revision 1.108  2005/01/23 11:28:24  panther
// Thread ID modification broke timer...
//
// 400 lines of logging removed... version 1.109?
