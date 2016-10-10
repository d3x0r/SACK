/* please Include sthdrs.h */



/* Define most of the sack core types on which everything else is
   based. Also defines some of the primitive container
   structures. We also handle a lot of platform/compiler
   abstraction here.


   A reFactoring for stdint.h and uint32_t etc would be USEFUL!
   where types don't exist, define them as apprpritate types instead.
But WHO doesn't have stdint?  BTW is sizeof( size_t ) == sizeof( void* )



   This is automatically included with stdhdrs.h; however, when
   including sack_types.h, the minimal headers are pulled. */
#define HAS_STDINT
#include <stdint.h>

//#define USE_SACK_CUSTOM_MEMORY_ALLOCATION
	// this has to be a compile option (option from cmake)
   // enables debug dump mem...
#ifdef USE_SACK_CUSTOM_MEMORY_ALLOCATION
#define USE_CUSTOM_ALLOCER 1
#else
#define USE_CUSTOM_ALLOCER 0
#endif


#ifdef _MSC_VER
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif

#ifndef WIN32
#ifdef _WIN32
#define WIN32 _WIN32
#endif
#endif

// force windows on __MSVC
#  ifndef WIN32
#    define WIN32
#  endif


#endif

#if !defined( __NO_THREAD_LOCAL__ ) && ( defined( _MSC_VER ) || defined( __WATCOMC__ ) )
#  define HAS_TLS 1
#  define DeclareThreadLocal static __declspec(thread)
#  define DeclareThreadVar __declspec(thread)
#elif !defined( __NO_THREAD_LOCAL__ ) && ( defined( __GNUC__ ) )
#  define HAS_TLS 1
#  define DeclareThreadLocal static __thread
#  define DeclareThreadVar __thread
#else
#  define DeclareThreadLocal static
#  define DeclareThreadVar
#endif


#ifdef __cplusplus_cli
// these things define a type called 'Byte'
	// which causes confusion... so don't include vcclr for those guys.
#ifdef SACK_BAG_EXPORTS
// maybe only do this while building sack_bag project itself...
#if !defined( ZCONF_H ) \
	&& !defined( __FT2_BUILD_GENERIC_H__ ) \
	&& !defined( ZUTIL_H ) \
	&& !defined( SQLITE_PRIVATE ) \
	&& !defined( NETSERVICE_SOURCE ) \
	&& !defined( LIBRARY_DEF )
#include <vcclr.h>
//using namespace System;
#endif
#endif
#endif

// Defined for building visual studio monolithic build.  These symbols are not relavent with cmakelists.
#ifdef SACK_BAG_EXPORTS

#define SACK_BAG_CORE_EXPORTS

// exports don't really matter with CLI compilation.
#  ifndef BAG

//#ifndef TARGETNAME
//#  define TARGETNAME "sack_bag.dll"  //$(TargetFileName)
//#endif

#ifndef __cplusplus_cli
// cli mode, we use this directly, and build the exports in sack_bag.dll directly
#else
#define LIBRARY_DEADSTART
#endif

#define USE_SACK_FILE_IO
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define MEM_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SYSLOG_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define _TYPELIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define HTTP_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define TIMER_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define IDLE_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define CLIENTMSG_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define FRACTION_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define NETWORK_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define CONFIGURATION_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define FILESYSTEM_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SYSTEM_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define FILEMONITOR_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define VECTOR_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SHA1_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define CONSTRUCT_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define PROCREG_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SQLPROXY_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define TYPELIB_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
//#define SEXPAT_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SERVICE_SOURCE
#  ifndef __NO_SQL__
#    ifndef __NO_OPTIONS__
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.    and not NO_SQL and not NO_OPTIONS   */
#define SQLGETOPTION_SOURCE
#    endif
#  endif

/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define PSI_SOURCE

#ifdef _MSC_VER

#ifndef JPEG_SOURCE
//wouldn't matter... the external things wouldn't need to define this
//#error projects were not generated with CMAKE, and JPEG_SORUCE needs to be defined
#endif
//#define JPEG_SOURCE
//#define __PNG_LIBRARY_SOURCE__
//#define FT2_BUILD_LIBRARY   // freetype is internal
//#define FREETYPE_SOURCE		// build Dll Export
#endif

/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define MNG_BUILD_DLL

/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define BAGIMAGE_EXPORTS
/* Defined when SACK_BAG_EXPORTS is defined. This was an
 individual library module once upon a time.           */
#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SYSTRAY_LIBRARAY

/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define SOURCE_PSI2

/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
#define VIDEO_LIBRARY_SOURCE
/* Defined when SACK_BAG_EXPORTS is defined. This was an
   individual library module once upon a time.           */
	/* define RENDER SOURCE when building monolithic. */
#ifndef RENDER_LIBRARY_SOURCE
#define RENDER_LIBRARY_SOURCE
#endif


#ifndef __NO_WIN32API__
// this is moved to a CMake option (based on whter it's arm or not right now)
//#define _OPENGL_ENABLED
#endif
// define a type that is a public name struct type...
// good thing that typedef and struct were split
// during the process of port to /clr option.
//#define PUBLIC_TYPE public
#else
//#define PUBLIC_TYPE
#ifdef __cplusplus_CLR
#include <vcclr.h>
//using namespace System;
#endif

#ifdef __CYGWIN__
#include <wchar.h> // wchar for X_16 definition
#endif
#ifdef _MSC_VER
#include <sys/stat.h>
#endif


#endif
#endif

#ifndef MY_TYPES_INCLUDED
#define MY_TYPES_INCLUDED
// include this before anything else
// thereby allowing us to redefine exit()
#include <limits.h> // CHAR_BIT
#include <stdlib.h>
#include <stdarg.h> // typelib requires this
#ifdef _MSC_VER
#ifndef UNDER_CE
#include <intrin.h> // memlib requires this, and it MUST be included befoer string.h if it is used.
#endif
#endif
#include <string.h> // typelib requires this
#if !defined( WIN32 ) && !defined( _WIN32 ) && !defined( _PNACL )
#include <dlfcn.h>
#endif

#if defined( _MSC_VER )
// disable pointer conversion warnings - wish I could disable this
// according to types...
//#pragma warning( disable:4312; disable:4311 )
// disable deprication warnings of snprintf, et al.
//#pragma warning( disable:4996 )
#define EMPTY_STRUCT struct { char nothing[]; }
#endif
#if defined( __WATCOMC__ )
#define EMPTY_STRUCT char
#endif

#ifdef __cplusplus
/* Could also consider defining 'SACK_NAMESPACE' as 'extern "C"
   ' {' and '..._END' as '}'                                    */
#define SACK_NAMESPACE namespace sack {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define SACK_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _CONTAINER_NAMESPACE namespace containers {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _CONTAINER_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _LINKLIST_NAMESPACE namespace list {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _LINKLIST_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _DATALIST_NAMESPACE namespace data_list {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _DATALIST_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _SETS_NAMESPACE namespace sets {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _SETS_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _TEXT_NAMESPACE namespace text {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _TEXT_NAMESPACE_END }
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define TEXT_NAMESPACE SACK_NAMESPACE _CONTAINER_NAMESPACE namespace text {
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define TEXT_NAMESPACE_END  } _CONTAINER_NAMESPACE_END SACK_NAMESPACE_END
#else
/* Define the sack namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define SACK_NAMESPACE
/* Define the sack namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define SACK_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _CONTAINER_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _CONTAINER_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _LINKLIST_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _LINKLIST_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _DATALIST_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _DATALIST_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _SETS_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _SETS_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _TEXT_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define _TEXT_NAMESPACE_END
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define TEXT_NAMESPACE
/* Define the container namespace (when building with C++, the
   wrappers are namespace{} instead of extern"c"{} )           */
