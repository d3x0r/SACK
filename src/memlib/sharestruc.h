#if !defined( MEMORY_STRUCT_DEFINED ) || defined( DEFINE_MEMORY_STRUCT )

#ifdef _DEBUG
#  define USE_DEBUG_LOGGING 1
#else
#  define USE_DEBUG_LOGGING 0
#endif

#define MEMORY_STRUCT_DEFINED
#include "sack_types.h"

#ifdef _DEBUG
#include <sharemem.h>
//  Define this symbol in SHAREMEM.H!
// if you define it here it will not work as expected...
//// defined in sharemem.h #define DEBUG_CRITICAL_SECTIONS
//// defined in sharemem.h #define LOG_DEBUG_CRITICAL_SECTIONS
#endif
#define _SHARED_MEMORY_LIBRARY

#ifdef __cplusplus
namespace sack {
	namespace timers {
#endif
// bit set on dwLocks when someone hit it and it was locked
#define SECTION_LOGGED_WAIT 0x80000000
// If you change this structure please change the public
// reference of this structure, and please, do hand-count
// the bytes to set there... so not include this file
// to get the size.  The size there should be the worst
// case - debug or release mode.
#ifdef NO_PRIVATE_DEF
struct critical_section_tag {
	volatile _32 dwUpdating; // this is set when entering or leaving (updating)...
   volatile _32 dwLocks;
	THREAD_ID dwThreadID; // windows upper 16 is process ID, lower is thread ID
   THREAD_ID dwThreadWaiting; // ID of thread waiting for this..
   //PDATAQUEUE pPriorWaiters;
#ifdef DEBUG_CRITICAL_SECTIONS
	_32 bCollisions ;
	CTEXTSTR pFile;
	_32  nLine;
#endif
};
typedef struct critical_section_tag CRITICALSECTION;
#endif
#ifdef __cplusplus
	};
};
#endif

#ifdef __cplusplus
namespace sack {
	namespace memory {
		using namespace sack::timers;
#endif
// pFile, nLine has been removed from this
// the references for this info are now
// stored at the end of the block
		// after the 0x12345678 tag.

#  ifdef _MSC_VER
#    pragma pack (push, 1)
#  endif
// custom allocer, use heap_chunk_tag

PREFIX_PACKED struct malloc_chunk_tag
{
	_32 dwOwners;            // if 0 - block is free
#ifdef __64__
	_32 pad;
#endif
	PTRSZVAL dwSize;  // limited to allocating 4 billion bytes...
#ifdef ENABLE_NATIVE_MALLOC_PROTECTOR
	_32 LeadProtect[2];
#endif
	_8 byData[1]; // _8 is the smallest valid datatype could be _0
} PACKED;

struct heap_chunk_tag
{
	_16 dwOwners;            // if 0 - block is free
	_16 dwPad;   // extra bytes 4/12 typical, sometimes pad untill next.
	// which is < ( CHUNK_SIZE + nMinAllocate )
	// real size is then dwSize - dwPad.
	// this is actually where the end of block tag(s) should begin!
	PTRSZVAL dwSize;  // limited to allocating 4 billion bytes...
	struct heap_chunk_tag *pPrior;         // save some math backwards...
	struct memory_block_tag * pRoot;  // pointer to master allocation struct (pMEM)
	DeclareLink( struct heap_chunk_tag );
	_8 byData[1]; // _8 is the smallest valid datatype could be _0
};

// a chunk of memory in a heap space, heaps are also tracked, so extents
// of that space are known, therefore one can identify a heap chunk
// from a non-heap (malloc?) chunk.
typedef PREFIX_PACKED struct heap_chunk_tag HEAP_CHUNK, *PHEAP_CHUNK;
// CHUNK and HEAP_CHUNK are the same.  They were not the same when using an
// ifdef to separate custom allocation from malloc allocation.  HeapAllocate
// could still be passed a heap before, and would be able to allocate from it.
typedef PREFIX_PACKED struct heap_chunk_tag CHUNK, *PCHUNK;
typedef PREFIX_PACKED struct malloc_chunk_tag MALLOC_CHUNK, *PMALLOC_CHUNK;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

// chunks allocated have no debug information.
#define HEAP_FLAG_NO_DEBUG 0x0001

struct memory_block_tag
{
	PTRSZVAL dwSize;
	_32 dwHeapID; // unique value 0xbab1f1ea (baby flea);
	// lock between multiple processes/threads
	CRITICALSECTION cs;
	_32 dwFlags;
	PHEAP_CHUNK pFirstFree;
	HEAP_CHUNK pRoot[1];
};
typedef struct memory_block_tag MEM;

#ifdef __cplusplus
	};
};
#endif

#endif

