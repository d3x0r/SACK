#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE
#define TERRAIN_MAIN_SOURCE
#include <math.h>
#include <render.h>
#include <render3d.h>
#include <vectlib.h>
#include <sharemem.h>


#include <brain.hpp>
#include <../board/brainshell.hpp>

#include <psi.h>
#include <virtuality/view.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "land.h"

#define  BODY_COUNT 5
#define SPHERE_SIZE PLANET_RADIUS

//#define POLE_NEAR_AREA_DEBUG
//#define DEBUG_RENDER_POLE_NEARNESS

// land hex
//     x
//    ___
// y / 0 /\ x
//  /___/ 2\    ...
//  \  1\  /    ...
// y \___\/ y
//      x


#if 0
enum { UP
	  , RIGHT_UP
	  , RIGHT_DOWN
	  , DOWN
	  , LEFT_DOWN
	  , LEFT_UP
} DIRECTIONS;
#endif

#include "local.h"

EasyRegisterControlWithBorder( WIDE("Terrain View"), 0, BORDER_NONE );

struct world_body_map bodymap;
int cur_x, cur_y, cur_s;

void ConvertToSphereGrid( P_POINT p, int *s, int *x, int *y ); // s 0-3, s 4-9, 10-12

void ConvertPolarToRect( int level
							, int c
							, int *x
							, int *y )
{
	// all maps must include c == HEX_SIZE
   c %= (level*2+1);
		if( c < level )
		{
         // need to redo the flat distortion....
			(*x) = level;
			(*y) = c;
		}
		else 
		{
			//if( c == level )
			(*x) = ((level)-(c-level));
			(*y) = level;
		}
}





// unit vector parts
struct band {
	// the 4 corners that make up this sqaure.
	// could, in theory, scan this to come up with the 
	// correct square to point conversions
	struct {
		int r, c;
	} corners[HEX_SIZE][HEX_SIZE][4]; // the 4 corners that are this hex square.
	struct {
      // grid is col, row (x, y)
		VECTOR grid[HEX_SIZE][HEX_SIZE+1]; // +1 cause there's this many squares bounded by +1 lines.
		struct {
			int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
		} near_area[HEX_SIZE][HEX_SIZE+1][4]; // the 4 corners that are this hex square.
		// each area is near 4 other areas.
		// these areas are
		//        1
      //        |
		//    2 - o - 0
      //        |
		//        3
      //

	} patches[6]; // actually this is continuous...
	band() 
	{
		int s;
		int r;
		int c;
		for( s = 0; s < 6; s++ )
			for( r = 0; r < HEX_SIZE; r++ )
				for( c = 0; c < HEX_SIZE; c++ )
				{
					if( c < (HEX_SIZE-1) )
					{
						bodymap.near_areas[s+3][c][r][0].s = s+3;
						bodymap.near_areas[s+3][c][r][0].x = c+1;
						bodymap.near_areas[s+3][c][r][0].y = r;
					}
					else
					{
						bodymap.near_areas[s+3][c][r][0].s = (s+1)%6+3;
						bodymap.near_areas[s+3][c][r][0].x = 0;
						bodymap.near_areas[s+3][c][r][0].y = r;
					}
					if( r < (HEX_SIZE-1) )
					{
						bodymap.near_areas[s+3][c][r][1].s = s+3;
						bodymap.near_areas[s+3][c][r][1].x = c;
						bodymap.near_areas[s+3][c][r][1].y = r+1;
					}
					else
					{
						bodymap.near_areas[s+3][c][r][1].s = (s/2)+9;
						ConvertPolarToRect( HEX_SIZE, ((s%2)?HEX_SIZE:0)+c
												 , &bodymap.near_areas[s+3][c][r][1].x
												 , &bodymap.near_areas[s+3][c][r][1].y
												);
					}
					if( c )
					{
						bodymap.near_areas[s+3][c][r][2].s = s+3;
						bodymap.near_areas[s+3][c][r][2].x = c-1;
						bodymap.near_areas[s+3][c][r][2].y = r;
					}
					else
					{
						bodymap.near_areas[s+3][c][r][2].s = (s+5)%6+3;
						bodymap.near_areas[s+3][c][r][2].x = HEX_SIZE-1;
						bodymap.near_areas[s+3][c][r][2].y = r;
					}
					if( r )
					{
						bodymap.near_areas[s+3][c][r][3].s = s+3;
						bodymap.near_areas[s+3][c][r][3].x = c;
						bodymap.near_areas[s+3][c][r][3].y = r-1;
					}
					else
					{
						bodymap.near_areas[s+3][c][r][3].s = (s/2);
						ConvertPolarToRect( HEX_SIZE, ((s%2)?HEX_SIZE:0)+c
												 , &bodymap.near_areas[s+3][c][r][3].x
												 , &bodymap.near_areas[s+3][c][r][3].y
												);
						lprintf( WIDE("%d,%d,%d is near(3) %d,%d,%d")
								 , s+3, c, r
								 , bodymap.near_areas[s+3][c][r][3].s
								 , bodymap.near_areas[s+3][c][r][3].x
								 , bodymap.near_areas[s+3][c][r][3].y
								 );
					}
				}


		for( s = 0; s < 6; s++ )
			for( r = 0; r <= HEX_SIZE; r++ )
				for( c = 0; c < HEX_SIZE; c++ )
				{
					// basically the bottom left corner is always in itself.

					if( r == HEX_SIZE )
					{
						// the first is upper left of this coord.  therefore
						// near is off the sector
						patches[s].near_area[c][r][0].s = s/2;
						ConvertPolarToRect( HEX_SIZE-1, (s%2)*HEX_SIZE + c
												, &patches[s].near_area[c][r][0].x
												, &patches[s].near_area[c][r][0].y );
						patches[s].near_area[c][r][3].s = s;
						patches[s].near_area[c][r][3].x = c;
						patches[s].near_area[c][r][3].y = r-1;

						if( c )
						{
							patches[s].near_area[c][r][1].s = s/2;
							ConvertPolarToRect( HEX_SIZE-1, (s%2)*HEX_SIZE + (c-1)
													, &patches[s].near_area[c][r][1].x
													, &patches[s].near_area[c][r][1].y );
							patches[s].near_area[c][r][2].s = 3+s;
							patches[s].near_area[c][r][2].x = c-1;
							patches[s].near_area[c][r][2].y = r;
						}
						else
						{
							patches[s].near_area[c][r][1].s = ( (s/2) + 2 ) % 3;
							ConvertPolarToRect( HEX_SIZE-1, (s%2)*HEX_SIZE + (HEX_SIZE)
													, &patches[s].near_area[c][r][1].x
													, &patches[s].near_area[c][r][1].y );

							if( s )
								patches[s].near_area[c][r][2].s = 3+s-1;
							else
								patches[s].near_area[c][r][2].s = 3+5;
							patches[s].near_area[c][r][2].x = HEX_SIZE;
							patches[s].near_area[c][r][2].y = r-1;
						}
					}
					else
					{
						patches[s].near_area[c][r][0].s = 3+s;
						patches[s].near_area[c][r][0].x = c;
						patches[s].near_area[c][r][0].y = r;
						if( r )
						{
							patches[s].near_area[c][r][3].s = 3+s;
							patches[s].near_area[c][r][3].x = c;
							patches[s].near_area[c][r][3].y = r-1;
						}
						else
						{
							patches[s].near_area[c][r][3].s = 9 + s/2;
							ConvertPolarToRect( HEX_SIZE-1, ( (s%2)*HEX_SIZE + c )
													, &patches[s].near_area[c][r][3].x
													, &patches[s].near_area[c][r][3].y );
						}
						if( !r )
						{
							if( c )
							{
								patches[s].near_area[c][r][1].s = 3+s;
								patches[s].near_area[c][r][1].x = c-1;
								patches[s].near_area[c][r][1].y = r;
								patches[s].near_area[c][r][2].s = 9+s/2;
								ConvertPolarToRect( HEX_SIZE-1, (s%2)*HEX_SIZE + (c-1)
														, &patches[s].near_area[c][r][2].x
														, &patches[s].near_area[c][r][2].y );
							}
							else
							{
								if( s )
									patches[s].near_area[c][r][1].s = 3+s-1;
								else
									patches[s].near_area[c][r][1].s = 3+5;
								patches[s].near_area[c][r][1].x = HEX_SIZE-1;
								patches[s].near_area[c][r][1].y = r;
								patches[s].near_area[c][r][2].s = 9+( (s/2) + 2 ) % 3;
								ConvertPolarToRect( HEX_SIZE-1, (s%2)*HEX_SIZE + (HEX_SIZE)
														, &patches[s].near_area[c][r][2].x
														, &patches[s].near_area[c][r][2].y );

							}
						}
						else
						{
							// first is this point...
							if( c )
							{
								patches[s].near_area[c][r][1].s = 3+s;
								patches[s].near_area[c][r][1].x = c-1;
								patches[s].near_area[c][r][1].y = r;

								patches[s].near_area[c][r][2].s = 3+s;
								patches[s].near_area[c][r][2].x = c-1;
								patches[s].near_area[c][r][2].y = r-1;
							}
							else
							{
								if( s )
									patches[s].near_area[c][r][1].s = 3+s-1;
								else
									patches[s].near_area[c][r][1].s = 3+5;
								patches[s].near_area[c][r][1].x = HEX_SIZE-1;
								patches[s].near_area[c][r][1].y = r;

								if( s )
									patches[s].near_area[c][r][2].s = 3+s-1;
								else
									patches[s].near_area[c][r][2].s = 3+5;
								patches[s].near_area[c][r][2].x = HEX_SIZE-1;
								patches[s].near_area[c][r][2].y = r-1;
							}
						}
					}
				}
			for( r = 0; r < HEX_SIZE; r++ )
				for( c = 0; c < HEX_SIZE; c++ )
				{
					corners[c][r][0].c = c;
					corners[c][r][3].r = r+1;
					if( s == 5 && c == (HEX_SIZE-1) )
					{
						corners[c][r][1].c = 0;
						corners[c][r][1].r = r;
						corners[c][r][2].c = 0;
						corners[c][r][2].r = r+1;
					}
					else
					{
						corners[c][r][1].c = c + 1;
						corners[c][r][1].r = r;
						corners[c][r][2].c = c + 1;
						corners[c][r][2].r = r + 1;
					}
				}
				{
					static PTRANSFORM work;
					int section, section2;
					int sections = 6*HEX_SIZE;
					int sections2= HEX_SIZE;
					if( !work )
						work = CreateTransform();
					else
						return;

					//scale( ref_point, VectorConst_X, SPHERE_SIZE );

					for( section2 = 0; section2 <= sections2; section2++ )
					{
						VECTOR patch1x;
						RotateAbs( work, 0, 0, ((((60.0/HEX_SIZE)*section2)-30.0)*(1*M_PI))/180.0 );
						GetAxisV( work, patch1x, vRight );
						for( section = 0; section < sections; section ++ )
						{
							RotateAbs( work, 0, ((float)section*(2.0*M_PI))/(float)sections, 0 );
							Apply( work
								, patches[section/HEX_SIZE].grid[section%HEX_SIZE][section2]
							, patch1x );
						}
					}

				}
	}
	void ExportPoints( FILE *file )
	{
		int s;
		int r;
		int c;
		for( s = 0; s < 6; s++ )
			for( r = 0; r < HEX_SIZE; r++ )
				for( c = 0; c < HEX_SIZE; c++ )
				{
					fprintf( file, WIDE("%g,%g,%g,\n"), patches[s].grid[c][r][0], patches[s].grid[c][r][1], patches[s].grid[c][r][2] );

				}

	}
	void ExportFaces( FILE *file )
	{
		int s;
		int r;
		int c;
		for( s = 0; s < 6; s++ )
			for( r = 0; r < HEX_SIZE; r++ )
				for( c = 0; c < HEX_SIZE; c++ )
				{
					int p;
               fprintf( file, WIDE("\t\t") );
					for( p = 0; p < 4; p++ )
					{
						// indexes of points for squares are...
						int idx;
                  idx = s * HEX_SIZE * HEX_SIZE + c * HEX_SIZE + r;
						fprintf( file, WIDE("%d,%d,%d,%d,-1,"), idx, idx + 1, idx + HEX_SIZE, idx + HEX_SIZE + 1 );
					}
               fprintf( file, WIDE("\n") );
				}
	}
} band;

