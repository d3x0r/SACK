#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE
#define TERRAIN_MAIN_SOURCE
//#define MAKE_RCOORD_SINGLE
#include <network.h>
#include <math.h>
#include <render.h>
#include <render3d.h>
#include <vectlib.h>
#include <sharemem.h>
#include <psi.h>

#include <btBulletDynamicsCommon.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "land.h"

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

#define TERRAIN_MAIN_SOURCE
#include "local.h"

EasyRegisterControlWithBorder( WIDE("Terrain View"), 0, BORDER_NONE );

struct world_body_map bodymap;

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

	int number;


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
			glPushMatrix();

			btScalar m[16];
			btTransform trans;
			patch->fallRigidBody->getMotionState()->getWorldTransform( trans );

			trans.getOpenGLMatrix(m);
			glMultMatrixf( m );

		for( s = 0; s < 3; s++ )
		{
			float tmpval[4];
			tmpval[3] = 1.0;
			switch( s )
			{
			case 0:
			case 1:
				tmpval[0] = 0.2f;
				tmpval[1] = 0.2f;
				tmpval[2] = 0.2f;
				break;
			case 2:
				tmpval[0] = RedVal( l.colors[(number-1)/15] )/(256*5.0f);
				tmpval[1] = GreenVal( l.colors[(number-1)/15] )/(256*5.0f);
				tmpval[2] = BlueVal( l.colors[(number-1)/15] )/(256*5.0f);
				break;
			}
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmpval );
			glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE); 
			for( level = 1; level <= HEX_SIZE; level++ )
			{
				VECTOR v1,v2;
				//if( bLog )lprintf( WIDE("---------") );
				//if( 0)  {

				glBegin( GL_TRIANGLE_STRIP );
				r = 0;
				for( c = 0; c <= level; c++ )
				{
					tmpval[0] = tmpval[0] + 0.1f;
					tmpval[1] = tmpval[1] + 0.1f;
					ConvertPolarToRect( level, c, &x, &y );
					scale( v1, pole_patch.patches[s].grid[x][y]
							, SPHERE_SIZE + patch[north].height[s][x][y] );
					if( north )
						v1[1] = -v1[1];
					//ApplyRotation( patch->T, v1, v2 );
					//add( v1, v1, patch->origin );
					glNormal3f( v1[0], v1[1], v1[2] );
					glColor3fv( tmpval );
					glVertex3f( v1[0], v1[1], v1[2] );
					if( c < (level) )
					{
						ConvertPolarToRect( level-1, c, &x, &y );
						scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
						if( north )
							v1[1] = -v1[1];
						//ApplyRotation( patch->T, v1, v2 );
						//add( v1, v1, patch->origin );
						glNormal3f( v1[0], v1[1], v1[2] );
						glColor3fv( tmpval );
						glVertex3f( v1[0], v1[1], v1[2] );
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
					ConvertPolarToRect( level, c, &x, &y );
					//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c,level);
					scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
					if( north )
						v1[1] = -v1[1];
					//ApplyRotation( patch->T, v1, v2 );
					//add( v1, v1, patch->origin );
					glNormal3f( v1[0], v1[1], v1[2] );
					glColor3fv( tmpval );
					glVertex3f( v1[0], v1[1], v1[2] );
					if( c < (level)*2 )
					{

						ConvertPolarToRect( level-1, c-1, &x, &y );
						//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c-1,level-1);
						scale( v1, pole_patch.patches[s].grid[x][y], SPHERE_SIZE + patch[north].height[s][x][y] );
						if( north )
							v1[1] = -v1[1];
						//ApplyRotation( patch->T, v1, v2 );
						//add( v1, v1, patch->origin );
						glNormal3f( v1[0], v1[1], v1[2] );
						glColor3fv( tmpval );
						glVertex3f( v1[0], v1[1], v1[2] );
					}
				}
				glEnd();
			}
		}
			glPopMatrix();
	}
	//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") );return 0; }
	return 1;
}


