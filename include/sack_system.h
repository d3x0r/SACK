/*
 *  Created by Jim Buckeyne
 *
 *  Purpose
 *    Generalization of system routines which began in
 *   dekware development.
 *   - Process control (load,start,stop)
 *   - Library runtime link control (load, unload)
 *
 */


#ifndef SYSTEM_LIBRARY_DEFINED
#define SYSTEM_LIBRARY_DEFINED


#ifdef SYSTEM_SOURCE
#define SYSTEM_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SYSTEM_PROC(type,name) IMPORT_METHOD type CPROC name
#endif


#ifdef __LINUX__
// Hmm I thought that dlopen resulted in an int...
// but this doc says void * (redhat9)
//typedef void *HLIBRARY;
#include <sack_types.h>
#else
#include <stdhdrs.h>
//typedef HMODULE HLIBRARY;
#endif
#ifdef __cplusplus
#define _SYSTEM_NAMESPACE namespace system {
#define _SYSTEM_NAMESPACE_END }
#else
#define _SYSTEM_NAMESPACE 
#define _SYSTEM_NAMESPACE_END
#endif
#define SACK_SYSTEM_NAMESPACE SACK_NAMESPACE _SYSTEM_NAMESPACE
#define SACK_SYSTEM_NAMESPACE_END _SYSTEM_NAMESPACE_END SACK_NAMESPACE_END

#ifndef UNDER_CE
#define HAVE_ENVIRONMENT
#endif

SACK_NAMESPACE
	_SYSTEM_NAMESPACE

typedef struct task_info_tag *PTASK_INFO;
typedef void (CPROC*TaskEnd)(uintptr_t, PTASK_INFO task_ended);
typedef void (CPROC*TaskOutput)(uintptr_t, PTASK_INFO task, CTEXTSTR buffer, size_t size );

// Run a program completely detached from the current process
// it runs independantly.  Program does not suspend until it completes.
// No way at all to know if the program works or fails.
#define LPP_OPTION_DO_NOT_HIDE           1
// for services to launch normal processes (never got it to work; used to work in XP/NT?)
#define LPP_OPTION_IMPERSONATE_EXPLORER  2
#define LPP_OPTION_FIRST_ARG_IS_ARG      4
#define LPP_OPTION_NEW_GROUP             8
#define LPP_OPTION_NEW_CONSOLE          16
#define LPP_OPTION_SUSPEND              32

SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramExx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															  , int flags
															  , TaskOutput OutputHandler
															  , TaskEnd EndNotice
															  , uintptr_t psv
																DBG_PASS
															  );

SYSTEM_PROC( PTASK_INFO, LaunchProgramEx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args, TaskEnd EndNotice, uintptr_t psv );
// launch a process, program name (including leading path), a optional path to start in (defaults to
// current process' current working directory.  And a array of character pointers to args
// args should be the NULL.
SYSTEM_PROC( PTASK_INFO, LaunchProgram )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR  args );
// abort task, no kill signal, sigabort basically.  Use StopProgram for a more graceful terminate.
// if (!StopProgram(task)) TerminateProgram(task) would be appropriate.
SYSTEM_PROC( uintptr_t, TerminateProgram )( PTASK_INFO task );
SYSTEM_PROC( void, ResumeProgram )( PTASK_INFO task );
SYSTEM_PROC( uintptr_t, GetProramAddress )( PTASK_INFO task );
// before luanchProgramEx, there was no userdata...
SYSTEM_PROC( void, SetProgramUserData )( PTASK_INFO task, uintptr_t psv );

// attempt to implement a method on windows that allows a service to launch a user process
// current systems don't have such methods
SYSTEM_PROC( void, ImpersonateInteractiveUser )( void );
// after launching a process should revert to a protected state.
SYSTEM_PROC( void, EndImpersonation )( void );


// generate a Ctrl-C to the task.
// maybe also signal systray icon
// maybe also signal process.lock region
// maybe end process?
// maybe then terminate process?
SYSTEM_PROC( LOGICAL, StopProgram )( PTASK_INFO task ); 

// ctextstr as its own type is a pointer so a
//  PcTextStr is a pointer to strings -
//   char ** - returns a quoted string if args have spaces (and escape quotes in args?)
SYSTEM_PROC( TEXTSTR, GetArgsString )( PCTEXTSTR pArgs );

// after a task has exited, this can return its code.
// undefined if task has not exited (probably 0)
SYSTEM_PROC( uint32_t, GetTaskExitCode )( PTASK_INFO task );

// returns the name of the executable that is this process (without last . extension   .exe for instance)
SYSTEM_PROC( CTEXTSTR, GetProgramName )( void );
// returns the path of the executable that is this process
SYSTEM_PROC( CTEXTSTR, GetProgramPath )( void );
// returns the path that was the working directory when the program started
SYSTEM_PROC( CTEXTSTR, GetStartupPath )( void );
// returns the path of the current sack library.
SYSTEM_PROC( CTEXTSTR, GetLibraryPath )( void );

// on windows, queries an event that indicates the system is rebooting.
SYSTEM_PROC( LOGICAL, IsSystemShuttingDown )( void );

