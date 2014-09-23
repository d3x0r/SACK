#define USE_RENDER_INTERFACE pri
#define USE_IMAGE_INTERFACE pii
#define USE_RENDER3D_INTERFACE pr3i

#include <render.h>
#include <render3d.h>
#include <GL/GL.h>

PRENDER_INTERFACE pri;
PIMAGE_INTERFACE pii;
PRENDER3D_INTERFACE pr3i;

PRENDERER render;
	static int drawn;

RCOORD x02m = 0.5;
RCOORD y02m = 0.2;

RCOORD x02j = 0.1;
RCOORD y02j = 0.2;

RCOORD xorg = 0.0;
RCOORD yorg = 0.0;

RCOORD display_scale = 1.0;
Image output_surface;

int mx, my;

int ii1_max_iter;
int _ii1_max_iter;
int ii2_max_iter;
int _ii2_max_iter;

int IsInfinite( RCOORD _x0, RCOORD _y0, RCOORD a, RCOORD b )
{
	RCOORD x, x0, y, y0;
	int iteration = 0;
	int  max_iteration = 100;
	RCOORD xtemp;

	x = a;
	y = b;
	x0 = _x0;
	y0 = _y0;

	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		RCOORD _y = y;
		RCOORD delta;
		xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( delta < 0.00000001 )
			break;

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

int DrawInfinite( Image surface, RCOORD a, RCOORD b, RCOORD da, RCOORD db, int ofsx, int ofsy )
{
	RCOORD x, x0, _y, y, y0;
	RCOORD delta;
	int iteration = 0;
	int  max_iteration = 255;
	RCOORD xtemp;

	x = x02j;
	y = y02j;
	x0 = a;
	y0 = b;
		da = 0.01;
		db = 0.01;

	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( delta < 0.000001 )
			break;
		do_line( surface, (x-xorg)/da+ofsx, (_y-yorg)/db+ofsy
			, (xtemp-xorg)/da+ofsx, (y-yorg)/db+ofsy, ColorAverage( BASE_COLOR_WHITE, BASE_COLOR_MAGENTA, iteration, max_iteration ) );
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
	//if( iteration < max_iteration && iteration > _ii1_max_iter )
	//	_ii1_max_iter = iteration;

   if( ii1_max_iter )
		return (iteration < max_iteration)?(iteration * 255 / ii1_max_iter):-1;
	else
		return (iteration < max_iteration)?(iteration * 255 / max_iteration):-1;

}
int IsInfinite2( RCOORD _x0, RCOORD _y0, RCOORD a, RCOORD b, int *result_direction )
{
	RCOORD x, x0, y, y0;
	int iteration = 0;
	int  max_iteration = 300;
	RCOORD xtemp;
	int direction = 1;
	int direction2 = 1;
	RCOORD _delta;
	RCOORD delta;
	RCOORD _y;

	x = _x0;
	y = _y0;
	x0 = a;
	y0 = b;

	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);

		if( iteration > 1 )
		{
			if( delta == 0 ) // dititally too small to see
			{
				direction = -1;
				break;
			}
			/*
			if( _delta / delta < 0.1 )
			{
				direction2 = -1;
				break;
			}
			*/
			if( delta/delta > 2 )
			{
				direction2 = -1;
				break;
			}
		}
		_delta = delta;

		if( iteration > 20 )
		{
			int a = 3;
		}
		x = xtemp;

		iteration = iteration + 1;
	}
	//if( iteration == max_iteration )
		{
			//if( b > 0.1 )
			//lprintf( "iterations: %d   %g %g %g %g", iteration, _x0, _y0, a, b  );
		}
	if( iteration < max_iteration && iteration > _ii2_max_iter )
		_ii2_max_iter = iteration;

   //if( ii2_max_iter )
	//	return (iteration < max_iteration)?(iteration * 255 / ii2_max_iter):-1;
	//else
	result_direction[0] = direction;
		return direction2 * ( (iteration < max_iteration)
			?(iteration * 255 / max_iteration)
			:-(iteration * 255 / max_iteration) );
	//return iteration < max_iteration?(iteration * 255 / max_iteration):-1;
}

