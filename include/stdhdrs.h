/* Includes the system platform as required or appropriate. If
   under a linux system, include appropriate basic linux type
   headers, if under windows pull "windows.h".
   
   
   
   Includes the MOST stuff here ( a full windows.h parse is many
   many lines of code.)                                          */


/* A macro to build a wide character string of __FILE__ */
#define _WIDE__FILE__(n) WIDE(n)
#define WIDE__FILE__ _WIDE__FILE__(__FILE__)

#ifndef STANDARD_HEADERS_INCLUDED
/* multiple inclusion protection symbol */
#define STANDARD_HEADERS_INCLUDED 
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#if _MSC_VER 
#  ifdef EXCLUDE_SAFEINT_H
#    define _INTSAFE_H_INCLUDED_
#  endif
#endif //_MSC_VER 

#ifndef WINVER
#  define WINVER 0x0601
#endif

#if !defined(__LINUX__)
#  ifndef STRICT
#    define STRICT
#  endif
#  define WIN32_LEAN_AND_MEAN

// #define NOGDICAPMASKS             // CC_*, LC_*, PC_*, CP_*, TC_*, RC_                          
// #define NOVIRTUALKEYCODES         // VK_*                                                       
// #define NOWINMESSAGES             // WM_*, EM_*, LB_*, CB_*                                     
// #define NOWINSTYLES               // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*                      
// #define NOSYSMETRICS              // SM_*                                                       
// #define NOMENUS                   // MF_*                                                       
// #define NOICONS                   // IDI_*                                                      
// #define NOKEYSTATES               // MK_*                                                       
// #define NOSYSCOMMANDS             // SC_*                                                       
// #define NORASTEROPS               // Binary and Tertiary raster ops                             
// #define NOSHOWWINDOW              // SW_*                                                       
#  define OEMRESOURCE               // OEM Resource values
// #define NOATOM                    // Atom Manager routines                                      
#  ifndef _INCLUDE_CLIPBOARD
#    define NOCLIPBOARD               // Clipboard routines
#  endif
// #define NOCOLOR                   // Screen colors                                              
// #define NOCTLMGR                  // Control and Dialog routines                                
//(spv) #define NODRAWTEXT                // DrawText() and DT_*                                        

// #define NOGDI                     // All GDI defines and routines                               
// #define NOKERNEL                  // All KERNEL defines and routines                            
// #define NOUSER                    // All USER defines and routines
#  ifndef _ARM_
#    ifndef _INCLUDE_NLS
#      define NONLS                     // All NLS defines and routines
#    endif
#  endif
// #define NOMB                      // MB_* and MessageBox()                                      
#  define NOMEMMGR                  // GMEM_*, LMEM_*, GHND, LHND, associated routines
#  define NOMETAFILE                // typedef METAFILEPICT
// #define NOMINMAX                  // Macros min(a,b) and max(a,b)                               
// #define NOMSG                     // typedef MSG and associated routines                        
// #define NOOPENFILE                // OpenFile(), OemToAnsi, AnsiToOem, and OF_*                 
// #define NOSCROLL                  // SB_* and scrolling routines                                
#  define NOSERVICE                 // All Service Controller routines, SERVICE_ equates, etc.
//#define NOSOUND                   // Sound driver routines                                      
#  define NOTEXTMETRIC              // typedef TEXTMETRIC and associated routines
// #define NOWH                      // SetWindowsHook and WH_*                                    
// #define NOWINOFFSETS              // GWL_*, GCL_*, associated routines                          
// #define NOCOMM                    // COMM driver routines                                       
#  define NOKANJI                   // Kanji support stuff.
#  define NOHELP                    // Help engine interface.
#  define NOPROFILER                // Profiler interface.
//#define NODEFERWINDOWPOS          // DeferWindowPos routines                                    
#  define NOMCX                     // Modem Configuration Extensions
#  define NO_SHLWAPI_STRFCNS   // no StrCat StrCmp StrCpy etc functions.  (used internally)
#  define STRSAFE_NO_DEPRECATE  // This also has defines that override StrCmp StrCpy etc... but no override

