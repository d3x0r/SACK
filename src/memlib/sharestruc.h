#if !defined( MEMORY_STRUCT_DEFINED ) || defined( DEFINE_MEMORY_STRUCT )

#define ENABLE_NATIVE_MALLOC_PROTECTOR

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
#ifdef LOG_DEBUG_CRITICAL_SECTIONS
#define SECTION_LOGGED_WAIT 0x80000000
#define AND_NOT_SECTION_LOGGED_WAIT(n) ((n)&(~SECTION_LOGGED_WAIT))
#define AND_SECTION_LOGGED_WAIT(n) ((n)&(SECTION_LOGGED_WAIT))
#else
#define SECTION_LOGGED_WAIT 0
#define AND_NOT_SECTION_LOGGED_WAIT(n) (n)
#define AND_SECTION_LOGGED_WAIT(n) (0)
#endif
// If you change this structure please change the public
// reference of this structure, and please, do hand-count
// the bytes to set there... so not include this file
// to get the size.  The size there should be the worst
// case - debug or release mode.
#ifdef NO_PRIVATE_DEF
struct critical_section_tag {
	volatile uint32_t dwUpdating; // this is set when entering or leaving (updating)...
	volatile uint32_t dwLocks;
	THREAD_ID dwThreadID; // windows upper 16 is process ID, lower is thread ID
	THREAD_ID dwThreadWaiting; // ID of thread waiting for this..
	//PDATAQUEUE pPriorWaiters;
#ifdef DEBUG_CRITICAL_SECTIONS
	uint32_t bCollisions ;
	CTEXTSTR pFile;
	uint32_t  nLine;
#endif
};
typedef struct critical_section_tag CRITICALSECTION;
#endif
#ifdef __cplusplus
	}
}
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
	uint16_t dwOwners;   // if 0 - block is free
	uint16_t dwPad;      // extra bytes 4/12 typical, sometimes pad untill next. (alignment extra bytes)
#ifdef __64__
	uint32_t pad;
#endif
	uintptr_t dwSize;  // limited to allocating 4 billion bytes...
#ifdef ENABLE_NATIVE_MALLOC_PROTECTOR
	uint32_t LeadProtect[2];
#endif
	uint16_t alignment; // this is additional to subtract to get back to start (aligned allocate)
	uint16_t to_chunk_start; // this is additional to subtract to get back to start (aligned allocate)
	uint8_t byData[1]; // uint8_t is the smallest valid datatype could be _0
} PACKED;

PREFIX_PACKED struct heap_chunk_tag
{
	uint16_t dwOwners;            // if 0 - block is free
	uint16_t dwPad;   // extra bytes 4/12 typical, sometimes pad untill next.
	// which is < ( CHUNK_SIZE + nMinAllocate )
	// real size is then dwSize - dwPad.
	// this is actually where the end of block tag(s) should begin!
	uintptr_t dwSize;  // limited to allocating 4 billion bytes...
	struct heap_chunk_tag *pPrior;         // save some math backwards...
	struct memory_block_tag * pRoot;  // pointer to master allocation struct (pMEM)
	DeclareLink( struct heap_chunk_tag );
	uint16_t alignment; // this is additional to subtract to get back to start (aligned allocate)
	uint16_t to_chunk_start; // this is additional to subtract to get back to start (aligned allocate)
	uint8_t byData[1]; // uint8_t is the smallest valid datatype could be _0
} PACKED;

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
	uintptr_t dwSize;
	uint32_t dwHeapID; // unique value 0xbab1f1ea (baby flea);
	// lock between multiple processes/threads
	CRITICALSECTION cs;
	uint32_t dwFlags;
	PHEAP_CHUNK pFirstFree;
	HEAP_CHUNK pRoot[1];
};
typedef struct memory_block_tag MEM;

#ifdef __cplusplus
	}
}
#endif

#endif

