//Requires a GUI environment, so if you are building with
//__NO_GUI__=1 this will never work.

#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <systray.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>
#include <timers.h>

#include "vlcint.h"

struct video_player {
	PRENDERER surface;
	struct my_vlc_interface *vlc;
   _32 fade_in_time;
};

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct
	{
		BIT_FIELD bBlacking : 1;
	}flags;

	_32 tick_to_switch;

	int show_time; // how long to show the iamge after fading in.
	int fade_in; // how long fade out is..
   int next_fade_in;

   int nImages; // convenience counter of the length of images.
	PLIST images; // list of Image's
   PLIST image_type;

   int current_image; // overall counter, stepped appropriately.
	// each display has a current image...
   int next_up; // index of the display that is next to show
	int currents[2];
   int is_up[2];
	PRENDERER displays[2];
   PRENDERER prior_up;
	PLIST video_displays;

	int x;
	int y;
	int w, h;

} GLOBAL;
static GLOBAL g;
Image imgGraphic;
//Image surface;

_32 width, height;
#define MAX_STEPS 16
int _ix[MAX_STEPS], _iy[MAX_STEPS];
int _n;
int ix, iy;
int dx, dy;
int wrapped ;
int not_first;



void CPROC Output( PTRSZVAL psv, PRENDERER display )
{
	if( g.is_up[psv] )
	{
		Image surface = GetDisplayImage( display );
		Image imgGraphic;
      //lprintf( "Current on %d is %d", psv, g.currents[psv] );
		imgGraphic = (Image)GetLink( &g.images, g.currents[psv] );
		BlotScaledImage( surface, imgGraphic );
	}
}

int target_out;
INDEX current_image;
int target_in;
int target_in_start;

