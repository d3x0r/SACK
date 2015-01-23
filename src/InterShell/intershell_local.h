
// each page will open on its own normal full screen sized canvas; and we can pan these canvases....
#define MULTI_FRAME_CANVAS

#ifndef global_shell_data_defined
#define global_shell_data_defined

#ifndef FORCE_NO_INTERFACE
#define USE_INTERFACES
#endif

#ifdef INTERSHELL_SOURCE
#ifdef USE_INTERFACES
#define USE_IMAGE_INTERFACE g.pImageInterface
#define USE_RENDER_INTERFACE g.pRenderInterface
#endif
#endif


#include <sack_types.h>
#include <image.h>
#include <pssql.h>
#include <render.h>
#include <configscript.h>
#include <timers.h>

#include "widgets/include/buttons.h"

#include "intershell_export.h"
#include "loadsave.h"

INTERSHELL_NAMESPACE


INTERSHELL_NAMESPACE_END

#include "pages.h"

INTERSHELL_NAMESPACE

#ifdef INTERSHELL_SOURCE
#define g global_pos_data
#endif



#define GLARE_FLAG_MULTISHADE 1
#define GLARE_FLAG_SHADE 2

typedef struct glare_set{
	TEXTCHAR *name;
	struct {
		BIT_FIELD bMultiShadeBackground : 1;
		BIT_FIELD bShadeBackground : 1;
	} flags;
	TEXTSTR glare;
	TEXTSTR up;
	TEXTSTR down;
	TEXTSTR mask;
	Image iGlare, iNormal, iPressed, iMask;
	PLIST _theme_set; // plist of PGLARE_SET
	PLIST *theme_set; // plist of PGLARE_SET
} GLARE_SET, *PGLARE_SET;

#ifndef DECLARE_GLOBAL
extern
#endif
struct configure_key_dispatch{
	PSI_CONTROL frame;
	PMENU_BUTTON button;
	SFTFont *new_font;
	PCanvasData canvas;
	CTEXTSTR new_font_name;
} configure_key_dispatch;

struct CanvasData {
	//PSI_CONTROL pc_focused_page;

	struct {
		BIT_FIELD bSuperMode : 1;
		BIT_FIELD bEditMode : 1;
		BIT_FIELD bIgnoreKeys : 1;
		//BIT_FIELD bWaitForCaller : 1;
		BIT_FIELD selected : 1;  // data in selection should be used for rendering
		BIT_FIELD selecting : 1; // mouse has started selecting an area...
		BIT_FIELD dragging : 1; // moving an existing control.
		BIT_FIELD sizing : 1;
#define NO_SIZE_OP 0
#define UPPER_LEFT 1
#define UPPER_RIGHT 2
#define LOWER_LEFT 3
#define LOWER_RIGHT 4
#define DRAG_BEGIN 5
		BIT_FIELD sizing_by_corner : 3; // 0 = upperleft, 1 = upperright, 2=lowerleft,, 3=lowerright
		BIT_FIELD bSetResolution : 1; // set after npartsx, npartsy is set.
		BIT_FIELD bDoingButton : 1;
		BIT_FIELD bShowCanvas : 1; // set this when the canvas should be shown... but don't show
		BIT_FIELD bButtonProcessing : 1;
		BIT_FIELD wide_aspect : 1; // use wide aspect controls
		BIT_FIELD bUseSingleFrame : 1; // classic mode where all controls are realy on same surface

		// I dunno... this could be an enum or something for a switch()...
		BIT_FIELD bAnimateFromRight: 1;
		BIT_FIELD bAnimateFromLeft: 1;
		BIT_FIELD bAnimateFromTop: 1;
		BIT_FIELD bAnimateFromBottom: 1;
		BIT_FIELD bAnimateFromBack: 1;
		BIT_FIELD bAnimateFromFront: 1;
		// set to accept that want_active_page has been setup....
		BIT_FIELD bMovingPages : 1; 
	} flags;
	struct {
		int _x, _y;
		int x, y, w, h;
	} selection;
	PMENU_BUTTON pCurrentControl;
	//PPAGE_DATA page;

	// each canvas can have a set of pages...
	_32 nPages;
	PLIST pages; // PPAGE_DATA list
	S_32 left_right_page_offset;

	struct {
		S_32 x;
		S_32 y;
	} slide_state;

	PLIST deleted_pages; // PPAGE_DATA no longer listed in pages... Undelete(?)

