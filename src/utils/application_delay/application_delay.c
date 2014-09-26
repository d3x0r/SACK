#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( loginfo )
{
   MessageBox( NULL, WIDE("Press OK to continue"), WIDE("Pause..."), MB_OK );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 ) || defined( __WATCOMC__ )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
