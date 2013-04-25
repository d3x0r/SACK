/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   A binary tree container storing a user pointer blob of some user defined structure
 *   and a PTRSZVAL key which is used to check for content matchin.
 *   Binary tree has algorithms to become balanced, if the input is known to be weighted,
 *   or if statistics are pulled that indicate that the tree should be balanced, this
 *   function is available on demand.  Also searching through the tree using
 *   Least, Greatest, lesser, and greater is available.
 *
 * see also - include/typelib.h
 *
 */
#include <sack_types.h>
#include <sharemem.h>
#include <logging.h>

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
	_32 children;

	POINTER userdata;
	PTRSZVAL key;
	struct treenode_tag *lesser;
	struct treenode_tag *greater;
	struct treenode_tag **me;
	struct treenode_tag *parent;
};
typedef struct treenode_tag TREENODE;
	
#define MAXTREENODESPERSET 256
DeclareSet( TREENODE );

typedef struct treeroot_tag {
	struct {
		BIT_FIELD bUsed:1;
		BIT_FIELD bRoot:1;
		BIT_FIELD bShadow:1; // tree points to the real TREEROOT (not a node)
		BIT_FIELD bNoDuplicate : 1;
	} flags;
	_32 children;

	GenericDestroy Destroy;
	GenericCompare Compare;
	PTREENODE tree;
	PTREENODE prior, current, lastfound;
} TREEROOT;

static TREENODE TreeNodeSet;

POINTER GetLesserNodeEx( PTREEROOT root, PTREENODE *from );
POINTER GetGreaterNodeEx( PTREEROOT root, PTREENODE *from );

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

int CPROC BinaryCompareInt( PTRSZVAL old, PTRSZVAL new_key )
{
	if( old > new_key )
		return 1;
	else if( old < new_key )
		return -1;
	return 0;
}

//---------------------------------------------------------------------------

PTREENODE RotateToRight( PTREENODE node )
{
	PTREENODE greater = node->greater;
	*node->me = node->greater;
	// my parent's nodes do NOT change....
	// node->parent->children += node->greater->children - node->children;
	greater->me       = node->me;
	greater->parent   = node->parent;
	node->children   -= (greater->children+1);

	if( ( node->greater = greater->lesser ) )
	{
		greater->lesser->me     = &node->greater;
		greater->lesser->parent = node;
		node->children    += (greater->lesser->children + 1);
		greater->children -= (greater->lesser->children + 1);
	}
	greater->lesser = node;
	node->me        = &greater->lesser;
	node->parent    = greater;

	greater->children += (node->children + 1);
	return greater;
}

//---------------------------------------------------------------------------

PTREENODE RotateToLeft( PTREENODE node )
{
	PTREENODE lesser = node->lesser;
	*node->me = node->lesser;
	// my parent's nodes do NOT change....
	// node->parent->children += node->lesser->children - node->children;
	lesser->me       = node->me;
	lesser->parent   = node->parent;
	node->children  -= (lesser->children+1);

	if( ( node->lesser = lesser->greater ) )
	{
		lesser->greater->me     = &node->lesser;
		lesser->greater->parent = node;
		node->children   += (lesser->greater->children + 1);
		lesser->children -= (lesser->greater->children + 1);
	}
	lesser->greater = node;
	node->me        = &lesser->greater;
	node->parent    = lesser;

	lesser->children += (node->children + 1);
	return lesser;
}

//---------------------------------------------------------------------------
// RotateToLeft - make left node root/current.
// RotateToRight - make right node root/current

int BalanceBinaryBranch( PTREENODE root )
{
	PTREENODE check;
	int balances = 0;
	//while( balances )
	{
		balances = 0;
   	if( ( check = root ) )
   	{
 	   	if( check->lesser && check->greater)
 		   {
			int left = check->lesser->children
			 , right = check->greater->children;
			if( left > right )
			{
				if( left > 2+((left+right)*55)/100 )
				{
		 			//Log2( WIDE("rotateing to left (%d/%d)"), left, right );
					root = RotateToLeft( check );
					balances++;
				}
				//else
				//	root = NULL;
			}
			else
			{
				if( right  > 2+((left+right)*55)/100 )
				{
		 			//Log2( WIDE("rotateing to right (%d/%d)"), right, left );
					root = RotateToRight( check );
					balances++;
				}
				//else
				//	root = NULL;
			}

 		}
 		else if( check->lesser && ( check->children >= 2 ) )
 		{
 			//Log1( WIDE("rotateing to left (%d)"), check->children );
 			root = RotateToLeft( check );
			balances++;
 		}
 		else if( check->greater && ( check->children >= 2 )  )
 		{
 			//Log1( WIDE("rotateing to right (%d)"), check->children );
 			root = RotateToRight( check );
			balances++;
 		}
 		//else
 		//	root = NULL;
 		if( root )
 		{
			balances += BalanceBinaryBranch( root->lesser );
			balances += BalanceBinaryBranch( root->greater );
  		}
 	   }
 	}
 	return balances;
}

