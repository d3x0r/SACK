#ifndef DEADSTART_DEFINED
#define DEADSTART_DEFINED

#ifdef WIN32
//#include <stdhdrs.h>
#endif
#include <sack_types.h>
#include <typelib.h> // leach, assuming this will be compiled with this part at least.

#define pastejunk_(a,b) a##b
#define pastejunk(a,b) pastejunk_(a,b)

#ifdef __cplusplus
#define USE_SACK_DEADSTART_NAMESPACE using namespace sack::app::deadstart;
#define SACK_DEADSTART_NAMESPACE   SACK_NAMESPACE namespace app { namespace deadstart {
#define SACK_DEADSTART_NAMESPACE_END    } } SACK_NAMESPACE_END
SACK_NAMESPACE
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

#else
#define USE_SACK_DEADSTART_NAMESPACE 
#define SACK_DEADSTART_NAMESPACE
#define SACK_DEADSTART_NAMESPACE_END
#endif

#ifdef TYPELIB_SOURCE
#define DEADSTART_SOURCE
#endif

/* A macro to specify the call type of schedule routines. This
   can be changed in most projects without affect, it comes into
   play if plugins built by different compilers are used,
   __cdecl is most standard.                                     */
#define DEADSTART_CALLTYPE CPROC

#  if defined( _TYPELIBRARY_SOURCE_STEAL )
#    define DEADSTART_PROC type DEADSTART_CALLTYPE name type CPROC name
#  elif defined( _TYPELIBRARY_SOURCE )
#    define DEADSTART_PROC EXPORT_METHOD
#  else
/* A definition for how to declare these functions. if the
   source itself is comipling these are _export, otherwise
   external things linking here are _import.               */
#    define DEADSTART_PROC IMPORT_METHOD
#  endif



   /* this is just a global space initializer (shared, named
      region, allows static link plugins to share information)
      
      
      
      Allocates its shared memory global region, so if this library
      is built statically and referenced in multiple plugins
      ConfigScript can share the same symbol tables. This also
      provides sharing between C++ and C.                           */
#define CONFIG_SCRIPT_PRELOAD_PRIORITY    (SQL_PRELOAD_PRIORITY-3)
   // this is just a global space initializer (shared, named region, allows static link plugins to share information)
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
   file\ :     usually __FILE__ of the code doing this
               registration.
   line :      usually __LINE__ of the code doing this
               registration.                                      */
DEADSTART_PROC  void DEADSTART_CALLTYPE  RegisterPriorityStartupProc( void(CPROC*)(void), CTEXTSTR,int,void* unused, CTEXTSTR,int);
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
   file\ :     usually __FILE__ of the code doing this
               registration.
   line :      usually __LINE__ of the code doing this
               registration.                                      */
