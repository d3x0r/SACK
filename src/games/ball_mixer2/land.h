
#include <sack_types.h>
#include <vectlib.h>
#define SQRT3_OVER_2 1.7320508075688772935274463415059 / 2

// divisions...
#define HEX_SIZE l.hex_count

// equitoral circumferance
//
// 24,901.55 miles (40,075.16 kilometers).
// 131480184 ft
// 40075160 m
//  6378159.7555797717226664788383706m radius
//
// right now 640 is the hex scale, would take
// 31308.71875 hexes to span the earth

// 111319.88888888888888888888888889   m per degree
// 1739.3732638888888888888888888889 m per 64th of a full hex/degree


// kilometer scale... 1:1m
//#define PLANET_CIRCUM ((6378*6.28318)*(1000.0))
// kilometer scale... 1:1km
#define PLANET_CIRCUM (45*6.28318) //*(1000.0))
// 10 kilometer scale  1:10km
//#define PLANET_CIRCUM (637.8*6.28318) //*(1000.0))
//#define PLANET_CIRCUM ((40075.16)*(1000.0))

#define PLANET_M_PER_ ((PLANET_CIRCUM)/(6.0))


#define PLANET_M_PER_DEG ((PLANET_CIRCUM)/(360.0))
#define PLANET_M_PER_MAJORHEX  ((PLANET_M_PER_DEG)/HEX_SIZE)

#define PLANET_RADIUS ((PLANET_CIRCUM)/(6.283185307179586476925286766559))
#define HEIGHT_AT_1_DEGREE ( PLANET_RADIUS * 0.000038076935828711262644835174 )

// sin = opp / hyp
// cos = adj / hyp

// 51492.623774494753828625694911041meters
//   242.86077971847967959579546315942
//
//
//
// 360 hexes around, in at least 3 directions, which mate
// at 180x180x180
// there is a sequence of hexes that map this...
//                                           ,
//                                      ,
//        ,                          ,
//            ,                   ,
//        .      ,             ,
//            .      ,      ,                                level + 1 = legnth of edge = 60(180/3)
//        7      .       ,                                     // so actually level 59 is the magic number
//
//   19       8      .   ,                                 1           0
//18      1       9                                           6  1*6    1
//    6       2      .   ,                                 12    2*6    2
//16      0      10                      .                   18  3*6    3
//    5       3      .   ,                                   24  4*6    4
//15      4      11                                              5*6
//   14      12      .   ,                                    so at what stage is it 180?
//                                                           // 30 ... so only 30 out becomes the full circumferance... but I still have to go 150
//.      13       .      ,                                              ,     ,
//    .       .       ,     ,                                        ,     ,     ,
//        .       ,   13       ,
/////         ,     7    8  14    ,                                  ,     ,     ,
//        ,      19  1  2            ,                            ,     ,     ,     ,
//              12 4      3 9           ,
//               17  5  6   15             ,                      ,     ,     ,     ,
//                  11   10                   ,                      ,     ,     ,
	//                     16
//                                                                   ,     ,     ,
//                                                                      ,     ,
//
	//
//.      13       .      ,                                      q ,     ,     ,
//    .       .       ,     ,                                  ,     ,     ,     ,
//        .       ,   13       ,                              r 2 a
/////         ,     7    8  14    ,                            ,    1,     ,     ,
//        ,      19  1  2            ,                            ,     ,     ,     ,
//              12 4      3 9           ,                                1 a
//               17  5  6   15             ,                      ,     ,    2,     ,
//                  11   10                   ,                      ,     ,     , q
	//                     16                                                     r
//                                                                   ,     ,     ,
//                                                                      ,     ,
//