// near_area points and this semi-polar representation needs a rectangular conversion

struct pole{
	int corners[HEX_SIZE][HEX_SIZE][4];
	struct {
		VECTOR grid[HEX_SIZE+1][HEX_SIZE+1];
		//int triangles[HEX_SIZE][HEX_SIZE*2][3];
		struct {
			int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
			// indexed by x, y from level, c translation.  Is the 4 near squares to any given point.
         // on the pole itself, this is short 1, and .s = -1
		} near_area[HEX_SIZE+1][HEX_SIZE+1][4]; // the 4 corners that are this hex square.
	} patches[3];
	pole()
	{
		{
			int p;
			int level;
			int col;
			int bLog = 0;
#ifdef POLE_NEAR_AREA_DEBUG
			bLog = 1;
#endif

			{
				int s, x, y;
				for( s = 0; s < 3; s++ )
				{
					for( x = 0; x < HEX_SIZE; x++ )
						for( y = 0; y < HEX_SIZE; y++ )
						{
							if( x < (HEX_SIZE-1) )
							{
								bodymap.near_areas[s][x][y][0].s = s;
								bodymap.near_areas[s][x][y][0].x = x + 1;
								bodymap.near_areas[s][x][y][0].y = y;

								bodymap.near_areas[s+9][x][y][0].s = bodymap.near_areas[s][x][y][0].s + 9;
								bodymap.near_areas[s+9][x][y][0].x = bodymap.near_areas[s][x][y][0].x;
								bodymap.near_areas[s+9][x][y][0].y = bodymap.near_areas[s][x][y][0].y;
							}
							else
							{
								bodymap.near_areas[s][x][y][0].s = s+3;
								bodymap.near_areas[s][x][y][0].x = x;
								bodymap.near_areas[s][x][y][0].y = 0;
								bodymap.near_areas[s+9][x][y][0].s = bodymap.near_areas[s][x][y][0].s + 3;
								bodymap.near_areas[s+9][x][y][0].x = bodymap.near_areas[s][x][y][0].x;
								bodymap.near_areas[s+9][x][y][0].y = HEX_SIZE-1;
							}
							if( y < (HEX_SIZE-1) )
							{
								bodymap.near_areas[s][x][y][1].s = s;
								bodymap.near_areas[s][x][y][1].x = x;
								bodymap.near_areas[s][x][y][1].y = y + 1;

								bodymap.near_areas[s+9][x][y][1].s = bodymap.near_areas[s][x][y][1].s + 9;
								bodymap.near_areas[s+9][x][y][1].x = bodymap.near_areas[s][x][y][1].x;
								bodymap.near_areas[s+9][x][y][1].y = bodymap.near_areas[s][x][y][1].y;
							}
							else
							{
								bodymap.near_areas[s][x][y][1].s = s+4;
								bodymap.near_areas[s][x][y][1].x = y;
								bodymap.near_areas[s][x][y][1].y = 0;

								bodymap.near_areas[s+9][x][y][0].s = bodymap.near_areas[s][x][y][0].s + 4;
								bodymap.near_areas[s+9][x][y][0].x = bodymap.near_areas[s][x][y][0].x;
								bodymap.near_areas[s+9][x][y][0].y = HEX_SIZE-1;
							}
							if( x )
							{
								bodymap.near_areas[s][x][y][2].s = s;
								bodymap.near_areas[s][x][y][2].x = x - 1;
								bodymap.near_areas[s][x][y][2].y = y;

								bodymap.near_areas[s+9][x][y][2].s = bodymap.near_areas[s][x][y][2].s + 9;
								bodymap.near_areas[s+9][x][y][2].x = bodymap.near_areas[s][x][y][2].x;
								bodymap.near_areas[s+9][x][y][2].y = bodymap.near_areas[s][x][y][2].y;
							}
							else
							{
								bodymap.near_areas[s][x][y][2].s = (s+2)%3;
								bodymap.near_areas[s][x][y][2].x = y;
								bodymap.near_areas[s][x][y][2].y = 0;

								bodymap.near_areas[s+9][x][y][2].s = bodymap.near_areas[s][x][y][2].s+9;
								bodymap.near_areas[s+9][x][y][2].x = bodymap.near_areas[s][x][y][2].x;
								bodymap.near_areas[s+9][x][y][2].y = bodymap.near_areas[s][x][y][2].y;
							}
							if( y )
							{
								bodymap.near_areas[s][x][y][3].s = s;
								bodymap.near_areas[s][x][y][3].x = x;
								bodymap.near_areas[s][x][y][3].y = y - 1;

								bodymap.near_areas[s+9][x][y][3].s = bodymap.near_areas[s][x][y][3].s + 9;
								bodymap.near_areas[s+9][x][y][3].x = bodymap.near_areas[s][x][y][3].x;
								bodymap.near_areas[s+9][x][y][3].y = bodymap.near_areas[s][x][y][3].y;
							}
							else
							{
								bodymap.near_areas[s][x][y][3].s = (s+1)%3;
								bodymap.near_areas[s][x][y][3].x = 0;
								bodymap.near_areas[s][x][y][3].y = x;

								bodymap.near_areas[s+9][x][y][3].s = bodymap.near_areas[s][x][y][3].s+9;
								bodymap.near_areas[s+9][x][y][3].x = bodymap.near_areas[s][x][y][3].x;
								bodymap.near_areas[s+9][x][y][3].y = bodymap.near_areas[s][x][y][3].y;
							}
						}
				}
			}


			for( p = 0; p < 3; p++ )
					{
						patches[p].near_area[0][0][0].s = 0;
						patches[p].near_area[0][0][0].x = 0;
						patches[p].near_area[0][0][0].y = 0;
						patches[p].near_area[0][0][1].s = 1;
						patches[p].near_area[0][0][1].x = 0;
						patches[p].near_area[0][0][1].y = 0;
						patches[p].near_area[0][0][2].s = 2;
						patches[p].near_area[0][0][2].x = 0;
						patches[p].near_area[0][0][2].y = 0;
						patches[p].near_area[0][0][3].s = -1;
						patches[p].near_area[0][0][3].x = 0;
						patches[p].near_area[0][0][3].y = 0;
						continue;
					}
			for( p = 0; p < 3; p++ )
			{
				for( level = 1; level <= HEX_SIZE; level++ )
				{
					for( col = 0; col < level; col++ )
					{
						int x, y;
						int ax, ay;

						// square 1, upper left, considering that up and left are totally relative 
						// beyond the first square.

						ConvertPolarToRect( level, col, &ax, &ay );
						if(bLog)lprintf( WIDE("level %d,%d is base square %d,%d"), level, col, ax, ay );
						if( level < (HEX_SIZE-1) )
						{
							ConvertPolarToRect( level, col, &x, &y );
							patches[p].near_area[x][y][0].s = p;
							patches[p].near_area[x][y][0].x = x;
							patches[p].near_area[ax][ay][0].y = y;
							if(bLog)lprintf( WIDE("level %d col %d is near (0) %d,%d,%d"), level, col, p, x, y );

							ConvertPolarToRect( level-1, col, &x, &y );
							patches[p].near_area[ax][ay][1].s = p;
							patches[p].near_area[ax][ay][1].x = x;
							patches[p].near_area[ax][ay][1].y = y;
							if(bLog)lprintf( WIDE("level %d col %d is near (1) %d,%d,%d"), level, col, p, x, y );

							if( col )
							{
								// level will be more than 1 if col is more than 1
								ConvertPolarToRect( level-1, col-1
									, &x
									, &y );
								patches[p].near_area[ax][ay][2].s = p;
								patches[p].near_area[ax][ay][2].x = x;
								patches[p].near_area[ax][ay][2].y = y;
								if(bLog)lprintf( WIDE("level %d col %d is near (2) %d,%d"), level, col, x, y );
								if( col == (level*2) )
								{	
									ConvertPolarToRect( level-1, 0, &x, &y );
									patches[p].near_area[ax][ay][3].s = (p+1)%3;
									patches[p].near_area[ax][ay][3].x = y;
									patches[p].near_area[ax][ay][3].y = x;
									if(bLog)lprintf( WIDE("level %d col %d is near (3) %d,%d"), level, col, x, y );
								}
								else
								{
									ConvertPolarToRect( level, col-1
										, &x
										, &y );
									patches[p].near_area[ax][ay][3].s = p;
									patches[p].near_area[ax][ay][3].x = x;
									patches[p].near_area[ax][ay][3].y = y;
									if(bLog)lprintf( WIDE("level %d col %d is near (3) %d,%d"), level, col, x, y );
								}
							}
							else
							{
								ConvertPolarToRect( level-1, (level-1)*2, &x, &y );
								patches[p].near_area[ax][ay][2].s = (p+2)%3;
								patches[p].near_area[ax][ay][2].x = x;
								patches[p].near_area[ax][ay][2].y = y;
								if(bLog)lprintf( WIDE("level %d col %d is near (2) %d,%d,%d"), level, col, (p+2)%3, x, y );

								ConvertPolarToRect( level, (level)*2, &x, &y );
								patches[p].near_area[ax][ay][3].s = (p+2)%3;
								patches[p].near_area[ax][ay][3].x = x;
								patches[p].near_area[ax][ay][3].y = y;
								if(bLog)lprintf( WIDE("level %d col %d is near (3) %d,%d,%d"), level, col, (p+2)%3, x, y );
							}

							}
							else
							{
								// outer ring...
							}
							
						}
					col = level;
					{
						int x, y;
						int ax, ay;
						ConvertPolarToRect( level, col, &ax, &ay );
#ifdef POLE_NEAR_AREA_DEBUG
						lprintf( WIDE("level %d,%d is base square %d,%d"), level, col, ax, ay );
#endif
							ConvertPolarToRect( level, col, &x, &y );
							patches[p].near_area[ax][ay][0].s = p;
							patches[p].near_area[ax][ay][0].x = x;
							patches[p].near_area[ax][ay][0].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (0) %d,%d"), level, col, x, y );
#endif
							ConvertPolarToRect( level, col+1, &x, &y );
							patches[p].near_area[ax][ay][1].s = p;
							patches[p].near_area[ax][ay][1].x = x;
							patches[p].near_area[ax][ay][1].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (1) %d,%d"), level, col, x, y );
#endif
							ConvertPolarToRect( level-1, col-1, &x, &y );
							patches[p].near_area[ax][ay][2].s = p;
							patches[p].near_area[ax][ay][2].x = x;
							patches[p].near_area[ax][ay][2].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (2) %d,%d"), level, col, x, y );
#endif
							ConvertPolarToRect( level, col-1, &x, &y );
							patches[p].near_area[ax][ay][3].s = p;
							patches[p].near_area[ax][ay][3].x = x;
							patches[p].near_area[ax][ay][3].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (3) %d,%d"), level, col, x, y );
#endif
					}
					for( col = level+1; col <= level*2; col++ )
					{
						int x, y;
						int ax, ay;
						ConvertPolarToRect( level, col, &ax, &ay );
#ifdef POLE_NEAR_AREA_DEBUG
						lprintf( WIDE("level %d,%d is base square %d,%d"), level, col, ax, ay );
#endif
						// square 1, upper left, considering that up and left are totally relative 
						// beyond the first square.

						if( level < (HEX_SIZE-1) )
						{
							ConvertPolarToRect( level, col, &x, &y );
							patches[p].near_area[ax][ay][0].s = p;
							patches[p].near_area[ax][ay][0].x = x;
							patches[p].near_area[ax][ay][0].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (0) %d,%d"), level, col, x, y );
#endif

							ConvertPolarToRect( level-1, col-2, &x, &y );
							patches[p].near_area[ax][ay][3].s = p;
							patches[p].near_area[ax][ay][3].x = x;
							patches[p].near_area[ax][ay][3].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (3) %d,%d"), level, col, x, y );
#endif

							if( col != level*2 )
							{
								// level will be more than 1 if col is more than 1
								ConvertPolarToRect( level, col + 1
									, &x
									, &y );
								patches[p].near_area[ax][ay][1].s = p;
								patches[p].near_area[ax][ay][1].x = x;
								patches[p].near_area[ax][ay][1].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (1) %d,%d"), level, col, x, y );
#endif
								ConvertPolarToRect( level-1, col -1
										, &x
										, &y );
								patches[p].near_area[ax][ay][2].s = p;
								patches[p].near_area[ax][ay][2].x = x;
								patches[p].near_area[ax][ay][2].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (2) %d,%d"), level, col, x, y );
#endif
							}
							else
							{
								ConvertPolarToRect( level, 0
									, &x
									, &y );

								patches[p].near_area[ax][ay][1].s = (p+1)%3;
								patches[p].near_area[ax][ay][1].x = x;
								patches[p].near_area[ax][ay][1].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
							lprintf( WIDE("level %d col %d is near (1) %d,%d,%d"), level, col, (p+1)%3, x, y );
#endif

								ConvertPolarToRect( level-1, 0
									, &x
									, &y );
								patches[p].near_area[ax][ay][2].s = (p+1)%3;
								patches[p].near_area[ax][ay][2].x = x;
								patches[p].near_area[ax][ay][2].y = y;
#ifdef POLE_NEAR_AREA_DEBUG
								lprintf( WIDE("level %d col %d is near (2) %d,%d,%d"), level, col, (p+1)%3, x, y );
#endif
							}
							}
							else
							{
								// outer ring...
							}
							/*
							patches[p].near_area[ax][ay][0].s = p;
							patches[p].near_area[ax][ay][0].x = x;
							patches[p].near_area[ax][ay][0].y = y;
							patches[p].near_area[ax][ay][1].s = p;
							patches[p].near_area[ax][ay][1].x = x;
							patches[p].near_area[ax][ay][1].y = y;
							patches[p].near_area[ax][ay][2].s = p;
							patches[p].near_area[ax][ay][2].x = x;
							patches[p].near_area[ax][ay][2].y = y;
							patches[p].near_area[ax][ay][3].s = p;
							patches[p].near_area[ax][ay][3].x = x;
							patches[p].near_area[ax][ay][3].y = y;
							*/
						}
					}
				}
					}
			//}
	
			{
				{
					int level;
					int col;
					for( level = 0; level < HEX_SIZE; level++ )
						for( col = 0; col <= level*2; col++ )
						{
							int x, y;
							ConvertPolarToRect( level, col, &x, &y );
							corners[x][y][0] = x;
							corners[x][y][1] = HEX_SIZE + x;
							corners[x][y][2] = HEX_SIZE + x + 1;
							corners[x][y][3] = x + 1;
						}
				}
		}
		{
			static PTRANSFORM work;
			VECTOR patch1, patch1x;
			int level;
			int c;
			int patch;
			if( !work )
				work = CreateTransform();
			else
				return;
			//__try
			{
				for( patch = 0; patch < 3; patch++ )
				{
					for( level = 0; level <= HEX_SIZE; level++ )
					{
						RCOORD patch_bias = -((patch*120) * M_PI ) / 180;
						int sections = ( level * 2 );
						RotateAbs( work, 0, 0, ((((60.0/HEX_SIZE)*level))*(1*M_PI))/180.0 );
						GetAxisV( work, patch1x, vUp );
						//Apply( work, patch1x, ref_point );
						if( !sections ) // level 0
						{
							SetPoint( patches[patch].grid[0][0], patch1x );
							continue;
						}
						//use common convert sphere to hex...
						for( c = 0; c <= sections; c++ )
						{
							int x, y;
							ConvertPolarToRect( level, c, &x, &y );
							RotateAbs( work, 0, patch_bias - ((120.0*c)*(1*M_PI))/((sections)*180.0), 0 );
							Apply( work, patch1, patch1x );
							SetPoint( patches[patch].grid[x][y], patch1 );
						}
					}
				}
			}
			//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") ); }
		}
	}

} pole_patch;



