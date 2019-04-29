#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( loginfo )
{
   MessageBox( NULL, "Press OK to continue", "Pause...", MB_OK );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 ) || defined( __WATCOMC__ )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
