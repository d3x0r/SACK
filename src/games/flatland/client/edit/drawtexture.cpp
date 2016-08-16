#include <stdhdrs.h>

#include <sharemem.h>
#include <logging.h>

#include "flatland_global.h"


#include <psi.h>
#include <controls.h>

typedef int DISPLAYPOINT[2];

typedef struct linedel_tag {
	int from, to; // point from and to in the list...
	int x;    // y is unneeded - a delerror struct only applies to one side.
	int xinc; // yinc is always +1
	int delx, dely;
	int err, len;
} LINEDEL, *PLINEDEL;

int FindTopPoint( int npoints, DISPLAYPOINT *list )
{
	int top = list[0][1];
	int n, result = 0;
	for( n = 1; n < npoints; n++ )
	{
		if( list[n][1] < top )
		{
			top = list[n][1];
         result = n;
		}	
	}
	return result;
}

int ClipDisplayPointsToScreen( int points, DISPLAYPOINT *pointlist )
{
	int n;
	for( n = 0; n < points; n++ )
	{
		
	}
	return points;
}

int IsTextureVisible( PCOMMON pc, int points, DISPLAYPOINT *pointlist )
{	
	int n;
   Image image = GetControlSurface( pc );
	for( n = 0; n < points; n++ )
	{
		if( pointlist[n][0] >=0 && 
		    pointlist[n][0] <= image->width &&
          pointlist[n][1] >=0 && 
		    pointlist[n][1] <= image->height )
			return TRUE;
	}
	return FALSE;
}

int StepLineDelta( PLINEDEL pld, int step, int npoints, DISPLAYPOINT *display_points )
{
	pld->x = display_points[pld->from][0];
	pld->to = pld->from + step;
	if( pld->to < 0 )
		pld->to += npoints;
	if( pld->to >= npoints )
		pld->to -= npoints;

	pld->delx = display_points[pld->to][0] - pld->x;
	if( pld->delx < 0 )
	{
		pld->xinc = -1;
		pld->delx = -pld->delx;
	}	
	else
		pld->xinc = 1;
	pld->dely = display_points[pld->to][1] - display_points[pld->from][1];
	if( pld->dely < 0 )
	{
		pld->dely = -pld->dely;
		return FALSE;
	}
	pld->len = pld->dely;
	if( pld->dely > pld->delx )
		pld->err = (pld->dely/2);
	else
		pld->err = (pld->delx/2) * -pld->xinc;
	return TRUE;
}

int DrawTextureSolidColor( PCOMMON pc, INDEX iSector, CDATA color )
{
	int npoints, n;
	_POINT *pointlist;
	PDISPLAY display = ControlData( PDISPLAY, pc );
	DISPLAYPOINT display_points[16];
	int top;
	// at this point - 4 extra points 1 for each 
	// clipping side of the display is sufficient 
	// when we introduce more clip plane - the current
	// count of planes must be added worst case;
	pointlist = ComputeSectorPointList( display->pWorld, iSector, &npoints );
	//pointlist = sector->pointlist;
	//npoints = sector->npoints;
	//glBegin();

	//lprintf( WIDE("points %p count %d"), pointlist, npoints );

	for( n = 0; n < npoints; n++ )
	{
		//gl
		display_points[n][0] = DISPLAY_X( display, pointlist[n][0] );
		display_points[n][1] = DISPLAY_Y( display, pointlist[n][1] );
		//Log3( "Point id: %d at: (%d,%d)", n
		//				, display_points[n][0]
		//				, display_points[n][1] );
	}

	//glEnd();

	npoints = ClipDisplayPointsToScreen( npoints, display_points );
	if( !IsTextureVisible( pc, npoints, display_points ) )
	{
		//Release( display_points );
		return FALSE;
	}
	if( npoints ) // if any points are left to display....
	{
		int y;
		LINEDEL left, right;
		top = FindTopPoint( npoints, display_points );
		//Log1( "Top point is : %d", top );
		left.to = right.to = top;
		left.from = right.from = top;
		left.len = right.len = 0;

		y = display_points[top][1];
		do
		{
			leftloop:
			{
				if( !left.len )
				{
					left.from = left.to;
					if( !StepLineDelta( &left, -1, npoints, display_points ) )
					{
               			//Log( "Breaking loop on step left delta" );
						break;
					}
					if( left.to == right.from )
					{
						//Log( "Leaving loop cause left overlapped right." );
						break;
					}
				}
				//Log2( "Stepped Left... doing loop len: %d %d", left.len, left.err );
				while( left.len && left.err < 0 )
				{
					left.err += left.dely;
					left.x += left.xinc;				
				}
				//Log1( "did loop... doing final increments %d", left.len );
				if( left.len )
				{
					left.err -= left.delx;
					left.len--;
				}
				else
				{
					//Log( "Supposed to go back and restep left!" );
					goto leftloop;
				}
			}

			rightloop:
			{
				if( !right.len )
				{
					right.from = right.to;
					if( !StepLineDelta( &right, 1, npoints, display_points ) )
					{
						//Log( "Breaking loop on step right delta" );
						break;
					}
					if( right.to == left.from )
					{
						//Log( "Leaving loop cause right overlapped left." );
						break;
					}
				}
				//Log1( "Stepped Right... doing loop len: %d", right.len );
				while( right.len && right.err < 0 )
				{
					right.err += right.dely;
					right.x += right.xinc;				
				}
				//Log1( "did loop... doing final increments %d", right.len );
				if( right.len )
				{
					right.err -= right.delx;
					right.len--;
				}
				else
				{
					//Log( "Supposed to go back and restep right!" );
					goto rightloop;
				}
			}

			{
				if( right.x > left.x )
					do_hlineAlpha( display->pImage, y, left.x, right.x-1, color );
				//BlatColorAlpha( display->pImage, left.x, y, right.x-left.x, 1, color );
				y++;
			}
			//	Log4( "DrawLine (%d,%d) - (%d,%d)", left.x, left.y, right.x, right.y );
		} while( 1 ); // rely on internal break....
	}
	/*
	for( n = 0; n < npoints; n++ )
	{
		plot( display->pImage, display_points[n][0]
								  , display_points[n][1]
								  , Color( 255, 0, 0 ) );
	}
	*/
	//Release( pointlist );
	//Release( display_points );	
	return TRUE;
}


extern int sectorsdrawn, sectorsskipped;

uintptr_t CPROC DrawSectorTexture( INDEX iSector, uintptr_t psv)
{
	PCOMMON pc = (PCOMMON)psv;
	PDISPLAY display = ControlData( PDISPLAY, pc );
	INDEX iTexture = GetSectorTexture( display->pWorld, iSector );
	PFLATLAND_TEXTURE texture;
	//lprintf( WIDE("Drawing a sector...") );
	GetTextureData( display->pWorld, iTexture, &texture );
	if( !texture )
	{
		//lprintf( WIDE("no texture to draw.") );
		return 0;
	}
	if( texture->flags.bColor )
	{
		//lprintf( WIDE("Draw color...") );
		if( DrawTextureSolidColor( pc, iSector, texture->data.color ) )
			sectorsdrawn++;
		else
			sectorsskipped++;
	}
	else
		lprintf (WIDE("missing data?") );
	return 0;
}
