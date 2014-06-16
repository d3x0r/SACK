#ifndef SOURCE_PSI2
#define SOURCE_PSI2
#endif

#ifndef __CONTROLS_DEFINED__
#define __CONTROLS_DEFINED__

//---------------------------------------------------------------
// PSI Version comments
//   1.1 )
//     - added PSI_VERSION so that after this required features
//       may be commented out...
//     - added fonts to common structure - controls and frames may define
//       frames an apply a scale factor to itself and or its children..
//
#include <stdhdrs.h>
#include <image.h>
#include <render.h>

#include <menuflags.h>
#include <fractions.h>

#include <psi/namespaces.h>

/* It's called System Abstraction Component Kit - that is almost
   what it is, self descriptively, but that's not what it
   entirely. It's more appropriately just a sack-o-goodies. A
   grab-bag of tidbits. The basic aggregation of group of
   objects in sack is bag. Bag contains everything that is not
   graphic.
   
   It abstracts loading external libraries (.dll and .so shared
   objects) so applications can be unbothered by platform
   details.
   
   It abstracts system features like threads into a consistent
   API.
   
   It abstracts system shared memory allocation and management,
   does have a custom allocation and release method, but
   currently falls back to using malloc and free.
   
   It abstracts access to system file system browsing; getting
   the contents of directories.
   
   It abstracts creating external processes, and the use of
   standard input-output methods to communicate with those
   tasks.
   
   It has a custom timer implementation using a thread and
   posting timer events as calls to user callbacks.
   
   It contains a variety of structures to manage data from
   lists, stack and queues to complex linked list of slab
   allocated structures, and management for text as words and
   phrases.
   
   It contains an event based networking system, using an
   external thread to coordinate network socket events.
   
   It contains methods to work with images.
   
   It contains methods to display images on a screen. This is
   also a system abstraction point, since to put a display out
   under Linux and Windows is entirely different.
   
   It contains a control system based on the above images and
   display, managing events to controls as user event callbacks.
   
   It contains a method of registering values, code, and even
   structures, and methods to browse and invoke registered code,
   create registered structures, and get back registered values.
   
   It contains a SQL abstraction that boils SQL communication to
   basically 2 methods, Commands and Queries.
   
   It provides application logging features, for debug, and
   crash diagnostics. This is basically a single command
   'lprintf'.
   
   It provides a vector and matrix library for 3D and even 4D
   graphics projects.
   
   
   
   Beyond that - it uses libpng, jpeg, and freetype for dealing
   with images. These are included in this one package for
   windows where they are not system libraries. Internal are up
   to date as of march 2010. Also includes sqlite. All of these
   \version compile as C++ replacing extern "C"{} with an
   appropriate namespace XXX {}.
   Example
   The current build system is CMake. Previously I had a bunch
   of makefiles, but then required gnu make to compile with open
   watcom, with cmake, the correct makefiles appropriate for
   each package is generated. Visual studio support is a little
   lacking in cmake. So installed output should be compatible
   with cmake find XXX.
   <code>
   PROJECT( new_project )
   ADD_EXECTUABLE( my_program my_source.c )
   LINK_TARGET_LIBRARIES( my_program ${SACK_LIBRARY} )
   </code>
   
   The problem with this... depends on the mode of sack being
   built against... maybe I should have a few families of
   compile-options and link libraries.
   Remarks
   If sack is built 'monolithic' then everything that is any
   sort of library is compiled together.
   
   If sack is not built monolithic, then it produces
     * \  bag &#45; everything that requires no system display,
       it is all the logic components for lists, SQL, processes,
       threads, networking
       * ODBC is currently linked here, so unixodbc or odbc32
         will be required as appropriate. Considering moving SQL
         entirely to a seperate module, but the core library can take
         advantage options, which may require SQL.
     * \  bag.external &#45; external libraries zlib, libpng,
       jpeg, freetype.
     * bag.sqlite3.external &#45; external sqlite library, with
       sqlite interface structure. This could be split to be just
       the sqlite interface, and link to a system sqlite library.
     * bag.image &#45; library that provides an image namespace
       interface.
     * bag.video &#45; library that provides a render namespace
       interface. Windows system only. Provides OpenGL support for
       displays also.
     * bag.display.image &#45; a client library that
       communicates to a remote image namespace interface.
     * bag.image.video &#45; a client library that communicates
       to a remote render namespace interface.
     * bag.display &#45; a host library that provides window
       management for render interface, which allows the application
       multliple windows. bag.display was built against SDL, and SDL
       only supplies a single display surface for an application, so
       popup dialogs and menus needed to be tracked internally.
       Bag.display can be loaded as a display service, and shared
       between multiple applications. SDL Can provide OpenGL support
       for render interface also. This can be mounted against
       bag.video also, for developing window support. This has
       fallen by the wayside... and really a display interface
       should be provided that can just open X displays directly to
       copy <link sack::image::Image, Images> to.
     * bag.psi &#45; provides the PSI namespace.                      */
SACK_NAMESPACE


/* PSI is Panther's Slick Interface.
   
   This is a control library which handles custom controls and
   regions within a form. This isn't a window manager.
   
   
   
   PSI_CONTROL is the primary structure that this uses. It
   esists as a pointer to a structure, the content of which
   should never be accessed by the real world. For purposes of
   documentation these structures (in controlstruc.h) are
   presented, but they are inaccessable from outside of SACK
   itself, and will not be provided in the SDK.
   
   
   
   A PSI_CONTROL can represent a 'Frame' which contains other
   controls. A Frame owns a PRENDERER which it uses to present
   its content to the display. A frame can have an outside
   border, and provides the ability to click on the border and
   resize the form. The frame can use a custom image, which will
   be automatically portioned up and used as corner pieces, edge
   pieces, and may automatically set the default background
   color for forms. The frame draws directly on the Image from
   the PRENDERER, and all controls know only their image.
   Controls are based on sub-images on the frame's displayed
   image, so when they draw they are drawing directly on the
   buffer targeted to the display. The image library prohibits
   any operations that go outside the bounds of a sub-image;
   though, the user can request the color buffer pointer from an
   image, and may draw directly anywhere on that surface.
   
   
   
   Controls are implemented as a pair of sub-images on the color
   buffer of the renderer. The pair is the image containing the
   border of the control, and a sub-image inside that
   representing the drawable area of a control. When created,
   controls are always sized including their frame, this allow
   positioning controls inside a frame without regard to how
   much extra space might have been added. The size of the Frame
   control outside, is handled differently, and is created with
   the size of the inside of the control. The area of the render
   surface is expanded outside this. There are BorderOptionTypes
   that can control whether the border on the frame is handled
   as an expansion or not.
   
   
   
   Controls are all drawn using an internal table of colors
   which can be index using ControlColorTypes with SetBaseColor
   and GetBaseColor.
   
   
   
   If the fancy border frame image is used for a control, then
   the color of the center pixel is set into the NORMAL color
   index.
   
   
   
   What else can I say about controls...
   
   
   
   Any control can contain any other control, but there is a
   specific container type Frame that is commonly used for
   dialogs. The top level control can display on a renderer. If
   a sub-control is told to show, then it is divorced from and
   opened in a new popup renderer.
   History
   Control registration was done original by filling in a
   CONTROL_REGISTRATION structure, and passing that structure to
   PSI to register. This method is still available, and is
   certainly more straight forward method of use, but it's not
   the easier method or the current method.
   
   Currently, a registration structure is still used, but only
   the first few elements are actually filled out, the functions
   to handle a control's events are declared by fancy macros.
   
   This relies on the deadstart features of compiler working,
   but there are fewer places to have to remember to make
   changes, and controls are much more straight forward to
   implement, and extend as required, without necessarily
   requiring everything to be done all at once. Methods
   registered this way MUST be static, otherwise compiler errors
   will result; they MUST have the correct return type for the
   method specified, and they must have the correct parameters.
   \Examples of each method supported will be provided with each
   method's documentation. These are nasty macros that insert a
   bit of magic code between the 'static int' and the parameters
   specified. The names of the parameter values to the callback
   are up to the user, but the types must match. This is a very
   exact method, that cannot be circumvented using bad
   typecasting.
   Example
   This is a custom control that shows red or green.
   <code lang="c++">
   struct control_data
   {
       // declare some data that you want to have associated with your control.
       CDATA color;
       _32 last_buttons; // used to track when a click happens
   };
   
   EasyRegisterControl( WIDE( "simple" ), sizeof( struct ball_timer_data ) );
   
   // define the function called when a new 'simple' control is created.
   static int OnCreateCommon( WIDE( "simple" ) )( PSI_CONTROL pc )
   {
       MyValidatedControlData( struct control_data*, my_data, pc );
       if( my_data )
       {
           // assign a color to start
           my_data-\>color = BASE_COLOR_RED;
           return 1;
       }
       return 0;
   }
   
   // define a method to draw the control
   static int OnDrawCommon( WIDE( "simple" ) )( PSI_CONTROL pc )
   {
       MyValidatedControlData( struct control_data*, my_data, pc );
       if( my_data )
       {
           Image image = GetControlSufrace( pc );
           BlatColor( image, my_data-\>color );
           return 1;  // return that the draw happened.
       }
       return 0;  // return no draw - prevents update.
   }
   
   // define a handler when the simple control is clicked.
   static int OnMouseCommon( WIDE( "simple" ) )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
   {
   </code>
   <code>
       MyValidatedControlData( struct control_data*, my_data, pc );
       // this checks to see if any mouse button goes down
       if( MAKE_NEWBUTTON( b, my_data-\>last_buttons ) )
       {
           if( my_data-\>color == BASE_COLOR_RED )
               my_data-\>color = BASE_COLOR_GREEN;
           else
               my_data-\>color = BASE_COLOR_RED;
           // tell the control to update.
           SmudgeCommon( pc );
       }
       // save this button state as the prior button state for future checks
       my_data-\>last_buttons = b;
   </code>
   <code lang="c++">
   }
   </code>
   See Also
   OnCreateCommon
   
   OnDrawCommon
   
   OnMouseCommon
   
   OnKeyCommon
   
   OnCommonFocus
   
   OnDestroyCommon
   
   
   
   \-- Less common
   
   OnMoveCommon
   
   OnSizeCommon
   
   OnMotionCommon
   
   OnHideCommon
   
   OnRevealCommon
   
   OnPropertyEdit
   
   OnPropertyEditOkay
   
   OnPropertyEditCancel
   
   OnPropertyEditDone
   
   
   
   \--- Much Less used
   
   OnEditFrame                                                                           */
_PSI_NAMESPACE


// this was never implemented.
#define MK_PSI_VERSION(ma,mi)  (((ma)<<8)|(mi))
// this was never implemented.
#define PSI_VERSION            MK_PSI_VERSION(1,1)
// this was never implemented.
#define REQUIRE_PSI(ma,mi)    ( PSI_VERSION >= MK_PSI_VERSION(ma,mi) )


#ifdef PSI_SOURCE
#define PSI_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PSI_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

// Control callback functions NEED to be declared in the same source as what
// created the control/frame...
#ifdef SOURCE_PSI2
#define CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( int, Config##name)( PSI_CONTROL );  \
	/*PSI_PROC( PSI_CONTROL, Make##name )( PSI_CONTROL pFrame, int attr */ \
	/*, int x, int y, int w, int h*/ \
	/*, PTRSZVAL nID, ... ); */
#define CAPTIONED_CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( int, Config##name)( PSI_CONTROL );  \
	/*PSI_PROC( PSI_CONTROL, Make##name )( PSI_CONTROL pFrame, int attr */\
	/*, int x, int y, int w, int h  */\
	/*, PTRSZVAL nID, CTEXTSTR caption, ... ); */
#else
#define CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( PSI_CONTROL, Config##name)( PSI_CONTROL );  \
	/*PSI_PROC( PSI_CONTROL, Make##name )( PSI_CONTROL pFrame, int attr*/ \
	/*, int x, int y, int w, int h*/ \
	/*, PTRSZVAL nID, ... );*/