int DrawInfinite2( Image surface, RCOORD _x0, RCOORD _y0, RCOORD a, RCOORD b, RCOORD da, RCOORD db, int ofsx, int ofsy )
{
	RCOORD x, x0, _y, y, y0;
	RCOORD delta, _delta = 0;
	int iteration = 0;
	int  max_iteration = 255;
	RCOORD xtemp;

	// setup f(0)
	x = _x0;
	y = _y0;
	x0 = a;
	y0 = b;

	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2*10*10)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( iteration > 1 )
		{
			if( delta == 0 ) // dititally too small to see
			{
				break;
			}
			if( _delta / delta < 0.01 )
			{
				break;
			}
		}
		_delta = delta;

		x = xtemp;
		iteration = iteration + 1;
	}


	x = _x0;
	y = _y0;
	x0 = a;
	y0 = b;

	// override display scaling
	da = 0.01;
	db = 0.01;

	iteration = 0;
	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( iteration > 1 )
		{
			if( delta == 0 ) // dititally too small to see
			{
				lprintf( "float underflow" );
				break;
			}

			/*
			lprintf( "delta del [%g,%g] [%g,%g] %g/%g = %g"
				, x, xtemp
				, _y, y 
				, _delta, delta, _delta / delta );
			*/
			if( _delta / delta < 0.01 )
			{
				break;
			}
		}
		_delta = delta;

		do_line( surface, (x-xorg)/da+ofsx, (_y-yorg)/db+ofsy
			, (xtemp-xorg)/da+ofsx, (y-yorg)/db+ofsy, BASE_COLOR_WHITE );
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
	//if( iteration < max_iteration && iteration > _ii1_max_iter )
	//	_ii1_max_iter = iteration;

   if( ii1_max_iter )
		return (iteration < max_iteration)?(iteration * 255 / ii1_max_iter):-1;
	else
		return (iteration < max_iteration)?(iteration * 255 / max_iteration):-1;

}

int GLDrawInfinite2( Image surface, RCOORD _x0, RCOORD _y0, RCOORD a, RCOORD b, RCOORD da, RCOORD db, int ofsx, int ofsy )
{
	RCOORD x, x0, _y, y, y0;
	RCOORD delta, _delta = 0;
	int iteration = 0;
	int  max_iteration = 255;
	RCOORD xtemp;
	float here[3];
	// setup f(0)
	x = _x0;
	y = _y0;
	x0 = a;
	y0 = b;

	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2 * 100 * 100 )  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( iteration > 1 )
		{
			if( delta == 0 ) // dititally too small to see
			{
				break;
			}
			if( _delta / delta < 0.01 )
			{
				break;
			}
		}
		_delta = delta;

		x = xtemp;
		iteration = iteration + 1;
	}


	x = _x0;
	y = _y0;
	x0 = a;
	y0 = b;

	// override display scaling
	da = 0.01;
	db = 0.01;

	iteration = 0;
		glBegin( GL_LINE_STRIP );
		glColor4ub( 255,255,255,255 );
	//plot( surface, x/da+ofsx, y/db+ofsy, Color(255-iteration,255-iteration,255-iteration) );
	while ( x*x + y*y <= (2*2 * 100 * 100)  &&  iteration < max_iteration )
	{
		xtemp = x*x - y*y + x0;
		_y = y;
		y = 2*x*y + y0;

		delta = (xtemp - x)*(xtemp-x) + (y-_y) * (y-_y);
		if( iteration > 100 )
		{
			if( delta == 0 ) // dititally too small to see
			{
				lprintf( "float underflow" );
				break;
			}

			/*
			lprintf( "delta del [%g,%g] [%g,%g] %g/%g = %g"
				, x, xtemp
				, _y, y 
				, _delta, delta, _delta / delta );
			*/
			if( _delta / delta < 0.01 )
			{
				break;
			}
			if( _delta / delta > 2 )
			{
				break;
			}
		}
		_delta = delta;
		here[vRight] = ( (xtemp-xorg)/da+ofsx ) / 10;
		here[vUp] = ( (y-yorg)/db+ofsy ) / 10;
		here[vForward] = iteration * 0.1;

		glVertex3fv( here );

		x = xtemp;

		iteration = iteration + 1;
	}
	glEnd();
	//if( iteration < max_iteration && iteration > _ii1_max_iter )
	//	_ii1_max_iter = iteration;

   if( ii1_max_iter )
		return (iteration < max_iteration)?(iteration * 255 / ii1_max_iter):-1;
	else
		return (iteration < max_iteration)?(iteration * 255 / max_iteration):-1;

}



	int ofsx, ofsy;
   RCOORD dx, dy;