	PPAGE_DATA want_active_page;
	PPAGE_DATA active_page; // mostly for tracking active if bUseSingleFrame

	// once upon a time there was only one page, and this is that page...
	// with time this will dissappear...

	PMENU pPageMenu; // list of pages
	PMENU pPageDestroyMenu; // destroy page  menu
	PMENU pPageUndeleteMenu; // undelete page
	// several of these have the above page menus
	// referenced as submenus.
	PMENU pSuperMenu;
	PMENU pEditMenu;
	PMENU pSelectionMenu;
	PMENU pControlMenu;
	PPAGE_DATA default_page;
	// if the canvas is contained within another canvas
	// then the width_scale/height_scale would change depending on the
		// outer resolution of the control. /* this is constant based on the canvas size... */
	FRACTION width_scale, height_scale;
	// this is maintained for simplicity...
	// it's a convenience for the PARTX, etc macros below.
	struct canvas_location {
		S_32 x, y;
		_32 width, height;
	} location;

	struct page_animation_state {
		_32 start_tick;
		_32 target_delta;
	} page_animation;
	// each canvas can have a different stting of partsx and partsy
	//_32 nPartsX, nPartsY;  // finest granulatity of control placement

	PRENDERER edit_glare; // this has it's own direct draw methods.  Without complication of being a control.
	PSI_CONTROL edit_glare_frame; // cause we need to reference the glare as a frame...
	PLIST selected_list;
	int nSelected;
	PDATASTACK prior_page_history; // this used to be global, but really this is a per-canvas property - since each has pages.  (struct page_history_node*)

	PLIST fonts;
};


typedef struct global_tag
{
	// this will contain information about the current
	// thing the menu is doing.  This region is named "Menu Evolved Instance"
	// or "Alternate Menu Instance"
	// if this region exists already, nice programs will exit, reporting information
	// found within this space before exiting.
	POINTER mem_lock;
	PTHREAD pMainThread;
	PRENDERER display; // used to allow external applications to wake me.

	PTRSZVAL psv_edit_security;

	PLIST frames;  // list of all canvases that have been created 

	//PSI_CONTROL keypad;
	PLIST extra_types; // char * name of extra types available to create
	TEXTCHAR *config_filename;
	SFTFont	*_keyfont;
	PLIST glare_sets;
	S_32 default_page_x, default_page_y;
	_32 default_page_width, default_page_height;

	PMENU_BUTTON clonebutton;
	int _px, _py;  // last part x, part y - marked on drag.
	struct {
		BIT_FIELD bExit : 2;
		BIT_FIELD bCluster : 1;
 		BIT_FIELD forceload : 1;	// first config read from file, on save saves to SQL
 		BIT_FIELD restoreload : 1;	// first config read from file, on save saves to SQL
		BIT_FIELD local_config : 1; // don't save config in SQL or get it from SQL
		BIT_FIELD multi_edit : 1; // popup pages in seperate frames.
		BIT_FIELD bInitFinished : 1;
		BIT_FIELD bPageUpdateDisabled : 1;
		BIT_FIELD bTerminateStayResident : 1;
		BIT_FIELD bNoEditSet : 1;
		BIT_FIELD bNoEdit : 1;
		BIT_FIELD bAllowMultiSet : 1;
		BIT_FIELD bAllowMultiLaunch : 1;
		BIT_FIELD bSQLConfig : 1;
		//BIT_FIELD bShowCanvas : 1; // set this when the canvas should be shown... but don't show
		//BIT_FIELD bButtonProcessing : 1;
		BIT_FIELD bLogNames : 1;
		BIT_FIELD bTopmost : 1;
		BIT_FIELD bSpanDisplay : 1;
		BIT_FIELD bUseCustomPositioning : 1;
		BIT_FIELD bTransparent : 1;
		BIT_FIELD bExternalApplicationhost : 1; // set by C# intro hook... so we don't make a auto
		// once this comes up, the memlock region is disabled
		// if this option is set in the config..
		//BIT_FIELD bAllowMultiLaunch : 1;
		BIT_FIELD bPageReturn : 1; // doing a return page (don't save current page to stack)
		BIT_FIELD bLogKeypresses : 1; // Log the text of button and the button type when it is pressed.
		BIT_FIELD bUseSingleDisplay : 1;
	} flags;
	// tokens which are used for testing
	// user security rights.
#ifdef USE_INTERFACES
	PRENDER_INTERFACE pRenderInterface;
	PIMAGE_INTERFACE pImageInterface;
#endif

	// ALL pages (in all controls)
	PLIST all_pages;

	// instead of this, evo menu prefers to use
	// controls list, the parts, and selection stuff below
	//int button_rows, button_cols, button_space;

	PLIST global_properties;	// list of void(*)(void) called for global properties...
	PLIST global_property_names; // list of global property names  - 1:1 relation with global_properties
	PLIST security_property_names; // list of global property names  - 1:1 relation with global_properties
	CTEXTSTR system_name; // my particular name, not nice'd (that is Pos1,JimsPewter)
	struct system_info *systems; // list of known systems (may query database?)
	PMENU pSelectionMenu;
	PMENU pGlobalPropertyMenu;
	PMENU_BUTTON CurrentlyCreatingButton;

	int theme_index;
	int max_themes; // 0==1, at 2 there's the default and 1 theme
	PCanvasData current_saving_canvas;
	CTEXTSTR single_frame_title;
	//PODBC configuration_option_db;
	TEXTCHAR configuration_dsn[256];
	int configuration_version;

	int target_display;   // option to keep
} MENU_GLOBAL, *PMENU_GLOBAL;

	struct system_info {
		PTEXT name;  // really only care about name...
		PTEXT nice_name; // spaced out, user friendly, name
		INDEX ID;
		DeclareLink( struct system_info );
	};

