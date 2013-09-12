#include <stdhdrs.h>
#define PLUGIN_MODULE
#include "plugin.h"




// common DLL plugin interface.....
//-----------------------------------------------------------------------------

PUBLIC( char *, RegisterRoutines )( void )
{
	// RegisterRoutine
	// RegisterDevice
	// RegisterObject
	// AddBehavior
	// Volatile Variables may also be able to be registered for everyone (such as now, time, me)

   // This defined value needs to be returned...
	return DekVersion;
}

//-----------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	// UnregisterRoutine
   // UnregisterDevice
}


//-----------------------------------------------------------------------------
//
//  TRAILER Banner goes here...
//
