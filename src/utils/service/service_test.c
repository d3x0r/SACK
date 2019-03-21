#include <stdhdrs.h>
#include <service_hook.h>

static LOGICAL programEnd;
static PTASK_INFO task;
static PTHREAD waiting;
static CTEXTSTR progname;
static CTEXTSTR *args;
static CTEXTSTR startin;


static void CPROC MyTaskDone( uintptr_t psv, PTASK_INFO task );

static void logOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( "%s", buffer );
}

static void runTask( void ) {
	lprintf( "Launching user process..." );
	//task = LaunchUserProcess( progname, NULL, args, 0, NULL, MyTaskDone, 0 DBG_SRC );
	task = LaunchPeerProgramExx( progname, startin, args, 0, logOutput, MyTaskDone, 0 DBG_SRC );
}

static void CPROC MyTaskDone( uintptr_t psv, PTASK_INFO task )
{
	task = NULL;
	lprintf( "task ended." );
	if( !programEnd )
		runTask();
}

void Start( void )
{
	waiting = MakeThread();
	runTask();
	// start is like init...
}

int main( int argc, char **argv )
{
	if( argc == 1 ) {
		lprintf( "usage: [install/uninstall] [service_description] <task> <start-in path> <arguments....>" );
		lprintf( "     install [service_description] <task> <start-in path> <arguments....>" );
		lprintf( "     uninstall" );
		printf( "usage: [install/uninstall] [service_description] <task> <start-in path> <arguments....>\n" );
		printf( "     install [service_description] <task> <start-in path> <arguments....>\n" );
		printf( "     uninstall\n" );
		return 0;
	}
	if( argc > 1 )
	{
		if( StrCaseCmp( argv[1], "uninstall" ) == 0 )
		{
			ServiceUninstall( GetProgramName() );
			return 0;
		}
		else if( StrCaseCmp( argv[1], "install" ) == 0 )
		{
			PVARTEXT pvt = VarTextCreate();
			{
				int first = 1;
				int argofs = 3;
				args = argv + 3;
				while( ( argofs < argc ) && args && args[0] )
				{
					//lprintf( "arg is %s", args[0] );
					if( args[0][0] == 0 )
						vtprintf( pvt, WIDE( "%s\"\"" ), first ? "" : " " );
					else if( StrChr( args[0], ' ' ) )
						vtprintf( pvt, WIDE("%s\\\"%s\\\""), first?"":" ", args[0] );
					else
						vtprintf( pvt, WIDE("%s%s"), first ? "" : " ", args[0] );
					first = 0;
					args++;
					argofs++;
				}
			}
			//lprintf( "args string is [%s]", GetText( VarTextPeek( pvt ) ) );
			ServiceInstallEx( GetProgramName(), argv[2], GetText( VarTextGet( pvt ) ) );
			return 0;
		}
		else {
			progname = argv[1];
			startin = argv[2];
			args = NewArray( CTEXTSTR, argc );
			args[0] = progname;
			{
				int n;
				for( n = 3; n < argc; n++ ) {
					args[n-2] = argv[n];
				}
				args[n-2] = NULL;
			}
		}
	}
	SetupService( (TEXTSTR)GetProgramName(), Start );
	programEnd = 1;
	if( task ) {
		StopProgram( task );
		WakeableSleep( 10 );
		if( task ) {
			WakeableSleep( 50 );
			if( task ) {
				TerminateProgram( task );
			}
		}
	}
	return 0;
}

