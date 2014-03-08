
// well this code could have been used... but sectors on the
// same span would be skipped - also if the sector is larger than the
// root it definatly occupies 2 or more quadrants.... 


#include "vectlib.h"

#define BRANCHES 4

typedef struct quadnode_tag
{
	void *data;
	_POINT min, max, p;
	struct quadnode_tag **me;
	struct quadnode_tag *parent;
	// quad 0 = x < 0, y >= 0, z >= 0
	// quad 1 = x >= 0, y >= 0, z >= 0
	// quad 2 = x < 0, y < 0, z >= 0
	// quad 3 = x >= 0, y < 0, z >= 0

	// quad 4 = x < 0, y >= 0, z < 0
	// quad 5 = x >= 0, y >= 0, z < 0
	// quad 6 = x < 0, y < 0, z < 0
	// quad 7 = x >= 0, y < 0, z < 0
	struct quadnode_tag *quads[BRANCHES];
	int children; // quick balance check?
} QUADNODE, *PQUADNODE;

void TestBalance( PQUADNODE *root );

void HangQuadNode( PQUADNODE *root, PQUADNODE quad );

void AddQuadNode( PQUADNODE *root, void *data, P_POINT min, P_POINT max );

void MoveQuadNode( PQUADNODE *root, PQUADNODE quad, P_POINT min, P_POINT max );

void DeleteQuadNode( PQUADNODE node );

void DeleteQuadTree( PQUADNODE *root );

void DumpQuadTree( PQUADNODE root );
