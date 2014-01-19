
#include <stdhdrs.h>

#include <render3d.h>


static class flc_calculator_local
{
public:
   RCOORD fi;
   RCOORD li;
	RCOORD ci;
	PSI_CONTROL control_frame;

public:
	l()
	{
      control_frame = LoadXMLFrame( NULL, "control_frame.frame" );
	}
}l;





