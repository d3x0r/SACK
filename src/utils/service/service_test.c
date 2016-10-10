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
		lprintf( "usage: <task> <path> <arguments....>" );
		printf( "usage: <task> <path> <arguments....>" );
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
				args = argv + 3;
				while( ( argc > 2 ) && args && args[0] )
				{
					lprintf( "arg is %s", args[0] );
					if( args[0][0] == 0 )
						vtprintf( pvt, WIDE( "%s\"\"" ), first ? "" : " " );
					else if( StrChr( args[0], ' ' ) )
						vtprintf( pvt, WIDE("%s\\\"%s\\\""), first?"":" ", args[0] );
					else
						vtprintf( pvt, WIDE("%s%s"), first ? "" : " ", args[0] );
					first = 0;
					args++;
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

