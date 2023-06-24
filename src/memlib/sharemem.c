/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   created to provide standard memory allocation features.
 *   Release( Allocate(size) )
 *   Hold( pointer ); // must release a second time.
 *   if DEBUG, memory bounds checking enabled and enableable.
 *   if RELEASE standard memory includes no excessive operations
 *
 *  standardized to never use int. (was a clean port, however,
 *  inaccurate, knowing the conversions on calculations of pointers
 *  are handled by cast to int! )
 *
 * see also - include/sharemem.h
 *
 */

//   DEBUG_SYMBOLS
// had some problems with OpenSpace opening a shared region under win98
// Apparently if a create happens with a size of 0, the name of the region
// becomes unusable, until a reboot happens.
//#define DEBUG_OPEN_SPACE


// this variable controls whether allocate/release is logged.
#ifndef NO_FILEOP_ALIAS
#  define NO_FILEOP_ALIAS
#endif
#define NO_UNICODE_C
//#define USE_SIMPLE_LOCK_ON_OPEN

#include <stddef.h>
#include <stdio.h>

#ifdef __LINUX__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#ifdef _MSC_VER
#ifndef UNDER_CE
#include <intrin.h>
#endif
#endif

#define DEFINE_MEMORY_STRUCT
#include <stdhdrs.h>
#include <filedotnet.h>
#include <sack_types.h>
#include <logging.h>
#include <signed_unsigned_comparisons.h>
#include <deadstart.h>
#include <sharemem.h>
#include <procreg.h>
#ifndef _SHARED_MEMORY_LIBRARY
#  include "sharestruc.h"
#endif
#include <sqlgetoption.h>
#include <ctype.h>

#if defined __ANDROID__
#include <linux/ashmem.h>
#endif

#ifdef _MSC_VER
//>= 900
#include <crtdbg.h>
#include <new.h>
#endif

#ifdef __cplusplus
namespace sack {
	namespace memory {

#endif

#ifdef __64__
#define CLEAR_MEMORY_TAG 0xDEADBEEFDEADBEEFULL
#define FREE_MEMORY_TAG 0xFACEBEADFACEBEADULL
#define LEAD_PROTECT_TAG 0xbabecafebabecafeULL
#define LEAD_PROTECT_BLOCK_TAIL 0xbeefcafebeefcafeULL
#else
#define CLEAR_MEMORY_TAG 0xDEADBEEFUL
#define FREE_MEMORY_TAG 0xFACEBEADUL
#define LEAD_PROTECT_TAG 0xbabecafeUL
#define LEAD_PROTECT_BLOCK_TAIL 0xbeefcafeUL
#endif

#ifdef g
#  undef g
#endif

#ifdef __64__
#  define makeULong(n) (~(n##ULL))
#else
#  define makeULong(n) (~(n##UL))
#endif
static uintptr_t masks[33] = { makeULong(0), makeULong(0), makeULong(1), 0
                  , makeULong(3), 0, 0, 0  // 4
	              , makeULong(7), 0, 0, 0, 0, 0, 0, 0   // 8
	              , makeULong(15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // 16
	              , makeULong(31) };  // 32


#define BASE_MEMORY (POINTER)0x80000000
// golly allocating a WHOLE DOS computer to ourselves? how RUDE
#define SYSTEM_CAPACITY  g.dwSystemCapacity


#define MALLOC_CHUNK_SIZE(pData) ( (pData)?( ( ( (uint16_t*)(pData))[-1] ) + offsetof( MALLOC_CHUNK, byData ) ):0 )
//#define CHUNK_SIZE(pData) ( ( (pData)?( (uint16_t*)(pData))[-1]:0 ) +offsetof( CHUNK, byData ) ) )
#define CHUNK_SIZE ( offsetof( CHUNK, byData ) )
#define MEM_SIZE  ( offsetof( MEM, pRoot ) )

// using lower level syslog bypasses some allocation requirements...

//#undef lprintf
//#undef _lprintf

#ifndef NO_LOGGING
#  ifdef _DEBUG
#    define ll_lprintf( f, ... ) { TEXTCHAR buf[256]; tnprintf( buf, 256, f,##__VA_ARGS__ ); SystemLogFL( buf FILELINE_SRC ); }
#    define _lprintf2( f, ... ) { TEXTCHAR buf[256]; tnprintf( buf, 256, FILELINE_FILELINEFMT f,_pFile,_nLine,##__VA_ARGS__ ); SystemLogFL( buf FILELINE_SRC ); } }
#    define ll__lprintf( a ) {const TEXTCHAR *_pFile = pFile; int _nLine = nLine; _lprintf2
#  else
#    define ll_lprintf( f, ... ) { TEXTCHAR buf[256]; tnprintf( buf, 256, f,##__VA_ARGS__ ); SystemLog( buf ); }
#    define _lprintf2( f, ... ) { TEXTCHAR buf[256]; tnprintf( buf, 256, f,##__VA_ARGS__ ); SystemLog( buf ); } }
#    define ll__lprintf( a ) { _lprintf2
#  endif
#else
#  define lprintf( f,... )
#endif

// last entry in space tracking array will ALWAYS be
// another space tracking array (if used)
// (32 bytes)
typedef struct space_tracking_structure {
	PMEM pMem;
#ifdef _WIN32
	HANDLE  hFile;
	HANDLE  hMem;
#else
	struct {
		uint32_t bTemporary : 1;
	} flags;
	int hFile;
#endif
	uintptr_t dwSmallSize;
	DeclareLink( struct space_tracking_structure );
} SPACE, *PSPACE;

typedef struct space_pool_structure {
	DeclareLink( struct space_pool_structure );
	SPACE spaces[(4096 - sizeof( struct space_pool_structure * )
		- sizeof( struct space_pool_structure ** ))
		/ sizeof( SPACE )];
} SPACEPOOL, *PSPACEPOOL;

#define MAX_PER_BLOCK (4096 - sizeof( struct space_pool_structure *) \
	- sizeof( struct space_pool_structure **) )  \
	/ sizeof( SPACE )


#ifdef _WIN32
//(0x10000 * 0x1000) //256 megs?
#define FILE_GRAN g.si.dwAllocationGranularity
#else
#define FILE_GRAN g.pagesize
#endif

struct global_memory_tag {
	size_t dwSystemCapacity; // basic OS block grabbed for allocation
//#ifdef _DEBUG
// may define one or the other of these but NOT both
	int bDisableDebug;
	int bDisableAutoCheck;
	int bLogCritical;
	//#endif
	size_t nMinAllocateSize;
	int pagesize;
	int bLogAllocate;
	int bLogAllocateWithHold;
	LOGICAL bCustomAllocer;  // this option couldn't work; different block tracking methods are incompatible
	LOGICAL bInit;
	LOGICAL allowLogging;
	PSPACEPOOL pSpacePool;
#ifdef _WIN32
	SYSTEM_INFO si;
#endif
	int InAdding; // don't add our tracking to ourselves...
	uint32_t bMemInstanced; // set if anybody starts to DIG.
	LOGICAL deadstart_finished;
	PMEM pMemInstance;
};

#ifdef __STATIC__
static struct global_memory_tag global_memory_data = { 0x10000 * 0x08, 1, 1
													, 0 /* log criticalsec */
													, 0 /* min alloc */
													, 0 /* pagesize */
													, 0/*logging*/
													, 0 /* log holds also */
																	  , USE_CUSTOM_ALLOCER
																	  , 0
																	  , 0
																	  , NULL
#ifdef _WIN32
																	  , { 0 }
#endif
																	  , 0
																	  , 0
																	  , FALSE
                                                     , NULL
};
#  define g (global_memory_data)

#else
#  ifdef _DEBUG
struct global_memory_tag global_memory_data = { 0x10000 * 0x08, 0, 0/*auto check(disable)*/
#    ifdef DEBUG_CRITICAL_SECTIONS
															, 1/* log critical sections*/
#    else
															, 0/* log critical sections*/
#    endif
															, 0 /* min alloc size */
															, 0 /* pagesize */
															, 0 /*log allocates*/
															, 0 /* logging too */
															 , USE_CUSTOM_ALLOCER /* custom allocer*/
																	  , 0
																	  , 0
																	  , NULL
#ifdef _WIN32
																	  , { 0 }
#endif
																	  , 0
																	  , 0
																	  , FALSE
                                                     , NULL
};
// this one has memory logging enabled by default...
//struct global_memory_tag global_memory_data = { 0x10000 * 0x08, 0, 1, 0, 0, 0, 1 };
#  else
struct global_memory_tag global_memory_data = { 0x10000 * 0x08, 1/* disable debug*/, 1/*auto check(disable)*/
															, 0 /* log crit */, 0 /* min alloc size */, 0 /* page size */
															, 0/*log allocates*/
															, 0  /* log holds */
															, USE_CUSTOM_ALLOCER  // custom allocer
																	  , 0
																	  , 0
																	  , NULL
#ifdef _WIN32
																	  , { 0 }
#endif
																	  , 0
																	  , 0
																	  , FALSE
                                                     , NULL
};
// this one has memory logging enabled by default...
//struct global_memory_tag global_memory_data = { 0x10000 * 0x08, 0, 1, 0, 0, 0, 1 };
#  endif
#define g global_memory_data
#endif

#ifndef NO_LOGGING
#  define ODSEx(s,pFile,nLine) SystemLogFL( s DBG_RELAY )
//#define ODSEx(s,pFile,nLine) SystemLog( s )
#  define ODS(s)  SystemLog(s)
#else
#  define ODSEx(s,file,line)
#  define ODS(s)
#endif

#define MAGIC_SIZE sizeof( void* )

#ifdef __64__
#define BLOCK_TAG(pc)  (*(uint64_t*)((pc)->byData + (pc)->dwSize - (pc)->info.dwPad ))
// so when we look at memory this stamp is 0123456789ABCDEF
#define TAG_FORMAT_MODIFIER "ll"
#define BLOCK_TAG_ID 0xefcdab8967452301LL
#else
#define BLOCK_TAG(pc)  (*(uint32_t*)((pc)->byData + (pc)->dwSize - (pc)->info.dwPad ))
// so when we look at memory this stamp is 12345678
#define TAG_FORMAT_MODIFIER "l"
#define BLOCK_TAG_ID 0x78563412L
#endif
// file/line info are at the very end of the physical block...
// block_tag is at the start of the padding...
#define BLOCK_FILE(pc) (*(CTEXTSTR*)((pc)->byData + (pc)->dwSize - MAGIC_SIZE*2))
#define BLOCK_LINE(pc) (*(int*)((pc)->byData + (pc)->dwSize - MAGIC_SIZE))

#ifndef _WIN32
#include <errno.h>
#endif

PRIORITY_PRELOAD( Deadstart_finished_enough, GLOBAL_INIT_PRELOAD_PRIORITY + 1 )
{
	g.deadstart_finished = 1;
#ifdef _WIN32
	GetSystemInfo( &g.si );
#else
	g.pagesize = sysconf(_SC_PAGESIZE);
#endif

#if !(USE_CUSTOM_ALLOCER)
        // enable Release().
	g.bInit = 1;
#endif
	//g.bLogAllocate = 1;
}

PRIORITY_PRELOAD( InitGlobal, DEFAULT_PRELOAD_PRIORITY )
{
#ifndef __NO_OPTIONS__
	g.bLogCritical = SACK_GetProfileIntEx( GetProgramName(), "SACK/Memory Library/Log critical sections", g.bLogCritical, TRUE );
	g.bLogAllocate = SACK_GetProfileIntEx( GetProgramName(), "SACK/Memory Library/Enable Logging", g.bLogAllocate, TRUE );
	if( g.bLogAllocate )
		ll_lprintf( "Memory allocate logging enabled." );
	g.bLogAllocateWithHold = SACK_GetProfileIntEx( GetProgramName(), "SACK/Memory Library/Enable Logging Holds", g.bLogAllocateWithHold, TRUE );
	//USE_CUSTOM_ALLOCER = SACK_GetProfileIntEx( GetProgramName(), "SACK/Memory Library/Custom Allocator", USE_CUSTOM_ALLOCER, TRUE );
	g.bDisableDebug = g.bDisableDebug || SACK_GetProfileIntEx( GetProgramName(), "SACK/Memory Library/Disable Debug", !USE_DEBUG_LOGGING, TRUE );
#else
	//g.bLogAllocate = 1;
#endif
	g.nMinAllocateSize = 32;
	g.allowLogging = 1;
}

#if __GNUC__
//#  pragma message( "GNUC COMPILER")
#  ifndef __ATOMIC_RELAXED
#    define __ATOMIC_RELAXED 0
#  endif
#  ifndef __GNUC_VERSION
#    define __GNUC_VERSION ( __GNUC__ * 10000 ) + ( __GNUC_MINOR__ * 100 )
#  endif
#  if  ( __GNUC_VERSION >= 40800 ) || defined(__MAC__) || defined( __EMSCRIPTEN__ )
#    define XCHG(p,val)  __atomic_exchange_n(p,val,__ATOMIC_RELAXED)
///  for some reason __GNUC_VERSION doesn't exist from android ?
#  elif defined __ARM__ || defined __ANDROID__
#    define XCHG(p,val)  __atomic_exchange_n(p,val,__ATOMIC_RELAXED)
#  else
#    pragma message( "gcc is a version without __atomic_exchange_n" )
inline uint32_t DoXchg( volatile uint32_t* p, uint32_t val ) { __asm__( "lock xchg (%2),%0" :"=a"(val) : "0"(val), "c"(p) ); return val; }
inline uint64_t DoXchg64( volatile int64_t* p, uint64_t val ) { __asm__( "lock xchg (%2),%0" :"=a"(val) : "0"(val), "c"(p) ); return val; }
#    define XCHG( p,val) ( ( sizeof( val ) > sizeof( uint32_t ) )?DoXchg64( (volatile int64_t*)p, (uint64_t)val ):DoXchg( (volatile uint32_t*)p, (uint32_t)val ) )
#  endif
//#  endif
#else
#  define XCHG(p,val)  LockedExchange( p, val )
#endif
//-------------------------------------------------------------------------
#if !defined( HAS_ASSEMBLY ) || defined( __CYGWIN__ )
uint32_t  LockedExchange( volatile uint32_t* p, uint32_t val )
{
	// Windows only available - for linux platforms please consult
	// the assembly version should be consulted
#if ( defined( _WIN32 ) || defined( WIN32 ) ) && !defined( __ANDROID__ )
#  if !defined(_MSC_VER)
	return InterlockedExchange( (volatile LONG *)p, val );
#  else
	//return _InterlockedExchange_HLEAcquire( (volatile long*)p, val );
	return _InterlockedExchange( (volatile long*)p, val );

	// windows wants this as a LONG not ULONG
	//return InterlockedExchange( (volatile LONG *)p, val );
#  endif
#else
#  if ( defined( __LINUX__ ) ) //&& !( defined __ARM__ || defined __ANDROID__ )
	return XCHG( p, val );
	//   return __atomic_exchange_n(p,val,__ATOMIC_RELAXED);
#  else /* some other system, not windows, not linux... */
	{
		#warning compiling C fallback locked exchange.This is NOT atomic, and needs to be
		// swp is the instruction....
		uint32_t prior = *p;
		*p = val;
		return prior;
	}
#  endif
#endif
}

uint32_t LockedIncrement( volatile uint32_t* p ) {
#ifdef _WIN32
	return InterlockedIncrement( (volatile LONG *)p );
#endif
#ifdef __LINUX__
	return __atomic_add_fetch( p, 1, __ATOMIC_RELAXED );
#endif
}
uint32_t LockedDecrement( volatile uint32_t* p ) {
#ifdef _WIN32
	return InterlockedDecrement( (volatile LONG *)p );
#endif
#ifdef __LINUX__
	return __atomic_sub_fetch( p, 1, __ATOMIC_RELAXED );
#endif
}

uint64_t  LockedExchange64( volatile uint64_t* p, uint64_t val )
{
	// Windows only available - for linux platforms please consult
	// the assembly version should be consulted
#if defined WIN32 && !defined __ANDROID__
#ifdef _MSC_VER
#ifdef __64__
	uint64_t prior = (uint64_t)InterlockedExchange64( (volatile __int64 *)p, (int64_t)val );
#else
	// because the value is a LONG (signed) it has to be made unsigned of the same lenght (ULONG) then extended (uint64_t).
	// otherwise the sign extension was a bug.
	uint64_t prior = (uint64_t)(ULONG)InterlockedExchange( (DWORD*)p, (DWORD)val ) | ((uint64_t)InterlockedExchange( ((DWORD*)p) + 1, (DWORD)(val >> 32) ) << 32);
#endif
#else
	uint64_t prior = InterlockedExchange( (volatile LONG*)p, (int32_t)val ) | InterlockedExchange( ((volatile LONG*)p) + 1, (uint32_t)(val >> 32) );
#endif
	return prior;
#else
#  if defined __GNUC__
#     if !defined( __ANDROID__ ) || ( ANDROID_NDK_TARGET_PLATFORM > 16 )
	return XCHG( p, val );//__atomic_exchange_n(p,val,__ATOMIC_RELAXED);
#else
	{
		// swp is the instruction....
		// going to have to set IRQ, PIRQ on arm...
		uint64_t prior = *p;
		*p = val;
		return prior;
	}
#endif
#  else
	{
		// swp is the instruction....
		// going to have to set IRQ, PIRQ on arm...
		uint64_t prior = *p;
		*p = val;
		return prior;
	}
#  endif
#endif
}

#endif

//-------------------------------------------------------------------------

#ifdef DEBUG_CRITICAL_SECTIONS
#if 0
static void DumpSection( PCRITICALSECTION pcs )
{
	ll_lprintf( "Critical Section....." );
	ll_lprintf( "------------------------------" );
	ll_lprintf( "Update: %08x", pcs->dwUpdating );
	ll_lprintf( "Current Process: %16" _64fx, pcs->dwThreadID );
	ll_lprintf( "Next Process:    %16" _64fx, pcs->dwThreadWaiting );
	ll_lprintf( "Last update: %s(%d)", pcs->pFile ? pcs->pFile : "unknown", pcs->nLine );
}
#endif
#endif

#ifdef __cplusplus
} // namespace memory {
	namespace timers { // begin timer namespace

#endif
#ifndef USE_NATIVE_CRITICAL_SECTION
		uint32_t  CriticalSecOwners( PCRITICALSECTION pcs )
		{
			return pcs->dwLocks;
		}
#endif

#ifndef USE_NATIVE_CRITICAL_SECTION
//#  ifdef _MSC_VER
//#    pragma optimize( "st", off )
//#  endif
		int32_t  EnterCriticalSecNoWaitEx( PCRITICALSECTION pcs, THREAD_ID *prior DBG_PASS )
		{
			THREAD_ID dwCurProc;
			// need to aquire lock on section...
			// otherwise our old mechanism allowed an enter in another thread
			// to falsely identify the section as its own while the real owner
			// tried to exit...

			if( XCHG( &pcs->dwUpdating, 1 ) ) return -1;
			dwCurProc = GetThisThreadID();

			if( !pcs->dwLocks ) {
				// section is unowned...
				if( pcs->dwThreadWaiting ) {
					// someone was waiting for it...
					if( pcs->dwThreadWaiting != dwCurProc ) {
						if( prior && (*prior ) ) {
							// prior is set, so someone has set their prior to me....
							pcs->dwUpdating = 0;
							return 0;
						}
					}
					else { //  waiting is me
						if( prior && (*prior) ) {
							if( (*prior) == 1 ) pcs->dwThreadWaiting = 0;
							else pcs->dwThreadWaiting = (*prior);
							(*prior) = 0;
						}
						else
							pcs->dwThreadWaiting = 0;
					}
				}
				else {
					if( prior && *prior ) {
						// shouldn't happen, if there's no waiter set, then there shouldn't be a prior.
						if( *prior != 1 )
							DebugBreak();
					}
					//ll_lprintf( "Claimed critical section." );
				}
				pcs->dwThreadID = dwCurProc; // claim the section and return success
				pcs->dwLocks = 1;
				pcs->dwUpdating = 0;
				return 1;
			}
			else if( dwCurProc == pcs->dwThreadID )
			{
				// otherwise 1) I won the thread already... (threadID == me )
				pcs->dwLocks++;
				pcs->dwUpdating = 0;
				return 1;
			}
			// if the prior is wanted to be saved...
			if( prior )
			{
				if( *prior )
				{
					if( pcs->dwThreadWaiting != dwCurProc )
					{
						if( !pcs->dwThreadWaiting ) {
							ll_lprintf( "@@@ Someone stole the critical section that we were wiating on before we reentered. fail. %" _64fx " %" _64fx " %" _64fx, pcs->dwThreadWaiting, dwCurProc, *prior );
							DebugBreak();
							// go back to sleep again.
							pcs->dwThreadWaiting = dwCurProc;
						}
						else {
							if( (*prior) == pcs->dwThreadWaiting ) {
								ll_lprintf( "prior is thread wiaiting (normal?!) %" _64fx " %" _64fx, pcs->dwThreadWaiting, *prior );
								DebugBreak();
								(*prior) = 0;
							}
						}
						// assume that someone else kept our waiting ID...
						// cause we're not the one waiting, and we have someone elses ID..
						// we are awake out of order..
						pcs->dwUpdating = 0;
						return 0;
					}
				}
				else if( pcs->dwThreadWaiting != dwCurProc )
				{
					if( pcs->dwThreadWaiting ) *prior = pcs->dwThreadWaiting;
					else *prior = 1;
					pcs->dwThreadWaiting = dwCurProc;
				}
			}
			// else no prior... so don't set the dwthreadwaiting...
			pcs->dwUpdating = 0;
			return 0;
		}
#endif

