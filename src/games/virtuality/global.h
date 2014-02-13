
#ifndef FORCE_NO_INTERFACE
#define USE_RENDER_INTERFACE g.pri
#define USE_RENDER_3D_INTERFACE g.pr3i
#define USE_IMAGE_INTERFACE g.pii
#endif

#include <image.h>
#include <render.h>
#include <render3d.h>
#include <image3d.h>
//#include <shader
#ifdef USE_GLES2
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <timers.h>

#include <virtuality.h>


typedef struct {
	LOGICAL bEditing;
	LOGICAL bInit;
	POBJECT pEditObject;
	PFACET pFacet;
	PFACETSET pfs;
	int nFacetSet;
	int nFacet;
	int Invert;
	PTRANSFORM TEdit;
	VECTOR vmin;
	RCOORD t_facet; // time along mouse-ray of intersection (closeness test)
} EDIT_INFO, *PEDIT_INFO;



#ifndef VIEW_MAIN
extern
#endif
struct global_virtuality_data_tag {
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pri;
	PRENDER3D_INTERFACE pr3i;
	PIMAGE_3D_INTERFACE pi3i;

	PMYLINESEGSET *ppLinePool; // common pool for utilities

	EDIT_INFO EditInfo;
	POBJECT pFirstRootObject;
	PTRANSFORM camera;
} global_virtuality_data;


#define g global_virtuality_data


#define pFirstObject g.pFirstRootObject //FirstObject.next

#define GetEditFacetSet() g.EditInfo.pfs
#define GetEditFacetIndex() g.EditInfo.nFacet
#define GetEditFacet() g.EditInfo.pFacet
