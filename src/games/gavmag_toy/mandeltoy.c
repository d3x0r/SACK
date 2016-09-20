//#define USE_RENDER_INTERFACE pri
//#define USE_IMAGE_INTERFACE pii

#include <render.h>
#include <math.h>

PRENDER_INTERFACE pri;
PIMAGE_INTERFACE pii;

double x02 = 1.0;
double y02 = 0.0;

PRELOAD( x )
{
//	pri = GetDisplayInterface();
//   pii = GetImageInterface();
}

int ii1_max_iter;
int _ii1_max_iter;
int ii2_max_iter;
int _ii2_max_iter;

int mx, my; // current mouse (last mouse)

int IsInfinite( double a, double b )
{
	double x, x0, y, y0;
	int iteration = 0;
	int  max_iteration = 100;
	double xtemp;

	x = a;
	y = b;
	x0 = x02;
   y0 = y02;

	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;

		x = xtemp;

		iteration = iteration + 1;
	}
	if( iteration < max_iteration && iteration > _ii1_max_iter )
		_ii1_max_iter = iteration;

   if( ii1_max_iter )
		return (iteration < max_iteration)?(iteration * 255 / ii1_max_iter):-1;
	else
		return (iteration < max_iteration)?(iteration * 255 / max_iteration):-1;

}

int IsInfinite2( double a, double b )
{
	double x, x0, y, y0;
	int iteration = 0;
	int  max_iteration = 100;
	double xtemp;

	x = x02;
	y = y02;
	x0 = a;
   y0 = b;

	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;

		x = xtemp;

		iteration = iteration + 1;
	}

	if( iteration < max_iteration && iteration > _ii2_max_iter )
		_ii2_max_iter = iteration;

   if( ii2_max_iter )
		return (iteration < max_iteration)?(iteration * 255 / ii2_max_iter):-1;
	else
		return (iteration < max_iteration)?(iteration * 255 / max_iteration):-1;
	//return iteration < max_iteration?(iteration * 255 / max_iteration):-1;
}

	int ofsx, ofsy;
   double dx, dy;

