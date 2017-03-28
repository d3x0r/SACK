
#include <stdhdrs.h>
#include <network.h>
#include <filesys.h>
#include <salty_generator.h>

SaneWinMain( argc, argv )
{
	if( argc > 1 )
	{
		TEXTSTR result;
		uint8_t* buf;
		size_t length;
		FILE *file = sack_fopen( 0, argv[1], "rb" );
		length = sack_fseek( file, 0, SEEK_END );
		buf = NewArray( uint8_t, length );
		sack_fseek( file, 0, SEEK_SET );
		sack_fread( buf, length, 1, file );
		sack_fclose( file );
		result = SRG_EncryptData( buf, length );
		{
			int wrote = 0;
			while( result[wrote] )
			{
				wrote += printf( "%.80s", result + wrote );
				if( result[wrote] )
					printf("\\\n" );
			}
		}
		{
			size_t testlen;
			uint8_t* testbuf;
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
