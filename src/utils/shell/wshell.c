
#include <stdio.h>
#include <sack_system.h>
#include <timers.h>
#include <filesys.h>
int done;
PTHREAD main_thread;
PTASK_INFO task;

void CPROC output( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( WIDE("%*.*s"), size, size, buffer );
}

void CPROC ended( PTRSZVAL psv, PTASK_INFO task )
{
   lprintf( WIDE("Task has ended.") );
   done = 1;
}


SaneWinMain( argc, argv )
{
   int nowait = 0;
	int pos = 0;
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
		const TEXTCHAR * const args[] ={ WIDE("cmd.exe"), NULL };
#else
		const TEXTCHAR * const args[] ={ WIDE("/bin/sh"), NULL };
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
			if( StrCaseCmp( argv[offset]+1, WIDE("nowait") ) == 0 )
            nowait = 1;
			if( StrCaseCmp( argv[offset]+1, WIDE("pos") ) == 0 )
            pos = 1;
			if( StrCaseCmp( argv[offset]+1, WIDE("noin") ) == 0 )
            noinput = 1;
			if( StrCaseCmp( argv[offset]+1, WIDE("local") ) == 0 )
			{
            SetCurrentPath( OSALOT_GetEnvironmentVariable( WIDE("MY_LOAD_PATH") ) );
			}
			if( StrCaseCmp( argv[offset]+1, WIDE("delay") ) == 0 )
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
			const TEXTCHAR * const args[] ={ WIDE("cmd.exe"), NULL };
#else
			const TEXTCHAR * const args[] ={ WIDE("/bin/sh"), NULL };
#endif
			if( !( task = LaunchPeerProgram( args[0], NULL, args, noinput?NULL:output, ended, 0 ) ) )
				done = 1;
		}
	}
	main_thread = MakeThread();
	while( !done )
	{
		TEXTCHAR buf[256];
		if( nowait )
			WakeableSleep( 1000 );
		else
		{
         nowait = 1;
		}
	}
   if( pos )
		SendMessage( FindWindow( WIDE("TaskMonClass"), WIDE("Task Completion Monitor") ), WM_USER+500, 0, 0 );
   fprintf( stdout, WIDE("Shell Completed.") );
   return 0;
}
EndSaneWinMain()
