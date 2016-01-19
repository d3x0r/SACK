
#include <stdhdrs.h>
#include <network.h>
#include "../intershell_registry.h"
#include "../intershell_export.h"

struct application_network_address
{
	CTEXTSTR send_from;
	CTEXTSTR send_to;
	PCLIENT pc;
	SOCKADDR *sendto;
};

struct application_tracker
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
	struct application_network_address *socket;
	PLIST controls;
};

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	CTEXTSTR app_window_name;
	CTEXTSTR app_class_name;
	CTEXTSTR send_from;
	CTEXTSTR send_to;
	struct application_tracker *app;
};

static struct {
	PTHREAD waiting;
	PLIST controls;
	PLIST sockets;
	PLIST apps;
	//PCLIENT udp_socket;
	//SOCKADDR *sendto;
} local_application_mount;
#define l local_application_mount

enum {
	EDIT_APP_WINDOW_NAME = 4100,
	EDIT_APP_CLASS_NAME,
	EDIT_APP_ADDRESS,
	EDIT_APP_SEND_FROM,
};
EasyRegisterControl( WIDE("Application Mount"), sizeof( MY_CONTROL) );

PRELOAD( RegisterKeypadIDs )
{
	EasyRegisterResource( WIDE("InterShell/Application Mount") _WIDE(TARGETNAME), EDIT_APP_WINDOW_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Application Mount") _WIDE(TARGETNAME), EDIT_APP_CLASS_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Application Mount") _WIDE(TARGETNAME), EDIT_APP_ADDRESS, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Application Mount") _WIDE(TARGETNAME), EDIT_APP_SEND_FROM, EDIT_FIELD_NAME );
}

