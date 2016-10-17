#include <stdhdrs.h>

#if defined( __LINUX__ )

#if !defined( __ANDROID__ )
#include <pty.h>
#else
#include <termios.h>
#endif
#include <utmp.h> // login_tty
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#ifdef __ANDROID__
#include <linux/wait.h>
#include <poll.h>
#else
#include <sys/poll.h>
#endif

extern TEXTCHAR **environ;
#endif


#define __USE_SACK_COMMON_LAUNCH__

#include <filesys.h>
#include <logging.h>
#include <stdio.h>
#include <stdlib.h>
#define PLUGIN_MODULE
#include <plugin.h>

#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif
#ifdef __CYGWIN__
#ifndef RAND_MAX
#include <sys/config.h>
#define RAND_MAX __RAND_MAX
#endif
#endif

#ifndef __LINUX__
//__declspec(dllimport) int b95;


int CPROC SystemShutdown( PSENTIENT ps, PTEXT param )
{
	{
		HANDLE hToken, hProcess;
		TOKEN_PRIVILEGES tp;
		if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
                        , GetCurrentProcess(), &hProcess, 0 
                        , FALSE, DUPLICATE_SAME_ACCESS  ) )
		{
         if( b95 || OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
         {
            tp.PrivilegeCount = 1;
            if( b95 || LookupPrivilegeValue( NULL
                                    , SE_SHUTDOWN_NAME
                                    , &tp.Privileges[0].Luid ) )
            {
            	if( !b95 )
            	{
	               tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
   					AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
				}
               //ExitWindowsEx( EWX_LOGOFF|EWX_FORCE, 0 );
               {
                  PTEXT temp;
		            DECLTEXT( msg, WIDE("Initiating system shutdown...") );
      		      EnqueLink( &ps->Command->Output, &msg );
                  if( !(temp = GetParam( ps, &param ) ) )
                     ExitWindowsEx( EWX_REBOOT|EWX_FORCE, 0 );
                  else
                  {
                     if( TextLike( temp, WIDE("logoff") ) )
                        ExitWindowsEx( EWX_LOGOFF|EWX_FORCE, 0 );
                     else if( TextLike( temp, WIDE("reboot") ) )
                        ExitWindowsEx( EWX_REBOOT|EWX_FORCE, 0 );
                     else if( TextLike( temp, WIDE("shutdown") ) )
                        ExitWindowsEx( EWX_SHUTDOWN|EWX_FORCE, 0 );
                  }
               }
            }
            else
            {
            	DECLTEXT( msg, WIDE("Failed to get privledge lookup...#######") );
	            msg.data.size = snprintf( msg.data.data, msg.data.size, WIDE("Failed to get privledge lookup...%ld"), GetLastError() );
            	EnqueLink( &ps->Command->Output, &msg );
				GetLastError();

            }
         }
         else
         {
            DECLTEXT( msg, WIDE("Failed to open process token... ########") );
            msg.data.size = snprintf( msg.data.data, msg.data.size, WIDE("Failed to open process token...%ld"), GetLastError() );
            EnqueLink( &ps->Command->Output, &msg );
            GetLastError();
			}
		}
		else
			GetLastError();
		CloseHandle( hProcess );
		CloseHandle( hToken );
   }
   return 0;
}

TEXTCHAR savedir[512];

void ResetDirectory( void )
{
	SetCurrentDirectory( savedir );
}

void CPROC AddSoundName( uintptr_t psv, CTEXTSTR name, int flags )
{
	AddLink( (PLIST*)psv, name );
}

int GetSoundList( PSENTIENT ps, PLIST *pList, PTEXT pName )
{
	TEXTCHAR *str, *p, *p2;
	int cnt = 0;
	TEXTCHAR buf[256];
	GetCurrentPath( savedir, 512 );
	str = GetText( pName );
	do
	{
		p = strchr( str, '\\' );
		p2 = strchr( str, '/' );
		if( p )
		{
			if( p2 )
			{  	
				if( p2 < p )
				{
				try_p2:
					memcpy( buf, str, p2 - str );
					buf[p2-str] = 0;
				   SetCurrentPath( buf );
			   	str = p2+1;
				}
				else
				{
					goto try_p;
				}
			}
			else
			{
		try_p:
				memcpy( buf, str, p - str );
				buf[p-str] = 0;
				SetCurrentPath( buf );
			   str = p+1;
			}
		}	
		else if( p2 )
			goto try_p2;
	}while( p || p2 );
	
	{
		void *pInfo = NULL;
		while( ScanFiles( NULL, str, &pInfo, AddSoundName, 0, (uintptr_t)pList ) )
		 	cnt++; // don't know if this is right...
	}	
	return cnt;
}