		//-------------------------------------------------------------------------

#ifndef USE_NATIVE_CRITICAL_SECTION
//#  ifdef _MSC_VER
//#    pragma optimize( "st", off )
//#  endif
		static LOGICAL LeaveCriticalSecNoWakeEx( PCRITICALSECTION pcs DBG_PASS )
#define LeaveCriticalSecNoWake(pcs) LeaveCriticalSecNoWakeEx( pcs DBG_SRC )
		{
			THREAD_ID dwCurProc;
			while( XCHG( &pcs->dwUpdating, 1 ) )
				Relinquish();
			dwCurProc = GetThisThreadID();
			if( !pcs->dwLocks )
			{
				ll_lprintf( DBG_FILELINEFMT "Leaving a blank critical section (excessive leaves?)" DBG_RELAY );
				DebugBreak();
				pcs->dwUpdating = 0;
				return FALSE;
			}
			if( pcs->dwThreadID == dwCurProc )
			{
				pcs->dwLocks--;
				if( !pcs->dwLocks )
				{
					pcs->dwThreadID = 0;
					if( pcs->dwThreadWaiting )
					{
						pcs->dwUpdating = 0;
						Relinquish(); // allow whoever was waiting to go now...
						return TRUE;
					}
				}
			}
			else
			{
				lprintf( "Sorry - you can't leave a section you don't own...(shouldn't have been past your Enter anyway?)" );
				DebugBreak();
				pcs->dwUpdating = 0;
				return FALSE;
			}
			// allow other locking threads immediate access to section
			// but I know when that happens - since the waiting process
			// will flag - SECTION_LOGGED_WAIT
			pcs->dwUpdating = 0;
			return TRUE;
		}
#else
#define LeaveCriticalSecNoWake(pcs) LeaveCriticalSection(pcs)
#endif


//-------------------------------------------------------------------------
#ifndef USE_NATIVE_CRITICAL_SECTION
		void  InitializeCriticalSec( PCRITICALSECTION pcs )
		{
			memset( pcs, 0, sizeof( CRITICALSECTION ) );
			return;
		}
#endif

#ifdef __cplusplus
	} // namespace timers {
	namespace memory { // resume memory namespace
#endif
//-------------------------------------------------------------------------

#ifdef _DEBUG
static uint32_t dwBlocks; // last values from getmemstats...
static uint32_t dwFreeBlocks;
static uint32_t dwAllocated;
static uint32_t dwFree;
#endif

//------------------------------------------------------------------------------------------------------
#ifndef __NO_MMAP__
static void DoCloseSpace( PSPACE ps, int bFinal );
#endif
//------------------------------------------------------------------------------------------------------

#ifndef __NO_MMAP__

LOGICAL OpenRootMemory()
{
	uintptr_t size = sizeof( SPACEPOOL );
	uint32_t created;
	char spacename[32];
	if( g.pSpacePool != NULL )
	{
		// if local already has something, just return.
		return FALSE;
	}
#ifdef DEBUG_GLOBAL_REGISTRATION
	ll_lprintf( "Opening space..." );
#endif

#ifdef WIN32
	snprintf( spacename, sizeof( spacename ), "memory:%" _32fx, GetCurrentProcessId() );
#else
	snprintf( spacename, sizeof( spacename ), "memory:%08X", getpid() );
#endif
	// hmm application only shared space?
	// how do I get that to happen?
#ifdef __STATIC_GLOBALS__
	 g.pSpacePool = (PSPACEPOOL)OpenSpaceExx( NULL, NULL, 0, &size, &created );
#else
	 g.pSpacePool = (PSPACEPOOL)OpenSpaceExx( spacename, NULL, 0, &size, &created );
#endif
	// I myself must have a global space, which is kept sepearte from named spaces
	// but then... blah
	return created;
}

#endif

// hmm this runs
PRIORITY_ATEXIT(ReleaseAllMemory,ATEXIT_PRIORITY_SHAREMEM)
{
#if defined( __SKIP_RELEASE_OPEN_SPACES__ ) || defined( __NO_MMAP__ )
	// actually, under linux, it releases /tmp/.shared files.
	//ll_lprintf( "No super significant reason to release all memory blocks?" );
	//ll_lprintf( "Short circuit on memory shutdown." );
	return;
#else
	// need to try and close /tmp/.shared region files...  so we only close
	// temporary spaces
	PSPACEPOOL psp;
	PSPACE ps;
	while( ( psp = g.pSpacePool ) )
	{
		int i;
		// I didn't allocate at the root; someone else is responsible.
		if( psp->me != &g.pSpacePool )
			break;
		for( i = 0; i < (((int)(MAX_PER_BLOCK))-1); i++ )
		{
			ps = psp->spaces + i;
			if( ps->pMem )
			{
				/*
				* if we do this, then logging will attempt to possibly use memory which was allocated from this?
#ifdef _DEBUG
				if( !g.bDisableDebug )
				{
					ll_lprintf( "Space: %p mem: %p-%p", ps, ps->pMem, (uint8_t*)ps->pMem + ps->dwSmallSize );
					ll_lprintf( "Closing tracked space..." );
				}
#endif
*/
#ifndef _WIN32
				if( ps->flags.bTemporary )
#endif
					DoCloseSpace( ps, TRUE );
			}
		}
		if( !(*psp->me) )
			break;
		if( ( (*psp->me) = psp->next ) )
			psp->next->me = psp->me;
#ifdef _WIN32
		UnmapViewOfFile( ps->pMem );
		CloseHandle( ps->hMem );
		CloseHandle( ps->hFile );
#else
		//ll_lprintf( "unmaping space tracking structure..." );
		munmap( ps, MAX_PER_BLOCK * sizeof( SPACE ) );
		//close( (int)ps->pMem );
		//if( ps->hFile >= 0 )
		//	close( ps->hFile );
#endif
	}
#endif
	g.bInit = FALSE;
}

//------------------------------------------------------------------------------------------------------

void InitSharedMemory( void )
{
#ifndef __NO_MMAP__
	if( !g.bInit )
	{
	// this would be really slick to do
	// especially in the case where files have been used
	// to back storage...
	// so please do make releaseallmemory smarter and dlea
	// only with closing those regions which have a file
		// backing, espcecially those that are temporary chickens.
		//atexit( ReleaseAllMemory );
#ifdef VERBOSE_LOGGING
		if( !g.bDisableDebug )
			Log2( "CHUNK: %d  MEM:%d", CHUNK_SIZE(0), MEM_SIZE );
#endif
		g.bInit = TRUE;  // onload was definatly a zero.
		{
			if( OpenRootMemory() )
			{
				MemSet( g.pSpacePool, 0, sizeof( SPACEPOOL ) );
				g.pSpacePool->me = &g.pSpacePool;
#ifdef VERBOSE_LOGGING
				if( !g.bDisableDebug )
					Log1( "Allocated Space pool %lu", dwSize );
#endif
			}
		}
	}
	else
	{
#ifdef VERBOSE_LOGGING
		if( !g.bDisableDebug )
			ODS( "already initialized?" );
#endif
	}
#endif
}

//------------------------------------------------------------------------------------------------------
// private
#ifndef __NO_MMAP__
static PSPACE AddSpace( PSPACE pAddAfter
#if defined( WIN32 ) || defined( __CYGWIN__ )
							, HANDLE hFile
							, HANDLE hMem
#else
							, int hFile
							, int hMem
#endif
							, POINTER pMem, uintptr_t dwSize, int bLink )
{
	PSPACEPOOL psp;
	PSPACEPOOL _psp = NULL;
	PSPACE ps;
	int i;
	if( !g.pSpacePool || g.InAdding )
	{
#ifdef VERBOSE_LOGGING
		if( !g.bDisableDebug )
			Log2( "No space pool(%p) or InAdding(%d)", g.pSpacePool, g.InAdding );
#endif
		return NULL;
	}
	g.InAdding = 1;
	//_ps = NULL;
	psp = g.pSpacePool;
Retry:
	do {
		ps = psp->spaces;
		for( i = 0; i < (((int)(MAX_PER_BLOCK))-1); i++ )
		{
			if( !ps[i].pMem )
			{
				ps += i;
				break;
			}
		}
		if( i == (MAX_PER_BLOCK-1) )
		{
			_psp = psp;
			psp = psp->next;
		}
		else
			break;
	} while( psp );

	if( !psp )
	{
		//DebugBreak(); // examine conditions for allocating new space block...
		dwSize = sizeof( SPACEPOOL );
		if( _psp )
		{
			psp = _psp->next = (PSPACEPOOL)OpenSpace( NULL, NULL, &dwSize );
			MemSet( psp, 0, dwSize );
			psp->me = &_psp->next;
		}
		goto Retry;
	}
	//Log7( "Managing space (s)%p (pm)%p (hf)%08" _32fx " (hm)%08" _32fx " (sz)%" _32f " %08" _32fx "-%08" _32fx ""
	//				, ps, pMem, (uint32_t)hFile, (uint32_t)hMem, dwSize
	//				, (uint32_t)pMem, ((uint32_t)pMem + dwSize)
	//				);
	ps->pMem = (PMEM)pMem;

	// okay yes I made this line ugly.
	ps->hFile =
#ifdef _WIN32
					(HANDLE)
#endif
								hFile;
#ifdef _WIN32
	ps->hMem = hMem;
#endif
	ps->dwSmallSize = dwSize;
	/*
	if( bLink )
	{
		while( AddAfter && AddAfter->next )
			AddAfter = AddAfter->next;
		//Log2( "Linked into space...%p after %p ", ps, AddAfter );
		if( AddAfter )
		{
			ps->me = &AddAfter->next;
			AddAfter->next = ps;
		}
  		ps->next = NULL;
	}
	*/
	g.InAdding = 0;
	return ps;
}

//------------------------------------------------------------------------------------------------------

PSPACE FindSpace( POINTER pMem )
{
	PSPACEPOOL psp;
	INDEX idx;
	for( psp = g.pSpacePool;psp; psp = psp->next)
		for( idx = 0; idx < MAX_PER_BLOCK; idx++ )
			if( psp->spaces[idx].pMem == pMem )
				return psp->spaces + idx;
	return NULL;
}

//------------------------------------------------------------------------------------------------------

static void DoCloseSpace( PSPACE ps, int bFinal )
{
	if( ps )
	{
		//Log( "Closing a space..." );
#ifdef _WIN32
		UnmapViewOfFile( ps->pMem );
		CloseHandle( ps->hMem );
		CloseHandle( ps->hFile );
#else
		munmap( ps->pMem, ps->dwSmallSize );
		if( (ps->hFile >= 0) )
		{
			if( ps->flags.bTemporary && bFinal )
			{
				char file[256];
				char fdname[64];
				snprintf( fdname, sizeof(fdname), "/proc/self/fd/%d", (int)ps->hFile );
				file[readlink( fdname, file, sizeof( file ) )] = 0;
				remove( file );
			}
			close( (int)ps->hFile );
		}
#endif
		MemSet( ps, 0, sizeof( SPACE ) );
	}
}

//------------------------------------------------------------------------------------------------------

