//#define DEBUG_TIMING

/*
 * Theory of operation?
 *
 * idle: shows ball rack behind, for all called balls, and blower in middle
 *    move any balls that are racking toward the rack
 *  (Idle transition in demo mode, timer ticks and kicks BeginWatch with rand() )
 *  (Idle transition in non-demo, BeginWatch on the next queued ball, if not active, no next, etc...)
 * 1) BeginWatch
 *     Points the camera at the ball/orients the ball toward the camera,
 *     sets l.watch_ball_tick, sets l.next_active_ball
 *  when l.watch_ball_tick expires, BeginApproach();
 * 2) BeginApproach
 *     move the ball toward the camera/ moves the camera toward the ball,
 *     sets l.next_active_tick
 *  when l.next_active_tick expires, l.active_ball is updated to l.next_active_ball
 *  if nextball_mode, then BeginPivot, else wait for call, then BeginPivot
 * 3) BeginPivot
 *     starts orienting the l.active_ball to face the room,
 *     sets l.active_ball_forward_tick (this stays set, and if it has elapsed, is just a constant
 *   At some point EndWatchBall() is called.
 * 4) EndWatchBall
 *     sets l.return_to_home
 *   when l.return_to_home elapses, the current l.active_ball is set to fade, l.active_ball is set to 0,
 *   return_to_home is set to 0
 *
 *  phase 1) l.next_active_ball = 0;
 *           l.active_ball = 0;
 *  phase 2) [align/approach ball, delta on watch_ball, delta on active_tick to approach]
 *           l.next_active_ball = n;
 *           l.active_ball = 0;
 *           l.watch_ball_tick (goes not 0, then 0 when expired)
 *           l.next_active_tick (set to not zero, then 0 when expired)
 *  phase 3) [show ball back, stays in this mode, unless was nextball, which auto triggers to ball_forward ]
 *           l.next_active_ball = 0;
 *           l.active_ball = n;
 *           l.active_ball_forward_tick = 0;
 *  phase 4) [show ball number, if forward tick is not exipired, apply as a fractional rotation]
 *           l.next_active_ball = 0;
 *           l.active_ball = n;
 *           l.active_ball_forward_tick = n;
 *  phase 5) [release the currrent ball back to mixer to fade/send ball to rack]
 *           l.return_to_home = n;
 *
 */

#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE
#define TERRAIN_MAIN_SOURCE
//#define MAKE_RCOORD_SINGLE
#include <sqlgetoption.h>
#include <math.h>
#include <render.h>
#include <render3d.h>
#include <vectlib.h>
#include <sharemem.h>
#include <psi.h>

#include <btBulletDynamicsCommon.h>

#include <GL/glew.h>
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

#define TIME_TO_TURN_BALL  l.time_to_turn_ball
#define TIME_SCALE (1.0f)

#define TIME_TO_APPROACH l.time_to_approach
#define TIME_TO_WATCH l.time_to_track
#define TIME_TO_HOME l.time_to_home 

#include "local.h"

EasyRegisterControlWithBorder( WIDE("Terrain View"), 0, BORDER_NONE );

struct world_body_map *bodymap;

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
	struct corner_tag {
		int r, c;
	} ***corners;// the 4 corners that are this hex square.
	struct  band_patch_tag {
		// grid is col, row (x, y)
		VECTOR **grid; // +1 cause there's this many squares bounded by +1 lines.
		struct near_sector {
			int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
		} ***near_area; // the 4 corners that are this hex square.
		// each area is near 4 other areas.
		// these areas are
		//        1
		//        |
		//    2 - o - 0
		//        |
		//        3
		//

	} patches[6]; // actually this is continuous...
	int max_hex_size;
	int hex_size;

	struct band_render_info {
		Image source;
		struct SACK_3D_Surface *fragment;
	};
	PLIST bands;  

	void CreateBandFragments2( void )
	{
	}


	void CreateBandFragments( void )
	{
		struct SACK_3D_Surface *tmp;
		int verts = ( hex_size + 1 ) * 2;
		_POINT *tangents = NewArray( _POINT, verts );
		_POINT *texture = NewArray( _POINT, verts );
		int nShape;
	
		struct band *band = this;
		int section, section2;
		int sections = 6*band->hex_size;
		int sections2= band->hex_size;


		for( section = 0; section < sections; section ++ )
		{
			_POINT *shape = NewArray( _POINT, verts);
			int other_s = (section/(band->hex_size+1))%6; // might be 6, which needs to be 0.
			int s = (section/band->hex_size)%6; // might be 6, which needs to be 0.
			int s2 = ((section+1)/band->hex_size)%6; // might be 6, which needs to be 0.
			int section_offset = ( section % band->hex_size );
			int section_offset_1_a = ( (section+1) % band->hex_size );
		
			nShape = 0;
			for( section2 = 0; section2 <= sections2; section2++ )
			{
				_POINT tmp2;
				scale( shape[nShape], band->patches[s].grid[section_offset][section2], SPHERE_SIZE );
				//texture[nShape][0] = l.numbers.coords[row][col][section_offset][section2][0];
				//texture[nShape][1] = l.numbers.coords[row][col][section_offset][section2][1];
				//crossproduct( tmp1, _Z, shape[nShape] );
				// cross with Y to get horizontal.
				crossproduct( tmp2, _Y, shape[nShape] );
				//if( Length( tmp1 ) > Length( tmp2 ) )
				//	SetPoint( tangents[nShape], tmp1 );
				//else
					SetPoint( tangents[nShape], tmp2 );
				//normalize(shape[nShape]);
				normalize( tangents[nShape] );
				{
					//lprintf( "texture point %d", nShape );
					//PrintVector( shape[nShape] );
					//PrintVector( tangents[nShape] );

					//crossproduct( tmp1, shape[nShape], tangents[nShape] );
					//normalize( tmp1 );
					//PrintVector( tmp1);
				}

				nShape++;
				
				// s2 can change patches, whereas the number coords... go forward
				scale( shape[nShape], band->patches[s2].grid[section_offset_1_a][section2], SPHERE_SIZE );
				//texture[nShape][0] = l.numbers.coords[row][col][section_offset+1][section2][0];
				//texture[nShape][1] = l.numbers.coords[row][col][section_offset+1][section2][1];
				crossproduct( tangents[nShape], _Y, shape[nShape] );
				normalize( tangents[nShape] );
				nShape++;
			}
			AddLink( &bands, tmp = CreateBumpTextureFragment( verts, (PCVECTOR*)shape, (PCVECTOR*)shape
				, (PCVECTOR*)tangents, (PCVECTOR*)NULL, (PCVECTOR*)texture ) );
			tmp->color = (((section/band->hex_size)%6) < 3)?TRUE:FALSE;

		}
	}

	void Texture( int number )
	{
		int verts = ( hex_size + 1 ) * 2;
		_POINT *texture = NewArray( _POINT, verts );
		int nShape;
	
		struct band *band = this;
		int section, section2;
		int sections = 6*band->hex_size;
		int sections2= band->hex_size;
		int row;
		int col;

		if( number )
		{
			row = (number-1)/l.numbers.cols;
			col = (number-1)%l.numbers.cols;
		}
		for( section = 0; section < sections; section ++ )
		{
			struct SACK_3D_Surface * surface = (struct SACK_3D_Surface *)GetLink( &bands, section );
			if( surface )
			{
				int section_offset = ( section % band->hex_size );
				int section_offset_1 = ( (section) % band->hex_size ) + 1;
				nShape = 0;
				for( section2 = 0; section2 <= sections2; section2++ )
				{
					if( number )
					{
						texture[nShape][0] = l.numbers.coords[row][col][section_offset][section2][0];
						texture[nShape][1] = l.numbers.coords[row][col][section_offset][section2][1];
						texture[nShape][2] = 0;
						nShape++;

						// s2 can change patches, whereas the number coords... go forward
						texture[nShape][0] = l.numbers.coords[row][col][section_offset+1][section2][0];
						texture[nShape][1] = l.numbers.coords[row][col][section_offset+1][section2][1];
						texture[nShape][2] = 0;
						nShape++;
					}
					else
					{
						texture[nShape][0] = (float)section_offset / band->hex_size;
						texture[nShape][1] = 1.0f - (float)section2/band->hex_size;
						texture[nShape][2] = 0;
						nShape++;

						// s2 can change patches, whereas the number coords... go forward
						texture[nShape][0] = (float)section_offset_1 / band->hex_size;
						texture[nShape][1] = 1.0f - (float)section2/band->hex_size;
						texture[nShape][2] = 0;
						nShape++;
					}
				}
				//lprintf( "verts %d shape %d", verts, nShape );
				AddBumpTextureFragmentTexture( surface, number, (PCVECTOR*)texture );
			}
		}
	}


	void resize( int size )
	{
		hex_size = size;
		if( hex_size > max_hex_size )
		{
			if( max_hex_size )
			{
				{
					int n;
					for( n = 0; n < max_hex_size; n++ )
					{
						int m;
						for( m = 0; m < max_hex_size; m ++ )
							Release( corners[n][m] );
						Release( corners[n] );
					}
					Release( corners );

					for( n = 0; n < 6; n++ )
					{
						int m;
						for( m = 0; m < max_hex_size; m++ )
							Release( patches[n].grid[m] );
						Release( patches[n].grid );
						for( m = 0; m < max_hex_size; m++ )
						{
							int k;
							for( k = 0; k < max_hex_size + 1; k++ )
								Release( patches[n].near_area[m][k] );
							Release( patches[n].near_area[m] );
						}
						Release( patches[n].near_area );
					}
				}

			}
			// allocate
			{
				{
					int n;
					corners = NewArray( struct corner_tag**, size );
					for( n = 0; n < size; n++ )
					{
						int m;
						corners[n] = NewArray( struct corner_tag *, size );
						for( m = 0; m < size; m ++ )
							corners[n][m] = NewArray( struct corner_tag, 4 );
					}
					for( n = 0; n < 6; n++ )
					{
						int m;
						patches[n].grid = NewArray( VECTOR *, size );
						for( m = 0; m < size; m++ )
							patches[n].grid[m] = NewArray( VECTOR, size + 1 );
						patches[n].near_area = NewArray( struct band_patch_tag::near_sector **, size );
						for( m = 0; m < size; m++ )
						{
							int k;
							patches[n].near_area[m] = NewArray( struct band_patch_tag::near_sector *, size + 1 );
							for( k = 0; k < size + 1; k++ )
								patches[n].near_area[m][k] = NewArray( struct band_patch_tag::near_sector, 4 );
						}
					}
				}
			}
		}
				{
					PTRANSFORM work;
					int section, section2;
					int sections = 6*hex_size;
					int sections2= hex_size;
						work = CreateTransform();

					//scale( ref_point, VectorConst_X, SPHERE_SIZE );

					for( section2 = 0; section2 <= sections2; section2++ )
					{
						VECTOR patch1x;
						RotateAbs( work, 0, 0, ((((60.0/hex_size)*section2)-30.0)*(1*M_PI))/180.0 );
						GetAxisV( work, patch1x, vRight );
						for( section = 0; section < sections; section ++ )
						{
							RotateAbs( work, 0, ((float)section*(2.0*M_PI))/(float)sections, 0 );
							Apply( work
								, patches[section/hex_size].grid[section%hex_size][section2]
							, patch1x );
						}
					}
					DestroyTransform( work );
				}
		CreateBandFragments( );
	}

	band( int size ) 
	{
		bands = NULL;
		max_hex_size = 0;
		resize( size );

	}
	void ExportPoints( FILE *file )
	{
		int s;
		int r;
		int c;
		for( s = 0; s < 6; s++ )
			for( r = 0; r < hex_size; r++ )
				for( c = 0; c < hex_size; c++ )
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
			for( r = 0; r < hex_size; r++ )
				for( c = 0; c < hex_size; c++ )
				{
					int p;
					fprintf( file, WIDE("\t\t") );
					for( p = 0; p < 4; p++ )
					{
						// indexes of points for squares are...
						int idx;
						idx = s * hex_size * hex_size + c * hex_size + r;
						fprintf( file, WIDE("%d,%d,%d,%d,-1,"), idx, idx + 1, idx + hex_size, idx + hex_size + 1 );
					}
					fprintf( file, WIDE("\n") );
				}
	}
};

