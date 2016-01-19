// well do stuff here....
#include <stdhdrs.h>
#include <stdio.h>
#include "plugin.h"


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