#  ifdef _MSC_VER
#    ifndef _WIN32_WINDOWS
// needed at least this for what - updatelayeredwindow?
#      define _WIN32_WINDOWS 0x0601
#    endif
#  endif


// INCLUDE WINDOWS.H
#  ifdef __WATCOMC__
#    undef _WINDOWS_
#  endif

#  ifdef UNDER_CE
// just in case windows.h also fails after undef WIN32
// these will be the correct order for primitives we require.
#    include <excpt.h>
#    include <windef.h>
#    include <winnt.h>
#    include <winbase.h>
#    include <wingdi.h>
#    include <wtypes.h>
#    include <winuser.h>
#    undef WIN32
#  endif
#  define _WINSOCKAPI_
#  include <windows.h>
#  undef _WINSOCKAPI_
#  if defined( WIN32 ) && defined( NEED_SHLOBJ )
#    include <shlobj.h>
#  endif

//#  include <windowsx.h>
// we like timeGetTime() instead of GetTickCount()
//#  include <mmsystem.h>
#ifdef __cplusplus
extern "C"
#endif
__declspec(dllimport) DWORD WINAPI timeGetTime(void);


#  if defined( NEED_SHLAPI )
#    include <shlwapi.h>
#    include <shellapi.h>
#  endif

#  ifdef NEED_V4W
#    include <vfw.h>
#  endif

#  if defined( HAVE_ENVIRONMENT )
#    define getenv(name)       OSALOT_GetEnvironmentVariable(name)
#    define setenv(name,val)   SetEnvironmentVariable(name,val)
#  endif
#  define Relinquish()       Sleep(0)
//#pragma pragnoteonly("GetFunctionAddress is lazy and has no library cleanup - needs to be a lib func")
//#define GetFunctionAddress( lib, proc ) GetProcAddress( LoadLibrary( lib ), (proc) )

#  ifdef __cplusplus
#    ifdef __GNUC__
#      ifndef min
#        define min(a,b) ((a)<(b))?(a):(b)
#      endif
#    endif
#  endif

#  ifdef __cplusplus_cli
#    include <vcclr.h>
#    define DebugBreak() System::Console::WriteLine( /*lprintf( */gcnew System::String( WIDE__FILE__ WIDE("(") STRSYM(__LINE__) WIDE(") Would DebugBreak here...") ) );
//typedef unsigned int HANDLE;
//typedef unsigned int HMODULE;
//typedef unsigned int HWND;
//typedef unsigned int HRC;
//typedef unsigned int HMENU;
//typedef unsigned int HICON;
//typedef unsigned int HINSTANCE;
#  endif

#else // ifdef unix/linux
#  include <pthread.h>
#  include <sched.h>
#  include <unistd.h>
#  include <sys/time.h>
#  include <errno.h>
#  if defined( __ARM__ )
#    define DebugBreak()
#  else
/* A symbol used to cause a debugger to break at a certain
   point. Sometimes dynamicly loaded plugins can be hard to set
   the breakpoint in the debugger, so it becomes easier to
   recompile with a breakpoint in the right place.
   Example
   <code lang="c++">
   DebugBreak();
	</code>                                                      */
#    ifdef __ANDROID__
#      define DebugBreak()
#    else
#      define DebugBreak()  asm("int $3\n" )
#    endif
#  endif

#  ifdef __ANDROID_OLD_PLATFORM_SUPPORT__
extern __sighandler_t bsd_signal(int, __sighandler_t);

#  endif

// moved into timers - please linnk vs timers to get Sleep...
//#define Sleep(n) (usleep((n)*1000))
#  define Relinquish() sched_yield()
#  define GetLastError() (int32_t)errno
/* return with a THREAD_ID that is a unique, universally
   identifier for the thread for inter process communication. */
#  define GetCurrentProcessId() ((uint32_t)getpid())
#  define GetCurrentThreadId() ((uint32_t)getpid())

/* Define a min(a,b) macro when the compiler lacks it. */
#  ifndef min
#    define min(a,b) (((a)<(b))?(a):(b))
#  endif
/* Why not add the max macro, also? */
#  ifndef max
#    define max(a,b) (((a)>(b))?(a):(b))
#  endif

