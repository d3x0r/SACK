/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   A binary tree is often fine for sorting, but for relational things
 *   a Family tree that tracks parent, child, and peers is often required.
 *   The list of peers is a circular linked list based on LinkThing macros
 *   The List is actually broken foward cicularly, but not in a reverse direction
 *
 * see also - include/typelib.h
 *
 */
#define FAMILY_TREE_SOURCE_CODE
#include <sack_types.h>
#include <sharemem.h>
#include <logging.h>

#ifdef __cplusplus
namespace sack {
namespace containers {
namespace family {
//	using namespace sack::memory;
#endif

	
// consider slab allocation... 32 bytes even.
struct familynode_tag {
	struct {
		BIT_FIELD bUsed:1;
	} flags;
	POINTER userdata;
	uintptr_t key;
	struct familynode_tag *elder, *younger, *parent, *child;
};
typedef struct familynode_tag FAMILYNODE;

#define MAXFAMILYNODESPERSET 256
DeclareSet( FAMILYNODE );

struct familyroot_tag {
	struct {
		BIT_FIELD bUsed:1;
		BIT_FIELD bRoot:1;
		BIT_FIELD bShadow:1; // family points to the real FAMILYTREE (not a node)
		BIT_FIELD bNoDuplicate : 1;
	} flags;
	void (CPROC *Destroy)( POINTER user, uintptr_t key );
	int (CPROC *Compare)(uintptr_t old,uintptr_t newx);
	PFAMILYNODESET nodes;
	PFAMILYNODE family;
	PFAMILYNODE prior
		// current is where things are added
		// newly added nodes become current?
		// prior is the last state, so after adding
      // a child node, the parent may be returned to.
										, current//, prior
	// hmm lastfound... enumeration from this value?
	// what sort of enumeration of family trees exist?
										, lastfound;
};

typedef struct familyroot_tag FAMILYTREE;


//----------------------------------------------------------------------------

 PFAMILYTREE  CreateFamilyTree ( int (CPROC*Compare)(uintptr_t old,uintptr_t new_key),
															 void (CPROC*Destroy)( POINTER user, uintptr_t key ) )
{
	PFAMILYTREE root = (PFAMILYTREE)Allocate( sizeof( FAMILYTREE ) );
	MemSet( root, 0, sizeof( FAMILYTREE ) );
	root->Compare = Compare;
	root->Destroy = Destroy;
	return root;
}

//----------------------------------------------------------------------------

enum {
	RELATE_CHILD_OF
      , RELATE
};

//----------------------------------------------------------------------------

 POINTER  FamilyTreeFindChildEx ( PFAMILYTREE root, PFAMILYNODE root_node
													 , uintptr_t psvKey )
{
	PFAMILYNODE node = root_node;
	root->prior = root_node;
	if( node )
		node = node->child;
	else
		node = root->family;
	while( node )
	{
		int d;
		if( root->Compare )
			d = root->Compare( node->key, psvKey );
		else
			d = node->key > psvKey?1:node->key<psvKey?-1:0;
		if( !d )
			break;
		node = node->elder;
	}
	root->current = node;
	if( !node )
		return NULL;
	root->lastfound = node;
	return node->userdata;
}

POINTER  FamilyTreeFindChild ( PFAMILYTREE root, uintptr_t psvKey )
{
	return FamilyTreeFindChildEx( root, root->lastfound, psvKey );
}

//----------------------------------------------------------------------------

// scans the whole tree to find a node
LOGICAL FamilyTreeForEachChild( PFAMILYTREE root, PFAMILYNODE node
			, LOGICAL (CPROC *ProcessNode)( uintptr_t psvForeach, uintptr_t psvNodeData )
			, uintptr_t psvUserData )
{
	if( !node )
		node = root->family;
	else
		node = node->child;
	while( node )
	{
		LOGICAL process_result;
		process_result = ProcessNode( psvUserData, (uintptr_t)node->userdata );
		if( !process_result )
			return process_result;
		node = node->elder;
	}
	return TRUE;
}

// scans the whole tree to find a node
LOGICAL FamilyTreeForEach( PFAMILYTREE root, PFAMILYNODE node
			, LOGICAL (CPROC *ProcessNode)( uintptr_t psvForeach, uintptr_t psvNodeData, int level )
			, uintptr_t psvUserData )
{
	static int level;
	if( !node )
		node = root->family;
	else
		node = node->child;
	level++;
	while( node )
	{
		LOGICAL process_result;
		//lprintf( "node %p", node );
		process_result = ProcessNode( psvUserData, (uintptr_t)node->userdata, level );
		if( !process_result )
			return process_result;
		if( node->child )
			FamilyTreeForEach( root, node, ProcessNode, psvUserData );
		node = node->elder;
	}
	level--;
	return TRUE;
}

static  uintptr_t CPROC DestroyNode(void* p,uintptr_t psvUser )
{
	PFAMILYTREE option_tree = (PFAMILYTREE)psvUser;
	if( option_tree->Destroy )
		option_tree->Destroy( ((PFAMILYNODE)p)->userdata, ((PFAMILYNODE)p)->key );
	DeleteFromSet( FAMILYNODE, option_tree->nodes, p );
	return 0;
}

void  FamilyTreeClear ( PFAMILYTREE option_tree )
{
	ForAllInSet( FAMILYNODE, option_tree->nodes, DestroyNode, (uintptr_t)option_tree );
	DeleteSetEx( FAMILYNODE, &option_tree->nodes );
	option_tree->family = NULL;
//	option_tree->nodes
}
//----------------------------------------------------------------------------

// resets the search conditions, and possibley makes aa tree if it isn't already.
void  FamilyTreeReset ( PFAMILYTREE *option_tree )
{
	if( !option_tree )
		return;
	if( !(*option_tree ) )
		(*option_tree) = CreateFamilyTree( NULL, NULL );
	(*option_tree)->lastfound = NULL;
	(*option_tree)->current = NULL;
}

//----------------------------------------------------------------------------

PFAMILYNODE  FamilyTreeAddChild ( PFAMILYTREE *root, PFAMILYNODE parent, POINTER userdata, uintptr_t key )
{
	if( root )
	{
		PFAMILYNODE node;
		if( !(*root ) )
			(*root) = CreateFamilyTree( NULL, NULL );
		node = (PFAMILYNODE)GetFromSet( FAMILYNODE, &(*root)->nodes /*Allocate( sizeof( FAMILYNODE )*/ );
		node->child = NULL;
		node->younger = NULL;
		node->flags.bUsed = 0;
		node->userdata = userdata;
		node->key = key;
		node->parent = parent;//(*root)->prior;
		if( !node->parent )
		{
			if( (*root)->prior ) {
				if( ( node->elder = (*root)->prior->child ) )
					(*root)->family->younger = node;
			} else
				node->elder = NULL;
			(*root)->family = node;
		}
		else
		{
			if( (*root)->prior ) {
				if( ( node->elder = (*root)->prior->child ) )
					node->elder->younger = node;
				(*root)->prior->child = node;
			}
		}
		(*root)->prior = node;
		(*root)->lastfound = node;
		(*root)->current = node;
		return node;
	}
	return NULL;
}

#ifdef __cplusplus
}; //namespace family {
}; //namespace containers {
}; //namespace sack {
#endif