/*
 *
 *                                         sqrt(3)/2  = 1                     .
 *                                                                        _,     ,_
 *                                                                      ,/    ,    \,
 *                                                                   .  |     |     |  .
//.      13       .      ,                                              ,     ,     ,
//    .       .       ,     ,                                        ,     ,     ,     ,
//        .       ,   13       ,                                    /|     |  .  |     |\
/////         ,     7    8  14    ,                                 \,     ,     ,     ,/
//        ,      19  1  2            ,                                  ,     ,     ,
//              12 4      3 9           ,                            .  |     |     |  .
//               17  5  6   15             ,                            ,     ,     ,
//                  11   10                   ,                          \_,     ,_/
 *                                                                            .
 *
 *
 *
 *


 the length of each section at the equator

40075160
 6679193.3333333333333333333333333
        .
     .     .
         ____
     .   \ .
 *      . \                                sqrt(3)/2  = 1                     .
 *                                                  .   .                  ,     ,
 *                                               .   \ /   .            ,     ,     ,
 *                                          .     \   .   /          .  |     |     |  .
//.      13       .      ,               .     .___.     .___.          ,     ,     ,
//    .       .       ,     ,               *  |   |  *  |   |             ,     ,
//        .       ,   13       ,         .     .___.     .___.            /|  .  |\
/////         ,     7    8  14    ,         .     /   .   \               \,     ,/
//        ,      19  1  2            ,           .   / \   .            ,     ,     ,
//              12 4      3 9           ,           .   .            .        |     |  .
//               17  5  6   15             ,                            ,     ,     ,
//                  11   10                   ,                            ,     ,
 *                                                                            .
 *
 *
 *
 *
 *
 *
 *                                              ___
	//                                          | x |   .
	//                                       / \ ___  /  \
	//                                       \y /\  /\ x
	//                                        \/___/_2\/  ...
	//                                        /\  /\  /\  ...
	//                                       /y \___\/ y\
	//                                       \ /   x  \./
	//                                          |___|
 *
 *            \_\_\_\_\_\_\_\
 *             \_\_\_\_\_\_\_\
 *              \_\_\_\_\_\_\_\
 *               \ \ \ \ \ \ \ \
 *               _ _____________
 *              _ / ___________                 ___
	//          _ / / _________                 | x |   .
	//         _ / / / _______               / \ ___  /  \
	//        _ / / / / _____                \y /\  /\ x
	//       _ / / / / / ___                  \/___/_2\/  ...
	//      _ / / / / / / _                   /\  /\  /\  ...
	//       / / / / / / / .                 /y \___\/ y\
	//                                       \ /   x  \./
	//                                          |___|
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */


struct degree_patch
{
   // and still this, this is like 1.7m resolution
   float height[3][64][64];
};

struct hexpatch
{
	//float height[3][60][60];
	// resoution of 111.31km
	// 111319.88888888888888888888888889
	// 64 1739.3732638888888888888888888889
	// 64 27.177707248263888888888888888889
   // 27
	float **height[3];//[HEX_SIZE][HEX_SIZE];
	hexpatch(int size )
	{
		int n,m;
		for( n = 0; n < 3; n++ )
		height[n] = NewArray( float*, size );
		for( m = 0; m < size; m++ )
			height[n][m] = NewArray( float, size );
	}
   // this hexpatch is mapped differently...

	// a hex patch is 3 sets of squares..
	//
	// radial mapping here...
	// it is produced with level, r
   // where level is 0-hexsize from center to outside.
	// r is a counter clockwise rotation from zero to 120 degrees in hex_size total intervals
	// land hex
	//     x - zero at origin. y zero at origin.
	//    ___
	// y / 0 /\ y - zero at origin y at origin
	//  /___/+1\    ...
	//  \ +2\  /    ...
	// x \___\/ x
	//  3l y
	//
	//  edge from zero degrees is section 0, axis y
   //  edge from 60 degrees is section 0 axis hesize-x
	// building the map of coordinates results in differnet 'nearness' constants...
   //

	// level, R is near
	// so along the way one bit is dropped, but then the average geometry is
   // square, but there is a slow migration towards being diagonal... 45 degrees almost.


};

struct world_height_map
{
	// 6 hexpatches which are 60 units wide.
	// 3 mate together for a length of 180 between any two centers
	// 6 poles, 3 pair 180 apart... it works, I don't quite get it as
	// I attempt to explain it.
	int hex_size;
   float **band[6];//[HEX_SIZE+1][HEX_SIZE+1]; // band arond the center... 30 degrees...
   float **pole_patch[2][3];//[HEX_SIZE+1][HEX_SIZE+1];
   world_height_map( int size )
   {
	   int n, m;
	   hex_size = size + 1;
	   for( n = 0; n < 6; n++ )
	   {
		   band[n] = NewArray( float *, hex_size );
		   for( m = 0; m < hex_size; m++  )
			   band[n][m] = NewArray( float, hex_size );
		   pole_patch[0][n] = NewArray( float *, hex_size );
		   for( m = 0; m < hex_size; m++  )
			   pole_patch[0][n][m] = NewArray( float, hex_size );

	   }
   }
};

