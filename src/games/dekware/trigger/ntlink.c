// well do stuff here....
#include <stdhdrs.h>
#include "plugin.h"


// common DLL plugin interface.....
#ifdef WIN32
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

//--------------------------------------------------------------------------

// $Log: ntlink.c,v $
// Revision 1.8  2005/01/27 08:28:30  d3x0r
// Make triggers self contained - eliminate more possible conflicts of library linkage
//
// Revision 1.7  2003/03/25 08:59:03  panther
// Added CVS logging
//
