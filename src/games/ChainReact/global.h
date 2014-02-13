
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pri

#include <controls.h>

#ifndef CHAIN_REACT_MAIN
extern
#endif
	struct {
		PIMAGE_INTERFACE pii;
      PRENDER_INTERFACE pri;
	} global_chain_react_data;

#define g global_chain_react_data