#ifdef __LCC__
#include <mmsystem.h>
#endif

int CPROC Sound( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp, name = NULL, save;
   PLIST pSounds = CreateList( );
   INDEX sIndex;
   while( save=parameters,
   		 temp = GetParam( ps, &parameters ) )
   {
   	if( save == temp )
	   	name = SegAppend( name, TextDuplicate( temp, TRUE ) );
	   else
	   {
			if( temp->flags & TF_INDIRECT )	   	
		   	name = SegAppend( name, TextDuplicate( GetIndirect(temp), FALSE ) );
			else
		   	name = SegAppend( name, TextDuplicate( temp, FALSE ) );
	   }	
	}
	name->format.position.offset.spaces = 0;
	temp = BuildLine( name );
	LineRelease( name );
//	{
//		PTEXT pOut;
//		pOut = SegDuplicate( temp );
//		EnqueLink( &ps->Command->Output, pOut );
//	}
	
	if( GetSoundList( ps, &pSounds, temp ) )
	{
	   LineRelease( temp );
		if( pSounds->Cnt > 1 )
		{
			sIndex = ( rand() * pSounds->Cnt ) / RAND_MAX;
		}
		else
			sIndex = 0;
		{
			
		}
  //WINMMAPI BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, uint32_t fdwSound);
//          _declspec( dllimport )  BOOL _stdcall PlaySoundA( TEXTCHAR* name, P_0 hMod, uint32_t Flags );
		// SND_FILENAME 0x00020000L
		// SND_ASYNC 1
		// SND_NODEFAULT  2
		Log1( WIDE("Playing Sound: %s"), GetText( (PTEXT)GetLink( &pSounds, sIndex ) ) );
	   PlaySound( GetText( (PTEXT)GetLink( &pSounds, sIndex ) ), NULL, 0x00020003L );
	   {
   		INDEX idx;
   		PTEXT pText;
	   	LIST_FORALL( pSounds, idx, PTEXT, pText )
   			LineRelease( pText );
	   }
	}
	else
	{
		Log1( WIDE("Playing Sound: \"%s\""), GetText( temp ) );
	   PlaySound( GetText( temp ), NULL, 0x00020001L );
	   LineRelease( temp );
	}
	DeleteList( &pSounds );
   ResetDirectory();
   return 0;
}


int CPROC Record( PSENTIENT ps, PTEXT parameters )
{
   // well outputting a sound should have a mating record command
   // RecordSound
   return 0;
}

#endif

//--------------------------------------------------------------------------
// system device commands
//--------------------------------------------------------------------------
extern int nSystemID;

typedef struct handle_info_tag
{
	struct mydatapath_tag *pdp;
	PTEXT pLine; // partial inputs...
	TEXTCHAR *name;
	int       bNextNew;
	PTHREAD   hThread;
#ifdef WIN32
	HANDLE    handle;   // read/write handle
#else
	int       pair[2];
	int       handle;   // read/write handle
#endif
} HANDLEINFO, *PHANDLEINFO;

typedef struct mydatapath_tag {
	DATAPATH common;
   PTASK_INFO task;
	PSENTIENT ps;
	// probably need process handles, partial line gatherers
	// 
	struct {
#ifdef __LINUX__
		uint32_t use_pty : 1;
#endif
		uint32_t no_auto_newline : 1;
		uint32_t unused:1;
	} flags;
	HANDLEINFO hStdIn;
	HANDLEINFO hStdOut;
	//HANDLEINFO hStdErr;
	PTEXT pEndOfLine; // this should be substituted for all EndOfLine segs.
#ifdef __LINUX__
	int child_pid;
#else
	HANDLE hProcess;    // spawned proces handle... 
#endif
} MYDATAPATH, *PMYDATAPATH;

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------

//---------------------------------------------------------------------------

