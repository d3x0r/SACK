#include <malloc.h>

typedef int (__cdecl*fpointer)( long, int );


int main( void )
{
	int (__cdecl**list)(long,int);
	list =
		(int(**)(long,int))
		malloc(
				 (sizeof( int (__cdecl *)(long,int) )));
   list = (fpointer*)malloc( sizeof( fpointer) * 1 );
	return 0;
}
