// well do stuff here....
#include <windows.h>
#include <stdio.h>
#include "plugin.h"
#define FORCE_95 0

// common DLL plugin interface.....

#ifndef _LINUX
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif

int myTypeID; // supplied for uhmm... grins...

int b95;

int CPROC OpenViewPort( PSENTIENT ps, PTEXT parameters );


char *RegisterRoutines( /*PEXPORTTABLE pExportTable*/ )
{
   //pExportedFunctions = pExportTable;
#ifndef _LINUX
	{
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof( osvi );
		GetVersionEx( &osvi );
		if( (osvi.dwPlatformId  != VER_PLATFORM_WIN32_NT) || FORCE_95)
			b95 = TRUE;
		else
			b95 = FALSE;
	}
#endif
//   myTypeID = RegisterDevice( "console", "Windows based console....", Open );
 	RegisterRoutine( "3DView", "Open a 3D Viewport...", OpenViewPort );
	// should create some devices....

	return DekVersion;
}

void UnloadPlugin( void ) // this routine is called when /unload is invoked
{
	UnregisterRoutine( "3DView" );
//   UnregisterRoutine( "lines" );
//   UnregisterDevice( "console" );
}

// $Log: Ntlink.c,v $
// Revision 1.3  2003/03/25 09:41:11  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:01  panther
// Added CVS logging
//
