// well do stuff here....
#include <stdhdrs.h>
#include <deadstart.h>
#include "plugin.h"


// common DLL plugin interface.....
LIBMAIN()
{
   return TRUE; // success whatever the reason...
}
LIBEXIT()
{
   return TRUE; // success whatever the reason...
}
LIBMAIN_END();

// $Log: ntlink.c,v $
// Revision 1.7  2005/01/17 09:01:15  d3x0r
// checkpoint ...
//
// Revision 1.6  2004/05/12 08:20:52  d3x0r
// Implement file ops with standard options handling
//
// Revision 1.5  2003/03/25 08:59:01  panther
// Added CVS logging
//
