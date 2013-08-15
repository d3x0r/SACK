
#include <stdio.h>
#include <sack_system.h>
#include <timers.h>
#include <filesys.h>
int done;
PTHREAD main_thread;
PTASK_INFO task;

void CPROC output( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	fprintf( stdout, "%*.*s", size, size, buffer );
   fflush( stdout );
}

void CPROC ended( PTRSZVAL psv, PTASK_INFO task )
{
   fprintf( stderr, "Task has ended." );
   done = 1;
}


int main( int argc, char const *const*argv )
{
   int nowait = 0;
	int pos = 0;
	int noinput = 0;
#ifdef __WATCOMC__
	char **newargv = NewArray( char*, argc + 1 );
   int cp_argv;
	for( cp_argv = 0; cp_argv < argc; cp_argv++ )
		newargv[cp_argv] = (char*)argv[cp_argv];
	newargv[cp_argv] = NULL;
#define argv newargv
#endif
	if( argc < 2 )
	{
#ifdef WIN32
		const char * const args[] ={ "cmd.exe", NULL };
#else
		const char * const args[] ={ "/bin/sh", NULL };
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
			if( StrCaseCmp( argv[offset]+1, "pos" ) == 0 )
            pos = 1;
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
			if( !( task = LaunchPeerProgram( argv[offset], NULL, (char const*const*)argv + offset, noinput?NULL:output, ended, 0 ) ) )
				done = 1;
		}
		else
		{
#ifdef WIN32
			const char * const args[] ={ "cmd.exe", NULL };
#else
			const char * const args[] ={ "/bin/sh", NULL };
#endif
			if( !( task = LaunchPeerProgram( args[0], NULL, args, noinput?NULL:output, ended, 0 ) ) )
				done = 1;
		}
	}
	main_thread = MakeThread();
	while( !done )
	{
		char buf[256];
		if( nowait )
			WakeableSleep( 1000 );
		else
		{
			if( !noinput )
			{
				if( fgets( buf, 256, stdin ) )
				{
					pprintf( task, "%s", buf );
				}
			}
			else
            nowait = 1;
		}
	}
   if( pos )
		SendMessage( FindWindow( "TaskMonClass", "Task Completion Monitor" ), WM_USER+500, 0, 0 );
   fprintf( stdout, "Shell Completed." );
   return 0;
}
