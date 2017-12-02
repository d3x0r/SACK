
#ifndef SHARED_MEM_DEFINED
/* Multiple inclusion protection symbol. */
#define SHARED_MEM_DEFINED

#include <sack_types.h>

#if defined (_WIN32)
//#define USE_NATIVE_CRITICAL_SECTION
#endif

#if defined( _SHLWAPI_H ) || defined( _INC_SHLWAPI )
#undef StrChr
#undef StrCpy
#undef StrDup
#undef StrRChr
#undef StrStr
#endif

#ifdef __cplusplus
#define SACK_MEMORY_NAMESPACE SACK_NAMESPACE namespace memory {
#define SACK_MEMORY_NAMESPACE_END } SACK_NAMESPACE_END
#else
#define SACK_MEMORY_NAMESPACE
#define SACK_MEMORY_NAMESPACE_END
#endif

/* A declaration of the call type for memory library routines. */
#define MEM_API CPROC
#    ifdef MEM_LIBRARY_SOURCE
#      define MEM_PROC EXPORT_METHOD
#    else
/* Defines library linkage specification. */
#      define MEM_PROC IMPORT_METHOD
#    endif

#ifndef TIMER_NAMESPACE
#ifdef __cplusplus
#define _TIMER_NAMESPACE namespace timers {
/* define a timer library namespace in C++. */
#define TIMER_NAMESPACE SACK_NAMESPACE namespace timers {
/* define a timer library namespace in C++ end. */
#define TIMER_NAMESPACE_END } SACK_NAMESPACE_END
#else
#define TIMER_NAMESPACE 
#define TIMER_NAMESPACE_END
#endif
#endif

	TIMER_NAMESPACE
   // enables file/line monitoring of sections and a lot of debuglogging
//#define DEBUG_CRITICAL_SECTIONS
   /* this symbol controls the logging in timers.c... (higher level interface to NoWait primatives)*/
//#define LOG_DEBUG_CRITICAL_SECTIONS

/* A custom implementation of windows CRITICAL_SECTION api.
   Provides same capability for Linux type systems. Can be
   checked as a study in how to implement safe locks.
   
   
   
   
   See Also
   InitCriticalSec
   
   EnterCriticalSec
   
   LeaveCriticalSec
   
   
   Example
   <c>For purposes of this example this is declared in global
   memory, known to initialize to all 0.</c>
   <code lang="c++">
   CRITICALSECTION cs_lock_test;
   
   
   </code>
   
   In some bit of code that can be executed by several
   threads...
   <code lang="c++">
   {
      EnterCriticalSec( &amp;cs_lock_test );
      // the code in here will only be run by a single thread
      LeaveCriticalSec( &amp;cs_lock_test );
   }
   </code>
   
   Remarks
   The __Ex versions of functions passes source file and line
   information in debug mode. This can be used if critical
   section debugging is turned on, or if critical section
   logging is turned on. (See ... ) This allows applications to
   find deadlocks by tracking who is entering critical sections
   and probably failing to leave them.                          */
struct critical_section_tag {
	uint32_t dwUpdating; // this is set when entering or leaving (updating)...
	uint32_t dwLocks;  // count of locks entered.  (only low 24 bits may count for 16M entries, upper bits indicate internal statuses.
	THREAD_ID dwThreadID; // windows upper 16 is process ID, lower is thread ID
	THREAD_ID dwThreadWaiting; // ID of thread waiting for this..
#ifdef DEBUG_CRITICAL_SECTIONS
#define MAX_SECTION_LOG_QUEUE 16
	uint32_t bCollisions ;
	CTEXTSTR pFile[16];
	uint32_t  nLine[16];
	uint32_t  nLineCS[16];
	THREAD_ID dwThreadPrior[16]; // windows upper 16 is process ID, lower is thread ID
	uint8_t isLock[16]; // windows upper 16 is process ID, lower is thread ID
	int nPrior;

#endif
};

#if !defined( _WIN32 )
#undef USE_NATIVE_CRITICAL_SECTION
#endif

/* <combine sack::timers::critical_section_tag>
   
   \ \                                          */
#if defined( USE_NATIVE_CRITICAL_SECTION )
#define CRITICALSECTION CRITICAL_SECTION
#else
typedef struct critical_section_tag CRITICALSECTION;
#endif
/* <combine sack::timers::critical_section_tag>
   
   defines a pointer to a CRITICALSECTION type  */
#if defined( USE_NATIVE_CRITICAL_SECTION )
#define PCRITICALSECTION LPCRITICAL_SECTION
#else
#define InitializeCriticalSection InitializeCriticalSec
typedef struct critical_section_tag *PCRITICALSECTION;
#endif
/* attempts to enter the critical section, and does not block.
   Returns
   If it enters the return is 1, else the return is 0.
   Parameters
   pcs :    pointer to a critical section
   prior :  if not NULL, prior will be set to the current thread
            ID of the owning thread.                             */
#ifndef USE_NATIVE_CRITICAL_SECTION
MEM_PROC  int32_t MEM_API  EnterCriticalSecNoWaitEx ( PCRITICALSECTION pcs, THREAD_ID *prior DBG_PASS );
#define EnterCriticalSecNoWait( pcs,prior ) EnterCriticalSecNoWaitEx( pcs, prior DBG_SRC )
#else
#define EnterCriticalSecNoWait( pcs,prior ) TryEnterCriticalSection( (pcs) )
#endif

/* <combine sack::timers::EnterCriticalSecNoWaitEx@PCRITICALSECTION@THREAD_ID *prior>
   
   \ \                                                                                */
