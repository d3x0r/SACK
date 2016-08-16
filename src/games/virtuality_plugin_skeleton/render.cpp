
#define RAGDOLL_MAIN_SOURCE
#include "local.h"


RAGDOLL_NAMESPACE

 void CPROC UpdatePositions( uintptr_t psv )
{
	uint32_t now = timeGetTime();

	//ApplyExternalForces();
	if( l.last_tick )
	{
		//lprintf( "Step simulation %d", now-l.last_tick );
		l.bullet.dynamicsWorld->stepSimulation( (now-l.last_tick)/(l.time_scale*1000.f), 10, 0.016/l.time_scale );
	}
	l.last_tick = now;	
}       


static void OnDraw3d( "Ragdoll Physics" )( uintptr_t psv )
{

	l.bullet.dynamicsWorld->debugDrawWorld();
}

static void OnBeginDraw3d( "Ragdoll Physics" )( uintptr_t psv, PTRANSFORM camera )
{
	UpdatePositions( 0 );

	
}

static uintptr_t OnInit3d( "Ragdoll physics" )( PMatrix projection, PTRANSFORM camera, RCOORD *unit_distance, RCOORD *aspect )
{
	return (uintptr_t)camera;
}



RAGDOLL_NAMESPACE_END


