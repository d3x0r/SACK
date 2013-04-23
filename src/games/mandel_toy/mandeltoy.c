//#define USE_RENDER_INTERFACE pri
//#define USE_IMAGE_INTERFACE pii

#include <render.h>

PRENDER_INTERFACE pri;
PIMAGE_INTERFACE pii;

double x02 = 0.0;
double y02 = 0.0;

double xorg = 0.0;
double yorg = 0.0;

double scale = 1.0;

int mx, my;

PRELOAD( x )
{
	pri = GetDisplayInterface();
   pii = GetImageInterface();
}

int ii1_max_iter;
int _ii1_max_iter;
int ii2_max_iter;
int _ii2_max_iter;

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

int DrawInfinite( Image surface, double a, double b, double da, double db, int ofsx, int ofsy )
{
	double x, x0, y, y0;
	int iteration = 0;
	int  max_iteration = 255;
	double xtemp;

	x = x02;
	y = y02;
	x0 = a;
   y0 = b;

	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;

		x = xtemp;

		iteration = iteration + 1;
		switch( iteration & 3 )
		{
		case 0:
		plot( surface, (x-xorg)/da+ofsx, (y-yorg)/db+ofsy,
			  Color(255-iteration,255-iteration,0)

			 );
		break;
		case 1:
		plot( surface, (x-xorg)/da+ofsx, (y-yorg)/db+ofsy,
			  Color(255-iteration,0, 255-iteration)

			 );
         break;
		case 2:
		plot( surface, (x-xorg)/da+ofsx, (y-yorg)/db+ofsy,
			  Color(0, 255-iteration,255-iteration)

			 );
         break;
		case 3:
		plot( surface, (x-xorg)/da+ofsx, (y-yorg)/db+ofsy,
			  Color(255-iteration,0,0)

			 );
		break;
		}
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
	int  max_iteration = 25;
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
	dx = scale*4.0 / (double)surface->width;
	dy = scale*4.0 / (double)surface->height;

   _ii1_max_iter = 0;
   _ii2_max_iter = 0;
	for( a = 0; a < surface->width; a++ )
	{
		for( b = 0; b < surface->height; b++ )
		{
			double da, db;
         int r, g, bl  = 0;
         da = ( a - ofsx ) * dx + xorg;
			db = ( b - ofsy ) * dy + yorg;
			//if( (r = IsInfinite( da, db )) == -1 )
				r = 0;
			if( (g = IsInfinite2( da, db )) == -1 )
				g = 0;
			plot( surface, a, b, Color(r,g,bl) );
		}
	}

	{
		double da, db;
		int r, g, bl  = 0;
		da = ( mx - ofsx ) * dx + xorg;
		db = ( my - ofsy ) * dy + yorg;
		DrawInfinite( surface, da, db, dx, dy, ofsx, ofsy );
	}

   ii1_max_iter = _ii1_max_iter;
   ii2_max_iter = _ii1_max_iter;
	if( output )
	{
		// actually put the changes on the screen....
      PutString( surface, 10, 10, BASE_COLOR_WHITE, BASE_COLOR_BLACK, "Alt+TAB and select application,\nThen Press ALT+F4 to Exit. " );
		UpdateDisplay( r );
	}
}

void CPROC MyRedrawCallback( PTRSZVAL psv, PRENDERER r )
{
   Render( r, FALSE );
}


int CPROC MyMouseCallback( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	if( b & MK_SCROLL_DOWN )
	{
		xorg += ( x - ofsx ) * (dx);
		yorg += ( y - ofsy ) * (dy);

      scale = scale - (scale*0.1);


		SetMousePosition( (PRENDERER)psv,ofsx, ofsy );
		x = ofsx;
      y = ofsy;
	}

	if( b & MK_SCROLL_UP )
	{
		xorg += ( x - ofsx ) * (dx);
		yorg += ( y - ofsy ) * (dy);

      scale = scale + (scale*0.1);


		SetMousePosition( (PRENDERER)psv,ofsx, ofsy );
		x = ofsx;
      y = ofsy;
	}

   //if( !b )
	//	ii1_max_iter = 0;
   //if( !b )
	//	ii2_max_iter = 0;

	//x02 = ( x - ofsx )* dx;
	//y02 = ( y - ofsy ) * dy;
	mx = x;
	my = y;


   Render( (PRENDERER)psv, TRUE );

   return 0;
}

int main( void )
{
	PRENDERER render = OpenDisplay( 0 );
	SetRedrawHandler( render, MyRedrawCallback, 0 );
   SetMouseHandler( render, MyMouseCallback, (PTRSZVAL)render );
	UpdateDisplay( render );
	while( 1 )
      WakeableSleep( 10000 );

   return 0;
}


int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, LPSTR cmd, int show )
{
	return main();
}
