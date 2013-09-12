#include <stdio.h>
#include <sharemem.h>

int main( void )
{
	FILE *file = fopen( "data.txt", "rb" );
	fclose( file );

	{
      size_t size;
      POINTER mem = OpenSpace( NULL, WIDE("data.txt"), &size );
	}
   return 0;
}

