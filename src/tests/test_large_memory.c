
#include <sharemem.h>

int main( void )
{
	POINTER p;
	int n;
	for( n = 0; n < 400; n++ )
		p = Allocate( 1000000 );
	while( 1 )
      WakeableSleep( 500 );
}

