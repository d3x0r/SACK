
#include <stdio.h>
#include <sack_system.h>
#include <timers.h>
#include <filesys.h>
int done;
PTHREAD main_thread;
PTASK_INFO task;

void CPROC output( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( "%*.*s", size, size, buffer );
}

void CPROC ended( uintptr_t psv, PTASK_INFO task )
{
	lprintf( "Task has ended." );
	done = 1;
}


SaneWinMain( argc, argv )
{
	int nowait = 0;
	int task_monitor = 0;
	int noinput = 0;
#ifdef __WATCOMC__
	TEXTCHAR **newargv = NewArray( TEXTCHAR*, argc + 1 );
	int cp_argv;
	for( cp_argv = 0; cp_argv < argc; cp_argv++ )
		newargv[cp_argv] = (TEXTCHAR*)argv[cp_argv];
	newargv[cp_argv] = NULL;
#define argv newargv
#endif
	if( argc < 2 )
	{
#ifdef WIN32
		const TEXTCHAR * const args[] ={ "cmd.exe", NULL };
#else
		const TEXTCHAR * const args[] ={ "/bin/sh", NULL };
#endif

		if( !( task = LaunchPeerProgram( args[0], NULL, args, output, ended, 0 ) ) )
			done = 1;
	}
	else
	{
		int delay = 0;
		int offset = 1;
		while( argv[offset] && argv[offset][0] == '-' )
		{
			if( StrCaseCmp( argv[offset]+1, "nowait" ) == 0 )
				nowait = 1;
			if( StrCaseCmp( argv[offset]+1, "taskmon" ) == 0 )
				task_monitor = 1;
			if( StrCaseCmp( argv[offset]+1, "noin" ) == 0 )
				noinput = 1;
			if( StrCaseCmp( argv[offset]+1, "local" ) == 0 )
			{
				SetCurrentPath( OSALOT_GetEnvironmentVariable( "MY_LOAD_PATH" ) );
			}
			if( StrCaseCmp( argv[offset]+1, "delay" ) == 0 )
			{
				delay = atoi( argv[offset+1] );
				offset++; // used an extra parameter
			}
			offset++;
		}
		if( delay )
			WakeableSleep( delay );

		if( offset < argc )
		{
			if( !( task = LaunchPeerProgram( argv[offset], NULL, argv + offset, noinput?NULL:output, ended, 0 ) ) )
				done = 1;
		}
		else
		{
#ifdef WIN32
			const TEXTCHAR * const args[] ={ "cmd.exe", NULL };
#else
			const TEXTCHAR * const args[] ={ "/bin/sh", NULL };
#endif
			if( !( task = LaunchPeerProgram( args[0], NULL, args, noinput?NULL:output, ended, 0 ) ) )
				done = 1;
		}
	}
	main_thread = MakeThread();
	while( !done )
	{
		if( nowait )
			WakeableSleep( 1000 );
		else
		{
			nowait = 1;
		}
	}
	if( task_monitor )
		SendMessage( FindWindow( "TaskMonClass", "Task Completion Monitor" ), WM_USER+500, 0, 0 );
	fprintf( stdout, "Shell Completed." );
	return 0;
}
EndSaneWinMain()
