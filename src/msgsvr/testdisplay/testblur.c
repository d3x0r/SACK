#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi

#include <stdhdrs.h>
#include <stdlib.h>
#include <time.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <render.h>

#define NUM_PICS 3
#define NUM_BLURS 20
#define NUM_REELS 3

#define REEL_STEPX 106
#define REEL_OFSX 167
#define REEL_OFSY 114

typedef struct global_tag {
	struct {
		uint32_t bSpinning : 1;
	} flags;
	uint32_t bReelSpinning[NUM_REELS];
	int32_t ofs;
	uint32_t nReels;
   Image background;
	Image strip;
	Image images[10];
   Image blurs[NUM_BLURS];
   Image reel[5][NUM_PICS + 2];
	PRENDERER render;
	Image surface;
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;

GLOBAL g;

void Blur( Image dst, Image src[] )
{
   int y, n, x, row;
	for( x = 0; x < 96; x++ )
	{
		uint32_t idx;
      uint32_t divisor = 1;
		uint8_t rvals[96];
		uint8_t gvals[96];
		uint8_t bvals[96];
      uint8_t gain[96];
		uint32_t red = 0
	, green = 0
	, blue = 0, img = 0;
		idx = 0;
		for( row = 0; row < 96; row++ )
		{
         CDATA pixel;
			pixel = getpixel( src[0], x, row );
         ( rvals[row] = RedVal( pixel ) );
			( bvals[row] = BlueVal( pixel ) );
			( gvals[row] = GreenVal( pixel ) );
			gain[row] = ( (gvals[row] + bvals[row] + rvals[row]) / 128 ) + 1;
         divisor += gain[row];
         red += ( rvals[row] * gain[row] );
			blue += ( bvals[row] * gain[row] );
			green += ( gvals[row] * gain[row] );
		}
		for( img = 1; img <= NUM_PICS; img++ )
		{
			for( y = 0; y < 96; y++ )
			{
				CDATA pixel;
				pixel = getpixel( src[img], x, y);
            red -= rvals[idx] * gain[idx];
            blue -= bvals[idx] * gain[idx];
				green -= gvals[idx] * gain[idx];
            divisor -= gain[idx];
			   ( rvals[idx] = RedVal( pixel ) );
				( bvals[idx] = BlueVal( pixel ) );
				( gvals[idx] = GreenVal( pixel ) );
				gain[idx] = ( ( gvals[idx] + bvals[idx] + rvals[idx]) / 128 )  + 1;
            divisor += gain[idx];
			   red += ( rvals[idx] * gain[idx] );
				blue += ( bvals[idx] * gain[idx] );
				green += ( gvals[idx] * gain[idx] );
				idx++;
				if( idx >= 96 )
               idx = 0;
				plot( dst, x, y + (img-1)*96, Color( red/divisor, green/divisor, blue/divisor ) );
			}
		}
	}
}

void DrawReel( int nReel )
{
	int n;
	int i;
   n = nReel;
	{
		IMAGE_RECTANGLE rect;
		rect.x = REEL_OFSX + REEL_STEPX * n;
		rect.y = REEL_OFSY;
		rect.width = 96;
		rect.height = 288;
		for( i = 0; i < NUM_PICS+2; i++ )
		{
			BlotImage( g.surface, g.reel[n][i], REEL_OFSX + REEL_STEPX*n, (REEL_OFSY-96) + g.ofs + i*96 );
		}
	}
}

void DrawReels( void )
{
	int n;
	for( n = 0; n < NUM_REELS; n++ )
      DrawReel( n );
   UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(g.nReels-1) + 96, 288 );
}

void DrawSpinningReels( void )
{
   int n;
	for( n = 0; n < g.nReels; n++ )
	{
      if( g.bReelSpinning[n] )
			BlotImageSizedTo( g.surface, g.blurs[rand()%NUM_BLURS], REEL_OFSX + REEL_STEPX * n, REEL_OFSY, 0, 0, 96, 288 );
   }
   UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(g.nReels-1) + 96, 288 );
}