static PTEXT ValidateEOL( PTEXT line )
{
	PTEXT end, check;
	if( !line )
		return line;
	end = line;
	SetEnd( end );
	check = end;
	while( check && ( check->flags & TF_INDIRECT ) )
	{
		PTEXT tmp = GetIndirect( check );
		if( tmp )
		{
			check = tmp;
			SetEnd( check );
		}
		else
			break;
	}
   // check will never be NULL. it will always point to the last valid segment
	// if check == NULL the last segment is a indirect with 0 content...

	if( GetTextSize( check ) && !(check->flags&TF_BINARY) )
	{
		SegAppend( check, SegCreate( 0 ) );
	}	
	return line;
}

//--------------------------------------------------------------------------

static int CPROC WriteSystem( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	//Log3( WIDE("Write system... %d %d %d")
	//	 , IsQueueEmpty( &pdp->common.Output )
	//	 , pdp->child_pid
	//	 , waitpid( pdp->child_pid, NULL, WNOHANG )
	//	 );
	while( 
#ifdef _WIN32
			WaitForSingleObject( pdp->hProcess, 0 ) &&
#else
			pdp->child_pid != waitpid( pdp->child_pid, NULL, WNOHANG ) &&
#endif
			!IsQueueEmpty( &pdp->common.Output ) 
		  )
	{
#ifdef _WIN32
		uint32_t dwWritten;
#endif
	PTEXT pLine, pOutput;
	pLine = (PTEXT)DequeLink( &pdp->common.Output );
	if( !pdp->flags.no_auto_newline )
		ValidateEOL( pLine );
		//if( pdp->common.flags.Formatted )
		{
			if( TextIs( pLine, WIDE(".") ) )
			{
				Log( WIDE("Forwarding a dot command to next layer...") );
      			if( pdp->common.pPrior )
      			{
      				EnqueLink( &pdp->common.pPrior->Output
      							, SegBreak( NEXTLINE( pLine ) ) );
	      			LineRelease( pLine );
   	   				continue;
      			}
			}
   			pOutput = BuildLineExx( pLine, FALSE, pdp->pEndOfLine DBG_SRC );
			LineRelease( pLine );
		}
		/*
		else
		{
			pOutput = pLine;
		}
		*/
		{
			PTEXT seg;
			seg = pOutput;
			//Log2( WIDE("Writing to system...(%d)%08x"), GetTextSize( seg ), NEXTLINE(seg) );
         //LogBinary( seg );
			while( seg )
			{
#ifdef _WIN32
				pprintf( pdp->task, "%s", GetText( seg ) );
   	   	//WriteFile( pdp->hStdIn.handle
      	//				, GetText( seg )
		///      			, (DWORD)GetTextSize( seg )
      		//			, &dwWritten
      			//		, NULL );
#else
				{
					struct pollfd pfd = { pdp->hStdIn.handle, POLLHUP|POLLERR, 0 };
					if( poll( &pfd, 1, 0 ) &&
						 pfd.revents & POLLERR )
					{
						Log( WIDE("Pipe has no readers...") );
							break;
					}
					LogBinary( GetText( seg ), GetTextSize( seg ) );
					write( pdp->hStdIn.handle
						 , GetText( seg )
						 , GetTextSize( seg ) );
				}
#endif
				seg = NEXTLINE(seg);
			}
		}
		pdp->hStdOut.bNextNew = TRUE;
		//pdp->hStdErr.bNextNew = TRUE;
		LineRelease( pOutput );
	}
	return 0;	
}

//--------------------------------------------------------------------------
#ifdef __LINUX__
int CanRead( int handle )
{
	fd_set n;
	int rval;
	struct timeval tv;
	FD_ZERO( &n );
	FD_SET( handle, &n );
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	rval = select( handle+1, &n, NULL, NULL, &tv );
	//fprintf( stderr, WIDE("select : %d %d\n"), rval, handle );
	if( rval > 0 )
		return TRUE;
	return  FALSE;
}
#endif
//--------------------------------------------------------------------------

// returns true of this is the last 
void CheckPendingStatus( PMYDATAPATH pdp )
{

	if( !pdp->hStdOut.handle  
	  //&& !pdp->hStdErr.handle
	  )
	{
#ifdef _DEBUG
		{
//			DECLTEXT( msg, WIDE("Setting close on data channel.") );
//			EnqueLink( pdp->&ps->Command->Output, &msg );
		}
#endif
		pdp->common.flags.Closed = TRUE;	
	}
}

