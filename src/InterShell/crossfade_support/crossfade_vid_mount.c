#include <stdhdrs.h>
#include <network.h>
#include <psi.h>
#include <sqlgetoption.h>
#include "../intershell_registry.h"
#include "../intershell_export.h"

struct crossfade_app_address
{
	CTEXTSTR send_from;
	CTEXTSTR send_to;
	PCLIENT pc;
	SOCKADDR *sendto;
};

struct crossfade_app_tracker
{
	CTEXTSTR title;
	CTEXTSTR classname;
	S_32 last_x, last_y;
	_32 last_w, last_h;
#ifdef WIN32
	HWND hWnd;
#endif
	struct {
		BIT_FIELD bShown : 1;
		BIT_FIELD bWantShow : 1;
		BIT_FIELD bFound : 1;
		BIT_FIELD visible : 1;
	} flags;
	struct crossfade_app_address *socket;
};

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	CTEXTSTR app_window_name;
	CTEXTSTR app_class_name;
	CTEXTSTR send_from;
	CTEXTSTR send_to;
	struct crossfade_app_tracker *app;
};

static struct {
	struct {
		BIT_FIELD shutdown : 1;
		BIT_FIELD kill_complete : 1;
	} flags;
	PTHREAD waiting;
	PLIST controls;
	PLIST sockets;
	PLIST apps;
	//PCLIENT udp_socket;
	//SOCKADDR *sendto;
	struct crossfade_app_tracker *crossfade;
	TEXTCHAR video_application[256];
	TEXTCHAR video_application_args[1024];
	PLIST video_players;
	_32 last_attempt;
	PTASK_INFO task;
	PTHREAD launch_thread;
} local_crossfade_app_mount;
#define l local_crossfade_app_mount

enum {
	EDIT_CROSSFADE_MOUNT_WINDOW_NAME = 4100,
	EDIT_CROSSFADE_MOUNT_CLASS_NAME,
	EDIT_CROSSFADE_MOUNT_ADDRESS,
	EDIT_CROSSFADE_MOUNT_SEND_FROM,
};

EasyRegisterControl( WIDE("Crossfade Media Mount"), sizeof( MY_CONTROL) );

PRELOAD( RegisterKeypadIDs )
{
	SACK_GetProfileString( GetProgramName(), WIDE("Crossfade Player/application path"), WIDE("@/crossfade_vid_playlist.exe"), l.video_application, sizeof( l.video_application ) );
	SACK_GetProfileString( GetProgramName(), WIDE("Crossfade Player/application args"), WIDE("@crossfade_vid_playlist.config"), l.video_application_args, sizeof( l.video_application_args ) );

	EasyRegisterResource( WIDE("InterShell/Crossfade Media Mount") _WIDE(TARGETNAME), EDIT_CROSSFADE_MOUNT_WINDOW_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Crossfade Media Mount") _WIDE(TARGETNAME), EDIT_CROSSFADE_MOUNT_CLASS_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Crossfade Media Mount") _WIDE(TARGETNAME), EDIT_CROSSFADE_MOUNT_ADDRESS, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Crossfade Media Mount") _WIDE(TARGETNAME), EDIT_CROSSFADE_MOUNT_SEND_FROM, EDIT_FIELD_NAME );
}

//------------------------- External program loader -------------------------------------------
//------------------------------------------------------------------------------------------
ATEXIT( CloseVlcWindows )
{
#if defined( DEBUG_MESSAGES )
	lprintf( WIDE("Terminating task...") );
#endif
	l.flags.shutdown = 1;
	TerminateProgram( l.task );
}

static void OnBeginShutdown( WIDE("VLC Stream Host") )( void )
{
	l.flags.shutdown = 1;
	TerminateProgram( l.task );
}


void CPROC KilTaskOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	// pskill output; don't really care
}

void CPROC KillTaskEnded( PTRSZVAL psv, PTASK_INFO task )
{
	// don't respawn when exiting.
	l.flags.kill_complete = TRUE;
	WakeThread( l.launch_thread );
}