//---------------------------------------------------------------------------

void BalanceBinaryTree( PTREEROOT root )
{
	
	while( BalanceBinaryBranch( root->tree ) > 1 && 0);
	//Log( WIDE("=========") );;
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
         DeleteFromSet( TREENODE, &TreeNodeSet, node );
         //Release( node );
			return 0;
		}
      else
		{
			int leftchildren = 0, rightchildren = 0;
			if( check->lesser )
				leftchildren = check->lesser->children;
			if( check->greater )
				rightchildren = check->greater->children;
			if( leftchildren <= rightchildren )
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
   return 1;
}

//---------------------------------------------------------------------------

int AddBinaryNodeEx( PTREEROOT root
						, POINTER userdata
						, PTRSZVAL key DBG_PASS )
{
	PTREENODE node;
	if( !root )
		return 0;
	node = GetFromSet( TREENODE, &TreeNodeSet );//AllocateEx( sizeof( TREENODE ) DBG_RELAY );
	node->lesser = NULL;
	node->greater = NULL;
	node->me = NULL;
	node->children = 0;
	node->userdata = userdata;
	node->key = key;
	node->flags.bUsed = 1;
	node->flags.bRoot = 0;
	return HangBinaryNode( root, node );
}

#undef AddBinaryNode
int AddBinaryNode( PTREEROOT root
						, POINTER userdata
					  , PTRSZVAL key )
{
   return AddBinaryNodeEx( root, userdata, key DBG_SRC );
}
//---------------------------------------------------------------------------

static void RehangBranch( PTREEROOT root, PTREENODE node )
{
	if( node )
	{
		(*node->me) = NULL; // make sure I'm out of the tree...
		if( node->greater )
		{
			RehangBranch( root, node->greater );
		}
		if( node->lesser )
		{
			RehangBranch( root, node->lesser );
		}
      node->children = 0;
		//lprintf( "putting self node back in tree %p", node );
		HangBinaryNode( root, node );
	}
}

static void DecrementParentCounts( PTREENODE node, int count )
{
	PTREENODE parent;
	for( parent = node; parent && !parent->flags.bRoot; parent = parent->parent )
	{
		parent->children -= count;
	}
}

static void NativeRemoveBinaryNode( PTREEROOT root, PTREENODE node )
{
	if( root )
	{
		// lprintf( "Removing node from tree.. %p under %p", node, node->parent );
		if( node->parent->lesser != node && node->parent->greater != node )
		{
			*(int*)0=0;
		}
		// lprintf( "%p should be removed!", node );
		(*node->me) = NULL; // pull me out of the tree.
		DecrementParentCounts( node->parent, node->children+1 );

		// hang my right...
		RehangBranch( root, node->greater );
		// hang my left...
		RehangBranch( root, node->lesser );
		if( root->Destroy )
			root->Destroy( node->userdata, node->key );

		MemSet( node, 0, sizeof( node ) );
		DeleteFromSet( TREENODE, &TreeNodeSet, node );
		//Release( node );
		return;
	}
	lprintf( WIDE("Fatal RemoveBinaryNode could not find the root!") );
}

//---------------------------------------------------------------------------

 void  RemoveBinaryNode ( PTREEROOT root, POINTER data, PTRSZVAL key )
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

#define MAXTREEROOTSPERSET 128
DeclareSet( TREEROOT );
static PTREEROOTSET treepool;

//---------------------------------------------------------------------------

void DestroyBinaryTree( PTREEROOT root )
{
	while( root->tree )
		NativeRemoveBinaryNode( root, root->tree );
   DeleteFromSet( TREEROOT, &treepool, root );
}

//---------------------------------------------------------------------------