int mating_edges[6][2] = { { 0, 1 }, { 2, 1 }, { 2, 0 }
								 , { 1, 0 }, { 1, 2 }, { 0, 2 } };



int bwait_move_gliders = 1;
	PLIST gliders = NULL;
	PLIST cycles = NULL;
	PLIST bodies = NULL;

class Radar 
	{
	// need to find the defualts for the brain.ss
	NATIVE power; // 0-256 I guess... 
	NATIVE turn;
	NATIVE current_angle;
	NATIVE nearest;
	connector *motion[4];
	PBRAIN_STEM brainstem; // connectors to the body
	void Tick( void )
	{
		//DebugBreak();
		current_angle += ( turn / 8.0 );
		if( current_angle > 256.0 )
			current_angle -= 512.0;
		if( current_angle < -256.0 )
			current_angle += 512.0;
	}
	void CreateRadar( PBRAIN_STEM add_to_brainstem )
	{
		static int n;
		TEXTCHAR buffer[16];

		/* make a timer to call tick callback with this parameter....*/
		/* this is a total hack, and works under MSVC only, so far. */
		{
#ifdef __WATCOMC__
#define __thiscall
#endif
			union {
				void  (__thiscall Radar::*Tickx)( void );
				void (CPROC *Ticky)( PTRSZVAL );
			} cast_f;
			cast_f.Tickx = &Radar::Tick;
			AddTimer( 10, cast_f.Ticky, (PTRSZVAL)this );
		}



		if( add_to_brainstem )
		{
			int n;
			PBRAIN_STEM check_stem;
			for( n = 0; n < 24; n++ )
			{
				snprintf( buffer, sizeof( buffer ), WIDE("radar %d"), n+1 );
				for( check_stem = add_to_brainstem->first_module(); 
						check_stem; 
						check_stem = add_to_brainstem->next_module() )
				{
					if( StrCmp( check_stem->name(), buffer ) == 0 )
						break;
				}
				if( !check_stem )
					break;
			}
		}
		else
		{
			snprintf( buffer, sizeof( buffer ), WIDE("radar %d"), ++n );
		}
		brainstem = new BRAIN_STEM( buffer );
		power = 256;
		motion[0] = new connector( WIDE("power"), &power );
		turn = 0;
		motion[1] = new connector( WIDE("turn"), &turn );
		current_angle = 0;
		motion[2] = new connector( WIDE("angle"), &current_angle );
		nearest = 0;
		motion[3] = new connector( WIDE("nearest"), &nearest );
		brainstem->AddOutput( motion[0] );
		brainstem->AddOutput( motion[1] );
		brainstem->AddInput( motion[2] );
		brainstem->AddInput( motion[3] );
		if( add_to_brainstem )
			add_to_brainstem->AddModule( brainstem );
	}
public:
	Radar( PBRAIN_STEM add_to_brainstem )
	{
		CreateRadar( add_to_brainstem );
	}
	Radar()
	{
		CreateRadar( NULL );
	}
};


struct position_history {
	_POINT origin;
	_POINT up;
	_POINT right;
};


	void SphereBodyCorrect( PTRSZVAL psv, PTRANSFORM pt );


typedef class Body BODY, *PBODY;
class Body
{
public:
	DeclareLink( class Body );
	POBJECT object;
	PBRAIN_STEM brainstem; // connectors to the body
	PBRAIN brain;
	PBRAINBOARD board; 

	// these are 256 to 0 range.
	NATIVE speed, rotation; 
	NATIVE own_speed, own_rotation; 
	PCONNECTOR motion[2];
	Radar *radars[3]; // need at least 3... offset on body.
	Body( POBJECT body_object )
	{
		int n;
		next = NULL;
		me = NULL;
		board = NULL;
		object = body_object;

		speed = 64; // quarter speed, but initialized.
		rotation = 64;

		own_speed = 10;
		own_rotation = 2;
		AddTransformCallback( object->Ti, SphereBodyCorrect, (PTRSZVAL)this );
		brainstem = new BRAIN_STEM( WIDE("body") );
		motion[0] = new CONNECTOR( WIDE("forward"), &speed );
		motion[1] = new CONNECTOR( WIDE("turn"), &rotation );
		brainstem->AddOutput( motion[0] );
		brainstem->AddOutput( motion[1] );
		for( n = 0; n < 3; n++ )
			radars[n] = new Radar( brainstem );
		brain = new BRAIN( brainstem );
	}

	void CorrectPosition( void )
	{
      /* update position, already must be setup...*/
		//sack::math::vector::Move( object->Ti );
      /* correct position to sphere boundry */
		{
			VECTOR vertical;
			GetOriginV( object->Ti, vertical );
			Invert( vertical );
			RotateMast( object->Ti, vertical );
			if( !Near( vertical, VectorConst_0 ) )
			{
				VECTOR force;
				normalize( vertical );
				scale( vertical, vertical, (-SPHERE_SIZE*0.99) );
				sub( force, vertical, GetOrigin( object->Ti ) );
				Invert( force );
				//if( Length( force ) > 1.0 )
				//	DebugBreak();
				//lprintf( WIDE("Force vector is %g"), Length( force ) );
				{
					_POINT v;
					
					//add( v, GetSpeed( object->Ti, v ), force );
					//SetSpeed( object->Ti, v );
				}
				TranslateV( object->Ti, vertical );
				{
					int s, x, y;
					ConvertToSphereGrid( vertical, &s, &x, &y );
					//PrintVector( vertical );
					//lprintf( WIDE("Body at sphere part %d,%d,%d"), s, x, y );
					UnlinkThing( this );
					LinkThing( bodymap.band[s][x][y], this );
				}
			}
		}
	}


	virtual	void Draw( void )
	{
	}

};

// C interface callback..
	void SphereBodyCorrect( PTRSZVAL psv, PTRANSFORM pt )
	{
		class Body *body = (class Body*)psv;
		body->CorrectPosition();
	}

void RandomTurn( PTRSZVAL psv, PTRANSFORM pt );

class Glider:public Body
{
public:
	Glider( POBJECT object ):Body(object)
	{
		AddTransformCallback( object->Ti, RandomTurn, (PTRSZVAL)this );
	}
	void Move( void )
	{
		//Body::Move();
		if( ( ( rand() & 0xFF ) > 0xF0 ) )
		{

			if( ( rand() & 0xFF ) > 0x80 )
			{
				RotateAroundMast( object->Ti, (float)rand() / 600000.0 );
			}
			else
				RotateAroundMast( object->Ti, (float)rand() / -600000.0 );
		}
	}
};

void RandomTurn( PTRSZVAL psv, PTRANSFORM pt )
{
	class Glider *glider = (class Glider*)psv;
	glider->Move();
}

void InvokeCycleMove( PTRSZVAL psv, PTRANSFORM pt );