#define PART_RESOLUTION 256
//#define CANVAS_X(canvas,n) (((n) * (canvas)->width)/(PART_RESOLUTION))
//#define CANVAS_Y(canvas,n) (((n) * (canvas)->height)/(PART_RESOLUTION))

//#define _WIDTH(canvas,w) ( ( (PART_RESOLUTION) - ( (canvas)->button_space * ((canvas)->button_cols + 1) ) ) / (canvas)->button_cols );

	// helps to compute the part of a coordinate
#define _COMPUTEPARTOFX( canvas,x, parts )  ((x)*parts / ((canvas)->width) )
	// helps to compute the part of a coordinate
#define _COMPUTEPARTOFY( canvas,y, parts )  ((y)*parts / ((canvas)->height) )

	// helps to compute X coordinate of a part
#define _COMPUTEX( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * (npart) ) ) * ((canvas)->location.width) ) / ((parts)*PART_RESOLUTION) )
	// helps to compute Y coordinate of a part
#define _COMPUTEY( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * (npart) ) ) * ((canvas)->location.height) ) / ((parts)*PART_RESOLUTION) )

//#define _MODX( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * npart ) ) * ((canvas)->width) ) % ((parts)*PART_RESOLUTION) )
//#define _MODY( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * npart ) ) * ((canvas)->height) ) % ((parts)*PART_RESOLUTION) )

// get the X coordinate of a part
#define _PARTX(page,part) (S_32)_COMPUTEX(page->canvas,part,(page)->grid.nPartsX)
// get the Y coordinate of a part
#define _PARTY(page,part) (S_32)_COMPUTEY(page->canvas,part,(page)->grid.nPartsY)
// get the width coordinate of a part width
#define _PARTW(page,x,w) (_32)(_PARTX(page,x+w)-_PARTX(page,x))
// get the height coordinate of a part height
#define _PARTH(page,y,h) (_32)(_PARTY(page,y+h)-_PARTY(page,y))

// result with current parts
#define _PARTSX(page) (page)->grid.nPartsX
// result with current parts
#define _PARTSY(page) (page)->grid.nPartsY

//#define X(n) CANVAS_X(page)
//#define Y(n) CANVAS_Y(page)

#define WIDTH(w) _WIDTH(page,w)

#define COMPUTEPARTOFX(x,parts)		  _COMPUTEPARTOFX(page->canvas,x,parts )
#define COMPUTEPARTOFY(y,parts)		  _COMPUTEPARTOFY(page->canvas,y,parts )
#define COMPUTEX( npart, parts ) _COMPUTEX( page,npart, parts )
#define COMPUTEY( npart, parts ) _COMPUTEY( page,npart, parts )
#define MODX( npart, parts )	  _MODX( page,npart, parts )
#define MODY( npart, parts )	  _MODY( page,npart, parts )
#define PARTX(part)				  _PARTX(page,part)
#define PARTY(part)				  _PARTY(page,part)
#define PARTW(x,w)					_PARTW(page,x,w)
#define PARTH(y,h)					_PARTH(page,y,h)
#define PARTSX						 _PARTSX(page)
#define PARTSY						 _PARTSY(page)



