#include <stdhdrs.h>

#ifdef __LINUX__
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#else
// include extended symbol definitions.
// including windows.h auto includes opengl.
#include "../../glext.h"
#endif


#include <render.h>
#include <render3d.h>


PRELOAD( Initialization )
{
}

static PTRSZVAL OnInit3d( "Example" )( PTRANSFORM camera, RCOORD *pIdentityDepth, RCOORD *pAspect )
{
   return 1;
}

static void OnBeginDraw3d( "Example" )( PTRSZVAL psv )
{
	// psv is the return value from init

	// this is called on the beginning of a new draw sequence....

   // not really sure what the suggested thing to do here is; most work should be in Draw3d

}

static void Draw3d( "Example" )( PTRSZVAL psv )
{
	// psv is the return value from init
	// this is called after all normal flat displays are drawn.

   // do normal scene drawing here.
}

static LOGICAL OnMouse3d( "Example" )( PTRSZVAL psv, PRAY mouse_ray, _32 b )
{
	// psv is the return value from init.
	// mouse_ray is a ray that describes a line from the camera point into the scene.
	// b is the button states of the mouse, as would be received with MouseMethod

   return FALSE; // return whether the mouse was used or not
}