PTREEROOT CreateBinaryTreeExtended( _32 flags
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
	root->children = 0;
	root->Destroy = Destroy;
	//root->return  = NULL; // upgoing... (return from right )
	root->current = NULL;
	root->prior = NULL;
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
void DumpNode( PTREENODE node, int level, int (*DumpMethod)( POINTER user, PTRSZVAL key ) )
{
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
	if( print )
		lprintf( WIDE("[%3d] %p Node has %3")_32f WIDE(" children (%p %3")_32f WIDE(",%p %3")_32f WIDE("). %10") PTRSZVALfs
				 , level, node, node->children
				 , node->lesser
				 , (node->lesser)?(node->lesser->children+1):0
				 , node->greater
				 , (node->greater)?(node->greater->children+1):0
				 , node->key
				 );
	DumpNode( node->greater, level+1, DumpMethod );
}

//---------------------------------------------------------------------------

void DumpTree( PTREEROOT root 
				 , int (*Dump)( POINTER user, PTRSZVAL key ) )
{
	maxlevel = 0;
	lprintf( WIDE("Tree has %")_32f WIDE(" nodes. %p is root"), root->children, root->tree );
	DumpNode( root->tree, 1, Dump );
	lprintf( WIDE("Tree had %d levels."), maxlevel );
}

//---------------------------------------------------------------------------

POINTER FindInBinaryTree( PTREEROOT root, PTRSZVAL key )
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


int CPROC TextMatchLocate( PTRSZVAL key1, PTRSZVAL key2 )
{
	size_t k1len = StrLen( (CTEXTSTR)key1 );
	size_t k2len = StrLen( (CTEXTSTR)key2 );
	//lprintf( "COmpare %s(%d) vs %s(%d)", key1, k1len, key2, k2len );
	if( k2len < k1len )
	{
		// cannot match this.... but should
		// try to choose a direction
		int dir = StrCaseCmpEx( (CTEXTSTR)key1, (CTEXTSTR)key2, k2len );
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
POINTER LocateInBinaryTree( PTREEROOT root, PTRSZVAL key
								  , int (CPROC*fuzzy)( PTRSZVAL psv, PTRSZVAL node_key )

								  )
{
	PTREENODE node;
	node = root->tree;
	if( !fuzzy )
		fuzzy = TextMatchLocate;

	while( node )
	{
		int dir = fuzzy( key, node->key );
		if( dir == 100 )
		{
			PTREENODE one_up;
			// this matched, in an inexact length.
			// to be really careful we should match one up and one down.
			// well, we'll match better only if we had exact length
         // so - go up one node, until we find exact length
			//lprintf( " - Found a near match..." );
			one_up = node;
			do
			{
				GetLesserNodeEx( root, &one_up );
				dir = fuzzy( key, one_up->key );
				if( dir == 100 )
					continue;
				if( dir == 0 )
				{
					root->lastfound = one_up;
					return (one_up->userdata);
				}
				else
					break;
			}
			while( 1 );
			root->lastfound = node;
			return( node->userdata );
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

POINTER GetCurrentNode( PTREEROOT root )
{
	if( !root || !root->current )
		return NULL;
   return root->current->userdata;
}

//---------------------------------------------------------------------------

void RemoveLastFoundNode( PTREEROOT root )
{
	if( !root || !root->lastfound )
		return;
   NativeRemoveBinaryNode( root, root->lastfound );
}

//---------------------------------------------------------------------------

void RemoveCurrentNode( PTREEROOT root )
{
	if( !root || !root->current )
		return;
	NativeRemoveBinaryNode( root, root->current );
   root->current = NULL;
}

//---------------------------------------------------------------------------

POINTER GetGreaterNodeEx( PTREEROOT root, PTREENODE *from )
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

POINTER GetGreaterNode( PTREEROOT root )
{
   return GetGreaterNodeEx( root, &root->current );
}

//---------------------------------------------------------------------------

POINTER GetLesserNodeEx( PTREEROOT root, PTREENODE *from )
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

POINTER GetLesserNode( PTREEROOT root )
{
   return GetLesserNodeEx( root, &root->current );
}

//---------------------------------------------------------------------------

POINTER GetLeastNode( PTREEROOT root )
{
	if( !root ) return 0;
	root->current = root->tree;
	root->prior = NULL;
	while( root->current && root->current->lesser )
		root->current = root->current->lesser;
	if( root->current )
		return root->current->userdata;
	return 0;
}

//---------------------------------------------------------------------------

POINTER GetGreatestNode( PTREEROOT root )
{
	if( !root ) return 0;
	root->prior = NULL;
	root->current = root->tree;
	while( root->current && root->current->greater )
		root->current = root->current->greater;
	if( root->current )
		return root->current->userdata;
	return 0;
}

//---------------------------------------------------------------------------

POINTER GetRootNode( PTREEROOT root )
{
	if( !root ) return 0;
	root->prior = NULL;
	root->current = root->tree;
	if( root->current )
		return root->current->userdata;
	return 0;
}

//---------------------------------------------------------------------------

POINTER GetParentNode( PTREEROOT root )
{
	if( !root ) return 0;
	if( root->current )
	{
		root->prior = root->current;
		if( !root->current->parent->flags.bRoot )
			root->current = root->current->parent;
		if( root->current )
			return root->current->userdata;
	}
	return 0;
}

//---------------------------------------------------------------------------

POINTER GetChildNode( PTREEROOT root, int direction )
{
	if( !root ) return 0;
	if( root->current )
	{
		root->prior = root->current;
		if( direction < 0 )
		{
			root->current = root->current->lesser;
		}
		else 
			root->current = root->current->greater;
		if( root->current )
			return root->current->userdata;
	}
	return 0;
}

//---------------------------------------------------------------------------

POINTER GetPriorNode( PTREEROOT root )
{
	PTREENODE cur;
	if( !root ) return 0;
   cur = root->current;
	root->current = root->prior;
	root->prior = cur;
	if( root->current )
		return root->current->userdata;
	return 0;
}

//---------------------------------------------------------------------------

_32 GetNodeCount( PTREEROOT root )
{
   return root->children;
}

//---------------------------------------------------------------------------

PTREEROOT ShadowBinaryTree( PTREEROOT Original )
{
	PTREEROOT root;
	Log( WIDE("Use of binary tree shadows is fraught with danger!") );
	root = (PTREEROOT)Allocate( sizeof( TREEROOT ) );
	MemSet( root, 0, sizeof( TREEROOT ) );
	root->flags.bRoot = 1;
	root->flags.bUsed = 1;
	root->flags.bShadow = 1;
	root->children = 0;
	//root->return  = NULL; // upgoing... (return from right )
	root->current = NULL;
	root->prior = NULL;
	root->Compare = Original->Compare;
	root->Destroy = Original->Destroy;
	root->tree = Original->tree;
	return root;
}

#ifdef __cplusplus
}; // namespace BinaryTree {
}; //namespace containers {
}; //namespace sack {
#endif

//---------------------------------------------------------------------------
// $Log: binarylist.c,v $
// Revision 1.19  2005/01/27 07:18:34  panther
// Linux cleaned.
//
// Revision 1.18  2004/05/04 17:23:44  d3x0r
// Fix getlessernode
//
// Revision 1.17  2004/04/26 09:47:26  d3x0r
// Cleanup some C++ problems, and standard C issues even...
//
// Revision 1.16  2004/01/31 01:30:20  d3x0r
// Mods to extend/test procreglib.
//
// Revision 1.15  2004/01/29 10:13:44  d3x0r
// Remove ifdeffed logging, fix dumpnode to dump to log if no write method
//
// Revision 1.14  2003/10/24 14:50:11  panther
// Fix remove binary node, keep last found for quick delete
//
// Revision 1.13  2003/03/06 09:06:07  panther
// Oops - forgot to decrement the root count itself
//
// Revision 1.12  2003/03/06 08:56:06  panther
// fix code to unwind non-hung nodes
//
// Revision 1.11  2003/03/06 08:39:16  panther
// Stripped \r's.  Added GetNodeCount()
//
// Revision 1.10  2003/03/04 16:28:36  panther
// Cleanup warnings in typecode.  Convert PTRSZVAL to POINTER literal in binarylist
//
// Revision 1.9  2003/03/02 18:50:21  panther
// Added NO_DUPLICATES opption to  binary trees
//
// Revision 1.8  2003/02/20 02:35:17  panther
// Added debug message option flag
//
// Revision 1.7  2003/01/13 00:40:13  panther
// removed old msvc projects.
// Added new visual studio projects.
// Mods to compile cleanly under msvc.
//
// Revision 1.6  2002/08/12 22:16:02  panther
// Fixed buf in GetGreaterNode - last test tested prior->greater vs current
// which will never be true.
//
//