// near_area points and this semi-polar representation needs a rectangular conversion

struct pole{
	int max_hex_size;
	int hex_size;
	int ***corners;//[HEX_SIZE][HEX_SIZE][4];
	struct pole_patch_tag {
		VECTOR **grid;//[HEX_SIZE+1][HEX_SIZE+1];
		//int triangles[HEX_SIZE][HEX_SIZE*2][3];
		struct pole_near_area {
			int s, x, y; // global map x, y that this is... this could be optimized cause y is always related directly to X by HEX_SIZE*y+x
			// indexed by x, y from level, c translation.  Is the 4 near squares to any given point.
         // on the pole itself, this is short 1, and .s = -1
		} ***near_area;//[HEX_SIZE+1][HEX_SIZE+1][4]; // the 4 corners that are this hex square.
	} patches[3];
	PLIST pole_bands[2];
	PLIST bands;
	int number;
	void CreatePoleFragments( void )
	{
		int maxverts = ( hex_size + 1 ) * 2;
		int s, level, c, r;
		int x, y; // used to reference patch level
		//int x2, y2;
		int bLog = 0;

	#ifdef DEBUG_RENDER_POLE_NEARNESS
		bLog = 1;
	#endif
//__try
		{
			int north;
			int verts = ( hex_size + 1 ) * 2;
			_POINT *tangents = NewArray( _POINT, verts );
			_POINT *texture = NewArray( _POINT, verts );
			_POINT *shape = NewArray( _POINT, verts);
			int nShape;

			for( north = 0; north <= 1; north++ )
			{
				struct SACK_3D_Surface *tmp;
				for( s = 0; s < 3; s++ )
				{
					for( level = 1; level <= hex_size; level++ )
					{

						nShape = 0;

						r = 0;

						for( c = 0; c <= level; c++ )
						{
							ConvertPolarToRect( level, c, &x, &y );
							scale( shape[nShape], patches[s].grid[x][y]
									, SPHERE_SIZE /*+ height[s+(north*6)][x][y]*/ );
							if( north )
								shape[nShape][1] = -shape[nShape][1];
					
							crossproduct( tangents[nShape], _Y, shape[nShape] );
							normalize( tangents[nShape] );

							nShape++;
							if( c < (level) )
							{
								ConvertPolarToRect( level-1, c, &x, &y );
								scale( shape[nShape], patches[s].grid[x][y], SPHERE_SIZE /*+ patch->height[s+(north*6)][x][y]*/ );
								if( north )
									shape[nShape][1] = -shape[nShape][1];
								crossproduct( tangents[nShape], _Y, shape[nShape] );
								normalize( tangents[nShape] );
								nShape++;
							}
							r++;
						}

						AddLink( &bands, tmp = CreateBumpTextureFragment( nShape, (PCVECTOR*)shape, (PCVECTOR*)shape, (PCVECTOR*)tangents, (PCVECTOR*)NULL, (PCVECTOR*)NULL ) );
						switch( s )
						{
						case 0:
						case 1:
							tmp->color = 1;
							break;
						case 2:
							tmp->color = 0;
							break;
						}

						nShape = 0;
						if( bLog )lprintf( WIDE("---------") );


						for( c = level; c <= level*2; c++ )
						{
							ConvertPolarToRect( level, c, &x, &y );
							//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c,level);
							scale( shape[nShape], patches[s].grid[x][y], SPHERE_SIZE /*+ patch->height[s+(north*6)][x][y]*/ );
							if( north )
								shape[nShape][1] = -shape[nShape][1];

							crossproduct( tangents[nShape], _Y, shape[nShape] );
							normalize( tangents[nShape] );
							nShape++;
							if( c < (level)*2 )
							{

								ConvertPolarToRect( level-1, c-1, &x, &y );
								//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c-1,level-1);
								scale( shape[nShape], patches[s].grid[x][y], SPHERE_SIZE /*+ patch->height[s+(north*6)][x][y]*/ );
								if( north )
									shape[nShape][1] = -shape[nShape][1];
								crossproduct( tangents[nShape], _Y, shape[nShape] );
								normalize( tangents[nShape] );
								nShape++;
							}
						}
						
						AddLink( &bands, tmp = CreateBumpTextureFragment( nShape, (PCVECTOR*)shape, (PCVECTOR*)shape, (PCVECTOR*)tangents, (PCVECTOR*)NULL, (PCVECTOR*)NULL ) );

						switch( s )
						{
						case 0:
							tmp->color = 1;
							break;
						case 1:
						case 2:
							tmp->color = 0;
							break;
						}

						nShape = 0;
					}
				}
			}
		}
		//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") );return 0; }
	}


	void resize( int size )
	{
		hex_size = size;
		if( hex_size > max_hex_size )
		{
			if( max_hex_size )
			{
				int n, m;
				for( n = 0; n < max_hex_size; n++ )
				{
					for( m = 0; m < max_hex_size; m++ )
						Release( corners[n][m] );
					Release( corners[n] );
				}
				Release( corners );

				for( n = 0; n < 3; n++ )
				{
					for( m = 0; m < max_hex_size + 1; m++ )
					{
						int k;
						for( k = 0; k < max_hex_size + 1; k++ )
						{
							Release( patches[n].near_area[m][k] );
						}
						Release( patches[n].near_area[m] );
					}
					Release( patches[n].near_area );
					for( m = 0; m < max_hex_size + 1; m++ )
						Release( patches[n].grid[m] );
					Release( patches[n].grid );
				}
			}
			max_hex_size = hex_size;
			{
				int n, m;
				corners = NewArray( int **, hex_size );
				for( n = 0; n < hex_size; n++ )
				{
					corners[n] = NewArray( int *, hex_size );
					for( m = 0; m < hex_size; m++ )
						corners[n][m] = NewArray( int, 4 );
				}
				for( n = 0; n < 3; n++ )
				{
					patches[n].grid = NewArray( VECTOR *, hex_size+1 );
					for( m = 0; m < hex_size + 1; m++ )
						patches[n].grid[m] = NewArray( VECTOR, hex_size+1 );				
					patches[n].near_area = NewArray( struct pole_patch_tag::pole_near_area**, hex_size+1 );
					for( m = 0; m < hex_size + 1; m++ )
					{
						int k;
						patches[n].near_area[m] = NewArray( struct pole_patch_tag::pole_near_area*, hex_size+1 );
						for( k = 0; k < hex_size + 1; k++ )
						{
							patches[n].near_area[m][k] = NewArray( struct pole_patch_tag::pole_near_area, 4 );
						}
					}
				}
			}
		}
		{
			PTRANSFORM work;
			VECTOR patch1, patch1x;
			int level;
			int c;
			int patch;
				work = CreateTransform();
			//__try
			{
				for( patch = 0; patch < 3; patch++ )
				{
					for( level = 0; level <= hex_size; level++ )
					{
						RCOORD patch_bias = -((patch*120) * M_PI ) / 180;
						int sections = ( level * 2 );
						RotateAbs( work, 0, 0, ((((60.0/hex_size)*level))*(1*M_PI))/180.0 );
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
					DestroyTransform( work );
			//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") ); }
		}
		CreatePoleFragments();
	}
	pole( int size )
	{
		bands = NULL;
		max_hex_size = 0;
		resize( size );
	}
};


int mating_edges[6][2] = { { 0, 1 }, { 2, 1 }, { 2, 0 }
								 , { 1, 0 }, { 1, 2 }, { 0, 2 } };

	int number;


// if not north, then south.
int RenderPolePatch( PHEXPATCH patch, btScalar *m, int mode, int north )
{
	int s, level, c, r;
	float back_color[4];
	float fore_color[4];
	float tmpval[4];
   float fade;
	pole *pole_patch = patch->pole;
	int x, y; // used to reference patch level
	//int x2, y2;
	int bLog = 0;
#ifdef DEBUG_RENDER_POLE_NEARNESS
	bLog = 1;
#endif

	back_color[0] = RedVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[1] = GreenVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[2] = BlueVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[3] = 1.0f;

	fore_color[0] = 0.25f;
	fore_color[1] = 0.25f;
	fore_color[2] = 0.25f;
	fore_color[3] = 1.0f;

	if( patch->flags.fade )
	{
		if( patch->fade_target_tick < l.last_tick )
		{
			back_color[3]
            = fade
				= tmpval[3]
				= fore_color[3]
				= (RCOORD)( 0 );
		}
		else
		{
			back_color[3]
            = fade
				= tmpval[3]
				= fore_color[3]
				= (RCOORD)( ( patch->fade_target_tick - l.last_tick ) ) / (RCOORD)l.fade_duration;
		}
	}
	else
	{
		back_color[3] 
			= fade
			= tmpval[3]
			= 1.0;
		fore_color[3] = 1.0;
	}
	
	tmpval[0] = 0.5;//l.values[MAT_SPECULAR0] / 256.0f;
	tmpval[1] = 0.5;//l.values[MAT_SPECULAR1] / 256.0f;
	tmpval[2] = 0.5;//l.values[MAT_SPECULAR2] / 256.0f;
	//tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpval );
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64/*l.values[MAT_SHININESS]/2*/ ); // 0-128
	tmpval[0] = 0.5;
	tmpval[1] = 0.5;
	tmpval[2] = 0.45;
	//tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmpval );
	// create array of normals.
	//__try
	{
		if( !mode )
		{
			INDEX idx;
			struct SACK_3D_Surface * surface;
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);

			glUseProgram( l.shader.simple_shader.shader );
#ifdef BT_USE_DOUBLE_PRECISION
			glUniformMatrix4dv
#else
			glUniformMatrix4fv
#endif
				( l.shader.simple_shader.modelview, 1, GL_FALSE, m );

			glUseProgram( l.shader.normal_shader.shader );
			#ifdef BT_USE_DOUBLE_PRECISION
			glUniformMatrix4dv
#else
			glUniformMatrix4fv
#endif
				( l.shader.normal_shader.modelview, 1, GL_FALSE, m );

			glUseProgram( 0 );
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			glDisable(GL_VERTEX_PROGRAM_ARB);

			LIST_FORALL( patch->pole->bands, idx, struct SACK_3D_Surface *, surface )
			{
				if( surface->color )
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fore_color );
				else
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );

				// somehow this surface thing needs extra data for the fragment color
				if( surface->color )
					RenderBumpTextureFragment( NULL, NULL, m, 0, fore_color, surface );
				else
					RenderBumpTextureFragment( NULL, NULL, m, 0, back_color, surface );
			}
		}

	}

	if( mode )
	{
		glColorMaterial (GL_FRONT_AND_BACK, GL_DIFFUSE);
		for( s = 0; s < 3; s++ )
		{
			for( level = 1; level <= pole_patch->hex_size; level++ )
			{
				VECTOR v1,v2;

				{
					switch( s )
					{
					case 0:
					case 1:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fore_color );
						break;
					case 2:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );
						break;
					}
				}
				glBegin( GL_TRIANGLE_STRIP );
				r = 0;

				for( c = 0; c <= level; c++ )
				{
					ConvertPolarToRect( level, c, &x, &y );
					scale( v1, pole_patch->patches[s].grid[x][y]
							, SPHERE_SIZE + patch->height[s+(north*6)][x][y] );
					if( north )
						v1[1] = -v1[1];
					
					glNormal3d( v1[0], v1[1], v1[2] );
					glVertex3d( v1[0], v1[1], v1[2] );
					if( c < (level) )
					{
						ConvertPolarToRect( level-1, c, &x, &y );
						scale( v1, pole_patch->patches[s].grid[x][y], SPHERE_SIZE + patch->height[s+(north*6)][x][y] );
						if( north )
							v1[1] = -v1[1];
						glNormal3d( v1[0], v1[1], v1[2] );
						glVertex3d( v1[0], v1[1], v1[2] );
					}
					r++;
				}
				glEnd();
				if( bLog )lprintf( WIDE("---------") );

				{
					switch( s )
					{
					case 0:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fore_color );
						break;
					case 1:
					case 2:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );
						break;
					}
				}
				glBegin( GL_TRIANGLE_STRIP );
				for( c = level; c <= level*2; c++ )
				{
					ConvertPolarToRect( level, c, &x, &y );
					//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c,level);
					scale( v1, pole_patch->patches[s].grid[x][y], SPHERE_SIZE + patch->height[s+(north*6)][x][y] );
					if( north )
						v1[1] = -v1[1];
					glNormal3d( v1[0], v1[1], v1[2] );
					glVertex3d( v1[0], v1[1], v1[2] );
					if( c < (level)*2 )
					{

						ConvertPolarToRect( level-1, c-1, &x, &y );
						//if( bLog )lprintf( WIDE("Render corner %d,%d"), 2*level-c-1,level-1);
						scale( v1, pole_patch->patches[s].grid[x][y], SPHERE_SIZE + patch->height[s+(north*6)][x][y] );
						if( north )
							v1[1] = -v1[1];
						glNormal3d( v1[0], v1[1], v1[2] );
						glVertex3d( v1[0], v1[1], v1[2] );
					}
				}
				glEnd();
			}
		}
	}
	//__except(EXCEPTION_EXECUTE_HANDLER){ lprintf( WIDE("Pole Patch Excepted.") );return 0; }
	return 1;
}