struct world_body_map
{
	class Body * **band[12];//[HEX_SIZE][HEX_SIZE];
	struct near_area2 {
		int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
	} ***near_areas[12];//[HEX_SIZE][HEX_SIZE][4]; // the 4 corners that are this hex square.

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


	world_body_map( int size )
	{
		int n;
		for( n = 0; n < 12; n++ )
		{
			int m;
			band[n] = NewArray( class Body **, size );
			for( m = 0; m < size; m ++ )
				band[n][m] = NewArray( class Body *, size );
			near_areas[n] = NewArray( struct near_area2 **, size );
			for( m = 0; m < size; m ++ )
			{
				int k;
				near_areas[n][m] = NewArray( struct near_area2 *, size );
				for( k = 0; k < size; k++ )
					near_areas[n][m][k] = NewArray( struct near_area2, 4 );			
			}
			
		}

		int s, r, c;
			for( s = 0; s < 6; s++ )
			for( r = 0; r < size; r++ )
				for( c = 0; c < size; c++ )
				{
					if( c < (size-1) )
					{
						near_areas[s+3][c][r][0].s = s+3;
						near_areas[s+3][c][r][0].x = c+1;
						near_areas[s+3][c][r][0].y = r;
					}
					else
					{
						near_areas[s+3][c][r][0].s = (s+1)%6+3;
						near_areas[s+3][c][r][0].x = 0;
						near_areas[s+3][c][r][0].y = r;
					}
					if( r < (size-1) )
					{
						near_areas[s+3][c][r][1].s = s+3;
						near_areas[s+3][c][r][1].x = c;
						near_areas[s+3][c][r][1].y = r+1;
					}
					else
					{
						near_areas[s+3][c][r][1].s = (s/2)+9;
						ConvertPolarToRect( size, ((s%2)?size:0)+c
												 , &near_areas[s+3][c][r][1].x
												 , &near_areas[s+3][c][r][1].y
												);
					}
					if( c )
					{
						near_areas[s+3][c][r][2].s = s+3;
						near_areas[s+3][c][r][2].x = c-1;
						near_areas[s+3][c][r][2].y = r;
					}
					else
					{
						near_areas[s+3][c][r][2].s = (s+5)%6+3;
						near_areas[s+3][c][r][2].x = size-1;
						near_areas[s+3][c][r][2].y = r;
					}
					if( r )
					{
						near_areas[s+3][c][r][3].s = s+3;
						near_areas[s+3][c][r][3].x = c;
						near_areas[s+3][c][r][3].y = r-1;
					}
					else
					{
						near_areas[s+3][c][r][3].s = (s/2);
						ConvertPolarToRect( size, ((s%2)?size:0)+c
												 , &near_areas[s+3][c][r][3].x
												 , &near_areas[s+3][c][r][3].y
												);
						lprintf( WIDE("%d,%d,%d is near(3) %d,%d,%d")
								 , s+3, c, r
								 , near_areas[s+3][c][r][3].s
								 , near_areas[s+3][c][r][3].x
								 , near_areas[s+3][c][r][3].y
								 );
					}
				}

			{
				int s, x, y;
				for( s = 0; s < 3; s++ )
				{
					for( x = 0; x < size; x++ )
						for( y = 0; y < size; y++ )
						{
							if( x < (size-1) )
							{
								near_areas[s][x][y][0].s = s;
								near_areas[s][x][y][0].x = x + 1;
								near_areas[s][x][y][0].y = y;

								near_areas[s+9][x][y][0].s = near_areas[s][x][y][0].s + 9;
								near_areas[s+9][x][y][0].x = near_areas[s][x][y][0].x;
								near_areas[s+9][x][y][0].y = near_areas[s][x][y][0].y;
							}
							else
							{
								near_areas[s][x][y][0].s = s+3;
								near_areas[s][x][y][0].x = x;
								near_areas[s][x][y][0].y = 0;
								near_areas[s+9][x][y][0].s = near_areas[s][x][y][0].s + 3;
								near_areas[s+9][x][y][0].x = near_areas[s][x][y][0].x;
								near_areas[s+9][x][y][0].y = size-1;
							}
							if( y < (size-1) )
							{
								near_areas[s][x][y][1].s = s;
								near_areas[s][x][y][1].x = x;
								near_areas[s][x][y][1].y = y + 1;

								near_areas[s+9][x][y][1].s = near_areas[s][x][y][1].s + 9;
								near_areas[s+9][x][y][1].x = near_areas[s][x][y][1].x;
								near_areas[s+9][x][y][1].y = near_areas[s][x][y][1].y;
							}
							else
							{
								near_areas[s][x][y][1].s = s+4;
								near_areas[s][x][y][1].x = y;
								near_areas[s][x][y][1].y = 0;

								near_areas[s+9][x][y][0].s = near_areas[s][x][y][0].s + 4;
								near_areas[s+9][x][y][0].x = near_areas[s][x][y][0].x;
								near_areas[s+9][x][y][0].y = size-1;
							}
							if( x )
							{
								near_areas[s][x][y][2].s = s;
								near_areas[s][x][y][2].x = x - 1;
								near_areas[s][x][y][2].y = y;

								near_areas[s+9][x][y][2].s = near_areas[s][x][y][2].s + 9;
								near_areas[s+9][x][y][2].x = near_areas[s][x][y][2].x;
								near_areas[s+9][x][y][2].y = near_areas[s][x][y][2].y;
							}
							else
							{
								near_areas[s][x][y][2].s = (s+2)%3;
								near_areas[s][x][y][2].x = y;
								near_areas[s][x][y][2].y = 0;

								near_areas[s+9][x][y][2].s = near_areas[s][x][y][2].s+9;
								near_areas[s+9][x][y][2].x = near_areas[s][x][y][2].x;
								near_areas[s+9][x][y][2].y = near_areas[s][x][y][2].y;
							}
							if( y )
							{
								near_areas[s][x][y][3].s = s;
								near_areas[s][x][y][3].x = x;
								near_areas[s][x][y][3].y = y - 1;

								near_areas[s+9][x][y][3].s = near_areas[s][x][y][3].s + 9;
								near_areas[s+9][x][y][3].x = near_areas[s][x][y][3].x;
								near_areas[s+9][x][y][3].y = near_areas[s][x][y][3].y;
							}
							else
							{
								near_areas[s][x][y][3].s = (s+1)%3;
								near_areas[s][x][y][3].x = 0;
								near_areas[s][x][y][3].y = x;

								near_areas[s+9][x][y][3].s = near_areas[s][x][y][3].s+9;
								near_areas[s+9][x][y][3].x = near_areas[s][x][y][3].x;
								near_areas[s+9][x][y][3].y = near_areas[s][x][y][3].y;
							}
						}
				}
			}

	}

};

