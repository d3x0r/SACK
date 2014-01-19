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
typedef unsigned char  _8;
typedef _8 *P_8;
typedef unsigned short _16;
typedef _16 *P_16;
typedef unsigned long  _32;
typedef _32 *P_32;
typedef signed   char  S_8;
typedef signed   short S_16;
typedef signed   long  S_32;
typedef const unsigned char *CTEXTSTR;
typedef unsigned char TEXTCHAR;
typedef TEXTCHAR *TEXTSTR;

#ifdef _WIN32
#ifndef CALLBACK
#pragma message ("Setting CALLBACK to __stcall" )
#define CALLBACK    __stdcall
#endif
#else
#ifndef CALLBACK
#pragma message ("Setting CALLBACK to c call" )
#define CALLBACK
#endif
#endif
//#define SOCKADDR    sockaddr

typedef _32   INDEX;
#define INVALID_INDEX ((_32)-1) 
typedef void  *POINTER;
typedef const void *CPOINTER;
typedef _32 LOGICAL;
typedef _32 PTRSZVAL;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define DECLDATA(name,sz) struct {_32 size; _8 data[sz];} name

typedef struct DataBlock {
   _32 size;     // size is sometimes a pointer value...
                 // this means bad thing when we change platforms...
   _8  data[1]; // beginning of var data - this is created size+sizeof(VPA)
} DATA, *PDATA;

typedef struct LinkBlock
{
   _32     Cnt;
   _32     Lock;
   POINTER pNode[1];
} LIST, *PLIST;

typedef struct DataListBlock
{
   _32     Cnt;
   _32     Size;
   _8      data[1];
} DATALIST, *PDATALIST; 

typedef struct LinkStack
{
   _32     Top;
   _32     Cnt;
   POINTER pNode[1];
} LINKSTACK, *PLINKSTACK;

typedef struct DataListStack
{
   _32     Top; // next avail...
   _32     Cnt;
   _32     Size;
   _8      data[1];
} DATASTACK, *PDATASTACK;

typedef struct LinkQueue
{
   _32     Top;
   _32     Bottom;
   _32     Cnt;
   _32     Lock;  // thread interlock using InterlockedExchange semaphore
   POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKQUEUE, *PLINKQUEUE;

typedef struct LinkStackQueue // additional step function... 
{
   _32     Top;
   _32     Bottom;
   _32     Next;
   _32     Cnt;
   POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKSTACKQUEUE, *PLINKSTACKQUEUE;

//#undef NULL
//#define NULL ((BYTE FAR *)0)

#endif