static void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task )
{
	if( task )
		l.task = NULL;

	// don't respawn when exiting.
	if( l.flags.shutdown )
		return;

	if( IsSystemShuttingDown() )
	{
		lprintf( WIDE("Do not re-start tasks, system is shutting down.") );
		return;
	}

	if( !l.task )
	{
		PTASK_INFO tmp_task;
		CTEXTSTR *args;
		CTEXTSTR executable_name;
		args = NewArray( CTEXTSTR, 3 );
		args[0] = WIDE("pskill");
		executable_name = pathrchr( l.video_application );
		if( executable_name )
			executable_name++;
		else
			executable_name = l.video_application;
		args[1] = executable_name;
		args[2] = NULL;
		l.flags.kill_complete = 0;
		tmp_task = LaunchPeerProgram( WIDE("pskill"), NULL, args, KilTaskOutput, KillTaskEnded, 0 );
		if( tmp_task )
		{
			l.launch_thread = MakeThread();
			while( !l.flags.kill_complete )
				WakeableSleep( 10000 );
		}
		{
			int n;
			int argc;
			TEXTCHAR **argv;
			ParseIntoArgs( l.video_application_args, &argc, &argv );
			if( argc > 2 )
				args = NewArray( CTEXTSTR, 2 + argc );
			for( n = 0; n < argc; n++ )
				args[ 1 + n ] = argv[n];
			args[1+n] = NULL;
		}
		args[0] = l.video_application;

		if( ( l.last_attempt + 5000 ) < GetTickCount() )
		{
			l.task = LaunchProgramEx( l.video_application, NULL, args, TaskEnded, 0 );
		}
		l.last_attempt = GetTickCount();
	}
}

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------


static PTRSZVAL OnEditControl( WIDE( "Crossfade Media Mount" ) )( PTRSZVAL psv, PSI_CONTROL pc_parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, WIDE("ConfigureApplicationMount.isFrame") );
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );

		SetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_WINDOW_NAME ), control->app_window_name?control->app_window_name:WIDE("") );
		SetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_CLASS_NAME ), control->app_class_name?control->app_class_name:WIDE("") );
		SetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_ADDRESS ), control->send_to?control->send_to:WIDE("") );
		SetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_SEND_FROM ), control->send_from?control->send_from:WIDE("") );
		DisplayFrameOver( frame, pc_parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR name[258];
			GetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_WINDOW_NAME ), name, sizeof( name ) );
			if( StrLen( name ) )
			{
				if( control->app_window_name )
					Release( (POINTER)control->app_window_name );
				control->app_window_name = StrDup( name );
			}
			else
			{
				if( control->app_window_name )
				{
					Release( (POINTER)control->app_window_name );
					control->app_window_name = NULL;
				}
			}

			GetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_CLASS_NAME ), name, sizeof( name ) );
			if( StrLen( name ) )
			{
				if( control->app_class_name )
					Release( (POINTER)control->app_class_name );
				control->app_class_name = StrDup( name );
			}
			else
			{
				if( control->app_class_name )
				{
					Release( (POINTER)control->app_class_name );
					control->app_class_name = NULL;
				}
			}

			GetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_CLASS_NAME ), name, sizeof( name ) );
			if( StrLen( name ) )
			{
				if( control->app_class_name )
					Release( (POINTER)control->app_class_name );
				control->app_class_name = StrDup( name );
			}
			else
			{
				if( control->app_class_name )
				{
					Release( (POINTER)control->app_class_name );
					control->app_class_name = NULL;
				}
			}

			GetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_ADDRESS ), name, sizeof( name ) );
			if( StrLen( name ) )
			{
				if( control->send_to )
					Release( (POINTER)control->send_to );
				control->send_to = StrDup( name );
			}
			else
			{
				if( control->send_to )
				{
					Release( (POINTER)control->send_to );
					control->send_to = NULL;
				}
			}

			GetControlText( GetControl( frame, EDIT_CROSSFADE_MOUNT_SEND_FROM ), name, sizeof( name ) );
			if( StrLen( name ) )
			{
				if( control->send_from )
					Release( (POINTER)control->send_from );
				control->send_from = StrDup( name );
			}
			else
			{
				if( control->send_from )
				{
					Release( (POINTER)control->send_from );
					control->send_from = NULL;
				}
			}

		}
		DestroyFrame( &frame );
	}
	return psv;
}

