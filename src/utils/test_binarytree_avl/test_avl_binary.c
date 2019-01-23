#include <stdhdrs.h>
#include <salty_generator.h>

static void dumper( CPOINTER user, uintptr_t key ) {
	//lprintf( "KEY: %d %d", (int)user, (int)key );
}

#ifdef DEBUG_PERF
void DumpPerfStats( char *leader, PTREEROOT tree ) {
	{
		int *heights;
		int *swaps;
		int *maxScans;
		int *balanceFromleft;
		int *balanceFromRight;
		int n;
		GetTreePerf( tree, &heights, &swaps, &maxScans, &balanceFromleft, &balanceFromRight );
		printf( "%s: %d %d %d", leader, maxScans, balanceFromleft, balanceFromRight );
		for( n = 0; n < 30; n++ ) {
			printf( "  h  %d : %d\n", n, heights[n] );
		}
		for( n = 0; n < 10; n++ ) {
			printf( "  s  %d : %d\n", n, swaps[n] );
		}
	}

}
#endif

int main( void ) {
	PTREEROOT tree = CreateBinaryTree();
	int i;

//	__declspec(dllimport) void c( PTREEROOT root, int **heights, int **swaps, int *maxScans, int*bfl, int *bfr );

	for(  i = 0; i < 1000000; i++ ) {
		AddBinaryNode( tree, i, i );
	}
	AddBinaryNode( tree, i, i );
#ifdef DEBUG_PERF
	DumpPerfStats( "All Right", tree );
#endif
	DestroyBinaryTree( tree );

	tree = CreateBinaryTree();
	for( i = 1000000; i > 00; i-- ) {
		AddBinaryNode( tree, i, i );
	}
	AddBinaryNode( tree, i, i );
#ifdef DEBUG_PERF
	DumpPerfStats( "All left", tree );
#endif
	DestroyBinaryTree( tree );

	tree = CreateBinaryTree();
	for( i = 0; i < 1000000; i++ ) {
		AddBinaryNode( tree, rand(), rand() );
	}
	i = rand();
	AddBinaryNode( tree, i,i );
#ifdef DEBUG_PERF
	DumpPerfStats( "rand()", tree );
#endif
	DestroyBinaryTree( tree );

	{
		struct random_context *rng = SRG_CreateEntropy( NULL, 0 );
		tree = CreateBinaryTree();
		for( i = 0; i < 1000000; i++ ) {
			AddBinaryNode( tree, SRG_GetEntropy( rng, 30, TRUE ),SRG_GetEntropy( rng, 30, TRUE ) );
		}
		i = SRG_GetEntropy( rng, 30, TRUE );
		AddBinaryNode( tree, i, i );
#ifdef DEBUG_PERF
		DumpPerfStats( "SRG 1", tree );
#endif
		DestroyBinaryTree( tree );
	}

	{
		struct random_context *rng = SRG_CreateEntropy2( NULL, 0 );
		tree = CreateBinaryTree();
		for( i = 0; i < 1000000; i++ ) {
			AddBinaryNode( tree, SRG_GetEntropy( rng, 30, TRUE ), SRG_GetEntropy( rng, 30, TRUE ) );
		}
		i = SRG_GetEntropy( rng, 30, TRUE );
		AddBinaryNode( tree, i, i );
#ifdef DEBUG_PERF
		DumpPerfStats( "SRG 2", tree );
#endif
		DestroyBinaryTree( tree );
	}
	{
		struct random_context *rng = SRG_CreateEntropy3( NULL, 0 );
		tree = CreateBinaryTree();
		for( i = 0; i < 1000000; i++ ) {
			AddBinaryNode( tree, SRG_GetEntropy( rng, 30, TRUE ), SRG_GetEntropy( rng, 30, TRUE ) );
		}
		i = SRG_GetEntropy( rng, 30, TRUE );
		AddBinaryNode( tree, i, i );
#ifdef DEBUG_PERF
		DumpPerfStats( "SRG 3", tree );
#endif
		DestroyBinaryTree( tree );
	}

	return 0;
}