#define TEXT_NAMESPACE_END
#endif

/* declare composite SACK_CONTAINER namespace to declare sack::container in a single line */
#define SACK_CONTAINER_NAMESPACE SACK_NAMESPACE _CONTAINER_NAMESPACE
/* declare composite SACK_CONTAINER namespace to close sack::container in a single line */
#define SACK_CONTAINER_NAMESPACE_END _CONTAINER_NAMESPACE_END SACK_NAMESPACE_END
/* declare composite SACK_CONTAINER namespace to declare sack::container::list in a single line */
#define SACK_CONTAINER_LINKLIST_NAMESPACE SACK_CONTAINER_NAMESPACE _LISTLIST_NAMESPACE
/* declare composite SACK_CONTAINER namespace to close sack::container::list in a single line */
#define SACK_CONTAINER_LINKLIST_NAMESPACE_END _LISTLIST_NAMESPACE_END SACK_CONTAINER_NAMESPACE

// this symbols is defined to enforce
// the C Procedure standard - using a stack, and resulting
// in EDX:EAX etc...
#define CPROC

#ifdef SACK_BAG_EXPORTS
# ifdef BUILD_GLUE
// this is used as the export method appropriate for C#?
#  define EXPORT_METHOD [DllImport(LibName)] public
# else
#  ifdef __cplusplus_cli
#   if defined( __STATIC__ ) || defined( __LINUX__ ) || defined( __ANDROID__ )
#     define EXPORT_METHOD
#     define IMPORT_METHOD extern
#   else
#     define EXPORT_METHOD __declspec(dllexport)
#     define IMPORT_METHOD __declspec(dllimport)
#   endif
#   define LITERAL_LIB_EXPORT_METHOD __declspec(dllexport)
#   define LITERAL_LIB_IMPORT_METHOD extern
//__declspec(dllimport)
#  else
#   if defined( __STATIC__ ) || defined( __LINUX__ ) || defined( __ANDROID__ )
#      define EXPORT_METHOD
#      define IMPORT_METHOD extern
#    else
/* Method to declare functions exported from a DLL. (nothign on
   LINUX or building statically, but __declspec(dllimport) on
   windows )                                                    */
#      define EXPORT_METHOD __declspec(dllexport)
/* method to define a function which will be Imported from a
   library. Under windows, this is probably
   __declspec(dllimport). Under linux this is probably 'extern'. */
#      define IMPORT_METHOD __declspec(dllimport)
#    endif
#      define LITERAL_LIB_EXPORT_METHOD __declspec(dllexport)
#      define LITERAL_LIB_IMPORT_METHOD __declspec(dllimport)
#  endif
# endif
#else
# if ( !defined( __STATIC__ ) && defined( WIN32 ) && !defined( __cplusplus_cli) )
#  define EXPORT_METHOD __declspec(dllexport)
#  define IMPORT_METHOD __declspec(dllimport)
#  define LITERAL_LIB_EXPORT_METHOD __declspec(dllexport)
#  define LITERAL_LIB_IMPORT_METHOD __declspec(dllimport)
# else
// MRT:  This is needed.  Need to see what may be defined wrong and fix it.
#  if defined( __LINUX__ ) || defined( __STATIC__ ) /*&& !defined( __cplusplus_cli ) */
#    define EXPORT_METHOD
#    define IMPORT_METHOD extern
#    define LITERAL_LIB_EXPORT_METHOD
#    define LITERAL_LIB_IMPORT_METHOD extern
#  else
#    define EXPORT_METHOD __declspec(dllexport)
#    define IMPORT_METHOD __declspec(dllimport)
/* Define how methods in LITERAL_LIBRARIES are exported.
   literal_libraries are libraries that are used for plugins,
   and are dynamically loaded by code. They break the rules of
   system prefix and suffix extensions. LITERAL_LIBRARIES are
   always dynamic, and never static.                           */
#    define LITERAL_LIB_EXPORT_METHOD __declspec(dllexport)
/* Define how methods in LITERAL_LIBRARIES are imported.
   literal_libraries are libraries that are used for plugins,
   and are dynamically loaded by code. They break the rules of
   system prefix and suffix extensions. LITERAL_LIBRARIES are
   always dynamic, and never static.                           */
#    define LITERAL_LIB_IMPORT_METHOD __declspec(dllimport)
#  endif
# endif
#endif
// used when the keword specifying a structure is packed
// needs to prefix the struct keyword.
#define PREFIX_PACKED

// private thing left as a note, and forgotten.  some compilers did not define offsetof
#define my_offsetof( ppstruc, member ) ((uintptr_t)&((*ppstruc)->member)) - ((uintptr_t)(*ppstruc))


SACK_NAMESPACE


#ifndef __LINUX__
//#include <sys/types.h>
//typedef int pid_t;
#endif

