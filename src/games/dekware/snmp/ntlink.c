// well do stuff here....
#include <windows.h>
#include <stdio.h>
#include "plugin.h"
//#define FORCE_95 1

// common DLL plugin interface.....

int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}

int myTypeID; // supplied for uhmm... grins...

//PDATAPATH Open( PSENTIENT ps, PTEXT parameters );
//int SetLines( PSENTIENT ps, PTEXT parameters );
//extern int b95;
BOOL ReadMib( void );

int SNMP( PSENTIENT ps, PTEXT parameters );

extern command_entry commands[];
extern int nCommands;

PUBLIC( char *, RegisterRoutines )( void )
{
	if( ReadMib() ) 
	{
	   UpdateMinSignficants( commands, nCommands );
		RegisterRoutine( "SNMP", "Accesss SNMP functions...", SNMP );
	}
//	else
//		UnloadSelf....
	return DekVersion;


//   myTypeID = RegisterDevice( "console", "Windows based console....", Open );
//	RegisterRoutine( "lines", "Set limit of console lines before pause", SetLines );
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{

}

// $Log: ntlink.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
