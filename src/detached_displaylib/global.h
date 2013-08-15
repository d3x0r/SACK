#include "ghdr.h"

#ifdef __cplusplus
#define DISPLAY_NAMESPACE RENDER_NAMESPACE namespace display {
#define DISPLAY_NAMESPACE_END } RENDER_NAMESPACE_END
#else
#define DISPLAY_NAMESPACE 
#define DISPLAY_NAMESPACE_END
#endif

DISPLAY_NAMESPACE

#if !defined(INTERNAL_BUFFER) && defined( __ARM__ )
//#define INTERNAL_BUFFER // need this for fluffy soft cursors...
#endif


typedef struct penging_rectangle_tag
{
	struct {
		_32 bHasContent : 1;
		_32 bTmpRect : 1;
	} flags;
   CRITICALSECTION cs;
	S_32 x, y;
   _32 width, height;
} PENDING_RECT, *PPENDING_RECT;



#ifndef DISPLAY_RENDER_CLIENT
typedef struct global_tag
{
	S_16 width, height;
	_16 ofs_x, ofs_y;
   // while operating on this root ...
   // lock out anyone else mucking with it also...
   CRITICALSECTION csSpaceRoot;
   PSPACENODE pSpaceRoot;
   PPANEL pRootPanel  // desktop/container of all other panels.
        , pTopMost;   // the 'top' of the display... the panel MOST visible.
	PPANEL pMouseOwner   // panel which gets all mouse messages
	     , pFocusedPanel // default panel to get keyboard messages.
	     , pLastFocusedPanel // when we get a lose focus - save the last focused thing here...
		  ; 
	struct {
		_32 reading_config      : 1;
		_32 close_dead_panels   : 1;
		_32 auto_owned_mouse    : 1; // set if mouse button down caused a own mouse
		_32 soft_cursor_always  : 1; // just put it out...
		_32 soft_cursor_down    : 1; // any button down
		_32 soft_cursor_up      : 1; // any button up
		_32 soft_cursor_on_down : 1; // when button goes down
		_32 soft_cursor_on_up   : 1; // when button goes up
		_32 soft_cursor_on      : 1; // yes do show it now.
		_32 soft_cursor_was_on  : 1; // yes did show it, clear and then ignore
		_32 touch_display       : 1;
		_32 calibrating_touch   : 1; // disable a lot of drawing operations.
		_32 in_mouse : 1; // any display updates while in mouse are only queued and are not done.
		_32 was_in_mouse : 1; // in_mouse was checked and found wanting, so we set this ...
		_32 in_draw_dispatch : 1; // updates are queued here also...  hmm this should still be in_mouse
		_32 configured : 1; // read the config file already.
	} flags;
   // only way we can implement soft cursors is with an internal buffer layer.
#ifdef INTERNAL_BUFFER
   S_32 SoftCursorAlpha;
   S_32 SoftCursorHotX, SoftCursorHotY; // offset to comput cursorX, Y with
   S_32 SoftCursorX, SoftCursorY; // top left corner of cursor sprite
	Image SoftCursor;
#endif
	S_32 last_cursor_x, last_cursor_y;
	_32 _last_buttons;

#ifdef __LINUX__
#ifndef __NO_SDL__
	SDL_Surface *surface;
#endif
#else
	PVIDEO hVideo;
#endif

#ifdef __RAW_FRAMEBUFFER__
	Image PhysicalSurface;  // updates are posted from realsurface to physical...
#endif
	Image RealSurface; // this is a composite between SoftSurface, Mouse and sprites
#ifdef INTERNAL_BUFFER
	Image SoftSurface;  // this is the surface apps write to...
#endif

	//PENDING_RECT update_rect;
   // address validation for return reload.
   //char   *pNameSpace; // list of names - \0\0 terminated. otherwise \0 is end of one.

	SFTFont TitleFont;

   PIMAGE_INTERFACE ImageInterface;
	PRENDER_INTERFACE RenderInterface;

	// this should be the last element, helps detect global structure
	// size changes.
   struct global_tag *MyAddress;

	PKEYDEFINE KeyDefs;
   // 2 bits per key for toggled and pressed states.
	FLAGSET( KeyboardState, 512 );
	FLAGSET( KeyboardMetaState, 256 );
	_32 KeyDownTime[256];
#ifdef __NO_SDL__
	_32 dwMsgBase; // LoadService(NULL) event msg offset result
#endif
   /* this is to protect between display updates and update to the physical display...*/
   CRITICALSECTION csUpdating;
	PLIST sprites;
	LOGICAL debug_log;
} GLOBAL, *PGLOBAL;

#define g global_display_data

#ifndef GLOBAL_STRUCTURE_DEFINED 
extern GLOBAL g;
#else
GLOBAL g;
#endif
#endif

#define TOGGLEKEY(display,code)    TOGGLEFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2 )
#define SETTOGGLEKEY(display,code)    SETFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2 )
#define CLEARTOGGLEKEY(display,code)    RESETFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2 )
#define SETKEY(display,code)       SETFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2+1 )
#define CLEARKEY(display,code)     RESETFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2+1 )
#define KEYPRESSED(display, code)  TESTFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2+1 )
#define KEYDOWN(display, code)     TESTFLAG( (display)?(display)->KeyboardState:g.KeyboardState, (code)*2 ) // toggled down...

void BuildSpaceTreeEx( PPANEL panel );

void DrawSpace( void *panel, PSPACEPOINT min, PSPACEPOINT max, CDATA color );

