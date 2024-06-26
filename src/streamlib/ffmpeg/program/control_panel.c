#include "video_player_local.h"

#include <psi/knob.h>

#define MyName "Media player control panel"

struct media_control_panel
{
	struct control_panel_flags {
		BIT_FIELD playing : 1;
		BIT_FIELD setting_position : 1;
	} flags;
	struct my_button *media;
	struct ffmpeg_file *file;

	PSI_CONTROL panel;
	PSI_CONTROL knob;
	Image knob_image;
	PSI_CONTROL stop_button;
	PSI_CONTROL pause_button;
	PSI_CONTROL seek_slider;
	PSI_CONTROL progress;
#ifdef _DEBUG
	PSI_CONTROL debug_mem_button;
#endif
};

EasyRegisterControl( MyName, sizeof( struct media_control_panel ) );


static int OnCreateCommon( MyName )( PSI_CONTROL pc )
{
	MyValidatedControlData( struct media_control_panel*, media, pc );
	if( media )
	{
		media->panel = pc;
		media->media = NULL;
		AddLink( &l.controls, media );
	}
	return 1;
}

static int OnDrawCommon( MyName )( PSI_CONTROL pc )
{
	ClearImageTo( GetControlSurface( pc ), BASE_COLOR_BLACK );
	return 1;
}

static int OnMouseCommon( MyName )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	static int _b;
	static int _x, _y;
	if( b && !_b )
	{
		_x = x;
		_y = y;
	}
	if( b && _b )
	{
		MoveCommonRel( pc, x - _x, y - _y );
	}
	_b = b;
	return 1;
}

void HideMediaPanel( struct my_button *media )
{
	INDEX idx;
	struct media_control_panel *panel;
	LIST_FORALL( l.controls, idx, struct media_control_panel *, panel )
	{
		if( panel->media == media )
			break;
	}
	if( panel )
	{
		HideControl( panel->panel );
	}
}

static void CPROC stop_pushed( uintptr_t psvPanel, PSI_CONTROL pc )
{
	struct media_control_panel *panel = ( struct media_control_panel *)psvPanel;
	ffmpeg_StopFile( panel->media->file );
}

static void CPROC debug_mem( uintptr_t psvPanel, PSI_CONTROL pc )
{
	struct media_control_panel *panel = (struct media_control_panel *)psvPanel;
	DebugDumpMemEx(  TRUE );
}

static void CPROC pause_pushed( uintptr_t psvPanel, PSI_CONTROL pc )
{
	struct media_control_panel *panel = ( struct media_control_panel *)psvPanel;
	if( panel->flags.playing )
	{
		panel->flags.playing = 0;
		ffmpeg_PauseFile( panel->media->file );
		SetControlText( panel->pause_button, "Play" );
	}
	else
	{
		panel->flags.playing = 1;
		ffmpeg_PlayFile( panel->media->file );
		SetControlText( panel->pause_button, "Pause" );
	}
}

static void UpdateProgress( struct media_control_panel *panel, int val )
{
	TEXTCHAR progress[12];
	tnprintf( progress, 12, "%d.%02d%%", val / 100, val % 100 );
	SetControlText( panel->progress, progress );
}

static void CPROC seek_changed( uintptr_t psvPanel, PSI_CONTROL pc, int val )
{
	struct media_control_panel *panel = ( struct media_control_panel *)psvPanel;
	UpdateProgress( panel, val );
	if( !panel->flags.setting_position )
		ffmpeg_SeekFile( panel->media->file, (int64_t)val * 100000LL );
	// update position text...
}

static void CPROC video_position_update( uintptr_t psvPanel, uint64_t tick )
{
	struct media_control_panel *panel = ( struct media_control_panel *)psvPanel;
	panel->flags.setting_position = 1;
	SetSliderValues( panel->seek_slider, 0, (int)(tick / (100000)), 10000 );  // 100.00%
	panel->flags.setting_position = 0;
	UpdateProgress( panel, (int)(tick / 100000) );

}

static void CPROC KnobTick( uintptr_t psv, int ticks )
{
	struct media_control_panel *panel = (struct media_control_panel *)psv;
	ffmpeg_AdjustVideo( panel->media->file, ticks );
}

void ShowMediaPanel( struct my_button *media )
{
	INDEX idx;
	struct media_control_panel *panel;
	LIST_FORALL( l.controls, idx, struct media_control_panel *, panel )
	{
		if( panel->media == media )
			break;
	}
	if( !panel )
	{
		PSI_CONTROL newPanel = MakeNamedControl( NULL, MyName, 0, 0, 500, 75, -1 );
		MyValidatedControlData(struct media_control_panel*,  new_panel, newPanel );
		panel = new_panel;
		panel->media = media;
		panel->flags.playing = 1; // is playing, otherwise media panel wouldn't be showing...
		panel->knob = MakeNamedControl( newPanel, CONTROL_SCROLL_KNOB_NAME, 0, 0, 50, 50, -1 );
		panel->knob_image = LoadImageFile( "images/dial2a.png" );
		SetScrollKnobImage( panel->knob, panel->knob_image );
		SetScrollKnobEvent( panel->knob, KnobTick, (uintptr_t)panel );
		panel->stop_button = MakeNamedCaptionedControl( newPanel, "Button", 50, 0, 50, 25, -1, "Stop" );
		SetButtonPushMethod( panel->stop_button, stop_pushed, (uintptr_t)panel );
#ifdef _DEBUG
		panel->debug_mem_button = MakeNamedCaptionedControl( newPanel, "Button", 100, 55, 100, 25, -1, "Debug Memory" );
		SetButtonPushMethod( panel->debug_mem_button, debug_mem, (uintptr_t)panel );
#endif
		panel->pause_button = MakeNamedCaptionedControl( newPanel, "Button", 50, 25, 50, 25, -1, "Pause" );
		SetButtonPushMethod( panel->pause_button, pause_pushed, (uintptr_t)panel );
		panel->progress = MakeNamedCaptionedControl( newPanel, "Button", 50, 50, 50, 25, -1, "???" );
		//panel->progress = MakeNamedCaptionedControl( newPanel, "Button", 50, 75, 50, 25, -1, "???" );
		panel->seek_slider = MakeNamedControl( newPanel, SLIDER_CONTROL_NAME,100, 0, 400, 50, -1 );
		SetSliderOptions( panel->seek_slider, SLIDER_HORIZ );
		SetSliderValues( panel->seek_slider, 0, 0, 10000 );  // 100.00%
		SetSliderUpdateHandler( panel->seek_slider, seek_changed, (uintptr_t)panel );
		DisplayFrame( newPanel );
	}
	else
	{
		// have an existing panel to just show.
		RevealCommon( panel->panel );
	}
 	ffmpeg_SetPositionUpdateCallback( panel->media->file, video_position_update, (uintptr_t)panel );
}

void ClosePanel( struct my_button *media )
{
}
