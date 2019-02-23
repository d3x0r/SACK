/*
 *  Crafted by Jim Buckeyne
 * 
 *  (c)2001-2006++ Freedom Collective
 *
 *  Provide API interface for timers, critical sections
 *  and other thread things.
 *
 */

#ifndef TIMERS_DEFINED
/* timers.h mutliple inclusion protection symbol. */
#define TIMERS_DEFINED

#if defined( _WIN32 )
// on windows, we add a function that returns HANDLE
#  include <stdhdrs.h>
#endif
#include <sack_types.h>
#include <sharemem.h>

#ifdef __LINUX__
#  include <pthread.h>
#endif

#ifndef _TIMER_NAMESPACE
#ifdef __cplusplus
#define _TIMER_NAMESPACE namespace timers {
/* define a timer library namespace in C++. */
#define TIMER_NAMESPACE SACK_NAMESPACE namespace timers {
/* define a timer library namespace in C++ end. */
#define TIMER_NAMESPACE_END } SACK_NAMESPACE_END
#else
#define _TIMER_NAMESPACE
#define TIMER_NAMESPACE 
#define TIMER_NAMESPACE_END
#endif
#endif

// this is a method replacement to use PIPEs instead of SEMAPHORES
// replacement code only affects linux.
#if defined( __QNX__ ) || defined( __MAC__) || defined( __LINUX__ )
#  if defined( __ANDROID__ ) || defined( EMSCRIPTEN ) || defined( __MAC__ )
// android > 21 can use pthread_mutex_timedop
#    define USE_PIPE_SEMS
#  else
//   Default behavior is to use pthread_mutex_timedlock for wakeable sleeps.
// no semtimedop; no semctl, etc
//#    include <sys/sem.h>
//originally used semctl; but that consumes system resources that are not
//cleaned up when the process exits.
#endif
#endif

#ifdef USE_PIPE_SEMS
#  define _NO_SEMTIMEDOP_
#endif


SACK_NAMESPACE
/* This namespace contains methods for working with timers and
   threads. Since timers are implemented in an asynchronous
   thread, the thread creation and control can be exposed here
   also.
   
   
   
   ThreadTo
   
   WakeThread
   
   WakeableSleep [Example]
   
   AddTimer
   
   RemoveTimer
   
   RescheduleTimer
   
   EnterCriticalSec see Also
 EnterCriticalSecNoWait
   
   LeaveCriticalSec                                            */
_TIMER_NAMESPACE


#ifdef TIMER_SOURCE 
#define TIMER_PROC(type,name) EXPORT_METHOD type CPROC name
#else
/* Defines import export and call method for timers. Looks like
   timers are native calltype by default instead of CPROC.      */
#define TIMER_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#if defined( __LINUX__ ) || defined( __ANDROID__ )
TIMER_PROC( uint32_t, timeGetTime )( void );
TIMER_PROC( uint32_t, GetTickCount )( void );
TIMER_PROC( void, Sleep )( uint32_t ms );
#endif

/* Function signature for user callbacks passed to AddTimer. */
typedef void (CPROC *TimerCallbackProc)( uintptr_t psv );
/* Adds a new periodic timer. From now, until the timer is
   removed with RemoveTimer, it will call the timer procedure at
   the specified frequency of milliseconds. The delay until the
   first time the timer fires can be specified independant of
   frequency. If it is not specified, the first time the timer
   will get invoked is at +1 frequency from now.
   
   
   Parameters
   start :      how long in milliseconds until the timer starts. Can
                be 0 and timer will fire at the next opportunity.
   frequency :  how long the delay is between event invocations,
                in milliseconds.
   callback :   user routine to call when the timer's delay
                expires.
   user :       user data to pass to the callback when it is
                invoked.
   
   Returns
   a 32 bit ID that identifies the timer for this application.
   Example
   First some setup valid for all timer creations...
   <code lang="c++">
   void CPROC TimerProc( uintptr_t user_data )
   {
       // user_data of the timer is the 'user' parameter passed to AddTimer(Exx)
   }
   </code>
   you might want to save this for something like
   RescheduleTimer
   <code>
   uint32_t timer_id;
   
   
   </code>
   Create a simple timer, it will fire at 250 milliseconds from
   now, and again every 250 milliseconds from the time it
   starts.
   <code lang="c++">
   timer_id = AddTimer( 250, TimerProc, 0 );
   </code>
   
   Create a timer that fires immediately, and 732 milliseconds
   after, passing some value 1234 as user data...
   <code lang="c++">
   timer_id = AddTimerEx( 0, 732, TimerProc, 1234 );
   
	</code>
	Remarks
	if a timer is dispatched and needs to wait - please link with idlelib, and call Idle.
	this will allow other timers to fire on schedule.  The timer that is waiting is not
	in the list of timers to process.
	*/
