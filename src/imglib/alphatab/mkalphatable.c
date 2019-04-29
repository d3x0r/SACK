

#include <stdio.h>

	
int main( void )
{
	int original, newalpha, val;
	printf( "unsigned char AlphaTable[256][256] = { \n");
	for( original = 0; original < 256; original++ )
	{
		printf( "    %c ", (!original)?' ':',' );
		for( newalpha = 0; newalpha < 256; newalpha++ )
		{
			
			//val = ( ( 0x100 * original ) + ( ( 0x100 - original ) * newalpha ) ) / 0x100;
			val = ( (newalpha+1) * (original) ) / 0x100;
			printf( "%c %3d", (!newalpha)?'{':',', val );
		}
		printf( " }\n" );
	}
	printf( "};\n" );
	return 0;
}
// $Log: $