class CyberCycle:public Body
{
	struct {
		BIT_FIELD turn_left : 1;
		BIT_FIELD turn_right : 1;
		BIT_FIELD turned_left : 1;
		BIT_FIELD turned_right : 1;
		BIT_FIELD started : 1;
	} flags;
	_32 portion_to_turn; // 360/portion
	PCONNECTOR motion[2];
	struct position_history pos;
	PDATAQUEUE position_history; // struct position_history queue
public:
	CyberCycle( POBJECT body_object ):Body(body_object)
	{
      //DebugBreak();
      position_history = CreateLargeDataQueueEx( sizeof( struct position_history ), 500, 2500 DBG_SRC );
		//motion[0] = new CONNECTOR( WIDE("right"), &speed );
		//motion[1] = new CONNECTOR( WIDE("left"), &rotation );
      //motion[2] = new CONNECTOR( WIDE("forward_distance"), &rotation );
	  AddTransformCallback( body_object->Ti, InvokeCycleMove, (PTRSZVAL)this );
	}
	void Reset( void )
	{
		flags.started = 0;
      //
		//EmptyDataQueue( position_history );
		//
      position_history->Top = position_history->Bottom = 0;
	}
	void RightTurnClyde( int yes )
	{
		if( yes )
		{
			if( !flags.turn_right )
				flags.turn_right = 1;
		}
		else
			if( flags.turn_right )
			{
            flags.turned_right = 0;
				flags.turn_right = 0;
			}
	}
	void LeftTurnClyde( int yes )
	{
		if( yes )
		{
			if( !flags.turn_left )
				flags.turn_left = 1;
		}
		else
			if( flags.turn_left )
			{
				flags.turned_left = 0;
				flags.turn_left = 0;
			}
	}
	void Move( void )
	{
		/* override default move so we can look at left/right turn status */
		if( ( ( rand() & 0xFF ) > 0xFE ) )
		{
			if( ( rand() & 0xFF ) > 0x80 )
			{
				LeftTurnClyde( TRUE );
			}
			else
				RightTurnClyde( TRUE );
		}
		else
		{
			LeftTurnClyde( 0 );
			RightTurnClyde(0);
		}
		if( flags.started )
		{
			//PrintVector( pos.origin );
			EnqueData( &position_history, &pos );
		}
		if( flags.turn_left && !flags.turned_left )
		{
			//RotateAroundMast( object->Ti, -M_PI/2 );
			RotateAroundMast( object->Ti, -M_PI/10 );
		}
		if( flags.turn_right && !flags.turned_right )
		{
			//RotateAroundMast( object->Ti, M_PI/2 );
			RotateAroundMast( object->Ti, M_PI/10 );
		}
		own_speed = ( PLANET_CIRCUM / (HEX_SIZE * 6) )  / 10;
		//Forward( object->Ti, 100.0 );

		GetOriginV( object->Ti, pos.origin );
		GetAxisV( object->Ti, pos.up, vUp );
		GetAxisV( object->Ti, pos.right, vRight );
		flags.started = 1;
	}
	void Draw( void )
	{
		if( flags.started )
		{
			struct position_history curpos;
			INDEX idx = 0;
			VECTOR v;
			// draw the wall-strip
			glBegin( GL_QUAD_STRIP );
			glColor3d( 1.0, 0.0, 1.0 );
			for( idx = 256;PeekDataQueueEx( &position_history, struct position_history, &curpos, idx ); ) 
				DequeData( &position_history, NULL );
			for( idx = 0; PeekDataQueueEx( &position_history, struct position_history, &curpos, idx ); idx++ )
			{
				//PrintVector( curpos.origin );
				glVertex3dv( curpos.origin );
				addscaled( v, curpos.origin, curpos.up, 20.0 );
				//PrintVector( v );
				glVertex3dv( v );
			}
			//PrintVector( pos.origin );
			glVertex3dv( pos.origin );
			addscaled( v, pos.origin, pos.up, 20.0 );
			glVertex3dv( v );
			glEnd();
         /*
			glBegin( GL_LINE_STRIP );
         glColor3d( 1.0, 1.0, 1.0 );
			for( idx = 0; PeekDataQueueEx( &position_history, &curpos, idx ); idx++ )
			{
            glVertex3dv( curpos.origin );
			}
			glVertex3dv( pos.origin );
			glEnd();
         */
		}
	}
};

void InvokeCycleMove( PTRSZVAL psv, PTRANSFORM pt )
{
	class CyberCycle *cycle = (class CyberCycle *)psv;
	cycle->Move();
}

// if not north, then south.
int RenderPolePatch( PHEXPATCH patch, int north )
{
	int s, level, c, r;

	int x, y; // used to reference patch level
	//int x2, y2;
	int bLog = 0;
#ifdef DEBUG_RENDER_POLE_NEARNESS
	bLog = 1;
#endif
	// create array of normals.
	//__try
	{
		for( s = 0; s < 3; s++ )
		{
			for( level = 1; level <= HEX_SIZE; level++ )
			{
				VECTOR v1;
				//if( bLog )lprintf( WIDE("---------") );
				//if( 0)  {
				glBegin( GL_TRIANGLE_STRIP );
				r = 0;
				for( c = 0; c <= level; c++ )
				{
					int bodynear = 0;
					int bodynear2 = 0;
					int bodynearnear = 0;
					int bodynearnear2 = 0;
					ConvertPolarToRect( level, c, &x, &y );
					// consider removign the same function used below.
					{
						int n;
						for( n = 0; n < 4; n++ )
						{
							int m;
							int s2, x2, y2;
							s2 = pole_patch.patches[s].near_area[x][y][n].s + (north*9);
							x2 = pole_patch.patches[s].near_area[x][y][n].x;
							y2 = pole_patch.patches[s].near_area[x][y][n].y;

							if( bLog )lprintf( WIDE("point %d,%d,%d,%d is near %d,%d,%d")
								, s, level, c, n
								, pole_patch.patches[s].near_area[x][y][n].s
								, pole_patch.patches[s].near_area[x][y][n].x
								, pole_patch.patches[s].near_area[x][y][n].y 
								);

							//if( bodymap.band[pole_patch.patches[s].near_area[x][y][n].s+(north*10)]
							//		[pole_patch.patches[s].near_area[x][y][n].x]
							//		[pole_patch.patches[s].near_area[x][y][n].y] )
							if( s2 == cur_s &&
								x2 == cur_x &&
								y2 == cur_y )
							{
								bodynear++;
							}
							else for( m = 0; m < 4; m++ )
							{
								if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								{
									bodynearnear++;
								}
							}

						}
					}
					if( bodynear )
					{
						if( bLog )lprintf( WIDE("Is bodynear") );
						glColor3d( 1.0, 1.0, 1.0 );
					}
					else if( bodynearnear )
					{
						if( bLog )lprintf( WIDE("Is bodynearnear") );
						glColor3d( 0.4, 0, 0 );
					}
					else
					{
						switch( s )
						{
						case 0:
							glColor3d( 1.0, 0.0, 0.0 );
							break;
						case 1:
							glColor3d( 0.0, 1.0, 0.0 );
							break;
						case 2:
							glColor3d( 0.0, 0.0, 10 );
							break;
						}
						/*
						if( c & 1 )
						glColor3d( 1.0, 0.0, 0.0 );
						else
						glColor3d( 0.0, 1.0, 0.0 );
						*/
						//glColor3d( 0,0,0);
					}
					scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
					glVertex3d( v1[0]
					, north?v1[1]:-v1[1]
					, v1[2]
					);
					if( c < (level) )
					{
						int bodynear2 = 0;
						int bodynearnear2 = 0;
						int n;
						ConvertPolarToRect( level-1, c, &x, &y );
						for( n = 0; n < 4; n++ )
						{
							int m;
							int s2, x2, y2;
							//if( bodymap.band[pole_patch.patches[s].near_area[x][y][n].s+(north*10)]
							//		[pole_patch.patches[s].near_area[x][y][n].x]
							//		[pole_patch.patches[s].near_area[x][y][n].y] )
							if( pole_patch.patches[s].near_area[x][y][n].s < 0 )
								continue;
							if( bLog )lprintf( WIDE("point %d,%d,%d,%d is near %d,%d,%d")
								, s, level-1, c, n
								, pole_patch.patches[s].near_area[x][y][n].s
								, pole_patch.patches[s].near_area[x][y][n].x
								, pole_patch.patches[s].near_area[x][y][n].y 
								);
							s2 = pole_patch.patches[s].near_area[x][y][n].s + (north*9);
							x2 = pole_patch.patches[s].near_area[x][y][n].x;
							y2 = pole_patch.patches[s].near_area[x][y][n].y;
							if( s2 == cur_s &&
								x2 == cur_x &&
								y2 == cur_y )
								bodynear2++;
							else for( m = 0; m < 4; m++ )
							{
								if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								{
									bodynearnear2++;
								}
							}
						}
						if( bodynear2 )
						{
							if( bLog )lprintf( WIDE("Is bodynear") );
							glColor3d( 1.0, 1.0, 1.0 );
						}
						else if( bodynearnear2 )
						{
							if( bLog )lprintf( WIDE("Is bodynearnearnear") );
							glColor3d( 0.4, 0.0, 0.0 );
						}
						else
						{
							if( c & 1 )
								glColor3d( 0.9, 0.0, 1.0 );
							else
								glColor3d( 0.0, 0.9, 1.0 );
							//glColor3d( 0,0,0);
						}
						scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
						glVertex3d( v1[0]
						, north?v1[1]:-v1[1]
						, v1[2]
						);
					}
					r++;
				}
				glEnd();
				//}
				if( bLog )lprintf( WIDE("---------") );
				//glFlush();
				glBegin( GL_TRIANGLE_STRIP );
				for( c = level; c <= level*2; c++ )
				{
					{
						int bodynear = 0;
						int bodynearnear = 0;
						int n;
						ConvertPolarToRect( level, c, &x, &y );
						for( n = 0; n < 4; n++ )
						{
							int m;
							int s2, x2, y2;
							if( bLog )lprintf( WIDE("zzzpoint %d,%d,%d,%d is near %d,%d,%d")
								, s, level, c, n
								, pole_patch.patches[s].near_area[x][y][n].s
								, pole_patch.patches[s].near_area[x][y][n].x
								, pole_patch.patches[s].near_area[x][y][n].y 
								);
							s2 = pole_patch.patches[s].near_area[x][y][n].s+(north*9);
							x2 = pole_patch.patches[s].near_area[x][y][n].x;
							y2 = pole_patch.patches[s].near_area[x][y][n].y;
							//if( bodymap.band[pole_patch.patches[s].near_area[x][y][n].s+(north*10)]
							//		[pole_patch.patches[s].near_area[x][y][n].x]
							//		[pole_patch.patches[s].near_area[x][y][n].y] )
							if( s2 == cur_s &&
								x2 == cur_x &&
								y2 == cur_y )
								bodynear++;
							else for( m = 0; m < 4; m++ )
							{
								if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								{
									bodynearnear++;
								}
							}
						}
						if( bodynear )
						{
							if( bLog )lprintf( WIDE("Is bodynear") );
							glColor3d( 1.0, 1.0, 1.0 );
						}
						else if( bodynearnear )
						{
							if( bLog )lprintf( WIDE("Is bodynearnear") );
							glColor3d( 0.4, 0.0, 0.0 );
						}
						else
						{
							if( c & 1 )
								glColor3d( 0.8, 0.0, 0.0 );
							else
								glColor3d( 0.0, 0.8, 0.0 );
							//glColor3d( 0,0,0);
						}
					}
					//ConvertPolarToRect( level, c, &x, &y );
					//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c,level);
					scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
					glVertex3d( v1[0]
					, north?v1[1]:-v1[1]
					, v1[2]
					);
					if( c < (level)*2 )
				 {
					 {
						 int bodynear = 0;
						 int bodynearnear = 0;
						 int n;
						 ConvertPolarToRect( level-1, c-1, &x, &y );
						 for( n = 0; n < 4; n++ )
						 {
							 int m;
							 int s2, x2, y2;
							 if( (pole_patch.patches[s].near_area[x][y][n].s) < 0 )
								 continue;
							 if( bLog )lprintf( WIDE("point %d,%d,%d,%d is near %d,%d,%d")
								 , s, level-1, c-1, n
								 , pole_patch.patches[s].near_area[x][y][n].s
								 , pole_patch.patches[s].near_area[x][y][n].x
								 , pole_patch.patches[s].near_area[x][y][n].y 
								 );
							 s2 = pole_patch.patches[s].near_area[x][y][n].s+(north*9);
							 x2 = pole_patch.patches[s].near_area[x][y][n].x;
							 y2 = pole_patch.patches[s].near_area[x][y][n].y;
							 if( s2 == cur_s &&
								 x2 == cur_x &&
								 y2 == cur_y )
								 //if( bodymap.band[s2]
								 //  	[pole_patch.patches[s].near_area[x][y][n].x]
								 //		[pole_patch.patches[s].near_area[x][y][n].y] )
								 bodynear++;
							 else for( m = 0; m < 4; m++ )
							 {
								 if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									 bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									 bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								 {
									 bodynearnear++;
								 }
							 }
						 }
						 if( bodynear )
						 {
							 if( bLog )lprintf( WIDE("Is bodynear") );
							 glColor3f( 1.0,1.0,1.0 );
						 }
						 else if( bodynear )
						 {
							 if( bLog )lprintf( WIDE("Is bodynearnear") );
							 glColor3f( 0.5, 0.0, 0.0 );

						 }
						 else
						 {
							 if( c & 1 )
								 glColor3f( 1, 0.0, 1.0 );
							 else
								 glColor3f( 0.0, 1, 1.0 );
							 //glColor3d( 0,0,0);
						 }
					 }

					 //ConvertPolarToRect( level-1, c-1, &x, &y );
					 //if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c-1,level-1);
					 scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
					 glVertex3d( v1[0]
					 , north?v1[1]:-v1[1]
					 , v1[2]
					 );
				 }
				}
				glEnd();
			}
		}
	}
	//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") );return 0; }
	return 1;
}


