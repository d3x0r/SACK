
#include <../src/apps/milk/milk_registry.h>




OnCreateControl( "Video Capture") (PSI_CONTROL parent,S_32 x,S_32 y,_32 width,_32 height)
{
   PSI_CONTROL pc = MakeNamedControl( parent, "Video Control", x, y, width, height, -1 );
	// return PTRSZVAL:
   return (PTRSZVAL)pc;
}

OnGetControl( "Video Capture" )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}


