/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   A binary tree container storing a user pointer blob of some user defined structure
 *   and a uintptr_t key which is used to check for content matchin.
 *   Binary tree has algorithms to become balanced, if the input is known to be weighted,
 *   or if statistics are pulled that indicate that the tree should be balanced, this
 *   function is available on demand.  Also searching through the tree using
 *   Least, Greatest, lesser, and greater is available.
 *
 * see also - include/typelib.h
 *
 */
#include <stdhdrs.h>

//#define DEFINE_BINARYLIST_PERF_COUNTERS
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
#  include <deadstart.h>
#endif
//#include <sack_types.h>
//#include <sharemem.h>
//#include <logging.h>

//#define DEBUG_STEPPING
#ifdef __cplusplus
namespace sack {
	namespace containers {
		namespace BinaryTree {
		using namespace sack::memory;
		using namespace sack::logging;
	
#endif
// consider slab allocation... 32 bytes even.
struct treenode_tag {
	struct {
		BIT_FIELD bUsed:1;
		BIT_FIELD bRoot:1;
	} flags;
	int depth;
	int children;  // required to know how many nodes are in the tree; especially with branch transplants.
	CPOINTER userdata;
	uintptr_t key;
	struct treenode_tag *lesser;
	struct treenode_tag *greater;
	struct treenode_tag **me;
	struct treenode_tag *parent;
};
typedef struct treenode_tag TREENODE;
	
#define MAXTREENODESPERSET 4096
DeclareSet( TREENODE );

typedef struct treeroot_tag {
	struct {
		BIT_FIELD bUsed:1;
		BIT_FIELD bRoot:1;
		BIT_FIELD bShadow:1; // tree points to the real TREEROOT (not a node)
		BIT_FIELD bNoDuplicate : 1;
	} flags;
	int depth;
	int children;
	uint32_t lock;
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
	int maxHeights[30];
	int maxSwaps[10];
	int maxScans;
	int balancedFromLeft;
	int balancedFromRight;
#endif
	GenericDestroy Destroy;
	GenericCompare Compare;
	PTREENODE tree;
	PTREENODE prior, current, lastfound;
} TREEROOT;

static PTREENODESET TreeNodeSet;

//---------------------------------------------------------------------------

#define MAXTREEROOTSPERSET 128
DeclareSet( TREEROOT );
static PTREEROOTSET treepool;


CPOINTER GetLesserNodeExx( PTREEROOT root, PTREENODE *from );
CPOINTER GetGreaterNodeExx( PTREEROOT root, PTREENODE *from );

//---------------------------------------------------------------------------

PTREEROOT FindTreeRoot( PTREENODE node )
{
	while( node && (!node->flags.bRoot) && node->parent )
	{
		node = node->parent;
	}
	return (PTREEROOT)node;
}

//---------------------------------------------------------------------------

int CPROC BinaryCompareInt( uintptr_t old, uintptr_t new_key )
{
	if( old > new_key )
		return 1;
	else if( old < new_key )
		return -1;
	return 0;
}

//---------------------------------------------------------------------------

void BalanceBinaryTree( PTREEROOT root )
{
#if SACK_BINARYLIST_USE_CHILD_COUNTS
	while( LockedExchange( &root->lock, 1 ) )
		Relinquish();
	while( BalanceBinaryBranch( root->tree ) > 1 && 0);
	root->lock = 0;
#endif
	//Log( "=========" );;
}

//---------------------------------------------------------------------------

#ifdef DEBUG_AVL_VALIDATION
//-------------------------------------------------------------------------- -
void ValidateTreeNode( PTREENODE node ) {
	if( node->parent && !node->parent->flags.bRoot ) {
		if( node->parent->children != ( ( node->parent->lesser ? ( node->parent->lesser->children + 1 ) : 0 )
			+ ( node->parent->greater ? ( node->parent->greater->children + 1 ) : 0 )
			) ) {
			lprintf( "child account is failed." );
			DebugBreak();
		}
		if( node->parent->depth <= node->depth ) {
			lprintf( "Depth tracking is failure." );
			DebugBreak();
		}
		else if( ( node->parent->depth - node->depth ) > 2 ) {
			lprintf( "Depth tracking is failure(2)." );
			DebugBreak();
		}
		if( node->parent->lesser != node && node->parent->greater != node ) {
			lprintf( "My parent is not pointing at me." );
			DebugBreak();
		}
	}
	if( node->lesser ) {
		if( node->lesser->parent != node ) {
			lprintf( "My Lesser does not point back to me as a parent." );
			DebugBreak();
		}
		ValidateTreeNode( node->lesser );
	}
	if( node->greater ) {
		if( node->greater->parent != node ) {
			lprintf( "My Greater does not point back to me as a parent." );
			DebugBreak();
		}
		ValidateTreeNode( node->greater );
	}

}
//-------------------------------------------------------------------------- -
void ValidateTree( PTREEROOT root ) {
	//lprintf( "--------------------------- VALIDATE TREE -----------------------------" );
	//DumpTree( root, NULL );
	ValidateTreeNode( root->tree );
}
#endif
//---------------------------------------------------------------------------

//static PTREENODE AVL_RotateToRight( PTREENODE node )
#define AVL_RotateToRight(node)                                          \
{                                                                        \
	PTREENODE left = node->lesser;                                   \
	PTREENODE T2 = left->greater;                                    \
/*lprintf( "RTR %p %p %p", node, left, T2 );     */                   \
	node->children -= (left->children + 1);                          \
                                                                         \
	node->me[0] = left;                                              \
	left->me = node->me;                                             \
	left->parent = node->parent;                                     \
                                                                         \
	/* Perform rotation*/                                            \
	left->greater = node;                                            \
	node->me = &left->greater;                                       \
	node->parent = left;                                             \
                                                                         \
	node->lesser = T2;                                               \
	if( T2 ) {                                                       \
		T2->me = &node->lesser;                                  \
		T2->parent = node;                                       \
		node->children += (T2->children + 1);         \
		left->children -= (T2->children + 1);         \
	}                                                                \
	left->children += (node->children + 1);                          \
                                                                         \
	/* Update heights */                                             \
	{                                                                \
		int leftDepth, rightDepth;                               \
		leftDepth = node->lesser ? node->lesser->depth : 0;      \
		rightDepth = node->greater ? node->greater->depth : 0;   \
		if( leftDepth > rightDepth )                             \
			node->depth = leftDepth + 1;                     \
		else                                                     \
			node->depth = rightDepth + 1;                    \
                                                                         \
		leftDepth = left->lesser ? left->lesser->depth : 0;      \
		rightDepth = left->greater ? left->greater->depth : 0;   \
		if( leftDepth > rightDepth ) {                           \
			left->depth = leftDepth + 1;                     \
		}                                                        \
		else                                                     \
			left->depth = rightDepth + 1;                    \
	}                                                                \
}

//---------------------------------------------------------------------------

//static PTREENODE AVL_RotateToLeft( PTREENODE node )
#define AVL_RotateToLeft(node)                                           \
{                                                                        \
	PTREENODE right = node->greater;                                 \
	PTREENODE T2 = right->lesser;                                    \
	/*lprintf( "RTL %p %p %p", node, right, T2 );  */                \
	node->children -= (right->children + 1);                         \
                                                                         \
	node->me[0] = right;                                             \
	right->me = node->me;                                            \
	right->parent = node->parent;                                    \
                                                                         \
	/* Perform rotation  */                                          \
	right->lesser = node;                                            \
	node->me = &right->lesser;                                       \
	node->parent = right;                                            \
	node->greater = T2;                                              \
	if( T2 ) {                                                       \
		T2->me = &node->greater;                                 \
		T2->parent = node;                                       \
		node->children += (T2->children + 1);         \
		right->children -= (T2->children + 1);        \
	}                                                                \
	right->children += (node->children + 1);                         \
	/*  Update heights */                                            \
	{                                                                \
		int left, rightDepth;                                    \
		left = node->lesser ? node->lesser->depth : 0;           \
		rightDepth = node->greater ? node->greater->depth : 0;   \
		if( left > rightDepth )                                  \
			node->depth = left + 1;                          \
		else                                                     \
			node->depth = rightDepth + 1;                    \
                                                                         \
		left = right->lesser ? right->lesser->depth : 0;         \
		rightDepth = right->greater ? right->greater->depth : 0; \
		if( left > rightDepth )                                  \
			right->depth = left + 1;                         \
		else                                                     \
			right->depth = rightDepth + 1;                   \
	}                                                                \
}                                                                        \

//---------------------------------------------------------------------------

#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
int zz;
PRIORITY_PRELOAD( InitReadyToLog, 999 ) {
	zz = 1;
}
#endif

static void AVLbalancer( PTREEROOT root, PTREENODE node ) {
	PTREENODE _x = NULL;
	PTREENODE _y = NULL;
	PTREENODE _z = NULL;
	PTREENODE tmp;
	int leftDepth;
	int rightDepth;
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
	int height = 0;
	int swaps = 0;
#endif
	_z = node;

	while( _z && !_z->flags.bRoot ) {
		int doBalance;
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
		height++;
#endif
		doBalance = FALSE;
		if( tmp = _z->greater )
			rightDepth = tmp->depth;
		else
			rightDepth = 0;
		if( tmp = _z->lesser )
			leftDepth = tmp->depth;
		else
			leftDepth = 0;

		if( leftDepth > rightDepth ) {
			if( (1 + leftDepth) == _z->depth ) {
				//if( zz )
				//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
				root->balancedFromLeft++;
#endif
				break;
			}
			_z->depth = 1 + leftDepth;
			if( (leftDepth -rightDepth) > 1 ) {
				doBalance = TRUE;
			}
		} else {
			if( (1 + rightDepth) == _z->depth ) {
				//if(zz)
				//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
				root->balancedFromRight++;
#endif
				break;
			}
			_z->depth = 1 + rightDepth;
			if( (rightDepth- leftDepth) > 1 ) {
				doBalance = TRUE;
			}
		}
		if( doBalance ) {
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
			swaps++;
#endif
			if( _x ) {
				if( _x == _y->lesser ) {
					if( _y == _z->lesser ) {
						// left/left
						AVL_RotateToRight( _z );
					}
					else {
						//left/rightDepth
						AVL_RotateToRight( _y );
						AVL_RotateToLeft( _z );
					}
				}
				else {
					if( _y == _z->lesser ) {
						AVL_RotateToLeft( _y );
						AVL_RotateToRight( _z );
						// rightDepth.left
					}
					else {
						//rightDepth/rightDepth
						AVL_RotateToLeft( _z );
					}
				}
			}
			else {
				//lprintf( "Not deep enough for balancing." );
			}
		}
		_x = _y;
		_y = _z;
		_z = _z->parent;
	}
#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
	if( !node->parent->flags.bRoot 
		&& node->parent->lesser != node 
		&& node->parent->greater != node ) {
		*(int*)0 = 0;
	}
	if( height < 31 )
		root->maxHeights[height]++;
	else
		root->maxHeights[0]++;
	if( swaps < 10 )
		root->maxSwaps[swaps]++;
	else
		root->maxSwaps[0]++;
	if( !_z )
		root->maxScans++;
#endif

}

//---------------------------------------------------------------------------


int HangBinaryNode( PTREEROOT root, PTREENODE node )
{
	PTREENODE check;
	if( !node )
		return 0;

	root->children += ( node->children + 1 );

	if( !(root->tree) )
	{
		root->tree = node;
		node->me = &root->tree;
		node->parent = (PTREENODE)root;
#ifdef DEBUG_AVL_VALIDATION
		ValidateTree( root );
#endif
		return 1;
	}
 	check = root->tree;
 	while( check )
 	{
 		int dir = root->Compare( node->key, check->key );
		check->children += (node->children + 1);
 		if( dir < 0 )
 		{
 			if( check->lesser )
 			{
	 			check = check->lesser;
	 		}
			else
			{
				check->lesser = node;
				node->me = &check->lesser;
				node->parent = check;
				break;
			}
 		}
 		else if( dir > 0 )
 			if( check->greater )
 			{
	 			check = check->greater;
	 		}
			else
			{
				check->greater = node;
				node->me = &check->greater;
				node->parent = check;
				break;
			}
		else if( root->flags.bNoDuplicate )
		{
			while( check && !check->flags.bRoot )
			{
				check->children -= (node->children + 1);
				check = check->parent;
			}
			if( check )
				check->children -= (node->children + 1);
			DeleteFromSet( TREENODE, TreeNodeSet, node );
			return 0;
		}
		else
		{
			// allow duplicates; but link in as a near node, either left
			// or right... depending on the depth.
			int leftdepth = 0, rightdepth = 0;
			if( check->lesser )
				leftdepth = check->lesser->depth;
			if( check->greater )
				rightdepth = check->greater->depth;
			if( leftdepth < rightdepth )
			{
				if( check->lesser )
					check = check->lesser;
				else
				{
					check->lesser = node;
					node->me = &check->lesser;
					node->parent = check;
					break;
				}
			}
			else
			{
				if( check->greater )
					check = check->greater;
				else
				{
					check->greater = node;
					node->me = &check->greater;
					node->parent = check;
					break;
				}
			}
		}
	}
	if( node->parent->lesser != node && node->parent->greater != node ) {
		*(int*)0 = 0;
	}
	AVLbalancer( root, node );
#ifdef DEBUG_AVL_VALIDATION
	ValidateTree( root );
#endif
	return 1;
}

//---------------------------------------------------------------------------

int AddBinaryNodeEx( PTREEROOT root
                   , CPOINTER userdata
                   , uintptr_t key DBG_PASS )
{
	PTREENODE node;
	if( !root )
		return 0;
	node = GetFromSet( TREENODE, &TreeNodeSet );//AllocateEx( sizeof( TREENODE ) DBG_RELAY );
	node->lesser = NULL;
	node->greater = NULL;
	node->me = NULL;
	node->children = 0;
	node->depth = 0;
	node->userdata = userdata;
	node->key = key;
	node->flags.bUsed = 1;
	node->flags.bRoot = 0;
	return HangBinaryNode( root, node );
}

#undef AddBinaryNode
int AddBinaryNode( PTREEROOT root
                 , CPOINTER userdata
                 , uintptr_t key )
{
	return AddBinaryNodeEx( root, userdata, key DBG_SRC );
}
//---------------------------------------------------------------------------


static void NativeRemoveBinaryNode( PTREEROOT root, PTREENODE node )
{
	if( root )
	{
		CPOINTER userdata = node->userdata;
		uintptr_t userkey = node->key;
		LOGICAL no_children = FALSE;
		// lprintf( "Removing node from tree.. %p under %p", node, node->parent );
		if( !node->parent->flags.bRoot
			&& node->parent->lesser != node
			&& node->parent->greater != node ) {
			*(int*)0=0;
		}
		PTREENODE least = NULL;
		PTREENODE backtrack;
		PTREENODE bottom;  // deepest node a change was made on.
		if( !node->lesser ) {
			if( node->greater ) {
				bottom = (*node->me) = node->greater;
				bottom->parent = node->parent;
			} else {
				(*node->me) = NULL;
				bottom = node;
				no_children = TRUE;
			}
		} else if( !node->greater ) {
			bottom = (*node->me) = node->lesser;
			bottom->parent = node->parent;
		} else {
			node->children--;

			bottom = node;
			// have a lesser and a greater.
			if( node->lesser->depth > node->greater->depth ) {
				least = node->lesser;
				while( least->greater ) { bottom = least; least = least->greater; }
				if( least->lesser ) {
					(*(least->lesser->me =least->me)) = least->lesser;
					least->lesser->parent  = least->parent;
				} else {
					(*(least->me)) = NULL;
				}
			} else {
				least = node->greater;
				while( least->lesser ) { bottom = least; least = least->lesser; }
				if( least->greater ) {
					(*(least->greater->me = least->me)) = least->greater;
					least->greater->parent  = least->parent;
				} else {
					(*(least->me)) = NULL;
				}
			}
		}
		{
			LOGICAL updating = 1;
			backtrack = bottom;
			do {
				backtrack = backtrack->parent;
				while( backtrack && ( no_children || backtrack != node ) ) {
					backtrack->children--;
					if( updating )
						if( backtrack->lesser )
							if( backtrack->greater ) {
								int tmp1, tmp2;
								PTREENODE z_, y_/*, x_*/;

								if( (tmp1=backtrack->lesser->depth) > (tmp2=backtrack->greater->depth) ) {
									if( backtrack->depth != ( tmp1 + 1 ) ) 
										backtrack->depth = tmp1 + 1;
									else 
										updating = 0;
									if( (tmp1-tmp2) > 1 ) {
										// unblanced here...
										int tmp3, tmp4;
										tmp3 = backtrack->lesser->lesser?backtrack->lesser->lesser->depth:0;
										tmp4 = backtrack->lesser->greater?backtrack->lesser->greater->depth:0;
										z_ = backtrack;
										y_ = backtrack->lesser;
										if( tmp3 > tmp4 ) {
											//x_ = backtrack->lesser->lesser; 
											// left-left Rotate Right(Z)
											AVL_RotateToRight( z_ );
										} else {
											// left-right
											//x_ = backtrack->lesser->greater; 
											AVL_RotateToLeft( y_ );
											AVL_RotateToRight( z_ );
										}
									}
								} else {
									if( backtrack->depth != ( tmp2 + 1 ) )
										backtrack->depth = tmp2 + 1;
									else 
										updating = 0;
									if( (tmp2-tmp1) > 1 ) {
										// unblanced here...
										int tmp3, tmp4;
										tmp3 = backtrack->greater->lesser?backtrack->greater->lesser->depth:0;
										tmp4 = backtrack->greater->greater?backtrack->greater->greater->depth:0;
										z_ = backtrack;
										y_ = backtrack->greater;
										if( tmp4 > tmp3 ) {
											//x_ = y_->greater; 
											// right-right Rotate Right(Z)
											AVL_RotateToLeft( y_ );
										} else {
											// right-left
											//x_ = y_->lesser; 
											AVL_RotateToRight( y_ );
											AVL_RotateToLeft( z_ );
										}
									}
								}
							} else
									if( backtrack->depth != ( backtrack->lesser->depth + 1 ) )
										backtrack->depth = backtrack->lesser->depth + 1;
									else 
										updating = 0;
						else
							if( backtrack->greater )
									if( backtrack->depth != ( backtrack->greater->depth + 1 ) )
										backtrack->depth = backtrack->greater->depth + 1;
									else 
										updating = 0;
							else
									if( backtrack->depth != 0 )
										backtrack->depth = 0;
									else 
										updating = 0;
					backtrack = backtrack->parent;
				}
				if( least ) {
					node->userdata = least->userdata;
					node->key      = least->key;
					DeleteFromSet( TREENODE, TreeNodeSet, least );
					node   = NULL;
					least  = NULL;
				}
			} while( backtrack );
		}
		
		AVLbalancer( root, bottom );

		if( root->Destroy )
			root->Destroy( userdata, userkey );

		if( node )
			DeleteFromSet( TREENODE, TreeNodeSet, node );
#ifdef DEBUG_AVL_VALIDATION
		ValidateTree( root );
#endif
		return;
	}
	lprintf( "Fatal RemoveBinaryNode could not find the root!" );
}

//---------------------------------------------------------------------------