//--------------------------------------------------------------------------


uintptr_t CPROC CommandInputThread( PTHREAD pThread )
{
	PHANDLEINFO phi = (PHANDLEINFO)GetThreadParam( pThread );
	PTEXT pInput = SegCreate( 4096 );
	int done, lastloop, do_start = 1, check_time;
	uint32_t start;
	Hold( phi->pdp );
#ifdef _DEBUG
	{
//	   DECLTEXT( msg, WIDE("Started system input thread!") );
//   	EnqueLink( phi->pdp->&ps->Command->Output, &msg );
	}
#endif
	done = lastloop = FALSE;
	do
	{
		uint32_t dwRead, dwAvail;
		if( done )
      		lastloop = TRUE;
		if( do_start )
		{
			start = GetTickCount() + 50;
			do_start = FALSE;
			check_time = TRUE;
		}
#ifdef _WIN32
		while( PeekNamedPipe( phi->handle
  						   , NULL, 0, NULL, &dwAvail, NULL ) &&
		       ( dwAvail > 0 ) )
#else
		//while( CanRead( phi->handle ) )
#endif
		{
#ifdef _WIN32
			ReadFile( phi->handle
	      			, GetText( pInput ), (DWORD)GetTextSize( pInput ) - 1
	      			, &dwRead, NULL);  //read the  pipe
#else
			dwRead = read( phi->handle
							 , GetText( pInput )
							 , GetTextSize( pInput ) - 1 );
			if( !dwRead )
			{
#ifdef _DEBUG
				{
					DECLTEXT( msg, WIDE("Ending system thread because of broke pipe!") );
					Log( WIDE("Ending system thread because of broke pipe!") );
					//EnqueLink( &phi->pdp->common.Owner->Command->Output, &msg );
				}
#endif
				break;
			}
#endif
			GetText( pInput )[dwRead] = 0;
			pInput->data.size = dwRead;
			EnqueLink( &phi->pdp->common.Input, SegDuplicate( pInput ) );
			WakeAThread( phi->pdp->common.Owner );
      		pInput->data.size = 4096;
      		do_start = TRUE;
      		check_time = FALSE;
		}
		//allow a minor time for output to be gathered before sending 
	   // partial gathers...	
#ifdef _WIN32
		if( WaitForSingleObject( phi->pdp->hProcess, 0 ) )
			Sleep(1);
		else
		{
			// Ending system thread because of process exit!
      		done = TRUE;
		}
#else
		if( !dwRead )
			break;
		if( phi->pdp->child_pid == waitpid( phi->pdp->child_pid, NULL, WNOHANG ) )
		{
         Log( WIDE("Setting done event on system reader.") );
			done = TRUE; // do one pass to make sure we completed read
		}
		else
		{
         Relinquish();
		}
#endif
	}
	while( !lastloop );
#ifdef _DEBUG
	if( lastloop )
	{
		//DECLTEXT( msg, WIDE("Ending system thread because of process exit!") );
		//EnqueLink( phi->pdp->&ps->Command->Output, &msg );
	}
	else
	{
		//DECLTEXT( msg, WIDE("Guess we exited from broken pipe") );
		//EnqueLink( phi->pdp->&ps->Command->Output, &msg );
	}
#endif
	LineRelease( pInput );
#ifdef _WIN32
	CloseHandle( phi->handle );
	phi->hThread = 0;
#else
	close( phi->handle );
#endif
	phi->pdp->common.flags.Closed = 1;
	if( phi->handle == phi->pdp->hStdIn.handle )
		phi->pdp->hStdIn.handle = INVALID_HANDLE_VALUE;
	phi->handle = INVALID_HANDLE_VALUE;
	WakeAThread( phi->pdp->common.Owner );
	Release( phi->pdp );
	return 0xdead;
}


//--------------------------------------------------------------------------

static void CPROC TaskEndHandler(uintptr_t psvpdp, PTASK_INFO task_ended)
{
	PMYDATAPATH pdp = (PMYDATAPATH)psvpdp;
	Hold( pdp );

	pdp->task = NULL;
	pdp->common.flags.Closed = 1;
	WakeAThread( pdp->common.Owner );

	Release( pdp );
}

