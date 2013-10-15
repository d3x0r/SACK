
#include <stdhdrs.h>


int main( void )
{

	PLIST list = NULL;
	POINTER p1 = (POINTER)0x1234;
	POINTER p2 = (POINTER)0x5678;

   AddLink( &list, p1 );
	AddLink( &list, p2 );
	DeleteLink( &list, p1 );

	{
		INDEX idx;
		POINTER p;
		LIST_FORALL( list, idx, POINTER, p )
			printf( "links in list: %p", p );
	}

}
