
// Source mostly from

#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

// define local instance.
#define TERRAIN_MAIN_SOURCE  
#include <vectlib.h>
#include <render.h>
#include <render3d.h>

#ifndef SOMETHING_GOES_HERE
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

static struct local
{
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;
} l;

void CPROC draw( uintptr_t psv, PRENDERER r )
{

}

SaneWinMain( argc, argv )
{
	int n;
	RCOORD d = 1000;
	RCOORD p = 0;
	RCOORD v = 0;
	RCOORD a = 0;
	PRENDERER r;
	InvokeDeadstart();
	l.pri = GetDisplayInterface();
	l.pii = GetImageInterface();
	r = OpenDisplaySizedAt( 0, 2000, 2000, 0, 0 );
	//SetRedrawHandler( r, draw, 0 );
	UpdateDisplay(r );
	for( n = 0; n < 2000; n++ )
	{
		int opt = 0;
		// a += 1/d; 
		if( opt == 0 )
			a = (  1 / ( d - p) ) ;
		else
			a = (  1 / ( ( d - p) * ( d - p) ) ) * 600;
		v += a;
		p += v;


		lprintf( "p = %g d = %g 1/d^2 = %g v = %g a = %g "
			, p, d - p, 1/((d-p)*(d-p)), v, a );
		if( opt == 0 )
		{
		plot( GetDisplayImage( r ), n, p , BASE_COLOR_WHITE );
		plot( GetDisplayImage( r ), n, v * 100, BASE_COLOR_RED );
		plot( GetDisplayImage( r ), n, a* 2000, BASE_COLOR_BLUE );
		}
		else
		{
		plot( GetDisplayImage( r ), n, p /1.5, BASE_COLOR_WHITE );
		plot( GetDisplayImage( r ), n, v * 200, BASE_COLOR_RED );
		plot( GetDisplayImage( r ), n, a* 200000, BASE_COLOR_BLUE );
		}
	}
	Redraw( r );
	while( 1 ) WakeableSleep( 2505000 );

   return 0;
}
EndSaneWinMain()
