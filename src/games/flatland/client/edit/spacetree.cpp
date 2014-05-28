#include <stdio.h>
#include <sharemem.h>
#include <logging.h>
#define SPACE_STRUCT_DEFINED

#include <vectlib.h>

// if 3d space partitioning - AREAS will be 27 - ICK!
#define AREAS 9

typedef struct spacenode {
    void *data;
    _POINT min, max;
    //int children; // hmm... this may or may not be usable...
    struct spacenode *parent; // may have as many parents as it is.
    // think this will be more space than use... cept THE root.
    struct spacenode **me; // can span up to the same number of areas...
    struct spacenode *prior, *next; // double link list of other nodes that are me.
    struct spacenode *area[AREAS]; // other areas linked off this...
} SPACENODE, *PSPACENODE;

#define AREA_UPPERLEFT 0
#define AREA_UP 1
#define AREA_UPPERRIGHT 2
#define AREA_LEFT 3
#define AREA_HERE 4
#define AREA_RIGHT 5
#define AREA_LOWERLEFT 6
#define AREA_DOWN 7
#define AREA_LOWERRIGHT 8

// search is first done left, up, right, down.... 

#include "spacetree.h"

//#define HANG_DEBUG

//FILE *log_file;

void *GetNodeData( PSPACENODE node )
{
    return node->data;
}


static PSPACENODE pDeepest;
static int deepest;

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
            //Log2( "Deepest: %d %08x", level, root );
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
                _lprintf( DBG_RELAY )( "Node in quadrant %d does not reference itself..??" ,i );
            if( root->area[i]->parent != root )
            {
                _lprintf(DBG_RELAY)( "Node %08x in quadrant %d does not reference me?(%08x)"
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
            if( ((int)((*root)->parent)) & 3 )
            {
                Log( "We're in trouble - attempting to fix..." );
            (*root)->parent = (PSPACENODE)((int)((*root)->parent) + 1 );
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
        if( root = node->me )
            *(node->me) = NULL;
        while( node->parent )
        {
            //Log2( "Updating %08x's children from %d", node->parent, node->parent->children );
            //node->parent->children -= node->children + 1;
            node->parent = node->parent->parent;
        }
        node->me = NULL;
        return root;
    }
    return NULL;
}

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
    Release( node );
}

void CollapseNodes( PSPACENODE node )
{
    PSPACENODE space, space2, parent, next;
    int i;
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
        Log( "Couldn't remove nothing" );
        return NULL;
    }

    chunk = space;
    while( chunk->next )
        chunk = chunk->next;
    while( space->prior )
        space = space->prior;
    if( !chunk->me || *chunk->me != chunk )
    {
        Log1( "Couldn't remove something already removed (%08X)", chunk );
        return NULL;
    }

    last = chunk;         // keep this for second stage looping...

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
        Release( chunk );
        chunk = next;
    }
    if( chunk )
    {
        if( !chunk->next )
            Log( "We're screwed! should be 2 nodes - the original, and one other" );
        if( chunk != space )
        {
            SetPoint( space->min, chunk->min );
            SetPoint( space->max, chunk->max );
            chunk->next->prior = NULL;
            Release( chunk );
        }
        else
        {
            Release( chunk->next );
            chunk->next = NULL;
        }
    }
    if( space->prior || space->next )
    {
        Log( "Fatal - removed node is not solitary..." );
    }
    //Log( "This should be non null result..." );
    return space;
}


