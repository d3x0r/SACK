#define DEFINE_DEFAULT_RENDER_INTERFACE

#include <psi.h>
#include <psi/console.h>
#include <render.h>

#ifdef __LINUX__
//#include <termios.h>
#include <pty.h>
#endif

static int done;
static PTHREAD pThread;

void CPROC OutputHandle( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	pcprintf( (PSI_CONTROL)psv, "%s", buffer );
}

void CPROC OutputHandle2( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	pcprintf( (PSI_CONTROL)psv, "%s", buffer );
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
	// if LPP_OPTION_INTERACTIVE
	//    pprintf( (PTASK_INFO)psv, "%s\r", GetText( out ) );
	// else 
	//    pprintf( (PTASK_INFO)psv, "%s\n", GetText( out ) );

	// but really, this should hook to the console key input handler.
	LineRelease( out );
   // for a command prompt, do not echo result.
}

void setTitle( uintptr_t psv, PTEXT text){
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	SetControlText( pc, GetText( text ));
}

static int generateEvent( uintptr_t pc, uint32_t key ) { 
	SendPTYKeyEvent( (PTASK_INFO)pc, key );
	return 0;
}

SaneWinMain( argc, argv )
{
	{
		PTASK_INFO task;
		PSI_CONTROL pc;
		pc = MakeNamedCaptionedControl( NULL, "PSI Console", 0, 0, 640, 480, INVALID_INDEX, "Command Prompt" );
		SetAllocateLogging( TRUE );
		PSIConsoleSetLocalEcho( pc, FALSE );
		DisplayFrame( pc );
		SetSystemLoggingLevel( 2000 );
		SetSystemLog( SYSLOG_AUTO_FILE, 0 );
		//task = LaunchPeerProgram( argc>1?argv[1]:"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe", ".", NULL, OutputHandle, TaskEnded, (uintptr_t)pc );
		task = LaunchPeerProgram_v2( argc>1?argv[1]
#ifdef WIN32
					:"cmd.exe", 
#endif
#ifdef __LINUX__
					:"bash", 
#endif
					".", (char const* const*)(argc>2?argv+2:(char **)NULL)
		                        , LPP_OPTION_FIRST_ARG_IS_ARG /*LPP_OPTION_DO_NOT_HIDE*/
		                         | LPP_OPTION_INTERACTIVE
		                        , OutputHandle
		                        , OutputHandle2
		                        , TaskEnded, (uintptr_t)pc
		                        , NULL
                                  DBG_SRC
				);
		if( task )
		{
			int cols, rows;
			SetConsoleKeyHandler( pc, generateEvent, (uintptr_t)task );
			PSI_Console_GetConsoleSize( pc, &cols, &rows, NULL, NULL );
			SetProcessConsoleSize( task, cols, rows, 0, 0 );
			// process console size has a PTASK_INFO instead of uintptr_t; otherwise this is compatible
			PSI_Console_SetSizeCallback( pc, (void(*)(uintptr_t,int,int,int,int))SetProcessConsoleSize, (uintptr_t)task );
			SetControlUserData( pc, (uintptr_t)task );
			PSIConsoleInputEvent( pc, WindowInput, (uintptr_t)task );
			//PSI_Console_SetWriteCallback( PSI_CONTROL pc, void (*)(uintptr_t, PTEXT), uintptr_t );
			PSI_Console_SetTitleCallback( pc, setTitle, (uintptr_t) pc );

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