void Render( PRENDERER r, int output )
{
	int a,b;
	Image surface = GetDisplayImage( r );


	ofsx = (surface->width/2);
	ofsy = (surface->height/2);
	dx = 4.0 / (double)surface->width;
	dy = 4.0 / (double)surface->height;

   _ii1_max_iter = 0;
   _ii2_max_iter = 0;


	for( a = 0; a < surface->width; a++ )
	{
		for( b = 0; b < surface->height; b++ )
		{
         double real_x = a - ofsx;
			double real_y = b - ofsy;

			double da, db;
			int r, g, bl  = 0;

			double dist = (real_x!= 0 || real_y != 0)? sqrt(real_x*real_x + real_y*real_y):1;
         double m;

			double grav;
			double val;
#if close_in
         double c1 = (3.54598e+9);// + (x02*10000000));
			double c2 = (1.4796e+7);//  + (y02*100000));
#endif

#define known_constants 1
//#define first_harmonic_center 1

#if first_harmonic_center
			double c1 = 4202362101.323539;
         double c2 = 10258554.101331;
#endif

#if first_harmonic_browsing
         double c1 = (221812290.44687984787069008657358)*(10*x02);// + (x02*10000000));
			double c2 = (1812508.9578073492607741750548493)*(10*y02);//  + (y02*100000));
#endif
#if zoom_level_x
			double c1 = 60033744304.621979;
			double c2 = 146550772.876157;
#endif
#if zoom_level_x2
			double c1 = 30076878553.9987;
			double c2 = 73421953.3340178;
#endif
#if zoom_level_far_away
         //6138138.480408,130661.108790
			double c1 = 7570370.792503;
			double c2 = 143047.941734;
#endif

#if known_constants
			double vol1 = 268082585672699.63737465178886978; // cm3
			double density1 = 5.52; // g-cm
			double earth_oersted = 0.7;

			double G = 6.67428e-11; // m3/kg-s2
			// scaled by mass of earth 398600.4418 km3/s2
			// S = 1.98892 +/- .00025)e30 kg
			// S = 4 pi^2 x 1au3  / G x 1yr^2     // heh - 1-(yr^2) that's pretty arbitrary and a 1 implied, just to satisfy units
			//               Earth to the Sun was about 1/28,700. Later he determined that this value was based upon a faulty
			//  value for the solar parallax, which was used to estimate the distance to the Sun (1 AU). He revised his result
			//  to obtain a ratio of 1/169,282 in the third edition of the Principia. The current value for the solar parallax
			//  is smaller still, giving the correct mass ratio of 1/332,946.[5] - assumes a density.




         // A = astronomical unit (1)D mean solar day S solar mass
			// guass had a constant called k = 0.01720209895 A^3/2 /  (1)D * S^1/2
         //   If instead of mean solar day we use the sidereal year as our time unit, the value of k is very close to 2p (k = 6.28315).



         double earth_mass = vol1 * density1;
			double earth_field = vol1 * earth_oersted;
         double earth_g = G * earth_mass;
         //double earth_mass =

			double c1 = 1; // force in dyne
			double c2 = 6.67e-8; // force in dyne

         double zoom = 100e+24;
#endif

#if coral_castle_base
         //6138138.480408,130661.108790
			double c1 = 6105195;
			double c2 = 7129;
#endif

         //c1 = c1 + 41947917.3053104 * 50*x02;
         //c2 = c2 + 101155.06159597 * 50*x02;

			c1 *= zoom * 1;//( 100.001 * x02 );
         c2 *= zoom * 1;//( 100.001 * y02 );


         //8.80464e+13,-2.07748e+12 0.248299,-1.40408
         // okay so at very big distances...

			//c1 *= 100000000*x02;
			//c2 *= 100000000*y02;

         m =    c1  / (dist*dist*dist);
			grav = c2 / (dist*dist);

         if( real_x == 50 && real_y == 0 )
         lprintf( "x,y = %lf,%lf %lf,%lf %g %g", c1, c2, x02, y02, m, grav );

// outside is red with 0 green


//			x,y = 3.54598e+09/031.9728 ,1.4796e+06/0.816327 0.319728,0.0816327 28367.8 591.84
//         x,y = 3.54598e+09,1.4796e+06 1.9932,1.83401 28367.8 591.84

			//3.54598e+09,1.4796e+07
			//0.295918,0.206803

			//m = m / (100*x02);
         //grav = grav / (10*y02);

			val = m-grav ;
#if 0
			if( grav > 255 )
				grav = 255;
			if( m > 255 )
				m = 255;
#endif
			//if( a == mx && b == my )
         //   lprintf( "point val grav,m %g,%g at %g,%g   is %g,%g", grav, m, real_x,real_y,(sin(m/5.0) * 125) + 125, -((cos(grav/10.0) * 125) + 125) );
			//r = ((int)m) %256;
			//g = ((int)grav) %256;

         // at 0, this is 0
			r = -(cos(m) * 125) + 126;

         // at 0 this is 0
			g = -((cos(grav) * 125) + 126);
         //if( val < 128 val =
         //bl = (int)val%256;

			plot( surface, a, b, Color(r,g,bl) );
		}
	}
   ii1_max_iter = _ii1_max_iter;
   ii2_max_iter = _ii1_max_iter;
	//if( output )
	{
		// actually put the changes on the screen....
      PutString( surface, 10, 10, BASE_COLOR_WHITE, BASE_COLOR_BLACK, "Alt+TAB and select application,\nThen Press ALT+F4 to Exit. " );
		UpdateDisplay( r );
	}
}

void CPROC MyRedrawCallback( uintptr_t psv, PRENDERER r )
{
   Render( r, FALSE );
}


int CPROC MyMouseCallback( uintptr_t psv, int32_t x, int32_t y, uint32_t b )
{
	if( b & MK_SCROLL_DOWN )
	{
 //     scale = 0;
	}

   //if( !b )
	//	ii1_max_iter = 0;
   //if( !b )
	//	ii2_max_iter = 0;
	mx = x;
   my = y;

	x02 = ( x - ofsx )* dx;
	y02 = ( y - ofsy ) * dy;
   Redraw( (PRENDERER)psv );

   return 0;
}

int main( void )
{
	PRENDERER render = OpenDisplay( 0 );
	SetRedrawHandler( render, MyRedrawCallback, 0 );
   SetMouseHandler( render, MyMouseCallback, (uintptr_t)render );
	UpdateDisplay( render );
	while( 1 )
      WakeableSleep( 10000 );

   return 0;
}


int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, LPSTR cmd, int show )
{
	return main();
}



