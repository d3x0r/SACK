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
	PTRSZVAL key;
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
	void (CPROC *Destroy)( POINTER user, PTRSZVAL key );
	int (CPROC *Compare)(PTRSZVAL old,PTRSZVAL newx);
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

 PFAMILYTREE  CreateFamilyTree ( int (CPROC*Compare)(PTRSZVAL old,PTRSZVAL new_key),
															 void (CPROC*Destroy)( POINTER user, PTRSZVAL key ) )
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

 POINTER  FamilyTreeFindChild ( PFAMILYTREE root
													 , PTRSZVAL psvKey )
{
	PFAMILYNODE node = root->lastfound;
   	root->prior = root->lastfound;
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

//----------------------------------------------------------------------------

// scans the whole tree to find a node
LOGICAL FamilyTreeForEachChild( PFAMILYTREE root, PFAMILYNODE node
			, LOGICAL (CPROC *ProcessNode)( PTRSZVAL psvForeach, PTRSZVAL psvNodeData )
			, PTRSZVAL psvUserData )
{
	if( !node )
		node = root->family;
	else
		node = node->child;
	while( node )
	{
		LOGICAL process_result;
		process_result = ProcessNode( psvUserData, node->userdata );
		if( !process_result )
			return process_result;
		node = node->elder;
	}
	return TRUE;
}

//----------------------------------------------------------------------------

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

PFAMILYNODE  FamilyTreeAddChild ( PFAMILYTREE *root, POINTER userdata, PTRSZVAL key )
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
		node->parent = (*root)->prior;
		if( !node->parent )
		{
			if( ( node->elder = (*root)->family ) )
				(*root)->family->younger = node;
			(*root)->family = node;
		}
		else
		{
			if( ( node->elder = (*root)->prior->child ) )
				node->elder->younger = node;
			(*root)->prior->child = node;
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
