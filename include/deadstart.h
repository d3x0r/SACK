/* Deadstart interface. Deadstart is like bootstrap, and handles
   code that runs before main(). See <link deadstart>            */
#ifndef DEADSTART_DEFINED
#define DEADSTART_DEFINED

#ifdef WIN32
//#include <stdhdrs.h>
#endif
#include <sack_types.h>
#include <sack_typelib.h> // leach, assuming this will be compiled with this part at least.

#define pastejunk_(a,b) a##b
#define pastejunk(a,b) pastejunk_(a,b)

#ifdef __cplusplus

#  define USE_SACK_DEADSTART_NAMESPACE using namespace sack::app::deadstart;
#  define SACK_DEADSTART_NAMESPACE  SACK_NAMESPACE namespace app { namespace deadstart {
#  define SACK_DEADSTART_NAMESPACE_END  } } SACK_NAMESPACE_END

namespace sack{
	namespace app{
/* Application namespace. */
/* These are compiler-platform abstractions to provide a method
   of initialization that allows for creation of threads, and
   transparent (easy to use) method of scheduling routines for
   initialization.


   Example
   This schedules a routine to run at startup. Fill in the
   routine with the code you want, and it will run at
   DEFAULT_PRELOAD_PRIORITY which is the number 69.

   <code lang="c++">

   PRELOAD( MyCustomInit )
   {
       // do something here (do anything here,
       // without limitations that are imposed by DllMain/LibMain.
   }

   </code>

   If you wanted a routine which was guaranteed to run before
   MyCustomInit you might use PRIORITY_PRELOAD whcih allows you
   to specify a priority.
   <code lang="c++">

   PRIORITY_PRELOAD( MyOtherInit, DEFAULT_PRELOAD_PRIORITY-10 )
   {
      // this will run before other things.
   }

   </code>

   Priorities are listed in deadstart.h and exit_priorities.h. The
   priorities are treated backwards, so low number startup
   priorities go first, and higher number shutdown priorities go
   first.




   Remarks
   In some compilers and compile modes this is also fairly easy
   to do. A lot of compilers do not offer priority, and are
   impossible to maintain an order in. Some compilers only
   provide startup priority for C++ mode. This system works as
   \long as there is a way to run a single function at some
   point before main() and after C runtime initializes.



   In Windows, you might think you have this ability with
   DllMain, but there are severe limitations that you would have
   to get around; primary is the inability to create a thread,
   well, you can create it, but it will remain suspended until
   you leave DllMains and all DllMains finish. There is also no
   way to consistantly provide initialization order, like memory
   needs to be initialized before anything else.


                                                                   */

		namespace deadstart {
		}
	}
} //SACK_NAMESPACE_END   
#else
#define USE_SACK_DEADSTART_NAMESPACE
#define SACK_DEADSTART_NAMESPACE
#define SACK_DEADSTART_NAMESPACE_END
#endif

#ifdef TYPELIB_SOURCE
#define DEADSTART_SOURCE
#endif

#ifdef __cplusplus
namespace sack{
	namespace app{
		namespace deadstart {

//SACK_DEADSTART_NAMESPACE
#endif

/* A macro to specify the call type of schedule routines. This
   can be changed in most projects without affect, it comes into
   play if plugins built by different compilers are used,
   __cdecl is most standard.                                     */
#define DEADSTART_CALLTYPE CPROC

#  if defined( _TYPELIBRARY_SOURCE_STEAL )
#    define DEADSTART_PROC extern
#  elif defined( _TYPELIBRARY_SOURCE )
#    define DEADSTART_PROC EXPORT_METHOD
#  else
/* A definition for how to declare these functions. if the
   source itself is comipling these are _export, otherwise
   external things linking here are _import.               */
#    define DEADSTART_PROC IMPORT_METHOD
#  endif


     // 28 (thread ID for critical sections used to allocate memory)
#define TIMER_MODULE_PRELOAD_PRIORITY  (CONFIG_SCRIPT_PRELOAD_PRIORITY-3)

