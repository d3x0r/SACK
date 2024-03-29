#include <stdhdrs.h>
#include "plugin.h"
#include <time.h>

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
   srand( (unsigned int)time( NULL ) );
   //RegisterRoutine( "Roll", "Roll dice stored in macro result", Roll );
	}
	//return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
   UnregisterRoutine( "Roll" );
}
// $Log: ntlink.c,v $
// Revision 1.7  2003/10/08 02:29:20  panther
// Turn off logging by default, fix bash script, hack psi to display
//
// Revision 1.6  2003/10/01 00:44:04  panther
// Checkpoint
//
// Revision 1.5  2003/03/25 08:59:01  panther
// Added CVS logging
//
