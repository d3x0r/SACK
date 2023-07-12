
//#define DEFINE_DEFAULT_RENDER_INTERFACE
//#define USE_IMAGE_INTERFACE GetImageInterface()
//#define DEBUG_MESSAGES

#include <stdhdrs.h>
#include <filesys.h>
#include <psi.h>
#include <network.h>
#include <timers.h>
#include <sqlgetoption.h>
#include "../InterShell.stable/vlc_hook/vlcint.h"
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define SetButtonText InterShell_SetButtonText
#include "../InterShell.stable/intershell_registry.h"
#include "../InterShell.stable/intershell_export.h"
#include "../InterShell.stable/widgets/keypad/keypad/keypad.h"

#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif



static struct {
	PCLIENT pc;
	PTASK_INFO task;
	SOCKADDR *sendto;
	TEXTSTR channel;
	PVARIABLE channel_var;
	TEXTSTR volume;
	PVARIABLE volume_var;
	struct flags
	{
		BIT_FIELD shutdown : 1;
		BIT_FIELD tv_playing : 1;
		BIT_FIELD tv_last_play : 1;
		BIT_FIELD bReceived : 1; // if any packet was received - this allows sending
	}flags;
	uint32_t last_attempt;
	uint32_t last_receive;
	uint32_t last_x, last_y, last_width, last_height;
	LOGICAL kill_complete;
	PTHREAD launch_thread;
	PLIST on_off_buttons;
	TEXTCHAR video_application[256];
	TEXTCHAR video_application_args[1024];
	PLIST video_players;
} l;

//--------------------------------------------------------------------

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	PRENDERER surface;
	struct {
		BIT_FIELD bShown : 1;
	} flags;
};

ATEXIT( CloseVlcWindows )
{
#if defined( DEBUG_MESSAGES )
	lprintf( "Terminating task..." );
#endif
	l.flags.shutdown = 1;
	TerminateProgram( l.task );
}

static void OnBeginShutdown( "VLC Stream Host" )( void )
{
	l.flags.shutdown = 1;
	TerminateProgram( l.task );
}

PRELOAD( InitVlcWindows )
{
	l.flags.tv_last_play = 1;
	SACK_GetProfileString( GetProgramName(), "Video Player/application path", "@/vlc_window.exe", l.video_application, sizeof( l.video_application ) );
	SACK_GetProfileString( GetProgramName(), "Video Player/application args", "dshow://", l.video_application_args, sizeof( l.video_application_args ) );
}

EasyRegisterControl( "Video Control", sizeof( MY_CONTROL) );

#if defined( DEBUG_MESSAGES )
#define SendUDPEx(a,b,c,d) if( l.flags.bReceived ){ LogBinary( b, c ); SendUDPEx( a,b,c,d ); }
#else
#define SendUDPEx(a,b,c,d) if( l.flags.bReceived ){ SendUDPEx( a,b,c,d ); }
#endif

#define QueryAllowTV(name) \
	  DefineRegistryMethod("SACK",AllowTV,"Stream Control","Allow Turn On", name "_allow_turn_on",LOGICAL,(void),__LINE__)

#define EventAllowTV(name) \
	  DefineRegistryMethod("SACK",AllowTV,"Stream Control","Allow Turn On Changed", name "_allow_turn_on_changed",void,(LOGICAL),__LINE__)

static void AllowOn( LOGICAL yes_no )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( "SACK/Stream Control/Allow Turn On Changed", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(LOGICAL);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (LOGICAL) );
		if( f )
			f(yes_no);
	}
}

static LOGICAL RequestAllowOn( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( "SACK/Stream Control/Allow Turn On", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		LOGICAL (CPROC *f)(void);
		//lprintf( "Send tv query to %s", name );
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, LOGICAL, name, (void) );
		if( f )
			if( !f() )
			{
				//lprintf( "Something disallowed TV" );
				return FALSE;
			}
	}
	//lprintf( "nothing disallowed TV" );
	return TRUE;
}

