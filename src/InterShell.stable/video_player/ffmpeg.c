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

static CTEXTSTR GetActiveDisplay( void )
{
	static TEXTCHAR result[12];
	if( l.full_display == -1 )
		return WIDE( "Not Full Screen" );
	if( l.full_display == 0 )
		return WIDE( "Default" );

	tnprintf( result, 12,  WIDE( "%d" ), l.full_display );
	return result;
}

PRELOAD( LoadInterface )
{
	l.pdi = GetDisplayInterface();
	l.pii = GetImageInterface();
	l.button_display_id = -1;
	GetDisplaySize( &l.display_width, &l.display_height );
	l.label_active_display = CreateLabelVariable( WIDE( "<Video/Active Display>"), LABEL_TYPE_PROC, GetActiveDisplay );
}

static PTRSZVAL OnCreateMenuButton( WIDE( "Test Video Player Load" ) )( PMENU_BUTTON button )
{
	struct my_button *me = New( struct my_button );
	MemSet( me, 0, sizeof( struct my_button ) );
	me->ID = l.button_id++;
	me->button = button;
	me->file = NULL;
	AddLink( &l.buttons, me );
	return (PTRSZVAL)me;
}

struct GetDisplayParams
{
	PRENDERER result;
};

static PRENDERER CPROC GetDisplay( PTRSZVAL psv, _32 w, _32 h )
{
	struct GetDisplayParams *p = (struct GetDisplayParams*)psv;

	PRENDERER result;
	if( ( l.x_ofs + w ) > l.display_width )
	{
		l.x_ofs = 0;
		l.y_ofs += h;
	}
	if( ( l.y_ofs + h ) > l.display_height )
		l.y_ofs = 0;
	//lprintf( "GetOpenDsiplay at %d %d", w, h );

	// all movie content is opaque anyway.
	result = OpenDisplaySizedAt( 
#ifdef WIN32
		DISPLAY_ATTRIBUTE_LAYERED  /* this allows in-thread update to surface */
#else
		0 /*DISPLAY_ATTRIBUTE_LAYERED*/
#endif
		, w, h, l.x_ofs, l.y_ofs );
	// auto hide idle mouse on this surface
	DisableMouseOnIdle( result, TRUE );
#ifdef __ANDROID__
	SetDisplayFullScreen( result, l.full_display );
#endif
	
	//MakeTopmost( result );
	p->result = result;
#ifndef __ANDROID__
	l.x_ofs += w;
	if( l.x_ofs >= l.display_width )
	{
		l.x_ofs = 0;
		l.y_ofs += h;
		if( l.y_ofs >= l.display_height )
			l.y_ofs = 0;
	}
#endif
	return result;
}

/* tick is from 0-1000000000  as 0-1.0 or 0-100% */
static void CPROC UpdateCallback( PTRSZVAL psv, _64 tick )
{
	UpdatePositionCallback(  psv, tick );
}

static void CPROC VideoError( CTEXTSTR message )
{
   CreateBanner2Ex( NULL, NULL, message, BANNER_ALPHA|BANNER_NOWAIT|BANNER_CLICK|BANNER_TOP|BANNER_TIMEOUT, 5000 );
}

static void CPROC VideoEndedCallback( PTRSZVAL psv )
{
	struct my_button *me = ( struct my_button *)psv;
	HideDisplay( me->render );
	SuspendSystemSleep( 0 );
}

static PTRSZVAL LoadMovieThread( PTHREAD thread )
{
	PTRSZVAL psv_button = GetThreadParam( thread );
	struct my_button *me = ( struct my_button *)psv_button;
	struct GetDisplayParams params;
		TEXTSTR files[] = { 
#ifdef WIN32
#define movie_prefix "c:/tmp/"
#else
#define movie_prefix
#endif
			//#if 0
			//, "F:/continuum/Season 2/Continuum.S02E01.HDTV.XviD-AFG.avi"
			//, "F:/continuum/Season 2/continuum.s02e02.hdtv.x264-2hd.mp4" 
			//, "F:/continuum/Season 2/continuum.s02e03.hdtv.x264-2hd.mp4" 
			//, "F:/continuum/Season 2/Continuum.S02E04.HDTV.XviD-AFG.avi"
			//"G:/downloads/Lord.of.War.2005.WS.DVDRip.x264-REKoDE/Lord.of.War.2005.WS.DVDRip.x264-REKoDE.mkv"
			movie_prefix"$HP.mkv"//"OCCUPY FREE ENERGY.mp4"
								,movie_prefix/*"/storage/sdcard0/download/"*/"$wildlife.mkv"  //"$wildlife.mkv"
								,movie_prefix/*"/storage/sdcard0/download/"*/"$chills.mkv"
								,movie_prefix/*"/storage/sdcard0/download/"*/"$a_different_corner.mkv"
								,  "http://d3x0r.org:85/Movies/Doctor%20who/doctor_who_2005.2013_christmas_special.the_time_of_the_doctor.hdtv_x264-fov.mp4"
			, "Immortal Technique- Bin Laden.mp4"
			, "http://d3x0r.org:85/Movies/The%20Lone%20Gunmen/The%20Lone%20Gunmen%201x01%20-%20Pilot.avi"
			, "http://d3x0r.org:85/Movies/Seven.Psychopaths.2012.DVDSCR.XviD.AbSurdiTy.avi"
			, "http://d3x0r.org:85/Movies/The%20Incredible%20Burt%20Wonderstone%202013%20BRRip%20AAC%20x264-SSDD.mp4"
			//BROKEN, "http://d3x0r.org:85/Movies/Monty%20Python%20-%20life-of-brian.avi"
			// "http://d3x0r.org:85/Movies/THE%20SECRET%20OF%20NIKOLA%20TESLA.mp4"
			// " laithewait heretic is broken too
			// broken "http://d3x0r.org:85/Movies/The.Wolf.of.Wall.Street.2013.DVDScr.x264-HaM.m4v"
         ,
//#endif
		  "E:/downloads/Sirius Movie.mp4"
		 , "E:/downloads/The.Last.Stand.2013.ENG.HDRip.1.46GB.-Lum1x-.avi"
		 , "E:/downloads/The.Big.Bang.Theory.S07E04.HDTV.XviD-AFG/The.Big.Bang.Theory.S07E04.HDTV.XviD-AFG.avi"
		 , "E:/downloads/Breaking.Bad.S05E10.HDTV.x264-ASAP/Breaking.Bad.S05E10.HDTV.x264-ASAP.mp4"
		};
			me->file = ffmpeg_LoadFile( files[me->ID], GetDisplay, (PTRSZVAL)&params
				, NULL, 0
											  , UpdateCallback, (PTRSZVAL)me
											  , VideoEndedCallback, (PTRSZVAL)me
											  , VideoError
											  );
		if( me->file )
		{
			AddLink( &l.active_files, me->file );
			SetLink( &l.files, me->ID, me->file );
			me->render = params.result;

         lprintf( "suspend sleep..." );
			SuspendSystemSleep( 1 );
         lprintf( "restore sleep..." );
			RestoreDisplay( me->render );
         lprintf( "play file..." );
			ffmpeg_PlayFile( me->file );
		}

}