//#define EnterCriticalSecNoWait( pcs,prior ) EnterCriticalSecNoWaitEx( (pcs),(prior) DBG_SRC )
/* clears all members of a CRITICALSECTION.  Same as memset( pcs, 0, sizeof( CRITICALSECTION ) ); */
#ifndef USE_NATIVE_CRITICAL_SECTION
MEM_PROC  void MEM_API  InitializeCriticalSec ( PCRITICALSECTION pcs );
#else
#define InitializeCriticalSec(pcs)  InitializeCriticalSection(pcs)
#endif
/* Get a count of how many times a critical section is locked */
//MEM_PROC  uint32_t MEM_API  CriticalSecOwners ( PCRITICALSECTION pcs );

/* Namespace of all memory related functions for allocating and
   releasing memory.                                            */
#ifdef __cplusplus
}; // namespace timers
}; // namespace sack
using namespace sack::timers;
#endif

#ifdef __cplusplus
namespace sack {
/* Memory namespace contains functions for allocating and
   releasing memory. Also contains methods for accessing shared
   memory (if available on the target platform).
   
   
   
   Allocate
   
   Release
   
   Hold
   
   OpenSpace                                                    */
namespace memory {
#endif

typedef struct memory_block_tag* PMEM;

// what is an abstract name for the memory mapping handle...
// where is a filename for the filebacking of the shared memory
// DigSpace( WIDE(TEXT( "Picture Memory" )), WIDE(TEXT( "Picture.mem" )), 100000 );

/* <combinewith sack::memory::OpenSpaceExx@CTEXTSTR@CTEXTSTR@uintptr_t@uintptr_t *@uint32_t*>
   
   \ \                                                                                 */
MEM_PROC  POINTER MEM_API  OpenSpace ( CTEXTSTR pWhat, CTEXTSTR pWhere, uintptr_t *dwSize );

/* <unfinished>
   
   Open a shared memory region. The region may be named with a
   text string (this does not work under linux platforms, and
   the name of the file to back the shared region is the sharing
   point). The region may be backed with a file (and must be if
   it is to be shared on linux.
   
   If the region exists by name, the region is opened, and a
   pointer to that region is returned.
   
   If the file exists, the file is opened, and mapped into
   memory, and a pointer to the file backed memory is returned.
   
   if the file does not exist, and the size parameter passed is
   not 0, then the file is created, and expanded to the size
   requested. The bCreate flag is set to true.
   
   If NULL is passed for pWhat and pWhere, then a block of
   memory is allocated in system memory, backed by pagefile.
   
   if dwSize is 0, then the region is specified for open only,
   and will not create.
   Parameters
   pWhat :     String to a named shared memory region. NULL is
               unnamed.
   pWhere :    Filename to back the shared memory with. The file
               name itself may also be used to share the memory.
   address :   A base address to map the memory at. If 0,
               specifies do not care.
   dwSize :    pointer to a uintptr_t that defines the size to
               create. If 0, then the region is only opened. The
               size of the region opened is set back into this
               value after it is opened.
   bCreated :  pointer to a boolean to indicate whether the space
               was created or not.
   
   Returns
   Pointer to region requested to be opened. NULL on failure.
   Example
   Many examples of this are appropriate.
   
   1) Open or create a file backed shared space.
   
   2) Open a file for direct memory access, the file is loaded
   into memory by system paging routines and not any API.         */
MEM_PROC  POINTER MEM_API  OpenSpaceExx ( CTEXTSTR pWhat, CTEXTSTR pWhere, uintptr_t address
	, uintptr_t *dwSize, uint32_t* bCreated );
/* <combine sack::memory::OpenSpaceExx@CTEXTSTR@CTEXTSTR@uintptr_t@uintptr_t *@uint32_t*>
   
   \ \                                                                             */
#define OpenSpaceEx( what,where,address,psize) OpenSpaceExx( what,where,address,psize,NULL )

/* Closes a shared memory region. Calls CloseSpaceEx() with
   bFinal set TRUE.
   Parameters
   pMem :  pointer to a memory region opened by OpenSpace.  */
MEM_PROC  void MEM_API  CloseSpace ( POINTER pMem );
/* Closes a memory region. Release can also be used to close
   opened spaces.
   Parameters
   pMem :    pointer to a memory region opened with OpenSpace()
   bFinal :  If final is set, the file used for backing the shared
             region is deleted.                                    */
MEM_PROC  void MEM_API  CloseSpaceEx ( POINTER pMem, int bFinal );
/* This can give the size back of a memory space.
   Returns
   The size of the memory block.
   Parameters
   pMem :  pointer to a block of memory that was opened with
           OpenSpace().                                      */
MEM_PROC  uintptr_t MEM_API  GetSpaceSize ( POINTER pMem );

/* even if pMem is just a POINTER returned from OpenSpace this
   will create a valid heap pointer.
   
   will result TRUE if a valid heap is present will result FALSE
   if heap is not able to init (has content)
   Parameters
   pMem :    pointer to a memory space to setup as a heap.
   dwSize :  size of the memory space pointed at by pMem.        */
MEM_PROC  int MEM_API  InitHeap( PMEM pMem, uintptr_t dwSize );


/* Dumps all blocks into the log.
   Parameters
   pHeap :     Heap to dump. If NULL or unspecified, dump the
               default heap.
   bVerbose :  Specify to dump each block's information,
               otherwise only summary information is generated. */
MEM_PROC  void MEM_API  DebugDumpHeapMemEx ( PMEM pHeap, LOGICAL bVerbose );
/* <combine sack::memory::DebugDumpHeapMemEx@PMEM@LOGICAL>
   
   Logs all of the blocks tracked in a specific heap.
   Parameters
   Heap :  Heap to dump the memory blocks of.              */
#define DebugDumpHeapMem(h)     DebugDumpMemEx( (h), TRUE )

/* <combine sack::memory::DebugDumpHeapMemEx@PMEM@LOGICAL>
   
   \ \                                                     */
MEM_PROC  void MEM_API  DebugDumpMemEx ( LOGICAL bVerbose );
/* Dumps all tracked heaps.
   Parameters
   None.                    */
#define DebugDumpMem()     DebugDumpMemEx( TRUE )
/* Dumps a heap to a specific file.
   Parameters
   pHeap :      Heap. If NULL or unspecified, dumps default heap.
   pFilename :  name of the file to write output to.              */
MEM_PROC  void MEM_API  DebugDumpHeapMemFile ( PMEM pHeap, CTEXTSTR pFilename );
/* <combine sack::memory::DebugDumpHeapMemFile@PMEM@CTEXTSTR>
   
   \ \                                                        */
MEM_PROC  void MEM_API  DebugDumpMemFile ( CTEXTSTR pFilename );

#ifdef __GNUC__
MEM_PROC  POINTER MEM_API  HeapAllocateAlignedEx ( PMEM pHeap, uintptr_t dwSize, uint16_t alignment DBG_PASS ) __attribute__( (malloc) );
MEM_PROC  POINTER MEM_API  HeapAllocateEx ( PMEM pHeap, uintptr_t nSize DBG_PASS ) __attribute__((malloc));
MEM_PROC  POINTER MEM_API  AllocateEx ( uintptr_t nSize DBG_PASS ) __attribute__((malloc));
#else

/* \ \ 
   Parameters
   pHeap :  pointer to a heap which was initialized with
            InitHeap()
   Size :   Size of block to allocate                    */
MEM_PROC  POINTER MEM_API  HeapAllocateEx ( PMEM pHeap, uintptr_t nSize DBG_PASS );
/* \ \
Parameters
pHeap :  pointer to a heap which was initialized with
InitHeap()
Size :   Size of block to allocate                    
Alignment : count of bytes to return block on (1,2,4,8,16,32)  */
MEM_PROC  POINTER MEM_API  HeapAllocateAlignedEx( PMEM pHeap, uintptr_t nSize, uint16_t alignment DBG_PASS );
/* Allocates a block of memory of specific size. Debugging
   information if passed is recorded on the block.
   Parameters
   size :  size of the memory block to create              */
MEM_PROC  POINTER MEM_API  AllocateEx ( uintptr_t nSize DBG_PASS );
#endif

/* A simple macro to allocate a new single unit of a structure. Adds
   a typecast automatically to be (type*) so C++ compilation is
   clean. Does not burden the user with extra typecasts. This,
   being in definition use means that all other things that are
   typecast are potentially error prone. Memory is considered
   uninitialized.
   Parameters
   type :  type to allocate
   
   Example
   <code lang="c++">
   int *p_int = New( int );
   </code>                                                           */
#define New(type) ((type*)HeapAllocate(0,sizeof(type)))
/* Reallocates an array of type.
   Parameters
   type :  type to use for sizeof(type) * sz for resulting size.
   p :     pointer to realloc
   sz :    count of elements in the array                        */
#define Renew(type,p,sz) ((type*)HeapReallocate(0,p, sizeof(type)*sz))
/* an advantage of C, can define extra space at end of structure
   which is allowed to carry extra data, which is unknown by
   other code room for exploits rock.
   Parameters
   type :   passed to sizeof()
   extra :  Number of additional bytes to allocate beyond the
            sizeof( type )
   
   Example
   Create a text segment plus 18 characters of data. (This
   should not be done, use SegCreate instead)
   <code lang="c#">
   PTEXT text = NewPlus( TEXT, 18 ); 
   </code>                                                       */
#define NewPlus(type,extra) ((type*)HeapAllocate(0,sizeof(type)+(extra)))
/* Allocate a new array of type.
   Parameters
   type :   type to determine size of array element to allocate.
   count :  count of elements to allocate in the array.
   
   Returns
   A pointer to type. (this is important, since in C++ it's cast
   correctly to the destination type).                           */
#define NewArray(type,count) ((type*)HeapAllocate(0,sizeof(type)*(count)))
/* Allocate sizeof(type). Will invoke some sort of registered
   initializer
   Parameters
   type :  type to allocate for. Passes the name of the type so
           the allocator can do a registered procedure lookup and
           invok an initializer for the type.                     */
//#define NewObject(type) ((type*)FancyAllocate(sizeof(type),#type DBG_SRC))
#ifdef __cplusplus
/* A 'safe' release macro. casts the block to the type to
   release. Makes sure the pointer being released is the type
   specified.
   Parameters
   type :   type of the variable
   thing :  the thing to actually release.                    */
#define Deallocate(type,thing) for(type _zzqz_tmp=thing;ReleaseEx((POINTER)(_zzqz_tmp)DBG_SRC),0;)
#else
#define Deallocate(type,thing) (ReleaseEx((POINTER)(thing)DBG_SRC))
#endif
/* <combine sack::memory::HeapAllocateEx@PMEM@uintptr_t nSize>
   
   \ \                                                        */
#define HeapAllocate(heap, n) HeapAllocateEx( (heap), (n) DBG_SRC )
   /* <combine sack::memory::HeapAllocateAlignedEx@PMEM@uintptr_t@uint32_t>

   \ \                                                        */
#define HeapAllocateAligned(heap, n, m) HeapAllocateAlignedEx( (heap), (n), m DBG_SRC )
   /* <combine sack::memory::AllocateEx@uintptr_t nSize>
   
   \ \                                               */
#ifdef FIX_RELEASE_COM_COLLISION
#else
#define Allocate( n ) HeapAllocateEx( (PMEM)0, (n) DBG_SRC )
#endif
//MEM_PROC  POINTER MEM_API  AllocateEx ( uintptr_t nSize DBG_PASS );
//#define Allocate(n) AllocateEx(n DBG_SRC )
MEM_PROC  POINTER MEM_API  GetFirstUsedBlock ( PMEM pHeap );

/* Releases an allocated block. Memory becomes free to allocate
   again. If debugging information is passed, the releasing
   source and line is recorded in the block. (can be used to
   find code deallocating memory it shouldn't).
   
   This also works with Hold(), and decrements the hold counter.
   If there are no more holds on the block, then the block is
   released.
   Parameters
   p :  pointer to allocated block to release.                   */
MEM_PROC  POINTER MEM_API  ReleaseEx ( POINTER pData DBG_PASS ) ;
/* <combine sack::memory::ReleaseEx@POINTER pData>
   
   \ \                                             */
#ifdef FIX_RELEASE_COM_COLLISION
#else
/* <combine sack::memory::ReleaseEx@POINTER pData>
   
   \ \                                             */
#define Release(p) ReleaseEx( (p) DBG_SRC )
#endif

/* Adds a usage count to a block of memory. For each count
   added, an additional release must be used. This can be used
   to keep a copy of the block, even if some other code
   automatically releases it.
   Parameters
   pointer :  pointer to a block of memory that was Allocate()'d.
   
   Example
   Allocate a block of memory, and release it properly. But we
   passed it to some function. That function wanted to keep a
   copy of the block, so it can apply a hold. It needs to later
   do a Release again to actually free the memory.
   <code lang="c++">
   POINTER p = Allocate( 32 );
   
   call_some_function( p );
   
   Release( p );
   
   
   void call_some_function( POINTER p )
   {
      static POINTER my_p_copy;
      my_p_copy = p;
      Hold( p );
   }
   </code>                                                        */
MEM_PROC  POINTER MEM_API  HoldEx ( POINTER pData DBG_PASS  );
/* <combine sack::memory::HoldEx@POINTER pData>
   
   \ \                                          */
#define Hold(p) HoldEx(p DBG_SRC )

/* This can be used to add additional space after the end of a
   memory block.
   Parameters
   pHeap :   If NULL or not specified, uses the common memory heap.
   source :  pointer to the block to pre\-allocate. If NULL, a new
             memory block will be allocated that is filled with 0.
   size :    the new size of the block.
   
   Returns
   A pointer to a new block of memory that is the new size.
   Remarks
   If the size specified for the new block is larger than the
   previous size of the block, the curernt data is copied to the
   beginning of the new block, and the memory after the existing
   content is cleared to 0.
   
   If the size specified for the new block is smaller than the
   previous size, the end of the original block is not copied to
   the new block.
   
   If NULL is passed as the source block, then a new block
   filled with 0 is created.                                        */
MEM_PROC  POINTER MEM_API  HeapReallocateAlignedEx ( PMEM pHeap, POINTER source, uintptr_t size, uint16_t alignment DBG_PASS );
MEM_PROC  POINTER MEM_API  HeapReallocateEx ( PMEM pHeap, POINTER source, uintptr_t size DBG_PASS );
/* <combine sack::memory::HeapReallocateEx@PMEM@POINTER@uintptr_t size>
   
   \ \                                                                 */
#define HeapReallocateAligned(heap,p,sz,al) HeapReallocateEx( (heap),(p),(sz),(al) DBG_SRC )
#define HeapReallocate(heap,p,sz) HeapReallocateEx( (heap),(p),(sz) DBG_SRC )
/* <combine sack::memory::HeapReallocateEx@PMEM@POINTER@uintptr_t size>
   
   \ \                                                                 */
MEM_PROC  POINTER MEM_API  ReallocateEx ( POINTER source, uintptr_t size DBG_PASS );
/* <combine sack::memory::ReallocateEx@POINTER@uintptr_t size>
   
   \ \                                                        */
#ifdef FIX_RELEASE_COM_COLLISION
#else
#define Reallocate(p,sz) ReallocateEx( (p),(sz) DBG_SRC )
#endif
/* This can be used to add additional space before the beginning
   of a memory block.
   Parameters
   pHeap :   If NULL or not specified, uses the common memory heap.
   source :  pointer to the block to pre\-allocate. If NULL, a new
             memory block will be allocated that is filled with 0.
   size :    the new size of the block.
   Returns
   A pointer to a new block of memory that is the new size.
   Remarks
   If the size specified for the new block is larger than the
   previous size of the block, the content data is copied to the
   end of the new block, and the memory leading up to the block
   is cleared to 0.
   
   If the size specified for the new block is smaller than the
   previous size, the end of the original block is not copied to
   the new block.
   
   If NULL is passed as the source block, then a new block
   filled with 0 is created.                                        */
MEM_PROC  POINTER MEM_API  HeapPreallocateEx ( PMEM pHeap, POINTER source, uintptr_t size DBG_PASS );
/* <combine sack::memory::HeapPreallocateEx@PMEM@POINTER@uintptr_t size>
   
   \ \                                                                  */
#define HeapPreallocate(heap,p,sz) HeapPreallocateEx( (heap),(p),(sz) DBG_SRC )
/* <combine sack::memory::HeapPreallocateEx@PMEM@POINTER@uintptr_t size>
   
   \ \                                                                  */
MEM_PROC  POINTER MEM_API  PreallocateAlignedEx ( POINTER source, uintptr_t size, uint16_t alignment DBG_PASS );
MEM_PROC  POINTER MEM_API  PreallocateEx ( POINTER source, uintptr_t size DBG_PASS );
/* <combine sack::memory::PreallocateEx@POINTER@uintptr_t size>
   
   \ \                                                         */
#define PreallocateAligned(p,sz,al) PreallocateAlignedEx( (p),(sz),(al) DBG_SRC )
#define Preallocate(p,sz) PreallocateEx( (p),(sz) DBG_SRC )

/* Moves a block of memory from one heap to another.
   Parameters
   pNewHeap :  heap target to move the block to.
   source :    source block to move \- pointer to the data in the
               block.
   
   Remarks
   Since each block remembers its own size, it is possible to
   move a block from one heap to another. A heap might be a
   memory mapped file at a specific address for instance.         */
MEM_PROC  POINTER MEM_API  HeapMoveEx ( PMEM pNewHeap, POINTER source DBG_PASS );
/* <combine sack::memory::HeapMoveEx@PMEM@POINTER source>
   
   \ \                                                    */
#define HeapMove(h,s) HeapMoveEx( (h), (s) DBG_SRC )

/* \returns the size of a memory block which was Allocate()'d.
   Parameters
   pData :  pointer to a allocated memory block.
   
   Returns
   The size of the block that was specified by the Allocate(). */
MEM_PROC uintptr_t MEM_API  SizeOfMemBlock ( CPOINTER pData );

/* \returns the allocation alignment of a memory block which was Allocate()'d.
Parameters
pData :  pointer to a allocated memory block.

Returns
The alignment of the block that was specified from Allocate(). */
MEM_PROC uint16_t  AlignOfMemBlock( CPOINTER pData );

/* not so much of a fragment as a consolidation. Finds a free
   spot earlier in the heap and attempts to move the block
   there. This can help alleviate heap fragmentation.
   Parameters
   ppMemory :  pointer to a pointer to memory which might move */
MEM_PROC  LOGICAL MEM_API  Defragment ( POINTER *ppMemory );

/* \ \ 
   Parameters
   pHeap :        pointer to a heap
   pFree :        pointer to a 32 bit value to receive the size
                  of free space
   pUsed :        pointer to a 32 bit value to receive the size
                  of used space
   pChunks :      pointer to a 32 bit value to receive the total
                  count of chunks.
   pFreeChunks :  pointer to a 32 bit value to receive the total
                  count of free chunks.
   
   Remarks
   It looks like DBG_PASS parameter isn't used... not sure why
   it would here, there is no allocate or delete.
   
   The count of allocated chunks can be gotten by subtracting
   FreeChunks from Chunks.
   Example
   <code lang="c++">
   uint32_t free;
   uint32_t used;
   uint32_t chunks;
   uint32_t free_chunks;
   GetHeapMemStatsEx( NULL, &amp;free, &amp;used, &amp;chunks, &amp;free_chunks );
   </code>                                                                         */
MEM_PROC  void MEM_API  GetHeapMemStatsEx ( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks DBG_PASS );
/* <combine sack::memory::GetHeapMemStatsEx@PMEM@uint32_t *@uint32_t *@uint32_t *@uint32_t *pFreeChunks>
   
   \ \                                                                               */
#define GetHeapMemStats(h,f,u,c,fc) GetHeapMemStatsEx( h,f,u,c,fc DBG_SRC )
//MEM_PROC  void MEM_API  GetHeapMemStats ( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks );
MEM_PROC  void MEM_API  GetMemStats ( uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks );

/* Sets whether to log allocations or not.
   
   \returns the prior state of logging...
   Parameters
   bTrueFalse :  if TRUE, allocation logging is turned on. Enables
                 logging when each block is Allocated, Released,
                 or Held.                                          */
MEM_PROC  int MEM_API  SetAllocateLogging ( LOGICAL bTrueFalse );
/* disables storing file/line, also disables auto GetMemStats
   checking
   Parameters
   bDisable :  set to TRUE to disable allocate debug logging. */
MEM_PROC  int MEM_API  SetAllocateDebug ( LOGICAL bDisable );
/* disables auto GemMemStats on every allocate/release/Hold
   
   GetMemStats will evaluate each and every block allocated in
   memory and inspect it for corruption.
   Parameters
   bDisable :  set to TRUE to disable auto mem check.          */
MEM_PROC  int MEM_API  SetManualAllocateCheck ( LOGICAL bDisable );
/* Sets whether to log critical sections or not.
   
   \returns the prior state of logging...
   Parameters
   bTrueFalse :  if TRUE, critical section logging is turned on. Logs
                 when each thread enters or leaves a
                 CRITICIALSECTION.                                    */
MEM_PROC  int MEM_API  SetCriticalLogging ( LOGICAL bTrueFalse );
/* Sets the minimum size to allocate. If a block size less than
   this is allocated, then this much is actually allocated.
   Parameters
   nSize :  Specify the minimum allocation size                 */
MEM_PROC  void MEM_API  SetMinAllocate ( size_t nSize );
/* Sets how much a heap is expanded by when it is out of space. Default
   is like 512k.
   Parameters
   dwSize :  the new size to expand heaps by.
   
   Remarks
   Probably internally, this is rounded up to the next 4k
   boundary.                                                            */
MEM_PROC  void MEM_API  SetHeapUnit ( size_t dwSize );


/* Multi-processor safe exchange operation. Returns the prior
   value at the pointer.
   Parameters
   p :    pointer to a volatile 64 bit value.
   val :  a new 64 bit value to put at (*p)
   
   Example
   <code lang="c#">
   uint64_t value = 13;
   uint64_t oldvalue = LockedExchange64( &amp;value, 15 );
   // old value will be 13
   // value will be 15
   </code>                                                    */
MEM_PROC  uint64_t MEM_API  LockedExchange64 ( volatile uint64_t* p, uint64_t val );
/* A multi-processor safe increment of a variable.
   Parameters
   p :  pointer to a 32 bit value to increment.    */
MEM_PROC  uint32_t MEM_API  LockedIncrement ( uint32_t* p );
/* Does a multi-processor safe decrement on a variable.
   Parameters
   p :  pointer to a 32 bit value to decrement.         */
MEM_PROC  uint32_t MEM_API  LockedDecrement ( uint32_t* p );

#ifdef __cplusplus
// like also __if_assembly__
//extern "C" {
#endif

#ifdef __64__
#define LockedExchangePtrSzVal(a,b) LockedExchange64((volatile uint64_t*)(a),b)
#else
#define LockedExchangePtrSzVal(a,b) LockedExchange((volatile uint32_t*)(a),b)
#endif

/* Multiprocessor safe swap of the contents of a variable with a
   new value, and result with the old variable.
   Parameters
   p :    pointer to a 32 bit value to exchange
   val :  value to set into the variable
   Returns
   The prior value in p.
   Example
   <code>
   uint32_t variable = 0;
   uint32_t oldvalue = LockedExchange( &amp;variable, 1 );
   </code>                                                       */
MEM_PROC  uint32_t MEM_API  LockedExchange ( volatile uint32_t* p, uint32_t val );
/* Sets a 32 bit value into memory. If the length to set is not
   a whole number of 32 bit words, the last bytes may contain
   the low 16 bits of the value and the low 8 bits.
   
   
   Parameters
   p :   pointer to memory to set
   n :   32 bit value to set memory with
   sz :  length to set
   
   Remarks
   Writes as many 32 it values as will fit in sz.
   
   If (sz &amp; 2), the low 16 bits of n are written at the end.
   
   then if ( sz &amp; 1 ) the low 8 bits of n are written at the
   end.                                                          */
MEM_PROC  void MEM_API  MemSet ( POINTER p, uintptr_t n, size_t sz );
//#define _memset_ MemSet
/* memory copy operation. not safe when buffers overlap. Performs
   platform-native memory stream operation to copy from one
   place in memory to another. (32 or 64 bit operations as
   possible).
   Parameters
   pTo :    Memory to copy to
   pFrom :  memory to copy from
   sz :     size of block of memory to copy                       */
MEM_PROC  void MEM_API  MemCpy ( POINTER pTo, CPOINTER pFrom, size_t sz );
//#define _memcpy_ MemCpy
/* Binary byte comparison of one block of memory to another. Results
   \-1 if less, 1 if more and 0 if equal.
   Parameters
   pOne :  pointer to memory one
   pTwo :  pointer to some other memory
   sz :    count of bytes to compare
   
   Returns
   0 if equal
   
   \-1 if the first different byte in pOne is less than pTwo.
   
   1 if the first different byte in pOne is more than pTwo.          */
MEM_PROC  int MEM_API  MemCmp ( CPOINTER pOne, CPOINTER pTwo, size_t sz );

