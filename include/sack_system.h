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
// Use GetTaskExitCode() to get the return code of the process
#define LPP_OPTION_DO_NOT_HIDE           1
// for services to launch normal processes (never got it to work; used to work in XP/NT?)
#define LPP_OPTION_IMPERSONATE_EXPLORER  2
#define LPP_OPTION_FIRST_ARG_IS_ARG      4
#define LPP_OPTION_NEW_GROUP             8
#define LPP_OPTION_NEW_CONSOLE          16
#define LPP_OPTION_SUSPEND              32
#define LPP_OPTION_ELEVATE              64
// use ctrl-break instead of ctrl-c for break (see also LPP_OPTION_USE_SIGNAL)
#define LPP_OPTION_USE_CONTROL_BREAK   128
// specify CREATE_NO_WINDOW in create process
#define LPP_OPTION_NO_WINDOW           256
// use process signal to kill process instead of ctrl-c or ctrl-break
#define LPP_OPTION_USE_SIGNAL          512
// This might be a useful windows option in some cases
#define LPP_OPTION_DETACH             1024
// this is a Linux option - uses forkpty() instead of just fork() to 
// start a process - meant for interactive processes.
#define LPP_OPTION_INTERACTIVE        2048

struct environmentValue {
	char* field;
	char* value;
};

SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramExx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
                                               , int flags
                                               , TaskOutput OutputHandler
                                               , TaskEnd EndNotice
                                               , uintptr_t psv
                                                DBG_PASS
                                               );
SYSTEM_PROC( PTASK_INFO, LaunchPeerProgram_v2 )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
                                               , int flags
                                               , TaskOutput OutputHandler
                                               , TaskOutput OutputHandler2
                                               , TaskEnd EndNotice
                                               , uintptr_t psv
                                               , PLIST envStrings
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

enum terminate_program_flags {
	TERMINATE_PROGRAM_CHAIN = 1,
	TERMINATE_PROGRAM_CHILDMOST = 2,
};
// abort task, no kill signal, sigabort basically.  Use StopProgram for a more graceful terminate.
// if (!StopProgram(task)) TerminateProgram(task) would be appropriate.
// additional flags from the enum terminate_program_flags may be used.
//   _CHAIN = terminate the whole chain, starting from child-most task.
//   _CHILDMOST = terminate the youngest child in the chain.
SYSTEM_PROC( uintptr_t, TerminateProgramEx )( PTASK_INFO task, int options );

SYSTEM_PROC( void, ResumeProgram )( PTASK_INFO task );
// get first address of program startup code(?) Maybe first byte of program code?
SYSTEM_PROC( uintptr_t, GetProgramAddress )( PTASK_INFO task );
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
// this approximates the install path, as the parent of a program in /bin/ so GetProgramPath()/..; otherwise is TARGET_INSTALL_PREFIX
SYSTEM_PROC( CTEXTSTR, GetInstallPath )( void );
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

// send data to child process.  buffer is an array of bytes of length buflen
SYSTEM_PROC( size_t, task_send )( PTASK_INFO task, const uint8_t*buffer, size_t buflen );

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
	DefineRegistryMethod("SACK",_OnLibraryLoad,"system/library","load_event",name "_LoadEvent",void,(void), __LINE__)

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
SYSTEM_PROC( void, ParseIntoArgs )( TEXTCHAR const *lpCmdLine, int *pArgc, TEXTCHAR ***pArgv );

#define UnloadFunction(p) UnloadFunctionEx(p DBG_SRC )

/* 
   Check if task spawning is allowed...
*/
SYSTEM_PROC( LOGICAL, sack_system_allow_spawn )( void );
/*
   Disallow task spawning.
*/
SYSTEM_PROC( void, sack_system_disallow_spawn )( void );

#ifdef __ANDROID__
// sets the path the program using this is at
SYSTEM_PROC(void, SACKSystemSetProgramPath)( char *path );
// sets the name of the program using this library
SYSTEM_PROC(void, SACKSystemSetProgramName)( char *name );

// sets the current working path of the system using this library(getcwd doesn't work?)
SYSTEM_PROC(void, SACKSystemSetWorkingPath)( char *name ); 
// Set the path of this library.
SYSTEM_PROC(void, SACKSystemSetLibraryPath)( char *name );
#endif

#if _WIN32 
/*
  moves the window of the task; if there is a main window for the task within the timeout perioud.
  callback is called when the window is moved; this allows a background thread to wait
  until the task has created its window.
*/
SYSTEM_PROC( void, MoveTaskWindow )( PTASK_INFO task, int timeout, int left, int top, int width, int height, void cb( uintptr_t, LOGICAL ), uintptr_t psv );

/*
  sets styles for window (class and window style attributes)
  runs a thread which is able to wait for the task's window to be created.  callback is called when completed.
  If no callback is supplied, there is no notification of success or failure.
  `int` status passed to the callback is a combination of statuses for window(1), windowEx(2), and class(4) styles
  and is 7 if all styles are set successfully.

  -1 can be passed as a style value to prevent updates to that style type.
*/
SYSTEM_PROC( void, StyleTaskWindow )( PTASK_INFO task, int timeout, int windowStyle, int windowExStyle, int classStyle, void cb( uintptr_t, int ), uintptr_t psv );
/*
  Moves the window of the specified task to the specified display device; using a lookup to get the display size.
  -1 is an invalid display.
  0 is the default display
  1+ is the first display and subsequent displays - one of which may be the default
*/
SYSTEM_PROC( void, MoveTaskWindowToDisplay )( PTASK_INFO task, int timeout, int display, void cb( uintptr_t, LOGICAL ), uintptr_t psv );

/*
  Moves the window of the specified task to the specified monitor; using a lookup to get the display size.
  0 and less is an invalid display.
  1+ is the first monitor and subsequent monitors
*/
SYSTEM_PROC( void, MoveTaskWindowToMonitor )( PTASK_INFO task, int timeout, int display, void cb( uintptr_t, LOGICAL ), uintptr_t psv );

/*
* Creates a process-identified exit event which can be signaled to terminate the process.
*/
SYSTEM_PROC( void, EnableExitEvent )( void );
/*
  Add callback which is called when the exit event is executed.
  The callback can return non-zero to prevent the task from exiting; but the event is no
  longer valid, and cannot be triggered again.
*/
SYSTEM_PROC( void, AddKillSignalCallback )( int( *cb )( uintptr_t ), uintptr_t );

/*
  Remove a callback which was added to event callback list.
*/
SYSTEM_PROC( void, RemoveKillSignalCallback )( int( *cb )( uintptr_t ), uintptr_t );

/*
  Refresh internal window handle for task; uses internal handle as cached value for performance.
*/
SYSTEM_PROC( HWND, RefreshTaskWindow )( PTASK_INFO task );

/*
  Returns a character string with the window title in it.  If the window is not found for
  the task the string is "No Window".
  The caller is responsible for releasing the string buffer;
*/
SYSTEM_PROC( char*, GetWindowTitle )( PTASK_INFO task );

struct process_tree_pair {
    int process_id;
    INDEX parent_id;
    INDEX child_id;
    INDEX next_id;
};
/*
  returns a datalist of process_tree_pair members;
    parent_id is an index into the datalist...

    current = GetDataItem( &pdlResult, 0)
    while( current->child_id >= 0 ) {
      current = GetDataItem( &pdlResult,current->child_id );
    }
    // although that doesn't account for peers - and assumes a linear 
    // child list.
    struct depth_node {
      struct process_tree_pair *pair;
      int level;
    }
    PDATASTACK stack = CreateDataStack( sizeof( struct depth_node ));
    struct depth_node node;
    struct depth_node deepest_node;
    deepest_node.level = -1;
    node.pair = GetDataItem( &pdlResult, 0);
    node.level = 0;
    PushData( &node );
    while( current = PopData( &stack ) ) {
      if( current->child_id >= 0 ){
        node.pair = GetDataItem( &pdlResult, current->child_id );
        node.level = current.level+1;
        if( node.level > deepest_node.level ) {
          deepest_node = node;
        }
        PushData( &node );
      } 
      if( current->next_id >= 0 ){
        node.pair = GetDataItem( &pdlResult, current->next_id );
        node.level = current.level;
        PushData( &node );
      } 
    }

*/
SYSTEM_PROC( PDATALIST, GetProcessTree )( PTASK_INFO task );


#endif

#ifdef __LINUX__
/*
  Processes launched with LPP_OPTION_INTERACTIVE have a PTY handle.
  This retrieves that handle so things like setting terminal size can
  be done.
*/
SYSTEM_PROC( int, GetTaskPTY )( PTASK_INFO task );
#endif

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