void RenderBandPatch( PHEXPATCH patch )
{
	int section, section2;
	int sections = 6*HEX_SIZE;
	int sections2= HEX_SIZE;
	int row;
   int col;
	VECTOR tmp_patch1, tmp_patch2;
	VECTOR norm_patch1, norm_patch2;
	VECTOR patch1, patch2;
	SYSTEMTIME st;
	//VECTOR ref_point;
	//InitBandPatch();
	float tmpval[4];
	float tmpval_diff[4];
	float tmpval_amb[4];
	GetSystemTime( &st );
	number = (st.wSecond + patch->number )%75 + 1;
	row = (number-1)/l.numbers.cols;
	col = (number-1)%l.numbers.cols;

   tmpval[0] = l.values[MAT_DIFFUSE0] / 256.0f;
   tmpval[1] = l.values[MAT_DIFFUSE1] / 256.0f;
   tmpval[2] = l.values[MAT_DIFFUSE2] / 256.0f;
   tmpval[3] = 1.0f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmpval );
   tmpval[0] = l.values[MAT_AMBIENT0] / 256.0f;
   tmpval[1] = l.values[MAT_AMBIENT1] / 256.0f;
   tmpval[2] = l.values[MAT_AMBIENT2] / 256.0f;
   tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmpval );
   tmpval[0] = l.values[MAT_SPECULAR0] / 256.0f;
   tmpval[1] = l.values[MAT_SPECULAR1] / 256.0f;
   tmpval[2] = l.values[MAT_SPECULAR2] / 256.0f;
   tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpval );
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, l.values[MAT_SHININESS]/2 ); // 0-128


	tmpval[3] = 1.0;
	//scale( ref_point, VectorConst_X, SPHERE_SIZE );
	//lprintf( " Begin --------------"  );
	for( section = 0; section < sections; section ++ )
	{
		int other_s = (section/(HEX_SIZE+1))%6; // might be 6, which needs to be 0.
		int s = (section/HEX_SIZE)%6; // might be 6, which needs to be 0.
		int s2 = ((section+1)/HEX_SIZE)%6; // might be 6, which needs to be 0.
		int layer;
		if( section == HEX_SIZE )
		{
			//lprintf( "%d %d", s, s2 );
			//continue;
		}
		//lprintf( "section %d = %d,%d", section, s, s2 );
		for( layer = 0; layer < 2; layer++ )
		{
			if( layer == 0 )
			{
				switch( other_s )
				{
				case 0:
				case 1:
				case 2:
					tmpval[0] = 0.2f;
					tmpval[1] = 0.2f;
					tmpval[2] = 0.2f;
					break;
				case 3:
				case 4:
				case 5:
					tmpval[0] = RedVal( l.colors[(number-1)/15] )/(256*5.0f);
					tmpval[1] = GreenVal( l.colors[(number-1)/15] )/(256*5.0f);
					tmpval[2] = BlueVal( l.colors[(number-1)/15] )/(256*5.0f);
					break;
				}
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmpval );
				glBindTexture( GL_TEXTURE_2D, 0 );
			}
			else
			{
				if( other_s == 0 )
					glBindTexture( GL_TEXTURE_2D, l.numbers.texture );
				else
				{
					continue;
					//glBindTexture( GL_TEXTURE_2D, 0 );
				}

			}


			glPushMatrix();

			btScalar m[16];
			btTransform trans;
			patch->fallRigidBody->getMotionState()->getWorldTransform( trans );

			trans.getOpenGLMatrix(m);
			glMultMatrixf( m );

			glBegin( GL_TRIANGLE_STRIP );
			for( section2 = 0; section2 <= sections2; section2++ )
			{

				scale( patch1, band.patches[s].grid[section%HEX_SIZE][section2], SPHERE_SIZE );
				//ApplyRotation( patch->T, norm_patch1, tmp_patch1 );
				//ApplyRotation( patch->T, patch1, tmp_patch1 );
				//add( patch1, patch1, patch->origin );
				
				scale( patch2, band.patches[s2].grid[(section+1)%HEX_SIZE][section2], SPHERE_SIZE );
				//ApplyRotation( patch->T, norm_patch2, tmp_patch2 );
				//ApplyRotation( patch->T, patch2, tmp_patch2 );
				//add( patch2, patch2, patch->origin );
				



				{
					
					glTexCoord2f( l.numbers.coords[(row*(HEX_SIZE+1))+section2]
									[(col*(HEX_SIZE+1))+(section%(HEX_SIZE+1))][0]
						, l.numbers.coords[(row*(HEX_SIZE+1))+section2]
									[(col*(HEX_SIZE+1))+(section%(HEX_SIZE+1))][1] );
					//glColor3f( 0.5f, 0.5f, 0.5f );

					glNormal3f( patch1[0]
						, patch1[1]
						, patch1[2] );
					glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
					glVertex3f( patch1[0]
						, patch1[1]
						, patch1[2] );


					glTexCoord2f( l.numbers.coords[(row*(HEX_SIZE+1))+section2]
									[(col*(HEX_SIZE+1))+((section+1)%(HEX_SIZE+1))][0]
						, l.numbers.coords[(row*(HEX_SIZE+1))+section2]
									[(col*(HEX_SIZE+1))+((section+1)%(HEX_SIZE+1))][1] );
					glNormal3f( patch2[0]
						, patch2[1]
						, patch2[2] );
					glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
					glVertex3f( patch2[0]
						, patch2[1]
						, patch2[2] );
				}
			}
			glEnd();

			glPopMatrix();
		}
	}
	glBindTexture( GL_TEXTURE_2D, 0 );
}