 void  CloseSpaceEx ( POINTER pMem, int bFinal )
{
	DoCloseSpace( FindSpace( pMem ), bFinal );
}

//------------------------------------------------------------------------------------------------------

 void  CloseSpace ( POINTER pMem )
{
	DoCloseSpace( FindSpace( pMem ), TRUE );
}
//------------------------------------------------------------------------------------------------------

 uintptr_t  GetSpaceSize ( POINTER pMem )
{
	PSPACE ps;
	ps = FindSpace( pMem );
	if( ps )
		return ps->dwSmallSize;
	return 0;
}
#endif

#if defined( __LINUX__ ) && !defined( __CYGWIN__ )
uintptr_t GetFileSize( int fd )
{
	uintptr_t len = lseek( fd, 0, SEEK_END );
	lseek( fd, 0, SEEK_SET );
	return len;
}

#endif
//------------------------------------------------------------------------------------------------------

#ifndef __NO_MMAP__
 POINTER  OpenSpaceExx ( CTEXTSTR pWhat, CTEXTSTR pWhere, uintptr_t address, uintptr_t *dwSize, uint32_t* bCreated )
{
	POINTER pMem = NULL;
#ifdef USE_SIMPLE_LOCK_ON_OPEN
	static uint32_t bOpening;
#endif

#ifndef USE_SIMPLE_LOCK_ON_OPEN
	static CRITICALSECTION cs;
	static int first = 1;
#endif
	int readonly = FALSE;
	if( !dwSize ) return NULL;
	if( !g.bInit )
	{
		//ODS( "Doing Init" );
		InitSharedMemory();
	}
#ifndef USE_SIMPLE_LOCK_ON_OPEN
	if( g.deadstart_finished )
	{
		if( first )
		{
			InitializeCriticalSection( &cs );
			first = 0;
		}
		while( !EnterCriticalSecNoWait( &cs, NULL ) )
			Relinquish();
	}

#else
	while( XCHG( &bOpening, 1 ) )
		Relinquish();
#endif
	{
#ifdef __LINUX__
		char *filename = NULL;
		int fd = -1;
		int bTemp = FALSE;
		int exists = FALSE;
		if( !pWhat && !pWhere)
		{
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif
			pMem = mmap( 0, *dwSize
						 , PROT_READ|PROT_WRITE
						 , MAP_SHARED|MAP_ANONYMOUS
						 ,
#ifdef __QNX__
							NOFD  // QNX Note; NOFD = -1
#else // other systems were quite happy to have a 0 here for the handle.
							0
#endif
						  , 0 );
			if( pMem == (POINTER)-1 )
			{
				if( errno == ENODEV ) {
					pMem = malloc( *dwSize );
				} else {
				ll_lprintf( "Something bad about this region sized %" _PTRSZVALfs "(%d)", *dwSize, errno );
				DebugBreak();
				}
			}
			if( pMem != (POINTER)-1 )
			{
				//ll_lprintf( "Clearing anonymous mmap %p %" _size_f "", pMem, *dwSize );
				MemSet( pMem, 0, *dwSize );
			}
		}
		else if( pWhere ) // name doesn't matter, same file cannot be called another name
		{
			filename = (char*)pWhere;
 		}
		else if( pWhat )
		{
#ifndef __STATIC_GLOBALS__
			int len;
         		char tmpbuf[256];
#ifdef __ANDROID__
			//if( !IsPath( "./tmp" ) )
			//	if( !MakePath( "./tmp" ) )
			//		ll_lprintf( "Failed to create a temporary space" );
			filename = tmpbuf;
			snprintf( tmpbuf, 256, "./tmp.shared.%s", pWhat );
#else
			filename = tmpbuf;
			snprintf( tmpbuf, 256, "/tmp/.shared.%s", pWhat );

#endif
			bTemp = TRUE;
#endif
		}

		//ll_lprintf( "Open Space: %s", filename?filename:"anonymous" );

		if( !pMem && filename )
		{
#ifdef __ANDROID__
			//fd = ashmem_create_region( filename , size );
			if( pWhat )
			{
				fd = open(filename, O_RDWR);
				if (fd < 0 )
				{
					int ret;
					if( !(*dwSize ) )
					{
						ll_lprintf( "Region didn't exist... and no size... return" );
						return NULL;
					}
#   ifdef DEBUG_SHARED_REGION_CREATE
					ll_lprintf( "Shared region didn't already exist...: %s", filename );
#   endif
					fd = open("/dev/ashmem", O_RDWR);
					if( fd < 0 )
					{
						ll_lprintf( "Failed to open core device..." );
						return NULL;
					}
					if( bCreated )
						(*bCreated) = 1;

					ret = ioctl(fd, ASHMEM_SET_NAME, filename + 12 ); // skip 11 for the "/dev/ashmem/"
					if (ret < 0)
					{
						ll_lprintf( "Failed to set the name of ashmem region: %s", filename + 12 );
						//							goto error;
					}

					ret = ioctl(fd, ASHMEM_SET_SIZE, (*dwSize) );
					if (ret < 0)
					{
						ll_lprintf( "Failed to set IOCTL size to %d", (*dwSize) );
						//goto error;
					}
					/*
					 {
						// unpin; pages will be pined to start (I think)
						struct ashmem_pin pin = {
							.offset = 0,
							.len    = (*dwSize)
						};
						ret = ioctl(fd, ASHMEM_UNPIN, &pin);
					}
					*/
				}
				else
				{
					if( bCreated )
						(*bCreated) = 1;
				}
			}
			else
#endif
			{
				mode_t prior;
				if( bCreated )
					(*bCreated) = 1;
				prior = umask( 0 );
				fd = open( filename, O_RDWR|O_CREAT|O_EXCL, 0600 );
				umask(prior);
			}
			if( fd == -1 )
			{
				//ll_lprintf( "open is %d %s %d", errno, filename, prior );
				// if we didn't create the file...
				// then it can't be marked as temporary...
				bTemp = FALSE;
				if( GetLastError() == EEXIST )
				{
					exists = TRUE;
					fd = open( filename, O_RDWR );
					bTemp = FALSE;
					if( bCreated )
						(*bCreated) = 0;
				}
				if( fd == -1 )
				{
					readonly = TRUE;
					fd = open( filename, O_RDONLY );
				}
				if( fd == -1 )
				{
					Log2( "Sorry - failed to open: %d %s"
						, errno
						, filename );
#ifndef USE_SIMPLE_LOCK_ON_OPEN
					if( g.deadstart_finished )
					{
						LeaveCriticalSecNoWake( &cs );
					}
#else
					bOpening = FALSE;
#endif
					//if(filename)Release( filename );
					return NULL;
				}
			}
			if( exists )
			{
				if( GetFileSize( fd ) < (uintptr_t)*dwSize )
				{
					// expands the file...
					ftruncate( fd, *dwSize );
					//*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
				}
				else
				{
					// expands the size requested to that of the file...
					(*dwSize) = GetFileSize( fd );
				}
			}
			else
			{
				if( !*dwSize )
				{
					// can't create a 0 sized file this way.
					(*dwSize) = 1; // not zero.
					close( fd );
					unlink( filename );
#ifndef USE_SIMPLE_LOCK_ON_OPEN
					if( g.deadstart_finished )
					{
						LeaveCriticalSecNoWake( &cs );
					}
#else
					bOpening = FALSE;
#endif
					//if(filename)Release( filename );
					return NULL;
				}
				//*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
				ftruncate( fd, *dwSize );
			}
			pMem = mmap( 0, *dwSize
			          , PROT_READ|(readonly?(0):PROT_WRITE)
			          , MAP_SHARED|((fd<0)?MAP_ANONYMOUS:0)
			          , fd, 0 );
			if( !exists && pMem )
			{
				MemSet( pMem, 0, *dwSize );
			}
		}
		if( pMem )
		{
			PSPACE ps = AddSpace( NULL, fd, 0, pMem, *dwSize, TRUE );
			if( ps )
				ps->flags.bTemporary = bTemp;
		}
#ifndef USE_SIMPLE_LOCK_ON_OPEN
		if( g.deadstart_finished )
		{
			LeaveCriticalSecNoWake( &cs );
		}
#else
		bOpening = FALSE;
#endif
		//if(filename)Release( filename );
		return pMem;

#elif defined( _WIN32 )
#ifndef UNDER_CE
		LOGICAL didCreate = FALSE;
		HANDLE hFile;
		HANDLE hMem = NULL;
		*dwSize = ( ( (*dwSize) + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
		if( !pWhat && !pWhere )
		{
			//ll_lprintf( "ALLOCATE %" _64fx"d", (*dwSize)>>32, 0 );
			hMem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL
											, PAGE_READWRITE
											|SEC_COMMIT
#if __64__
											, (*dwSize)>>32 // dwSize is sometimes 64 bit... this should be harmless
											, (*dwSize) & (0xFFFFFFFF)
#else
											, 0
											, (*dwSize)
#endif
											, pWhat ); // which should be NULL... but is consistant
			if( !hMem )
			{
				//ll_lprintf( "Failed to allocate pagefile memory?! %p %d", *dwSize, GetLastError() );

				{
					POINTER p = malloc( *dwSize );
					//ll_lprintf(" but we could allocate it %p", p  );
#ifndef USE_SIMPLE_LOCK_ON_OPEN
					if( g.deadstart_finished )
					{
						LeaveCriticalSecNoWake( &cs );
					}
#else
					bOpening = 0;
#endif
					return p;
				}
			}
			else
			{
			// created and this size is right...
				if( bCreated )
					(*bCreated) = TRUE;
			}
		}
		else if( pWhat )
		{
			hMem = OpenFileMapping( FILE_MAP_READ|FILE_MAP_WRITE
										, FALSE
										, pWhat );
			if( hMem )
			{
				if( bCreated )
					(*bCreated) = FALSE;
			}
			else
			{
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "Failed to open region named %s %d", pWhat, GetLastError() );
#endif
				if( (*dwSize) == 0 )  // don't continue... we're expecting open-existing behavior
				{
#ifndef USE_SIMPLE_LOCK_ON_OPEN
					if( g.deadstart_finished )
					{
						LeaveCriticalSecNoWake( &cs );
					}
#else
					bOpening = 0;
#endif
					return FALSE;
				}
			}
		}

		hFile = INVALID_HANDLE_VALUE;
		// I would have hmem here if the file was validly opened....
		if( !hMem )
		{
			hFile = CreateFile( pWhere, GENERIC_READ|GENERIC_WRITE
									,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
									,NULL // default security
									,(dwSize&&(*dwSize)) ? OPEN_ALWAYS : OPEN_EXISTING
									,FILE_ATTRIBUTE_NORMAL //|FILE_ATTRIBUTE_TEMPORARY
									//| FILE_FLAG_WRITE_THROUGH
									//| FILE_FLAG_NO_BUFFERING
									// must access on sector bournds
									// must read complete sectors
									//| FILE_FLAG_DELETE_ON_CLOSE
									, NULL );
#ifdef DEBUG_OPEN_SPACE
			ll_lprintf( "Create file %s result %d", pWhere, hFile );
			ll_lprintf( "File result is %ld (error %ld)", hFile, GetLastError() );
#endif
			if( hFile == INVALID_HANDLE_VALUE )
			{
				readonly = 1;
				if( ( dwSize && (!(*dwSize )) ) && ( GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_FILE_NOT_FOUND ) )
				{
#ifdef DEBUG_OPEN_SPACE
					ll_lprintf( "File did not exist, and we're not creating the file (0 size passed)" );
#endif
#ifndef USE_SIMPLE_LOCK_ON_OPEN
					if( g.deadstart_finished )
					{
						LeaveCriticalSecNoWake( &cs );
					}
#else
					bOpening = 0;
#endif
					return NULL;
				}
				hFile = CreateFile( pWhere, GENERIC_READ
										,FILE_SHARE_READ|FILE_SHARE_DELETE
										,NULL // default security
										,OPEN_ALWAYS
										,FILE_ATTRIBUTE_NORMAL //|FILE_ATTRIBUTE_TEMPORARY
										//| FILE_FLAG_WRITE_THROUGH
										//| FILE_FLAG_NO_BUFFERING
										// must access on sector bournds
										// must read complete sectors
										//| FILE_FLAG_DELETE_ON_CLOSE
										, NULL );
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "Create file %s result %d", pWhere, hFile );
#endif
				if( hFile != INVALID_HANDLE_VALUE ) {
					SetLastError( ERROR_ALREADY_EXISTS ); // lie...
				}
			}
			else {
				SetLastError( ERROR_ALREADY_EXISTS ); // lie...
			}

			if( hFile == INVALID_HANDLE_VALUE )
			{
				// might still be able to open it by shared name; even if the file share is disabled
				readonly = 0;
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "file is still invalid(alreadyexist?)... new size is %d %d on %p", (*dwSize), FILE_GRAN, hFile );
#endif
				hMem = CreateFileMapping( hFile // is INVALID_HANDLE_VALUE, but is consistant
												, NULL
												, (readonly?PAGE_READONLY:PAGE_READWRITE)
												/*|SEC_COMMIT|SEC_NOCACHE*/
#ifdef __64__
												, (uint32_t)((*dwSize)>>32)
#else
												, 0
#endif
												, (uint32_t)(*dwSize)
												, pWhat );
				if( hMem )
				{
					if( bCreated )
						(*bCreated) = 1;
					goto isokay;
				}
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "Sorry - Nothing good can happen with a filename like that...%s %d", pWhat, GetLastError());
#endif
					 //bOpening = FALSE;
#ifndef USE_SIMPLE_LOCK_ON_OPEN
				if( g.deadstart_finished )
				{
					LeaveCriticalSecNoWake( &cs );
				}
#else
				bOpening = 0;
#endif
				return NULL;
			}

