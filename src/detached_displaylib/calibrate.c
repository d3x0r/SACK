#include <stdhdrs.h>
#include <idle.h>
#include <timers.h>
#include "global.h"

RENDER_NAMESPACE

#if defined( __LINUX__ ) || defined( WIN32 )
// and probalby #ifdef __SDL__

IMAGE_PROC( void, DisplayBlatColor )( Image image, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

// 2 points - 1/12, 1/12 and 11/12, 11/12
// 4 points - 1/12, 1/12
// 6 points

#undef SDL_PATCHED

#ifdef SDL_PATCHED
void SDL_ResetELOCalibration( void );
#endif

extern void Mouse( PTRSZVAL unused, S_32 x, S_32 y, _32 b );


typedef struct calibration_point_tag
{
   _32 points;   // number of points in the set...
	_32 divisor;  // how many divisions on the screen (res indepandant)...
   IMAGE_POINT where[25];   // actual spot...
   IMAGE_POINT touched[25]; // touched spot...
} CALIBRATION_POINT_SET, *PCALIBRATION_POINT_SET;

#define NUM_POINT_SETS ( sizeof( cal_points ) / sizeof( cal_points[0] ) )
static CALIBRATION_POINT_SET cal_points[] = {
  { 2, 12, { { 1, 1 }, { 11, 11 } } }
, { 4, 12, { { 1, 1 }, { 11, 11 }, { 1, 11 }, { 11, 1 } } }
, { 6, 11, { { 1, 1 }, { 10, 10 }, { 4, 4 }, { 7, 7 }
			 , { 10, 1 }, { 1, 10 } } }
};



typedef struct calibrate_state_tag
{
	struct {
		_32 calibrating : 1;
	} flags;
   _32 spot_width, spot_height;
   _32 curpoint;
	PCALIBRATION_POINT_SET points;
	_32 last_release_time;
   _32 last_buttons;
   S_32 EloMinX, EloMinY;
	S_32 EloMaxX, EloMaxY;
   S_32 Orig_EloMinX, Orig_EloMinY;
	S_32 Orig_EloMaxX, Orig_EloMaxY;
} CALIBRATE_STATE;

static CALIBRATE_STATE c;

int ComputeNewMinMax( int ActualPoint1, int ActualPoint2 // actual point drawn on screen
			  , int OutputPoint1, int OutputPoint2 // point touched
           , int PointRange  // x == 800, y == 600 for instance
			  , int elomin, int elomax
			  , S_32 *pNewELOMin, S_32 *pNewELOMax
			  )
{
	if( !pNewELOMin || !pNewELOMax )
		return FALSE;
	{
   int pa1 = ActualPoint1;
	int pa2 = ActualPoint2;

	int po1 = OutputPoint1;
	int po2 = OutputPoint2;


	//int en = 400;
	//int ex = 3400;
	int en = elomin;
	int ex = elomax;

   int ew = ex - en;

   int r = PointRange;

   int eo1 = elomin + ( ( ( elomax-elomin ) / r ) * po1 );
	int eo2 = elomin + ( ( ( elomax-elomin ) / r ) * po2 );

   int ea1 = elomin + ( ( ( elomax-elomin ) / r ) * pa1 );
	int ea2 = elomin + ( ( ( elomax-elomin ) / r ) * pa2 );


	//int ei1 = ( ( ( po1 ) * ( ew ) ) / r ) + en;
	//int ei2 = ( ( ( po2 ) * ( ew ) ) / r ) + en;


	//lprintf( WIDE(" elo expected %d,%d"), eo1, eo2 );
   //lprintf( WIDE(" elo actual %d,%d"), ei1, ei2 );

	// ei1, ei2 are the elo coordinates I got for the touched points

	//printf( WIDE("%d = %d (old params)\n"), ei1, po1 );
	//printf( WIDE("%d = %d (old params)\n"), ei2, po2 );

	//int nen = ( pa2 * ei1 - pa1 * ei2 ) / ( pa2 - pa1 );
	//int new = ( ( ei1 - nen ) * r ) / pa1;
	//int nex = new + nen;
//   int eloslope = (ei2-ei1)/(pa2-pa1);
   int nen = ((eo1-ea1) * (pa1*ew)/r);
   int nex = (ew*(eo2-eo1))/(ea2-ea1) + nen;

	*pNewELOMin = nen;
   *pNewELOMax = nex;
	//printf( WIDE("min %d  max %d\n"), nen, nex );

	//int npo1 = r * ( ei1 - nen ) / ( nex - nen );
   //printf( WIDE("%d = %d (with new params)\n"), ei1, npo1 );
	//int npo2 = r * ( ei2 - nen ) / ( nex - nen );
	//printf( WIDE("%d = %d (with new params)\n"), ei2, npo2 );
	}
   return TRUE;
}

void CalculateNewValues( int pointset )
{
	int n = pointset;
   int Min, Max;
	{
		int coord1, coord2;
      coord1 = g.width * c.points->where[n*2][0] / c.points->divisor;
		coord2 = g.width * c.points->where[n*2+1][0] / c.points->divisor;
		Min = c.EloMinX;
      Max = c.EloMaxX;
		ComputeNewMinMax( coord1, coord2
							 , c.points->touched[n*2][0], c.points->touched[n*2+1][0]
							 , g.width
							 , c.Orig_EloMinX, c.Orig_EloMaxX
							, &c.EloMinX, &c.EloMaxX );
      Log4( WIDE("Original X's = %d,%d  New X's = %") _32f WIDE(",%") _32f WIDE(""), Min, Max, c.EloMinX, c.EloMaxX );
		if( abs( Min - c.EloMinY ) < 2 )
         Log( WIDE("Min didn't update much... good show...") );
		if( abs( Max - c.EloMaxY ) < 2 )
         Log( WIDE("Max didn't update much... good show...") );
      coord1 = g.height * c.points->where[n*2][1] / c.points->divisor;
      coord2 = g.height * c.points->where[n*2+1][1] / c.points->divisor;
		Min = c.EloMinY;
      Max = c.EloMaxY;
		ComputeNewMinMax( coord1, coord2
							 , c.points->touched[n*2][1], c.points->touched[n*2+1][1]
							 , g.height
							 , c.Orig_EloMinY, c.Orig_EloMaxY
                      , &c.EloMinY, &c.EloMaxY );
		Log4( WIDE("Original Y's = %d,%d  New Y's = %"_32fs",%"_32fs""), Min, Max, c.EloMinY, c.EloMaxY );
		if( abs( Min - c.EloMinY ) < 2 )
         Log( WIDE("Min didn't update much... good show...") );
		if( abs( Max - c.EloMaxY ) < 2 )
         Log( WIDE("Max didn't update much... good show...") );
	}
#ifdef __LINUX__
	{
		char val[12];
		snprintf( val, 12, WIDE("%") _32f WIDE(""), c.EloMinX );
		setenv( WIDE("SDL_ELO_MIN_X"), val, TRUE );
		snprintf( val, 12, WIDE("%") _32f WIDE(""), c.EloMinY );
		setenv( WIDE("SDL_ELO_MIN_Y"), val, TRUE );
		snprintf( val, 12, WIDE("%") _32f WIDE(""), c.EloMaxX );
		setenv( WIDE("SDL_ELO_MAX_X"), val, TRUE );
		snprintf( val, 12, WIDE("%") _32f WIDE(""), c.EloMaxY );
		setenv( WIDE("SDL_ELO_MAX_Y"), val, TRUE );
#ifdef SDL_PATCHED
		SDL_ResetELOCalibration();
#endif
	}
#endif
}


int CPROC CalibrationMouse( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	// ignore all mouse events for 100 milliseconds after last
   // release...
	if( ( c.last_release_time + 100 > GetTickCount() ) )
		return 1;

	if( !b && c.last_buttons )
	{
		c.last_release_time = GetTickCount();
      c.points->touched[c.curpoint][0] = x;
      c.points->touched[c.curpoint][1] = y;
		c.curpoint++;
		if( !( c.curpoint & 1 ) ) // is an even number...
		{
         CalculateNewValues( ( c.curpoint - 1 ) / 2 );
		}
		if( c.curpoint == c.points->points )
		{
			// unset mouse handler, unhide panels...
			{
				PPANEL panel = g.pRootPanel->below;
				while( panel )
				{
					// turn all windows invisible for a moment...
					if( panel != g.pRootPanel )
					{
						// hmm need a prior state per window I suppose...
						panel->common.flags.invisible =
							panel->common.flags.prior_invisible;
					}
          panel = panel->below;
        }
			}
			UpdateDisplay( g.pRootPanel );
         SetMouseHandler( g.pRootPanel, NULL, 0 );
			c.flags.calibrating = FALSE;
         return 1;
		}
		else
		{
			DisplayBlatColor( GetDisplayImage( g.pRootPanel )
					  , g.width * c.points->where[c.curpoint - 1][0]  / c.points->divisor - c.spot_width / 2
					  , g.height * c.points->where[c.curpoint - 1][1] / c.points->divisor - c.spot_height / 2
					  , c.spot_width, c.spot_height
					  , Color( 0,0,0 ) );
			DisplayBlatColor( GetDisplayImage( g.pRootPanel )
					  , g.width * c.points->where[c.curpoint][0]  / c.points->divisor - c.spot_width / 2
					  , g.height * c.points->where[c.curpoint][1] / c.points->divisor - c.spot_height / 2
					  , c.spot_width, c.spot_height
					  , Color( 255,255,255 ) );
      
      UpdateDisplay( g.pRootPanel );

		}
	}
	c.last_buttons = b;
   return 1;
}

RENDER_PROC( int, BeginCalibration )( _32 points )
{
	{
		_32 n;
		for( n = 0; n < NUM_POINT_SETS; n++ )
		{
			if( cal_points[n].points == points )
				break;
		}
		if( n == NUM_POINT_SETS )
			return 0;
		c.points = &cal_points[n];
      c.spot_width = 10;
      c.spot_height = 10;
	}
   sscanf( getenv( WIDE("SDL_ELO_MIN_Y") ), WIDE("%") _32f WIDE(""), &c.Orig_EloMinY );
   sscanf( getenv( WIDE("SDL_ELO_MAX_Y") ), WIDE("%") _32f WIDE(""), &c.Orig_EloMaxY );
   sscanf( getenv( WIDE("SDL_ELO_MIN_X") ), WIDE("%") _32f WIDE(""), &c.Orig_EloMinX );
	sscanf( getenv( WIDE("SDL_ELO_MAX_X") ), WIDE("%") _32f WIDE(""), &c.Orig_EloMaxX );
	lprintf( WIDE("Mins: %") _32f WIDE(" %") _32f WIDE("  maxs: %") _32f WIDE(" %") _32f WIDE(""), c.Orig_EloMinX, c.Orig_EloMinY, c.Orig_EloMaxX, c.Orig_EloMaxY );
   // only allow one calibration at a time...
	if( c.flags.calibrating )
		return 0;
   c.flags.calibrating = TRUE;
	if( g.pRootPanel )
	{
		BuildSpaceTreeEx( NULL );

      c.curpoint = 0;
		SetMouseHandler( g.pRootPanel, CalibrationMouse, 0 );
      // show firsst point...
		DisplayBlatColor( GetDisplayImage( g.pRootPanel )
				  , g.width * c.points->where[c.curpoint][0]  / c.points->divisor - c.spot_width / 2
				  , g.height * c.points->where[c.curpoint][1] / c.points->divisor - c.spot_height / 2
				  , c.spot_width, c.spot_height
				  , Color( 255,255,255 ) );
		UpdateDisplay( g.pRootPanel );
		// wait for touch...

	}
	while( c.flags.calibrating )
	{
		Idle();
	}
   return 1;
}
#else
RENDER_PROC( int, BeginCalibration )( _32 points )
{
	return 1;
}
#endif

RENDER_NAMESPACE_END