int DrawSphereThing( PHEXPATCH patch )
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	{
		GLfloat global_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);	
	}

	{
		GLfloat lightpos[] = {.5, -10, 10., 0.};
		GLfloat lightdir[] = {-.5, -1., -10., 0.};
		GLfloat lightamb[] = {l.values[LIT_AMBIENT0]/256.0f, l.values[LIT_AMBIENT1]/256.0f, l.values[LIT_AMBIENT2]/256.0f, 1.0};
		//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
		GLfloat lightdif[] = {l.values[LIT_DIFFUSE0]/256.0f, l.values[LIT_DIFFUSE1]/256.0f, l.values[LIT_DIFFUSE2]/256.0f, 1.0};
		//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
		GLfloat lightspec[] = {l.values[LIT_SPECULAR0]/256.0f, l.values[LIT_SPECULAR1]/256.0f, l.values[LIT_SPECULAR2]/256.0f, 1.0};
		//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

		glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightspec);
		glLightfv( GL_LIGHT0, GL_SPOT_DIRECTION, lightdir );
	}

	{
		GLfloat lightpos[] = {.5, 10, -10., 0.};
		GLfloat lightdir[] = {-.5, -1., -10., 0.};
		GLfloat lightamb[] = {1.0, 1.0, 1.0, 1.0};
		//GLfloat lightamb[] = {0.1, 0.1, 0.1, 1.0};
		GLfloat lightdif[] = {1.0, 1.0, 1.0, 1.0};
		//GLfloat lightdif[] = {0.1, 0.1, 0.1, 0.1};
		GLfloat lightspec[] = {1.0, 1.0, 1.0, 1.0};
		//GLfloat lightspec[] = {0.1, 0.1, 0.1, 0.1};
		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_POSITION, lightpos);

		glLightfv(GL_LIGHT1, GL_AMBIENT, lightamb);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, lightdif);
		glLightfv(GL_LIGHT1, GL_SPECULAR, lightspec);
		glLightfv( GL_LIGHT1, GL_SPOT_DIRECTION, lightdir );
	}

	//which is a 1x1x1 sort of patch array
	RenderBandPatch( patch );

	if( !RenderPolePatch( patch, 0 ) )
		return 0;
	if( !RenderPolePatch( patch, 1 ) )
		return 0;

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	return 1;
}


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



