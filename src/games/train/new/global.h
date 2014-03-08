
#ifndef FORCE_NO_INTERFACE
#define USE_RENDER_INTERFACE g.pri
#define USE_IMAGE_INTERFACE g.pii
#endif

#include <image.h>
#include <render.h>

#ifndef TRAIN_MAIN
extern
#endif
struct {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pri;
} global_virtuality_data;

#define g global_virtuality_data

