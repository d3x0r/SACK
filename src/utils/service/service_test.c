#include <stdhdrs.h>
#include <deadstart.h>
#include <service_hook.h>

//#define DEBUG_MEMORY_LEAKS

static LOGICAL programEnd;
static PTASK_INFO task;
static PTHREAD waiting;
static PTHREAD waiting2;
static CTEXTSTR progname;
static CTEXTSTR *args;
static CTEXTSTR startin;
static TEXTCHAR eventName[256];
static HANDLE hRestartEvent;
#ifdef DEBUG_MEMORY_LEAKS
  static HANDLE hDebugMemEvent;
#endif
static LOGICAL useBreak;
static LOGICAL useSignal;
static LOGICAL useInteract;

static void CPROC MyTaskDone( uintptr_t psv, PTASK_INFO task );

static void logOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	int less=-1;
	if( buffer[size +less]=='\n' )
		less--;
	if( less<0 && buffer[size +less]=='\r' )
		less--;
	
	lprintf( "%.*s", size+less+1, buffer );
}

static void runTask( void ) {
	lprintf( "Launching service process..." );
	// task = LaunchUserProcess( progname, NULL, args, 0, NULL, MyTaskDone, 0 DBG_SRC );
	task = LaunchPeerProgramExx( progname, startin, args
	                           , ( useBreak ? LPP_OPTION_USE_CONTROL_BREAK : 0 )
	                           | ( useSignal ? LPP_OPTION_USE_SIGNAL : 0 )
	                           | LPP_OPTION_NEW_GROUP
	                           | LPP_OPTION_NEW_CONSOLE
	                                  | LPP_OPTION_IMPERSONATE_EXPLORER
	                           , logOutput, MyTaskDone, 0 DBG_SRC );
}

static void CPROC MyTaskDone( uintptr_t psv, PTASK_INFO task_done )
{
	task = NULL;
	if( waiting2 ) WakeThread( waiting2 );
	lprintf( "Service Task Exited..." );
	if( !programEnd ) {
		runTask();
	} else {
		if( waiting ) WakeThread( waiting );
	}
}

#ifdef DEBUG_MEMORY_LEAKS
static uintptr_t WaitMemSignal( PTHREAD thread ) {
	//lprintf( "Waiting to restart...%p", hRestartEvent );
	while( TRUE ) {
		DWORD status = WaitForSingleObject( hDebugMemEvent, INFINITE );
		if( status == WAIT_OBJECT_0 ) {
			DebugDumpHeapMemEx( NULL, 1 );
		}
	}
}
#endif

static uintptr_t WaitRestart( PTHREAD thread ) {
	//lprintf( "Waiting to restart...%p", hRestartEvent );
	while( TRUE ) {
		PTASK_INFO pOldTask;
		while( !task ) Relinquish();
		pOldTask = task;
		DWORD status = WaitForSingleObject( hRestartEvent, INFINITE );
		if( status == WAIT_OBJECT_0 ) {
			if( programEnd ) break; // this program is exiting, don't restart it...
			waiting2 = thread;
			lprintf( "Ending service task to restart..." );
			StopProgram( task );
			WakeableSleep( 2000 );
			lprintf( "waited a bit but still have a task %p %p", task, pOldTask );
			if( task == pOldTask ) {
				WakeableSleep( 2000 );
				if( task == pOldTask ) {
					lprintf( "Task still running... generating terminate : %p %p", task, pOldTask );
					TerminateProgram( task );
				}
				// wait for task to at least exit, and maybe restart.
				while( task == pOldTask ) {
					WakeableSleep( 100 );
				}
			}
			waiting2 = NULL;
		}
	}
	return 0;
}

void Start( void )
{
	runTask();
}