static void OnKeyPressEvent( WIDE( "Test Video Player Load" ) )( PTRSZVAL psv_button )
{
	struct my_button *me = ( struct my_button *)psv_button;
	//struct GetDisplayParams params;
	if( !me->file )
	{
      lprintf( "Set suspend sleep here.." );
		SuspendSystemSleep( 1 );
		ThreadTo( LoadMovieThread, (PTRSZVAL)me );
      lprintf( ".." );
		/*
		me->file = ffmpeg_LoadFile( files[me->ID], GetDisplay, (PTRSZVAL)&params
			, UpdateCallback, (PTRSZVAL)me 
			, VideoEndedCallback, (PTRSZVAL)me );
		if( me->file )
		{
			AddLink( &l.active_files, me->file );
			SetLink( &l.files, me->ID, me->file );
			me->render = params.result;
		}
		*/
	}
	else
	{
		SuspendSystemSleep( 1 );
		RestoreDisplay( me->render );
		ffmpeg_PlayFile( me->file );
	}
}

static PTRSZVAL OnCreateMenuButton( WIDE( "Test Video Player Set Display" ) )( PMENU_BUTTON button )
{
	struct my_button *me = New( struct my_button );
	MemSet( me, 0, sizeof( struct my_button ) );
	me->ID = l.button_display_id++;
	me->button = button;
	return (PTRSZVAL)me;
}

static void OnKeyPressEvent( WIDE( "Test Video Player Set Display" ) )( PTRSZVAL psv_button )
{
	struct my_button *me = ( struct my_button *)psv_button;
	l.full_display = me->ID;
	LabelVariableChanged( l.label_active_display );
}

static PTRSZVAL OnCreateMenuButton( WIDE( "Test Video Player Pause" ) )( PMENU_BUTTON button )
{
	struct my_button *me = New( struct my_button );
	MemSet( me, 0, sizeof( struct my_button ) );
	me->ID = l.button_pause_id++;
	me->button = button;
	return (PTRSZVAL)me;
}

static void OnKeyPressEvent( WIDE( "Test Video Player Pause" ) )( PTRSZVAL psv_button )
{
	struct my_button *me = ( struct my_button *)psv_button;
	INDEX idx;
	struct my_button *related;
	LIST_FORALL( l.buttons, idx, struct my_button *, related )
	{
		if( related->ID == me->ID )
			if( related->file )
			{
				lprintf( "PAUSE IS CLICKED and a related button %d is %p", me->ID, related->file );
				SuspendSystemSleep( 0 );
				ffmpeg_PauseFile( related->file );
			}
	}
}


static PTRSZVAL OnCreateMenuButton( WIDE( "Test Video Player Stop" ) )( PMENU_BUTTON button )
{
	struct my_button *me = New( struct my_button );
	me->ID = l.button_stop_id++;
	me->button = button;
	return (PTRSZVAL)me;
}

static void OnKeyPressEvent( WIDE( "Test Video Player Stop" ) )( PTRSZVAL psv_button )
{
	struct my_button *me = ( struct my_button *)psv_button;
	INDEX idx;
	struct my_button *related;
	LIST_FORALL( l.buttons, idx, struct my_button *, related )
	{
		if( related->ID == me->ID )
			if( related->file )
			{
				DeleteLink( &l.active_files, related->file );

				ffmpeg_UnloadFile( related->file );
				related->file = NULL;
				CloseDisplay( related->render );
				related->render = NULL;
			}
	}
}

static void OnDisplayPause( "Video Player" )( void )
{
	struct ffmpeg_file * file;
	INDEX idx;
	SuspendSystemSleep( 0 );
	LIST_FORALL( l.active_files, idx, struct ffmpeg_file *, file )
		ffmpeg_PauseFile( file );
}

static void OnDisplayResume( "Video Player" )( void )
{
	struct ffmpeg_file * file;
	INDEX idx;
	int resumed = 0;
	LIST_FORALL( l.active_files, idx, struct ffmpeg_file *, file )
	{
		if( !resumed )
		{
			SuspendSystemSleep( 1 );
			resumed = 1;
		}
		ffmpeg_PlayFile( file );
	}
}