enum {
	LISTBOX_BODIES=5000
	, BUTTON_SHOW_BRAIN
} ;


PRELOAD( RegisterResources )
{
	l.pri = GetDisplayInterface();
	l.pii = GetImageInterface();
}


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


void ParseImage( Image image, int rows, int cols )
{
	_32 w, h;
	int r, c;
	int divisions = HEX_SIZE+1;
	GetImageSize( image, &w, &h );
	l.numbers.rows = rows;
	l.numbers.cols = cols;
	l.numbers.coords = NewArray( VECTOR*, (rows+1)*(divisions));
	for( r = 0; r <= (divisions)*rows; r++ )
	{
		l.numbers.coords[r] = NewArray( VECTOR, (cols+1)*(divisions));
	}
	for( r = 0; r <= (divisions*rows); r++ )
	{
		for( c = 0; c <= (divisions*cols); c++ )
		{
			l.numbers.coords[r][c][0] = (( -(c/divisions) + ((HEX_SIZE-(c%divisions)) + (divisions*(c/divisions)) ))/((double)cols*(HEX_SIZE)));
			if( 0 )
			lprintf( "%d %d %d", c%divisions
				,  (HEX_SIZE-(c%divisions))
				, ((divisions*(c/divisions)) )
				);
			l.numbers.coords[r][c][1] = ((r)/((double)rows*(HEX_SIZE+1)));
			l.numbers.coords[r][c][2] = 0;
			if( 0 )
			PrintVector( l.numbers.coords[r][c] );
 		}
	}
}

static void CPROC UpdateSliderVal( PTRSZVAL psv, PSI_CONTROL pc, int val )
{
   l.values[psv] = val;
	switch( psv )
	{
	case 0:
      break;
	}
}

static void CPROC SaveColors( PTRSZVAL psv, PSI_CONTROL pc )
{
	FILE *file = sack_fopen( 0, "values.dat", "wb" );
	if( file )
	{
      fwrite( l.values, sizeof( l.values ), 1, file );
      fclose( file );
	}
}

static void CPROC LoadColors( PTRSZVAL psv, PSI_CONTROL pc )
{
	FILE *file = sack_fopen( 0, "values.dat", "rb" );
	if( file )
	{
      fread( l.values, sizeof( l.values ), 1, file );
      fclose( file );
	}
	{
		int n;
		for( n = 0; n < 40; n++ )
		{
			SetSliderValues( l.sliders[n], 0, l.values[n], 256 );
		}
	}
}

