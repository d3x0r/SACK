#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;
static GLOBAL g;


int main( int argc, char **argv )
{
	_32 width, height;
	SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	printf( WIDE("Set log to stout...\n") );
	lprintf( WIDE("Set log to stdout...") );
	if(argc<3)lprintf( WIDE("Usage: %s <background image> <daubing image>\n"), argv[0] );
	GetDisplaySize( &width, &height );;
	{
	PRENDERER display = OpenDisplaySizedAt( 0, width, height, 0, 0 );
	Image back = LoadImageFile( argc > 1?argv[1]:"sky.jpg" );
	Image spot = LoadImageFile( argc > 2?argv[2]:"daub.png" );
   Image surface = GetDisplayImage( display );
	int x, y;
	int i, _i;
	time_t then, now, start;
	SetSystemLog( SYSLOG_FILE, stdout );
	if( argc > 3 )
	{
      if( argv[3][0] == 'M' || argv[3][0] == 'm' )
			SetBlotMethod( BLOT_MMX );
      if( argv[3][0] == 'A' || argv[3][0] == 'a' )
			SetBlotMethod( BLOT_ASM );
      if( argv[3][0] == 'C' || argv[3][0] == 'c' )
			SetBlotMethod( BLOT_C );
	}
   /*
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		//BlotImageAlpha( surface, spot, x, y, ALPHA_TRANSPARENT_INVERT );
		UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (update no image)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
   */
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotImageAlpha( surface, spot, x, y, ALPHA_TRANSPARENT );
		//UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (alpha image,no update)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotScaledImageSizedEx( surface, spot, x, y
									 , 150, 150
									 , 0, 0
									 , spot->width, spot->height
									 , ALPHA_TRANSPARENT_INVERT + (then-start)*10
									 , BLOT_COPY);
		UpdateDisplayPortion( display, x, y, 150, 150 );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (alpha image)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotImageAlpha( surface, spot, x, y, ALPHA_TRANSPARENT_INVERT + ((then-start)*10) );
		UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (alpha image)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotImageAlpha( surface, spot, x, y, ALPHA_TRANSPARENT );
		UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (alpha image)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotImage( surface, spot, x, y );
		//UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (image, single transparent, no update)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	BlotScaledImage( surface, back );
	UpdateDisplay( display );
	time(&then);
   start = then;
	for( _i = i = 0; (then-start) < 10; i++ )
	{
      x = rand( ) % (640 - spot->width);
		y = rand( ) % (480 - spot->height);
		BlotImage( surface, spot, x, y );
		UpdateDisplayPortion( display, x, y, spot->width, spot->height );
		if( ( time(&now) ) > then )
		{
			char message[128];
			sprintf( message, WIDE("%5d spots/second (image, single transparent)"), (i - _i) / (now-then) );
         printf( WIDE("%s\n"), message );
			PutString( surface, 0, 0, BASE_COLOR_BLACK, BASE_COLOR_WHITE, message );
         UpdateDisplayPortion( display, 0, 0, 400, 20 );
			_i = i;
         then = now;
		}
	}
	}
   return 0;
}
