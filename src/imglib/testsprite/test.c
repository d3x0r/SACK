#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE

#include <stdhdrs.h>
#include <timers.h>
#include <render.h>
#include <image.h>

#define MAX_SPRITES 150

struct {
	PSPRITE sprite;
	int dx, dy;
} sprite[MAX_SPRITES];
PRENDERER render;
int rot;
int sprites = 1;

Image background;

void ProcessMotion( void )
{
	int n;
	for( n = 0; n < sprites; n++ )
	{
      sprite[n].sprite->curx += sprite[n].dx;
      sprite[n].sprite->cury += sprite[n].dy;
		if( sprite[n].sprite->curx < 50 )
         sprite[n].dx = ( 15 * rand() ) / RAND_MAX + 1;
		if( sprite[n].sprite->cury < 50 )
         sprite[n].dy = ( 15 * rand() ) / RAND_MAX + 1;
		if( sprite[n].sprite->curx > (1024-50) )
         sprite[n].dx = -(( 15 * rand() ) / RAND_MAX + 1);
		if( sprite[n].sprite->cury > (768-50) )
         sprite[n].dy = -(( 15 * rand() ) / RAND_MAX + 1);
	}
}

void CPROC SpriteDrawProc( PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h )
{
	int n;
   lprintf( WIDE("Update sprites...") );
	for( n = 0; n < sprites; n++ )
	{
		rotate_scaled_sprite( GetDisplayImage( renderer ), sprite[n].sprite, rot, 0x10000, 0x10000 );
	}
   lprintf( WIDE("Updated sprites...") );

}

void CPROC DrawProc( PTRSZVAL psv, PRENDERER renderer )
{
	int n;
   lprintf( WIDE("uhmm nothing I guess") );
   if( background )
		BlotScaledImage( GetDisplayImage( renderer ), background );
//   BlotImage( GetDisplayImage( renderer ), background, 0, 0 );
	//ClearDisplay( renderer );
  // ProcessMotion();
	//printf( WIDE("... %d\n"), rot >> 16 );
	//lprintf( WIDE("image") );
	//for( n = 0; n < 50; n++ )
	{
		//BlotImage( GetDisplayImage( renderer ), sprite[n]->image, sprite[n]->curx, sprite[n]->cury );
	}
	lprintf( WIDE("done") );
}

void CPROC Tick( PTRSZVAL psv )
{
	int n;
	//for( n = 0; n < 16; n++ )
	{
		rot = rot + 0x1000000;
		//ProcessMotion();
      Log( WIDE("BeginDraw") );
		//DrawProc( 0, render );
		Log( WIDE("EndDraw") );
		UpdateDisplay( render );
      Log( WIDE("EndUpdate") );
	}
}

SaneWinMain( argc, argv )
{
	Image image = LoadImageFile( WIDE("images/firestar2.png") );
	background = LoadImageFile( WIDE("images/sky.jpg") );
   SetSystemLoggingLevel( LOG_NOISE + 1000 );
	SystemLogTime( SYSLOG_TIME_CPU| SYSLOG_TIME_DELTA );
	SetBlotMethod( BLOT_MMX );
	if( !image )
		return 0;
	{
		render = OpenDisplaySizedAt( 0, 1024, 768, 0, 0 );
		if( !render )
			return 0;
		{
			int n;
         /*
			do_hline( image, 0, 0, image->width-1, BASE_COLOR_WHITE );
			do_hline( image, image->height - 1, 0, image->width-1, BASE_COLOR_WHITE );
			do_vline( image, 0, 0, image->height-1, BASE_COLOR_WHITE );
			do_vline( image, image->width - 1, 0, image->height-1, BASE_COLOR_WHITE );
			*/
         PSPRITE_METHOD psm = EnableSpriteMethod( render, SpriteDrawProc, 0 );
         for( n = 0; n < MAX_SPRITES; n++ )
			{
				sprite[n].sprite = MakeSpriteImage( image );
				if( !sprite[n].sprite )
					return 0;
            sprite[n].sprite->pSpriteMethod = psm;
				sprite[n].sprite->curx = rand() * 500 / RAND_MAX;
				sprite[n].sprite->cury = rand() * 500 / RAND_MAX;
				sprite[n].dx = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].dy = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].sprite->hotx = image->width / 2;
				sprite[n].sprite->hoty = image->height / 2;
			}
			lprintf( WIDE("%d, %d\n"), image->width, image->height );
			rot = rot + 0x10000 * 19;
			if( rot == 0x1000000 )
				rot = 0;
			SetRedrawHandler( render, DrawProc, 0 );
			UpdateDisplay( render );
			AddTimer( 33, Tick, 0 );
			while( 1 )
				WakeableSleep( 1000 );
		}
	}
   return 0;

}
EndSaneWinMain()
