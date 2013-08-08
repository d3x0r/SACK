#include <stdhdrs.h>
#ifdef _MSC_VER
#include <WinSvc.h>
#endif

// so we get the correct import/export attributes on the function
#include <service_hook.h>

SERVICE_NAMESPACE

static struct {
	CTEXTSTR next_service_name;
	void (CPROC*Start)(void);
	void (CPROC*Stop)(void);
	PTRSZVAL (CPROC*StartThread)(PTHREAD);
	PTRSZVAL psvStartThread;
	PTHREAD main_thread;
} local_service_info;
#define l local_service_info

static SERVICE_STATUS ServiceStatus;
static SERVICE_STATUS_HANDLE hStatus;

static void ControlHandler( DWORD request )
{
	//lprintf( WIDE( "Received %08x" ), request );
   switch(request) 
   { 
      case SERVICE_CONTROL_STOP: 
         //WriteToLog(WIDE( "Monitoring stopped." ));
			if( l.Stop )
				l.Stop();

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
			WakeThread( l.main_thread );
         return; 
 
      case SERVICE_CONTROL_SHUTDOWN: 
         //WriteToLog(WIDE( "Monitoring stopped." ));
			if( l.Stop )
				l.Stop();

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
			SetServiceStatus (hStatus, &ServiceStatus);
         WakeThread( l.main_thread );
         return; 

	case SERVICE_CONTROL_PAUSE:
		break;
	//case SERVICE_CONTROL_RESUME:
      default:
         break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus, &ServiceStatus);
 
    return; 
}


static void APIENTRY ServiceMain( _32 argc, TEXTCHAR **argv )
{

   int error; 
 
   ServiceStatus.dwServiceType = 
      SERVICE_WIN32; 
   ServiceStatus.dwCurrentState = 
      SERVICE_START_PENDING; 
   ServiceStatus.dwControlsAccepted   =  
      SERVICE_ACCEPT_STOP | 
		SERVICE_ACCEPT_SHUTDOWN;

   ServiceStatus.dwWin32ExitCode = 0; 
   ServiceStatus.dwServiceSpecificExitCode = 0; 
   ServiceStatus.dwCheckPoint = 0; 
   ServiceStatus.dwWaitHint = 0; 

	//lprintf( WIDE( "Register service handler..." ) );
   hStatus = RegisterServiceCtrlHandler(
      l.next_service_name,
      (LPHANDLER_FUNCTION)ControlHandler); 
   if (hStatus == (SERVICE_STATUS_HANDLE)0) 
   { 
		// Registering Control Handler failed
      lprintf( WIDE( "Failed." ) );
      return; 
	}

	// Initialize Service
	if( l.Start )
	{
      //lprintf( WIDE( "Send the initialize..." ) );
		l.Start();
	}

	if( l.StartThread )
	{
		//lprintf( WIDE( "thread off to start thread..." ) );
		ThreadTo( l.StartThread, l.psvStartThread );
	}

   //lprintf( WIDE( "Startup completd." ) );

   error = 0;//InitService();
   if (error) 
   {
      // Initialization failed
      ServiceStatus.dwCurrentState = 
         SERVICE_STOPPED; 
      ServiceStatus.dwWin32ExitCode = -1; 
      SetServiceStatus(hStatus, &ServiceStatus); 
      return; 
   } 
   // We report the running status to SCM. 
   ServiceStatus.dwCurrentState = SERVICE_RUNNING;
   SetServiceStatus (hStatus, &ServiceStatus);

	l.main_thread = MakeThread();

   //MEMORYSTATUS memory;
   // The worker loop of a service
   while (ServiceStatus.dwCurrentState == 
          SERVICE_RUNNING)
	{
		WakeableSleep( 100000 );
   }
   return; 
}


void SetupService( TEXTSTR name, void (CPROC*Start)( void ) )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.Start = Start;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( WIDE( "Send to startup monitor.." ) );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( WIDE( "Startup monitor failed! %d" ), GetLastError() );
}

