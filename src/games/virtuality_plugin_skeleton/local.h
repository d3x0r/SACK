
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

#include <render3d.h>


#ifdef __cplusplus
#include <btBulletDynamicsCommon.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>


#define RAGDOLL_NAMESPACE namespace ragdoll {
#define RAGDOLL_NAMESPACE_END }



RAGDOLL_NAMESPACE

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
}local_ragdoll_data;
#define l local_ragdoll_data



RAGDOLL_NAMESPACE_END