PSPACENODE RemoveSpaceNode( PSPACENODE space )
{
    // unlink from the tree, reque all children back on the tree...
    // unlink all relatives from the tree, again reque all children
    int i;
    PSPACENODE last, chunk, next, *root;
    if( !space )
    {
        Log( "Couldn't remove nothing" );
        return NULL;
    }
    // first chunk may not actually be IN the tree... but just track
    // the space for rehanging as a child removed and rehung..
    if( space->prior )
    {
        Log( "***Removing a node which is not the master... may be bad!" );
    }
    chunk = space;
    while( chunk->next )
        chunk = chunk->next;
    if( !chunk->me || *chunk->me != chunk )
    {
        Log1( "Couldn't remove something already removed (%08X)", chunk );
        return NULL;
    }
    last = chunk;         // keep this for second stage looping...
    root = chunk->me;
    if( chunk->parent ) // if this was the root of the tree.. don't go back
    {
        while( (*root)->parent ) // while there is a parent of this...
        {
            if( ((int)((*root)->parent)) & 3 )
            {
                Log( "We're in trouble - attempting to fix..." );
            (*root)->parent = (PSPACENODE)((int)((*root)->parent) + 1 );
            }
            //Log3( "step to root...%08x %08x %08x", (*root), (*root)->parent, (*root)->parent->me );
            root = (*root)->parent->me;
        }
       //Log( "step to root Final..." );
        root = (*root)->me; // step back one more - REAL root of tree.
    }

   while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
   {
    //Log( "To grab chunk..." );
        GrabSpace( chunk ); // space->me is NOT valid...
        //chunk->children = 0;
        chunk = chunk->prior;
        //Log3( "Grabbed a chunk...%08x %08x %08x", chunk, (chunk)?chunk->prior:0, (chunk)?chunk->next:0 );
    }

    chunk = last;
   while( chunk && ( chunk->prior || ( !chunk->next && !chunk->prior ) ) )
   {
        for( i = 0; i < AREAS; i++ )
            if( chunk->area[i] )
            {  
                //Log( "Rehang child of me..." );       
                // if this was linked to itself within the same node
                // then by now it is currently unlinked from itself, and
                // this part of this loop would not run... therefore it
                // should be safe and never hit this point...
                if( chunk->area[i]->data == space->data )
                {
                    Log( "DIE DIE! we're hanging on ourselves still :(" );
                }
                HangSpaceNode( root, RemoveSpaceNode2( root, chunk->area[i] ) );
                if( chunk->area[i] )
                {
                    Log( "Removal failed somehow - or perhaps we requeued more?" );
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
        Release( chunk );
        chunk = next;
    }
    if( chunk )
    {
        if( !chunk->next )
            Log( "We're screwed! should be 2 nodes - the original, and one other" );
        if( chunk != space )
        {
            Log( "Abnormal chunk space linking - but that's ok..." );
            SetPoint( space->min, chunk->min );
            SetPoint( space->max, chunk->max );
            chunk->next->prior = NULL;
            Release( chunk );
        }
        else
        {
            Release( chunk->next );
            chunk->next = NULL;
        }
    }
    if( space->prior || space->next )
    {
        Log( "Fatal - removed node is not solitary..." );
    }
    /*
    Log4( "This should be non null result...%08x %08x %08x %08x", space->prior, space->next, space->parent, space->me );
    for( i = 0; i < AREAS; i++ )
    {
        Log1( "My areas are: %08x", space->area[i] );
    }
    if( root && *root )
    { 
        Log4( "This should be non null result...%08x %08x %08x %08x", (*root)->prior, (*root)->next, (*root)->parent, (*root)->me );
        for( i = 0; i < AREAS; i++ )
        {
            Log1( "My areas are: %08x", (*root)->area[i] );
        }
    }
    */
    return space;
}

void *FindPointInSpace( PSPACENODE root, P_POINT p, int (*Validate)(void *data, P_POINT p ) )
{
    while( root )
    {
        //Log1( "Finding point in space: %08x", root );
        if( p[0] <= root->min[0] ) // space is left of here total.
        {
            if( p[1] >= root->max[1] ) // space is above this.
            {
                root = root->area[AREA_UPPERLEFT];
            }
            else if( p[1] <= root->min[1] ) // space is below this.
            {
                root = root->area[AREA_LOWERLEFT];
            }
            else
            {
                root = root->area[AREA_LEFT];
            }
        }
        else if( p[1] >= root->max[1] ) // space is above this
        {
            if( p[0] <= root->min[0] ) // space is left of root...
            {
                root = root->area[AREA_UPPERLEFT];
            }
            else if( p[0] >= root->max[0] ) // space is right of this...
            {
                root = root->area[AREA_UPPERRIGHT];
            }
            else
            {
                root = root->area[AREA_UP];
            }
        }
        else if( p[0] >= root->max[0] ) // space is right of this one...
        {
            if( p[1] <= root->min[1] ) // space is below this one
            {
                root = root->area[AREA_LOWERRIGHT];
            }
            else
            {
                root = root->area[AREA_RIGHT];
            }
        }
        else if( p[1] <= root->min[1] )
        {
            root = root->area[AREA_DOWN];
        }
        else // point is in this center section....
        {
            if( Validate )
            {
                //Log2( "Validating data %08x->%08x", root, root->data );
                if( Validate( root->data, p ) )
                    return root->data;
            }
            root = root->area[AREA_HERE];
        }
    }
    if( root )
        return root->data; // null.
    return NULL;
}

// when duplicating a node the new node has the parameters min
// and max... the old node is adjusted to exclude min, max

static PSPACENODE DuplicateSpaceNodeEx( PSPACENODE space DBG_PASS )
#define DuplicateSpaceNode(s) DuplicateSpaceNodeEx((s) DBG_SRC )
#define DupNodeEx DuplicateSpaceNodeEx
#define DupNode DuplicateSpaceNode 
{
    PSPACENODE dup = New( SPACENODE );
    MemSet( dup, 0, sizeof( SPACENODE ) );
    dup->data = space->data;
    SetPoint( dup->min, space->min );
    SetPoint( dup->max, space->max );
    if( space->next )
        space->next->prior = dup;
    dup->prior = space;
    dup->next = space->next;
    space->next = dup;
    //Log( "Duplicating Node..." );
    return dup;
}

void HangSpaceNodeExx( PSPACENODE *root, PSPACENODE parent, PSPACENODE space DBG_PASS )
{
    PSPACENODE here, dup;
    _POINT min, max;
    if( !space || !root )
        return;
    while( ( here = *root ) )
    {
        int nextspace = 0;
        //here->children += space->children + 1; // this WILL have one more child.
        parent = *root;
#ifdef HANG_DEBUG
        {
            char name[256], rootname[256];
            GetNameText( rootname, ((PSECTOR)here->data)->name );
            GetNameText( name, ((PSECTOR)space->data)->name );
            Log2( "adding node (%08x) under (%08x)", space, parent );   
        }
#endif
        if( space->max[0] <= here->min[0] ) // space is left of here total.
        {
            if( space->min[1] >= here->max[1] ) // space is above this.
            {
#ifdef HANG_DEBUG
                Log1( "Total area to upper left....%d", __LINE__ );
#endif
                root = here->area + AREA_UPPERLEFT;
            }
            else if( space->max[1] <= here->min[1] ) // space is below this.
            {
#ifdef HANG_DEBUG
                Log1( "Total area to lower left....%d", __LINE__ );
#endif
                root = here->area + AREA_LOWERLEFT;
            }
            else
            {
                // this_miny < root_maxy
                // this_maxy > root_miny
                if( space->min[1] < here->min[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[1] = here->min[1];
                    space->min[1] = here->min[1];
#ifdef HANG_DEBUG
                    Log1( "part is to lower left....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERLEFT, parent, dup DBG_RELAY );
                }
                if( space->max[1] > here->max[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[1] = here->max[1];
                    space->max[1] = here->max[1];
#ifdef HANG_DEBUG
                    Log1( "part is to upper left....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERLEFT, parent, dup DBG_RELAY );
                }
#ifdef HANG_DEBUG
                Log1( "This (part) is to the left....%d", __LINE__ );
#endif
                root = here->area + AREA_LEFT;
            }
        }
        else if( space->min[1] >= here->max[1] ) // space is above this
        {
            if( space->max[0] <= here->min[0] ) // space is left of here...
            {
#ifdef HANG_DEBUG
                Log1( "Total area to upper left....%d", __LINE__ );
#endif
                root = here->area + AREA_UPPERLEFT;
            }
            else if( space->min[0] >= here->max[0] ) // space is right of this...
            {
#ifdef HANG_DEBUG
                Log1( "Total area to upper right....%d", __LINE__ );
#endif
                root = here->area + AREA_UPPERRIGHT;
            }
            else
            {   
                // this_minx < root_maxx
                // this_maxx > root_minx
                if( space->min[0] < here->min[0] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[0] = here->min[0];
                    space->min[0] = here->min[0];
#ifdef HANG_DEBUG
                    Log1( "This part is to upper left....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERLEFT, parent, dup DBG_RELAY);
                }
                if( space->max[0] > here->max[0] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[0] = here->max[0];
                    space->max[0] = here->max[0];
#ifdef HANG_DEBUG
                    Log1( "This part is to upper right...%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, parent, dup DBG_RELAY);
                }
#ifdef HANG_DEBUG
                Log1( "This (part) is to the up....%d", __LINE__ );
#endif
                root = here->area + AREA_UP;
            }
        }
        else if( space->min[0] >= here->max[0] ) // space is right of this one...
        {
            // UPPER_RIGHT is handled above when space is above and right.
            if( space->max[1] <= here->min[1] ) // space is below this one
            {
#ifdef HANG_DEBUG
                Log1( "Total area is right and below this....%d", __LINE__ );
#endif
                root = here->area + AREA_LOWERRIGHT;
            }
            else
            {
                // this_miny < root_maxy
                // this_maxy > root_miny
                if( space->min[1] < here->min[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[1] = here->min[1];
                    space->min[1] = here->min[1];
#ifdef HANG_DEBUG
                    Log1( "This part is to lower right....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, parent, dup DBG_RELAY );
                }
                if( space->max[1] > here->max[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[1] = here->max[1];
                    space->max[1] = here->max[1];
#ifdef HANG_DEBUG
                    Log1( "This part is to upper right....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, parent, dup DBG_RELAY );
                }
#ifdef HANG_DEBUG
                Log1( "This (part) is to the right....%d", __LINE__ );
#endif
                root = here->area + AREA_RIGHT;
            }
        }
        else if( space->max[1] <= here->min[1] )
        {
            //LOWERLEFT is handled above....
            //LOWER_RIGHT is handles above....
            {   
                // this_minx < root_maxx
                // this_maxx > root_minx
                if( space->min[0] < here->min[0] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[0] = here->min[0];
                    space->min[0] = here->min[0];
#ifdef HANG_DEBUG
                    Log1( "This part is to lower left...%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERLEFT, parent, dup DBG_RELAY );
                }
                if( space->max[0] > here->max[0] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[0] = here->max[0];
                    space->max[0] = here->max[0];
#ifdef HANG_DEBUG
                    Log1( "This part is to lower right....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, parent, dup DBG_RELAY );
                }
#ifdef HANG_DEBUG
                Log1( "This (part) is to the down....%d", __LINE__ );
#endif
                root = here->area + AREA_DOWN;
            }
        }
        else // overlaps this center section....
        {
            RCOORD miny, maxy;
            miny = space->min[1];
            maxy = space->max[1];

            if( space->min[0] < here->min[0] ) // part of this is to the left
            {
                if( space->max[1] > here->max[1] ) // part of this is above
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[0] = here->min[0];
                    dup->min[1] = here->max[1];
                    space->max[1] = here->max[1];
#ifdef HANG_DEBUG
                    Log1( "This part(2) is to upper left....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERLEFT, parent, dup DBG_RELAY );
                }
                if( space->max[1] < here->min[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->max[0] = here->min[0];
                    dup->max[1] = here->min[1];
                    space->min[1] = here->min[1];
#ifdef HANG_DEBUG
                    Log1( "This part(2) is to lower left....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERLEFT, parent, dup DBG_RELAY );
                }
                dup = DupNodeEx( space DBG_RELAY );
                if( !(space->prior) )
                    space = DupNodeEx( space DBG_RELAY );
                dup->max[0] = here->min[0];
#ifdef HANG_DEBUG
                Log1( "This part(2) is to the left....%d", __LINE__ );
#endif
                HangSpaceNodeExx( here->area + AREA_LEFT, parent, dup DBG_RELAY );
            }

            space->min[1] = miny;
            space->max[1] = maxy;

            if( space->max[0] > here->max[0] ) // part of this is to the right...
            {
                if( space->max[1] > here->max[1] ) // part of this is above
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[0] = here->max[0];
                    dup->min[1] = here->max[1];
                    space->max[1] = here->max[1];
#ifdef HANG_DEBUG
                    Log1( "This part(2) is to upper right....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_UPPERRIGHT, parent, dup DBG_RELAY );
                }
                if( space->min[1] < here->min[1] )
                {
                    dup = DupNodeEx( space DBG_RELAY );
                    if( !(space->prior) )
                        space = DupNodeEx( space DBG_RELAY );
                    dup->min[0] = here->max[0];
                    dup->max[1] = here->min[1];
                    space->min[1] = here->min[1];
#ifdef HANG_DEBUG
                    Log1( "This part(2) is to lower right....%d", __LINE__ );
#endif
                    HangSpaceNodeExx( here->area + AREA_LOWERRIGHT, parent, dup DBG_RELAY );
                }
                dup = DupNodeEx( space DBG_RELAY );
                if( !(space->prior) )
                    space = DupNodeEx( space DBG_RELAY );
                dup->min[0] = here->max[0];
#ifdef HANG_DEBUG
                Log1( "This part(2) is to the right....%d", __LINE__ );
#endif
                HangSpaceNodeExx( here->area + AREA_RIGHT, parent, dup DBG_RELAY );
            }

            space->max[1] = maxy;
            space->min[1] = miny;

            if( space->max[1] > here->max[1] )
            {
                dup = DupNodeEx( space DBG_RELAY );
                if( !(space->prior) )
                    space = DupNodeEx( space DBG_RELAY );
                dup->min[1] = here->max[1];
                space->max[1] = here->max[1];
#ifdef HANG_DEBUG
                Log1( "This part(2) is to the up....%d", __LINE__ );
#endif
                HangSpaceNodeExx( here->area + AREA_UP, parent, dup DBG_RELAY );
            }
            if( space->min[1] < here->min[1] )
            {
                dup = DupNodeEx( space DBG_RELAY );
                if( !(space->prior) )
                    space = DupNodeEx( space DBG_RELAY );
                dup->max[1] = here->min[1];
                space->min[1] = here->min[1];
#ifdef HANG_DEBUG
                Log1( "This part(2) is to the down....%d", __LINE__ );
#endif
                HangSpaceNodeExx( here->area + AREA_DOWN, parent, dup DBG_RELAY );
            }
            // resulting space should be within this area totally...
            // might only be partial... but will be within.
#ifdef HANG_DEBUG
            Log1( "This (part) is here....%d", __LINE__ );
#endif
            root = here->area + AREA_HERE;
        }
    }                                   
    
    {
#ifdef HANG_DEBUG
        Log2( " -- Added space node %08x %08x-- ", space, parent );
#endif
        space->parent = parent;
        *root = space;
        space->me = root;
    }
}

PSPACENODE ReAddSpaceNodeEx( PSPACENODE *root, PSPACENODE node, P_POINT min, P_POINT max DBG_PASS )
{
    SetPoint( node->min, min );
    SetPoint( node->max, max );
    HangSpaceNodeExx( root, NULL, node DBG_RELAY );
    return node;
}

PSPACENODE AddSpaceNodeEx( PSPACENODE *root, void *data, P_POINT min, P_POINT max DBG_PASS )
{
    PSPACENODE space;
    if( !data )
        Log( "Storing NULL data?!" );
#ifdef HANG_DEBUG
    Log1( "Adding node for %08x", data );
#endif
    if( !root )
    {
        Log( "Cannot add a node to an unreferenced tree (root = NULL)" );
        return NULL; // can't add to a non tree...
    }
    space = New( SPACENODE );
    MemSet( space, 0, sizeof( SPACENODE ) );
    SetPoint( space->min, min );
    SetPoint( space->max, max );
    space->data = data;
#ifdef HANG_DEBUG
    {
        char name[256];
        GetNameText( name, ((PSECTOR)data)->name );
        Log1( "---- Building nodes for %s ----", name );    
    }
#endif
    HangSpaceNodeExx( root, NULL, space DBG_RELAY );
    return space; // to attach the node to the related data....
}

void MoveSpace( PSPACENODE *root, PSPACENODE space, P_POINT min, P_POINT max )
{
    RemoveSpaceNode( space );
    SetPoint( space->min, min );
    SetPoint( space->max, max );
    HangSpaceNode( root, space );   
}

void DeleteSpaceNode( PSPACENODE node )
{
    //Log( "Delete Node?" );
    if( node )
    {
        PSPACENODE parent;
        Release( node );
   }
}

void DeleteSpaceTree( PSPACENODE *root )
{
    int i;
    if( *root )
    {
        for( i = 0; i < AREAS; i++ )
            DeleteSpaceTree( (*root)->area+i );
        Release( *root );
        (*root) = NULL;
    }
}


FILE *spaceout;
int maxlevels, levels, nodecount;

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

void WriteSpaceTree( PSPACENODE root )
{
    int i;
    //if( levels > 5 )
    //  return;
    if( !root )
        return;
    nodecount++;
    levels++;
    if( levels > maxlevels )
        maxlevels = levels;
    fprintf( spaceout, "(%d) Node(%08x) under %08x has %d"
                    , levels, root, root->parent, CountSpaces( root ) );
    fprintf( spaceout, "(%g,%g,%g)-(%g,%g,%g) data %08x\n"
                    , root->min[0]
                    , root->min[1]
                    , root->min[2]
                    , root->max[0]
                    , root->max[1]
                    , root->max[2]
                    , root->data );
    fflush( spaceout );
    {
        for( i = 0; i < AREAS; i++ )
        {
            if( root->area[i] )
            {
                fprintf( spaceout, "Q%d:", i );
                WriteSpaceTree( root->area[i] );
            }
        }
    }                       
    fflush( spaceout );
    levels--;
}

void DumpSpaceTree( PSPACENODE root )
{
    if( !spaceout )
        spaceout = fopen( "SpaceTree.dump", "wt" );
    if( spaceout )
    {
        maxlevels = 0;
        levels = 0;
        nodecount = 0;
        WriteSpaceTree( root );
        {
            fprintf( spaceout, "Max levels of nodes: %d total: %d"
                            , maxlevels
                            , nodecount
                    );
        }
        fprintf( spaceout, "----------------------------------------\n" );
        fflush( spaceout );
    }
}

