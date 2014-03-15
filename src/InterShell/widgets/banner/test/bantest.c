
#include <../../include/banner.h>


int main( void )
{
	char msg[256];
	int n;
	for( n = 0; n < 100000; n++ )
	{
		snprintf( msg, sizeof( msg ), "iteration: %d\ntime?\nDate?", n );
      BannerNoWait( msg );
	}
   return 0;
}
