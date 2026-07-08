/*
 * RemoveBinaryNode regression / stress test.
 *
 * test_avl_binary.c only ever inserts; removal went untested and hid a
 * corruption bug: NativeRemoveBinaryNode's single-child promotion did not
 * update the promoted child's 'me' back-pointer, leaving it aimed into the
 * freed (and usually recycled) node.  The next rotation involving that child
 * wrote its relink through the stale pointer, detaching a subtree; the
 * damage surfaced later as the (*(int*)0)=0 integrity trap inside a
 * subsequent RemoveBinaryNode.
 *
 * Sections:
 *   1. deterministic regressions for that exact shape (both mirror images)
 *   2. duplicate-rejection accounting under BT_OPT_NODUPLICATES
 *   3. systematic build/dismantle sweeps in several removal orders
 *   4. randomized churn against a shadow bitmap with destructor accounting
 *
 * Any linkage corruption shows up either as a membership CHECK failure
 * (a key that should be reachable no longer is) or as the integrity trap
 * crashing the test - both count as a failed run.
 */
#include <stdhdrs.h>

static int destroyCount;
static int failures;

static void CPROC destroyCounter( CPOINTER user, uintptr_t key )
{
	(void)user; (void)key;
	destroyCount++;
}

#define CHECK( cond, ... ) do { if( !(cond) ) { failures++;            \
		fprintf( stderr, "FAIL %s(%d): ", __FILE__, __LINE__ );          \
		fprintf( stderr, __VA_ARGS__ ); fprintf( stderr, "\n" ); } } while(0)

// keys are stored 1:1 as userdata and are always >= 1, so a NULL result
// from FindInBinaryTree always means 'not present'.
static void expectPresent( PTREEROOT tree, uintptr_t key, int present )
{
	CPOINTER found = FindInBinaryTree( tree, key );
	if( present )
		CHECK( (uintptr_t)found == key, "key %d should be found (got %p)", (int)key, found );
	else
		CHECK( found == NULL, "key %d should be gone (got %p)", (int)key, found );
}

//---------------------------------------------------------------------------
// 1a. remove a node whose only child hangs on its greater side, then force
//     a rotation through the promoted child.  With the stale 'me' pointer
//     the rotation detaches the subtree and keys 5/6 become unreachable.
static void testSingleChildPromotionRight( void )
{
	PTREEROOT tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, NULL, destroyCounter );
	uintptr_t k;
	destroyCount = 0;
	for( k = 1; k <= 4; k++ )      // 3 ends up holding the single child 4
		AddBinaryNode( tree, (POINTER)k, k );
	RemoveBinaryNode( tree, (POINTER)(uintptr_t)3, 3 );  // single-child promotion
	AddBinaryNode( tree, (POINTER)(uintptr_t)5, 5 );
	AddBinaryNode( tree, (POINTER)(uintptr_t)6, 6 );     // rotation at the promoted node
	expectPresent( tree, 1, TRUE );
	expectPresent( tree, 2, TRUE );
	expectPresent( tree, 3, FALSE );
	expectPresent( tree, 4, TRUE );
	expectPresent( tree, 5, TRUE );
	expectPresent( tree, 6, TRUE );
	// dismantling through the (formerly) damaged linkage is what trapped
	for( k = 1; k <= 6; k++ )
		if( k != 3 )
			RemoveBinaryNode( tree, (POINTER)k, k );
	CHECK( destroyCount == 6, "expected 6 destroys, got %d", destroyCount );
	DestroyBinaryTree( tree );
}

// 1b. mirror image: only child on the lesser side, rotation to the right.
static void testSingleChildPromotionLeft( void )
{
	PTREEROOT tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, NULL, destroyCounter );
	uintptr_t k;
	destroyCount = 0;
	for( k = 6; k >= 3; k-- )      // 4 ends up holding the single child 3
		AddBinaryNode( tree, (POINTER)k, k );
	RemoveBinaryNode( tree, (POINTER)(uintptr_t)4, 4 );
	AddBinaryNode( tree, (POINTER)(uintptr_t)2, 2 );
	AddBinaryNode( tree, (POINTER)(uintptr_t)1, 1 );
	expectPresent( tree, 1, TRUE );
	expectPresent( tree, 2, TRUE );
	expectPresent( tree, 3, TRUE );
	expectPresent( tree, 4, FALSE );
	expectPresent( tree, 5, TRUE );
	expectPresent( tree, 6, TRUE );
	for( k = 1; k <= 6; k++ )
		if( k != 4 )
			RemoveBinaryNode( tree, (POINTER)k, k );
	CHECK( destroyCount == 6, "expected 6 destroys, got %d", destroyCount );
	DestroyBinaryTree( tree );
}