TIMER_PROC( uint32_t, AddTimerExx )( uint32_t start, uint32_t frequency
					, TimerCallbackProc callback
					, uintptr_t user DBG_PASS);
/* <combine sack::timers::AddTimerExx@uint32_t@uint32_t@TimerCallbackProc@uintptr_t user>
   
   \ \                                                                         */
#define AddTimerEx( s,f,c,u ) AddTimerExx( (s),(f),(c),(u) DBG_SRC )
/* <combine sack::timers::AddTimerExx@uint32_t@uint32_t@TimerCallbackProc@uintptr_t user>
   
   \ \                                                                         */
#define AddTimer( f, c, u ) AddTimerExx( (f), (f), (c), (u) DBG_SRC)

/* Stops a timer. The next time this timer would run, it will be
   removed. If it is currently dispatched, it is safe to remove
   from within the timer itself.
   Parameters
   timer :  32 bit timer ID from AddTimer.                       */
TIMER_PROC( void, RemoveTimer )( uint32_t timer );
/* Reschedule when a timer can fire. The delay can be 0 to make
   wake the timer.
   Parameters
   timer :  32 bit timer identifier from AddTimer.
   delay :  How long before the timer should run now.<p />If 0,
            will issue timer immediately.<p />If not specified,
            using the macro, the default delay is the timer's
            frequency. (can prevent the timer from firing until
            it's frequency from now.)                           */
TIMER_PROC( void, RescheduleTimerEx )( uint32_t timer, uint32_t delay );
/* <combine sack::timers::RescheduleTimerEx@uint32_t@uint32_t>
   
   \ \                                               */
TIMER_PROC( void, RescheduleTimer )( uint32_t timer );
/* Changes the frequency of a timer. Reschedule timer only
   changes the next time it fires, this can adjust the
   frequency. The simple ChangeTimer macro is sufficient.
   Parameters
   ID :         32 bit ID of the time created by AddTimer.
   initial :    initial delay of the timer. (Might matter if the
                timer hasn't fired the first time)
   frequency :  new delay between timer callback invokations.    */
TIMER_PROC( void, ChangeTimerEx )( uint32_t ID, uint32_t initial, uint32_t frequency );
/* <combine sack::timers::ChangeTimerEx@uint32_t@uint32_t@uint32_t>
   
   \ \                                               */
#define ChangeTimer( ID, Freq ) ChangeTimerEx( ID, Freq, Freq )

/* This is the type returned by MakeThread, and passed to
   ThreadTo. This is a private structure, and no definition is
   publicly available, this should be treated like a handle.   */
typedef struct threads_tag *PTHREAD;

/* Function signature for a thread entry point passed to
   ThreadTo.                                             */
typedef uintptr_t (CPROC*ThreadStartProc)( PTHREAD );
/* Function signature for a thread entry point passed to
   ThreadToSimple.                                             */
typedef uintptr_t (*ThreadSimpleStartProc)( POINTER );


/* Create a separate thread that starts in the routine
   specified. The uintptr_t value (something that might be a
   pointer), is passed in the PTHREAD structure. (See
   GetThreadParam)
   Parameters
   proc :       starting routine for the thread
   user_data :  some value that can be stored in the number of
                bits that a pointer is. This is passed to the
                proc when the thread starts.
   
   Example
   See WakeableSleepEx.                                        */
TIMER_PROC( PTHREAD, ThreadToEx )( ThreadStartProc proc, uintptr_t param DBG_PASS );
/* <combine sack::timers::ThreadToEx@ThreadStartProc@uintptr_t param>
   
   \ \                                                               */
#define ThreadTo(proc,param) ThreadToEx( proc,param DBG_SRC )

