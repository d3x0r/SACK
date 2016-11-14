// create a 'main' for this project...
#define BUILD_WEB_TESTER
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <vectlib.h>
#include <image.h>

#ifdef __ANDROID__
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif
#include <render3d.h>

#define DEBUG_MIGRATE 1
#define CS_PROTECT_NODES
// should be able to parallel search on a significantly large
// collection.
//#endif

typedef struct spaceweb SPACEWEB, *PSPACEWEB;
typedef struct spaceweb_node SPACEWEB_NODE, *PSPACEWEB_NODE;
typedef struct spaceweb_link SPACEWEB_LINK, *PSPACEWEB_LINK;
typedef struct spaceweb_link_data SPACEWEB_LINK_DATA, *PSPACEWEB_LINK_DATA;

struct spaceweb_link_data
{
	//PSPACEWEB_NODE primary;
	//PSPACEWEB_NODE secondary;
	//int valence;
	int paint; // when drawing.. mark the links as drawn.
	RAY plane;
	PSPACEWEB_LINK from;
	PSPACEWEB_LINK to;
	//LOGICAL edge;
};

struct spaceweb_link
{
	PSPACEWEB_NODE node;
	// plus characteristics of the link may imply... like
	//  target link - solves as a distant relative, something I might like to get to
	//  elasticity, spring, gravitation, repulsion
	//  the link is shared between the two nodes.
	PSPACEWEB_LINK_DATA data; // shared between both - links exest on both sides independantly... reduces need compare if (this or other)

	LOGICAL invert; 
};

struct spaceweb_node
{
	_POINT point;
	RCOORD t; // internal usage... keeps last computed t
	struct {
		BIT_FIELD bLinked : 1;
		BIT_FIELD bDeleted : 1;
		BIT_FIELD bBucketed : 1;
		BIT_FIELD bNegative : 1; // two negatives are a NULL; two postiives are a NULL; a positive is allowed entity space.
	} flags;
	// island is a debug check - it's a color to make sure all nodes are within the web.
	// all nodes are contained in a seperate list for debug purposes...
	uintptr_t island;
	uint32_t paint;
	uint32_t near_count;
	uint32_t id;
	// keep this, but migrate the name... otherwise we'll miss translation points...
	//PLIST near_nodes;  // link through an edge

	PLIST links;

#ifdef CS_PROTECT_NODES
	//
	CRITICALSECTION cs;
#endif
	PSPACEWEB web; // have to find root node to find islands (please make this go awawy)
	uintptr_t data;
};
#define MAXSPACEWEB_NODESPERSET 256 
DeclareSet( SPACEWEB_NODE );

#define NodeIndex( node ) GetMemberIndex( SPACEWEB_NODE, &node->web->nodes, node )
#define LinkIndex( link ) GetMemberIndex( SPACEWEB_NODE, &link->node->web->links, node )

#define MAXSPACEWEB_LINKSPERSET (6*MAXSPACEWEB_NODESPERSET)
DeclareSet( SPACEWEB_LINK );
#define MAXSPACEWEB_LINK_DATASPERSET (MAXSPACEWEB_LINKSPERSET/2)
DeclareSet( SPACEWEB_LINK_DATA );

struct spaceweb
{
	CRITICALSECTION cs;
	PSPACEWEB_NODESET nodes;
	PSPACEWEB_NODE root;
	PSPACEWEB_LINKSET links;
	PSPACEWEB_LINK_DATASET link_data;
};

/*
struct spaceweb_bucket
{
	PLIST nodes;
};

struct spaceweb_cursor
{
	PSPACEWEB web;
	PSPACEWEB_NODE current;
};
*/



int CountNear( PSPACEWEB_NODE node )
{
	int c = 0;
	INDEX idx;
	POINTER tmp;
	LIST_FORALL( node->links, idx, POINTER, tmp ) c++;
	return c;
}


//--------------------------------------------------------------------------------------------
// tests and validations
//--------------------------------------------------------------------------------------------

static int count;
uintptr_t CPROC IsOrphan( void* thisnode, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)thisnode;
	if( node->flags.bLinked )
	{
		count++;
		{
			int c = CountNear( node );
			return (c == 0)?(uintptr_t)thisnode:0;
		}
	}
	return 0;
}

void FindOrphans( PSPACEWEB web )
{
	static int ok = 0;
	PSPACEWEB_NODE orphan;
	count = 0;
	if( orphan = (PSPACEWEB_NODE)ForAllInSet( SPACEWEB_NODE, web->nodes, IsOrphan, 0 ) )
	{
		if( ok )
 			DebugBreak();
	}
	// if we count to more than 1... then we can break if fail.
	if( count > 1 )
		ok = 1;
}
//--------------------------------------------------------------------------------------------

uintptr_t CPROC IsIsland( void* thisnode, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)thisnode;

	// seek root...
	if( node->island && node->island != psv )
	{
		lprintf( WIDE("Node %p not signed."), node );
		return (uintptr_t)node;
	}
	return 0;
}

void SignIsland( PSPACEWEB_NODE node, uintptr_t value )
{
	if( node->flags.bLinked )
	{
		if( node->island == value )
			return;
#if ( DEBUG_ALL )
		lprintf( WIDE("node %p is island %d"), node, value );
#endif
		//EnterCriticalSec( &node->cs );
		node->island = value;
		{
			INDEX idx;
			PSPACEWEB_LINK tmp;
			LIST_FORALL( node->links, idx, PSPACEWEB_LINK, tmp )
			{
				SignIsland( tmp->node, value );
			}
		}
	}
}

void UnsignIsland( PSPACEWEB_NODE node, uintptr_t old_value, uintptr_t value )
{
	if( node->flags.bLinked )
	{
		if( node->island != old_value )
			return;
#if ( DEBUG_ALL )
		lprintf( WIDE("node %p is island %d"), node, value );
#endif
		node->island = value;
		//LeaveCriticalSec( &node->cs );
		{
			INDEX idx;
			PSPACEWEB_LINK tmp;
			LIST_FORALL( node->links, idx, PSPACEWEB_LINK, tmp )
			{
				// I just signed these...
				if( tmp->node->island == (old_value) )
					UnsignIsland( tmp->node, old_value, value );
			}
		}
	}
}
//--------------------------------------------------------------------------------------------

LOGICAL FindIslands( PSPACEWEB_NODE node )
{
	static uint32_t zzz;
	PSPACEWEB web = node->web;
	PSPACEWEB_NODE orphan;
	count = 0;
	zzz++;
	SignIsland( web->root, zzz );
	//zzzz = zzz++;
	if( orphan = (PSPACEWEB_NODE)ForAllInSet( SPACEWEB_NODE, web->nodes, IsIsland, zzz ) )
	{
		//UnsignIsland( web->root, zzzz, zzz );
		return TRUE;
		//DebugBreak();
	}
	//UnsignIsland( web->root, zzzz, zzz );
	return FALSE;
	// if we count to more than 1... then we can break if fail.
	//if( count > 1 )
	//   ok = 1;
}

//--------------------------------------------------------------------------------------------


PSPACEWEB_NODE FindNearest( PLIST *nodes, PLIST *came_from, PSPACEWEB_NODE from, PCVECTOR to, int paint );
void RelinkANode( PSPACEWEB_NODE web, PSPACEWEB_NODE came_from, PSPACEWEB_NODE node, int final );
void InvalidateLinks( PSPACEWEB_NODE node, PCVECTOR new_point, int bPrevalLink  );
void BreakSingleNodeLinkEx( PSPACEWEB_NODE node, PSPACEWEB_NODE other DBG_PASS );
#define BreakSingleNodeLink(a,b) BreakSingleNodeLinkEx(a,b DBG_SRC )
int LinkWebNodeEx( PSPACEWEB_NODE node, PSPACEWEB_NODE linkto DBG_PASS );
#define LinkWebNode( node, linkto ) LinkWebNodeEx( node,linkto DBG_SRC )

static void DeleteNodeLink( PLIST *links, PSPACEWEB_NODE delMe ) {
	INDEX idx;
	PSPACEWEB_LINK_DATA data;
	PSPACEWEB_LINK link;
	PSPACEWEB_LINK other_link;

	LIST_FORALL( (*links), idx, PSPACEWEB_LINK, link ) {
		data = link->data;
		other_link = link->invert?data->from:data->to;

		if( link->node == delMe ) {
			SetLink( links, idx, NULL );
			SetLink( &other_link->node->links, FindLink( &other_link->node->links, other_link ), NULL );
			DeleteFromSet( SPACEWEB_LINK_DATA, &delMe->web->link_data, link->data );
			DeleteFromSet( SPACEWEB_LINK, &delMe->web->links, link );
			DeleteFromSet( SPACEWEB_LINK, &delMe->web->links, other_link );
		}
	}
}


