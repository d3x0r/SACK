/* <link sack::image::render::PRENDERER, Render> provides a
   method to display images on a screen. It is the interface
   between memory images and the window desktop or frame buffer
   the user is viewing on a monitor.
   
   
   
   Under windows, this is implemented as an HWND and an HBITMAP
   used to allow the application to draw. Updates are done
   directly from the drawable surface to the HWND as appropriate
   for the type of service. This is implemented with Vidlib.dll.
   
   
   
   Under Linux, this is mounted against SDL. SDL, however, does
   not give multiple display surfaces, so a more direct method
   should be used someday, other than SDL does a good job of
   aliasing frame buffer and X display windows to a consistant
   interface. This is implemented wit DisplayLib (as yet outside
   of the documentation). Display lib can interface either
   directly, or be mounted as a service across from a shared
   memory message service, and host multiple applications on a
   single frame buffer surface.
   TODO
   Implement displays as direct X displays, and allow managment
   there under linux.
   
   Displaylib was a good project, and although suffers from
   code-rot, it is probably still mostly usable. Message
   services were easily transported across a network, but then
   location services started failing.
   Example
   <code lang="c++">
   
   // get a render display, just a default window of some size
   // extended features are available for more precision.
   Render render = OpenDisplay(0);
   
   </code>
   
   A few methods of using this surface are available. One, you
   may register for events, and be called as required.
   <code lang="c++">
   RedrawCallback MyDraw = DrawHandler;
   MouseCallback MyMouse;
   </code>
   <code>
   KeyProc MyKey;
   CloseCallback MyClose;
   
   </code>
   <code lang="c++">
   // called when the surface is initially shown, or when its surface changes.
   // otherwise, the image drawn by the application is static, and does
   // not get an update event.
   SetRedrawHandler( render, MyDraw, 0 );
   
   // This will get an event every time a mouse event happens.
   // If no Key handler is specified, key strokes will also be mouse events.
   SetMouseHandler( render, MyMouse, 0 );
   
   // If the window was closed, get an event.
   SetCloseHandler( render, MyClose, 0 );
   
   // specify a handler to get keyboard events...
   SetKeyboardHandler( render, MyKey, 0 );
   
   </code>
   
   Or, if you don't really care about any events...
   <code lang="c++">
   // load an image
   Image image = LoadImageFile( "sample.jpg" );
   // get the image target of render
   Image display = GetDisplayImage( render );
   // copy the loaded image to the display image
   BlotImage( display, image );
   // and update the display
   UpdateDisplay( render );
   </code>
   
   <code lang="c++">
   
   void CPROC DrawHandler( PTRSZVAL psvUserData, 31~PRENDERER render )
   {
       Image display = GetDisplayImage( render );
       // the display image may change, because of an external resize
   
       // update the image to display as desired...
   
       // when done, the draw handler should call UpdateDisplay or...
       UpdateDisplayPortion( render, 0, 0, 100, 100 );
   }
   </code>
   
   Oh! And most importantly! Have to call this to put the window
   on the screen.
   <code lang="c++">
   UpdateDisplay( render );
   </code>
   
   Or maybe can pretend it was hidden
   <code lang="c++">
   RestoreDisplay( render );
   </code>                                                                     */



// this shouldprobably be interlocked with
//  display.h or vidlib.h(video.h)
#ifndef RENDER_INTERFACE_INCLUDED
// multiple inclusion protection symbol.
#define RENDER_INTERFACE_INCLUDED

#include <stdhdrs.h>

#ifdef __cplusplus
#ifdef _D3D_DRIVER
#define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d {
#define _RENDER_NAMESPACE namespace render { namespace d3d {
#define RENDER_NAMESPACE_END }}}}
#elif defined( _D3D10_DRIVER )
#define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d10 {
#define _RENDER_NAMESPACE namespace render { namespace d3d10 {
#define RENDER_NAMESPACE_END }}}}
#elif defined( _D3D11_DRIVER )
#define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d11 {
#define _RENDER_NAMESPACE namespace render { namespace d3d11 {
#define RENDER_NAMESPACE_END }}}}
#else
#define RENDER_NAMESPACE namespace sack { namespace image { namespace render {
/* <copy render.h>
   
   \ \             */
#define _RENDER_NAMESPACE namespace render {
#define RENDER_NAMESPACE_END }}}
#endif
#else
#define RENDER_NAMESPACE 
#define _RENDER_NAMESPACE 
#define RENDER_NAMESPACE_END
#endif

#include <sack_types.h>
#include <keybrd.h>
#include <image.h>
#ifndef __NO_INTERFACES__
#include <procreg.h>   // for interface, can omit if no interfaces
#endif
#ifndef __NO_MSGSVR__
#include <msgprotocol.h>  // for interface across the message service
#endif

#ifndef SECOND_RENDER_LEVEL
#define SECOND_RENDER_LEVEL
#define PASTE(sym,name) name
#else
#define PASTE2(sym,name) sym##name
#define PASTE(sym,name) PASTE2(sym,name)
#endif

#        ifdef RENDER_LIBRARY_SOURCE 
#           define RENDER_PROC(type,name) EXPORT_METHOD type CPROC PASTE(SECOND_RENDER_LEVEL,name)
#        else
#           define RENDER_PROC(type,name) IMPORT_METHOD type CPROC PASTE(SECOND_RENDER_LEVEL,name)
#        endif

SACK_NAMESPACE
/* <copy render.h>
   
   \ \             */
BASE_IMAGE_NAMESPACE
/* PRENDERER is the primary object this namespace deals with.
   
   
   See Also
   <link render.h>                                            */

_RENDER_NAMESPACE

/* Application layer abstract structure to handle displays. This
 is the type returned by OpenDisplay.                          */
typedef struct HVIDEO_tag *PRENDERER;
typedef struct key_function  KEY_FUNCTION;
typedef struct key_function *PKEY_FUNCTION;


// disable this functionality, it was never fully implemented, and is a lot to document.
#if ACTIVE_MESSAGE_IMPLEMENTED

// Message IDs 0-99 are reserved for
// very core level messages.
// Message IDs 100-999 are for general purpose window input/output
// Message ID 1000+ Usable by applications to transport messages via
//                  the image's default message loop.
enum active_msg_id {
   ACTIVE_MSG_PING    // Message ID 0 - contains a active image to respond to
   , ACTIVE_MSG_PONG    // Message ID 0 - contains a active image to respond to
   , ACTIVE_MSG_MOUSE = 100
   , ACTIVE_MSG_GAIN_FOCUS
   , ACTIVE_MSG_LOSE_FOCUS
   , ACTIVE_MSG_DRAG
   , ACTIVE_MSG_KEY
   , ACTIVE_MSG_DRAW
   , ACTIVE_MSG_CREATE
   , ACTIVE_MSG_DESTROY

   , ACTIVE_MSG_USER = 1000
};

typedef struct {
   enum active_msg_id ID;
   _32  size; // the size of the cargo potion of the message. (mostly data.raw)
   union {
  //--------------------
      struct {
         PRENDERER respondto; 
      } ping;
  //--------------------
      struct {
         int x, y, b;
      } mouse;
  //--------------------
      struct {
         PRENDERER lose;
      } gain_focus;
  //--------------------
      struct {
         PRENDERER gain;
      } lose_focus;
  //--------------------
      struct {
         _8 no_informaiton;
      } draw;
  //--------------------
      struct {
         _8 no_informaiton;
      } close;
  //--------------------
      struct {
         _8 no_informaiton;
      } create;
  //--------------------
      struct {
         _8 no_informaiton;
      } destroy;     
  //--------------------
      struct {
         _32 key;
      } key;
  //--------------------
      _8 raw[1];
   } data;
} ACTIVEMESSAGE, *PACTIVEMESSAGE;
#endif

// Event Message ID's CANNOT be 0
// Message Event ID (base+0) is when the
// server teriminates, and ALL client resources
// are lost.
// Message Event ID (base+1) is when the
// final message has been received, and any
// pending events collected should be dispatched.

#ifndef __NO_MSGSVR__
enum {
   /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_CloseMethod = MSG_EventUser,
  /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_RedrawMethod    
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_MouseMethod     
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_LoseFocusMethod 
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_KeyMethod       
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_GeneralMethod   
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_RedrawFractureMethod
 , /* These are internal messages to pass to the display handling
    thread. most are unimplemented.                             */
 MSG_ThreadEventPost 
};
#endif

#ifdef __WATCOMC__
#if ( __WATCOMC__ < 1291 )
#define NO_TOUCH
#endif
#endif

#ifndef WIN32
#define NO_TOUCH
#endif


#if defined( MINGW_SUX ) || defined( __LINUX__ )
#define NO_TOUCH
#endif

#if defined( __ANDROID__ )
// definately IS touch
#undef NO_TOUCH
#define MINGW_SUX
#endif

// static void OnBeginShutdown( "Unique Name" )( void ) { /* run shutdown code */ }
#define OnBeginShutdown(name) \
	__DefineRegistryMethod(WIDE("SACK"),BeginShutdown,WIDE("System"),WIDE("Begin Shutdown"),name WIDE("_begin_shutdown"),void,(void),__LINE__)


/* function signature for the close callback  which can be specified to handle events to close the display.  see SetCloseHandler. */
typedef void (CPROC*CloseCallback)( PTRSZVAL psvUser );
/* function signature to define hide/restore callback, it just gets the user data of the callback... */
typedef void (CPROC*HideAndRestoreCallback)( PTRSZVAL psvUser );
/* function signature for the redraw callback  which can be specified to handle events to redraw the display.  see SetRedrawHandler. */
typedef void (CPROC*RedrawCallback)( PTRSZVAL psvUser, PRENDERER self );
/* function signature for the mouse callback  which can be specified to handle events from mouse motion on the display.  see SetMouseHandler.
  would be 'wise' to retun 0 if ignored, 1 if observed (perhaps not used), but NOT ignored.*/
