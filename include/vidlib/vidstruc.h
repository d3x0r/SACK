#ifndef VIDEO_STRUCTURE_DEFINED
#define VIDEO_STRUCTURE_DEFINED

#if defined( __LINUX__ ) && defined( PURE_OPENGL_ENABLED )
#include <stdio.h>
#if defined( USE_GLES2 )
#define USE_EGL
#include <EGL/egl.h>
#ifdef _egl_h
#define _GLES_EGL_H_INCLUDED
#elif defined( _GLES_EGL_H_INCLUDED )
#define _egl_h
#endif
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/keysym.h>
#endif

#endif

#if defined( __QNX__ )
#include <gf/gf.h>
#include <gf/gf3d.h>
#endif


#ifndef RENDER_NAMESPACE
# ifdef __cplusplus
#ifdef _D3D_DRIVER
#  define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d {
#  define RENDER_NAMESPACE_END }}}}
#else
#  define RENDER_NAMESPACE namespace sack { namespace image { namespace render {
#  define RENDER_NAMESPACE_END }}}
#endif
# else
#  define RENDER_NAMESPACE
#  define RENDER_NAMESPACE_END
# endif
#endif


#include <imglib/imagestruct.h>


#ifdef WIN32
#  ifndef _ARM_
#    include <gl\gl.h>         // Header File For The OpenGL32 Library
#    include <gl\glu.h>        // Header File For The GLu32 Library
#  else
#    define __NO_OPENGL__
#  endif
#endif

#include <sack_types.h>
#include <timers.h> // criticalsection
#include <vidlib/keydef.h>

#include <render.h>
#include <keybrd.h>




RENDER_NAMESPACE

//DOM-IGNORE-BEGIN
 /* this will probably break when we try to build without OPenGL */
typedef struct PBOInfo
{
	int index; // increment to flop between pboIds
   /* the next pixel buffer object to draw into. */
   int nextIndex;
#define PBO_COUNT 2
#ifndef __NO_OPENGL__
# ifdef WIN32
	 GLuint pboIds[PBO_COUNT];           // IDs of PBOs
# endif
#endif
	 /* buffer to read into. */
	 Image dest_buffer;
	 PCDATA raw; // the raw pixels mapped from the card... re-stuffed into an image... which then must be moved to a context.
} PBO_Info, *PPBO_Info;
//DOM-IGNORE-END

#  ifdef __LINUX__
#    define HDC void*
#    define HGLRC void*
#    define HWND void*
#  endif


#ifdef __LINUX__
/* stuff about our window grouped together */
typedef struct {
#if !defined( __ANDROID__ ) && !defined( __QNX__ )
	Display *dpy;
    Window win;
    GLXContext ctx;
    XSetWindowAttributes attr;
    Bool fs;
    Bool doubleBuffered;
    XF86VidModeModeInfo deskMode;
    Atom atom_create;
#endif
    int screen;
    int x, y;
    unsigned int width, height;
    unsigned int depth;
    PTHREAD pThread;  //make sure we use the correct thread for getting events
    int mouse_b, mouse_y, mouse_x;
    int _mouse_b, _mouse_y, _mouse_x;
    int real_mouse_x, real_mouse_y;
} GLWindow;
#endif

#if defined( __ANDROID__ ) || defined( __LINUX__ )
typedef struct WindowPos
{
   int x, y, cx, cy;
} WINDOWPOS;
#endif

/* Private structure for Vidlib. See PRENDERER. Exposed, but
   applications should use appropriate methods in render
   namespace.                                                */
