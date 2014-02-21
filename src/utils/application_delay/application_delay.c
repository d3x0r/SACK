#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( loginfo )
{
   MessageBox( NULL, WIDE("Press OK to continue"), WIDE("Pause..."), MB_OK );
}

#if ( __WATCOMC__ < 2001 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
