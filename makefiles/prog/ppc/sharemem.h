
#ifndef SHARED_MEM_DEFINED
#define SHARED_MEM_DEFINED

#include "./types.h"

#    ifdef MEM_LIBRARY_SOURCE
#      define MEM_PROC(type,name) type CPROC name
#    else
#      define MEM_PROC(type,name) extern type CPROC name
#    endif

#ifndef MEM_LIBRARY_SOURCE
typedef uintptr_t PMEM;
#endif

#ifdef __cplusplus
extern "C"  {
#endif

// what is an abstract name for the memory mapping handle...
// where is a filename for the filebacking of the shared memory
// DigSpace( WIDE("Picture Memory"), WIDE("Picture.mem"), 100000 );

// raw shared file view...
MEM_PROC( POINTER, OpenSpace )( TEXTSTR pWhat, TEXTSTR pWhere, uint32_t *dwSize );

// an option to specify a requested address would be MOST handy...
MEM_PROC( POINTER, OpenSpaceEx )( TEXTSTR pWhat, TEXTSTR pWhere, uint32_t address, uint32_t *dwSize );

MEM_PROC( void, CloseSpace )( POINTER pMem );
MEM_PROC( uint32_t, GetSpaceSize )( POINTER pMem );

// even if pMem is just a POINTER returned from OpenSpace
// this will create a valid heap pointer.
MEM_PROC( void, InitHeap)( PMEM pMem, uint32_t dwSize );

// not sure about this one - perhaps with custom heaps
// we DEFINATLY need to disallow auto-additions
//MEM_PROC( void, AddMemoryHeap )( POINTER pMem, LOGICAL bInit );


MEM_PROC( void, DebugDumpHeapMemEx )( PMEM pHeap, LOGICAL bVerbose );
#define DebugDumpHeapMem(h)     DebugDumpMemEx( (h), TRUE )

MEM_PROC( void, DebugDumpMemEx )( LOGICAL bVerbose );
#define DebugDumpMem()     DebugDumpMemEx( TRUE )
#define DumpMemory() DebugDumpMemEx( TRUE )
MEM_PROC( void, DebugDumpHeapMemFile )( PMEM pHeap, char *pFilename );

MEM_PROC( void, DebugDumpMemFile )( char *pFilename );


MEM_PROC( POINTER, HeapAllocateEx )( PMEM pHeap, uint32_t nSize DBG_PASS );
#define HeapAllocate(heap, n) HeapAllocateEx( (heap), (n) DBG_SRC )
MEM_PROC( POINTER, AllocateEx )( uint32_t nSize DBG_PASS );
#define Allocate( n ) HeapAllocateEx( 0, (n) DBG_SRC )
//MEM_PROC( POINTER, AllocateEx )( uint32_t nSize DBG_PASS );
//#define Allocate(n) AllocateEx(n DBG_SRC )
MEM_PROC( POINTER, GetFirstUsedBlock )( PMEM pHeap );

MEM_PROC( POINTER, ReleaseEx )( POINTER pData DBG_PASS ) ;
#define Release(p) ReleaseEx( (p) DBG_SRC )
#ifdef _DEBUG
# define ReleaseExx(p) do { ReleaseEx( *p ); } while(0)
#else
# define ReleaseExx(p) do { Release( *p ); } while(0)
#endif
MEM_PROC( POINTER, HoldEx )( POINTER pData DBG_PASS  );
#define Hold(p) HoldEx(p DBG_SRC )

MEM_PROC( POINTER, HeapReallocateEx )( PMEM pHeap, POINTER source, uint32_t size DBG_PASS );
#define HeapReallocate(heap,p,sz) HeapReallocateEx( (heap),(p),(sz) DBG_SRC )
MEM_PROC( POINTER, ReallocateEx )( POINTER source, uint32_t size DBG_PASS );
#define Reallocate(p,sz) ReallocateEx( (p),(sz) DBG_SRC )
MEM_PROC( POINTER, HeapMoveEx )( PMEM pNewHeap, POINTER source DBG_PASS );
#define HeapMove(h,s) HeapMoveEx( (h), (s) DBG_SRC )

MEM_PROC( uint32_t, SizeOfMemBlock )( CPOINTER pData );

MEM_PROC( LOGICAL, Defragment )( POINTER *ppMemory );

MEM_PROC( void, GetHeapMemStatsEx )( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks DBG_PASS );
#define GetHeapMemStats(h,f,u,c,fc) GetHeapMemStatsEx( h,f,u,c,fc DBG_SRC )
//MEM_PROC( void, GetHeapMemStats )( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks );
MEM_PROC( void, GetMemStats )( uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks );

MEM_PROC( void, SetAllocateLogging )( LOGICAL bTrueFalse );
MEM_PROC( void, SetAllocateDebug )( LOGICAL bDisable );
MEM_PROC( void, SetCriticalLogging )( LOGICAL bTrueFalse );
MEM_PROC( void, SetMinAllocate )( int nSize );
MEM_PROC( void, SetHeapUnit )( int dwSize );

MEM_PROC( uint32_t, LockedExchange )( uint32_t* p, uint32_t val );
MEM_PROC( uint32_t, LockedIncrement )( uint32_t* p );
MEM_PROC( uint32_t, LockedDecrement )( uint32_t* p );


MEM_PROC( void, MemSet )( POINTER p, uint32_t n, uint32_t sz );
MEM_PROC( void, MemCpy )( POINTER pTo, CPOINTER pFrom, uint32_t sz );
MEM_PROC( int, MemCmp )( CPOINTER pOne, CPOINTER pTwo, uint32_t sz );
MEM_PROC( POINTER, MemDupEx )( CPOINTER thing DBG_PASS );
#define MemDup(thing) MemDupEx(thing DBG_SRC )

MEM_PROC( TEXTSTR, StrDupEx )( CTEXTSTR original DBG_PASS );
#define StrDup(o) StrDupEx( (o) DBG_SRC )

//------------------------------------------------------------------------

#ifdef __cplusplus
}
# include <stddef.h>
# ifdef _DEBUG
inline void operator delete( void * p )
{ Release( p ); }
inline void operator delete (void * p DBG_PASS )
{ ReleaseEx( p DBG_RELAY ); }
//#define delete delete( DBG_VOIDSRC )
//#define deleteEx(file,line) delete(file,line)