			if( GetLastError() == ERROR_ALREADY_EXISTS )
			{
				LARGE_INTEGER lSize;
				GetFileSizeEx( hFile, &lSize );
			// mark status for memory... dunno why?
				// in theory this is a memory image of valid memory already...
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "Getting existing size of region..." );
#endif
				if( SUS_LT( lSize.QuadPart, LONGLONG, (*dwSize), uintptr_t ) )
				{
#ifdef DEBUG_OPEN_SPACE
					ll_lprintf( "Expanding file to size requested." );
#endif
					didCreate = 1;
					SetFilePointer( hFile, (LONG) * (int32_t*)dwSize, (sizeof(dwSize[0])>4)?(PLONG)(((int32_t*)dwSize) + 1):NULL, FILE_BEGIN );
					SetEndOfFile( hFile );
				}
				else
				{
#ifdef DEBUG_OPEN_SPACE
					ll_lprintf( "Setting size to size of file (which was larger.." );
#endif
					if( *dwSize ) {
						SetFilePointer( hFile, (LONG) * (int32_t*)dwSize, (sizeof( dwSize[0] ) > 4) ? (PLONG)(((int32_t*)dwSize) + 1) : NULL, FILE_BEGIN );
						SetEndOfFile( hFile );
					}
					else
						(*dwSize) = (uintptr_t)(lSize.QuadPart);
				}
			}
			else
			{
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "New file, setting size to requested %d", *dwSize );
#endif
				SetFilePointer( hFile, (LONG)*(int32_t*)dwSize, (sizeof( dwSize[0] ) > 4) ? (PLONG)(((int32_t*)dwSize)+1) : NULL, FILE_BEGIN );
				SetEndOfFile( hFile );
				didCreate = 1;
			}
			if( bCreated )
				(*bCreated) = didCreate;
			//(*dwSize) = GetFileSize( hFile, NULL );
#ifdef DEBUG_OPEN_SPACE
			ll_lprintf( "%s Readonly? %d  hFile %d", pWhat, readonly, hFile );
#endif
			hMem = CreateFileMapping( hFile
											, NULL
											, (readonly?PAGE_READONLY:PAGE_READWRITE)
											/*|SEC_COMMIT|SEC_NOCACHE*/
											, 0, 0
											, pWhat );
			if( pWhat && !hMem )
			{
#ifdef DEBUG_OPEN_SPACE
				ll_lprintf( "Create of mapping failed on object specified? %d %p", GetLastError(), hFile );
#endif
				(*dwSize) = 1;
				CloseHandle( hFile );
				//bOpening = FALSE;
#ifndef USE_SIMPLE_LOCK_ON_OPEN
				if( g.deadstart_finished )
				{
					LeaveCriticalSecNoWake( &cs );
				}
#else
				bOpening = 0;
#endif
				return NULL;
			}
		}
	isokay:
      /*
		if( !hMem )
		{
			pMem = VirtualAlloc( address, (*dwSize), 0 ,PAGE_READWRITE );
			if( !VirtualLock( pMem, (*dwSize ) ) )
            DebugBreak();
		}
		else
      */
		{
			pMem = MapViewOfFileEx( hMem
										, FILE_MAP_READ| ((readonly)?(0):(FILE_MAP_WRITE))
										, 0, 0  // offset high, low
										, 0	 // size of file to map
										, (POINTER)address ); // don't specify load location... irrelavent...
		}
	if( !pMem )
	{
#ifdef DEBUG_OPEN_SPACE
		Log1( "Create view of file for memory access failed at %p", (POINTER)address );
#endif
		CloseHandle( hMem );
		if( hFile != INVALID_HANDLE_VALUE )
			CloseHandle( hFile );
#ifndef USE_SIMPLE_LOCK_ON_OPEN
		if( g.deadstart_finished )
		{
			LeaveCriticalSecNoWake( &cs );
		}
#else
		bOpening = 0;
#endif
		return NULL;
	}
	else
	{
		if( bCreated && !(*bCreated) && ((*dwSize) == 0) )
		{
			MEMORY_BASIC_INFORMATION meminfo;
			VirtualQuery( pMem, &meminfo, sizeof( meminfo ) );
			(*dwSize) = meminfo.RegionSize;
#ifdef DEBUG_OPEN_SPACE
			ll_lprintf( "Fixup memory size to %ld %s:%s(reported by system on view opened)"
					, *dwSize, pWhat?pWhat:"ANON", pWhere?pWhere:"ANON" );
#endif
		}
	}
	// store information about this
	// external to the space - do NOT
	// modify content of memory opened!
	AddSpace( NULL, hFile, hMem, pMem, *dwSize, TRUE );
#ifndef USE_SIMPLE_LOCK_ON_OPEN
	if( g.deadstart_finished )
	{
		LeaveCriticalSecNoWake( &cs );
	}
#else
      bOpening = 0;
#endif
	return pMem;
#else
	if( bCreated )
		(*bCreated) = 1;
	return malloc( *dwSize );
#endif
#endif
	}
}

//------------------------------------------------------------------------------------------------------
#undef OpenSpaceEx
 POINTER  OpenSpaceEx ( CTEXTSTR pWhat, CTEXTSTR pWhere, uintptr_t address, uintptr_t *dwSize )
{
	uint32_t bCreated;
	return OpenSpaceExx( pWhat, pWhere, address, dwSize, &bCreated );
}

