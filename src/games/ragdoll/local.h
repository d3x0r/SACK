#ifdef __cplusplus
#include <btBulletDynamicsCommon.h>
//#include "../Demos/OpenGL/GlDebugDrawer.h"
#endif

#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

#include <render3d.h>



#include <GL/gl.h>
#include <GL/glu.h>


#define RAGDOLL_NAMESPACE namespace ragdoll {
#define RAGDOLL_NAMESPACE_END }



RAGDOLL_NAMESPACE

struct BulletInfo
{
	btBroadphaseInterface* broadphase;

	btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;

	btSequentialImpulseConstraintSolver* solver;

	btDiscreteDynamicsWorld* dynamicsWorld;


	btRigidBody* fallRigidBody;  // for test routine 
	btRigidBody* groundRigidBody; // the ground plane at 0.
};	

struct body_part
{
	CTEXTSTR name;
	btCollisionShape *shape;
	 btRigidBody *fallRigidBody ;
};


#ifndef RAGDOLL_MAIN_SOURCE
extern
#endif
	struct local_ragdoll_data_tag
{
	struct {
		BIT_FIELD field : 1;
	} flags;
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;

   struct BulletInfo bullet;
  // GLDebugDrawer *debug_drawer;
   _32 last_tick;  // this is what tick we are rendering for.  (it's more like now than last.)
   RCOORD time_scale;
   PTRANSFORM origin;
}local_ragdoll_data;
#define l local_ragdoll_data



RAGDOLL_NAMESPACE_END
