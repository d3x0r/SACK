// well do stuff here....
#include <stdhdrs.h>
#include <stdlib.h> // srand
#include <stdio.h>

#include "plugin.h"

#ifndef __LINUX__
int CPROC SystemShutdown( PSENTIENT ps, PTEXT param );
int CPROC Sound( PSENTIENT ps, PTEXT parameters );
#endif
int nSystemID;
PDATAPATH CPROC SysCommand( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
INDEX iProcess;
int CPROC MakeProcess( PSENTIENT ps, PENTITY peInit, PTEXT parameters );

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
#ifndef __LINUX__
		RegisterRoutine( "System", "Shutdown", "Windows System Shutdown...", SystemShutdown );
		RegisterRoutine( "System", "sound", "Play a sound using windows multimedia...", Sound );
		srand( GetTickCount() );
#endif
		nSystemID = RegisterDevice( "system", "Launch a system command for use with IO commands", SysCommand );
		iProcess = RegisterExtension( "process" );
		RegisterObject( "process", "Process monitor/launch/control", MakeProcess );
		//return DekVersion;
	}
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
#ifndef __LINUX__
   UnregisterRoutine( "Shutdown" );
   UnregisterRoutine( "sound" );
#endif
	UnregisterObject( "process" );
	UnregisterDevice( "system" );
}
// $Log: ntlink.c,v $
// Revision 1.6  2005/02/21 12:08:59  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.5  2003/03/25 08:59:03  panther
// Added CVS logging
//