//------------------------------------------------------------------------------------------------------
#undef OpenSpace
 POINTER  OpenSpace ( CTEXTSTR pWhat, CTEXTSTR pWhere, uintptr_t *dwSize )
{
	return OpenSpaceEx( pWhat, pWhere, 0, dwSize );
}
#endif
//------------------------------------------------------------------------------------------------------

 int  InitHeap( PMEM pMem, uintptr_t dwSize )
{
	//pMem->dwSize = *dwSize - MEM_SIZE;
	// size of the PMEM block is all inclusive (from pMem(0) to pMem(dwSize))
	// do NOT need to substract the size of the tracking header
	// otherwise we would be working from &pMem->pRoot + dwSize
	if( pMem->dwSize )
	{
		if( pMem->dwHeapID != 0xbab1f1ea )
		{
			ll_lprintf( "Memory has content, and is NOT a heap!" );
			return FALSE;
		}
		ll_lprintf( "Memory was already initialized as a heap?" );
		return FALSE;
	}
#ifndef __NO_MMAP__
	if( !FindSpace( pMem ) )
	{
		//ll_lprintf( "space for heap has not been tracked yet...." );
		// a heap must be in the valid space pool.
		// it may not have come from a file, and will not have
		// a file or memory handle.
		AddSpace( NULL, 0, 0, pMem, dwSize, TRUE );
	}
#endif
	// the size passed is the full size of the memory, so we need to remove sizeof(MEM)
	// so there is room to track heap info at the start of the heap.
	dwSize -= sizeof( MEM );
	pMem->dwSize = dwSize;
	pMem->dwHeapID = 0xbab1f1ea;
	pMem->pFirstFree = NULL;
	pMem->dwFlags = 0;
        // this uses the address of next to assign a member.
        // next is the first member of this structure;
        // even if it is packed, there's no issue surely with taking the address of the first member?
	LinkThing( pMem->pFirstFree, pMem->pRoot );
	InitializeCriticalSec( &pMem->cs );
	pMem->pRoot[0].dwSize = dwSize - MEM_SIZE - CHUNK_SIZE;
	pMem->pRoot[0].info.dwPad = MAGIC_SIZE;
	pMem->pRoot[0].info.dwOwners = 0;
	pMem->pRoot[0].pRoot  = pMem;
	pMem->pRoot[0].pPrior = NULL;
#ifdef _DEBUG
	if( !g.bDisableDebug )
	{
#ifdef VERBOSE_LOGGING
		ll_lprintf( "Initializing %p %d"
				, pMem->pRoot[0].byData
				, pMem->pRoot[0].dwSize );
#endif
		MemSet( pMem->pRoot[0].byData, 0x1BADCAFE, pMem->pRoot[0].dwSize );
		BLOCK_TAG( pMem->pRoot ) = BLOCK_TAG_ID;
	}
	{
		pMem->pRoot[0].info.dwPad += 2*MAGIC_SIZE;
		BLOCK_FILE( pMem->pRoot ) = __FILE__;
		BLOCK_LINE( pMem->pRoot ) = __LINE__;
	}
#endif
	return TRUE;
}

//------------------------------------------------------------------------------------------------------
#ifndef __NO_MMAP__

PMEM DigSpace( TEXTSTR pWhat, TEXTSTR pWhere, uintptr_t *dwSize )
{
	PMEM pMem = (PMEM)OpenSpace( pWhat, pWhere, dwSize );

	if( !pMem )
	{
		// did reference BASE_MEMORY...
		ll_lprintf( "Create view of file for memory access failed at %p %p", pWhat, pWhere );
		CloseSpace( (POINTER)pMem );
		return NULL;
	}
#ifdef VERBOSE_LOGGING
	Log( "Go to init the heap..." );
#endif
	pMem->dwSize = 0;
#if USE_CUSTOM_ALLOCER
	InitHeap( pMem, *dwSize );
#endif
	return pMem;
}

//------------------------------------------------------------------------------------------------------

int ExpandSpace( PMEM pHeap, uintptr_t dwAmount )
{
	PSPACE pspace = FindSpace( (POINTER)pHeap ), pnewspace;
	PMEM pExtend;
	//ll_lprintf( "Expanding by %d %d", dwAmount );
	pExtend = DigSpace( NULL, NULL, &dwAmount );
	if( !pExtend )
	{
		ll_lprintf( "Failed to expand space by %" _PTRSZVALfs, dwAmount );
		return FALSE;
	}
	pnewspace = FindSpace( pExtend );
	if( pnewspace )
	{
		while( pspace && pspace->next )
			pspace = pspace->next;
		if( ( pspace->next = pnewspace ) )
		{
			pnewspace->me = &pspace->next;
		}
	}
	return TRUE;
}

#endif
//------------------------------------------------------------------------------------------------------

static PMEM InitMemory( void ) {
	uintptr_t MinSize = SYSTEM_CAPACITY;
	// generic internal memory, unnamed, unshared, unsaved
#ifndef __NO_MMAP__
	g.pMemInstance = DigSpace( NULL, NULL, &MinSize );
	if( !g.pMemInstance )
	{
		g.bMemInstanced = FALSE;
		ODS( "Failed to allocate memory - assuming fatailty at Allocation service level." );
		return NULL;
	}
#endif
	return g.pMemInstance;
}

//------------------------------------------------------------------------------------------------------

static PMEM GrabMemEx( PMEM pMem DBG_PASS )
#define GrabMem(m) GrabMemEx( m DBG_SRC )
{
	if( !pMem )
	{
		// use default heap...
		if( !XCHG( &g.bMemInstanced, TRUE ) )
			pMem = InitMemory();
		else {
			pMem = g.pMemInstance;
		}
	}
	//ll_lprintf( "grabbing memory %p", pMem );
	{
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
		int log = g.bLogCritical;
		g.bLogCritical = 0;
#endif
#ifdef USE_NATIVE_CRITICAL_SECTION
		while( !TryEnterCriticalSection( &pMem->cs ) )
		{
			Relinquish();
		}
#else
		while( EnterCriticalSecNoWaitEx( &pMem->cs, NULL DBG_RELAY ) <= 0 )
		{
			Relinquish();
		}
#endif
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
		g.bLogCritical = log;
#endif
	}
	return pMem;
}

//------------------------------------------------------------------------------------------------------

static void DropMemEx( PMEM pMem DBG_PASS )
#define DropMem(m) DropMemEx( m DBG_SRC)
{
	if( !pMem )
		return;
	//ll_lprintf( "dropping memory %p", pMem );
	{
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
		int log = g.bLogCritical;
		g.bLogCritical = 0;
#endif
#ifdef USE_NATIVE_CRITICAL_SECTION
		LeaveCriticalSection( &pMem->cs );
#else
		LeaveCriticalSecNoWakeEx( &pMem->cs DBG_RELAY );
#endif
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
		g.bLogCritical = log;
#endif
	}
}

//------------------------------------------------------------------------------------------------------

