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
#include <network.h>
#include <sqlgetoption.h>
#include <controls.h>

typedef struct POSImage
{
	int number;
	struct {
		BIT_FIELD bShown : 1;
		BIT_FIELD bUpdated : 1;
	} flags;
   // this is the tick when this received.
	uint32_t tick_show;
	struct POSImage *next;
	struct POSImage **me;
   Image image;
} POSIMAGE, *PPOSIMAGE;


typedef struct global_tag {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct
	{
		BIT_FIELD bBlacking : 1;
		BIT_FIELD bLog : 1;
	}flags;

	uint32_t tick_to_switch;

	int show_time; // how long to show the iamge after fading in.
   int fade_in; // how long fade in is..
   int fade_out; // how long fade out is.. (last available to leave)

   int nImages; // convenience counter of the length of images.
	PLIST images; // list of Image's

   int current_image; // overall counter, stepped appropriately.
	// each display has a current image...
	int next_up; // index of the display that is next to show
   int last_up; // index of the prior display that was put UP
	int currents[2];
	int is_up[2];

	PRENDERER displays[2];

   PPOSIMAGE service_available;
   CRITICALSECTION cs;

} GLOBAL;
static GLOBAL g;
Image imgGraphic;
//Image surface;



uint32_t width, height;
#define MAX_STEPS 16
int _ix[MAX_STEPS], _iy[MAX_STEPS];
int _n;
int ix, iy;
int dx, dy;
int wrapped ;
int not_first;



void CPROC Output( uintptr_t psv, PRENDERER display )
{
	if( g.is_up[psv] )
	{
		Image surface = GetDisplayImage( display );
		Image imgGraphic;
		int pos_id = g.currents[psv];
		PPOSIMAGE image = (PPOSIMAGE)GetLink( &g.images, pos_id );
		if( !image->flags.bUpdated )
		{
			if( g.flags.bLog ) lprintf( "Showing image for %d (on %d)", pos_id, psv );
			imgGraphic = image->image;
			BlotScaledImage( surface, imgGraphic );
         image->flags.bUpdated = 1;
		}
		//lprintf( "Ouptut image to surface (once) %d", psv );
		UpdateDisplay( display );
	}
}

int target_out;
int target_in;
// set this to trigger fade in.
int target_in_start;
int target_out_start;