void UnlinkWebNode( PSPACEWEB_NODE node )
{
	INDEX idx;
	PLIST needs_someone = NULL;
	PSPACEWEB_LINK anyone_else = NULL;
	PSPACEWEB_LINK linked;
	PLIST linked_list = NULL;
	uintptr_t prior = node->island;


	if( node->web->root == node )
	{
#if ( DEBUG_ALL )
		lprintf( WIDE("Going to have to pivot root.") );
#endif
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, linked )
		{
			if( !anyone_else )
			{
#if ( DEBUG_ALL )
				lprintf( WIDE("new root is first one %p"), linked );
#endif
				anyone_else = linked;
			}
			else
				LinkWebNode( anyone_else->node, linked->node );
			AddLink( &linked_list, linked );
		}

		LIST_FORALL( linked_list, idx, PSPACEWEB_LINK, linked )
		{
#if ( DEBUG_ALL )
			lprintf( WIDE("Safely break each link between %p and %p"), node, linked );
#endif
			BreakSingleNodeLink( node, linked->node );
		}
		LIST_FORALL( linked_list, idx, PSPACEWEB_LINK, linked )
		{
			InvalidateLinks( linked->node, NULL, 0 );
		}
		DeleteList( &linked_list );
		node->web->root = anyone_else->node;
	}

	anyone_else = NULL;
#if ( DEBUG_ALL )
	lprintf( WIDE("Eiher we pivoted the root, and have no links, or search naers...") );
#endif
	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, linked )
	{
		if( anyone_else )
		{
#if ( DEBUG_ALL )
			lprintf( WIDE("Someone else is %p... going to link %p"), anyone_else, linked );
#endif
			LinkWebNode( anyone_else->node, linked->node );
		}

		AddLink( &linked_list, linked );
		anyone_else = linked;
	}

	LIST_FORALL( linked_list, idx, PSPACEWEB_LINK, linked )
	{
#if ( DEBUG_ALL )
		lprintf( WIDE("okay now we can break all links... %p to %p"), node, linked );
#endif
		BreakSingleNodeLink( node, linked->node );
	}
	LIST_FORALL( linked_list, idx, PSPACEWEB_LINK, linked )
	{
		InvalidateLinks( linked->node, NULL, 0 );
	}
//		DeleteLink( &linked->near_nodes, node );
  // 	SetLink( &node->near_nodes, idx, NULL );

	LIST_FORALL( linked_list, idx, PSPACEWEB_LINK, linked )
	{
#if ( DEBUG_ALL )
		lprintf( WIDE("Validate %p"), linked );
#endif
		InvalidateLinks( linked->node, NULL, 0 );
	}
	DeleteList( &linked_list );
#if ( DEBUG_ALL )
	lprintf( WIDE("Reinsert %p"), node );
#endif
	//RelinkANode( node->web->root, node );
	return;

	anyone_else = NULL;
	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, linked )
	{
		lprintf( WIDE("divorce nodes %p and %p"), node, linked );
		DeleteNodeLink( &linked->node->links, node );
		linked->node->near_count--;
		SetLink( &node->links, idx, NULL );
		node->near_count--;
		if( IsOrphan( linked, 0 ) )
		{
			if( anyone_else )
			{
				lprintf( WIDE("already know someone else in the web, use them and relink orphan") );
				RelinkANode( anyone_else->node, NULL, linked->node, 0 );
			}
			else
			{
				if( node->web && node == node->web->root )
				{
					lprintf( WIDE("had to fix root...") );
					node->web->root = linked->node;
					node->web = NULL;
					anyone_else = linked;
				}
				else
				{
					lprintf( WIDE("Don't know anyone stable, making orphan in list.") );
					if( linked->node != node->web->root )
						AddLink( &needs_someone, linked );
					else
					{
						lprintf( WIDE("Oh alright - it's the root, everything ELSE is orphan %p"), linked );
						anyone_else = linked;
					}
				}
			}
		}
		else
		{
			if( !node->web )
			{
				SignIsland( anyone_else->node->web->root, prior-1 );
			}
			else
			{
				SignIsland( node->web->root, prior-1 );
			}
			if( node->island == prior )
			{

				//DebugBreak();
				// caused an island.
				if( anyone_else )
				{
					lprintf( WIDE("What about the peers of this? "));
					LinkWebNode( anyone_else->node, linked->node );
					//RelinkANode( anyone_else, linked );
				}
				else
				{
					lprintf( WIDE("didn't know anyone, but since it's just a relink, add to needs?") );
					if( linked->node != linked->node->web->root )
						AddLink( &needs_someone, linked );
					else
					{
						lprintf( WIDE("Oh - island is the root, and it's the other way orphaned.") );
						anyone_else = linked;
					}
				}
			}
			else
			{
				anyone_else = linked;
			}
			prior--;
		}

	}
	if( needs_someone )
	{
		PSPACEWEB_NODE orphan;
		INDEX idx;
		LIST_FORALL( needs_someone, idx, PSPACEWEB_NODE, orphan )
		{
			lprintf( WIDE("Recovered an orphan") );
			if( orphan->web->root == orphan )
			{
				DebugBreak();
			}
			RelinkANode( orphan->web->root, NULL, orphan, 0 );
		}
		DeleteList( &needs_someone );
	}
	if( !node->web )
	{
		// restore this...
		node->web = anyone_else->node->web;
	}

}

int update_pause = 50000;
PSPACEWEB_LINK_DATA IsLinked( PSPACEWEB_NODE node, PSPACEWEB_NODE other );

void BreakSingleNodeLinkEx( PSPACEWEB_NODE node, PSPACEWEB_NODE other DBG_PASS )
#define BreakSingleNodeLink(a,b) BreakSingleNodeLinkEx(a,b DBG_SRC )
{
	_lprintf(DBG_RELAY)( WIDE("Seperate nodes %d and %d"), NodeIndex( node ), NodeIndex( other ) );
	if( IsLinked( node, other ) )
	{
#if 0
		INDEX idx;
		PSPACEWEB_LINK link;
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, link )
		{
			if( link->node == other )
			{
				if( link->data->primary == node )
				{
					if( link->data->secondary )
					{
						lprintf( WIDE("Had a secondary on this, promote secondary to primary") );
						link->data->primary = link->data->secondary;
						link->data->secondary = NULL;
						link->data->valence--;
						return;
					}
					else
					{
						lprintf( WIDE("only the primary side was linked, and we're breaking that.") );
					}
				}
				else
				{
					if( link->data->secondary == node )
					{
						lprintf( WIDE("My link is the secondary one on this... just remove mine.") );
						link->data->secondary = NULL;
						link->data->valence--;
						return;
					}
					else
					{
						lprintf( WIDE("Uhmm my deletion is secondary, think I'm not allowed.") );
						return;
					}
				}

				DeleteFromSet( SPACEWEB_LINK_DATA, &node->web->link_data, link->data );
				DeleteFromSet( SPACEWEB_LINK, &node->web->links, link );
				SetLink( &node->links, idx, NULL );
				LIST_FORALL( other->links, idx, PSPACEWEB_LINK, link )
				{
					if( link->node == node )
					{
						DeleteFromSet( SPACEWEB_LINK, &node->web->links, link );
						SetLink( &other->links, idx, NULL );
					}
				}
				break;
			}
		}
#endif
		/*
		LIST_FORALL( other->links, idx, PSPACEWEB_LINK, link )
		{
			if( link->node == node )
			{
				if( link->data->primary == other )
				{
					if( link->data->secondary )
					{
						lprintf( WIDE("Uhmm... this data is shared on THIS link. ... there better be no secondary...") );
						DebugBreak();
						link->data->primary = link->data->secondary;
						link->data->secondary = NULL;
						link->data->valence--;
						return;
					}
					else
					{
						lprintf( WIDE("only the primary side was linked, and we're breaking that.") );
					}
				}
				DeleteFromSet( PSPACEWEB_LINK_DATA, &node->web->link_data, link->data );
				DeleteFromSet( PSPACEWEB_LINK, &node->web->links, link );
				SetLink( &other->links, idx, NULL );
				break;
			}
		}
		*/
		DeleteNodeLink( &node->links, other ); // deletes both sides - entire link.
	}
	else
		lprintf( WIDE("Link didn't exist.") );
}


