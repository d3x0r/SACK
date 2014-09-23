
#include "intershell_local.h"
#include <image.h>
//#include "intershell_export.h"
//#include "intershell_registry.h"

// handles creation of sprite type objects and effects.  The rendering of sprites will be magic.

// method 1>
//    save the current spot of the sprite, draw sprite, then restore sprite location
//    - when should this be done, it's a non deterministic time ATM.
//
// method 2>
//    hook into the drawing sequence, just before outputting the page's image
//      - copy the current image
//      - draw sprites on the image
//      - allow the actual output
//      - Restore original image.


//
//  some problems with current display layered system.
//    - video surface is required for a frame to work.
//    - without a video surface, there is no image for a frame to render on.
//       - maybe a hack could be done real quick to enable memory renderer.
//    - frames know intimately the display library.
//    - the display library could in essense be modified to support sprites...
//      (odd note, hardware doesn't serve multiple planes, which have an additive property for
//       parallax type games, (multiple layer side scroller?)
//    - a background layer, the control layer, the sprite layer, and a final composite layer.
//    - this all requires 4 moves to compute the final output, resulting in 1/4 the frame rate ideal.
//    -
//


#define DISABLE_SPRITES
#define USE_SPRITES 0

struct {
	PSPRITE sprite;
	int dx, dy;
	int alpha;
	int ttl;
} sprite[50];
static struct {
	Image image;
	PSPRITE_METHOD psm;
	PRENDERER renderer;
} l;
static int rot;
//static PRENDERER render;

void ProcessMotion( void )
{
	int n;
	for( n = 0; n < USE_SPRITES; n++ )
	{
		if( sprite[n].ttl )
		{
			Image surface = GetDisplayImage( l.renderer );
			sprite[n].ttl--;
			sprite[n].sprite->curx += sprite[n].dx;
			sprite[n].sprite->cury += sprite[n].dy;
			if( sprite[n].sprite->curx < 50 )
				sprite[n].dx = ( 15 * rand() ) / RAND_MAX + 1;
			if( sprite[n].sprite->cury < 50 )
				sprite[n].dy = ( 15 * rand() ) / RAND_MAX + 1;
			if( sprite[n].sprite->curx > (surface->width - 50) )
				sprite[n].dx = -(( 15 * rand() ) / RAND_MAX + 1);
			if( sprite[n].sprite->cury > (surface->height-50) )
				sprite[n].dy = -(( 15 * rand() ) / RAND_MAX + 1);
		}
	}
}

void CPROC SpriteDrawProc( PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h )
{
	int had_one = 0;
	int n;
	for( n = 0; n < USE_SPRITES; n++ )
	{
		if( sprite[n].ttl )
		{
			rotate_scaled_sprite( GetDisplayImage( renderer ), sprite[n].sprite, rot, 0x10000, 0x10000 );
			had_one = 1;
		}
	}
	if( !had_one )
		l.renderer = NULL;

}



void GenerateSprites( PRENDERER renderer, int x, int y )
{
	int n;
#ifdef DISABLE_SPRITES
	return;
#endif
	{
		for( n = 0; n < USE_SPRITES; n++ )
		{
			sprite[n].ttl = (rand() * 20) / RAND_MAX + 8;
			if( !sprite[n].sprite )
			{
				if( !l.image )
					l.image = LoadImageFile( WIDE( "images/firestar2.png" ) );

				if( !l.psm )
					l.psm = EnableSpriteMethod( renderer, SpriteDrawProc, 0 );
				sprite[n].sprite = MakeSpriteImage( l.image );
				if( !sprite[n].sprite )
					return;
				sprite[n].sprite->pSpriteMethod = l.psm;//psm;
				sprite[n].ttl = (rand() * 300) / RAND_MAX + 50;
				sprite[n].sprite->curx = rand() * 500 / RAND_MAX;
				sprite[n].sprite->cury = rand() * 500 / RAND_MAX;
				sprite[n].dx = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].dy = 15 - ( rand() * 31 / RAND_MAX );
				sprite[n].sprite->hotx = l.image->width / 2;
				sprite[n].sprite->hoty = l.image->height / 2;			
			}
			sprite[n].sprite->curx = x;
			sprite[n].sprite->cury = y;
			sprite[n].dx = 15 - ( rand() * 31 / RAND_MAX );
			sprite[n].dy = 15 - ( rand() * 31 / RAND_MAX );
		}
		l.renderer = renderer;
	}
}


void CPROC Tick( PTRSZVAL psv )
{
	if( !l.renderer )
		return;
	//for( n = 0; n < 16; n++ )
	{
		rot = rot + 0x1000;
		if( rot == 0x1000000 )
			rot = 0;
#ifdef DEBUG_TIMING
		Log( "BeginDraw" );
#endif
		ProcessMotion();
		//DrawProc( 0, render );
#ifdef DEBUG_TIMING
		Log( "EndDraw" );
#endif
		UpdateDisplay( l.renderer );
#ifdef DEBUG_TIMING
		Log( "EndUpdate" );
#endif
	}
}


int InitSpriteEngine( void )
	// create some sprite objects here... should probably hook into the configuration file also...
{
#ifdef DISABLE_SPRITES
	return 0;
#endif
	//Image image = LoadImageFile( "images/firestar2.png" );
	//render = _render;
	//SetBlotMethod( BLOT_MMX );
	{
		{
			//int n;
         /*
			do_hline( image, 0, 0, image->width-1, BASE_COLOR_WHITE );
			do_hline( image, image->height - 1, 0, image->width-1, BASE_COLOR_WHITE );
			do_vline( image, 0, 0, image->height-1, BASE_COLOR_WHITE );
			do_vline( image, image->width - 1, 0, image->height-1, BASE_COLOR_WHITE );
         */
//			printf( "%d, %d\n", image->width, image->height );
			rot = rot + 0x10000 * 19;
			if( rot == 0x1000000 )
				rot = 0;
		}
	}
	return 1;

}

static void OnFinishInit( WIDE("Sprite") )( PCanvasData pc_canvas )
{
#ifdef DISABLE_SPRITES
	return;
#else
	InitSpriteEngine();
	AddTimer( 33, Tick, 0 );
#endif
}
