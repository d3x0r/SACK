
#include <deadstart.h>
#define SLIDER_UPDATE( name, args ) PUBLIC( void, name )args;   \
	PRELOAD( name##_Init ) { SimpleRegisterMethod( PSI_ROOT_REGISTRY "/control/Slider/Update" \
	                                        , name, "void", _WIDE(#name), "(uintptr_t,PCONTROL,int)" ); } \
	void CPROC name args