//---------------------------------------------------------------------------
// 2. BT_OPT_NODUPLICATES must reject a re-add without touching the stored
//    node and without invoking the destructor for the rejected attempt.
static void testDuplicateRejection( void )
{
	PTREEROOT tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, NULL, destroyCounter );
	destroyCount = 0;
	CHECK( AddBinaryNode( tree, (POINTER)(uintptr_t)7, 7 ), "first insert rejected" );
	CHECK( !AddBinaryNode( tree, (POINTER)(uintptr_t)7, 7 ), "duplicate accepted" );
	CHECK( destroyCount == 0, "destructor ran on duplicate rejection" );
	expectPresent( tree, 7, TRUE );
	RemoveBinaryNode( tree, (POINTER)(uintptr_t)7, 7 );
	expectPresent( tree, 7, FALSE );
	CHECK( destroyCount == 1, "expected 1 destroy, got %d", destroyCount );
	DestroyBinaryTree( tree );
}

//---------------------------------------------------------------------------
// 3. for every size up to 64, build 1..N and dismantle it in three orders,
//    verifying full membership after every single removal.  This walks the
//    remover through every leaf / single-child / two-child shape the AVL
//    balancer can produce at these sizes.
static void testSweep( void )
{
	int N, k, j, order;
	for( N = 1; N <= 64; N++ ) {
		for( order = 0; order < 3; order++ ) {
			PTREEROOT tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, NULL, destroyCounter );
			char present[65];
			destroyCount = 0;
			for( k = 1; k <= N; k++ ) {
				AddBinaryNode( tree, (POINTER)(uintptr_t)k, (uintptr_t)k );
				present[k] = 1;
			}
			for( k = 1; k <= N; k++ ) {
				int r = ( order == 0 ) ? k                     // ascending
				      : ( order == 1 ) ? ( N + 1 - k )         // descending
				      : ( ( N / 2 + k ) % N ) + 1;             // rotated start
				RemoveBinaryNode( tree, (POINTER)(uintptr_t)r, (uintptr_t)r );
				present[r] = 0;
				for( j = 1; j <= N; j++ )
					expectPresent( tree, (uintptr_t)j, present[j] );
			}
			CHECK( destroyCount == N, "N=%d order=%d: %d destroys", N, order, destroyCount );
			DestroyBinaryTree( tree );
		}
	}
}

//---------------------------------------------------------------------------
// 4. randomized insert/remove churn against a shadow bitmap.  Deterministic
//    seed so failures reproduce.  Periodic full-range membership sweeps
//    catch detached subtrees; destructor accounting catches double frees
//    and leaks of tree nodes.
#define CHURN_RANGE 2048
#define CHURN_OPS   200000

static uint32_t lcg_state = 0x5eed5eed;
static uint32_t lcg( void )
{
	lcg_state = lcg_state * 1664525 + 1013904223;
	return lcg_state >> 8;
}

static void testChurn( void )
{
	static char present[CHURN_RANGE + 1];
	PTREEROOT tree = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, NULL, destroyCounter );
	int op, added = 0, removed = 0, j;
	destroyCount = 0;
	memset( present, 0, sizeof( present ) );
	for( op = 0; op < CHURN_OPS; op++ ) {
		uintptr_t key = ( lcg() % CHURN_RANGE ) + 1;
		if( present[key] ) {
			RemoveBinaryNode( tree, (POINTER)key, key );
			present[key] = 0;
			removed++;
			expectPresent( tree, key, FALSE );
		} else {
			CHECK( AddBinaryNode( tree, (POINTER)key, key ), "insert %d rejected", (int)key );
			present[key] = 1;
			added++;
			expectPresent( tree, key, TRUE );
		}
		if( ( op % 5000 ) == 4999 )
			for( j = 1; j <= CHURN_RANGE; j++ )
				expectPresent( tree, (uintptr_t)j, present[j] );
	}
	CHECK( destroyCount == removed, "destructor ran %d times for %d removals", destroyCount, removed );
	DestroyBinaryTree( tree );
	CHECK( destroyCount == added, "after destroy: %d destroys for %d inserts", destroyCount, added );
}

//---------------------------------------------------------------------------

int main( void )
{
	testSingleChildPromotionRight();
	testSingleChildPromotionLeft();
	testDuplicateRejection();
	testSweep();
	testChurn();
	if( failures ) {
		fprintf( stderr, "%d FAILURES\n", failures );
		return 1;
	}
	printf( "binary tree remove tests OK\n" );
	return 0;
}