void SetupServiceEx( TEXTSTR name, void (CPROC*Start)( void ), void (CPROC*Stop)( void ) )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.Start = Start;
	l.Stop = Stop;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( WIDE( "Send to startup monitor.." ) );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( WIDE( "Startup monitor failed! %d" ), GetLastError() );
}

void SetupServiceThread( TEXTSTR name, PTRSZVAL (CPROC*Start)( PTHREAD ), PTRSZVAL psv )
{
	SERVICE_TABLE_ENTRY ServiceTable[2];
	l.StartThread = Start;
	l.psvStartThread = psv;
	l.next_service_name = name;
	ServiceTable[0].lpServiceName = name;
	ServiceTable[0].lpServiceProc = ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	//lprintf( WIDE( "Send to startup monitor.." ) );
	if( !StartServiceCtrlDispatcher( ServiceTable ) )
		lprintf( WIDE( "Startup monitor failed! %d" ), GetLastError() );

}

//---------------------------------------------------------------------------

LOGICAL IsThisAService( void )
{
	TEXTCHAR buffer[256];
	DWORD length = 0;
	GetUserObjectInformation( GetProcessWindowStation(), UOI_NAME, buffer, 256, &length);
	if (!StrCaseCmp(buffer, WIDE("WinSta0")))
	{
		// normal user session
		return FALSE;
	}
	else
	{
		// service session
		return TRUE;
	}

	return (l.Start != NULL) || (l.StartThread != NULL );
}

//---------------------------------------------------------------------------

static int task_done;
static void CPROC MyTaskEnd( PTRSZVAL psv, PTASK_INFO task )
{
	task_done = 1;
}

static void CPROC GetOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t length )
{
	lprintf( WIDE( "%s" ), buffer );
}

//---------------------------------------------------------------------------

void ServiceInstall( CTEXTSTR ServiceName )
{
	TEXTCHAR **args;
	int nArgs;
	PVARTEXT pvt_cmd = VarTextCreate();
	vtprintf( pvt_cmd, WIDE( "sc create \"%s\" binpath= %s\\%s.exe start= auto" )
			  , ServiceName
			  , GetProgramPath()
			  , GetProgramName() );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( WIDE( "sc.exe" ), NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
	vtprintf( pvt_cmd, WIDE( "sc start \"%s\"" )
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( WIDE( "sc.exe" ), NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
}

//---------------------------------------------------------------------------

void ServiceUninstall( CTEXTSTR ServiceName )
{
	TEXTCHAR **args;
	int nArgs;
	PVARTEXT pvt_cmd = VarTextCreate();
	vtprintf( pvt_cmd, WIDE( "sc stop \"%s\"" )
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( WIDE( "sc" ), NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
	vtprintf( pvt_cmd, WIDE( "sc delete \"%s\"" )
			  , ServiceName );
	ParseIntoArgs( GetText( VarTextPeek( pvt_cmd ) ), &nArgs, &args );
	VarTextEmpty( pvt_cmd );
	LaunchPeerProgram( WIDE( "sc" ), NULL, (PCTEXTSTR)args,  GetOutput, MyTaskEnd, 0 );
	while( !task_done )
		WakeableSleep( 100 );
	task_done = 0;
}

PTASK_INFO LaunchUserProcess( CTEXTSTR program, CTEXTSTR path, PCTEXTSTR args
									 , int flags
									 , TaskOutput OutputHandler
									 , TaskEnd EndNotice
									 , PTRSZVAL psv
									  DBG_PASS
									 )
{
	PTASK_INFO pTask;
	ImpersonateInteractiveUser();
	pTask = LaunchPeerProgramExx( program, path, args, flags|LPP_OPTION_IMPERSONATE_EXPLORER, OutputHandler, EndNotice, psv DBG_RELAY );
   EndImpersonation();
   return pTask;
}

SERVICE_NAMESPACE_END