/* Create a separate thread that starts in the routine
   specified. The uintptr_t value (something that might be a
   pointer), is passed in the PTHREAD structure. (See
   GetThreadParam)
   Parameters
   proc :       starting routine for the thread
   user_data :  some value that can be stored in the number of
                bits that a pointer is. This is passed to the
                proc when the thread starts.
   
   Example
   See WakeableSleepEx.                                        */
TIMER_PROC( PTHREAD, ThreadToSimpleEx )( ThreadSimpleStartProc proc, POINTER param DBG_PASS );

/* <combine sack::timers::ThreadToEx@ThreadStartProc@uintptr_t param>
   
   \ \                                                               */
#define ThreadToSimple(proc,param) ThreadToSimpleEx( proc,param DBG_SRC )
/* \Returns a PTHREAD that represents the current thread. This
   can be used to create a PTHREAD identifier for the main
   thread.
   
   
   Parameters
   None.
   Returns
   a pointer to a thread structure that identifies the current
   thread. If this thread already has this structure created,
   the same one results on subsequent MakeThread calls.        */
TIMER_PROC( PTHREAD, MakeThread )( void );
/* Releases resources associated with a PTHREAD. For purposes of
   waking a thread, and providing a wakeable point for the
   thread, a system blocking event object is allocated, named
   with the THREAD_ID so it can be referenced by other
   processes. This is only allowed to be done by the thread
   itself.
   Parameters
   Param1 :  \Description
   Param2 :  \Description
   
   Example
   <code lang="c++">
   int main( void )
   {
       PTHREAD myself = MakeThread();
       // create threads, do stuff...
       UnmakeThread();
       At this point the pointer in 'myself' is invalid, and should be cleared.
       myself = NULL;
   }
   </code>                                                                      */
TIMER_PROC( void, UnmakeThread )( void );
/* This returns the parameter passed as user data to ThreadTo.
   
   
   Parameters
   thread :  thread to get the parameter from.
   
   Example
   See WakeableSleepEx.                                        */
TIMER_PROC( uintptr_t, GetThreadParam )( PTHREAD thread );
/* \returns the numeric THREAD_ID from a PTHREAD.
   Parameters
   thread :  thread to get the system wide unique ID of. */
TIMER_PROC( THREAD_ID, GetThreadID )( PTHREAD thread );

/* \returns the numeric THREAD_ID from a PTHREAD.
   Parameters
   thread :  thread to get the system wide unique ID of. */
TIMER_PROC( THREAD_ID, GetThisThreadID )( void );

/* Symbol defined to pass to Wakeable_Sleep to sleep until
   someone calls WakeThread.                               */
#define SLEEP_FOREVER 0xFFFFFFFF
/* Sleeps a number of milliseconds or until the thread is passed
   to WakeThread.
   Parameters
   dwMilliseconds :  How long to sleep. Can be indefinite if
                     value is SLEEP_FOREVER.
   Example
   <code lang="c++">
   PTHREAD main_thread;
   
   uintptr_t CPROC WakeMeThread( PTHREAD thread )
   {
      // get the value passed to ThreadTo as user_data.
      uintptr_t user_data = GetThreadParam( thread );
      // let the main thread sleep a little wile
       WakeableSleep( 250 );
      // then wake it up
       WakeThread( main_thread );
       return 0;
   }
   
   int main( void )
   {
       // save my PTHREAD globally.
       main_thread = MakeThread();
       // create a thread that can wake us
       ThreadTo( WakeMeThread, 0 );
       // demonstrate sleeping
       WakableSleep( SLEEP_FOREVER );
       return 0;
   }
   </code>                                                       */
TIMER_PROC( void, WakeableSleepEx )( uint32_t milliseconds DBG_PASS );
TIMER_PROC( void, WakeableSleep )( uint32_t milliseconds );
TIMER_PROC( void, WakeableNamedSleepEx )( CTEXTSTR name, uint32_t n DBG_PASS );
#define WakeableNamedSleep( name, n )   WakeableNamedSleepEx( name, n DBG_SRC )
TIMER_PROC( void, WakeNamedSleeperEx )( CTEXTSTR name DBG_PASS );
#define WakeNamedSleeper( name )   WakeNamedSleeperEx( name DBG_SRC )

