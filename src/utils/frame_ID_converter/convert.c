
#include <stdhdrs.h>
#include <psi.h>


SaneWinMain( argc, argv )
{
	for( ; argc > 1; argc--, argv++ )
	{
		PSI_CONTROL frame = LoadXMLFrame( argv[1] );
		if( frame )
			SaveXMLFrame( frame, argv[1] );
	}
	return 0;
}
EndSaneWinMain()
