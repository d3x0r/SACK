#include <stdhdrs.h>
#include <salty_generator.h>

SaneWinMain( argc, argv )
{
	lprintf( "%s", SRG_ID_Generator() );
	lprintf( "%s", SRG_ID_Generator() );
	lprintf( "%s", SRG_ID_Generator() );
	lprintf( "%s", SRG_ID_Generator() );
	lprintf( "%s", SRG_ID_Generator() );
	return 0;
}
EndSaneWinMain()
