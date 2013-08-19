#include <stdhdrs.h>
#include <string.h>
#include <sharemem.h>
#include <keybrd.h>
#include "controlstruc.h"
#include "controls.h"

// Tree Control API
//  MakeTreeControl...
//  PTREEITEM AddTreeItem( tree, int level, char *text );
//
//  PListITEM InsertTreeItem( tree, pItemAfter, int level, char *text )
//    if pItemAfter == NULL - add to first thing in list.
//  SetItemData
//  GetItemData



// will also take advantage of the existing scrollbar control.

typedef struct treeitem_tag {
	char data;
} TREEITEM, *PTREEITEM;

typedef struct treelist_tag {
	CONTROL common;
} TREELIST, *PTREELIST;


CONTROL_INIT( TreeControl )
{
   PTREELIST ptl = (PTREELIST)pControl;
	ptl->common.common.nType = TREELIST_CONTROL;
	//ptl->common.DrawThySelf = RenderListBox;
	//ptl->common.KeyProc = KeyListControl;
	//ptl->common.Destroy = DestroyListBox;
}

CONTROL_PROC_DEF( TREELIST, TreeControl, BORDER_THIN|BORDER_INVERT )
{
	return pControl;
}
// $Log: ctltree.c,v $
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.6  2003/09/22 10:45:08  panther
// Implement tree behavior in standard list control
//
// Revision 1.5  2003/09/11 13:09:25  panther
// Looks like we maintained integrety while overhauling the Make/Create/Init/Config interface for controls
//
// Revision 1.4  2003/03/29 22:52:00  panther
// New render/image layering ability.  Added support to Display for WIN32 usage (native not SDL)
//
// Revision 1.3  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
