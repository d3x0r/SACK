#define DEFINES_DEKWARE_INTERFACE
#ifdef _WIN32
#include <stdhdrs.h>
//#include <windows.h>
//#include <stdio.h>
#include <filesys.h>
#include <logging.h>
#include <plugin.h>


typedef struct process_tag {
	HANDLE hProcess;
	PTEXT  command;
	PTEXT  directory;
	PROCESS_INFORMATION pi;
	STARTUPINFO  si;
} PROCESS, *PPROCESS;

extern INDEX iProcess;


int IsDead( PSENTIENT ps, PTEXT parameters )
{
	//
	return 0;
}

void DestroyProcess( PENTITY pe )
{
	PPROCESS process = (PPROCESS)GetLink( &pe->pPlugin, iProcess );
	Log( WIDE("Killing process this represents") );
	if( process )
	{
		if( WaitForSingleObject( process->pi.hProcess, 0 ) == WAIT_TIMEOUT )
		{
	      TerminateProcess( process->pi.hProcess, 0xD1E );
		}
		CloseHandle( process->pi.hProcess );
		CloseHandle( process->pi.hThread );
		LineRelease( process->command );
		LineRelease( process->directory );
		Release( process );
		SetLink( &pe->pPlugin, iProcess, NULL );
	}
}

static int StartProcess( PPROCESS process )
{
	if( process )
	{
		if( !CreateProcess( NULL, // command
								  GetText( process->command ), // args
								  NULL, // process attributes
								  NULL, // Security Attributes
								  FALSE, // no inherit
								  DETACHED_PROCESS|NORMAL_PRIORITY_CLASS,
								  NULL, // environment block
								  GetText( process->directory ), // current path
								  &process->si,  // startup info
								  &process->pi ) )  // process info...
		{
	      return -1;
		}
	}
	return 0;
}

int CPROC MakeProcess( PSENTIENT ps, PENTITY peInit, PTEXT parameters )
{
	// parameters specify command and parameters to launch...
	// perhaps a working directory? May or may not want it
	// in the current directory...
	// /make process test WIDE("command arguments") WIDE("work path") <attributes?>
	// /make process test WIDE("notepad trigger.txt") 
	PSENTIENT ps2;
	PPROCESS process = New( PROCESS );
	TEXTCHAR MyPath[256];
	GetCurrentPath( MyPath, sizeof( MyPath ) );
	ps2 = CreateAwareness( peInit );
	MemSet( process, 0, sizeof( PROCESS ) );
	Log( WIDE("Have a process, and woke it up... setting the link") );
	SetLink( &peInit->pPlugin, iProcess, process );
	SetLink( &peInit->pDestroy, iProcess, DestroyProcess );
	Log( WIDE("Set the link, getting params...") );
	{
		PTEXT text, cmd = NULL;
		text = GetParam( ps, &parameters );
		if( text && TextIs( text, WIDE("\"") ) )
		{
	      Log( WIDE("Found a quote, getting command line") );
			while( (text = GetParam( ps, &parameters )) && !TextIs( text, WIDE("\"") ) )
			{
				cmd = SegAppend( cmd, SegDuplicate( text ) );
			}
			cmd->format.position.offset.spaces = 0;
			process->command = BuildLine( cmd );
			if( text ) // closed, and may have start path....
			{
				text = GetParam( ps, &parameters );
		   	if( text && TextIs( text, WIDE("\"") ) )
				{
					Log( WIDE("Found a quote, getting the path") );
					while( (text = GetParam( ps, &parameters )) && !TextIs( text, WIDE("\"") ) )
					{
						cmd = SegAppend( cmd, SegDuplicate( text ) );
		   		}
		   		cmd->format.position.offset.spaces = 0;
		   		process->directory = BuildLine( cmd );
				}
			}
		}
		else
		{
			DECLTEXT( msg, WIDE("Must specify process to execute in quotes (\")") );
			EnqueLink( &ps->Command->Output, &msg );
		WakeAThread( ps2 );
			return -1; // abort creation.
		}
	}
	Log2( WIDE("Starting %s in %s"), GetText( process->command ), GetText( process->directory ) );
	process->si.cb = sizeof( process->si );
	// remaining startup info members are NULL - specifying we do NOT care
	// why why when where how the process starts.
	if( StartProcess( process ) )
	{
		DECLTEXTSZ( msg, 256 );
		msg.data.size = snprintf( msg.data.data, 256*sizeof(TEXTCHAR), WIDE("Failed to start \"%s\" in \"%s\" error: %ld"),
										 GetText( process->command ),
										 GetText( process->directory ),
										 GetLastError() );
		EnqueLink( &ps->Command->Output, SegDuplicate( (PTEXT)&msg ) );
		WakeAThread( ps2 );
		return -1; // abort creation.
	}
	// well otherwise would seem we've launched a valid application
	// we have valid process and thread handles to it which can be monitored
	// and well that's about that.
	WakeAThread( ps2 );
	return 0;
}


#else
#include <space.h>
int MakeProcess( PSENTIENT ps, PENTITY peInit, PTEXT parameters )
{
	return 1;
}

#endif
//----------------------------------------------------------------
// $Log: process.c,v $
// Revision 1.5  2005/02/21 12:09:00  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.4  2005/01/17 09:01:32  d3x0r
// checkpoint ...
//
// Revision 1.3  2002/09/16 01:13:57  panther
// Removed the ppInput/ppOutput things - handle device closes on
// plugin unload.  Attempting to make sure that we can restore to a
// clean state on exit.... this lets us track lost memory...
//
// Revision 1.2  2002/07/29 01:12:27  panther
// Removed the space before the sound name for /sound to work.
// Added CVS Logging.
//
//