void RenderBandPatch( PHEXPATCH patch, btScalar *m, int mode )
{
	struct band *band = patch->band;
	int section, section2;
	int sections = 6*band->hex_size;
	int sections2= band->hex_size;
	int row;
	int col;
	VECTOR patch1, patch2;
	//VECTOR ref_point;
	//InitBandPatch();
	float back_color[4];
	float tmpval[4];
	float fore_color[4];
	float bright_fore_color[4];
   float fade;

	number = patch->number;
	row = (number-1)/l.numbers.cols;
	col = (number-1)%l.numbers.cols;

	back_color[0] = RedVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[1] = GreenVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[2] = BlueVal( l.colors[(number-1)/15] )/(256*4.0f);
	back_color[3] = 1.0;

	fore_color[0] = 0.25f;
	fore_color[1] = 0.25f;
	fore_color[2] = 0.25f;

	if( patch->flags.fade )
	{
		if( patch->fade_target_tick < l.last_tick )
		{
			bright_fore_color[3]
            = fade
				= back_color[3]
				= tmpval[3]
				= fore_color[3]
				= (RCOORD)0;
		}
		else
		{
			bright_fore_color[3]
            = fade
				= back_color[3]
				= tmpval[3]
				= fore_color[3]
				= (RCOORD)( ( patch->fade_target_tick - l.last_tick ) ) / (RCOORD)l.fade_duration;
		}
	}
	else
	{
		bright_fore_color[3]
			= fade
			= back_color[3]
			= tmpval[3]
			= 1.0;
		fore_color[3] = 1.0;
	}


	bright_fore_color[0] = 0.5f;
	bright_fore_color[1] = 0.5f;
	bright_fore_color[2] = 0.5f;

	tmpval[0] = l.values[MAT_DIFFUSE0] / 256.0f;
	tmpval[1] = l.values[MAT_DIFFUSE1] / 256.0f;
	tmpval[2] = l.values[MAT_DIFFUSE2] / 256.0f;
	//tmpval[3] = 1.0f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmpval );
	tmpval[0] = l.values[MAT_AMBIENT0] / 256.0f;
	tmpval[1] = l.values[MAT_AMBIENT1] / 256.0f;
	tmpval[2] = l.values[MAT_AMBIENT2] / 256.0f;
	//tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmpval );
	tmpval[0] = l.values[MAT_SPECULAR0] / 256.0f;
	tmpval[1] = l.values[MAT_SPECULAR1] / 256.0f;
	tmpval[2] = l.values[MAT_SPECULAR2] / 256.0f;
	//tmpval[3] = 1.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpval );
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64/*l.values[MAT_SHININESS]/2*/ ); // 0-128

	if( !mode )
	{
		INDEX idx;
		struct SACK_3D_Surface * surface;
		LIST_FORALL( patch->band->bands, idx, struct SACK_3D_Surface *, surface )
		{
			int s = (idx/band->hex_size)%6; // might be 6, which needs to be 0.
				if( surface->color )
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fore_color );
				else
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );
			if( s == 1 )
			{
				// select the texture vertext buffer here somehow...
				RenderBumpTextureFragment( l.numbers.image, l.numbers.bump_image, m, patch->number, fore_color, surface );
			}
			else if( s == 4 )
			{
				// select the texture vertext buffer here somehow...
				RenderBumpTextureFragment( l.logo, NULL, m, 0, back_color, surface );
			}
			else
			{
				if( s < 3 )
					RenderBumpTextureFragment( NULL, NULL, m, 0, fore_color, surface );
				else
					RenderBumpTextureFragment( NULL, NULL, m, 0, back_color, surface );
			}
		}
	}

	if( mode )
	{
		//scale( ref_point, VectorConst_X, SPHERE_SIZE );
		//lprintf( " Begin --------------"  );
		for( section = 0; section < sections; section ++ )
		{
			int bound, front;
			int other_s = (section/(band->hex_size+1))%6; // might be 6, which needs to be 0.
			int s = (section/band->hex_size)%6; // might be 6, which needs to be 0.
			int s2 = ((section+1)/band->hex_size)%6; // might be 6, which needs to be 0.
			int section_offset = ( section % band->hex_size );
			int section_offset_1 = ( (section) % band->hex_size ) + 1;
			int section_offset_1_a = ( (section+1) % band->hex_size );
			int layer;
			if( section == band->hex_size )
			{
				//lprintf( "%d %d", s, s2 );
				//continue;
			}
			//lprintf( "section %d = %d,%d", section, s, s2 );
			for( layer = 0; layer < 2; layer++ )
			{
				if( layer == 0 )
				{
					tmpval[0] = 0.5;//l.values[MAT_SPECULAR0] / 256.0f;
					tmpval[1] = 0.5;//l.values[MAT_SPECULAR1] / 256.0f;
					tmpval[2] = 0.5;//l.values[MAT_SPECULAR2] / 256.0f;
					glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpval );
					glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64/*l.values[MAT_SHININESS]/2*/ ); // 0-128
					tmpval[0] = 0.5;
					tmpval[1] = 0.5;
					tmpval[2] = 0.45;
					//tmpval[3] = 1.0f;
					glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmpval );
					switch( s )
					{
					case 0:
					case 1:
					case 2:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, fore_color);
						break;
					case 3:
					case 4:
					case 5:
						glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );
						break;
					}
					glBindTexture( GL_TEXTURE_2D, 0 );
					bound = 0;
				}
				else
				{
					tmpval[0] = 0.35f;
					tmpval[1] = 0.35f;
					tmpval[2] = 0.35f;
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tmpval );
					glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32/*l.values[MAT_SHININESS]/2*/ ); // 0-128
					tmpval[0] = 0.5f;
					tmpval[1] = 0.5f;
					tmpval[2] = 0.5f;
					//tmpval[3] = 1.0f;
					glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpval );
					tmpval[0] = 0.5;
					tmpval[1] = 0.5;
					tmpval[2] = 0.5;
					//tmpval[3] = 1.0f;
					glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tmpval );
					if( s == 1 )
					{
						glColor4f( 1.0f, 1.0f, 1.0f, fade );
						glBindTexture( GL_TEXTURE_2D, l.numbers.texture );
						bound = 1;
						front = 1;
					}
					else if( s == 4 )
					{
						glColor4f( 1.0f, 1.0f, 1.0f, fade );
						glBindTexture( GL_TEXTURE_2D, l.logo_texture );
						bound = 1;
						front = 0;
					}
					else
					{
						// nothign to do for this layer.
						continue;
						//glBindTexture( GL_TEXTURE_2D, 0 );
					}

				}


				glBegin( GL_TRIANGLE_STRIP );
				for( section2 = 0; section2 <= sections2; section2++ )
				{
					scale( patch1, band->patches[s].grid[section_offset][section2], SPHERE_SIZE );
					// s2 can change patches, whereas the number coords... go forward
					scale( patch2, band->patches[s2].grid[section_offset_1_a][section2], SPHERE_SIZE );

					{
						if( bound )	
						{
							if( front )
							{
								//lprintf( "one  Using %g,%g", l.numbers.coords[row][col][section_offset][section2][0]
								//	, l.numbers.coords[row][col][section_offset][section2][1] );
								glTexCoord2d( l.numbers.coords[row][col][section_offset][section2][0]
											, l.numbers.coords[row][col][section_offset][section2][1] );
							}
							else
							{
								//lprintf( "offset1 is %d %d", section_offset, HEX_SIZE );
								glTexCoord2f( (float)section_offset / band->hex_size, 1.0f - (float)section2/band->hex_size );
							}
						}
						glNormal3fv( patch1 );
						glVertex3fv( patch1 );

						if( bound )
						{
							if( front )
							{
								//lprintf( "then Using %g,%g", l.numbers.coords[row][col][section_offset_1_a][section2][0]
							//		, l.numbers.coords[row][col][section_offset+1][section2][1] );
								glTexCoord2d( l.numbers.coords[row][col][section_offset+1][section2][0]
											, l.numbers.coords[row][col][section_offset+1][section2][1] );
							}
							else
							{
								//lprintf( "offset2 is %d %d", section_offset_1, HEX_SIZE );
								glTexCoord2f( (float)section_offset_1 / band->hex_size, 1.0f - (float)section2/band->hex_size );
							}
						}
						glNormal3fv( patch2 );
						glVertex3fv( patch2 );
					}
				}
				glEnd();

			}
		}

		glBindTexture( GL_TEXTURE_2D, 0 );
	}
}
int DrawSphereThing( PHEXPATCH patch, int mode )
{
	RCOORD scale = 0.0;


	if( patch->number == l.next_active_ball && ( l.next_active_tick > 0 ) )
	{
		scale = 1.0 - ( (RCOORD)l.next_active_tick - l.last_tick ) / TIME_TO_APPROACH;
	}
	else if( patch->number == l.active_ball )
		scale = 1.0;

	if( scale > 0.0 )
	{
		RCOORD target1 = 40;
		RCOORD target2 = 540;
		RCOORD real_factor = (target1 * scale + target2 * (1.0-scale));
		lprintf( "scale is %g   %g", scale, real_factor );

		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glPushMatrix();
		glLoadIdentity();									// Reset The Projection Matrix

		glOrtho(-real_factor*(l.aspect[0])
				 , real_factor*(l.aspect[0])
				 , -real_factor, real_factor, -1000.0f, 3000.0f);

		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	}
          
	//if( mode )
	glPushMatrix();

	btScalar m[16];
	btTransform trans;
	patch->fallRigidBody->getMotionState()->getWorldTransform( trans );

	trans.getOpenGLMatrix(m);
	btVector3 r = trans.getOrigin();

#ifdef BT_USE_DOUBLE_PRECISION 
		glMultMatrixd( m );
#else
		glMultMatrixf( m );
#endif

	//which is a 1x1x1 sort of patch array
	RenderBandPatch( patch, m, mode );
	RenderPolePatch( patch, m, mode, 0 );
	RenderPolePatch( patch, m, mode, 1 );

	glPopMatrix();

	if( scale > 0.0 )
	{
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	}
	return 1;
}