static void CPROC read_complete( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *sa )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	else
	{
	}
	ReadUDP( pc, buffer, 4096 );
}

void SendHide( struct crossfade_app_address* socket )
{
	_32 buffer[2];
	buffer[0] = timeGetTime();
	buffer[1] = 1;
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
}

void SendShow( struct crossfade_app_address* socket )
{
	_32 buffer[2];
	buffer[0] = timeGetTime();
	buffer[1] = 2;
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
}

void SendPosition( struct crossfade_app_address* socket, int x, int y, int w, int h )
{
	S_32 buffer[6];
	buffer[0] = timeGetTime();
	buffer[1] = 3;
	buffer[2] = x;
	buffer[3] = y;
	buffer[4] = w;
	buffer[5] = h;
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
	SendUDPEx( socket->pc, buffer, sizeof( buffer ), socket->sendto );
}


struct crossfade_app_address *ConnectTo( CTEXTSTR to, CTEXTSTR from )
{
	if( to && from )
	{
		struct crossfade_app_address *socket;
		INDEX idx;
		LIST_FORALL( l.sockets, idx, struct crossfade_app_address *, socket )
		{
			if( StrCaseCmp( socket->send_to, to ) == 0 )
			{
				break;
			}
		}
		if( !socket )
		{
			socket = New( struct crossfade_app_address );
			NetworkStart();
			socket->send_to = to;
			socket->send_from = from;
			socket->pc = ServeUDP( from, 3021, read_complete, NULL );
			if( socket->pc )
			{
				UDPEnableBroadcast( socket->pc, TRUE );
				socket->sendto = CreateSockAddress( to, 3020 );
			}
			else
				socket->sendto = NULL;
			AddLink( &l.sockets, socket );
		}
		return socket;
	}
	return NULL;
}


struct crossfade_app_tracker *FindAppWindow( CTEXTSTR send_to, CTEXTSTR send_from )
{
	{
		if( !l.crossfade )
		{
			l.crossfade = New( struct crossfade_app_tracker );
			MemSet( l.crossfade, 0, sizeof( struct crossfade_app_tracker ) );
			l.crossfade->flags.bWantShow = 0;
			l.crossfade->flags.bShown = 0;
			l.crossfade->flags.bFound = 0;
			if( send_to && send_from )
			{
				l.crossfade->socket = ConnectTo( send_to, send_from );
			}
		}
		return l.crossfade;
	}
	return NULL;
}


static PTRSZVAL CPROC WaitForApplication( PTHREAD thread )
{
	LOGICAL need_change;
	LOGICAL need_find;
	PSI_CONTROL pc;
	do
	{
		struct crossfade_app_tracker *app;
		//xlprintf(2100)( "awake..." );
		need_change = FALSE;
		need_find = FALSE;
		//LIST_FORALL( l.apps, idx, struct crossfade_app_tracker *, app )
		{
			INDEX idx2;
			LIST_FORALL( l.controls, idx2, PSI_CONTROL, pc )
			{
				MyValidatedControlData( PMY_CONTROL, control, pc );
				control->app = FindAppWindow( control->send_to, control->send_from );
				app = control->app;
				//xlprintf(2100)( "control %p %p", control, pc );
				if( app && app->flags.bWantShow )
				{
					//lprintf( "Want to be shown..." );
					if( !app->flags.bFound || (!app->flags.bShown || !app->flags.visible) )
					{
						if( control->app_class_name || control->app_window_name )
						{
							//lprintf( "hWnd = %p", app->hWnd );
							if( app->hWnd )
							{
								ShowWindow( app->hWnd, SW_RESTORE );
								SetWindowPos( app->hWnd, HWND_TOPMOST, app->last_x, app->last_y, app->last_w, app->last_h, 0 );
								app->flags.bShown = 1;
								app->flags.bFound = 1;
							}
							else
							{
								app->flags.bFound = 0;
								need_change = 1;
							}
						}
						else
						{
							if( app->socket )
							{
								SendPosition( app->socket, app->last_x, app->last_y, app->last_w, app->last_h );
								app->flags.bFound = 1;
							}
						}
					}
				}
				else
				{
					//xlprintf(2100)( WIDE("Want to be hidden...") );
					if( app && ( !app->flags.bFound || app->flags.bShown || app->flags.visible ) )
					{
						//xlprintf(2100)( WIDE("...") );
						{
							if( app->socket )
							{
								SendHide( app->socket );
								app->flags.bFound = 1;
							}
						}
					}
					else
					{
						//lprintf( WIDE("no app?") );
					}
				}
			}
		}
		if( !need_change )
			WakeableSleep( 4000 );
	} while( 1 );
	l.waiting = NULL;
	return 0;
}

