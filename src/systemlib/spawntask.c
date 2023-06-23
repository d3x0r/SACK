#define NO_UNICODE_C
#define TASK_INFO_DEFINED
#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sharemem.h>

#ifndef __NO_IDLE__
#  include <idle.h>
#endif

#include <timers.h>
#include <filesys.h>

#ifdef __LINUX__
#include <sys/poll.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
extern char **environ;
#endif


#include <sack_system.h>


//--------------------------------------------------------------------------

SACK_SYSTEM_NAMESPACE

#include "taskinfo.h"

typedef struct task_info_tag TASK_INFO;



//--------------------------------------------------------------------------

#ifdef WIN32
static int DumpErrorEx( DBG_VOIDPASS )
#define DumpError() DumpErrorEx( DBG_VOIDSRC )
{
	const int err = GetLastError();
	//_xlprintf( LOG_NOISE+1 DBG_RELAY)( "Failed create process:%d", err );
	return 0;
}
#endif

//--------------------------------------------------------------------------
extern uintptr_t CPROC WaitForTaskEnd( PTHREAD pThread );

#ifdef _WIN32
static uintptr_t CPROC HandleTaskOutput(PTHREAD thread )
{
	struct taskOutputStruct* taskParams = (struct taskOutputStruct*)GetThreadParam( thread );
	PTASK_INFO task = taskParams->task;  // (PTASK_INFO)GetThreadParam( thread );
	if( task )
	{
		Hold( task );
		if( taskParams->stdErr )
			task->pOutputThread2 = thread;
		else
			task->pOutputThread = thread;
		// read input from task, montiro close and dispatch TaskEnd Notification also.
		{
			PHANDLEINFO phi = taskParams->stdErr?&task->hStdErr:&task->hStdOut;
			PTEXT pInput = SegCreate( 4096 );
			int done, lastloop;
			size_t offset = 0;
			done = lastloop = FALSE;
			do
			{
				DWORD dwRead;
				DWORD dwAvail;
				if( task->flags.log_input )
					lprintf( "Go to read task's stdout." );
				offset = 0;
				while( 1 ) {
					BOOL readSuccess = ReadFile( phi->handle
						, GetText( pInput ) + offset, (DWORD)( GetTextSize( pInput ) - ( 1 + offset ) )
						, &dwRead, NULL );
					DWORD dwError = ( !readSuccess ) ? GetLastError() : 0;
					if( readSuccess || dwError == ERROR_BROKEN_PIPE )
					{
						offset += dwRead;
						if( dwError != ERROR_BROKEN_PIPE ) {
							if( offset < 4095 ) {
								Relinquish();
								if( PeekNamedPipe( phi->handle, NULL, 0, NULL, &dwAvail, NULL ) ) {
									if( dwAvail ) {
										if( task->flags.log_input )
											lprintf( "More data became available: %d", dwAvail );
										continue;
									}
								}
							}
						} else if( !dwRead ) {
							lastloop = 1;
							break;
						}
						if( task->flags.log_input )
							lprintf( "got read on task's stdout: %d %d", taskParams->stdErr, dwRead );
						//lprintf( "result %d", dwRead );
						GetText( pInput )[offset] = 0;
						pInput->data.size = offset;
						offset = 0;
						//LogBinary( GetText( pInput ), GetTextSize( pInput ) );
						if( taskParams->stdErr ) {
							if( task->OutputEvent2 )
								task->OutputEvent2( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
							else if( task->OutputEvent )
								task->OutputEvent( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
						} else {
							if( task->OutputEvent )
								task->OutputEvent( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
						}

						pInput->data.size = 4096;

						if( dwError == ERROR_BROKEN_PIPE )
							break;
					} else {
						DWORD dwError = GetLastError();
						offset = 0;
						//lprintf( "Thread Read was 0? %d %d", taskParams->stdErr, dwError );

						if( ( dwError == ERROR_BROKEN_PIPE
							|| dwError == ERROR_OPERATION_ABORTED )
							&& task->flags.process_ended ) {
							lastloop = TRUE;
							break;
						}
					}
				}
				//allow a minor time for output to be gathered before sending
				// partial gathers...
			}
			while( !lastloop );
			
			LineRelease( pInput );

			//lprintf( "Clearing thread handle (done)" );
			phi->hThread = 0;
			phi->handle = INVALID_HANDLE_VALUE;
			if( taskParams->stdErr )
				task->pOutputThread2 = NULL;
			else
				task->pOutputThread = NULL;

			Release( task );
			//WakeAThread( phi->pdp->common.Owner );
			return 0xdead;
		}
	}
	return 0;
}
#endif


#ifdef __LINUX__

static uintptr_t CPROC HandleTaskOutput( PTHREAD thread ) {
	struct taskOutputStruct* taskParams = (struct taskOutputStruct*)GetThreadParam( thread );
	PTASK_INFO task = taskParams->task;  // (PTASK_INFO)GetThreadParam( thread );
	if( task ) {
		Hold( task );
		if( taskParams->stdErr )
			task->pOutputThread2 = thread;
		else
			task->pOutputThread = thread;
		// read input from task, montiro close and dispatch TaskEnd Notification also.
		{
			PHANDLEINFO phi = taskParams->stdErr ? &task->hStdErr : &task->hStdOut;
			PTEXT pInput = SegCreate( 4096 );
			int done, lastloop;
			done = lastloop = FALSE;
			do {
				uint32_t dwRead;
				if( done )
					lastloop = TRUE;
				{
					if( task->flags.log_input )
						lprintf( "Go to read task's stdout." );
#ifdef _WIN32
					if( //!task->flags.process_ended &&
						ReadFile( phi->handle
							, GetText( pInput ), (DWORD)( GetTextSize( pInput ) - 1 )
							, (LPDWORD)&dwRead, NULL ) )  //read the  pipe
					{
#else
					dwRead = read( phi->handle
						, GetText( pInput )
						, GetTextSize( pInput ) - 1 );
					if( !dwRead ) {
#  ifdef _DEBUG
						//lprintf( "Ending system thread because of broke pipe! %d", errno );
#  endif
#  ifdef WIN32
						continue;
#  else
												//lprintf( "0 read = pipe failure." );
						break;
#  endif
					}
#endif
					if( task->flags.log_input )
						lprintf( "got read on task's stdout: %d %d", taskParams->stdErr, dwRead );
					if( task->flags.bSentIoTerminator ) {
						if( dwRead > 1 )
							dwRead--;
						else {
							if( task->flags.log_input )
								lprintf( "Finished, no more data, task has ended; no need for once more around" );
							lastloop = 1;
							break; // we're done; task ended, and we got an io terminator on XP
						}
					}
					//lprintf( "result %d", dwRead );
					if( dwRead < 4096 ) {
						GetText( pInput )[dwRead] = 0;
						pInput->data.size = dwRead;
						//LogBinary( GetText( pInput ), GetTextSize( pInput ) );
						if( taskParams->stdErr ) {
							if( task->OutputEvent2 )
								task->OutputEvent2( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
							else if( task->OutputEvent )
								task->OutputEvent( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
						} else {
							if( task->OutputEvent )
								task->OutputEvent( task->psvEnd, task, GetText( pInput ), GetTextSize( pInput ) );
						}

						pInput->data.size = 4096;
					}
#ifdef _WIN32
					} else {
					DWORD dwError = GetLastError();
					int32_t dwAvail;
					//lprintf( "Thread Read was 0? %d", taskParams->stdErr );

					if( ( dwError == ERROR_OPERATION_ABORTED ) && task->flags.process_ended ) {
						if( PeekNamedPipe( phi->handle, NULL, 0, NULL, (LPDWORD)&dwAvail, NULL ) ) {
							if( dwAvail > 0 ) {
								lprintf( "caught data" );
								// there is still data in the pipe, just that the process closed
								// and we got the sync even before getting the data.
							} else
								break;
						}
					}
				}
#endif
				}
			//allow a minor time for output to be gathered before sending
			// partial gathers...
#ifndef _WIN32
			if( task->flags.process_ended ) {
				// Ending system thread because of process exit!
				done = TRUE; // do one pass to make sure we completed read
			}
#endif
			} while( !lastloop );

			LineRelease( pInput );
#ifdef _WIN32
			/*
			CloseHandle( task->hReadIn );
			CloseHandle( task->hReadOut );
			CloseHandle( task->hReadErr );
			CloseHandle( task->hWriteIn );
			CloseHandle( task->hWriteOut );
			CloseHandle( task->hWriteErr );
			*/
			//lprintf( "Closing process handle %p", task->pi.hProcess );
			phi->hThread = 0;
#else
			//close( phi->handle );
			close( task->hStdIn.pair[1] );
			close( task->hStdOut.pair[0] );
			close( task->hStdErr.pair[0] );
#define INVALID_HANDLE_VALUE -1
#endif
			if( phi->handle == task->hStdIn.handle )
				task->hStdIn.handle = INVALID_HANDLE_VALUE;
			phi->handle = INVALID_HANDLE_VALUE;
			if( taskParams->stdErr )
				task->pOutputThread2 = NULL;
			else
				task->pOutputThread = NULL;

			Release( task );
			//WakeAThread( phi->pdp->common.Owner );
			return 0xdead;
		}
	}
return 0;
}

#endif

//--------------------------------------------------------------------------

static int FixHandles( PTASK_INFO task )
{
#ifdef WIN32
	if( task->pi.hProcess )
		CloseHandle( task->pi.hProcess );
	task->pi.hProcess = 0;
	if( task->pi.hThread )
		CloseHandle( task->pi.hThread );
	task->pi.hThread = 0;
#endif
	return 0; // must return 0 so expression continues
}

//--------------------------------------------------------------------------

void ResumeProgram( PTASK_INFO task )
{
#ifdef WIN32
	//DWORD WINAPI ResumeThread(  _In_ HANDLE hThread);
	ResumeThread( task->pi.hThread );
#endif
}

uintptr_t GetProgramAddress( PTASK_INFO task ) {
#ifdef WIN32

	/*
	BOOL WINAPI GetThreadContext(
  _In_	 HANDLE    hThread,
  _Inout_ LPCONTEXT lpContext
  );
  */
	uintptr_t memstart;
	CONTEXT ctx;
#ifdef __64__
	WOW64_CONTEXT ctx64;
	ctx64.ContextFlags = CONTEXT_INTEGER;
	Wow64GetThreadContext( task->pi.hThread, &ctx64 );
	memstart = ctx64.Ebx;
	ctx.ContextFlags = CONTEXT_INTEGER;
	GetThreadContext( task->pi.hThread, &ctx );
	//memstart = ctx.Ebx;
	return memstart;
#else
	GetThreadContext( task->pi.hThread, &ctx );
	memstart = ctx.Ebx;
	return memstart;
#endif
#else
	lprintf( "non-windows system; cannot find program address... yet" );
	return 0;
#endif


}

#if 0
void LoadReadExe( PTASK_INFO task, uintptr_t base )
   //-------------------------------------------------------
// function to process a currently loaded program to get the
// data offset at the end of the executable.

{
#ifdef WIN32
#  define Seek(a,b) (((uintptr_t)a)+(b))
	//uintptr_t source_memory_length = block_len;
	//POINTER source_memory = block;

	{
		IMAGE_DOS_HEADER source_dos_header;// = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header;// = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		SIZE_T nRead;
		ReadProcessMemory( task->pi.hProcess, (void*)base, (void*)&source_dos_header, sizeof( source_dos_header ), &nRead );
		LogBinary((uint8_t*) &source_dos_header, sizeof( source_dos_header ));
		if( source_dos_header.e_magic != IMAGE_DOS_SIGNATURE ) {
			LoG( "Basic signature check failed; not a library" );
			return ;
		}
/*
		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE ) {
			LoG( "Basic NT signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				LoG( "Optional header signature is incorrect..." );
				return NULL;
			}
		}
		{
			int n;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].PointerToRawData) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			dwSize += (BLOCK_SIZE*2)-1; // pad 1 full block, plus all but 1 byte of a full block(round up)
			dwSize &= ~(BLOCK_SIZE-1); // mask off the low bits; floor result to block boundary
			return (POINTER)Seek( source_memory, dwSize );
			}
		*/
	}
#  undef Seek
#else
	// need to get elf size...
	return 0;
#endif
}

#endif

//--------------------------------------------------------------------------
#ifdef WIN32
extern HANDLE GetImpersonationToken( void );
#endif

SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramExx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															  , int flags
															  , TaskOutput OutputHandler
															  , TaskEnd EndNotice
															  , uintptr_t psv
																DBG_PASS
															  ){
   return LaunchPeerProgram_v2( program, path, args, flags, OutputHandler, OutputHandler, EndNotice, psv, NULL DBG_RELAY );
}

// Run a program completely detached from the current process
// it runs independantly.  Program does not suspend until it completes.
// No way at all to know if the program works or fails.
SYSTEM_PROC( PTASK_INFO, LaunchPeerProgram_v2 )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															  , int flags
															  , TaskOutput OutputHandler
															  , TaskOutput OutputHandler2
															  , TaskEnd EndNotice
															  , uintptr_t psv
															  , PLIST list
																DBG_PASS
															  )
{
	PTASK_INFO task = NULL;
	if( !sack_system_allow_spawn() ) return NULL;
	TEXTSTR expanded_path;// = ExpandPath( program );
	TEXTSTR expanded_working_path = path ? ExpandPath( path ) : NULL;
	PLIST oldStrings = NULL;
	{
		INDEX idx;
		struct environmentValue* val;
		LIST_FORALL( list, idx, struct environmentValue*, val ) {
			const char *oldVal = OSALOT_GetEnvironmentVariable( val->field );
			if( oldVal ) oldVal = StrDup( oldVal );
			SetLink( &oldStrings, idx, oldVal );
			OSALOT_SetEnvironmentVariable( val->field, val->value );
		}
	}
	if( path ) {
		path = ExpandPath( path );
		if( IsAbsolutePath( program ) ) {
			expanded_path = ExpandPath( program );
		}
		else {
			PVARTEXT pvtPath;
			pvtPath = VarTextCreate();
			if( path[0] == '.' && path[1] == 0 )
				vtprintf( pvtPath, "%s", program );
			else
				vtprintf( pvtPath, "%s" "/" "%s", path, program );
			expanded_path = ExpandPath( GetText( VarTextPeek( pvtPath ) ) );
			VarTextDestroy( &pvtPath );
		}
	} else {
		path = ExpandPath( "." );
		expanded_path = ExpandPath( program );
	}
	if( program && program[0] )
	{
#ifdef WIN32
		int launch_flags = ( ( flags & LPP_OPTION_NEW_CONSOLE ) ? CREATE_NEW_CONSOLE : 0 )
		                 | ( ( flags & LPP_OPTION_NEW_GROUP ) ? CREATE_NEW_PROCESS_GROUP : 0 )
		                 | ( ( flags & LPP_OPTION_SUSPEND ) ? CREATE_SUSPENDED : 0 )
		                 | ( ( flags & LPP_OPTION_NO_WINDOW ) ? CREATE_NO_WINDOW : 0 )
			;
		PVARTEXT pvt = VarTextCreateEx( DBG_VOIDRELAY );
		PTEXT cmdline;
		TEXTSTR new_path;
		PTEXT final_cmdline;
		LOGICAL needs_quotes;
		int first = TRUE;
		int success = 0;
		int shellExec = 0;
		if( path )
			Release( path );
		//TEXTCHAR saved_path[256];
		task = (PTASK_INFO)AllocateEx( sizeof( TASK_INFO ) DBG_RELAY );
		MemSet( task, 0, sizeof( TASK_INFO ) );
		task->flags.useCtrlBreak = ( flags & LPP_OPTION_USE_CONTROL_BREAK ) ? 1 : 0;
		task->flags.useEventSignal = ( flags & LPP_OPTION_USE_SIGNAL ) ? 1 : 0;
		task->flags.noKillOnExit = ( flags & LPP_OPTION_NO_KILL_ON_EXIT ) ? 1 : 0;
		{
			CTEXTSTR nameStart = StrRChr( (CTEXTSTR)program, '/' );
			CTEXTSTR nameEnd = StrRChr( (CTEXTSTR)program, '.' );
			if( !nameStart ) {
				nameStart = StrRChr( (CTEXTSTR)program, '\\' );
				if( !nameStart ) nameStart = program;
				else nameStart++;
			} else nameStart++;
			if( !nameEnd ) nameEnd = nameStart + StrLen( nameStart );
			snprintf( task->name, 256, "%.*s", (int)(nameEnd-nameStart), nameStart );
			//lprintf( "Set Spawned Task Name to : %s", task->name );
		}
		task->psvEnd = psv;
		task->flags.runas_root = (flags & LPP_OPTION_ELEVATE) != 0;
		task->EndNotice = EndNotice;
		if( l.ExternalFindProgram ) {
			new_path = l.ExternalFindProgram( expanded_path );
			if( new_path ) {
				Release( expanded_path );
				expanded_path = new_path;
			}
		}
#ifdef _DEBUG
		//xlprintf(LOG_NOISE)( "%s[%s]", path, expanded_working_path );
#endif
		if( StrCmp( path, "." ) == 0 )
		{
			path = NULL;
			Release( expanded_working_path );
			expanded_working_path = NULL;
		}
		if( expanded_path && StrChr( expanded_path, ' ' ) )
			needs_quotes = TRUE;
		else
			needs_quotes = FALSE;


		if( needs_quotes )
			vtprintf( pvt,  "\"" );

		vtprintf( pvt, "%s", expanded_path );

		if( needs_quotes )
			vtprintf( pvt, "\"" );

		if( flags & LPP_OPTION_FIRST_ARG_IS_ARG )
			;
		else
		{
			if( args && args[0] )// arg[0] is passed with linux programs, and implied with windows.
				args++;
		}
		while( args && args[0] )
		{
			if( args[0][0] == 0 )
				vtprintf( pvt, " \"\"" );
			else if( StrChr( args[0], ' ' ) )
				vtprintf( pvt, " \"%s\"", args[0] );
			else
				vtprintf( pvt, " %s", args[0] );
			first = FALSE;
			args++;
		}
		cmdline = VarTextGet( pvt );
		vtprintf( pvt, "cmd.exe /c %s", GetText( cmdline ) );
		final_cmdline = VarTextGet( pvt );
		VarTextDestroy( &pvt );
		MemSet( &task->si, 0, sizeof( STARTUPINFO ) );
		task->si.cb = sizeof( STARTUPINFO );

#ifdef _DEBUG
		//xlprintf(LOG_NOISE)( "quotes?%s path [%s] program [%s]  [cmd.exe (%s)]", needs_quotes?"yes":"no", expanded_working_path, expanded_path, GetText( final_cmdline ) );
#endif
		/*
		if( path )
		{
			GetCurrentPath( saved_path, sizeof( saved_path ) );
			SetCurrentPath( path );
		}
		*/
		task->OutputEvent = OutputHandler;
		task->OutputEvent2 = OutputHandler2;
		if( OutputHandler || OutputHandler2 )
		{
			SECURITY_ATTRIBUTES sa;
			//lprintf( "setting IO handles." );
			sa.bInheritHandle = TRUE;
			sa.lpSecurityDescriptor = NULL;
			sa.nLength = sizeof( sa );
			if( OutputHandler )
				CreatePipe( &task->hReadOut, &task->hWriteOut, &sa, 0 );
			if( OutputHandler2 )
				CreatePipe( &task->hReadErr, &task->hWriteErr, &sa, 0 );
			CreatePipe( &task->hReadIn, &task->hWriteIn, &sa, 0 );
			task->si.hStdInput = task->hReadIn;
			if( OutputHandler2 )
				task->si.hStdError = task->hWriteErr;
			if( OutputHandler )
				task->si.hStdOutput = task->hWriteOut;
			task->si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			if( !( flags & LPP_OPTION_DO_NOT_HIDE ) )
				task->si.wShowWindow = SW_HIDE;
			else
				task->si.wShowWindow = SW_SHOW;
		}
		else
		{
			//lprintf( "Not setting IO handles." );
			task->si.dwFlags |= STARTF_USESHOWWINDOW;
			if( !( flags & LPP_OPTION_DO_NOT_HIDE ) )
				task->si.wShowWindow = SW_HIDE;
			else
				task->si.wShowWindow = SW_SHOW;
		}

		{
			HINSTANCE hShellProcess = 0;
			if( flags & LPP_OPTION_IMPERSONATE_EXPLORER )
			{
				HANDLE hExplorer = GetImpersonationToken();
				if( ( CreateProcessAsUser( hExplorer, NULL //program
												 , GetText( cmdline )
												 , NULL, NULL, TRUE
												 , launch_flags | CREATE_NEW_PROCESS_GROUP
												 , NULL
												 , expanded_working_path
												 , &task->si
												 , &task->pi ) || FixHandles(task) || DumpError() ) ||
					( CreateProcessAsUser( hExplorer, program
												, GetText( cmdline )
												, NULL, NULL, TRUE
												, launch_flags | CREATE_NEW_PROCESS_GROUP
												, NULL
												, expanded_working_path
												, &task->si
												, &task->pi ) || FixHandles(task) || DumpError() ) ||
					( CreateProcessAsUser( hExplorer, program
												, NULL // GetText( cmdline )
												, NULL, NULL, TRUE
												, launch_flags | CREATE_NEW_PROCESS_GROUP
												, NULL
												, expanded_working_path
												, &task->si
												, &task->pi ) || FixHandles(task) || DumpError() ) ||
					( CreateProcessAsUser( hExplorer, "cmd.exe"
												, GetText( final_cmdline )
												, NULL, NULL, TRUE
												, launch_flags | CREATE_NEW_PROCESS_GROUP
												, NULL
												, expanded_working_path
												, &task->si
												, &task->pi ) || FixHandles(task) || DumpError() )
				  )
				{
					success = 1;
				}
				CloseHandle( hExplorer );
			}
			else
			{
				//lprintf( "Using launch flags; %s %08x", task->name, launch_flags );
				if( ( (!task->flags.runas_root) && ( CreateProcess( program
										, GetText( cmdline )
										, NULL, NULL, TRUE
										, launch_flags
										, NULL
										, expanded_working_path
										, &task->si
										, &task->pi ) || FixHandles(task) || DumpError()) ) ||
					((!task->flags.runas_root) && (CreateProcess( NULL //program
										 , GetText( cmdline )
										 , NULL, NULL, TRUE
										 , launch_flags
										 , NULL
										 , expanded_working_path
										 , &task->si
										 , &task->pi ) || FixHandles(task) || DumpError()) ) ||
					((!task->flags.runas_root) && (CreateProcess( program
										, NULL // GetText( cmdline )
										, NULL, NULL, TRUE
										, launch_flags
										, NULL
										, expanded_working_path
										, &task->si
										, &task->pi ) || FixHandles(task) || DumpError()) ) ||
					( (shellExec=1),TryShellExecute( task, expanded_working_path, program, cmdline ) ) ||
					( (shellExec=0),CreateProcess( NULL//"cmd.exe"
										, GetText( final_cmdline )
										, NULL, NULL, TRUE
										, launch_flags
										, NULL
										, expanded_working_path
										, &task->si
										, &task->pi ) || FixHandles(task) || DumpError() ) ||
				   0
				  )
				{
					success = 1;
				}
			}
			if( success )
			{
				//CloseHandle( task->hReadIn );
				//CloseHandle( task->hWriteOut );
#ifdef _DEBUG
				//xlprintf(LOG_NOISE)( "Success running %s[%s] in %s (%p): %d", program, GetText( cmdline ), expanded_working_path, task->pi.hProcess, GetLastError() );
#endif
				if( !shellExec && ( OutputHandler || OutputHandler2 ) )
				{
					task->hStdIn.handle 	 = task->hWriteIn;
					task->hStdIn.pLine 	 = NULL;
					//task->hStdIn.pdp 		= pdp;
					task->hStdIn.hThread  = 0;
					task->hStdIn.bNextNew = TRUE;
					if( task->OutputEvent ) {
						task->hStdOut.handle   = task->hReadOut;
						task->hStdOut.pLine 	  = NULL;
						//task->hStdOut.pdp 		 = pdp;
						task->hStdOut.bNextNew = TRUE;
						task->args1.task       = task;
						task->args1.stdErr     = FALSE;
						task->hStdOut.hThread  = ThreadTo( HandleTaskOutput, (uintptr_t)&task->args1 );
					}
					if( task->OutputEvent2 )
					{
						task->hStdErr.handle   = task->hReadErr;
						task->hStdErr.pLine 	  = NULL;
						//task->hStdOut.pdp 		 = pdp;
						task->hStdErr.bNextNew = TRUE;
						task->args2.task       = task;
						task->args2.stdErr     = TRUE;
						task->hStdErr.hThread  = ThreadTo( HandleTaskOutput, (uintptr_t)&task->args2 );
					}

					ThreadTo( WaitForTaskEnd, (uintptr_t)task );
				}
				else
				{
					if( shellExec ) {
						// shell exec doesn't get any of this specified... it doesn't use any of it.
						if( OutputHandler2 ) {
							CloseHandle( task->hWriteErr ); 
							CloseHandle( task->hReadErr );
							task->hReadErr = task->hWriteErr = INVALID_HANDLE_VALUE;
						}
						if( OutputHandler ) {
							CloseHandle( task->hWriteOut );
							CloseHandle( task->hReadOut );
							task->hReadOut = task->hWriteOut = INVALID_HANDLE_VALUE;
						}
						CloseHandle( task->hWriteIn );
						CloseHandle( task->hReadIn );
						task->hReadIn = task->hWriteIn = INVALID_HANDLE_VALUE;
					}

					//task->hThread =
					ThreadTo( WaitForTaskEnd, (uintptr_t)task );
				}
				// close my side of the pipes...
				if( task->hWriteOut != INVALID_HANDLE_VALUE ) {
					CloseHandle( task->hWriteOut );
					task->hWriteOut = INVALID_HANDLE_VALUE;
				}
				if( task->hWriteErr != INVALID_HANDLE_VALUE ) {
					CloseHandle( task->hWriteErr );
					task->hWriteErr = INVALID_HANDLE_VALUE;
				}
				if( task->hReadIn != INVALID_HANDLE_VALUE ) {
					CloseHandle( task->hReadIn );
					task->hReadIn = INVALID_HANDLE_VALUE;
				}
			}
			else
			{
				xlprintf(LOG_NOISE)( "Failed to run %s[%s]: %d", program, GetText( cmdline ), GetLastError() );
				if( task->hWriteIn    != INVALID_HANDLE_VALUE ) CloseHandle( task->hWriteIn );
				if( task->hReadIn     != INVALID_HANDLE_VALUE ) CloseHandle( task->hReadIn );
				if( task->hWriteOut   != INVALID_HANDLE_VALUE ) CloseHandle( task->hWriteOut );
				if( task->hReadOut    != INVALID_HANDLE_VALUE ) CloseHandle( task->hReadOut );
				if( task->pi.hProcess != INVALID_HANDLE_VALUE ) CloseHandle( task->pi.hProcess );
				if( task->pi.hThread  != INVALID_HANDLE_VALUE ) CloseHandle( task->pi.hThread );
				Release( task );
				task = NULL;
			}
		}
		LineRelease( cmdline );
		LineRelease( final_cmdline );
		goto reset_env;

#endif
#ifdef __LINUX__
		{
			pid_t newpid;
			TEXTCHAR saved_path[256];
			task = (PTASK_INFO)Allocate( sizeof( TASK_INFO ) );
			MemSet( task, 0, sizeof( TASK_INFO ) );
			//task->flags.log_input = TRUE;
			task->psvEnd = psv;
			task->EndNotice = EndNotice;
			task->OutputEvent = OutputHandler;
			task->OutputEvent2 = OutputHandler2;
			task->args1.task       = task;
			task->args1.stdErr     = FALSE;
			task->args2.task       = task;
			task->args2.stdErr     = TRUE;
			if( OutputHandler )
			{
				if( pipe(task->hStdIn.pair) < 0 ) { // pipe failed
					Release( task );
					task = NULL;
					goto reset_env;
				}
				task->hStdIn.handle = task->hStdIn.pair[1];

				if( pipe(task->hStdOut.pair) < 0 ) {
					close( task->hStdIn.pair[0] );
					close( task->hStdIn.pair[1] );
					Release( task );
					task = NULL;
					goto reset_env;
				}
				task->hStdOut.handle = task->hStdOut.pair[0];

				if( OutputHandler2 ) {
					if( pipe(task->hStdErr.pair) < 0 ) {
						close( task->hStdIn.pair[0] );
						close( task->hStdIn.pair[1] );
						close( task->hStdOut.pair[0] );
						close( task->hStdOut.pair[1] );
						Release( task );
						task = NULL;
						goto reset_env;
					}
					task->hStdErr.handle = task->hStdErr.pair[0];
				} else
					task->hStdErr.handle =task->hStdOut.pair[0];

			}

			// always have to thread to taskend so waitpid can clean zombies.
			ThreadTo( WaitForTaskEnd, (uintptr_t)task );
			if( !( newpid = fork() ) )
			{
				// after fork; check that args has a space for
				// the program name to get filled into.
				// this memory doesn't leak; it's squashed by exec.
				if( flags & LPP_OPTION_FIRST_ARG_IS_ARG ) {
					char ** newArgs;
					int n;
					if( args )
						for( n = 0; args[n]; n++ );
					else n = 0;
					newArgs = NewArray( char *, n + 2 );
					if( args ) {
						for( n = 0; args[n]; n++ ) {
							newArgs[n + 1] = (char*)args[n];
						}
						newArgs[n + 1] = (char*)args[n];
					} else {
						newArgs[1] = NULL;
					}
					newArgs[0] = (char*)program;
					args = (PCTEXTSTR)newArgs;
				}
				if( path )
					chdir( expanded_working_path );

				char *_program = CStrDup( program );
				// in case exec fails, we need to
				// drop any registered exit procs...
				//close( task->hStdIn.pair[1] );
				//close( task->hStdOut.pair[0] );
				//close( task->hStdErr.pair[0] );
				if( OutputHandler ) {
					dup2( task->hStdIn.pair[0], 0 );
					dup2( task->hStdOut.pair[1], 1 );				
					dup2( task->hStdErr.pair[1], 2 );
				}
				DispelDeadstart();

				execve( _program, (char *const*)args, environ );
				//lprintf( "Direct execute failed... trying along path..." );
				{
					char *tmp = strdup( getenv( "PATH" ) );
					char *tok;
					for( tok = strtok( tmp, ":" ); tok; tok = strtok( NULL, ":" ) )
					{
						char fullname[256];
						snprintf( fullname, sizeof( fullname ), "%s/%s", tok, _program );

						lprintf( "program:[%s]", fullname );
						((char**)args)[0] = fullname;
						execve( fullname, (char*const*)args, environ );
					}
					Release( tmp );
				}
				lprintf( "exec failed - and this is ALLL bad... %d", errno );
				if( OutputHandler ) {
					close( task->hStdIn.pair[0] );
					close( task->hStdOut.pair[1] );
					if( OutputHandler2 )
						close( task->hStdErr.pair[1] );
				}
				//close( task->hWriteErr );
				close( 0 );
				close( 1 );
				close( 2 );
				//DebugBreak();
				// well as long as this runs before
				// the other all will be well...
				task = NULL;
				// shit - what can I do now?!
				exit(0); // just in case exec fails... need to fault this.
			}
			else
			{
				Release( path );
				if( OutputHandler ) {
					close( task->hStdIn.pair[0] );
					close( task->hStdOut.pair[1] );
					if( OutputHandler2 )
						close( task->hStdErr.pair[1] );
				}
			}

			if( OutputHandler )
				ThreadTo( HandleTaskOutput, (uintptr_t)&task->args1 );
			if( OutputHandler2 ) { // only if it was opened as a separate handle...
				ThreadTo( HandleTaskOutput, (uintptr_t)&task->args2 );
			} 

			task->pid = newpid;
			// how can I know if the command failed?
			// well I can't - but the user's callback will be invoked
			// when the above exits.
		}
#endif
	}

reset_env:
	if( expanded_working_path )
		Release( expanded_working_path );
	Release( expanded_path );
	if( oldStrings )
	{
		INDEX idx;
		struct environmentValue* val;
		LIST_FORALL( list, idx, struct environmentValue*, val ) {
			const char *oldVal = (const char*)GetLink( &oldStrings, idx );
			OSALOT_SetEnvironmentVariable( val->field, oldVal );
			if( oldVal ) Deallocate( const char *, oldVal );
		}
		DeleteList( &oldStrings );
	}
	return task;
}

SYSTEM_PROC( PTASK_INFO, LaunchPeerProgramEx )( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
															 , TaskOutput OutputHandler
															 , TaskEnd EndNotice
															 , uintptr_t psv
                                                DBG_PASS
															  )
{
	return LaunchPeerProgramExx( program, path, args, LPP_OPTION_DO_NOT_HIDE, OutputHandler, EndNotice, psv DBG_RELAY );
}

//------------- System() ---------- simplest form of launch process (with otuput handler, and pprintf support )

struct task_end_notice
{
	PTHREAD thread;
	LOGICAL ended;
	uintptr_t psv_output;
	TaskOutput output_handler;
};

static void CPROC SystemTaskEnd( uintptr_t psvUser, PTASK_INFO task )
{
	struct task_end_notice *end_data = (struct task_end_notice *)psvUser;
	end_data->ended = TRUE;
	WakeThread( end_data->thread );
}

static void CPROC SystemOutputHandler( uintptr_t psvUser, PTASK_INFO Task, CTEXTSTR buffer, size_t len )
{
	struct task_end_notice *end_data = (struct task_end_notice *)psvUser;
	end_data->output_handler( end_data->psv_output, Task, buffer, len );
}

ATEXIT( SystemAutoShutdownTasks )
{
	INDEX idx;
	PTASK_INFO task;
	if( local_systemlib ) {
		( *local_systemlib ).flags.shutdown = TRUE;
		LIST_FORALL( (*local_systemlib).system_tasks, idx, PTASK_INFO, task )
		{
			if( !task->flags.noKillOnExit )
				TerminateProgram( task );
		}
	}
}

SYSTEM_PROC( PTASK_INFO, SystemEx )( CTEXTSTR command_line
															  , TaskOutput OutputHandler
															  , uintptr_t psv
																DBG_PASS
											)
{
	TEXTCHAR *command_line_tmp = StrDup( command_line );
	struct task_end_notice end_notice;
	PTASK_INFO result;
	int argc;
	TEXTSTR *argv;
	end_notice.ended = FALSE;
	end_notice.thread = MakeThread();
	end_notice.psv_output = psv;
	end_notice.output_handler = OutputHandler;
	ParseIntoArgs( command_line_tmp, &argc, &argv );
	Release( command_line_tmp );
	result = LaunchPeerProgramExx( argv[0], NULL, (PCTEXTSTR)argv, 0, OutputHandler?SystemOutputHandler:NULL, SystemTaskEnd, (uintptr_t)&end_notice DBG_RELAY );
	if( result )
	{
		AddLink( &(*local_systemlib).system_tasks, result );
		// we'll get woken when it ends, might as well be infinite.
		while( !end_notice.ended )
		{
#ifndef __NO_IDLE__
			if( !Idle( ) )
				WakeableSleep( 10000 );
			else
				WakeableSleep( 10 );
#else
			WakeableSleep( 25 );
#endif
		}
		DeleteLink( &(*local_systemlib).system_tasks, result );
	}
	{
		POINTER tmp = (POINTER)argv;
		while( argv[0] )
		{
			Release( (POINTER)argv[0] );
			argv++;
		}
		Release( tmp );
	}
	return result;
}

//----------------------- Utility to send to launched task's stdin ----------------------------

int vpprintf( PTASK_INFO task, CTEXTSTR format, va_list args )
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT output;
	vvtprintf( pvt, format, args );
	output = VarTextGet( pvt );
	if(
#ifdef _WIN32
		WaitForSingleObject( task->pi.hProcess, 0 )
#else
		task->pid != waitpid( task->pid, NULL, WNOHANG )
#endif
	  )
	{
#ifdef _WIN32
		DWORD dwWritten;
#endif
		//lprintf( "Allowing write to process pipe..." );
		{
			PTEXT seg = output;
			while( seg )
			{
#ifdef _WIN32
				//LogBinary( GetText( seg )
				//			, GetTextSize( seg ) );
					WriteFile( task->hStdIn.handle
							, GetText( seg )
							, (DWORD)GetTextSize( seg )
							, &dwWritten
							, NULL );
#else
				{
					struct pollfd pfd = { task->hStdIn.handle, POLLHUP|POLLERR, 0 };
					if( poll( &pfd, 1, 0 ) &&
						 pfd.revents & POLLERR )
					{
						Log( "Pipe has no readers..." );
							break;
					}
					LogBinary( (uint8_t*)GetText( seg ), GetTextSize( seg ) );
					write( task->hStdIn.handle
						 , GetText( seg )
						 , GetTextSize( seg ) );
				}
#endif
				seg = NEXTLINE(seg);
			}
		}
		LineRelease( output );
	}
	else
	{
		lprintf( "Task has ended, write  aborted." );
	}
	VarTextDestroy( &pvt );
	return 0;
}


int pprintf( PTASK_INFO task, CTEXTSTR format, ... )
{
	va_list args;
	va_start( args, format );
	return vpprintf( task, format, args );
}


SACK_SYSTEM_NAMESPACE_END


//-------------------------------------------------------------------------

