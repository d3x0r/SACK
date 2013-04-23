
#include <stdio.h>

int main( int argc, char **argv )
{
	FILE *in;
   FILE *out;
	if( argc < 4 )
	{
		printf( "Usage: %s <input filename> <header file name> <byte array name>\n", argv[0] );
	}

	in = fopen( argv[1], "rb" );
	out = fopen( argv[2], "wt" );
	if( out )
	{
		fprintf( out, "\n\n#include <sack_types.h>\n\n_8 %s[] = {\n", argv[3] );
	}
	else
      printf( "Failed to open %s for output\n", argv[2] );
	if( in )
	{
      int first = 1;
      int count = 0;
		int c;
		for( c = fgetc( in ); c != -1; c = fgetc(in ) )
		{
			fprintf( out, "%s0x%02X", first?"  ":", ", c );
			count++;
			first = 0;
			if( count == 16 )
			{
            count = 0;
				fprintf( out, "\n" );
			}
		}
		if( count )
			fprintf( out, "\n" );
		fprintf( out, "};\n" );
		fclose( in );
	}
	fclose( out );
   return 0;
}