DEADSTART_PROC  void DEADSTART_CALLTYPE  RegisterPriorityShutdownProc( void(CPROC*)(void), CTEXTSTR,int,void* unused, CTEXTSTR,int);
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
/* \returns whether MarkRootDeadstartComplete has been called. */
DEADSTART_PROC  LOGICAL DEADSTART_CALLTYPE  IsRootDeadstartStarted ( void );

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
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {    \
	RegisterPriorityStartupProc( name,_WIDE(#name),priority,(void*)this,WIDE__FILE__,__LINE__ );\
	  }  \
	} pastejunk(do_schedule_,name);     \
	static void name(void)
/* This is used once in deadstart_prog.c which is used to invoke
   startups when the program finishes loading.                   */
#define MAGIC_PRIORITY_PRELOAD(name,priority) static void CPROC name(void); \
   static class pastejunk(schedule_,name) {   \
	  public:pastejunk(schedule_,name)() {  \
	name();  \
	  }  \
	} pastejunk(do_schedul_,name);     \
	static void name(void)
/* A macro to define some code to run during program shutdown. An
   additional priority may be specified if the order matters. Higher
   numbers are called first.
   
   
                                                                     */
#define ATEXIT_PRIORITY(name,priority) static void CPROC name(void); \
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {    \
	RegisterPriorityShutdownProc( name,_WIDE(#name),priority,(void*)this,WIDE__FILE__,__LINE__ );\
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
   RegisterPriorityShutdownProc( name,_WIDE(#name),priority,(void*)this,WIDE__FILE__,__LINE__ );\
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
#define ROOT_ATEXIT(name) ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_ROOT)

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
	RegisterPriorityStartupProc( name,_WIDE(#name),priority,&pastejunk(name,_ctor_label),WIDE__FILE__,__LINE__ );\
	}                                       \
	void name(void)
#define ATEXIT_PRIORITY(name,priority) static void pastejunk(schedule_exit_,name)(void); static void CPROC name(void); \
	static struct rt_init __based(__segname("XI")) pastejunk(name,_dtor_label)={0,69,pastejunk(schedule_exit_,name)}; \
	static void pastejunk(schedule_exit_,name)(void) {                                              \
	RegisterPriorityShutdownProc( name,_WIDE(#name),priority,&name##_dtor_label,WIDE__FILE__,__LINE__ );\
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
#if defined( __LINUX64__ ) ||defined( __arm__ )||defined( __GNUC__ )
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
#ifdef __LINUX64__
    // this provides padding - inter-object segments are packed
    // to 32 bytes...
	 struct rt_init *junk2[3];
#endif
} __attribute__((packed));

#define JUNKINIT(name) ,&pastejunk(name,_ctor_label)

#define RTINIT_STATIC static

#define ATEXIT_PRIORITY PRIORITY_ATEXIT

#define PRIORITY_PRELOAD(name,pr) static void name(void); \
	RTINIT_STATIC struct rt_init pastejunk(name,_ctor_label) \
	  __attribute__((section("deadstart_list"))) __attribute__((used)) \
	={0,0,pr INIT_PADDING    \
	 ,__LINE__,name         \
	 ,WIDE__FILE__        \
	,#name        \
	JUNKINIT(name)}; \
	void name(void) __attribute__((used));  \
	void name(void)

typedef void(*atexit_priority_proc)(void (*)(void),CTEXTSTR,int,CTEXTSTR,int);
#define PRIORITY_ATEXIT(name,priority) static void name(void); \
static void pastejunk(atexit,name)(void) __attribute__((constructor));  \
void pastejunk(atexit,name)(void)                                                  \
{                                                                        \
	RegisterPriorityShutdownProc(name,#name,priority,NULL,WIDE__FILE__,__LINE__);                          \
}                                                                          \
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
#if defined( __LINUX64__ ) ||defined( __arm__ ) || defined( __CYGWIN__ )
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
#ifdef __LINUX64__
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

#ifdef __cplusplus
#define PRIORITY_PRELOAD(name,priority) static void name(void); \
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {    \
	static char myname[256];HMODULE mod;if(myname[0])return;myname[0]='a';GetModuleFileName( NULL, myname, sizeof( myname ) );\
	mod=LoadLibrary(myname);DebugBreak();if(mod){\
   typedef void (*x)(void);void(*rsp)( x,const CTEXTSTR,int,const CTEXTSTR,int); \
	if((rsp=((void(*)(void(*)(void),const CTEXTSTR,int,const CTEXTSTR,int))(GetProcAddress( mod, WIDE("RegisterPriorityStartupProc"))))))\
	{rsp( name,#name,priority,WIDE__FILE__,__LINE__ );}}\
     FreeLibrary( mod); \
    }\
	} pastejunk(do_schedul_,name);     \
	static void name(void)
#define MAGIC_PRIORITY_PRELOAD(name,priority) static void name(void); \
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {  name();  \
	  }  \
	} pastejunk(do_schedule_,name);     \
	static void name(void)
#define ATEXIT_PRIORITY(name,priority) static void name(void); \
   static class pastejunk(schedule_,name) {   \
     public:pastejunk(schedule_,name)() {    \
	static char myname[256];HMODULE mod;DebugBreak();if(myname[0])return;myname[0]='a';GetModuleFileName( NULL, myname, sizeof( myname ) );\
	mod=LoadLibrary(myname);if(mod){\
   typedef void (*x)(void);void(*rsp)( x,const CTEXTSTR,int,const CTEXTSTR,int); \
	if((rsp=((void(*)(void(*)(void),const CTEXTSTR,int,const CTEXTSTR,int))(GetProcAddress( mod, WIDE("RegisterPriorityShutdownProc"))))))\
	{rsp( name,#name,priority,WIDE__FILE__,__LINE__ );}}\
     FreeLibrary( mod); \
      }\
	} do_schedul_##name;     \
	static void name(void)

#else

typedef void(*atexit_priority_proc)(void (*)(void),CTEXTSTR,int,CTEXTSTR,int);
#define ATEXIT_PRIORITY(name,priority) static void name(void); static void atexit##name(void) __attribute__((constructor));  \
	void atexit_failed##name(void(*f)(void),int i,CTEXTSTR s1,CTEXTSTR s2,int n) { lprintf( WIDE("Failed to load atexit_priority registerar from core program.") );} \
void atexit##name(void)                                                  \
{                                                                        \
	static char myname[256];HMODULE mod;if(myname[0])return;myname[0]='a';GetModuleFileName( NULL, myname, sizeof( myname ) );\
	mod=LoadLibrary(myname);if(mod){\
   typedef void (*x)(void);void(*rsp)( x,const CTEXTSTR,int,const CTEXTSTR,int); \
	if((rsp=((void(*)(void(*)(void),const CTEXTSTR,int,const CTEXTSTR,int))(GetProcAddress( mod, WIDE("RegisterPriorityShutdownProc"))))))\
	 {rsp( name,#name,priority,WIDE__FILE__,__LINE__ );}\
	 else atexit_failed##name(name,priority,#name,WIDE__FILE__,__LINE__);        \
	}\
     FreeLibrary( mod); \
	}             \
void name( void)

#error blah


#define PRIORITY_PRELOAD(name,pr) static void name(void); \
	RTINIT_STATIC struct pastejunk(rt_init name,_ctor_label) \
	  __attribute__((section("deadstart_list"))) \
	={0,0,pr INIT_PADDING    \
	 ,__LINE__,name         \
	 ,WIDE__FILE__        \
	,#name        \
	JUNKINIT(name)}; \
	static void name(void)
#endif

#define ATEXIT(name)      ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_DEFAULT)
#define PRIORITY_ATEXIT ATEXIT_PRIORITY

#define ROOT_ATEXIT(name) static void name(void) __attribute__((destructor)); \
   static void name(void)

#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)

#ifdef __old_deadstart
#define InvokeDeadstart() do {  \
	TEXTCHAR myname[256];HMODULE mod;GetModuleFileName( NULL, (LPSTR)myname, sizeof( myname ) );\
	mod=LoadLibrary((LPSTR)myname);if(mod){        \
   void(*rsp)(void); \
	if((rsp=((void(*)(void))(GetProcAddress( mod, default_name_for_deadstart_runner))))){rsp();}else{lprintf( WIDE("Hey failed to get proc %d"), GetLastError() );}\
	FreeLibrary( mod);  } \
	} while(0)
#endif

#ifdef __old_deadstart
#define DispelDeadstart() do {  \
	void *hMe = dlopen(NULL, RTLD_LAZY );   \
	if(hMe){ void (*f)(void)=(void(*)(void))dlsym( hMe,"ClearDeadstarts" ); if(f)f(); dlclose(hMe); }\
	} while(0)
#endif

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
// this might only be needed for __64__
#pragma section(".CRT$XIM",long,read)

#pragma data_seg(".CRT$XCY")
// this might only be needed for __64__
#pragma section(".CRT$XCY",long,read)
//#pragma data_seg(".CRT$YCZ")
#pragma data_seg(".CRT$XTM")
// this might only be needed for __64__
#pragma section(".CRT$XTM",long,read)
#pragma data_seg()


#define PRIORITY_PRELOAD(name,priority) static void CPROC name(void); \
   static int CPROC pastejunk(schedule_,name)(void);   \
	static __declspec(allocate(_STARTSEG_)) int (CPROC*pastejunk(TARGET_LABEL,pastejunk( pastejunk(x_,name),__LINE__)))(void) = pastejunk(schedule_,name); \
	int CPROC pastejunk(schedule_,name)(void) {                 \
	RegisterPriorityStartupProc( name,WIDE(#name),priority,pastejunk(TARGET_LABEL,pastejunk( pastejunk(x_,name),__LINE__)),WIDE__FILE__,__LINE__ );\
	return 0; \
	}                                       \
	/*static __declspec(allocate(_STARTSEG_)) void (CPROC*pointer_##name)(void) = pastejunk(schedule_,name);*/ \
	static void CPROC name(void)

#define ROOT_ATEXIT(name) static void name(void); \
	__declspec(allocate(_ENDSEG_)) static void (*f##name)(void)=name; \
   static void name(void)
#define ATEXIT(name) PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)

typedef void(*atexit_priority_proc)(void (*)(void),int,CTEXTSTR,CTEXTSTR,int);


#define PRIORITY_ATEXIT(name,priority) static void CPROC name(void); \
   static int schedule_atexit_##name(void);   \
	static __declspec(allocate(_STARTSEG_)) void (CPROC*pastejunk(TARGET_LABEL,pastejunk( x_##name,__LINE__)))(void) = (void(CPROC*)(void))schedule_atexit_##name; \
	static int schedule_atexit_##name(void) {                 \
	RegisterPriorityShutdownProc( name,WIDE(#name),priority,pastejunk(TARGET_LABEL,pastejunk( x_##name,__LINE__)),WIDE__FILE__,__LINE__ );\
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
	if((rsp=((void(*)(void))(GetProcAddress( mod, "RunDeadstart"))))){rsp();}else{lprintf( WIDE("Hey failed to get proc %d"), GetLastError() );}\
	FreeLibrary( mod); }} while(0)
#else
#endif
#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)

//extern _32 deadstart_complete;
//#define DEADSTART_LINK _32 *deadstart_link_couple = &deadstart_complete; // make sure we reference this symbol
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

SACK_DEADSTART_NAMESPACE_END
USE_SACK_DEADSTART_NAMESPACE
#endif
