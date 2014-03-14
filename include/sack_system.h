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
typedef void (CPROC*TaskEnd)(PTRSZVAL, PTASK_INFO task_ended);
typedef void (CPROC*TaskOutput)(PTRSZVAL, PTASK_INFO task, CTEXTSTR buffer, size_t size );

// Run a program completely detached from the current process
// it runs independantly.  Program does not suspend until it completes.
// No way at all to know if the program works or fails.
#define LPP_OPTION_DO_NOT_HIDE  1
#define LPP_OPTION_IMPERSONATE_EXPLORER 2
#define LPP_OPTION_FIRST_ARG_IS_ARG 4
SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramExx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															  , int flags
															  , TaskOutput OutputHandler
															  , TaskEnd EndNotice
															  , PTRSZVAL psv
																DBG_PASS
															  );

SYSTEM_PROC( PTASK_INFO, LaunchProgramEx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args, TaskEnd EndNotice, PTRSZVAL psv );
SYSTEM_PROC( PTASK_INFO, LaunchProgram )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR  args );
SYSTEM_PROC( PTRSZVAL, TerminateProgram )( PTASK_INFO task );
SYSTEM_PROC( void, SetProgramUserData )( PTASK_INFO task, PTRSZVAL psv );

SYSTEM_PROC( void, ImpersonateInteractiveUser )( void );
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
SYSTEM_PROC( _32, GetTaskExitCode )( PTASK_INFO task );

SYSTEM_PROC( CTEXTSTR, GetProgramName )( void );
SYSTEM_PROC( CTEXTSTR, GetProgramPath )( void );
SYSTEM_PROC( CTEXTSTR, GetStartupPath )( void );

SYSTEM_PROC( LOGICAL, IsSystemShuttingDown )( void );

// HandlePeerOutput is called whenever a peer task has generated output on stdout or stderr
//   - someday evolution may require processing stdout and stderr with different event handlers
SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramEx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															 , TaskOutput HandlePeerOutput
															 , TaskEnd EndNotice
															 , PTRSZVAL psv
															  DBG_PASS
															 );
#define LaunchPeerProgram(prog,path,args,out,end,psv) LaunchPeerProgramEx(prog,path,args,out,end,psv DBG_SRC)


SYSTEM_PROC( PTASK_INFO, SystemEx )( CTEXTSTR command_line
															  , TaskOutput OutputHandler
															  , PTRSZVAL psv
																DBG_PASS
											  );
#define System(command_line,output_handler,user_data) SystemEx( command_line, output_handler, user_data DBG_SRC )

// generate output to a task... read by peer task on standard input pipe
SYSTEM_PROC( int, pprintf )( PTASK_INFO task, CTEXTSTR format, ... );
SYSTEM_PROC( int, vpprintf )( PTASK_INFO task, CTEXTSTR format, va_list args );

typedef void (CPROC*generic_function)(void);
SYSTEM_PROC( generic_function, LoadFunctionExx )( CTEXTSTR library, CTEXTSTR function, LOGICAL bPrivate DBG_PASS);
SYSTEM_PROC( generic_function, LoadFunctionEx )( CTEXTSTR library, CTEXTSTR function DBG_PASS);
/* 
  Add a custom loaded library; attach a name to the DLL space; this should allow
  getcustomsybmol to resolve these 
  */
SYSTEM_PROC( void, AddMappedLibrary)( CTEXTSTR libname, POINTER image_memory );

#define LoadFunction(l,f) LoadFunctionEx(l,f DBG_SRC )
SYSTEM_PROC( generic_function, LoadPrivateFunctionEx )( CTEXTSTR libname, CTEXTSTR funcname DBG_PASS );
#define LoadPrivateFunction(l,f) LoadPrivateFunctionEx(l,f DBG_SRC )

#define OnLibraryLoad(name)  \
	__DefineRegistryMethod(WIDE("SACK"),_OnLibraryLoad,WIDE("system/library"),WIDE("load_event"),name WIDE("_LoadEvent"),void,(void), __LINE__)


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
// Revision 1.13  2005/07/06 00:20:59  jim
// Fix system.h to not define HLIBRARY which is unused anyhow.
//
// Revision 1.12  2005/07/06 00:03:17  jim
// Typecasts to make getenv macro happy.  Implemented in OSALOT since watcom's getenv implementation SUCKS.
//
// Revision 1.11  2005/06/20 17:23:04  jim
// Add function to load libraries privately - needed for certain things like PLUGINS
//
// Revision 1.11  2005/06/19 05:12:32  d3x0r
// Add ability to load private libraries...
//
// Revision 1.10  2005/05/30 11:56:20  d3x0r
// various fixes... working on psilib update optimization... various stabilitizations... also extending msgsvr functionality.
//
// Revision 1.9  2005/05/25 16:50:09  d3x0r
// Synch with working repository.
//
// Revision 1.9  2005/04/19 22:49:30  jim
// Looks like the display module technology nearly works... at least exits graceful are handled somewhat gracefully.
//
// Revision 1.8  2004/09/20 20:41:09  d3x0r
// Fix function type for load function
//
// Revision 1.7  2004/08/13 16:49:55  d3x0r
// Implement a terminate process, and better monitor for exit completion
//
// Revision 1.6  2004/06/14 10:46:28  d3x0r
// Define force focus and stacking operations for render panels...
//
// Revision 1.5  2004/05/02 05:06:22  d3x0r
// Sweeping changes to logging which by default release was very very noisy...
//
// Revision 1.4  2004/04/26 09:47:25  d3x0r
// Cleanup some C++ problems, and standard C issues even...
//
// Revision 1.3  2004/03/04 01:09:47  d3x0r
// Modifications to force slashes to wlib.  Updates to Interfaces to be gotten from common interface manager.
//
// Revision 1.2  2003/10/24 14:59:21  panther
// Added Load/Unload Function for system shared library abstraction
//
// Revision 1.1  2003/10/24 13:22:06  panther
// Initial commit
//
//
