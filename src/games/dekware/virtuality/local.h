#define FIX_RELEASE_COM_COLLISION
#define MAKE_RCOORD_SINGLE
#define USE_IMAGE_INTERFACE l.pii
#define USE_IMAGE_3D_INTERFACE l.pi3i
#define USE_RENDER3D_INTERFACE l.pr3i

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE

#include <plugin.h>
#include <vectlib.h>
#include <virtuality.h>
#include <render3d.h>
#include <image3d.h>

#include <brain.hpp>
#include <brainshell.hpp>


#ifndef MAIN_SOURCE
extern
#endif
	struct local_virtuality_interface_tag
{
   INDEX extension;
   PIMAGE_INTERFACE pii;
   PIMAGE_3D_INTERFACE pi3i;
   PRENDER3D_INTERFACE pr3i;
   PTRANSFORM transform;
   PLIST objects;
   POBJECT root_object;
} l;

struct virtuality_object
{
	//VECTOR o;// objects actually origins already
	Image label;  // objects actually have labels already
	POBJECT object;
	PBRAIN brain;
	PBRAINBOARD brain_board;
	VECTOR speed;
	VECTOR rotation_speed;
};

void ExtendEntityWithBrain( PENTITY pe_created );