#endif  // end if( !__LINUX__ )

#  include <sack_types.h>

// incldue this first so we avoid a conflict.
// hopefully this comes from sack system?
#  include <sack_system.h>


#if defined( _MSC_VER )|| defined(__LCC__) || defined( __WATCOMC__ ) || defined( __GNUC__ )
#  include "loadsock.h"
#  include <malloc.h>               // _heapmin() included here
#else
//#include "loadsock.h"
#endif
//#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef __CYGWIN__
#include <errno.h> // provided by -lgcc
// lots of things end up including 'setjmp.h' which lacks sigset_t defined here.
#include <sys/types.h>
// lots of things end up including 'setjmp.h' which lacks sigset_t defined here.
#include <sys/signal.h>

#endif

#include <logging.h>

// GetTickCount() and Sleep(n) Are typically considered to be defined by including stdhdrs...
#include <timers.h>


#ifndef MAXPATH
// windef.h has MAX_PATH
# define MAXPATH MAX_PATH
#endif

#ifndef PATH_MAX
// sometimes PATH_MAX is what's used, well it's should be MAXPATH which is MAX_PATH
# define PATH_MAX MAXPATH
#endif

#ifdef _UNICODE
#  ifdef _WIN32
#    ifdef CONSOLE_SHELL
    // in order to get wide characters from the commandline we have to use the GetCommandLineW function, convert it to utf8 for internal usage.
#      define SaneWinMain(a,b) int main( int a, char **argv_real ) { char *tmp; TEXTCHAR **b; ParseIntoArgs( GetCommandLineW(), &a, &b ); Deallocate( char*, tmp ); {
	//int n; TEXTCHAR **b; b = NewArray( TEXTSTR, a + 1 ); for( n = 0; n < a; n++ ) b[n] = DupCharToText( argv_real[n] ); b[n] = NULL; {
#      define EndSaneWinMain() } }
#    else
#      define SaneWinMain(a,b) int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow ) { char *tmp; int a; TEXTCHAR **b; ParseIntoArgs( tmp = WcharConvert( GetCommandLineW() ), &a, &b ); Deallocate( char*, tmp ); {
#      define EndSaneWinMain() } }
#    endif
#  else
#    if defined( __ANDROID__ ) && !defined( ANDROID_CONSOLE_UTIL )
#      define SaneWinMain(a,b) int SACK_Main( int a, char **b )
#      define EndSaneWinMain()
#    else
#      define SaneWinMain(a,b) int main( int a, char **argv_real ) { int n; TEXTCHAR **b; b = NewArray( TEXTSTR, a + 1 ); for( n = 0; n < a; n++ ) b[n] = DupCharToText( argv_real[n] ); b[n] = NULL; {
#      define EndSaneWinMain() } }
#    endif
#  endif
#else
#  ifdef _WIN32
#    ifdef CONSOLE_SHELL
// in order to get wide characters from the commandline we have to use the GetCommandLineW function, convert it to utf8 for internal usage.
#      define SaneWinMain(a,b) int main( int a, char **argv_real ) { char *tmp; TEXTCHAR **b; ParseIntoArgs( tmp = WcharConvert( GetCommandLineW() ), &a, &b ); Deallocate( char*, tmp ); {
#      define EndSaneWinMain() } }
#    else
#      define SaneWinMain(a,b) int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow ) { int a; char *tmp; TEXTCHAR **b; ParseIntoArgs( tmp = WcharConvert( GetCommandLineW() ), &a, &b ); {
#      define EndSaneWinMain() } }
#    endif
#  else
#    if defined( __ANDROID__ ) && !defined( ANDROID_CONSOLE_UTIL )
#      define SaneWinMain(a,b) int SACK_Main( int a, char **b )
#      define EndSaneWinMain()
#    else
#      define SaneWinMain(a,b) int main( int a, char **b ) { char **argv_real = b; {
#      define EndSaneWinMain() } }
#    endif
#  endif
#endif


#include <final_types.h>

#endif