PSPACEWEB_LINK_DATA IsLinked( PSPACEWEB_NODE node, PSPACEWEB_NODE other )
{
	INDEX idx;
	PSPACEWEB_LINK link;
	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, link )
	{
		if( (link->invert?link->data->from:link->data->to)->node == other )
			break;
	}
	if( link )
	{
		LIST_FORALL( other->links, idx, PSPACEWEB_LINK, link )
		{
			if( (link->invert ? link->data->from : link->data->to)->node == node )
				return link->data;
		}
		DebugBreak();  // not reflective link
	}
	else
	{
		LIST_FORALL( other->links, idx, PSPACEWEB_LINK, link )
		{
			if( (link->invert ? link->data->from : link->data->to)->node == node )
				DebugBreak(); // not reflective link.
		}
	}
	return NULL;
}

// return the(a) node that invalidates this link... (it's beyond this. (may be others))
PSPACEWEB_NODE IsNodeWithinEx( PSPACEWEB_NODE node, PCVECTOR new_point, PSPACEWEB_NODE test_node, PCVECTOR new_test_point )
{
	{
		INDEX idx;
		PSPACEWEB_LINK link;
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, link )
		{
			_POINT p;
			if( link->node == test_node )
				continue;

			sub( p, link->node->point, new_point?new_point:node->point );
			{
				RCOORD t;

				t = PointToPlaneT( link->data->plane.n, link->data->plane.o, new_test_point?new_test_point:test_node->point );
				if( link->invert )
					t = -t;
				if( t > 0.5 )
					return link->node;
			}
			//lprintf( WIDE("node %d is near..."), NodeIndex( link->node ) );
		}
	}

	return NULL;
}

LOGICAL IsNodeWithin( PSPACEWEB_NODE node, PCVECTOR new_point, PSPACEWEB_NODE test_node )
{
	// if ex returns a node, it was aborted, and not valid.
	if( IsNodeWithinEx( node, new_point, test_node, NULL ) )
		return FALSE;
	return TRUE;
}

// this does have an 'affinity' or directionality...
int LinkWebNodeEx( PSPACEWEB_NODE node, PSPACEWEB_NODE linkto DBG_PASS )
#define LinkWebNode( node, linkto ) LinkWebNodeEx( node,linkto DBG_SRC )
{
	PSPACEWEB_LINK_DATA linked_data;
	if( node == linkto )
		DebugBreak();
	if( linked_data = IsLinked( node, linkto ) )
	{
		//DebugBreak();
		if( linkto == linked_data->from->node ||
			linkto == linked_data->to->node
			)
		{
			/*
			if( !linked_data->secondary )
				linked_data->secondary = linkto;
			if( linked_data->valence == 1 )
				linked_data->valence = 2;
			*/
		}
		//linked_data->valence += 0x10;
		_lprintf(DBG_RELAY)( WIDE("Link already exists...(%d to %d)"), NodeIndex( node ), NodeIndex( linkto ) );
		return 0;
	}
	_lprintf(DBG_RELAY)( WIDE("link %d to %d"), NodeIndex( node ), NodeIndex( linkto ) );

	{
		PSPACEWEB_LINK link = GetFromSet( SPACEWEB_LINK, &node->web->links );
		PSPACEWEB_LINK_DATA data = GetFromSet( SPACEWEB_LINK_DATA, &node->web->link_data );
		SetPoint( data->plane.o, node->point );
		sub( data->plane.n, linkto->point, node->point );
		addscaled( data->plane.o, data->plane.o, data->plane.n, 0.5 );

		data->from = link;

		link->node = node;
		link->data = data;
		link->invert = FALSE;

		AddLink( &node->links, link );
		node->near_count++;

		link = GetFromSet( SPACEWEB_LINK, &node->web->links );
		data->to = link;

		link->node = linkto;
		link->data = data;
		link->invert = TRUE;
		AddLink( &linkto->links, link );
		linkto->near_count++;
	}
	return 1;
}

void CheckOkLink( PSPACEWEB_NODE node, PSPACEWEB_NODE linkto, PSPACEWEB_NODE memberof )
{
	INDEX idx;
	PSPACEWEB_LINK check;

	// current
	if( memberof )
	{
	recheck_list:

		LIST_FORALL( linkto->links, idx, PSPACEWEB_LINK, check )
		{
			_POINT p;
			RCOORD t;
			// don't check the node against itself.
			if( check->node == node )
			{
				DebugBreak(); // we shouldn't already be linked ?
				continue;
			}

			sub( p, check->node->point, linkto->point );
			t = PointToPlaneT( p, check->node->point, node->point );

			if( t < -1.0 )
			{
				// point is on the outside of this point... so it's probably nearer to this point.
				UnlinkWebNode( check->node );
				// uhmm yeah... this check is beyond this new point... so link that one to node
				//RelinkNode( node, check );
				CheckOkLink( check->node, node, linkto );
				//LinkWebNode( node, check );
				goto recheck_list;
			}
		}
		if( !check )
		{
			LinkWebNode( linkto, node );
		}
	}
}

// confirm that node to check1 and check2 is valid...
LOGICAL IsWithin( PSPACEWEB_NODE node, PSPACEWEB_NODE check1, PSPACEWEB_NODE check2 )
{
	_POINT p;
	sub( p, check1->point, node->point );
	{
		RCOORD t;

		t = PointToPlaneT( p, check1->point, check2->point );
		PrintVector( check1->point );
		PrintVector( check2->point );
		PrintVector( p );
		lprintf( WIDE("one is %g"), t );
		if( t > 0 )
		{
			return FALSE;
		}
	}
	return TRUE;
}


// confirm that node to check1 and check2 is valid...
LOGICAL CameThrough( PSPACEWEB_NODE node, PCVECTOR new_point, PSPACEWEB_NODE check1, PSPACEWEB_NODE check2 )
{
	_POINT p;
	lprintf( WIDE("checking to see if %d<->%d<->%d"), NodeIndex( check1 ), NodeIndex( node ), NodeIndex( check2 ) );
	sub( p, check1->point, new_point?new_point:node->point );
	{
		RCOORD t;

		t = PointToPlaneT( p, new_point?new_point:node->point, check2->point );
		lprintf( WIDE("one is %g"), t );
		if( t < 0 )
		{
			lprintf( WIDE("Goes through (definatly from check1 to node before check2. (from check1))") );
			return TRUE;
		}
	}
	lprintf( WIDE("does not go through.") );
	return FALSE;
}

// confirm that node to check1 and check2 is valid...
LOGICAL IsBeyond( PSPACEWEB_NODE node, PCVECTOR new_point, PSPACEWEB_NODE check1, PSPACEWEB_NODE check2 )
{
	_POINT p;
	lprintf( WIDE("checking to see if %d<->%d<->%d"), NodeIndex( node ), NodeIndex( check1 ), NodeIndex( check2 ) );
	sub( p, check1->point, new_point?new_point:node->point );
	{
		RCOORD t;

		t = PointToPlaneT( p, check1->point, check2->point );
		lprintf( WIDE("one is %g"), t );
		if( t > 0 )
		{
			lprintf( WIDE("Goes check2 beyond check1.") );
			return TRUE;
		}
	}
	lprintf( WIDE("check2 is not beyond check1 - may be vice versa.") );
	return FALSE;
}