void DrawSphereThing2( PHEXPATCH patch )
{
	btTransform trans;
	patch->fallRigidBody->getMotionState()->getWorldTransform( trans );
	btVector3 r = trans.getOrigin();

	{
		PTRANSFORM t = GetImageTransformation( patch->label );
		_POINT p;
		_POINT p2;
		p[0] = 0;
		p[1] = PLANET_RADIUS * 1.4;
		p[2] = 0;
		ApplyRotation( l.transform, p2, p );
		Translate( t, r[0] + p2[0], r[1] + p2[1], r[2] + p2[2] );		
		Render3dImage( patch->label, TRUE );
	}
}

void ResizeHeightMap( PHEXPATCH patch, int size )
{
	if( size > patch->max_hex_size )
	{
		if( patch->max_hex_size )
		{
			int m;
			for( m = 0; m < 12; m ++ )
			{
				int n;
				for( n = 0; n < patch->max_hex_size + 1; n++ )
					Release( patch->height[m][n] );
				Release( patch->height[m] );
			}
		}
		patch->max_hex_size = patch->hex_size = size;
		{
			int m;
			for( m = 0; m < 12; m ++ )
			{
				int n;
				patch->height[m] = NewArray( RCOORD *, size + 1 );
				for( n = 0; n < size + 1; n++ )
				{
					patch->height[m][n] = NewArray( RCOORD, size + 1 );
					MemSet( patch->height[m][n], 0, sizeof( RCOORD ) * (size + 1) );
				}
			}
		}
	}
}
PHEXPATCH CreatePatch( int size, PHEXPATCH *nearpatches )
{
	int s;
	// create both polar patches for quick hack.
	PHEXPATCH patch = (PHEXPATCH)Allocate( sizeof( HEXPATCH ) );
	MemSet( patch, 0, sizeof( HEXPATCH ) );
	patch->time_to_rack = timeGetTime() + 4000;
	patch->label = MakeImageFile( 60, 20 );
	ClearImage( patch->label );
	SetImageTransformRelation( patch->label, IMAGE_TRANSFORM_RELATIVE_CENTER, NULL );

	{
		int m;
		for( m = 0; m < 12; m ++ )
		{
			int n;
			patch->height[m] = NewArray( RCOORD *, size + 1 );
			for( n = 0; n < size + 1; n++ )
			{
				patch->height[m][n] = NewArray( RCOORD, size + 1 );
				MemSet( patch->height[m][n], 0, sizeof( RCOORD ) * (size + 1) );
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
				if( x == (patch->hex_size-1) )
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
			if( x == (patch->hex_size-1) )
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
			if( y == (patch->hex_size-1) )
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
			if( x == (patch->hex_size-1) )
			{
				near_points[3][2] = -1;
				near_points[4][2] = -1;
			}
			if( y == (patch->hex_size-1) )
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
			if( x == (patch->hex_size-1) )
			{
				near_points[2][2] = -1;
				near_points[3][2] = -1;
			}
			if( y == (patch->hex_size-1) )
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


void ParseImage( Image image, int size, int rows, int cols )
{
	_32 w, h;
	int r, c;
	int divisions = size + 1;
	GetImageSize( image, &w, &h );
	l.numbers.rows = rows;
	l.numbers.cols = cols;

	lprintf( "parsing image for %dx%d and size %d", rows, cols, size );

	if( size > l.numbers.max_size )
	{
		if( l.numbers.max_size )
		{
			for( r = 0; r < rows; r++ )
			{
				for( c = 0; c < cols; c++ )
				{
					int x;
					for( x = 0; x <= l.numbers.max_size; x++ )
					{
						Release( l.numbers.coords[r][c][x] );
					}
					Release( l.numbers.coords[r][c] );
				}
				Release( l.numbers.coords[r] );
			}
		}
		l.numbers.max_size = size;

		l.numbers.coords = NewArray( VECTOR***, (rows));
		for( r = 0; r < rows; r++ )
		{
			l.numbers.coords[r] = NewArray( VECTOR**, (cols));
			for( c = 0; c < cols; c++ )
			{
				int x, y;
				l.numbers.coords[r][c] = NewArray( VECTOR *, size+1 );
				for( x = 0; x <= size; x++ )
				{
					l.numbers.coords[r][c][x] = NewArray( VECTOR, size + 1 );
				}
			}
		}
	}
	for( r = 0; r < rows; r++ )
	{
		for( c = 0; c < cols; c++ )
		{
			int x, y;
			for( x = 0; x <= size; x++ )
			{
				for( y = 0; y <= size; y++ )
				{
					l.numbers.coords[r][c][x][y][0] = (c / (double)cols) + ( ( size - x ) / (double)( rows*size ) );
					l.numbers.coords[r][c][x][y][1] = (r / (double)rows) + ( ( y ) / (double)( cols*size ) );
					l.numbers.coords[r][c][x][y][2] = 0;
				}
			}
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

void RemoveBall( int ball )
{
	PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, ball - 1 );
	if( patch->flags.simulated )
	{
		patch->flags.simulated = 0;
		l.bullet.dynamicsWorld->removeRigidBody( patch->fallRigidBody );
	}
}


void BeginApproach( void )
{
	// don't re-begin approach if already approaching.
	{
		// stop simulating now.
		RemoveBall( l.next_active_ball );
		{
			PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, l.next_active_ball - 1 );
			patch->flags.grabbed = 1;
		}
	}
	if( !l.next_active_tick )
		l.next_active_tick = l.last_tick + TIME_TO_APPROACH;
}

void BeginPivot( void )
{
	// a ball is being watched...
   // the pivot is pre-empting all prior activity up to turning
	if( !l.active_ball && l.next_active_ball )
	{
		l.active_ball = l.next_active_ball;
		l.next_active_ball = 0;
		l.next_active_tick = 0;
		l.watch_ball_tick = 0;
	}
	// don't begin pivot if pivot already set
	if( !l.active_ball_forward_tick )
		l.active_ball_forward_tick = l.last_tick + TIME_TO_TURN_BALL;

}


void BeginWatch( int ball )
{
	if( ball <= 0 )
		return;
	l.next_active_ball = ball;

	if( 1 ) 
	{
	}
	l.watch_ball_tick = l.last_tick + TIME_TO_WATCH;
}

void EndWatchBall( void )
{
	if( l.active_ball )
	{
		if( 1 )
		{
			PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, l.active_ball - 1 );
			patch->flags.grabbed = 0;
			patch->time_to_rack = l.last_tick + l.time_to_rack;
		}
		else
		{
			//lprintf( "End watcom ball...set return to home" );
			l.return_to_home = l.last_tick + TIME_TO_HOME;
		}

	}
}


void FadeBall( int ball )
{
	PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, ball - 1 );
	if( !patch )
	{
		return;
	}
	if( patch->flags.simulated )
	{
		patch->fade_target_tick = l.last_tick + l.fade_duration;
		patch->flags.fade = 1;
	}
	else
	{
		patch->flags.removed_before_simulated = 1;
	}
}
void ShowBalls( void )
{
	INDEX idx;
	PHEXPATCH patch;
	LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
	{
		if( patch->flags.simulated )
		{
			btVector3 ball_vector = patch->fallRigidBody->getWorldTransform().getOrigin();
			lprintf( "result at %g,%g,%g", ball_vector[0], ball_vector[1], ball_vector[2] );
		}
	}
}


void RackBalls( void )
{
	INDEX idx;
	PHEXPATCH patch;
	LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
	{
		if( !patch->flags.simulated )
		{
			if( patch->flags.removed_before_simulated )
			{
				patch->flags.removed_before_simulated = 0;
			}
			patch->flags.fade = 0;
			patch->fade_target_tick = 0;
			if( 0 )
				lprintf( "putting patch %d at %g,%g,%g", patch->number, patch->origin[0], patch->origin[1], patch->origin[2] );
			patch->fallRigidBody->translate( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] ) );
			{
				btTransform t;
				lprintf( "SET ROTATION" );
				t.setRotation(btQuaternion(0,0,0,1));
				t.setOrigin(btVector3(patch->origin[0], patch->origin[1], patch->origin[2]));
      
				patch->fallRigidBody->setWorldTransform(t);

				patch->fallRigidBody->setLinearVelocity(btVector3(0.0f,0.0f,0.0f));
				patch->fallRigidBody->setAngularVelocity(btVector3(0.0f,0.0f,0.0f));
   
				patch->fallRigidBody->setInterpolationWorldTransform(patch->fallRigidBody->getWorldTransform());
				patch->fallRigidBody->setInterpolationLinearVelocity(patch->fallRigidBody->getLinearVelocity());
				patch->fallRigidBody->setInterpolationAngularVelocity(patch->fallRigidBody->getAngularVelocity());

				patch->fallRigidBody->clearForces();
				patch->fallRigidBody->activate();   			
			}

			patch->flags.simulated = TRUE;
			l.bullet.dynamicsWorld->addRigidBody(patch->fallRigidBody);
			if( 0 )
			{
				btVector3 ball_vector = patch->fallRigidBody->getWorldTransform().getOrigin();
				lprintf( "result at %g,%g,%g", ball_vector[0], ball_vector[1], ball_vector[2] );
			}
		}
	}
}


void PointCameraAtNextActiveBall( void )
{
	PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, l.next_active_ball - 1 );
	if( !patch )
		return;

	lprintf( "Point at next active" );
	btVector3 ball_vector = patch->fallRigidBody->getWorldTransform().getOrigin();
	SetPoint( l.ball_grab_position, ball_vector );

	btQuaternion ball_orient;
	btQuaternion ball_orient2;
	{
		RCOORD quat[4];
		GetRotationMatrix( l.transform, quat );
		// this strange order turns the ball around 180x180 from the camera.
		// it has the builtin multiplcation by constants
		ball_orient[0] = -quat[0];
		ball_orient[1] = quat[3];
		ball_orient[2] = -quat[2];
		ball_orient[3] = -quat[1];
		ball_orient2 = ball_orient * btQuaternion( 0, 0, 1, 0 );
		ball_orient2 = ball_orient2 * btQuaternion( 0, 1, 0, 0 );
	}

	btTransform trans;
	patch->fallRigidBody->getMotionState()->getWorldTransform(trans);
	lprintf( "SET ROTATION" );
	trans.setRotation( ball_orient );
	patch->fallRigidBody->getMotionState()->setWorldTransform(trans);

}