int main( int argc, char **argv )
{
	FLAGSET( opts, SYSLOG_OPT_MAX );
	InvokeDeadstart();
	SETFLAG( opts, SYSLOG_OPT_OPEN_BACKUP );
	SETFLAG( opts, SYSLOG_OPT_LOG_SOURCE_FILE );
	SystemLogTime( SYSLOG_TIME_LOG_DAY | SYSLOG_TIME_HIGH );
	SetSyslogOptions( opts );
	SetSystemLog( SYSLOG_AUTO_FILE, NULL );
	SetSystemLoggingLevel( 2000 );

	if( argc == 1 ) {
		lprintf( "usage: <service options> [install/uninstall] <options> [service_description] <task> <start-in path> <environment... --> <arguments....>" );
		lprintf( "   service options:" );
		lprintf( "       -interact : configures self as an interactive service.\n" );
		lprintf( "   options:" );
		lprintf( "       -break : use ctrl-break to terminate child." );
		lprintf( "       -signal : use process signal to terminate child." );
		lprintf( "   environment : additional environment variables to set in the form 'name=value'." );
		lprintf( "      if an argument to the process looks like an environment value '--' can be used to terminate the environment list." );
		lprintf( "     install [service_description] <task> <start-in path> <arguments....>" );
		lprintf( "     uninstall" );
		printf( "usage: <service options> [install/uninstall] <options> [service_description] <task> <start-in path> <environment... --> <arguments....>\n" );
		printf( "   service options:\n" );
		printf( "       -interact : configures self as an interactive service.\n" );
		printf( "   options:\n" );
		printf( "       -break : use ctrl-break to terminate child.\n" );
		printf( "       -signal : use process signal to terminate child.\n" );
		printf( "   environment : additional environment variables to set in the form 'name=value'.\n" );
		printf( "      if an argument to the process looks like an environment value '--' can be used to terminate the environment list.\n" );
		printf( "     install [service_description] <task> <start-in path> <arguments....>\n" );
		printf( "     uninstall\n" );
		return 0;
	}
	snprintf( eventName, 256, "Global\\%s:restart", GetProgramName() );

	while( argc > 1 )
	{
		if( StrCaseCmp( argv[1], "uninstall" ) == 0 ) {
			ServiceUninstall( GetProgramName() );
			return 0;
		} else if( StrCaseCmp( argv[1], "restart" ) == 0 ) {
			HANDLE hEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, eventName );
			if( hEvent != INVALID_HANDLE_VALUE ) if( !SetEvent( hEvent ) ) lprintf( "Failed to set event? %d", GetLastError() );
			return 0;			
		} else if( StrCaseCmp( argv[1], "install" ) == 0 ) {
			PVARTEXT pvt = VarTextCreate();
			{
				int first = 1;
				int argofs = 3;
				args = argv + 3;
				while( ( argofs < argc ) && args && args[0] )
				{
					//lprintf( "arg is %s", args[0] );
					if( args[0][0] == 0 )
						vtprintf( pvt,  "%s\"\"" , first ? "" : " " );
					else if( StrChr( args[ 0 ], '\\' ) ) {
						char c;
						char *str = args[ 0 ];
						int inQuotes = 0;
						if( StrChr( args[ 0 ], ' ' ) )
							inQuotes = 1;

						if( !first )
							VarTextAddCharacter( pvt, ' ' );
						if( inQuotes )
							VarTextAddCharacter( pvt, '"' );
						for( ; ( c = *str ); str++ ) {
							if( c == '\\' ) {
								VarTextAddCharacter( pvt, '\\' );
								VarTextAddCharacter( pvt, '\\' );
							} else if( c == '\"' ) {
								VarTextAddCharacter( pvt, '\\' );
								VarTextAddCharacter( pvt, '\"' );
							} else if( c == '\'' ) {
								VarTextAddCharacter( pvt, '\\' );
								VarTextAddCharacter( pvt, '\'' );
							} else
								VarTextAddCharacter( pvt, c );
						}
						if( inQuotes )
							VarTextAddCharacter( pvt, '"' );
					} else if( StrChr( args[ 0 ], ' ' ) )
						vtprintf( pvt, "%s\\\"%s\\\"", first?"":" ", args[0] );
					else
						vtprintf( pvt, "%s%s", first ? "" : " ", args[0] );
					first = 0;
					args++;
					argofs++;
				}
			}
			ServiceInstallEx( GetProgramName(), argv[2], GetText( VarTextGet( pvt ) ), useInteract?1:0 );
			return 0;
		} else if( StrCaseCmp( argv[1], "-interact" ) == 0 ) {
			useInteract = 1;
		} else {
			int argofs = 3;
			progname = argv[1];
			startin = argv[2];
			while( argofs < argc ){
				const char *p;
				if( argv[argofs][0] == '-' ) {
					// --  force end of options
					if( argv[argofs][1] == '-'
						&& argv[argofs][2] == 0
						){
						argofs++;
						break;
					} else if( StrCaseCmp( argv[argofs] + 1, "break" ) == 0 ) {
						useBreak = 1;
					} else if( StrCaseCmp( argv[argofs] + 1, "signal" ) == 0 ) {
						useSignal = 1;
					} else
						lprintf( "Bad option:%s", argv[argofs] );
					argofs++;
				}
				else if( ( p = StrChr( argv[argofs], '=' ) ) ){
					char *tmpname = DupCStrLen( argv[argofs], p-argv[argofs] );
					char *tmpval = DupCStr( p+1 );
					lprintf( "Setting Environment: %s = %s", tmpname, tmpval );
					OSALOT_SetEnvironmentVariable( tmpname, tmpval );
					Release( tmpname );
					Release( tmpval );
					argofs++;
				} else break;
			}
			args = NewArray( CTEXTSTR, argc );
			args[0] = progname;
			{
				int n;
				for( n = argofs; n < argc; n++ ) {
					args[n-(argofs-1)] = argv[n];
				}
				args[n-(argofs-1)] = NULL;
			}
			break;
		}
		argc--;
		argv++;
	}
	
	{
		PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR)LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH );
		InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION );
		SetSecurityDescriptorDacl( psd, TRUE, NULL, FALSE );

		SECURITY_ATTRIBUTES sa = { 0 };
		sa.nLength = sizeof( sa );
		sa.lpSecurityDescriptor = psd;
		sa.bInheritHandle = FALSE;

		//HANDLE hEvent = CreateEvent( &sa, TRUE, FALSE, TEXT( "Global\\Test" ) );
		hRestartEvent = CreateEvent( &sa, FALSE, FALSE, eventName );
#ifdef DEBUG_MEMORY_LEAKS
		snprintf( eventName, 256, "Global\\%s:dumpmem", GetProgramName() );
		hDebugMemEvent = CreateEvent( &sa, FALSE, FALSE, eventName );
#endif
		LocalFree( psd );

	}
	ThreadTo( WaitRestart, 0 );
#ifdef DEBUG_MEMORY_LEAKS
	ThreadTo( WaitMemSignal, 0 );
#endif
	SetupService( (TEXTSTR)GetProgramName(), Start );
	programEnd = 1;
	waiting = MakeThread();
	if( task ) {
		StopProgram( task );
		WakeableSleep( 10 );
		if( task ) {
			WakeableSleep( 5000 );
			if( task ) {
				TerminateProgram( task );
			}
		}
	}
	return 0;
}