int PrevalLink( PSPACEWEB_LINK check, PSPACEWEB_LINK check2, PSPACEWEB_NODE removing, PCVECTOR new_point )
{
	int keep_link = 0;
	int okay = 1;
	lprintf( WIDE("Check to see that linking %d to %d is ok, was near %d"), NodeIndex( check->node ), NodeIndex(check2->node ), NodeIndex( removing ) );
	{
		INDEX idx;
		_POINT p3;
		PSPACEWEB_LINK check_near;
		LOGICAL a, b;
		// have to check if it's going to come through this point


		a=CameThrough( removing, NULL, check->node, check2->node );
		b=CameThrough( removing, new_point, check->node, check2->node );
		if( (a) && (b) )
		{
			lprintf( "check ->node -> check2 is a chain, and check to check2 should not be kept." );
			keep_link = 0;
			okay = 0;
			//lprintf( WIDE("we also need to do this link, if it's valid.") );
		}
		else if( !a && !b ) {
			keep_link = 1;
			okay = 1;
			//lprintf( WIDE("we also need to do this link, if it's valid.") );
		}else {
			if( !a && b )
			{
				lprintf( WIDE("wasn't related, and now is") );
				keep_link= 0;
			}
			else
			{
				lprintf( WIDE("a and !b? was a passthrough and now isn't?") );
				//okay = 0;
			}
		}
		if( okay )
		{
			sub( p3, check2->node->point, check->node->point );
			LIST_FORALL( check2->node->links, idx, PSPACEWEB_LINK, check_near )
			{
				RCOORD t3;
				if( check_near->node == removing )
					continue;
				t3 = PointToPlaneT( sub( p3, check_near->node->point, check2->node->point )
										, check_near->node->point, check->node->point );
				//lprintf( WIDE("%d->%d v %d = %g"), NodeIndex( check ), NodeIndex( check2 ), NodeIndex( check_near ), t3 );
				if( t3 > 0 )
				{
					okay = 0;
					break;
				}
			}
		}
		if( okay )
		{
			sub( p3, check->node->point, check2->node->point );
			LIST_FORALL( check->node->links, idx, PSPACEWEB_LINK, check_near )
			{
				RCOORD t3;
				if( check_near->node == removing )
					continue;

				t3 = PointToPlaneT( sub( p3, check_near->node->point, check->node->point )
												 , check_near->node->point, check2->node->point );
				//lprintf( WIDE("%d->%d v %d = %g"), NodeIndex( check2 ), NodeIndex( check ), NodeIndex( check_near ), t3 );
				if( t3 > 0 )
				{
					okay = 0;
					break;
				}
			}
		}

	}
	if( okay )
		LinkWebNode( check->node, check2->node );

	return keep_link;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC IsLink( uintptr_t value, INDEX i, POINTER *link )
{
	if( value == (uintptr_t)(*link) )
		return i + 1; // 0 might be value so add one to make it non zero
	return 0;
}

//--------------------------------------------------------------------------

INDEX  FindNodeLink( PLIST *pList, POINTER value )
{
	if( !pList || !(*pList) )
		return INVALID_INDEX;
	return ForAllLinks( pList, IsLink, (uintptr_t)value ) - 1;
}



LOGICAL ValidLink( PSPACEWEB_NODE node, PSPACEWEB_NODE other_node )
{
	if( !IsNodeWithinEx( node, NULL, other_node, NULL ) || !IsNodeWithinEx( other_node, NULL, node, NULL ) )
		return TRUE;
	return FALSE;

}

// makes suare all links from here are valid for myself
// (if they are valid for what they are linked to, they must also stay
void InvalidateLinks( PSPACEWEB_NODE node, PCVECTOR new_point, int bPrevalLink )
{
	PSPACEWEB_LINK _check;
	INDEX idx;


	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, _check )
	{
		PSPACEWEB_LINK check = (_check->invert ? _check->data->from : _check->data->to);
		PSPACEWEB_LINK _check2;
		_POINT p;
		INDEX idx2;
		idx2 = idx;
		sub( p, check->node->point, new_point?new_point:node->point );
		LIST_NEXTALL( node->links, idx, PSPACEWEB_LINK, _check2 )
		{
			PSPACEWEB_LINK check2 = (_check2->invert ? _check2->data->from : _check2->data->to);
			// test check2 point above node->check1
			PSPACEWEB_NODE other = check2->node;
			RCOORD t = PointToPlaneT( p, new_point?new_point:node->point, other->point );
			_POINT p2;
			{
			// test check point above node->check2
			RCOORD t2 = PointToPlaneT( sub( p2, other->point, new_point?new_point:node->point ), new_point?new_point:node->point, check->node->point );
//#if ( DEBUG_ALL )
			lprintf( WIDE("Hrm..%d(base) %d vs %d %g  %g"), NodeIndex( node ), NodeIndex( check->node ), NodeIndex( other ), t, t2 );
			//#endif

#if 0
			// this is what validatelinks is now....
			if( t > 0 && t < 1 )
			{
				if( t2 > 0 && t2 < 1 )
				{
					PSPACEWEB_NODE a, b;
					a = IsNodeWithinEx( check, NULL, check2, NULL );
					b = IsNodeWithinEx( check2, NULL, check, NULL );
					lprintf( WIDE("maybe these should be linked? %d %d"), a?NodeIndex(a):-1, b?NodeIndex(b):-1 );
					if( !a && !b )
					{
						// okay they're definatly valid this way...
						LinkWebNode( check, check2 );
					}

					if( a && !b )
					{
						PSPACEWEB_NODE ab, aa;
						lprintf( WIDE(" uhmm okay we can get from one to the other, but not vice-versa... can we solve this point?") );
						aa = IsNodeWithinEx( check2, NULL, a, NULL );
						ab = IsNodeWithinEx( a, NULL, check2, NULL );
						if( !aa && !ab )
							LinkWebNode( a, check2 );
						b = IsNodeWithinEx( check2, NULL, check, NULL );
						if( !b )
							lprintf( WIDE("oh well.") );
					}

					if( !a && b )
					{
						PSPACEWEB_NODE bb, ba;
						lprintf( WIDE(" uhmm okay we can get from one to the other, but not vice-versa... can we solve this point?") );
						ba = IsNodeWithinEx( check2, NULL, b, NULL );
						bb = IsNodeWithinEx( b, NULL, check2, NULL );
						if( !ba && !bb )
							LinkWebNode( b, check2 );
						a = IsNodeWithinEx( check2, NULL, check, NULL );
						if( !a )
							lprintf( WIDE("oh well.") );
					}
					if( a && b )
					{
						lprintf( WIDE("Uhmm these are well divorced already.") );
					}
				}
			}

#endif
			if( t >= 1  || t <= -1 )
			{
//#if ( DEBUG_ALL )
				lprintf( WIDE("Removing node to check2...") );
				//#endif
				// check is between check2 and node, so we should link check2 and check and
				// remove check2 from self...

				// remove self, so linking won't fail. (though, if link fails... do we get orphans)
				if( PrevalLink( check, check2, node, new_point ) ) {
					BreakSingleNodeLink( check2->node, node );
				}
				//else
				//	lprintf( WIDE("nevermind, it was already linked with a via.") );
				//InvalidateLinks( check );
				//InvalidateLinks( check2 );
			}
			else if( t2 >= 1 || t2 <= -1 )
			{
//#if ( DEBUG_ALL )
				lprintf( WIDE("Removing node to check...") );
//#endif
				if( PrevalLink( check2, check, node, new_point ) )
				{
					INDEX idx;
					PSPACEWEB_LINK near_node;
					LIST_FORALL( node->links, idx, PSPACEWEB_LINK, near_node )
					{
						if( near_node->node == check2->node )
							continue;
						if( near_node->node == check->node )
							continue;
						if( CameThrough( node, NULL, check->node, near_node->node ) )
						{
						}

						if( CameThrough( node, new_point, check->node, near_node->node ) )
						{
							lprintf( WIDE("maybe we have to spare this link?") );
						}
					}
					BreakSingleNodeLink( check->node, node );
				}
				//PrevalLink( check, check2 );
				//else
				//	lprintf( WIDE("nevermind, it was already linked with a via.") );
				//InvalidateLinks( check );
				//InvalidateLinks( check2 );
			}
			}
		}
		idx = idx2;
	}
}

// near_node
void ValidateLink( PSPACEWEB_NODE resident, PCVECTOR new_point, PSPACEWEB_NODE node )
{
	INDEX idx2;
	PSPACEWEB_NODE near2;
	PSPACEWEB_LINK link;
	_POINT p;
	// node was recently linked to this resident node (already belongs to the web).

	// this checks all other links from near_point (the resident)
	// versus this new point, if the point is beyond this new point, then
	// migrate a link to node from the resident

	// from resident -> node delta
	sub( p, new_point?new_point:node->point, resident->point );

	LIST_FORALL( resident->links, idx2, PSPACEWEB_LINK, link )
	{
		RCOORD t;
		PSPACEWEB_LINK_DATA data = link->data;
		// don't check the node against itself.
		near2 = (link->invert ? data->from : data->to)->node;
		if( near2 == node )
			continue;

		// compare node base point versus near2 (the relative of near that I'm linked against)
		t = PointToPlaneT( data->plane.o, data->plane.n, near2->point );
		if( link->invert ) t = -t;


		lprintf( WIDE("%d->%d v %d is %g"), NodeIndex( resident ), NodeIndex( node ), NodeIndex( near2 ), t );
		if( t > 1)
		{
			lprintf( WIDE("So we steal the link to me %d , and remove from resident %d  (%d)")
					, NodeIndex( node ), NodeIndex( resident ), NodeIndex( near2 ) );
			// one for one exchange
			/*
			LinkWebNode( node, resident );
			//BreakSingleNodeLink( resident, near2 );

			if( IsNodeWithin( near2, NULL, resident ) )
				BreakSingleNodeLink( resident, near2 );
			lprintf( WIDE("And again validate my own links? considering the near2 as resident and me new") );
			//ValidateLink( near2, NULL, node );
			*/
		}
		else
			lprintf( WIDE("ok..") );
	}
}



