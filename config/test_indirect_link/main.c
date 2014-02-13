
#include <stdio.h>
int main()
{
	extern void f( void );
	extern void h( void );
	f();
   h();
	return 1;
}
