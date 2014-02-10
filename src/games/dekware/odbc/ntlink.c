// well do stuff here....
#include <stdhdrs.h>
#include <stdio.h>
#include "plugin.h"


// common DLL plugin interface.....
#if !defined( __STATIC__ ) && !defined( __LINUX__ )
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

//int HandleODBC( PSENTIENT ps, PTEXT parameters );

int CPROC Create( PSENTIENT ps, PENTITY pe, PTEXT parameters );

INDEX iODBC;
//int myTypeID; // supplied for uhmm... grins...

PUBLIC( char *, RegisterRoutines )( void )
{
   //pExportedFunctions = pExportTable;

   //UpdateMinSignficants( commands, nCommands, NULL );
   RegisterObject( "odbc", "Generic ODBC Object... parameters determine database", Create );
   //RegisterRoutine( "odbc", "Microsoft SQL Interface Commands (/odbc help)", HandleODBC );
   // not a real device type... but need the ID...
   //myTypeID = RegisterDevice( "odbc", "ODBC Database stuff", NULL );
   iODBC = RegisterExtension( "odbc" );
	return DekVersion;
}


PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterObject( "odbc" );
   //UnregisterRoutine( "odbc" );
}

// $Log: ntlink.c,v $
// Revision 1.9  2005/02/22 12:28:48  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.8  2004/12/15 05:59:34  d3x0r
// Updates for linux building happiness
//
// Revision 1.7  2003/10/01 14:47:43  panther
// Update to use colmn count instead of faulting last column
//
// Revision 1.6  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.5  2003/03/25 08:59:02  panther
// Added CVS logging
//
