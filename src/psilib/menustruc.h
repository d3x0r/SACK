
#include <sack_types.h>
#include "global.h"
#include "controlstruc.h"
#define MENU_DRIVER_SOURCE


PSI_MENU_NAMESPACE
#ifdef CUSTOM_MENUS

// these are a little bigger than the actual thing drawn
// to give some padding around the text...
#define CHECK_WIDTH 12
#define SUB_WIDTH 16

// Should do something to disable infinite recursions
// Though they may be allowed, they really really are
// a bad idea when you can return selection data for all
// levels of submenus...

typedef struct menu_tag {
    struct {
        uint32_t changed:1;
        uint32_t tracking:1;
		  uint32_t abort:1;
		  uint32_t showing : 1;
		  uint32_t bSubmenuOpen : 1;
    }flags;
    struct menuitem_tag *items;
#ifdef __64__ 
	int64_t selection;
#else
    int32_t selection; // -1 while no selection
#endif
    struct menuitem_tag *selected;
    int16_t   height, width;
    struct {
        int32_t    x, y; // display x and y
    } display;
    SFTFont  font;
    PCOMMON image;
    int _x, _y, _b;
    struct menu_tag *child, *parent; // currently shown child if any...
    Image surface;
} MENU;

typedef struct menuitemflags_tag {
	uint32_t bSeparator:1; // pretty much no other data is useful...
	uint32_t bOwnerDraw:1; // information is only useful to an external routine
	uint32_t bHasText:1;   // text field valid...
	uint32_t bChecked:1;
	uint32_t bSelected:1; // current option shown...
	uint32_t bOpen:1; // sub menu with active popup shown
	uint32_t bSubMenu:1; // data content is a submenu.. else ID data
} ITEMFLAGS;

typedef struct draw_popup_item_tag  DRAWPOPUPITEM;
typedef struct draw_popup_item_tag  *PDRAWPOPUPITEM;
// duplicated in controls.h - applications need this structure...
struct draw_popup_item_tag 
{
	uintptr_t ID;
	struct {
		uint32_t selected : 1;
		uint32_t checked  : 1;
	} flags;
	union {
		struct {
			uint32_t width, height;
		} measure;
		struct {
			int32_t x, y;
			uint32_t width, height;
			Image image;
		} draw;
	};
};


typedef void (*DrawPopupItemProc)( LOGICAL measure, PDRAWPOPUPITEM pdi );

typedef struct menuitem_tag {
	ITEMFLAGS  flags;
	union {
		struct {
			size_t textlen;// shortcut length save...
			TEXTCHAR *text; // use PutMenuString when rendering this...
			TEXTCHAR key;   // selected active Key...
		} text;
		struct {
	      DrawPopupItemProc DrawPopupItem;
		} owner;
	} data;
	union {
  		uintptr_t userdata;
		uintptr_t ID;
		PMENU menu;
	} value;
	int baseline;
   uint32_t height; // not including +2 for above/below highlights
   uint32_t width; // reported from owner draw measuring...
   uint32_t offset; // left offset (for check/icon space)
	struct menuitem_tag *next, **me;
} MENUITEM;

#else
//#define PMENU HMENU
//#define PMENUITEM int
//typedef int PMENU;
//typedef int PMENUITEM;
#endif
PSI_MENU_NAMESPACE_END

// $Log: menustruc.h,v $
// Revision 1.11  2005/03/12 23:31:21  panther
// Edit controls nearly works... have some issues with those dang popups.
//
// Revision 1.10  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.3  2004/10/13 11:13:53  d3x0r
// Looks like this is cleaning up very nicely... couple more rough edges and it'll be good to go.
//
// Revision 1.2  2004/10/12 08:10:51  d3x0r
// checkpoint... frames are controls, and anything can be displayed...
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.9  2003/03/31 04:18:49  panther
// Okay - drawing dispatch seems to work now.  Client/Server timeouts occr... rapid submenus still fail
//
// Revision 1.8  2003/03/29 17:21:06  panther
// Focus problems, mouse message problems resolved... Focus works through to the client side now
//
// Revision 1.7  2003/03/27 00:23:14  panther
// Enable owner draw popup items - measure and draw
//
// Revision 1.6  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
