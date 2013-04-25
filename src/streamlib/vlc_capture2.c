
//#define DEFINE_DEFAULT_RENDER_INTERFACE
//#define USE_IMAGE_INTERFACE GetImageInterface()
//#define DEBUG_MESSAGES

#include <stdhdrs.h>
#include <psi.h>
#include <system.h>
#include <network.h>
#include <timers.h>
#include <sqlgetoption.h>
#include "../InterShell/vlc_hook/vlcint.h"
#ifdef MILK_PLUGIN
#define USES_MILK_INTERFACE
#define DEFINES_MILK_INTERFACE
#define SetButtonText MILK_SetButtonText
#include "../src/apps/milk/milk_registry.h"
#include "../src/apps/milk/milk_export.h"
#include "../src/fut/widgets/keypad/keypad_plugin/keypad.h"
#else
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define SetButtonText InterShell_SetButtonText
#include "../InterShell/intershell_registry.h"
#include "../InterShell/intershell_export.h"
#include "../InterShell/widgets/keypad/keypad/keypad.h"
#endif

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
	_32 last_attempt;
	_32 last_receive;
	_32 last_x, last_y, last_width, last_height;
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

PRELOAD( InitVlcWindows )
{
	l.flags.tv_last_play = 1;
   SACK_GetProfileString( GetProgramName(), WIDE("Video Player/application path"), WIDE("@/vlc_window.exe"), l.video_application, sizeof( l.video_application ) );
	SACK_GetProfileString( GetProgramName(), WIDE("Video Player/application args"), WIDE("dshow://"), l.video_application_args, sizeof( l.video_application_args ) );
}

EasyRegisterControl( WIDE("Video Control"), sizeof( MY_CONTROL) );

#if defined( DEBUG_MESSAGES )
#define SendUDPEx(a,b,c,d) if( l.flags.bReceived ){ LogBinary( b, c ); SendUDPEx( a,b,c,d ); }
#else
#define SendUDPEx(a,b,c,d) if( l.flags.bReceived ){ SendUDPEx( a,b,c,d ); }
#endif

#define QueryAllowTV(name) \
	  __DefineRegistryMethod(WIDE("SACK"),AllowTV,WIDE( "Stream Control" ),WIDE( "Allow Turn On" ), name WIDE( "_allow_turn_on" ),LOGICAL,(void),__LINE__)

#define EventAllowTV(name) \
	  __DefineRegistryMethod(WIDE("SACK"),AllowTV,WIDE( "Stream Control" ),WIDE( "Allow Turn On Changed" ), name WIDE( "_allow_turn_on_changed" ),void,(LOGICAL),__LINE__)

static void AllowOn( LOGICAL yes_no )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( WIDE("SACK/Stream Control/Allow Turn On Changed"), &data );
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
	for( name = GetFirstRegisteredName( WIDE("SACK/Stream Control/Allow Turn On"), &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		LOGICAL (CPROC *f)(void);
		//lprintf( WIDE("Send tv query to %s"), name );
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX WIDE("/common/save common/%s"), name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, LOGICAL, name, (void) );
		if( f )
			if( !f() )
			{
				//lprintf( WIDE("Something disallowed TV") );
				return FALSE;
			}
	}
	//lprintf( WIDE("nothing disallowed TV") );
	return TRUE;
}

void SendUpdate( _32 x, _32 y, _32 w, _32 h )
{
	_32 buffer[7];
	if( !l.pc )
	{
      lprintf( WIDE("Socket isn't setup yet.") );
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
	DumpAddr( WIDE("vlc_sendto"), l.sendto );
#endif
	SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendChannelUp( void )
{
	_32 buffer[2];
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
	_32 buffer[2];
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
	_32 buffer[3];
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
	_32 buffer[2];
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
	_32 buffer[2];
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
	_32 buffer[7];
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
	_32 buffer[7];
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
	_32 buffer[7];
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

static int OnDrawCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
   return 1;
}

void CPROC KilTaskOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
   lprintf( "%s", buffer );
   // pskill output; don't really care
}

void CPROC KillTaskEnded( PTRSZVAL psv, PTASK_INFO task )
{
	// don't respawn when exiting.
	l.kill_complete = TRUE;
	WakeThread( l.launch_thread );
}

void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task )
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
		args[0] = WIDE("pskill.exe");
		executable_name = pathrchr( l.video_application );
		if( executable_name )
			executable_name++;
		else
         executable_name = l.video_application;
		args[1] = executable_name;
		args[2] = NULL;
		l.kill_complete = FALSE;
		tmp_task = LaunchPeerProgramExx( WIDE("pskill.exe"), NULL, args
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
		_32 tick = ((_32*)buffer)[0];
		if( tick != l.last_receive )
		{
#if defined( DEBUG_MESSAGES )
         lprintf( WIDE("Receive...") );
			LogBinary( buffer, len );
#endif
         l.flags.bReceived = 1;
			l.last_receive = tick;

			switch( ((_32*)buffer)[1] )
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
								S_32 x = 0;
								S_32 y = 0;
								Image image = GetControlSurface( pc );
								control->flags.bShown = TRUE;
								GetPhysicalCoordinate( pc, &x, &y, TRUE );
#if defined( DEBUG_MESSAGES )
								lprintf( WIDE("Posting VLC Update.") );
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
					tmp = ((_32*)buffer)[2];
					snprintf( l.channel, sizeof( TEXTCHAR ) * 5, WIDE("%d"), tmp );
					LabelVariableChanged( l.channel_var );
					tmp = ((_32*)buffer)[3];
					snprintf( l.volume, sizeof( TEXTCHAR ) * 5, WIDE("%d"), tmp );
					LabelVariableChanged( l.volume_var );
					tmp = ((_8*)buffer)[16];
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

static int OnCreateCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	control->flags.bShown = FALSE;
	AddLink( &l.video_players, pc );
	if( !l.pc )
	{
		CreateKeypadType( WIDE("Video/Channel") );
		l.pc = ConnectUDP( WIDE("127.0.0.1"), 2998, WIDE("127.0.0.1"), 2999, ReadTvStatus, NULL );
		l.sendto = CreateSockAddress( WIDE("127.0.0.1"), 2999 );
		l.last_receive = 1; // first receive is from 0.
		l.channel = NewArray( TEXTCHAR, 8 );
		StrCpy( l.channel, WIDE("No TV") );
		l.volume = NewArray( TEXTCHAR, 8 );
		StrCpy( l.volume, WIDE("No TV") );
		l.channel_var = CreateLabelVariable( WIDE("<video/channel>"), LABEL_TYPE_STRING, &l.channel );
		l.volume_var = CreateLabelVariable( WIDE("<video/volume>"), LABEL_TYPE_STRING, &l.volume );
		// just call it once when we startup the socket... (bad form)
		TaskEnded( 0, NULL );
	}
	return TRUE;
}

PUBLIC( void, link_vlc_stream )( void )
{
}

OnChangePage(WIDE("Video Control") )( void )
{
#if defined( DEBUG_MESSAGES )
	lprintf( WIDE("Change page - default control to off?") );
#endif
	SendUpdate( 0, 0, 0, 0 );
   return TRUE;
}

static void OnHideCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
#if defined( DEBUG_MESSAGES )
		lprintf( WIDE("Posting VLC Update.") );
      DumpAddr( WIDE("vlc_sendto"), l.sendto );
#endif
		control->flags.bShown = FALSE;
		SendUpdate( 0, 0, 0, 0 );
	}
}

