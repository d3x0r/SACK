/* Header file defining specific slider interface methods. These
   work on the generic PSI_CONTROL that is a button, but do
   expect specifically a control that is their type.             */


#include <deadstart.h>
#define SLIDER_UPDATE( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY "/control/Slider/Update" \
	                                        , name, "void", #name, "(uintptr_t,PCONTROL,int)" ); } \
	void CPROC name args