static PTRSZVAL OnInit3d( WIDE( "Terrain View" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
   int n;
	l.frame = CreateFrame( "Light Slider Controls", 0, 0, 1024, 768, 0, NULL );
	for( n = 0; n < 40; n++ )
	{
		PSI_CONTROL pc;
		l.sliders[n] = MakeSlider( l.frame, 5 + 25*n, 5, 20, 420, 1, 0, UpdateSliderVal, n );
		SetSliderValues( l.sliders[n], 0, 128, 256 );
		pc = MakeButton( l.frame, 5, 430, 45, 15, 0, "Save", 0, SaveColors, 0 );
		pc = MakeButton( l.frame, 55, 430, 45, 15, 0, "Load", 0, LoadColors, 0 );
	}
	DisplayFrame( l.frame );
	l.numbers.image = LoadImageFile( "NewBallTextGrid.png" );
	ParseImage( l.numbers.image, 10, 10 );
	{
		PTRANSFORM transform = CreateNamedTransform( WIDE("render.camera") );
		Translate( transform, 500, 800, 0 );
		RotateAbs( transform, M_PI, 0, 0 );
		RotateRel( transform, 0, -M_PI/2, 0 );
		RotateRel( transform, -M_PI/2, 0, 0 );
		MoveRight( transform, -400 );
		RotateRel( transform, 0, M_PI/8, 0 );
		{
			VECTOR v;
			v[0] = 0;
			v[1] = 0; //3.14/2;
			v[2] = 3.14/4;
			//SetRotation( transform, v );
		}
	}
	return 1;
}

static void OnBeginDraw3d( WIDE( "Terrain View" ) )( PTRSZVAL psv )
{
   glShadeModel( GL_FLAT );
	//l.numbers.texture = ReloadTexture( l.numbers.image, 0 );
	l.numbers.texture = ReloadMultiShadedTexture( l.numbers.image, 0, AColor( 5, 5, 5, 32), Color( 12, 12, 83 ), Color( 0, 170, 170 ) );
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

		INDEX idx;
		PHEXPATCH patch;
		LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
		{
			DrawSphereThing( patch );
		}

		if( l.barRigidBody )
		{
			btTransform trans;
			l.barRigidBody->getMotionState()->getWorldTransform( trans );

			glPushMatrix();

			btScalar m[16];
			trans.getOpenGLMatrix(m);
			glMultMatrixf( m );
			
			glBegin( GL_LINES );

			glVertex3f( 0, 0, 0 );
			glVertex3f( 1000, 0, 0 );
			glEnd();
			glBegin( GL_LINES );

			glVertex3f( 0, 10, 0 );
			glVertex3f( 1000, 10, 0 );
			glEnd();

			glBegin( GL_LINES );

			glVertex3f( 0, -10, 0 );
			glVertex3f( 1000, -10, 0 );
			glEnd();

			glBegin( GL_LINES );

			glVertex3f( 0, 0, 10 );
			glVertex3f( 1000, 0, 10 );
			glEnd();
			//getWorldTransform();
			//getMotionState()
			glPopMatrix();
		}
	}
	l.bullet.dynamicsWorld->debugDrawWorld();
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

enum move_mode{
	MOVE_MODE_REMOVE,
	MOVE_MODE_REMOVE_IDLE,
	MOVE_MODE_WAIT,
	MOVE_MODE_RESET_ORIGIN,
	MOVE_UNTIL_DISTANCE,
	MOVE_ALL_STOP,
	MOVE_MODE_TURN1,
	MOVE_SPIN_MAX,
	MOVE_SPIN_STOP,
	MOVE_SPIN_ANGLE,
};

CRITICALSECTION csUpdate;

static void CPROC UpdatePositions( PTRSZVAL psv )
{
	_32 now = timeGetTime();
	if( l.last_tick )
	{
		l.bullet.dynamicsWorld->stepSimulation( (now-l.last_tick)/1000.f, 10);
	}
	l.last_tick = now;


	{
		INDEX idx;
		PHEXPATCH patch;
		btVector3 air_source( 0, -200, 0 );
		btScalar magnitude = 6000.0 + ( 200 * 100 );
		btVector3 air_target( 0, 800, 0 );
		btScalar target_magnitude = -30 *100.0;
		btVector3 up = btVector3(0,1,0);
		LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
		{
			btVector3 origin = patch->fallRigidBody->getWorldTransform().getOrigin();
			// lower blower
			{
			btScalar dist = origin.distance(air_source);

			btVector3 dir = origin - air_source;
			btScalar x = dir.dot( up ) / dir.length();
			if( x > 0.8 )
				patch->fallRigidBody->applyCentralImpulse( magnitude/(dist*dist*dist) * dir);   
			}

			{

			btScalar dist = origin.length();
			btVector3 dir = origin;
			btScalar x = dir.dot( up ) / dir.length();
			if( x < 0.3 )
				patch->fallRigidBody->applyCentralImpulse( - 50/(dist*dist) * dir);   
			}

			if( 0 )
			{
				btScalar dist = origin.distance(air_target);

				btVector3 dir = origin - air_target;
				btScalar x = dir.dot( up ) / dir.length();
				if( x < -0.6 )
					patch->fallRigidBody->applyCentralImpulse( target_magnitude/(dist*dist*dist) * dir);   
			}

		}
	}

	//lprintf( "bar is %d",  );
	if( l.barRigidBody )
		l.barRigidBody->activate();
	{
		int r,c;
		btScalar *m = NewArray( btScalar, 16 );
		MATRIX setmat;
		PHEXPATCH patch;
		INDEX index;
		LIST_FORALL( l.patches, index, PHEXPATCH, patch )
		{

		    btTransform trans;

            patch->fallRigidBody->getMotionState()->getWorldTransform(trans);
			patch->origin[0] = trans.getOrigin().getX();
			patch->origin[1] = trans.getOrigin().getY();
			patch->origin[2] = trans.getOrigin().getZ();
				
			trans.getOpenGLMatrix( (btScalar*)m );
			for( r = 0; r < 15; r++ )
				setmat[0][r] = m[r];
			SetGLMatrix( setmat, patch->T );

		}

	}
}       


void SetupBullet( struct BulletInfo *_bullet )
{
	_bullet->broadphase = new btDbvtBroadphase();

	_bullet->collisionConfiguration = new btDefaultCollisionConfiguration();
    _bullet->dispatcher = new btCollisionDispatcher(_bullet->collisionConfiguration);

	_bullet->solver = new btSequentialImpulseConstraintSolver;

	_bullet->dynamicsWorld = new btDiscreteDynamicsWorld(_bullet->dispatcher,_bullet->broadphase,_bullet->solver,_bullet->collisionConfiguration);

	_bullet->dynamicsWorld->setGravity(btVector3(0.1,-900,2));
	btIDebugDraw *draw = _bullet->dynamicsWorld->getDebugDrawer();
	//_bullet->dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);

}


void PopulateBulletGround( struct BulletInfo *_bullet )
{
	/* build the ground */
	 btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	 //groundShape= new btConeShape( 1000, 200 );



	  btDefaultMotionState* groundMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));

	  btRigidBody::btRigidBodyConstructionInfo
                groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
	  //groundRigidBodyCI.m_restitution
        _bullet->groundRigidBody = new btRigidBody(groundRigidBodyCI);
		lprintf( "old was %g", _bullet->groundRigidBody->getRestitution()  );
		_bullet->groundRigidBody->setRestitution( 1.0f );
		_bullet->groundRigidBody->setFriction( 10.0f );

		//_bullet->groundRigidBody->
		_bullet->dynamicsWorld->addRigidBody(_bullet->groundRigidBody);



}

