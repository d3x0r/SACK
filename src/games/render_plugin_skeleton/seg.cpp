
// Source mostly from

#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

// define local instance.
#define TERRAIN_MAIN_SOURCE  
#include <vectlib.h>
#include <render.h>
#include <render3d.h>

#ifndef SOMETHING_GOES_HERE
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <virtuality.h>
//#include "local.h"




static uintptr_t OnInit3d( "Seg Renderer" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{

	return 1;
}

static void OnFirstDraw3d( "Seg Renderer" )( uintptr_t psvInit )
{
}

static void OnBeginDraw3d( "Seg Renderer" )( uintptr_t psvInit, PRAY mouse )
{

}

static void OnDraw3d( "Seg Renderer" )( uintptr_t psvInit, PRAY mouse )
{
}

static void OnMouse3d( "Virtuality" )( uintptr_t psvInit, PRAY mouse, uint32_t b )
{
}