typedef int  (CPROC*MouseCallback)( PTRSZVAL psvUser, S_32 x, S_32 y, _32 b );

typedef struct input_point
{
   //
	RCOORD x, y;
	struct {
		BIT_FIELD new_event : 1;  // set on first down, clear on subsequent events
		BIT_FIELD end_event : 1; // set on first up, clear on first down,
	} flags;
} *PINPUT_POINT;

#ifndef NO_TOUCH

#ifdef MINGW_SUX
/*
 * Touch input mask values (TOUCHINPUT.dwMask)
 */
#define TOUCHINPUTMASKF_TIMEFROMSYSTEM  0x0001  // the dwTime field contains a system generated value
#define TOUCHINPUTMASKF_EXTRAINFO       0x0002  // the dwExtraInfo field is valid
#define TOUCHINPUTMASKF_CONTACTAREA     0x0004  // the cxContact and cyContact fields are valid
#endif

#define TOUCHEVENTF_USED 0x8000 // added to flags as touches are used.  Controls may use some of the touches but not all.

/* function signature for the touch callback  which can be specified to handle events from touching the display.  see SetMouseHandler.
  would be 'wise' to retun 0 if ignored, 1 if observed (perhaps not used), but NOT ignored.  Return 1 if some of the touches are used.
  This will trigger a check to see if there are unused touches to continue sending... oh but on renderer there's only one callback, more
  important as a note of the control touch event handerer.
  */
typedef int  (CPROC*TouchCallback)( PTRSZVAL psvUser, PINPUT_POINT pTouches, int nTouches );

#endif
/* function signature for the close callback  which can be specified to handle events to redraw the display.  see SetLoseFocusHandler. */
typedef void (CPROC*LoseFocusCallback)( PTRSZVAL dwUser, PRENDERER pGain );
// without a keyproc, you will still get key notification in the mousecallback
// if KeyProc returns 0 or is NULL, then bound keys are checked... otherwise
// priority is given to controls with focus that handle keys.
typedef int (CPROC*KeyProc)( PTRSZVAL dwUser, _32 keycode );
// without any other proc, you will get a general callback message.
#if ACTIVE_MESSAGE_IMPLEMENTED
typedef void (CPROC*GeneralCallback)( PTRSZVAL psvUser
                                     , PRENDERER image
												, PACTIVEMESSAGE msg );
#endif
typedef void (CPROC*RenderReadCallback)(PTRSZVAL psvUser, PRENDERER pRenderer, TEXTSTR buffer, INDEX len );
// called before redraw callback to update the background on the scene...
typedef void (CPROC*_3DUpdateCallback)( PTRSZVAL psvUser );

//----------------------------------------------------------
//   Mouse Button definitions
//----------------------------------------------------------
// the prefix of these may either be interpreted as MAKE - as in
// a make/break state of a switch.  Or may be interpreted as
// MouseKey.... such as KB_ once upon a time stood for KeyBoard,
// and not Keebler as some may have suspected.
enum ButtonFlags {
#ifndef MK_LBUTTON 
	MK_LBUTTON = 0x01, // left mouse button  MouseKey_ ?
#endif
#ifndef MK_MBUTTON
	MK_RBUTTON = 0x02,  // right mouse button MouseKey_ ?
#endif
#ifndef MK_RBUTTON
	MK_MBUTTON = 0x10,  // middle mouse button MouseKey_ ?
#endif
#ifndef MK_CONTROL
  MK_CONTROL = 0x08,  // the control key on the keyboard
#endif
#ifndef MK_ALT
  MK_ALT = 0x20,   // the alt key on the keyboard
#endif
#ifndef MK_SHIFT
  MK_SHIFT = 0x40,   // the shift key on the keyboard
#endif

  MK_SCROLL_DOWN  = 0x100,  // scroll wheel click down
  MK_SCROLL_UP    = 0x200,  // scroll wheel click up
  MK_SCROLL_LEFT  = 0x400,  // scroll wheel click left
  MK_SCROLL_RIGHT = 0x800,  // scroll wheel click right
#ifndef MK_NO_BUTTON
// used to indicate that there is
// no known button information available.  The mouse
// event which triggered this was beyond the realm of
// this mouse handler, but it is entitled to know that
// it now knows nothing.
  MK_NO_BUTTON = 0xFFFFFFFF,
#endif
// this bit will NEVER NEVER NEVER be set
// for ANY reason whatsoever. ( okay except when it's in MK_NO_BUTTON )
  MK_INVALIDBUTTON = 0x80000000,
// One or more other buttons were pressed.  These
// buttons are available by querying the keyboard state.
  MK_OBUTTON = 0x80, // any other button (keyboard)
  MK_OBUTTON_UP = 0x1000 // any other button (keyboard) went up
};

// mask to test to see if some button (physical mouse, not logical)
// is currently pressed...
#define MK_SOMEBUTTON       (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)
/* test to see if any button is clicked */
#define MAKE_SOMEBUTTONS(b)     ((b)&(MK_SOMEBUTTON))
// test a button variable to see if no buttons are currently pressed
// NOBUTTON, NOBUTTONS may be confusing, consider renaming these....
#define MAKE_NOBUTTONS(b)     ( !((b) & MK_SOMEBUTTON ) )
// break of some button
#define BREAK_NEWBUTTON(b,_b) ((((b)^(_b))&(_b))&MK_SOMEBUTTON)
// make of some button (the first down of a button)
#define MAKE_NEWBUTTON(b,_b) ((((b)^(_b))&(b))&MK_SOMEBUTTON)
// test current b vs prior _b to see if the  last button pressed is
// now not pressed...
#define BREAK_LASTBUTTON(b,_b)  ( BREAK_NEWBUTTON(b,_b) && MAKE_NOBUTTONS(b) )
// test current b vs prior _b to see if there is now some button pressed
// when previously there were no buttons pressed...
#define MAKE_FIRSTBUTTON(b,_b) ( MAKE_NEWBUTTON(b,_b) && MAKE_NOBUTTONS(_b) )
// these button states may reflect the current
// control, alt, shift key states.  There may be further
// definitions (meta?) And as of the writing of this comment
// these states may not be counted on, if you care about these
// please do validate that the code gives them to you all the way
// from the initial mouse message through all layers to the final
// application handler.





//----------------------------------------------------------
enum DisplayAttributes {
   /* when used by the Display Lib manager, this describes how to manage the subsurface */
  PANEL_ATTRIBUTE_ALPHA    = 0x10000,
   /* when used by the Display Lib manager, this describes how to manage the subsurface */
  PANEL_ATTRIBUTE_HOLEY    = 0x20000,
// when used by the Display Lib manager, this describes how to manage the subsurface
// focus on this window excludes any of it's parent/sibling panels
// from being able to focus.
  PANEL_ATTRIBUTE_EXCLUSIVE = 0x40000,
// when used by the Display Lib manager, this describes how to manage the subsurface
// child attribute affects the child is contained within this parent
  PANEL_ATTRIBUTE_INTERNAL  = 0x88000,
    // open the window as layered - allowing full transparency.
  DISPLAY_ATTRIBUTE_LAYERED = 0x0100,
    // window will not be in alt-tab list
  DISPLAY_ATTRIBUTE_CHILD = 0x0200,
    // set to WS_EX_TRANSPARENT - all mouse is passed, regardless of alpha/shape
  DISPLAY_ATTRIBUTE_NO_MOUSE = 0x0400,
    // when created, the display does not attempt to set itself into focus, otherwise we try to focus self.
  DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS = 0x0800,
};


    RENDER_PROC( int , InitDisplay) (void); // does not HAVE to be called but may

	 // this generates a mouse event though the mouse system directly
    // there is no queuing, and the mouse is completed before returning.
    RENDER_PROC( void, GenerateMouseRaw)( S_32 x, S_32 y, _32 b );
	 /* Create mouse events to self?
	    Parameters
	    x :  x of the mouse
	    y :  y of the mouse
	    b :  buttons of the mouse    */
	 RENDER_PROC( void, GenerateMouseDeltaRaw )( S_32 x, S_32 y, _32 b );

    /* Sets the title of the application window. Once upon a time,
       applications only were able to make a SINGLE window. Internally,
       all windows are mounted against a hidden application window,
       and this appilcation window gets the title.
       Parameters
       title :  Title for the application                               */
    RENDER_PROC( void , SetApplicationTitle) (const TEXTCHAR *title );
    /* Sets the title of the window (shows up in windows when
       alt-tabbing). Also shows up on the task tray icon (if there
       is one)
       Parameters
       render :  display to set the title of
       title :   new text for the title.                           */
    RENDER_PROC( void , SetRendererTitle) ( PRENDERER render, const TEXTCHAR *title );
    /* Sets the icon to show for the application's window.
       Parameters
       Icon :  this really has to be an HICON I think... it's for
               setting the icon on Windows' windows.              */
    RENDER_PROC( void , SetApplicationIcon)  (Image Icon); 
    /* Gets the size of the default desktop screen.
       Parameters
       width :   pointer to a 32 value for the display's width.
       height :  pointer to a 32 value for the display's height.
       
       Example
       <code lang="c++">
       _32 w, h;
       GetDisplaySize( &amp;w, &amp;h );
       </code>
       See Also
       <link sack::image::render::GetDisplaySizeEx@int@S_32 *@S_32 *@_32 *@_32 *, GetDisplaySizeEx> */
    RENDER_PROC( void , GetDisplaySize)      ( _32 *width, _32 *height );
	 /* \ \ 
	    Parameters
	    nDisplay :  display to get the coordinates of. 0 is the
	                default display from GetDesktopWindow(). 1\-n are
	                displays for multiple display systems, 1,2,3,4
	                etc..
	    x :         left screen coordinate of this display
	    y :         top screen coordinate of this display
	    width :     how wide this display is
	    height :    how tall this display is
	    
	    Example
	    <code lang="c#">
	    S_32 x, y;
	    _32 w, h;
	    
	    GetDisplaySizeEx( 1, &amp;x, &amp;y, &amp;w, &amp;h );
	    </code>                                                       */
	 RENDER_PROC (void, GetDisplaySizeEx) ( int nDisplay
													  , S_32 *x, S_32 *y
													  , _32 *width, _32 *height);
    /* Sets the first displayed physical window to a certain size. This
       should actually adjust the screen size. Like GetDisplaySize
       \returns the size of the actual display, this should set the
       size of the actual display.
       Parameters
       width :   new width of the screen
       height :  new height of the screen.                              */
    RENDER_PROC( void , SetDisplaySize)      ( _32 width, _32 height );