int CPROC MouseMethod( uintptr_t psv, int32_t x, int32_t y, uint32_t b )
{
	if( !g.flags.bSpinning )
	{
		if( b )
		{
			g.ofs = -7;
			DrawReels();
		}
		else
		{
			if( g.ofs == -7 )
			{
				int n;
				for( n = 0; n < NUM_REELS; n++ )
				{
               g.bReelSpinning[n] = 1;
				}
				g.flags.bSpinning = 1;
				{
					int n, i;
					for( n = 0; n < g.nReels; n++ )
						for( i = 0; i < NUM_PICS+2; i++ )
						{
							g.reel[n][i] = g.images[rand()%10];
						}
				}
			}
		}
	}
	else
	{
		if( b )
		{
			int reel = (x - REEL_OFSX) / REEL_STEPX;
         if( reel >= 0 && reel < NUM_REELS )
				if( g.bReelSpinning[reel] )
				{
					g.bReelSpinning[reel] = 0;

					g.ofs = -32;
					DrawReel(reel);
					UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(reel), 288 );
					WakeableSleep( 100 );
					g.ofs = 7;
					DrawReel(reel);
					UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(reel), 288 );
					WakeableSleep( 100 );
					g.ofs = 0;
					DrawReel(reel);
					UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(reel), 288 );
					{
						int n;
						for( n = 0; n < NUM_REELS; n++ )
							if( g.bReelSpinning[n] )
								break;
						if( n == NUM_REELS )
                     g.flags.bSpinning = 0;
					}
				}
		}

	}
   return 1;
}

uintptr_t CPROC ReadInput( PTHREAD thread )
{
   char buf[256];
	while( fgets( buf, 256, stdin ) || buf[0] == '\x1b' )
	{
		if( !g.flags.bSpinning )
		{
			MouseMethod( 0, 0, 0, 1 );
			MouseMethod( 0, 0, 0, 0 );
		}
		else
		{
			int n;
			for( n = 0; n < NUM_REELS; n++ )
			{
				if( g.bReelSpinning[n] )
				{
					MouseMethod( 0, REEL_OFSX + n * REEL_STEPX, 0, 1 );
					MouseMethod( 0, REEL_OFSX + n * REEL_STEPX, 0, 0 );
					break;
				}
			}
		}
	}
	exit(0);
   return 0;
}



int main( void )
{
	uint32_t width, height;
	srand( time( NULL ) );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	SetSystemLog( SYSLOG_FILE, stdout );
	GetDisplaySize( &width, &height );;
   g.background = LoadImageFile( WIDE("background.jpg") );
	g.strip = LoadImageFile( WIDE("slot_strip.jpg") );
	{
		int n;
		for( n = 0; n < 10; n++ )
		{
         g.images[n] = MakeSubImage( g.strip, 96 * n, 0, 96, 96 );
		}
	}
	g.render = OpenDisplaySizedAt( 0, width, height, 0, 0 );
	SetMouseHandler( g.render, MouseMethod, 0 );
	ThreadTo( ReadInput, 0 );
	g.surface = GetDisplayImage( g.render );
   BlotImage( g.surface, g.background, 0, 0 );
	//BlotImage( g.surface, g.strip, 0, 0 );
   //UpdateDisplay( g.render );
	g.nReels = 3;
	{
      int n, i;
		for( n = 0; n < NUM_BLURS; n++ )
		{
         g.blurs[n] = MakeImageFile( 96, (NUM_PICS) * 96 );
			for( i = 0; i < NUM_PICS+2; i++ )
			{
				g.reel[0][i] = g.images[rand()%10];
			}
			Blur( g.blurs[n], g.reel[0] );
			//BlotImage( g.surface, g.blurs[n], n * 96, 0 );
		}
	}
	{
		int n, i;
		for( n = 0; n < g.nReels; n++ )
			for( i = 0; i < NUM_PICS+2; i++ )
			{
				g.reel[n][i] = g.images[rand()%10];
			}
	}

	DrawReels();
   UpdateDisplay(g.render);
	{
		uint32_t start = GetTickCount();
      g.ofs = 0;
		while( 1 )
		{
			if( g.flags.bSpinning )
			{
				DrawSpinningReels();
			}
#ifndef __ARM__
         // scale to approx unit speeds..
			WakeableSleep( 250 );
#endif
		}
	}
	CloseDisplay( g.render );
	UnmakeImageFile( g.strip );
   return 0;
}