typedef struct HVIDEO_tag
{
//DOM-IGNORE-BEGIN
   KEYBOARD kbd;
	PKEYDEFINE KeyDefs;
   CRITICALSECTION cs;
	struct ImageFile_tag *pImage;
   TEXTCHAR *pTitle; // window title... need this if we draw manually anyhow
   // this is the thread that created the hwndoutput (events get dispatched to this.)
   PTHREAD pThreadWnd;
   struct {
	   S_32 x;
	   S_32 y;
   } cursor_bias;
   WINDOWPOS pWindowPos;  // should always contain current information.
#ifdef _WIN32
	HWND hWndOutput;
	// do opengl rendering to this... then move from window to window the updated stuff for layered window openGL junk.
   HWND hWndOutputFake;

   PLIST dropped_file_acceptors;
	HDC hDCOutput; // handle to the window....
#elif defined( __LINUX__ ) && defined( PURE_OPENGL_ENABLED )
#define CW_USEDEFAULT 0x40000000
	GLWindow *x11_gl_window;
	struct my_windowpos_clone_tag {
		S_32 x, y;
      _32 cx, cy;
	} WindowPos;
#endif
#  ifdef _OPENGL_ENABLED
	int nFractures, nFracturesAvail;
	int _prior_fracture;
	struct fracture_tag{
		int x, y, w, h;
		void *hBm;
		void *hOldBitmap;
		void *hDCBitmap;
		HWND hWndFakeWindow;
		HDC hDCFakeWindow;
		struct ImageFile_tag *pImage;
		HGLRC    hRC;     // Permanent Rendering Context
	} *pFractures;
#endif
#  if defined( PURE_OPENGL_ENABLED ) || defined( PURE_D3D_ENABLED )
	struct display_camera *camera;
	MATRIX fModelView;
	PTRANSFORM transform;
#    if defined( __QNX__ )
	gf_surface_t pSurface;
    gf_layer_t pLayer;
	gf_layer_info_t layer_info;
	gf_3d_target_t pTarget;
	EGLint num_config;
	EGLConfig config;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext econtext;

#    endif

#  endif
#  ifdef _OPENGL_DRIVER
	Image pAppImage; // this is the image returned for the application's reference.  The real image is a larger surface than this.
   GLuint		texture[1]; // texture that is this real image...
#  endif
#  ifdef _D3D_DRIVER
#  endif
#ifdef _WIN32
#  ifndef _OPENGL_DRIVER
	HBITMAP hBm;
	HDC hDCBitmap; // hdcMem
	HDC hDCFakeWindow;
	HDC hDCFakeBitmap; // compatible dc with window, not the window, and selected bitmap
	HBITMAP hOldFakeBm;
   PPBO_Info PBO;
#  endif
#endif
	THREAD_ID thread;
	struct {
		S_32 x, y;
      _32 b;
	} mouse;

   _32 idle_timer_id;
   MouseCallback pMouseCallback;
   PTRSZVAL  dwMouseData;
   HideAndRestoreCallback pHideCallback;
   PTRSZVAL  dwHideData;
   HideAndRestoreCallback pRestoreCallback;
   PTRSZVAL  dwRestoreData;

#if !defined( NO_TOUCH )
	TouchCallback pTouchCallback;
   PTRSZVAL dwTouchData;
#endif
   RedrawCallback pRedrawCallback; 
   PTRSZVAL dwRedrawData;

   CloseCallback pWindowClose;
   PTRSZVAL dwCloseData;

   KeyProc pKeyProc;
   PTRSZVAL dwKeyData;

   RenderReadCallback ReadComplete;
   PTRSZVAL psvRead;
   PLINKQUEUE pInput;

   struct {
      BIT_FIELD  bExternalImage:1; // locks the frame from being resized...
      BIT_FIELD  bShown:1; // can keep the window invisible until we draw
      BIT_FIELD  bFull:1;
      BIT_FIELD  bReady:1; // this video structure is initialized and ready.
      BIT_FIELD  bFocused : 1;
		BIT_FIELD  bExclusive : 1;
		BIT_FIELD bTopmost : 1;
		BIT_FIELD bAbsoluteTopmost : 1;
		BIT_FIELD mouse_pending : 1;
		BIT_FIELD key_dispatched : 1;
		BIT_FIELD bDestroy : 1; // set during destryction process...
		BIT_FIELD bInDestroy : 1;
		BIT_FIELD event_dispatched : 1;
		BIT_FIELD bHidden : 1;
		BIT_FIELD bCaptured : 1;
		//BIT_FIELD bShowing : 1;
		BIT_FIELD bHiding : 1;
		BIT_FIELD bHidden_while_showing : 1;
		BIT_FIELD bShown_while_hiding : 1;
		BIT_FIELD bOpenGL : 1; // delay updates to video thread - just post invalidate...
		BIT_FIELD bLayeredWindow : 1;
		BIT_FIELD bChildWindow : 1; // when used with layered, also sets TOOLWINDOW so it's not a target for alt-tab, and no taskbar icon
		BIT_FIELD bNoMouse : 1; // when used with bLayeredWindow, all mouse is passed to under form.
		BIT_FIELD bAnchored : 1; // if anchored, ignore NC_MOUSEACTIVATE
		BIT_FIELD bOrdering : 1; // if ordering, don't attemopt to re-order...
		BIT_FIELD bOpenedBehind : 1; // ordered at startup to be behind a barrier window
		BIT_FIELD bIdleMouse : 1; // this window wants mouse to disappear at idle.
		BIT_FIELD bIgnoreChanging : 1; // one shot ignore change set by paint.... 
		BIT_FIELD bDeferedPos : 1; // while in defered posisitiongin, windows marked with this.
		BIT_FIELD bNoAutoFocus : 1; // foreground/setfocus are not called on initial show.
		BIT_FIELD bForceTopmost : 1; // when removed from very top, put self back at very top.
		BIT_FIELD mouse_on : 1; // an indicator of whether we have set the mouse for this window yet (may have been set by forces outside)
		BIT_FIELD bD3D : 1; // delay updates to video thread - just post invalidate...
		BIT_FIELD bRestoring : 1; // set during SW_RESTORE operation (kills resizes, which happen when expanding the screen)
		BIT_FIELD bUpdated : 1;
		BIT_FIELD bForceSurfaceUpdate : 1; // set when we change size.
	} flags;

   struct HVIDEO_tag *pNext, *pPrior;
   struct HVIDEO_tag *pAbove  // this is above specified window
      , *pBelow;  // this is below specified window

   LoseFocusCallback pLoseFocus;
   PTRSZVAL dwLoseFocus;
   

   void *hOldBitmap;

#ifdef _OPENGL_ENABLED
   HGLRC    hRC;     // Permanent Rendering Context
#endif
#ifdef _D3D_ENABLED
    LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
	LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
// function prototypes
//void initD3D(HWND hWnd);    // sets up and initializes Direct3D
//void render_frame(void);    // renders a single frame
//void cleanD3D(void);    // closes Direct3D and releases memory
#endif
	PLIST sprites; // list of methods to draw sprites on this surface.
#ifdef WIN32
	HWND hWndContainer;
#endif
	struct HVIDEO_tag *under;
	struct HVIDEO_tag *over;
#ifdef WIN32
	HWND hDeferedAfter;
#endif
   int fade_alpha; // actually only 0-255 value... passed to transparency layered window...

   // this is the structure of the bitmap created the uhmm window drawing surface just before the window surface itself.
	//BITMAPINFO bmInfo;
   //HDRAWDIB hDd;
//DOM-IGNORE-END

   int mouse_b, mouse_y, mouse_x;
   int _mouse_b, _mouse_y, _mouse_x;
   int real_mouse_x, real_mouse_y;

} VIDEO, *PVIDEO;