void CPROC tick( PTRSZVAL psv )
{
	_32 now = GetTickCount();

	if( target_in_start )
	{
		if( now >= target_in_start )
		{
			if( !target_in )
			{
				PRENDERER r;
            //lprintf( "Begin fading in the new image..." );
				target_in = target_in_start + g.fade_in;
				g.currents[g.next_up] = g.current_image;

				if( GetLink( &g.image_type, g.current_image ) == (POINTER)1 )
				{
					r = g.displays[g.next_up];
					g.is_up[g.next_up] = 1;
				}
				else
				{
					struct video_player *player = (struct video_player *)GetLink( &g.images, g.current_image );
					r = player->surface;
					PlayItem( player->vlc ); // start the video clip now... allow fadein.
               target_in = target_in_start + player->fade_in_time;
				}

            //lprintf( "Begin Showing %p", r );
				//lprintf( "(!)Setting fade to..> %d", 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
				SetDisplayFade( r, 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
				RestoreDisplay( r );
				//ForceDisplayFront( r );
			}
			else
			{
				if( target_in && now > target_in )
				{
               int upd;
					PRENDERER r;
					if( GetLink( &g.image_type, g.current_image ) == (POINTER)1 )
					{
                  upd = 1;
						r = g.displays[g.next_up];
						g.next_up++;
						if( g.next_up >= 2 ) // two displayss
							g.next_up = 0;
						g.is_up[g.next_up] = 0;
						target_in_start = now + g.show_time;
					}
					else
					{
						struct video_player *player = (struct video_player *)GetLink( &g.images, g.current_image );
						r = player->surface;
                  // wait until stop event to set when next fadin starts.
						target_in_start = 0;
 					}
               //lprintf( "Fade is is complete, set alpha to 0, and hide old display..." );
					SetDisplayFade( r, 0 );
					if( upd )
                  UpdateDisplay( r );

					if( g.prior_up )
					{
                  //lprintf( "hiding %p", g.prior_up );
						HideDisplay( g.prior_up );
					}

					g.prior_up = r;

               //lprintf( "Image fully up - setup to show next image..." );
					g.current_image++;
					if( g.current_image >= g.nImages )
						g.current_image = 0;


					target_in = 0;
				}
				else
				{
               int upd = 0;
					PRENDERER r;
					if( GetLink( &g.image_type, g.current_image ) == (POINTER)1 )
					{
                  upd = 1;
						r = g.displays[g.next_up];
					}
					else
					{
						struct video_player *player = (struct video_player *)GetLink( &g.images, g.current_image );
                  r = player->surface;
					}
					//lprintf( "Setting fade to..> %d", 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
               //lprintf( "Increasing %p", r );
					SetDisplayFade( r, 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
               if( upd )
						UpdateDisplay( r );
				}
			}
		}
	}
}

static void CPROC OnStopFade( PTRSZVAL psv )
{
   struct video_player *player = (struct video_player*)psv;
	target_in_start = GetTickCount(); // tick now.

   StopItem( player->vlc );
}

void LoadVideo( CTEXTSTR file )
{
   struct my_vlc_interface *vlc;
	struct video_player *player = New( struct video_player );
	player->fade_in_time = g.next_fade_in?g.next_fade_in:g.fade_in;
   g.next_fade_in = 0;
	player->surface = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
													, g.w //width
													, g.h //height
													, g.x //0
													, g.y //0
													);
	player->vlc = PlayItemOnEx( player->surface, file, "--repeat --loop" );
	SetStopEvent( player->vlc, OnStopFade, (PTRSZVAL)player );

   // only used if there is only 1 video - just show video.
	AddLink( &g.video_displays, player );
	AddLink( &g.images, player );
	AddLink( &g.image_type, (POINTER)2 );
   g.nImages++;
}

SaneWinMain(argc, argv )
//int main( int argc, char **argv )
{
   _32 width, height;
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
   RegisterIcon( NULL );
	GetDisplaySize( &width, &height );
	g.w = width;
	g.h = height;


   g.y = g.x  = 0;

	{
      int state = 0;
		int arg;
		g.fade_in = 500;
      g.show_time = 1000;
		for( arg = 1; arg < argc; arg++ )
		{

			if( argv[arg][0] == '-' )
			{
				switch( state )
				{
				case 0:
					g.x = atoi( argv[arg]+1 );
               break;
				case 1:
					g.y = atoi( argv[arg]+1 );
               break;
				case 2:
					g.w = atoi( argv[arg]+1 );
               break;
				case 3:
					g.h = atoi( argv[arg]+1 );
               break;
				case 4:
					g.show_time = atoi( argv[arg]+1 );
               break;
				case 5:
					g.fade_in = atoi( argv[arg]+1 );
					break;
				case 6:
               g.next_fade_in = atoi( argv[arg]+1 );
               state--; // stay at state 6
               break;
				}
            state++;
			}
			else if( argv[arg][0] == '@' )
			{
				FILE *file = fopen( argv[arg] + 1, "rt" );
				TEXTCHAR filename[256];
				while( fgets( filename, sizeof( filename ), file ) )
				{
					int len = StrLen( filename );
					if( len < 2 )
						continue;
					if( filename[0] == '#' )
						continue;
					if( isdigit( filename[0] ) && (len < 12))
					{
                  g.next_fade_in = atoi( filename );
                  continue;
					}
					if( filename[StrLen( filename )-1] == '\n' )
						filename[StrLen( filename )-1] = 0;

					{
						if( StrCaseStr( filename, ".jpg" ) ||
							StrCaseStr( filename, ".jpeg" ) ||
							StrCaseStr( filename, ".bmp" ) ||
							StrCaseStr( filename, ".png" ) ||
							StrCaseStr( filename, ".tga" ) )
						{
							Image img = LoadImageFile( filename );
							if( img )
							{
								g.nImages++;
								AddLink( &g.images, img );
								AddLink( &g.image_type, (POINTER)1 );
							}
						}
						else
						{
							LoadVideo( filename );
						}
					}

				}
			}
			else
			{
				if( StrCaseStr( argv[arg], ".jpg" ) ||
					StrCaseStr( argv[arg], ".jpeg" ) ||
					StrCaseStr( argv[arg], ".bmp" ) ||
					StrCaseStr( argv[arg], ".png" ) ||
					StrCaseStr( argv[arg], ".tga" ) )
				{
					Image img = LoadImageFile( argv[arg] );
					if( img )
					{
						g.nImages++;
						AddLink( &g.images, img );
						AddLink( &g.image_type, (POINTER)1 );
					}
				}
				else
				{
               LoadVideo( argv[arg] );
				}
			}
		}
	}

   if( g.nImages )
	{
		g.displays[0] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
													 , g.w //width
													 , g.h //height
													 , g.x //0
													 , g.y //0
													 );
		g.displays[1] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
													 , g.w //width
													 , g.h //height
													 , g.x //0
													 , g.y //0
													 );

		SetRedrawHandler( g.displays[0], Output, 0 );
		SetRedrawHandler( g.displays[1], Output, 1 );

		if( g.nImages > 1 )
		{
			target_in_start = GetTickCount();
			AddTimer( 33, tick, 0 );
		}
		else
		{
			//lprintf( "Show the first and only the first image." );
			if( GetLink( &g.image_type, 0 ) == (POINTER)1 )
			{
				g.is_up[0] = 1;
				RestoreDisplay( g.displays[0] );
				UpdateDisplay( g.displays[0] );
			}
			else
			{
            RestoreDisplay( ((struct video_player*)GetLink( &g.video_displays, 0 ))->surface );
			}
		}

		while( 1 )
			WakeableSleep( 10000 );
	}
	else
	{
      printf( "No Images to display, exiting\n" );
	}
   return 0;
}
EndSaneWinMain()

