#include <stdhdrs.h>
#include <vectlib.h>
#include <timers.h>
#include <sharemem.h>

#include <virtuality.h>


#define CUBE_SIDES 6
BASIC_PLANE CubeNormals[] = { { {0,0,0}, {    0,  0.5,    0 }/*, 1, 62 */}
									 , { {0,0,0}, { -0.5,    0,    0 }/*, 1, 126 */}
									 , { {0,0,0}, {  0.5,    0,    0 }/*, 1, 190 */}
									 , { {0,0,0}, {    0,    0, -0.5 }/*, 1, 244 */}
									 , { {0,0,0}, {    0,    0,  0.5 }/*, 1, 31 */}
									 , { {0,0,0}, {    0, -0.5,    0 }/*, 1, 94 */}
};


POBJECT master;
PLIST slaves;

void CPROC UpdateObjects( PTRSZVAL psv )
{
	// master->position accellerate in direction of origin
	//    master also seeks to go away from the closest three
	//    slaves...

#define KPH (3.6)   // m/s
#define MPH (KPH/0.62137)  // m/s

#define UNIVERSAL_GRAVITY  980.0   // 980 units per second, 1 unit = 1 cm (0.01m) /s
#define MASTER_FLEE_ACCEL_MAX  50.0 // 50cm/s/s ... in
#define SLAVE_PURSUE_ACCEL_MAX 45.0 // slightly slower accelleratino than master can flee at

#define UNIVERSAL_FRICTION (0.001)  /* applies such that v(n)=v(n-1)* UNIVERSAL_FRICTION */
#define BODY_TERMINAL_VELOCITY (120*MPH)

	{
		VECTOR o;
		struct {
			RCOORD dist;
			VECTOR v;
			POBJECT o;
#define MAX_CLOSE_TEST 5
		} close[MAX_CLOSE_TEST];
		int nClose = 0;
      INDEX idx;
		POBJECT slave;
		GetOriginV( master->Ti, o );
      PrintVector( o );
		LIST_FORALL( slaves, idx, POBJECT, slave )
		{
			VECTOR so;
			GetOriginV( slave->Ti, so );
			sub( so, so, o );
			if( nClose < MAX_CLOSE_TEST )
			{
				close[nClose].o = slave;
				SetPoint( close[nClose].v, so );
				close[nClose].dist = Length( so );
            nClose++;
			}
			else
			{
				int n;
            RCOORD dist = Length( so );
				for( n = 0; n < nClose; n++ )
				{
					if( dist < close[n].dist )
					{
                  int m;
						// do save this close...
						// find a slot that this slot is closer than...
						// then move this slot into that slot.
						// ... 16, 5, 9, 17, 18 - new is 7
                  // ... hrm... would seem that
						for( m = 0; m < nClose; m++ )
						{
							if( n == m )
								continue;
							if( close[n].dist < close[m].dist )
							{
								close[m] = close[n];
                        break;
							}
						}
						close[n].o = slave;
						SetPoint( close[n].v, so );
						close[n].dist = dist;
					}
				}
			}
		}
		{
			VECTOR v;
			int n;
         SetPoint( v, VectorConst_0 );
			for( n = 0; n < MAX_CLOSE_TEST; n++ )
			{
				//lprintf( "adding..." );
            //PrintVector( close[n].v );
            //add( v, v, close[n].v );
			}
			// head away from these...
			//scale( v, v, -1.0/50.0 );

			{
				VECTOR v;
            PrintVector( GetSpeed( master->Ti, v ) );
            SetSpeed( master->Ti, scale( v, GetSpeed( master->Ti, v ), 1.0-(UNIVERSAL_FRICTION) ) );
			}

			{
				RCOORD dist = Length( o )/100; // origin distance... scaled by 50.0 = 1, 100 = 0.125, 200 = 0.000125
				// 2 = 100, ...
				//lprintf( "dist = %g", dist );
				if( dist < 1 )
					dist = 1;
				if( dist > 3 )
               dist = 3;
				addscaled( v, v, o, -1/(25*dist*dist*dist) );
			}
			SetAccel( master->Ti, v );
         PrintVector( v );
         Move( master->Ti );
		}
		{
			INDEX idx;
			POBJECT slave;
			LIST_FORALL( slaves, idx, POBJECT, slave )
			{
				VECTOR v, so;
				{
					VECTOR v;
					SetSpeed( slave->Ti, scale( v, GetSpeed( slave->Ti, v ), 1.0-(10*UNIVERSAL_FRICTION) ) );
				}
				GetOriginV( slave->Ti, so );
				sub( v, o, so );
				{
					RCOORD dist = Length( v )/1000; // origin distance... scaled by 50.0 = 1, 100 = 0.125, 200 = 0.000125
					// 2 = 100, ...
					//lprintf( "dist = %g", dist );
					if( dist < 1 )
						dist = 1;
					if( dist > 3 )
						dist = 3;
					scale( v, v, 1/(25*dist*dist*dist) );
				}
				//scale( v, v, 1.0/100.0 );
				SetAccel( slave->Ti, v );
				Move( slave->Ti );
			}
		}
		// if I add rotation, this vector is screwed :)
		// I imply forward from the frame of motion, therefore
		// the rotation of the matrix needs to be applied to the
		// vector for true-space calculations - such as a fixed
      // point of gravity.
	}


}




int main( void )
{
	POBJECT pWorld;
	VECTOR v, tmp;
	int n = 0, m = 0, z = 0;
	v[0] = 100;
	v[1] = 50;
   v[2] = 75;
	//SetTimeScale( 0.5 );
	pWorld = CreateScaledInstance( CubeNormals, CUBE_SIDES, 1.0f, v, (PVECTOR)VectorConst_Z, (PVECTOR)VectorConst_X, (PVECTOR)VectorConst_Y );
	SetObjectColor( pWorld, BASE_COLOR_WHITE );
	{
		VECTOR speed;
		addscaled( speed, addscaled( speed, speed, VectorConst_X, -0.5 ), VectorConst_Y, -1.5 );
		SetSpeed( pWorld->Ti, speed );
	}
   master = pWorld;

#define CUBES 12
	//#define CUBES 0
   SetPoint( v, VectorConst_0 );
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
               AddLink( &slaves, po );
				}
            //DebugDumpMem();
			}
			//DebugDumpMem();
		}
		SetRootObject( pWorld );

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
      AddTimer( 50, UpdateObjects, 0 );
		{
         while( 1 ) WakeableSleep( 10000 );
		}
      return 0;
}