uintptr_t CPROC MakeOrphan( POINTER p, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)p;
	INDEX idx;
	PSPACEWEB_LINK linked;
	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, linked )
	{
		lprintf( WIDE("divorce nodes %p and %p"), node, linked );
		DeleteLink( &linked->node->links, node );
		SetLink( &node->links, idx, NULL );
		//DeleteFromSet( node->web->, linked )
	}

	return 0;
}


void RelinkNode( PSPACEWEB web, PSPACEWEB_NODE node );

uintptr_t CPROC RebuildOrphan( POINTER p, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)p;
	RelinkNode( node->web, node );
	return 0;
}

// give the position to set the node to..
// then we have the source and destination addresses... and we can know how it moved
void MigrateLink( PSPACEWEB_NODE node, PCVECTOR p_dest )
{
	static int migrating;
	INDEX idx;

	PSPACEWEB_NODE any_node = NULL;
	PSPACEWEB_LINK check;
	if( migrating )
		return;
	migrating = 1;

	if(0)
	{
		ForAllInSet( SPACEWEB_NODE, node->web->nodes, MakeOrphan, 0 );
		node->web->root = NULL;

		ForAllInSet( SPACEWEB_NODE, node->web->nodes, RebuildOrphan, 0 );
		migrating = 0;
		return;
	}

	lprintf( WIDE("------ Begin a migration(%d) ----------"), NodeIndex( node ) );

	{
		INDEX idx;
		PSPACEWEB_NODE any_near = NULL;
		PSPACEWEB_LINK check;

		// self's links maybe invalid now... so, use the regular check on this node
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, check )
		{
			_POINT p;
			RCOORD t;
			INDEX idx2 = idx;
			sub( p, check->node->point, node->point );
			t = PointToPlaneT( p, node->point, p_dest );
			lprintf( WIDE("%d->%d  %g"), NodeIndex( node ), NodeIndex( check->node ), t );
			if( t > 1 )
				break;
		}
		if( check )
		{
			// bad.
			PLIST pListNear = NULL;

			// have never testwed the case that the motion put the point outside
			// of its current locale.  (high density points?)
			//DebugBreak();
			lprintf( WIDE("Fell outside the lines... best to orphan, and rebuild (probably)") );
			FindNearest( &pListNear, NULL, node, p_dest, 0 );
			{
				INDEX idx;
				PSPACEWEB_NODE near_node;
				PSPACEWEB_LINK near_link;
				INDEX idx2;
				PSPACEWEB_NODE near2;
				LIST_FORALL( pListNear, idx, PSPACEWEB_NODE, near_node )
				{
					lprintf( WIDE("node %d is near..."), NodeIndex( near_node ) );
				}

				LIST_FORALL( node->links, idx, PSPACEWEB_LINK, near_link )
				{
					BreakSingleNodeLink( node, near_link->node );
				}

				LIST_FORALL( node->links, idx, PSPACEWEB_LINK, near_link )
				{
					LIST_FORALL( pListNear, idx2, PSPACEWEB_NODE, near2 )
					{
						if( near2 == near_link->node )
						{
							break;
						}
					}
					if( !near2 )
					{
						lprintf( WIDE("%d is no longer near... break link."), NodeIndex( near_node ) );
						BreakSingleNodeLink( node, near_node );
					}
				}

    				//DebugBreak();
				LIST_FORALL( pListNear, idx2, PSPACEWEB_NODE, near2 )
				{
					LIST_FORALL( node->links, idx, PSPACEWEB_LINK, near_link )
					{
						if( near2 == near_link->node )
						{
							break;
						}
					}
					if( !near_node )
					{
						lprintf( WIDE("%d was not a link... adding it"), NodeIndex( near2 ) );
						LinkWebNode( node ,near2 );
					}
				}
			}

			DeleteList( &pListNear );

		}
		else
		{
			// okay it's still within it's local region... probably just update point
			// and be done with it... though it can cause some of my nears to invalidate others
			//lprintf( WIDE("Still within my own bounds, should validate that my nears are still valid.") );

			PLIST pListNear = NULL;

			FindNearest( &pListNear, NULL, node, p_dest, 0 );
			{
				INDEX idx;
				PSPACEWEB_NODE near_node;
				LIST_FORALL( pListNear, idx, PSPACEWEB_NODE, near_node )
				{
					if( near_node == node )
						continue;
					lprintf( WIDE("node %d is near..."), NodeIndex( near_node ) );
					if( IsNodeWithin( node, p_dest, near_node ) )
					{
						LinkWebNode( near_node, node );
						ValidateLink( near_node, p_dest, node );
					}
					else
						lprintf( WIDE("yeah... but it's not within our bounds...") );

				}
			}

			DeleteList( &pListNear );
			InvalidateLinks( node, p_dest, 0 );

		}

		SetPoint( node->point, p_dest );

		migrating = 0;
		return;
	}

	// migrate this node...
	lprintf( WIDE("migration - first - check validity uhhmm... between node(near) and node(near(near)) from (near) to (near(near)) vs point") );
	LIST_FORALL( node->links, idx, PSPACEWEB_LINK, check )
	{
		INDEX idx2;
		PSPACEWEB_LINK check2;
		//if( NEAR( node->point, check->point ) )
		{
			// points are invalid, cause they are the same point.
#if ( DEBUG_MIGRATE )
			//lprintf( WIDE("die...") );
#endif
		}
		LIST_FORALL( check->node->links, idx2, PSPACEWEB_LINK, check2 )
		{
			if( check2->node == node )
				continue;
			{
				_POINT p;
				RCOORD t = PointToPlaneT( sub( p, check2->node->point, check->node->point ), check2->node->point, node->point );
#if ( DEBUG_MIGRATE )
				lprintf( WIDE("point is %g ..."), t );
#endif
				if( t >= 2.0 )
				{
#if ( DEBUG_MIGRATE )
					lprintf( WIDE("point is invalid.  checknode is above another plane near node") );
#endif
					UnlinkWebNode( node );
#if ( DEBUG_MIGRATE )
					lprintf( WIDE(" -- unlink finished, now to link... ") );
#endif
					LinkWebNode( check2->node, node );
					//RelinkANode( check2, node );
					{
						int c = 0;
						INDEX idx;
						PSPACEWEB_NODE tmp;
						LIST_FORALL( node->links, idx, PSPACEWEB_NODE, tmp ) c++;
						if( c == 0 )
						{
							lprintf( WIDE(" *** Oops dropped the node entirely.") ) ;
							RelinkANode( check->node, NULL, node, 0 );
						}
					}
					migrating = 0;
					return;
					//break;
				}
				else
					lprintf( WIDE("safe - link %p to %p?") );
			}
			
		}
	}

	if( any_node )
	{
		lprintf( WIDE("attach node to any node...") );
		//RelinkANode( any_node, node );
	}

	//if(0)
	{
		//
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, check )
		{
			INDEX idx2 = idx;
			PSPACEWEB_LINK check2;
			LIST_NEXTALL( node->links, idx, PSPACEWEB_LINK, check2 )
			{
				lprintf( WIDE("compare %d v %d"), idx2, idx );
				// this is certainly one way to do this :)
				if( check->data != check2->data )
				{
					int okay = 1;
					INDEX idx3;
					PSPACEWEB_LINK check3;
					_POINT p;
					RCOORD t;
					LIST_FORALL( check->node->links, idx3, PSPACEWEB_LINK, check3 )
					{

						sub( p, check3->node->point, check->node->point );
						t = PointToPlaneT( p, check->node->point, check2->node->point );
						if( t > 2.0 )
						{
#if ( DEBUG_MIGRATE )
							lprintf( WIDE("Fail.") );
#endif
							okay = 0;
							break;
						}

					}
					if( okay )
					{
						LIST_FORALL( check2->node->links, idx3, PSPACEWEB_LINK, check3 )
						{

							sub( p, check3->node->point, check2->node->point );
							t = PointToPlaneT( p, check2->node->point, check->node->point );
							if( t > 2.0 )
							{
#if ( DEBUG_MIGRATE )
								lprintf( WIDE("Fail.(2)") );
#endif
								okay = 0;
								break;
							}

						}
					}
					if( okay )
					{
						lprintf( WIDE("check and check2 should link now.") );
						LinkWebNode( check->node, check2->node );
						InvalidateLinks( check2->node, NULL, 0 );
					}
				}
			}
			idx = idx2;
		}
	}
	migrating = 0;
}