#ifdef BCC16
#define __inline__
#define MAINPROC(type,name)     type _pascal name
// winproc is intended for use at libmain/wep/winmain...
#define WINPROC(type,name)      type _far _pascal _export name
// callbackproc is for things like timers, dlgprocs, wndprocs...
#define CALLBACKPROC(type,name) type _far _pascal _export name
#define PUBLIC(type,name)       type STDPROC _export name
#define LIBMAIN() WINPROC(int, LibMain)(HINSTANCE hInstance, WORD wDataSeg, WORD wHeapSize, LPSTR lpCmdLine ) \
		{ /* here would be if dwReason == process_attach */ {
#define LIBEXIT() } /* end if */ } /*endproc*/ \
	   int STDPROC WEP(int nSystemExit )  {
#define LIBMAIN_END()  }

// should use this define for all local defines...
// this will allow one place to modify ALL _pascal or not to _pascal decls.
#define STDPROC _far _pascal

#endif

#if defined( __LCC__ ) || defined( _MSC_VER ) || defined(__DMC__) || defined( __WATCOMC__ )
#ifdef __WATCOMC__
#undef CPROC
#define CPROC __cdecl
#define STDPROC __cdecl
#ifndef __WATCOMC__
// watcom windef.h headers define this
#define STDCALL _stdcall
#endif
#if __WATCOMC__ >= 1280
// watcom windef.h headers no longer define this.
#define STDCALL __stdcall
#endif
#undef PREFIX_PACKED
#define PREFIX_PACKED _Packed
#else
#undef CPROC
//#error blah
#define CPROC __cdecl
#define STDPROC
#define STDCALL _stdcall
#endif

#define far
#define huge
#define near
#define _far
#define _huge
#define _near
/* portability type for porting legacy 16 bit applications. */
/* portability macro for legacy 16 bit applications. */
#define __far
#ifndef FAR
#define FAR
#endif
//#define HUGE
//#ifndef NEAR
//#define NEAR
//#endif
#define _fastcall
#ifdef __cplusplus
#ifdef __cplusplus_cli
#define PUBLIC(type,name) extern "C"  LITERAL_LIB_EXPORT_METHOD type CPROC name
#else
//#error what the hell!?
// okay Public functions are meant to be loaded with LoadFuncion( "library" , "function name"  );
#define PUBLIC(type,name) extern "C"  LITERAL_LIB_EXPORT_METHOD type CPROC name
#endif
#else
#define PUBLIC(type,name) LITERAL_LIB_EXPORT_METHOD type CPROC name
#endif
#define MAINPROC(type,name)  type WINAPI name
#define WINPROC(type,name)   type WINAPI name
#define CALLBACKPROC(type,name) type CALLBACK name

#if defined( __WATCOMC__ )
#define LIBMAIN()   static int __LibMain( HINSTANCE ); PRELOAD( LibraryInitializer ) { \
	__LibMain( GetModuleHandle(_WIDE(TARGETNAME)) );   } \
	static int __LibMain( HINSTANCE hInstance ) { /*Log( WIDE("Library Enter" ) );*/
#define LIBEXIT() } static int LibExit( void ); ATEXIT( LiraryUninitializer ) { LibExit(); } int LibExit(void) { /*Log( WIDE("Library Exit" ) );*/
#define LIBMAIN_END() }
#else
#ifdef TARGETNAME
#define LIBMAIN()   static int __LibMain( HINSTANCE ); PRELOAD( LibraryInitializer ) { \
	__LibMain( GetModuleHandle(_WIDE(TARGETNAME)) );   } \
	static int __LibMain( HINSTANCE hInstance ) { /*Log( WIDE("Library Enter" ) );*/
#else
#define LIBMAIN()   TARGETNAME_NOT_DEFINED
#endif
#define LIBEXIT() } static int LibExit( void ); ATEXIT( LiraryUninitializer ) { LibExit(); } int LibExit(void) { /*Log( WIDE("Library Exit" ) );*/
#define LIBMAIN_END() }
#endif
#define PACKED
#endif

#if defined( __GNUC__ )
#  ifndef STDPROC
#    define STDPROC
#  endif
#  ifndef STDCALL
#    define STDCALL // for IsBadCodePtr which isn't a linux function...
#  endif
#  ifndef WINAPI
#    ifdef __LINUX__
#       define WINAPI
#    else
#       define WINAPI __stdcall
#    endif
#  endif
#  ifndef PASCAL
//#define PASCAL
#  endif
#  define WINPROC(type,name)   type WINAPI name
#  define CALLBACKPROC( type, name ) type name
#  define PUBLIC(type,name) EXPORT_METHOD type CPROC name
#  define LIBMAIN()   static int __LibMain( HINSTANCE ); PRELOAD( LibraryInitializer ) { \
	__LibMain( GetModuleHandle(TARGETNAME) );   } \
	static int __LibMain( HINSTANCE hInstance ) { /*Log( WIDE("Library Enter" ) );*/
#  define LIBEXIT() } static int LibExit( void ); ATEXIT( LiraryUninitializer ) { LibExit(); } int LibExit(void) { /*Log( WIDE("Library Exit" ) );*/
#  define LIBMAIN_END()  }
/* Portability Macro for porting legacy code forward. */
#  define FAR
#  define NEAR
//#define HUGE
#  define far
#  define near
#  define huge
#  define PACKED __attribute__((packed))
#endif

#if defined( BCC32 )
#define far
#define huge
/* define obsolete keyword for porting purposes */
/* defined for porting from 16 bit environments */
#define near
/* portability macro for legacy 16 bit applications. */
#define _far
#define _huge
#define _near
/* portability type for porting to compilers that don't inline. */
/* portability macro for legacy 16 bit applications. */
#define __inline__
#define MAINPROC(type,name)     type _pascal name
// winproc is intended for use at libmain/wep/winmain...
#define WINPROC(type,name)      EXPORT_METHOD type _pascal name
// callbackproc is for things like timers, dlgprocs, wndprocs...
#define CALLBACKPROC(type,name) EXPORT_METHOD type _stdcall name
#define STDCALL _stdcall
#define PUBLIC(type,name)        type STDPROC name
#ifdef __STATIC__
#define LIBMAIN() static WINPROC(int, LibMain)(HINSTANCE hInstance, DWORD dwReason, void *unused ) \
		{ if( dwReason == DLL_PROCESS_ATTACH ) {\
			/*Log( WIDE("Library Enter" ) );*//* here would be if dwReason == process_attach */
#define LIBEXIT() } /* end if */ if( dwReason == DLL_PROCESS_DETACH ) {  \
	  									 /*Log( WIDE("Library Exit" ) );*/
#define LIBMAIN_END()  } return 1; }
#else
#define LIBMAIN() WINPROC(int, LibMain)(HINSTANCE hInstance, DWORD dwReason, void *unused ) \
		{ if( dwReason == DLL_PROCESS_ATTACH ) {\
			/*Log( WIDE("Library Enter" ) );*//* here would be if dwReason == process_attach */
#define LIBEXIT() } /* end if */ if( dwReason == DLL_PROCESS_DETACH ) {  \
	   									 /*Log( WIDE("Library Exit" ) );*/
#define LIBMAIN_END()  } return 1; }
#endif

// should use this define for all local defines...
// this will allow one place to modify ALL _pascal or not to _pascal decls.
#define STDPROC _pascal
#define PACKED
#endif

#define TOCHR(n) #n[0]
#define TOSTR(n) WIDE(#n)
#define STRSYM(n) TOSTR(n)

#define _WIDE__FILE__(n) WIDE(n)
#define WIDE__FILE__ _WIDE__FILE__(__FILE__)

/* a constant text string that represents the current source
   filename and line... fourmated as "source.c(11) :"        */
#define FILELINE  TEXT(__FILE__) WIDE("(" ) TEXT(STRSYM(__LINE__))WIDE(" : " ))
#if defined( _MSC_VER ) || defined( __PPCCPP__ )
/* try and define a way to emit comipler messages... but like no compilers support standard ways to do this accross the board.*/
#define pragnote(msg) message( FILELINE msg )
/* try and define a way to emit comipler messages... but like no compilers support standard ways to do this accross the board.*/
#define pragnoteonly(msg) message( msg )
#else
/* try and define a way to emit comipler messages... but like no compilers support standard ways to do this accross the board.*/
#define pragnote(msg) msg
/* try and define a way to emit comipler messages... but like no compilers support standard ways to do this accross the board.*/
#define pragnoteonly(msg) msg
#endif

/* specify a consistant macro to pass current file and line information.   This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_SRC         , (CTEXTSTR)_WIDE(__FILE__), __LINE__
/* specify a consistant macro to pass current file and line information, to functions which void param lists.   This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_VOIDSRC     (CTEXTSTR)_WIDE(__FILE__), __LINE__
//#define FILELINE_LEADSRC     (CTEXTSTR)_WIDE(__FILE__), __LINE__,
/* specify a consistant macro to define file and line parameters, to functions with otherwise void param lists.  This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_VOIDPASS    CTEXTSTR pFile, uint32_t nLine
//#define FILELINE_LEADPASS    CTEXTSTR pFile, uint32_t nLine,
/* specify a consistant macro to define file and line parameters.   This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_PASS        , CTEXTSTR pFile, uint32_t nLine
/* specify a consistant macro to forward file and line parameters.   This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_RELAY       , pFile, nLine
/* specify a consistant macro to forward file and line parameters, to functions which have void parameter lists without this information.  This are appended parameters, and common usage is to only use these with _DEBUG set. */
#define FILELINE_VOIDRELAY   pFile, nLine
/* specify a consistant macro to format file and line information for printf formated strings. */
#define FILELINE_FILELINEFMT WIDE("%s(%") _32f WIDE("): ")
#define FILELINE_FILELINEFMT_MIN WIDE("%s(%") _32f WIDE(")")
#define FILELINE_NULL        , NULL, 0
#define FILELINE_VOIDNULL    NULL, 0
/* define static parameters which are the declaration's current file and line, for stubbing in where debugging is being stripped.
  usage
    FILELINE_VARSRC: // declare pFile and nLine variables.
	*/
#define FILELINE_VARSRC       CTEXTSTR pFile = _WIDE(__FILE__); uint32_t nLine = __LINE__

// this is for passing FILE, LINE information to allocate
// useful during DEBUG phases only...

// drop out these debug relay paramters for managed code...
// we're going to have the full call frame managed and known...
#if !defined( _DEBUG ) && !defined( _DEBUG_INFO )
#  if defined( __LINUX__ ) && !defined( __PPCCPP__ )
//#warning "Setting DBG_PASS and DBG_FORWARD to be ignored."
#  else
//#pragma pragnoteonly("Setting DBG_PASS and DBG_FORWARD to be ignored"  )
#  endif
#define DBG_AVAILABLE   0
/* in NDEBUG mode, pass nothing */
#define DBG_SRC
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
#define DBG_VOIDSRC
/* <combine sack::DBG_PASS>

   \#define DBG_LEADSRC in NDEBUG mode, declare (void) */
/* <combine sack::DBG_PASS>

   \ \                      */
#define DBG_VOIDPASS    void
/* <combine sack::DBG_PASS>
   in NDEBUG mode, pass nothing */
#define DBG_PASS
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
#define DBG_RELAY
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
#define DBG_VOIDRELAY
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
#define DBG_FILELINEFMT
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
#define DBG_FILELINEFMT_MIN
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing
   Example
   printf( DBG_FILELINEFMT ": extra message" DBG_PASS ); */
#define DBG_VARSRC
#else
	// these DBG_ formats are commented out from duplication in sharemem.h
#  if defined( __LINUX__ ) && !defined( __PPCCPP__ )
//#warning "Setting DBG_PASS and DBG_FORWARD to work."
#  else
//#pragma pragnoteonly("Setting DBG_PASS and DBG_FORWARD to work"  )
#  endif
// used to specify whether debug information is being passed - can be referenced in compiled code
#define DBG_AVAILABLE   1
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_SRC */
#define DBG_SRC         FILELINE_SRC
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_VOIDSRC */
#define DBG_VOIDSRC     FILELINE_VOIDSRC
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_VOIDPASS */
#define DBG_VOIDPASS    FILELINE_VOIDPASS
/* <combine sack::DBG_PASS>

   in NDEBUG mode, pass nothing */
/* Example
   This example shows forwarding debug information through a
   chain of routines.
   <code lang="c++">
   void ReportFunction( int sum DBG_PASS )
   {
       printf( "%s(%d):started this whole mess\\n" DBG_RELAY );
   }

   void TrackingFunction( int a, int b DBG_PASS )
   {
       ReportFunction( a+b, DBG_RELAY );
   }

   void CallTrack( void )
   {
       TrackingFunction( 1, 2 DBG_SRC );
   }
   </code>

   In this example, the debug information is passed to the
   logging system. This allows logging to blame the user
   application for allocations, releases, locks, etc...
   <code lang="c++">
   void MyAlloc( int size DBG_PASS )
   {
       _lprintf( DBG_RELAY )( ": alloc %d\\n", size );
   }
   void g( void )
   {
       lprintf( "Will Allocate %d\\n", 32 );
       MyAlloc( 32 DBG_SRC );
   }

   </code>

   This example uses the void argument macros
   <code>

   void SimpleFunction( DBG_VOIDPASS )
   {
       // this function usually has (void) parameters.
   }

   void f( void )
   {
       SimpleFunction( DBG_VOIDSRC );
   }
   </code>
   Description
   in NDEBUG mode, pass nothing.



   This function allows specification of DBG_RELAY or DBG_SRC
   under debug compilation. Otherwise, the simple AddLink macro
   should be used. DBG_RELAY can be used to forward file and
   line information which has been passed via DBG_PASS
   declaration in the function parameters.



   This is a part of a set of macros which allow additional
   logging information to be passed.



   These 3 are the most commonly used.



   DBG_SRC - this passes the current __FILE__, __LINE__
   \parameters.

   DBG_PASS - this is used on a function declaration, is a
   filename and line number from DBG_SRC or DBG_RELAY.

   DBG_RELAY - this passes the file and line passed to this
   function to another function with DBG_PASS defined on it.



   DBG_VOIDPASS - used when the argument list is ( void )
   without debugging information.

   DBG_VOIDSRC - used to call a function who's argument list is
   ( void ) without debugging information.

   DBG_VOIDRELAY - pass file and line information forward to
   another function, who's argument list is ( void ) without
   debugging information.
   Remarks
   The SACK library is highly instrumented with this sort of
   information. Very commonly the only difference between a
   specific function called 'MyFunctionName' and
   'MyFunctionNameEx' is the addition of debug information
   tracking.



   The following code blocks show the evolution added to add
   instrumentation...

   <code lang="c++">
   int MyFunction( int param )
   {
       // do stuff
   }

   int CallingFunction( void )
   {
       return MyFunction();
   }
   </code>

   Pretty simple code, a function that takes a parameter, and a
   function that calls it.

   The first thing is to extend the called function.
   <code>
   int MyFunctionEx( int param DBG_PASS )
   {
       // do stuff
   }
   </code>

   And provide a macro for everyone else calling the function to
   automatically pass their file and line information
   <code lang="c++">
   \#define MyFunction(param)  MyFunctionEx(param DBG_SRC)
   </code>

   Then all-together
   <code>
   \#define MyFunction(param)  MyFunctionEx(param DBG_SRC)

   int MyFunctionEx( int param DBG_PASS )
   {
       // do stuff
   }


   int CallingFunction( void )
   {
       // and this person calling doesn't matter
       // does require a recompile of source.
       return MyFunction( 3 );
   }

   </code>

   But then... what if CallingFunction decided wasn't really the
   one at fault, or responsible for the allocation, or other
   issue being tracked, then she could be extended....
   <code>
   int CallingFunctionEx( DBG_VOIDPASS )
   \#define CallingFunction() CallingFunction( DBG_VOIDSRC )
   {
       // and this person calling doesn't matter
       // does require a recompile of source.
       return MyFunction( 1 DBG_RELAY );
   }
   </code>

   Now, calling function will pass it's callers information to
   MyFunction....



   Why?



   Now, when you call CreateList, your code callng the list
   creation method is marked as the one who allocates the space.
   Or on a DeleteList, rather than some internal library code
   being blamed, the actual culprit can be tracked and
   identified, because it's surely not the fault of CreateList
   that the reference to the memory for the list wasn't managed
   correctly.
   Note
   It is important to note, every usage of these macros does not
   have a ',' before them. This allows non-debug code to
   eliminate these extra parameters cleanly. If the ',' was
   outside of the macro, then it would remain on the line, and
   an extra parameter would have be be passed that was unused.
   This is also why DBG_VOIDPASS exists, because in release mode
   this is substituted with 'void'.



   In Release mode, DBG_VOIDRELAY becomes nothing, but when in
   debug mode, DBG_RELAY has a ',' in the macro, so without a
   paramter f( DBG_RELAY ) would fail; on expansion this would
   be f( , pFile, nLine ); (note the extra comma, with no
   parameter would be a syntax error.                            */
#define DBG_PASS        FILELINE_PASS
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_RELAY */
#define DBG_RELAY       FILELINE_RELAY
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_VOIDRELAY */
#define DBG_VOIDRELAY   FILELINE_VOIDRELAY
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_FILELINEFMT */
#define DBG_FILELINEFMT FILELINE_FILELINEFMT
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_FILELINEFMT_MIN */
#define DBG_FILELINEFMT_MIN FILELINE_FILELINEFMT_MIN
/* <combine sack::DBG_PASS>

   in _DEBUG mode, pass FILELINE_VARSRC */
#define DBG_VARSRC      FILELINE_VARSRC

#endif


// cannot declare _0 since that overloads the
// vector library definition for origin (0,0,0,0,...)
//typedef void             _0; // totally unusable to declare 0 size things.
/* the only type other than when used in a function declaration that void is valid is as a pointer to void. no _0 type exists (it does, but it's in vectlib, and is an origin vector)*/
typedef void             *P_0;

/*
 * several compilers are rather picky about the types of data
 * used for bit field declaration, therefore this type
 * should be used instead of uint32_t (DWORD)
 */
typedef unsigned int  BIT_FIELD;
// have to do this on a per structure basis - otherwise
// any included headers with structures to use will get FUCKED
#ifndef PACKED
#warning NO PREVIOUS deintion of PACKED...
#define PACKED
#endif

/* An pointer to a volatile unsigned integer type that is 64 bits long. */
//typedef volatile uint64_t  *volatile int64_t*;
/* An pointer to a volatile pointer size type that is as long as a pointer. */
typedef volatile uintptr_t        *PVPTRSZVAL;

/* an unsigned type meant to index arrays.  (By convention, arrays are not indexed negatively.)  An index which is not valid is INVALID_INDEX, which equates to 0xFFFFFFFFUL or negative one cast as an INDEX... ((INDEX)-1). */
typedef size_t         INDEX;
/* An index which is not valid; equates to 0xFFFFFFFFUL or negative one cast as an INDEX... ((INDEX)-1). */
#define INVALID_INDEX ((INDEX)-1)

#ifdef __CYGWIN__
typedef unsigned short wchar_t;
#endif
// may consider changing this to uint16_t* for unicode...
typedef wchar_t X_16;
/* This is a pointer to wchar_t. A 16 bit value that is
   character data, and is not signed or unsigned.       */
typedef wchar_t *PX_16;

#if defined( UNICODE ) || defined( SACK_COM_OBJECT )
//should also consider revisiting code that was updated for TEXTCHAR to char conversion methods...
#  ifdef _MSC_VER
#    ifdef UNDER_CE
#      define NULTERM
#    else
#      define NULTERM __nullterminated
#    endif
#  else
#    define NULTERM
#  endif
#define WIDE(s)  L##s
#define _WIDE(s)  WIDE(s)
#define cWIDE(s)  s
#define _cWIDE(s)  cWIDE(s)
typedef NULTERM          const X_16      *CTEXTSTR; // constant text string content
typedef NULTERM          CTEXTSTR        *PCTEXTSTR; // pointer to constant text string content
typedef NULTERM          X_16            *TEXTSTR;
/* a text 16 bit character  */
typedef X_16             TEXTCHAR;

#else
#define WIDE(s)   s
#define _WIDE(s)  s
#define cWIDE(s)   s
/* Modified WIDE wrapper that actually forces non-unicode
   string.                                                */
#define _cWIDE(s)  s
// constant text string content
typedef const char     *CTEXTSTR;
/* A non constant array of TEXTCHAR. A pointer to TEXTCHAR. A
   pointer to non-constant characters. (A non-static string
   probably)                                                  */
typedef char           *TEXTSTR;
#if defined( __LINUX__ ) && defined( __cplusplus )
// pointer to constant text string content
typedef TEXTSTR const  *PCTEXTSTR;
#else
// char const *const *
typedef CTEXTSTR const *PCTEXTSTR;
#endif
/* a text 8 bit character  */
typedef char            TEXTCHAR;
#endif
/* a character rune.  Strings should be interpreted as UTF-8 or 16 depending on UNICODE compile option.
   GetUtfChar() from strings.  */
typedef uint32_t             TEXTRUNE;


//typedef enum { FALSE, TRUE } LOGICAL; // smallest information
#ifndef FALSE
#define FALSE 0
/* Define TRUE when not previously defined in the platform. TRUE
   is (!FALSE) so anything not 0 is true.                        */
#define TRUE (!FALSE)
#endif
/* Meant to hold boolean and only boolean values. Should be
   implemented per-platform as appropriate for the bool type the
   compiler provides.                                            */
typedef uint32_t LOGICAL;

/* This is a pointer. It is a void*. It is meant to point to a
   single thing, and cannot be used to reference arrays of bytes
   without recasting.                                            */
typedef P_0 POINTER;
/* This is a pointer to constant data. void const *. Compatible
   with things like char const *.                               */
typedef const void *CPOINTER;

SACK_NAMESPACE_END

//------------------------------------------------------
// formatting macro defintions for [vsf]printf output of the above types
#ifndef _MSC_VER
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

SACK_NAMESPACE

/* 16 bit unsigned decimal output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _16f   WIDE("u" )
/* 16 bit hex output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _16fx   WIDE("x" )
/* 16 bit HEX output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _16fX   WIDE("X" )
/* 16 bit signed decimal output printf format specifier. This
   would otherwise be defined in \<inttypes.h\>               */
#define _16fs   WIDE("d" )

/* 8 bit unsigned decimal output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _8f   WIDE("u" )
/* 8 bit hex output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _8fx   WIDE("x" )
/* 8 bit HEX output printf format specifier. This would
   otherwise be defined in \<inttypes.h\>                */
#define _8fX   WIDE("X" )
/* 8 bit signed decimal output printf format specifier. This
   would otherwise be defined in \<inttypes.h\>               */
#define _8fs   WIDE("d" )

#if defined( __STDC_FORMAT_MACROS )

#  define _32f   _WIDE( PRIu32 )
#  define _32fx   _WIDE( PRIx32 )
#  define _32fX   _WIDE( PRIX32 )
#  define _32fs   _WIDE( PRId32 )

#  define _64f    _WIDE(PRIu64)
#  define _64fx   _WIDE(PRIx64)
#  define _64fX   _WIDE(PRIX64)
#  define _64fs   _WIDE(PRId64)

// non-unicode strings
#  define c_32f    PRIu32
#  define c_32fx   PRIx32
#  define c_32fX   PRIX32
#  define c_32fs   PRId32
#  define c_64f    PRIu64
#  define c_64fx   PRIx64
#  define c_64fX   PRIX64
#  define c_64fs   PRId64

#else
#  define _32f   WIDE("u" )
#  define _32fx   WIDE("x" )
#  define _32fX   WIDE("X" )
#  define _32fs   WIDE("d" )

#  define c_32f   "u"
#  define c_32fx  "x"
#  define c_32fX  "X"
#  define c_32fs  "d"
#  define c_64f    "llu"
#  define c_64fx   "llx"
#  define c_64fX   "llX"
#  define c_64fs   "lld"

#endif

#if defined( UNICODE )
#  define _cstring_f WIDE("S")
#  define _string_f WIDE("s")
#  define _ustring_f WIDE("s")
#else
#  define _cstring_f WIDE("s")
#  define _string_f WIDE("s")
#  define _ustring_f WIDE("S")
#endif

#if defined( __64__ )

#  if defined( __STDC_FORMAT_MACROS )

#    if !defined( __GNUC__ ) || defined( _WIN32 )
#      define _size_f    _WIDE( PRIu64 )
#      define _size_fx   _WIDE( PRIx64 )
#      define _size_fX   _WIDE( PRIX64 )
#      define _size_fs   _WIDE( PRId64 )
#      define c_size_f    PRIu64
#      define c_size_fx   PRIx64
#      define c_size_fX   PRIX64
#      define c_size_fs   PRId64
#    else
#      define _size_f    WIDE( "zu" )
#      define _size_fx   WIDE( "zx" )
#      define _size_fX   WIDE( "zX" )
#      define _size_fs   WIDE( "zd" )
#      define c_size_f    "zu"
#      define c_size_fx   "zx"
#      define c_size_fX   "zX"
#      define c_size_fs   "zd"
#    endif

#    define _PTRSZVALfs _WIDE( PRIu64 )
#    define _PTRSZVALfx _WIDE( PRIx64 )
#    define cPTRSZVALfs PRIu64
#    define cPTRSZVALfx PRIx64

#  else
#    if !defined( __GNUC__ ) || defined( _WIN32 )
#      define _size_f    _64f
#      define _size_fx   _64fx
#      define _size_fX   _64fX
#      define _size_fs   _64fs
#      define c_size_f   c_64f
#      define c_size_fx  c_64fx
#      define c_size_fX  c_64fX
#      define c_size_fs  c_64fs
#    else
#      define _size_f    WIDE( "zu" )
#      define _size_fx   WIDE( "zx" )
#      define _size_fX   WIDE( "zX" )
#      define _size_fs   WIDE( "zd" )
#      define c_size_f    "zu"
#      define c_size_fx   "zx"
#      define c_size_fX   "zX"
#      define c_size_fs   "zd"
#    endif

#    define _PTRSZVALfs _64fs
#    define _PTRSZVALfx _64fx
#    define cPTRSZVALfs c_64fs
#    define cPTRSZVALfx c_64fx
#  endif

#else

#  if defined( __STDC_FORMAT_MACROS )

      // this HAS been fixed in UCRT - 2015!  but it'll take 5 years before everyone has that...
#    if !defined( __GNUC__ ) || defined( _WIN32 )
#      define _size_f    _WIDE( PRIu32 )
#      define _size_fx   _WIDE( PRIx32 )
#      define _size_fX   _WIDE( PRIX32 )
#      define _size_fs   _WIDE( PRId32 )
#      define c_size_f    PRIu32
#      define c_size_fx   PRIx32
#      define c_size_fX   PRIX32
#      define c_size_fs   PRId32
#    else
#      define _size_f    WIDE( "zu" )
#      define _size_fx   WIDE( "zx" )
#      define _size_fX   WIDE( "zX" )
#      define _size_fs   WIDE( "zd" )
#      define c_size_f    "zu"
#      define c_size_fx   "zx"
#      define c_size_fX   "zX"
#      define c_size_fs   "zd"
#    endif

#    define _PTRSZVALfs _WIDE( PRIu32 )
#    define _PTRSZVALfx _WIDE( PRIx32 )
#    define cPTRSZVALfs PRIu32
#    define cPTRSZVALfx PRIx32
#  else

      // this HAS been fixed in UCRT - 2015!  but it'll take 5 years before everyone has that...
#    if !defined( __GNUC__ ) || defined( _WIN32 )
#      define _size_f    _32f
#      define _size_fx   _32fx
#      define _size_fX   _32fX
#      define _size_fs   _32fs
#      define c_size_f    c_32f
#      define c_size_fx   c_32fx
#      define c_size_fX   c_32fX
#      define c_size_fs   c_32fs
#    else
#      define _size_f    WIDE( "zu" )
#      define _size_fx   WIDE( "zx" )
#      define _size_fX   WIDE( "zX" )
#      define _size_fs   WIDE( "zd" )
#      define c_size_f    "zu"
#      define c_size_fx   "zx"
#      define c_size_fX   "zX"
#      define c_size_fs   "zd"
#    endif

#    define _PTRSZVALfs _32fs
#    define _PTRSZVALfx _32fx
#    define cPTRSZVALfs c_32fs
#    define cPTRSZVALfx c_32fs
#  endif
#endif

#define PTRSZVALf WIDE("p" )
#define _PTRSZVALf WIDE("p" )

#if defined( _MSC_VER )
/* 64 bit unsigned decimal output printf format specifier. This would
   otherwise be defined in \<inttypes.h\> as PRIu64              */
#define _64f    WIDE("llu")
/* 64 bit hex output printf format specifier. This would
   otherwise be defined in \<inttypes.h\> as PRIxFAST64                */
#define _64fx   WIDE("llx")
/* 64 bit HEX output printf format specifier. This would
   otherwise be defined in \<inttypes.h\> as PRIxFAST64                */
#define _64fX   WIDE("llX")
/* 64 bit signed decimal output printf format specifier. This
   would otherwise be defined in \<inttypes.h\> as PRIdFAST64               */
#define _64fs   WIDE("lld")
#endif



// This should be for several years a
// sufficiently large type to represent
// threads and processes.
typedef uint64_t THREAD_ID;
#define GetMyThreadIDNL GetMyThreadID

#if defined( _WIN32 ) || defined( __CYGWIN__ )
#define _GetMyThreadID()  ( (( ((uint64_t)GetCurrentProcessId()) << 32 ) | ( (uint64_t)GetCurrentThreadId() ) ) )
#define GetMyThreadID()  (GetThisThreadID())
#else
// this is now always the case
// it's a safer solution anyhow...
#ifndef GETPID_RETURNS_PPID
#define GETPID_RETURNS_PPID
#endif
#ifdef GETPID_RETURNS_PPID
#ifdef __ANDROID__
#define GetMyThreadID()  (( ((uint64_t)getpid()) << 32 ) | ( (uint64_t)(gettid()) ) )
#else
#define GetMyThreadID()  (( ((uint64_t)getpid()) << 32 ) | ( (uint64_t)(pthread_self()) ) )
#endif
#else
#define GetMyThreadID()  (( ((uint64_t)getppid()) << 32 ) | ( (uint64_t)(getpid()|0x40000000)) )
#endif
#define _GetMyThreadID GetMyThreadID
#endif

//#error blah
// general macros for linking lists using
#define DeclareLink( type )  type *next;type **me

#define RelinkThing( root, node )   \
	((( node->me && ( (*node->me)=node->next ) )?  \
	node->next->me = node->me:0),(node->next = NULL),(node->me = NULL),node), \
	((( node->next = root )?        \
	(root->me = &node->next):0),  \
	(node->me = &root),             \
	(root = node) )

/* Link a new node into the list.


   Example
   struct mynode

   {

   DeclareLink( struct mynode );

   } *node;



   struct mynode *list;

   LinkThing( list_root, node );  */
#define LinkThing( root, node )     \
		((( (node)->next = (root) )?        \
	(((root)->me) = &((node)->next)):0),  \
	(((node)->me) = &(root)),             \
	((root) = (node)) )

/* Link a node to the end of a list. Link thing inserts the new
   node as the new head of the list.                            */
#define LinkLast( root, type, node ) if( node ) do { if( !root ) \
	{ root = node; (node)->me=&root; } \
	else { type tmp; \
	for( tmp = root; tmp->next; tmp = tmp->next ); \
	tmp->next = (node); \
	(node)->me = &tmp->next; \
	} } while (0)


// put 'Thing' after 'node'
#define LinkThingAfter( node, thing ) \
	( ( (thing)&&(node))   \
	?(((((thing)->next = (node)->next))?((node)->next->me = &(thing)->next):0) \
	 ,((thing)->me = &(node)->next), ((node)->next = thing))  \
	:((node)=(thing)) )


//
// put 'Thing' before 'node'... so (*node->me) = thing
#define LinkThingBefore( node, thing ) \
	{  \
thing->next = (*node->me);\
	(*node->me) = thing;    \
thing->me = node->me;       \
node->me = &thing->next;     \
}

/* Remove a node from a list. Requires only the node. */
#define UnlinkThing( node )                      \
	((( (node) && (node)->me && ( (*(node)->me)=(node)->next ) )?  \
	(node)->next->me = (node)->me:0),((node)->next = NULL),((node)->me = NULL),(node))

// this has two expressions duplicated...
// but in being so safe in this expression,
// the self-circular link needs to be duplicated.
// GrabThing is used for nodes which are circularly bound
#define GrabThing( node )    \
	((node)?(((node)->me)?(((*(node)->me)=(node)->next)? \
	((node)->next->me=(node)->me),((node)->me=&(node)->next):NULL):((node)->me=&(node)->next)):NULL)

/* Go to the next node with links declared by DeclareLink */
#define NextLink(node) ((node)?(node)->next:NULL)
// everything else is called a thing... should probably migrate to using this...
#define NextThing(node) ((node)?(node)->next:NULL)
//#ifndef FALSE
//#define FALSE 0
//#endif

//#ifndef TRUE
//#define TRUE (!FALSE)
//#endif


/* the default type to use for flag sets - flag sets are arrays of bits which can be toggled on and off by an index. */
#define FLAGSETTYPE uint32_t
/* the number of bits a specific type is.
   Example
   int bit_size_int = FLAGTYPEBITS( int ); */
#define FLAGTYPEBITS(t) (sizeof(t)*CHAR_BIT)
/* how many bits to add to make sure we round to the next greater index if even 1 bit overflows */
#define FLAGROUND(t) (FLAGTYPEBITS(t)-1)
/* the index of the FLAGSETTYPE which contains the bit in question */
#define FLAGTYPE_INDEX(t,n)  (((n)+FLAGROUND(t))/FLAGTYPEBITS(t))
/* how big the flag set is in count of FLAGSETTYPEs required in a row ( size of the array of FLAGSETTYPE that contains n bits) */
#define FLAGSETSIZE(t,n) (FLAGTYPE_INDEX(t,n) * sizeof( FLAGSETTYPE ) )
// declare a set of flags...
#define FLAGSET(v,n)   FLAGSETTYPE (v)[((n)+FLAGROUND(FLAGSETTYPE))/FLAGTYPEBITS(FLAGSETTYPE)]
// set a single flag index
#define SETFLAG(v,n)   ( (v)[(n)/FLAGTYPEBITS((v)[0])] |= 1 << ( (n) & FLAGROUND((v)[0]) ))
// clear a single flag index
#define RESETFLAG(v,n) ( (v)[(n)/FLAGTYPEBITS((v)[0])] &= ~( 1 << ( (n) & FLAGROUND((v)[0]) ) ) )
// test if a flags is set
#define TESTFLAG(v,n)  ( (v)[(n)/FLAGTYPEBITS((v)[0])] & ( 1 << ( (n) & FLAGROUND((v)[0]) ) ) )
// reverse a flag from 1 to 0 and vice versa
#define TOGGLEFLAG(v,n)   ( (v)[(n)/FLAGTYPEBITS((v)[0])] ^= 1 << ( (n) & FLAGROUND((v)[0]) ))

// 32 bits max for range on mask
#define MASK_MAX_LENGTH 32
// gives a 32 bit mask possible from flagset..
#define MASKSET_READTYPE uint32_t
// gives byte index...
#define MASKSETTYPE uint8_t
/* how many bits the type specified can hold
   Parameters
   t :  data type to measure (int, uint32_t, ... ) */
#define MASKTYPEBITS(t) (sizeof(t)*CHAR_BIT)
/* the maximum number of bits storable in a type */
#define MASK_MAX_TYPEBITS(t) (sizeof(t)*CHAR_BIT)
/* round up to the next count of types that fits 1 bit - used as a cieling round factor */
#define MASKROUND(t) (MASKTYPEBITS(t)-1)
/* define MAX_MAX_ROUND factor based on MASKSET_READTYPE - how to read it... */
#define MASK_MAX_ROUND() (MASK_MAX_TYPEBITS(MASKSET_READTYPE)-1)
/* byte index of the start of the mask
   Parameters
   t :  type to measure with
   n :  mask index                     */
#define MASKTYPE_INDEX(t,n)  (((n)+MASKROUND(t))/MASKTYPEBITS(t))
/* The number of bytes the set would be.
   Parameters
   t :  the given type to measure with
   n :  the count of masks to fit.       */
#define MASKSETSIZE(t,n) (MASKTYPE_INDEX(t,(n+1)))
// declare a set of flags...

#define MASK_TOP_MASK_VAL(length,val) ((val)&( (0xFFFFFFFFUL) >> (32-(length)) ))
/* the mask in the dword resulting from shift-right.   (gets a mask of X bits in length) */
#define MASK_TOP_MASK(length) ( (0xFFFFFFFFUL) >> (32-(length)) )
/* the mast in the dword shifted to the left to overlap the field in the word */
#define MASK_MASK(n,length)   (MASK_TOP_MASK(length) << (((n)*(length))&0x7) )
// masks value with the mask size, then applies that mask back to the correct word indexing
#define MASK_MASK_VAL(n,length,val)   (MASK_TOP_MASK_VAL(length,val) << (((n)*(length))&0x7) )

/* declare a mask set. */
#define MASKSET(v,n,r)  MASKSETTYPE  (v)[(((n)*(r))+MASK_MAX_ROUND())/MASKTYPEBITS(MASKSETTYPE)]; const int v##_mask_size = r;
// set a field index to a value
#define SETMASK(v,n,val)    (((MASKSET_READTYPE*)((v)+((n)*(v##_mask_size))/MASKTYPEBITS((v)[0])))[0] =    \
( ((MASKSET_READTYPE*)((v)+((n)*(v##_mask_size))/MASKTYPEBITS(uint8_t)))[0]                                 \
 & (~(MASK_MASK(n,v##_mask_size))) )                                                                           \
	| MASK_MASK_VAL(n,v##_mask_size,val) )
// get the value of a field
#define GETMASK(v,n)  ( ( ((MASKSET_READTYPE*)((v)+((n)*(v##_mask_size))/MASKTYPEBITS((v)[0])))[0]                                 \
 & MASK_MASK(n,v##_mask_size) )                                                                           \
	>> (((n)*(v##_mask_size))&0x7))



/* This type stores data, it has a self-contained length in
   bytes of the data stored.  Length is in characters       */
_CONTAINER_NAMESPACE
#define DECLDATA(name,length) struct {size_t size; TEXTCHAR data[length];} name

// Hmm - this can be done with MemLib alone...
// although this library is not nessecarily part of that?
// and it's not nessecarily allocated.
typedef struct SimpleDataBlock {
   size_t size;/* unsigned size; size is sometimes a pointer value... this
                    means bad thing when we change platforms... Defined as
                    uintptr_t now, so it's relative to the size of the platform
                    anyhow.                                                    */
#ifdef _MSC_VER
#pragma warning (disable:4200)
#endif
   uint8_t  data[
#ifndef __cplusplus
   1
#endif
   ]; // beginning of var data - this is created size+sizeof(uint8_t)
#ifdef _MSC_VER
#pragma warning (default:4200)
#endif
} DATA, *PDATA;

/* This is a slab array of pointers, each pointer may be
   assigned to point to any user data.
   Remarks
   When the list is filled to the capacity of Cnt elements, the
   list is reallocated to be larger.



   Cannot add NULL pointer to list, empty elements in the list
   are represented with NULL, and may be filled by any non-NULL
   value.                                                       */
_LINKLIST_NAMESPACE

/* <combine sack::containers::list::LinkBlock>

   \ \                                         */
typedef struct LinkBlock
{
	/* How many pointers the list can contain now. */
	INDEX     Cnt;
	/* \ \  */
	POINTER pNode[1];
} LIST, *PLIST;

_LINKLIST_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::containers::list;
#endif

_DATALIST_NAMESPACE
/* a list of data structures... a slab array of N members of X size */
typedef struct DataBlock  DATALIST;
/* A typedef of a pointer to a DATALIST struct DataList. */
typedef struct DataBlock *PDATALIST;

/* Data Blocks are like LinkBlocks, and store blocks of data in
   slab format. If the count of elements exceeds available, the
   structure is grown, to always contain a continuous array of
   structures of Size size.
   Remarks
   When blocks are deleted, all subsequent blocks are shifted
   down in the array. So the free blocks are always at the end. */
struct DataBlock
{
	/* How many elements are used. */
	INDEX     Cnt;
	/* How many elements are available in his array. */
	INDEX     Avail;
	/* A simple exchange lock on the data for insert and delete. For
	   thread safety.                                                */
	//volatile uint32_t     Lock;
	/* How big each element of the array is. */
	INDEX     Size;
	/* The physical array. */
	uint8_t      data[1];
};
_DATALIST_NAMESPACE_END

/* This is a stack that contains pointers to user objects.
   Remarks
   This is a stack 'by reference'. When extended, the stack will
   occupy different memory, care must be taken to not duplicate
   pointers to this stack.                                       */
typedef struct LinkStack
{
	/* This is the index of the next pointer to be pushed or popped.
	   If top == 0, the stack is empty, until a pointer is added and
	   top is incremented.                                           */
	INDEX     Top;
	/* How many pointers the stack can contain. */
	INDEX     Cnt;
	/* thread interlock using InterlockedExchange semaphore. For
	                  thread safety.                                            */
	//volatile uint32_t     Lock;
	/*  a defined maximum capacity of stacked values... values beyond this are lost from the bottom  */
	uint32_t     Max;
	/* Reserved data portion that stores the pointers. */
	POINTER pNode[1];
} LINKSTACK, *PLINKSTACK;

/* A Stack that stores information in an array of structures of
   known size.
   Remarks
   The size of each element must be known at stack creation
   time. Structures are literally copied to and from this stack.
   This is a stack 'by value'. When extended, the stack will
   occupy different memory, care must be taken to not duplicate
   pointers to this stack.                                       */
typedef struct DataListStack
{
	volatile INDEX     Top; /* enable logging the program executable (probably the same for
	                all messages, unless they are network)
	                                                                             */
	INDEX     Cnt; // How many elements are on the stack.
	//volatile uint32_t     Lock;  /* thread interlock using InterlockedExchange semaphore. For
	//                  thread safety.                                            */
	INDEX     Size; /* Size of each element in the stack. */
	INDEX     Max; /* maximum elements in the stack */
	uint8_t      data[1]; /* The actual data area of the stack.  */
} DATASTACK, *PDATASTACK;

/* A queue which contains pointers to user objects. If the queue
   is filled to capacity and new queue is allocated, and all
   existing pointers are transferred.                            */
typedef struct LinkQueue
{
	/* This is the index of the next pointer to be added to the
	   queue. If Top==Bottom, then the queue is empty, until a
	   pointer is added to the queue, and Top is incremented.   */
	volatile INDEX     Top;
	/* This is the index of the next element to leave the queue. */
	volatile INDEX     Bottom;
	/* This is the current count of pointers that can be stored in
	   the queue.                                                  */
	INDEX     Cnt;
	/* thread interlock using InterlockedExchange semaphore. For
	   thread safety.                                            */
#if USE_CUSTOM_ALLOCER
	volatile uint32_t     Lock;
#endif
	POINTER pNode[2]; // need two to have distinct empty/full conditions
} LINKQUEUE, *PLINKQUEUE;

/* A queue of structure elements.
   Remarks
   The size of each element must be known at stack creation
   time. Structures are literally copied to and from this stack.
   This is a stack 'by value'. When extended, the stack will
   occupy different memory, care must be taken to not duplicate
   pointers to this stack.                                       */
typedef struct DataQueue
{
	/* This is the next index to be added to. If Top==Bottom, the
	   queue is empty, until an entry is added at Top, and Top
	   increments.                                                */
	volatile INDEX     Top;
	/* The current bottom index. This is the next one to be
	   returned.                                            */
	volatile INDEX     Bottom;
	/* How many elements the queue can hold. If a queue has more
	   elements added to it than it has count, it will be expanded,
	   and a new queue returned.                                    */
	INDEX     Cnt;
	/* thread interlock using InterlockedExchange semaphore */
	//volatile uint32_t     Lock;
	/* How big each element in the queue is. */
	INDEX     Size;
	/* How many elements to expand the queue by, when its capacity
	   is reached.                                                 */
	INDEX     ExpandBy;
	/* The data area of the queue. */
	uint8_t      data[1];
} DATAQUEUE, *PDATAQUEUE;

/* A mostly obsolete function, but can return the status of
   whether all initially scheduled startups are completed. (Or
   maybe whether we are not complete, and are processing
   startups)                                                   */
_CONTAINER_NAMESPACE_END
SACK_NAMESPACE_END

#include <sack_typelib.h>


SACK_NAMESPACE

#ifndef IS_DEADSTART
// this is always statically linked with libraries, so they may contact their
// core executable to know when it's done loading everyone else also...
#  ifdef __cplusplus
extern "C"
#  endif
#  if defined( WIN32 ) && !defined( __STATIC__ ) && !defined( __ANDROID__ )
#    ifdef __NO_WIN32API__
// DllImportAttribute ?
#    else
__declspec(dllimport)
#    endif
#  else
#ifndef __cplusplus
extern
#endif
#  endif
/* a function true/false which indicates whether the root
   deadstart has been invoked already. If not, one should call
   InvokeDeadstart and MarkDeadstartComplete.

   <code lang="c++">
   int main( )
   {
       if( !is_deadstart_complete() )
       {
           InvokeDeadstart();
           MarkDeadstartComplete()
       }

       ... your code here ....


       return 0;  // or some other appropriate return.
   }
   </code>

   sack::app::deadstart                                        */
LOGICAL
#  if defined( __WATCOMC__ )
__cdecl
#  endif
is_deadstart_complete( void );
#endif

/* Define a routine to call for exit().  This triggers specific code to handle shutdown event registration */
#ifndef NO_EXPORTS
#  ifdef SACK_BAG_CORE_EXPORTS
EXPORT_METHOD
#  else
IMPORT_METHOD
#  endif
#else
#  ifndef SACK_BAG_CORE_EXPORTS
	extern
#  endif
#endif
		void CPROC BAG_Exit( int code );
#ifndef NO_SACK_EXIT_OVERRIDE
#define exit(n) BAG_Exit(n)
#endif

SACK_NAMESPACE_END // namespace sack {

// this should become common to all libraries and programs...
#include <construct.h> // pronounced 'kahn-struct'
#include <logging.h>
#include <signed_unsigned_comparisons.h>

#ifdef __cplusplus
using namespace sack;
using namespace sack::containers;
#endif


#endif
