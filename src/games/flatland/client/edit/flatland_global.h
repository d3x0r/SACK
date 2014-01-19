
#ifndef FLATLAND_GLOBAL_DEFINED
#define FLATLAND_GLOBAL_DEFINED

#ifdef USE_WORLDSCAPE_INTERFACE
#define WORLDSCAPE_INTERFACE_USED
#define USE_WORLD_SCAPE_INTERFACE g.wsi
#endif

#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pri

#include <world.h>
#include "display.h"

typedef struct flatland_global_data
{ 
	PDISPLAY pDisplay;
   PSI_CONTROL pc;
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pri;
#ifdef USE_WORLDSCAPE_INTERFACE
	PWORLD_SCAPE_INTERFACE wsi;
#endif
} FLATLAND_GLOBAL;

#ifndef FLATLAND_MAIN
extern
#endif
	FLATLAND_GLOBAL g;


#endif