// find nearest does a recusive search and finds nodes
// which may qualify for linking to the new node (to).
// from is some source point, in a well linked web, should be irrelavent which to start from
// paint is passed to show nodes touched during the (last)search.
PSPACEWEB_NODE FindNearest( PLIST *nodes, PLIST *came_from, PSPACEWEB_NODE from, PCVECTOR to, int paint )
{
	INDEX idx;
	int moved;
	int successes = 0;
	PSPACEWEB_NODE current = from;
	PSPACEWEB_LINK check;
	PLINKQUEUE maybe = CreateLinkQueue();
	PLIST _came_from = NULL;
	int log = 1;
	if( log ) lprintf( WIDE("Begin Find.") );
	if( !came_from )
		came_from = &_came_from;
	EmptyList( nodes );
	do
	{
		int okay = 1;
		moved = 0;
		//if( current == to )
		{
			//if( log ) lprintf( WIDE("found myself! yay. I am closest.") );
			//return NULL;
		}
		AddLink( came_from, current );
		if( log ) lprintf( WIDE("Begin check %d"), NodeIndex( current ) );
		LIST_FORALL( current->links, idx, PSPACEWEB_LINK, check )
		{
			_POINT p;
			RCOORD t;
			PSPACEWEB_LINK otherCheck = check->invert ? check->data->from : check->data->to;

			if( log ) lprintf( WIDE("checking near %d"), NodeIndex( otherCheck->node ) );
			if( FindLink( came_from, otherCheck->node ) != INVALID_INDEX )
			{
				if( log ) lprintf( WIDE("already checked...(or will be)") );
				t = PointToPlaneT( sub( p, check->node->point, current->point ), current->point, to );
				if( t > 1.0 )
				{
					if( log ) lprintf( WIDE("might not have been checked in this direction, so checked, and discovered it's not a valid near.") );
					okay = 0;
				}
				continue;
			}

			t = PointToPlaneT( sub( p, otherCheck->node->point, current->point ), current->point, to );

			if( log ) lprintf( WIDE("??%d vs %d->%d is %g")
					 , 0//GetMemberIndex( SPACEWEB_NODE, &to->web->nodes, to )
					 , GetMemberIndex( SPACEWEB_NODE, &current->web->nodes, current )
					 , GetMemberIndex( SPACEWEB_NODE, &otherCheck->node->web->nodes, check )
					 , t );
			if( ( t > 0 ) || ( successes == 0 && t < 1 ) )
			{
				if( FindLink( came_from, otherCheck->node ) == INVALID_INDEX )
				{
					check->node->paint = paint;
					if( log ) lprintf( WIDE("Adding check to maybe..") );
					EnqueLink( &maybe, otherCheck->node );
				}
				else
					if( log ) lprintf( WIDE("came from %d"), NodeIndex( check->node ) );
				if( t > 1 )
					okay = 0;
			}
		}
		if( okay )
		{
			if( log ) lprintf( WIDE("Add nearest as %d"), NodeIndex( current ) );
			if( FindLink( nodes, current ) == INVALID_INDEX )
			{
				successes++;
				AddLink( nodes, current );
			}
		}

	} while( current = (PSPACEWEB_NODE)DequeLink( &maybe ) );
	if( log ) lprintf( WIDE("Completed find.") );
	DeleteList( &_came_from );
	return NULL; // returns a list really.
}





void RelinkANode( PSPACEWEB_NODE web, PSPACEWEB_NODE came_from, PSPACEWEB_NODE node, int final )
{
	PSPACEWEB_NODE current = web;
	int linked = 0;
	PLIST list = NULL;
	static int levels;
	static int paint;
	// just mkae sure it's really not linked to anything.

	if( !levels )
	{
		paint++;
	}
	if( web->paint == paint )
	{
		lprintf( WIDE("We already checked this locale.") );
		return;
	}
	current->paint = paint;
	levels++;

	// the second list is where we came from, don't think I need that...

	FindNearest( &list, NULL, web, node->point, paint );
	{
		INDEX idx;
		PSPACEWEB_NODE near_node;
		LIST_FORALL( list, idx, PSPACEWEB_NODE, near_node )
		{
			INDEX idx2 = idx;
			PSPACEWEB_NODE near2;
			lprintf( "nearest is %d %d %d", GetMemberIndex( SPACEWEB_NODE, &near_node->web->nodes, near_node ),
				(int32_t)node->point[0], (int32_t)node->point[2] );
				LIST_NEXTALL( list, idx, PSPACEWEB_NODE, near2 )
			{
				if( !IsWithin( node, near_node, near2 ) )
				{
					lprintf( WIDE("oh, near2 is no good, near1 obsoletes") );
					SetLink( &list, idx, NULL );
				}
				if( !IsWithin( node, near2, near_node ) )
				{
					lprintf( WIDE("oh, near1 is no good, near2 obsoletes") );
					SetLink( &list, idx2, NULL );
				}
			}
			idx = idx2;
		}

		LIST_FORALL( list, idx, PSPACEWEB_NODE, near_node )
		{
			// in this place, I am near myself... but
			// I should never link myself to me
			if( near_node == node )
				continue;
			LinkWebNode( near_node, node );
			ValidateLink( near_node, NULL,node );
		}
#if 0
		}
#endif
		InvalidateLinks( node, NULL, 1 );
	}
	DeleteList( &list );

	lprintf( WIDE("Finished RelinkANode") );
	levels--;
}

void RelinkNode( PSPACEWEB web, PSPACEWEB_NODE node )
{
	PSPACEWEB_NODE current = web->root;
	if( !current )
	{
		lprintf( WIDE("First node ever.") );
		web->root = node;
		return;
	}
	RelinkANode( current, NULL, node, 0 );

}
void EnumNearNodes( PSPACEWEB web, PSPACEWEB_NODE node )
{

}

// this should have
//  a source node and a destination node, and enumerate possible routes from and to or maybe just points...
void EnumWebNodes( PSPACEWEB web, void (*f)(PSPACEWEB web, PSPACEWEB_NODE node, uintptr_t psvUser ), uintptr_t psvUser )
{

}


uintptr_t GetNodeData( PSPACEWEB_NODE node )
{
	if( node )
		return node->data;
	return 0;
}

PSPACEWEB_NODE AddWebNode( PSPACEWEB web, PC_POINT pt, uintptr_t psv )
{
	PSPACEWEB_NODE node = GetFromSet( SPACEWEB_NODE, &web->nodes );
	SetPoint( node->point, pt );
	node->web = web;
	node->data = psv;
	node->island = 0;
	node->links = NULL;
	EnterCriticalSec( &web->cs );
	RelinkNode( web, node );
	node->flags.bLinked = 1;
	LeaveCriticalSec( &web->cs );
	return node;
}

void MoveWebNode( PSPACEWEB_NODE node, PCVECTOR v )
{
	EnterCriticalSec( &node->web->cs );
	//SetPoint( node->point, v );
	MigrateLink( node, v );
	FindOrphans( node->web );
	if( FindIslands( node->web->root ) )
		DebugBreak();
	LeaveCriticalSec( &node->web->cs );

}

PSPACEWEB CreateSpaceWeb( void )
{
	PSPACEWEB web = New( SPACEWEB );
	InitializeCriticalSec( &web->cs );
	web->nodes = NULL;
	web->root = NULL;
	web->links = NULL;
	web->link_data = NULL;
	return web;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// BEGIN TEST PROGRAM STUB
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------



#ifdef BUILD_WEB_TESTER
#include <psi.h>

EasyRegisterControl( WIDE("VWeb Tester"), 0 );

static struct {
	PSPACEWEB web;
	RCOORD scale;
	_POINT origin;
	PSI_CONTROL control;
	PLIST nodes;
	PSI_CONTROL tester;
	PRENDERER surface;
	FILE *file;
	uint32_t x, y;
	INDEX root;
	PSPACEWEB_NODE pRoot;
	int paint;
} test;

static int OnMouseCommon( WIDE("VWeb Tester") )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	static uint32_t _b;
	if( ( b & MK_LBUTTON ) && !( _b & MK_LBUTTON ) )
	{
		// it's an array in some context, and a pointer in most...
		// the size is the sizeof [nDimensions] but address of that is not PVECTOR
		VECTOR v;
		v[vRight] = (RCOORD)x;
		v[vForward] = (RCOORD)y;
		v[vUp] = 0;
		lprintf( WIDE("----------------- NEW NODE -----------------------") );
		fprintf( test.file, WIDE("%d,%d\n"), x, y );
		fflush( test.file );
		AddLink( &test.nodes, AddWebNode( test.web, v, 0 ) );
		SmudgeCommon( pc );
	}
	if( ( test.x != x ) || ( test.y != y ) )
	{
		//lprintf( WIDE("...") );
		test.x = x;
		test.y = y;
		SmudgeCommon( pc );
	}
	_b = b;
	return 1;
}


