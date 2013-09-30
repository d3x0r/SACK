#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h>
#include <timers.h>
#include <render.h>
#include <image.h>

#define MAX_SPRITES 20

struct {
	PSPRITE sprite;
	int dx, dy;
	int alpha;
   int ttl;
} sprite[MAX_SPRITES];
PRENDERER render;
int rot;

Image background;

void ProcessMotion( void )
{
	int n;
	for( n = 0; n < MAX_SPRITES; n++ )
	{
		if( sprite[n].ttl )
		{
			sprite[n].ttl--;
			sprite[n].sprite->curx += sprite[n].dx;
			sprite[n].sprite->cury += sprite[n].dy;
			if( sprite[n].sprite->curx < 50 )
				sprite[n].dx = ( 15 * rand() ) / RAND_MAX + 1;
			if( sprite[n].sprite->cury < 50 )
				sprite[n].dy = ( 15 * rand() ) / RAND_MAX + 1;
			if( sprite[n].sprite->curx > (1024 - 50) )
				sprite[n].dx = -(( 15 * rand() ) / RAND_MAX + 1);
			if( sprite[n].sprite->cury > (767-50) )
				sprite[n].dy = -(( 15 * rand() ) / RAND_MAX + 1);
		}
	}
}

void CPROC SpriteDrawProc( PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h )
{
   int n;
	for( n = 0; n < MAX_SPRITES; n++ )
	{
		if( sprite[n].ttl )
		{
         lprintf( "------------- (BEGIN SPRITE)" );
			rotate_scaled_sprite( GetDisplayImage( renderer ), sprite[n].sprite, rot, 0x10000, 0x10000 );
         lprintf( "------------- (BEGIN BLOT)" );
			BlotImage( GetDisplayImage( renderer ), sprite[n].sprite->image, sprite[n].sprite->curx, sprite[n].sprite->cury );
         lprintf( "------------- (DONE)" );
		}
	}

}



void CPROC DrawProc( PTRSZVAL psv, PRENDERER renderer )
{
	int n;
   if( background )
   BlotScaledImage( GetDisplayImage( renderer ), background );
	//ClearDisplay( renderer );
  // ProcessMotion();
	//printf( "... %d\n", rot >> 16 );
	//lprintf( "image" );
	//for( n = 0; n < MAX_SPRITES; n++ )
	{
		//BlotImage( GetDisplayImage( renderer ), sprite[n]->image, sprite[n]->curx, sprite[n]->cury );
	}
	lprintf( "done" );
}

/*
void CPROC DrawProc( PTRSZVAL psv, PRENDERER renderer )
{
   int n;
	ClearDisplay( renderer );
   ProcessMotion();
	//printf( "... %d\n", rot >> 16 );
	//lprintf( "image" );
	//for( n = 0; n < MAX_SPRITES; n++ )
	{
		//BlotImage( GetDisplayImage( renderer ), sprite[n]->image, sprite[n]->curx, sprite[n]->cury );
	}
   lprintf( "sprite" );
	for( n = 0; n < MAX_SPRITES; n++ )
	{
      if( sprite[n].ttl )
			rotate_scaled_sprite( GetDisplayImage( renderer ), sprite[n].sprite, rot, 0x10000 );
	}
	lprintf( "done" );
}
*/

void CPROC MouseProc( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	int n;
   static int _b;
	// generate sprites at mouse click...
	if( b & MK_LBUTTON && !( _b & MK_LBUTTON ) )
	{
		for( n = 0; n < MAX_SPRITES; n++ )
		{
			sprite[n].ttl = (rand() * 20) / RAND_MAX + 20;
			sprite[n].sprite->curx = x;
			sprite[n].sprite->cury = y;
			sprite[n].dx = 15 - ( rand() * 31 / RAND_MAX );
			sprite[n].dy = 15 - ( rand() * 31 / RAND_MAX );
		}
	}
	_b = b;
}


void CPROC Tick( PTRSZVAL psv )
{
	int n;
	//for( n = 0; n < 16; n++ )
	{
		rot = rot + 0x10000000;
      Log( "BeginDraw (tick, this is idle time)" );
		ProcessMotion();
		//DrawProc( 0, render );
		//Log( "EndDraw" );
		UpdateDisplay( render );
      Log( "EndUpdate" );
	}
}


SaneWinMain( argc, argv )
{
	Image image = LoadImageFile( "images/firestar2.png" );
   background = LoadImageFile( "images/sky.jpg" );
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
         PSPRITE_METHOD psm = EnableSpriteMethod( render, SpriteDrawProc, 0 );
         /*
			do_hline( image, 0, 0, image->width-1, BASE_COLOR_WHITE );
			do_hline( image, image->height - 1, 0, image->width-1, BASE_COLOR_WHITE );
			do_vline( image, 0, 0, image->height-1, BASE_COLOR_WHITE );
			do_vline( image, image->width - 1, 0, image->height-1, BASE_COLOR_WHITE );
         */
         for( n = 0; n < MAX_SPRITES; n++ )
			{
				sprite[n].sprite = MakeSpriteImage( image );
				if( !sprite[n].sprite )
					return 0;
            sprite[n].sprite->pSpriteMethod = psm;
            sprite[n].ttl = (rand() * 300) / RAND_MAX + 50;
				sprite[n].sprite->curx = rand() * 500 / RAND_MAX;
				sprite[n].sprite->cury = rand() * 500 / RAND_MAX;
				sprite[n].dx = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].dy = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].sprite->hotx = image->width / 2;
				sprite[n].sprite->hoty = image->height / 2;
			}
			printf( "%d, %d\n", image->width, image->height );
			rot = rot + 0x10000 * 19;
			if( rot == 0x1000000 )
				rot = 0;
			SetRedrawHandler( render, DrawProc, 0 );
         SetMouseHandler( render, MouseProc, 0 );
			UpdateDisplay( render );
			AddTimer( 33, Tick, 0 );
			while( 1 )
				WakeableSleep( 1000 );
		}
	}
   return 0;

}
EndSaneWinMain()

