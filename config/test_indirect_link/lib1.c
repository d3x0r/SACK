
#include <stdio.h>
void f( void )
{
	extern void g( void );
	g();
   printf( "f()" );
}
