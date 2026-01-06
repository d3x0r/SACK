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

#ifdef __LINUX__
void updateSize( uintptr_t psv, int cols, int rows, int width, int height){
	PTASK_INFO task = (PTASK_INFO)psv;
	struct winsize size;
	int pty = GetTaskPTY( task );
	if( !rows ) rows = 24;
	if( !cols ) cols = 80;
	//lprintf( "Set PTY size: %d %d %d", pty, rows, cols);
	size.ws_row = rows;
	size.ws_col = cols;
	size.ws_xpixel = width;
	size.ws_ypixel = height;
	ioctl(pty, TIOCSWINSZ, &size );
}
#endif

static int generateEvent( PSI_CONTROL pc, uint32_t key ) { 
	lprintf( "key event: %08lx", key );
	CTEXTSTR text = GetKeyText( key );
	PTASK_INFO task = (PTASK_INFO)GetControlUserData( pc );
	/*
	* 
	*  from https://github.com/microsoft/terminal/blob/main/doc/specs/%234999%20-%20Improved%20keyboard%20handling%20in%20Conpty.md#cons-4 2026-01-06
	   ^[ [ Vk ; Sc ; Uc ; Kd ; Cs ; Rc _

       Vk: the value of wVirtualKeyCode - any number. If omitted, defaults to '0'.

       Sc: the value of wVirtualScanCode - any number. If omitted, defaults to '0'.

       Uc: the decimal value of UnicodeChar - for example, NUL is "0", LF is
           "10", the character 'A' is "65". If omitted, defaults to '0'.

       Kd: the value of bKeyDown - either a '0' or '1'. If omitted, defaults to '0'.

       Cs: the value of dwControlKeyState - any number. If omitted, defaults to '0'.

       Rc: the value of wRepeatCount - any number. If omitted, defaults to '1'.
	*/
	pprintf( task, "\x1b[%d;%d;%d;%d;%d;%d_", KEY_CODE( key ), KEY_REAL_CODE( key ), text?GetUtfChar( &text ):0,
	         IsKeyPressed( key )?1:0, (KEY_MOD( key ) & KEY_MOD_CTRL)?1:0, 1 );
	return 0; 
}

SaneWinMain( argc, argv )
{
	{
		PTASK_INFO task;
		PSI_CONTROL pc;
		pc = MakeNamedCaptionedControl( NULL, "PSI Console", 0, 0, 640, 480, INVALID_INDEX, "Command Prompt" );
		
		SetCommonKey( pc, generateEvent );

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
#ifdef __LINUX__
			PSI_Console_SetSizeCallback( pc, updateSize, (uintptr_t)task );
#endif		
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

