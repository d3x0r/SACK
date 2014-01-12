
#ifndef FORCE_NO_INTERFACE
#define USE_RENDER_INTERFACE g.pri
#define USE_RENDER_3D_INTERFACE g.pr3i
#define USE_IMAGE_INTERFACE g.pii
#endif

#include <image.h>
#include <render.h>
#include <render3d.h>
#ifdef USE_GLES2
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <timers.h>

#include <virtuality.h>

#ifndef VIEW_MAIN
extern
#endif
struct global_virtuality_data_tag {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pri;
	PRENDER3D_INTERFACE pr3i;

	PMYLINESEGSET *ppLinePool; // common pool for utilities

} global_virtuality_data;

#define g global_virtuality_data