	/* nothing.
   
   does nothing, returns nothing. */
//#define memnop(mem,sz,comment)

#ifdef __cplusplus
//};
#endif

/* Compares two strings. Must match exactly.
   Parameters
   s1 :  string to compare
   s2 :  string to compare
   Returns
   0 if equal.
   
   1 if (s1 \>s2)
   
   \-1 if (s1 \< s2)
   
   if s1 is NULL and s2 is not NULL, return is -1.
   
   if s2 is NULL and s1 is not NULL, return is 1.
   
	if s1 and s2 are NULL return is 0.              */
#ifdef StrCmp
#undef StrCmp
#endif // StrCmp
MEM_PROC  int MEM_API  StrCmp ( CTEXTSTR pOne, CTEXTSTR pTwo );
/* Compares two strings, case insensitively.
   Parameters
   s1 :  string to compare
   s2 :  string to compare
   
   Returns
   0 if equal.
   
   1 if (s1 \>s2)
   
   \-1 if (s1 \< s2)
   
   if s1 is NULL and s2 is not NULL, return is -1.
   
   if s2 is NULL and s1 is not NULL, return is 1.
   
   if s1 and s2 are NULL return is 0.              */
MEM_PROC  int MEM_API  StrCaseCmp ( CTEXTSTR s1, CTEXTSTR s2 );
/* String insensitive case comparison with maximum length
   specified.
   Parameters
   s1 :      string to compare
   s2 :      string to compare
   maxlen :  maximum character required to match
   Returns
   0 if equal up to the number of characters.
   
   1 if (s1 \>s2)
   
   \-1 if (s1 \< s2)
   
   if s1 is NULL and s2 is not NULL, return is -1.
   
   if s2 is NULL and s1 is not NULL, return is 1.
   
   if s1 and s2 are NULL return is 0.                     */
MEM_PROC  int MEM_API  StrCaseCmpEx ( CTEXTSTR s1, CTEXTSTR s2, size_t maxlen );
/* This searches a string for the first character that matches
   some specified character.
   
   A custom strchr function, since microsoft is saying this is
   an unsafe function. This Compiles to compare native strings,
   if UNICODE uses unicode, otherwise uses 8 bit characters.
   Parameters
   s1 :  String to search
   c :   Character to find
   Returns
   pointer in string to search that is the first character that
   matches. NULL if no character matches.
   Note
   This flavor is the only one on C where operator overloading
   cannot switch between CTEXTSTR and TEXTSTR parameters, to
   \result with the correct type. If a CTEXTSTR is passed to
   this it should result with a CTEXTSTR, but if that's the only
   choice, then the result of this is never modifiable, even if
	it is a pointer to a non-const TEXTSTR.                       */
MEM_PROC  CTEXTSTR MEM_API  StrChr ( CTEXTSTR s1, TEXTCHAR c );
/* copy S2 to S1, with a maximum of N characters.
   
   The last byte of S1 will always be a 'nul'. If S2 was longer
   than S1, then it will be truncated to fit within S1. Perferred
   method over this is SaveText or StrDup.
   Parameters
   s1 :      desitnation TEXTCHAR buffer
   s2 :      source string
   length :  the maximum number of characters that S1 can hold. (this
             is not a size, but is a character count)                 */
MEM_PROC  TEXTSTR MEM_API  StrCpyEx ( TEXTSTR s1, CTEXTSTR s2, size_t n );
/* copy S2 to S1. This is 'unsafe', since neither paramter's
   size is known. Prefer StrCpyEx which passes the maximum
   length for S1.
   Parameters
   s1 :  desitnation TEXTCHAR buffer
   s2 :  source string                                       */
MEM_PROC  TEXTSTR MEM_API  StrCpy ( TEXTSTR s1, CTEXTSTR s2 );
/* \Returns the count of characters in a string.
   Parameters
   s :  string to measure
   
   Returns
   length of string.                             */
MEM_PROC  size_t MEM_API  StrLen ( CTEXTSTR s );
/* Get the length of a string in C chars.
   Parameters
   s :  char * to count.
   
   Returns
   the length of s. If s is NULL, return 0. */
MEM_PROC  size_t MEM_API  CStrLen ( char const*const s );


/* Finds the last instance of a character in a string.
   Parameters
   s1 :  String to search in
   c :   character to find
   
   Returns
   NULL if character is not in the string.
   
   a pointer to the last character in s1 that matches c. */
MEM_PROC  CTEXTSTR MEM_API  StrRChr ( CTEXTSTR s1, TEXTCHAR c );

#ifdef __cplusplus
/* This searches a string for the first character that matches
   some specified character.
   
   A custom strchr function, since microsoft is saying this is
   an unsafe function. This Compiles to compare native strings,
   if UNICODE uses unicode, otherwise uses 8 bit characters.
   Parameters
   s1 :  String to search
   c :   Character to find
   Returns
   pointer in string to search that is the first character that
   matches. NULL if no character matches.
   Note
   This second flavor is only available on C++ where operator
   overloading will switch between CTEXTSTR and TEXTSTR
   \parameters, to result with the correct type. If a CTEXTSTR
   is passed to this it should result with a CTEXTSTR, but if
   that's the only choice, then the result of this is never
   modifiable, even if it is a pointer to a non-const TEXTSTR.  */
MEM_PROC  TEXTSTR MEM_API  StrChr ( TEXTSTR s1, TEXTCHAR c );
/* This searches a string for the last character that matches
   some specified character.
   
   A custom strrchr function, since microsoft is saying this is
   an unsafe function. This Compiles to compare native strings,
   if UNICODE uses unicode, otherwise uses 8 bit characters.
   Parameters
   s1 :  String to search
   c :   Character to find
   Returns
   pointer in string to search that is the first character that
   matches. NULL if no character matches.
   Note
   This second flavor is only available on C++ where operator
   overloading will switch between CTEXTSTR and TEXTSTR
   \parameters, to result with the correct type. If a CTEXTSTR
   is passed to this it should result with a CTEXTSTR, but if
   that's the only choice, then the result of this is never
   modifiable, even if it is a pointer to a non-const TEXTSTR.  */
MEM_PROC  TEXTSTR MEM_API  StrRChr ( TEXTSTR s1, TEXTCHAR c );
/* <combine sack::memory::StrCmp@CTEXTSTR@CTEXTSTR>
   
   \ \                                              */
MEM_PROC  int MEM_API  StrCmp ( const char * s1, CTEXTSTR s2 );
#endif

/* <combine sack::memory::StrCmp@char *@CTEXTSTR>
   
   \ \                                            */
MEM_PROC  int MEM_API  StrCmpEx ( CTEXTSTR s1, CTEXTSTR s2, INDEX maxlen );
/* Finds an instance of a string in another string.
   
   Custom implementation because strstr is declared unsafe, and
   to handle switching between unicode and char.
   
   
   Parameters
   s1 :  the string to search in
   s2 :  the string to locate
   
   Returns
   NULL if s2 is not in s1.
   
   The beginning of the string in s1 that matches s2.
   
   
   Example
   <code lang="c++">
   TEXTCHAR const *found = StrStr( WIDE( "look in this string" ), WIDE( "in" ) );
                                               ^returns a pointer to here.
   </code>                                                                        */
MEM_PROC  CTEXTSTR MEM_API  StrStr ( CTEXTSTR s1, CTEXTSTR s2 );
#ifdef __cplusplus
/* Finds an instance of a string in another string.
   
   Custom implementation because strstr is declared unsafe, and
   to handle switching between unicode and char.
   
   
   Parameters
   s1 :  the string to search in
   s2 :  the string to locate
   
   Returns
   NULL if s2 is not in s1.
   
   The beginning of the string in s1 that matches s2.
   
   
   Example
   <code>
   TEXTCHAR *writable_string = StrDup( WIDE( "look in this string" ) );
   TEXTCHAR *found = StrStr( writable_string, WIDE( "in" ) );
   // returns a pointer to 'in' in the writable string, which can then be modified.
   </code>                                                                          */
MEM_PROC  TEXTSTR MEM_API  StrStr ( TEXTSTR s1, CTEXTSTR s2 );
#endif
/* Searches for one string in another. Compares case
   insensitively.
   Parameters
   s1 :  string to search in
   s2 :  string to locate
   
   See Also
   <link sack::memory::StrStr@CTEXTSTR@CTEXTSTR, StrStr> */
MEM_PROC  CTEXTSTR MEM_API  StrCaseStr ( CTEXTSTR s1, CTEXTSTR s2 );

/* This duplicates a block of memory.
   Parameters
   p :  pointer to a block of memory that was allocated.
   
   Returns
   a pointer to a new block of memory that has the same content
   as the original.                                             */
MEM_PROC  POINTER MEM_API  MemDupEx ( CPOINTER thing DBG_PASS );
/* <combine sack::memory::MemDupEx@CPOINTER thing>
   
   \ \                                             */
#define MemDup(thing) MemDupEx(thing DBG_SRC )

/* Duplicates a string, and returns a pointer to the copy.
   Parameters
   original :  string to duplicate                         */
MEM_PROC  TEXTSTR MEM_API  StrDupEx ( CTEXTSTR original DBG_PASS );
/* Translates from a TEXTCHAR string to a char string. Probably
   only for UNICODE to non wide translation points.
   Parameters
   original :  string to duplicate                              */
MEM_PROC  char *  MEM_API  CStrDupEx ( CTEXTSTR original DBG_PASS );
/* Translates from a TEXTCHAR string to a wchar_t string. Probably
   only for UNICODE to non wide translation points.
   Parameters
   original :  string to duplicate                              */
MEM_PROC  wchar_t *  MEM_API  DupTextToWideEx( CTEXTSTR original DBG_PASS );
#define DupTextToWide(s) DupTextToWideEx( s DBG_SRC )
/* Translates from a TEXTCHAR string to a wchar_t string. Probably
   only for UNICODE to non wide translation points.
   Parameters
   original :  string to duplicate                              */
MEM_PROC  char *     MEM_API  DupTextToCharEx( CTEXTSTR original DBG_PASS );
#define DupTextToChar(s) DupTextToCharEx( s DBG_SRC )
/* Translates from a TEXTCHAR string to a wchar_t string. Probably
   only for UNICODE to non wide translation points.
   Parameters
   original :  string to duplicate                              */
MEM_PROC TEXTSTR     MEM_API  DupWideToTextEx( const wchar_t *original DBG_PASS );
#define DupWideToText(s) DupWideToTextEx( s DBG_SRC )
/* Translates from a TEXTCHAR string to a wchar_t string. Probably
   only for UNICODE to non wide translation points.
   Parameters
   original :  string to duplicate                              */
MEM_PROC TEXTSTR     MEM_API  DupCharToTextEx( const char *original DBG_PASS );
#define DupCharToText(s) DupCharToTextEx( s DBG_SRC )
/* Converts from 8 bit char to 16 bit wchar (or no-op if not
   UNICODE compiled)
   Parameters
   original :  original string of C char. 
   
   Returns
   a pointer to a wide character string.                     */
MEM_PROC  TEXTSTR MEM_API  DupCStrEx ( const char * original DBG_PASS );
/* Converts from 8 bit char to 16 bit wchar (or no-op if not
UNICODE compiled)
Parameters
original :  original string of C char.

Returns
a pointer to a wide character string.                     */
MEM_PROC  TEXTSTR MEM_API  DupCStrLenEx( const char * original, size_t chars DBG_PASS );
/* <combine sack::memory::StrDupEx@CTEXTSTR original>
   
   \ \                                                */
#define StrDup(o) StrDupEx( (o) DBG_SRC )
/* <combine sack::memory::CStrDupEx@CTEXTSTR original>
   
   \ \                                                 */
#define CStrDup(o) CStrDupEx( (o) DBG_SRC )
/* <combine sack::memory::DupCStrEx@char * original>
   
   \ \                                               */
#define DupCStr(o) DupCStrEx( (o) DBG_SRC )

/* <combine sack::memory::DupCStrLenEx@char * original@size_t chars>
   
   \ \                                               */
#define DupCStrLen(o,l) DupCStrLenEx( (o),(l) DBG_SRC )

//------------------------------------------------------------------------

#if 0
// this code was going to provide network oriented shared memory.
#ifndef TRANSPORT_STRUCTURE_DEFINED
typedef uintptr_t PTRANSPORT_QUEUE;
struct transport_queue_tag { uint8_t private_data_here; };

#endif

MEM_PROC  struct transport_queue_tag * MEM_API  CreateQueue ( int size );
MEM_PROC  int MEM_API  EnqueMessage ( struct transport_queue_tag *queue, POINTER msg, int size );
MEM_PROC  int MEM_API  DequeMessage ( struct transport_queue_tag *queue, POINTER msg, int *size );
MEM_PROC  int MEM_API  PequeMessage ( struct transport_queue_tag *queue, POINTER *msg, int *size );
#endif


//------------------------------------------------------------------------

#ifdef __cplusplus

}; // namespace memory
}; // namespace sack
using namespace sack::memory;