// Creates a compound shaped tower centered at the ground level
static btCompoundShape* BuildTowerCompoundShape(const btVector3& brickFullDimensions=btVector3(151.0,300.0,2.0),unsigned int numRows=10,unsigned int numBricksPerRow=16,bool useConvexHullShape=true)
{
      if (numBricksPerRow<3) numBricksPerRow = 3;
      
      const btVector3 brickHalfDimensions = brickFullDimensions*btScalar(0.5);
      const btScalar h=brickFullDimensions.y(); const btScalar hh= h*0.5f;
      const btScalar l=brickFullDimensions.x(); const btScalar hl= l*0.5f; 
      const btScalar z=brickFullDimensions.z(); const btScalar hz= z*0.5f;  

      const btScalar perimeter = l*numBricksPerRow;
      const btScalar radius = perimeter/(2.0f*3.1415f) + hz;
      const btScalar elementOffsetAngle = btRadians(360.0f/numBricksPerRow);
      const btScalar floorOffsetAngleBase = elementOffsetAngle * 0.5f;
      btScalar collisionMargin(0.0);
      
      btCollisionShape* shape = NULL;
      if (!useConvexHullShape) shape = new btBoxShape(brickHalfDimensions);
      else   {
         const btScalar HL = hl + z * btTan(floorOffsetAngleBase);      

         btVector3 points[8]={
            btVector3(HL,hh,hz),
            btVector3(-HL,hh,hz),
            btVector3(hl,hh,-hz),
            btVector3(-hl,hh,-hz),

            btVector3(HL,-hh,hz),
            btVector3(-HL,-hh,hz),
            btVector3(hl,-hh,-hz),
            btVector3(-hl,-hh,-hz)            
         };
         //btConvexHullShape* chs = new btConvexHullShape(&points.x(),sizeof(points)/sizeof(points[0]));
         // /*
         btConvexHullShape* chs = new btConvexHullShape();
         for (int t=0;t<sizeof(points)/sizeof(points[0]);t++) chs->addPoint(points[t]);
         // */
         shape = chs;
         collisionMargin = 2.0f * shape->getMargin();   // ...so when using the convex hull shape we keep it into considerations (even if decreasing it probably helps)
      }
      if (!shape) return NULL;                        
      
      btCompoundShape* csh = new btCompoundShape();
      
      btTransform T=btTransform::getIdentity();
      T.setOrigin(T.getOrigin()+T.getBasis().getColumn(1)*(hh+collisionMargin*0.5f));   
      btTransform T2;
      {
         for (unsigned f=0;f< numRows;f++)   {
            btScalar floorOffsetAngle = (f%2==1 ? floorOffsetAngleBase : 0.0f);
            for (unsigned t=0;t< numBricksPerRow;t++)   {
               T2=T;                  
               T2.setRotation(btQuaternion(t * elementOffsetAngle+ floorOffsetAngle,0,0));//assignAY( t * elementOffsetAngle+ floorOffsetAngle );
               T2.setOrigin(T2.getOrigin()+T2.getBasis().getColumn(2)*radius);                     
               // Body Creation:
               csh->addChildShape(T2,shape);
            }
            T.setOrigin(T.getOrigin()+T.getBasis().getColumn(1)*(h+collisionMargin));                                    
         }
      }   

      return csh;
}