inline void * operator new( size_t size DBG_PASS )
{ return AllocateEx( (uint32_t)size DBG_RELAY ); }
inline void * operator new[]( size_t size DBG_PASS )
{ return AllocateEx( (uint32_t)size DBG_RELAY ); }
#  define new new( DBG_VOIDSRC )
#  define newEx(file,line) new(file,line)
// common names - sometimes in conflict when declaring
// other functions... AND - release is a common 
// component of iComObject 
//#undef Allocate
//#undef Release

// Hmm wonder where this conflicted....
//#undef LineDuplicate


# else
inline void * operator new(size_t size)
{ return AllocateEx( size ); }
inline void operator delete (void * p)
{ ReleaseEx( p ); }
# endif

#endif


#endif
// $Log: sharemem.h,v $
// Revision 1.1  2003/08/12 14:54:37  panther
// Optimize watcom ppc - nhmalloc was hella slow
//
// Revision 1.24  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.23  2003/05/18 19:30:56  panther
// hmm use pretteir names for StrDup
//
// Revision 1.22  2003/05/13 09:12:37  panther
// Update types to be more protected for memlib
//
// Revision 1.21  2003/04/27 01:25:45  panther
// Don't disable allocate, release in header
//
// Revision 1.20  2003/04/24 00:03:49  panther
// Added ColorAverage to image... Fixed a couple macros
//
// Revision 1.19  2003/04/21 19:59:43  panther
// Option to return filename only
//
// Revision 1.18  2003/03/25 08:38:11  panther
// Add logging
//
