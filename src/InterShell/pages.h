
#ifndef page_structure_defined
#define page_structure_defined

//#ifndef USE_IMAGE_INTERFACE
//#define USE_IMAGE_INTERFACE g.pImageInterface
//#define USE_RENDER_INTERFACE g.pRenderInterface
//#endif


#include <colordef.h>
#include <image.h>
#include <controls.h>
#include "intershell_export.h"

INTERSHELL_NAMESPACE
/*
struct page_layout_tag
{
// specific layouts for specific aspec ratio
   FRACTION aspect;
	PLIST controls;
   PLIST systems;
};
*/


struct page_history_node
{
	enum page_transition prior_transition;
	uint32_t time;
	struct page_data *page;
};

struct page_data
{
	struct CanvasData  *canvas;
	CDATA    background_color;
	Image    background_image;
	CTEXTSTR background;

	PLIST    background_colors;
	PLIST    background_images;
	PLIST    backgrounds;
	CTEXTSTR title;
	// permissions....
	// other stuffs....
	PSI_CONTROL frame;
	PRENDERER renderer;

	struct {
		PLIST wide_controls; // List of PMENU_BUTTONs
		PLIST tall_controls; // list of PMENU_BUTTONs
	} layout;

	uint32_t ID;
	struct {
		BIT_FIELD bActive : 1; // quick checkable flag to see if page is active (last changed to)
		BIT_FIELD showing : 1;
	} flags;

	struct {
		//struct {
		//	BIT_FIELD snap_to : 1; // is grid enabled?
		//	BIT_FIELD intelligent : 1; // intelligent grid functions like dialog control placement for VS
		//} flags;
		// by default this is 3/4 (height/width) (rise/run) (y/x)

		// pages with different aspect ratios may be ignored in normal states, based on screen resolution proportion.
		// a command line option perhaps can enable All Aspect ?
		//FRACTION aspect;  // applied to
		int  nPartsX, nPartsY; // x divisiosn total and y divisions total.
		int origin_offset_x, origin_offset_y;
		int min_offset_x, max_offset_x;
		int min_offset_y, max_offset_y;
		int overflow_x;
		int overflow_y;
	} grid;

	struct {
		int x, y;
		int delta_x, delta_y;
	} offset;

};
typedef struct page_data PAGE_DATA;

#define PAGE_CHANGER_NAME "page/Page Changer"

// pages have multiple lists of controls depending on layouts
// so this returns the appropriate one.
PLIST *GetPageControlList( PPAGE_DATA page, int bWide );

void SetCurrentPageID( PSI_CONTROL pc_canvas, uint32_t ID ); // MNU_CHANGE_PAGE ID (minus base)
void DestroyPageID( PSI_CONTROL pc_canvas, uint32_t ID ); // MNU_DESTROY_PAGE ID (minus base)
void UnDestroyPageID( PSI_CONTROL pc_canvas, uint32_t ID ); // MNU_DESTROY_PAGE ID (minus base)

#ifdef INTERSHELL_SOURCE

void AddPage( PCanvasData canvas, struct page_data * page );
void RestorePageEx( PCanvasData canvas, struct page_data * page, int bFull, int active, LOGICAL bWide );
#define RestorePage(c,p,f) RestorePageEx(c,p,f,1,c->flags.wide_aspect)

struct page_data * GetPageFromFrame( PCanvasData frame );


void ChangePageEx( PPAGE_DATA page, enum page_transition direction, uint32_t how_long DBG_PASS );
//void ChangePagesEx( struct page_data * page DBG_PASS);
#define ChangePage(p,d,t) ChangePageEx(p,d,t DBG_SRC)

void InsertStartupPage( PCanvasData pc_canvas, CTEXTSTR page_name );
// this is actually insert page...
// creates a new pagename for the startup page
// and creates a new startup page in place.
void RenamePage( PPAGE_DATA page );
PPAGE_DATA CPROC CreateNamedPage( PCanvasData pc_canvas, CTEXTSTR page_name );

int InvokePageChange( PCanvasData pc_canvas );

// on destroy button call this...
void UpdatePageRange( PPAGE_DATA page );

// call this to add a button to the page; updates page grid limits for scrolling.
void PutButtonOnPage( PPAGE_DATA page, PMENU_BUTTON button );
void SetPageOffsetRelative( PPAGE_DATA page, int x, int y );

// called when a theme is added
void AddTheme( PCanvasData canvas, int theme_id );
// called when theme is about to change, but has not yet
void StoreTheme( PCanvasData canvas );
// called when theme has changed, but before final display update
void UpdateTheme( PCanvasData canvas );


#endif

INTERSHELL_NAMESPACE_END

#endif