void PopulateBulletContainer( struct BulletInfo *_bullet )
{
	/* build the ground */
	 btCollisionShape* groundShape = BuildTowerCompoundShape();

	  btDefaultMotionState* groundMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));

	  btRigidBody::btRigidBodyConstructionInfo
                groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));

        btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);

		groundRigidBody->setRestitution( 1.0f );
		groundRigidBody->setFriction( 0.01f );

		_bullet->dynamicsWorld->addRigidBody(groundRigidBody);


}



void PopulateBulletTest( struct BulletInfo *_bullet )
{

		/* build a falling ball */
	  btCollisionShape* fallShape = new btSphereShape(1);
	  btDefaultMotionState* fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));

		btScalar mass = 1;
        btVector3 fallInertia(0,0,0);
        //fallShape->calculateLocalInertia(mass,fallInertia);

		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,fallShape,fallInertia);
        btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
		_bullet->fallRigidBody = fallRigidBody;
        _bullet->dynamicsWorld->addRigidBody(fallRigidBody);
}

void CreateMixerBar( struct BulletInfo *_bullet )
{
	btCollisionShape* barShape = new btCylinderShapeX( btVector3( 235, 10, 1.0 ) );

	  btDefaultMotionState* fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));

		btScalar mass = 100;
        btVector3 fallInertia(0,0,0);
        barShape->calculateLocalInertia(mass,fallInertia);

		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,barShape,fallInertia);
        l.barRigidBody = new btRigidBody(fallRigidBodyCI);
        _bullet->dynamicsWorld->addRigidBody(l.barRigidBody);

		l.barRigidBody->setRestitution( 0.8 );

	btHingeConstraint* pivot = new btHingeConstraint( *_bullet->groundRigidBody
		, *l.barRigidBody
		, btVector3( 0, 35, 0 ) // pivitInA 
		, btVector3( 0, 0, 0 ) // PivotInB
		, btVector3( 0, 1, 0 ) // axisInA
		, btVector3( 0, 1, 0 ) // axisInB
		, true // use refernceframeA (the ground)
		);
	lprintf( "yesno = %d", pivot->getAngularOnly() );
	lprintf( "Limits = %g", pivot->getLowerLimit() );
	lprintf( "Limits = %g", pivot->getUpperLimit() );
	pivot->setLimit( 1, -1
		, 0.1  // softness
		, 0.3 // biasFactor
		, 1.0f // relaxationFactor
		);
	//pivot->enableMotor( true );
	pivot->enableAngularMotor( true, 3, 100000 );

	//pivot->
	//pivot->setLimit(btScalar(-0.75 * M_PI_4), btScalar(M_PI_8));
			//hingeC->setLimit(btScalar(-0.1), btScalar(0.1));
			_bullet->dynamicsWorld->addConstraint(pivot, true);
		
}



