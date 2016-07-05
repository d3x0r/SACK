#include <stdhdrs.h>

int main( void )
{
	int n, m;
	int total = 1;
	for( n = 1; n <= 100; n++ ) {
		printf( "start cardset_compare %d %%BASE_PATH%%\\%%BASE_NAME%%%d.dat \n", total, n);
		if( (total % 8) == 0 )
		{
			printf( "rundelay 12\n" );
		}
		total++;
	}
	for( n = 1; n <= 100; n++ ) {
		printf( "start cardset_compare %d %%OTHER_BASE_PATH%%\\%%OTHER_BASE_NAME%%%d.dat \n", total, n );
		if( (total % 8) == 0 )
		{
			printf( "rundelay 12\n" );
		}
		total++;
	}
	for( n = 1; n <= 100; n++ )
		for( m = 1; m <= 100; m++ ) {
			printf( "start cardset_compare %d %%BASE_PATH%%\\%%BASE_NAME%%%d.dat %%OTHER_BASE_PATH%%\\%%OTHER_BASE_NAME%%%d.dat\n", total, n, m );
			if( ( total % 8 ) == 0 )
			{
				printf( "rundelay 30\n" );
			}
			total++;
		}
}

