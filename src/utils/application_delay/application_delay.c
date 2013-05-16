#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( loginfo )
{
   MessageBox( NULL, "Press OK to continue", "Pause...", MB_OK );
}

#if ( __WATCOMC__ < 1291 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
