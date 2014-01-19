#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( InitRandom )
{
	srand( timeGetTime() );
}

#ifdef __WATCOMC__
PUBLIC( void, ExportThis)( void )
{
}
#endif
