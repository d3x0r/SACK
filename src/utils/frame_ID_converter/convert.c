
#include <stdhdrs.h>
#include <psi.h>


SaneWinMain( argc, argv )
{
	for( ; argc > 1; argc--, argv++ )
	{
		PSI_CONTROL frame = LoadXMLFrameOverOption( NULL, argv[1], 0 );
		if( frame )
			SaveXMLFrame( frame, argv[1] );
	}
	return 0;
}
EndSaneWinMain()