void CPROC tick( uintptr_t psv )
{
	uint32_t now = GetTickCount();

   EnterCriticalSec( &g.cs );
	if( !target_out_start && target_in_start )
	{
		if( g.flags.bLog ) lprintf( "want to show something..." );
		if( !g.service_available )
		{
         if( g.flags.bLog ) lprintf( "No image available..." );
         target_out_start = target_in_start;
			target_in_start = 0;
		}
		else if( now >= target_in_start )
		{
         if( g.flags.bLog ) lprintf( "First image..." );
			if( !g.service_available->flags.bShown || !target_in )
			{
				target_in = target_in_start + g.fade_in;
            if( g.flags.bLog ) lprintf( "my first is %d", g.service_available->number );
            g.currents[g.next_up] = g.service_available->number;
				//g.current_image++;
				//if( g.current_image >= g.nImages )
				//	g.current_image = 0;
				//if( g.current_image >= g.nImages )
				//{
            //   return;
				if( g.flags.bLog )
					lprintf( "(!)Setting fade to..> %d", 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
				SetDisplayFade( g.displays[g.next_up], 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
				g.is_up[g.next_up] = 1;
            g.service_available->flags.bShown = 1;
            g.service_available->flags.bUpdated = 0;
				RestoreDisplay( g.displays[g.next_up] );
				MakeTopmost( g.displays[g.next_up] );

            g.last_up = g.next_up;
				//ForceDisplayFront( g.displays[g.next_up] );
			}
			else
			{
				if( target_in && now > target_in )
				{
					SetDisplayFade( g.displays[g.next_up], 0 );
					//Redraw( g.displays[g.next_up] );
					//target_in_start = now + g.show_time;
					target_in = 0;
               if( g.flags.bLog ) lprintf( "Set last_up, and move to next_up" );
               g.last_up = g.next_up;
					g.next_up++;
					if( g.next_up >= 2 ) // two displayss
						g.next_up = 0;

					// hide the old display...
					if( g.flags.bLog ) lprintf( "hiding old display..." );
					g.is_up[g.next_up] = 0;
					HideDisplay( g.displays[g.next_up] );
					target_in_start = 0;
               target_in = 0;
				}
				else
				{

					if( g.flags.bLog ) lprintf( "Setting fade to..> %d", 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
					SetDisplayFade( g.displays[g.next_up], 255 - (255*(now-target_in_start))/(target_in-target_in_start) );
					//Redraw( g.displays[g.next_up] );
				}
			}
		}
	}
	if( target_out_start )
	{
      if( g.flags.bLog ) lprintf( "want to hide the last one." );
		if( !target_out )
		{
         if( g.flags.bLog ) lprintf( "setup new target fade." );
			target_out = target_out_start + g.fade_out;

         if( g.flags.bLog ) lprintf( "Set to %d", (255*(now-target_out_start))/(target_out-target_out_start) );
			SetDisplayFade( g.displays[g.last_up], (255*(now-target_out_start))/(target_out-target_out_start) );
		}
		else
		{
				if( target_out && ( ( now > target_out ) || ( ( 255*(now-target_out_start))/(target_out-target_out_start) == 0 ) ) )
				{
					if( g.flags.bLog ) lprintf( "Fade complete" );
					HideDisplay( g.displays[g.last_up] );
               g.is_up[g.last_up] = 0;
					target_out_start = 0;
					target_out = 0;
					if( target_in_start )
                  target_in_start = now;
				}
				else
				{
					//if( g.flags.bLog ) lprintf( "BLAH" );
               if( g.flags.bLog ) lprintf( "set to %d", (255*(now-target_out_start))/(target_out-target_out_start) );
					SetDisplayFade( g.displays[g.last_up], (255*(now-target_out_start))/(target_out-target_out_start) );
					//UpdateDisplay( g.displays[g.last_up] );
				}
		}
	}
	LeaveCriticalSec( &g.cs );

}

void CPROC UDPReceive( PCLIENT pc, POINTER buffer, int size, SOCKADDR *saFrom )
{
	if( !buffer )
	{
		size = 1024;
      buffer = Allocate( 1024 );
	}
	else
	{
      uint32_t number;
		char seq[9];
		seq[8] = 0;
		((uint64_t*)seq)[0] = ((uint64_t*)buffer)[0];
		sscanf( seq, "%X", &number );
		if( (((uint64_t*)buffer)+1)[0] == ((uint64_t*)"SERVING ")[0] )
		{
         char seq[9];
			int POS = atoi( ((TEXTSTR)((uintptr_t)buffer+16)) );
			EnterCriticalSec( &g.cs );
         if( g.flags.bLog ) lprintf( "Serving." );

			if( POS < 32 && POS >= 1 )
			{
				PPOSIMAGE pos_image = (PPOSIMAGE)GetLink( &g.images, POS );
				if( pos_image && !pos_image->me )
				{
					if( pos_image->tick_show != number )
					{
						LinkLast( g.service_available, PPOSIMAGE, pos_image );
                  if( g.flags.bLog ) lprintf( "Adding to service available. %p", pos_image );
						pos_image->tick_show = number;
						pos_image->flags.bShown = 0;
                  if( g.service_available == pos_image )
							target_in_start = GetTickCount();
					}
				}
			}
			LeaveCriticalSec( &g.cs );
		}
		else if( (((uint64_t*)buffer)+1)[0] == ((uint64_t*)"SERVED  ")[0] )
		{
			int POS = atoi( ((TEXTSTR)((uintptr_t)buffer+16)) );
         if( g.flags.bLog ) lprintf( "Served." );
			EnterCriticalSec( &g.cs );
			if( POS < 32 && POS >= 1 )
			{
				PPOSIMAGE pos_image = (PPOSIMAGE)GetLink( &g.images, POS );
            if( g.flags.bLog ) lprintf( "... %p", pos_image );
 				if( pos_image && pos_image->me )
				{
               if( g.flags.bLog ) lprintf( "and it was %d %d", pos_image->tick_show, number );
					if( pos_image->tick_show != number )
					{
                  // if it's the first... show... (need update)
						if( pos_image == g.service_available )
							target_in_start = GetTickCount();

						UnlinkThing( pos_image );
                  if( g.flags.bLog ) lprintf( "global is %p", g.service_available );
						pos_image->tick_show = number;
                  if( g.flags.bLog ) lprintf( "Cleared an image!? (first?" );
					}
				}
			}
			LeaveCriticalSec( &g.cs );
		}
		else if( (((uint64_t*)buffer)+1)[0] == ((uint64_t*)"CLEARALL")[0] )
		{
			static uint32_t last_number;
			EnterCriticalSec( &g.cs );
			if( number != last_number )
			{
            last_number = number;
				while( g.service_available )
				{
               PPOSIMAGE tmp = g.service_available;
					UnlinkThing( tmp );
				}
				target_in_start = GetTickCount() - g.fade_out;
			}
			LeaveCriticalSec( &g.cs );
		}
		else
		{
		}
	}
   ReadUDP( pc, buffer, 1024 );
}

int main( int argc, char **argv )
{
	int x = 0;
	int y = 0;
   uint32_t width, height;
   int w, h;
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
   RegisterIcon( NULL );
	GetDisplaySize( &width, &height );
   w = width; h = height;
   InitializeCriticalSec( &g.cs );

	{

		TEXTCHAR buf[256];
		int n;
		BeginBatchUpdate();
		for( n = 1; n < 32; n++ )
		{
			TEXTCHAR entry[64];
			TEXTCHAR def_val[64];
			snprintf( entry, sizeof( entry ), "Image File %d", n );
			snprintf( def_val, sizeof( def_val ), "images/pos%d.jpg", n );
			SACK_GetPrivateProfileStringEx( "Images", entry, def_val, buf, sizeof( buf ),GetProgramName(), TRUE );
			{
				Image x = LoadImageFile( buf );
				if( x )
				{
					PPOSIMAGE newpos = New( POSIMAGE );
               MemSet( newpos, 0, sizeof( POSIMAGE ) );
					newpos->image = x;
					newpos->number = n;
               SetLink( &g.images, n, newpos );
				}
			}
		}
      EndBatchUpdate();
	}
	{
      TEXTCHAR buf[256];
		PCLIENT pc;
      g.flags.bLog = SACK_GetProfileIntEx( GetProgramName(), "Enable Logging", 0, TRUE );
		SACK_GetProfileStringEx( GetProgramName(), "UDP Service", "0.0.0.0:3233", buf, sizeof( buf ), TRUE );
      NetworkStart();
		pc = ServeUDP( buf, 3233, UDPReceive, NULL );
		if( !pc )
		{
         SimpleMessageBox( NULL, "Failed to open on interface (title), default port 3233", buf );
         return 0;
		}
      UDPEnableBroadcast( pc, TRUE );

	}

   y = x  = 0;

	{
      int state = 0;
		int arg;
		g.fade_in = 500;
      g.fade_out = 750;
      g.show_time = 1000;
		for( arg = 1; arg < argc; arg++ )
		{

			if( argv[arg][0] == '-' )
			{
				switch( state )
				{
				case 0:
					x = atoi( argv[arg]+1 );
               break;
				case 1:
					y = atoi( argv[arg]+1 );
               break;
				case 2:
					w = atoi( argv[arg]+1 );
               break;
				case 3:
					h = atoi( argv[arg]+1 );
               break;
				case 4:
					g.show_time = atoi( argv[arg]+1 );
               break;
				case 5:
					g.fade_in = atoi( argv[arg]+1 );
               break;
				case 6:
					g.fade_out = atoi( argv[arg]+1 );
               break;
				}
            state++;
			}
		}
	}

   //if( g.nImages )
	{
		g.displays[0] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
														  , w //width
														  , h //height
														  , x //0
														  , y //0
											 );
		g.displays[1] = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_CHILD|DISPLAY_ATTRIBUTE_NO_MOUSE|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
														  , w //width
														  , h //height
														  , x //0
														  , y //0
											 );
		SetRedrawHandler( g.displays[0], Output, 0 );
		SetRedrawHandler( g.displays[1], Output, 1 );
      MakeTopmost( g.displays[0] );
      MakeTopmost( g.displays[1] );

		AddTimer( 33, tick, 0 );

		while( 1 )
			WakeableSleep( 10000 );
	}
   return 0;
}