 void  RemoveBinaryNode ( PTREEROOT root, POINTER data, uintptr_t key )
{
	PTREENODE node;
	if( !root )
		return;
	node = root->tree;
	while( node )
	{
		int dir = root->Compare( key, node->key );
		if( dir > 0 )
			node = node->greater;
		else if( dir < 0 )
			node = node->lesser;
		else
		{
			if( node->userdata == data )
			{
				NativeRemoveBinaryNode( root, node );
				break;
			}
			else
			{
				// hmm same key different data...
				break;
			}
		}
	}
	return ;
}

//---------------------------------------------------------------------------
void ResetBinaryTree( PTREEROOT root )
{
	while( root->tree )
		NativeRemoveBinaryNode( root, root->tree );
}
//---------------------------------------------------------------------------

static void DestroyBinaryTreeNode( PTREEROOT root, PTREENODE node )
{
	if( node ) {
		if( node->lesser )
			DestroyBinaryTreeNode( root, node->lesser );
		if( node->greater )
			DestroyBinaryTreeNode( root, node->greater );

		if( root->Destroy )
			root->Destroy( node->userdata, node->key );
		DeleteFromSet( TREENODE, TreeNodeSet, node );
	}
}

void DestroyBinaryTree( PTREEROOT root )
{
	DestroyBinaryTreeNode( root, root->tree );
	DeleteFromSet( TREEROOT, treepool, root );
}

//---------------------------------------------------------------------------

PTREEROOT CreateBinaryTreeExtended( uint32_t flags
									  , GenericCompare Compare
									  , GenericDestroy Destroy DBG_PASS )
{
	PTREEROOT root;
	root = GetFromSet( TREEROOT, &treepool );//(PTREEROOT)AllocateEx( sizeof( TREEROOT ) DBG_RELAY );
	MemSet( root, 0, sizeof( TREEROOT ) );
	root->flags.bRoot = 1;
	root->flags.bUsed = 1;
	if( flags & BT_OPT_NODUPLICATES  )
		root->flags.bNoDuplicate = 1;
	root->Destroy = Destroy;
	//root->return  = NULL; // upgoing... (return from rightDepth )
	if( Compare )
		root->Compare = Compare;
	else
		root->Compare = BinaryCompareInt;
	return root;
}

#undef CreateBinaryTreeEx
PTREEROOT CreateBinaryTreeEx( GenericCompare Compare
								    , GenericDestroy Destroy )
{
	return CreateBinaryTreeExx( 0, Compare, Destroy );
}

//---------------------------------------------------------------------------
int maxlevel = 0;
void DumpNode( PTREENODE node, int level, int (*DumpMethod)( CPOINTER user, uintptr_t key ) )
{
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
	static char buf[256];
#endif
	int print;
	if( !node )
		return;
	if( level > maxlevel )
		maxlevel = level;
	DumpNode( node->lesser, level+1, DumpMethod );
	if( DumpMethod )
		print = DumpMethod( node->userdata, node->key );
	else
		print = TRUE;
	//else
	if( print ) {
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
		snprintf( buf, 256, "[%3d] %p Node has %3d depth  %3" _32f " children (%p %3" _32f ",%p %3" _32f "). %10" _PTRSZVALfs
			, level, node, node->depth, node->children
			, node->lesser
			, (node->lesser) ? (node->lesser->children + 1) : 0
			, node->greater
			, (node->greater) ? (node->greater->children + 1) : 0
			, node->key
		);
		puts( buf );
#else
		lprintf( "[%3d] %p Node has %3d depth  %3" _32f " children (%p %3" _32f ",%p %3" _32f "). %10" _PTRSZVALfs
			, level, node, node->depth, node->children
			, node->lesser
			, (node->lesser) ? (node->lesser->children + 1) : 0
			, node->greater
			, (node->greater) ? (node->greater->children + 1) : 0
			, node->key
		);
#endif
	}
	DumpNode( node->greater, level+1, DumpMethod );
}

//---------------------------------------------------------------------------

#ifdef DEFINE_BINARYLIST_PERF_COUNTERS
PUBLIC( void, GetTreePerf )( PTREEROOT root, int **heights, int **swaps, int *maxScans, int*bfl, int *bfr ) {
	if( heights ) heights[0] = root->maxHeights;
	if( swaps ) swaps[0] = root->maxSwaps;
	if( maxScans ) maxScans[0] = root->maxScans;
	if( bfl ) bfl[0] = root->balancedFromLeft;
	if( bfr ) bfr[0] = root->balancedFromRight;
}
#endif

void DumpTree( PTREEROOT root
				 , int (*Dump)( CPOINTER user, uintptr_t key ) )
{
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
	static char buf[256];
	maxlevel = 0;
	if( !Dump ) {
		snprintf( buf, 256, "Tree %p has %" _32f " nodes. %p is root", root, root->children, root->tree );
		puts( buf );
	}
	DumpNode( root->tree, 1, Dump );
	if( !Dump ) {
		snprintf( buf, 256, "Tree had %d levels.", maxlevel );
		puts( buf );
	}
	fflush( stdout );
#else
	maxlevel = 0;
	if( !Dump ) {
		lprintf(  "Tree %p has %" _32f " nodes. %p is root", root, root->children, root->tree );
	}
	DumpNode( root->tree, 1, Dump );
	if( !Dump ) {
		lprintf( "Tree had %d levels.", maxlevel );
	}
#endif
}

//---------------------------------------------------------------------------

void DumpNodeInOrder( PLINKQUEUE *queue, int (*DumpMethod)( CPOINTER user, uintptr_t key ) )
{
	PTREENODE node;
	while( node = (PTREENODE)DequeLink( queue ) )
	{
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
	static char buf[256];
#endif
	int print;
	if( !node )
		return;
	if( node->lesser )
		EnqueLink( queue, node->lesser );
	if( node->greater )
		EnqueLink( queue, node->greater );
	if( DumpMethod )
		print = DumpMethod( node->userdata, node->key );
	else
		print = TRUE;
	//else
	if( print ) {
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
		snprintf( buf, 256, "[%3d] %p Node has %3d depth  %3" _32f " children (%p %3" _32f ",%p %3" _32f "). %10" _PTRSZVALfs
			, level, node, node->depth, node->children
			, node->lesser
			, (node->lesser) ? (node->lesser->children + 1) : 0
			, node->greater
			, (node->greater) ? (node->greater->children + 1) : 0
			, node->key
		);
		puts( buf );
#else
		lprintf( "%p Node has %3d depth  %3" _32f " children (%p %3" _32f ",%p %3" _32f "). %10" _PTRSZVALfs
			, node, node->depth, node->children
			, node->lesser
			, (node->lesser) ? (node->lesser->children + 1) : 0
			, node->greater
			, (node->greater) ? (node->greater->children + 1) : 0
			, node->key
		);
#endif
	}
	}
}

//---------------------------------------------------------------------------

void DumpInOrder( PTREEROOT root
				 , int (*Dump)( CPOINTER user, uintptr_t key ) )
{
	PLINKQUEUE plq = CreateLinkQueue();
	EnqueLink( &plq, root->tree );
#ifdef SACK_BINARYLIST_USE_PRIMITIVE_LOGGING
	static char buf[256];
	if( !Dump ) {
		snprintf( buf, 256, "Tree %p has %" _32f " nodes. %p is root", root, root->children, root->tree );
		puts( buf );
	}
	DumpNodeInOrder( &plq, root->tree, 1, Dump );
	fflush( stdout );
#else
	maxlevel = 0;
	if( !Dump ) {
		lprintf(  "Tree %p has %" _32f " nodes. %p is root", root, root->children, root->tree );
	}
	DumpNodeInOrder( &plq, Dump );
#endif
}

//---------------------------------------------------------------------------

CPOINTER FindInBinaryTree( PTREEROOT root, uintptr_t key )
{
	PTREENODE node;
	if( !root )
		return 0;
	node = root->tree;
	while( node )
	{
		int dir = root->Compare( key, node->key );
		if( dir > 0 )
			node = node->greater;
		else if( dir < 0 )
			node = node->lesser;
		else
			break;
	}
	root->lastfound = node;
	if( node )
		return node->userdata;
	return 0;
}

//---------------------------------------------------------------------------


int CPROC TextMatchLocate( uintptr_t key1, uintptr_t key2 )
{
	size_t k1len = StrLen( (CTEXTSTR)key1 );
	size_t k2len = StrLen( (CTEXTSTR)key2 );
	//lprintf( "COmpare %s(%d) vs %s(%d)", key1, k1len, key2, k2len );
	if( k2len < k1len )
	{
		// cannot match this.... but should
		// try to choose a direction
		int dir = StrCaseCmpEx( (CTEXTSTR)key1, (CTEXTSTR)key2, k2len );
		if( dir == 0 )
			return 101;
		if( dir > 0 )
			return 1;
		return -1;
	}
	else if( k2len > k1len )
	{
		int dir = StrCaseCmpEx( (CTEXTSTR)key1, (CTEXTSTR)key2, k1len );
		// is exact match, but only part of key2
		if( dir == 0 )
			return 100;

		// I doubt these will really matter...
		// could compute distance...
		if( dir > 0 )
			return 1;
		else
			return -1;
	}
	else
	{
		int dir = StrCaseCmp( (CTEXTSTR)key1, (CTEXTSTR)key2 );
		if( dir == 0 )
			return 0;
		if( dir > 0 )
			return 1;
		else
			return -1;
	}
}

// the key value passed does not have to be the same as the key in the tree
// it can be an abstrat reference of a strucutre that contains a key for the tree
// result of fuzzy routine is 0 = match.  100 = inexact match
// 1 = no match, actual may be larger
// -1 = no match, actual may be lesser
// 100 = inexact match- checks nodes near for better match.
CPOINTER LocateInBinaryTree( PTREEROOT root, uintptr_t key
                           , int (CPROC*fuzzy)( uintptr_t psv, uintptr_t node_key )
                           )
{
	PTREENODE node;
	node = root->tree;
	if( !fuzzy )
		fuzzy = TextMatchLocate;

	while( node )
	{
		int _dir;
		int dir = fuzzy( key, node->key );
		if( dir == 100 || dir == 101 )
		{
			PTREENODE one_up;
			PTREENODE one_down;
			// this matched, in an inexact length.
			// to be really careful we should match one up and one down.
			// well, we'll match better only if we had exact length
			// so - go up one node, until we find exact length
			//lprintf( " - Found a near match..." );
			one_up = node;
			one_down = node;
			_dir = dir;
			do
			{
				GetLesserNodeExx( root, &one_up );
				if( one_up )
				{
					dir = fuzzy( key, one_up->key );
					if( dir == 100 )
						continue;
					if( dir == 0 )
					{
						root->lastfound = one_up;
						return (one_up->userdata);
					}
					else
						one_up = NULL;
				}
				GetGreaterNodeExx( root, &one_down );
				if( one_down )
				{
					dir = fuzzy( key, one_down->key );
					if( dir == 100 )
						continue;
					if( dir == 0 )
					{
						root->lastfound = one_down;
						return (one_down->userdata);
					}
					else
						one_down = NULL;
				}
			}
			while( one_up || one_down );
			if( _dir == 101 )
			{
				node = NULL;
			}
			root->lastfound = node;
			if( node )
				return( node->userdata );
			return 0;
		}
		if( dir > 0 )
		{
			node = node->greater;
		}
		else if( dir < 0 )
		{
			node = node->lesser;
		}
		else
			break;
	}
	root->lastfound = node;
	if( node )
		return node->userdata;
	return 0;
}

//---------------------------------------------------------------------------

CPOINTER GetCurrentNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root || !(*cursor) )
		return NULL;
	return (*(struct treenode_tag **)cursor)->userdata;
}
CPOINTER GetCurrentNode( PTREEROOT root )
{
	return GetCurrentNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------

void RemoveLastFoundNode( PTREEROOT root )
{
	if( !root || !root->lastfound )
		return;
	NativeRemoveBinaryNode( root, root->lastfound );
}

//---------------------------------------------------------------------------

void RemoveCurrentNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root || !(*cursor) )
		return;
	NativeRemoveBinaryNode( root, (PTREENODE)(*cursor) );
	(*cursor) = NULL;
}

