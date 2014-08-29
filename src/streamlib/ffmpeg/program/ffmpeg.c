#include <stdhdrs.h>


// have to include this first to setup common image and render interface defines...
#define VIDEO_PLAYER_MAIN_SOURCE
#include "video_player_local.h"



PRELOAD( LoadInterface )
{
	l.pdi = GetDisplayInterface();
	l.pii = GetImageInterface();
	l.button_display_id = -1;
	GetDisplaySize( &l.display_width, &l.display_height );
}


struct GetDisplayParams
{
	PRENDERER result;
	struct my_button *media;
};

static int CPROC myKeyProc( PTRSZVAL dwUser, _32 key )
{
   lprintf( "key..." );
	if( IsKeyPressed( key ) && ( KEY_CODE( key ) == KEY_SPACE ) )
	{
		struct my_button *media = (struct my_button *)dwUser;
      lprintf( "magic key..." );
		if( media->flags.showing_panel )
		{
			media->flags.showing_panel = 0;
			HideMediaPanel( media );
		}
		else
		{
			media->flags.showing_panel = 1;
			ShowMediaPanel( media );
		}
	}
   return 0;
}

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
	// output on windows is more direct using layered window (full screen may not work right)
	if( !( result = l.renderer ) )
	{
		l.renderer = result = OpenDisplaySizedAt( 0 /*DISPLAY_ATTRIBUTE_LAYERED*/, w, h, l.x_ofs, l.y_ofs );
		SetKeyboardHandler( result, myKeyProc, (PTRSZVAL)p->media );
		// auto hide idle mouse on this surface
		DisableMouseOnIdle( result, TRUE );
		SetDisplayFullScreen( result, l.full_display );
	}
	else
	{
		SizeDisplay( result, w, h );
	}
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
//static void CPROC UpdateCallback( PTRSZVAL psv, _64 tick )
//{
//	UpdatePositionCallback(  psv, tick );
//}

static void CPROC VideoEndedCallback( PTRSZVAL psv )
{
	struct my_button *me = ( struct my_button *)psv;
	HideDisplay( me->render );
	SuspendSystemSleep( 0 );
	l.stopped = TRUE;
   WakeThread( l.main_thread );
}

static void CPROC VideoPlayError( CTEXTSTR string )
{
	//Banner2Message( string );
}

static struct my_button * PlayVideo( CTEXTSTR name )
{
	struct GetDisplayParams params;
	struct my_button *me;
	me = &l.me;
	{
		params.media = me;

		me->file = ffmpeg_LoadFile( name, GetDisplay, (PTRSZVAL)&params
			, NULL, 0
			, NULL
			, VideoEndedCallback, (PTRSZVAL)me
			, VideoPlayError );
		if( me->file )
		{
			//SetLink( &l.files, me->ID, me->file );
			me->render = params.result;
		}
	}
	if( me->file )
	{
		SuspendSystemSleep( 1 );
		RestoreDisplay( me->render );
		ffmpeg_PlayFile( me->file );
	}
   return me;
}


static void OnDisplayPause( "Video Player" )( void )
{
	struct ffmpeg_file * file;
	INDEX idx;
	SuspendSystemSleep( 0 );
	ffmpeg_PauseFile( l.me.file );
}

static void OnDisplayResume( "Video Player" )( void )
{
	struct ffmpeg_file * file;
	INDEX idx;
	int resumed = 0;
	{
		if( !resumed )
		{
			SuspendSystemSleep( 1 );
			resumed = 1;
		}
		ffmpeg_PlayFile( l.me.file );
	}
}

static void ReloadPlayed( void )
{
	FILE *file = sack_fopen( 0, "playlist.m3u", "rt" );
	if( file )
	{
		TEXTCHAR buf[256];
		while( fgets( buf, 256, file ) )
		{
			buf[strlen(buf)-1] = 0;
			AddLink( &l.videos_played, StrDup( buf ) );
		}
		fclose( file );
	}
}

static void SavePlayed( CTEXTSTR newname )
{
	INDEX idx;
	CTEXTSTR name;

	LIST_FORALL( l.videos_played, idx, CTEXTSTR, name )
	{
		if(StrCmp( name, newname ) == 0 )
		{
			break;
		}
	}
	if( !name )
	{
		FILE *file = sack_fopen( 0, "playlist.m3u", "wt" );
		if( file )
		{
			fprintf( file, "%s\n", newname );
			LIST_FORALL( l.videos_played, idx, CTEXTSTR, name )
			{
				fprintf( file, "%s\n", name );
			}
			fclose( file );
		}
	}
}

#ifndef __ANDROID__
SaneWinMain( argc, argv )
{
	int n;
   ReloadPlayed();
	for( n = 1; n < argc; n++ )
	{
		struct my_button *video;
		if( argv[n][0] == '@' )
		{
			FILE *file = sack_fopen( 0, argv[n] + 1, "rt" );
			TEXTCHAR buf[256];
			if( file )
			{
				while( fgets( buf, 256, file ) )
				{
               buf[strlen(buf)-1] = 0;

					l.stopped = 0;
               lprintf( "start newvideo...");
					video = PlayVideo( buf );
					l.main_thread = MakeThread();
					while( !l.stopped )
					{
						WakeableSleep( 100000 );
					}
					ffmpeg_UnloadFile( l.me.file );

				}
				fclose( file );
			}
         continue;
		}
		else if( argv[n][0] == '-' )
		{
			if( argv[n][1] == 'd' )
			{
				n++;
            l.full_display = atoi( argv[n] );
			}
         continue;
		}
		{
			SavePlayed( argv[n] );
			video = PlayVideo( argv[n] );
			l.main_thread = MakeThread();
			while( !l.stopped )
			{
				WakeableSleep( 100000 );
			}
		}
	}
   return 0;
}
EndSaneWinMain()
#endif

#ifdef __ANDROID__

static PTRSZVAL CPROC PlayMediaThread( PTHREAD thread )
{
   char *media = (char*)GetThreadParam( thread );
	struct my_button *video;
	l.stopped = 0;
	lprintf( "start newvideo...");
	video = PlayVideo( media );
	while( l.player_thread != NULL )
      WakeableSleep( 1000 );
	l.player_thread = thread;
	while( !l.stopped )
	{
		WakeableSleep( 100000 );
	}
	ffmpeg_UnloadFile( l.me.file );
	l.player_thread = NULL;
}

// entry point from Java to play video....
void VideoPlayer_PlayMedia( char *media )
{
   ThreadTo( PlayMediaThread, (PTRSZVAL)media );
}

// generic do-nothing activity code
SaneWinMain( argc, argv )
{
	l.main_thread = MakeThread();
	while( !l.quit )
	{
		WakeableSleep( 100000 );
	}
   return 0;
}
EndSaneWinMain()

#endif