void RenderBandPatch( void )
{
	int section, section2;
	int sections = 6*HEX_SIZE;
	int sections2= HEX_SIZE;
	VECTOR patch1, patch2;
	//VECTOR ref_point;
	//InitBandPatch();

	//scale( ref_point, VectorConst_X, SPHERE_SIZE );

	for( section2 = 0; section2 < sections2; section2++ )
	{
		glBegin( GL_TRIANGLE_STRIP );
		for( section = 0; section <= sections; section ++ )
		{
			int s = (section/HEX_SIZE)%6; // might be 6, which needs to be 0.
			scale( patch1, band.patches[s].grid[section%HEX_SIZE][section2], SPHERE_SIZE );
			scale( patch2, band.patches[s].grid[section%HEX_SIZE][section2+1], SPHERE_SIZE );

			{
				int bodynear = 0;
				int bodynearnear = 0;
				int bodynear2 = 0;
				int bodynearnear2 = 0;
				int n;
				//lprintf( WIDE("---------------------------------------------") );
				for( n = 0; n < 4; n++ )
				{
					/*
					lprintf( WIDE("%d,%d,%d is near %d,%d,%d"), s, section%HEX_SIZE, section2
					, band.patches[s].near_area[section%HEX_SIZE][section2][n].s
					, band.patches[s].near_area[section%HEX_SIZE][section2][n].x
					, band.patches[s].near_area[section%HEX_SIZE][section2][n].y
					);
					lprintf( WIDE("%d,%d,%d is near %d,%d,%d"), s, section, section2+1
					, band.patches[s].near_area[section%HEX_SIZE][section2+1][n].s
					, band.patches[s].near_area[section%HEX_SIZE][section2+1][n].x
					, band.patches[s].near_area[section%HEX_SIZE][section2+1][n].y
					);
					*/
					{
						int m;
						int s2, x2, y2;
						s2 = band.patches[s].near_area[section%HEX_SIZE][section2][n].s;
						x2 = band.patches[s].near_area[section%HEX_SIZE][section2][n].x;
						y2 = band.patches[s].near_area[section%HEX_SIZE][section2][n].y;
						if( s2 == cur_s &&
							x2 == cur_x &&
							y2 == cur_y )
							//if(
							//	bodymap.band[band.patches[s].near_area[section%HEX_SIZE][section2][n].s]
							//	[band.patches[s].near_area[section%HEX_SIZE][section2][n].x]
							//	[band.patches[s].near_area[section%HEX_SIZE][section2][n].y]
							//  )
						{
							bodynear++;
						}
						else for( m = 0; m < 4; m++ )
						{
							if( s2 > 2 && s2 < 9 )
								if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								{
									bodynearnear++;
								}
						}
					}
					{
						int m;
						int s2, x2, y2;
						s2 = band.patches[s].near_area[section%HEX_SIZE][section2+1][n].s;
						x2 = band.patches[s].near_area[section%HEX_SIZE][section2+1][n].x;
						y2 = band.patches[s].near_area[section%HEX_SIZE][section2+1][n].y;
						if( s2 == cur_s &&
							x2 == cur_x &&
							y2 == cur_y )
							//if(
							//	bodymap.band[band.patches[s].near_area[section%HEX_SIZE][section2+1][n].s]
							//	[band.patches[s].near_area[section%HEX_SIZE][section2+1][n].x]
							//	[band.patches[s].near_area[section%HEX_SIZE][section2+1][n].y]
							//  )
						{
							bodynear2++;
						}
						else for( m = 0; m < 4; m++ )
						{
							if( s2 > 2 && s2 < 9 )
								if( bodymap.near_areas[s2][x2][y2][m].s == cur_s &&
									bodymap.near_areas[s2][x2][y2][m].x == cur_x &&
									bodymap.near_areas[s2][x2][y2][m].y == cur_y )
								{
									bodynearnear2++;
								}
						}
					}
				}
				//lprintf( WIDE("---------------------------------------------") );

				if( bodynear )
				{
					glColor3f( 1.0, 1.0, 1.0 );
				}
				else if( bodynearnear )
				{
					glColor3f( 0.6, 0.6, 0.6 );
				}
				else
				{
					if( section2 & 1 )
					{
						if( section & 1 )
							glColor3f( 1.0//(float)section/(float)sections
							, 0.0, 0.0 );
						else
							glColor3f( 0.0, 1.0 //(float)section/(float)sections
							, 0.0 );
					}
					else
					{
						if( section & 1 )
							glColor3f( 1.0//(float)section/(float)sections
							, 1.0, 0.0 );
						else
							glColor3f( 1.0, 1.0 //(float)section/(float)sections
							, 0.0 );
					}
					//glColor3f( 0,0,0 );
				}
				glVertex3d( patch1[0]
				, patch1[1]
				, patch1[2] );
				if( bodynear2 )
				{
					glColor3f( 1.0, 1.0, 1.0 );
				}
				else if( bodynearnear2 )
				{
					glColor3f( 0.6, 0.6, 0.6 );
				}
				else
				{
					if( !(section & 1 ) )
						glColor3f( 1.0//(float)section/(float)sections
						, 0.0, 1.0 );
					else
						glColor3f( 0.0, 1.0 //(float)section/(float)sections
						, 1.0 );
					//glColor3f( 0,0,0 );
				}
				glVertex3d( patch2[0]
				, patch2[1]
				, patch2[2] );
			}
		}
		glEnd();
	}
}

int DrawSphereThing( PHEXPATCH patch )
{
	//which is a 1x1x1 sort of patch array
	RenderBandPatch();
	//glFlush();
	//glFlush();
	if( !RenderPolePatch( patch, 0 ) )
		return 0;
	if( !RenderPolePatch( patch, 1 ) )
		return 0;
	//glFlush();
	return 1;
}

//void MatePoleEdge( PHEXPATCH patch, int section, PSQUAREPATCH square )
//{
//}


void MateEdge( PHEXPATCH patch, int section1, int section2 )
{
	int n;
   lprintf( WIDE("mating %d and %d"), section1, section2 );
	switch( section1 )
	{
	case 0:
		switch( section2 )
		{
		case 0:
			break;
		case 1:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][n][0] =
					patch->height[section1][n][HEX_SIZE-1];
			break;
		case 2:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][0][n] =
					patch->height[section1][HEX_SIZE-1][n];
         break;
		}
		break;
	case 1:
		switch( section2 )
		{
		case 0:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][n][HEX_SIZE-1] =
					patch->height[section1][n][0];
			break;
		case 1:
			break;
		case 2:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][n][HEX_SIZE-1] =
					patch->height[section1][HEX_SIZE-1][n];
         break;
		}
		break;
	case 2:
		switch( section2 )
		{
		case 0:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][HEX_SIZE-1][n] =
					patch->height[section1][0][n];
			break;
		case 1:
			for( n = 0; n < HEX_SIZE; n++ )
				patch->height[section2][HEX_SIZE-1][n] =
					patch->height[section1][n][HEX_SIZE-1];
			break;
		case 2:
         break;
		}
	}
}



void MateAnotherEdge( PHEXPATCH patch1, PHEXPATCH patch2, int section1, int section2 )
{
	int n;
   lprintf( WIDE("mating %d and %d"), section1, section2 );
	switch( section1 )
	{
	case 0:
		switch( section2 )
		{
		case 0:
			break;
		case 1:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][n][0] =
					patch2->height[section2][n][HEX_SIZE-1];
			break;
		case 2:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][0][n] =
					patch2->height[section2][HEX_SIZE-1][n];
			break;
		}
		break;
	case 1:
		switch( section2 )
		{
		case 0:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][n][HEX_SIZE-1] =
					patch2->height[section2][n][0];
			break;
		case 1:
			break;
		case 2:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][0][n] =
					patch2->height[section2][n][0];
			break;
		}
      break;
	case 2:
		switch( section2 )
		{
		case 0:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][HEX_SIZE-1][n] =
					patch2->height[section2][0][n];
			break;
		case 1:
			for( n = 0; n < HEX_SIZE; n++ )
				patch1->height[section1][n][0] =
					patch2->height[section2][0][n];
			break;
		case 2:
			break;
		}
		break;
	}
}


PHEXPATCH CreatePatch( PHEXPATCH *nearpatches )
{
	int s;
   // create both polar patches for quick hack.
	PHEXPATCH patch = (PHEXPATCH)Allocate( sizeof( HEXPATCH ) * 2 );
	MemSet( patch, 0, sizeof( HEXPATCH ) * 2 );
   //SetPoint( patch->origin, VectorConst_0 );
	if( nearpatches )
	{
		for( s = 0; s < 6; s++ )
		{
			if( nearpatches[s] )
			{
				MateAnotherEdge( patch
									, nearpatches[s]
									, mating_edges[s][0]
									, mating_edges[s][1] );
			}
		}
	}
   return patch;
}

// only bodies within a certain grid portion need to be checked
// for_all_near_bodies...
// where near_area is for x-3, y-3 to x+3, y+3 and finding bodies in those sectors;