void MoveBallsToRack( void )
{
	INDEX idx;
	PHEXPATCH patch;
	LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
	{

		if( !patch->flags.simulated && !patch->flags.grabbed )
		{
			btVector3 work;
			RCOORD distance1 = 120;// = 72;//= 125;
			RCOORD distance2 = 480;//= 550;
			btVector3 target( (patch->number - 1) % 15, ( patch->number - 1 ) / 15, distance2 /** (l.identity_depth[0]/400.0)*/ );
			
			PCVECTOR axis;

			axis = GetAxis( l.transform, vRight );
			work[0] = ( distance1 * (target[0]-7)) * axis[0];
			work[1] = ( distance1 * (target[0]-7)) * axis[1];
			work[2] = ( distance1 * (target[0]-7)) * axis[2];
			axis = GetAxis( l.transform, vUp );
			work[0] += ( distance1 * (4-target[1])) * axis[0];
			work[1] += ( distance1 * (4-target[1])) * axis[1];
			work[2] += ( distance1 * (4-target[1])) * axis[2];
			axis = GetAxis( l.transform, vForward );
			work[0] += target[2] * axis[0];
			work[1] += target[2] * axis[1];
			work[2] += target[2] * axis[2];


			//PTRANSFORM pt_dome = CreateTransform();
			RCOORD outer_distance; // to center....
			RCOORD center_distance; 
			//RotateTo( pt_dome


				PCVECTOR origin = GetOrigin( l.transform );
				_POINT tmp;
				addscaled( tmp, origin, GetAxis( l.transform, vForward ), 75 );

				btVector3 camera( tmp[0], tmp[1], tmp[2] );
				
			if( patch->time_to_rack < l.last_tick )
			{
				patch->flags.racked = 1;
				patch->time_to_rack = 0;
				work = camera + work;
				patch->rack_delta = 1.0;
			}
			else
			{
				// goes from 1.0 to 0.0
				patch->flags.racked = 1;
				RCOORD delta = ((RCOORD)(patch->time_to_rack-l.last_tick))/l.time_to_rack;
				
				work = camera + work*( patch->rack_delta = ( 1.0-delta ) );
			}


			btQuaternion rotation;
			{
				RCOORD quat[4];
				GetRotationMatrix( l.transform, quat );
				// this strange order turns the ball around 180x180 from the camera.
				// it has the builtin multiplcation by constants
				rotation[0] = -quat[0];
				rotation[1] = quat[3];
				rotation[2] = -quat[2];
				rotation[3] = -quat[1];
			}

			btTransform trans;
			patch->fallRigidBody->getMotionState()->getWorldTransform(trans);
			trans.setOrigin( work );
			lprintf( "SET ROTATION" );
			trans.setRotation( rotation );
			//trans.setRotation(  btQuaternion( 0, 0, 1, 0 ) );
			patch->fallRigidBody->getMotionState()->setWorldTransform(trans);
		}
	}

}

void MoveBallToCameraApproach( void )
{
	LOGICAL rotating = FALSE;
	LOGICAL rotated = FALSE;
	RCOORD delta;
	RCOORD scale;
	PHEXPATCH patch = (PHEXPATCH)GetLink( &l.patches, l.next_active_ball - 1 );
	lprintf( "Ball to camera..." );
	if( l.next_active_ball )
	{
		// the ball is not locked-to-view yet, but is being watched, or is moving into position...
		if( l.next_active_tick < l.last_tick )
			scale = 1.0;
		else if( l.next_active_ball )
			scale = 1.0 - ( ((RCOORD)( l.next_active_tick - l.last_tick )) / TIME_TO_APPROACH);
		else
			scale = 0;
		patch = (PHEXPATCH)GetLink( &l.patches, l.next_active_ball - 1 );
	}
	else if( l.active_ball )
	{
		scale = 1.0;
		// we now have a ball directly
		lprintf( "active (%d)", l.active_ball_forward_tick );
		if( l.active_ball_forward_tick )
		{
			if(  l.active_ball_forward_tick < l.last_tick )
			{
				lprintf( "rotated." );
				rotating =FALSE;
				rotated = TRUE;
				delta = 1.0;
			}
			else
			{
				rotating = TRUE;
				delta = 1.0 - ( ((RCOORD)(l.active_ball_forward_tick-l.last_tick)) / TIME_TO_TURN_BALL );
			}
		}
		else
		{
			delta = 0.0;
		}
		patch = (PHEXPATCH)GetLink( &l.patches, l.active_ball - 1 );
	}
	else
	{
		return;
	}
	if( !patch )
		return;

	btVector3 ball_position;
	btTransform trans;
	patch->fallRigidBody->getMotionState()->getWorldTransform(trans);

	btVector3 ball_vector = l.ball_grab_position;
	PCVECTOR camera_origin = GetOrigin( l.transform );
	PCVECTOR forward;
	_POINT tmp;
	addscaled( tmp, camera_origin, forward = GetAxis( l.transform, vForward ), 70 );
	btVector3 bt_camera_origin_real;
	btVector3 bt_camera_origin;

	RCOORD quat[4];
	bt_camera_origin[0] = tmp[0];
	bt_camera_origin[1] = tmp[1];
	bt_camera_origin[2] = tmp[2];
	bt_camera_origin_real[0] = camera_origin[0];
	bt_camera_origin_real[1] = camera_origin[1];
	bt_camera_origin_real[2] = camera_origin[2];

	btVector3 new_ball_pos = bt_camera_origin + btVector3( forward[0], forward[1], forward[2] );
	//btVector3 new_ball_pos = ((l.ball_grab_position -bt_camera_origin) *( 1.0 - scale )) + bt_camera_origin;
	if( 0 )
	{
		lprintf( "new ball pos = %g,%g,%g  (from %g,%g,%g to %g,%g,%g)  %g %g"
			, new_ball_pos[0], new_ball_pos[1], new_ball_pos[2]
						, l.ball_grab_position[0]
						, l.ball_grab_position[1]
						, l.ball_grab_position[2]
						, bt_camera_origin[0]
						, bt_camera_origin[1]
						, bt_camera_origin[2]
						, scale, ( 1.0 - scale ));
	}
	trans.setOrigin( new_ball_pos );

	// normalizes when set.
	GetRotationMatrix( l.transform, quat );

	btQuaternion ball_orient;
	ball_orient[0] = quat[1];
	ball_orient[1] = quat[2];
	ball_orient[2] = quat[3];
	ball_orient[3] = -quat[0];// -quat[0];

	if( rotated )
	{
		ball_orient = ball_orient * btQuaternion( 0, 1.0, 0, 0 );
		ball_orient = ball_orient * btQuaternion( 0, 0, 1.0, 0 );
	}
	else if( rotating  )
	{
		if( delta > 0 )
		{
			btQuaternion ball_orient2;
			ball_orient2[0] = -quat[0];
			ball_orient2[1] = quat[3];
			ball_orient2[2] = -quat[2];
			ball_orient2[3] = -quat[1];

			ball_orient = ball_orient * ( 1.0 - delta ) + ball_orient2 * delta;
		}
	}
	//lprintf( "Ball orient %g,%g,%g,%g", ball_orient[0], ball_orient[1], ball_orient[2], ball_orient[3] );
	trans.setRotation( ball_orient );

	patch->fallRigidBody->getMotionState()->setWorldTransform(trans);
}