#ifdef INTERSHELL_SOURCE
#ifndef DECLARE_GLOBAL
#if defined( INTERSHELL_SOURCE ) || defined( GCC )
extern
#else
#ifdef __WINDOWS
__declspec(dllimport)
#else
extern
#endif
#endif
#else
#ifdef WIN32
__declspec(dllexport)
#endif
#endif
MENU_GLOBAL g;
#endif

PMENU_BUTTON CPROC CreateSomeControl( PSI_CONTROL pc_canvas, int x, int y, int w, int h
										, CTEXTSTR name );
void DestroyButton( PMENU_BUTTON button );


// various methods availble for external invokation...
int InvokeEditEnd( PMENU_BUTTON button );
int QueryShowControl( PMENU_BUTTON button );
PSI_CONTROL CPROC QueryGetControl( PMENU_BUTTON button );
//void DestroyButton( PMENU_BUTTON button );

// when a control is shown on a page this is called (after query show)
void InvokeShowControl( PMENU_BUTTON button );


//void SetButtonColors( CDATA cText, CDATA cBackground1, CDATA cBackground2, CDATA cBackground3 );
void SetButtonType( char *name ); // square, round, custom..

void EditCurrentPageProperties( PCanvasData canvas, PSI_CONTROL pc_canvas, PPAGE_DATA page );
void CreateNewPage( PSI_CONTROL pc_canvas, PCanvasData canvas );
PSI_CONTROL OpenPageFrame( PPAGE_DATA page, LOGICAL show ); // used for multi edit to open each root page in a sepearate frame... (sub canvas pages also! LOL)

 // this exists in pages.h ATM
//int SimpleUserQuery( char *result, int reslen, char *question, PCOMMON pAbove );


PGLARE_SET GetGlareSet( PCanvasData canvas, CTEXTSTR name );

PMENU_BUTTON CreateInvisibleControl( PCanvasData canvas, TEXTCHAR *name );
void ConfigureKeyEx( PSI_CONTROL parent, PMENU_BUTTON button );
void ConfigureKeyExx( PSI_CONTROL parent, PMENU_BUTTON button, int bWaitComplete, int bIgnorePrivate );

void DumpGeneric( FILE *file, PMENU_BUTTON button );

PCanvasData GetCanvasEx( PSI_CONTROL pc DBG_PASS );
PMENU_BUTTON CreateButton( PCanvasData parent, PPAGE_DATA page );

// macros.c, invoke this after onfinish init
void InvokeStartupMacro( void );
void InvokeShutdownMacro( void );

void LoadInterShellPlugins( CTEXTSTR mypath, CTEXTSTR mask, CTEXTSTR extra_path ); // used by intershell_common

void InvokeLoadCommon( void );

void InvokeCloneControl( PMENU_BUTTON button, PMENU_BUTTON original );
void CloneCommonButtonProperties( PMENU_BUTTON clone, PMENU_BUTTON  clonebutton );
PMENU_BUTTON GetCloneButton( PCanvasData canvas, PPAGE_DATA page, int x, int y, int bInvisible ); // new button is invisible (or not)



/* this is begining of a function to control macros.  It is used between macros.c and banners.c at this time*/
void SetMacroResult( int allow_continue );

// text_label.h .... sets a variable - used by command line variable defintions...
void SetVariable( CTEXTSTR name, CTEXTSTR value );


void PublicAddCommonButtonConfig( PMENU_BUTTON button );

// this is common between intershell sources...
void FixupButtonEx( PMENU_BUTTON button DBG_PASS );
void FlushToKey( PMENU_BUTTON button );

void SetMacroResult( int allow_continue );
void AddSystemName( PSI_CONTROL list, CTEXTSTR name );
CTEXTSTR InterShell_GetSaveIndent( void );

// from fonts.c
void UpdateFontScaling( PCanvasData canvas );
void CPROC SetTextLabelOptions( PMENU_BUTTON label, LOGICAL center, LOGICAL right, LOGICAL scroll, LOGICAL shadow );




INTERSHELL_NAMESPACE_END

#include "intershell_registry.h"



#endif
