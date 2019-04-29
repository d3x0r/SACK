#include <stdhdrs.h>
#define USES_INTERSHELL_INTERFACE
#define USE_RENDER_INTERFACE l.pdi
#define USE_IMAGE_INTERFACE l.pii
#include <render.h>

#include <ffmpeg_interface.h>
#include "../widgets/include/banner.h"
#include "../intershell_registry.h"


#define VIDEO_PLAYER_MAIN_SOURCE
#include "video_player_local.h"


struct player_button
{
	PMENU_BUTTON button;
	CTEXTSTR url_name;
};

/*
static struct ffmpeg_button_local {
	struct ffmpeg_file *ffmpeg_serve;
	PLIST players;
} l;
*/

typedef struct ffmpeg_surface {
	PSI_CONTROL pc;
	struct ffmpeg_file *ffmpeg;
	LOGICAL playing; // is showing something.
} *PFFMPEG;

EasyRegisterControlWithBorder( "FFMPEG Surface", sizeof( struct ffmpeg_surface ), BORDER_NONE );

void PlayOnPlayers( CTEXTSTR url_name )
{
	PFFMPEG ffmpeg;
	INDEX idx;
	if( !url_name )
		return;
	//LIST_FORALL( l.players, idx, PFFMPEG, ffmpeg )
	{
		if( ffmpeg->playing )
			//StopItemIn( ffmpeg->pc );
		lprintf( "PC is %p", ffmpeg->pc );
		//PlayItemInEx( ffmpeg->pc, url_name, NULL  );
		ffmpeg->playing = TRUE;
	}
}

static void OnKeyPressEvent( "FFMPEG/button" )( uintptr_t psv )
{
	static int n;
	PMENU_BUTTON button = (PMENU_BUTTON)psv;
	PSI_CONTROL pc_button = InterShell_GetButtonControl( button );
	PRENDERER renderer = GetButtonAnimationLayer( pc_button );
	n++;
	if( n == 5 )
		n = 0;
}

static uintptr_t OnCreateMenuButton( "FFMPEG/button" )( PMENU_BUTTON button )
{

	return (uintptr_t)button;
}


static void OnKeyPressEvent( "FFMPEG/Player button" )( uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	PlayOnPlayers( play->url_name );
}

static uintptr_t OnCreateMenuButton( "FFMPEG/Player button" )( PMENU_BUTTON button )
{
	struct player_button *play = New( struct player_button );
	play->button = button;
	play->url_name = NULL;
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Play_Video" );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "FFMPEG/Stop Player button" )( uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	PFFMPEG ffmpeg;
	INDEX idx;
	//LIST_FORALL( l.players, idx, PFFMPEG, ffmpeg )
	{
		if( ffmpeg->playing )
			StopItemIn( ffmpeg->pc );
		ffmpeg->playing = FALSE;
	}


}

static uintptr_t OnCreateMenuButton( "FFMPEG/Stop Player button" )( PMENU_BUTTON button )
{
	struct player_button *play = New( struct player_button );
	play->button = button;
	play->url_name = NULL;
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Stop_Player" );
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

static void OnLoadControl( "FFMPEG/Player button" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Play Media:%m", SetButtonMedia );
}

static void OnSaveControl( "FFMPEG/Player button" )( FILE *file, uintptr_t psv )
{
	struct player_button *play = (struct player_button *)psv;
	if( play->url_name )
		sack_fprintf( file, "Play Media:%s\n", play->url_name );
}



static uintptr_t OnCreateControl( "FFMPEG/Player" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{

	PSI_CONTROL pc = MakeNamedControl( parent, "FFMPEG Surface", x, y, w, h, -1 );
	MyValidatedControlData( PFFMPEG, ffmpeg, pc );
	ffmpeg->pc = pc;
	AddLink( &l.players, ffmpeg );
	return (uintptr_t)ffmpeg;
}

static PSI_CONTROL OnGetControl( "FFMPEG/Player")(uintptr_t psv )
{
	PFFMPEG ffmpeg = (PFFMPEG)psv;
	return ffmpeg->pc;
}

static void OnShowControl( "FFMPEG/Player" )(uintptr_t psv )
{
	PFFMPEG ffmpeg = (PFFMPEG)psv;
	//ffmpeg->ffmpeg = PlayItemInEx( ffmpeg->pc, "dshow://", NULL );

}

static void OnHideControl( "FFMPEG/Player" )(uintptr_t psv )
{
	PFFMPEG ffmpeg = (PFFMPEG)psv;
	//StopItem(  ffmpeg->ffmpeg );

}

//------------------------------------------------------------------------------------