void RemoveCurrentNode( PTREEROOT root )
{
	RemoveCurrentNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------

CPOINTER GetGreaterNodeExx( PTREEROOT root, PTREENODE *from )
{
	if( !root || !(*from) ) return 0;

	if( !(*from)->greater && !(*from)->lesser )
	{
		// Up 1
		root->prior = (*from);
		(*from) = (*from)->parent;
		if( (*from)->flags.bRoot )
		{
			// Root - end
			(*from) = NULL;
			return 0;
		}
		while( root->prior == (*from)->greater )
		{
			// up 2
			root->prior = (*from);
			(*from) = (*from)->parent;
			if( (*from)->flags.bRoot )
			{
				// Root
				(*from) = NULL;
				return 0;
			}
		}
		// Do it
		return (*from)->userdata;	
	}

	if( (*from)->greater )
	{
		// right
		(*from) = (*from)->greater;
		while( (*from)->lesser )
		{
			// Left
			(*from) = (*from)->lesser;
		}
		// Do it 1
		return (*from)->userdata;
	}

	do
	{
		// Up 3
		root->prior = (*from);
		(*from) = (*from)->parent;
		if( (*from)->flags.bRoot )
		{
			// Root
			(*from) = NULL;
			return 0;
		}
	} while( (*from)->greater == root->prior );
	// Do it 2
	return (*from)->userdata;
}

CPOINTER GetGreaterNodeEx( PTREEROOT root, POINTER *cursor )
{
	return GetGreaterNodeExx( root, (PTREENODE*)cursor );
}
CPOINTER GetGreaterNode( PTREEROOT root )
{
	return GetGreaterNodeExx( root, &root->current );
}
//---------------------------------------------------------------------------

CPOINTER GetLesserNodeExx( PTREEROOT root, PTREENODE *from )
{
	if( !root || !(*from) ) return 0;

	if( !(*from)->lesser && !(*from)->greater )
	{
		// Up 1
		root->prior = (*from);
		(*from) = (*from)->parent;
		if( (*from)->flags.bRoot )
		{
			// Root - end
			(*from) = NULL;
			return 0;
		}
		while( root->prior == (*from)->lesser )
		{
			// up 2
			root->prior = (*from);
			(*from) = (*from)->parent;
			if( (*from)->flags.bRoot )
			{
				// Root
				(*from) = NULL;
				return 0;
			}
		}
		// Do it
		return (*from)->userdata;	
	}

	if( (*from)->lesser )
	{
		// right
		(*from) = (*from)->lesser;
		while( (*from)->greater )
		{
			// Left
			(*from) = (*from)->greater;
		}
		// Do it 1
		return (*from)->userdata;
	}

	do
	{
		// Up 3
		root->prior = (*from);
		(*from) = (*from)->parent;
		if( (*from)->flags.bRoot )
		{
			// Root
			(*from) = NULL;
			return 0;
		}
	} while( (*from)->lesser == root->prior );
	// Do it 2
	return (*from)->userdata;
}

CPOINTER GetLesserNodeEx( PTREEROOT root, POINTER *cursor )
{
	return GetLesserNodeExx( root, (PTREENODE*)cursor );
}
CPOINTER GetLesserNode( PTREEROOT root )
{
	return GetLesserNodeExx( root, &root->current );
}

//---------------------------------------------------------------------------

CPOINTER GetLeastNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root ) return 0;
	(*(struct treenode_tag **)cursor) = root->tree;
	root->prior = NULL;
	while( (*(struct treenode_tag **)cursor) && (*(struct treenode_tag **)cursor)->lesser )
		(*(struct treenode_tag **)cursor) = (*(struct treenode_tag **)cursor)->lesser;
	if( (*(struct treenode_tag **)cursor) )
		return (*(struct treenode_tag **)cursor)->userdata;
	return 0;
}

