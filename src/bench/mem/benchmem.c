#include <stdhdrs.h>

void *blocks[100000];

int main( void )
{
	int n;
	int m;
	for( m = 0; m < 100; m++ )
	{
   lprintf( "malloc" );
	for( n = 0; n < 100000; n++ )
	{
      blocks[n] = malloc( 100 );
	}
   lprintf( "free" );
	for( n = 0; n < 100000; n++ )
	{
      free( blocks[n] );
	}
   lprintf( "malloc" );
	for( n = 0; n < 100000; n++ )
	{
      blocks[n] = malloc( 100 );
	}
   lprintf( "free" );
	for( n = 0; n < 100000; n+=2 )
	{
      free( blocks[n] );
	}
	for( n = 1; n < 100000; n+=2 )
	{
      free( blocks[n] );
	}
	}

	lprintf( "Allocate" );
	for( m = 0; m < 1000; m++ )
	{
		for( n = 0; n < 100000; n++ )
		{
			blocks[n] = Allocate( 100 );
		}
		for( n = 0; n < 100000; n++ )
		{
			Release( blocks[n] );
		}
	}
	lprintf( "Release" );

	lprintf( "Complete." );

}