void MoveCameraToHome( void )
{
#define Avg( c1, c2, d, max ) ((((c1)*(max-(d))) + ((c2)*(d)))/max)
	RCOORD scaled_quat[4];
	_POINT scaled_origin;
	RCOORD length;
	_32 delta = l.return_to_home - l.last_tick;
	int n;
	for( n = 0; n < 4; n++ )
	{
		// delta counts down from N to 0, so we start at all final, and move to inital
		scaled_quat[n] = Avg( l.initial_view_quat[n], l.final_view_quat[n]
		                    , delta, TIME_TO_HOME );
		/*
		lprintf( "quat %d %g %g %g %d %d"
			, n, scaled_quat[n], l.final_view_quat[n], l.initial_view_quat[n]
		    , l.return_to_home-l.last_tick
			, TIME_TO_HOME );
		*/
	}
	length = scaled_quat[0] * scaled_quat[0] 
			+ scaled_quat[1] * scaled_quat[1] 
			+ scaled_quat[2] * scaled_quat[2] 
			+ scaled_quat[3] * scaled_quat[3] ;
	length = sqrt( length );
	scaled_quat[0] = scaled_quat[0] / length;
	scaled_quat[1] = scaled_quat[1] / length;
	scaled_quat[2] = scaled_quat[2] / length;
	scaled_quat[3] = scaled_quat[3] / length;
	for( n = 0; n < 3; n++ )
	{
		scaled_origin[n] = Avg( l.initial_view_origin[n], l.final_view_origin[n]
		                    , delta, TIME_TO_HOME );
	}
	//scaled_quat[0] = -scaled_quat[0];
	lprintf( "SET ROTATION" );
	SetRotationMatrix( l.transform, scaled_quat );
	{
		RCOORD test[4];
		//GetRotationMatrix( l.transform, test );
		//lprintf( "%g %g %g %g", test[0], test[1], test[2], test[3] );
	}
				
	TranslateV( l.transform, scaled_origin  );
}

static void OnFirstDraw3d( WIDE( "Terrain View" ) )( PTRSZVAL psvInit )
{
	PTRANSFORM camera = (PTRANSFORM)psvInit;
	// and really if initshader fails, it sets up in local flags and 
	// states to make sure we just fall back to the old way.
	// so should load the classic image along with any new images.

	glGetFloatv( GL_PROJECTION_MATRIX, l.projection );
	InitShader();


	{
		int n;
		struct band *initial_band = new band( l.hex_size );
		struct pole *initial_pole = new pole( l.hex_size );
		initial_band->Texture( 0 );

		for( n = 0; n < 75; n++ )
		{
			PHEXPATCH patch = CreatePatch( l.hex_size, NULL );

			patch->band = initial_band;
			patch->pole = initial_pole;
		
			initial_band->Texture( n + 1 );


			patch->origin[0] =  (n % 5) * 100 - 250 ;
			patch->origin[1] = (n / 5) * 100 ;
			patch->origin[2] = n;
			patch->number = n + 1;

			{
				TEXTCHAR text[12];
				PTRANSFORM t;
				_32 w, h;
				snprintf( text, 12, "%d", patch->number );
				GetStringSizeFont( text, &w, &h, NULL );
				PutStringFont( patch->label, (60-w)/2, (20-h)/2, BASE_COLOR_WHITE, 0, text, NULL );
				t = GetImageTransformation( patch->label );
				//Scale( t, 10, 10, 1 );
				MoveUp( t, -( PLANET_RADIUS * 2.0f ) );
				MoveRight( t, -30 );
				MoveUp( t, -20 );
			}

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
			patch->fallRigidBody->setFriction( 0.8f ) ;

			AddLink( &l.patches, patch );
		}
	}

}

static PTRSZVAL OnInit3d( WIDE( "Terrain View" ) )( PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.identity_depth = identity_depth;
	l.aspect = aspect;

	l.numbers.image = LoadImageFile( l.numbers.image_name );
	l.numbers.bump_image = LoadImageFile( l.numbers.bump_image_name );
	ParseImage( l.numbers.image, l.hex_size, 10, 10 );

	l.logo = LoadImageFile( l.logo_name );

	// override display default camera
	{
		PTRANSFORM transform = CreateNamedTransform( WIDE("render.camera") );
		RCOORD quat[4];

		//<-31.9047,309.653,711.449> <0.0023503,0.0506664,0.996322,-0.0690609>
		quat[0] = 0.0023503;
		quat[1] = 0.0506664;
		quat[2] = 0.996322;
		quat[3] = -0.0690609;

		SetRotationMatrix( transform, quat );
		Translate( transform, -31.9047,309.653,711.449);
	}

	return (PTRSZVAL)camera;
}

static void CPROC UpdatePositions( PTRSZVAL psv );
static void OnUpdate3d( WIDE( "Terrain View" ) )( PTRANSFORM origin )
{
	l.transform = origin;
	{
		PCVECTOR o= GetOrigin( l.transform );
		SetPoint( l.initial_view_origin, o );
	}
	GetRotationMatrix( l.transform, l.initial_view_quat );

#ifdef DEBUG_TIMING
	lprintf( "Tick." );
#endif
	UpdatePositions( 0 );  // updates last_tick to now.
#ifdef DEBUG_TIMING
	lprintf( "(end tick physics)Tick." );
#endif
}

LOGICAL hold_update;
static void OnBeginDraw3d( WIDE( "Terrain View" ) )( PTRSZVAL psv,PTRANSFORM camera )
{
	int mode = 1;
#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81F8
#define GL_SEPARATE_SPECULAR_COLOR 0x81FA
#endif
	glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
	glShadeModel( GL_SMOOTH );
	l.transform = camera;

	l.logo_texture = ReloadTexture( l.logo, 0 );
	if( mode == 0 )
	{
		l.numbers.texture = ReloadTexture( l.numbers.image, 0 );
	}
	else
	{

		l.numbers.texture = ReloadMultiShadedTexture( l.numbers.image, 0, AColor( 5, 5, 5, 32), Color( 12, 12, 83 ), Color( 0, 170, 170 ) );
	}

	// this transform is initialized to the viewpoint in vidlib.
	glGetFloatv( GL_PROJECTION_MATRIX, l.projection );

	if( l.flags.rack_balls )
	{
		RackBalls();
		l.flags.rack_balls = 0;
	}

	hold_update = TRUE;

	MoveBallsToRack();

	if( l.return_to_home )
	{
		lprintf( "return to home..." );
		if( 1 )
		{
			l.return_to_home = 0;
		}
		else
		{
			// want to return to home; 
			// clear out the last active ball

			if( l.return_to_home > l.last_tick )
			{
				// still under the final tick, move camera towards home
				MoveCameraToHome();
			}
			else
			{
				//lprintf( "at home position..." );
				if( l.active_ball )
				{
					FadeBall( l.active_ball );
					l.active_ball = 0;
				}
				l.return_to_home = 0;
			}
		}
	}
	// update the camera to one of the balls....
	else if( l.next_active_ball && !l.active_ball )
	{
		lprintf( "next ball is %d now(%d)  watch(%d)  next(%d)"
			, l.last_tick, l.next_active_ball, l.watch_ball_tick, l.next_active_tick );
		if( l.next_active_ball && l.watch_ball_tick || l.next_active_tick == 0 )
		{
			lprintf( "Pointing camera..." );
			PointCameraAtNextActiveBall();
			// when we are done watching; clear watch flag.
			if( l.watch_ball_tick <= l.last_tick )
			{
				l.watch_ball_tick = 0;
				// if watch flag is clear, setup approach time.
				BeginApproach();
			}
		}
		else
		{
			if( l.next_active_tick < l.last_tick )
			{
				lprintf( "Trigger pivot" );
				l.active_ball = l.next_active_ball;
				l.next_active_ball = 0;
				l.next_active_tick = 0;
				l.active_ball_forward_tick = 0;
				if( l.flags.nextball_mode )
				{
					BeginPivot();
				}
			}

			MoveBallToCameraApproach();
			//MoveCameraOnBallApproach();
		}
	}
	else if( l.active_ball )
	{
		PHEXPATCH ball = (PHEXPATCH)GetLink( &l.patches, l.active_ball-1 ); // pick one of the balls to follow....
		lprintf( "active ball..." );
		if( !ball->flags.simulated && !ball->flags.grabbed )
		{
			lprintf( "No ball." );
			// no, ball is no longer simulated.
			l.active_ball = 0;
			l.return_to_home = l.last_tick + TIME_TO_HOME;
		}

		MoveBallToCameraApproach();
		if( 0 ) // this is old mode active ball turn around thing...
		{			
			btTransform trans;
			ball->fallRigidBody->getMotionState()->getWorldTransform( trans );
			btVector3 ball_origin = trans.getOrigin();
			lprintf( "Rotating?" );
			{
				btQuaternion rotation = trans.getRotation();
				RCOORD r2[4];
				RCOORD r3[4];
				r2[0] = -rotation[3];
				r2[1] = rotation[0];
				r2[2] = rotation[1];
				r2[3] = rotation[2];
				SetRotationMatrix( camera, r2 );
				
				Translate( camera, ball_origin.x(), ball_origin.y(), ball_origin.z()  );
				if( l.active_ball_forward_tick == 0 )
				{
					lprintf( "Fixed pos" );
					MoveForward( camera, -70 );
				}
				else if(  l.active_ball_forward_tick > l.last_tick )
				// rotate around until we lock...
				{
					lprintf( "rotating" );
					RotateRel( camera, 0, (M_PI) * ( TIME_TO_TURN_BALL - (l.active_ball_forward_tick - l.last_tick) ) / TIME_TO_TURN_BALL, 0 ) ;
					MoveForward( camera, -70 );
					RotateRel( camera, 0, 0, M_PI  * ( TIME_TO_TURN_BALL - (l.active_ball_forward_tick - l.last_tick) ) / TIME_TO_TURN_BALL );
				}
				else
				{
					l.active_ball_forward_tick = 0;

					RotateRel( camera, 0, (M_PI), 0 ) ;
					MoveForward( camera, -70 );
					RotateRel( camera, 0, 0, M_PI );
					if( !l.return_to_home )
					{
						SetPoint( l.final_view_origin, GetOrigin( camera ) );
						GetRotationMatrix( camera, l.final_view_quat );
					}	
				}
			}
		}
	}
	else
	{
		lprintf( "begin watch   %d", l.nNextBalls[0] );
		BeginWatch( l.nNextBalls[0] );
		//lprintf( "at home position" );
	}
}

