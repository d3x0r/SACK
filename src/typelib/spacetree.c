#define NO_UNICODE_C
//#define NO_LOGGING
//#define HANG_DEBUG
//#define FIND_DEBUG
//#define DIRTY_NODE_DEBUG
#include <stdhdrs.h>
#include <stdio.h>
#include <idle.h>
#include <sharemem.h>
#include <timers.h> // enter/leave critical sec
#include <logging.h>

#include "spacetree.h"

#ifdef __ANDROID_OLD_PLATFORM_SUPPORT__
#define rand lrand48
#endif

SPACETREE_NAMESPACE

static CRITICALSECTION csSpace;
// if 3d space partitioning - AREAS will be 27 - ICK!
#define AREAS 9

struct spacenode {
	void *data;
	SPACEPOINT min, max;
	struct {
		BIT_FIELD bDirty : 1;
	} flags;
	int col_span, row_span; // if considered in a table of all top level cells, this is the span of this one.
	IMAGE_RECTANGLE dirty;
	//int children; // hmm... this may or may not be usable...
	struct spacenode *parent; // may have as many parents as it is.
	// think this will be more space than use... cept THE root.
	struct spacenode **me; // can span up to the same number of areas...
	struct spacenode *prior, *next; // double link list of other nodes that are me.
	struct spacenode *area[AREAS]; // other areas linked off this...
};
typedef struct spacenode SPACENODE;
#define SPACE_STRUCT_DEFINED

#define AREA_TOPUPPERLEFT 9+0
#define AREA_TOPUP 9+1
#define AREA_TOPUPPERRIGHT 9+2
#define AREA_TOPLEFT 9+3
#define AREA_TOPHERE 9+4
#define AREA_TOPRIGHT 9+5
#define AREA_TOPLOWERLEFT 9+6
#define AREA_TOPDOWN 9+7
#define AREA_TOPLOWERRIGHT 9+8

#define AREA_UPPERLEFT 0
#define AREA_UP 1
#define AREA_UPPERRIGHT 2
#define AREA_LEFT 3
#define AREA_HERE 4
#define AREA_RIGHT 5
#define AREA_LOWERLEFT 6
#define AREA_DOWN 7
#define AREA_LOWERRIGHT 8

#define AREA_BOTTOMUPPERLEFT 18+0
#define AREA_BOTTOMUP 18+1
#define AREA_BOTTOMUPPERRIGHT 18+2
#define AREA_BOTTOMLEFT 18+3
#define AREA_BOTTOMHERE 18+4
#define AREA_BOTTOMRIGHT 18+5
#define AREA_BOTTOMLOWERLEFT 18+6
#define AREA_BOTTOMDOWN 18+7
#define AREA_BOTTOMLOWERRIGHT 18+8

#define MAXSPACENODESPERSET 128
DeclareSet( SPACENODE );


SPACENODESET nodeset;
SPACENODESET *SpaceNodes = &nodeset;
PLIST pDirtyNodes;


// search is first done left, up, right, down....

// this is done to be nice and avoid warnings of duplicate definitoin.
// which it IS - see above SPACEPOINT, PSPACEPOINT
//#undef SPACEPOINT
//#undef PSPACEPOINT
//#include "spacetree.h"


void *GetNodeData( PSPACENODE node )
{
	if( node )
	{
		if( !node->data )
			lprintf( WIDE("Node without data!") );
		return node->data;
	}
	return NULL;
}

