#include "sack_ucb_typelib.h"

int dumpCallback( CPOINTER data, uintptr_t key ) {
	printf( "Tree: %d\n", key );
}


int main() {
   char *p[15];
	PTREEROOT tree = CreateBinaryTree();
   AddBinaryNode( tree, p[7]="test7", 7 );
   AddBinaryNode( tree, p[3]="test6", 3 );
   AddBinaryNode( tree, p[10]="test8", 10 );
   AddBinaryNode( tree, p[1]="test1", 1 );
   AddBinaryNode( tree, p[2]="test2", 2 );
   AddBinaryNode( tree, p[8]="test8", 8 );
   AddBinaryNode( tree, p[12]="test8", 12 );
	AddBinaryNode( tree, p[13]="test8", 13 );

printf( "---------------- original tree\n" );
   DumpInOrder( tree, dumpCallback );

// remove a node that has one leaf
   RemoveBinaryNode( tree, p[12], 12 );

printf( "----------------removed 12\n" );
   DumpInOrder( tree, dumpCallback );

   AddBinaryNode( tree, p[12], 12 );

printf( "----------------added 12\n" );
   DumpInOrder( tree, dumpCallback );

// remove a node that has no leaves
   RemoveBinaryNode( tree, p[8], 8 );
printf( "----------------removed 8\n" );
   DumpInOrder( tree, dumpCallback );


   AddBinaryNode( tree, p[8], 8 );
printf( "----------------added 8\n" );
   DumpInOrder( tree, dumpCallback );


// remove a node that has leaves with full branches
   RemoveBinaryNode( tree, p[10], 10 );
printf( "----------------removed 10\n" );
   DumpInOrder( tree, dumpCallback );

   AddBinaryNode( tree, p[10], 10 );
printf( "----------------added 10\n" );
   DumpInOrder( tree, dumpCallback );


// remove a node that has leaves with full branches
   RemoveBinaryNode( tree, p[10], 7 );
printf( "----------------removed 10\n" );
   DumpInOrder( tree, dumpCallback );

   AddBinaryNode( tree, p[10], 7 );
printf( "----------------added 10\n" );
   DumpInOrder( tree, dumpCallback );


}

