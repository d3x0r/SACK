
#include <sack_types.h>
#include <vectlib.h>
#define SQRT3_OVER_2 1.7320508075688772935274463415059 / 2

// divisions...
#define HEX_SIZE 33

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
#define PLANET_CIRCUM (350*6.28318) //*(1000.0))
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
	float height[3][HEX_SIZE][HEX_SIZE];

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
   float band[6][HEX_SIZE+1][HEX_SIZE+1]; // band arond the center... 30 degrees...
   float pole_patch[2][3][HEX_SIZE+1][HEX_SIZE+1];
};

struct world_body_map
{
	class Body * band[12][HEX_SIZE][HEX_SIZE];
	struct {
		int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
	} near_areas[12][HEX_SIZE][HEX_SIZE][4]; // the 4 corners that are this hex square.
};

struct world_coords
{
	// 6 hexpatches which are 60 units wide.
	// 3 mate together for a length of 180 between any two centers
	// 6 poles, 3 pair 180 apart... it works, I don't quite get it as
	// I attempt to explain it.
   VECTOR band[6*HEX_SIZE][HEX_SIZE]; // band arond the center... 30 degrees...
   VECTOR pole_patch[3][HEX_SIZE+1][HEX_SIZE+1];// south pole is same map with negative Y coordinate.
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
   _POINT origin; // center of hex...
	RCOORD height[3][HEX_SIZE+1][HEX_SIZE+1];

} HEXPATCH, *PHEXPATCH;