void ConvertToSphereGrid( P_POINT p, int *s, int *x, int *y ) // s 0-3, s 4-9, 10-12
{
   // vertical projection... soh cah toa, cah  cos of angle...
	normalize( p );
	(*x)=-1;
	(*y)=-1;
	//lprintf( WIDE("z is %g"), p[2] );
   (*s) = 0;
	if( p[vUp] < -0.5 )
	{
      //(*s) = 10;
		p[vUp] = -p[vUp];
	}
	else // north is 9+ // south is normal.
		(*s) = 9;
	if( p[vUp] > 0.5 )
	{
		int level;
		int r;
		int tmpx, tmpy;
		int _tmpx, _tmpy;
		VECTOR v;
		v[0] = p[0];
      v[1] = 0;
      v[2] = p[2];
		RCOORD len = Length( v );
/*
		if( *s )

			lprintf( WIDE("South pole") );
		else
			lprintf( WIDE("North pole") );
			*/
		//lprintf( WIDE("radial length: %g"), len );
		//PrintVector( p );
		for( level = 0; level <= HEX_SIZE; level++ )
		{
			//PrintVector( pole_patch.patches[0].grid[level][0] );
			if( len < -pole_patch.patches[0].grid[level][0][vRight] )
			{
				//lprintf( WIDE("Comparison is %g > %g at %d"), len, pole_patch.patches[0].grid[level][0][vRight], level );
				//lprintf( WIDE("result level is %d"), level );
				level = level-1;
				break;
			}
		}
		if( p[vForward] > 0 )
		{
			//lprintf( WIDE("forward from origin...") );
			if( p[vRight] > -0.5 )
			{
				for( r = 0; r <= (level*2); r++ )
				{
					ConvertPolarToRect( level, r, &tmpx, &tmpy );
					//PrintVector( pole_patch.patches[0].grid[tmpx][tmpy] );
					if( p[vRight] < pole_patch.patches[0].grid[tmpx][tmpy][vRight] )
					{
						break;
					}
					_tmpx = tmpx;
					_tmpy = tmpy;
				}
				if( r )
				{
					tmpx = _tmpx;
					tmpy = _tmpy;
				}
				(*s) += 0;
			}
			else
			{
				for( r = 0; r <= (level*2); r++ )
				{
					ConvertPolarToRect( level, r, &tmpx, &tmpy );
					if( p[vUp] > pole_patch.patches[1].grid[tmpx][tmpy][vUp] )
						break;
					_tmpx = tmpx;
					_tmpy = tmpy;
				}
				if( r )
				{
					tmpx = _tmpx;
					tmpy = _tmpy;
				}
				(*s) +=1;
			}
			(*x) = tmpx;
			(*y) = tmpy;
		}
		else
		{
			if( p[vRight] > -0.5 )
			{
				for( r = 0; r <= (level*2); r++ )
				{
					ConvertPolarToRect( level, r, &tmpx, &tmpy );
					if( p[vRight] > pole_patch.patches[0].grid[tmpx][tmpy][vRight] )
						break;
					_tmpx = tmpx;
					_tmpy = tmpy;
				}
				if( r )
				{
					tmpx = _tmpx;
					tmpy = _tmpy;
				}
				(*s) += 2;
			}
			else
			{
				for( r = 0; r <= (level*2); r++ )
				{
					ConvertPolarToRect( level, r, &tmpx, &tmpy );
					if( p[vUp] > pole_patch.patches[1].grid[tmpx][tmpy][vUp] )
						break;
					_tmpx = tmpx;
					_tmpy = tmpy;
				}
				if( r )
				{
					tmpx = _tmpx;
					tmpy = _tmpy;
				}
				(*s)+=1;
			}
			(*x) = tmpx;
			(*y) = tmpy;
		}
	}
	else
	{
		int level; // y level of band
		//lprintf( WIDE("equator") );
		//PrintVector( p );
		// turns out the the band coordinates are stored from top town... and that everything
      // we're viewing is upside down.
		for( level = 0; level < HEX_SIZE; level++ )
		{
			if( p[vUp] < band.patches[0].grid[0][level][vUp] )
			{
				break;
			}
		}
		(*y) = level-1;
		(*s) = 3;
		if( p[vRight] > 0.5 )
		{
			if( p[vForward] > 0 )
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					//PrintVector( band.patches[0].grid[n][level] );
					if( p[vForward] < band.patches[0].grid[n][level][vForward] )
					{
						break;
					}
				}
				(*x) = n--;
				(*s) += 0;
			}
			else
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					//PrintVector(band.patches[5].grid[n][level] );
					if( p[vForward] < band.patches[5].grid[n][level][vForward] )
						break;
				}
				(*x) = n-1;
				(*s) += 5;
			}
		}
		else if( p[vRight] < -0.5 )
		{
			if( p[vForward] > 0 )
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					if( p[vForward] > band.patches[2].grid[n][level][vForward] )
						break;
				}
				(*x) = n-1;
				(*s) += 2;
			}
			else
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					if( p[vForward] > band.patches[3].grid[n][level][vForward] )
						break;
				}
				(*x) = n-1;
				(*s) += 3;
			}
		}
		else
		{
			if( p[2] > 0 )
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					if( p[vRight] > band.patches[1].grid[n][level][vRight] )
					break;
				}
				(*x) = n-1;
				(*s) += 1;
			}
			else
			{
				int n;
				for( n = 0; n < HEX_SIZE; n++ )
				{
					//PrintVector( band.patches[4].grid[n][level] );
					if( p[vRight] < band.patches[4].grid[n][level][vRight] )
						break;
				}
				(*x) = n-1;
				(*s) += 4;
			}
		}
		//lprintf( WIDE("Middle Band") );
	}
   //lprintf( WIDE("Result %d,%d,%d"), (*s), (*x), (*y) );
   //   sqrt( p[0]*p[0]+p[2]*p[2] )
}

#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
int mode = MODE_UNKNOWN;

void BeginVisPersp( void )
{
	//if( mode != MODE_PERSP )
	{
		mode = MODE_PERSP;
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glLoadIdentity();									// Reset The Projection Matrix
		// Calculate The Aspect Ratio Of The Window
		gluPerspective(45.0f,1.0f,0.1f,10000.0f);
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		glLoadIdentity();									// Reset The Modelview Matrix
	}
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
		glClear(GL_COLOR_BUFFER_BIT
			| GL_DEPTH_BUFFER_BIT
			);	// Clear Screen And Depth Buffer
	BeginVisPersp();
	return TRUE;										// Initialization Went OK
}