static PTRSZVAL OnEditControl( WIDE( "Application Mount" ) )( PTRSZVAL psv, PSI_CONTROL pc_parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, WIDE("ConfigureApplicationMount.isFrame") );
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );

		SetControlText( GetControl( frame, EDIT_APP_WINDOW_NAME ), control->app_window_name?control->app_window_name:WIDE("") );
		SetControlText( GetControl( frame, EDIT_APP_CLASS_NAME ), control->app_class_name?control->app_class_name:WIDE("") );
		SetControlText( GetControl( frame, EDIT_APP_ADDRESS ), control->send_to?control->send_to:WIDE("") );
		SetControlText( GetControl( frame, EDIT_APP_SEND_FROM ), control->send_from?control->send_from:WIDE("") );
		DisplayFrameOver( frame, pc_parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR name[258];
			GetControlText( GetControl( frame, EDIT_APP_WINDOW_NAME ), name, sizeof( name ) );
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

			GetControlText( GetControl( frame, EDIT_APP_CLASS_NAME ), name, sizeof( name ) );
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

			GetControlText( GetControl( frame, EDIT_APP_CLASS_NAME ), name, sizeof( name ) );
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

			GetControlText( GetControl( frame, EDIT_APP_ADDRESS ), name, sizeof( name ) );
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

			GetControlText( GetControl( frame, EDIT_APP_SEND_FROM ), name, sizeof( name ) );
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

void SendHide( struct application_network_address* socket )
{
	SendUDPEx( socket->pc, "<hide/>", 7, socket->sendto );
}

void SendShow( struct application_network_address* socket )
{
	SendUDPEx( socket->pc, "<show/>", 7, socket->sendto );
}

void SendPosition( struct application_network_address* socket, int x, int y, int w, int h )
{
	char tmp[128];
	int len;
#undef snprintf
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
	len = snprintf( tmp, sizeof( tmp ), "<move x=\"%d\" y=\"%d\" width=\"%d\" Height=\"%d\"/>", x, y, w, h );
	SendUDPEx( socket->pc, tmp, len, socket->sendto );
}


struct application_network_address *ConnectTo( CTEXTSTR to, CTEXTSTR from )
{
	if( to && from )
	{
		struct application_network_address *socket;
		INDEX idx;
		LIST_FORALL( l.sockets, idx, struct application_network_address *, socket )
		{
			if( StrCaseCmp( socket->send_to, to ) == 0 )
			{
				break;
			}
		}
		if( !socket )
		{
			socket = New( struct application_network_address );
			NetworkStart();
			socket->send_to = to;
			socket->send_from = from;
			socket->pc = ServeUDP( from, 2996, read_complete, NULL );
			if( socket->pc )
			{
				UDPEnableBroadcast( socket->pc, TRUE );
				socket->sendto = CreateSockAddress( to, 2997 );
			}
			else
				socket->sendto = NULL;
			AddLink( &l.sockets, socket );
		}
		return socket;
	}
	return NULL;
}


struct application_tracker *FindAppWindow( PMY_CONTROL app_control )
{
	if( app_control && ( app_control->app_window_name || app_control->app_class_name ) )
	{
		struct application_tracker *app;
		INDEX idx;
		CTEXTSTR name= app_control->app_window_name;
		CTEXTSTR classname = app_control->app_class_name;
		CTEXTSTR send_to = app_control->send_to;
		CTEXTSTR send_from = app_control->send_from;
		//lprintf( "Finding window... %p %s %s", app_control, name, classname );
		LIST_FORALL( l.apps, idx, struct application_tracker *, app )
		{
			if( app->title && ( StrCaseCmp( app->title, name ) == 0 ) )
			{
				// update hwnd, it might have exited
				app->hWnd = FindWindow( app->classname, app->title );
				//lprintf(" Find of %s %s = %p", app->title, app->classname, app->hWnd );
				break;
			}
			if( app->classname && ( StrCaseCmp( app->classname, classname ) == 0 ) )
			{
				// update hwnd, it might have exited
				app->hWnd = FindWindow( app->classname, app->title );
				//lprintf(" Find of %s %s = %p", app->title, app->classname, app->hWnd );
				break;
			}
			if( FindLink( &app->controls, app_control ) == INVALID_INDEX )
			{
				app_control->app = app;
				AddLink( &app->controls, app_control );
			}
		}
		if( !app )
		{
			app = New( struct application_tracker );
			app->controls = NULL;
			app->title = name;
			app->classname = classname;
			app->hWnd = FindWindow( app->classname, app->title );
			//lprintf(" Find of %s %s = %p", app->title, app->classname, app->hWnd );
			app->flags.bWantShow = 0;
			app->flags.bShown = 0;
			app->flags.bFound = 0;
			if( send_to && send_from )
			{
				app->socket = ConnectTo( send_to, send_from );
			}
			AddLink( &l.apps, app );
			app_control->app = app;
			AddLink( &app->controls, app_control );
		}
		if( app->hWnd && IsWindow( app->hWnd ) )
			app->flags.visible = IsWindowVisible( app->hWnd );
		else if( !app->hWnd )
		{
			app->flags.visible = 0;
		}
		return app;
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
		struct application_tracker *app;
		//xlprintf(2100)( "awake..." );
		need_change = FALSE;
		need_find = FALSE;
		//LIST_FORALL( l.apps, idx, struct application_tracker *, app )
		{
			INDEX idx2;
			INDEX idx;
			struct application_tracker *app;
			LIST_FORALL( l.apps, idx, struct application_tracker *, app )
			//LIST_FORALL( l.controls, idx2, PSI_CONTROL, pc )
			{
				if( app->classname || app->title )
				{
					HWND hWnd;
					hWnd = FindWindow( app->classname, app->title );
					//lprintf(" Find of %s %s = %p %p", app->title, app->classname, app->hWnd, hWnd );
					if( hWnd != app->hWnd )
					{
						app->hWnd = hWnd;
						app->flags.bFound = 0;
						app->flags.bShown = 0;
					}
					else if( !app->hWnd || !IsWindow( app->hWnd ) )
					{
						app->flags.bFound = 0;
						app->flags.bShown = 0;
					}
				}
				// same control will be same app....
				// FindAppWindow also refreshes hWnd

				//xlprintf(2100)( "control %p ", app  );
				if( app && app->flags.bWantShow )
				{
					//lprintf( "Want to be shown..." );
					if( !app->flags.bFound || (!app->flags.bShown || !app->flags.visible) )
					{
						if( app->classname || app->title )
						{
							//lprintf( "hWnd = %p", app->hWnd );
							if( app->hWnd )
							{
								if( IsWindow( app->hWnd ) )
								{
									ShowWindow( app->hWnd, SW_RESTORE );
									SetWindowPos( app->hWnd, HWND_TOPMOST, app->last_x, app->last_y, app->last_w, app->last_h, 0 );
									app->flags.bShown = 1;
									app->flags.bFound = 1;
								}
								else
									app->flags.bFound = 0;

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
						if( app->classname || app->title )
						{
							//xlprintf(2100)( WIDE("... %p"), app->hWnd  );
							if( app->hWnd )
							{
								ShowWindow( app->hWnd, SW_HIDE );
								app->flags.bShown = 0;
								app->flags.bFound = 1;

							}
							else
							{
								//xlprintf(2100)( WIDE("no window...") );
								/* haven't found the window, and if it needs to be shown, don't wait*/
								/* if it's hidden, and doesn't exist, that's ok to wait on */
								app->flags.bFound = 0;
								if( app->flags.bWantShow )
									need_change = 1;
							}
						}
						else
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
		else
			WakeableSleep( 100 );
	} while( 1 );
	l.waiting = NULL;
	return 0;
}

static PTRSZVAL OnCreateControl( WIDE("Application Mount") )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, WIDE("Application Mount"), x, y, w, h, -1 );
	{
		MyValidatedControlData( PMY_CONTROL, control, pc );
		control->app_class_name = NULL;
		control->app_window_name = NULL;
		AddLink( &l.controls, pc );
	}
	if( !l.waiting )
		l.waiting = ThreadTo( WaitForApplication, 0 );
	return (PTRSZVAL)pc;
}

static void OnFinishInit( WIDE("Application Mount") )( PSI_CONTROL pc_canvas  )
{
	if( !l.waiting )
		l.waiting = ThreadTo( WaitForApplication, 0 );
}

static PSI_CONTROL OnGetControl( WIDE("Application Mount") )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}

static void OnSaveControl( WIDE("Application Mount") )( FILE* file, PTRSZVAL psv )
{
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	sack_fprintf( file, WIDE("Application Title=%s\n"), control->app_window_name?control->app_window_name:WIDE("") );
	sack_fprintf( file, WIDE("Application Class=%s\n"), control->app_class_name?control->app_class_name:WIDE("") );
	sack_fprintf( file, WIDE("Application Server Address=%s\n"), control->send_to?control->send_to:WIDE("") );
	sack_fprintf( file, WIDE("Application Send From Address=%s\n"), control->send_from?control->send_from:WIDE("") );
}

static PTRSZVAL CPROC SetApplicationName( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	MyValidatedControlData( PMY_CONTROL, control, (PSI_CONTROL)psv );
	//lprintf( "Setting appname of %p to %s", control, name );
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

static void OnLoadControl( WIDE("Application Mount") )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	AddConfigurationMethod( pch, WIDE("Application Title=%m"), SetApplicationName );
	AddConfigurationMethod( pch, WIDE("Application Class=%m"), SetApplicationClass );
	AddConfigurationMethod( pch, WIDE("Application Server Address=%m\n"), SetApplicationSendTo );
	AddConfigurationMethod( pch, WIDE("Application Send From Address=%m\n"), SetApplicationSendFrom );
}

static void OnHideCommon( WIDE("Application Mount") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		//lprintf( "Showing %p", control );
		//lprintf( WIDE("begin hide...") );
		//lprintf( "Do hide on control %p", control );
		control->app = FindAppWindow( control );
		control->app->flags.bWantShow = 0;
		if( !l.waiting )
			l.waiting = ThreadTo( WaitForApplication, 0 );
		else
			WakeThread( l.waiting );
		//lprintf( WIDE("began hide...") );
	}
}

static void OnRevealCommon( WIDE("Application Mount") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		S_32 x = 0;
		S_32 y = 0;
		Image image = GetControlSurface( pc );
		//lprintf( "Showing %p", control );
		//lprintf( WIDE("begin show(move)...") );
		GetPhysicalCoordinate( pc, &x, &y, TRUE );
		//lprintf( "Do Reveal on control %p", control );
		control->app = FindAppWindow( control );
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

static void OnCloneControl( WIDE("Application Mount") )( PTRSZVAL copy,PTRSZVAL original)
{
	PSI_CONTROL pc_copy = (PSI_CONTROL)copy;
	PSI_CONTROL pc_original = (PSI_CONTROL)original;
	ValidatedControlData( PMY_CONTROL, MyControlID, p_copy, pc_copy);
	ValidatedControlData( PMY_CONTROL, MyControlID, p_original, pc_original);
	if( p_copy && p_original )
	{
		p_copy->app_class_name = StrDup( p_original->app_class_name );
		p_copy->app_window_name = StrDup( p_original->app_window_name );
		p_copy->send_from = StrDup( p_original->send_from );
		p_copy->send_to = StrDup( p_original->send_to );
	}		
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
