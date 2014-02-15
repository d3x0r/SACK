#ifndef DISPLAY_STRUCTURES_DEFINED
#include <sack_types.h>

struct panel_tag;
struct region_tag;

typedef struct PRENDERER PPANEL;
typedef struct RENDERER PANEL;
//#undef PRENDERER 
//#define PRENDERER PPANEL

#include <vidlib/keydef.h>

#include <render.h>
// -- Option flags --
// enable to log regions queued to update and the dispatch
// of redraw events to application.
//#define REDRAW_DEBUG

// this is a future work - each panel's image is retained
// exactly by the server, requiring 0 redraw from the application.
// applciations are then free to merely update as they will.
//#define SEPERATE_PANEL_IMAGES

//#if !defined(INTERNAL_BUFFER) && defined( __ARM__ )
#define INTERNAL_BUFFER // need this for fluffy soft cursors...
//#endif

#include "spacetree.h"

#define DISPLAY_STRUCTURES_DEFINED

#define IF_FLAG_PANEL_ROOT   IF_FLAG_USER1
#define IF_FLAG_IS_PANEL     IF_FLAG_USER2


typedef struct common_panel_region_tag
{
	Image RealImage;
	Image StableImage;
	struct {
		_32 region     : 1;  // is a region if this is set/else is panel
		_32 alpha      : 1;  // must draw covered portions on forced update.
		_32 opaque     : 1; // may have transparent parts, must call draw on covered
		_32 invisible  : 1; // not shown, not active
		_32 prior_invisible : 1;
		_32 disabled   : 1; // not active, values can be displayed with alt attributes.
		_32 active     : 1; // should only be 1 ever (focused)
		_32 initial    : 1; // region has JUST been created, and is drawing it's frame.

		_32 backdrop_image : 1; // draw image (if not tiled, stretch to extent)
		_32 backdrop_tiled : 1; // draw image in natural sizing tiled
		_32 backdrop_color : 1; // draw solid color (if alpha use alpha blot)
		_32 backdrop_custom : 1; // call routine to draw the backdrop (gradients, etc)
		_32 dirty_rect_valid : 1;
	} flags;
	const char *caption;
#ifdef HAVE_ANONYMOUS_STRUCTURES
	IMAGE_RECTANGLE;
#else
	// this information should be excactly the real_x, y, width, height of it's image?
	// well no if the panel is a subpanel of a panel...
	int x,y,width,height; // information needed to recreate this region.
#endif

	// this is the actual rect on this image which is
	// dirty.  This allows things like the background image
	// and some other more complex applications to update only
	// the portion dirty (moving a small window around)
	IMAGE_RECTANGLE dirty;
	union {
		Image Background;
		CDATA Color;
		struct {
			void (*DrawBackdrop)( PTRSZVAL psvUser, struct panel_tag * panel );
			PTRSZVAL psvUserDraw;
			LOGICAL (*PointOnPanel)( PTRSZVAL psvUser, struct panel_tag * panel, PSPACEPOINT p );
			PTRSZVAL psvUserOn;
		} custom;
	} background;



	struct panel_tag *parent;

} COMMON_PANEL_REGION, *PCOMMON_PANEL_REGION;



struct panel_tag
{
	COMMON_PANEL_REGION common;
	struct {
		_32 contained : 1; // if not contained is above parent.
		_32 retained  : 1;
		_32 dragging  : 1;
		_32 owning_parents_mouse : 1;
		_32 dirty : 1; // panel needs to be updated...
			 // a panel will be dirty and cleaning
			 // while the client is still processing events.
			 // a message will come in 'syncrender' which will
      // indicate which panel is now done.
		_32 cleaning : 1;
		_32 destroy : 1; // was in use when destroy was called
		_32 destroying : 1;
		_32 bOpenGL : 1; // if openGL enabled...
	} flags;
   _32 used;
	char *name;
   //PLIST sprites;
	CloseCallback CloseMethod;
	PTRSZVAL psvClose;
   MouseCallback MouseMethod;
   PTRSZVAL psvMouse;
   RedrawCallback RedrawMethod;
   PTRSZVAL psvRedraw;
   LoseFocusCallback FocusMethod;
	PTRSZVAL psvFocus;
	KeyProc KeyMethod;
   PTRSZVAL psvKey;
//   GeneralCallback DefaultMethod;
//	PTRSZVAL psvDefault;

   struct panel_tag *parent, *child, *elder, *younger, *above, *below;

	PKEYDEFINE KeyDefs;
   // 2 bits per key for toggled and pressed states.
	FLAGSET( KeyboardState, 512 );
	FLAGSET( KeyboardMetaState, 256 );
	_32 KeyDownTime[256];
};


#endif