CPOINTER GetLeastNode( PTREEROOT root )
{
	return GetLeastNodeEx( root, (POINTER*)&root->current );
}
//---------------------------------------------------------------------------

CPOINTER GetGreatestNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root ) return 0;
	root->prior = NULL;
	(*(struct treenode_tag **)cursor) = root->tree;
	while( (*(struct treenode_tag **)cursor) && (*(struct treenode_tag **)cursor)->greater )
		(*(struct treenode_tag **)cursor) = (*(struct treenode_tag **)cursor)->greater;
	if( (*(struct treenode_tag **)cursor) )
		return (*(struct treenode_tag **)cursor)->userdata;
	return 0;
}
CPOINTER GetGreatestNode( PTREEROOT root )
{
	return GetGreatestNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------

CPOINTER GetRootNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root ) return 0;
	root->prior = NULL;
	(*(struct treenode_tag **)cursor) = root->tree;
	if( (*(struct treenode_tag **)cursor) )
		return (*(struct treenode_tag **)cursor)->userdata;
	return 0;
}
CPOINTER GetRootNode( PTREEROOT root )
{
	return GetRootNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------

CPOINTER GetParentNodeEx( PTREEROOT root, POINTER *cursor )
{
	if( !root ) return 0;
	if( (*(struct treenode_tag **)cursor) )
	{
		root->prior = (*(struct treenode_tag **)cursor);
		if( !(*(struct treenode_tag **)cursor)->parent->flags.bRoot )
			(*(struct treenode_tag **)cursor) = (*(struct treenode_tag **)cursor)->parent;
		if( (*(struct treenode_tag **)cursor) )
			return (*(struct treenode_tag **)cursor)->userdata;
	}
	return 0;
}
CPOINTER GetParentNode( PTREEROOT root )
{
	return GetParentNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------

CPOINTER GetChildNodeEx( PTREEROOT root, POINTER *cursor, int direction )
{
	if( !root ) return 0;
	if( (*(struct treenode_tag **)cursor) )
	{
		root->prior = (*(struct treenode_tag **)cursor);
		if( direction < 0 )
		{
			(*(struct treenode_tag **)cursor) = (*(struct treenode_tag **)cursor)->lesser;
		}
		else
			(*(struct treenode_tag **)cursor) = (*(struct treenode_tag **)cursor)->greater;
		if( (*(struct treenode_tag **)cursor) )
			return (*(struct treenode_tag **)cursor)->userdata;
	}
	return 0;
}


CPOINTER GetChildNode( PTREEROOT root, int direction )
{
	return GetChildNodeEx( root, (POINTER*)&root->current, direction );
}

//---------------------------------------------------------------------------

CPOINTER GetPriorNodeEx( PTREEROOT root, POINTER *cursor )
{
	PTREENODE cur;
	if( !root ) return 0;
	cur = (*(struct treenode_tag **)cursor);
	(*(struct treenode_tag **)cursor) = root->prior;
	root->prior = cur;
	if( (*(struct treenode_tag **)cursor) )
		return (*(struct treenode_tag **)cursor)->userdata;
	return 0;
}

CPOINTER GetPriorNode( PTREEROOT root )
{
	return GetPriorNodeEx( root, (POINTER*)&root->current );
}

//---------------------------------------------------------------------------
int GetNodeCount( PTREEROOT root )
{
	return root->children;
}

//---------------------------------------------------------------------------

PTREEROOT ShadowBinaryTree( PTREEROOT Original )
{
	PTREEROOT root;
	Log( "Use of binary tree shadows is fraught with danger!" );
	root = (PTREEROOT)Allocate( sizeof( TREEROOT ) );
	MemSet( root, 0, sizeof( TREEROOT ) );
	root->flags.bRoot = 1;
	root->flags.bUsed = 1;
	root->flags.bShadow = 1;
	root->children = 0;
	root->depth = 0;
	root->Compare = Original->Compare;
	root->Destroy = Original->Destroy;
	root->tree = Original->tree;
	return root;
}

#ifdef __cplusplus
} // namespace BinaryTree {
} //namespace containers {
} //namespace sack {
#endif

