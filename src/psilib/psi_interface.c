

#include <controls.h>

#undef ALIAS_WRAPPER

// quick and dirty fix... just get rid of this param
#define ALIAS_WRAPPER(name) name

PSI_PROC(PSI_CONTROL, NotNULL)( PCOMMON pf, int options, int x, int y, int w, int h,
                                 int viewport_x, int viewport_y, int total_x, int total_y,
									 int row_thickness, int column_thickness, PTRSZVAL nID )
{
   return NULL;
}


static struct control_interface_tag ControlInterface = {
#include "interface_data.h"
};

PCONTROL_INTERFACE GetControlInterface( void )
{
	return &ControlInterface;
}

void DropControlInterface( void )
{
}

// $Log: psi_interface.c,v $
// Revision 1.2  2004/10/12 23:55:10  d3x0r
// checkpoint
//
// Revision 1.1  2004/09/19 19:22:32  d3x0r
// Begin version 2 psilib...
//
// Revision 1.9  2003/05/18 16:22:32  panther
// Handle missing make grid box better
//
// Revision 1.8  2003/04/10 00:31:03  panther
// Working on PSI client/server/interface
//
// Revision 1.7  2003/04/09 15:58:18  panther
// Fix PSI interface to current definition
//
// Revision 1.6  2003/03/27 15:26:12  panther
// Add Service Unloading in message service layer - still need a service notification message
//
// Revision 1.5  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
