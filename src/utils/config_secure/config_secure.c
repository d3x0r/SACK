
#include <stdhdrs.h>
#include <network.h>
#include <filesys.h>
#include <salty_generator.h>

SaneWinMain( argc, argv )
{
	if( argc > 1 )
	{
		TEXTSTR result;
		P_8 buf;
		size_t length;
		FILE *file = sack_fopen( 0, argv[1], "rb" );
		length = sack_fseek( file, 0, SEEK_END );
		buf = NewArray( _8, length );
		sack_fseek( file, 0, SEEK_SET );
		sack_fread( buf, 1, length, file );
		sack_fclose( file );
		result = SRG_EncryptData( buf, length );
		printf( "%s", result );
		{
			size_t testlen;
			P_8 testbuf;
			SRG_DecryptData( result, &testbuf, &testlen );
			if( testlen != length )
				printf( "\n length fail \n" );
			if( MemCmp( testbuf, buf, testlen ) )
				printf( "\nFAIL\n" );
			Release( testbuf );
		}
		Release( buf );
		Release( result );

	}
}
EndSaneWinMain(  )
