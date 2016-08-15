#include <stdhdrs.h>
#include <service_hook.h>

static PTASK_INFO task;
static PTHREAD waiting;

static void CPROC MyTaskDone( uintptr_t psv, PTASK_INFO task )
{
	task = NULL;
   WakeThread( waiting );
}

void Start( void )
{
	CTEXTSTR args[3];
	args[0] = "whatever";
	args[1] = "test.txt";
	args[2] = NULL;
   lprintf( "Launching user process..." );
	task = LaunchUserProcess( "notepad", NULL, args, 0, NULL, MyTaskDone, 0 DBG_SRC );
#if 0
	waiting = MakeThread();

	WakeableSleep( 10000 );
	if( task )
		StopProgram( task );
#endif
   // start is like init...
}

int main( int argc, char **argv )
{
	if( argc > 1 )
	{
		if( StrCaseCmp( argv[1], "install" ) )
		{
         ServiceUninstall( GetProgramName() );
         return 0;
		}
		else if( StrCaseCmp( argv[1], "uninstall" ) )
		{
         ServiceInstall( GetProgramName() );
         return 0;
		}
	}
   SetSystemLog( SYSLOG_FILENAME, "service.log" );
	SetupService( (TEXTSTR)GetProgramName(), Start );
	if( task )
      StopProgram( task );
   return 0;
}

