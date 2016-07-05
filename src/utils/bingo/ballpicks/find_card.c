#include <stdhdrs.h>
#include <filesys.h>

int totalNums = 0;
int nums[25];

POINTER OpenAFile( const char *file, PTRSZVAL *size )
{
	POINTER result = OpenSpace( NULL, file, size );
	return result;
}

struct params {
	P_8 p;
	PTRSZVAL size;
};


void CPROC ProcessFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	struct params pa;
	pa.size = 0;
	pa.p = (P_8)OpenAFile( name, &pa.size );
	if( pa.p ) {
      int cardOfs = 0;
		int r = 0;
		int c = 0;
		int n = 0;
		do
		{
			for( n = 0; n < totalNums; n++ )
				if( ( ( pa.p[cardOfs + (n/2)] >> (( ~n & 1 )*4) ) & 0xF ) != nums[n] )
					break;
			if( n == totalNums ) {
				printf( "Found at card %s : %d\n", name, ( cardOfs / 12 ) + 1 );
				//break;
			}
			cardOfs += 12;
		}while( cardOfs < pa.size );
	}

}

//0123456789abcdef01234567
//bbbbbiiiiinnnngggggooooo
//0 1 2 3 4 5 6 7 8 9 a b
int main( int argc, char **argv )
{
	if( argc > 2 )
	{
		int n;
      void *info = NULL;
		struct params pa;
		for( n = 2; n < argc; n++ ){
			nums[totalNums] = atoi( argv[n] );
			if( ( nums[totalNums] > ( 15 * ( (totalNums)/5 +1 ) ) )
				|| ( nums[totalNums] <= ( 15 * ((totalNums)/5)-1 ) ) )
			{
				printf( "Invalid number to search for entered... %s", argv[n] );
            return 0;
			}
			nums[totalNums] -= ((totalNums)/5) * 15;
			totalNums++;
		}
		pa.size = 0;
		while( ScanFiles( NULL, argv[1], &info, ProcessFile, 0, 0 ) );

	}
	else
		printf( "usage: %s [card file mask]  [number1 number2 number3 number4.....]\n" );
	return 0;
}
