
#include <stdio.h>
void g( void )
{
	extern void h( void );
	h();
   printf( "g()" );
}