static void CPROC TaskOutputHandler(uintptr_t psvpdp, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
   PMYDATAPATH pdp = (PMYDATAPATH)psvpdp;
	Hold( pdp );

	EnqueLink( &pdp->common.Input, SegCreateFromText( buffer ) );
	WakeAThread( pdp->common.Owner );
	Release( pdp );
}


int LaunchSystemCommand( PMYDATAPATH pdp, PTEXT Command )
{
#ifdef __USE_SACK_COMMON_LAUNCH__
	TEXTSTR *argv;
	int argc;
	ParseIntoArgs( GetText( Command ), &argc, &argv );

	pdp->task = LaunchPeerProgramExx( argv[0], NULL, (PCTEXTSTR)argv, 0, TaskOutputHandler, TaskEndHandler, (uintptr_t)pdp DBG_SRC );

	{
		POINTER tmp = (POINTER)argv;
		while( argv[0] )
		{
			Release( (POINTER)argv[0] );
			argv++;
		}
		Release( tmp );
	}
	if( pdp->task )
		return TRUE;
   return FALSE;
#else
#ifdef _WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	HANDLE hReadOut, hWriteOut;
	//HANDLE hReadErr, hWriteErr;
	HANDLE hReadIn, hWriteIn;
	int bRunSuccess;
	SECURITY_ATTRIBUTES sa;

	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof( sa );

	CreatePipe( &hReadOut, &hWriteOut, &sa, 0 );
	//CreatePipe( &hReadErr, &hWriteErr, &sa, 0 );
	CreatePipe( &hReadIn, &hWriteIn, &sa, 0 );

	MemSet( &si, 0, sizeof( si ) );
	si.cb = sizeof( STARTUPINFO );
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	//si.hStdError = hWriteErr;
	si.hStdInput = hReadIn;
	si.hStdError = hWriteOut;
	si.hStdOutput = hWriteOut;
	//DebugBreak();
	if( !Command || !GetText( Command )
		 || !(bRunSuccess=CreateProcess( NULL, GetText( Command ), NULL
							 , NULL
							 , TRUE // must set to true to have handle set right!
							 , 0
							 , NULL, NULL, &si, &pi ) ) )
	{
		DECLTEXT( cmd, WIDE("cmd.exe /c ") );
		DECLTEXT( cmd95, WIDE("command.com /c ") );
		PTEXT pNewCmd;
		if( b95 )
			Command = SegAppend( SegCreateIndirect( (PTEXT)&cmd95 ), Command );
		else
			Command = SegAppend( SegCreateIndirect( (PTEXT)&cmd ), Command );
		pNewCmd = BuildLine( Command );
		Log1( WIDE("Attempting to run: %s"), GetText( pNewCmd ) );
		bRunSuccess = CreateProcess( NULL, GetText( pNewCmd ), NULL
											, NULL
											, TRUE // must set to true to have handle set right!
											, 0
											, NULL, NULL, &si, &pi );
		LineRelease( pNewCmd );
		LineRelease( Command ); 
	}

	if( bRunSuccess )
	{
			//DECLTEXT( msg, WIDE("Ran command success!") );
			//EnqueLink( &pdp->ps->Command->Output, &msg );

			// close our half of the desciptors passed to the child....
			//CloseHandle( hWriteErr );
			CloseHandle( hReadIn );
			CloseHandle( hWriteOut );

		// don't need thread access on child 
		//CloseHandle( pi.hThread );

		pdp->hProcess = pi.hProcess;

		pdp->hStdIn.handle 	= hWriteIn;
		pdp->hStdIn.pLine 	= NULL;
		pdp->hStdIn.pdp 		= pdp;
		pdp->hStdIn.hThread  = 0;
		pdp->hStdIn.bNextNew = TRUE;

		pdp->hStdOut.handle 	 = hReadOut;
		pdp->hStdOut.pLine 	 = NULL;
		pdp->hStdOut.pdp 		 = pdp;
		pdp->hStdOut.bNextNew = TRUE;
		pdp->hStdOut.hThread  = ThreadTo( CommandInputThread, (uintptr_t)&pdp->hStdOut );
	}
	else
	{
		//CloseHandle( hWriteErr );
		CloseHandle( hReadIn );
		CloseHandle( hWriteOut );
		//CloseHandle( hReadErr );
		CloseHandle( hWriteIn );
		CloseHandle( hReadOut );
	}
	
	return bRunSuccess;
#else
#ifdef __LINUX__
	{
		PLIST pArgs;
		TEXTCHAR *p;
		TEXTCHAR ActCommand[256];

		pArgs = NULL;
		ActCommand[0] = 0;
		Log1( WIDE("Command input: %s"), GetText( Command ) );
		p = strtok( GetText( Command ), WIDE(" ") );
		if( p )
		{
			do
			{
				if( !ActCommand[0] ) 
				{
					strcpy( ActCommand, p );
					AddLink( &pArgs, ActCommand );
				}
				else
				{
					TEXTCHAR *pNew;
					pNew = NewArray( TEXTCHAR, strlen( p ) + 1 );
					strcpy( pNew, p );
					AddLink( &pArgs, pNew );
				}
			}
			while( p = strtok( NULL, WIDE(" ") ) );
		}
		AddLink( &pArgs, NULL ); //assuming all prior adds were non-null...
										// this SHOULD add a NULL at end of list...
										// any subsequent AddLink would over-write
										//this, however...
		
	   {
			int child_pid, hMaster, hSlave;
			if( pdp->flags.use_pty )
			{
				struct winsize winsize;
            PTEXT val;;
				val = GetVolatileVariable( pdp->ps->Current, WIDE("rows") );
            if( val )
					winsize.ws_row = atoi( GetText( val ) );
            else
					winsize.ws_row = 25;
				val = GetVolatileVariable( pdp->ps->Current, WIDE("cols") );
            if( val )
					winsize.ws_col = atoi( GetText( val ) );
				else
					winsize.ws_col = 80;
				winsize.ws_xpixel = 0;
				winsize.ws_ypixel = 0;
				if( openpty( &hMaster
							  , &hSlave
							  , NULL, NULL, &winsize ) )
				{
					Log( WIDE("PTY Failed!") );
               return FALSE;
				}
				else
               Log( WIDE("Opened pseudo tty okay...") );
			}
			else
			{
				if( pipe( pdp->hStdIn.pair ) )
				{
					DECLTEXT( msg, WIDE("Failure to create stdin pipe") );
					EnqueLink( &pdp->ps->Command->Output, &msg );
				}
				if( pipe( pdp->hStdOut.pair ) )
				{
					DECLTEXT( msg, WIDE("Failure to create stdout pipe") );
					EnqueLink( &pdp->ps->Command->Output, &msg );
				}
				/*
				 if( pipe( pdp->hStdErr.pair ) )
				 {
				 DECLTEXT( msg, WIDE("Failure to create stderr pipe") );
				 EnqueLink( &pdp->ps->Command->Output, &msg );
				 }
				 */
			}
			if( !(child_pid = fork()) )
			{
				if( pdp->flags.use_pty )
				{
					Log( WIDE("duping pseudo tty ...") );
               login_tty( hSlave );
               close( hSlave );
				}
				else
				{
					close( pdp->hStdIn.pair[1] );
					close( pdp->hStdOut.pair[0] );
					dup2( pdp->hStdIn.pair[0], 0 );
					dup2( pdp->hStdOut.pair[1], 1 );
					//dup2( pdp->hStdOut.pair[1], 2 );
					close( pdp->hStdIn.pair[0] );
					close( pdp->hStdOut.pair[1] );
				}
#ifdef _DEBUG
//	   	   write( 2, WIDE("Running command..."), sizeof( WIDE("Running command...") ) );
#endif
				Log1( WIDE("Running program: %s"), ActCommand );
				execvp( ActCommand, (TEXTCHAR**)pArgs->pNode );
				Log1( WIDE("Running program: %s"), getenv(WIDE("SHELL")) );
				execvp( getenv( WIDE("SHELL") ), NULL );
				write( 1, WIDE("Command not found."), 18 );
				if( !pdp->flags.use_pty )
				{
					close( 0 );
					close( 1 );
					//close( 2 );
					exit(0);
				}
			}
			else
			{
				if( pdp->flags.use_pty )
				{
               Log( WIDE("To monitor pseudo tty okay..") );
					pdp->hStdIn.handle = hMaster;
               pdp->hStdOut.handle =hMaster;
				}
            else
				{
					//int status, done, lastloop;
					close( pdp->hStdIn.pair[0] );
					close( pdp->hStdOut.pair[1] );
					pdp->hStdIn.handle = pdp->hStdIn.pair[1];
					pdp->hStdOut.handle = pdp->hStdOut.pair[0];
				}
				pdp->child_pid    = child_pid;
				pdp->hStdIn.pLine = NULL;
				pdp->hStdIn.pdp   = pdp;

				pdp->hStdOut.pLine = NULL;
				pdp->hStdOut.pdp 	 = pdp;
				pdp->hStdOut.hThread = ThreadTo( CommandInputThread, (uintptr_t)&pdp->hStdOut );
				DeleteList( &pArgs );
			}
	   }
   }
   return TRUE;
#endif
#endif
#endif
}

