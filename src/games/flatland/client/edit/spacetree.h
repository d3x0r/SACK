#ifndef _SPACETREE_H_
#define _SPACETREE_H_
#include <vectlib.h>



#ifndef SPACE_STRUCT_DEFINED
typedef int SPACENODE, *PSPACENODE;
#endif

PSPACENODE AddSpaceNodeEx( PSPACENODE *root, void *data
								, P_POINT min, P_POINT maxpoint DBG_PASS );
#define AddSpaceNode(r,d,mn,mx) AddSpaceNodeEx((r),(d),(mn),(mx) DBG_SRC )

PSPACENODE ReAddSpaceNodeEx( PSPACENODE *root, PSPACENODE node
								, P_POINT min, P_POINT maxpoint DBG_PASS );
#define ReAddSpaceNode(r,n,mn,mx) ReAddSpaceNodeEx((r),(n),(mn),(mx) DBG_SRC )

void HangSpaceNodeExx( PSPACENODE *root, PSPACENODE parent, PSPACENODE space DBG_PASS );
#define HangSpaceNodeEx(root,nul, space) HangSpaceNodeExx((root),(nul),(space) DBG_SRC )
#define HangSpaceNode(root,space) HangSpaceNodeEx((root),NULL,(space))

PSPACENODE RemoveSpaceNode( PSPACENODE space );
void DeleteSpaceNode( PSPACENODE node );
void DeleteSpaceTree( PSPACENODE *root );

void *FindPointInSpace( PSPACENODE root, P_POINT p
									, int (*Validate)(void *data, P_POINT p ) );

void TestSpaceBalance( PSPACENODE *root );
void DumpSpaceTree( PSPACENODE root );

void ValidateSpaceTreeEx( PSPACENODE root DBG_PASS );
#define ValidateSpaceTree(r)  ValidateSpaceTreeEx(r DBG_SRC) 

PSPACENODE FindDeepestNode( PSPACENODE root, int level );
// call this with level = 0; - else result undefined.

void *GetNodeData( PSPACENODE node );

#endif