RENDER_NAMESPACE_END

#endif

// $Log: vidstruc.h,v $
// Revision 1.14  2005/03/28 09:44:12  panther
// Use single surface to project surround-o-vision.  This btw has the benefit of uniform output.
//
// Revision 1.13  2005/03/13 23:34:35  panther
// Focus and mouse capture issues resolved for windows libraries... need to tinker with this same function within Linux.
//
// Revision 1.12  2005/03/13 10:02:39  panther
// Define hidden flag for video surfaces.
//
// Revision 1.11  2004/12/19 15:44:57  panther
// Extended set methods to interact with raw index numbers
//
// Revision 1.10  2004/12/13 11:08:17  panther
// Checkpoint, minor tweaks
//
// Revision 1.9  2004/12/04 01:09:20  panther
// Set destroy flag to avoid dispatch killfocus during destruction
//
// Revision 1.8  2004/10/02 19:49:54  d3x0r
// Fix logging... trying to track down multiple update display issues.... keys are queued, events are locally queued...
//
// Revision 1.7  2004/09/30 01:14:47  d3x0r
// Cleaned up consistancy of PID and thread ID... extended message service a bit to supply event PID both ways.
//
// Revision 1.6  2004/09/23 00:36:00  d3x0r
// Implement key and mouse messages across message system... soon to allow applications to catch these directly instead of callback methods.
//
// Revision 1.5  2004/09/22 20:28:44  d3x0r
// Updated to have a pid
//
// Revision 1.4  2004/09/01 03:27:20  d3x0r
// Control updates video display issues?  Image blot message go away...
//
// Revision 1.3  2004/08/11 12:00:37  d3x0r
// Migrate to new, more common keystruct...
//
// Revision 1.2  2004/08/11 11:41:07  d3x0r
// Begin seperation of key and render
//
// Revision 1.1  2004/06/21 07:47:37  d3x0r
// Checkpoint - make system rouding out nicly.
//
// Revision 1.16  2004/06/21 07:39:36  d3x0r
// use moved include files for image structures
//
// Revision 1.15  2004/06/16 21:13:53  d3x0r
// Cleanups for keybind duplication between here and vidlib... that's all been cleaned...
//
// Revision 1.14  2004/05/03 06:15:39  d3x0r
// Define buffered render read
//
// Revision 1.13  2004/05/02 05:44:38  d3x0r
// Implement  BindEventToKey and UnbindKey
//
// Revision 1.12  2004/04/29 11:16:41  d3x0r
// Reformat, collect global data into a structure
//
// Revision 1.11  2004/04/27 21:14:30  d3x0r
// Fix nasty thread problem
//
// Revision 1.10  2004/04/27 09:55:11  d3x0r
// Add keydef to keyhandler path
//
// Revision 1.9  2003/09/01 20:04:37  panther
// Added OpenGL Interface to windows video lib, Modified RCOORD comparison
//
// Revision 1.8  2003/03/29 22:52:00  panther
// New render/image layering ability.  Added support to Display for WIN32 usage (native not SDL)
//
// Revision 1.7  2003/03/29 17:21:06  panther
// Focus problems, mouse message problems resolved... Focus works through to the client side now
//
// Revision 1.6  2003/03/25 20:41:00  panther
// Fix what CVS logging addition broke
//
// Revision 1.5  2003/03/25 08:45:58  panther
// Added CVS logging tag
//
