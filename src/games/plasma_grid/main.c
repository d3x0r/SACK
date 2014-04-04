#define MAKE_RCOORD_SINGLE

#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#include <render.h>
#include <image.h>

struct plasma_local
{
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	struct plasma_state *plasma;
	PRENDERER render;
} l;

void CPROC DrawPlasma( PTRSZVAL psv, PRENDERER render )
{
	Image surface = GetDisplayImage( render );
	RCOORD *data = PlasmaGetSurface( l.plasma );
	int w;
	int h;
	RCOORD min = 999999999;
	RCOORD max = 0;
	for( w = 0; w < surface->width; w++ )
	{
		for( h = 0; h < surface->height; h++ )
		{
			if( data[ h * surface->width + w ] > max )
				max = data[ h * surface->width + w ];
			if( data[ h * surface->width + w ] < min )
				min = data[ h * surface->width + w ];
			plot( surface, w, h, ColorAverage( BASE_COLOR_BLACK,
														 BASE_COLOR_WHITE, 1024 - data[ h * surface->width + w ] * 1024, 2024 ) );
			//lprintf( "%d,%d  %g", w, h, data[ h * surface->width + w ] );
		}
	}
	lprintf( "Result is %g,%g", min, max );
	UpdateDisplay( render );
}

SaneWinMain( argc, argv )
{

	RCOORD coords[4];
	l.pii = GetImageInterface();
	l.pdi = GetDisplayInterface();

	l.render = OpenDisplaySized( 0, 1024, 768 );
	SetRedrawHandler( l.render, DrawPlasma, 0 );

	coords[0] = 1.0;
	coords[1] = 0.0;
	coords[2] = 0.0;
	coords[3] = 1.0;
	l.plasma = PlasmaCreate( coords, 1024, 768 );
	RestoreDisplay( l.render );

	while( 1 )
		WakeableSleep( 100000 );
}
EndSaneWinMain()
