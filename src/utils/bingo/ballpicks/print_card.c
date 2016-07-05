#include <stdhdrs.h>

POINTER OpenAFile( const char *file, PTRSZVAL *size )
{
	POINTER result = OpenSpace( NULL, file, size );
	return result;
}

struct params {
	P_8 p;
	PTRSZVAL size;
};

//0123456789abcdef01234567
//bbbbbiiiiinnnngggggooooo
//0 1 2 3 4 5 6 7 8 9 a b
int main( int argc, char **argv )
{
	if( argc > 2 )
	{
		int n;
		struct params pa;
		int card = atoi( argv[2] ) - 1;
		pa.size = 0;
		pa.p = (P_8)OpenAFile( argv[1], &pa.size );
		if( pa.p ) {
			printf( "%2d %2d %2d %2d %2d\n"
				, (pa.p[12 * card + 0] >> 4)
				, (pa.p[12 * card + 2] & 0xF) + 15
				, (pa.p[12 * card + 5] >> 4) + 30
				, (pa.p[12 * card + 7] >> 4) + 45
				, (pa.p[12 * card + 9] & 0xF) + 60
				);
			printf( "%2d %2d %2d %2d %2d\n"
				, (pa.p[12 * card + 0] & 0xF)
				, (pa.p[12 * card + 3] >> 4) + 15
				, (pa.p[12 * card + 5] & 0xF) + 30
				, (pa.p[12 * card + 7] & 0xF) + 45
				, (pa.p[12 * card + 10] >> 4) + 60
				);
			printf( "%2d %2d    %2d %2d\n"
				, (pa.p[12 * card + 1] >> 4)
				, (pa.p[12 * card + 3] & 0xF) + 15
				//, ( pa.p[12*card+5] & 0xF ) + 30
				, (pa.p[12 * card + 8] >> 4) + 45
				, (pa.p[12 * card + 10] & 0xF) + 60
				);
			printf( "%2d %2d %2d %2d %2d\n"
				, (pa.p[12 * card + 1] & 0xF)
				, (pa.p[12 * card + 4] >> 4) + 15
				, (pa.p[12 * card + 6] >> 4) + 30
				, (pa.p[12 * card + 8] & 0xF) + 45
				, (pa.p[12 * card + 11] >> 4) + 60
				);
			printf( "%2d %2d %2d %2d %2d\n"
				, (pa.p[12 * card + 2] >> 4)
				, (pa.p[12 * card + 4] & 0xF) + 15
				, (pa.p[12 * card + 6] & 0xF) + 30
				, (pa.p[12 * card + 9] >> 4) + 45
				, (pa.p[12 * card + 11] & 0xF) + 60
				);
		}
		else
			printf( "Failed to open %s", argv[1] );
	}
	return 0;
}