static PTRSZVAL OnCreateControl( WIDE("Crossfade Media Mount") )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, WIDE("Crossfade Media Mount"), x, y, w, h, -1 );
	{
		MyValidatedControlData( PMY_CONTROL, control, pc );

		AddLink( &l.controls, pc );
	}
	if( !l.waiting )
	{
		TaskEnded( 0, NULL );
		l.waiting = ThreadTo( WaitForApplication, 0 );
	}
	return (PTRSZVAL)pc;
}

static void OnFinishInit( WIDE("Crossfade Media Mount") )( PCanvasData pc_canvas )
{
	if( !l.waiting )
		l.waiting = ThreadTo( WaitForApplication, 0 );
}

static PSI_CONTROL OnGetControl( WIDE("Crossfade Media Mount") )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}

static void OnSaveControl( WIDE("Crossfade Media Mount") )( FILE* file, PTRSZVAL psv )
{
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	fprintf( file, WIDE("Crossfade Server Address=%s\n"), control->send_to?control->send_to:WIDE("") );
	fprintf( file, WIDE("Crossfade Send From Address=%s\n"), control->send_from?control->send_from:WIDE("") );
}

static PTRSZVAL CPROC SetApplicationName( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	if( StrLen( name ) )
		control->app_window_name = StrDup( name );
	return psv;
}

static PTRSZVAL CPROC SetApplicationClass( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	if( StrLen( name ) )
		control->app_class_name = StrDup( name );
	return psv;
}

static PTRSZVAL CPROC SetApplicationSendFrom( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	if( StrLen( name ) )
		control->send_from = StrDup( name );
	return psv;
}

static PTRSZVAL CPROC SetApplicationSendTo( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	if( StrLen( name ) )
		control->send_to = StrDup( name );
	return psv;
}

static void OnLoadControl( WIDE("Crossfade Media Mount") )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	AddConfigurationMethod( pch, WIDE("Application Server Address=%m\n"), SetApplicationSendTo );
	AddConfigurationMethod( pch, WIDE("Application Send From Address=%m\n"), SetApplicationSendFrom );
}

static void OnHideCommon( WIDE("Crossfade Media Mount") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		//lprintf( WIDE("begin hide...") );
		control->app = FindAppWindow( control->send_to, control->send_from );
		control->app->flags.bWantShow = 0;
		if( !l.waiting )
			l.waiting = ThreadTo( WaitForApplication, 0 );
		else
			WakeThread( l.waiting );
		//lprintf( WIDE("began hide...") );
	}
}

static void OnRevealCommon( WIDE("Crossfade Media Mount") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		S_32 x = 0;
		S_32 y = 0;
		Image image = GetControlSurface( pc );
		//lprintf( WIDE("begin show(move)...") );
		GetPhysicalCoordinate( pc, &x, &y, TRUE );

		control->app = FindAppWindow( control->send_to, control->send_from );
		control->app->last_x = x;
		control->app->last_y = y;
		control->app->last_w = image->width;
		control->app->last_h = image->height;
		control->app->flags.bWantShow = 1;
		if( !l.waiting )
			l.waiting = ThreadTo( WaitForApplication, 0 );
		else
			WakeThread( l.waiting );
		//lprintf( WIDE("begin show(move)...") );
	}
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif

