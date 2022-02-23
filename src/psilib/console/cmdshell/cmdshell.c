#include <psi.h>
#include <psi/console.h>

static int done;
static PTHREAD pThread;

void CPROC OutputHandle( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	int ofs = 0;
   //lprintf( "output %s", buffer );
	pcprintf( (PSI_CONTROL)psv, "%s", buffer + ofs );
}

void CPROC OutputHandle2( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	int ofs = 0;
	lprintf( "stderr output %s", buffer );
	pcprintf( (PSI_CONTROL)psv, "%s", buffer + ofs );
}
void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task )
{
	done = TRUE;
	WakeThread( pThread );
}

void CPROC WindowInput( uintptr_t psv, PTEXT text )
{
	// collapse text to a single segment.
	PTEXT out = BuildLine( text );
	//LogBinary( GetText( out ), GetTextSize( out ) );
	pprintf( (PTASK_INFO)psv, "%s\n", GetText( out ) );
	LineRelease( out );
   // for a command prompt, do not echo result.
}

SaneWinMain( argc, argv )
{
	{
		PTASK_INFO task;
		PSI_CONTROL pc;
		pc = MakeNamedCaptionedControl( NULL, "PSI Console", 0, 0, 640, 480, INVALID_INDEX, "Command Prompt" );
		PSIConsoleSetLocalEcho( pc, FALSE );
		DisplayFrame( pc );

		//task = LaunchPeerProgram( argc>1?argv[1]:"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", ".", NULL, OutputHandle, TaskEnded, (uintptr_t)pc );
		task = LaunchPeerProgram_v2( argc>1?argv[1]
#ifdef WIN32
					:"cmd.exe", 
#endif
#ifdef __LINUX__
					:"bash", 
#endif
					".", NULL
		                        , 0 /*LPP_OPTION_DO_NOT_HIDE*/
		                        , OutputHandle
		                        , OutputHandle2
		                        , TaskEnded, (uintptr_t)pc
		                        , NULL
                                  DBG_SRC
											);
		if( task )
		{
			PSIConsoleInputEvent( pc, WindowInput, (uintptr_t)task );
			pThread = MakeThread();
			while( !done )
			{
				WakeableSleep( 25000 );
			}
		}
	}
	return 0;
}
EndSaneWinMain()

