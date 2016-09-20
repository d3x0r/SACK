#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <stdhdrs.h>
#include <image.h>
//#include "/binglink/link_events.h"
#include "../../intershell_registry.h"
#include "../../intershell_export.h"
#include "../vlcint.h"


struct player_button
{
	PMENU_BUTTON button;
	CTEXTSTR url_name;
};
static struct vlc_button_local {
	struct my_vlc_interface *vlc_serve;
	PLIST players;
} l;

typedef struct vlc_surface {
	PSI_CONTROL pc;
	struct my_vlc_interface *vlc;
	LOGICAL playing; // is showing something.
} *PVLC;

EasyRegisterControlWithBorder( WIDE("VLC Surface"), sizeof( struct vlc_surface ), BORDER_NONE );

void PlayOnPlayers( CTEXTSTR url_name )
{
	PVLC vlc;
	INDEX idx;
	if( !url_name )
		return;
	LIST_FORALL( l.players, idx, PVLC, vlc )
	{
		if( vlc->playing )
			StopItemIn( vlc->pc );
		lprintf( WIDE("PC is %p"), vlc->pc );
		PlayItemInEx( vlc->pc, url_name, NULL  );
		vlc->playing = TRUE;
	}
}

static void OnKeyPressEvent( WIDE("VLC/button") )( uintptr_t psv )
{
	static int n;
	PMENU_BUTTON button = (PMENU_BUTTON)psv;
	PSI_CONTROL pc_button = InterShell_GetButtonControl( button );
	PRENDERER renderer = GetButtonAnimationLayer( pc_button );
	n++;
	if( n == 5 )
		n = 0;
}

static uintptr_t OnCreateMenuButton( WIDE("VLC/button") )( PMENU_BUTTON button )
{

	return (uintptr_t)button;
}


static void OnKeyPressEvent( WIDE("VLC/Player button") )( uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	PlayOnPlayers( play->url_name );
}

static uintptr_t OnCreateMenuButton( WIDE("VLC/Player button") )( PMENU_BUTTON button )
{
	struct player_button *play = New( struct player_button );
	play->button = button;
	play->url_name = NULL;
	InterShell_SetButtonStyle( button, WIDE("bicolor square") );
	InterShell_SetButtonText( button, WIDE("Play_Video") );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( WIDE("VLC/Stop Player button") )( uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	PVLC vlc;
	INDEX idx;
	LIST_FORALL( l.players, idx, PVLC, vlc )
	{
		if( vlc->playing )
			StopItemIn( vlc->pc );
		vlc->playing = FALSE;
	}


}

static uintptr_t OnCreateMenuButton( WIDE("VLC/Stop Player button") )( PMENU_BUTTON button )
{
	struct player_button *play = New( struct player_button );
	play->button = button;
	play->url_name = NULL;
	InterShell_SetButtonStyle( button, WIDE("bicolor square") );
	InterShell_SetButtonText( button, WIDE("Stop_Player") );
	return (uintptr_t)button;
}

static uintptr_t CPROC SetButtonMedia( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, url );
	struct player_button *play = (struct player_button *)psv;
	if( play->url_name )
		Release( (POINTER)play->url_name );
	play->url_name = StrDup( url );
	return psv;
}

static void OnLoadControl( WIDE("VLC/Player button") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, WIDE("Play Media:%m"), SetButtonMedia );
}

static void OnSaveControl( WIDE("VLC/Player button") )( FILE *file, uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	if( play->url_name )
		fprintf( file, WIDE("Play Media:%s\n"), play->url_name );
}



static uintptr_t OnCreateControl( WIDE("VLC/Player") )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{

	PSI_CONTROL pc = MakeNamedControl( parent, WIDE("VLC Surface"), x, y, w, h, -1 );
	MyValidatedControlData( PVLC, vlc, pc );
	vlc->pc = pc;
	AddLink( &l.players, vlc );
	return (uintptr_t)vlc;
}

static PSI_CONTROL OnGetControl( WIDE("VLC/Player"))(uintptr_t psv )
{
	PVLC vlc = (PVLC)psv;
	return vlc->pc;
}

static void OnShowControl( WIDE("VLC/Player") )(uintptr_t psv )
{
	PVLC vlc = (PVLC)psv;
	//vlc->vlc = PlayItemInEx( vlc->pc, WIDE("dshow://"), NULL );

}

static void OnHideControl( WIDE("VLC/Player") )(uintptr_t psv )
{
	PVLC vlc = (PVLC)psv;
	//StopItem(  vlc->vlc );

}

//------------------------------------------------------------------------------------

static uintptr_t OnCreateControl( WIDE("VLC/Video Link") )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	return 1;
}


//------------------------------------------------------------------------------------
/*
static void VideoLinkCommandServeMaster( WIDE("VLC_Video Link") )( void )
{
	// enable reading dshow:// and writing a stream out, Need the service vlc_interface.
	if( !l.vlc_serve )
	{
		Image image = MakeImageFile( 320, 240 );
		l.vlc_serve = PlayItemAgainst( image, WIDE("dshow:// --sout '#transcode{vcodec=mp4v,acodec=mpga,vb=3072,height=480,width=720,ab=192,channels=2}:duplicate{dst=display,dst=standard{access=http,mux=ts,dst=0.0.0.0:1234,height=480,width=720}}' >/tmp/vlc.log 2>&1") );
	}
}
*/
#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