     // 30 specify where to load external resources from... like the option database
#define VIRTUAL_FILESYSTEM_PRELOAD_PRIORITY (CONFIG_SCRIPT_PRELOAD_PRIORITY-1)

   /* this is just a global space initializer (shared, named
      region, allows static link plugins to share information)



      Allocates its shared memory global region, so if this library
      is built statically and referenced in multiple plugins
      ConfigScript can share the same symbol tables. This also
		provides sharing between C++ and C.                           */
         // 31
#define CONFIG_SCRIPT_PRELOAD_PRIORITY    (SQL_PRELOAD_PRIORITY-3)
			// this is just a global space initializer (shared, named region, allows static link plugins to share information)
         // 34
#define SQL_PRELOAD_PRIORITY    (SYSLOG_PRELOAD_PRIORITY-1)
/* Level at which logging is initialized. Nothing under this
   should be doing logging, if it does, the behavior is not as
   well defined.                                               */
#define SYSLOG_PRELOAD_PRIORITY 35
   // global_init_preload_priority-1 is used by sharemem.. memory needs init before it can register itself
#define GLOBAL_INIT_PRELOAD_PRIORITY 37

#define OSALOT_PRELOAD_PRIORITY (CONFIG_SCRIPT_PRELOAD_PRIORITY-1) // OS A[bstraction] L[ayer] O[n] T[op] - system lib

/* Level which names initializes. Names is the process
   registration code. It has a common shared global registered.

   <link sack::app::registry, procreg; aka names.c>             */
#define NAMESPACE_PRELOAD_PRIORITY 39
/* image_preload MUST be after Namespce preload (anything that
   uses RegisterAndCreateGlobal) should init this before vidlib
   (which needs image?)                                         */
#define IMAGE_PRELOAD_PRIORITY  45
/* Level at which the video render library performs its
   initialization; RegisterClass() level code.          */
#define VIDLIB_PRELOAD_PRIORITY 46
/* Initialization level where PSI registers its builtin
   controls.                                            */
#define PSI_PRELOAD_PRIORITY    47

// need to open the queues and threads before the service server can begin...
#define MESSAGE_CLIENT_PRELOAD_PRIORITY 65
/* Level which message core service initializes. During startup
   message services can register themselves also; but not before
   this priority level.                                          */
#define MESSAGE_SERVICE_PRELOAD_PRIORITY 66
/* Routines are scheduled at this priority when the PRELOAD
   function is used.                                        */
#define DEFAULT_PRELOAD_PRIORITY (DEADSTART_PRELOAD_PRIORITY-1)
/* Not sure where this is referenced, this the core routine
   itself is scheduled with this symbol to the compiler if
   appropriate.                                             */
#define DEADSTART_PRELOAD_PRIORITY 70

#define PRIORITY_UNLOAD(proc,priority) PRIORITY_ATEXIT( proc##_unload, priority )
/* Used by PRELOAD and PRIORITY_PRELOAD macros to register a
   startup routine at a specific priority. Lower number
   priorities are scheduled to run before higher number
   priorities*backwards from ATEXIT priorities*. Using this
   scheduling mechanisms, routines which create threads under
   windows are guaranteed to run before main, and are guaranteed
   able to create threads. (They are outside of the loader lock)


   Parameters
   function :  pointer to a function to call at startup.
   name :      text name of the function
   priority :  priority at which to call the function.
   unused :    this is an unused parameter. The macros fill it
               with &amp;ThisRegisteringRoutine, so that the
               routine itself is referenced by code, and helps
               the compile not optimize out this code. The
               functions which perform the registration are prone
               to be optimized because it's hard for the compiler
               to identify that they are refernced by other names
               indirectly.
   file\ :     usually DBG_PASS of the code doing this
               registration.
   line :      usually DBG_PASS of the code doing this
               registration.                                      */
DEADSTART_PROC  void DEADSTART_CALLTYPE  RegisterPriorityStartupProc( void(CPROC*)(void), CTEXTSTR,int,void* unused DBG_PASS);
/* Used by ATEXIT and PRIORITY_ATEXIT macros to register a
   shutdown routine at a specific priority. Higher number
   priorities are scheduled to run before lower number
   priorities. *backwards from PRELOAD priorities* This
   registers functions which are run while the program exits if
   it is at all able to run when exiting. calling exit() or
   BAG_Exit() will invoke these.
   Parameters
   function :  pointer to a function to call at shutdown.
   name :      text name of the function
   priority :  priority at which to call the function.
   unused :    this is an unused parameter. The macros fill it
               with &amp;ThisRegisteringRoutine, so that the
               routine itself is referenced by code, and helps
               the compile not optimize out this code. The
               functions which perform the registration are prone
               to be optimized because it's hard for the compiler
               to identify that they are refernced by other names
               indirectly.
   file\ :     usually DBG_PASS of the code doing this
               registration.
   line :      usually DBG_PASS of the code doing this
               registration.                                      */
DEADSTART_PROC  void DEADSTART_CALLTYPE  RegisterPriorityShutdownProc( void(CPROC*)(void), CTEXTSTR,int,void* unused DBG_PASS);
/* This routine is used internally when LoadFunction is called.
   After MarkDeadstartComplete is called, any call to a
   RegisterPriorityStartupProc will call the startup routine
   immediately instead of waiting. This function disables the
   auto-running of this function, and instead enques the startup
   to the list of startups. When completed, at some later point,
   call ResumeDeadstart() to dispatched all scheduled routines,
   and release the suspend; however, if initial deastart was not
   dispatched, then ResumeDeadstart does not do the invoke, it
   only releases the suspend.                                    */
DEADSTART_PROC  void DEADSTART_CALLTYPE  SuspendDeadstart ( void );
/* Resumes a suspended deadstart. If root deadstart is
   completed, then ResumeDeadstart will call InvokeDeadstarts
   after resuming deadstart.                                  */
DEADSTART_PROC  void DEADSTART_CALLTYPE  ResumeDeadstart ( void );
/* Not usually used by user code, but this invokes all the
   routines which have been scheduled to run for startup. If
   your compiler doesn't have a method of handling deadstart
   code, this can be manually called. It can also be called if
   you loaded a library yourself without using the LoadFunction
   interface, to invoke startups scheduled in the loaded
   library.                                                     */
DEADSTART_PROC  void DEADSTART_CALLTYPE  InvokeDeadstart (void);
/* This just calls the list of shutdown procedures. This should
   not be used usually from user code, since internally this is
   handled by catching atexit() or with a static destructor.    */
DEADSTART_PROC  void DEADSTART_CALLTYPE  InvokeExits (void);
/* This is typically called after the first InvokeDeadstarts
   completes. The code that runs this is usually a routine just
   before main(). So once code in main begins to run, all prior
   initialization has been performed.                           */
DEADSTART_PROC  void DEADSTART_CALLTYPE  MarkRootDeadstartComplete ( void );
/* \returns whether InvokeDeadstarts has been called. */
DEADSTART_PROC  LOGICAL DEADSTART_CALLTYPE  IsRootDeadstartStarted ( void );
/* \returns whether MarkRootDeadstartComplete has been called. */
DEADSTART_PROC  LOGICAL DEADSTART_CALLTYPE  IsRootDeadstartComplete ( void );

/*
   Setup flags to ignore control C Events on windows.  use 1 << (ControlType) or'd together to set ignore.
   Use 0 to clear ignore.
*/
DEADSTART_PROC void DEADSTART_CALLTYPE IgnoreBreakHandler( int ignore );

#if defined( __LINUX__ )
// call this after a fork().  Otherwise, it will falsely invoke shutdown when it exits.
DEADSTART_PROC  void DEADSTART_CALLTYPE  DispelDeadstart ( void );
#endif

#ifdef DOC_O_MAT
// call this after a fork().  Otherwise, it will falsely invoke shutdown when it exits.
DEADSTART_PROC  void DEADSTART_CALLTYPE  DispelDeadstart ( void );
#endif

#ifdef __cplusplus

/* Defines some code to run at program inialization time. Allows
   specification of a priority. Lower priorities run first. (default
   is 69).
   Example
   <code>

   PRIORITY_PRELOAD( MyOtherInit, 153 )
   {
      // run some code probably after most all other initializtion is done.
   }

   </code>
   See Also
   <link sack::app::deadstart, deadstart Namespace>                         */
#define PRIORITY_PRELOAD(name,priority) static void CPROC name(void); \
	namespace { static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {    \
	RegisterPriorityStartupProc( name,TOSTR(name),priority,(void*)this DBG_SRC);\
	  }  \
	} pastejunk(do_schedule_,name);   }  \
	static void name(void)
/* This is used once in deadstart_prog.c which is used to invoke
   startups when the program finishes loading.                   */
#define MAGIC_PRIORITY_PRELOAD(name,priority) static void CPROC name(void); \
	namespace { static class pastejunk(schedule_,name) {   \
	  public:pastejunk(schedule_,name)() {  \
	name();  \
	  }  \
	} pastejunk(do_schedul_,name);   }  \
	static void name(void)
/*
  Internal macro used to trigger InvokeExits() which runs scheduled exits.
                                                                     */
#define ATEXIT_INVOKE_INTERNAL(name) static void CPROC name(void); \
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(~schedule_,name)() {    \
			name();\
	  }  \
	} pastejunk(do_schedule_,name);     \
	static void name(void)
/* Defines some code to run at program shutdown time. Allows
   specification of a priority. Higher priorities are run first.
   Example
   <code>

   PRIORITY_ATEXIT( MyOtherShutdown, 153 )
   {
      // run some code probably before most library code dissolves.
      // last to load, first to unload.
   }

   </code>
   See Also
   <link sack::app::deadstart, deadstart Namespace>                 */
#define PRIORITY_ATEXIT(name,priority) static void CPROC name(void); \
   static class pastejunk(shutdown_,name) {   \
	public:pastejunk(shutdown_,name)() {    \
   RegisterPriorityShutdownProc( name,TOSTR(name),priority,(void*)this DBG_SRC );\
	/*name(); / * call on destructor of static object.*/ \
	  }  \
	} do_shutdown_##name;     \
	void name(void)


/* This is the most basic way to define some code to run
   initialization before main.




   Example
   <code lang="c++">

   PRELOAD( MyInitCode )
   {
      // some code here
   }

   </code>
   See Also
   <link sack::app::deadstart, deadstart Namespace>      */
#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)
/* Basic way to register a routine to run when the program exits
   gracefully.
   Example
   \ \
   <code>

   ATEXIT( MyExitRoutine )
   {
       // this will be run sometime during program shutdown
   }
   </code>                                                       */
#define ATEXIT(name)      PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)
/* This is the core atexit. It dispatches all other exit
   routines. This is defined for internal use only...    */
#define ROOT_ATEXIT(name) ATEXIT_INVOKE_INTERNAL(name)

//------------------------------------------------------------------------------------
// Win32 Watcom
//------------------------------------------------------------------------------------
#elif defined( __WATCOMC__ )
#pragma off (check_stack)
/* code taken from openwatcom/bld/watcom/h/rtinit.h */
typedef unsigned char   __type_rtp;
typedef unsigned short  __type_pad;
typedef void(*__type_rtn ) ( void );
#ifdef __cplusplus
#pragma pack(1)
#else
#pragma pack(1)
#endif
struct rt_init // structure placed in XI/YI segment
{
#define DEADSTART_RT_LIST_START 0xFF
    __type_rtp  rtn_type; // - near=0/far=1 routine indication
                          //   also used when walking table to flag
                          //   completed entries
    __type_rtp  priority; // - priority (0-highest 255-lowest)
    __type_rtn  rtn;      // - routine
};
#pragma pack()
/* end code taken from openwatcom/bld/watcom/h/rtinit.h */


//------------------------------------------------------------------------------------
// watcom
//------------------------------------------------------------------------------------
//void RegisterStartupProc( void (*proc)(void) );


#define PRIORITY_PRELOAD(name,priority) static void pastejunk(schedule_,name)(void); static void CPROC name(void); \
	static struct rt_init __based(__segname("XI")) pastejunk(name,_ctor_label)={0,(DEADSTART_PRELOAD_PRIORITY-1),pastejunk(schedule_,name)}; \
	static void pastejunk(schedule_,name)(void) {                 \
	RegisterPriorityStartupProc( name,TOSTR(name),priority,&pastejunk(name,_ctor_label) DBG_SRC );\
	}                                       \
	void name(void)
#define ATEXIT_PRIORITY(name,priority) static void pastejunk(schedule_exit_,name)(void); static void CPROC name(void); \
	static struct rt_init __based(__segname("XI")) pastejunk(name,_dtor_label)={0,69,pastejunk(schedule_exit_,name)}; \
	static void pastejunk(schedule_exit_,name)(void) {                                              \
	RegisterPriorityShutdownProc( name,TOSTR(name),priority,&name##_dtor_label DBG_SRC );\
	}                                       \
	void name(void)

// syslog runs preload at priority 65
// message service runs preload priority 66
// deadstart itself tries to run at priority 70 (after all others have registered)
#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)


// this is a special case macro used in client.c
// perhaps all PRIORITY_ATEXIT routines should use this
// this enables cleaning up things that require threads to be
// active under windows... (message disconnect)
// however this routine is only triggered in windows by calling
// BAG_Exit(nn) which is aliased to replace exit(n) automatically

#define PRIORITY_ATEXIT(name,priority) ATEXIT_PRIORITY( name,priority)
/*
static void name(void); static void name##_x_(void);\
	static struct rt_init __based(__segname("YI")) name##_dtor_label={0,priority,name##_x_}; \
	static void name##_x_(void) { char myname[256];myname[0]=*(CTEXTSTR)&name##_dtor_label;GetModuleFileName(NULL,myname,sizeof(myname));name(); } \
	static void name(void)
  */
#define ROOT_ATEXIT(name) ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_ROOT)
#define ATEXIT(name)      PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)
// if priority_atexit is used with priority 0 - the proc is scheduled into
// atexit, and exit() is then invoked.
//#define PRIORITY_ATEXIT(name,priority) ATEXIT_PRIORITY(name,priority )

//------------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------------
#elif defined( __GNUC__ )

/* code taken from openwatcom/bld/watcom/h/rtinit.h */
typedef unsigned char   __type_rtp;
typedef void(*__type_rtn ) ( void );
struct rt_init // structure placed in XI/YI segment
{
#define DEADSTART_RT_LIST_START 0xFF
    __type_rtp  rtn_type; // - near=0/far=1 routine indication
                          //   also used when walking table to flag
                          //   completed entries
    __type_rtp  scheduled; // has this been scheduled? (0 if no)
    __type_rtp  priority; // - priority (0-highest 255-lowest)
#if defined( __64__ ) ||defined( __arm__ )||defined( __GNUC__ )
#define INIT_PADDING ,{0}
	 char padding[1]; // need this otherwise it's 23 bytes and that'll be bad.
#else
#define INIT_PADDING
#endif
	 int line; // 32 bits in 64 bits....
// this ends up being nicely aligned for 64 bit platforms
// specially with the packed attributes
	 __type_rtn  routine;      // - routine (rtn)
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
	 CTEXTSTR file;
#endif
	 CTEXTSTR funcname;
	 struct rt_init *junk;
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
#if defined( __GNUC__ ) && defined( __64__)
    // this provides padding - inter-object segments are packed
    // to 32 bytes...
	 struct rt_init *junk2[3];
#endif
#endif

} __attribute__((packed));

#if defined( _DEBUG ) || defined( _DEBUG_INFO )
#  if defined( __GNUC__ ) && defined( __64__)
#    define JUNKINIT(name) ,&pastejunk(name,_ctor_label), {0,0}
#  else
#    define JUNKINIT(name) ,&pastejunk(name,_ctor_label)
#  endif
#else
#  define JUNKINIT(name) ,&pastejunk(name,_ctor_label)
#endif

#define RTINIT_STATIC static

#define ATEXIT_PRIORITY PRIORITY_ATEXIT

#if defined( _DEBUG ) || defined( _DEBUG_INFO )
#  define PASS_FILENAME ,WIDE__FILE__
#else
#  define PASS_FILENAME
#endif

#ifdef __MAC__
#  define DEADSTART_SECTION "TEXT,deadstart_list"
#else
#  define DEADSTART_SECTION "deadstart_list"
#endif


#ifdef __MANUAL_PRELOAD__

#define PRIORITY_PRELOAD(name,pr) static void name(void); \
	RTINIT_STATIC struct rt_init pastejunk(name,_ctor_label)	\
	__attribute__((section(DEADSTART_SECTION))) __attribute__((used))	 = \
	{0,0,pr INIT_PADDING, __LINE__, name PASS_FILENAME	, TOSTR(name) JUNKINIT(name)} ; \
	void name(void); \
	void pastejunk(registerStartup,name)(void) __attribute__((constructor)); \
	void pastejunk(registerStartup,name)(void) {	 RegisterPriorityStartupProc(name,TOSTR(name),pr,NULL DBG_SRC); } \
	void name(void)

#else

#if defined( _WIN32 ) || defined( __GNUC__ )
#  define HIDDEN_VISIBILITY
#else
#  define HIDDEN_VISIBILITY  __attribute__((visibility("hidden")))
#endif

#define PRIORITY_PRELOAD(name,pr) static void name(void);         \
	RTINIT_STATIC struct rt_init pastejunk(name,_ctor_label)       \
	  __attribute__((section(DEADSTART_SECTION))) __attribute__((used))  \
	={0,0,pr INIT_PADDING                                          \
	 ,__LINE__,name                                                \
	 PASS_FILENAME                                                 \
	,TOSTR(name)                                                   \
	JUNKINIT(name)};                                               \
	static void name(void) __attribute__((used)); \
	void name(void)

#endif

typedef void(*atexit_priority_proc)(void (*)(void),CTEXTSTR,int DBG_PASS);
#define PRIORITY_ATEXIT(name,priority) static void name(void);           \
static void pastejunk(atexit,name)(void) __attribute__((constructor));   \
void pastejunk(atexit,name)(void)                                        \
{                                                                        \
	RegisterPriorityShutdownProc(name,TOSTR(name),priority,NULL DBG_SRC); \
}                                                                        \
void name(void)

#define ATEXIT(name) PRIORITY_ATEXIT( name,ATEXIT_PRIORITY_DEFAULT )
#define ROOT_ATEXIT(name) static void name(void) __attribute__((destructor)); \
   static void name(void)

#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)



//------------------------------------------------------------------------------------
// CYGWIN (-mno-cygwin)
//------------------------------------------------------------------------------------
#elif defined( __CYGWIN__ )

/* code taken from openwatcom/bld/watcom/h/rtinit.h */
typedef unsigned char   __type_rtp;
typedef void(*__type_rtn ) ( void );
struct rt_init // structure placed in XI/YI segment
{
#ifdef __cplusplus
	//rt_init( int _rtn_type ) { rt_init::rtn_type = _rtn_type; }
	/*rt_init( int _priority, CTEXTSTR name, __type_rtn rtn, CTEXTSTR _file, int _line )
	{
		rtn_type = 0;
		scheduled = 0;
		priority = priority;
		file = _file;
		line = _line;
      routine = rtn;
		}
      */
#endif
#define DEADSTART_RT_LIST_START 0xFF
    __type_rtp  rtn_type; // - near=0/far=1 routine indication
                          //   also used when walking table to flag
                          //   completed entries
    __type_rtp  scheduled; // has this been scheduled? (0 if no)
    __type_rtp  priority; // - priority (0-highest 255-lowest)
#if defined( __GNUC__ ) || defined( __64__ ) || defined( __arm__ ) || defined( __CYGWIN__ )
#define INIT_PADDING ,{0}
	 char padding[1]; // need this otherwise it's 23 bytes and that'll be bad.
#else
#define INIT_PADDING
#endif
	 int line; // 32 bits in 64 bits....
// this ends up being nicely aligned for 64 bit platforms
// specially with the packed attributes
	 __type_rtn  routine;      // - routine (rtn)
	 CTEXTSTR file;
	 CTEXTSTR funcname;
	 struct rt_init *junk;
#if defined( __GNUC__ ) && defined( __64__ )
    // this provides padding - inter-object segments are packed
    // to 32 bytes...
	 struct rt_init *junk2[3];
#endif
} __attribute__((packed));

#define JUNKINIT(name) ,&pastejunk(name,_ctor_label)

#ifdef __cplusplus
#define RTINIT_STATIC
#else
#define RTINIT_STATIC static
#endif


typedef void(*atexit_priority_proc)(void (*)(void),CTEXTSTR,int DBG_PASS);
#define ATEXIT_PRIORITY(name,priority) static void name(void); static void atexit##name(void) __attribute__((constructor));  \
	void atexit_failed##name(void(*f)(void),int i,CTEXTSTR s1,CTEXTSTR s2,int n) { lprintf( "Failed to load atexit_priority registerar from core program." );} \
void atexit##name(void)                                                  \
{                                                                        \
	static char myname[256];HMODULE mod;if(myname[0])return;myname[0]='a';GetModuleFileName( NULL, myname, sizeof( myname ) );\
	mod=LoadLibrary(myname);if(mod){\
   typedef void (*x)(void);void(*rsp)( x,const CTEXTSTR,int,const CTEXTSTR,int); \
	if((rsp=((void(*)(void(*)(void),const CTEXTSTR,int,const CTEXTSTR,int))(GetProcAddress( mod, "RegisterPriorityShutdownProc")))))\
	 {rsp( name,TOSTR(name),priority DBG_SRC);}\
	 else atexit_failed##name(name,priority,TOSTR(name) DBG_SRC);        \
	}\
     FreeLibrary( mod); \
	}             \
void name( void)

#ifdef _DEBUG
#  define PASS_FILENAME ,WIDE__FILE__
#else
#  define PASS_FILENAME
#endif

#define PRIORITY_PRELOAD(name,pr) static void name(void); \
	RTINIT_STATIC struct pastejunk(rt_init name,_ctor_label) \
	  __attribute__((section("deadstart_list"))) \
	={0,0,pr INIT_PADDING    \
	 ,__LINE__,name         \
	 PASS_FILENAME        \
	,TOSTR(name)        \
	JUNKINIT(name)}; \
	static void name(void)

#define ATEXIT(name)      ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_DEFAULT)
#define PRIORITY_ATEXIT ATEXIT_PRIORITY

#define ROOT_ATEXIT(name) static void name(void) __attribute__((destructor)); \
   static void name(void)

#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)

//------------------------------------------------------------------------------------
// WIN32 MSVC
//------------------------------------------------------------------------------------
#elif defined( _MSC_VER ) && defined( _WIN32 )
//#define PRELOAD(name) __declspec(allocate(".CRT$XCAA")) void CPROC name(void)

//#pragma section(".CRT$XCA",long,read)
//#pragma section(".CRT$XCZ",long,read)

// put init in both C startup and C++ startup list...
// looks like only one or the other is invoked, not both?


/////// also the variables to be put into these segments
#if defined( __cplusplus_cli )
#define LOG_ERROR(n) System::Console::WriteLine( gcnew System::String(n) + gcnew System::String( myname) ) )
#else
#define LOG_ERROR(n) SystemLog( n )

// since we get linked first, then the runtime is added, we have to link against the last indicator of section,
// so we get put between start to end.
#define _STARTSEG_ ".CRT$XIM"
#define _STARTSEG2_ ".CRT$XCY"
#define _ENDSEG_ ".CRT$XTM"

//#pragma data_seg(".CRT$XIA")
#pragma data_seg(".CRT$XIM")
#pragma section(".CRT$XIM",long,read)

#pragma data_seg(".CRT$XCY")
#pragma section(".CRT$XCY",long,read)
//#pragma data_seg(".CRT$XIZ")

//#pragma data_seg(".CRT$YCZ")
#pragma data_seg(".CRT$XTM")
#pragma section(".CRT$XTM",long,read)
#pragma data_seg()


#define PRIORITY_PRELOAD(name,priority) static void CPROC name(void); \
   static int CPROC pastejunk(schedule_,name)(void);   \
	__declspec(allocate(_STARTSEG_)) int (CPROC*pastejunk(TARGET_LABEL,pastejunk( pastejunk(x_,name),__LINE__)))(void) = pastejunk(schedule_,name); \
	int CPROC pastejunk(schedule_,name)(void) {                 \
	RegisterPriorityStartupProc( name,TOSTR(name),priority,pastejunk(TARGET_LABEL,pastejunk( pastejunk(x_,name),__LINE__)) DBG_SRC );\
	return 0; \
	}                                       \
	/*static __declspec(allocate(_STARTSEG_)) void (CPROC*pointer_##name)(void) = pastejunk(schedule_,name);*/ \
	static void CPROC name(void)

#define ROOT_ATEXIT(name) static void name(void); \
	__declspec(allocate(_ENDSEG_)) static void (*f##name)(void)=name; \
   static void name(void)
#define ATEXIT(name) PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)

typedef void(*atexit_priority_proc)(void (*)(void),int,CTEXTSTR DBG_PASS);


#define PRIORITY_ATEXIT(name,priority) static void CPROC name(void); \
   static int schedule_atexit_##name(void);   \
	__declspec(allocate(_STARTSEG_)) void (CPROC*pastejunk(TARGET_LABEL,pastejunk( x_##name,__LINE__)))(void) = (void(CPROC*)(void))schedule_atexit_##name; \
	static int schedule_atexit_##name(void) {                 \
	RegisterPriorityShutdownProc( name,TOSTR(name),priority,pastejunk(TARGET_LABEL,pastejunk( x_##name,__LINE__)) DBG_SRC );\
	return 0; \
	}                                       \
	static void CPROC name(void)


#define ATEXIT_PRIORITY(name,priority) PRIORITY_ATEXIT(name,priority)

#endif

#ifdef __cplusplus_cli
#define InvokeDeadstart() do {                                              \
	TEXTCHAR myname[256];HMODULE mod; \
	mod=LoadLibrary("sack_bag.dll");if(mod){        \
   void(*rsp)(void); \
	if((rsp=((void(*)(void))(GetProcAddress( mod, "RunDeadstart"))))){rsp();}else{lprintf( "Hey failed to get proc %d", GetLastError() );}\
	FreeLibrary( mod); }} while(0)
#else
#endif
#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)

//extern uint32_t deadstart_complete;
//#define DEADSTART_LINK uint32_t *deadstart_link_couple = &deadstart_complete; // make sure we reference this symbol
//#pragma data_seg(".CRT$XCAA")
//extern void __cdecl __security_init_cookie(void);
//static _CRTALLOC(".CRT$XCAA") _PVFV init_cookie = __security_init_cookie;
//#pragma data_seg()

//------------------------------------------------------------------------------------
// UNDEFINED
//------------------------------------------------------------------------------------
#else
#error "there's nothing I can do to wrap PRELOAD() or ATEXIT()!"
/* This is the most basic way to define some startup code that
   runs at some point before the program starts. This code is
   declared as static, so the same preload initialization name
   can be used in multiple files.

   <link sack::app::deadstart, See Also.>                      */
#define PRELOAD(name)
#endif

#include <exit_priorities.h>

#ifdef __cplusplus
} } } //SACK_DEADSTART_NAMESPACE_END
#endif
USE_SACK_DEADSTART_NAMESPACE
#endif