struct drawdata {
	Image surface;
	Image icon;
	PLIST path;
	PLIST pathway;
	PSPACEWEB_NODE prior;
	int step;
	int paint;
};

static void DrawLine( PCVECTOR a, PCVECTOR b, CDATA c )
{
	glBegin( GL_LINES );
	glColor4ubv( (unsigned char *)&c );
	glVertex3fv( a );
	glVertex3fv( b );
	glEnd();
}


static uintptr_t CPROC something3d( void* thisnode, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)thisnode;
	struct drawdata *data = (struct drawdata*)psv;
	CDATA c, c2;
	if( !node->flags.bLinked )
		return 0;

	{
		TEXTCHAR tmp[32];
		int c = 0;
		int len;
		c = CountNear( node );

		len = snprintf( tmp, sizeof( tmp ), WIDE("%d[%zd]"), node->paint, NodeIndex( node ) );
		Render3dText( tmp, len, 0xFFFFFFFF, NULL, node->point, TRUE );
	}

	ClearImageTo( data->icon, BASE_COLOR_GREEN );
	Render3dImage( data->icon, node->point, TRUE );

	c = ColorAverage( BASE_COLOR_RED, BASE_COLOR_YELLOW, data->step, 32 );
	c2 = ColorAverage( BASE_COLOR_GREEN, BASE_COLOR_MAGENTA, data->step, 32 );
	data->step++;
	if( data->step > 32 )
		data->step = 0;

	{
		static int had_lines = 0;
		int lines = 0;
		INDEX idx;
		PSPACEWEB_NODE dest;
		PSPACEWEB_LINK link;
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, link )
		{
			lines++;
			if( link->data->paint == data->paint )
				continue;
			link->data->paint = data->paint;
			dest = link->node;
			//lprintf( WIDE("a near node! %d -> %d  v:%d"), NodeIndex( node ), NodeIndex( link->node ), link->data->valence );
#if ( DEBUG_ALL )
			lprintf( WIDE("a near node! %d %d %d %d")
					 ,(int)node->point[vRight], (int)node->point[vForward]
					 , (int)dest->point[vRight], (int)dest->point[ vUp ] );
#endif
			DrawLine( node->point, dest->point, c );

			// draw the perpendicular lines at caps ( 2d perp only)
			{
				VECTOR m;
				RCOORD tmp;
				VECTOR p1, p2;
				// get the slop...
				sub( m, node->point, dest->point );
				// inverse it (sorta)
				m[vRight] = -m[vRight];
				// and swap x/y
				tmp = m[vForward];
				m[vForward] = m[vRight];
				m[vRight] = tmp;
				addscaled( p1, node->point, m, 0.125 );
				addscaled( p2, node->point, m, -0.125 );
				DrawLine( p1, p2, c2 );
				addscaled( p1, dest->point, m, 0.125 );
				addscaled( p2, dest->point, m, -0.125 );
				DrawLine( p1, p2, c2 );
			}

			// perpendicular line at these points?

		}
		{
			INDEX idx2;
			PSPACEWEB_NODE is_path;
			LIST_FORALL( data->path, idx2, PSPACEWEB_NODE, is_path )
			{
				RCOORD tmp;
				VECTOR p1, p2;
				SetPoint( p1, is_path->point );
#define AAA 8
				p1[vRight] -= AAA;
				p1[vForward] -= AAA;
				SetPoint( p2, p1 );
				p2[vRight] += AAA;
				p2[vForward] += AAA;
				DrawLine( p1, p2, BASE_COLOR_WHITE );
				// reverse slope, perpendicular
				tmp = p1[vForward];
				p1[vForward] = p2[vRight];
				p2[vRight] = tmp;
				DrawLine( p1, p2, BASE_COLOR_WHITE );
			}
		}
		{
			INDEX idx2;
			PSPACEWEB_NODE is_path;
			LIST_FORALL( data->pathway, idx2, PSPACEWEB_NODE, is_path )
			{
				RCOORD tmp;
				VECTOR p1, p2;
				SetPoint( p1, is_path->point );
				p1[vRight] -= 3;
				p1[vForward] -= 4;
				SetPoint( p2, p1 );
				p2[vRight] += 3;
				p2[vForward] += 4;
				lprintf( WIDE("path %d,%d"), is_path->point[vRight], is_path->point[vForward] );
				DrawLine( p1, p2, BASE_COLOR_LIGHTCYAN );
				tmp = p1[vForward];
				p1[vForward] = p2[vRight];
				p2[vRight] = tmp;
				DrawLine( p1, p2, BASE_COLOR_LIGHTCYAN );
			}
		}
		if( !lines )
		{
			//lprintf( WIDE("a point has no lines from it!") );
			if( had_lines )
				DebugBreak();
		}
		else
			had_lines = 1;
	}

	if( data->prior )
	{
		//do_line( data->surface, data->prior->point[vRight], data->prior->point[vForward]
		//     , node->point[vRight], node->point[vForward], BASE_COLOR_WHITE );
	}
	data->prior = node;

	return 0; // don't end scan.... foreach can be used for searching too.
}

static void OnDraw3d( WIDE("Space Web(3d)") )( uintptr_t psv )
{
	if( test.web )
	{
		// for each node in web, draw it.
		struct drawdata data;
		data.icon = MakeImageFile( 4, 4 );
		ClearImageTo( data.icon, BASE_COLOR_WHITE );
		data.surface = NULL;
		data.prior = NULL;
		data.paint = ++test.paint;
		data.path = NULL;
		data.pathway = NULL;
		EnterCriticalSec( &test.web->cs );
		{
			VECTOR v;
			v[vRight] = (RCOORD)test.x;
			v[vForward] = (RCOORD)test.y;
			v[vUp] = 0;
			if( test.pRoot )
				FindNearest( &data.path, &data.pathway, test.pRoot, v, 0 );
			//lprintf( WIDE("Draw.") );
		}
		ForAllInSet( SPACEWEB_NODE, test.web->nodes, something3d, (uintptr_t)&data );
		DeleteList( &data.path );
		LeaveCriticalSec( &test.web->cs );
		UnmakeImageFile( data.icon );
	}
}

