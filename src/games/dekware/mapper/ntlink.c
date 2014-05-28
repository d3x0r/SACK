// well do stuff here....
#include <windows.h>
#include <stdio.h>
#include "plugin.h"


// common DLL plugin interface.....
#ifdef _WIN32
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

int myTypeID; // supplied to tag the device as mine

extern int InitMapObject( PSENTIENT ps, PENTITY pe, PTEXT parameters );
extern PDATAPATH OpenMapMon( PSENTIENT ps, PTEXT parameters );

char *RegisterRoutines( PEXPORTTABLE pExportTable )
{
   pExportedFunctions = pExportTable;
   myTypeID = RegisterDevice( "MapMon", "Device to monitor information for mappper", OpenMapMon );
   RegisterObject( "Map", "Object for MUDs to monitor data and provide mapping", InitMapObject );
//   RegisterRoutine( "LoadFile", "Load file as a binary variable", LoadFile );
//   RegisterRoutine( "StoreFile", "Save File from binary variable", StoreFile );
//   RegisterRoutine( "SaveFile", "Save File from binary variable", StoreFile );
   return DekVersion;
}

void UnloadPlugin( void ) // this routine is called when /unload is invoked
{
   UnregisterDevice( "MapMon" );
   UnregisterObject( "Map" );
   // destroy all map objects
	DestroyMapObjects();
   CloseMapDatapaths();

   // close all open datapaths...
//   UnregisterRoutine( "LoadFile" );
//   UnregisterRoutine( "StoreFile" );
//   UnregisterRoutine( "SaveFile" );
}

// $Log: ntlink.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:02  panther
// Added CVS logging
//
