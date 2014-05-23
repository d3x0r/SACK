
#include <stdhdrs.h>
#include <network.h>

#include <salty_generator.h>

SaneWinMain( argc, argv )
{
	if( argc > 1 )
	{
      CTEXTSTR result;
      P_8 buf;
      size_t length;
		FILE *file = sack_fopen( 0, argv[1], "rb" );
		length = sack_fseek( file, 0, SEEK_END );
      buf = NewArray( _8, length );
		sack_fseek( file, 0, SEEK_SET );
		fread( buf, 1, length, file );
		fclose( file );
		result = SRG_EncryptData( buf, length );
		Release( buf );
      printf( "%s", buf );

	}
}
EndSaneWinMain(  )