static void OnRevealCommon( WIDE("Video Control") )( PSI_CONTROL pc )
{
	MyValidatedControlData( PMY_CONTROL, control, pc );
	if( control )
	{
		S_32 x = 0;
		S_32 y = 0;
		Image image = GetControlSurface( pc );
      control->flags.bShown = TRUE;
		GetPhysicalCoordinate( pc, &x, &y, TRUE );
#if defined( DEBUG_MESSAGES )
		lprintf( WIDE("Posting VLC Update.") );
#endif
		SendUpdate( x, y, image->width, image->height );
	}
}


OnCreateMenuButton( WIDE("Video/Channel Up") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("Channel_Up") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Channel Up") )( PTRSZVAL psv )
{
	SendChannelUp();
}

OnCreateMenuButton( WIDE("Video/Channel Down") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("Channel_Down") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Channel Down") )( PTRSZVAL psv )
{
	SendChannelDown();
}


OnCreateMenuButton( WIDE("Video/Channel Set") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("3") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Channel Set") )( PTRSZVAL psv )
{
	SendChannel( 3 );
}


OnCreateMenuButton( WIDE("Video/Volume Up") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("Volume_Up") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Volume Up") )( PTRSZVAL psv )
{
	SendVolumeUp();
}

OnCreateMenuButton( WIDE("Video/Volume Down") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("Volume_Down") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Volume Down") )( PTRSZVAL psv )
{
	SendVolumeDown();
}


OnCreateMenuButton( WIDE("Video/Volume Set") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("1000") );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Volume Set") )( PTRSZVAL psv )
{
	SendVolume( 1000 );
}

OnCreateMenuButton( WIDE("Video/Turn On") )( PMENU_BUTTON button )
{
	SetButtonText( button, WIDE("On") );
   AddLink( &l.on_off_buttons, button );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Turn On") )( PTRSZVAL psv )
{
	SendTurnOn();
}

OnShowControl( WIDE("Video/Turn On") )( PTRSZVAL psv )
{
#ifdef MILK_PLUGIN
	MILK_SetButtonHighlight( (PMENU_BUTTON)psv, l.flags.tv_playing );
#else
	InterShell_SetButtonHighlight( (PMENU_BUTTON)psv, l.flags.tv_playing );
#endif
}

OnCreateMenuButton( WIDE("Video/Turn Off") )( PMENU_BUTTON button )
{
   SetButtonText( button, WIDE("Off") );
	AddLink( &l.on_off_buttons, button );
   return (PTRSZVAL)button;
}

OnKeyPressEvent( WIDE("Video/Turn Off") )( PTRSZVAL psv )
{
	SendTurnOff();
}

OnShowControl( WIDE("Video/Turn Off") )( PTRSZVAL psv )
{
#ifdef MILK_PLUGIN
	MILK_SetButtonHighlight( (PMENU_BUTTON)psv, !l.flags.tv_playing );
#else
	InterShell_SetButtonHighlight( (PMENU_BUTTON)psv, !l.flags.tv_playing );
#endif
}

OnKeypadEnterType( WIDE("video"), WIDE("Video/Channel") )( PSI_CONTROL pc )
{
	_64 value = GetKeyedValue( pc );
   ClearKeyedEntryOnNextKey( pc );
   SendChannel( (int)value );
}

static void EventAllowTV( WIDE("Stream Plugin 2") )( LOGICAL allow_on )
{
	if( !allow_on )
	{
		if( l.flags.tv_playing )
         SendTurnOff();
	}
}



