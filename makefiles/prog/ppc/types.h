#ifndef MY_TYPES_INCLUDED
#define MY_TYPES_INCLUDED
#include <stdlib.h>
#include <ctype.h>

#if defined( GCC) && !defined( __ARM__ ) && !defined( __EMSCRIPTEN__ )
#  define DebugBreak() asm( "int $3\n" )
#else
#  define DebugBreak()
#endif

#ifdef __WATCOMC__
#  define CPROC _cdecl
#else
#define CPROC 
#endif

#ifdef _MSC_VER
#define LONGEST_INT __int64
#else
#define LONGEST_INT long long
#endif
#define LONGEST_FLT double

// this is for passing FILE, LINE information to allocate
// useful during DEBUG phases only...
#ifdef _DEBUG
#  define DBG_SRC         , __FILE__, __LINE__
#  define DBG_VOIDSRC     __FILE__, __LINE__
#  define DBG_VOIDPASS    char *pFile, int nLine
#  define DBG_PASS        , char *pFile, int nLine
#  define DBG_RELAY       , pFile, nLine
#  define DBG_FORWARD     , pFile, nLine
#  define DBG_FILELINEFMT "%s(%d)"
#else
#  define DBG_SRC
#  define DBG_VOIDSRC
#  define DBG_VOIDPASS    void
#  define DBG_PASS
#  define DBG_RELAY
#  define DBG_FORWARD     , __FILE__, __LINE__
#  define DBG_FILELINEFMT
#endif

#ifndef FALSE
#  define FALSE 0
#endif
#ifndef TRUE
#  define TRUE (!FALSE)
#endif
//typedef void           _0;
typedef void *P_0;
#include <stdint.h>

typedef const unsigned char *CTEXTSTR;
typedef unsigned char TEXTCHAR;
typedef TEXTCHAR *TEXTSTR;


typedef size_t   INDEX;
#define INVALID_INDEX ((size_t)-1) 
typedef void  *POINTER;
typedef const void *CPOINTER;
typedef uint32_t LOGICAL;

#define DECLDATA(name,sz) struct {uintptr_t size; char data[sz];} name

typedef struct DataBlock {
	uintptr_t size;  // size is sometimes a pointer value...
	                 // this means bad thing when we change platforms...
	char  data[1];   // beginning of var data - this is created size+sizeof(VPA)
} DATA, *PDATA;

typedef struct LinkBlock
{
	size_t     Cnt;
	uint32_t     Lock;
	POINTER pNode[1];
} LIST, *PLIST;

typedef struct DataListBlock
{
	size_t     Cnt;
	size_t     Size;
	uint8_t      data[1];
} DATALIST, *PDATALIST; 

typedef struct LinkStack
{
	size_t     Top;
	size_t     Cnt;
	POINTER pNode[1];
} LINKSTACK, *PLINKSTACK;

typedef struct DataListStack
{
	size_t     Top; // next avail...
	size_t     Cnt;
	size_t     Size;
	uint8_t      data[1];
} DATASTACK, *PDATASTACK;

typedef struct LinkQueue
{
	size_t     Top;
	size_t     Bottom;
	size_t     Cnt;
	uint32_t     Lock;  // thread interlock using InterlockedExchange semaphore
	POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKQUEUE, *PLINKQUEUE;

typedef struct LinkStackQueue // additional step function... 
{
	size_t     Top;
	size_t     Bottom;
	size_t     Next;
	size_t     Cnt;
	POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKSTACKQUEUE, *PLINKSTACKQUEUE;

#endif
