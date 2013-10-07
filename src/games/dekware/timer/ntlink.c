
// well do stuff here....
#include <stdhdrs.h>
#include "plugin.h"


// common DLL plugin interface.....
#if defined( _MSC_VER ) || defined( WIN32 )
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

// $Log: ntlink.c,v $
// Revision 1.8  2005/01/17 09:01:32  d3x0r
// checkpoint ...
//
// Revision 1.7  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.6  2003/03/25 08:59:03  panther
// Added CVS logging
//
