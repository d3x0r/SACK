#ifndef MY_TYPES_INCLUDED
#define MY_TYPES_INCLUDED

#ifdef __LINUX__
#define DebugBreak() asm( WIDE("int $3\n") )
#else
#define DebugBreak()
#endif

#ifdef __WATCOMC__
#define CPROC _cdecl
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
#define DBG_SRC         , __FILE__, __LINE__
#define DBG_VOIDSRC     __FILE__, __LINE__ 
#define DBG_VOIDPASS    char *pFile, int nLine
#define DBG_PASS        , char *pFile, int nLine
#define DBG_RELAY       , pFile, nLine
#define DBG_FORWARD     , pFile, nLine
#define DBG_FILELINEFMT "%s(%d)"
#else
#define DBG_SRC 
#define DBG_VOIDSRC     
#define DBG_VOIDPASS    void
#define DBG_PASS
#define DBG_RELAY 
#define DBG_FORWARD     , __FILE__, __LINE__
#define DBG_FILELINEFMT
#endif

#ifdef UNICODE
#error unicode not defined.
#else
#define WIDE(s)  s
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif
//typedef void           _0;
typedef void *P_0;
#include <stdint.h>
/*
 refactors out - no more _xx types.
typedef unsigned char  uint8_t;
typedef uint8_t *uint8_t*;
typedef unsigned short uint16_t;
typedef uint16_t *uint16_t*;
typedef unsigned long  uint32_t;
typedef uint32_t *uint32_t*;
typedef signed   char  int8_t;
typedef signed   short int16_t;
typedef signed   long  int32_t;
*/
typedef const unsigned char *CTEXTSTR;
typedef unsigned char TEXTCHAR;
typedef TEXTCHAR *TEXTSTR;

#ifdef _WIN32
#ifndef CALLBACK
//#pragma message ("Setting CALLBACK to __stcall" )
#define CALLBACK    __stdcall
#endif
#else
#ifndef CALLBACK
//#pragma message ("Setting CALLBACK to c call" )
#define CALLBACK
#endif
#endif
//#define SOCKADDR    sockaddr

typedef uint32_t   INDEX;
#define INVALID_INDEX ((uint32_t)-1) 
typedef void  *POINTER;
typedef const void *CPOINTER;
typedef uint32_t LOGICAL;
//typedef uint32_t uintptr_t;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define DECLDATA(name,sz) struct {uint32_t size; uint8_t data[sz];} name

typedef struct DataBlock {
   uint32_t size;     // size is sometimes a pointer value...
                 // this means bad thing when we change platforms...
   uint8_t  data[1]; // beginning of var data - this is created size+sizeof(VPA)
} DATA, *PDATA;

typedef struct LinkBlock
{
   uint32_t     Cnt;
   uint32_t     Lock;
   POINTER pNode[1];
} LIST, *PLIST;

typedef struct DataListBlock
{
   uint32_t     Cnt;
   uint32_t     Size;
   uint8_t      data[1];
} DATALIST, *PDATALIST; 

typedef struct LinkStack
{
   uint32_t     Top;
   uint32_t     Cnt;
   POINTER pNode[1];
} LINKSTACK, *PLINKSTACK;

typedef struct DataListStack
{
   uint32_t     Top; // next avail...
   uint32_t     Cnt;
   uint32_t     Size;
   uint8_t      data[1];
} DATASTACK, *PDATASTACK;

typedef struct LinkQueue
{
   uint32_t     Top;
   uint32_t     Bottom;
   uint32_t     Cnt;
   uint32_t     Lock;  // thread interlock using InterlockedExchange semaphore
   POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKQUEUE, *PLINKQUEUE;

typedef struct LinkStackQueue // additional step function... 
{
   uint32_t     Top;
   uint32_t     Bottom;
   uint32_t     Next;
   uint32_t     Cnt;
   POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKSTACKQUEUE, *PLINKSTACKQUEUE;

//#undef NULL
//#define NULL ((BYTE FAR *)0)

#endif
