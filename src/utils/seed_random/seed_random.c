#include <stdhdrs.h>
#include <deadstart.h>

PRELOAD( InitRandom )
{
	srand( timeGetTime() );
}