POINTER HeapAllocateAlignedEx( PMEM pHeap, size_t dwSize, uint16_t alignment DBG_PASS )
{
   // if a heap is passed, it's a private heap, and allocation is as normal...
	uint32_t dwAlignPad = 0;
#if defined( __64__ )
	if( !alignment ) alignment = 8;
#endif
	if( alignment ) {
		dwSize += (alignment - 1);
		dwAlignPad = (alignment - 1);
	}
	if( !pHeap && !USE_CUSTOM_ALLOCER )
	{
		PMALLOC_CHUNK pc;
		uintptr_t mask;
#ifdef ENABLE_NATIVE_MALLOC_PROTECTOR
		pc = (PMALLOC_CHUNK)malloc( sizeof( MALLOC_CHUNK ) - 1 + alignment + dwSize + sizeof( pc->LeadProtect ) );
		if( !pc )
			DebugBreak();
		MemSet( pc->LeadProtect, LEAD_PROTECT_TAG, sizeof( pc->LeadProtect ) );
		MemSet( pc->byData + dwSize, LEAD_PROTECT_BLOCK_TAIL, sizeof( pc->LeadProtect ) );
#else
		pc = (PMALLOC_CHUNK)malloc( sizeof( MALLOC_CHUNK ) - 1 + dwSize );
#endif
		pc->info.dwOwners = 1;
		pc->info.dwPad = dwAlignPad;
		pc->dwSize = dwSize;
#ifndef NO_LOGGING
#  ifdef _DEBUG
		if( g.bLogAllocate )
		{
			ll__lprintf(DBG_RELAY)( "alloc %p(%p) %zd", pc, pc->byData, dwSize );
		}
#  endif
#endif
		if( alignment > (sizeof( masks ) / sizeof( masks[0] )) )
			mask = (~((uintptr_t)(alignment-1)));
		else
			mask = masks[alignment];
		if( alignment && ( (uintptr_t)pc->byData & ~mask) ) {
			uintptr_t retval = ((((uintptr_t)pc->byData) + (alignment - 1)) & mask);
			//pc->info.dwPad = (uint16_t)( dwAlignPad - sizeof(uintptr_t) );
			// to_chunk_start is the last thing in chunk, so it's pre-allocated space
			((uint16_t*)(retval - sizeof(uint32_t)))[0] = alignment;
			((uint16_t*)(retval - sizeof(uint32_t)))[1] = (uint16_t)(((((uintptr_t)pc->byData) + (alignment - 1)) & mask) - (uintptr_t)pc->byData);
			return (POINTER)retval; //-V773
		}
		else {
			pc->info.alignment = 0;
			pc->info.to_chunk_start = 0;
			return pc->byData;
		}
	}
#if USE_CUSTOM_ALLOCER
	else
	{
		PHEAP_CHUNK pc;
		PMEM pMem, pCurMem = NULL;
		PSPACE pMemSpace;
		uint32_t dwPad = 0;
		uint32_t dwMin = 0;
		intptr_t mask;
		if( alignment > ( sizeof( masks ) / sizeof( masks[0] ) ) )
			mask = ( ~( (uintptr_t)( alignment - 1 ) ) );
		else
			mask = masks[alignment];

		//ll__lprintf(DBG_RELAY)( "..." );
#ifdef _DEBUG
		if( !g.bDisableAutoCheck )
			GetHeapMemStatsEx(pHeap, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
		if( !dwSize ) // no size is NO space!
		{
			return NULL;
		}
		// if memstats is used - memory could have been initialized there...
		// so wait til now to grab g.pMemInstance.
		if( !pHeap )
			pHeap = g.pMemInstance;
		pMem = GrabMem( pHeap );
		dwPad = dwAlignPad;
#ifdef __64__
		//dwPad = (uint32_t)( (((dwSize + 7) & 0xFFFFFFFFFFFFFFF8) - dwSize) );
		//dwSize += 7; // fix size to allocate at least _32s which
		//dwSize &= 0xFFFFFFFFFFFFFFF8;
#else
		//dwPad = (((dwSize + 3) & 0xFFFFFFFC) -dwSize);
		//dwSize += 3; // fix size to allocate at least _32s which
		//dwSize &= 0xFFFFFFFC;
#endif

#ifdef _DEBUG
		if( pMem && !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
		{
			dwPad += MAGIC_SIZE * 2;
			dwSize += MAGIC_SIZE * 2; // pFile, nLine per block...
									  //ll_lprintf( "Adding 8 bytes to block size..." );
		}
		if( !g.bDisableDebug )
		{
			dwPad += MAGIC_SIZE;
			dwSize += MAGIC_SIZE;  // add a uint32_t at end to mark, and check for application overflow...
		}
		dwMin = dwPad;
#endif

		// re-search for memory should step long back...
	search_for_free_memory:
		for( pc = NULL, pMemSpace = FindSpace( pMem ); !pc && pMemSpace; pMemSpace = pMemSpace->next )
		{
			// grab the new memory (might be old, is ok)
			GrabMem( (PMEM)pMemSpace->pMem );
			// then drop old memory, don't need that anymore.
			if( pCurMem ) // first time through, there is no current.
				DropMem( pCurMem );
			// then mark that this block is our current block.
			pCurMem = (PMEM)pMemSpace->pMem;
			//ll_lprintf( "region %p is now owned.", pCurMem );

			for( pc = pCurMem->pFirstFree; pc; pc = pc->next )
			{
				if( pc->dwSize >= dwSize ) // if free block size is big enough...
				{
					// split block
					if( ( pc->dwSize - dwSize ) <= ( dwMin + CHUNK_SIZE + g.nMinAllocateSize ) ) // must allocate it all.
					{
						pc->info.dwPad = (uint16_t)(dwPad + ( pc->dwSize - dwSize ));
						UnlinkThing( pc );
						pc->info.dwOwners = 1;
						break; // successful allocation....
					}
					else
					{
						PHEAP_CHUNK pNew;  // cleared, NEW, uninitialized block...
						PHEAP_CHUNK next;
						next = (PHEAP_CHUNK)( pc->byData + pc->dwSize );
						pNew = (PHEAP_CHUNK)(pc->byData + dwSize);
						pNew->info.dwPad = 0;
						pNew->dwSize = ((pc->dwSize - CHUNK_SIZE) - dwSize);
#ifdef _DEBUG
						if( pNew->dwSize > 0x80000000 )
							DebugBreak();
						if( pMem && !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
						{
							pNew->info.dwPad += MAGIC_SIZE * 2;
						}
						if( !g.bDisableDebug )
						{
							pNew->info.dwPad += MAGIC_SIZE;
							BLOCK_TAG( pNew ) = BLOCK_TAG_ID;
						}
						if( pMem && !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
						{
							BLOCK_FILE( pNew ) = pFile;
							BLOCK_LINE( pNew ) = nLine;
						}
#endif

						pc->info.dwPad = (uint16_t)dwPad;
						pc->dwSize = dwSize; // set old size?  this can wait until we have the block.
						if( pc->dwSize & 0x80000000 )
							DebugBreak();

						if( (uintptr_t)next - (uintptr_t)pCurMem < (uintptr_t)pCurMem->dwSize )  // not beyond end of memory...
							next->pPrior = pNew;

						pNew->info.dwOwners = 0;
//#ifdef _DEBUG
						pNew->pRoot = pc->pRoot;
//#endif
						pNew->pPrior = pc;

						// copy link...
						if( ( pNew->next = pc->next ) )
							pNew->next->me = &pNew->next;
						pNew->me = pc->me;
						pc->me[0] = pNew;
						pc->info.dwOwners = 1;  // set owned block.
						break; // successful allocation....
					}
				}
			}
		}
		if( !pc )
		{
			if( dwSize < SYSTEM_CAPACITY )
			{
				if( ExpandSpace( pMem, SYSTEM_CAPACITY ) )
					goto search_for_free_memory;
			}
			else
			{
				// after 1 allocation, need a free chunk at end...
				// and let's just have a couple more to spaere.
				if( ExpandSpace( pMem, dwSize + 4096 + (CHUNK_SIZE*4) + MEM_SIZE + 8 * MAGIC_SIZE ) )
				{
#ifndef NO_LOGGING
					//ll__lprintf(DBG_RELAY)( "Creating a new expanded space... %" _size_fs, dwSize + (CHUNK_SIZE*4) + MEM_SIZE + 8 * MAGIC_SIZE );
#endif
					goto search_for_free_memory;
				}
			}
			DropMem( pCurMem );
			pCurMem = NULL;

#ifdef _DEBUG
			if( !g.bDisableDebug )
				ODS( "Remaining space in memory block is insufficient.  Please EXPAND block.");
#endif
			DropMem( pMem );
			return NULL;
		}

#if defined( _DEBUG ) //|| !defined( __NO_WIN32API__ )
		if( !g.bDisableDebug )
		{
			// set end of block tag(s).
			// without disabling memory entirely, blocks are
			// still tagged and trashed in debug mode.
			MemSet( pc->byData, CLEAR_MEMORY_TAG, pc->dwSize );
			BLOCK_TAG(pc) = BLOCK_TAG_ID;
		}
		if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
		{
			if( pc->info.dwPad < 2*sizeof( uintptr_t) )
				DebugBreak();
			BLOCK_FILE(pc) = pFile;
			BLOCK_LINE(pc) = nLine;
		}
#endif
		DropMem( pCurMem );
		DropMem( pMem );
		//#if DBG_AVAILABLE
#ifndef NO_LOGGING
#  ifdef _DEBUG
		if( g.bLogAllocate && g.allowLogging )
		{
			_xlprintf( 2 DBG_RELAY )("Allocate : %p(%p) - %" _PTRSZVALfs " bytes", pc->byData, pc, pc->dwSize);
		}
#  endif
#endif
		//#endif
		if( alignment && ((uintptr_t)pc->byData & ~mask ) ) {
			uintptr_t retval = ((((uintptr_t)pc->byData) + (alignment - 1)) & mask );
			((uint16_t*)(retval - sizeof( uint32_t )))[0] = alignment;
			((uint16_t*)(retval - sizeof( uint32_t )))[1] = (uint16_t)(((((uintptr_t)pc->byData) + (alignment - 1)) & mask ) - (uintptr_t)pc->byData);
			return (POINTER)retval;
		}
		else {
			pc->info.alignment = 0;
			pc->info.to_chunk_start = 0;
			return pc->byData;
		}
	}
#endif

	return NULL;
}

//------------------------------------------------------------------------------------------------------
POINTER HeapAllocateEx( PMEM pHeap, uintptr_t dwSize DBG_PASS ) {
	return HeapAllocateAlignedEx( pHeap, dwSize, 0 DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------
#undef AllocateEx
POINTER  AllocateEx ( uintptr_t dwSize DBG_PASS )
{
	return HeapAllocateAlignedEx( g.pMemInstance, dwSize, 0 DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

POINTER  HeapReallocateAlignedEx ( PMEM pHeap, POINTER source, uintptr_t size, uint16_t alignment DBG_PASS )
{
	POINTER dest;
	uintptr_t minSize;

	dest = HeapAllocateAlignedEx( pHeap, size, alignment DBG_RELAY );
	if( source )
	{
		minSize = SizeOfMemBlock( source );
		if( size < minSize )
			minSize = size;
		MemCpy( dest, source, minSize );
		if( minSize < size )
			MemSet( ((uint8_t*)dest) + minSize, 0, size - minSize );
		ReleaseEx( source DBG_RELAY );
	}
	else
		MemSet( dest, 0, size );


	return dest;
}

//------------------------------------------------------------------------------------------------------

POINTER  HeapReallocateEx ( PMEM pHeap, POINTER source, uintptr_t size DBG_PASS )
{
	return HeapReallocateAlignedEx( pHeap, source, size, 0 DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

POINTER  HeapPreallocateAlignedEx ( PMEM pHeap, POINTER source, uintptr_t size, uint16_t alignment DBG_PASS )
{
	POINTER dest;
	uintptr_t minSize;

	dest = HeapAllocateAlignedEx( pHeap, size, alignment DBG_RELAY );
	if( source )
	{
		minSize = SizeOfMemBlock( source );
		if( size < minSize )
			minSize = size;
		MemCpy( (uint8_t*)dest + (size-minSize), source, minSize );
		if( minSize < size )
			MemSet( dest, 0, size - minSize );
		ReleaseEx( source DBG_RELAY );
	}
	else
		MemSet( dest, 0, size );
	return dest;
}

POINTER  HeapPreallocateEx ( PMEM pHeap, POINTER source, uintptr_t size DBG_PASS ){
	return HeapPreallocateAlignedEx( pHeap, source, size, AlignOfMemBlock(source) DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

 POINTER  HeapMoveEx( PMEM pNewHeap, POINTER source DBG_PASS )
{
	return HeapReallocateAlignedEx( pNewHeap, source, SizeOfMemBlock( source ), AlignOfMemBlock( source ) DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

 POINTER  ReallocateEx( POINTER source, uintptr_t size DBG_PASS )
{
	return HeapReallocateAlignedEx( g.pMemInstance, source, size, AlignOfMemBlock( source ) DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

 POINTER  PreallocateEx( POINTER source, uintptr_t size DBG_PASS )
{
	return HeapPreallocateAlignedEx( g.pMemInstance, source, size, AlignOfMemBlock( source ) DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

#if USE_CUSTOM_ALLOCER
static void Bubble( PMEM pMem )
{
	// handle sorting free memory to be least signficant first...
	PCHUNK temp, next;
	PCHUNK *prior;
	prior = &pMem->pFirstFree;
	temp = *prior;
	if( !temp )
		return;
	next = temp->next;
	while( temp && next )
	{
		if( (uintptr_t)next < (uintptr_t)temp )
		{
			UnlinkThing( temp );
			UnlinkThing( next );
			LinkThing( *prior, next );
			LinkThing( next->next, temp );
			prior = &next->next;
			temp = *prior;
			next = temp->next;
		}
		else
		{
			prior = &temp->next;
			temp = *prior;
#ifdef _DEBUG
			if( temp->next == temp )
			{
				ll_lprintf( "OOps this block is way bad... how'd that happen? %s(%d)", BLOCK_FILE( temp ), BLOCK_LINE( temp ) );
				DebugBreak();
			}
#endif
			next = temp->next;
		}
	}
}
#endif

//------------------------------------------------------------------------------------------------------

 uintptr_t  SizeOfMemBlock ( CPOINTER pData )
{
	if( pData )
	{
		if( USE_CUSTOM_ALLOCER )
		{
			PCHUNK pc = (PCHUNK)(((uintptr_t)pData) - (((uint16_t*)pData)[-1] + offsetof( CHUNK, byData )));
			return pc->dwSize - pc->info.dwPad;
		}
		else
		{
			PMALLOC_CHUNK pc = (PMALLOC_CHUNK)(((uintptr_t)pData) - MALLOC_CHUNK_SIZE(pData));
			return pc->dwSize - pc->info.dwPad;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------------------------------

uint16_t  AlignOfMemBlock( CPOINTER pData )
{
	if( pData )
	{
		return (((uint16_t*)pData)[-2]);
	}
	return 0;
 }

//------------------------------------------------------------------------------------------------------

 POINTER  MemDupEx ( CPOINTER thing DBG_PASS )
{
	uintptr_t size = SizeOfMemBlock( thing );
	POINTER result;
	result = HeapAllocateAlignedEx( g.pMemInstance, size, AlignOfMemBlock( thing ) DBG_RELAY );
	MemCpy( result, thing, size );
	return result;
}

#undef MemDup
 POINTER  MemDup (CPOINTER thing )
{
	return MemDupEx( thing DBG_SRC );
}
//------------------------------------------------------------------------------------------------------

POINTER ReleaseEx ( POINTER pData DBG_PASS )
{
	if( !g.bInit ) return NULL;
	if( pData )
	{
#ifndef __NO_MMAP__
		// how to figure if it's a CHUNK or a HEAP_CHUNK?
		if( !( ((uintptr_t)pData) & 0x3FF ) )
		{
			// system allocated blocks ( OpenSpace ) will be tracked as spaces...
			// and they will be aligned on large memory blocks (4096 probably)
			PSPACE ps = FindSpace( pData );
			if( ps )
			{
				DoCloseSpace( ps, TRUE );
				return NULL;
			}
		}
#endif
		if( !USE_CUSTOM_ALLOCER )
		{
			//PMEM pMem = (PMEM)(pData - offsetof( MEM, pRoot ));
			PMALLOC_CHUNK pc = (PMALLOC_CHUNK)(((uintptr_t)pData) - MALLOC_CHUNK_SIZE(pData) );
			pc->info.dwOwners--;
			if( !pc->info.dwOwners )
			{
				extern int  MemChk ( POINTER p, uintptr_t val, size_t sz );
#ifndef NO_LOGGING
#  ifdef _DEBUG
				if( g.bLogAllocate )
				{
					ll__lprintf(DBG_RELAY)( "Release %p(%p)", pc, pc->byData );
				}
#  endif
#endif
#ifdef ENABLE_NATIVE_MALLOC_PROTECTOR
				if( !MemChk( pc->LeadProtect, LEAD_PROTECT_TAG, sizeof( pc->LeadProtect ) ) ||
					!MemChk( pc->byData + pc->dwSize, LEAD_PROTECT_BLOCK_TAIL, sizeof( pc->LeadProtect ) ) )
				{
					ll_lprintf( "overflow block (%p) %p", pData, pc );
					DebugBreak();
				}
#endif

				free( pc );
				return NULL;
			}
			else
			{
#ifndef NO_LOGGING
				if( g.bLogAllocate && g.bLogAllocateWithHold )
				{
					ll__lprintf(DBG_RELAY)( "Release(holding) %p(%p)", pc, pc->byData );
				}
#endif
			}
			return pData;
		}
#if USE_CUSTOM_ALLOCER
		else
		{
			PCHUNK pc = (PCHUNK)(((uintptr_t)pData) - ( ( (uint16_t*)pData)[-1] +
													offsetof( CHUNK, byData ) ) );
			PMEM pMem, pCurMem;
			PSPACE pMemSpace;

			// Allow a simple release() to close a shared memory file mapping
			// this is a slight performance hit for all deallocations
#ifdef _DEBUG
			if( !g.bDisableAutoCheck )
				GetHeapMemStatsEx(pc->pRoot, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
#ifndef NO_LOGGING
#  ifdef _DEBUG
			if( g.bLogAllocate )
			{
				if( !g.bDisableDebug )
					_xlprintf( 2 DBG_RELAY )("Release  : %p(%p) - %" _PTRSZVALfs " bytes %s(%d)", pc->byData, pc, pc->dwSize, BLOCK_FILE( pc ), BLOCK_LINE( pc ));
				else
					_xlprintf( 2 DBG_RELAY )("Release  : %p(%p) - %" _PTRSZVALfs " bytes", pc->byData, pc, pc->dwSize);
			}
#  endif
#endif

			pMem = GrabMem( pc->pRoot );
#ifdef _DEBUG
			if( !pMem )
			{
#  ifndef NO_LOGGING
				ll__lprintf( DBG_RELAY )( "ERROR: Chunk to free does not reference a heap!" );
#  endif
				DebugDumpHeapMemEx( pc->pRoot, 1 );
				DebugBreak();
			}
#endif
			pMemSpace = FindSpace( pMem );

#ifdef _DEBUG
			while( pMemSpace && ( ( pCurMem = (PMEM)pMemSpace->pMem ),
										(	( (uintptr_t)pData < (uintptr_t)pCurMem )
										||  ( (uintptr_t)pData > ( (uintptr_t)pCurMem + pCurMem->dwSize ) ) )
									 )
				 )
			{
				Log( "ERROR: This block should have immediatly referenced it's correct heap!" );
				pMemSpace = pMemSpace->next;
			}
#endif

			if( !pMemSpace )
			{
#ifndef NO_LOGGING
#  ifdef _DEBUG
				ll__lprintf( DBG_RELAY )( "This Block is NOT within the managed heap! : %p", pData );
#  endif
#endif
				ll_lprintf( "this may not be an error.  This could be an old block from not using customallocer..." );
				DebugDumpHeapMemEx( pc->pRoot, 1 );
				DebugBreak();
				DropMem( pMem );
				return NULL;
			}
#ifdef _DEBUG
			pCurMem = (PMEM)pMemSpace->pMem;
#endif
			if( pData && pc )
			{
				if( !pc->info.dwOwners )
				{
#ifndef NO_LOGGING
#  ifdef _DEBUG
					if( !g.bDisableDebug &&
						!(pCurMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
						_xlprintf( 2
									, BLOCK_FILE(pc)
									, BLOCK_LINE(pc)
									)( "Block is already Free! %p "
									, pc );
					else
#  endif
						// CRITICAL ERROR!
						_xlprintf( 2 DBG_RELAY)( "Block is already Free! %p ", pc );
#endif
					DropMem( pMem );
					return pData;
				}
#ifdef _DEBUG
				if( !g.bDisableDebug )
					if( BLOCK_TAG( pc ) != BLOCK_TAG_ID )
					{
						ll_lprintf( "Application overflowed memory:%p", pc->byData );
						DebugDumpHeapMemEx( pc->pRoot, 1 );
						DebugBreak();
					}
#endif
				pc->info.dwOwners--;
				if( pc->info.dwOwners )
				{
#ifdef _DEBUG
					if( !(pCurMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
					{
						// store where it was released from
						BLOCK_FILE(pc) = pFile;
						BLOCK_LINE(pc) = nLine;
					}
#endif
					DropMem( pMem );
					return pData;
				}
				else
				{
					LOGICAL bCollapsed = FALSE;
					PCHUNK next, nextNext, pPrior;
					uintptr_t nNext;
					// fill memory with a known value...
					// this will allow me to check usage after release....

#ifdef _DEBUG
					if( !(pCurMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
					{
						// store where it was released from
						BLOCK_FILE(pc) = pFile;
						BLOCK_LINE(pc) = nLine;
					}
					if( !g.bDisableDebug )
					{
						BLOCK_TAG(pc)=BLOCK_TAG_ID;
						MemSet( pc->byData, FREE_MEMORY_TAG, pc->dwSize - pc->info.dwPad );
					}
#endif
					next = (PCHUNK)(pc->byData + pc->dwSize);
#ifndef _DEBUG
					pCurMem = (PMEM)pMemSpace->pMem;
#endif
					if( (nNext = (uintptr_t)next - (uintptr_t)pCurMem) >= pCurMem->dwSize )
					{
						// if next is NOT within valid memory...
						next = NULL;
					}

					if( ( pPrior = pc->pPrior ) ) // is not root chunk...
					{
						if( !pPrior->info.dwOwners ) // prior physical is free
						{
							pPrior->dwSize += CHUNK_SIZE + pc->dwSize; // add this header plus size
#ifdef _DEBUG
							//if( bLogAllocate )
							{
								//ll_lprintf( "Collapsing freed block with prior block...%p %p", pc, pPrior );
							}
							if( !g.bDisableDebug )
							{
								pPrior->info.dwPad = MAGIC_SIZE;
								if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
								{
									pPrior->info.dwPad += 2 * MAGIC_SIZE;
									BLOCK_FILE( pPrior ) = pFile;
									BLOCK_LINE( pPrior ) = nLine;
								}
								BLOCK_TAG( pPrior ) = BLOCK_TAG_ID;
								MemSet( pPrior->byData, FREE_MEMORY_TAG, pPrior->dwSize - pPrior->info.dwPad );
							}
							else
#endif
							{
								pPrior->info.dwPad = 0; // *** NEEDFILELINE ***
#ifdef _DEBUG
								if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
								{
									pPrior->info.dwPad += 2 * MAGIC_SIZE;
									BLOCK_FILE( pPrior ) = pFile;
									BLOCK_LINE( pPrior ) = nLine;
								}
#endif
							}
							if( pPrior->dwSize & 0x80000000 )
								DebugBreak();
							pc = pPrior; // use prior block as base ....
							if( next )
								next->pPrior = pPrior;
							bCollapsed = TRUE;
						}
					}
					// begin checking NEXT physical memory block for conglomerating
					if( next )
					{
						if( !next->info.dwOwners )
						{
							pc->dwSize += CHUNK_SIZE + next->dwSize;
							if( bCollapsed )
							{
								// pc is already in free list...
								UnlinkThing( next );
							}
							else
							{
								// otherwise need to use next's link spot
								// for this pc...
								if( (pc->next = next->next) )
									pc->next->me = &pc->next;
								pc->me = next->me;
								pc->me[0] = pc;
								bCollapsed = TRUE;
							}
#ifdef _DEBUG
							//if( bLogAllocate )
								//ll_lprintf( "Collapsing freed block with next block...%p %p", pc, next );
							if( !g.bDisableDebug )
							{
								pc->info.dwPad = MAGIC_SIZE;
								if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
								{
									pc->info.dwPad += 2 * MAGIC_SIZE;
									BLOCK_FILE( pc ) = pFile;
									BLOCK_LINE( pc ) = nLine;
								}
								BLOCK_TAG( pc ) = BLOCK_TAG_ID;
								MemSet( pc->byData, FREE_MEMORY_TAG, pc->dwSize - pc->info.dwPad );
							}
							else
#endif
							{
								pc->info.dwPad = 0;
#ifdef _DEBUG
								if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
								{
                            // *** NEEDFILELINE ***
									pc->info.dwPad += 2 * MAGIC_SIZE;
									BLOCK_FILE( pc ) = pFile;
									BLOCK_LINE( pc ) = nLine;
								}
#endif
							}

							if( pc->dwSize & 0x80000000 )
								DebugBreak();
							nextNext = (PCHUNK)(pc->byData + pc->dwSize );

							if( (((uintptr_t)nextNext) - ((uintptr_t)pCurMem)) < (uintptr_t)pCurMem->dwSize )
							{
								nextNext->pPrior = pc;
							}
						}
					}

					if( !bCollapsed ) // no block near this one was free...
					{
						LinkThing( pc->pRoot->pFirstFree, pc );
					}
				}
			}
			Bubble( pMem );
			DropMem( pMem );
#  ifdef _DEBUG
			if( !g.bDisableAutoCheck )
				GetHeapMemStatsEx(pMem, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#  endif
		}
#endif
	}
	return NULL;
}

		//------------------------------------------------------------------------------------------------------

 POINTER  HoldEx ( POINTER pData DBG_PASS )
{
	if( pData )
	{
		if( !USE_CUSTOM_ALLOCER )
		{
			PMALLOC_CHUNK pc = (PMALLOC_CHUNK)((uintptr_t)pData - MALLOC_CHUNK_SIZE(pData));
			//ll__lprintf( DBG_RELAY )( "holding block %p", pc );
#ifndef NO_LOGGING
			if( g.bLogAllocate && g.bLogAllocateWithHold )
				_xlprintf( 2 DBG_RELAY)( "Hold	 : %p - %" _PTRSZVALfs " bytes",pc, pc->dwSize );
#endif
			pc->info.dwOwners++;
		}
		else
		{
			PCHUNK pc = (PCHUNK)(((uintptr_t)pData) - ( ( (uint16_t*)pData)[-1] +
													offsetof( CHUNK, byData ) ) );
			PMEM pMem = GrabMem( pc->pRoot );

#ifndef NO_LOGGING
			if( g.bLogAllocate )
			{
				_xlprintf( 2 DBG_RELAY)( "Hold	 : %p - %" _PTRSZVALfs " bytes",pc, pc->dwSize );
			}
#endif
			if( !pc->info.dwOwners )
			{
				ll_lprintf( "Held block has already been released!  too late to hold it!" );
				DebugBreak();
				DropMem( pMem );
				return pData;
			}
			pc->info.dwOwners++;
			DropMem(pMem );
#ifdef _DEBUG
			if( !g.bDisableAutoCheck )
				GetHeapMemStatsEx(pc->pRoot, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
		}
	}
	return pData;
}

//------------------------------------------------------------------------------------------------------

 POINTER  GetFirstUsedBlock ( PMEM pHeap )
{
	return pHeap->pRoot[0].byData;
}

//------------------------------------------------------------------------------------------------------

void  DebugDumpHeapMemEx ( PMEM pHeap, LOGICAL bVerbose )
{
#if USE_CUSTOM_ALLOCER
	if( USE_CUSTOM_ALLOCER )
	{
		PCHUNK pc, _pc;
		uintptr_t nTotalFree = 0;
		uintptr_t nChunks = 0;
		uintptr_t nTotalUsed = 0;
		PSPACE pMemSpace;
		if( !pHeap ) pHeap = g.pMemInstance;
		PMEM pMem = GrabMem( pHeap ), pCurMem;

		pc = pMem->pRoot;

		ll_lprintf(" ------ Memory Dump ------- " );
		{
			xlprintf(LOG_ALWAYS)( "FirstFree : %p",
										pMem->pFirstFree );
		}

		for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->next )
		{
			pCurMem = (PMEM)pMemSpace->pMem;
			pc = pCurMem->pRoot; // current pChunk(pc)

			while( (((uintptr_t)pc) - ((uintptr_t)pCurMem)) < (uintptr_t)pCurMem->dwSize ) // while PC not off end of memory
			{
#ifndef __LINUX__
				Relinquish(); // allow debug log to work... (OutputDebugString() Win32, also network streams may require)
#endif
				nChunks++;
				if( !pc->info.dwOwners )
				{
					nTotalFree += pc->dwSize;
#ifndef NO_LOGGING
					if( bVerbose )
					{
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
						CTEXTSTR pFile =  !IsBadReadPtr( BLOCK_FILE(pc), 1 )
							?BLOCK_FILE(pc)
							:"Unknown";
						uint32_t nLine = BLOCK_LINE(pc);
#endif
						_xlprintf(LOG_ALWAYS DBG_RELAY)( "Free at %p size: %" _PTRSZVALfs "(%" _PTRSZVALfx ") Prior:%p NF:%p",
																 pc, pc->dwSize, pc->dwSize,
																 pc->pPrior,
																 pc->next );
					}
#endif
				}
				else
				{
					nTotalUsed += pc->dwSize;
#ifndef NO_LOGGING
					if( bVerbose )
					{
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
						CTEXTSTR pFile =  !IsBadReadPtr( BLOCK_FILE(pc), 1 )
							?BLOCK_FILE(pc)
							:"Unknown";
						uint32_t nLine = BLOCK_LINE(pc);
#endif
						_xlprintf(LOG_ALWAYS DBG_RELAY)( "Used at %p size: %" _PTRSZVALfs "(%" _PTRSZVALfx ") Prior:%p",
																 pc, pc->dwSize, pc->dwSize,
																 pc->pPrior );
					}
#endif
				}
				_pc = pc;
				pc = (PCHUNK)(pc->byData + pc->dwSize );
				if( pc == _pc )
				{
					ll_lprintf( "Next block is the current block..." );
					DebugBreak(); // broken memory chain
					break;
				}
			}
		}
		xlprintf(LOG_ALWAYS)( "Total Free: %" _PTRSZVALfs "  TotalUsed: %" _PTRSZVALfs "  TotalChunks: %" _PTRSZVALfs " TotalMemory:%" _PTRSZVALfs,
									nTotalFree, nTotalUsed, nChunks,
									(nTotalFree + nTotalUsed + nChunks * CHUNK_SIZE) );
		DropMem( pMem );
	}
	else
#endif
		xlprintf(LOG_ALWAYS)( "Cannot log chunks allocated that are not using custom allocer." );
}

	//------------------------------------------------------------------------------------------------------

 void  DebugDumpMemEx ( LOGICAL bVerbose )
{
	DebugDumpHeapMemEx( g.pMemInstance, bVerbose );
}

//------------------------------------------------------------------------------------------------------

 void  DebugDumpHeapMemFile ( PMEM pHeap, CTEXTSTR pFilename )
{
#if USE_CUSTOM_ALLOCER
	FILE *file;
#endif
	if( !USE_CUSTOM_ALLOCER )
		return;
#if USE_CUSTOM_ALLOCER

	Fopen( file, pFilename, "wt" );
	if( file )
	{
		PCHUNK pc, _pc;
		PMEM pMem, pCurMem;
		PSPACE pMemSpace;
		size_t nTotalFree = 0;
		size_t nChunks = 0;
		size_t nTotalUsed = 0;
		char byDebug[256];
		if( !pHeap ) pHeap = g.pMemInstance;
		pMem = GrabMem( pHeap );

		fprintf( file, " ------ Memory Dump ------- \n" );
		{
			char  byDebug[256];
			snprintf( byDebug, sizeof( byDebug ), "FirstFree : %p",
						pMem->pFirstFree );
			byDebug[255] = 0;
			fprintf( file, "%s\n", byDebug );
		}

		for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->next )
		{
			pCurMem = (PMEM)pMemSpace->pMem;
			pc = pCurMem->pRoot; // current pChunk(pc)

			while( (((uintptr_t)pc) - ((uintptr_t)pCurMem)) < (uintptr_t)pCurMem->dwSize ) // while PC not off end of memory
			{
				//Relinquish(); // allow debug log to work...
				nChunks++;
				if( !pc->info.dwOwners )
				{
					nTotalFree += pc->dwSize;
					snprintf( byDebug, sizeof(byDebug), "Free at %p size: %" cPTRSZVALfs "(%" cPTRSZVALfx ") Prior:%p NF:%p",
						pc, pc->dwSize, pc->dwSize,
						pc->pPrior,
						pc->next );
					byDebug[255] = 0;
				}
				else
				{
					nTotalUsed += pc->dwSize;
					snprintf( byDebug, sizeof(byDebug), "Used at %p size: %" cPTRSZVALfs "(%" cPTRSZVALfx ") Prior:%p",
						pc, pc->dwSize, pc->dwSize,
						pc->pPrior );
					byDebug[255] = 0;
				}
#ifdef _DEBUG
				if( !g.bDisableDebug && !(pCurMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
				{
					CTEXTSTR pFile =  !IsBadReadPtr( BLOCK_FILE(pc), 1 )
							?BLOCK_FILE(pc)
							:"Unknown";
					fprintf( file, "%s(%d):%s\n", pFile, BLOCK_LINE(pc), byDebug );
				}
				else
#endif
					fprintf( file, "%s\n", byDebug );
				_pc = pc;
				pc = (PCHUNK)(pc->byData + pc->dwSize );
				if( pc == _pc )
				{
					DebugBreak(); // broken memory chain
					break;
				}
			}
		}
		fprintf( file, "--------------- FREE MEMORY LIST --------------------\n" );

		for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->next )
		{
			pCurMem = (PMEM)pMemSpace->pMem;
			pc = pCurMem->pFirstFree; // current pChunk(pc)

			while( pc ) // while PC not off end of memory
			{
				snprintf( byDebug, sizeof(byDebug), "Free at %p size: %" c_size_fs "(%" c_size_fx ") ",
							pc, pc->dwSize, pc->dwSize );
				byDebug[255] = 0;

	#ifdef _DEBUG
				if( /*!g.bDisableDebug && */ !(pCurMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
				{
					const char * pFile =  !IsBadReadPtr( BLOCK_FILE(pc), 1 )
							?CStrDup(BLOCK_FILE(pc))
							:"Unknown";
					fprintf( file, "%s(%d):%s\n", pFile, BLOCK_LINE(pc), byDebug );
				}
				else
	#endif
					fprintf( file, "%s\n", byDebug );
				pc = pc->next;
			}
		}
		snprintf( byDebug, sizeof(byDebug), "Total Free: %" c_size_f "  TotalUsed: %" c_size_f "  TotalChunks: %" c_size_f " TotalMemory:%" c_size_f
					, nTotalFree, nTotalUsed, nChunks
					, (nTotalFree + nTotalUsed + nChunks * CHUNK_SIZE) );
		byDebug[255] = 0;
		fprintf( file, "%s\n", byDebug );
		//Relinquish();
		DropMem( pMem );

 		fclose( file );
	}
#endif
}

//------------------------------------------------------------------------------------------------------
 void  DebugDumpMemFile ( CTEXTSTR pFilename )
{
	DebugDumpHeapMemFile( g.pMemInstance, pFilename );
}
//------------------------------------------------------------------------------------------------------

 LOGICAL  Defragment ( POINTER *ppMemory ) // returns true/false, updates pointer
{
	// this is broken... needs
	// to fixup BLOCK_TAG, BLOCK_FILE, etc...
#if 1
	return FALSE;
#else
	// pass an array of allocated memory... for all memory blocks in list,
	// check to see if they can be reallocated lower, and or just moved to
	// a memory space lower than they are now.
	PCHUNK pc, pPrior;
	PMEM pMem;
	if( !ppMemory || !*ppMemory)
		return FALSE;
	pc = (PCHUNK)(((uintptr_t)(*ppMemory)) - (((uint16_t*)pData)[-1] + offsetof( CHUNK, byData )));
	pMem = GrabMem( pc->pRoot );

		// check if prior block is free... if so - then...
		// move this data down, and reallocate the freeness at the end
		// this reallocation may move the free next to another free, which
		// should be collapsed into this one...
	pPrior = pc->pPrior;
	if( ( pc->info.dwOwners == 1 ) && // not HELD by others... no way to update their pointers
		pPrior &&
		!pPrior->info.dwOwners )
	{
		CHUNK Free = *pPrior;
		CHUNK Allocated, *pNew;
		Allocated = *pc; // save this chunk...
		MemCpy( pPrior->byData, pc->byData, Allocated.dwSize );
		pNew = (PCHUNK)(pPrior->byData + Allocated.dwSize);
		pNew->dwSize = Free.dwSize;
		pNew->info.dwOwners = 0;
		pNew->pPrior = pPrior; // now pAllocated...
		pNew->pRoot = Free.pRoot;
		if( ( pNew->next = Free.next ) )
			pNew->next->me = &pNew->next;
		if( ( pNew->me = Free.me ) )
			(*pNew->me) = pNew;
#ifdef _DEBUG
		if( !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG ) )
		{
			BLOCK_FILE(pNew) = BLOCK_FILE(&Free);
			BLOCK_LINE(pNew) = BLOCK_LINE(&Free);
		}
#endif
		pPrior->dwSize = Allocated.dwSize;
		pPrior->info.dwOwners = 1;
		pPrior->next = NULL;
		pPrior->me = NULL;
		// update NEXT NEXT real block...
		{
			PCHUNK next;
			next = (PCHUNK)( pNew->byData + pNew->dwSize );

			if( (((uintptr_t)next) - ((uintptr_t)pMem)) < (uintptr_t)pMem->dwSize )
			{
				if( !next->info.dwOwners ) // if next is free.....
				{
					// consolidate...
					if( (pNew->next = next->next) )
						pNew->next->me = &pNew->next;
					pNew->me = next->me;
					pNew->me[0] = pNew;

					pNew->dwSize += next->dwSize + CHUNK_SIZE;
					next = (PCHUNK)( pNew->byData + pNew->dwSize );
					if( (uint32_t)(((char *)next) - ((char *)pMem)) < pMem->dwSize )
					{
						next->pPrior = pNew;
					}
				}
				else
					next->pPrior = pNew;
			}
		}
		*ppMemory = pPrior->byData;
		DropMem( pMem );
		GetHeapMemStats( g.pMemInstance, NULL, NULL, NULL, NULL );
		return TRUE;
	}

	DropMem( pMem );
	return FALSE;
#endif
}

//------------------------------------------------------------------------------------------------------

 void  GetHeapMemStatsEx ( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks DBG_PASS )
{
#if USE_CUSTOM_ALLOCER
	int nChunks = 0, nFreeChunks = 0, nSpaces = 0;
	uintptr_t nFree = 0, nUsed = 0;
	PCHUNK pc, _pc;
	PMEM pMem;
	PSPACE pMemSpace;
#endif
	if( !USE_CUSTOM_ALLOCER )
      return;
#if USE_CUSTOM_ALLOCER
	if( !pHeap )
		pHeap = g.pMemInstance;
	pMem = GrabMem( pHeap );
	pMemSpace = FindSpace( pMem );
	while( pMemSpace )
	{
		PMEM pMemCheck = ((PMEM)pMemSpace->pMem);
		pc = pMemCheck->pRoot;
		GrabMem( pMemCheck );
		while( (((uintptr_t)pc) - ((uintptr_t)pMemCheck)) < (uintptr_t)pMemCheck->dwSize ) // while PC not off end of memory
		{
			nChunks++;
			if( !pc->info.dwOwners )
			{
				nFree += pc->dwSize;
				nFreeChunks++;
			}
			else
			{
				nUsed += pc->dwSize;
#ifdef _DEBUG
				if( !g.bDisableDebug )
				{
					if( pc->dwSize > pMemCheck->dwSize )
					{
						ll_lprintf( "Memory block %p has a corrupt size.", pc->byData );
						DebugBreak();
					}
					else
					{
						int minPad = MAGIC_SIZE;
						if( pMem && !(pMem->dwFlags & HEAP_FLAG_NO_DEBUG) )
							minPad += MAGIC_SIZE * 2;
						if( ( pc->info.dwPad >= minPad ) && ( BLOCK_TAG(pc) != BLOCK_TAG_ID ) )
						{
#ifndef NO_LOGGING
							ll_lprintf( "memory block: %p(%p) %08" TAG_FORMAT_MODIFIER "x instead of %08" TAG_FORMAT_MODIFIER "x", pc, pc->byData, BLOCK_TAG(pc), BLOCK_TAG_ID );
							if( !(pMemCheck->dwFlags & HEAP_FLAG_NO_DEBUG ) )
							{
								CTEXTSTR file = BLOCK_FILE(pc);
#  ifdef _WIN32
								if( IsBadReadPtr( file, 4 ) )
									file = "(corrupt)";
#  endif
								_xlprintf( 2, file, BLOCK_LINE(pc) )( "Application overflowed allocated memory." );
							}
							else
								ODS( "Application overflowed allocated memory." );
#endif

							DebugDumpHeapMemEx( pHeap, 1 );
							DebugBreak();
						}
					}
				}
#endif
			}

			_pc = pc;
			pc = (PCHUNK)(pc->byData + pc->dwSize );
			if( (((uintptr_t)pc) - ((uintptr_t)pMemCheck)) < (uintptr_t)pMemCheck->dwSize  )
			{
				if( pc == _pc )
				{
					Log( "Current block is the same as the last block we checked!" );
					DebugDumpHeapMemEx( pHeap, 1 );
					DebugBreak(); // broken memory chain
					break;
				}
				if( pc->pPrior != _pc )
				{
					ll_lprintf( "Block's prior is not the last block we checked! prior %p sz: %" _PTRSZVALfs " current: %p currentprior: %p"
						, _pc
						, _pc->dwSize
						, pc
						, pc->pPrior );
					DebugDumpHeapMemEx( pHeap, 1 );
					DebugBreak();
					break;
				}
			}
		}
		_pc = NULL;
		pc = pMemCheck->pFirstFree;
		while( pc )
		{
			if( pc->info.dwOwners )
			{  // owned block is in free memory chain ! ?
				ll_lprintf( "Owned block %p is in free memory chain!", pc );
				DebugBreak();
				break;
			}
			_pc = pc;
			pc = pc->next;
		}
		nSpaces++;
		pMemSpace = pMemSpace->next;
		DropMem( pMemCheck );
	}
	DropMem( pMem );
	if( pFree )
		*pFree = (uint32_t)nFree;
	if( pUsed )
		*pUsed = (uint32_t)nUsed;
	if( pChunks )
		*pChunks = nChunks;
	if( pFreeChunks )
		*pFreeChunks = nFreeChunks;
#endif
}

//------------------------------------------------------------------------------------------------------

 void  GetMemStats ( uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks )
{
	GetHeapMemStats( g.pMemInstance, pFree, pUsed, pChunks, pFreeChunks );
}

//------------------------------------------------------------------------------------------------------
 int  SetAllocateLogging ( LOGICAL bTrueFalse )
{
	LOGICAL prior = g.bLogAllocate;
	g.bLogAllocate = bTrueFalse;
	return prior;
}

//------------------------------------------------------------------------------------------------------

 int  SetCriticalLogging ( LOGICAL bTrueFalse )
{
#ifdef _DEBUG
	int prior = g.bLogCritical;
	g.bLogCritical = bTrueFalse;
	return prior;
#else
	return 0;
#endif
}
//------------------------------------------------------------------------------------------------------

 int  SetAllocateDebug ( LOGICAL bDisable )
{
#ifdef _DEBUG
	int save = g.bDisableDebug;
	g.bDisableDebug = !bDisable;
	g.bDisableAutoCheck = !bDisable;
	return save;
#else
	return 1;
#endif
}

 int  SetManualAllocateCheck ( LOGICAL bDisable )
{
#ifdef _DEBUG
	int save = g.bDisableAutoCheck;
	g.bDisableAutoCheck = bDisable;
	return save;
#else
	return 1;
#endif
}

//------------------------------------------------------------------------------------------------------

 void  SetMinAllocate ( size_t nSize )
{
	g.nMinAllocateSize = nSize;
}

//------------------------------------------------------------------------------------------------------

 void  SetHeapUnit ( size_t dwSize )
{
	g.dwSystemCapacity = dwSize;
}

//------------------------------------------------------------------------------------------------------
#undef GetHeapMemStats
 void  GetHeapMemStats ( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks )
{
	GetHeapMemStatsEx( pHeap, pFree, pUsed, pChunks, pFreeChunks DBG_SRC );
}


#if 0
#  ifdef _MSC_VER
//>= 900

_CRT_ALLOC_HOOK prior_hook;

int allocHook(int allocType, void *userData, size_t size, int
blockType, long requestNumber, const unsigned char *filename, int
lineNumber)
{
	static int logging;
	if( logging )
		return TRUE;
	logging = 1;
	switch( allocType )
	{
	case _HOOK_ALLOC:
		ll_lprintf( "CRT Alloc: %d bytes %s(%d)"
			, size
			, filename, lineNumber
			);
		break;
	case _HOOK_REALLOC:
		ll_lprintf( "CRT Realloc: %d bytes %s(%d)"
			, size
			, filename, lineNumber
			);
		break;
	case _HOOK_FREE:
		ll_lprintf( ( "CRT Free: %p[%" _PTRSZVALfs "](%d) %s(%d)" )
			, userData
			, (uintptr_t)userData
			, size
			, filename, lineNumber
			);
		break;
	default:
		DebugBreak();
	}
	logging = 0;
	if( prior_hook )
		return prior_hook( allocType, userData, size, blockType, requestNumber, filename, lineNumber );
	return TRUE;
}

//int handle_program_memory_depletion( size_t )
//{
   // Your code
//}


PRELOAD( ShareMemToVSAllocHook )
{
	//_CRT_ALLOC_HOOK allocHook;
	//allocHook = 0 ;
	/* this is about useless... the free doesn't report the correct address
	* the allocate doesn't report the block
	* the free doesn't reprot the size
	* there is no way to relate what is freed with what is allocated
	*/
	//prior_hook = _CrtSetAllocHook(	allocHook );
	//_set_new_handler( pn );
}
#  endif
#endif

#ifdef __cplusplus

}//namespace sack {
}//	namespace memory {

#endif
