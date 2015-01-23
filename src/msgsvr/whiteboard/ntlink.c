

#define PLUGIN_MODULE
#include <plugin.h>

// not sure we really need to do anything here
// other than invoke the deadstart to register the control, etc.

PUBLIC( char *, RegisterRoutines )( void )
{
	return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
}
