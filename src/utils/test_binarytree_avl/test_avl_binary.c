#include <stdhdrs.h>


static void dumper( CPOINTER user, uintptr_t key ) {
	lprintf( "KEY: %d %d", (int)user, (int)key );
}

int main( void ) {
	PTREEROOT tree = CreateBinaryTree();
	for( int i = 0; i < 1000000; i++ ) {
		AddBinaryNode( tree, i, i );
	}
   DumpTree( tree, dumper );
}