//--------------------------------------------------------------------------

static int CPROC SystemRead( PDATAPATH pdp )
{
	return RelayInput( pdp, NULL );
}

//--------------------------------------------------------------------------

int CPROC EndSysCommand( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	PSENTIENT ps = pdpX->Owner;
	//RemoveMethod( pdp->ps, methods );
	pdp->common.Type = 0;
	pdp->common.Read = NULL;
	pdp->common.Write = NULL;
	pdp->common.Close = NULL;
	LineRelease( pdp->pEndOfLine );
#ifdef _WIN32
	if( pdp->hStdOut.handle )
		CloseHandle( pdp->hStdOut.handle );
	/*
	 if( pdp->hStdErr.handle )
	 CloseHandle( pdp->hStdErr.handle );
   */
   CloseHandle( pdp->hStdIn.handle );

   if( pdp->hStdOut.hThread &&
       WaitForSingleObject( pdp->hStdOut.hThread, 0 ) )
   {
   		//DECLTEXT( msg, WIDE("Forcing Output thread death!") );
   		//EnqueLink( pdp->&ps->Command->Output, &msg );
   		TerminateThread( pdp->hStdOut.hThread, 0xD1E );
	}
   /*
   if( pdp->hStdErr.hThread &&
       WaitForSingleObject( pdp->hStdErr.hThread, 0 ) )
   {
   	//DECLTEXT( msg, WIDE("Forcing error thread death!") );
   	//EnqueLink( pdp->&ps->Command->Output, &msg );
   	TerminateThread( pdp->hStdErr.hThread, 0xD1E );
		}
   CloseHandle( pdp->hStdErr.hThread );
	*/

	if( WaitForSingleObject( pdp->hProcess, 0 ) )
	{
   	//DECLTEXT( msg, WIDE("Forcing system process death!") );
   	//EnqueLink( pdp->&ps->Command->Output, &msg );
   	TerminateProcess( pdp->hProcess, 0xD1E );	
   }
   CloseHandle( pdp->hProcess );
#else
	while( ( pdp->hStdOut.handle != -1 ) )
      Relinquish();
   if( pdp->hStdIn.handle > -1 )
		close( pdp->hStdIn.handle );
   //close( pdp->hStdErr.pair[0] );
#endif
   InvokeBehavior( WIDE("close_process"), ps->Current, ps, NULL );
   return 0;
}