TIMER_PROC( void, WakeableNamedThreadSleepEx )( CTEXTSTR name, uint32_t n DBG_PASS );
#define WakeableNamedThreadSleep( name, n )   WakeableNamedThreadSleepEx( name, n DBG_SRC )
TIMER_PROC( void, WakeNamedThreadSleeperEx )( CTEXTSTR name, THREAD_ID therad DBG_PASS );
#define WakeNamedThreadSleeper( name, thread )   WakeNamedThreadSleeperEx( name, thread DBG_SRC )

#ifdef USE_PIPE_SEMS
TIMER_PROC( int, GetThreadSleeper )( PTHREAD thread );
#endif
/* <combine sack::timers::WakeableSleepEx@uint32_t milliseconds>
   
   \ \                                                      */
#define WakeableSleep(n) WakeableSleepEx(n DBG_SRC )

/* Wake a thread by ID, if the pThread is not available. Can be
   used cross-process for instance. Although someone could add a
   method to provide a PTHREAD wrapper around THREAD_ID for
   threads in remote processes, this may not be a best practice.
   Parameters
   thread_id :  THREAD_ID from GetMyThreadID, which is a macro
                appropriate for a platform.                      */
TIMER_PROC( void, WakeThreadIDEx )( THREAD_ID thread DBG_PASS );
/* Wake a thread.
   Example
   See WakeableSleepEx.
   Parameters
   pThread :  thread to wake up from a WakeableSleep. */
TIMER_PROC( void, WakeThreadEx )( PTHREAD thread DBG_PASS );

/* <combine sack::timers::WakeThreadIDEx@THREAD_ID thread>
   
   \ \                                                     */
#define WakeThreadID(thread) WakeThreadIDEx( thread DBG_SRC )
/* <combine sack::timers::WakeThreadEx@PTHREAD thread>
   
   \ \                                                 */
#define WakeThread(t) WakeThreadEx(t DBG_SRC )

/* This can be checked to see if the THREAD_ID to wake still has
   an event. Sometimes threads end.
   Parameters
   thread :  thread identifier to check to see if it exists/can be
             woken.
   Returns
   TRUE if the thread can be signaled to wake up.
   
   FALSE if the thread cannot be found or cannot be woken up.      */
TIMER_PROC( int, TestWakeThreadID )( THREAD_ID thread );
/* This can be checked to see if the PTHREAD to wake still has
   an event. Sometimes threads call UnmakeThread(). This is a
   more practical test using a THREAD_ID instead. See
   TestWakeThreadID.
   Returns
   TRUE if the thread can be signaled to wake up.
   
   FALSE if the thread cannot be found or cannot be woken up.  */
TIMER_PROC( int, TestWakeThread )( PTHREAD thread );

//TIMER_PROC( void, WakeThread )( PTHREAD thread );

TIMER_PROC( void, EndThread )( PTHREAD thread );
/* This tests to see if a pointer to a thread references the
   current thread.
   Parameters
   thread :  thread to check to see if it is the current thread.
   Returns
   TRUE if this thread is the same as the PTHREAD passed.
   otherwise FALSE.
   Example
   <code lang="c++">
   PTHREAD main_thread;
   LOGICAL thread_finished_check;
   
   uintptr_t CPROC ThreadProc( PTHREAD thread )
   {
       if( IsThisThread( main_thread ) )
            printf( "This thread is not the main thread.\\n" );
       else
            printf( "This is the main thread - cannot happen :)\\n" );
   </code>
   <code>
   
       // mark that this thread is complete
       thread_finished_check = TRUE;
   
   </code>
   <code lang="c++">
       // hmm - for some reason, just pass the uintptr_t that was passed to ThreadTo as the result.
       return GetThreadParam( thread );
   }
   
   int main( void )
   {
        main_thread = MakeThread();
        ThreadTo( ThreadProc, 0 );
   
        // wait for the thread to finish its thread identity check.
        while( !thread_finished_check )
            Relinquish();
   
        return 0;
   }
   </code>                                                                                         */
TIMER_PROC( int, IsThisThreadEx )( PTHREAD pThreadTest DBG_PASS );
/* <combine sack::timers::IsThisThreadEx@PTHREAD pThreadTest>
   
   \ \                                                        */
#define IsThisThread(thread) IsThisThreadEx(thread DBG_SRC)