#include <stddef.h>

#if defined( _DEBUG ) || defined( _DEBUG_INFO )
/*
inline void operator delete( void * p )
{ Release( p ); }
#ifdef DELETE_HANDLES_OPTIONAL_ARGS
inline void operator delete (void * p DBG_PASS )
{ ReleaseEx( p DBG_RELAY ); }
#define delete delete( DBG_VOIDSRC )
#endif
//#define deleteEx(file,line) delete(file,line)
#ifdef USE_SACK_ALLOCER
inline void * operator new( size_t size DBG_PASS )
{ return AllocateEx( (uintptr_t)size DBG_RELAY ); }
static void * operator new[]( size_t size DBG_PASS )
{ return AllocateEx( (uintptr_t)size DBG_RELAY ); }
#define new new( DBG_VOIDSRC )
#define newEx(file,line) new(file,line)
#endif
*/
// common names - sometimes in conflict when declaring 
// other functions... AND - release is a common 
// component of iComObject 
//#undef Allocate
//#undef Release

// Hmm wonder where this conflicted....
//#undef LineDuplicate

#else
#ifdef USE_SACK_ALLOCER
inline void * operator new(size_t size)
{ return AllocateEx( size ); }
inline void operator delete (void * p)
{ ReleaseEx( p ); }
#endif
#endif

#endif


#endif
