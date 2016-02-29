
#include "local.h"


RAGDOLL_NAMESPACE




void SetupBullet( struct BulletInfo *_bullet )
{
	_bullet->broadphase = new btDbvtBroadphase();

	_bullet->collisionConfiguration = new btDefaultCollisionConfiguration();
    _bullet->dispatcher = new btCollisionDispatcher(_bullet->collisionConfiguration);

	_bullet->solver = new btSequentialImpulseConstraintSolver;

	_bullet->dynamicsWorld = new btDiscreteDynamicsWorld(_bullet->dispatcher,_bullet->broadphase,_bullet->solver,_bullet->collisionConfiguration);

	_bullet->dynamicsWorld->setGravity(btVector3(0,-900,0));

}



PRIORITY_PRELOAD( InitLocal, DEFAULT_PRELOAD_PRIORITY - 1 )
{
	l.pri = GetDisplayInterface();
	l.pii = GetImageInterface();

	SetupBullet( &l.bullet );
	//l.debug_drawer = new GLDebugDrawer();
	//l.bullet.dynamicsWorld->setDebugDrawer( l.debug_drawer );
	//l.debug_drawer->setDebugMode( btIDebugDraw::DBG_DrawWireframe );
	l.time_scale = 100.0;
}



RAGDOLL_NAMESPACE_END