void Render( PRENDERER r, int output )
{
	int a,b;
	Image surface = GetDisplayImage( r );
	RCOORD aspect;

	if( !output_surface )
		output_surface = MakeImageFile( surface->width, surface->height );
	aspect = (RCOORD)output_surface->width / (RCOORD)output_surface->height;
	ofsx = (output_surface->width/2);
	ofsy = (output_surface->height/2);
	dx = display_scale*4.0 / (RCOORD)output_surface->width;
	dy = display_scale*4.0 / (RCOORD)output_surface->height;

	if( !drawn )
	{
		drawn = 1;
	ClearImage( output_surface);

   _ii1_max_iter = 0;
   _ii2_max_iter = 0;
   {
		RCOORD dam, dbm;
		dam = ( mx - ofsx ) * dx + xorg;
		dbm = ( my - ofsy ) * dy + yorg;
	for( a = 0; a < output_surface->width; a++ )
	{
		for( b = 0; b < output_surface->height; b++ )
		{
			RCOORD da, db;
			RCOORD rda, rdb;
			int r, g = 0, bl  = 0;
			int direction;
			//rda = ( ( b *2 ) / (RCOORD)output_surface->height) * sin(3.14159/2 + ( ( a * 2*3.14159 ) / (RCOORD)output_surface->width));
			//rdb = ( ( b *2 ) / (RCOORD)output_surface->height) * cos(3.14159/2 + ( ( a * 2*3.14159 ) / (RCOORD)output_surface->width));
			//da = ( rda  ) + xorg;
			//db = ( rdb ) + yorg;

			da = aspect * ( a - ofsx ) * dx + xorg;
			db = ( b - ofsy ) * dy + yorg;
			//if( (r = IsInfinite( dam, dbm, da, db )) == -1 )
				r = 0;

			/*
			// draw just in range of the cursor 
			if( !( ( (mx - a) > -20) && ( (mx-a) < 20 )
				&& ( (my - b) > -20 ) && ( (my - b) < 20 ) ) )
			{
				continue;
			}
			*/
			//if( (g = IsInfinite2( dam, dbm, da, db )) == -1 )
			//	g = 0;
			bl = IsInfinite2( 0, 0, da, db, &direction );
			if( (direction) < 0 )
			{
				r = ( -bl );
				bl = 0;
			}
			else if( bl < 0 )
			{
				g = 32;
				bl = 0;
				//r = 0;
			}
			else if( bl != 0 )
			{
				//r = 0;
				bl = bl;
			}
			//else
			//	r = 0;
			if( g == 255 )
			{
				int a = 3;
			}
			{

				// point pair horiz -0.123 (R) 0.35071355833500363833634934966131      
				//                    0.422(I)  0.64961527075646859365525325998975
				//
				// point pair vert  -0.563(R)  0.75033325929216279143681201481957  0(I)     /*distance to next 0.49986672471039669667784846697923*/
				// point pair vert  -1.563(R)  1.2501999840025594881146604817988   0(I)    /* to next 0.1125178884860932415243100526011 */
				// next is -1.857         (R)  1.3627178724886527296389705343999   0(I)

				RCOORD unity = da*da + db*db;
				RCOORD offset_unity = (da+0.123)*(da+0.123) + db*db;
			if( unity < 0.564 && unity >= 0.562 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_WHITE, 64, 256 ) );
			else if( offset_unity < 0.422 && offset_unity >= 0.420 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_WHITE, 64, 256 ) );
			else if( unity < 1.001 && unity >= 0.999 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_WHITE, 138, 256 ) );
			else if( unity < 1.564 && unity >= 1.562 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_WHITE, 64, 256 ) );
			else if( unity < 1.868 && unity >= 1.866 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_WHITE, 64, 256 ) );
			else if( da >= -0.125 && da <= -0.122 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_BLUE, 64, 256 ) );
			else if( da >= -0.001 && da <= 0.001 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_GREEN, 64, 256 ) );
			else if( db >= -0.001 && db <= 0.001 )
				plot( output_surface, a, b, ColorAverage( Color(r,g,bl), BASE_COLOR_ORANGE, 64, 256 ) );
			else
				plot( output_surface, a, b, Color(r,g,bl) );
			}
		}
	}
   }


   ii1_max_iter = _ii1_max_iter;
   ii2_max_iter = _ii1_max_iter;
	if( output )
	{
		// actually put the changes on the screen....
		PutString( output_surface, 10, 10, BASE_COLOR_WHITE, BASE_COLOR_BLACK, "Alt+TAB and select application,\nThen Press ALT+F4 to Exit. " );
	}
	}
	BlotImage( surface, output_surface, 0, 0 );

	{
	   {
			RCOORD dam, dbm;
			dam = ( mx - ofsx ) * dx + xorg;
			dbm = ( my - ofsy ) * dy + yorg;
		for( a = -20; a <= 20; a++ )
		{
			for( b = -20; b <= 20; b++ )
			{
				RCOORD da, db;
				int r, g = 0, bl  = 0;
				int direction;
				float v1[3];
				float v2[3];
				da = aspect * ( mx + a - ofsx ) * dx + xorg;
				db = ( my + b - ofsy ) * dy + yorg;
				r = 0;
				bl = IsInfinite2( 0, 0, da, db, &direction );
				if( (direction) < 0 )
				{
					v1[vForward] = bl * 0.05;
					r = ( bl );
					bl = 0;
				}
				else if( bl < 0 )
				{
					v1[vForward] = 0.5;
					g = 32;
					bl = 0;
					//r = 0;
				}
				else if( bl != 0 )
				{
					v1[vForward] = bl * 0.04;
					//r = 0;
					bl = bl;
				}
				//else
				//	r = 0;
				if( g == 255 )
				{
					int a = 3;
				}
				glBegin( GL_LINES );
				v2[vRight] = v1[vRight] = ( mx + a ) * 0.1;
				v2[vUp] = v1[vUp] = ( my + b ) * 0.1;
				v2[vForward] = 0;
				glColor4ub( r*0xFF,g,bl,255 );
				glVertex3fv( v1 );
				glVertex3fv( v2 );
				glEnd();
				//plot( output_surface, a, b, Color(r,g,bl) );
			}
		}
	   }

	}


	{
		RCOORD da, db;
		int r, g, bl  = 0;
		int iter;
		da = aspect * ( mx - ofsx ) * dx + xorg;
		db = ( my - ofsy ) * dy + yorg;
		//iter = DrawInfinite2( output_surface, 0, 0, da, db, dx, dy, ofsx, ofsy );
		iter = GLDrawInfinite2( output_surface, 0, 0, da, db, dx, dy, ofsx, ofsy );
		if( 0 )
		{
			TEXTCHAR buf[256];
			snprintf( buf,256, "%d   %d,%d = %g, %g", iter, mx, my, da, db );
			PutString( output_surface, 10, 50, BASE_COLOR_WHITE, BASE_COLOR_BLACK, buf );
		}
	}
}

