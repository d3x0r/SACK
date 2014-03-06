#include <stdio.h>


int main( void )
{
#define size(n) printf( "sizeof "#n" is %ld\n", sizeof( n ) )
   size(int);
   size(short);
   size(char);
   size(long long);
	size(long);
   return 0;
}

