#ifndef _SPACETREE_H_
#define _SPACETREE_H_

#ifdef __cplusplus
#define SPACETREE_NAMESPACE namespace sack { namespace containers { namespace spacetree {
#define SPACETREE_NAMESPACE_END }}}
#else
#define SPACETREE_NAMESPACE 
#define SPACETREE_NAMESPACE_END
#endif


#define PARTITION_SCREEN

#ifdef PARTITION_SCREEN
#include <image.h>
#define PSPACEPOINT P_IMAGE_POINT
#define SPACEPOINT IMAGE_POINT
#define SPACECOORD IMAGECOORD
#else
#include <vectlib.h>
#define PSPACEPOINT P_POINT
#define SPACEPOINT _POINT
#define SPACECOORD RCOORD
#endif



SPACETREE_NAMESPACE

#ifndef SPACE_STRUCT_DEFINED
typedef struct spacenode *PSPACENODE;
#endif

EXPORT_METHOD PSPACENODE AddSpaceNodeEx( PSPACENODE *root, void *data
								, PSPACEPOINT min, PSPACEPOINT maxpoint DBG_PASS );
#define AddSpaceNode(r,d,mn,mx) AddSpaceNodeEx((r),(d),(mn),(mx) DBG_SRC )

EXPORT_METHOD PSPACENODE ReAddSpaceNodeEx( PSPACENODE *root, PSPACENODE node
								, PSPACEPOINT min, PSPACEPOINT maxpoint DBG_PASS );
#define ReAddSpaceNode(r,n,mn,mx) ReAddSpaceNodeEx((r),(n),(mn),(mx) DBG_SRC )

//void HangSpaceNodeExx( PSPACENODE *root, PSPACENODE parent, PSPACENODE space DBG_PASS );
#define HangSpaceNodeEx(root,nul, space) HangSpaceNodeExx((root),(nul),(space) DBG_SRC )
#define HangSpaceNode(root,space) HangSpaceNodeEx((root),NULL,(space))

EXPORT_METHOD PSPACENODE RemoveSpaceNode( PSPACENODE space );
EXPORT_METHOD void DeleteSpaceNode( PSPACENODE node );
EXPORT_METHOD void DeleteSpaceTree( PSPACENODE *root );
EXPORT_METHOD void MarkNodeDirty( PSPACENODE node, P_IMAGE_RECTANGLE rect );
EXPORT_METHOD PSPACENODE GetDirtyNode( void *p, P_IMAGE_RECTANGLE rect ); // get a dirty node which has this data content...
EXPORT_METHOD int IsNodeDirty( PSPACENODE node, P_IMAGE_RECTANGLE rect );

EXPORT_METHOD PSPACENODE FindPointInSpace( PSPACENODE root, PSPACEPOINT p
									, int (*Validate)(void *data, PSPACEPOINT p ) 
									);
/*
PSPACENODE OldFindRectInSpace( PSPACENODE root
									, PSPACEPOINT min
									, PSPACEPOINT max
									, int (*Validate)(void *data
																, PSPACEPOINT min
																, PSPACEPOINT max ) );
*/
// pointer to a void pointer.
// this void pointer may(should) be initialized to zero.
EXPORT_METHOD PSPACENODE FindRectInSpaceEx( PSPACENODE root
							, PSPACEPOINT min
							, PSPACEPOINT max
							, void **data 
							DBG_PASS ); 
#define FindRectInSpace(r,p,v,fd) FindRectInSpaceEx(r,p,v,fd DBG_SRC)
// call this to allow the tree to delete it's queue.
// If FindRectInSpace resulted in a NULL - this routine
// does not have to be called, only when all results are not retrieved.
EXPORT_METHOD void EndFindRectInSpace( void **data );
EXPORT_METHOD PSPACENODE GetNodeUnder( PSPACENODE node );

EXPORT_METHOD void TestSpaceBalance( PSPACENODE *root );
EXPORT_METHOD void DumpSpaceTree( PSPACENODE root );
EXPORT_METHOD void BrowseSpaceTree( PSPACENODE root	
											 , void (*Callback)( POINTER data, PSPACEPOINT min, PSPACEPOINT max ) );
EXPORT_METHOD void BrowseSpaceTreeEx( PSPACENODE root
												, void (*Callback)( uintptr_t, PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max )
												, uintptr_t psv );
EXPORT_METHOD void OutputHTMLSpaceTable( PSPACENODE root
													, PVARTEXT pvt_output
													, void (*Callback)(uintptr_t psv, PVARTEXT pvt, POINTER node_data, PSPACEPOINT min, PSPACEPOINT max )
													, uintptr_t psv );

EXPORT_METHOD void ValidateSpaceTreeEx( PSPACENODE root DBG_PASS );
#define ValidateSpaceTree(r)  ValidateSpaceTreeEx(r DBG_SRC) 

EXPORT_METHOD PSPACENODE GetFirstRelativeSpace( PSPACENODE node );
EXPORT_METHOD PSPACENODE GetNextRelativeSpace( PSPACENODE node );

EXPORT_METHOD PSPACENODE FindDeepestNode( PSPACENODE root, int level );
// call this with level = 0; - else result undefined.

EXPORT_METHOD PSPACEPOINT GetSpaceMax( PSPACENODE node );
EXPORT_METHOD PSPACEPOINT GetSpaceMin( PSPACENODE node );

EXPORT_METHOD void *GetNodeData( PSPACENODE node );
EXPORT_METHOD void GetNodeRect( PSPACENODE node, PSPACEPOINT min, PSPACEPOINT max );

SPACETREE_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::containers::spacetree;
#endif

#endif