static void CPROC OnDraw3d( "Mandelbrot renderer" )( PTRSZVAL psvInit )
{
	Render( render, 0 );

}

void CPROC MyRedrawCallback( PTRSZVAL psv, PRENDERER r )
{
   Render( r, FALSE );
}


int CPROC MyMouseCallback( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	if( b & MK_RBUTTON )
	{
		xorg += ( mx - x ) * ( display_scale*4.0 / (RCOORD)output_surface->width );
		yorg += ( my - y ) * ( display_scale*4.0 / (RCOORD)output_surface->height );
			drawn = 0;
	}
	else
	{
		if( ( b & MK_SHIFT ) && ( b & MK_LBUTTON ) )
		{
			if( output_surface )
			{
				RCOORD newscale = display_scale - (display_scale*0.3);
				xorg -= (x*newscale/output_surface->width - x*display_scale/output_surface->width );
				yorg -= (y*newscale/output_surface->height -y*display_scale/output_surface->height );

				display_scale = newscale;
				drawn = 0;
			}
		}


		if( ( b & MK_CONTROL )&& ( b & MK_LBUTTON ) )
		{
			RCOORD newscale = display_scale + (display_scale*0.01);
			xorg -= (x*newscale/output_surface->width -mx *display_scale/output_surface->width );
			yorg -= (y*newscale/display_scale/output_surface->height -my *display_scale/output_surface->height );

			display_scale = newscale;
			drawn = 0;
		}
	}
   //if( !b )
	//	ii1_max_iter = 0;
   //if( !b )
	//	ii2_max_iter = 0;

	//x02 = ( x - ofsx )* dx;
	//y02 = ( y - ofsy ) * dy;
	mx = x;
	my = y;



   Redraw( (PRENDERER)psv );

   return 0;
}


static PTRSZVAL CPROC OnInit3d( "Mandelbrot renderer" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	_32 w, h;
	GetDisplaySize( &w, &h );
	render = OpenDisplaySizedAt( 0, w, h, 0, 0 );
	SetRedrawHandler( render, MyRedrawCallback, 0 );
	SetMouseHandler( render, MyMouseCallback, (PTRSZVAL)render );
	UpdateDisplay( render );
	return 0;
}

PRELOAD( main )
{
	pri = GetDisplayInterface();
	pii = GetImageInterface();
	pr3i = GetRender3dInterface();


	return 0;
}


