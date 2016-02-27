#include <stdhdrs.h>
#include <sack_types.h>
#include <sharemem.h>
#include <timers.h>
#include <idle.h>

#include <procreg.h>
#include <deadstart.h>

#ifdef __cplusplus
namespace sack {
	namespace timers {
		using namespace sack::memory;
#endif
      typedef struct idle_proc_tag IDLEPROC;
      typedef struct idle_proc_tag *PIDLEPROC;
struct idle_proc_tag
{
	struct {
		BIT_FIELD bDispatched : 1;
		BIT_FIELD bRemove : 1;
	} flags;
	// return -1 if not the correct thread
	// to handle this callback
	// return 0 if no messages were processed
	// return 1 if messages were processed
	int (CPROC*function)(PTRSZVAL);
	PTRSZVAL data;
	THREAD_ID thread;
	//PDATAQUEUE threads;
	PIDLEPROC similar; // same function references go here - for multiple thread entries...
	DeclareLink( struct idle_proc_tag );
};
struct idle_global_tag {
	CRITICALSECTION idle_cs;
	LOGICAL cs_inited;
	PIDLEPROC registered_idle_procs;
};
#ifndef __STATIC_GLOBALS__
static struct idle_global_tag *idle_global;// registered_idle_procs;

PRIORITY_PRELOAD( InitGlobalIdle, OSALOT_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( idle_global );
	if( !(*idle_global).cs_inited )
	{
		InitializeCriticalSec( &(*idle_global).idle_cs );
		(*idle_global).cs_inited = 1;
	}
}
#else
static struct idle_global_tag _idle_global;// registered_idle_procs;
static struct idle_global_tag *idle_global = &_idle_global;// registered_idle_procs;
#endif

#ifdef procs
#  undef procs
#endif
#define procs ((*idle_global).registered_idle_procs)

IDLE_PROC( void, AddIdleProc )( int (CPROC*Proc)( PTRSZVAL psv ), PTRSZVAL psvUser )
{
	PIDLEPROC proc = NULL;
#ifndef __STATIC_GLOBALS__
	if( !idle_global )
		SimpleRegisterAndCreateGlobal( idle_global );
#endif
	if( !(*idle_global).cs_inited )
	{
		InitializeCriticalSec( &(*idle_global).idle_cs );
		(*idle_global).cs_inited = TRUE;
	}
	EnterCriticalSec( &(*idle_global).idle_cs );

	for( proc = procs; proc; proc = proc->next )
	{
		if( Proc == proc->function )
		{
			PIDLEPROC newproc = (PIDLEPROC)Allocate( sizeof( IDLEPROC ) );
			MemSet( newproc, 0, sizeof( IDLEPROC ) );
			newproc->function = Proc;
			newproc->data = psvUser;
			LinkLast( proc->similar, PIDLEPROC, newproc );
			break;
		}
	}
	// if the function is not already registered as an idle proc, register it.
	if( !proc )
	{
		proc = (PIDLEPROC)Allocate( sizeof( IDLEPROC ) );
		MemSet( proc, 0, sizeof( IDLEPROC ) );
		proc->function = Proc;
		proc->data = psvUser;
		LinkThing( procs, proc );
	}
	LeaveCriticalSec( &(*idle_global).idle_cs );
}

IDLE_PROC( int, RemoveIdleProc )( int (CPROC*Proc)(PTRSZVAL psv ) )
{
	PIDLEPROC check_proc;
#ifndef __STATIC_GLOBALS__
	if( !idle_global )
		SimpleRegisterAndCreateGlobal( idle_global );
#endif
	if( !(*idle_global).cs_inited )
	{
		InitializeCriticalSec( &(*idle_global).idle_cs );
		(*idle_global).cs_inited = TRUE;
	}
	EnterCriticalSec( &(*idle_global).idle_cs );
	for( check_proc = procs; check_proc; check_proc = check_proc->next )
	{
		if( Proc == check_proc->function )
		{
			if( !check_proc->flags.bDispatched )
			{
				UnlinkThing( check_proc );
				if( check_proc->similar )
					LinkThing( check_proc->similar, procs );
				Release( check_proc );
			}
			else
			{
				check_proc->flags.bRemove = 1;
			}
			break;
		}
	}
	LeaveCriticalSec( &(*idle_global).idle_cs );
	return 0;
}

IDLE_PROC( int, IdleEx )( DBG_VOIDPASS )
{
	THREAD_ID me = GetMyThreadID();
	int success = 0;
	PIDLEPROC proc;
#ifndef __STATIC_GLOBALS__
	if( idle_global )
#endif
	for( proc = procs; proc;  )
	{
		PIDLEPROC check;
		for( check = proc; check; check = check->similar )
		{
			check->flags.bDispatched = 1;
			//lprintf( "attempt proc %p in %Lx  procthread=%Lx", check, GetThreadID( MakeThread() ), check->thread );
			//if( !check->thread || ( check->thread == me ) )
			// sometimes... a function belongs to multiple threads...
			if( check->function( check->data ) != -1 )
			{
				check->thread = me;
				success = 1;
			}
			check->flags.bDispatched = 0;
			if( check->flags.bRemove )
			{
				UnlinkThing( check );
				if( check->similar && check == proc )
					LinkThing( check->similar, procs );
				Release( proc );
				proc = procs;
				break;
			}
			else
			{
				//if( check->thread == me )
				{
					proc = proc->next;
					break;
				}
			}
		}
		if( check == NULL )
			proc = proc->next;
	}
	//_lprintf( DBG_AVAILABLE, WIDE("Is Going idle.") DBG_RELAY );
	Relinquish();
	//_lprintf( DBG_AVAILABLE, WIDE("Is back from idle.") DBG_RELAY );
	return success;
}

#undef Idle
IDLE_PROC( int, Idle )( void )
{
   return IdleEx( DBG_VOIDSRC );
}

IDLE_PROC( int, IdleForEx )( _32 dwMilliseconds DBG_PASS )
{
	_32 dwStart = timeGetTime();
	while( ( dwStart + dwMilliseconds ) > timeGetTime() )
	{
		if( !IdleEx( DBG_VOIDRELAY ) )
		{
			// sleeping... cause ew didn't do any idle procs...
			WakeableSleep( dwMilliseconds );
		}
	}
	return 0;
}

#undef IdleFor
IDLE_PROC( int, IdleFor )( _32 dwMilliseconds )
{
   return IdleForEx( dwMilliseconds DBG_SRC );
}


#ifdef __cplusplus
}; //namespace sack {
}; //	namespace idle {
#endif

