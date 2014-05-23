
#include <stdhdrs.h>
#include <logging.h>
#include <image.h>
#include <render.h>
#include <vectlib.h>

#include <sharemem.h>
//#include ""
//#include "view.hpp" // perhaps should just USE view code?
#include <virtuality.h>

#if 0
#define CUBE_SIDES 6
BASIC_PLANE CubeNormals[] = { { {0,0,0}, {    0,  0.5,    0 }/*, 1, 62 */},
                    { {0,0,0}, { -0.5,    0,    0 }/*, 1, 126 */},
                    { {0,0,0}, {  0.5,    0,    0 }/*, 1, 190 */},
                    { {0,0,0}, {    0,    0, -0.5 }/*, 1, 244 */},
                    { {0,0,0}, {    0,    0,  0.5 }/*, 1, 31 */},
                    { {0,0,0}, {    0, -0.5,    0 }/*, 1, 94 */},
                                                  
                    { {0,0,0}, {  0.22, 0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22,-0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, { -0.22, 0.22,  0.4}/*, 1, 168 */},
                    { {0,0,0}, { -0.22,-0.22,  0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22, 0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.22,-0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, { -0.22, 0.22, -0.4}/*, 1, 168 */},
                    { {0,0,0}, { -0.22,-0.22, -0.4}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, {  0.43,-0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, { -0.43, 0.43,  0}/*, 1, 168 */},
                    { {0,0,0}, { -0.43,-0.43,  0}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0, 0.43}/*, 1, 46 */},
                    { {0,0,0}, {  0.43, 0, -0.43}/*, 1, 46 */},
                    { {0,0,0}, { -0.43, 0, 0.43}/*, 1, 168 */},
                    { {0,0,0}, { -0.43, 0, -0.43}/*, 1, 46 */},

// offset to origin
                    { {0,0.5,0}, { 0, -0.1, 0}/*, -1, 126*/},
                    { {0.5,0.5,0}, { -0.1, 0, 0}/*, -1,  126*/},
                    { {-0.5,0.5,0}, { 0.1, 0, 0}/*, -1,  126*/},
                    { {0,0.5,0.5}, { 0, 0, -0.1}/*, -1,  126*/},
                    { {0,0.5,-0.5}, { 0, 0, 0.1}/*, -1,  126*/}

                  };


// 100.8 degrees  ( 0, 72, 144, -144, -72 )
// 142.6 degrees
// 79.2           ( 36, 108, 180, -108, -36 )
// 34.4
//BASIC_PLANE icosahedron[] = { };

#endif
#define DODECA_SIDES 12
BASIC_PLANE dodecahedron[] = { { {0,0,0}, { -0.526, 0.0, 0.851 } }
									  , { {0,0,0}, { 0.526, 0.0, 0.851 } }
									  , { {0,0,0}, { -0.526, 0.0, -0.851 } }
									  , { {0,0,0}, { 0.526, 0.0, -0.851 } }
									  , { {0,0,0}, { 0.0, 0.851, 0.525 } }
									  , { {0,0,0}, { 0.0, 0.851, -0.525 } }
									  , { {0,0,0}, { 0.0, -0.851, 0.525 } }
									  , { {0,0,0}, { 0.0, -0.851, -0.525 } }
									  , { {0,0,0}, { 0.851, 0.525, 0.0 } }
									  , { {0,0,0}, { -0.851, 0.525, 0.0 } }
									  , { {0,0,0}, { 0.851, -0.525, 0.0 } }
									  , { {0,0,0}, { -0.851, -0.525, 0.0 } } };
#define ICOSA_SIDES 20
BASIC_PLANE icosahedron[20] = { { {0,0,0}, { 0, 0.342952, 0.895252 } }
										, { {0,0,0}, { -0.553051, 0.553576, 0.552725 } }
										, { {0,0,0}, { -0.3423, 0.89355, 0 } }
										, { {0,0,0}, { 0.3423, 0.89355, 0 } }
										, { {0,0,0}, { 0.553051, 0.553576, 0.552725 } }
										, { {0,0,0}, { 0.89355, 0, 0.34125 } }
										, { {0,0,0}, { 0.89355, 0, -0.34125 } }
										, { {0,0,0}, { 0.553051, 0.553576, -0.552725 } }
										, { {0,0,0}, { 0, 0.342952, -0.895252 } }
										, { {0,0,0}, { 0, -0.342952, -0.895252 } }
										, { {0,0,0}, { 0.553051, -0.553576, -0.552725 } }
										, { {0,0,0}, { 0.3423, -0.89355, 0 } }
										, { {0,0,0}, { -0.3423, -0.89355, 0 } }
										, { {0,0,0}, { -0.553051, -0.553576, 0.552725 } }
										, { {0,0,0}, { 0, -0.342952, 0.895252 } }
										, { {0,0,0}, { 0.553051, -0.553576, 0.552725 } }
										, { {0,0,0}, { -0.89355, 0, 0.34125 } }
										, { {0,0,0}, { -0.89355, 0, -0.34125 } }
										, { {0,0,0}, { -0.553051, 0.553576, -0.552725 } }
										, { {0,0,0}, { -0.553051, -0.553576, -0.552725 } } };

#define _80_SIDES 80
BASIC_PLANE _80sides[80];

BASIC_PLANE _320sides[320];
BASIC_PLANE _1280sides[1280];
/*
 taken from http://www.cs.clemson.edu/~bobd/illuminate/index5.html
**  The code below creates an icosahedron, based on the
** code from page 84 of the OpenGL Programming Guide, second
** edition.  In the book, the triangles are not specified in
** counter clockwise order, so they had to be reordered here.
*/
// also dodeca hedron above is the normals here - (points) - probably
// smothing of the points to cause a sphere effect with opengl
_POINT icosa_points[] = { { -0.526, 0.0, 0.851 }
							  , { 0.526, 0.0, 0.851 }
							  , { -0.526, 0.0, -0.851 }
							  , { 0.526, 0.0, -0.851, }
							  , { 0.0, 0.851, 0.525 }
							  , { 0.0, 0.851, -0.525 }
							  , { 0.0, -0.851, 0.525 }
							  , { 0.0, -0.851, -0.525 }
							  , { 0.851, 0.525, 0.0 }
							  , { -0.851, 0.525, 0.0 }
							  , { 0.851, -0.525, 0.0 }
							  , { -0.851, -0.525, 0.0 } };
int icosa_index[][3] = { { 4, 0, 1 }
							  , { 9, 0, 4 }
							  , {  5, 9, 4 }
							  , {  5, 4, 8 }
							  , {  8, 4, 1 }
							  , {  10, 8, 1 }
							  , {  3, 8, 10 }
							  , {  3, 5, 8 }
							  , {  2, 5, 3 }
							  , {  7, 2, 3 }
							  , {  10, 7, 3 }
							  , {  6, 7, 10 }
							  , {  11, 7, 6 }
							  , {  0, 11, 6 }
							  , {  1, 0, 6 }
							  , {  1, 6, 10 }
							  , {  0, 9, 11 }
							  , {  11, 9, 2 }
							  , {  2, 9, 5 }
							  , {  2, 7, 11 } };

static int depth;
void FillQuadSet( BASIC_PLANE *set, int base, int levels, P_POINT i1, P_POINT i2, P_POINT i3 )
{
	VECTOR p1, p2, p3;
	VECTOR v1, v2;
	levels--;
   depth++;
	add( p1, i2, i1 );
   normalize( p1 );
	//scale( p1, p1, 1.0 / Length( p1 ) );
	add( p2, i3, i2 );
   normalize( p2 );
   //scale( p2, p2, 1.0 / Length( p2 ) );
	add( p3, i1, i3 );
   normalize( p3 );
	//scale( p3, p3, 1.0 / Length( p3 ) );
	if( levels )
	{
      /*
		if( levels > 1 )
		{
			FillQuadSet( set, base, levels, p1, p2, p3 );
			FillQuadSet( set, base+16, levels, p1, p2, i2 );
			FillQuadSet( set, base+32, levels, p2, p3, i3 );
			FillQuadSet( set, base+48, levels, p3, p1, i1 );
		}
		else
      */
		{
			FillQuadSet( set, base+(0<<(2*levels)), levels, p1, p2, p3 );
			FillQuadSet( set, base+(1<<(2*levels)), levels, p1, p2, i2 );
			FillQuadSet( set, base+(2<<(2*levels)), levels, p2, p3, i3 );
			FillQuadSet( set, base+(3<<(2*levels)), levels, p3, p1, i1 );
		}
	}
	else
	{
		//Log1( "Base facet is : %d", base );
		sub( v1, p3, p2 );
		sub( v2, p2, p1 );
		crossproduct( set[base].n, v1, v2 );
		normalize( set[base].n );
		//if( depth == 1 )
		//	scale( set[base].n, set[base].n, 0.99 );
      //if( depth == 2 )
		//	scale( set[base].n, set[base].n, 0.999 );
		//PrintVector( set[base].n );
		sub( v1, i2, p1 );
		sub( v2, p2, i2 );
		crossproduct( set[base+1].n, v1, v2 );
		normalize( set[base+1].n );
		//PrintVector( set[base+1].n );
		sub( v1, i3, p2 );
		sub( v2, p3, i3 );
		crossproduct( set[base+2].n, v1, v2 );
		normalize( set[base+2].n );
		//PrintVector( set[base+2].n );
		sub( v1, i1, p3 );
		sub( v2, p1, i1 );
		crossproduct( set[base+3].n, v1, v2 );
		normalize( set[base+3].n );
		//PrintVector( set[base+3].n );
	}
   depth--;
}

int gbExitProgram;
int main( void )
{
   SetHeapUnit( 16000000 );
   if( 0 )
	{
		int n, p;
		for( n = 0; n < 12; n++ )
		{
         normalize( icosa_points[n] );
		}

		for( n = 0; n < 20; n++ )
		{
			VECTOR p1, p2, p3;
         VECTOR v1, v2;
			scale( p1, add( p1, icosa_points[icosa_index[n][1]], icosa_points[icosa_index[n][0]] ), 0.5 );
         scale( p1, p1, 1.0 / Length( p1 ) );
         scale( p2, add( p2, icosa_points[icosa_index[n][2]], icosa_points[icosa_index[n][1]] ), 0.5 );
         scale( p2, p2, 1.0 / Length( p2 ) );
			scale( p3, add( p3, icosa_points[icosa_index[n][0]], icosa_points[icosa_index[n][2]] ), 0.5 );
         scale( p3, p3, 1.0 / Length( p3 ) );
			sub( v1, icosa_points[icosa_index[n][1]], icosa_points[icosa_index[n][0]] );
         sub( v2, icosa_points[icosa_index[n][2]], icosa_points[icosa_index[n][1]] );
			crossproduct( icosahedron[n].n, v1, v2 );
         normalize( icosahedron[n].n );
         PrintVector( icosahedron[n].n );
		}
		for( n = 0; n < 20; n++ )
		{
         Log( "Fill 80 sides..." );
			FillQuadSet( _80sides, n*4, 1
						  , icosa_points[icosa_index[n][0]]
						  , icosa_points[icosa_index[n][1]]
						  , icosa_points[icosa_index[n][2]] );
         Log( "Fill 320 sides..." );
			FillQuadSet( _320sides, n*16, 2
						  , icosa_points[icosa_index[n][0]]
						  , icosa_points[icosa_index[n][1]]
						  , icosa_points[icosa_index[n][2]] );
         Log( "Fill 1280 sides..." );
			FillQuadSet( _1280sides, n*64, 3
						  , icosa_points[icosa_index[n][0]]
						  , icosa_points[icosa_index[n][1]]
						  , icosa_points[icosa_index[n][2]] );
      }
/*
		for( n = 0; n < 20; n++ )
		{
			VECTOR p1, p2, p3;
         VECTOR v1, v2;
			scale( p1, add( p1, icosa_points[icosa_index[n][1]], icosa_points[icosa_index[n][0]] ), 0.5 );
         scale( p1, p1, 1.0 / Length( p1 ) );
         scale( p2, add( p2, icosa_points[icosa_index[n][2]], icosa_points[icosa_index[n][1]] ), 0.5 );
         scale( p2, p2, 1.0 / Length( p2 ) );
			scale( p3, add( p3, icosa_points[icosa_index[n][0]], icosa_points[icosa_index[n][2]] ), 0.5 );
         scale( p3, p3, 1.0 / Length( p3 ) );
			sub( v1, p3, p2 );
         sub( v2, p2, p1 );
			crossproduct( _80sides[n*4].n, v1, v2 );
			normalize( _80sides[n*4].n );
         scale( _80sides[n*4].n, _80sides[n*4].n, 0.99 );
         PrintVector( _80sides[n*4].n );
			sub( v1, icosa_points[icosa_index[n][1]], p1 );
         sub( v2, p2, icosa_points[icosa_index[n][1]] );
			crossproduct( _80sides[n*4+1].n, v1, v2 );
         normalize( _80sides[n*4+1].n );
         PrintVector( _80sides[n*4+1].n );
		   sub( v1, icosa_points[icosa_index[n][2]], p2 );
         sub( v2, p3, icosa_points[icosa_index[n][2]] );
			crossproduct( _80sides[n*4+2].n, v1, v2 );
         normalize( _80sides[n*4+2].n );
         PrintVector( _80sides[n*4+2].n );
			sub( v1, icosa_points[icosa_index[n][0]], p3 );
         sub( v2, p1, icosa_points[icosa_index[n][0]] );
			crossproduct( _80sides[n*4+3].n, v1, v2 );
         normalize( _80sides[n*4+3].n );
			PrintVector( _80sides[n*4+3].n );

			//crossproduct( _80sides[n].n, v1, v2 );
         //normalize( _80sides[n].n );
         //PrintVector( _80sides[n].n );
			}
         */
		for( n = 0; n < 20; n++ )
		{
			VECTOR p1, p2, p3;
         VECTOR v1, v2;
			scale( p1, add( p1, icosa_points[icosa_index[n][1]], icosa_points[icosa_index[n][0]] ), 0.5 );
         scale( p1, p1, 1.0 / Length( p1 ) );
         scale( p2, add( p2, icosa_points[icosa_index[n][2]], icosa_points[icosa_index[n][1]] ), 0.5 );
         scale( p2, p2, 1.0 / Length( p2 ) );
			scale( p3, add( p3, icosa_points[icosa_index[n][0]], icosa_points[icosa_index[n][2]] ), 0.5 );
         scale( p3, p3, 1.0 / Length( p3 ) );
			sub( v1, icosa_points[icosa_index[n][1]], icosa_points[icosa_index[n][0]] );
         sub( v2, icosa_points[icosa_index[n][2]], icosa_points[icosa_index[n][1]] );
			crossproduct( icosahedron[n].n, v1, v2 );
         normalize( icosahedron[n].n );
         PrintVector( icosahedron[n].n );
		}
	}

   {
		POBJECT pWorld;
		VECTOR v, tmp;
      int n = 0, m = 0, z = 0;
		pWorld = //CreateScaledInstance( icosahedron, 20, 20.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
			CreateScaledInstance( CubeNormals, CUBE_SIDES, 10.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
		SetPoint( v, VectorConst_0 );
		///if( 0 )
#define CUBES 12
//#define CUBES 0
		for( z = 0; z < CUBES; z++ )
		{
			//scale( v, VectorConst_Z, z );
			for( m = 0; m < CUBES; m++ )
			{
				//add( v, v, VectorConst_Y );
				for( n = 0; n < CUBES; n++ )
				{
					POBJECT po;
               lprintf( "Create %d,%d,%d", z, m, n );
               add( v, scale( tmp, VectorConst_Z, z ), add( v, scale( tmp, VectorConst_Y, m ),scale( v, VectorConst_X, n ) ) );
					po = CreateScaledInstance( CubeNormals, CUBE_SIDES, 0.3f, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
					//po = CreateScaledInstance( icosahedron, 20, 0.3f, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
					SetObjectColor( po, Color( (255 *n)/ CUBES, (255*m)/CUBES, (255*z)/CUBES  ) );

				}
            //DebugDumpMem();
			}
			//DebugDumpMem();
		}
		//SetPoint( v, VectorConst_0 );
		add( v, scale( tmp, VectorConst_Z, z ), add( v, scale( tmp, VectorConst_Y, m ),scale( v, VectorConst_X, n ) ) );
		SetPoint( v, VectorConst_0 );
      if(0)
		{
         POBJECT po;
			po = CreateScaledInstance( icosahedron, 20, 100.0f, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
			//pWorld = CreateScaledInstance( dodecahedron, DODECA_SIDES, 10.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
			//pWorld = CreateScaledInstance( icosahedron, ICOSA_SIDES, 10.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
			SetObjectColor( po, Color( 159, 135, 23 ) );  // purple boundry...
			//CreateScaledInstance( _80sides, 80, 1.0f, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
		}

		//SetObjectColor( pWorld, Color( 79, 135, 153 ) );  // purple boundry...
   if(0)

		{
         POBJECT
				po;
			//po = CreateScaledInstance( _320sides, 320, 1000.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
			//SetObjectColor( po, Color( 179, 20, 153 ) );  // purple boundry...
		}
		{
         POBJECT po;
				//po = CreateScaledInstance( _1280sides, 1280, 10.0f, (PVECTOR)VectorConst_0, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
				//SetObjectColor( po, Color( 179, 20, 20 ) );  // purple boundry...
		}
      //InvertObject( pWorld ); // turn object inside out...
      SetRootObject( pWorld );
	{
		//PEDITOR pe;
		POBJECT po = CreatePlane( VectorConst_0, VectorConst_Z, VectorConst_X, 50, Color( 32, 163, 32 ) );
		PutIn( po, pWorld );
		//pe =  CreateEditor( po );

		{
			PVIEW pView;
			//pView = CreateView( NULL, "World View" );
			CreateViewEx( V_FORWARD, NULL, "Forward", 1, 1 );
			//CreateViewEx( V_UP, NULL, "Forward", 1, 0 );
			//CreateViewEx( V_DOWN, NULL, "Forward", 1, 2 );
			//CreateViewEx( V_LEFT, NULL, "Forward", 0, 1 );
			//CreateViewEx( V_RIGHT, NULL, "Forward", 2, 1 );
			//CreateViewEx( V_BACK, NULL, "Forward", 3, 1 );
		}
		{
         while( 1 ) WakeableSleep( 10000 );
		}
	//	DestroyEditor( pe );
	}	
   }

	return 0;
}

