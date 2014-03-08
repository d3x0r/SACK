
// well this code could have been used... but sectors on the
// same span would be skipped - also if the sector is larger than the
// root it definatly occupies 2 or more quadrants.... 

#include <stdio.h>
#include <sharemem.h>
#include <logging.h>
#include "quadtree.h"

#include <world.h> // temporary!!!

void ValidateQuadTree( PQUADNODE root, int quad, P_POINT ref )
{
    int i;
    char name[256], pname[256], children;
    if( !root )
        return;
    GetNameText( name, ((PSECTOR)root->data)->iName );
    if( quad >= 0 )
        GetNameText( pname, ((PSECTOR)root->parent->data)->name );
    children = 0;
    for( i = 0; i < BRANCHES ; i++ )
    {
        if( root->quads[i] )
        {
            children += 1 + root->quads[i]->children;
            if( root->quads[i]->me != &root->quads[i] )
                Log1( "Node in quadrant %d does not reference itself..??", i );
            if( root->quads[i]->parent != root )
                Log1( "Node in quadrant %d does not reference me??", i );
            switch( quad )
            {
            case 0:
                if( root->p[0] >= ref[0] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
                if( root->p[1] < ref[1] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
               break;
            case 1:
                if( root->p[0] < ref[0] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
                if( root->p[1] < ref[1] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
               break;
            case 2:
                if( root->p[0] >= ref[0] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
                if( root->p[1] >= ref[1] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
               break;
            case 3:
                if( root->p[0] < ref[0] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
                if( root->p[1] >= ref[1] )
                    Log3( "%s in %s is in wrong quadrant...(%d)", name, pname, __LINE__ );
               break;
            }
            if( quad < 0 )
                ValidateQuadTree( root->quads[i], i, root->p );
            else
                ValidateQuadTree( root->quads[i], quad, ref );
        }
    }
    if( children != root->children )
        Log3( "Children count of %s does not match actual (%d vs %d)"
                    , name, children, root->children );
}

// return the address of the root thing... 
//PQUADNODE *
void GrabQuad( PQUADNODE node )
{
    if( node )
    {
        if( node->me )
            *(node->me) = NULL;
        while( node->parent )
        {
            node->parent->children -= node->children + 1;
            node->parent = node->parent->parent;
        }
        //node->parent = NULL;
        //return node->me;
    }
    //return NULL;
}

void RehangQuads( PQUADNODE *root, PQUADNODE quad )
{
    int i;
    if( quad )
    {
        GrabQuad( quad );
        for( i = 0; i < BRANCHES; i++ )
            RehangQuads( root, quad->quads[i] );
        HangQuadNode( root, quad );
    }
}

void RotateQuad( PQUADNODE *root, int quad )
{
    PQUADNODE node, node2;
   int otherquad = (BRANCHES-1)-quad;
    Log2( "Rotating quad: %d into %d", quad, otherquad );
    if( !root || !(*root) )
        return;
    node = *root;
    if( node->quads[quad] )
    {
        int n1, n2; // for lack of better names.
// note - the calculations previously used +1 to include the node
// being moved itself - but result was -1 then +1 which is net 0.

        *root = node->quads[quad];
        
        if( ( node2 = (*root)->quads[otherquad] ) )
            n2 = node2->children + 1;
        else
            n2 = 0;

        (*root)->me      = node->me;
        (*root)->parent = node->parent;

        node->me         = &(*root)->quads[otherquad];
        node->parent    = (*root);

        if( node2 )
        {
            node2->me    = &node->quads[quad];
            node2->parent= node;
        }

        node->children -= ((*root)->children + 1);
        node->children += n2;
        (*root)->children -= n2;
        (*root)->children += ( node->children + 1);

        node->quads[quad] = node2;
        (*root)->quads[otherquad] = node;

        // node == old root now part of otherquad of root, containing
        //         quad of *root in itself.
        // *root == new root 
            
        // there are portion of quadrant quad ^ 1 of root which should be on node
        // either otherquad or otherquad ^ 1
        {
            int errquad = quad^1;
            RehangQuads( root, node->quads[errquad] );
            errquad = quad^2;
            RehangQuads( root, node->quads[errquad] );
        }

        // there is a portion of quadrant otherquad ^ 2 of node which should be
        // part of quad or quad^1 of root

        // there is a portion of quadrant quad ^ 2 of root which should be on node
        // either otherquad or otherquad ^ 2

    

    }
    else    
        Log( "Rotating quadrant from NULL into current... no operation" );
}

void TestBalance( PQUADNODE *root )
{
    int i;
    PQUADNODE quad1= (*root)->quads[0]
            , quad2=(*root)->quads[1]
            , quad3=(*root)->quads[2]
            , quad4=(*root)->quads[3];

    ValidateQuadTree( *root, -1, NULL );
    Log("--------Initialvalidateend--------" );

    for( i = 0; i < BRANCHES/2; i++ )
    {
      quad1 = (*root)->quads[i];
      quad2 = (*root)->quads[(BRANCHES-1)-i];
      if( !quad1 && !quad2 )
      {
        Log( "Opposing quadrants are empty?!" );
        continue;
      }
        if( !quad2 )
        {
            RotateQuad( root, i );
            ValidateQuadTree( *root, -1, NULL );
            Log( "Quad1 move --------------- End" );
            DumpQuadTree( *root );
            return;
            i = -1;
        }
        else if( !quad1 )
        {
            RotateQuad( root, (BRANCHES-1)-i );
            ValidateQuadTree( *root, -1, NULL );
            Log( "Quad2 move --------------- End" );
            DumpQuadTree( *root );
            return;
            i = -1;
        }
    }
    for( i = 0; i < BRANCHES/2; i++ )
    {
      quad1 = (*root)->quads[i];
      quad2 = (*root)->quads[(BRANCHES-1)-i];
        {
            if( ( (quad1->children+1) / (quad2->children+1) ) > 1 )
            {
                RotateQuad( root, i );
                ValidateQuadTree( *root, -1, NULL );
                Log( "Quad1 into 2 move --------------- End" );
                DumpQuadTree( *root );
                return;
                i = -1;
            }
         else if( ( (quad2->children+1) / (quad1->children+1) ) > 1 )
            {
                RotateQuad( root, (BRANCHES-1)-i );
                ValidateQuadTree( *root, -1, NULL );
                Log( "Quad2 into 1 move --------------- End" );
                DumpQuadTree( *root );
                return;
                i = -1;
            }
        }
    }
}

void RemoveQuadNode( PQUADNODE quad )
{
    int i;
    PQUADNODE root;
    GrabQuad( quad ); // quad->me is still valid...
    for( i = 0; i < BRANCHES; i++ )
        if( quad->quads[i] )
        {
            HangQuadNode( quad->me, quad->quads[i] );
            quad->quads[i] = NULL;
        }
    quad->children = 0;
}

void HangQuadNode( PQUADNODE *root, PQUADNODE quad )
{
    PQUADNODE parent = NULL;
    if( !quad || !root )
        return;
    while( *root )
    {
        int nextquad = 0;
        (*root)->children += quad->children + 1; // this WILL have one more child.

        if( quad->min[0] >= (*root)->p[0] )
            nextquad += 1;
        if( quad->p[1] < (*root)->p[1] )
            nextquad += 2;
#if (BRANCHES==8)
        if( quad->p[2] < (*root)->p[2] )
            nextquad += 4;
#endif
        parent = *root;
        root = (*root)->quads + nextquad;
    }
    quad->parent = parent;
    *root = quad;
    quad->me = root;
}

void AddQuadNode( PQUADNODE *root, void *data, P_POINT min, P_POINT max )
{
    PQUADNODE quad;
    if( !data )
        Log( "Storing NULL data?!" );
    if( !root )
    {
        Log( "Cannot add a node to an unreferenced tree (root = NULL)" );
        return; // can't add to a non tree...
    }
    quad = Allocate( sizeof( QUADNODE ) );
    MemSet( quad, 0, sizeof( QUADNODE ) );
    SetPoint( quad->min, min );
    SetPoint( quad->max, max );
    quad->data = data;
    HangQuadNode( root, quad );
}

void MoveQuadNode( PQUADNODE *root, PQUADNODE quad, P_POINT p )
{
    RemoveQuadNode( quad );
    SetPoint( quad->p, p );
    HangQuadNode( root, quad ); 
}

void DeleteQuadTree( PQUADNODE *root )
{
    int i;
    if( *root )
    {
        for( i = 0; i < BRANCHES; i++ )
            DeleteQuadTree( (*root)->quads+i );
        Release( *root );
        (*root) = NULL;
    }
}


FILE *quadout;
int maxlevels, levels;

void WriteQuadTree( PQUADNODE root )
{
    int i;
    char name[256];
    //if( levels > 5 )
    //  return;
    if( !root )
        return;
    levels++;
    if( levels > maxlevels )
        maxlevels = levels;
    GetNameText( name, ((PSECTOR)root->data)->name );
    fprintf( quadout, "(%d) Node(%s) has %d ("
                    , levels, name, root->children );
    for( i = 0; i < BRANCHES; i++ )
    {
        if( i )
            fprintf( quadout, "," );
        if( root->quads[i] )
            fprintf( quadout, "%d"
                        , root->quads[i]->children );
        else
            fprintf( quadout, "(0)" );
    }
    fprintf( quadout, ") children, (%g,%g,%g)-(%g,%g,%g) data %08x\n"
                    , root->min[0]
                    , root->min[1]
                    , root->min[2]
                    , root->max[0]
                    , root->max[1]
                    , root->max[2]
                    , root->data );
    {
        for( i = 0; i < BRANCHES; i++ )
        {
            if( root->quads[i] )
            {
                fprintf( quadout, "Q%d:", i );
                WriteQuadTree( root->quads[i] );
            }
        }
    }                       
    levels--;
}

void DumpQuadTree( PQUADNODE root )
{
    quadout = fopen( "quadtree.dump", "at+" );
    if( quadout )
    {
        maxlevels = 0;
        levels = 0;
        WriteQuadTree( root );
        {
            int pow, expectedlevels = 1;
            pow = 4;
            for( ; pow < root->children+1 ; pow *= 4, expectedlevels++ );
            fprintf( quadout, "Max levels of nodes: %d total: %d expect: %d(%d)"
                            , maxlevels
                            , root->children+1 
                            , expectedlevels
                            , pow
                    );
        }
        fprintf( quadout, "----------------------------------------\n" );
        fclose( quadout );
    }
}