static uintptr_t CPROC something( void* thisnode, uintptr_t psv )
{
	PSPACEWEB_NODE node = (PSPACEWEB_NODE)thisnode;
	struct drawdata *data = (struct drawdata*)psv;
	CDATA c, c2;

	{
		TEXTCHAR tmp[32];
		int c = 0;
		c = CountNear( node );

		snprintf( tmp, sizeof( tmp ), WIDE("%d[%zd]"), node->paint, NodeIndex( node ) );
		PutString( data->surface, (int32_t)node->point[vRight], (int32_t)node->point[vForward], BASE_COLOR_WHITE, 0, tmp );
	}
	plot( data->surface, (int32_t)node->point[vRight], (int32_t)node->point[vForward], BASE_COLOR_GREEN );

	if( !node->flags.bLinked )
		return 0;

	c = ColorAverage( BASE_COLOR_RED, BASE_COLOR_YELLOW, data->step, 32 );
	c2 = ColorAverage( BASE_COLOR_GREEN, BASE_COLOR_MAGENTA, data->step, 32 );
	data->step++;
	if( data->step > 32 )
		data->step = 0;

	{
		static int had_lines = 0;
		int lines = 0;
		INDEX idx;
		PSPACEWEB_NODE dest;
		PSPACEWEB_LINK link;
		LIST_FORALL( node->links, idx, PSPACEWEB_LINK, link )
		{
			lines++;
			if( link->data->paint == data->paint )
				continue;
			link->data->paint = data->paint;
			dest = (link->invert?link->data->from:link->data->to)->node;
			//lprintf( WIDE("a near node! %d -> %d  v:%d"), NodeIndex( node ), NodeIndex( link->node ), link->data->valence );
#if ( DEBUG_ALL )
			lprintf( WIDE("a near node! %d %d %d %d")
					 ,(int)node->point[vRight], (int)node->point[vForward]
					 , (int)dest->point[vRight], (int)dest->point[ vUp ] );
#endif
			do_line( data->surface, (int32_t)node->point[vRight], (int32_t)node->point[vForward]
					 , (int32_t)dest->point[vRight], (int32_t)dest->point[ vForward ]
					 , c );

			// draw the perpendicular lines at caps ( 2d perp only)
			{
				VECTOR m;
				RCOORD tmp;
				VECTOR p1, p2;
				// get the slop...
				sub( m, node->point, dest->point );
				// inverse it (sorta)
				m[vRight] = -m[vRight];
				// and swap x/y
				tmp = m[vForward];
				m[vForward] = m[vRight];
				m[vRight] = tmp;
				addscaled( p1, node->point, m, 0.125 );
				addscaled( p2, node->point, m, -0.125 );
				do_line( data->surface, (int32_t)p1[vRight], (int32_t)p1[vForward]
						 , (int32_t)p2[vRight], (int32_t)p2[ vForward ]
						 , c2 );
				addscaled( p1, dest->point, m, 0.125 );
				addscaled( p2, dest->point, m, -0.125 );
				do_line( data->surface, (int32_t)p1[vRight], (int32_t)p1[vForward]
						 , (int32_t)p2[vRight], (int32_t)p2[ vForward ]
						 , c2 );
			}

			// perpendicular line at these points?

		}
		{
			INDEX idx2;
			PSPACEWEB_NODE is_path;
			LIST_FORALL( data->path, idx2, PSPACEWEB_NODE, is_path )
			{
				VECTOR p1, p2;
				SetPoint( p1, is_path->point );
#define AAA 8
				p1[vRight] -= AAA;
				p1[vForward] -= AAA;
				SetPoint( p2, p1 );
				p2[vRight] += AAA;
				p2[vForward] += AAA;
				do_line( data->surface, (int32_t)p1[vRight], (int32_t)p1[vForward]
						 , (int32_t)p2[vRight], (int32_t)p2[vForward], BASE_COLOR_WHITE);
				do_line( data->surface, (uint32_t)p2[vRight], (int32_t)p1[vForward]
						 , (int32_t)p1[vRight], (int32_t)p2[vForward], BASE_COLOR_WHITE);
			}
		}
		{
			INDEX idx2;
			PSPACEWEB_NODE is_path;
			LIST_FORALL( data->pathway, idx2, PSPACEWEB_NODE, is_path )
			{
				VECTOR p1, p2;
				SetPoint( p1, is_path->point );
				p1[vRight] -= 3;
				p1[vForward] -= 4;
				SetPoint( p2, p1 );
				p2[vRight] += 3;
				p2[vForward] += 4;
				lprintf( WIDE("path %d,%d"), is_path->point[vRight], is_path->point[vForward] );
				do_line( data->surface, (int32_t)p1[vRight], (int32_t)p1[vForward]
						 , (int32_t)p2[vRight], (int32_t)p2[vForward], BASE_COLOR_LIGHTCYAN);
				do_line( data->surface, (uint32_t)p2[vRight], (int32_t)p1[vForward]
						 , (int32_t)p1[vRight], (int32_t)p2[vForward], BASE_COLOR_LIGHTCYAN);
			}
		}
		if( !lines )
		{
			//lprintf( WIDE("a point has no lines from it!") );
			if( had_lines )
				DebugBreak();
		}
		else
			had_lines = 1;
	}

	if( data->prior )
	{
		//do_line( data->surface, data->prior->point[vRight], data->prior->point[vForward]
		//     , node->point[vRight], node->point[vForward], BASE_COLOR_WHITE );
	}
	data->prior = node;

	return 0; // don't end scan.... foreach can be used for searching too.
}

static int OnDrawCommon( WIDE("VWeb Tester") )( PSI_CONTROL pc )
{
	Image surface = GetControlSurface( pc );
	ClearImageTo( surface, SetAlpha( BASE_COLOR_BLUE, 32 ) );
	if( test.web )
	{
		// for each node in web, draw it.
		struct drawdata data;
		data.step = 0;
		data.surface = surface;
		data.prior = NULL;
		data.paint = ++test.paint;
		data.path = NULL;
		data.pathway = NULL;
		EnterCriticalSec( &test.web->cs );
		{
			VECTOR v;
			v[vRight] = (RCOORD)test.x;
			v[vForward] = (RCOORD)test.y;
			v[vUp] = 0;
			if( test.pRoot )
				FindNearest( &data.path, &data.pathway, test.pRoot, v, 0 );
			//lprintf( WIDE("Draw.") );
		}
		ForAllInSet( SPACEWEB_NODE, test.web->nodes, something, (uintptr_t)&data );
		DeleteList( &data.path );
		LeaveCriticalSec( &test.web->cs );
	}
	return 1;
}

static int OnKeyCommon( WIDE("VWeb Tester") )( PSI_CONTROL pc, uint32_t key )
{
	if( IsKeyPressed(key) && KEY_CODE(key) == KEY_SPACE )
		update_pause = 0;
	if( IsKeyPressed(key) && KEY_CODE(key) == KEY_N )
	{
		test.root++;
		test.pRoot = GetUsedSetMember( SPACEWEB_NODE, &test.web->nodes, test.root );
		if( !test.pRoot )
		{
			test.root = 0;
			test.pRoot = GetUsedSetMember( SPACEWEB_NODE, &test.web->nodes, test.root );
		}
		SmudgeCommon( pc );
	}
	return 0;
}

void CPROC MoveWeb( uintptr_t psv )
{
	PSPACEWEB_NODE node;
	VECTOR v;
	INDEX idx;
	static int cycle;
	//return;
	update_pause -= 50;
	if( update_pause < 0 )
		update_pause = 0;
	if( update_pause > 0 )
		return;

	// 500 has some issues.
	//	if( cycle >= 500 )
	//if( cycle > 300 )
		update_pause = 150000;
	//else
	//   update_pause = 1000;
	cycle++;
	lprintf( WIDE("cycle %d"), cycle );
	LIST_FORALL( test.nodes, idx, PSPACEWEB_NODE, node )
	{
		if( idx %10 == 0 )
		{
			v[vRight] = (RCOORD)(rand() %13)-6;
			v[vForward] = (RCOORD)(rand() %13)-6;
			v[vUp] = (RCOORD)(rand() %13)-6;
			add( v, v, node->point );
			MoveWebNode( node, v );
		}
		//break;
	}
	SmudgeCommon( test.tester );
}

static uintptr_t OnInit3d( WIDE("Space Web(3d)" ) )(PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	return 1;
}


SaneWinMain( argc,argv )
{
	InvokeDeadstart();
	test.file = fopen( "points.dat", "at+" );
	fseek( test.file, 0, SEEK_SET );
	test.tester = MakeNamedCaptionedControl( NULL, WIDE("VWeb Tester"), 0, 0, 512, 512, -1, WIDE("Test Space Web") );
	test.surface = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED, 512, 512, 0, 0 );
	AttachFrameToRenderer( test.tester, test.surface );
	DisplayFrame( test.tester );
	Redraw( test.surface );
	test.web = CreateSpaceWeb();
	if(0)
	{
		int x, y;
		for( x = 0; x < 400; x += 50 )
			for( y = 0; y < 400; y += 50 )
			{
				VECTOR v;
				v[vRight] = (RCOORD)(rand()%500);
				v[vForward] = (RCOORD)(rand()%500);
				v[vUp] = 0;
				AddWebNode( test.web, v, 0 );
			}
	}
	{
		TEXTCHAR buf[256];
		int x, y;
		EnterCriticalSec( &test.web->cs );
		while( fgets( buf, sizeof( buf ), test.file ) )
		{
			VECTOR v;
			sscanf( buf, WIDE("%d,%d"), &x, &y );
			v[vRight] = (RCOORD)x;
			v[vForward] = (RCOORD)y;
			v[vUp] = 0;
			lprintf( WIDE("----------------- NEW NODE -----------------------") );
			//fprintf( test.file, WIDE("%d,%d\n"), x, y );
			AddLink( &test.nodes, AddWebNode( test.web, v, 0 ) );
		}
		fseek( test.file, 0, SEEK_SET );
		LeaveCriticalSec( &test.web->cs );

	}
	Redraw( test.surface );
	AddTimer( 5, MoveWeb, 0 );
	while( 1 )
		WakeableSleep( 10000 );
	return 0;
}
EndSaneWinMain()
#endif