#ifdef WIN32
    /* Enable logging when updates happen to the real display.
       Parameters
       bEnable :  TRUE to enable, FALSE to disable.            */
    RENDER_PROC (void, EnableLoggingOutput)( LOGICAL bEnable );

	 /* A method to promote any arbitrary HWND to a PRENDERER. This
	    can be used to put SACK display surfaces in .NET
	    applications.
	    Parameters
	    hWnd :  HWND to make into a renderer.
	    
	    Returns
	    PRENDERER new renderer that uses HWND to update to.         */
	 RENDER_PROC (PRENDERER, MakeDisplayFrom) (HWND hWnd);
#endif

    /* This opens a display for output. It is opened hidden, so the
       application might draw to its surface before it is shown.
       
       This is not the most capable creation routine, but it is the
       most commonly aliased.
       Parameters
       attributes :  one or more <link sack::image::render::DisplayAttributes, DisplayAttributes>
                     or'ed togeteher.
       width :       width of the display
       height :      height of the display
       x :           x position of left the display
       y :           y position of the top of the display                                         */
    RENDER_PROC( PRENDERER, OpenDisplaySizedAt)     ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y );
    /* This opens a display for output. It is opened hidden, so the
       application might draw to its surface before it is shown.
       
       This is not the most capable creation routine, but it is the
       most commonly aliased.
       
       
       Parameters
       attributes :  one or more <link sack::image::render::DisplayAttributes, DisplayAttributes>
                     or'ed togeteher.
       width :       width of the display
       height :      height of the display
       x :           x position of left the display
       y :           y position of the top of the display
       above :       display to put this one above.                                               */
    RENDER_PROC( PRENDERER, OpenDisplayAboveSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above );
    /* This opens a display for output. It is opened hidden, so the
       application might draw to its surface before it is shown.
       
       This is not the most capable creation routine, but it is the
       most commonly aliased.
       Parameters
       attributes :  one or more <link sack::image::render::DisplayAttributes, DisplayAttributes>
                     or'ed togeteher.
       width :       width of the display
       height :      height of the display
       x :           x position of left the display
       y :           y position of the top of the display
       above :       display to put this one above.
       below :       display to put this one under. (for building
                     behind a cover window)                                                       */
    RENDER_PROC( PRENDERER, OpenDisplayAboveUnderSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under );
	 /* Sets the alpha level of the overall display window.
	    Parameters
	    hVideo :  display to set the overall fade level on
	    level :   the level of fade from 0 (transparent) to 255
	              (opaque)
	    
	    Example
	    <code lang="c++">
	    PRENDERER render = OpenDisplay( 0 );
	    int i;
	    UpdateDisplay( render );
	    </code>
	    <code>
	    
	    // the window will slowly fade out
	    for( i = 255; i \> 0; i-- )
	    </code>
	    <code lang="c++">
	        SetDisplayFade( render, i );
	    
	    CloseDisplay( render );  // Hiding the display works too, if it is to be reused.
	    </code>                                                                          */
	 RENDER_PROC( void, SetDisplayFade )( PRENDERER hVideo, int level );

    /* closes a display, releasing all resources assigned to it.
       Parameters
       hDisplay :  Render display to close.                      */
    RENDER_PROC( void         , CloseDisplay) ( PRENDERER );

    /* Updates just a portion of a display window. Minimizing the
       size required for screen output greatly increases efficiency.
       Also on vista+, this will update just a portion of a
       transparent display.
       Parameters
       hVideo :  the display to update
       x :       the left coordinate of the region to update
       y :       the top coordinate of the region to update
       width :   the width of the region to update
       height :  the height of the region to update
       DBG_PASS information is used to track who is doing updates
       when update logging is enabled.                               */
    RENDER_PROC( void , UpdateDisplayPortionEx) ( PRENDERER, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
/* <combine sack::image::render::UpdateDisplayPortionEx@PRENDERER@S_32@S_32@_32@_32 height>
   
   \ \                                                                                      */
#define UpdateDisplayPortion(r,x,y,w,h) UpdateDisplayPortionEx(r,x,y,w,h DBG_SRC )
	 /* Updates the entire surface of a display.
	    Parameters
	    display :  display to update
	    DBG_PASS information is passed for logging writing to
	    physical display.
	                                                          */
	 RENDER_PROC( void , UpdateDisplayEx)        ( PRENDERER DBG_PASS );
#define UpdateDisplay(r) UpdateDisplayEx(r DBG_SRC)
/* Gets the current location and size of a display.
       Parameters
       hVideo :  display to get the position of
       x :       pointer to a signed 32 bit value to get the left
                 edge of the display.
       y :       pointer to a signed 32 bit value to get the top edge
                 of the display.
       width :   pointer to a unsigned 32 bit value to get the width.
       height :  pointer to a unsigned 32 bit value to get the
                 height.                                              */
    RENDER_PROC( void, GetDisplayPosition)   ( PRENDERER, S_32 *x, S_32 *y, _32 *width, _32 *height );
    /* Moves a display to an absolute position.
       Parameters
       render :  the display to move
       x :       new X coordinate for the left of the display
       y :       new Y coordinate for the top of the display  */
    RENDER_PROC( void , MoveDisplay)          ( PRENDERER, S_32 x, S_32 y );
    /* Moves a display relative to its current position.
       Parameters
       render :  the display to move
       delx :    a signed amount to add to its X coordiante
       dely :    a signed amount ot add to its Y coordinate. ( bigger
                 values go down the screen )                          */
    RENDER_PROC( void , MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely );
    /* Sets the display's current size. If it is different than
       before, will invoke render's redraw callback.
       Parameters
       display :  the display to set the size of
       w :        new width of the display
       h :        new height of the display                     */
    RENDER_PROC( void , SizeDisplay)          ( PRENDERER, _32 w, _32 h );
    /* Sets the display's current size relative to what it currently
       is. If it is different than before, will invoke render's
       redraw callback.
       Parameters
       display :  the display to set the size of
       w :        signed value to add to current width
       h :        signed value to add to current height              */
    RENDER_PROC( void , SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh );
   /* Change the position and size of a display.
      Parameters
      hVideo :  display to move and size
      x :       new left coordinate of the display
      y :       new top coordinate of the display
      w :       new width of the display
      h :       new height of the display          */
   RENDER_PROC( void, MoveSizeDisplay )( PRENDERER hVideo
                                        , S_32 x, S_32 y
                                        , S_32 w, S_32 h );
   /* Moves and changes the display size relative to its current
      size. All parameters are relative to current.
      Parameters
      hVideo :  display to move and change the size of
      delx :    amount to modify the left coordinate by
      dely :    amount to modify the top coordinate by
      delw :    amount to change the width by
      delh :    amount to change the height by                   */
   RENDER_PROC( void, MoveSizeDisplayRel )( PRENDERER hVideo
                                        , S_32 delx, S_32 dely
                                        , S_32 delw, S_32 delh );
		/* Put the display above another display. This makes sure that
		   the displays are stacked at least in this order.
		   Parameters
		   this_display :  the display to put above another
		   that_display :  the display that will be on the bottom.     */
		RENDER_PROC( void , PutDisplayAbove)      ( PRENDERER this_display, PRENDERER that_display );
      /* put this in container
	   Parameters
	   hVideo :      Display to put into another display surface
	   hContainer :  The new parent window of the hVideo.
	   
	   Example
	   <code lang="c#">
	   Render render = OpenDisplay( 0 );
	   Render parent = OpenDisplay( 0 );
	   PutDisplayIn( render, parent );
	   </code>                                                   */
	 RENDER_PROC (void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer); 

    /* Gets the Image from the Render.
       Parameters
       renderer :  the display window to get the surface of.
       
       Returns
       Image that is the surface of the window to draw to.   */
    RENDER_PROC( Image , GetDisplayImage)     ( PRENDERER );

    /* Sets the close handler callback. Called when a window is
       closed externally.
       Parameters
       hVideo :     display to set the close handler for
       callback :   close method to call when the display is called
       user_data :  user data passed to close method when invoked.  */
    RENDER_PROC( void , SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL );
    /* Specifies the mouse event handler for a display.
       Parameters
       hVideo :     display to set the mouse handler for
       callback :   the routine to call when a mouse event happens.
       user_data :  this value is passed to the callback routine when
                    it is called.                                     */
    RENDER_PROC( void , SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL );
    /* Specifies the hide event handler for a display.
       Parameters
       hVideo :     display to set the hide handler for
       callback :   the routine to call when a hide event happens.
       user_data :  this value is passed to the callback routine when
                    it is called.                                     */
    RENDER_PROC( void , SetHideHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
    /* Specifies the restore event handler for a display.
       Parameters
       hVideo :     display to set the restore handler for
       callback :   the routine to call when a restore event happens.
       user_data :  this value is passed to the callback routine when
                    it is called.                                     */
    RENDER_PROC( void , SetRestoreHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
#ifndef NO_TOUCH
    /* Specifies the touch event handler for a display.
       Parameters
       hVideo :     display to set the touch handler for
       callback :   the routine to call when a touch event happens.
       user_data :  this value is passed to the callback routine when
                    it is called.                                     */
 	  RENDER_PROC( void , SetTouchHandler)      ( PRENDERER, TouchCallback, PTRSZVAL );
#endif
	 /* Sets the function to call when a redraw event is required.
	    Parameters
	    hVideo :     display to set the handler for
	    callback :   function to call when a redraw is required (or
	                 requested).
	    user_data :  this value is passed to the redraw callback.
	    
	    Example
	    See <link render.h>
	    
	    
	    See Also
	    <link sack::image::render::Redraw@PRENDERER, Redraw>        */
	 RENDER_PROC( void , SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL );
	 // call this to call the callback registered. as appropriate.  Said callback
    // should never be directly called by application.
    RENDER_PROC( void, Redraw )( PRENDERER hVideo );


    /* Sets the keyboard handler callback for a display
       Parameters
       hVideo :     display to receive key events for.
       callback :   callback invoked when a key event happens.
       user_data :  user data passed to the callback when invoked.
       
       Remarks
       the keyboard handler may make use of the scan code itself for
       PKEYDEFINE structures. There are also a variety of methods
       for checking the 32 bit key value. The value passed to the
       keyboard handler contains most all of the information about
       the state of the keyboard and specific key.                   */
    RENDER_PROC( void , SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* Sets a callback handler called when focus is gained or lost
       by the display.
       Parameters
       hVideo :     display to set the event on
       callback :   the user callback to call when focus is lost or
                    gained.
       user_data :  user data passed to the callback when invoked.
       
       Note
       When the LoseFocusCallback is called, the renderer is the one
       that is getting the focus. This may be you, may be NULL
       (everyone losing focus) or may be another PRENDERER in your
       application.                                                  */
    RENDER_PROC( void , SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
    /* Undefined */
    RENDER_PROC( void, SetRenderReadCallback )( PRENDERER pRenderer, RenderReadCallback callback, PTRSZVAL psv );
#if ACTIVE_MESSAGE_IMPLEMENTED
    RENDER_PROC( void , SetDefaultHandler)    ( PRENDERER, GeneralCallback, PTRSZVAL );
#endif
    /* Receives the current global mouse state, and position in
       screen coordinates.
       Parameters
       x :  pointer to a signed 32 bit value for the mouse X position.
       y :  pointer to a signed 32 bit value for the mouse Y position.
       b :  current state of mouse buttons. See <link sack::image::render::ButtonFlags, ButtonFlags>. */
    RENDER_PROC( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b );
    /* Gets the current mouse position in screen coordinates.
       Parameters
       x :  pointer to a signed 32 bit value for the mouse position
       y :  pointer to a signed 32 bit value for the mouse position
       
       Example
       <code lang="c++">
       S_32 x, y;
       GetMousePosition( &amp;x, &amp;y );
       </code>                                                      */
    RENDER_PROC( void , GetMousePosition)     ( S_32 *x, S_32 *y );
    /* Sets the mouse pointer at the specified display coordinates.
       Parameters
       hDisplay :  display to use to where to position the mouse. Will
                   fault if NULL is passed.
       x :         x relative to the display to set the mouse
       y :         y relative to the display to set the mouse          */
    RENDER_PROC( void , SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );

    /* Test a display to see if it is focused.
       Parameters
       hVideo :  display to check to see if it has focus. (keyboard
                 \input)
       
       Returns
       TRUE if focused, else FALSE.                                 */
    RENDER_PROC( LOGICAL , HasFocus)          ( PRENDERER );

#if ACTIVE_MESSAGE_IMPLEMENTED
    RENDER_PROC( int, SendActiveMessage)     ( PRENDERER dest, PACTIVEMESSAGE msg );
    RENDER_PROC( PACTIVEMESSAGE , CreateActiveMessage) ( int ID, int size, ... );
#endif
    /* Translates a key's scancode into text. Handles things like
       capslock, shift...
       Parameters
       key :  KEY_ to translate
       
       Returns
       char that the key represents. (should implement a method to
       get back the UNICODE character).                            */
    RENDER_PROC( TEXTCHAR, GetKeyText)             ( int key );
    /* Simple check to see if a key is in a pressed state.
       Parameters
       display :  display to check the key state in
       key :      KEY_ symbol to check.                    */
    RENDER_PROC( _32, IsKeyDown )              ( PRENDERER display, int key );
    /* \ \ 
       Parameters
       display :  display to test the key status in
       key :      KEY_ symbol to check if the key is pressed
       
       Returns
       TRUE if the key is down, else FALSE.                  */
    RENDER_PROC( _32, KeyDown )                ( PRENDERER display, int key );
    /* Sometimes displays can be closed by external forces (the
       close button on most windows). This tests to see if a display
       is still valid, or if it has been closed externally.
       Returns
       TRUE if display is still okay. FALSE if the display is no
       longer able to be used.
       Parameters
       display :  the display to check the validity of.              */
    RENDER_PROC( LOGICAL, DisplayIsValid )     ( PRENDERER display );
    /* Assigns all mouse input to a window. This allows the window
       to process messages which are outside of itself normally.
       Parameters
       display :  which window wants to own the mouse
       own :      1 to own, 0 to release ownership.                */
    RENDER_PROC( void, OwnMouseEx )            ( PRENDERER display, _32 bOwn DBG_PASS );
    /* Proprietary routine for reading touch screen serial devices
       directly and performing self calibration. Should rely on
       system driver and it's calibration instead.                 */
    RENDER_PROC( int, BeginCalibration )       ( _32 points );
    /* Used when display is accessed via a remote message pipe, this
       allows all render operations to be flushed and processed.
       Parameters
       display :  display to flush                                   */
    RENDER_PROC( void, SyncRender )            ( PRENDERER display );

/* Makes a display topmost. There isn't a way to un-topmost a
   window.
   Parameters
   hVideo :  display to make topmost
   
   Note
   Windows maintains at least two distinct stacks of windows. Normal
   windows in the normal window stack, and a set of windows that
   are above all other windows (except other windows that are
   also topmost).                                                    */
RENDER_PROC( void, MakeTopmost )( PRENDERER hVideo );
/* This makes the display topmost, but more so, any window that
   gets put over it it will attempt put itself over it.
   Parameters
   hVideo :  display to make top top most.                      */
RENDER_PROC (void, MakeAbsoluteTopmost) (PRENDERER hVideo);
/* Tests a display to see if it is set as topmost.
   Parameters
   hVideo :  display to inquire if it's topmost.
   
   Returns
   TRUE if display is topmost, else FALSE.         */
RENDER_PROC( int, IsTopmost )( PRENDERER hVideo );
/* Hides a display. That is, the content no longer shows on the
   users display.
   Parameters
   hVideo :  the handle of the Render to hide.
   
   See Also
   <link sack::image::render::RestoreDisplay@PRENDERER, RestoreDisplay> */
RENDER_PROC( void, HideDisplay )( PRENDERER hVideo );
/* Puts a display back on the screen. This is used in
   conjunction with HideDisplay().
   Parameters
   hVideo :  display to restore                       */
RENDER_PROC( void, RestoreDisplay )( PRENDERER hVideo );
	RENDER_PROC( void, RestoreDisplayEx )( PRENDERER hVideo DBG_PASS );
#define RestoreDisplay(n) RestoreDisplayEx( n DBG_SRC )
/* A check to see if HideDisplay has been applied to the
   display.
   Returns
   TRUE if the display is hidden, otherwise FALSE.
   Parameters
   video :  the display to check if hidden               */
RENDER_PROC( LOGICAL, IsDisplayHidden )( PRENDERER video );

// set focus to display, no events are generated if display already
// has the focus.
RENDER_PROC( void, ForceDisplayFocus )( PRENDERER display );

// display set as topmost within it's group (normal/bottommost/topmost)
RENDER_PROC( void, ForceDisplayFront )( PRENDERER display );
// display is force back one layer... or forced to bottom?
// alt-n pushed the display to the back... alt-tab is different...
RENDER_PROC( void, ForceDisplayBack )( PRENDERER display );

/* Not implemented on windows native, is for getting back
   display information over message service abstraction.
   
   
   
   if a readcallback is enabled, then this will be no-wait, and
   one will expect to receive the read data in the callback.
   Otherwise this will return any data which is present already,
   also non wait. Returns length read, INVALID_INDEX if no data
   read.
   
   If there IS a read callback, return will be 1 if there was no
   previous read queued, and 0 if there was already a read
   pending there may be one and only one read queued (for now)
   In either case if the read could not be queued, it will be
   0..
   
   If READLINE is true - then the result of the read will be a
   completed line. if there is no line present, and no callback
   defined, this will return INVALID_INDEX characters... 0
   characters is a n only (in line mode) 0 will be returned for
   no characters in non line mode...
   
   it will not have the end of line terminator (as generated by
   a non-bound enter key) I keep thinking there must be some
   kinda block mode read one can do, but no, uhh no, there's no
   way to get the user to put in X characters exactly....?
   
   
   Parameters
   pRenderer :  display to read from
   buffer :     buffer to read into
   maxlen :     maximum length of buffer to read
   bReadLine :  ???                                              */
RENDER_PROC( _32, ReadDisplayEx )( PRENDERER pRenderer, TEXTSTR buffer, _32 maxlen, LOGICAL bReadLine );
/* Unused. Incomplete. */
#define ReadDisplay(r,b,len)      ReadDisplayEx(r,b,len,FALSE)
/* Unused. Incomplete. */
#define ReadDisplayLine(r,b,len)  ReadDisplayEx(r,b,len,TRUE)


/* Issues an update to a layered (transparent) window. This does
   the update directly, and does not have to be done within the
   redraw event.
   Parameters
   hVideo :    display to update a part of
   bContent :  TRUE is only the passed rectangle should update
   x :         left coordinate of the region to update to
               physical display
   y :         top coordinate of the region to update to physical
               display
   w :         width of the region to update to physical display
   h :         height of the region to update to physical display */
RENDER_PROC( void, IssueUpdateLayeredEx )( PRENDERER hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS );


#ifndef KEY_STRUCTURE_DEFINED
typedef LOGICAL (CPROC*KeyTriggerHandler)(PTRSZVAL,_32 keycode);
typedef struct KeyDefine *PKEYDEFINE;
#endif
/* Can create an external key binder to store key event
   bindings. One of these is available per display.
   Example
   <code lang="c++">
   void Alt_A_Pressed(PTRSZVAL user_data,_32 keycode)
   {
       // do something when alt-a is pressed.
   }
   
   {
      PKEYDEFINE my_key_events = CreateKeyBinder();
      BindKeyToEventEx( my_key_events, KEY_A, KEY_MOD_ALT, Alt_A_Pressed, 0 );
   }
   
   // then later, in a KeyProc handler...
   HandleKeyEvents( my_key_events, keycode );
   </code>                                                                     */
RENDER_PROC( PKEYDEFINE, CreateKeyBinder )( void );
/* Destroyes a PKEYDEFINE previously created with
   CreateKeyBinder.
   Parameters
   pKeyDef :  key binder to destroy.              */
RENDER_PROC( void, DestroyKeyBinder )( PKEYDEFINE pKeyDef );
/* Evaluates a key against the key defines to trigger possible
   events.
   Parameters
   KeyDefs :  PKEYDEFINE keystate which has keys bound to it.
   keycode :  the keycode passed to a KeyProc handler.         */
RENDER_PROC( int, HandleKeyEvents )( PKEYDEFINE KeyDefs, _32 keycode );

/* Assigns a callback routine to a key event.
   Parameters
   KeyDefs :   pointer to key table to set event in
   scancode :  scancode of the key \- this is a KEY_ code from
               keybrd.h
   modifier :  specific modifiers pressed for this event (control,
               alt, shift)
   trigger :   the trigger function to invoke when the key is
               pressed
   psv :       a PTRSZVAL user data passed to the trigger function
               when invoked.                                       */
RENDER_PROC( int, BindEventToKeyEx )( PKEYDEFINE KeyDefs, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
/* Binds a key to a display.
   Parameters
   pRenderer :  display to set the event in (each display has a
                PKEYDEFINE internally. If this is NULL, then the
                event is bound to global events, an applies for
                any display window that gets a key input.
   scancode :   key scancode (a KEY_ identifier from keybrd.h)
   modifier :   key state modifier to apply to match the trigger
                on (control, alt, shift)
   trigger :    callback to invoke when the key combination is
                pressed
   psv :        user data to pass to the trigger when invoked.   */
RENDER_PROC( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
/* Remove a previous binding to a key.
   Parameters
   pRenderer :  renderer to remove the key bind from
   scancode :   key scancode to stop checking
   modifier :   key modifier to stop checking        */
RENDER_PROC( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier );

/* A way to test to see if the current input device is a touch
   display. This can affect how mouse clicks are handles for
   things like buttons.
   Parameters
   None.
   
   
   Returns
   0.                                                          */
RENDER_PROC( int, IsTouchDisplay )( void );

// static void OnInputTouch( "Touch Handler" )(
#define OnSurfaceInput(name) \
	__DefineRegistryMethod(WIDE("sack/render"),SurfaceInput,WIDE("surface input"),WIDE("SurfaceInput"),name,void,( int nInputs, PINPUT_POINT pInputs ),__LINE__)


#ifndef PSPRITE_METHOD
/* Unused. Incomplete. */
#define PSPRITE_METHOD PSPRITE_METHOD
RENDER_NAMESPACE_END

IMAGE_NAMESPACE
   /* define sprite draw method structure */
	typedef struct sprite_method_tag *PSPRITE_METHOD;
IMAGE_NAMESPACE_END
   
RENDER_NAMESPACE

#endif

/* Adds a sprite rendering method to the display. Just before
   updating to the display, the display is saved, and sprite
   update callbacks are issued. then the resulting display is
   \output. Sprite data only exists on the output image just
   before it is put on the physical display.
   Parameters
   render :    the display to attach a sprite render method to
   callback :  callback to draw sprites
   psv :       user data passed to callback when it is called
   
   Returns
   Pointer to a SpriteMethod that can be used in SavePortion...
   uhmm
   Note
   Has fallen into disrepair, and may need work before sprites
   work this way.                                               */
RENDER_PROC( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv );

/* signature for callback method to pass to
   WinShell_AcceptDroppedFiles.             */
typedef void (CPROC*dropped_file_acceptor)(PTRSZVAL psv, CTEXTSTR filename, S_32 x, S_32 y );
/* Adds a callback to call when a file is dropped. Each callback
   can return 0 that it did not accept the file, or 1 that it
   did. once the file is accepted by a handler, it is not passed
   to any other handlers.
   Parameters
   renderer :  display to handle dropped files for
   f :         callback to acceptor
   psvUser :   user data passed to acceptor when it is invoked   */
RENDER_PROC( void, WinShell_AcceptDroppedFiles )( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser );

/* Enables a timer on the mouse to hide the cursor after a
   second that the mouse is not being moved.
   Parameters
   hVideo :   display to hide the mouse automatically for
   bEnable :  enable automatic hiding. After a few seconds, the
              mouse goes away until it moves(not click).        */
RENDER_PROC (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable );
/* Sets whether the display wants to get any mouse events at
   all.
   Parameters
   hVideo :    display to set the property for
   bNoMouse :  if 1, disables any mouse events. if 0, enables mouse
               events to the display.                               */
RENDER_PROC( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse );

#ifdef WIN32
	/* \returns the native handle used to output to. this can be an
	   SDL_Screen or HWND depending on platform.
	   Parameters
	   video :  display to get the native handle for
	   
	   Returns
	   the system handle of the display object being used to output. */
	RENDER_PROC( HWND, GetNativeHandle )( PRENDERER video );
#endif

/* <combine sack::image::render::OwnMouseEx@PRENDERER@_32 bOwn>
   
   \ \                                                          */
#define OwnMouse(d,o) OwnMouseEx( d, o DBG_SRC )

/* <combine sack::image::render::OpenDisplaySizedAt@_32@_32@_32@S_32@S_32>
   
   \ \                                                                     */
#define OpenDisplay(a)            OpenDisplaySizedAt(a,-1,-1,-1,-1)
/* <combine sack::image::render::OpenDisplaySizedAt@_32@_32@_32@S_32@S_32>
   
   \ \                                                                     */
#define OpenDisplaySized(a,w,h)   OpenDisplaySizedAt(a,w,h,-1,-1)
/* <combine sack::image::render::OpenDisplayAboveSizedAt@_32@_32@_32@S_32@S_32@PRENDERER>
   
   \ \                                                                                    */
#define OpenDisplayAbove(p,a)            OpenDisplayAboveSizedAt(p,-1,-1,-1,-1,a)
/* <combine sack::image::render::OpenDisplayAboveSizedAt@_32@_32@_32@S_32@S_32@PRENDERER>
   
   \ \                                                                                    */
#define OpenDisplayAboveSized(p,a,w,h)   OpenDisplayAboveSizedAt(p,w,h,-1,-1,a)
/* <combine sack::image::render::OpenDisplayAboveUnderSizedAt@_32@_32@_32@S_32@S_32@PRENDERER@PRENDERER>
   
   \ \                                                                                                   */
#define OpenDisplayUnderSizedAt(p,a,w,h,x,y) OpenDisplayAboveUnderSizedAt(a,w,h,x,y,NULL,p) 

/* Lock the renderer for this thread to use. */
RENDER_PROC( void, LockRenderer )( PRENDERER render );
/* Unlock the renderer for other threads to use. */
RENDER_PROC( void, UnlockRenderer )( PRENDERER render );

/* Function to check if the draw mode of the renderer requires
   an ALL update (opengl/direct3d) every frame the whole display
   must be drawn.                                                */
RENDER_PROC( LOGICAL, RequiresDrawAll )( void );

RENDER_PROC( void, MarkDisplayUpdated )( PRENDERER );


#ifndef __NO_INTERFACES__
/* Interface defines the functions that are exported from the
   render library. This interface may be retrieved with
   LoadInterface( "\<appropriate name" ).                     */
_INTERFACE_NAMESPACE

/* Macro to define exports for render.h */
#define RENDER_PROC_PTR(type,name) type  (CPROC*_##name)
/* <combine sack::image::render::render_interface_tag>
   
	\ \                                                 */
typedef struct render_interface_tag RENDER_INTERFACE;
/* <combine sack::image::render::render_interface_tag>
   
	\ \                                                 */
typedef struct render_interface_tag *PRENDER_INTERFACE;
/* This is a function table interface to the video library. Allows
   application to not be linked to the video portion directly,
   allowing dynamic replacement.                                   */
struct render_interface_tag
{
      /* <combine sack::image::render::InitDisplay>
         
         \ \                                        */
       RENDER_PROC_PTR( int , InitDisplay) (void); 

       /* <combine sack::image::render::SetApplicationTitle@TEXTCHAR *>
          
          \ \                                                           */
			 RENDER_PROC_PTR( void , SetApplicationTitle) (const TEXTCHAR *title );
          /* <combine sack::image::render::SetApplicationIcon@Image>
                                                    
                                                    \ \                                                     */
       RENDER_PROC_PTR( void , SetApplicationIcon)  (Image Icon); 
    /* <combine sack::image::render::GetDisplaySize@_32 *@_32 *>
       
       \ \                                                       */
    RENDER_PROC_PTR( void , GetDisplaySize)      ( _32 *width, _32 *height );
    /* <combine sack::image::render::SetDisplaySize@_32@_32>
       
       \ \                                                   */
    RENDER_PROC_PTR( void , SetDisplaySize)      ( _32 width, _32 height );

    /* <combine sack::image::render::OpenDisplaySizedAt@_32@_32@_32@S_32@S_32>
       
       \ \                                                                     */
    RENDER_PROC_PTR( PRENDERER , OpenDisplaySizedAt)     ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y );
    /* <combine sack::image::render::OpenDisplayAboveSizedAt@_32@_32@_32@S_32@S_32@PRENDERER>
       
       \ \                                                                                    */
    RENDER_PROC_PTR( PRENDERER , OpenDisplayAboveSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above );
    /* <combine sack::image::render::CloseDisplay@PRENDERER>
       
       \ \                                                   */
    RENDER_PROC_PTR( void        , CloseDisplay) ( PRENDERER );

    /* <combine sack::image::render::UpdateDisplayPortionEx@PRENDERER@S_32@S_32@_32@_32 height>
       
       \ \                                                                                      */
    RENDER_PROC_PTR( void, UpdateDisplayPortionEx) ( PRENDERER, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
    /* <combine sack::image::render::UpdateDisplayEx@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( void, UpdateDisplayEx)        ( PRENDERER DBG_PASS);
                             
    /* <combine sack::image::render::GetDisplayPosition@PRENDERER@S_32 *@S_32 *@_32 *@_32 *>
       
       \ \                                                                                   */
    RENDER_PROC_PTR( void, GetDisplayPosition)   ( PRENDERER, S_32 *x, S_32 *y, _32 *width, _32 *height );
    /* <combine sack::image::render::MoveDisplay@PRENDERER@S_32@S_32>
       
       \ \                                                            */
    RENDER_PROC_PTR( void, MoveDisplay)          ( PRENDERER, S_32 x, S_32 y );
    /* <combine sack::image::render::MoveDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely );
    /* <combine sack::image::render::SizeDisplay@PRENDERER@_32@_32>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, SizeDisplay)          ( PRENDERER, _32 w, _32 h );
    /* <combine sack::image::render::SizeDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh );
    /* <combine sack::image::render::MoveSizeDisplayRel@PRENDERER@S_32@S_32@S_32@S_32>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, MoveSizeDisplayRel )  ( PRENDERER hVideo
                                                 , S_32 delx, S_32 dely
                                                 , S_32 delw, S_32 delh );
    RENDER_PROC_PTR( void, PutDisplayAbove)      ( PRENDERER, PRENDERER ); /* <combine sack::image::render::PutDisplayAbove@PRENDERER@PRENDERER>
                                                              
                                                              \ \                                                                */
 
    /* <combine sack::image::render::GetDisplayImage@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( Image, GetDisplayImage)     ( PRENDERER );

    /* <combine sack::image::render::SetCloseHandler@PRENDERER@CloseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetMouseHandler@PRENDERER@MouseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRedrawHandler@PRENDERER@RedrawCallback@PTRSZVAL>
       
       \ \                                                                               */
    RENDER_PROC_PTR( void, SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL );
    /* <combine sack::image::render::SetKeyboardHandler@PRENDERER@KeyProc@PTRSZVAL>
       
       \ \                                                                          */
    RENDER_PROC_PTR( void, SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
       
       \ \                                                                                     */
    RENDER_PROC_PTR( void, SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
    /* <combine sack::image::render::SetDefaultHandler@PRENDERER@GeneralCallback@PTRSZVAL>
       
       \ \                                                                                 */
#if ACTIVE_MESSAGE_IMPLEMENTED
			 RENDER_PROC_PTR( void, SetDefaultHandler)    ( PRENDERER, GeneralCallback, PTRSZVAL );
#else
       POINTER junk1;
#endif
    /* <combine sack::image::render::GetMousePosition@S_32 *@S_32 *>
       
		 \ \                                                           */
    RENDER_PROC_PTR( void, GetMousePosition)     ( S_32 *x, S_32 *y );
    /* <combine sack::image::render::SetMousePosition@PRENDERER@S_32@S_32>
       
       \ \                                                                 */
    RENDER_PROC_PTR( void, SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );

    /* <combine sack::image::render::HasFocus@PRENDERER>
       
       \ \                                               */
    RENDER_PROC_PTR( LOGICAL, HasFocus)          ( PRENDERER );

    /* <combine sack::image::render::GetKeyText@int>
       
       \ \                                           */
    RENDER_PROC_PTR( TEXTCHAR, GetKeyText)           ( int key );
    /* <combine sack::image::render::IsKeyDown@PRENDERER@int>
       
       \ \                                                    */
    RENDER_PROC_PTR( _32, IsKeyDown )        ( PRENDERER display, int key );
    /* <combine sack::image::render::KeyDown@PRENDERER@int>
       
       \ \                                                  */
    RENDER_PROC_PTR( _32, KeyDown )         ( PRENDERER display, int key );
    /* <combine sack::image::render::DisplayIsValid@PRENDERER>
       
       \ \                                                     */
    RENDER_PROC_PTR( LOGICAL, DisplayIsValid )  ( PRENDERER display );
    /* <combine sack::image::render::OwnMouseEx@PRENDERER@_32 bOwn>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS);
    /* <combine sack::image::render::BeginCalibration@_32>
       
       \ \                                                 */
    RENDER_PROC_PTR( int, BeginCalibration )       ( _32 points );
    /* <combine sack::image::render::SyncRender@PRENDERER>
       
       \ \                                                 */
    RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */

	 /* <combine sack::image::render::MoveSizeDisplay@PRENDERER@S_32@S_32@S_32@S_32>
	    
	    \ \                                                                          */
	 RENDER_PROC_PTR( void, MoveSizeDisplay )( PRENDERER hVideo
                                        , S_32 x, S_32 y
                                        , S_32 w, S_32 h );
   /* <combine sack::image::render::MakeTopmost@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, MakeTopmost )    ( PRENDERER hVideo );
   /* <combine sack::image::render::HideDisplay@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, HideDisplay )      ( PRENDERER hVideo );
   /* <combine sack::image::render::RestoreDisplay@PRENDERER>
      
      \ \                                                     */
   RENDER_PROC_PTR( void, RestoreDisplay )   ( PRENDERER hVideo );

	/* <combine sack::image::render::ForceDisplayFocus@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFocus )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayFront@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFront )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayBack@PRENDERER>
	   
	   \ \                                                       */
	RENDER_PROC_PTR( void, ForceDisplayBack )( PRENDERER display );

	/* <combine sack::image::render::BindEventToKey@PRENDERER@_32@_32@KeyTriggerHandler@PTRSZVAL>
	   
	   \ \                                                                                        */
	RENDER_PROC_PTR( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
	/* <combine sack::image::render::UnbindKey@PRENDERER@_32@_32>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier );
	/* <combine sack::image::render::IsTopmost@PRENDERER>
	   
	   \ \                                                */
	RENDER_PROC_PTR( int, IsTopmost )( PRENDERER hVideo );
	/* Used as a point to sync between applications and the message
	   display server; Makes sure that all draw commands which do
	   not have a response are done.
	   
	   
	   
	   Waits until all commands are processed; which is wait until
	   this command is processed.                                   */
	RENDER_PROC_PTR( void, OkaySyncRender )            ( void );
   /* <combine sack::image::render::IsTouchDisplay>
      
      \ \                                           */
   RENDER_PROC_PTR( int, IsTouchDisplay )( void );
	/* <combine sack::image::render::GetMouseState@S_32 *@S_32 *@_32 *>
	   
	   \ \                                                              */
	RENDER_PROC_PTR( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b );
	/* <combine sack::image::render::EnableSpriteMethod@PRENDERER@void__cdecl*RenderSpritesPTRSZVAL psv\, PRENDERER renderer\, S_32 x\, S_32 y\, _32 w\, _32 h@PTRSZVAL>
	   
	   \ \                                                                                                                                                               */
	RENDER_PROC_PTR ( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv );
	/* <combine sack::image::render::WinShell_AcceptDroppedFiles@PRENDERER@dropped_file_acceptor@PTRSZVAL>
	   
	   \ \                                                                                                 */
	RENDER_PROC_PTR( void, WinShell_AcceptDroppedFiles )( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser );
	/* <combine sack::image::render::PutDisplayIn@PRENDERER@PRENDERER>
	   
	   \ \                                                             */
	RENDER_PROC_PTR(void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer);
#ifdef WIN32
	/* <combine sack::image::render::MakeDisplayFrom@HWND>
	   
	   \ \                                                 */
			RENDER_PROC_PTR (PRENDERER, MakeDisplayFrom) (HWND hWnd) ;
#else
      POINTER junk4;
#endif
	/* <combine sack::image::render::SetRendererTitle@PRENDERER@TEXTCHAR *>
	   
	   \ \                                                                  */
	RENDER_PROC_PTR( void , SetRendererTitle) ( PRENDERER render, const TEXTCHAR *title );
	/* <combine sack::image::render::DisableMouseOnIdle@PRENDERER@LOGICAL>
	   
	   \ \                                                                 */
	RENDER_PROC_PTR (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable );
	/* <combine sack::image::render::OpenDisplayAboveUnderSizedAt@_32@_32@_32@S_32@S_32@PRENDERER@PRENDERER>
	   
	   \ \                                                                                                   */
	RENDER_PROC_PTR( PRENDERER, OpenDisplayAboveUnderSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under );
	/* <combine sack::image::render::SetDisplayNoMouse@PRENDERER@int>
	   
	   \ \                                                            */
	RENDER_PROC_PTR( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse );
	/* <combine sack::image::render::Redraw@PRENDERER>
	   
	   \ \                                             */
	RENDER_PROC_PTR( void, Redraw )( PRENDERER hVideo );
	/* <combine sack::image::render::MakeAbsoluteTopmost@PRENDERER>
	   
	   \ \                                                          */
	RENDER_PROC_PTR(void, MakeAbsoluteTopmost) (PRENDERER hVideo);
	/* <combine sack::image::render::SetDisplayFade@PRENDERER@int>
	   
	   \ \                                                         */
	RENDER_PROC_PTR( void, SetDisplayFade )( PRENDERER hVideo, int level );
	/* <combine sack::image::render::IsDisplayHidden@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( LOGICAL, IsDisplayHidden )( PRENDERER video );
#ifdef WIN32
	/* <combine sack::image::render::GetNativeHandle@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( HWND, GetNativeHandle )( PRENDERER video );
#endif
		 /* <combine sack::image::render::GetDisplaySizeEx@int@S_32 *@S_32 *@_32 *@_32 *>
		    
		    \ \                                                                           */
		 RENDER_PROC_PTR (void, GetDisplaySizeEx) ( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height);

	/* Locks a video display. Applications shouldn't be locking
	   this, but if for some reason they require it; use this
	   function.                                                */
	RENDER_PROC_PTR( void, LockRenderer )( PRENDERER render );
	/* Release renderer lock critical section. Applications
	   shouldn't be locking this surface.                   */
	RENDER_PROC_PTR( void, UnlockRenderer )( PRENDERER render );
	/* Provides a way for applications to cause the window to flush
	   to the display (if it's a transparent window)                */
	RENDER_PROC_PTR( void, IssueUpdateLayeredEx )( PRENDERER hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS );
	/* Check to see if the render mode is always redraw; changes how
	   smudge works in PSI. If always redrawn, then the redraw isn't
	   done during the smudge, and instead is delayed until a draw
	   is triggered at which time all controls are drawn.
	   
	   Returns
	   TRUE if full screen needs to be drawn during a draw,
	   otherwise partial updates may be done.                        */
	RENDER_PROC_PTR( LOGICAL, RequiresDrawAll )( void );
#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
       
       \ \                                                                             */
			RENDER_PROC_PTR( void, SetTouchHandler)      ( PRENDERER, TouchCallback, PTRSZVAL );
#endif
    RENDER_PROC_PTR( void, MarkDisplayUpdated )( PRENDERER );
    /* <combine sack::image::render::SetHideHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetHideHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRestoreHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetRestoreHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
		 RENDER_PROC_PTR( void, RestoreDisplayEx )   ( PRENDERER hVideo DBG_PASS );
		 /* added for android extensions; call to enable showing the keyboard in the correct thread
        ; may have applications for windows tablets 
		  */
       RENDER_PROC_PTR( void, SACK_Vidlib_ShowInputDevice )( void );
		 /* added for android extensions; call to enable hiding the keyboard in the correct thread
		  ; may have applications for windows tablets */
       RENDER_PROC_PTR( void, SACK_Vidlib_HideInputDevice )( void );

	/* Check to see if the render mode is allows updates from any thread.
	   If supported can simplify updates (requiring less scheduling queues).
	   If it is not supported (such as an X display where only a single thread
	   can write to the server, otherwise the socket state gets confused) then
	   Redraw() should be used to dispatch appriorately.  PSI Implements this 
	   internally, so smudge() on a control will behave appriopriately.
	   
	   If RequiresDrawAll() this is irrelavent.
	   
	   
	   
	   Returns
	   TRUE if any thread is allowed to generate UpdateDisplayPortion().
	   otherwise must call Redraw() on the surface to get a event in the 
	   correct thread.*/
	RENDER_PROC_PTR( LOGICAL, AllowsAnyThreadToUpdate )( void );
};

#ifdef DEFINE_DEFAULT_RENDER_INTERFACE
#define USE_RENDER_INTERFACE GetDisplayInterface()
#endif

#ifdef FORCE_NO_INTERFACE
#undef USE_RENDER_INTERFACE
#endif

#ifdef FORCE_NO_RENDER_INTERFACE
#undef USE_RENDER_INTERFACE
#endif

#if !defined(FORCE_NO_RENDER_INTERFACE)
/* RENDER_PROC( PRENDER_INTERFACE, GetDisplayInterface )( void );
   
   Gets the interface the proper way - by name.
   Returns
   Pointer to the render interface.                            */

#  define GetDisplayInterface() (PRENDER_INTERFACE)GetInterface( WIDE("render") )
/* RENDER_PROC( void, DropDisplayInterface )( PRENDER_INTERFACE interface );
   
   release the interface (could be cleanup, most are donothing....
   parameters
   interface   - Pointer to the render interface.                            */

#  define DropDisplayInterface(x) DropInterface( WIDE("render"), x )
#endif

#ifdef USE_RENDER_INTERFACE


typedef int check_this_variable;
// these methods are provided for backwards compatibility
// these should not be used - but rather use the interface defined below
// (the ones not prefixed by ActImage_ - except for ActImage_Init, which
// may(should) be called before any other function.
#define REND_PROC_ALIAS(name) ((USE_RENDER_INTERFACE)->_##name)
#define REND_PROC_ALIAS_VOID(name) if(USE_RENDER_INTERFACE)(USE_RENDER_INTERFACE)->_##name

#define SetApplicationTitle       REND_PROC_ALIAS(SetApplicationTitle)
#define SetRendererTitle       REND_PROC_ALIAS(SetRendererTitle)
#define SetApplicationIcon        REND_PROC_ALIAS(SetApplicationIcon)
#define GetDisplaySize            REND_PROC_ALIAS(GetDisplaySize)
#define GetDisplaySizeEx            REND_PROC_ALIAS(GetDisplaySizeEx)
#define MarkDisplayUpdated            REND_PROC_ALIAS(MarkDisplayUpdated)
#define SetDisplaySize            REND_PROC_ALIAS(SetDisplaySize)
#define GetDisplayPosition        REND_PROC_ALIAS(GetDisplayPosition)
#define IssueUpdateLayeredEx      REND_PROC_ALIAS(IssueUpdateLayeredEx)

#define MakeDisplayFrom        REND_PROC_ALIAS(MakeDisplayFrom)
#define OpenDisplaySizedAt        REND_PROC_ALIAS(OpenDisplaySizedAt)
#define OpenDisplayAboveSizedAt   REND_PROC_ALIAS(OpenDisplayAboveSizedAt)
#define OpenDisplayAboveUnderSizedAt   REND_PROC_ALIAS(OpenDisplayAboveUnderSizedAt)
#define CloseDisplay              REND_PROC_ALIAS(CloseDisplay)
#define UpdateDisplayPortionEx    REND_PROC_ALIAS(UpdateDisplayPortionEx)
#define UpdateDisplayEx             REND_PROC_ALIAS(UpdateDisplayEx)
#define SetMousePosition          REND_PROC_ALIAS(SetMousePosition)
#define GetMousePosition          REND_PROC_ALIAS(GetMousePosition)
#define GetMouseState          REND_PROC_ALIAS(GetMouseState)
#define EnableSpriteMethod          REND_PROC_ALIAS(EnableSpriteMethod)
#define WinShell_AcceptDroppedFiles REND_PROC_ALIAS(WinShell_AcceptDroppedFiles)
#define MoveDisplay               REND_PROC_ALIAS(MoveDisplay)
#define MoveDisplayRel            REND_PROC_ALIAS(MoveDisplayRel)
#define SizeDisplay               REND_PROC_ALIAS(SizeDisplay)
#define Redraw               REND_PROC_ALIAS(Redraw)
#define RequiresDrawAll()        (USE_RENDER_INTERFACE)?((USE_RENDER_INTERFACE)->_RequiresDrawAll()):0
#define SizeDisplayRel            REND_PROC_ALIAS(SizeDisplayRel)
#define MoveSizeDisplay        REND_PROC_ALIAS(MoveSizeDisplay)
#define MoveSizeDisplayRel        REND_PROC_ALIAS(MoveSizeDisplayRel)
#define PutDisplayAbove           REND_PROC_ALIAS(PutDisplayAbove)
#define PutDisplayIn           REND_PROC_ALIAS(PutDisplayIn)
#define GetDisplayImage           REND_PROC_ALIAS(GetDisplayImage)

#define SetCloseHandler           REND_PROC_ALIAS(SetCloseHandler)
#define SetMouseHandler           REND_PROC_ALIAS(SetMouseHandler)
#define SetHideHandler           REND_PROC_ALIAS(SetHideHandler)
#define SetRestoreHandler           REND_PROC_ALIAS(SetRestoreHandler)
#define AllowsAnyThreadToUpdate           REND_PROC_ALIAS(AllowsAnyThreadToUpdate)
#ifndef __LINUX__
#define SetTouchHandler           REND_PROC_ALIAS(SetTouchHandler)
#endif
#define SetRedrawHandler          REND_PROC_ALIAS(SetRedrawHandler)
#define SetKeyboardHandler        REND_PROC_ALIAS(SetKeyboardHandler)
#define SetLoseFocusHandler       REND_PROC_ALIAS(SetLoseFocusHandler)
#define SetDefaultHandler         REND_PROC_ALIAS(SetDefaultHandler)

#define GetKeyText                REND_PROC_ALIAS(GetKeyText)
#define HasFocus                  REND_PROC_ALIAS(HasFocus)

#define SACK_Vidlib_ShowInputDevice REND_PROC_ALIAS( SACK_Vidlib_ShowInputDevice )
#define SACK_Vidlib_HideInputDevice REND_PROC_ALIAS( SACK_Vidlib_HideInputDevice )

#define CreateMessage             REND_PROC_ALIAS(CreateMessage)
#define SendActiveMessage         REND_PROC_ALIAS(SendActiveMessage)
#define IsKeyDown                 REND_PROC_ALIAS(IsKeyDown)
#define KeyDown                   REND_PROC_ALIAS(KeyDown)
#define DisplayIsValid            REND_PROC_ALIAS(DisplayIsValid)
#define OwnMouseEx                REND_PROC_ALIAS(OwnMouseEx)
#define BeginCalibration          REND_PROC_ALIAS(BeginCalibration)
#define SyncRender                REND_PROC_ALIAS(SyncRender)
#define OkaySyncRender                REND_PROC_ALIAS(OkaySyncRender)


#define HideDisplay               REND_PROC_ALIAS(HideDisplay)
#define IsDisplayHidden               REND_PROC_ALIAS(IsDisplayHidden)
/* <combine sack::image::render::GetNativeHandle@PRENDERER>
   
   \ \                                                      */
#define GetNativeHandle             REND_PROC_ALIAS(GetNativeHandle)
//#define RestoreDisplay             REND_PROC_ALIAS(RestoreDisplay)
#define RestoreDisplayEx             REND_PROC_ALIAS(RestoreDisplayEx)
#define MakeTopmost               REND_PROC_ALIAS_VOID(MakeTopmost)
#define MakeAbsoluteTopmost               REND_PROC_ALIAS_VOID(MakeAbsoluteTopmost)
#define IsTopmost               REND_PROC_ALIAS(IsTopmost)
#define SetDisplayFade               REND_PROC_ALIAS(SetDisplayFade)

#define ForceDisplayFocus         REND_PROC_ALIAS(ForceDisplayFocus)
#define ForceDisplayFront       REND_PROC_ALIAS(ForceDisplayFront)
#define ForceDisplayBack          REND_PROC_ALIAS(ForceDisplayBack)
#define BindEventToKey          REND_PROC_ALIAS(BindEventToKey)
#define UnbindKey               REND_PROC_ALIAS(UnbindKey)
#define IsTouchDisplay          REND_PROC_ALIAS(IsTouchDisplay)
#define DisableMouseOnIdle      REND_PROC_ALIAS(DisableMouseOnIdle )
#define SetDisplayNoMouse      REND_PROC_ALIAS(SetDisplayNoMouse )
#define SetTouchHandler        REND_PROC_ALIAS(SetTouchHandler)
#endif

	_INTERFACE_NAMESPACE_END
#ifdef __cplusplus
#ifdef _D3D_DRIVER
	using namespace sack::image::render::d3d::Interface;
#elif defined( _D3D10_DRIVER )
	using namespace sack::image::render::d3d10::Interface;
#elif defined( _D3D11_DRIVER )
	using namespace sack::image::render::d3d11::Interface;
#else
	using namespace sack::image::render::Interface;
#endif
#endif
#endif

#ifndef __NO_MSGSVR__

#ifdef DEFINE_RENDER_PROTOCOL
#include <stddef.h>  // offsetof
// need to define BASE_RENDER_MESSAGE_ID before including this.
//#define MSG_ID(method)  ( ( offsetof( struct render_interface_tag, _##method ) / sizeof( void(*)(void) ) ) + BASE_RENDER_MESSAGE_ID + MSG_EventUser )
#define MSG_DisplayClientClose        MSG_ID(DisplayClientClose)
#define MSG_SetApplicationTitle       MSG_ID(SetApplicationTitle)
#define MSG_SetRendererTitle       MSG_ID(SetRendererTitle)
#define MSG_SetApplicationIcon        MSG_ID(SetApplicationTitle)
#define MSG_GetDisplaySize            MSG_ID(GetDisplaySize)
#define MSG_SetDisplaySize            MSG_ID(SetDisplaySize)
#define MSG_GetDisplayPosition        MSG_ID(GetDisplayPosition)
#define MSG_OpenDisplaySizedAt        MSG_ID(OpenDisplaySizedAt)
#define MSG_OpenDisplayAboveSizedAt   MSG_ID(OpenDisplayAboveSizedAt)
#define MSG_CloseDisplay              MSG_ID(CloseDisplay)
#define MSG_UpdateDisplayPortionEx    MSG_ID(UpdateDisplayPortionEx)
#define MSG_UpdateDisplay             MSG_ID(UpdateDisplayEx)
#define MSG_SetMousePosition          MSG_ID(SetMousePosition)
#define MSG_GetMousePosition          MSG_ID(GetMousePosition)
#define MSG_GetMouseState             MSG_ID(GetMouseState )
#define MSG_Redraw               MSG_ID(Redraw)

#define MSG_EnableSpriteMethod             MSG_ID(EnableSpriteMethod )
#define MSG_WinShell_AcceptDroppedFiles    MSG_ID(WinShell_AcceptDroppedFiles )
#define MSG_MoveDisplay               MSG_ID(MoveDisplay)
#define MSG_MoveDisplayRel            MSG_ID(MoveDisplayRel)
#define MSG_SizeDisplay               MSG_ID(SizeDisplay)
#define MSG_SizeDisplayRel            MSG_ID(SizeDisplayRel)
#define MSG_MoveSizeDisplay           MSG_ID(MoveSizeDisplay)
#define MSG_MoveSizeDisplayRel        MSG_ID(MoveSizeDisplayRel)
#define MSG_PutDisplayAbove           MSG_ID(PutDisplayAbove)
#define MSG_GetDisplayImage           MSG_ID(GetDisplayImage)
#define MSG_SetCloseHandler           MSG_ID(SetCloseHandler)
#define MSG_SetMouseHandler           MSG_ID(SetMouseHandler)
#define MSG_SetRedrawHandler          MSG_ID(SetRedrawHandler)
#define MSG_SetKeyboardHandler        MSG_ID(SetKeyboardHandler)
#define MSG_SetLoseFocusHandler       MSG_ID(SetLoseFocusHandler)
#define MSG_SetDefaultHandler         MSG_ID(SetDefaultHandler)
// -- all other handlers - client side only
#define MSG_HasFocus                  MSG_ID(HasFocus)
#define MSG_SendActiveMessage         MSG_ID(SendActiveMessage)
#define MSG_GetKeyText                MSG_ID(GetKeyText)
#define MSG_IsKeyDown                 MSG_ID(IsKeyDown)
#define MSG_KeyDown                   MSG_ID(KeyDown)
#define MSG_DisplayIsValid            MSG_ID(DisplayIsValid)
#define MSG_OwnMouseEx                 MSG_ID(OwnMouseEx)
#define MSG_BeginCalibration           MSG_ID(BeginCalibration)
#define MSG_SyncRender                 MSG_ID(SyncRender)
#define MSG_OkaySyncRender                 MSG_ID(OkaySyncRender)
#define MSG_HideDisplay               MSG_ID(HideDisplay)
#define MSG_IsDisplayHidden               MSG_ID(IsDisplayHidden)
#define MSG_RestoreDisplay             MSG_ID(RestoreDisplay)
#define MSG_MakeTopmost               MSG_ID(MakeTopmost)
#define MSG_BindEventToKey          MSG_ID(BindEventToKey)
#define MSG_UnbindKey               MSG_ID(UnbindKey)
#define MSG_IsTouchDisplay          MSG_ID(IsTouchDisplay )
#define MSG_GetNativeHandle             MSG_ID(GetNativeHandle)
#endif
#endif

RENDER_NAMESPACE_END
#ifdef __cplusplus
#ifdef _D3D_DRIVER
	using namespace sack::image::render::d3d;
#elif defined( _D3D10_DRIVER )
	using namespace sack::image::render::d3d10;
#elif defined( _D3D11_DRIVER )
	using namespace sack::image::render::d3d11;
#else
	using namespace sack::image::render;
#endif
#endif

#endif

// : $
// $Log: render.h,v $
// Revision 1.48  2005/05/25 16:50:09  d3x0r
// Synch with working repository.
//
// Revision 1.10  2003/03/25 08:38:11  panther
// Add logging
//
