
#include <stdhdrs.h>

int main( void )

{
   int n;
	PTREEROOT tree = CreateBinaryTree();
	for( n = 0; n < 1000000; n++ )
	{
		AddBinaryNode( tree, n, n );
      if( ( n % 1000 ) == 999 )
			BalanceBinaryTree( tree );
		if( ( n % 10000 ) == 0 )
		{
         //DumpBinaryTree( tree );
         fprintf( stderr, "tick.\n" );
		}
	}
   return 0;
}
