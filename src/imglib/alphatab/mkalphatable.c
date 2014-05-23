

#include <stdio.h>

#define WIDE(s) s
	
int main( void )
{
	int original, newalpha, val;
	printf( WIDE("unsigned char AlphaTable[256][256] = { \n"));
	for( original = 0; original < 256; original++ )
	{
		printf( WIDE("    %c "), (!original)?' ':',' );
		for( newalpha = 0; newalpha < 256; newalpha++ )
		{
			
			//val = ( ( 0x100 * original ) + ( ( 0x100 - original ) * newalpha ) ) / 0x100;
			val = ( (newalpha+1) * (original) ) / 0x100;
			printf( WIDE("%c %3d"), (!newalpha)?'{':',', val );
		}
		printf( WIDE(" }\n") );
	}
	printf( WIDE("};\n") );
	return 0;
}
// $Log: $