// total world size is 64800 elements, 259200 bytes


// mesh of 796262400 floating point heights
// 3G (3185M) (above number *4)

//struct world_map too_big;
//
//
//
//
//

//
//     // how to make a sensible maping of sectors in a beleivable 360 section map, with 360x360x360 bands
//
//
//
//
//
//
//
//
//

// length of an edge - divided by HEX_SIZE = width of segment
#define HEX_SCALE PLANET_M_PER_MAJORHEX

// Vertical_stride
#define HEX_VERT_STRIDE ( ( HEX_SCALE * 1.7320508075688772935274463415059 / 2 ) /*/ (double)HEX_SIZE*/ )

// horizontal_stride
#define HEX_HORZ_STRIDE ( ( HEX_SCALE ) /*/ (double)HEX_SIZE*/ )


typedef struct hexpatch_tag
{
	struct patch_flags
	{
		BIT_FIELD fade : 1;
		BIT_FIELD simulated : 1;
		BIT_FIELD grabbed : 1;  // this is the one we're showing... (may not be used)
		BIT_FIELD racked : 1; // this ball has been moved to its position in the rack;
		BIT_FIELD removed_before_simulated : 1;
	} flags;
	_32 time_to_rack;
	
	RCOORD rack_delta;  // used to compute orthagonal update for perspective camera....
		
	int number;
	_POINT origin; // center of hex...
	PTRANSFORM T;
	// 0-2 south pole, 3-8 band heights, 9-11 north pole
	RCOORD **height[12];//[HEX_SIZE+1][HEX_SIZE+1];
	btCollisionShape* fallShape;
	btRigidBody* fallRigidBody;
	_32 fade_target_tick;
	int max_hex_size;
	int hex_size;
	struct band *band;
	struct pole *pole;
	Image label;
	GLfloat *verts;
	GLfloat *norms;
	GLfloat *colors;
	
} HEXPATCH, *PHEXPATCH;