/* Enter a critical section. Only a single thread may be in a
   critical section, if a second thread attempts to enter the
   section while another thread is in it will block until the
   original thread leaves the section. The same thread may enter
   a critical section multiple times. For each time a critical
   section is entered, the thread must also leave the critical
   section (See LeaveCriticalSection).
   
   
   Parameters
   pcs :  pointer to a critical section to enter                 */
TIMER_PROC( LOGICAL, EnterCriticalSecEx )( PCRITICALSECTION pcs DBG_PASS );
/* Leaves a critical section. See EnterCriticalSecEx.
   
   
   Parameters
   pcs :  pointer to a critical section.              */
TIMER_PROC( LOGICAL, LeaveCriticalSecEx )( PCRITICALSECTION pcs DBG_PASS );
/* Does nothing. There are no extra resources required for
   critical sections, and the memory is allocated by the
   application.
   Parameters
   pcs :  pointer to critical section to do nothing with.  */
TIMER_PROC( void, DeleteCriticalSec )( PCRITICALSECTION pcs );

#ifdef _WIN32
	TIMER_PROC( HANDLE, GetWakeEvent )( void );
	TIMER_PROC( HANDLE, GetThreadHandle )( PTHREAD thread );
#endif

#ifdef __LINUX__
	TIMER_PROC( pthread_t, GetThreadHandle )(PTHREAD thread);
#endif

#ifdef USE_NATIVE_CRITICAL_SECTION
#define EnterCriticalSec(pcs) EnterCriticalSection( pcs )
#define LeaveCriticalSec(pcs) LeaveCriticalSection( pcs )
#else
/* <combine sack::timers::EnterCriticalSecEx@PCRITICALSECTION pcs>
   
   \ \                                                             */
#define EnterCriticalSec( pcs ) EnterCriticalSecEx( (pcs) DBG_SRC )
/* <combine sack::timers::LeaveCriticalSecEx@PCRITICALSECTION pcs>
   
   \ \                                                             */
#define LeaveCriticalSec( pcs ) LeaveCriticalSecEx( (pcs) DBG_SRC )

#endif

TIMER_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::timers;
#endif

#endif
// $Log: timers.h,v $
// Revision 1.37  2005/05/16 19:06:58  jim
// Extend wakeable sleep to know the originator of the sleep.
//
// Revision 1.36  2004/09/29 16:42:51  d3x0r
// fixed queues a bit - added a test wait function for timers/threads
//
// Revision 1.35  2004/07/07 15:33:54  d3x0r
// Cleaned c++ warnings, bad headers, fixed make system, fixed reallocate...
//
// Revision 1.34  2004/05/02 02:04:16  d3x0r
// Begin border exclusive option, define PushMethod explicitly, fix LaunchProgram in timers.h
//
// Revision 1.33  2003/12/10 15:38:25  panther
// Move Sleep and GetTickCount to real code
//
// Revision 1.32  2003/11/02 00:31:47  panther
// Added debuginfo pass to wakethread
//
// Revision 1.31  2003/10/24 14:59:21  panther
// Added Load/Unload Function for system shared library abstraction
//
// Revision 1.30  2003/10/17 00:56:04  panther
// Rework critical sections.
// Moved most of heart of these sections to timers.
// When waiting, sleep forever is used, waking only when
// released... This is preferred rather than continuous
// polling of section with a Relinquish.
// Things to test, event wakeup order uner linxu and windows.
// Even see if the wake ever happens.
// Wake should be able to occur across processes.
// Also begin implmeenting MessageQueue in containers....
// These work out of a common shared memory so multiple
// processes have access to this region.
//
// Revision 1.29  2003/09/21 04:03:30  panther
// Build thread ID with pthread_self and getgid
//
// Revision 1.28  2003/07/29 10:41:25  panther
// Predefine struct threads_tag to avoid warning
//
// Revision 1.27  2003/07/24 22:49:20  panther
// Define callback procs as CDECL
//
// Revision 1.26  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.25  2003/07/22 15:33:19  panther
// Added comment about idle()
//
// Revision 1.24  2003/04/03 10:10:20  panther
// Add file/line debugging to addtimer
//
// Revision 1.23  2003/03/27 13:47:14  panther
// Immplement a EndThread
//
// Revision 1.22  2003/03/25 08:38:11  panther
// Add logging
//