#define CAPTIONED_CONTROL_PROC( name,_args )  \
	PSI_PROC( PSI_CONTROL, Get##name##PropertyPage )( PSI_CONTROL pc ); \
	PSI_PROC( void, Apply##name##PropertyPage )( PSI_CONTROL pc, PSI_CONTROL page ); \
   PSI_PROC( PSI_CONTROL, Config##name)( PSI_CONTROL );  \
	/*PSI_PROC( PSI_CONTROL, Make##name )( PSI_CONTROL pFrame, int attr */\
	/*, int x, int y, int w, int h */\
	/*, PTRSZVAL nID, ... );*/
#endif

enum {
    COMMON_PROC_ANY
     , COMMON_PROC_DRAW
     , COMMON_PROC_MOUSE
     , COMMON_PROC_KEY
};

#define RegisterControl(name)  do { extern CTEXTSTR  ControlMakeProcName_##name, ControlLoadProcName_##name; \
    RegisterControlProcName( ControlMakeProcName_##name \
    , (POINTER)Make##name        \
    , ControlLoadProcName_##name \
    , (POINTER)Load##name );     \
} while(0)

#define RegisterFrame(name)       RegisterControlProcName( FrameProcName_##name \
    , (POINTER)Make##name  \
    , (POINTER)Load##name )

//PSI_PROC( int, RegisterControlProcName )( CTEXTSTR name, POINTER MakeProc, POINTER LoadProc );


#ifndef CONTROL_SOURCE
#define MKPFRAME(hVid) (((PTRSZVAL)(hVid))|1)
//typedef POINTER PSI_CONTROL;
// any remaining code should reference PSI_CONTROL
#define PCOMMON PSI_CONTROL
#define PCONTROL PSI_CONTROL
#endif
/* <combine sack::PSI>
   
   A handle to a PSI control or frame. The User's data is stored
   as the first member of this structure, so de-referencing the
   pointer twice gets the user data. All PSI functions work
   against PSI_CONTROL. Once upon a time this was just PSI_CONTROL,
   and so the methods for registering to handle events on the
   control still reference 'Common'.
   Remarks
   <code lang="c++">
   PSI_CONTROL control;
   POINTER user_data;
   user_data = *(POINTER*)control;
   </code>                                                       */
typedef struct common_control_frame *PSI_CONTROL;

#define COMMON_BUTTON_WIDTH 55
#define COMMON_BUTTON_HEIGHT 19
#define COMMON_BUTTON_PAD 5

/* This enumeration defines flags that can be used to specify
   the border of controls.                                    */
enum BorderOptionTypes {
 BORDER_NORMAL        =     0, // normal case needs to be 0 - but this is the thickest - go figure.
 BORDER_NONE          =     3, /* The control has no border - this overrides all other styles;
    no caption, no border at all, the surface drawable area is
    the same as the control's outer area.                        */
 
 BORDER_THIN          =     1, /* This is a 3 pixel frame line that indicates a stack up. */
 
 BORDER_THINNER       =     2, /* a thin line, that is a raised edge. */
 
 BORDER_DENT          =     4, /* A dent is a 3 pixel line which is a thin etch-line that is 1
    step in, 1 across and 1 step up.                             */
 
 BORDER_THIN_DENT     =     5, /* a thin dent border is an etch line */
 
 BORDER_THICK_DENT    =     6, /* A thick etch line - that is a step in, and a step out, so the
    content is the same level as its parent.                      */
 BORDER_USER_PROC     =     7, /* External user code is used to draw the border when required
    use PSI_SetCustomBorder to set this border type...
    or define an OnBorderDraw and OnBorderMeasure routine for custom controls */
 BORDER_TYPE          =  0x0f, // 16 different frame types standard...

 BORDER_INVERT        =  0x80, /* This modifies styles like BORDER_BUMP to make them
    BORDER_DENT.                                       */
 
 BORDER_CAPTION       =  0x40, /* Border should include a space to show the text of the control
    as a caption.                                                 */
 
 BORDER_NOCAPTION     =  0x20, /* Make sure that the border does NOT have a caption space. */
 
 BORDER_INVERT_THINNER=  (BORDER_THINNER|BORDER_INVERT), /* A even thinner line which is an indent. */
 
 BORDER_INVERT_THIN   =  (BORDER_THIN|BORDER_INVERT), /* It's a thin frame (3 pixels?) which is a descent step frame
    ... so instead of being stacked 'up' it's stacked 'down'.   */
 
 BORDER_BUMP          =  (BORDER_DENT|BORDER_INVERT), /* Draws a 3 pixel frame around a control - it is 1 up 1 acros
    and 1 down - a thin bump line.                              */
 

 BORDER_NOMOVE        =  0x0100, /* This indicates that the frame bordered by this does not move. */
 BORDER_CLOSE         =  0x0200, // well okay maybe these are border traits
 BORDER_RESIZABLE     =  0x0400, // can resize this window with a mouse
 BORDER_WITHIN        =  0x0800, // frame is on the surface of parent...

 BORDER_WANTMOUSE     =  0x1000, // frame surface desires any unclaimed mouse calls
 BORDER_EXCLUSIVE     =  0x2000, // frame wants exclusive application input.

 BORDER_FRAME         =  0x4000, // marks controls which were done with 'create frame', and without BORDER_WITHIN
 BORDER_FIXED         =  0x8000, // scale does not apply to coordinates... otherwise it will be... by default controls are scalable.

 BORDER_NO_EXTRA_INIT =        0x010000, // control is private to psi library(used for scrollbars in listboxes, etc) and as such does not call 'extra init'
};

// these are the indexes for base color
enum ControlColorTypes {
 HIGHLIGHT           = 0,
 NORMAL              = 1,
 SHADE               = 2,
 SHADOW              = 3,
 TEXTCOLOR           = 4,
 CAPTION             = 5,
 CAPTIONTEXTCOLOR    = 6,
 INACTIVECAPTION     = 7,
 INACTIVECAPTIONTEXTCOLOR = 8,
 SELECT_BACK         = 9,
 SELECT_TEXT         = 10,
 EDIT_BACKGROUND     = 11,
 EDIT_TEXT           = 12,
 SCROLLBAR_BACK      = 13
};
// these IDs are used to designate default control IDs for
// buttons...
#define TXT_STATIC -1
#ifndef IDOK
#define IDOK BTN_OKAY
#endif
#ifndef IDCANCEL
#define IDCANCEL BTN_CANCEL
#endif
#ifdef __LINUX__
#ifndef PSI_SOURCE
#define BTN_OKAY   1
#define BTN_CANCEL 2
#endif
#endif

#ifdef __cplusplus
namespace old_constants {
#endif
// enumeration for control->nType                    
//enum {
#define	CONTROL_FRAME  0// master level control framing...
#define	CONTROL_FRAME_NAME  WIDE("Frame")// master level control framing...
#define  UNDEFINED_CONTROL  1// returns a default control to user - type 1
#define  UNDEFINED_CONTROL_NAME  WIDE("Undefined")// returns a default control to user - type 1
#define  CONTROL_SUB_FRAME 2
#define  CONTROL_SUB_FRAME_NAME WIDE("SubFrame")
#define  STATIC_TEXT 3
#define  STATIC_TEXT_NAME WIDE("TextControl")
#define  NORMAL_BUTTON 4
#define  NORMAL_BUTTON_NAME WIDE("Button")
#define  CUSTOM_BUTTON 5
#define  CUSTOM_BUTTON_NAME WIDE("CustomDrawnButton")
#define  IMAGE_BUTTON 6
#define  IMAGE_BUTTON_NAME WIDE("ImageButton")
#define  RADIO_BUTTON 7// also subtype radio button
#define  RADIO_BUTTON_NAME WIDE("CheckButton")// also subtype radio button
#define  EDIT_FIELD 8
#define  EDIT_FIELD_NAME WIDE("EditControl")
#define  SLIDER_CONTROL 9
#define  SLIDER_CONTROL_NAME WIDE("Slider")
#define  LISTBOX_CONTROL 10
#define  LISTBOX_CONTROL_NAME WIDE("ListBox")
#define  SCROLLBAR_CONTROL 11
#define  SCROLLBAR_CONTROL_NAME WIDE("ScrollBar")
#define  GRIDBOX_CONTROL  12 // TBI (to be implemented)
#define  GRIDBOX_CONTROL_NAME  WIDE("Gridbox") // TBI (to be implemented)
#define  CONSOLE_CONTROL  13 // TBI (to be implemented)
#define  CONSOLE_CONTROL_NAME  WIDE("Console") // TBI (to be implemented)
#define  SHEET_CONTROL    14
#define  SHEET_CONTROL_NAME    WIDE("SheetControl")
#define  COMBOBOX_CONTROL 15
#define  COMBOBOX_CONTROL_NAME WIDE("Combo Box")

#define  BUILTIN_CONTROL_COUNT 16 // last known builtin control...
#define  USER_CONTROL   128 // should be sufficiently high as to not conflict with common controls
#ifdef __cplusplus
}
#endif
//};

_MENU_NAMESPACE
/* This is an item on a menu. (AppendMenuItem can return this I
   think)                                                       */
typedef struct menuitem_tag *PMENUITEM;
/* This is a popup menu or sub-menu. */
typedef struct menu_tag *PMENU;

#ifndef MENU_DRIVER_SOURCE
/* <combine sack::PSI::popup::draw_popup_item_tag>
   
   \ \                                             */
typedef struct draw_popup_item_tag  DRAWPOPUPITEM;
/* <combine sack::PSI::popup::draw_popup_item_tag>
   
   \ \                                             */
typedef struct draw_popup_item_tag *PDRAWPOPUPITEM;
/* This is used when a custom drawn menu item is used. Allows
   user code to draw onto the menu.                           */
struct draw_popup_item_tag 
{
   // ID param of append menu item
    PTRSZVAL psvUser; 
    /* Optional states an item might be in. */
    /* <combine sack::containers::text::format_info_tag::flags@1>
       
       \ \                                                        */
    struct {
        /* Menu item is in a selected state. (Mouse Over) */
        BIT_FIELD selected : 1;
        /* Menu item has a checkmark on it. */
        BIT_FIELD checked  : 1;
    } flags;
    /* Define options which may be passed to measure an item or to
       have an item drawn.                                         */
    union {
        /* Information which should be filled in when measuring popup
           items.                                                     */
        /* <combine sack::PSI::popup::draw_popup_item_tag::union@1::measure@1>
           
           \ \                                                                 */
        struct {
            /* Height of the menu item. */
            /* Width of the menu item. */
            _32 width, height;
        } measure;
        /* Contains information passed when the draw is required. */
        /* <combine sack::PSI::popup::draw_popup_item_tag::union@1::draw@1>
           
           \ \                                                              */
        struct {
            /* x to draw into */
            /* y coordinate to start drawing at. */
            S_32 x, y;
            /* Width to draw. */
            /* Height to draw. */
            _32 width, height;
            /* Image to draw into. */
            Image image;
        } draw;
    };
};

#endif
_MENU_NAMESPACE_END
USE_MENU_NAMESPACE

//-------- Initialize colors to current windows colors -----------
PSI_PROC( PRENDER_INTERFACE, SetControlInterface)( PRENDER_INTERFACE DisplayInterface );
PSI_PROC( PIMAGE_INTERFACE, SetControlImageInterface )( PIMAGE_INTERFACE DisplayInterface );

PSI_PROC( void, AlignBaseToWindows)( void );

PSI_PROC( void, SetBaseColor )( INDEX idx, CDATA c );
PSI_PROC( CDATA, GetBaseColor )( INDEX idx );
PSI_PROC( void, SetControlColor )( PSI_CONTROL pc, INDEX idx, CDATA c );
PSI_PROC( CDATA, GetControlColor )( PSI_CONTROL pc, INDEX idx );

//-------- Frame and generic control functions --------------

#ifdef CONTROL_SOURCE
#define MKPFRAME(hvideo) ((PSI_CONTROL)(((PTRSZVAL)(hvideo))|1))
#endif

/* Update the border type of a control.  See BorderOptionTypes
   Parameters
   pc :      control to modify the border
   BorderType :  new border style
 */
PSI_PROC( void, SetCommonBorderEx )( PSI_CONTROL pc, _32 BorderType DBG_PASS);
/* Update the border type of a control.  See BorderOptionTypes
   Parameters
   pc :      control to modify the border
   b :  new border style
 */
#define SetCommonBorder(pc,b) SetCommonBorderEx(pc,b DBG_SRC)

/* Update the border type of a control.  Border is drawn by routine
   Parameters
	pc :      control to modify the border draw routine
	proc:     draw routine; image parameter is the 'window' in which the surface is...
	measure_proc : get how much the outside border should be offset (or inside image should be inset)
 */
PSI_PROC( void, PSI_SetCustomBorder )( PSI_CONTROL pc, void (CPROC*proc)(PSI_CONTROL,Image)
                                      , void (CPROC*measure_proc)( PSI_CONTROL, int *x_offset, int *y_offset, int *right_inset, int *bottom_inset )
												 );

PSI_PROC( void, SetDrawBorder )( PSI_CONTROL pc ); // update to current border type drawing.
PSI_PROC( PSI_CONTROL, CreateFrame)( CTEXTSTR caption, int x, int y
										, int w, int h
										, _32 BorderFlags
											  , PSI_CONTROL hAbove );

// 1) causes all updates to be done in video thread, otherwise selecting opengl context fails.
// 2) ...
PSI_PROC( void, EnableControlOpenGL )( PSI_CONTROL pc );
//PSI_PROC( void, SetFrameDraw )( PSI_CONTROL pc, void (CPROC*OwnerDraw)(PSI_CONTROL pc) );
//PSI_PROC( void, SetFrameMouse )( PSI_CONTROL pc, void (CPROC*OwnerMouse)(PSI_CONTROL pc, S_32 x, S_32 y, _32 b) );


// Control Init Proc is called each time a control is created
// a control may be created either with a 'make' routine
// or by loading a dialog resource.
#ifdef SOURCE_PSI2
typedef int (CPROC*ControlInitProc)( PSI_CONTROL, va_list );
#else
typedef int (CPROC*ControlInitProc)( PTRSZVAL, PSI_CONTROL, _32 ID );
#endif
/* \Internal event callback definition. After creation, an
   initializer is available to call on controls to pass
   \arguments to. This is more useful for loading from an XML
   \file where the control may have specified extra data.     */
typedef int (CPROC*FrameInitProc)( PTRSZVAL, PSI_CONTROL, _32 ID );
PSI_PROC( void, SetFrameInit )( PSI_CONTROL, ControlInitProc, PTRSZVAL );
PSI_PROC( CTEXTSTR, GetControlTypeName)( PSI_CONTROL pc );
// internal routine now exposed... results in a frame from a given
// renderer - a more stable solution than MKPFRAME which
// would require MUCH work to implement all checks everywhere...
PSI_PROC( PSI_CONTROL, CreateFrameFromRenderer )( CTEXTSTR caption
                                                         , _32 BorderTypeFlags
														 , PRENDERER pActImg );
/* Attach a frame to a renderer. Not sure which is sized to
   which if they are not the same size... probably the control
   is sized to the display.
   Parameters
   pcf :      control to attach to the display frame
   pActImg :  the display surface to use to show the control.
   
   Returns
   the control which was being attached to a display surface.  */
PSI_PROC( PSI_CONTROL, AttachFrameToRenderer )( PSI_CONTROL pcf, PRENDERER pActImg );
// any control on a frame may be passed, and
// the top level
PSI_PROC( PRENDERER, GetFrameRenderer )( PSI_CONTROL );
PSI_PROC( PSI_CONTROL, GetFrameFromRenderer )( PRENDERER renderer );
PSI_PROC( void, GetPhysicalCoordinate )( PSI_CONTROL relative_to, S_32 *_x, S_32 *_y, int include_surface );


//PSI_PROC( void, DestroyFrameEx)( PSI_CONTROL pf DBG_PASS );
#ifdef SOURCE_PSI2
#define DestroyFrame(pf) DestroyCommonEx( pf DBG_SRC )
#else
#define DestroyFrame(pf) DestroyControlEx( pf DBG_SRC )
#endif
PSI_PROC( int, SaveFrame )( PSI_CONTROL pFrame, CTEXTSTR file );
/* This is actually a load/save namespace. These functions are
   used to save and load frames and their layouts to and from
   XML.                                                        */
_XML_NAMESPACE

/* Unused. Please Delete. */
PSI_PROC( void, SetFrameInitProc )( PSI_CONTROL pFrame, ControlInitProc InitProc, PTRSZVAL psvInit );
/* Saves the current layout and controls of a frame. Can be
   recreated later with LoadXMLFrame.
   Parameters
   frame :  Frame to save with all of its contents.
   file\ :  filename to save XML frame into.                */
PSI_PROC( int, SaveXMLFrame )( PSI_CONTROL frame, CTEXTSTR file );

/* results with the frame and all controls created
   whatever extra init needs to be done... needs to be done
   if parent, use DisplayFrameOver().
                                                          
                                                          If frame is specified in parameters, and is not NULL, then
                                                          this window is stacked against the other one so it is always
                                                          above the other window.
                                                          Parameters
                                                          file\ :     name of XML file to read and pass to ParseXMLFrame
                                                          frame :     frame to stack this frame against. (specify parent
                                                                      window.)
                                                          DBG_PASS :  passed to track allocation responsiblity.          */
	PSI_PROC( PSI_CONTROL, LoadXMLFrameEx )( CTEXTSTR file DBG_PASS );
	/* Handles recreating a frame from an XML description.
   
   
   Parameters
   buffer :    buffer to parse with XML frame loader
   size :      length of the buffer in bytes.
   DBG_PASS :  passed to track allocation responsiblity. */
PSI_PROC( PSI_CONTROL, ParseXMLFrameEx )( POINTER buffer, size_t size DBG_PASS );
/* <combine sack::PSI::xml::LoadXMLFrameEx@CTEXTSTR file>
   
   \ \                                                    */
PSI_PROC( PSI_CONTROL, LoadXMLFrameOverExx )( PSI_CONTROL frame, CTEXTSTR file, LOGICAL create DBG_PASS );
/* <combine sack::PSI::xml::LoadXMLFrameEx@CTEXTSTR file>
   
   \ \                                                    */
PSI_PROC( PSI_CONTROL, LoadXMLFrameOverEx )( PSI_CONTROL frame, CTEXTSTR file DBG_PASS );
/* <combine sack::PSI::xml::LoadXMLFrameOverEx@PSI_CONTROL@CTEXTSTR file>
   
   \ \                                                                    */
#define LoadXMLFrameOverOption(parent,file,create) LoadXMLFrameOverExx( parent,file,create DBG_SRC )
/* <combine sack::PSI::xml::LoadXMLFrameOverEx@PSI_CONTROL@CTEXTSTR file>
   
   \ \                                                                    */
#define LoadXMLFrameOver(parent,file) LoadXMLFrameOverEx( parent,file DBG_SRC )
/* <combine sack::PSI::xml::LoadXMLFrameEx@CTEXTSTR file>
   
   \ \                                                    */
#define LoadXMLFrame(file) LoadXMLFrameEx( file DBG_SRC )
/* <combine sack::PSI::xml::ParseXMLFrameEx@POINTER@_32 size>
   
   \ \                                                        */
#define ParseXMLFrame(p,s) ParseXMLFrameEx( (p),(s) DBG_SRC )
_XML_NAMESPACE_END
USE_XML_NAMESPACE


PSI_PROC( PSI_CONTROL, LoadFrameFromMemory )( POINTER info, _32 size, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv  );
PSI_PROC( PSI_CONTROL, LoadFrameFromFile )( FILE *in, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv  );
PSI_PROC( PSI_CONTROL, LoadFrame )( CTEXTSTR file, PSI_CONTROL hAbove, FrameInitProc InitProc, PTRSZVAL psv );
/* methods to edit frames at runtime. */
_PROP_NAMESPACE
/* Turns edit features of a frame on and off.
   Parameters
   pf :       frame to set the edit state of
   bEnable :  if TRUE, enable edit. if FALSE, disable edit.
   
   Example
   <code lang="c#">
   PSI_CONTROL frame = CreateFrame( "test", 0, 0, 100, 100, BORDER_NORMAL, NULL );
   EditFrame( frame );
   </code>
   This turns on edit features of a frame, right click you can
   add a new registered control, controls have hotspots on them,
   if you right click on a hotspot, then you can edit properties
   of that control like it's control ID.
   
   
   
   
   Note
   When LoadXMLFrame fails to find the file, a frame is created
   and edit enabled like this.                                                     */
PSI_PROC( void, EditFrame )( PSI_CONTROL pf, int bEnable );
_PROP_NAMESPACE_END
PSI_PROC( void, GetFramePosition )( PSI_CONTROL pf, S_32 *x, S_32 *y );
PSI_PROC( void, GetFrameSize )( PSI_CONTROL pf, _32 *w, _32 *h );

// results in the total width (left and right) of the frame
PSI_PROC( int, FrameBorderX )( PSI_CONTROL pc, _32 BorderFlags );
// results in left offset of the surface within the frame...
PSI_PROC( int, FrameBorderXOfs )( PSI_CONTROL pc, _32 BorderFlags );
// results in the total height (top and bottom) of frame (and caption)
PSI_PROC( int, FrameBorderY )( PSI_CONTROL pc, _32 BorderFlags, CTEXTSTR caption );
// results in top offset of the surface within the frame...
PSI_PROC( int, FrameBorderYOfs )( PSI_CONTROL pc, _32 BorderFlags, CTEXTSTR caption );

PSI_PROC( int, CaptionHeight )( PSI_CONTROL pf, CTEXTSTR text );


PSI_PROC( void, DisplayFrameOverOn )( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg );
// stacks the physical display behind this other frame... 
PSI_PROC( void, DisplayFrameUnder )( PSI_CONTROL pc, PSI_CONTROL under );

PSI_PROC( void, DisplayFrameOver )( PSI_CONTROL pc, PSI_CONTROL over );
PSI_PROC( void, DisplayFrameOn )( PSI_CONTROL pc, PRENDERER pActImg );
PSI_PROC( void, DisplayFrame)( PSI_CONTROL pf );
PSI_PROC( void, HideFrame )( PSI_CONTROL pf );
PSI_PROC( void, HideCommon )( PSI_CONTROL pf );
PSI_PROC( LOGICAL, IsControlHidden )( PSI_CONTROL pc );

PSI_PROC( void, RevealCommonEx )( PSI_CONTROL pf DBG_PASS );
#define RevealCommon(pc) RevealCommonEx(pc DBG_SRC );

PSI_PROC( void, SizeCommon)( PSI_CONTROL pf, _32 w, _32 h );
#define SizeControl(c,x,y) SizeCommon((PSI_CONTROL)c,x,y)
#define SizeFrame(c,x,y) SizeCommon((PSI_CONTROL)c,x,y)
PSI_PROC( void, SizeCommonRel)( PSI_CONTROL pf, _32 w, _32 h );
#define SizeControlRel(c,x,y) SizeCommonRel((PSI_CONTROL)c,x,y)
#define SizeFrameRel(c,x,y) SizeCommonRel((PSI_CONTROL)c,x,y)
PSI_PROC( void, MoveCommon)( PSI_CONTROL pf, S_32 x, S_32 y );
#define MoveControl(c,x,y) MoveCommon((PSI_CONTROL)c,x,y)
#define MoveFrame(c,x,y) MoveCommon((PSI_CONTROL)c,x,y)
PSI_PROC( void, MoveCommonRel)( PSI_CONTROL pf, S_32 x, S_32 y );
#define MoveControlRel(c,x,y) MoveCommonRel((PSI_CONTROL)c,x,y)
#define MoveFrameRel(c,x,y) MoveCommonRel((PSI_CONTROL)c,x,y)
PSI_PROC( void, MoveSizeCommon)( PSI_CONTROL pf, S_32 x, S_32 y, _32 width, _32 height );
#define MoveSizeControl(c,x,y,w,h) MoveSizeCommon((PSI_CONTROL)c,x,y,w,h)
#define MoveSizeFrame(c,x,y,w,h) MoveSizeCommon((PSI_CONTROL)c,x,y,w,h)
PSI_PROC( void, MoveSizeCommonRel)( PSI_CONTROL pf, S_32 x, S_32 y, _32 width, _32 height );
#define MoveSizeControlRel(c,x,y,w,h) MoveSizeCommonRel((PSI_CONTROL)c,x,y,w,h)
#define MoveSizeFrameRel(c,x,y,w,h) MoveSizeCommonRel((PSI_CONTROL)c,x,y,w,h)


PSI_PROC( PSI_CONTROL, GetControl )( PSI_CONTROL pContainer, int ID );
#ifdef PSI_SOURCE
//#define GetControl(pc,id) GetControl( &((pc)->common),id)
#define GetControl(pc,id) GetControl( (PSI_CONTROL)(pc),id)
#endif
//PSI_PROC( PSI_CONTROL, GetControl)( PSI_CONTROL pf, int ID );
PSI_PROC( PTRSZVAL, GetCommonUserData )( PSI_CONTROL pf );
#define GetFrameUserData(pf) GetCommonUserData( (PSI_CONTROL)pf )
PSI_PROC( void, SetCommonUserData )( PSI_CONTROL pf, PTRSZVAL psv );
#define SetFrameUserData(pf,d) SetCommonUserData( (PSI_CONTROL)pf,d )


PSI_PROC( void, BeginUpdate )( PSI_CONTROL pc );
PSI_PROC( void, EndUpdate )( PSI_CONTROL pc );


// do the draw to the display...
PSI_PROC( void, UpdateFrameEx )( PSI_CONTROL pf
                                      , int x, int y
										 , int w, int h DBG_PASS );
#define UpdateFrame(pf,x,y,w,h) UpdateFrameEx(pf,x,y,w,h DBG_SRC )

/* \INTERNAL
   
   This is for internal organization, events and routines the
   mouse uses and features it adds to the PSI control... like
   issuing auto updates on unlock... well... it's internal
   anyhow                                                     */
_MOUSE_NAMESPACE
/* Releases a use. This is the oppsite of AddWait(). */
PSI_PROC( void, ReleaseCommonUse )( PSI_CONTROL pc );
/* This one is public, Sets the mouse position relative to a
   point in a frame.
   Parameters
   frame :  frame to position the mouse relative to
   x :      x of the mouse
   y :      y of the mouse                                   */
PSI_PROC( void, SetFrameMousePosition )( PSI_CONTROL frame, int x, int y );
/* Captures the mouse to the current control, it's like an
   OwnMouse for a control.                                 */
PSI_PROC( void, CaptureCommonMouse )( PSI_CONTROL pc, LOGICAL bCapture );
_MOUSE_NAMESPACE_END
USE_MOUSE_NAMESPACE

PSI_PROC( SFTFont, GetCommonFontEx )( PSI_CONTROL pc DBG_PASS );
#define GetCommonFont(pc) GetCommonFontEx( pc DBG_SRC )
#define GetFrameFont(pf) GetCommonFont((PSI_CONTROL)pf)
PSI_PROC( void, SetCommonFont )( PSI_CONTROL pc, SFTFont font );
#define SetFrameFont(pf,font) SetCommonFont((PSI_CONTROL)pf,font)

// setting scale of this control immediately scales all contained
// controls, but the control itself remains at it's current size.
PSI_PROC( void, SetCommonScale )( PSI_CONTROL pc, PFRACTION scale_x, PFRACTION scale_y );
// use scale_x and scale_y to scale a, b, results are done in a, b
void ScaleCoords( PSI_CONTROL pc, PS_32 a, PS_32 b );

// bOwn sets the ownership of mouse events to a control, where it remains
// until released.  Some other control has no way to steal it.
PSI_PROC( void, OwnCommonMouse)( PSI_CONTROL pc, int bOwn );

//PSI_PROC void SetDefaultOkayID( PSI_CONTROL pFrame, int nID );
//PSI_PROC void SetDefaultCancelID( PSI_CONTROL pFrame, int nID );

//-------- Generic control functions --------------

// previously this was used;
PSI_PROC( PSI_CONTROL, GetCommonParent )( PSI_CONTROL pc );
#define GetCommonParent(pc)  GetParentControl(pc)
/* Return the container control of this control.
   NULL if there is no parent container. */
PSI_PROC( PSI_CONTROL, GetParentControl )( PSI_CONTROL pc );
/* Return the first control contained in the specified control.
 returns NULL if there is no child control

 <code>
 void enum_all_controls( PSI_CONTROL a_control )
 {
     PSI_CONTROL root_control = a_control;
	  PSI_CONTROL current_control;
	  while( current_control = GetParentControl( root_control ) )
	      root_control = current_control;
			for( current_control = GetFirstChildControl( root_control );
              current_control;
				  current_control = GetNextControl( current_control ) )
			{
             // hmm some sort of recursion on each of these too...
			}
 }
   </code>
 */
PSI_PROC( PSI_CONTROL, GetFirstChildControl )( PSI_CONTROL pc );
/* Return the next control after this one.
    returns NULL if there are no other controls after this one*/
PSI_PROC( PSI_CONTROL, GetNextControl )( PSI_CONTROL pc );

PSI_PROC( PSI_CONTROL, GetFrame)( PSI_CONTROL pc );
#define GetFrame(c) GetFrame((PSI_CONTROL)(c))

PSI_PROC( PSI_CONTROL, GetNearControl)( PSI_CONTROL pc, int ID );
PSI_PROC( void, GetCommonTextEx)( PSI_CONTROL pc, TEXTSTR  buffer, int buflen, int bCString );
#define GetControlTextEx(pc,b,len,str) GetCommonTextEx(pc,b,len,str)
#define GetControlText( pc, buffer, buflen ) GetCommonTextEx( (PSI_CONTROL)(pc), buffer, buflen, FALSE )
#define GetFrameText( pc, buffer, buflen ) GetCommonTextEx( (PSI_CONTROL)(pc), buffer, buflen, FALSE )

PSI_PROC( void, SetCommonText )( PSI_CONTROL pc, CTEXTSTR text );
PSI_PROC( void, SetControlText )( PSI_CONTROL pc, CTEXTSTR text );
PSI_PROC( void, SetFrameText )( PSI_CONTROL pc, CTEXTSTR text );

// set focus to this control,
// it's container which needs to be updated
// is discoverable from the control itself.
PSI_PROC( void, SetCommonFocus)( PSI_CONTROL pc );
PSI_PROC( void, EnableControl)( PSI_CONTROL pc, int bEnable );
PSI_PROC( int, IsControlFocused )( PSI_CONTROL pc );
PSI_PROC( int, IsControlEnabled)( PSI_CONTROL pc );
/*


PSI_PROC( PSI_CONTROL, CreateCommonExx)( PSI_CONTROL pContainer
											  , CTEXTSTR pTypeName
											  , _32 nType
											  , int x, int y
											  , int w, int h
											  , _32 nID
											  , CTEXTSTR caption
											  , _32 ExtraBorderType
											  , PTEXT parameters
											  //, va_list args
												DBG_PASS );
#define CreateCommonEx(pc,nt,x,y,w,h,id,caption) CreateCommonExx(pc,NULL,nt,x,y,w,h,id,caption,0,NULL DBG_SRC)
#define CreateCommon(pc,nt,x,y,w,h,id,caption) CreateCommonExx(pc,NULL,nt,x,y,w,h,id,caption,0,NULL DBG_SRC)
*/

/* returns the TypeID of the control, this can be used to validate the data received from the control.*/
#undef ControlType
PSI_PROC( INDEX, ControlType)( PSI_CONTROL pc );

PSI_PROC( PSI_CONTROL, MakeControl )( PSI_CONTROL pContainer
										  , _32 nType
										  , int x, int y
										  , int w, int h
										  , _32 nID
										  //, ...
										  );

// init is called with an extra parameter on the stack
// works as long as we guarantee C stack call basis...
// the register_control structure allows this override.
PSI_PROC( PSI_CONTROL, MakeControlParam )( PSI_CONTROL pContainer
													  , _32 nType
													  , int x, int y
													  , int w, int h
													  , _32 nID
													  , POINTER param
													  );

// MakePrivateControl passes BORDER_NO_EXTRA_INIT...
PSI_PROC( PSI_CONTROL, MakePrivateControl )( PSI_CONTROL pContainer
													, _32 nType
													, int x, int y
													, int w, int h
													, _32 nID
													//, ...
													);
// MakePrivateControl passes BORDER_NO_EXTRA_INIT...
PSI_PROC( PSI_CONTROL, MakePrivateNamedControl )( PSI_CONTROL pContainer
													, CTEXTSTR pType
													, int x, int y
													, int w, int h
													, _32 nID
													);
PSI_PROC( PSI_CONTROL, MakeCaptionedControl )( PSI_CONTROL pContainer
													  , _32 nType
													  , int x, int y
													  , int w, int h
													  , _32 nID
													  , CTEXTSTR caption
													  //, ...
													  );
PSI_PROC( PSI_CONTROL, VMakeCaptionedControl )( PSI_CONTROL pContainer
														, _32 nType
														, int x, int y
														, int w, int h
														, _32 nID
														, CTEXTSTR caption
														//, va_list args
														);

PSI_PROC( PSI_CONTROL, MakeNamedControl )( PSI_CONTROL pContainer
												 , CTEXTSTR pType
												 , int x, int y
												 , int w, int h
												 , _32 nID
												 //, ...
												 );
PSI_PROC( PSI_CONTROL, MakeNamedCaptionedControlByName )( PSI_CONTROL pContainer
																	 , CTEXTSTR pType
																	 , int x, int y
																	 , int w, int h
																	 , CTEXTSTR pIDName
                                                    , _32 nID // also pass this (if known)
																	 , CTEXTSTR caption
																	 );
PSI_PROC( PSI_CONTROL, MakeNamedCaptionedControl )( PSI_CONTROL pContainer
															 , CTEXTSTR pType
															 , int x, int y
															 , int w, int h
															 , _32 nID
															 , CTEXTSTR caption
															 //, ...
															 );
PSI_PROC( PSI_CONTROL, VMakeControl )( PSI_CONTROL pContainer
											, _32 nType
											, int x, int y
											, int w, int h
											, _32 nID
											//, va_list args
											);

/*
 depricated
 PSI_PROC( PSI_CONTROL, CreateControl)( PSI_CONTROL pFrame
 , int nID
 , int x, int y
 , int w, int h
 , int BorderType
 , int extra );
 */

PSI_PROC( Image,GetControlSurface)( PSI_CONTROL pc );
// result with an image pointer, will sue the image passed
// as prior_image to copy into (resizing if nessecary), if prior_image is NULL
// then a new Image will be returned.  If the surface has not been
// marked as parent_cleaned, then NULL results, as no Original image is
// available.  The image passed as a destination for the surface copy is
// not released, it is resized, and copied into.  THe result may still be NULL
// the image will still be the last valid copy of the surface.
PSI_PROC( Image, CopyOriginalSurface )( PSI_CONTROL pc, Image prior_image );
// this allows the application to toggle the transparency
// characteristic of a control.  If a control is transparent, then it behaves
// automatically as one should using CopyOriginalSurface and restoring that surface
// before doing the draw.  The application does not need to concern itself
// with restoring the prior image, but it must also assume that the entire surface
// has been destroyed, and partial updates are not possible.
PSI_PROC( void, SetCommonTransparent )( PSI_CONTROL pc, LOGICAL bTransparent );

PSI_PROC( void, OrphanCommonEx )( PSI_CONTROL pc, LOGICAL bDraw );
PSI_PROC( void, OrphanCommon )( PSI_CONTROL pc );
#define OrphanFrame(pf) OrphanCommonEx((PSI_CONTROL)pf, FALSE)
#define OrphanControl(pc) OrphanCommonEx((PSI_CONTROL)pc, FALSE)
#define OrphanControlEx(pc,d) OrphanCommonEx((PSI_CONTROL)pc, d)
PSI_PROC( void, AdoptCommonEx )( PSI_CONTROL pFoster, PSI_CONTROL pElder, PSI_CONTROL pOrphan, LOGICAL bDraw );
PSI_PROC( void, AdoptCommon )( PSI_CONTROL pFoster, PSI_CONTROL pElder, PSI_CONTROL pOrphan );
#define AdoptFrame(pff,pfe,pfo) AdoptCommonEx((PSI_CONTROL)pff,(PSI_CONTROL)pfe,(PSI_CONTROL)pfo, TRUE)
#define AdoptControl(pcf,pce,pco) AdoptCommonEx((PSI_CONTROL)pcf,(PSI_CONTROL)pce,(PSI_CONTROL)pco, TRUE)
#define AdoptControlEx(pcf,pce,pco,d) AdoptCommonEx((PSI_CONTROL)pcf,(PSI_CONTROL)pce,(PSI_CONTROL)pco, d)

PSI_PROC( void, SetCommonDraw )( PSI_CONTROL pf, int (CPROC*Draw)( PSI_CONTROL pc ) );
PSI_PROC( void, SetCommonDrawDecorations )( PSI_CONTROL pc, void(CPROC*DrawDecorations)(PSI_CONTROL) );
PSI_PROC( void, SetCommonKey )( PSI_CONTROL pf, int (CPROC*Key)(PSI_CONTROL,_32) );
PSI_PROC( void, SetCommonMouse)( PSI_CONTROL pc, int (CPROC*MouseMethod)(PSI_CONTROL, S_32 x, S_32 y, _32 b ) );
PSI_PROC( void, AddCommonDraw )( PSI_CONTROL pf, int (CPROC*Draw)( PSI_CONTROL pc ) );
PSI_PROC( void, AddCommonKey )( PSI_CONTROL pf, int (CPROC*Key)(PSI_CONTROL,_32) );
PSI_PROC( void, AddCommonMouse)( PSI_CONTROL pc, int (CPROC*MouseMethod)(PSI_CONTROL, S_32 x, S_32 y, _32 b ) );

PSI_PROC( void, SetCommonAcceptDroppedFiles)( PSI_CONTROL pc, void (CPROC*AcceptDroppedFilesMethod)(PSI_CONTROL, CTEXTSTR file, S_32 x, S_32 y ) );
PSI_PROC( void, AddCommonAcceptDroppedFiles)( PSI_CONTROL pc, void (CPROC*AcceptDroppedFilesMethod)(PSI_CONTROL, CTEXTSTR file, S_32 x, S_32 y ) );

PSI_PROC( void, SetCommonSave)( PSI_CONTROL pc, void (CPROC*)(int PSI_CONTROL) );
#define SetControlSave(pc,mm)   SetCommonSave((PSI_CONTROL)pc,(void (CPROC*)(int, PSI_CONTROL))mm)
#define SetFrameSave(pc,mm)     SetCommonSave((PSI_CONTROL)pc,(void (CPROC*)(int, PSI_CONTROL))mm)
PSI_PROC( void, SetCommonLoad)( PSI_CONTROL pc, void (CPROC*)(int PSI_CONTROL) );
#define SetControlLoad(pc,mm)   SetCommonLoad((PSI_CONTROL)pc,(void (CPROC*)(int, PSI_CONTROL))mm)
#define SetFrameLoad(pc,mm)     SetCommonLoad((PSI_CONTROL)pc,(void (CPROC*)(int, PSI_CONTROL))mm)

// ---
// restore background restores the prior background of the control
// so that semi-opaque controls can draw over the correct surface.
PSI_PROC( void, RestoreBackground )( PSI_CONTROL pc, P_IMAGE_RECTANGLE r );
// --
// output to the physical surface the rectangle of the control's surface specified.
PSI_PROC( void, UpdateSomeControls )( PSI_CONTROL pc, P_IMAGE_RECTANGLE pRect );

PSI_PROC( void, SetUpdateRegionEx )( PSI_CONTROL pc, S_32 rx, S_32 ry, _32 rw, _32 rh DBG_PASS );
#define SetUpdateRegion(pc,x,y,w,h) SetUpdateRegionEx( pc,x,y,w,h DBG_SRC )


PSI_PROC( void, EnableCommonUpdates )( PSI_CONTROL frame, int bEnable );
#define EnableFrameUpdates(pf,e) EnableCommonUpdates( (PSI_CONTROL)pf, e )
#define EnableControlUpdates(pc,e) EnableCommonUpdates( (PSI_CONTROL)pc, e )

//PSI_PROC void SetControlKey( PSI_CONTROL pc, void (*KeyMethod)( PSI_CONTROL pc, int key ) );
PSI_PROC( void, UpdateCommonEx )( PSI_CONTROL pc, int bDraw DBG_PASS );
PSI_PROC( void, SmudgeCommonAreaEx )( PSI_CONTROL pc, P_IMAGE_RECTANGLE rect DBG_PASS );
#define SmudgeCommonArea( pc, area ) SmudgeCommonAreaEx( pc, area DBG_SRC )
PSI_PROC( void, SmudgeCommonEx )( PSI_CONTROL pc DBG_PASS );
#define SmudgeCommon(pc) SmudgeCommonEx( pc DBG_SRC )
//#ifdef SOURCE_PSI2
#define UpdateCommon(pc) SmudgeCommon(pc)
//#else
//#define UpdateCommon(pc) UpdateCommonEx(pc,TRUE DBG_SRC)
//#endif
//#define UpdateControlEx(pc,draw) UpdateCommonEx( (PSI_CONTROL)pc, draw )
//#define UpdateFrameEx(pc,draw)   UpdateCommonEx( (PSI_CONTROL)pc, draw )

PSI_PROC( void, UpdateControlEx)( PSI_CONTROL pc DBG_PASS );
#define UpdateControl(pc) UpdateControlEx( pc DBG_SRC )
PSI_PROC( int, GetControlID)( PSI_CONTROL pc );
PSI_PROC( void, SetControlID )( PSI_CONTROL pc, int ID );

PSI_PROC( void, DestroyCommonEx)( PSI_CONTROL *ppc DBG_PASS);
#define DestroyCommon(ppc) DestroyCommonEx(ppc DBG_SRC )
PSI_PROC( void, DestroyControlEx)( PSI_CONTROL pc DBG_PASS);
#define DestroyControl(pc) DestroyControlEx( pc DBG_SRC )
PSI_PROC( void, SetNoFocus)( PSI_CONTROL pc );
PSI_PROC( void *, ControlExtraData)( PSI_CONTROL pc );
_PROP_NAMESPACE
/* Show a dialog to edit a control's properties.
   Parameters
   control :  pointer to a control to edit the properties of.
   
   TODO
   Add an example image of this.                              */
PSI_PROC( int, EditControlProperties )( PSI_CONTROL control );
/* Shows a dialog to edit the properties of the frame (the outer
   container control.) Borders matter, title, size, position...
   TODO
   Add ability to specify parent frame for stacking.
   Parameters
   frame :  frame to edit properties of
   x :      x of the left of the edit dialog
   y :      y of the top of the edit dialog                      */
PSI_PROC( int, EditFrameProperties )( PSI_CONTROL frame, S_32 x, S_32 y );
_PROP_NAMESPACE_END
USE_PROP_NAMESPACE

//------ General Utilities ------------
// adds OK and Cancel buttons based off the 
// bottom right of the frame, and when pressed set
// either done (cancel) or okay(OK) ... 
// could do done&ok or just done - but we're bein cheesey
PSI_PROC( void, AddCommonButtonsEx)( PSI_CONTROL pf
                                , int *done, CTEXTSTR donetext
                                , int *okay, CTEXTSTR okaytext );
PSI_PROC( void, AddCommonButtons)( PSI_CONTROL pf, int *done, int *okay );
PSI_PROC( void, SetCommonButtons)( PSI_CONTROL pf, int *pdone, int *pokay );
PSI_PROC( void, InitCommonButton )( PSI_CONTROL pc, int *value );

PSI_PROC( void, CommonLoop)( int *done, int *okay ); // perhaps give a callback for within the loop?
PSI_PROC( void, CommonWait)( PSI_CONTROL pf ); // perhaps give a callback for within the loop?
PSI_PROC( void, CommonWaitEndEdit)( PSI_CONTROL *pf ); // a frame in edit mode, once edit mode done, continue
PSI_PROC( void, ProcessControlMessages)(void);
/* Buttons. Clickable buttons, Radio buttons and checkboxes. */
_BUTTON_NAMESPACE
/* Symbol which can be used as an attribute of a button to not
   show the button border (custom drawn buttons)               */
#define BUTTON_NO_BORDER 0x0001
/* Defined function signature for the event attached to a button
   when the button is clicked.                                   */
typedef void (CPROC *ButtonPushMethod)(PTRSZVAL,PSI_CONTROL);
/* A function signature for the event attached to a "Custom
   Button" when it is drawn, this function is called.       */
typedef void (CPROC*ButtonDrawMethod)(PTRSZVAL psv, PSI_CONTROL pc);
CONTROL_PROC(Button,(CTEXTSTR,void (CPROC*PushMethod)(PTRSZVAL psv, PSI_CONTROL pc)
						  , PTRSZVAL Data));

// this method invokes the button push method...
PSI_PROC( void, InvokeButton )( PSI_CONTROL pc );
PSI_PROC( void, GetButtonPushMethod )( PSI_CONTROL pc, ButtonPushMethod *method, PTRSZVAL *psv );
PSI_PROC( PSI_CONTROL, SetButtonPushMethod )( PSI_CONTROL pc, ButtonPushMethod method, PTRSZVAL psv );
PSI_PROC( PSI_CONTROL, SetButtonAttributes )( PSI_CONTROL pc, int attr ); // BUTTON_ flags...
PSI_PROC( PSI_CONTROL, SetButtonDrawMethod )( PSI_CONTROL pc, ButtonDrawMethod method, PTRSZVAL psv );
/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   f :            frame to create the button in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   a :            SliderDirection
   c :            caption \- text for the button.
   update_proc :  button click callback function.
                  ButtonPushMethod.
   user_data :    user data to pass to callback when it is
                  invoked.
   
   Returns
   PSI_CONTROL that is a button.                                */
#define MakeButton(f,x,y,w,h,id,c,a,p,d) SetButtonAttributes( SetButtonPushMethod( MakeCaptionedControl(f,NORMAL_BUTTON,x,y,w,h,id,c), p, d ), a )

PSI_CONTROL PSIMakeImageButton( PSI_CONTROL parent, int x, int y, int w, int h, _32 ID
							 , Image pImage
							 , void (CPROC*PushMethod)(PTRSZVAL psv, PSI_CONTROL pc)
							 , PTRSZVAL Data );
/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   f :            frame to create the button in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   a :            SliderDirection
   c :            caption \- text for the button.
   update_proc :  button click callback function.
                  ButtonPushMethod.
   user_data :    user data to pass to callback when it is
                  invoked.
   
   Returns
   PSI_CONTROL that is a button.                                */
#define MakeImageButton(f,x,y,w,h,id,c,a,p,d) SetButtonPushMethod( SetButtonAttributes( SetButtonImage( MakeControl(f,IMAGE_BUTTON,x,y,w,h,id),c),a),p,d)
PSI_PROC( PSI_CONTROL, SetButtonImage )( PSI_CONTROL pc, Image image );

/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   f :            frame to create the button in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   a :            SliderDirection
   c :            caption \- text for the button.
   update_proc :  button click callback function.
                  ButtonPushMethod.
   user_data :    user data to pass to callback when it is
                  invoked.
   
   Returns
   PSI_CONTROL that is a button.                                */
#define MakeCustomDrawnButton(f,x,y,w,h,id,a,dr,p,d) SetButtonPushMethod( SetButtonDrawMethod( SetButtonAttributes( MakeControl(f,CUSTOM_BUTTON,x,y,w,h,id),a ), dr, d ), p,d)


PSI_PROC( void, PressButton)( PSI_CONTROL pc, int bPressed );
PSI_PROC( void, SetButtonFont)( PSI_CONTROL pc, SFTFont font );
PSI_PROC( int, IsButtonPressed)( PSI_CONTROL pc );
PSI_PROC( PSI_CONTROL, SetButtonGroupID )(PSI_CONTROL pc, int nGroupID );
PSI_PROC( PSI_CONTROL, SetButtonCheckedProc )( PSI_CONTROL pc
														, void (CPROC*CheckProc)(PTRSZVAL psv, PSI_CONTROL pc)
															, PTRSZVAL psv );
/* Attributes that affect behavior of radio buttons. */
enum RadioButtonAttributes{
 RADIO_CALL_ALL       = 0, /* Call event callback on every change. */
 
 RADIO_CALL_CHECKED   = 1, /* Call the click callback when the button is checked. */
 
 RADIO_CALL_UNCHECKED = 2, /* Call event callback when the button is unchecked. */
 
		RADIO_CALL_CHANGED   = (RADIO_CALL_CHECKED|RADIO_CALL_UNCHECKED) /* Call when the check state of a button changes. */
		
};

/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   f :            frame to create the button in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   a :            SliderDirection
   c :            caption \- text for the button.
   update_proc :  button click callback function.
                  ButtonPushMethod.
   user_data :    user data to pass to callback when it is
                  invoked.
   
   Returns
   PSI_CONTROL that is a button.                                */
#define MakeRadioButton(f,x,y,w,h,id,t,gr,a,p,d) SetCheckButtonHandler( SetButtonGroupID( SetButtonAttributes( MakeCaptionedControl(f,RADIO_BUTTON,x,y,w,h,id,t), a ), gr ), p, d )

/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   f :            frame to create the button in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   a :            SliderDirection
   c :            caption \- text for the button.
   update_proc :  button click callback function.
                  ButtonPushMethod.
   user_data :    user data to pass to callback when it is
                  invoked.
   
   Returns
   PSI_CONTROL that is a button.                                */
#define MakeCheckButton(f,x,y,w,h,id,t,a,p,d) SetCheckButtonHandler( SetButtonAttributes( SetButtonGroupID( MakeCaptionedControl(f,RADIO_BUTTON,x,y,w,h,id,t),0),a),p,d)
PSI_PROC( PSI_CONTROL, SetRadioButtonGroup )( PSI_CONTROL, int group_id );
PSI_PROC( PSI_CONTROL, SetCheckButtonHandler )( PSI_CONTROL
															 , void (CPROC*CheckProc)(PTRSZVAL psv, PSI_CONTROL pc)
															 , PTRSZVAL psv );
PSI_PROC( int, GetCheckState)( PSI_CONTROL pc );
PSI_PROC( void, SetCheckState)( PSI_CONTROL pc, int nState );
// set the button's background color...
PSI_PROC( void, SetButtonColor )( PSI_CONTROL pc, CDATA color );

_BUTTON_NAMESPACE_END
USE_BUTTON_NAMESPACE

/* Static Text - this control just shows simple text on a
   dialog. Non selectable.
                                                          */
	_TEXT_NAMESPACE
	/* Attributes which can be passed to SetTextControlAttributes. */
	enum TextControlAttributes {
 TEXT_NORMAL     = 0x00, /* Normal Text. */
 
 TEXT_VERTICAL   = 0x01,/* Text is centered vertical, else it is top aligned. */
 
 TEXT_CENTER     = 0x02, /* Text is center justified. */
 
 TEXT_RIGHT      = 0x04, /* Text is right justified. */
 
 TEXT_FRAME_BUMP = 0x10, /* frame of the text control is bump frame (1 up, 1 down thin
    frame)                                                     */
 
			TEXT_FRAME_DENT = 0x20, /* Frame of control is a dent (1 down, 1 up ) */
 TEXT_SHADOW_TEXT = 0x40 /* draw text shadowed */
	};
/* \ \ 
   Parameters
   pf :   frame to create the control in
   x :   left coordinate of control
   y :   top coordinate of the control
   w :   width of the control
   h :   height of the control
   ID :  an integer to identify the control
   text :   text to initialize the control with
   flags :   Attributes \- can be one or more of TextControlAttributes
   
   Returns
   a PSI_CONTROL which is an text control.               */
#define MakeTextControl( pf,x,y,w,h,id,text,flags ) SetTextControlAttributes( MakeCaptionedControl( pf, STATIC_TEXT, x, y, w,h, id, text ), flags )
/* Set the alignment of the text in the TextControl.
   Parameters
   pc :     a "TextControl" control.
   align :  A flag from TextControlAttributes        */
PSI_PROC( void, SetControlAlignment )( PSI_CONTROL pc, int align );

/* offset is a pixel specifier... +/- amount from it's normal
   position. returns if any text remains visible (true is
   visible, false is no longer visible, allowing upper level
   application to reset to 0 offset.
   
   The least amount and maximum effective amount can be received
   from GetControlTextOffsetMinMax.
   Parameters
   pc :      a Text control.
   offset :  offset to apply to the text.
   
   Returns
   TRUE if ti was a valid control, else FALSE.                   */
PSI_PROC( LOGICAL, SetControlTextOffset )( PSI_CONTROL pc, int offset );
/* Get the minimum and maximum offsets that can be applied to a
   text control based on its justification attributes before the
   text will not be drawn in the control. This can be used to
   marquee the text in from the left with the first update
   showing text, until the last bit of text scrolls out at the
   end.
   Parameters
   pc :          A STATIC_TEXT_NAME control.
   min_offset :  pointer to an integer that is filled with the
                 least amount that can be set for an offset
                 before text is not shown.
   max_offset :  pointer to an integer that is filled with the
                 most amount of offset that can be set before
                 text is not shown.
   
   Returns
   TRUE if a valid Text control was passed.
   
   FALSE otherwise.
   See Also
   SetControlTextOffset                                          */
PSI_PROC( LOGICAL, GetControlTextOffsetMinMax )( PSI_CONTROL pc, int *min_offset, int *max_offset );

CAPTIONED_CONTROL_PROC( TextControl, (void) );
/* Sets attributes of the text control.
   Parameters
   pc :     a STATIC_TEXT_NAME control.
   flags :  one or move values combined from
            TextControlAttributes.           */
PSI_PROC( PSI_CONTROL, SetTextControlAttributes )( PSI_CONTROL pc, int flags );
/* Sets the foreground and background of a text control.
   Parameters
   pc :    a Text control
   fore :  CDATA describing the foreground color to use
   back :  CDATA describing the background color to use. 0 is no
           color, and only the foreground text is output. See
           PutString.                                            */
PSI_PROC( void, SetTextControlColors )( PSI_CONTROL pc, CDATA fore, CDATA back );
_TEXT_NAMESPACE_END
USE_TEXT_NAMESPACE

/* Edit Control. This is a control that allows for text input. It
   also works as a drop file acceptor, and the name of the file
   or folder being dropped on it is entered as text.              */
	_EDIT_NAMESPACE
	/* Options which can be passed to the MakeEditControl macro. */
	enum EditControlOptions {
		EDIT_READONLY = 0x01, /* Option to set READONLY on create. */
		
		EDIT_PASSWORD = 0x02 /* Option to set password style on create. */
		
	};
CAPTIONED_CONTROL_PROC( EditControl, (CTEXTSTR text) );
/* This enters the text into the edit field as if it were typed.
   This respects things like marked region auto delete. It's not
   the same as SetText, but will insert at the position the
   cursor is at.
   Parameters
   control :  control to type into
   text :     the text to type.                                  */
PSI_PROC( void, TypeIntoEditControl )( PSI_CONTROL control, CTEXTSTR text );

/* \ \ 
   Parameters
   f :   frame to create the control in
   x :   left coordinate of control
   y :   top coordinate of the control
   w :   width of the control
   h :   height of the control
   ID :  an integer to identify the control
   t :   text to initialize the control with
   a :   Attributes \- can be one or more of EditControlOptions
   
   Returns
   a PSI_CONTROL which is an edit control.               */
#define MakeEditControl(f,x,y,w,h,id,t,a) SetEditControlPassword( SetEditControlReadOnly( MakeCaptionedControl( f,EDIT_FIELD,x,y,w,h,id,t ) \
	, ( a & EDIT_READONLY)?TRUE:FALSE )   \
	, ( a & EDIT_PASSWORD)?TRUE:FALSE )
/* Sets an edit control into read only mode. This actually means
   that the user cannot enter data, and can only see what is in
   it. The difference between this and a text control is the
   border and background style.
   Parameters
   pc :         edit control to set readonly
   bReadOnly :  if TRUE, sets readonly. If false, clears readonly.
   
   Returns
   The PSI Control passed is returned, for nesting.                */
PSI_PROC( PSI_CONTROL, SetEditControlReadOnly )( PSI_CONTROL pc, LOGICAL bReadOnly );
/* If password style is set, then the text in the edit field is
   shown as ******.
   Parameters
   pc :         pointer to an edit control
   bPassword :  if TRUE, Sets style to password, if FALSE, clears
                password style.                                   */
PSI_PROC( PSI_CONTROL, SetEditControlPassword )( PSI_CONTROL pc, LOGICAL bPassword );
//PSI_PROC( void, SetEditFont )( PSI_CONTROL pc, SFTFont font );
// Use GetControlText/SetControlText
_EDIT_NAMESPACE_END
USE_EDIT_NAMESPACE

/* Slider Control - this is a generic control that has a minimum
   value, and a maximum value, and a clickable knob that can
   select a value inbetween.
   
   the maximum value may be less than the minimum value, the
   current selected value will be between these.
   
   A horizontal slider, the value for minimum is on the left. A
   vertical slider, the value for minimum is on the bottom.      */
	_SLIDER_NAMESPACE
	/* These are values to pass to SetSliderOptions to control the
	   slider's direction.                                         */
	enum SliderDirection {
		SLIDER_HORIZ =1, /* Slider is horizontal. That is the knob moves left and right. The
		   value as minimum is at the left, and the value maximum is at
		   the right. numerically minimum can be more than maximum.         */
		
			SLIDER_VERT  =0 /* Slider is vertical. That is the knob moves up and down. The
			   value as minimum is at the bottom, and the value maximum is
			   at the top. numerically minimum can be more than maximum.   */
			
	};
/* An all-in-one macro to create a Slider control, set the
   callback, and set direction options.
   Parameters
   pf :           frame to create the slider in
   x :            left coordinate of the control
   y :            top coordinate of the control
   w :            how wide the control is
   h :            how tall to make the control
   nID :          ID of the control (any numeric ID you want to
                  call it)
   opt :          SliderDirection
   update_proc :  callback to which gets called when the slider's
                  current value changes.
   user_data :    user data to pass to callback when it is
                  invoked.                                        */
#define MakeSlider(pf,x,y,w,h,nID,opt,updateproc,updateval) SetSliderOptions( SetSliderUpdateHandler( MakeControl(pf,SLIDER_CONTROL,x,y,w,h,nID ), updateproc,updateval ), opt)
//CONTROL_PROC( Slider, (
typedef void (CPROC*SliderUpdateProc)(PTRSZVAL psv, PSI_CONTROL pc, int val);
/* Set the new minimum, maximum and current for the slider.
   Parameters
   pc :       pointer to a slider control \- does nothing
              otherwise.
   min :      new minimum value for the mimimum amount to scale.
   current :  the current selected value. (should be between
              min\-max)
   max :      new maximum value which can be selected.           */
PSI_PROC( void, SetSliderValues)( PSI_CONTROL pc, int min, int current, int max );
/* Set's the slider's direction. (horizontal or vertical)
   Parameters
   pc :    slider control
   opts :  SliderDirection                                */
PSI_PROC( PSI_CONTROL, SetSliderOptions )( PSI_CONTROL pc, int opts );
/* Set a callback function to allow responding to changes in the
   slider.
   Parameters
   pc :                 pointer to a slider control
   SliderUpdateEvent :  callback to a user function when the
                        slider's value changes.
   psvUser :            data to pass to callback function when
                        invoked.                                 */
PSI_PROC( PSI_CONTROL, SetSliderUpdateHandler )( PSI_CONTROL pc, SliderUpdateProc, PTRSZVAL psvUser );
_SLIDER_NAMESPACE_END
USE_SLIDER_NAMESPACE

/* SFTFont Control - Really this is all about a font picker. The
   data used to create the font to show can be retreived to be
   saved for later recreation.                                 */
_FONTS_NAMESPACE
#ifndef FONT_RENDER_SOURCE
// types of data which may result...
//typedef struct font_data_tag *PFONTDATA;
//typedef struct render_font_data_tag *PRENDER_FONTDATA;
#endif

// common dialog to get a font which is then available for all
// Image text operations (PutString, PutCharacter)
// result font selection data can be resulted in the area referenced by
// the pointer, and size pointer...
//
// actual work done here for pUpdateFontFor(NULL) ...
// if pUpdateFontFor is not null, an apply button will appear, allowing the actual
// control to be updated to the chosen font.
//
/* scale_width, height magically apply... and are saved in the
   font data structure for re-rendering... if a font is rendered
   here the same exact font result will be acheived using
   RenderScaledFont( pdata )
   Parameters
   width_scale :   pointer to a scalar fraction to scale
                   the font by
   height_scale :  pointer to a scalar fraction to scale
                   the font by                                          */
PSI_PROC( SFTFont, PickScaledFontWithUpdate )( S_32 x, S_32 y
														, PFRACTION width_scale
														, PFRACTION height_scale
														, size_t *pFontDataSize
														 // resulting parameters for the data and size of data
														 // which may be passe dto RenderFontData
														, POINTER *pFontData
														, PSI_CONTROL pAbove
														, void (CPROC *UpdateFont)( PTRSZVAL psv, SFTFont font )
														, PTRSZVAL psvUpdate );

/* <combine sack::PSI::font::PickScaledFontWithUpdate@S_32@S_32@PFRACTION@PFRACTION@P_32@POINTER *@PSI_CONTROL@void __cdecl *UpdateFont PTRSZVAL psv\, SFTFont font@PTRSZVAL>
   
   PickFontWithUpdate can be used to receive event notices when
   the font is changed, allowing immediately refresh using the
   changed font.
   
   
   Parameters
   x :              x screen position to put the dialog
   y :              y screen position to put the dialog
   pFontDataSize :  pointer to a 32 bit value to receive the font
                    data length. (can ba NULL to ignore)
   pFontData :      pointer to pointer to get font_data. (can ba
                    NULL to ignore)
   pAbove :         pointer to a frame to stack this one above.
   UpdateFont :     user callback passed the psvUpdate data and
                    the SFTFont that has been updated.
   psvUpdate :      user data to pass to use callback when
                    invoked.                                                                                                                                           */
PSI_PROC( SFTFont, PickFontWithUpdate )( S_32 x, S_32 y
										  , size_t *pFontDataSize
											// resulting parameters for the data and size of data
											// which may be passe dto RenderFontData
										  , POINTER *pFontData
										  , PSI_CONTROL pAbove
										  , void (CPROC *UpdateFont)( PTRSZVAL psv, SFTFont font )
												, PTRSZVAL psvUpdate );
// pick font for uses pickfontwithupdate where the update procedure
// sets the font on a control.
PSI_PROC( SFTFont, PickFontFor )( S_32 x, S_32 y
								  , size_t *pFontDataSize
									// resulting parameters for the data and size of data
                           // which may be passe dto RenderFontData
								  , POINTER *pFontData
									  , PSI_CONTROL pAbove
									  , PSI_CONTROL pUpdateFontFor );

/* Pick a font. This shows a dialog
   Parameters
   x :          screen location of the dialog
   y :          screen location of the dialog
   size :       pointer to a 32 bit value to receive the length
                of the font's data
   pFontData :  pointer to a pointer to be filled with the
                address of a FONTDATA describing the chosen font.
   pAbove :     parent window to make sure this is stacked above.
   
   Example
   <code lang="c++">
   POINTER data = NULL; // gets filled with font data - initialize to NULL or a previously returned FONTDATA.
   _32 data_length = 0; // result font data length.
   
   SFTFont font = PickFont( 0, 0, &amp;data_length, &amp;data, NULL );
   
   {
       // and like I really need to exemplify saving a block of data...
       \FILE *file = fopen( "font_description", "wb" );
       fwrite( data, 1, data_length, file );
       fclose( file );
   }
   </code>                                                                                                    */
PSI_PROC( SFTFont, PickFont)( S_32 x, S_32 y
                                  , size_t * size, POINTER *pFontData
								 , PSI_CONTROL pAbove );
/* <combine sack::PSI::font::PickScaledFontWithUpdate@S_32@S_32@PFRACTION@PFRACTION@P_32@POINTER *@PSI_CONTROL@void __cdecl *UpdateFont PTRSZVAL psv\, SFTFont font@PTRSZVAL>
   
   \ \                                                                                                                                                                 */
#define PickScaledFont( x,y,ws,hs,size,fd,pa) PickScaledFontWithUpdate( x,y,ws,hs,size,fd,pa,NULL,0)

// this can take the resulting data from a pick font operation
// and result in a font... concerns at the moment are for cases
// of trying to use the same  string on different systems (different font
// locations) and getting a same (even similar?) result
//PSI_PROC( void, DestroyFont)( SFTFont *font );
// takes an already rendered font, and writes it to a file.
// at the moment this will not work with display services....
//PSI_PROC( void, DumpFontFile )( CTEXTSTR name, SFTFont font );

_FONTS_NAMESPACE_END
USE_FONTS_NAMESPACE

PSI_PROC( void, DumpFrameContents )( PSI_CONTROL pc );

//------- ListBox Control --------
_LISTBOX_NAMESPACE
typedef struct listitem_tag *PLISTITEM;
#define LISTOPT_TREE   2
#define LISTOPT_SORT   1
CONTROL_PROC( ListBox, (void) );
#define MakeListBox(pf,x,y,w,h,nID,opt) SetListboxIsTree( MakeControl(pf,LISTBOX_CONTROL,x,y,w,h,nID), (opt & LISTOPT_TREE)?TRUE:FALSE )
PSI_PROC( PSI_CONTROL, SetListboxIsTree )( PSI_CONTROL pc, int bTree );
#define LISTBOX_SORT_NORMAL 1
#define LISTBOX_SORT_DISABLE 0
PSI_PROC( PSI_CONTROL, SetListboxSort )( PSI_CONTROL pc, int bSortTrue ); // may someday add SORT_INVERSE?
PSI_PROC( PSI_CONTROL, SetListboxMultiSelect )( PSI_CONTROL, int bEnable );
PSI_PROC( PSI_CONTROL, SetListboxMultiSelectEx )( PSI_CONTROL, int bEnable, int bLazy );
PSI_PROC( int, GetListboxMultiSelectEx )( PSI_CONTROL, int *multi, int *lazy );
PSI_PROC( int, GetListboxMultiSelect )( PSI_CONTROL ); // returns only multiselect option, not lazy with multselect
PSI_PROC( void, ResetList)( PSI_CONTROL pc );
// put an item at end of list.
PSI_PROC( PLISTITEM, AddListItem)( PSI_CONTROL pc, CTEXTSTR text );
PSI_PROC( PLISTITEM, AddListItemEx)( PSI_CONTROL pc, int nLevel, CTEXTSTR text );
// put an item after a known item... NULL to add at head of list.
PSI_PROC( PLISTITEM, InsertListItem)( PSI_CONTROL pc, PLISTITEM pAfter, CTEXTSTR text );
PSI_PROC( PLISTITEM, InsertListItemEx)( PSI_CONTROL pc, PLISTITEM pAfter, int nLevel, CTEXTSTR text );
PSI_PROC( void, DeleteListItem)( PSI_CONTROL pc, PLISTITEM hli );
PSI_PROC( PLISTITEM, SetItemData)( PLISTITEM hli, PTRSZVAL psv );
PSI_PROC( PTRSZVAL, GetItemData)( PLISTITEM hli );
PSI_PROC( void, GetListItemText)( PLISTITEM hli, TEXTSTR buffer, int bufsize );
/* depreicated, use GetListItemText instead, please */
PSI_PROC( void, GetItemText)( PLISTITEM hli, int bufsize, TEXTSTR buffer );
#define GetItemText(hli,bufsize,buf) GetListItemText(hli,buf,bufsize)
PSI_PROC( void, SetItemText )( PLISTITEM hli, CTEXTSTR buffer );
PSI_PROC( PLISTITEM, GetSelectedItem)( PSI_CONTROL pc );
PSI_PROC( void, ClearSelectedItems )( PSI_CONTROL plb );

PSI_PROC( void, SetSelectedItem)( PSI_CONTROL pc, PLISTITEM hli );
PSI_PROC( void, SetItemSelected)( PSI_CONTROL pc, PLISTITEM hli, int bSelect );
PSI_PROC( void, SetCurrentItem)( PSI_CONTROL pc, PLISTITEM hli );
PSI_PROC( PLISTITEM, FindListItem)( PSI_CONTROL pc, CTEXTSTR text );
PSI_PROC( PLISTITEM, GetNthTreeItem )( PSI_CONTROL pc, PLISTITEM pli, int level, int idx );
PSI_PROC( PLISTITEM, GetNthItem )( PSI_CONTROL pc, int idx );
typedef void (CPROC*SelectionChanged )( PTRSZVAL psvUser, PSI_CONTROL pc, PLISTITEM hli );
PSI_PROC( void, SetSelChangeHandler)( PSI_CONTROL pc, SelectionChanged proc, PTRSZVAL psvUser );
typedef void (CPROC*DoubleClicker)( PTRSZVAL psvUser, PSI_CONTROL pc, PLISTITEM hli );
PSI_PROC( void, SetDoubleClickHandler)( PSI_CONTROL pc, DoubleClicker proc, PTRSZVAL psvUser );
// if bopened - branch is being expanded, else it is being closed (collapsed)
typedef void (CPROC*ListItemOpened)( PTRSZVAL psvUser, PSI_CONTROL pc, PLISTITEM hli, LOGICAL bOpened );
PSI_PROC( void, SetListItemOpenHandler )( PSI_CONTROL pc, ListItemOpened proc, PTRSZVAL psvUser );
// returns the prior state of disabledness
PSI_PROC( int, DisableUpdateListBox )( PSI_CONTROL pc, LOGICAL bDisable );
// on right click down,up this proc is triggered...
PSI_PROC( void, SetItemContextMenu )( PLISTITEM pli, PMENU pMenu, void (CPROC*MenuProc)(PTRSZVAL, PLISTITEM, _32 menuopt ), PTRSZVAL psv );
PSI_PROC( int, OpenListItem )( PLISTITEM pli, int bOpen );
PSI_PROC( void, SetListBoxTabStops )( PSI_CONTROL pc, int nStops, int *pStops );
PSI_PROC( void, EnumListItems )( PSI_CONTROL pc
										 , PLISTITEM pliStart
										 , void (CPROC *HandleListItem )(PTRSZVAL,PSI_CONTROL,PLISTITEM)
										 , PTRSZVAL psv );
PSI_PROC( void, EnumSelectedListItems )( PSI_CONTROL pc
													, PLISTITEM pliStart
													, void (CPROC *HandleListItem )(PTRSZVAL,PSI_CONTROL,PLISTITEM)
													, PTRSZVAL psv );
PSI_PROC( PSI_CONTROL, GetItemListbox )( PLISTITEM pli );

PSI_PROC( void, MoveListItemEx )( PSI_CONTROL pc, PLISTITEM pli, int level_direction, int direction );
PSI_PROC( void, MoveListItem )( PSI_CONTROL pc, PLISTITEM pli, int direction );

_LISTBOX_NAMESPACE_END
USE_LISTBOX_NAMESPACE

//------- GridBox Control --------
#ifdef __LINUX__
typedef PTRSZVAL HGRIDITEM;
PSI_PROC(PSI_CONTROL, MakeGridBox)( PSI_CONTROL pf, int options, int x, int y, int w, int h,
                                 int viewport_x, int viewport_y, int total_x, int total_y,
                                 int row_thickness, int column_thickness, PTRSZVAL nID );
#endif
/* * ------ Popup Menus ----------- *
   popup interface.... these are mimics of windows... and at
   some point should internally alias to popup code - if I ever
   get it back.
   
   
   
   Actually, I depricated my own implementation (maybe on Linux
   they could resemble working), but since I cloned the Windows
   API, I just fell back to standard windows popups instead.    */

_MENU_NAMESPACE



/* Create a popup menu which can have items added to it. This is
   not shown until TrackPopup is called.
   Parameters
   None.                                                         */
PSI_PROC( PMENU, CreatePopup)( void );
/* Destroys a popup menu.
   Parameters
   pm :  menu to delete   */
PSI_PROC( void, DestroyPopup)( PMENU pm );
/* Clears all entries on a menu.
   Parameters
   pm :  menu to clear           */
PSI_PROC( void, ResetPopup)( PMENU pm );
/* get sub-menu data...
   Parameters
   pm :    PMENU to get popup data for... 
   item :  Item on the menu to get the popup menu for. */
PSI_PROC( void *,GetPopupData)( PMENU pm, int item );
/* Add a new item to a popup menu
   Parameters
   pm :     menu to add items to
   type :   type of the item (MF_TEXT,MF_POPUP, MF_SEPARATOR ?) 
   dwID :   ID of item. (PMENU of a popup menu if it's a popup
            item)
   pData :  text data of the item (unless separator)             */
PSI_PROC( PMENUITEM, AppendPopupItem)( PMENU pm, int type, PTRSZVAL dwID, CPOINTER pData );
/* Set a checkmark on a menu item.
   Parameters
   pm :     menu containing the item to check
   dwID :   ID of the item to check
   state :  (MF_CHECKED, MF_UNCHECKED?)       */
PSI_PROC( PMENUITEM, CheckPopupItem)( PMENU pm, PTRSZVAL dwID, _32 state );
/* Delete a single item from a menu.
   Parameters
   pm :     menu containing the item to delete
   dwID :   ID of the item to delete
   state :  (?)                                */
PSI_PROC( PMENUITEM, DeletePopupItem)( PMENU pm, PTRSZVAL dwID, _32 state );
/* This Shows a popup on the screen, and waits until it returns
   with a result. I guess this is actually the internal routine
   used for handling selected popup items on popup menus.
   Parameters
   hMenuSub :  sub menu to track
   parent :    parent menu that has started tracking a
               submenu.                                         */
PSI_PROC( int, TrackPopup)( PMENU hMenuSub, PSI_CONTROL parent );
_MENU_NAMESPACE_END
USE_MENU_NAMESPACE

//------- File Selector Control --------
// these are basic basic file selection dialogs... 
	// the concept is valid, and they should be common like controls...
	// types are tab sepeared list of default extensions to open.
	// returns TRUE if the filename is selected and the result buffer is filled.
   // returns FALSE if the filename selection is canceled, result is not modified.
	// if bcreate is used, then the filename does not HAVE to exist..
   // bCreate may also be read as !bMustExist
PSI_PROC( int, PSI_PickFile)( PSI_CONTROL parent, CTEXTSTR basepath, CTEXTSTR types, TEXTSTR result, _32 result_len, int bCreate );
PSI_PROC( int, PSI_OpenFile)( CTEXTSTR basepath, CTEXTSTR types, TEXTSTR result );
// this may be used for save I think....
//PSI_PROC( int, PSI_OpenFileEx)( PSI_CONTROL parent, CTEXTSTR basepath, CTEXTSTR types, CTEXTSTR result, _32 result_len, int Create );

//------- Scroll Control --------
#define SCROLL_HORIZONTAL 1
#define SCROLL_VERITCAL   0
/* This is a scrollbar type control. It's got an up/down or
   left/right arrow, and a thumb that can be clicked on. Used by
   the listbox control.                                          */
_SCROLLBAR_NAMESPACE
	// types values for MoveScrollBar
	enum MoveScrollBarTypes{
 UPD_1UP       = 0, /* Moved UP by one. */
 
 UPD_1DOWN     = 1, /* Moved DOWN by one. */
 
 UPD_RANGEUP   = 2, /* Moved UP by one range. */
 
 UPD_RANGEDOWN = 3, /* Moved DOWN by one range. */
 
 UPD_THUMBTO   = 4 /* Position of the thumb has changed to something. */
 
	};
/* \ \ 
   Parameters
   pc :     SCROLLBAR_CONTROL_NAME
   min :    minimum value (needs to be less than max)
   cur :    current value (should be between min to max
   range :  how many of the values to span
   max :    max value
   
   Remarks
   The range of the control controls how wide the thumb control
   is. It should reflect things like how many of the maximum
   items are showing in a list.                                 */
PSI_PROC( void, SetScrollParams)( PSI_CONTROL pc, int min, int cur, int range, int max );
CONTROL_PROC( ScrollBar, (_32 attr) );
/* Set the event callback for when the position on the control
   changes.
   Parameters
   pc :        a SCROLLBAR_CONTROL_NAME.
   callback :  the address of the function to call when the
               position changes.
   data :      user data passed to the event function when it is
               invoked.                                          */
PSI_PROC( void, SetScrollUpdateMethod)( PSI_CONTROL pc
                    , void (CPROC*UpdateProc)(PTRSZVAL psv, int type, int current)
                                                  , PTRSZVAL data );
/* move the scrollbar. operations to specify move are +1, -1, +1
   range, -1 range. the MOVE_THUMB operation is actually passed
   to the event callback?
   Parameters
   pc :    control that is a SCROLLBAR_CONTROL_NAME
   type :  a value from MoveScrollBarTypes                       */
PSI_PROC( void, MoveScrollBar )( PSI_CONTROL pc, int type );
_SCROLLBAR_NAMESPACE_END
USE_SCROLLBAR_NAMESPACE

//------- Misc Controls (and widgets) --------
_SHEETS_NAMESPACE
#define MakeSheetControl(c,x,y,w,h,id) MakeControl(c,SHEET_CONTROL,x,y,w,h,id)
/* Adds a sheet to a tab control. A sheet is just another
   PSI_CONTROL frame.
   Parameters
   pControl :  control to add the page to
   contents :  frame control to add                       */
PSI_PROC( void, AddSheet )( PSI_CONTROL pControl, PSI_CONTROL contents );
/* Removes a sheet from a tab control.
   Parameters
   pControl :  tab control to remove the sheet from
   ID :        identifier of the sheet to remove.   */
PSI_PROC( int, RemoveSheet )( PSI_CONTROL pControl, _32 ID );
/* Request a sheet control by ID.
   Parameters
   pControl :  tab control to get a sheet from.
   ID :        the ID of the page to get from the tab control.
   
   Returns
   Control that is the sheet that was added as the specified ID. */
PSI_PROC( PSI_CONTROL, GetSheet )( PSI_CONTROL pControl, _32 ID );
/* Gets a control from a specific sheet in the tab control
   Parameters
   pControl :   tab control to get the control from a sheet of.
   IDSheet :    identifier of the sheet to get a control from
   IDControl :  identifier of the control on a sheet in the tab
                control.
   
   Returns
   Control that has the requested ID on the requested page, if
   found. Otherwise NULL.                                       */
PSI_PROC( PSI_CONTROL, GetSheetControl )( PSI_CONTROL pControl, _32 IDSheet, _32 IDControl );
/* \returns the integer ID of the selected sheet.
   Parameters
   pControl :  Sheet control to get the current sheet ID from. */
PSI_PROC( PSI_CONTROL, GetCurrentSheet )( PSI_CONTROL pControl );
/* Set which property sheet is currently selected.
   Parameters
   pControl :  Sheet Control.
   ID :        ID of the page control to select.   */
PSI_PROC( void, SetCurrentSheet )( PSI_CONTROL pControl, _32 ID );
/* Get the size of sheets for this sheet control. So the page
   creator can create a page that is the correct size for the
   sheet dialog.
   Parameters
   pControl :  tab control to get the sheet size for
   width :     pointer to a 32 bit unsigned value to receive the
               width sheets should be.
   height :    pointer to a 32 bit unsigned value to receive the
               height sheets should be.                          */
PSI_PROC( int, GetSheetSize )( PSI_CONTROL pControl, _32 *width, _32 *height );
/* Sets a page as disabled. Disabled tabs cannot be clicked on
   to select.
   Parameters
   pControl :  tab control containing sheet to disable or enable.
   ID :        ID of the sheet to disable.
   bDisable :  TRUE to disable the page, FALSE to re\-enable the
               page.                                              */
PSI_PROC( void, DisableSheet )( PSI_CONTROL pControl, _32 ID, LOGICAL bDisable );

// Tab images are sliced and diced across the vertical center of the image
// the center is then spread as wide as the caption requires.
// set default tabs for the sheet control itself
PSI_PROC( void, SetTabImages )( PSI_CONTROL pControl, Image active, Image inactive );
// set tab images on a per-sheet basis, overriding the defaults specified.
PSI_PROC( void, SetSheetTabImages )( PSI_CONTROL pControl, _32 ID, Image active, Image inactive );
// with the ability to set the image for the tab, suppose it would be
// wise to set the sheet's text color on the tab...
// Initial tabs are black and white, with inverse black/white text...
PSI_PROC( void, SetTabTextColors )( PSI_CONTROL pControl, CDATA cActive, CDATA cInactive );
/* Sets the active and inactive colors for text.
   Parameters
   pControl :   sheet control to set the tab attribute of
   ID :         ID of the sheet to set the tab color attributes for
   cActive :    Color of the text when the tab is active.
   cInactive :  Color of the text when the tab is inactive.         */
PSI_PROC( void, SetSheetTabTextColors )( PSI_CONTROL pControl, _32 ID, CDATA cActive, CDATA cInactive );
_SHEETS_NAMESPACE_END
USE_SHEETS_NAMESPACE



//------- Misc Controls (and widgets) --------
PSI_PROC( void, SimpleMessageBox )( PSI_CONTROL parent, CTEXTSTR title, CTEXTSTR content );
// result is the address of a user buffer to read into, reslen is the size of that buffer.
// question is put above the question... pAbove is the window which this one should be placed above (lock-stacked)
PSI_PROC( int, SimpleUserQuery )( TEXTSTR result, int reslen, CTEXTSTR question, PSI_CONTROL pAbove );

PSI_PROC( void, RegisterResource )( CTEXTSTR appname, CTEXTSTR resource_name, int ID, int resource_name_range, CTEXTSTR type_name );
// assuming one uses a
#define SimpleRegisterResource( name, typename ) RegisterResource( WIDE("application"), WIDE(#name), name, 1, typename );
#define EasyRegisterResource( domain, name, typename ) RegisterResource( domain, WIDE(#name), name, 1, typename );
#define EasyRegisterResourceRange( domain, name, range, typename ) RegisterResource( domain, WIDE(#name), name, range, typename );
#define SimpleRegisterAppResource( name, typename, class ) RegisterResource( WIDE("application/") class, WIDE(#name), name, 1, typename );

PSI_PROC( size_t, _SQLPromptINIValue )(
												CTEXTSTR lpszSection,
												CTEXTSTR lpszEntry,
												CTEXTSTR lpszDefault,
												TEXTSTR lpszReturnBuffer,
												size_t cbReturnBuffer,
												CTEXTSTR filename
											  );

/* This namespace is not completely implemented (if at all). At
   the start it looked good, and a lot of it was a
   copy-insert-modify of the existing functions, it wasn't that
   much work to lay out. But then looking at the content of the
   interface, and realizing that each individual registered
   component needed to have custom methods associated with it,
   it will be a different sort of project to support dynamic
   remote controls... so we look at how WPF did it, and that is
   a solution, I suppose.                                       */
_PSI_INTERFACE_NAMESPACE

// hmm upon loading the thing from disk, need to query the callbacks of the application to
	// request the correct ptrszvals...
#ifndef __NO_MSGSVR__
enum {
    MSG_ControlInit = MSG_EventUser
     , MSG_ButtonDraw
    , MSG_ButtonClick
};
#endif


/* Quick wrapper to make consistant call method signatures for
   interface methods.                                          */
#define PSI_PROC_PTR( type, name ) type (CPROC*name)
/* Begin a basic interface to PSI to consider extending PSI
   across a message service.                                */
struct control_interface_tag
{

PSI_PROC_PTR( PRENDER_INTERFACE, SetControlInterface)( PRENDER_INTERFACE DisplayInterface );
/* <combine sack::PSI::SetControlImageInterface@PIMAGE_INTERFACE>
   
   \ \                                                            */
PSI_PROC_PTR( PIMAGE_INTERFACE, SetControlImageInterface )( PIMAGE_INTERFACE DisplayInterface );

/* <combine sack::PSI::AlignBaseToWindows>
   
   \ \                                     */
PSI_PROC_PTR( void, AlignBaseToWindows)( void );
PSI_PROC_PTR( void, SetBaseColor )( INDEX idx, CDATA c );
/* <combine sack::PSI::GetBaseColor@INDEX>
   
   \ \                                     */
PSI_PROC_PTR( CDATA, GetBaseColor )( INDEX idx );

/* <combine sack::PSI::CreateFrame@CTEXTSTR@int@int@int@int@_32@PSI_CONTROL>
   
   \ \                                                                       */
PSI_PROC_PTR( PSI_CONTROL, CreateFrame)( CTEXTSTR caption, int x, int y
                                        , int w, int h
                                        , _32 BorderFlags
                                        , PSI_CONTROL hAbove );

PSI_PROC_PTR( PSI_CONTROL, CreateFrameFromRenderer )( CTEXTSTR caption
                                                         , _32 BorderTypeFlags
                                                         , PRENDERER pActImg );
/* <combine sack::PSI::DestroyCommonEx@PSI_CONTROL *ppc>
   
   \ \                                                   */
PSI_PROC_PTR( void, DestroyFrameEx)( PSI_CONTROL pf DBG_PASS );

/* <combine sack::PSI::FrameBorderX@_32>
   
   \ \                                   */
PSI_PROC_PTR( int, FrameBorderX )( _32 flags );
/* <combine sack::PSI::FrameBorderXOfs@_32>
   
   \ \                                      */
PSI_PROC_PTR( int, FrameBorderXOfs )( _32 flags );
/* <combine sack::PSI::FrameBorderY@PSI_CONTROL@_32@CTEXTSTR>
   
   \ \                                                        */
PSI_PROC_PTR( int, FrameBorderY )( _32 flags, CTEXTSTR caption );
/* <combine sack::PSI::FrameBorderYOfs@PSI_CONTROL@_32@CTEXTSTR>
   
   \ \                                                           */
PSI_PROC_PTR( int, FrameBorderYOfs )( _32 flags, CTEXTSTR caption );
/* <combine sack::PSI::DisplayFrame@PSI_CONTROL>
   
   \ \                                           */
PSI_PROC_PTR( void, DisplayFrame)( PSI_CONTROL pf );
/* <combine sack::PSI::SizeCommon@PSI_CONTROL@_32@_32>
   
   \ \                                                 */
PSI_PROC_PTR( void, SizeCommon)( PSI_CONTROL pf, _32 w, _32 h );
/* <combine sack::PSI::SizeCommonRel@PSI_CONTROL@_32@_32>
   
   \ \                                                    */
PSI_PROC_PTR( void, SizeCommonRel)( PSI_CONTROL pf, _32 w, _32 h );
PSI_PROC_PTR( void, MoveCommon)( PSI_CONTROL pf, S_32 x, S_32 y );
/* <combine sack::PSI::MoveCommonRel@PSI_CONTROL@S_32@S_32>
   
   \ \                                                      */
PSI_PROC_PTR( void, MoveCommonRel)( PSI_CONTROL pf, S_32 x, S_32 y );
/* <combine sack::PSI::MoveSizeCommon@PSI_CONTROL@S_32@S_32@_32@_32>
   
   \ \                                                               */
PSI_PROC_PTR( void, MoveSizeCommon)( PSI_CONTROL pf, S_32 x, S_32 y, _32 w, _32 h );
PSI_PROC_PTR( void, MoveSizeCommonRel)( PSI_CONTROL pf, S_32 x, S_32 y, _32 w, _32 h );
/* <combine sack::PSI::GetControl@PSI_CONTROL@int>
   
   \ \                                             */
PSI_PROC_PTR( PSI_CONTROL, GetControl)( PSI_CONTROL pf, int ID );
/* <combine sack::PSI::GetCommonUserData@PSI_CONTROL>
   
   \ \                                                */
PSI_PROC_PTR( PTRSZVAL, GetFrameUserData )( PSI_CONTROL pf );
PSI_PROC_PTR( void, SetFrameUserData )( PSI_CONTROL pf, PTRSZVAL psv );
/* <combine sack::PSI::UpdateFrameEx@PSI_CONTROL@int@int@int@int h>
   
   \ \                                                              */
PSI_PROC_PTR( void, UpdateFrame )( PSI_CONTROL pf
                                            , int x, int y
                                            , int w, int h );
/* <combine sack::PSI::_mouse::SetFrameMousePosition@PSI_CONTROL@int@int>
   
   \ \                                                                    */
PSI_PROC_PTR( void, SetFrameMousePosition )( PSI_CONTROL frame, int x, int y );

/* <combine sack::PSI::GetFrame@PSI_CONTROL>
   
   \ \                                       */
PSI_PROC_PTR( PSI_CONTROL, GetFrame)( PSI_CONTROL pc );
/* <combine sack::PSI::GetNearControl@PSI_CONTROL@int>
   
   \ \                                                 */
PSI_PROC_PTR( PSI_CONTROL, GetNearControl)( PSI_CONTROL pc, int ID );
/* <combine sack::PSI::GetCommonTextEx@PSI_CONTROL@TEXTSTR@int@int>
   
   \ \                                                              */
PSI_PROC_PTR( void, GetControlTextEx)( PSI_CONTROL pc, TEXTSTR buffer, int buflen, int bCString );
/* <combine sack::PSI::SetControlText@PSI_CONTROL@CTEXTSTR>
   
   \ \                                                      */
PSI_PROC_PTR( void, SetControlText)( PSI_CONTROL pc, CTEXTSTR text );
/* <combine sack::PSI::SetCommonFocus@PSI_CONTROL>
   
   \ \                                             */
PSI_PROC_PTR( void, SetControlFocus)( PSI_CONTROL pf, PSI_CONTROL pc );
/* <combine sack::PSI::EnableControl@PSI_CONTROL@int>
   
   \ \                                                */
PSI_PROC_PTR( void, EnableControl)( PSI_CONTROL pc, int bEnable );
/* <combine sack::PSI::IsControlEnabled@PSI_CONTROL>
   
   \ \                                               */
PSI_PROC_PTR( int, IsControlEnabled)( PSI_CONTROL pc );
/* <combine sack::PSI::GetControlSurface@PSI_CONTROL>
   
   \ \                                                */
PSI_PROC_PTR( Image,GetControlSurface)( PSI_CONTROL pc );
PSI_PROC_PTR( void, SetCommonDraw )( PSI_CONTROL pf, void (CPROC*Draw)( PTRSZVAL, PSI_CONTROL pc ), PTRSZVAL psv );
PSI_PROC_PTR( void, SetCommonKey )( PSI_CONTROL pf, void (CPROC*Key)(PTRSZVAL,_32), PTRSZVAL psv );
PSI_PROC_PTR( void, SetCommonMouse)( PSI_CONTROL pc, void (CPROC*MouseMethod)(PTRSZVAL, S_32 x, S_32 y, _32 b ),PTRSZVAL psv );

/* <combine sack::PSI::UpdateControlEx@PSI_CONTROL pc>
   
   \ \                                                 */
PSI_PROC_PTR( void, UpdateControlEx)( PSI_CONTROL pc DBG_PASS);
/* <combine sack::PSI::GetControlID@PSI_CONTROL>
   
   \ \                                           */
PSI_PROC_PTR( int, GetControlID)( PSI_CONTROL pc );

PSI_PROC_PTR( void, DestroyControlEx)( PSI_CONTROL pc DBG_PASS );
/* <combine sack::PSI::SetNoFocus@PSI_CONTROL>
   
   \ \                                         */
PSI_PROC_PTR( void, SetNoFocus)( PSI_CONTROL pc );
/* <combine sack::PSI::ControlExtraData@PSI_CONTROL>
   
   \ \                                               */
PSI_PROC_PTR( void*, ControlExtraData)( PSI_CONTROL pc );
/* <combine sack::PSI::OrphanCommon@PSI_CONTROL>
   
   \ \                                           */
PSI_PROC_PTR( void, OrphanCommon )( PSI_CONTROL pc );
/* <combine sack::PSI::AdoptCommon@PSI_CONTROL@PSI_CONTROL@PSI_CONTROL>
   
   \ \                                                                  */
PSI_PROC_PTR( void, AdoptCommon )( PSI_CONTROL pFoster, PSI_CONTROL pElder, PSI_CONTROL pOrphan );

/* <combine sack::PSI::AddCommonButtonsEx@PSI_CONTROL@int *@CTEXTSTR@int *@CTEXTSTR>
   
   \ \                                                                               */
PSI_PROC_PTR( void, AddCommonButtonsEx)( PSI_CONTROL pf
                                , int *done, CTEXTSTR donetext
                                , int *okay, CTEXTSTR okaytext );
/* <combine sack::PSI::AddCommonButtons@PSI_CONTROL@int *@int *>
   
   \ \                                                           */
PSI_PROC_PTR( void, AddCommonButtons)( PSI_CONTROL pf, int *done, int *okay );

PSI_PROC_PTR( void, CommonLoop)( int *done, int *okay ); 
/* <combine sack::PSI::ProcessControlMessages>
   
   \ \                                         */
PSI_PROC_PTR( void, ProcessControlMessages)(void);

/* <combine sack::PSI::button::PressButton@PSI_CONTROL@int>
   
   \ \                                                      */
PSI_PROC_PTR( void, PressButton)( PSI_CONTROL pc, int bPressed );
/* <combine sack::PSI::button::IsButtonPressed@PSI_CONTROL>
   
   \ \                                                      */
PSI_PROC_PTR( int, IsButtonPressed)( PSI_CONTROL pc );

/* <combine sack::PSI::button::GetCheckState@PSI_CONTROL>
   
   \ \                                                    */
PSI_PROC_PTR( int, GetCheckState)( PSI_CONTROL pc );
/* <combine sack::PSI::button::SetCheckState@PSI_CONTROL@int>
   
   \ \                                                        */
PSI_PROC_PTR( void, SetCheckState)( PSI_CONTROL pc, int nState );

/* <combine sack::PSI::text::SetTextControlColors@PSI_CONTROL@CDATA@CDATA>
   
   \ \                                                                     */
PSI_PROC_PTR( void, SetTextControlColors )( PSI_CONTROL pc, CDATA fore, CDATA back );

/* <combine sack::PSI::slider::SetSliderValues@PSI_CONTROL@int@int@int>
   
   \ \                                                                  */
PSI_PROC_PTR( void, SetSliderValues)( PSI_CONTROL pc, int min, int current, int max );

/* <combine sack::PSI::colorwell::PickColor@CDATA *@CDATA@PSI_CONTROL>
   
   \ \                                                                 */
PSI_PROC_PTR( int, PickColor)( CDATA *result, CDATA original, PSI_CONTROL pAbove );
/* <combine sack::PSI::font::PickFont@S_32@S_32@P_32@POINTER *@PSI_CONTROL>
   
   \ \                                                                      */
PSI_PROC_PTR( SFTFont, PickFont)( S_32 x, S_32 y
                                  , P_32 size, POINTER *pFontData
                                  , PSI_CONTROL pAbove );
/* <combine sack::PSI::listbox::ResetList@PSI_CONTROL>
   
   \ \                                                 */
PSI_PROC_PTR( void, ResetList)( PSI_CONTROL pc );
/* <combine sack::PSI::listbox::InsertListItem@PSI_CONTROL@PLISTITEM@CTEXTSTR>
   
   \ \                                                                         */
PSI_PROC_PTR( PLISTITEM, InsertListItem)( PSI_CONTROL pc, PLISTITEM prior, CTEXTSTR text );
/* <combine sack::PSI::listbox::InsertListItemEx@PSI_CONTROL@PLISTITEM@int@CTEXTSTR>
   
   \ \                                                                               */
PSI_PROC_PTR( PLISTITEM, InsertListItemEx)( PSI_CONTROL pc, PLISTITEM prior, int nLevel, CTEXTSTR text );
PSI_PROC_PTR( PLISTITEM, AddListItem)( PSI_CONTROL pc, CTEXTSTR text );
/* <combine sack::PSI::listbox::AddListItemEx@PSI_CONTROL@int@CTEXTSTR>
   
   \ \                                                                  */
PSI_PROC_PTR( PLISTITEM, AddListItemEx)( PSI_CONTROL pc, int nLevel, CTEXTSTR text );
/* <combine sack::PSI::listbox::DeleteListItem@PSI_CONTROL@PLISTITEM>
   
   \ \                                                                */
PSI_PROC_PTR( void, DeleteListItem)( PSI_CONTROL pc, PLISTITEM hli );
/* <combine sack::PSI::listbox::SetItemData@PLISTITEM@PTRSZVAL>
   
   \ \                                                          */
PSI_PROC_PTR( void, SetItemData)( PLISTITEM hli, PTRSZVAL psv );
/* <combine sack::PSI::listbox::GetItemData@PLISTITEM>
   
   \ \                                                 */
PSI_PROC_PTR( PTRSZVAL, GetItemData)( PLISTITEM hli );
/* <combine sack::PSI::listbox::GetItemText@PLISTITEM@int@TEXTSTR>
   
   \ \                                                             */
PSI_PROC_PTR( void, GetItemText)( PLISTITEM hli, TEXTSTR buffer, int bufsize );
/* <combine sack::PSI::listbox::GetSelectedItem@PSI_CONTROL>
   
   \ \                                                       */
PSI_PROC_PTR( PLISTITEM, GetSelectedItem)( PSI_CONTROL pc );
PSI_PROC_PTR( void, SetSelectedItem)( PSI_CONTROL pc, PLISTITEM hli );
/* <combine sack::PSI::listbox::SetCurrentItem@PSI_CONTROL@PLISTITEM>
   
   \ \                                                                */
PSI_PROC_PTR( void, SetCurrentItem)( PSI_CONTROL pc, PLISTITEM hli );
/* <combine sack::PSI::listbox::FindListItem@PSI_CONTROL@CTEXTSTR>
   
   \ \                                                             */
PSI_PROC_PTR( PLISTITEM, FindListItem)( PSI_CONTROL pc, CTEXTSTR text );
/* <combine sack::PSI::listbox::GetNthItem@PSI_CONTROL@int>
   
   \ \                                                      */
PSI_PROC_PTR( PLISTITEM, GetNthItem )( PSI_CONTROL pc, int idx );
/* <combine sack::PSI::listbox::SetSelChangeHandler@PSI_CONTROL@SelectionChanged@PTRSZVAL>
   
   \ \                                                                                     */
PSI_PROC_PTR( void, SetSelChangeHandler)( PSI_CONTROL pc, SelectionChanged proc, PTRSZVAL psvUser );
PSI_PROC_PTR( void, SetDoubleClickHandler)( PSI_CONTROL pc, DoubleClicker proc, PTRSZVAL psvUser );

#ifdef __LINUX__
PSI_PROC_PTR(PSI_CONTROL, MakeGridBox)( PSI_CONTROL pf, int options, int x, int y, int w, int h,
                                 int viewport_x, int viewport_y, int total_x, int total_y,
                                 int row_thickness, int column_thickness, PTRSZVAL nID );
#endif
/* <combine sack::PSI::popup::CreatePopup>
   
   \ \                                     */
PSI_PROC_PTR( PMENU, CreatePopup)( void );
/* <combine sack::PSI::popup::DestroyPopup@PMENU>
   
   \ \                                            */
PSI_PROC_PTR( void, DestroyPopup)( PMENU pm );
/* <combine sack::PSI::popup::GetPopupData@PMENU@int>
   
   \ \                                                */
PSI_PROC_PTR( void *,GetPopupData)( PMENU pm, int item );
/* <combine sack::PSI::popup::AppendPopupItem@PMENU@int@PTRSZVAL@CPOINTER>
   
   \ \                                                                     */
PSI_PROC_PTR( PMENUITEM, AppendPopupItem)( PMENU pm, int type, PTRSZVAL dwID, CPOINTER pData );
PSI_PROC_PTR( PMENUITEM, CheckPopupItem)( PMENU pm, _32 dwID, _32 state );
/* <combine sack::PSI::popup::DeletePopupItem@PMENU@_32@_32>
   
   \ \                                                       */
PSI_PROC_PTR( PMENUITEM, DeletePopupItem)( PMENU pm, _32 dwID, _32 state );
PSI_PROC_PTR( int, TrackPopup)( PMENU hMenuSub, PSI_CONTROL parent );
PSI_PROC_PTR( int, PSI_OpenFile)( CTEXTSTR basepath, CTEXTSTR types, CTEXTSTR result );
/* <combine sack::PSI::PSI_OpenFile@CTEXTSTR@CTEXTSTR@TEXTSTR>
   
   \ \                                                         */
PSI_PROC_PTR( int, PSI_OpenFileEx)( CTEXTSTR basepath, CTEXTSTR types, CTEXTSTR result, int Create );
/* <combine sack::PSI::scrollbar::SetScrollParams@PSI_CONTROL@int@int@int@int>
   
   \ \                                                                         */
PSI_PROC_PTR( void, SetScrollParams)( PSI_CONTROL pc, int min, int cur, int range, int max );
PSI_PROC_PTR( PSI_CONTROL, MakeScrollBar)( PSI_CONTROL pf, int x, int y, int w, int h, PTRSZVAL nID, int flags );
/* <combine sack::PSI::scrollbar::SetScrollUpdateMethod@PSI_CONTROL@void __cdecl*UpdateProcPTRSZVAL psv\, int type\, int current@PTRSZVAL>
   
   \ \                                                                                                                                     */
PSI_PROC_PTR( void, SetScrollUpdateMethod)( PSI_CONTROL pc
                    , void (CPROC*UpdateProc)(PTRSZVAL psv, int type, int current)
                    , PTRSZVAL data );
/* <combine sack::PSI::scrollbar::MoveScrollBar@PSI_CONTROL@int>
   
   \ \                                                           */
PSI_PROC_PTR( void, MoveScrollBar )( PSI_CONTROL pc, int type );
/* <combine sack::PSI::SimpleMessageBox@PSI_CONTROL@CTEXTSTR@CTEXTSTR>
   
   \ \                                                                 */
PSI_PROC_PTR( void, SimpleMessageBox )( PSI_CONTROL parent, CTEXTSTR title, CTEXTSTR content );
/* <combine sack::PSI::HideFrame@PSI_CONTROL>
   
   \ \                                        */
PSI_PROC_PTR( void, HideFrame )( PSI_CONTROL pf );
/* <combine sack::PSI::UpdateCommonEx@PSI_CONTROL@int bDraw>
   
   \ \                                                       */
PSI_PROC_PTR( void, UpdateCommonEx )( PSI_CONTROL pc, int bDraw );
};
/* Type that is a pointer to a control interface. */
typedef struct control_interface_tag *PCONTROL_INTERFACE;

/* Gets the interface. */
PCONTROL_INTERFACE GetControlInterface( void );
/* Releases an interface when you are done with it. */
void DropControlInterface( void );

#ifdef USE_CONTROL_INTERFACE

#define ALIAS_WRAPPER(name)   ( (USE_CONTROL_INTERFACE)->(name))

#define SetControlInterface         ALIAS_WRAPPER(SetControlInterface)
#define SetControlImageInterface    ALIAS_WRAPPER(SetControlImageInterface)

#define AlignBaseToWindows          ALIAS_WRAPPER(AlignBaseToWindows)
// see indexes above.
#define SetBaseColor                ALIAS_WRAPPER(SetBaseColor)
#define GetBaseColor                ALIAS_WRAPPER(GetBaseColor)

//-------- Frame and generic control functions --------------
#define CreateFrame                 ALIAS_WRAPPER(CreateFrame)

                           
#define DestroyFrameEx                ALIAS_WRAPPER(DestroyFrameEx)
#define DisplayFrame                ALIAS_WRAPPER(DisplayFrame)
#define HideFrame                   ALIAS_WRAPPER(HideFrame)
#define SizeFrame                   ALIAS_WRAPPER(SizeFrame)
#define MoveFrame                   ALIAS_WRAPPER(MoveFrame)
#define MoveSizeFrame               ALIAS_WRAPPER(MoveSizeFrame)
#define SizeFrameRel                ALIAS_WRAPPER(SizeFrameRel)
#define MoveFrameRel                ALIAS_WRAPPER(MoveFrameRel)
#define MoveSizeFrameRel            ALIAS_WRAPPER(MoveSizeFrameRel)
#define GetControl                  ALIAS_WRAPPER(GetControl)
#define UpdateFrameEx                 ALIAS_WRAPPER(UpdateFrameEx)
#define SetFrameMousePosition       ALIAS_WRAPPED(SetFrameMousePosition)
//PSI_PROC_PTR void SetDefaultOkayID( PSI_CONTROL pFrame, int nID );
//PSI_PROC_PTR void SetDefaultCancelID( PSI_CONTROL pFrame, int nID );

//-------- Generic control functions --------------
#define GetFrame                    ALIAS_WRAPPER(GetFrame)
#define GetNearControl              ALIAS_WRAPPER(GetNearControl)
#define GetControlTextEx            ALIAS_WRAPPER(GetControlTextEx)
#define SetControlText              ALIAS_WRAPPER(SetControlText)
#define SetControlFocus             ALIAS_WRAPPER(SetControlFocus)
#define EnableControl               ALIAS_WRAPPER(EnableControl)
#define IsControlEnabled            ALIAS_WRAPPER(IsControlEnabled)
#define GetControlSurface           ALIAS_WRAPPER(GetControlSurface)
#define SetFrameDraw                ALIAS_WRAPPER(SetFrameDraw)
#define SetFrameMouse               ALIAS_WRAPPER(SetFrameMouse)
#define SetFrameKey                 ALIAS_WRAPPER(SetFrameKey)
#define SetControlDraw              ALIAS_WRAPPER(SetControlDraw)
#define SetControlMouse             ALIAS_WRAPPER(SetControlMouse)
#define UpdateControl               ALIAS_WRAPPER(UpdateControl)
#define UpdateControlEx             ALIAS_WRAPPER(UpdateControlEx)
#define GetControlID                ALIAS_WRAPPER(GetControlID)

#define DestroyControlEx            ALIAS_WRAPPER(DestroyControlEx)
#define SetNoFocus                  ALIAS_WRAPPER(SetNoFocus)
#define ControlExtraData            ALIAS_WRAPPER(ControlExtraData)
#define OrphanCommon                ALIAS_WRAPPER(OrphanCommon)
#define AdoptCommon                 ALIAS_WRAPPER(AdoptCommon)
//------ General Utilities ------------
#define AddCommonButtonsEx          ALIAS_WRAPPER(AddCommonButtonsEx)
#define AddCommonButtons            ALIAS_WRAPPER(AddCommonButtons)

#define CommonLoop                  ALIAS_WRAPPER(CommonLoop)
#define ProcessControlMessages      ALIAS_WRAPPER(ProcessControlMessages)
//------ BUTTONS ------------
#define MakeButton                  ALIAS_WRAPPER(MakeButton)
#define MakeImageButton             ALIAS_WRAPPER(MakeImageButton)
#define MakeCustomDrawnButton       ALIAS_WRAPPER(MakeCustomDrawnButton)
#define PressButton                 ALIAS_WRAPPER(PressButton)
#define IsButtonPressed             ALIAS_WRAPPER(IsButtonPressed)

#define MakeCheckButton             ALIAS_WRAPPER(MakeCheckButton)
#define MakeRadioButton             ALIAS_WRAPPER(MakeRadioButton)
#define GetCheckState               ALIAS_WRAPPER(GetCheckState)
#define SetCheckState               ALIAS_WRAPPER(SetCheckState)

//------ Static Text -----------
#define MakeTextControl             ALIAS_WRAPPER(MakeTextControl)
#define SetTextControlColors        ALIAS_WRAPPER(SetTextControlColors)
//------- Edit Control ---------
#define MakeEditControl             ALIAS_WRAPPER(MakeEditControl)
// Use GetContrcolText/SetControlText

//------- Slider Control --------
#define MakeSlider                  ALIAS_WRAPPER(MakeSlider)
#define SetSliderValues             ALIAS_WRAPPER(SetSliderValues)

//------- Color Control --------
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
#define PickColor                   ALIAS_WRAPPER(PickColor)
//------- SFTFont Control --------
#define PickFont                    ALIAS_WRAPPER(PickFont)

//------- ListBox Control --------
#define MakeListBox                 ALIAS_WRAPPER(MakeListBox)

                           
#define ResetList                   ALIAS_WRAPPER(ResetList)
#define InsertListItem              ALIAS_WRAPPER(InsertListItem)
#define InsertListItemEx            ALIAS_WRAPPER(InsertListItemEx)
#define AddListItem                 ALIAS_WRAPPER(AddListItem)
#define AddListItemEx               ALIAS_WRAPPER(AddListItemEx)
#define DeleteListItem              ALIAS_WRAPPER(DeleteListItem)
#define SetItemData                 ALIAS_WRAPPER(SetItemData)
#define GetItemData                 ALIAS_WRAPPER(GetItemData)
#define GetItemText                 ALIAS_WRAPPER(GetItemText)
#define GetSelectedItem             ALIAS_WRAPPER(GetSelectedItem)
#define SetSelectedItem             ALIAS_WRAPPER(SetSelectedItem)
#define SetCurrentItem              ALIAS_WRAPPER(SetCurrentItem)
#define FindListItem                ALIAS_WRAPPER(FindListItem)
#define GetNthItem                  ALIAS_WRAPPER(GetNthItem)
#define SetSelChangeHandler         ALIAS_WRAPPER(SetSelChangeHandler )
#define SetDoubleClickHandler       ALIAS_WRAPPER(SetDoubleClickHandler)
//------- GridBox Control --------
#define MakeGridBox                 ALIAS_WRAPPER(MakeGridBox)
    //------- Popup Menus ------------
// popup interface.... these are mimics of windows... 
#define CreatePopup                 ALIAS_WRAPPER(CreatePopup)
#define DestroyPopup                ALIAS_WRAPPER(DestroyPopup)
#define GetPopupData                ALIAS_WRAPPER(GetPopupData)
#define AppendPopupItem             ALIAS_WRAPPER(AppendPopupItem)
#define CheckPopupItem              ALIAS_WRAPPER(CheckPopupItem)
#define DeletePopupItem             ALIAS_WRAPPER(DeletePopupItem)
#define TrackPopup                  ALIAS_WRAPPER(TrackPopup)
//------- Color Control --------
// these are basic basic file selection dialogs... 
// the concept is valid, and they should be common like controls...
#define PSI_OpenFile                ALIAS_WRAPPER(PSI_OpenFile)
// this may be used for save I think....
#define PSI_OpenFileEx              ALIAS_WRAPPER(PSI_OpenFileEx)
//------- Scroll Control --------
#define SetScrollParams             ALIAS_WRAPPER(SetScrollParams)
#define MakeScrollBar               ALIAS_WRAPPER(MakeScrollBar)
#define SetScrollUpdateMethod       ALIAS_WRAPPER(SetScrollUpdateMethod)

#endif


#ifdef BASE_CONTROL_MESSAGE_ID
// need to define BASE_IMAGE_MESSAGE_ID before hand to determine what the base message is.
#define MSG_ID(method)  ( ( offsetof( struct control_interface_tag, method ) / sizeof( void (CPROC*)(void) ) ) + BASE_CONTROL_MESSAGE_ID + MSG_EventUser )

#define MSG_SetControlInterface         MSG_ID(SetControlInterface)
#define MSG_SetControlImageInterface    MSG_ID(SetControlImageInterface)

#define MSG_AlignBaseToWindows          MSG_ID(AlignBaseToWindows)
// see indexes above.
#define MSG_SetBaseColor                MSG_ID(SetBaseColor)
#define MSG_GetBaseColor                MSG_ID(GetBaseColor)

//-------- Frame and generic control functions --------------
#define MSG_CreateFrame                 MSG_ID(CreateFrame)

                           
#define MSG_DestroyFrameEx                MSG_ID(DestroyFrameEx)
#define MSG_DisplayFrame                MSG_ID(DisplayFrame)
#define MSG_HideFrame                   MSG_ID(HideFrame)
#define MSG_SizeCommon                   MSG_ID(SizeCommon)
#define MSG_MoveCommon                   MSG_ID(MoveCommon)
#define MSG_MoveSizeCommon               MSG_ID(MoveSizeCommon)
#define MSG_SizeCommonRel                   MSG_ID(SizeCommonRel)
#define MSG_MoveCommonRel                   MSG_ID(MoveCommonRel)
#define MSG_MoveSizeCommonRel               MSG_ID(MoveSizeCommonRel)
#define MSG_GetControl                  MSG_ID(GetControl)
#define MSG_UpdateFrame                 MSG_ID(UpdateFrame)
#define MSG_SetFrameMousePosition       MSG_ID(SetFrameMousePosition)
//PSI_PROC_PTR void SetDefaultOkayID( PSI_CONTROL pFrame, int nID );
//PSI_PROC_PTR void SetDefaultCancelID( PSI_CONTROL pFrame, int nID );

//-------- Generic control functions --------------
#define MSG_GetFrame                    MSG_ID(GetFrame)
#define MSG_GetNearControl              MSG_ID(GetNearControl)
#define MSG_GetControlTextEx            MSG_ID(GetControlTextEx)
#define MSG_SetControlText              MSG_ID(SetControlText)
#define MSG_SetControlFocus             MSG_ID(SetControlFocus)
#define MSG_EnableControl               MSG_ID(EnableControl)
#define MSG_IsControlEnabled            MSG_ID(IsControlEnabled)
#define MSG_GetControlSurface           MSG_ID(GetControlSurface)
#define MSG_SetFrameDraw                MSG_ID(SetFrameDraw)
#define MSG_SetFrameMouse               MSG_ID(SetFrameMouse)
#define MSG_SetFrameKey                 MSG_ID(SetFrameKey)
#define MSG_SetControlDraw              MSG_ID(SetControlDraw)
#define MSG_SetControlMouse             MSG_ID(SetControlMouse)
#define MSG_UpdateControl               MSG_ID(UpdateControl)
#define MSG_UpdateControlEx             MSG_ID(UpdateControlEx)
#define MSG_GetControlID                MSG_ID(GetControlID)

#define MSG_DestroyControlEx            MSG_ID(DestroyControlEx)
#define MSG_SetNoFocus                  MSG_ID(SetNoFocus)
#define MSG_ControlExtraData            MSG_ID(ControlExtraData)
#define MSG_OrphanCommon                MSG_ID(OrphanCommon)
#define MSG_AdoptCommon                 MSG_ID(AdoptCommon)

//------ General Utilities ------------
#define MSG_AddCommonButtonsEx          MSG_ID(AddCommonButtonsEx)
#define MSG_AddCommonButtons            MSG_ID(AddCommonButtons)

#define MSG_CommonLoop                  MSG_ID(CommonLoop)
#define MSG_ProcessControlMessages      MSG_ID(ProcessControlMessages)
//------ BUTTONS ------------
#define MSG_MakeButton                  MSG_ID(MakeButton)
#define MSG_MakeImageButton             MSG_ID(MakeImageButton)
#define MSG_MakeCustomDrawnButton       MSG_ID(MakeCustomDrawnButton)
#define MSG_PressButton                 MSG_ID(PressButton)
#define MSG_IsButtonPressed             MSG_ID(IsButtonPressed)

#define MSG_MakeCheckButton             MSG_ID(MakeCheckButton)
#define MSG_MakeRadioButton             MSG_ID(MakeRadioButton)
#define MSG_GetCheckState               MSG_ID(GetCheckState)
#define MSG_SetCheckState               MSG_ID(SetCheckState)

//------ Static Text -----------
#define MSG_MakeTextControl             MSG_ID(MakeTextControl)
#define MSG_SetTextControlColors        MSG_ID(SetTextControlColors)

//------- Edit Control ---------
#define MSG_MakeEditControl             MSG_ID(MakeEditControl)
// Use GetContrcolText/SetControlText

//------- Slider Control --------
#define MSG_MakeSlider                  MSG_ID(MakeSlider)
#define MSG_SetSliderValues             MSG_ID(SetSliderValues)

//------- Color Control --------
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
#define MSG_PickColor                   MSG_ID(PickColor)
//------- SFTFont Control --------
#define MSG_PickFont                    MSG_ID(PickFont)

//------- ListBox Control --------
#define MSG_MakeListBox                 MSG_ID(MakeListBox)

                           
#define MSG_ResetList                   MSG_ID(ResetList)
#define MSG_InsertListItem              MSG_ID(InsertListItem)
#define MSG_InsertListItemEx            MSG_ID(InsertListItemEx)
#define MSG_AddListItem                 MSG_ID(AddListItem)
#define MSG_AddListItemEx               MSG_ID(AddListItemEx)
#define MSG_DeleteListItem              MSG_ID(DeleteListItem)
#define MSG_SetItemData                 MSG_ID(SetItemData)
#define MSG_GetItemData                 MSG_ID(GetItemData)
#define MSG_GetItemText                 MSG_ID(GetItemText)
#define MSG_GetSelectedItem             MSG_ID(GetSelectedItem)
#define MSG_SetSelectedItem             MSG_ID(SetSelectedItem)
#define MSG_SetCurrentItem              MSG_ID(SetCurrentItem)
#define MSG_FindListItem                MSG_ID(FindListItem)
#define MSG_GetNthItem                  MSG_ID(GetNthItem)
#define MSG_SetSelChangeHandler         MSG_ID(SetSelChangeHandler)
#define MSG_SetDoubleClickHandler       MSG_ID(SetDoubleClickHandler)
//------- GridBox Control --------
#define MSG_MakeGridBox                 MSG_ID(MakeGridBox)
    //------- Popup Menus ------------
// popup interface.... these are mimics of windows... 
#define MSG_CreatePopup                 MSG_ID(CreatePopup)
#define MSG_DestroyPopup                MSG_ID(DestroyPopup)
#define MSG_GetPopupData                MSG_ID(GetPopupData)
#define MSG_AppendPopupItem             MSG_ID(AppendPopupItem)
#define MSG_CheckPopupItem              MSG_ID(CheckPopupItem)
#define MSG_DeletePopupItem             MSG_ID(DeletePopupItem)
#define MSG_TrackPopup                  MSG_ID(TrackPopup)
//------- Color Control --------
// these are basic basic file selection dialogs... 
// the concept is valid, and they should be common like controls...
#define MSG_PSI_OpenFile                MSG_ID(PSI_OpenFile)
// this may be used for save I think....
#define MSG_PSI_OpenFileEx              MSG_ID(PSI_OpenFileEx)
//------- Scroll Control --------
#define MSG_SetScrollParams             MSG_ID(SetScrollParams)
#define MSG_MakeScrollBar               MSG_ID(MakeScrollBar)
#define MSG_SetScrollUpdateMethod       MSG_ID(SetScrollUpdateMethod)

#endif
_PSI_INTERFACE_NAMESPACE_END


#define GetFrameSurface GetControlSurface

PSI_NAMESPACE_END

USE_PSI_NAMESPACE

#include <psi/edit.h>
#include <psi/buttons.h>
#include <psi/slider.h>
//#include <psi/controls.h>
#include <psi/shadewell.h>


#endif