static void OnDraw3d( WIDE("Terrain View") )( PTRSZVAL psvInit )
{
	MATRIX m;
	static _32 prior_tick;
	static int frames;
	static int skip = 30;
	_32 now = timeGetTime();
	static _32 fps;
	frames++;
#ifdef DEBUG_TIMING
	lprintf( "Tick." );
#endif
	do
	{
#ifdef DEBUG_TIMING7
		lprintf( "fps:%d (%d/%d %d)", fps, frames, now-prior_tick, (now-prior_tick)/frames  );
#endif
		if( ( frames % 100 ) == 0 )
		{
			prior_tick = now;
			frames = 0;
		}
		else if( prior_tick )
			fps = ( frames * 1000 ) / (now-prior_tick);
		else
			prior_tick = now;

		break;

		if( prior_tick )
		{
			//lprintf( "delay time: %ld / %d (%d)", now - prior_tick, frames, (now - prior_tick)/ frames );
			
			if( frames == 10 )
			{
				if( skip )
					skip--;		
				// if the prior rate was faster than 30fps
				// then increase complexity.
				else if( ( !l.active_ball ) && (  (( now - prior_tick)/frames) < 33 ) )
				{
				
					l.hex_size = l.hex_size + 1;
					ParseImage( l.numbers.image, l.hex_size, 10, 10 );
						{
							INDEX idx;
							PHEXPATCH patch;
							LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
							{
								if( !idx )
								{
									patch->band->resize( l.hex_size );
									patch->pole->resize( l.hex_size );
								}
								ResizeHeightMap( patch, l.hex_size );
							}
						}
					
				}
				//if( 0 )
				{
					// if the prior was longer time than 12 fps, then decrease complexity
				if( ((now - prior_tick)/frames) > 80 ) // try to be over 12 fps
				{
					if( l.hex_size > 1 )
					{
						l.hex_size = l.hex_size / 2;
						ParseImage( l.numbers.image, l.hex_size, 10, 10 );
						{
							INDEX idx;
							PHEXPATCH patch;
							LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
							{
								if( !idx )
								{
									patch->band->resize( l.hex_size );
									patch->pole->resize( l.hex_size );
								}
								ResizeHeightMap( patch, l.hex_size );
							}
						}
					}
			
				}
			}
				frames = 0;
				prior_tick = now;
			}
			else
			{

				// accumulating average...

			}
		}
		else
		{
			frames = 0;
			prior_tick = now;
		}
	} while( 0);
	// display already activated by the time we get here.
#ifdef _MSC_VER
//		__try
	{
#endif
	if( 1 )//SetActiveGLDisplay( pRend ) )
	{

		INDEX idx;
		MATRIX tmp;
		PTRANSFORM camera = l.transform;
		GetGLCameraMatrix( camera, tmp );
		for( idx = 0; idx < 16; idx++ )
			l.modelview[idx] = tmp[0][idx];

		{
			//RCOORD fProjection1[16];
			//RCOORD fProjection2[16];
			glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
			glPushMatrix();
			glLoadIdentity();									// Reset The Projection Matrix
			// this should cause the planes to pivot around the depth sorta...
			// near things will expand, and far things will shrink.
			glOrtho(-540*(l.aspect[0]), 540*(l.aspect[0]), -540, 540, -1000.0f, 3000.0f);
			//glGetDoublev( GL_PROJECTION_MATRIX, fProjection2 );
			//glLoadMatrixd( fProjection2 );

			glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
		}
		{
			PC_POINT o;
			float eye[3];
			o = GetOrigin( l.transform );
			eye[0] = o[0];
			eye[1] = o[1];
			eye[2] = o[2];
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);

			glUseProgram( l.shader.simple_shader.shader );
			glUniformMatrix4fv( l.shader.simple_shader.worldview, 1, GL_FALSE, l.modelview );
			glUniform3fv( l.shader.simple_shader.eye_point, 1, eye );

			glUseProgram( l.shader.normal_shader.shader );
			glUniformMatrix4fv( l.shader.normal_shader.worldview, 1, GL_FALSE, l.modelview );
			glUniform3fv( l.shader.normal_shader.eye_point, 1, eye );

			glUseProgram( 0 );
			glDisable(GL_FRAGMENT_PROGRAM_ARB);
			glDisable(GL_VERTEX_PROGRAM_ARB);
		}

		{
			glEnable(GL_LIGHTING);

			{
				GLfloat global_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
				glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);	
			}

			{
				GLfloat lightpos[] = {-1000, 1000, 10., 0.};
				GLfloat lightdir[] = {-.5, -1., -0.1, 0.};
				//GLfloat lightamb[] = {l.values[LIT_AMBIENT0]/256.0f, l.values[LIT_AMBIENT1]/256.0f, l.values[LIT_AMBIENT2]/256.0f, 1.0};
				GLfloat lightamb[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightdif[] = {l.values[LIT_DIFFUSE0]/256.0f, l.values[LIT_DIFFUSE1]/256.0f, l.values[LIT_DIFFUSE2]/256.0f, 1.0};
				GLfloat lightdif[] = {1.0, 1.0, 1.0, 1.0};
				//GLfloat lightspec[] = {l.values[LIT_SPECULAR0]/256.0f, l.values[LIT_SPECULAR1]/256.0f, l.values[LIT_SPECULAR2]/256.0f, 1.0};
				GLfloat lightspec[] = {1.0, 1.0, 1.0, 1.0};
				glEnable(GL_LIGHT0);
				glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

				glLightfv(GL_LIGHT0, GL_AMBIENT, lightamb);
				glLightfv(GL_LIGHT0, GL_DIFFUSE, lightdif);
				glLightfv(GL_LIGHT0, GL_SPECULAR, lightspec);
				glLightfv( GL_LIGHT0, GL_SPOT_DIRECTION, lightdir );
			}

			{
				GLfloat lightpos[] = {1000, -1000, -10., 0.};
				GLfloat lightdir[] = {-1, 1., 0.1, 0.};
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
		}

		{
			PHEXPATCH patch;
			btVector3 bt_view_origin;
			PC_POINT view_origin;
			btVector3 bt_view_forward;
			PC_POINT view_forward;
			PHEXPATCH the_ball;
			btTransform trans;// = patch->fallRigidBody->getWorldTransform();

			// if we're watching a single ball, setup for clipping balls behind us.
			if( l.active_ball )
			{
				the_ball = (PHEXPATCH)GetLink( &l.patches, l.active_ball-1 );
				view_origin = GetOrigin( l.transform );
				view_forward = GetAxis( l.transform, vForward );
				the_ball->fallRigidBody->getMotionState()->getWorldTransform( trans );
				bt_view_origin = trans.getOrigin( );
				//bt_view_origin[0] = view_origin[0];
				//bt_view_origin[1] = view_origin[1];
				//t_view_origin[2] = view_origin[2];
				bt_view_forward[0] = view_forward[0];
				bt_view_forward[1] = view_forward[1];
				bt_view_forward[2] = view_forward[2];
			}


			SetMaterial();
			// do this in two passes, and skip the faded (fading) balls.
			LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
			{
				if(  patch->fade_target_tick && ( patch->fade_target_tick < l.last_tick ) )
				{
					RemoveBall( patch->number );
					patch->fade_target_tick = 0;
					patch->flags.fade = 0;
					continue;
				}

				if( !patch->flags.simulated && !patch->flags.grabbed && !patch->flags.racked )
					continue;

				
				// this does additional clipping when we are close to the ball...
				if( l.active_ball && !l.return_to_home )
				{
					patch->fallRigidBody->getMotionState()->getWorldTransform( trans );
					btVector3 o = trans.getOrigin();

					o = (o-bt_view_origin) / o.length();
					if( o.dot( bt_view_forward ) < 0 )
					{
						// don't draw balls that are behind the camera...
						continue;
					}
				}
				
				// fading/faded balls do not draw here, they draw in the next pass.
				if( patch->flags.fade )
					continue;

				DrawSphereThing( patch, 1 );
			}
			glEnable( GL_BLEND );
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			// a faded ball will be in the center of attention...
			LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
			{
				if( !patch->flags.fade )
					continue;
				if( !patch->flags.simulated && !patch->flags.grabbed && !patch->flags.racked )
					continue;
				DrawSphereThing( patch, 1 );
			}

			if( 0 )
			{
				// this bit of code renders the text labels over each ball; debug/testing only.
				glEnable( GL_BLEND );
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable( GL_DEPTH_TEST );
				{
					float back_color[4];
					back_color[0] = 1.0;
					back_color[1] = 1.0;
					back_color[2] = 1.0;
					back_color[3] = 1.0;
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, back_color );
				}
				glColor4f( 1.0, 1.0, 1.0, 1.0 );
				LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
				{
					DrawSphereThing2( patch );
				}
			}
		}

		{
			//glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
		}

#ifdef ALTANIK_EXISTS
		DrawOverlay( l.transform );
#endif
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix

		// debug draw the mixer bar approximation
		if( l.barRigidBody && 0 )
		{
			btTransform trans;
			l.barRigidBody->getMotionState()->getWorldTransform( trans );

			glPushMatrix();

			btScalar m[16];
			trans.getOpenGLMatrix(m);
#ifdef BT_USE_DOUBLE_PRECISION 
			glMultMatrixd( m );
#else
			glMultMatrixf( m );
#endif
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
	//l.bullet.dynamicsWorld->debugDrawWorld();
#ifdef _MSC_VER
	}
	//					__except( EvalExcept( GetExceptionCode() ) )
					{
		//				lprintf( WIDE(" ...") );
						;
					}
#endif
	hold_update = FALSE;
#ifdef DEBUG_TIMING
	lprintf( "(end draw Tick." );
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

void ApplyBlowerForces( void )
{
	INDEX idx;
	PHEXPATCH patch;
	btVector3 air_source( 0, -8730, 0 );
	btScalar magnitude = (271192000 );  // this is the air pressure....
	btVector3 air_target( 0, 800, 0 );
	btScalar attractor_magnitude = 3000;
	btScalar target_magnitude = -300.0; // unused this is for the suction
	btVector3 up = btVector3(0,1,0);
   btScalar air_slow = 1.055;
	btScalar air_condense = 10;
	magnitude /= TIME_SCALE;
	target_magnitude /= TIME_SCALE;

	LIST_FORALL( l.patches, idx, PHEXPATCH, patch )
	{
		if( !patch->flags.simulated )
			continue;
		btVector3 origin = patch->fallRigidBody->getWorldTransform().getOrigin();
		// lower blower
		if( 1 )
		{
			btScalar dist = origin.length() / TIME_SCALE;
			btVector3 dir = (origin - air_source).normalize();
			btScalar x = dir.dot( up );
			if( x > 0.60 )
			{
            btScalar decay = dist*dist*dist;
				btScalar force = magnitude/(dist*dist*dist);
				if( force > 180 )
					force = 180;
				patch->fallRigidBody->applyCentralForce( force * dir );
				btVector3 velocity = patch->fallRigidBody->getLinearVelocity();

				velocity[0] = velocity[0]/air_slow;
				velocity[2] = velocity[2]/air_slow;
				patch->fallRigidBody->setLinearVelocity(velocity);


				velocity[0] = -origin[0];
				velocity[1] = 0.0;
				velocity[2] = -origin[2];
				patch->fallRigidBody->applyCentralForce( velocity.normalize() * air_condense / decay );
			}
		}
			
		// this is basically a slope on the bottom plane to center...
		if( 1 ) 
		{

			btScalar dist = origin.length();
			btVector3 dir = origin - btVector3( 0, 55, 0 );
			btScalar x = dir.dot( up ) / dir.length();
			if( x < 0.1 )
			{
				btScalar force = attractor_magnitude/(dist);
				if( force > 180 )
					force = 180;
				
				patch->fallRigidBody->applyCentralForce( -force * dir.normalize() );    // really dir/dist = unit vector * force
			}
		}

		// attractor at top (failed)
		if( 0 )
		{
			btScalar dist = origin.distance(air_target);

			btVector3 dir = origin - air_target;
			btScalar x = dir.dot( up ) / dir.length();
			if( x < -0.6 )
				patch->fallRigidBody->applyCentralForce( target_magnitude/(dist*dist*dist) * dir);   
		}

	}
}

CRITICALSECTION csUpdate;

 void CPROC UpdatePositions( PTRSZVAL psv )
{
	_32 now = timeGetTime();

	ApplyBlowerForces();

	// the bar kept stopping; make sure it's active.
	if( l.barRigidBody )
		l.barRigidBody->activate();

	if( l.last_tick )
	{
		//lprintf( "Step simulation %d", now-l.last_tick );
		l.bullet.dynamicsWorld->stepSimulation( (now-l.last_tick)/(TIME_SCALE*1000.f), 20, 0.016/TIME_SCALE );
	}
	l.last_tick = now;
}

void SetupBullet( struct BulletInfo *_bullet )
{
	_bullet->broadphase = new btDbvtBroadphase();

	_bullet->collisionConfiguration = new btDefaultCollisionConfiguration();
    _bullet->dispatcher = new btCollisionDispatcher(_bullet->collisionConfiguration);

	_bullet->solver = new btSequentialImpulseConstraintSolver;

	_bullet->dynamicsWorld = new btDiscreteDynamicsWorld(_bullet->dispatcher,_bullet->broadphase,_bullet->solver,_bullet->collisionConfiguration);

	// mm/sec
	_bullet->dynamicsWorld->setGravity(btVector3(0,-9800,0) );

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
		_bullet->groundRigidBody->setRestitution( 1.0f );
		_bullet->groundRigidBody->setFriction( 0.8f );

		//_bullet->groundRigidBody->
		_bullet->dynamicsWorld->addRigidBody(_bullet->groundRigidBody);



}

// Creates a compound shaped tower centered at the ground level
static btCompoundShape* BuildTowerCompoundShape(const btVector3& brickFullDimensions=btVector3(580.0,15000.0,440.0),unsigned int numRows=3,unsigned int numBricksPerRow=16,bool useConvexHullShape=true)
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
	lprintf( "SET ROTATION" );
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
	btCollisionShape* barShape = new btCylinderShapeX( btVector3( /*235*/ 175, 10, 1.0 ) );

	  btDefaultMotionState* fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,23,0)));

		btScalar mass = 100;
        btVector3 fallInertia(0,0,0);
        barShape->calculateLocalInertia(mass,fallInertia);

		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,barShape,fallInertia);
        l.barRigidBody = new btRigidBody(fallRigidBodyCI);
        _bullet->dynamicsWorld->addRigidBody(l.barRigidBody);

		l.barRigidBody->setRestitution( 0.8 );

	btHingeConstraint* pivot = new btHingeConstraint( *_bullet->groundRigidBody
		, *l.barRigidBody
		, btVector3( 0, 75, 0 ) // pivitInA 
		, btVector3( -175, 0, 0 ) // PivotInB
		, btVector3( 0, 1, 0 ) // axisInA
		, btVector3( 0, 1, 0 ) // axisInB
		, true // use refernceframeA (the ground)
		);
	//lprintf( "yesno = %d", pivot->getAngularOnly() );
	//lprintf( "Limits = %g", pivot->getLowerLimit() );
	//lprintf( "Limits = %g", pivot->getUpperLimit() );
	pivot->setLimit( 1, -1
		, 0.1  // softness
		, 0.3 // biasFactor
		, 1.0f // relaxationFactor
		);
	//pivot->enableMotor( true );
	pivot->enableAngularMotor( true, 0.8, 100000 );

	//pivot->
	//pivot->setLimit(btScalar(-0.75 * M_PI_4), btScalar(M_PI_8));
			//hingeC->setLimit(btScalar(-0.1), btScalar(0.1));
			_bullet->dynamicsWorld->addConstraint(pivot, true);
		
}