PDATAPATH CPROC SysCommand( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp;
	PTEXT pLine, pCmdLine;
	pdp = CreateDataPath( pChannel, MYDATAPATH );
	if( pdp )
	{
		pdp->ps = ps;
		pdp->common.Type = nSystemID;
		pdp->common.Read = SystemRead;
		pdp->common.Write = WriteSystem;
		pdp->common.Close = EndSysCommand;
		pdp->common.Owner = ps;
		//pdp->common.flags.Formatted = TRUE;

		pdp->hStdIn.name = WIDE("Standard In");
		pdp->hStdOut.name = WIDE("Standard Out");
		//pdp->hStdErr.name = WIDE("Standard Error");
	}
	{
		TEXTCHAR *pc;
		DECLTEXT( eol, WIDE("\n") );
		pdp->pEndOfLine = (PTEXT)&eol;
		while( ( pc = GetText( parameters ) ) &&
		       pc[0] == '_' && 
	   	    pc[1] == '_' )
		{
			PTEXT localopt;
			localopt = GetParam( ps, &parameters );
#ifndef __ANDROID__
#ifdef __LINUX__
			if( TextLike( localopt, WIDE("__pty") ) )
			{
            Log( WIDE("Opening Pseudo tty") );
            pdp->flags.use_pty = 1;
			}
#endif
#endif
			if( TextLike( localopt, WIDE("__raw") ) )
			{
				pdp->flags.no_auto_newline = TRUE;
			}
			if( TextLike( localopt, WIDE("__eol") ) )
			{
				pdp->pEndOfLine = GetParam( ps, &parameters );
				//pdp->common.flags.Formatted = FALSE;
			}
		}
	}
   //Log1( WIDE("Input(%s)"), GetText( parameters ) );
	pLine = MacroDuplicate( ps, parameters );
	pCmdLine = BuildLine( pLine );
	Log1( WIDE("BuiltLine(%s)"), GetText( pCmdLine ) );

	if( !LaunchSystemCommand( pdp, pCmdLine ) )
	{
		DECLTEXT( msg, WIDE("Failure to launch command") );
		EnqueLink( &ps->Command->Output, &msg );
		DestroyDataPath( (PDATAPATH)pdp );
		pdp = NULL;
	}
	AddBehavior( ps->Current, WIDE("close_process"), WIDE("Datapath has closed (multiple same-type datapaths fail") );

	LineRelease( pLine );
	return (PDATAPATH)pdp;
}