DISPLAY_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::image::render::display;
#endif

//--------------------------------------------------------
// $Log: global.h,v $
// Revision 1.33  2005/06/24 15:11:46  jim
// Merge with branch DisplayPerformance, also a fix for watcom compilation
//
// Revision 1.32.2.2  2005/06/22 23:12:10  jim
// checkpoint...
//
// Revision 1.32.2.1  2005/06/22 17:31:43  jim
// Commit test optimzied display
//
// Revision 1.33  2005/06/17 21:29:15  d3x0r
// Checkpoint... Seems that dirty rects can be used to minmize drawing esp. around moving/rsizing of windows
//
// Revision 1.32  2005/05/25 16:50:12  d3x0r
// Synch with working repository.
//
// Revision 1.32  2005/04/19 20:49:02  jim
// Abstract display render service to indexes through the message service
//
// Revision 1.31  2004/08/17 02:28:41  d3x0r
// Hmm seems to be an issue with update of focused complex surfaces... (palette).  Removed logging code (commented).  Seems that mouse render works all over, without failure, drag and draw work...
//
// Revision 1.30  2004/08/11 12:00:28  d3x0r
// Migrate to new, more common keystruct...
//
// Revision 1.29  2004/06/16 21:13:11  d3x0r
// Cleanups for keybind duplication between here and vidlib... that's all been cleaned...
//
// Revision 1.28  2004/06/16 10:27:23  d3x0r
// Added key events to display library...
//
// Revision 1.27  2004/05/28 17:11:52  d3x0r
// Just clean clean build, distclean... make system...
//
// Revision 1.26  2004/05/28 00:18:10  d3x0r
// Update structure
//
// Revision 1.25  2004/05/27 00:08:10  d3x0r
// Checkpoint - whatever.
//
// Revision 1.24  2004/05/19 20:32:54  d3x0r
// Fixed soft cursor under windows...
//
// Revision 1.24  2004/05/19 20:36:18  jim
// Okay soft cursor fixed for windows, and close to working under linux...
//
// Revision 1.23  2004/05/18 17:47:42  jim
// Fix cursor image loading...
//
// Revision 1.22  2003/12/04 10:40:51  panther
// Add to sync a more definitive sync
//
// Revision 1.21  2003/10/17 00:56:05  panther
// Rework critical sections.
// Moved most of heart of these sections to timers.
// When waiting, sleep forever is used, waking only when
// released... This is preferred rather than continuous
// polling of section with a Relinquish.
// Things to test, event wakeup order uner linxu and windows.
// Even see if the wake ever happens.
// Wake should be able to occur across processes.
// Also begin implmeenting MessageQueue in containers....
// These work out of a common shared memory so multiple
// processes have access to this region.
//
// Revision 1.20  2003/10/13 19:59:31  panther
// Hack some logging macros
// hack freetype to compile...
// hack displaylib to render blit_n directly
//
// Revision 1.19  2003/10/13 02:50:47  panther
// SFTFont's don't seem to work - lots of logging added back in
// display does work - but only if 0,0 biased, cause the SDL layer sucks.
//
// Revision 1.18  2003/04/24 16:32:50  panther
// Initial calibration support in display (incomplete)...
// mods to support more features in controls... (returns set interface)
// Added close service support to display_server and display_image_server
//
// Revision 1.17  2003/04/23 11:36:40  panther
// Multiple instances are not supported.  Global name space is not supported.  Revert all g-> to g.
//
// Revision 1.16  2003/04/16 08:15:22  panther
// Soft cursor mods
//
// Revision 1.15  2003/04/12 20:52:46  panther
// Added new type contrainer - data list.
//
// Revision 1.14  2003/04/11 13:35:11  panther
// new configuration - dropped color defs.  Added image to be coppied
//
// Revision 1.13  2003/03/30 18:19:55  panther
// Panel stacking issues...
//
// Revision 1.12  2003/03/29 22:52:00  panther
// New render/image layering ability.  Added support to Display for WIN32 usage (native not SDL)
//
// Revision 1.11  2003/03/27 15:26:12  panther
// Add Service Unloading in message service layer - still need a service notification message
//
// Revision 1.10  2003/03/26 11:11:12  panther
// Extend Display.Config optoins to specify the soft cursor options
//
// Revision 1.9  2003/03/21 09:55:36  panther
// Cleanup keyboard definitions - going to have to forward keyboards on a per-panel basis
//
// Revision 1.8  2003/03/21 08:57:27  panther
// Added IsKeyDown/KeyDown, SetMousePosition... cleaned obsolete code
//
// Revision 1.7  2003/02/18 10:29:30  panther
// Remove panels from focus when closed
//
// Revision 1.6  2003/02/08 16:20:34  panther
// Added messageserver to project.list
//
// Revision 1.5  2003/02/06 11:10:26  panther
// Migration to new interface headers
//
// Revision 1.4  2002/11/06 12:43:01  panther
// Updated display interface method, cleaned some code in the display image
// interface.  Have to establish who own's 'focus' and where windows
// are created.  The creation method REALLY needs the parent's window.  Which
// is a massive change (kinda)
//
// Revision 1.3  2002/11/05 09:55:55  panther
// Seems to work, added a sample configuration file.
// Depends on some changes in configscript.  Much changes accross the
// board to handle moving windows... now to display multiples.
//
// Revision 1.2  2002/10/29 09:31:34  panther
// Trimmed out Carriage returns, added CVS Logging.
// Fixed compilation under Linux.
//
//