// HandlePeerOutput is called whenever a peer task has generated output on stdout or stderr
//   - someday evolution may require processing stdout and stderr with different event handlers
SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramEx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															 , TaskOutput HandlePeerOutput
															 , TaskEnd EndNotice
															 , uintptr_t psv
															  DBG_PASS
															 );
#define LaunchPeerProgram(prog,path,args,out,end,psv) LaunchPeerProgramEx(prog,path,args,out,end,psv DBG_SRC)


SYSTEM_PROC( PTASK_INFO, SystemEx )( CTEXTSTR command_line
															  , TaskOutput OutputHandler
															  , uintptr_t psv
																DBG_PASS
											  );
#define System(command_line,output_handler,user_data) SystemEx( command_line, output_handler, user_data DBG_SRC )

// generate output to a task... read by peer task on standard input pipe
// if a task has been opened with an output handler, than IO is trapped, and this is a method of
// sending output to a task.
SYSTEM_PROC( int, pprintf )( PTASK_INFO task, CTEXTSTR format, ... );
// if a task has been opened with an otuput handler, than IO is trapped, and this is a method of
// sending output to a task.
SYSTEM_PROC( int, vpprintf )( PTASK_INFO task, CTEXTSTR format, va_list args );

typedef void (CPROC*generic_function)(void);
SYSTEM_PROC( generic_function, LoadFunctionExx )( CTEXTSTR library, CTEXTSTR function, LOGICAL bPrivate DBG_PASS);
SYSTEM_PROC( generic_function, LoadFunctionEx )( CTEXTSTR library, CTEXTSTR function DBG_PASS);
SYSTEM_PROC( void *, GetPrivateModuleHandle )( CTEXTSTR libname );

/* 
  Add a custom loaded library; attach a name to the DLL space; this should allow
  getcustomsybmol to resolve these 
  */
SYSTEM_PROC( void, AddMappedLibrary )( CTEXTSTR libname, POINTER image_memory );
SYSTEM_PROC( LOGICAL, IsMappedLibrary )( CTEXTSTR libname );
SYSTEM_PROC( void, DeAttachThreadToLibraries )( LOGICAL attach );

#define LoadFunction(l,f) LoadFunctionEx(l,f DBG_SRC )
SYSTEM_PROC( generic_function, LoadPrivateFunctionEx )( CTEXTSTR libname, CTEXTSTR funcname DBG_PASS );
#define LoadPrivateFunction(l,f) LoadPrivateFunctionEx(l,f DBG_SRC )

#define OnLibraryLoad(name)  \
	__DefineRegistryMethod(WIDE("SACK"),_OnLibraryLoad,WIDE("system/library"),WIDE("load_event"),name WIDE("_LoadEvent"),void,(void), __LINE__)

// the callback passed will be called during LoadLibrary to allow an external
// handler to download or extract the library; the resulting library should also
// be loaded by the callback using the standard 'LoadFunction' methods
SYSTEM_PROC( void, SetExternalLoadLibrary )( LOGICAL (CPROC*f)(const char *) );

// please Release or Deallocate the reutrn value
// the callback should search for the file specified, if required, download or extract it
// and then return with a Release'able utf-8 char *.
SYSTEM_PROC( void, SetExternalFindProgram )( char * (CPROC*f)(const char *) );
// override the default program name.
// Certain program wrappers might use this to change log location, configuration, etc other defaults.
SYSTEM_PROC( void, SetProgramName )( CTEXTSTR filename );


// this is a pointer pointer - being that generic_fucntion is
// a pointer...
SYSTEM_PROC( int, UnloadFunctionEx )( generic_function* DBG_PASS );
#ifdef HAVE_ENVIRONMENT
SYSTEM_PROC( CTEXTSTR, OSALOT_GetEnvironmentVariable )(CTEXTSTR name);
SYSTEM_PROC( void, OSALOT_SetEnvironmentVariable )(CTEXTSTR name, CTEXTSTR value);
SYSTEM_PROC( void, OSALOT_AppendEnvironmentVariable )(CTEXTSTR name, CTEXTSTR value);
SYSTEM_PROC( void, OSALOT_PrependEnvironmentVariable )(CTEXTSTR name, CTEXTSTR value);
#endif
/* this needs to have 'GetCommandLine()' passed to it.
 * Otherwise, the command line needs to have the program name, and arguments passed in the string
 * the parameter to winmain has the program name skipped
 */
SYSTEM_PROC( void, ParseIntoArgs )( TEXTCHAR *lpCmdLine, int *pArgc, TEXTCHAR ***pArgv );

#define UnloadFunction(p) UnloadFunctionEx(p DBG_SRC )

SACK_SYSTEM_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::system;
#endif
#endif
//----------------------------------------------------------------------
// $Log: system.h,v $
// Revision 1.14  2005/07/06 00:33:55  jim
// Fixes for all sorts of mangilng with the system.h header.
//
//
// Revision 1.2  2003/10/24 14:59:21  panther
// Added Load/Unload Function for system shared library abstraction
//
// Revision 1.1  2003/10/24 13:22:06  panther
// Initial commit
//
//
