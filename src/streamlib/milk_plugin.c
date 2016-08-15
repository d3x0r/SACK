
#include <../src/apps/milk/milk_registry.h>




OnCreateControl( "Video Capture") (PSI_CONTROL parent,int32_t x,int32_t y,uint32_t width,uint32_t height)
{
   PSI_CONTROL pc = MakeNamedControl( parent, "Video Control", x, y, width, height, -1 );
	// return uintptr_t:
   return (uintptr_t)pc;
}

OnGetControl( "Video Capture" )( uintptr_t psv )
{
   return (PSI_CONTROL)psv;
}