void SendUpdate( uint32_t x, uint32_t y, uint32_t w, uint32_t h )
{
	uint32_t buffer[7];
	if( !l.pc )
	{
		lprintf( "Socket isn't setup yet." );
		return;
	}
	buffer[0] = GetTickCount();
	buffer[1] = 0;
	l.last_x = buffer[2] = x;
	l.last_y = buffer[3] = y;
	l.last_width = buffer[4] = w;
	l.last_height = buffer[5] = h;
#if defined( DEBUG_MESSAGES )
	LogBinary( buffer, sizeof( buffer ) );
	DumpAddr( "vlc_sendto", l.sendto );
#endif
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendChannelUp( void )
{
	uint32_t buffer[2];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 2;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendChannelDown( void )
{
	uint32_t buffer[2];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 1;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendChannel( int channel )
{
	uint32_t buffer[3];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 3;
	buffer[2] = channel;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendVolumeUp( void )
{
	uint32_t buffer[2];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 5;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

static void UpdateOnOffButtons( void )
{
	INDEX idx;
	PMENU_BUTTON button;
	LIST_FORALL( l.on_off_buttons, idx, PMENU_BUTTON, button )
	{
		UpdateButton( button );
	}
}

void SendTurnOff( void )
{
	uint32_t buffer[2];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 7;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	l.flags.tv_last_play = 0;
	l.flags.tv_playing = 0;
	UpdateOnOffButtons();
}

void SendTurnOn( void )
{
	uint32_t buffer[7];
	if( !l.pc )
		return;
	if( RequestAllowOn() )
	{
 		buffer[0] = GetTickCount();
		buffer[1] = 8;
		SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
		WakeableSleep( 10 );
		SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
		WakeableSleep( 10 );
		SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
		l.flags.tv_last_play = 1;
		l.flags.tv_playing = 1;
		UpdateOnOffButtons();
	}
}

void SendVolumeDown( void )
{
	uint32_t buffer[7];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 4;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendVolume( int volume )
{
	uint32_t buffer[7];
	if( !l.pc )
		return;
	buffer[0] = GetTickCount();
	buffer[1] = 6;
	buffer[2] = volume;
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
	WakeableSleep( 10 );
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

static int OnDrawCommon( "Video Control" )( PSI_CONTROL pc )
{
	return 1;
}

void CPROC KilTaskOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	lprintf( "%s", buffer );
	// pskill output; don't really care
}

void CPROC KillTaskEnded( uintptr_t psv, PTASK_INFO task )
{
	// don't respawn when exiting.
	l.kill_complete = TRUE;
	WakeThread( l.launch_thread );
}

void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task )
{
	if( task )
		l.task = NULL;

	// don't respawn when exiting.
	if( l.flags.shutdown )
		return;

	if( IsSystemShuttingDown() )
	{
		lprintf( "Do not re-start tasks, system is shutting down." );
		return;
	}

	if( !l.task )
	{
		PTASK_INFO tmp_task;
		CTEXTSTR *args;
		CTEXTSTR executable_name;
		args = NewArray( CTEXTSTR, 3 );
		args[0] = "pskill.exe";
		executable_name = pathrchr( l.video_application );
		if( executable_name )
			executable_name++;
		else
			executable_name = l.video_application;
		args[1] = executable_name;
		args[2] = NULL;
		l.kill_complete = FALSE;
		tmp_task = LaunchPeerProgramExx( "pskill.exe", NULL, args
												 , 0  // flags
												 , KilTaskOutput, KillTaskEnded, 0 DBG_SRC );
		if( tmp_task )
		{
			l.launch_thread = MakeThread();
			while( !l.kill_complete )
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

static void CPROC ReadTvStatus( PCLIENT pc, POINTER buffer, size_t len, SOCKADDR *sa )
{
	if( !buffer )
	{
		buffer = Allocate( 1000 );
	}
	else
	{
		uint32_t tick = ((uint32_t*)buffer)[0];
		if( tick != l.last_receive )
		{
#if defined( DEBUG_MESSAGES )
			lprintf( "Receive..." );
			LogBinary( buffer, len );
#endif
			l.flags.bReceived = 1;
			l.last_receive = tick;

			switch( ((uint32_t*)buffer)[1] )
			{
			case 1:
				// this falls through... so we get to test for allow tv ON.

				{
					INDEX idx;
					PSI_CONTROL pc;
					LIST_FORALL( l.video_players, idx, PSI_CONTROL, pc )
					{
						MyValidatedControlData( PMY_CONTROL, control, pc );
						if( control )
						{
							if( control->flags.bShown )
							{
								int32_t x = 0;
								int32_t y = 0;
								Image image = GetControlSurface( pc );
								control->flags.bShown = TRUE;
								GetPhysicalCoordinate( pc, &x, &y, TRUE, FALSE );
#if defined( DEBUG_MESSAGES )
								lprintf( "Posting VLC Update." );
#endif
								SendUpdate( x, y, image->width, image->height );
								break;
							}
						}
					}
					if( !pc )
					{
						// should be hidden.
						SendUpdate( 0, 0, 0, 0 );
					}
				}
			case 0:
				{
					int tmp;
					tmp = ((uint32_t*)buffer)[2];
					snprintf( l.channel, sizeof( TEXTCHAR ) * 5, "%d", tmp );
					LabelVariableChanged( l.channel_var );
					tmp = ((uint32_t*)buffer)[3];
					snprintf( l.volume, sizeof( TEXTCHAR ) * 5, "%d", tmp );
					LabelVariableChanged( l.volume_var );
					tmp = ((uint8_t*)buffer)[16];
					l.flags.tv_playing = (tmp!=0);
					if( !RequestAllowOn() )
						l.flags.tv_last_play = 0;
					// channel and volume result
					if( l.flags.tv_last_play != l.flags.tv_playing )
					{
						if( l.flags.tv_last_play )
							SendTurnOn();
						else
							SendTurnOff();
					}
				}
				break;
			}
		}
	}
	ReadUDP( pc, buffer, 1000 );
}

static int OnCreateCommon( "Video Control" )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	control->flags.bShown = FALSE;
	AddLink( &l.video_players, pc );
	if( !l.pc )
	{
		CreateKeypadType( "Video/Channel" );
		l.pc = ConnectUDP( "127.0.0.1", 2998, "127.0.0.1", 2999, ReadTvStatus, NULL );
		l.sendto = CreateSockAddress( "127.0.0.1", 2999 );
		l.last_receive = 1; // first receive is from 0.
		l.channel = NewArray( TEXTCHAR, 8 );
		StrCpy( l.channel, "No TV" );
		l.volume = NewArray( TEXTCHAR, 8 );
		StrCpy( l.volume, "No TV" );
		l.channel_var = CreateLabelVariable( "<video/channel>", LABEL_TYPE_STRING, &l.channel );
		l.volume_var = CreateLabelVariable( "<video/volume>", LABEL_TYPE_STRING, &l.volume );
		// just call it once when we startup the socket... (bad form)
		TaskEnded( 0, NULL );
	}
	return TRUE;
}

PUBLIC( void, link_vlc_stream )( void )
{
}

static int OnChangePage("Video Control" )( PSI_CONTROL pc_canvas )
{
#if defined( DEBUG_MESSAGES )
	lprintf( "Change page - default control to off?" );
#endif
	SendUpdate( 0, 0, 0, 0 );
	return TRUE;
}

static void OnHideCommon( "Video Control" )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
#if defined( DEBUG_MESSAGES )
		lprintf( "Posting VLC Update." );
		DumpAddr( "vlc_sendto", l.sendto );
#endif
		control->flags.bShown = FALSE;
		SendUpdate( 0, 0, 0, 0 );
	}
}

static void OnRevealCommon( "Video Control" )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		int32_t x = 0;
		int32_t y = 0;
		Image image = GetControlSurface( pc );
		control->flags.bShown = TRUE;
		GetPhysicalCoordinate( pc, &x, &y, TRUE, FALSE );
#if defined( DEBUG_MESSAGES )
		lprintf( "Posting VLC Update." );
#endif
		SendUpdate( x, y, image->width, image->height );
	}
}


static uintptr_t OnCreateMenuButton( "Video/Channel Up" )( PMENU_BUTTON button )
{
	SetButtonText( button, "Channel_Up" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Channel Up" )( uintptr_t psv )
{
	SendChannelUp();
}

static uintptr_t OnCreateMenuButton( "Video/Channel Down" )( PMENU_BUTTON button )
{
	SetButtonText( button, "Channel_Down" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Channel Down" )( uintptr_t psv )
{
	SendChannelDown();
}


static uintptr_t OnCreateMenuButton( "Video/Channel Set" )( PMENU_BUTTON button )
{
	SetButtonText( button, "3" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Channel Set" )( uintptr_t psv )
{
	SendChannel( 3 );
}


static uintptr_t OnCreateMenuButton( "Video/Volume Up" )( PMENU_BUTTON button )
{
	SetButtonText( button, "Volume_Up" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Volume Up" )( uintptr_t psv )
{
	SendVolumeUp();
}

static uintptr_t OnCreateMenuButton( "Video/Volume Down" )( PMENU_BUTTON button )
{
	SetButtonText( button, "Volume_Down" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Volume Down" )( uintptr_t psv )
{
	SendVolumeDown();
}


static uintptr_t OnCreateMenuButton( "Video/Volume Set" )( PMENU_BUTTON button )
{
	SetButtonText( button, "1000" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Volume Set" )( uintptr_t psv )
{
	SendVolume( 1000 );
}

static uintptr_t OnCreateMenuButton( "Video/Turn On" )( PMENU_BUTTON button )
{
	SetButtonText( button, "On" );
	AddLink( &l.on_off_buttons, button );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Turn On" )( uintptr_t psv )
{
	SendTurnOn();
}

static void OnShowControl( "Video/Turn On" )( uintptr_t psv )
{
#ifdef MILK_PLUGIN
	MILK_SetButtonHighlight( (PMENU_BUTTON)psv, l.flags.tv_playing );
#else
	InterShell_SetButtonHighlight( (PMENU_BUTTON)psv, l.flags.tv_playing );
#endif
}

static uintptr_t OnCreateMenuButton( "Video/Turn Off" )( PMENU_BUTTON button )
{
	SetButtonText( button, "Off" );
	AddLink( &l.on_off_buttons, button );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "Video/Turn Off" )( uintptr_t psv )
{
	SendTurnOff();
}

static void OnShowControl( "Video/Turn Off" )( uintptr_t psv )
{
#ifdef MILK_PLUGIN
	MILK_SetButtonHighlight( (PMENU_BUTTON)psv, !l.flags.tv_playing );
#else
	InterShell_SetButtonHighlight( (PMENU_BUTTON)psv, !l.flags.tv_playing );
#endif
}

static void OnKeypadEnterType( "video", "Video/Channel" )( PSI_CONTROL pc )
{
	uint64_t value = GetKeyedValue( pc );
	ClearKeyedEntryOnNextKey( pc );
	SendChannel( (int)value );
}

static void EventAllowTV( "Stream Plugin 2" )( LOGICAL allow_on )
{
	if( !allow_on )
	{
		if( l.flags.tv_playing )
			SendTurnOff();
	}
}