void TestPopulatedBullet( struct BulletInfo *_bullet )
{
	for (int i=0 ; i<300 ; i++) {
                _bullet->dynamicsWorld->stepSimulation(1/60.f,10);
 
                btTransform trans;
                _bullet->fallRigidBody->getMotionState()->getWorldTransform(trans);
 
                lprintf( "sphere height: %g", trans.getOrigin().getY() );
        }
}

void ReleaseBullet( struct BulletInfo *_bullet )
{
		// Clean up behind ourselves like good little programmers
    delete _bullet->dynamicsWorld;
    delete _bullet->solver;
    delete _bullet->dispatcher;
    delete _bullet->collisionConfiguration;
    delete _bullet->broadphase;

}

PRELOAD( InitDisplay )
{
	int n;
	SetupBullet( &l.bullet );
	PopulateBulletGround( &l.bullet );
	PopulateBulletContainer( &l.bullet );

	CreateMixerBar( &l.bullet );

	//PopulateBulletTest( &l.bullet );
	//TestPopulatedBullet( &l.bullet );



	for( n = 0; n < 75; n++ )
	{
		PHEXPATCH patch = CreatePatch( NULL );
		patch->T = CreateTransform();
		CreateTransformMotionEx( patch->T, 1 );
		RotateRel( patch->T, 0, M_PI/6, 0 );
		
		patch->origin[0] =  (n % 5) * 100 - 250 ;
		patch->origin[1] = (n / 5) * 100 ;
		patch->origin[2] = n;
		patch->number = n;

		patch->fallShape = new btSphereShape( PLANET_RADIUS );
		btDefaultMotionState* fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));

		btScalar mass = 0.0027; // kilo-grams
        btVector3 fallInertia(0,0,0);
        patch->fallShape->calculateLocalInertia(mass,fallInertia);

		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,patch->fallShape,fallInertia);
		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        patch->fallRigidBody = new btRigidBody(fallRigidBodyCI);
		patch->fallRigidBody->setRestitution( 0.4 );
		lprintf( "Default sphere was %g", patch->fallRigidBody->getFriction() );
		patch->fallRigidBody->setFriction( 0.7f ) ;
		patch->fallRigidBody->translate( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] ) );
		l.bullet.dynamicsWorld->addRigidBody(patch->fallRigidBody);


		AddLink( &l.patches, patch );
	}

	InitializeCriticalSec( &csUpdate );
	l.colors[0] = BASE_COLOR_BLUE;
	l.colors[1] = BASE_COLOR_RED;
	l.colors[2] = BASE_COLOR_PURPLE;
	l.colors[3] = BASE_COLOR_BROWN;
	l.colors[4] = BASE_COLOR_YELLOW;

	NetworkStart();
	{
		PLIST addresses = GetLocalAddresses();
		INDEX idx;
		SOCKADDR *sockaddr;
		LIST_FORALL( addresses, idx, SOCKADDR *, sockaddr )
		{
			// this will be network byte ordered address...
			DumpAddr( "A local address", sockaddr );
		}
	}

	AddTimer( 50, UpdatePositions, 0 );
}


PUBLIC( void, ExportThis )( void )
{
}