int ShowPatch( PHEXPATCH patch )
{
	int x, y, s;
	if( !DrawSphereThing( patch ) )
		return 0;
	/*
	glBegin(GL_TRIANGLES);
	glColor4f( 1.0, 1.0, 1.0, 0.1 );
	glVertex3d( patch->origin[0] - 1 * HEX_HORZ_STRIDE
				 , patch->origin[1] + 1.2f * HEX_HORZ_STRIDE
				 , patch->origin[2] );
	glVertex3d( patch->origin[0] - 1  * HEX_HORZ_STRIDE
				 , patch->origin[1] - 1.2f * HEX_HORZ_STRIDE
				 , patch->origin[2] );
	glVertex3d( patch->origin[0] + 1  * HEX_HORZ_STRIDE
				 , patch->origin[1] - 0
				 , patch->origin[2] );
	glEnd();					// Done Drawing The Cube
	*/
	for( s = 0; s < 3; s++ )
//	for( s = 0; s < 2; s++ )
//   s=2;
	{
		for( x = 0; x < HEX_SIZE; x++ )
      //x = 0 ;
		{
			glBegin( GL_TRIANGLE_STRIP );
			// the way this works, Y will be represented as +1
			// make ssense? no... that is Y and Y+1 are alwasy
         // used, therefore at HEX_SIZE-1, HEX_SIZE-1+1 is invalid.
			for( y = 0; y < (HEX_SIZE-1); y++ )
			{
				switch( s )
				{
				case 0:
					//glBegin(GL_TRIANGLES);
					if( y == 0 )
					{
						glColor4f( 0.0, 1.0, 0.0, 0.5 );
						glVertex3d( patch->origin[0]
									  - (HEX_SIZE-x) * HEX_HORZ_STRIDE
									  + y * HEX_HORZ_STRIDE/2
									 , patch->origin[1]
									  - (-y) * HEX_VERT_STRIDE
									 , patch->origin[2]
									  - patch->height[s][x][y] );

						glColor4f( 0.0, 1.0, 1.0, 0.2 );
						glVertex3d( patch->origin[0]
									  - (HEX_SIZE-(x+1)) * HEX_HORZ_STRIDE
									  + y * HEX_HORZ_STRIDE/2
									 , patch->origin[1]
									  - (-y) * HEX_VERT_STRIDE
									 , patch->origin[2]
									  - ((x!=HEX_SIZE-1)
									  ? patch->height[s][(x+1)][y]
									  : patch->height[2][0][y]));
					}
					glColor4f( 0.0, 0.0, 1.0, 1.0 );
					glVertex3d( patch->origin[0]
								  - (HEX_SIZE-x) * HEX_HORZ_STRIDE
								  + (y+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  - (-(y+1)) * HEX_VERT_STRIDE
								 , patch->origin[2]
								  - patch->height[s][x][(y+1)] );
					glColor4f( 1.0, 0.0, 1.0, 0 );
					glVertex3d( patch->origin[0]
								  - (HEX_SIZE-(x+1)) * HEX_HORZ_STRIDE
								  + (y+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  - (-(y+1)) * HEX_VERT_STRIDE
								 , patch->origin[2]
								  - ((x!=HEX_SIZE-1)
								  ? patch->height[s][x+1][y+1]
								  : patch->height[2][0][y+1]));
					break;
				case 1:
					//glBegin(GL_TRIANGLES);
				//	lprintf( WIDE("uhh %g %g %g")
				//			 ,patch->origin[0] - (HEX_SIZE-x) * HEX_HORZ_STRIDE/2 - y * HEX_HORZ_STRIDE/2
				//			 , patch->origin[1] - (HEX_SIZE-y) * HEX_VERT_STRIDE
            //           , patch->origin[2] - patch->height[s][x][y] );
					//   tmp = patch->origin[0] - (HEX_SIZE-x) * HEX_HORZ_STRIDE - y * HEX_HORZ_STRIDE/2;
					//if( y == 0 )
					{
						glColor3f( 1.0, 0.0, 0.0 );
						glVertex3d( patch->origin[0]
									  - (HEX_SIZE-(x)) * HEX_HORZ_STRIDE
									  + y * HEX_HORZ_STRIDE/2
									 , patch->origin[1]
									  - (+y) * HEX_VERT_STRIDE
									 , patch->origin[2]
									  - patch->height[s][x][y] );
						glColor3f( 0.0, 0.0, 1.0 );
						glVertex3d( patch->origin[0]
									  - (HEX_SIZE-(x+1)) * HEX_HORZ_STRIDE
									  + y * HEX_HORZ_STRIDE/2
									 , patch->origin[1]
									  - (+y) * HEX_VERT_STRIDE
									 , patch->origin[2]
									  - ((x!=HEX_SIZE-1)
									  ? patch->height[s][x+1][y]
									  : patch->height[2][y][0])
									 );
					}
					glColor3f( 0.0, 1.0, 1.0 );
					glVertex3d( patch->origin[0]
								  - (HEX_SIZE-(x)) * HEX_HORZ_STRIDE
								  + (y+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  - (+y+1) * HEX_VERT_STRIDE
								 , patch->origin[2]
								  - patch->height[s][x][y+1] );
					glColor3f( 1.0, 1.0, 0.0 );
					glVertex3d( patch->origin[0]
								  - (HEX_SIZE-(x+1)) * HEX_HORZ_STRIDE
								  + (y+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  - (+(y+1)) * HEX_VERT_STRIDE
								 , patch->origin[2]
								  - ((x!=HEX_SIZE-1)
								  ? patch->height[s][x+1][(y+1)]
								  : patch->height[2][y+1][0])
								 );
               //glEnd();
					break;
				case 2:
//					glBegin(GL_TRIANGLES);
					//glColor3f( (1.0 * y) / HEX_SIZE, (1.0 * x) / HEX_SIZE, 1.0 );
			  // 	lprintf( WIDE("uhh %g %g %g")
			  // 			 ,patch->origin[0] - (HEX_SIZE-x) * HEX_HORZ_STRIDE/2 - y * HEX_HORZ_STRIDE/2
			  // 			 , patch->origin[1] - (HEX_SIZE-y) * HEX_VERT_STRIDE
           //            , patch->origin[2] - patch->height[s][x][y] );
					//    tmp = patch->origin[0] - (HEX_SIZE-x) * HEX_HORZ_STRIDE/2 - y * HEX_HORZ_STRIDE/2;
					if( y == 0 )
					{
						glColor3f( 1.0, 0.0, 1.0 );
					glVertex3d( patch->origin[0]
								  + (x) * HEX_HORZ_STRIDE/2
								  + y * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  + (y) * HEX_VERT_STRIDE
                          - ( x * HEX_VERT_STRIDE )
                         , patch->origin[2] - patch->height[s][x][y] );
						glColor3f( 1.0, 0.0, 0.0 );
					glVertex3d( patch->origin[0]
								  + ((x+1)) * HEX_HORZ_STRIDE/2
								  + (y) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  + (y) * HEX_VERT_STRIDE
                          - ( (x+1) * HEX_VERT_STRIDE )
								 , patch->origin[2]
								  - ((x!=HEX_SIZE-1)
								  ? patch->height[s][(x+1)][y]
								  : 0)
								 );
					}
						glColor3f( 1.0, 1.0, 0.0 );
					glVertex3d( patch->origin[0]
								  + (x) * HEX_HORZ_STRIDE/2
								  + (y	+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  + (y+1) * HEX_VERT_STRIDE
                          - ( x * HEX_VERT_STRIDE )
								 , patch->origin[2] - patch->height[s][x][y+1] );
						glColor3f( 0.0, 1.0, 1.0 );
					glVertex3d( patch->origin[0]
								  + (x+1) * HEX_HORZ_STRIDE/2
								  + (y+1) * HEX_HORZ_STRIDE/2
								 , patch->origin[1]
								  + ((y+1)) * HEX_VERT_STRIDE
                          - ( (x+1) * HEX_VERT_STRIDE )
								 , patch->origin[2]
								  - ((x!=HEX_SIZE-1)
								  ? patch->height[s][x+1][(y+1)]
								  : 0) // not sure what this would be... next patch I suppose...
								 );
//					glEnd();					// Done Drawing The Cube
					break;
				}
			}
				glEnd();					// Done Drawing The Cube
		}
	}
   /*
	glBegin(GL_TRIANGLES);
	glColor3f( 1.0, 1.0, 1.0 );
	glVertex3f( patch->origin[0] - 1 * HEX_HORZ_STRIDE/3
				 , patch->origin[1] + 1.2f * HEX_HORZ_STRIDE/3
				 , patch->origin[2] );
	glVertex3f( patch->origin[0] - 1  * HEX_HORZ_STRIDE/3
				 , patch->origin[1] - 1.2f * HEX_HORZ_STRIDE/3
				 , patch->origin[2] );
	glVertex3f( patch->origin[0] + 1  * HEX_HORZ_STRIDE/3
				 , patch->origin[1] - 0
				 , patch->origin[2] );
				 glEnd();
				 */// Done Drawing The Cube
  glFlush();
  return 1;
}

static int delta_table[3][6][2] = { { { -1,0 }, { 0, -1 }, { 1, -1 }, { 1, 0 }, { 0, 1 }, { -1, 1 } }
											 , { { -1,0 }, {-1, 1 }, { 0, 1 }, { 1, 0 }, { 1, -1 }, { 0, -1 } }
											 , { { -1, -1 }, { 0, -1 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 0 } }
};

void DistortPatch( PHEXPATCH patch
					  , int section, int x, int y
					  , RCOORD delta // delta up/down for this point.
					  , RCOORD tension // tension between this point and near points.
					  , RCOORD radius // total radius to consider moving... (tension?)
					  )
{
	if( tension >= 1.0 || tension <= -1.0 )
		return;
	//if( delta < 0.0001 )
   //   return;
	//while( delta > 0.00001 )
	{
      int p;
		int near_points[6][3];
		//lprintf( WIDE("Delta is %g"), delta );
		for( p = 0; p < 6; p++ )
		{
			near_points[p][0] = x + delta_table[section][p][0];
			near_points[p][1] = y + delta_table[section][p][1];
			near_points[p][2] = section;
		}
		patch->height[section][x][y] += delta;
		switch( section )
		{
		case 0:
			if( y == 0 )
			{
				patch->height[1][x][0] = patch->height[section][x][y];
				near_points[1][2] = 1;
				near_points[1][0] = x;
				near_points[1][1] = 1;
				if( x == (HEX_SIZE-1) )
				{
					lprintf( WIDE("this isn't right... might not be...") );
					near_points[2][2] = 2;
					near_points[2][0] = 1;
					near_points[2][1] = 1;
				}
				else
				{
					near_points[2][2] = 1;
					near_points[2][0] = x+1;
					near_points[2][1] = 1;
				}
			}
			if( x == 0 )
			{
				near_points[0][2] = -1;
				near_points[5][2] = -1;
            /*
				if( y == (HEX_SIZE-1) )
				{
					near_points[5][2] = -1;
				}
				else
				{
               near_points[5][2] = 2;
					}
               */
			}
			if( x == (HEX_SIZE-1) )
			{
				patch->height[2][y][0] = patch->height[section][x][y];
				near_points[3][2] = 2;
				near_points[3][0] = 0;
				near_points[3][1] = y;
				if( y )
				{
					near_points[2][2] = 2;
					near_points[2][0] = y;
					near_points[2][1] = 1;
				}
            // near_point 2 is taken care of above?
			}
			if( y == (HEX_SIZE-1) )
			{
				patch->height[2][y][0] = patch->height[section][x][y];
				near_points[4][2] = -1;
				near_points[5][2] = -1;
			}
         break;
		case 1:
			if( x == 0 )
			{
				near_points[0][2] = -1;
				near_points[1][2] = -1;
			}
			if( y == 0 )
			{
				near_points[4][2] = -1;
				near_points[5][2] = -1;
			}
			if( x == (HEX_SIZE-1) )
			{
				near_points[3][2] = -1;
				near_points[4][2] = -1;
			}
			if( y == (HEX_SIZE-1) )
			{
				near_points[1][2] = -1;
				near_points[2][2] = -1;
			}
			break;
		case 2:
			if( x == 0 )
			{
				near_points[0][2] = -1;
				near_points[5][2] = -1;
			}
			if( y == 0 )
			{
				near_points[0][2] = -1;
				near_points[1][2] = -1;
			}
			if( x == (HEX_SIZE-1) )
			{
				near_points[2][2] = -1;
				near_points[3][2] = -1;
			}
			if( y == (HEX_SIZE-1) )
			{
				near_points[3][2] = -1;
				near_points[4][2] = -1;
			}
         break;
		}
		{
			int p;
			for( p = 0; p < 6; p++ )
			{
            if( near_points[0][2] >= 0 )
					patch->height[near_points[p][2]][near_points[p][0]][near_points[p][1]] += delta*tension;
			}
		}
      //delta *= tension;
	}
}

void RipplePatch( PHEXPATCH patch )
{
	//int n;
	int x, y;
   RCOORD distort;
	int sector;
	sector = rand() % 3;
   x = rand() %HEX_SIZE;
	y = rand() %HEX_SIZE;
	distort = (float)rand() / (float)RAND_MAX;

	distort *= ((rand() & 0xFF) < 0x80)?-1:1;
	//distort *= 1.5;
   distort *= HEX_SCALE*0.75;
   DistortPatch( patch, sector, x, y, distort, 0.5, 0 );

}

	PTRANSFORM T;
	POBJECT current;

	void CPROC UpdateUserBot( PTRSZVAL unused )
	{
		static VECTOR KeySpeed, KeyRotation;
		//static VECTOR move = { 0.01, 0.02, 0.03 };
		{
			extern void ScanKeyboard( PRENDERER hDisplay, PVECTOR KeySpeed, PVECTOR KeyRotation );
			ScanKeyboard( NULL, KeySpeed, KeyRotation );
			SetSpeed( T, KeySpeed );
			SetRotation( T, KeyRotation );
			Move(T);  // relative rotation...
		}
		if( IsKeyDown( NULL, KEY_N ) )
		{
			if( current )
			{
				current = NULL;
				//LookAt( 
			}
			else
			{
				current = (POBJECT)GetLink( &cycles, 0 );
			}
		}
		{
			static int xdel1;
			static int ydel1;
			static int sdel1;
			static int xdel2;
			static int ydel2;
			static int sdel2;
			if( !xdel1 && IsKeyDown( NULL, KEY_1 ) )
			{
				cur_x ++;
				if( cur_x == HEX_SIZE )
				{
					cur_x = 0;
				}
				xdel1 = 1;
			}
			else if( xdel1 && !IsKeyDown( NULL, KEY_1 ) )
				xdel1 = 0;
			if( !xdel2 && IsKeyDown( NULL, KEY_2 ) )
			{
				if( cur_x )
					cur_x--;
				else
					cur_x = HEX_SIZE -1;
				xdel2 = 1;
			}
			else if( xdel2 && !IsKeyDown( NULL, KEY_2 ) )
				xdel2 = 0;

			if( !ydel1 && IsKeyDown( NULL, KEY_3 ) )
			{
				cur_y++;
				if( cur_y == HEX_SIZE )
					cur_y = 0;
				ydel1 = 1;
			}
			else if( ydel1 && !IsKeyDown( NULL, KEY_3 ) )
				ydel1 = 0;

			if( !ydel2 && IsKeyDown( NULL, KEY_4 ) )
			{
				if( cur_y )
					cur_y--;
				else
					cur_y = HEX_SIZE-1;
				ydel2 = 1;
			}
			else if( ydel2 && !IsKeyDown( NULL, KEY_4 ) )
				ydel2 = 0;

			if( !sdel1 && IsKeyDown( NULL, KEY_5 ) )
			{
				cur_s++;
				if( cur_s == 12 )
					cur_s = 0;
				sdel1 = 1;
			}
			else if( sdel1 && !IsKeyDown( NULL, KEY_5 ) )
				sdel1 = 0;

			if( !sdel2 && IsKeyDown( NULL, KEY_6 ) )
			{
				if( cur_s )
					cur_s--;
				else
					cur_s = 11;
				sdel2 = 1;
			}
			else if( sdel2 && !IsKeyDown( NULL, KEY_6 ) )
				sdel2 = 0;
		//add( KeyRotation, KeyRotation, move );
		if(0)
		{
			static int n;
			TEXTCHAR filename[32];
			n++;
			if( n > 25 ) n = 0;
			snprintf( filename, sizeof( filename ), WIDE("state-%d"), n );
			{
				FILE *last = sack_fopen( 0, WIDE("state-last"), WIDE("wt") );
				fprintf( last, WIDE("%d"), n );
				fclose( last );
			}
			SaveTransform( T, filename );
		}
		//SaveTransform( T, WIDE("recoverme") );
	}

	// update body motion according to brain statistics...
	if( !bwait_move_gliders )
	{
		INDEX idx;
		PBODY body;
		POBJECT glider;
		LIST_FORALL( bodies, idx, PBODY, body )
		{
			glider = body->object;
			//lprintf( WIDE("body->speed %g"), body->speed );
			Forward( glider->Ti, body->own_speed + ( body->speed / 25.60 ) );
			//Rotate( a,m[vForward],m[vRight] )
			{
				VECTOR r;
				r[0] = 0;
				r[1] = (body->rotation / 256.0) * 30 * 6.2831 / 360.0 ;
				r[2] = 0;
				CreateTransformMotion( glider->Ti );
				SetRotation( glider->Ti, r );
			}
			//body->Move();
		}
	}
		}



enum {
	LISTBOX_BODIES=5000
	, BUTTON_SHOW_BRAIN
} ;

PTRANSFORM TCam;

PRELOAD( RegisterResources )
{
	l.pri = GetDisplayInterface();
	l.pii = GetImageInterface();
	EasyRegisterResource( WIDE("terrain/body interface"), LISTBOX_BODIES, LISTBOX_CONTROL_NAME  );
	EasyRegisterResource( WIDE("terrain/body interface"), BUTTON_SHOW_BRAIN, NORMAL_BUTTON_NAME );
	TCam = CreateTransform();
}

void CPROC ShowBrain( PTRSZVAL psv, PSI_CONTROL button )
{
	PSI_CONTROL list = GetNearControl( button, LISTBOX_BODIES );
	PLISTITEM pli = GetSelectedItem( list );
	if( pli )
	{
		PBODY body = (PBODY)GetItemData( pli );
		if( !body->board )
			body->board = CreateBrainBoard( body->brain );
	}
}

static void CPROC BoardCloseEvent( PTRSZVAL psv, PIBOARD board )
{
	PBODY body = (PBODY)psv;
	body->board = NULL;
}

void CPROC ShowSelectedBrain( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	PBODY body = (PBODY)GetItemData( pli );
	if( !body->board )
	{
		body->board = CreateBrainBoard( body->brain );
		GetBoard( body->board )->SetCloseHandler( BoardCloseEvent, (PTRSZVAL)body );
	}
}

void CreateBodyInterface( void )
{
	PSI_CONTROL frame = LoadXMLFrame( WIDE("BodyInterface.Frame") );
	if( frame )
	{
		PSI_CONTROL list = GetControl( frame, LISTBOX_BODIES );
		SetButtonPushMethod( GetControl( frame, BUTTON_SHOW_BRAIN ), ShowBrain, 0 );
		SetDoubleClickHandler( list, ShowSelectedBrain, 0 );
		{
			INDEX idx;
			PBODY body;
			LIST_FORALL( bodies, idx, PBODY, body )
			{
				TEXTCHAR name[256];
				snprintf( name, sizeof( name ), WIDE("Body %d"), idx );
				SetItemData( AddListItem( list, name ), (PTRSZVAL)body );
			}
		}
		DisplayFrame( frame ); // leave it open forever?
	}
}

void CreateBodies( POBJECT pWorld )
{
	int n;
	PBODY body;
	POBJECT glider;
	VECTOR v;
	for( n = 0; n < BODY_COUNT; n++ )
	{
		AddLink( &gliders, glider = MakeGlider() );
		PutIn( pWorld, glider );
		scale( v, VectorConst_Z, (3000/3600)/100 ); // 3 k/hr
		CreateTransformMotion( glider->Ti );
		SetSpeed( glider->Ti, v );
		//SetSpeed( glider->Ti, VectorConst_Y );
		Translate( glider->Ti, 0, SPHERE_SIZE, 0 );
		{
			//VECTOR v1,v2,v3;
			//scale( v1, VectorConst_X, 1 );
			//GetAxisV( glider->Ti, v2, vRight );
			// in two operations one can observe
			// the tip in pitch because of forward motion
			//RotateAround( glider->Ti, v2, 0 );
		}
		//.RotateRel( glider->Ti, 0, ( 6.14 * n ) / 100.0, 0 );
		RotateRel( glider->Ti, 0, ( 6.28 * n ) / 4.0, 0 );
		//Forward( glider->Ti, 50 );
		
			AddLink( &bodies, body = new Glider( glider ) );
	}
	AddLink( &gliders, glider = MakeGlider() );
	AddLink( &cycles, glider = MakeGlider() );
	PutIn( pWorld, glider );
	scale( v, VectorConst_Z, (3000/3600)/100 ); // 3 k/hr
	CreateTransformMotion( glider->Ti );
	SetSpeed( glider->Ti, v );
	//SetSpeed( glider->Ti, VectorConst_Y );
	Translate( glider->Ti, 0, SPHERE_SIZE, 0 );
	AddLink( &bodies, body = new CyberCycle( glider ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	AddLink( &bodies, body = new CyberCycle( MakeGlider() ) );
	bwait_move_gliders = 0;
}

POBJECT CreateWorldCubeThing( VECTOR v, CDATA color )
{
	POBJECT pWorld;
	{
		pWorld = CreateScaledInstance( CubeNormals, CUBE_SIDES, 2, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
		SetObjectColor( pWorld, color );
      /// is also the same as AddRootObject
		SetRootObject( pWorld );
		{
			VECTOR r;
			r[0] = 0.05;
			r[1] = 0.02;
			r[2] = 0.15;
			CreateTransformMotion(  pWorld->Ti );
			SetRotation( pWorld->Ti, r );
		}
	}
      return pWorld;
}

#if 0
class VirtualityTester
{
public:
	VirtualityTester();
		void Step(void);

}  ;
VirtualityTester *VTest;
#endif

static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
		lprintf( WIDE("Access violation - OpenGL layer at this moment..") );
		return EXCEPTION_EXECUTE_HANDLER;
	default:
		lprintf( WIDE("Filter unknown : %08X"), n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}

PTRANSFORM T_camera = NULL;
PHEXPATCH patch;
PVIEW view;
int bEnabledGL;

static PTRSZVAL OnInit3d( WIDE( "Terrain View" ) )( PMatrix projection, PTRANSFORM transform, RCOORD *identity_depth, RCOORD *aspect )
{
	return 1;
}

static void OnDraw3d( WIDE("Terrain View") )( PTRSZVAL psvView )
//static int OnDrawCommon( WIDE("Terrain View") )( PSI_CONTROL pc )
{
	//PRENDERER pRend = GetFrameRenderer( pc );
	MATRIX m;
	// display already activated by the time we get here.
#ifdef _MSC_VER
		__try
	{
#endif
	if( 1 )//SetActiveGLDisplay( pRend ) )
	{
		//InitGL();
		//SaveTransform( T, WIDE("recoverme") );
		if(0)
		{
			static PTRANSFORM tmp[2];
			TEXTCHAR buffer[32];
			static int n = 19;
			static int first = 1;
			static int d;
			n++;
			if( n > 21) n=0;
			if( !tmp[0] )
			{
				tmp[0] = CreateTransform();
				tmp[1] = CreateTransform();
			}
			snprintf( buffer, sizeof( buffer ), WIDE("state-%d"), n );
			lprintf( WIDE("Loading transform %d"), n );
			LoadTransform( tmp[d], buffer );
			ShowTransform( tmp[d] );
			//if( ( tmp[d].m[0][1] > 0.983537 )
			//  &&( tmp[d].m[0][1] < 0.983539 ) )
			//	DebugBreak();
			if( first )
			{
				first = 0;
				return;
				//continue;
			}
			ApplyT( VectorConst_I, T, tmp[d] );
			//memcpy( T[0].rotation, tmp[d].rotation, sizeof( tmp[d].rotation ) );
			//memcpy( T[0].speed, tmp[d].speed, sizeof( tmp[d].speed ) );
			//Move( T );
			ShowTransform( T );
			ApplyT( VectorConst_I, T, tmp[d] );
			d = 1-d;
		}
		//LoadTransform( T, WIDE("state-21") );
		//PrintMatrix( T->m );
		//LoadTransform( T, WIDE("state-22") );
		//PrintMatrix( T->m );
		//LoadTransform( T, WIDE("state-23") );
		//PrintMatrix( T->m );
		//LoadTransform( T, WIDE("recoverme") );
		//PrintMatrix( T );
		//lprintf( WIDE("Getting matrix...") );
		//if( !T_camera )
		//	return;
		//GetGLMatrix( T_camera, m );
		//glLoadMatrixd( (RCOORD*)m );

		//RipplePatch( patch );
		//RipplePatch( patch+1 );
		//glLoadIdentity();
		//glTranslatef((-1.5f),1.0f,-30.0);		// Move Left 1.5 Units And Into The Screen 6.0
		if( !patch )
			return;
		if( DrawSphereThing( patch ) )

			//ShowPatch( patch );
			//if( T->m[3][2] != 0 )
			//	 DebugBreak();
			//PrintVector( GetOrigin( T ) );

			//ShowObjects(NULL,&TCam);
		{
			INDEX idx;
			PBODY body;
			LIST_FORALL( bodies, idx, PBODY, body )
			{
				body->Draw();
			}
			//ShowObjects(view,T_camera);
		}
		//VTest->Step();

		//SetActiveGLDisplay( NULL );
		//UpdateDisplay( pRend );
	}
#ifdef _MSC_VER
	}
						__except( EvalExcept( GetExceptionCode() ) )
					{
						lprintf( WIDE(" ...") );
						;
					}
#endif
	return;
}

static int OnMouseCommon( WIDE("Terrain View") )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	static int _b;
	static struct {
		struct {
			BIT_FIELD bMoving : 1;
		} flags;
		S_32 x, y;
	} mouse;
	if( b & MK_LBUTTON && !( _b &MK_LBUTTON ) )
	{
		mouse.x = x;
		mouse.y = y;
		mouse.flags.bMoving = 1;
	}
	else if( b&MK_LBUTTON )
	{
		MoveCommonRel( pc, x - mouse.x, y - mouse.y );
	}
	_b = b;
	return 0;
}


static void CPROC UpdateView( PTRSZVAL psv )
{
	SmudgeCommon( (PSI_CONTROL)psv );
}

CRITICALSECTION csUpdate;

static void CPROC UpdatePositions( PTRSZVAL psv )
{
	EnterCriticalSec( &csUpdate );
	VirtualityUpdate();
#if 0
			if( current )
			{
				static PTRANSFORM TDelta;
				static PTRANSFORM TDelta2;
				if( !TDelta )
				{
					TDelta = CreateTransform();
					TDelta2 = CreateTransform();
					TranslateRel( TDelta, 0, 40, -90 );
				}
				//TCam = current->Ti[0];
				//Invert( TCam.m[vUp] );
				ApplyT( view->Tglobal, TDelta2, TDelta ); 
				ApplyT( TDelta2, TCam, current->Ti );
				ApplyT( current->Ti, TCam, TDelta2 );
			}
#endif
	LeaveCriticalSec( &csUpdate );

}

int main( int argc, char **argv )
{
	POBJECT pWorld;
	// create patch returns two patches in array.
	//PSI_CONTROL pcDisplay;
	InvokeDeadstart();
	patch = CreatePatch( NULL );
	T = CreateTransform();
	CreateTransformMotion( T );
	{
		extern void CreateGPSGridDisplay( void );
		//CreateGPSGridDisplay();
	}
	InitializeCriticalSec( &csUpdate );
	//Translate( T, 0, 0, -30 );
	AddTimer( 10, UpdateUserBot, 0 );
	AddTimer( 50, UpdatePositions, 0 );
	//RipplePatch( patch );
	// MUST show the window before the context
	// can be activated - otherwise certain facts about the window
	// do not exist.

	//pcDisplay = MakeNamedControl( NULL, WIDE("Terrain View"), 0, 0, 640, 640, -1 );

	//DisplayFrame( pcDisplay );
	//SetAllocateLogging( TRUE );

	//EnableControlOpenGL( pcDisplay );

	//EnableOpenGL( GetFrameRenderer( pcDisplay ) );
	bEnabledGL = 1;
	//SetActiveGLDisplay( pRend );
	//view = CreateViewWithUpdateLockEx( V_FORWARD, NULL, WIDE("Forward 1"), 200, 200, &csUpdate );
#if 0
	CreateViewEx( V_RIGHT, NULL, WIDE("Forward 1"), 200, 200 );
	CreateViewEx( V_LEFT, NULL, WIDE("Forward 1"), 200, 200 );
	CreateViewEx( V_UP, NULL, WIDE("Forward 1"), 200, 200 );
	CreateViewEx( V_DOWN, NULL, WIDE("Forward 1"), 200, 200 );
	CreateViewEx( V_BACK, NULL, WIDE("Forward 1"), 200, 200 );
#endif
	{
		VECTOR v;

		v[0] = 0;
		v[1] = 0;
		v[2] = 15;
		CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_BLUE, BASE_COLOR_WHITE, 50, 100 ) );

		v[0] = 0;
		v[1] = 0;
		v[2] = -15;
		CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_BLUE, BASE_COLOR_BLACK, 50, 100 ) );

		v[0] = 0;
		v[1] = 15;
		v[2] = 0;
		CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_GREEN, BASE_COLOR_WHITE, 50, 100 ) );
		v[0] = 0;
		v[1] = -15;
		v[2] = 0;
		CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_GREEN, BASE_COLOR_BLACK, 50, 100 ) );

		v[0] = 15;
		v[1] = 0;
		v[2] = 0;
		CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_RED, BASE_COLOR_WHITE, 50, 100 ) );
		v[0] = -15;
		v[1] = 0;
		v[2] = 0;
		pWorld = CreateWorldCubeThing( v, ColorAverage( BASE_COLOR_RED, BASE_COLOR_BLACK, 50, 100 ) );
	}

	CreateBodies( pWorld );			

	//VTest = new VirtualityTester();

	// a dialog to interface to the bodies that
	// have been created within the world.
	//SetAllocateLogging( TRUE );
	CreateBodyInterface();

	while( 1 )
      Sleep( 100000 );
}


#ifdef _MSC_VER
int APIENTRY WinMain( HINSTANCE junk, HINSTANCE junk2, LPSTR szUnusedCmdLine, int nUnusedShowCmd )
{
	return main( 1, &szUnusedCmdLine );
}

#endif