void GetNodeRect( PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
{
	min = node->min;
	max = node->max;
}

PSPACENODE GetDirtyNode( void *p, P_IMAGE_RECTANGLE rect )
{
	PSPACENODE node;
	INDEX idx;
	// this is actually a funny way of geteting
	// the first node in the list...
	//lprintf( WIDE("Getting a dirty node... ") );
	EnterCriticalSec( &csSpace );
	//lprintf( WIDE("pDirtyNodes = %p"), pDirtyNodes );
	LIST_FORALL( pDirtyNodes, idx, PSPACENODE, node )
	{
		if( !node->flags.bDirty )
		{
			lprintf( WIDE("Node was not dirty (anymore)?") );
			SetLink( &pDirtyNodes, idx, NULL );
			continue;
		}
#ifdef DIRTY_NODE_DEBUG
		lprintf( WIDE("%p == %p? "), p, GetNodeData( node ) );
#endif
		if( !p || ( GetNodeData( node ) == p ) )
		{
		// mark all nodes cleaned.
#ifdef DIRTY_NODE_DEBUG
			lprintf( WIDE("Resulting with %p"), node );
#endif
			SetLink( &pDirtyNodes, idx, NULL );
			if( rect )
			{
				rect->x = node->dirty.x;
				rect->y = node->dirty.y;
				rect->width = node->dirty.width;
				rect->height = node->dirty.height;
			}
			node->flags.bDirty = 0;
			LeaveCriticalSec( &csSpace );
			return node;
		}
	}
#ifdef DIRTY_NODE_DEBUG
	lprintf( WIDE("Done...") );
#endif
	LeaveCriticalSec( &csSpace );
	return NULL;
}

void MergeRectangles( P_IMAGE_RECTANGLE r1, P_IMAGE_RECTANGLE r2 )
{
	if( r2->x < r1->x )
	{
		r1->width += r1->x - r2->x;
		r1->x = r2->x;
	}
	if( r2->y < r1->y )
	{
		r1->height += r1->y - r2->y;
		r1->y = r2->y;
	}
	if( (r2->width + r2->x) > ( r1->width + r1->x ) )
	{
		r1->width = (r2->width + r2->x) - r1->x;
	}
	if( (r2->height + r2->y) > ( r1->height + r1->y ) )
	{
		r1->height = (r2->height + r2->y) - r1->y;
	}
}

int IsNodeDirty( PSPACENODE node, P_IMAGE_RECTANGLE rect )
{
	if( node )
	{
		(*rect) = ((node)->dirty);
		return node->flags.bDirty;
	}
	return 0;
}

void MarkNodeDirty( PSPACENODE node, P_IMAGE_RECTANGLE rect )
{
	if( node )
	{
		if( !node->flags.bDirty )
		{
#ifdef DIRTY_NODE_DEBUG
			lprintf( WIDE("Adding dirty node ... %p "), node );
#endif
			if( rect )
				node->dirty = *rect;
			else
			{
				node->dirty.x = node->min[0];
				node->dirty.y = node->min[1];
				node->dirty.width = node->max[0]-node->min[0];
				node->dirty.height = node->max[1]-node->min[1];
			}
			node->flags.bDirty = 1;
			AddLink( &pDirtyNodes, node );
			//lprintf( WIDE("pDirtyNodes = %p"), pDirtyNodes );
		}
//#ifdef DIRTY_NODE_DEBUG
		else
		{
			//AddLink( &pDirtyNodes, node );
			//lprintf( WIDE("skipping dirty node(already dirty)...expanding rect? %p "), node );
			MergeRectangles( &node->dirty, rect );
		}
//#endif
	}
}

PSPACENODE pDeepest;
int deepest;

PSPACENODE FindDeepestNode( PSPACENODE root, int level )
{
	int i;
	if( !root )
		return NULL;
	if( level == 0 )
	{
		deepest = 0;
		pDeepest = root;
	}
	else
	{
		if( level > deepest )
		{
			//Log2( WIDE("Deepest: %d %08x"), level, root );
			deepest = level;
			pDeepest = root;
		}
	}
	for( i = 0; i < AREAS; i++ )
		if( root->area[i] )
			FindDeepestNode( root->area[i], level+1 );
	return pDeepest;
}


void ValidateSpaceTreeEx( PSPACENODE root DBG_PASS )
{
	int i;
	if( !root )
		return;
	for( i = 0; i < AREAS ; i++ )
	{
		if( root->area[i] )
		{
			if( root->area[i]->me != &root->area[i] )
				_xlprintf(LOG_NOISE DBG_RELAY)( WIDE(":Node in quadrant %d does not reference itself..?")
					 , i );
			if( root->area[i]->parent != root )
			{
				_xlprintf(LOG_NOISE DBG_RELAY)( WIDE(":Node %p in quadrant %d does not reference me?(%p)")
					 , root->area[i], i, root->area[i]->parent );
				//DumpSpaceTree( root );
			}
			ValidateSpaceTreeEx( root->area[i] DBG_RELAY );
		}
	}
}

void ValidateSpaceRoot( PSPACENODE chunk DBG_PASS )
#define ValidateSpaceRoot(c) ValidateSpaceRoot( (c) DBG_SRC)
{
	PSPACENODE *root;
	if( !chunk )
		return;
	root = chunk->me;
	if( chunk->parent ) // if this was the root of the tree.. don't go back
	{
		while( (*root)->parent ) // while there is a parent of this...
		{
			if( ((uintptr_t)((*root)->parent)) & 3 )
			{
				Log( WIDE("We're in trouble - attempting to fix...") );
				(*root)->parent = (PSPACENODE)((uintptr_t)((*root)->parent) + 1 );
			}
			root = (*root)->parent->me;
		}
		root = (*root)->me; // step back one more - REAL root of tree.
	}
	ValidateSpaceTreeEx( *root DBG_RELAY );
}

// return the address of the root thing... keep all things hung here
// attached to this node...
//PSPACENODE *
PSPACENODE *GrabSpace( PSPACENODE node )
{
	// returns me...
	PSPACENODE *root;
	if( node )
	{
		if( ( root = node->me ) )
			*(node->me) = NULL;
		while( node->parent )
		{
			//Log2( WIDE("Updating %08x's children from %d"), node->parent, node->parent->children );
			//node->parent->children -= node->children + 1;
			node->parent = node->parent->parent;
		}
		node->me = NULL;
		return root;
	}
	return NULL;
}

static void HangSpaceNodeExx( PSPACENODE *root, PSPACENODE parent, PSPACENODE space DBG_PASS );

void RehangSpaces( PSPACENODE *root, PSPACENODE space )
{
	int i;
	if( space )
	{
		GrabSpace( space );
		for( i = 0; i < AREAS; i++ )
			RehangSpaces( root, space->area[i] );
		HangSpaceNode( root, space );
	}
}

void DeleteNode( PSPACENODE node )
{
	int i;
	PSPACENODE *root;
	root = GrabSpace( node );
	for( i = 0; i < AREAS; i++ )
	{
		HangSpaceNode( root, node->area[i] );
	}
	DeleteFromSet( SPACENODE, SpaceNodes, node );
}

void CollapseNodes( PSPACENODE node )
{
	PSPACENODE space, space2, parent, next;
	do
	{

		space = node;
		while( space->prior ) space = space->prior;
		while( space )
		{
			space2 = space->next;
			while( space2 )
			{
				next = space2->next;
				//if(
				space2 = next;
			}
			space = space->next;
		}

		space = node;
		while( space->prior ) space = space->prior;
		while( space )
		{
			next = space->next;
			if( ( parent = space->parent ) )
			{
				if( parent->data == space->data )
				{
					if( space->min[0] == parent->min[0] &&
						 space->max[0] == parent->max[0] )
					{
						if( space->min[1] == parent->max[1] )
						{
							parent->min[1] = space->max[1];
							DeleteNode( space );
						}
						else if( space->max[1] == parent->min[1] )
						{
							parent->max[1] = space->min[1];
							DeleteNode( space );
						}
					}
					else
					if( space->min[1] == parent->min[1] &&
						 space->max[1] == parent->max[1] )
					{
						if( space->min[0] == parent->max[0] )
						{
							parent->min[0] = space->max[0];
							DeleteNode( space );
						}
						else if( space->max[0] == parent->min[0] )
						{
							parent->max[0] = space->min[0];
							DeleteNode( space );
						}
					}
				}
			}
			space = next;
		}
	}while(0);
}

PSPACENODE RemoveSpaceNode2( PSPACENODE *root, PSPACENODE space )
{
	// unlink from the tree, reque all children back on the tree...
	// unlink all relatives from the tree, again reque all children
	int i;
	PSPACENODE last, chunk, next;
	if( !space )
	{
		Log( WIDE("Couldn't remove nothing") );
		return NULL;
	}

	chunk = space;
	while( chunk->next )
		chunk = chunk->next;
	while( space->prior )
		space = space->prior;
	if( !chunk->me || *chunk->me != chunk )
	{
		Log1( WIDE("Couldn't remove something already removed (%p)"), chunk );
		return NULL;
	}

	last = chunk; 		  // keep this for second stage looping...

	while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
	{
		GrabSpace( chunk ); // space->me is still valid...
		//chunk->children = 0;
		chunk = chunk->prior;
	}
	chunk = last;

	while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
	{
		for( i = 0; i < AREAS; i++ )
			if( chunk->area[i] )
			{
				HangSpaceNode( root, RemoveSpaceNode2( root, chunk->area[i] ) );
				chunk->area[i] = NULL;
			}
		//chunk->children = 0;
		next = chunk->prior;
		if( chunk == space )
		{
			chunk = next;
			continue;
		}
		if( chunk->prior )
			chunk->prior->next = chunk->next;
		if( chunk->next )
			chunk->next->prior = chunk->prior;
		DeleteFromSet( SPACENODE, SpaceNodes, chunk );
		chunk = next;
	}
	if( chunk )
	{
		if( !chunk->next )
			Log( WIDE("We're screwed! should be 2 nodes - the original, and one other") );
		if( chunk != space )
		{
#ifdef PARTITION_SCREEN
			space->min[0] = chunk->min[0];
			space->min[1] = chunk->min[1];
			space->max[0] = chunk->max[0];
			space->max[1] = chunk->max[1];
#else
			SetPoint( space->min, chunk->min );
			SetPoint( space->max, chunk->max );
#endif
			chunk->next->prior = NULL;
			DeleteFromSet( SPACENODE, SpaceNodes, chunk );
		}
		else
		{
			DeleteFromSet( SPACENODE, SpaceNodes, chunk->next );
			chunk->next = NULL;
		}
	}
	if( space->prior || space->next )
	{
		Log( WIDE("Fatal - removed node is not solitary...") );
	}
	//Log( WIDE("This should be non null result...") );
	return space;
}


PSPACENODE RemoveSpaceNode( PSPACENODE space )
{
	// unlink from the tree, reque all children back on the tree...
	// unlink all relatives from the tree, again reque all children
	int i;
	PSPACENODE last, chunk, next;
	PSPACENODE *root;
	if( !space )
	{
		Log( WIDE("Couldn't remove nothing") );
		return NULL;
	}
	// first chunk may not actually be IN the tree... but just track
	// the space for rehanging as a child removed and rehung..
	if( space->prior )
	{
		Log( WIDE("***Removing a node which is not the master... may be bad!") );
	}
	chunk = space;
	while( chunk->next )
		chunk = chunk->next;
	if( !chunk->me || *chunk->me != chunk )
	{
		Log1( WIDE("Couldn't remove something already removed (%p)"), chunk );
		return NULL;
	}
	last = chunk; 		  // keep this for second stage looping...
	root = chunk->me;
	if( chunk->parent ) // if this was the root of the tree.. don't go back
	{
		while( (*root)->parent ) // while there is a parent of this...
		{
			if( ((uintptr_t)((*root)->parent)) & 3 )
			{
				Log( WIDE("We're in trouble - attempting to fix...") );
				(*root)->parent = (PSPACENODE)((uintptr_t)((*root)->parent) + 1 );
			}
			//Log3( WIDE("step to root...%08x %08x %08x"), (*root), (*root)->parent, (*root)->parent->me );
			root = (*root)->parent->me;
		}
		//Log( WIDE("step to root Final...") );
		root = (*root)->me; // step back one more - REAL root of tree.
	}

	while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
	{
		//Log( WIDE("To grab chunk...") );
		GrabSpace( chunk ); // space->me is NOT valid...
		//chunk->children = 0;
		chunk = chunk->prior;
		//Log3( WIDE("Grabbed a chunk...%08x %08x %08x"), chunk, (chunk)?chunk->prior:0, (chunk)?chunk->next:0 );
	}

	chunk = last;
	while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
	{
		for( i = 0; i < AREAS; i++ )
			if( chunk->area[i] )
			{
				//Log( WIDE("Rehang child of me...") );
				// if this was linked to itself within the same node
				// then by now it is currently unlinked from itself, and
				// this part of this loop would not run... therefore it
				// should be safe and never hit this point...
				if( chunk->area[i]->data == space->data )
				{
					Log( WIDE("DIE DIE! we're hanging on ourselves still :(") );
				}
				HangSpaceNode( root, RemoveSpaceNode2( root, chunk->area[i] ) );
				if( chunk->area[i] )
				{
					Log( WIDE("Removal failed somehow - or perhaps we requeued more?") );
					chunk->area[i] = NULL; // lose information :(
				}
			}
		//chunk->children = 0;
		next = chunk->prior;
		if( chunk == space ) // don't unlink the specific one asked for
		{
			chunk = next;
			continue;
		}
		if( chunk->prior )
			chunk->prior->next = chunk->next;
		if( chunk->next )
			chunk->next->prior = chunk->prior;
		DeleteFromSet( SPACENODE, SpaceNodes, chunk );
		chunk = next;
	}
	if( chunk )
	{
		if( !chunk->next )
			Log( WIDE("We're screwed! should be 2 nodes - the original, and one other") );
		if( chunk != space )
		{
			Log( WIDE("Abnormal chunk space linking - but that's ok...") );
#ifdef PARTITION_SCREEN
			space->min[0] = chunk->min[0];
			space->min[1] = chunk->min[1];
			space->max[0] = chunk->max[0];
			space->max[1] = chunk->max[1];
#else
			SetPoint( space->min, chunk->min );
			SetPoint( space->max, chunk->max );
#endif
			chunk->next->prior = NULL;
			DeleteFromSet( SPACENODE, SpaceNodes, chunk );
		}
		else
		{
			DeleteFromSet( SPACENODE, SpaceNodes, chunk->next );
			chunk->next = NULL;
		}
	}
	if( space->prior || space->next )
	{
		Log( WIDE("Fatal - removed node is not solitary...") );
	}
	/*
	Log4( WIDE("This should be non null result...%08x %08x %08x %08x"), space->prior, space->next, space->parent, space->me );
	for( i = 0; i < AREAS; i++ )
	{
		Log1( WIDE("My areas are: %08x"), space->area[i] );
	}
	if( root && *root )
	{
		Log4( WIDE("This should be non null result...%08x %08x %08x %08x"), (*root)->prior, (*root)->next, (*root)->parent, (*root)->me );
		for( i = 0; i < AREAS; i++ )
		{
			Log1( WIDE("My areas are: %08x"), (*root)->area[i] );
		}
	}
	*/
	return space;
}

// min and max are INCLUSIVE of
// all data.

PSPACENODE FindPointInSpace( PSPACENODE root, PSPACEPOINT p, int (*Validate)(void *data, PSPACEPOINT p ) )
{
	while( root )
	{
#ifdef FIND_DEBUG
		lprintf( WIDE("Finding point in space: %08x  %d,%d in (%d,%d)-(%d,%d)")
				 , root
				 , p[0], p[1]
				 , root->min[0], root->min[1]
				 , root->max[0], root->max[1] );
#endif
		if( p[0] < root->min[0] ) // space is left of here total.
		{
			if( p[1] < root->min[1] ) // space is above this.
			{
				root = root->area[AREA_UPPERLEFT];
			}
			else if( p[1] > root->max[1] ) // space is below this.
			{
				root = root->area[AREA_LOWERLEFT];
			}
			else
			{
				root = root->area[AREA_LEFT];
			}
		}
		else if( p[0] > root->max[0] ) // space is right of this
		{
			if( p[1] < root->min[1] ) // space is above of root...
			{
				root = root->area[AREA_UPPERRIGHT];
			}
			else if( p[1] > root->max[1] ) // space is below of this...
			{
				root = root->area[AREA_LOWERRIGHT];
			}
			else
			{
				root = root->area[AREA_RIGHT]; // to the right
			}
		}
		else if( p[1] > root->max[1] ) // space is below of this one...
		{
			root = root->area[AREA_DOWN];
		}
		else if( p[1] < root->min[1] )  // point is above this
		{
			root = root->area[AREA_UP];
		}
		else // point is in this center section....
		{
			if( Validate )
			{
				//Log2( WIDE("Validating data %08x->%08x"), root, root->data );
				SPACEPOINT x;
				PSPACENODE node = root;
#ifdef PARTITION_SCREEN
				while( node && node->prior )
					node = node->prior;
				x[0] = p[0] - node->min[0];
				x[1] = p[1] - node->min[1];
#else
				sub( x, p, root->min );
#endif
				// pass biased point to this region.
				// this should be an option - but this application
					  // this is the desired effect.
#ifdef FIND_DEBUG
				lprintf( WIDE("Pass this off to validation routine ...") );
#endif
				if( Validate( root->data, x ) )
					return root;
			}
			else
				return root;
			// only cover the first layer.
			root = NULL;
			//root = root->area[AREA_HERE];
		}
	}
	// root will be NULL here if there was no node found.
	return root; // null.
}

// min and max are INCLUSIVE of
// all data.

// min and max of rectangle of interest are in screen coordinates.
// this will call Validate for all spaces which intersect the min/max
// rectangle at any part.
// the min/max of the region discovered will be relative to the
// region itself.  
// Any region has the first node in the chain containing the original 
// space to be tracked.  This origin is subtracted from min and max.

// therefore screen coordinates may be recovered by adding the image-root's
// position back to the rectangle.   The rectangles will be 0 biased to this
// region though.

typedef struct {
	SPACEPOINT min, max;
	PSPACENODE node;
} FIND_RECT_DATA, *PFIND_RECT_DATA;


static PSPACENODE InternalFindRectInSpaceEx( PSPACENODE root
														 , PSPACEPOINT min
														 , PSPACEPOINT max
														 , PDATASTACK *stack
														 , int bGetNode
														  DBG_PASS )
{
	//static PDATASTACK pds;
	//static PDATASTACK *findstack;
	//int bGetNode;
#ifdef FIND_DEBUG
	static int levels;
#endif

	// what to do if no stack?!
	// well - we could pray?
#ifdef FIND_DEBUG
	Log5( WIDE("- Findinging area: min(%ld,%ld) max(%ld,%ld) level %d"), min[0], min[1], max[0], max[1], ++levels );
	if( root )
		Log5( WIDE("- Node: %p min(%ld,%ld) max(%ld,%ld)"), root
					, root->min[0], root->min[1]
					, root->max[0], root->max[1] );
#endif

	// this is the thread idenfiier...
	// this will lock other threads, and when
	// they are allowed to go, the stack will be null...
	// otherwise, we're in the same level of this proc...
	// probably could restructure this routine to hit crit sec less...
	if( root )
	{
		//bGetNode = FALSE;
		SPACEPOINT newmin, newmax;
		SPACEPOINT newmin2, newmax2; // secondary splits.
		//EmptyDataStack( findstack );

		while( root )
		{
#ifdef FIND_DEBUG
			Log5( WIDE("* Findinging area: min(%ld,%ld) max(%ld,%ld) level %d"), min[0], min[1], max[0], max[1], levels );
#endif
			if( max[0] < root->min[0] )
			{
				if( max[1] < root->min[1] )
					root = root->area[ AREA_UPPERLEFT ];
				else if( min[1] > root->max[1] )
					root = root->area[ AREA_LOWERLEFT ];
				else
				{
#ifdef FIND_DEBUG
					Log("Basecase discovered\n");
#endif
					if( min[1] < root->min[1] )
					{
						newmin[0] = min[0];
						newmin[1] = min[1];
						newmax[0] = max[0];
						newmax[1] = root->min[1] - 1;
						min[1] = root->min[1];
						if( root->area[AREA_UPPERLEFT] )
							InternalFindRectInSpaceEx( root->area[AREA_UPPERLEFT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					if( max[1] > root->max[1] )
					{
						newmin[0] = min[0];
						newmin[1] = root->max[1] + 1;
						newmax[0] = max[0];
						newmax[1] = max[1];
						max[1] = root->max[1];
						if( root->area[AREA_LOWERLEFT] )
							InternalFindRectInSpaceEx( root->area[AREA_LOWERLEFT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					//if( root->area[AREA_LEFT] )
					root = root->area[AREA_LEFT];
				}
			}	
			else if( min[0] > root->max[0] )
			{
				if( max[1] < root->min[1] )
					root = root->area[ AREA_UPPERRIGHT ];
				else if( min[1] > root->max[1] )
					root = root->area[ AREA_LOWERRIGHT ];
				else
				{
					if( min[1] < root->min[1] )
					{
						newmin[0] = min[0];
						newmin[1] = min[1];
						newmax[0] = max[0];
						newmax[1] = root->min[1] - 1;
						min[1] = root->min[1];
						if( root->area[AREA_UPPERRIGHT] )
							InternalFindRectInSpaceEx( root->area[AREA_UPPERRIGHT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					if( max[1] > root->max[1] )
					{
						newmin[0] = min[0];
						newmin[1] = root->max[1] + 1;
						newmax[0] = max[0];
						newmax[1] = max[1];
						max[1] = root->max[1];
						if( root->area[AREA_LOWERRIGHT] )
							InternalFindRectInSpaceEx( root->area[AREA_LOWERRIGHT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					//if( root->area[AREA_RIGHT] )
					root = root->area[AREA_RIGHT];
				}
			}
			else 
			{
				if( min[0] < root->min[0] )
				{
					newmin[0] = min[0];
					newmin[1] = min[1];
					newmax[0] = root->min[0] - 1;
					newmax[1] = max[1];
					min[0] = root->min[0];
					// handle top/center/lower subsplits.
					if( max[1] < root->min[1] )
					{
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to upper left") );
#endif
						if( root->area[AREA_UPPERLEFT] )
							InternalFindRectInSpaceEx( root->area[AREA_UPPERLEFT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					else if( min[1] > root->max[1] )
					{
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to lower left") );
#endif
						if( root->area[AREA_LOWERLEFT] )
							InternalFindRectInSpaceEx( root->area[AREA_LOWERLEFT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					else
					{
						if( min[1] < root->min[1] )
						{
							// secondary split
							newmin2[0] = newmin[0];
							newmin2[1] = newmin[1];
							newmax2[0] = newmax[0];
							newmax2[1] = root->min[1] - 1;
#ifdef FIND_DEBUG
							lprintf( WIDE("--- to upper left") );
#endif
							if( root->area[AREA_UPPERLEFT] )
							{
								InternalFindRectInSpaceEx( root->area[AREA_UPPERLEFT], newmin2, newmax2, stack, FALSE DBG_RELAY );
							}
						}
						if( max[1] > root->max[1] )
						{
							// secondary split
							newmin2[0] = newmin[0];
							newmin2[1] = root->max[1] + 1;
							newmax2[0] = newmax[0];
							newmax2[1] = newmax[1];
#ifdef FIND_DEBUG
							lprintf( WIDE("--- to lower left") );
#endif
							if( root->area[AREA_LOWERLEFT] )
								InternalFindRectInSpaceEx( root->area[AREA_LOWERLEFT], newmin2, newmax2, stack, FALSE DBG_RELAY );
						}
						newmin[1] = root->min[1];
						newmax[1] = root->max[1];
						// remaining part to the left.
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to left") );
#endif
						if( root->area[AREA_LEFT] )
							InternalFindRectInSpaceEx( root->area[AREA_LEFT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
				}
	
				if( max[0] > root->max[0] )
				{
					newmin[0] = root->max[0] + 1;
					newmin[1] = min[1];
					newmax[0] = max[0];
					newmax[1] = max[1];
					max[0] = root->max[0];
					// handle top/center/lower subsplits.
					if( max[1] < root->min[1] )
					{
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to upper right") );
#endif
						if( root->area[AREA_UPPERRIGHT] )
							InternalFindRectInSpaceEx( root->area[AREA_UPPERRIGHT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					else if( min[1] > root->max[1] )
					{
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to lower right") );
#endif
						if( root->area[AREA_LOWERRIGHT] )
							InternalFindRectInSpaceEx( root->area[AREA_LOWERRIGHT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					else
					{
						if( min[1] < root->min[1] )
						{
							// secondary split
							newmin2[0] = newmin[0];
							newmin2[1] = newmin[1];
							newmax2[0] = newmax[0];
							newmax2[1] = root->min[1] - 1;
							newmin[1] = root->min[1];
#ifdef FIND_DEBUG
							lprintf( WIDE("--- to upper right") );
#endif
							if( root->area[AREA_UPPERRIGHT] )
								InternalFindRectInSpaceEx( root->area[AREA_UPPERRIGHT], newmin2, newmax2, stack, FALSE DBG_RELAY );
						}
						if( max[1] > root->max[1] )
						{
							// secondary split
							newmin2[0] = newmin[0];
							newmin2[1] = root->max[1] + 1;
							newmax2[0] = newmax[0];
							newmax2[1] = newmax[1];
							newmax[1] = root->max[1];
#ifdef FIND_DEBUG
							lprintf( WIDE("--- to lower right") );
#endif
							if( root->area[AREA_LOWERRIGHT] )
								InternalFindRectInSpaceEx( root->area[AREA_LOWERRIGHT], newmin2, newmax2, stack, FALSE DBG_RELAY );
						}
						// remaining part to the left.
#ifdef FIND_DEBUG
						lprintf( WIDE("--- to  right") );
#endif
						if( root->area[AREA_RIGHT] )
							InternalFindRectInSpaceEx( root->area[AREA_RIGHT], newmin, newmax, stack, FALSE DBG_RELAY );
					}
				}
				// remaining portion here is centered, may have top/center/middle
				// parts... clip top and bottom over this one...
				if( max[1] < root->min[1] )
				{
					root = root->area[AREA_UP];
				}
				else if( min[1] > root->max[1] )
				{
					root = root->area[AREA_DOWN];
				}
				else
				{
					if( min[1] < root->min[1] )
					{
						newmin[0] = min[0];
						newmin[1] = min[1];
						newmax[0] = max[0];
						newmax[1] = root->min[1] - 1;
						min[1] = root->min[1];
						if( root->area[AREA_UP] )
							InternalFindRectInSpaceEx( root->area[AREA_UP], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					if( max[1] > root->max[1] )
					{
						newmin[0] = min[0];
						newmin[1] = root->max[1] + 1;
						newmax[0] = max[0];
						newmax[1] = max[1];
						max[1] = root->max[1];
						if( root->area[AREA_DOWN] )
							InternalFindRectInSpaceEx( root->area[AREA_DOWN], newmin, newmax, stack, FALSE DBG_RELAY );
					}
					{
						FIND_RECT_DATA frd;
						frd.max[0] = max[0];
						frd.max[1] = max[1];
						frd.min[0] = min[0];
						frd.min[1] = min[1];
						frd.node = root;
#ifdef FIND_DEBUG
						Log6( WIDE("Observing node: %p(%p) result (%ld,%ld)-(%ld,%ld)")
							 , frd.node
							 , frd.node->data
							 , min[0], min[1]
							 , max[0], max[1] );
#endif
						//EnqueData( findstack, (POINTER)&frd );
						//Log1( DBG_FILELINEFMT "Stack: %d" DBG_RELAY, (*findstack)->Top );
						PushData( stack, (POINTER)&frd );
					}
					root = root->area[AREA_HERE];
					root = NULL; // only return the first layer of rectangles.
					// therefore if the rectangle returned is 
					// not the one that is related - it 
					// well yeah - let's try this.
				}
			}
		}
	}
#ifdef FIND_DEBUG
	levels--;
#endif
	// the above loop should put everything in the stack
	// any time after the first ( root == NULL )
	// or when we're leaving processing the topmost, return the item.
	if( bGetNode && (*stack) )
	{
		PFIND_RECT_DATA data;
//      if( DequeData( findstack, (POINTER)&data ) )
#ifdef FIND_DEBUG
		Log1( WIDE("return Stack: %d"), (*stack)->Top );
#endif
		if( ( data = (PFIND_RECT_DATA)PopData( stack ) ) )
		{
			min[0] = data->min[0];
			min[1] = data->min[1];
			max[0] = data->max[0];
			max[1] = data->max[1];
#ifdef FIND_DEBUG
			lprintf( WIDE("result to (%d,%d)-(%d,%d) %p"), min[0], min[1], max[0], max[1], data->node );
#endif
			return data->node;
		}
		//Log( WIDE("No more data - clear find data stack...") );
		//findstack = NULL;
//		DeleteDataQueue( findstack );
		//DeleteDataStackEx( findstack DBG_RELAY );
		//LeaveCriticalSec( &csSpace );
		return NULL;	
	}
	//Log( WIDE("No get node result...") );
	//LeaveCriticalSec( &csSpace );
	return NULL;
}

	typedef struct {
		PDATASTACK pds;
		int bLocked;
		int bUsed;
	} FINDDATA, *PFINDDATA;

static FINDDATA fd[16];
//static int nDataFinder;

PSPACENODE FindRectInSpaceEx( PSPACENODE root
									, PSPACEPOINT min
									, PSPACEPOINT max
									, void **stack 
									DBG_PASS )
{
	int nDataFinder;
	PSPACENODE node;
	static uint32_t entering;
	static int bLocks;
	//Log( WIDE("No find stack (first call)") );
	// allow multiple people to do a find in the list
	// that is harmless... and lock it while all people finding are still busy

	if( !stack )
		return NULL; // no point, can't store results.

	if( root )
	{
		if((max[0] < min[0]) || (max[1] < min[1]))
		{
			Log( WIDE("Aborting on range failure.") );
			return NULL;
		}
	//lprintf( WIDE("Initial find started with a root.  Create a stack.") );
		do
		{
			for( nDataFinder = 0; nDataFinder < 16; nDataFinder++ )
				if( !fd[nDataFinder].bUsed )
				{
					(*stack) = fd + nDataFinder;
					fd[nDataFinder].bUsed = 1;
					break;
				}
			if( nDataFinder == 16 )
				Idle(); // let some other finder finish working...
		} while( nDataFinder == 16 );

		//(*stack) = fd + nDataFinder;//Allocate( sizeof( FINDDATA ) );
		if( !((PFINDDATA)(*stack))->pds )
			((PFINDDATA)(*stack))->pds = CreateDataStack(sizeof(FIND_RECT_DATA));
		EmptyDataStack( &((PFINDDATA)(*stack))->pds );
		((PFINDDATA)(*stack))->bLocked = 0;
	}
	if( !(*stack) )
		return NULL;
	while( LockedExchange( &entering, 1 ) )
		Relinquish();
	if( !bLocks )
	{
		//lprintf( WIDE("First find to start... %p %p"), root, *stack );
		EnterCriticalSec( &csSpace );
		((PFINDDATA)(*stack))->bLocked = 1;
	}
	if( root )
	{
		//_xlprintf( DBG_AVAILABLE DBG_RELAY )( WIDE("New stack... incrementing locks from %d to %d"), bLocks, bLocks + 1 );
		bLocks++;
	}
	entering = 0;

	//lprintf( WIDE("Begin finding a node...") );
	node = InternalFindRectInSpaceEx( root, min, max, &((PFINDDATA)(*stack))->pds, TRUE DBG_RELAY );
	//lprintf( WIDE("resulted with a node ... %p %p"), root, node );
	if( !node )
	{
		//_xlprintf(1 DBG_RELAY)( WIDE("Final node was returned from the stack. %ld %d")
		//		 , bLocks
		 ///  	 , ((PFINDDATA)(*stack))->bLocked );
		//DeleteDataStackEx( &((PFINDDATA)(*stack))->pds DBG_RELAY );
		while( LockedExchange( &entering, 1 ) )
			Idle();
		//_xlprintf( DBG_AVAILABLE DBG_RELAY )( WIDE("last node result... unlocking from %d to %d"), bLocks, bLocks - 1 );
		--bLocks;
		while( ((PFINDDATA)(*stack))->bLocked && bLocks )
		{
			entering = 0;
			//lprintf( WIDE("Waiting for another stupid thread is finding... waiting for him to finish.") );
			do
			{
				Relinquish();
			} while( LockedExchange( &entering, 1 ) );
		}
		if( !bLocks && ((PFINDDATA)(*stack))->bLocked )
		{
			//lprintf( WIDE("Final find leaving section.") );
			LeaveCriticalSec( &csSpace );
		}
		else
		{
			//lprintf( WIDE("Non final find leaving...%d,%d"), bLocks, ((PFINDDATA)(*stack))->bLocked );
		}

		if( bLocks && ((PFINDDATA)(*stack))->bLocked )
		{
			DebugBreak();
		}
		//Release( *stack );
		((PFINDDATA)(*stack))->bUsed = 0;
		(*stack) = NULL;
		entering = 0;
	}
	//_xlprintf( 1 DBG_RELAY )("Result node: %p", node );
	return node;
}

PSPACENODE GetNodeUnder( PSPACENODE node )
{
	if( node )
		return node->area[AREA_HERE];
	return NULL;
}

// when duplicating a node the new node has the parameters min
// and max... the old node is adjusted to exclude min, max

static PSPACENODE DuplicateSpaceNodeEx( PSPACENODE space DBG_PASS )
#define DuplicateSpaceNode(s) DuplicateSpaceNodeEx((s) DBG_SRC )
#define DupNodeEx DuplicateSpaceNodeEx
#define DupNode DuplicateSpaceNode
{
	PSPACENODE dup = GetFromSet( SPACENODE, SpaceNodes );
	MemSet( dup, 0, sizeof( SPACENODE ) );
	//Log1( WIDE("DuP!%d"), CountUsedInSet( SPACENODE, SpaceNodes )  );
	dup->data = space->data;
#ifdef PARTITION_SCREEN
	dup->min[0] = space->min[0];
	dup->min[1] = space->min[1];
	dup->max[0] = space->max[0];
	dup->max[1] = space->max[1];
#else
	SetPoint( dup->min, space->min );
	SetPoint( dup->max, space->max );
#endif
	if( ( dup->next = space->next ) )
		space->next->prior = dup;
	dup->prior = space;
	space->next = dup;
	//Log( WIDE("Duplicating Node...") );
	return dup;
}

// min and max are INCLUSIVE of
// all data.
static void HangSpaceNodeExx( PSPACENODE *root, PSPACENODE parent, PSPACENODE space DBG_PASS )
#define DBG_LEVEL DBG_SRC
{
	PSPACENODE here, dup;
	static int levels;
	if( !space || !root )
		return;
	//EnterCriticalSec( &csSpace );
#ifdef HANG_DEBUG
	Log5( WIDE("Adding area: min(%d,%d) max(%d,%d) level %d"), space->min[0], space->min[1], space->max[0], space->max[1], ++levels );
#endif
	while( ( here = *root ) )
	{
		//here->children += space->children + 1; // this WILL have one more child.
#ifdef HANG_DEBUG
		{
			//char name[256], rootname[256];
			//GetNameText( rootname, ((PSECTOR)here->data)->name );
			//GetNameText( name, ((PSECTOR)space->data)->name );
			Log10( WIDE("adding node (%08x)(%d,%d)-(%d,%d) under (%08x)(%d,%d)(%d,%d)")
								, space
								, space->min[0]
								, space->min[1]
								, space->max[0]
								, space->max[1]
								, here 
								, here->min[0]
								, here->min[1]
								, here->max[0]
								, here->max[1]
								);
		}
#endif
		if( space->max[0] < here->min[0] ) // space is left of here total.
		{
#ifdef HANG_DEBUG
			Log( WIDE("Total area is to the left.") );
#endif
			if( space->max[1] < here->min[1] ) // space is above this.
			{
#ifdef HANG_DEBUG
				Log1( WIDE("Total area to upper left....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_UPPERLEFT;
			}
			else if( space->min[1] > here->max[1] ) // space is below this.
			{
#ifdef HANG_DEBUG
				Log1( WIDE("Total area to lower left....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_LOWERLEFT;
			}
			else
			{
				// this_miny <= root_maxy
				// this_maxy >= root_miny
				if( space->min[1] < here->min[1] )
				{
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->max[1] = here->min[1] - 1;
#ifdef HANG_DEBUG
					Log1( WIDE("clipped top part, and hangning part is to upper left....%d"), __LINE__ );
#endif
					
					HangSpaceNodeExx( here->area + AREA_UPPERLEFT, here, dup DBG_LEVEL );
				}
				if( space->max[1] > here->max[1] )
				{
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->min[1] = here->max[1] + 1;
#ifdef HANG_DEBUG
					Log1( WIDE("clipped bottom part, hang clip to lower left....%d"), __LINE__ );
#endif
					HangSpaceNodeExx( here->area + AREA_LOWERLEFT, here, dup DBG_LEVEL );
				}
#ifdef HANG_DEBUG
				Log1( WIDE("This (remaining?) part is to the left....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_LEFT;
			}
		}
		else if( space->min[0] > here->max[0] ) // space is right of this one...
		{
#ifdef HANG_DEBUG
			Log( WIDE("Total area is to the right.") );
#endif
			// UPPER_RIGHT is handled above when space is above and right.
			if( space->min[1] > here->max[1] ) // space is below this one
			{
#ifdef HANG_DEBUG
				Log1( WIDE("Total area is right and below this....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_LOWERRIGHT;
			}
			else if( space->max[1] < here->min[1] ) // space is above this.
			{
#ifdef HANG_DEBUG
				Log1( WIDE("Total area to upper right....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_UPPERRIGHT;
			}
			else 
			{
				// this_miny < root_maxy
				// this_maxy > root_miny
				if( space->max[1] > here->max[1] )
				{
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->min[1] = here->max[1] + 1;
#ifdef HANG_DEBUG
					Log1( WIDE("This part is to lower right....%d"), __LINE__ );
#endif
					HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, here, dup DBG_LEVEL );
				}
				if( space->min[1] < here->min[1] )
				{
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->max[1] = here->min[1] - 1;
#ifdef HANG_DEBUG
					Log1( WIDE("This part is to upper right....%d"), __LINE__ );
#endif
					HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, here, dup DBG_LEVEL );
				}
#ifdef HANG_DEBUG
				Log1( WIDE("This (remaining?) part is to the right....%d"), __LINE__ );
#endif
				parent = here;
				root = here->area + AREA_RIGHT;
			}
		}
		else // space is somewhat at least within center left-right
		{
			if( space->min[0] < here->min[0] ) // space is partially to the left.
			{
				dup = DupNodeEx( space DBG_LEVEL ) ;
				if( !(space->prior) )
					space = DupNodeEx( space DBG_LEVEL );
				dup->max[0] = here->min[0] - 1;
				space->min[0] = here->min[0];
#ifdef HANG_DEBUG
				Log( WIDE("Clipped left peice off....") );
#endif
				{
					if( dup->max[1] < here->min[1] )
					{
#ifdef HANG_DEBUG
						Log( WIDE("Clipped peice is to the upper left of here") );
#endif
						HangSpaceNodeExx( here->area + AREA_UPPERLEFT, here, dup DBG_LEVEL );
					}
					else if( dup->min[1] > here->max[1] )
					{
#ifdef HANG_DEBUG
						Log( WIDE("Clipped peice is to the upper right of here") );
#endif
						HangSpaceNodeExx( here->area + AREA_LOWERLEFT, here, dup DBG_LEVEL );
					}
					else 
					{	
#ifdef HANG_DEBUG
						Log8( WIDE("Dup min(%d,%d) max(%d,%d) here min(%d,%d)  max(%d,%d)"), 
									dup->min[0], dup->min[1],
									dup->max[0], dup->max[1],
									here->min[0], here->min[1],
							  here->max[0], here->max[1] );
#endif
						if( dup->min[1] < here->min[1] )
						{
							PSPACENODE dup2 = DupNodeEx( dup DBG_LEVEL );
#ifdef HANG_DEBUG
							Log( WIDE("Clipped peice is up and left of here (hang clip)") );
#endif
							dup2->max[1] = here->min[1] - 1;
							dup->min[1] = here->min[1];
							HangSpaceNodeExx( here->area + AREA_UPPERLEFT, here, dup2 DBG_LEVEL );
						}
						if( dup->max[1] > here->max[1] )
						{
							PSPACENODE dup2 = DupNodeEx( dup DBG_LEVEL );
#ifdef HANG_DEBUG
							Log( WIDE("Clipped peice is below and left of here") );
#endif
							dup2->min[1] = here->max[1] + 1;
							dup->max[1] = here->max[1];
							HangSpaceNodeExx( here->area + AREA_LOWERLEFT, here, dup2 DBG_LEVEL );
						}
#ifdef HANG_DEBUG
						Log( WIDE("Remaining peice is to the left of here.") );
#endif
						HangSpaceNodeExx( here->area + AREA_LEFT, here, dup DBG_LEVEL );
					}
				}
			}
			if( space->max[0] > here->max[0] ) // space is partially to the right
			{
				dup = DupNodeEx( space DBG_LEVEL ) ;
				if( !(space->prior) )
					space = DupNodeEx( space DBG_LEVEL );
				dup->min[0] = here->max[0] + 1;
				space->max[0] = here->max[0];
#ifdef HANG_DEBUG
				Log( WIDE("Clipped right peice off...") );
#endif
				{
					if( dup->max[1] < here->min[1] )
					{
#ifdef HANG_DEBUG
						Log( WIDE("Clipped peice is to the upper right of here") );
#endif
						HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, here, dup DBG_LEVEL );
					}
					else if( dup->min[1] > here->max[1] )
					{
#ifdef HANG_DEBUG
						Log( WIDE("Clipped peice is to the lower right of here") );
#endif
						HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, here, dup DBG_LEVEL );
					}
					else 
					{	
						if( dup->min[1] < here->min[1] )
						{
							PSPACENODE dup2 = DupNodeEx( dup DBG_LEVEL );
							dup2->max[1] = here->min[1] - 1;
							dup->min[1] = here->min[1];
#ifdef HANG_DEBUG
							Log( WIDE("Clipped peice is to the right and up of here") );
#endif
							HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, here, dup2 DBG_LEVEL );
						}
						if( dup->max[1] > here->max[1] )
						{
							PSPACENODE dup2 = DupNodeEx( dup DBG_LEVEL );
							dup2->min[1] = here->max[1] + 1;
							dup->max[1] = here->max[1];
#ifdef HANG_DEBUG
							Log( WIDE("Clipped peice is to the right and below of here") );
#endif
							HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, here, dup2 DBG_LEVEL );
						}
#ifdef HANG_DEBUG
						Log( WIDE("Remaining peice is to the right of here") );
#endif
						HangSpaceNodeExx( here->area + AREA_RIGHT, here, dup DBG_LEVEL );
					}
				}
			}
			// here the space is left to right justified above/here/below this
			// no part is left or right.
			// 
			if( space->max[1] < here->min[1] )
			{
#ifdef HANG_DEBUG
				Log( WIDE("Total peice remaining is up from here") );
#endif
				parent = here;
				root =here->area + AREA_UP;
			}
			else if( space->min[1] > here->max[1] )
			{
#ifdef HANG_DEBUG
				Log( WIDE("Total peice remaining is down from here") );
#endif
				parent = here;
				root =here->area + AREA_DOWN;
			}
			else
			{

				if( space->min[1] < here->min[1] )
				{
#ifdef HANG_DEBUG
					Log( WIDE("Part of this is up") );
#endif
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->max[1] = here->min[1] - 1;
					space->min[1] = here->min[1];
					HangSpaceNodeExx( here->area + AREA_UP, here, dup DBG_LEVEL );
				}
				if( space->max[1] > here->max[1] )
				{
#ifdef HANG_DEBUG
					Log( WIDE("Part of this is down") );
#endif
					dup = DupNodeEx( space DBG_LEVEL );
					if( !(space->prior) )
						space = DupNodeEx( space DBG_LEVEL );
					dup->min[1] = here->max[1] + 1;
					space->max[1] = here->max[1];
					HangSpaceNodeExx( here->area + AREA_DOWN, here, dup DBG_LEVEL );
				}
#ifdef HANG_DEBUG
				Log( WIDE("Any (remaining?) part is HERE") );
#endif
				parent = here;
				root = here->area + AREA_HERE;
			}
		}
	}

	{
#ifdef HANG_DEBUG
		Log2( WIDE(" -- Added space node %08x %08x-- "), space, parent );
#endif
		space->parent = parent;
		*root = space;
		space->me = root;
	}
	levels--;
	//LeaveCriticalSec( &csSpace );
}

PSPACENODE ReAddSpaceNodeEx( PSPACENODE *root, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max DBG_PASS )
{
#ifdef PARTITION_SCREEN
	node->min[0] = min[0];
	node->min[1] = min[1];
	node->max[0] = max[0];
	node->max[1] = max[1];
#else
	SetPoint( node->min, min );
	SetPoint( node->max, max );
#endif
	HangSpaceNodeExx( root, NULL, node DBG_RELAY );
	return node;
}

PSPACENODE AddSpaceNodeEx( PSPACENODE *root, void *data, PSPACEPOINT min, PSPACEPOINT max DBG_PASS )
{
	PSPACENODE space;
	if( !data )
		Log( WIDE("Storing NULL data?!") );
#ifdef HANG_DEBUG
	Log1( WIDE("Adding node for %08x"), data );
#endif
	if( !root )
	{
		Log( WIDE("Cannot add a node to an unreferenced tree (root = NULL)") );
		return NULL; // can't add to a non tree...
	}
	//Log1( WIDE("Add Space Node : %d"), CountUsedInSet( SPACENODE, SpaceNodes ) );
	space = GetFromSet( SPACENODE, SpaceNodes );
	MemSet( space, 0, sizeof( SPACENODE ) );
#ifdef PARTITION_SCREEN
	space->min[0] = min[0];
	space->min[1] = min[1];
	space->max[0] = max[0];
	space->max[1] = max[1];
#else
	SetPoint( space->min, min );
	SetPoint( space->max, max );
#endif
	space->data = data;
#ifdef HANG_DEBUG
	{
		char name[256];
		//GetNameText( name, ((PSECTOR)data)->name );
		Log1( WIDE("---- Building nodes for %p ----"), data );
	}
#endif
	EnterCriticalSec( &csSpace );
	HangSpaceNodeExx( root, NULL, space DBG_RELAY );
	LeaveCriticalSec( &csSpace );
	return space; // to attach the node to the related data....
}

void MoveSpace( PSPACENODE *root, PSPACENODE space, PSPACEPOINT min, PSPACEPOINT max )
{
	RemoveSpaceNode( space );
#ifdef PARTITION_SCREEN
	space->min[0] = min[0];
	space->min[1] = min[1];
	space->max[0] = max[0];
	space->max[1] = max[1];
#else
	SetPoint( space->min, min );
	SetPoint( space->max, max );
#endif
	HangSpaceNode( root, space );
}

void DeleteSpaceNode( PSPACENODE node )
{
	//Log( WIDE("Delete Node?") );
	if( node )
	{
		DeleteFromSet( SPACENODE, SpaceNodes, node );
	}
}

void DeleteSpaceTree( PSPACENODE *root )
{
	static int del_level;
	int i;
	PSPACENODE here;
	if( !del_level )
	{
		// any dirty nodes queued are now invalid.
		EnterCriticalSec( &csSpace );
		DeleteList( &pDirtyNodes );
		//lprintf( WIDE("pDirtyNodes = %p"), pDirtyNodes );
	}
	del_level++;
	if( *root )
	{
		//Log( WIDE("Deleting a node... "));
		here = *root;
		*root = NULL;
		for( i = 0; i < AREAS; i++ )
			DeleteSpaceTree( here->area+i );
		if( here->prior && !here->prior->prior )
			DeleteFromSet( SPACENODE, SpaceNodes, here->prior );
  		DeleteFromSet( SPACENODE, SpaceNodes, here );
		here = NULL;
	}
	if( !(--del_level) )
		LeaveCriticalSec( &csSpace );
}

PSPACEPOINT GetSpaceMin( PSPACENODE node )
{
	if( node )
		return node->min;
	return NULL;
}

PSPACEPOINT GetSpaceMax( PSPACENODE node )
{
	if( node )
		return node->max;
	return NULL;
}

PSPACENODE GetFirstRelativeSpace( PSPACENODE node )
{
	while( node && node->prior )
		node = node->prior;
	return node;
}

PSPACENODE GetNextRelativeSpace( PSPACENODE node )
{
	if( node )
		node = node->next;
	return node;
}

static int maxlevels, levels, nodecount;
PSPACENODE WriteNode;

int CountSpaces( PSPACENODE space )
{
	PSPACENODE first = space;
	int cnt;
	while( first->prior )
		first = first->prior;
	cnt = 1;
	while( first->next )
		( first = first->next ), cnt++;
	return cnt;
}

void WriteSpaceNodeEx( uintptr_t psv, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
{
	Log4( WIDE("(%d) Node(%p) under %p has %d parts")
					, levels, WriteNode, WriteNode->parent, CountSpaces( WriteNode ) );
#ifdef PARTITION_SCREEN
	Log5( WIDE("(%") _32f WIDE(",%") _32f WIDE(")-(%") _32f WIDE(",%") _32f WIDE(") data %p")
					, min[0]
					, min[1]
					, max[0]
					, max[1]
		 , node?node->data:(POINTER)psv );
#else
	Log7( WIDE("(%g,%g,%g)-(%g,%g,%g) data %08x")
					, min[0]
					, min[1]
					, min[2]
					, max[0]
					, max[1]
					, max[2]
		 , node?node->data:psv );
#endif
}

void WriteSpaceNode( POINTER data, PSPACEPOINT min, PSPACEPOINT max )
{
	WriteSpaceNodeEx( (uintptr_t)data, 0, min, max );

}

void WalkSpaceTreeEx( PSPACENODE root
						  , void (*Callback)( uintptr_t psv, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
						  , uintptr_t psv )
{
	int i;
	//if( levels > 5 )
	//	return;
	if( !root )
		return;
	nodecount++;
	levels++;
	if( levels > maxlevels )
		maxlevels = levels;
	
	WriteNode = root;
	if( Callback )
		Callback( psv, root, root->min, root->max );
	{
		for( i = 0; i < AREAS; i++ )
		{
			if( i == AREA_HERE )
				continue;
			if( root->area[i] )
			{
				if( Callback == WriteSpaceNodeEx )
					Log1( WIDE("Q%d:"), i );
				WalkSpaceTreeEx( root->area[i], Callback, psv );
		  	}
		}
	}
	levels--;
}

void WalkSpaceTree( PSPACENODE root
						, void (*Callback)( POINTER data, PSPACEPOINT min, PSPACEPOINT max ) )
{
	int i;
	//if( levels > 5 )
	//	return;
	if( !root )
		return;
	nodecount++;
	levels++;
	if( levels > maxlevels )
		maxlevels = levels;
	
	WriteNode = root;
	if( Callback )
		Callback( root->data, root->min, root->max );
	{
		for( i = 0; i < AREAS; i++ )
		{
			if( root->area[i] )
			{
				if( Callback == WriteSpaceNode )
					Log1( WIDE("Q%d:"), i );
				WalkSpaceTree( root->area[i], Callback );
		  	}
		}
	}
	levels--;
}

void BrowseSpaceTreeEx( PSPACENODE root
							 , void (*Callback)( uintptr_t psv, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
							 , uintptr_t psv )
{
	maxlevels = 0;
	levels = 0;
	nodecount = 0;
	if( !Callback )
		Callback = WriteSpaceNodeEx;
	WalkSpaceTreeEx( root, Callback, psv );
	if( Callback == WriteSpaceNodeEx )
	{
		Log2( WIDE("Max levels of nodes: %d total: %d")
						, maxlevels
						, nodecount
				);
		Log( WIDE("----------------------------------------") );
	}	
}

void BrowseSpaceTree( PSPACENODE root
							, void (*Callback)( POINTER data, PSPACEPOINT min, PSPACEPOINT max ) )
{
	maxlevels = 0;
	levels = 0;
	nodecount = 0;
	if( !Callback )
		Callback = WriteSpaceNode;
	WalkSpaceTree( root, Callback );
	if( Callback == WriteSpaceNode )
	{
		Log2( WIDE("Max levels of nodes: %d total: %d")
						, maxlevels
						, nodecount
				);
		Log( WIDE("----------------------------------------") );
	}	
}


struct table_entry
{

	struct span {
		int rows, cols;
	} span;
} TABLE_CELL;
typedef struct image_table
{
	int rows, cols;
	int *row_height;
	int *col_width;
	PLIST row_cells; // list of PLISTS of cols of cells??
} IMAGE_TABLE, *PIMAGE_TABLE;


void BuildTableGrid( uintptr_t psv, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
{
	PIMAGE_TABLE table = (PIMAGE_TABLE)psv;
	int i,f,m; // initial, final, mid
	i = 0;
	f = table->cols - 1;
		  xlprintf(LOG_ALWAYS)( WIDE("Node %p is %")_32fs WIDE(",%") _32fs WIDE(" - %") _32fs WIDE(",%") _32fs, node, min[0], min[1], max[0], max[1] );
	while( i <= f )
	{
		m = (f+i)/2;
		if( min[0] < table->col_width[m] )
			f = m-1;
		else if( min[0] > table->col_width[m] )
			i = m+1;
		else
			break;
	}

	if( i > f ) // didn't find it, and M is
	{
		int n;
		int *new_widths = NewArray( int, table->cols+1 );
		lprintf( WIDE( "New min[0]" ) );
		table->cols++;
		for( n = 0; n < table->cols; n++ )
		{
			if( n == (table->cols-1) || min[0] < table->col_width[n] )
			{
				new_widths[n++] = min[0];
				for( ; n < table->cols; n++ )
				{
					new_widths[n] = table->col_width[n-1];
				}
				Release( table->col_width );
				table->col_width = new_widths;
				break;
			}
			else
			{
				new_widths[n] = table->col_width[n];
			}
		}
	}


	i = 0;
	f = table->cols - 1;
	while( i <= f )
	{
		m = (f+i)/2;
		if( (max[0]+1) < table->col_width[m] )
			f = m-1;
		else if( (max[0]+1) > table->col_width[m] )
			i = m+1;
		else
			break;
	}
	if( i > f ) // didn't find it, and M is
	{
		int n;
		int *new_widths = NewArray( int, table->cols+1 );
		lprintf( WIDE( "New max[0]" ) );
		table->cols++;
		for( n = 0; n < table->cols; n++ )
		{
			if( n == (table->cols-1) || (max[0]+1) < table->col_width[n] )
			{
				new_widths[n++] = max[0]+1;
				for( ; n < table->cols; n++ )
				{
					new_widths[n] = table->col_width[n-1];
				}
				Release( table->col_width );
				table->col_width = new_widths;
				break;
			}
			else
				new_widths[n] = table->col_width[n];
		}
	}



	i = 0;
	f = table->rows - 1;
	while( i <= f )
	{
		m = (f+i)/2;
		if( min[1] < table->row_height[m] )
			f = m-1;
		else if( min[1] > table->row_height[m] )
			i = m+1;
		else
			break;
	}

	if( i > f ) // didn't find it, and M is
	{
		int n;
		int *new_heights = NewArray( int, (table->rows+1) );
		lprintf( WIDE( "New min[1]" ) );
		table->rows++;
		for( n = 0; n < table->rows; n++ )
		{
			if( n == (table->rows-1) || min[1] < table->row_height[n] )
			{
				new_heights[n++] = min[1];
				for( ; n < table->rows; n++ )
				{
					new_heights[n] = table->row_height[n-1];
				}
				Release( table->row_height );
				table->row_height = new_heights;
				break;
			}
			else
				new_heights[n] = table->row_height[n];
		}
	}


	i = 0;
	f = table->rows - 1;
	while( i <= f )
	{
		m = (f+i)/2;
		if( (max[1]+1) < table->row_height[m] )
			f = m-1;
		else if( (max[1]+1) > table->row_height[m] )
			i = m+1;
		else
			break;
	}
	if( i > f ) // didn't find it, and M is
	{
		int n;
		int *new_heights = NewArray( int, (table->rows+1) );
		table->rows++;
		lprintf( WIDE( "New max[1]" ) );
		for( n = 0; n < table->rows; n++ )
		{
			if( n == (table->rows-1) || (max[1]+1) < table->row_height[n] )
			{
				new_heights[n++] = (max[1]+1);
				for( ; n < table->rows; n++ )
				{
					new_heights[n] = table->row_height[n-1];
				}
				Release( table->row_height );
				table->row_height = new_heights;
				break;
			}
			else
				new_heights[n] = table->row_height[n];
		}
	}
}

void FitTableGrid( uintptr_t psv, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
{
	PIMAGE_TABLE table = (PIMAGE_TABLE)psv;

//cpg26Dec2006   int n;
	int i,f,m, m2; // initial, final, mid, mid2
	// mid is the col search result
	// mid2 is the row search result.
	//int i,f,m; // initial, final, mid
	i = 0;
	f = table->cols - 1;
	while( i <= f )
	{
		m = (f+i)/2;
		if( min[0] < table->col_width[m] )
			f = m-1;
		else if( min[0] > table->col_width[m] )
			i = m+1;
		else
		{
			break;
			// no modification.
		}
	}
	if( i > f )
		DebugBreak();

	i = 0;
	f = table->rows - 1;
	while( i <= f )
	{
		m2 = (f+i)/2;
		if( min[1] < table->row_height[m2] )
			f = m2-1;
		else if( min[1] > table->row_height[m2] )
			i = m2+1;
		else
		{
			break;
			// no modification.
		}
	}
	if( i > f )
		DebugBreak();
	{
		int n, n2;
		for( n = m; n < table->cols; n++ )
		{
			if( max[0] < table->col_width[n] )
				break;
		}
		for( n2 = m2; n2 < table->rows; n2++ )
		{
			if( max[1] < table->row_height[n2] )
				break;
		}
		if( n < table->cols && n2 < table->cols )
		{
			if( !GetLink( &table->row_cells, m2 ) )
				SetLink( &table->row_cells, m2, CreateList() );
			node->col_span = n - m;
			node->row_span = n2 - m2;
			lprintf( WIDE( "Set image %p(%p) section %d,%d to width %d,%d" ), node, node->data, m, m2, node->col_span, node->row_span );
			SetLink( (PLIST*)GetLinkAddress( &table->row_cells, m2 ), m, node );
		}
		else
		{
			// should never get here.
			DebugBreak();
		}
	}
}

void GlobTableGrid( PIMAGE_TABLE table )
{
	int row, col;//cpg26Dec2006, _col;
	PLIST *cell_row;
	PLIST *_cell_row = NULL;
	PSPACENODE _cell;
	PSPACENODE cell;
	int matched;
	//do
	{
		matched = 0;
		for( row = 0; row < table->rows; row++ )
		{
			int matched2;
			int row2;
			_cell_row = (PLIST*)GetLinkAddress( &table->row_cells, row );
			for( row2 = row + 1; row2 < table->rows; row2++ )
			{
				cell_row = (PLIST*)GetLinkAddress( &table->row_cells, row2 );
				if( cell_row && (*cell_row ) )
				{
					matched2 = 0;
					for( col = 0; col < table->cols; col++ )
					{
						cell = (PSPACENODE)GetLink( cell_row, col );
						_cell = (PSPACENODE)GetLink( _cell_row, col );
						if( cell && _cell )
						{
							if( cell->data == _cell->data )
								if( cell->col_span == _cell->col_span )
								{
									_cell->row_span += cell->row_span;
									SetLink( cell_row, col, NULL );
									matched++;
									matched2++;
								}
						}
					}
					if( !matched2 )
						break;
				}
			}
		}
		//break;
#if 0
		// this was the OLD way... developed the above search for same data types
		for( row = 0; row < table->rows; row++ )
		{
			_cell = NULL;
			cell_row = (PLIST*)GetLinkAddress( &table->row_cells, row );
			for( col = 0; col < table->cols; col++ )
			{
				cell = (PSPACENODE)GetLink( cell_row, col );
				if( cell )
				{
					if( _cell )
					{
						if( _cell->data == cell->data && _cell->row_span == cell->row_span )
						{
							_cell->col_span += cell->col_span;
							SetLink( cell_row, col, NULL );
							matched++;
						}
						else
							_cell = cell;
					}
					else
						_cell = cell;
				}
			}
			_cell_row = cell_row;
		}
#endif
	}
		//while( matched );
}


void OutputHTMLSpaceTable( PSPACENODE root
								 , PVARTEXT pvt_output
								 , void (*Callback)(uintptr_t psv, PVARTEXT pvt, POINTER node_data, PSPACEPOINT min, PSPACEPOINT max )
								 , uintptr_t psv )
{
	PIMAGE_TABLE table;
	while( root->parent )
		root = root->parent;
	table = (PIMAGE_TABLE)Allocate( sizeof( *table ) );
	table->rows = 0;
	table->cols = 0;
	table->row_height = 0;
	table->col_width = 0;
	table->row_cells = NULL;
	BrowseSpaceTreeEx( root, BuildTableGrid, (uintptr_t)table );
	{
		int n;
		for( n = 0; n < table->cols; n++ )
		{
			lprintf( WIDE( "col %d is %d" ), n, table->col_width[n] );
		}
		for( n = 0; n < table->rows; n++ )
		{
			lprintf( WIDE( "row %d is %d" ), n, table->row_height[n] );
		}
	}
	BrowseSpaceTreeEx( root, FitTableGrid, (uintptr_t)table );
	{
		INDEX row, col;
		PLIST cols;
		vtprintf( pvt_output, WIDE( "<table border=\"1\">\r\n" ) );
		{
			int n;

			vtprintf( pvt_output, WIDE( "<colgroup>" ) );
			for( n = 0; n < (table->cols-1); n++ )
			{
				vtprintf( pvt_output, WIDE( "<col width=\"%d\">" ), (table->col_width[n+1]-table->col_width[n]) * 10 );
			}
			vtprintf( pvt_output, WIDE( "</colgroup>\n" ) );
		}
		LIST_FORALL( table->row_cells, row, PLIST, cols )
		{
			PSPACENODE node;
			vtprintf( pvt_output, WIDE( "<tr>" ) );
			LIST_FORALL( cols, col, PSPACENODE, node )
			{
				vtprintf( pvt_output, WIDE( "<td data=\"%08X\" bgcolor=\"#%06X\" colspan=\"%d\" rowspan=\"%d\">X" )
							, node->data
							, rand() << 6
						  , node->col_span, node->row_span );
				{
					SPACEPOINT min, max;
					min[0] = table->col_width[col];
					min[1] = table->row_height[row];
					max[0] = table->col_width[col+node->col_span];
					max[1] = table->row_height[row+node->row_span];
					Callback( psv, pvt_output, node->data, min, max );
				}
				vtprintf( pvt_output, WIDE( "</td>" ) );
			}
			// so what if this deletes the wrong reference?
			vtprintf( pvt_output, WIDE( "</tr>\n" ) );
		}
		vtprintf( pvt_output, WIDE( "</table><br>\n" ) );
	}

	GlobTableGrid( table );
	{
		INDEX row, col;
		PLIST cols;
		int output_row = 0;
		vtprintf( pvt_output, WIDE( "<table border=\"1\">\r\n" ) );
		{
			int n;

			vtprintf( pvt_output, WIDE( "<colgroup>" ) );
			for( n = 0; n < (table->cols-1); n++ )
			{
				vtprintf( pvt_output, WIDE( "<col width=\"%d\">" ), (table->col_width[n+1]-table->col_width[n]) * 10 );
			}
			vtprintf( pvt_output, WIDE( "</colgroup>" ) );
		}
		LIST_FORALL( table->row_cells, row, PLIST, cols )
		{
			PSPACENODE node;
			output_row = 0;
			LIST_FORALL( cols, col, PSPACENODE, node )
			{
				if( !output_row )
				{
					vtprintf( pvt_output, WIDE( "<tr>" ) );
					output_row = 1;
				}
				vtprintf( pvt_output, WIDE( "<td data=\"%08X\" bgcolor=\"#%06X\" colspan=\"%d\" rowspan=\"%d\">Y" )
							, node->data
							, rand() << 6
						  , node->col_span, node->row_span );
				{
					SPACEPOINT min, max;
					min[0] = table->col_width[col];
					min[1] = table->row_height[row];
					max[0] = table->col_width[col+node->col_span];
					max[1] = table->row_height[row+node->row_span];
					Callback( psv, pvt_output, node->data, min, max );
				}
				vtprintf( pvt_output, WIDE( "</td>" ) );
			}
			// so what if this deletes the wrong reference?
			DeleteList( &cols );
			if( output_row )
				vtprintf( pvt_output, WIDE( "</tr>\n" ) );
		}
		vtprintf( pvt_output, WIDE( "</table>" ) );
		DeleteList( &table->row_cells );
		Release( table->col_width );
		Release( table->row_height );
		Release( table );

	}
}




SPACETREE_NAMESPACE_END








//---------------------------------------------------------------------
// $Log: spacetree.c,v $
// Revision 1.35  2005/06/24 15:11:46  jim
// Merge with branch DisplayPerformance, also a fix for watcom compilation
//
// Revision 1.34.2.3  2005/06/23 20:24:57  jim
// Mdoified config file to allow loading of fingerdimple to actually work.  Fix mouse updates (and cured redudnant updates).  Fixed background updates... and all seems well and fairly log free.
//
// Revision 1.34.2.2  2005/06/22 23:12:10  jim
// checkpoint...
//
// Revision 1.34.2.1  2005/06/22 17:31:43  jim
// Commit test optimzied display
//
// Revision 1.37  2005/06/19 05:15:39  d3x0r
// Add copying the panel flag bit to adopted images, and clearing of orphaned images... debugging still included...
//
// Revision 1.36  2005/06/17 21:29:15  d3x0r
// Checkpoint... Seems that dirty rects can be used to minmize drawing esp. around moving/rsizing of windows
//
// Revision 1.35  2005/05/31 21:57:00  d3x0r
// Progress optimizing redraws... still have several issues...
//
// Revision 1.34  2005/05/30 12:31:37  d3x0r
// Cleanup some extra allocations...
//
// Revision 1.33  2005/05/25 16:50:12  d3x0r
// Synch with working repository.
//
// Revision 1.34  2005/05/18 23:45:17  jim
// Fix font transport... begin validation code for valid display things given from the client... ... need to do more checking all over.
//
// Revision 1.33  2005/05/12 21:04:42  jim
// Fixed several conflicts that resulted in various deadlocks.  Also endeavored to clean all warnings.
//
// Revision 1.32  2004/08/17 07:10:54  d3x0r
// Okay looks like it works under linux (sorta) boundrys fail.
//
// Revision 1.31  2004/08/17 02:28:41  d3x0r
// Hmm seems to be an issue with update of focused complex surfaces... (palette).  Removed logging code (commented).  Seems that mouse render works all over, without failure, drag and draw work...
//
// Revision 1.30  2004/08/17 01:17:32  d3x0r
// Looks like new drawing check code works very well.  Flaw dragging from top left to bottom right for root panel... can lose mouse ownership, working on this problem... fixed export of coloraverage
//
// Revision 1.29  2004/06/12 09:17:59  d3x0r
// Remove unused logging file...
//
// Revision 1.28  2003/12/15 09:34:13  panther
// Need to fix spacetree tracking more - need to lockout more than one scanner at a time.
// Fixed issues with settting an image boundary and blotting
// images into said rect.
//
// Revision 1.27  2003/10/18 04:52:49  panther
// Include timers.h to get enter/leave critical sec
//
// Revision 1.26  2003/04/11 16:03:53  panther
// Added  LogN for gcc.  Fixed set code to search for first available instead of add at end always.  Added MKCFLAGS MKLDFLAGS for lnx makes.
// Fixed target of APP_DEFAULT_DATA.
// Updated display to use a meta buffer between for soft cursors.
//
// Revision 1.25  2003/03/31 10:55:09  panther
// Removed much logging.  Fixed closing, introduced locks to spacetree
//
// Revision 1.24  2003/03/31 04:18:49  panther
// Okay - drawing dispatch seems to work now.  Client/Server timeouts occr... rapid submenus still fail
//
// Revision 1.23  2003/03/31 02:07:05  panther
// fix returning root root images...
//
// Revision 1.22  2003/03/31 01:11:28  panther
// Tweaks to work better under service application
//
// Revision 1.21  2003/03/30 21:18:44  panther
// Removed logging define
//
// Revision 1.20  2003/03/30 21:14:52  panther
// Fix release first node of list space nodes (total rect)
//
// Revision 1.19  2003/03/30 18:20:35  panther
// Spacetree fixes.
//
// Revision 1.18  2003/03/30 18:19:55  panther
// - Added comment - fixed browsing functions!
// Panel stacking issues...
//
// Revision 1.17  2003/03/29 04:12:10  panther
// Debugging draw/focus mathods
//
// Revision 1.16  2003/02/18 06:23:46  panther
// Improved key mappings - removed some logging
//
// Revision 1.15  2003/02/17 02:58:23  panther
// Changes include - better support for clipped images in displaylib.
// More events handled.
// Modifications to image structure and better unification of clipping
// ideology.
// Better definition of image and render interfaces.
// MUCH logging has been added and needs to be trimmed out.
//
// Revision 1.14  2003/02/11 07:56:04  panther
// Minor display patches/test program, server
//
// Revision 1.13  2003/02/10 01:23:53  panther
// Well - collect mouse and redraw events.  Collect inbound mouse evevents on service side
//
// Revision 1.12  2002/12/13 14:59:40  dave
// Just added a return on a trivial case in findRectWhatever to keep from recursing
// on itself
//
// Revision 1.11  2002/12/12 15:01:44  panther
// Fix Image structure mods.
//
// Revision 1.10  2002/11/16 16:36:13  panther
// Another stage - default panel mouse handler to handle 'move'
// now - let us try playing with SDL in framebuffer/svga mode.
//
// Revision 1.9  2002/11/06 12:43:01  panther
// Updated display interface method, cleaned some code in the display image
// interface.  Have to establish who own's 'focus' and where windows
// are created.  The creation method REALLY needs the parent's window.  Which
// is a massive change (kinda)
//
// Revision 1.8  2002/11/06 11:06:16  panther
// Patches to compile under linux - need to fix dynamic library loading.
//
// Revision 1.7  2002/11/06 09:50:01  panther
// Can now more, and find rectangles on the screen.  Next - to click on
// and re-order windows.  Also be able to move other windows.
//
// Revision 1.6  2002/11/05 09:55:55  panther
// Seems to work, added a sample configuration file.
// Depends on some changes in configscript.  Much changes accross the
// board to handle moving windows... now to display multiples.
//
//