//--------------------------------------------------------
// $Log: displaystruc.h,v $
// Revision 1.32  2005/06/24 15:11:46  jim
// Merge with branch DisplayPerformance, also a fix for watcom compilation
//
// Revision 1.31.2.1  2005/06/22 17:31:43  jim
// Commit test optimzied display
//
// Revision 1.29  2005/06/17 21:29:15  d3x0r
// Checkpoint... Seems that dirty rects can be used to minmize drawing esp. around moving/rsizing of windows
//
// Revision 1.28  2005/05/25 16:50:12  d3x0r
// Synch with working repository.
//
// Revision 1.31  2005/05/12 21:04:42  jim
// Fixed several conflicts that resulted in various deadlocks.  Also endeavored to clean all warnings.
//
// Revision 1.30  2005/04/25 16:08:00  jim
// On sync actually do an output... also removed a couple noisy messages.
//
// Revision 1.29  2005/03/30 03:26:37  panther
// Checkpoint on stabilizing display projects, and the exiting thereof
//
// Revision 1.27  2004/08/17 01:17:32  d3x0r
// Looks like new drawing check code works very well.  Flaw dragging from top left to bottom right for root panel... can lose mouse ownership, working on this problem... fixed export of coloraverage
//
// Revision 1.26  2004/08/11 12:00:28  d3x0r
// Migrate to new, more common keystruct...
//
// Revision 1.25  2004/06/16 21:13:11  d3x0r
// Cleanups for keybind duplication between here and vidlib... that's all been cleaned...
//
// Revision 1.24  2004/06/16 10:27:23  d3x0r
// Added key events to display library...
//
// Revision 1.23  2004/05/02 05:06:22  d3x0r
// Sweeping changes to logging which by default release was very very noisy...
//
// Revision 1.22  2003/10/12 02:47:05  panther
// Cleaned up most var-arg stack abuse ARM seems to work.
//
// Revision 1.21  2003/08/01 07:56:12  panther
// Commit changes for logging...
//
// Revision 1.20  2003/06/04 11:42:33  panther
// Reformatted some - handle redraw event... working on calibration also
//
// Revision 1.19  2003/04/11 13:35:11  panther
// new configuration - dropped color defs.  Added image to be coppied
//
// Revision 1.18  2003/03/29 22:52:00  panther
// New render/image layering ability.  Added support to Display for WIN32 usage (native not SDL)
//
// Revision 1.17  2003/03/28 12:05:14  panther
// Fix display panel relations.  Fix Mouse ownership issues.  Modify menu to handle new mouse ownership.  Fix client message range check
//
// Revision 1.16  2003/03/27 15:26:12  panther
// Add Service Unloading in message service layer - still need a service notification message
//
// Revision 1.15  2003/03/21 09:55:36  panther
// Cleanup keyboard definitions - going to have to forward keyboards on a per-panel basis
//
// Revision 1.14  2003/02/17 02:58:23  panther
// Changes include - better support for clipped images in displaylib.
// More events handled.
// Modifications to image structure and better unification of clipping
// ideology.
// Better definition of image and render interfaces.
// MUCH logging has been added and needs to be trimmed out.
//
// Revision 1.13  2003/02/12 20:46:32  panther
// Migrate all sources to new interface headers
//
// Revision 1.12  2003/02/06 11:10:26  panther
// Migration to new interface headers
//
// Revision 1.11  2003/02/05 10:32:31  panther
// Client/Server code compiles...
//
// Revision 1.10  2002/12/12 15:01:44  panther
// Fix Image structure mods.
//
// Revision 1.9  2002/11/24 21:37:41  panther
// Mods - network - fix server->accepted client method inheritance
// display - fix many things
// types - merge chagnes from verious places
// ping - make function result meaningful yes/no
// controls - fixes to handle lack of image structure
// display - fixes to handle moved image structure.
//
// Revision 1.8  2002/11/20 21:57:01  jim
// Modified all places which directly referenced ImageFile structure.
// Display library is still allowed intimate knowledge
//
// Revision 1.7  2002/11/16 16:36:13  panther
// Another stage - default panel mouse handler to handle 'move'
// now - let us try playing with SDL in framebuffer/svga mode.
//
// Revision 1.6  2002/11/10 03:12:00  panther
// Working on services to enable display server process to be remote.
// This will require shared memory transport, (a network proxy?), and some
// registration services.
//
// Revision 1.5  2002/11/06 12:43:01  panther
// Updated display interface method, cleaned some code in the display image
// interface.  Have to establish who own's 'focus' and where windows
// are created.  The creation method REALLY needs the parent's window.  Which
// is a massive change (kinda)
//
// Revision 1.4  2002/11/05 09:55:55  panther
// Seems to work, added a sample configuration file.
// Depends on some changes in configscript.  Much changes accross the
// board to handle moving windows... now to display multiples.
//
// Revision 1.3  2002/10/30 15:51:01  panther
// Modified display to handle controls, Stripped out regions.  Project now
// works wonderfully on Windows (kinda) now we go to Linux and see about
// the input events there.
//
// Revision 1.2  2002/10/29 09:31:34  panther
// Trimmed out Carriage returns, added CVS Logging.
// Fixed compilation under Linux.
//
//
