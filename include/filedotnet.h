/* A header for doing .NET /CLR compatiblity changes. Things
   like fopen needing to be _fopen_s and junk.               */

#ifndef FILE_DOT_NET_COMPAT
/* Header multiple inclusion protection symbol. */
#define FILE_DOT_NET_COMPAT
#include <filesys.h>

#ifdef __cplusplus_cli
#define Fopen( result, name, opts ) { char *tmp1 = CStrDup( name ); char *tmp2 = CStrDup( opts ); result = fopen( tmp1, tmp2 ); Deallocate( char *, tmp1 ); Deallocate( char *, tmp2 ); }
#if asdfasdlfkajsdflkj

#define fputs( msg, file ) { char *tmp = CStrDup( msg ); fputs( tmp, file ); Release( tmp ); }
#define unlink( name ) { char *tmp = CStrDup( name ); unlink( tmp ); Release( tmp ); }
#define rename( name1, name2 ) { char *tmp1 = CStrDup( name1 ); char *tmp2 = CStrDup( name2 ); rename( tmp1, tmp2 ); Release( tmp1 ); Release( tmp2 ); }

#define fprintf Fprintf
#endif
//int Fprintf( FILE *file, CTEXTSTR fmt, ... );

/*
using namespace Win32;

#define CreateEvent(a,b,c,d) Win32::Kernel::CreateEvent((SECURITY_ATTRIBUTES)a,b,c,d)
#define OpenEvent(a,b,c)     Win32::Kernel::OpenEvent(a,b,c)
#define Sleep(a)             Win32::Kernel::Sleep(a)
#define GetTickCount()       Win32::Kernel::GetTickCount()
#define GetCurrentProcessId() Win32::Kernel::GetCurrentProcessId()
#define GetCurrentThreadId()  Win32::Kernel::GetCurrentThreadId()
#define GetLastError()  Win32::Kernel::GetLastError()
#define SetEvent(a) Win32::Kernel::SetEvent(a)
#define ResetEvent(a) Win32::Kernel::ResetEvent(a)
#define CloseHandle(a) Win32::Kernel::CloseHandle(a)
#define WaitForSingleObject(a,b) Win32::Kernel::WaitForSingleObject(a,b)
#define PeekMessage(a,b,c,d,e)  Win32::User::PeekMessage(a,b,c,d,e)
#define DispatchMessage(a)   Win32::User::DispatchMessage(a)
#define GetModuleFileName(a,b) Win32::Kernel::GetModuleFileName(a,b)
*/

#if 0
typedef struct MyFile MYFILE;

MYFILE *Fopen( CTEXTSTR filename, CTEXTSTR mode );
int Fread( POINTER data, int count, int size, MYFILE *file );
int Fwrite( POINTER data, int count, int size, MYFILE *file );
int Fclose( MYFILE *file );
int Fseek( MYFILE *file, S_64 pos, int whence );
_64 Ftell( MYFILE *file );
MYFILE *Fdopen( int fd, CTEXTSTR mode );
int Ferror( MYFILE *file );
int Fflush( MYFILE *file );
int Rewind( MYFILE *file );
int Fputc( int c, MYFILE *file );
int Fgets( TEXTSTR buf, int buflen, MYFILE *file );
int Fputs( CTEXTSTR but, MYFILE *file );
int Unlink( CTEXTSTR filename );
int Rename( CTEXTSTR from, CTEXTSTR to );

#define rename Rename
#define unlink Unlink
#define FILE MYFILE
#define fopen Fopen
#define fseek Fseek
#define fclose Fclose
#define fprintf Fprintf
#define ftell Ftell
#define fread Fread
#define fwrite Fwrite
//#define fdopen Fdopen
#define ferror Ferror
#define fflush Fflush
#define rewind Rewind
#define fputc Fputc
#define fgets Fgets
#define fputs Fputs
#endif


#else
/* A macro which can be translated into microsoft so-called safe
   methods.                                                      */
#define Fopen( result, name, opts ) result = sack_fopen( 0, name, opts )
//#define MYFILE  FILE
//#define Fopen   fopen
//#define Fread   fread
//#define Fwrite  fread
//#define Fclose  fclose
//#define Fprintf fprintf
//#define Fseek   fseek
//#define Ftell   ftell
#endif


#endif
// end with a newline please.