void TestPopulatedBullet( struct BulletInfo *_bullet )
{
	for (int i=0 ; i<300 ; i++) {
                _bullet->dynamicsWorld->stepSimulation(1/60.f,100);
 
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


void CPROC tick( PTRSZVAL psv )
{
	_32 now = timeGetTime();

	static int skip = 40;
	static int phase;
	static int n;

	if( skip )
	{
		skip--;
		return;
	}
	// if no event yet....
	if( now < l.demo_tick_delay )
		return;
	//return;
	switch( phase )
	{
	case 0:
		l.flags.rack_balls = 1;
		l.demo_tick_delay = now + l.demo_time_wait_after_drop;
		phase = 1;
		break;
	case 1:
		{
			int ball = rand() * 75 / RAND_MAX;
			PHEXPATCH patch;
			patch = (PHEXPATCH)GetLink( &l.patches, ball );
			if( !patch )
				return;
			if( !patch->flags.simulated )
			{
				//if the ball wasn't picked, drop this, next tick, drop them.
				phase = 0;
				break;
			}
			BeginWatch( ball + 1 );
			l.demo_tick_delay = now + l.demo_time_wait_turn;
			phase = 2;
		}
		break;
	case 2:
		lprintf( "***** BEGIN PIVOT " );
		BeginPivot();
		l.demo_tick_delay = now + l.demo_time_wait_front;
		phase = 3;
		break;
	case 3:
		EndWatchBall();
		l.demo_tick_delay = now + l.demo_time_to_pick_ball;
		phase = 1;
		break;
	}
}

PRELOAD( InitDisplay )
{
	int n;
	TEXTCHAR name[256];
	SetupBullet( &l.bullet );
	PopulateBulletGround( &l.bullet );
	PopulateBulletContainer( &l.bullet );

	//if( 0 )
	//CreateMixerBar( &l.bullet );
	//PopulateBulletTest( &l.bullet );
	//TestPopulatedBullet( &l.bullet );

	if( SACK_GetPrivateProfileInt( "Ball Animation", "Enable Demo Mode", 1, "flashdrive.ini" ) )
	{
		AddTimer( 100, tick, 0 );
	}
	else
		l.flags.rack_balls = 1;

	//l.active_ball = 20;
	//l.active_ball_forward_tick = timeGetTime();
	
	l.time_to_home = SACK_GetPrivateProfileInt( "Ball Animation", "time to home position", 750, "flashdrive.ini" );
	l.time_to_home = 4000;
	l.time_to_track = SACK_GetPrivateProfileInt( "Ball Animation", "time to track ball", 1500, "flashdrive.ini" );
	l.time_to_rack = SACK_GetPrivateProfileInt( "Ball Animation", "time to rack ball", 1800, "flashdrive.ini" );
	l.time_to_approach = SACK_GetPrivateProfileInt( "Ball Animation", "time to approach ball", 1500, "flashdrive.ini" );
	l.time_to_turn_ball = SACK_GetPrivateProfileInt( "Ball Animation", "time to turn ball", 800, "flashdrive.ini" );

		 
	l.demo_time_wait_after_drop  = SACK_GetPrivateProfileInt( "Ball Animation", "Demo Mode/time to pick first ball", 12000, "flashdrive.ini" );
   // this also is time to watch, time to approach plus some time.
	l.demo_time_wait_turn = SACK_GetPrivateProfileInt( "Ball Animation", "Demo Mode/time to wait on back", 3200, "flashdrive.ini" );
	l.demo_time_wait_front = SACK_GetPrivateProfileInt( "Ball Animation", "Demo Mode/time to wait on front", 5000, "flashdrive.ini" );
	l.demo_time_to_pick_ball = SACK_GetPrivateProfileInt( "Ball Animation", "Demo Mode/time to pick a new ball", 7000, "flashdrive.ini" );


	l.flags.nextball_mode = SACK_GetPrivateProfileInt( "Ball Animation", "Display as Nextball mode", 0, "flashdrive.ini" );
	if( !l.flags.nextball_mode )
		l.show_ball_time = SACK_GetPrivateProfileInt( "Ball Animation", "Show Ball For How long", 8000, "flashdrive.ini" );
	else
		l.show_back_time = SACK_GetPrivateProfileInt( "Ball Animation", "Show Ball back For How long", 2000, "flashdrive.ini" );

	l.fade_duration = SACK_GetPrivateProfileInt( "Ball Animation", "Called balls fade in how long", 1300, "flashdrive.ini" );
	l.hex_size = SACK_GetPrivateProfileInt( "Ball Animation", "Ball Resolution", 9, "flashdrive.ini" );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Player Appreciation", "Images/balls/PlayerBall.png", name, 256, "flashdrive.ini" );
	l.player_image = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Wild Ball", "Images/balls/wild.png", name, 256, "flashdrive.ini" );
	l.wild_image = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 1", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[0] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 2", "Images/balls/troll.png", name, 256, "flashdrive.ini" );
	l.hotball_image[1] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 3", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[2] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 4", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[3] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 5", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[4] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 6", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[5] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 7", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[6] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 8", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[7] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 9", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[8] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 10", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[9] = LoadImageFile( name );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Hotball 11", "Images/balls/hotballtext.png", name, 256, "flashdrive.ini" );
	l.hotball_image[10] = LoadImageFile( name );

	SACK_GetPrivateProfileString( "Ball Animation", "Images/Ball Logo", "Images/balls/bingostarlogo.png", l.logo_name, 256, "flashdrive.ini" );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Ball Numbers", "Images/balls/NewBallTextGrid.png", l.numbers.image_name, 256, "flashdrive.ini" );
	SACK_GetPrivateProfileString( "Ball Animation", "Images/Ball Numbers Normal", "Images/balls/roughbump.png", l.numbers.bump_image_name, 256, "flashdrive.ini" );


	InitializeCriticalSec( &csUpdate );
	l.colors[0] = BASE_COLOR_BLUE;
	l.colors[1] = BASE_COLOR_RED;
	l.colors[2] = Color( 173,173,173 );
	l.colors[3] = Color( 25, 163, 25 );
	l.colors[4] = Color( 0xd8, 0x7d, 0x1e );

}


PUBLIC( void, ExportThis )( void )
{
}
