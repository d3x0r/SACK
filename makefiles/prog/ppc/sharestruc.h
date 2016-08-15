#ifndef MEMORY_STRUCT_DEFINED
#define MEMORY_STRUCT_DEFINED
//#include "./types.h"

#define _SHARED_MEMORY_LIBRARY

#define SECTION_LOGGED_WAIT 0x80000000

typedef struct critical_section_tag {
	uint32_t dwUpdating; // this is set when entering or leaving (updating)... 
   uint32_t dwThread; // windows upper 16 is process ID, lower is thread ID
   uint32_t dwLocks;
#ifdef _DEBUG
	CTEXTSTR pFile;
	int  nLine;
#endif
} CRITICALSECTION, *PCRITICALSECTION;

typedef struct chunk_tag
{
#define CHUNK_FLAG_OPEN  0x0001
	// pipes are allocated as 3*maximum segment size...
#define CHUNK_FLAG_PIPE  0x0002  // simple head/tail synchronization
								 // this is common producer/consumer type
								 // protection where only one side updates
								 // a set of values....
#define CHUNK_FLAG_STACK 0x0004  // complex synchronization
							     // this requires two processes to
								 // both be allowed to synchonistically
								 // update a single value - StackTop
//	uint32_t dwFlags;
	uint32_t dwSize;  // limited to allocating 4 billion bytes...
	uint32_t dwOwners;            // if 0 - block is free
	struct chunk_tag *pPrior;         // save some math backwards...
   struct memory_block_tag * pRoot;  // pointer to master allocation struct (pMEM)
	struct chunk_tag *pNextFree
                  , *pPriorFree;
#ifdef _DEBUG
   CTEXTSTR pFile;
   uint32_t     nLine; // shouldn't have more than 4 billion lines
#endif
	uint8_t byData[1]; // uint8_t is the smallest valid datatype could be _0
} CHUNK, *PCHUNK;


typedef struct memory_block_tag
{
//	HANDLE hFile;
//	HANDLE hMem;
	// these should (from experience be able to be
	// upper/lower words of same uint32_t ?
	uint32_t dwProcess;  // region creator process ID
	uint32_t dwThread;   // region creator thread ID...
	// lock between multiple processes/threads
	CRITICALSECTION cs;

	uint32_t dwFlags;
   uint32_t dwSize;
	PCHUNK pFirstFree;
	CHUNK pRoot[1];
} MEM, *PMEM;

typedef struct memory_stream_tag
{
   PMEM  Map;
   uint32_t   pointer; // MPI - Mem pos index
} MSTREAM, *PMSTREAM;

#endif
// $Log: sharestruc.h,v $
// Revision 1.1  2003/05/07 03:07:49  panther
// Initial commit sack tree
//
// Revision 1.4  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