//----------------------------------------------------------------------
// $Log: shutdown.c,v $
// Revision 1.33  2005/02/21 12:09:00  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.32  2005/01/28 16:02:21  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.31  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.30  2003/11/02 00:38:42  panther
// Remove some comments
//
// Revision 1.29  2003/11/01 21:27:09  panther
// Prevent pipe signal on write, remove silly logging
//
// Revision 1.28  2003/10/29 02:56:16  panther
// Misc fixes with base filters.  Data throughpaths...
//
// Revision 1.27  2003/10/27 16:53:09  panther
// Comment out debugbreak for testing
//
// Revision 1.26  2003/10/27 01:33:06  panther
// Wake thread on command input
//
// Revision 1.25  2003/10/27 01:08:11  panther
// Minor patches for linux compilation
//
// Revision 1.24  2003/10/26 12:38:17  panther
// Updates for newest scheduler
//
// Revision 1.23  2003/04/08 13:27:37  panther
// Provide INVALID_HANDLE_VALUE == -1
//
// Revision 1.22  2003/04/07 20:45:41  panther
// Compatibility fixes.  Cleaned makefiles.
//
// Revision 1.21  2003/04/06 23:26:13  panther
// Update to new SegSplit.  Handle new formatting option (delete chars) Issue current window size to launched pty program
//
// Revision 1.20  2003/04/06 09:59:08  panther
// Better end of device handling - not complete
//
// Revision 1.19  2003/03/31 01:18:42  panther
// Remove stderr win32 handle
//
// Revision 1.18  2003/03/31 01:12:01  panther
// Remove superfluous logging
//
// Revision 1.17  2003/03/30 21:20:15  panther
// Set stdout to seterror pipe also
//
// Revision 1.16  2003/03/28 20:41:18  panther
// Remove a release of buffer (binary logging)
//
// Revision 1.15  2003/03/28 13:54:30  panther
// Use the same pipe for both stdout and stderr of launched applications
//
// Revision 1.14  2003/03/28 12:16:29  panther
// Fix some minor issues with PSI interface
//
// Revision 1.13  2003/03/26 07:22:31  panther
// Try and allow custom end of line generations
//
// Revision 1.12  2003/01/28 16:40:30  panther
// Updated documentation.  Updated features to get the active device.
// Always hook trigger closest to the object - allows multiple connection
// layering better. Version updates.
//
// Revision 1.11  2003/01/27 14:37:25  panther
// Updated projects from old to new, included missing projects
//
// Revision 1.10  2003/01/21 11:02:50  panther
// Mods to launch system commands under linux better.
// Another option to the binary filter (log only)
//
// Revision 1.9  2002/09/26 09:16:43  panther
// Mods to fit with new modern make system.  Cursrcon completed conversion
// to stop using ppOutput.
//
// Revision 1.8  2002/09/16 01:13:57  panther
// Removed the ppInput/ppOutput things - handle device closes on
// plugin unload.  Attempting to make sure that we can restore to a
// clean state on exit.... this lets us track lost memory...
//
// Revision 1.7  2002/07/29 01:12:27  panther
// Removed the space before the sound name for /sound to work.
// Added CVS Logging.
//
//
