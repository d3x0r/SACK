#ifndef VIDEO_STRUCTURE_DEFINED
#define VIDEO_STRUCTURE_DEFINED

#if defined( __LINUX__ )
#  include <stdio.h>
#  if defined( USE_GLES2 )
#    ifndef USE_EGL
#      define USE_EGL
#    endif
#      include <EGL/egl.h>
#      ifdef _egl_h
#        define _GLES_EGL_H_INCLUDED
#      elif defined( _GLES_EGL_H_INCLUDED )
#        define _egl_h
#        endif
//#include <GLES/gl.h>
#      include <GLES2/gl2.h>
#    else
#      if defined( __MAC__ )
//#        include <glx.h>
#        include <gl.h>
#        include <glu.h>
#      else
#        include <GL/glx.h>
#        include <GL/gl.h>
#        include <GL/glu.h>
#        include <X11/Xlib.h>
#        include <X11/extensions/xf86vmode.h>
#        include <X11/keysym.h>
#    endif
#  endif
#endif

#if defined( _D3D10_DRIVER )
#include <D3D10_1.h>
#include <D2D1.h>
#include <wincodec.h>
#endif

#if defined( __QNX__ )
#include <gf/gf.h>
#include <gf/gf3d.h>
#endif


#ifndef RENDER_NAMESPACE
# ifdef __cplusplus
#if defined( _D3D_DRIVER )
#  define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d {
#  define RENDER_NAMESPACE_END }}}}
#elif defined( _D3D10_DRIVER )
#  define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d10 {
#  define RENDER_NAMESPACE_END }}}}
#elif defined( _D3D11_DRIVER )
#  define RENDER_NAMESPACE namespace sack { namespace image { namespace render { namespace d3d11 {
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
#    include <GL/gl.h>         // Header File For The OpenGL32 Library
#    include <GL/glu.h>        // Header File For The GLu32 Library
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
	GLuint pboIds[PBO_COUNT];      // IDs of PBOs
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

#if defined( MINGW_SUX ) && !defined( __ANDROID__ ) && !defined( __QNX__ )
typedef struct tagUPDATELAYEREDWINDOWINFO {
    DWORD               cbSize;
    HDC                 hdcDst;
    POINT CONST         *pptDst;
    SIZE CONST          *psize;
    HDC                 hdcSrc;
    POINT CONST         *pptSrc;
    COLORREF            crKey;
    BLENDFUNCTION CONST *pblend;
    DWORD               dwFlags;
    RECT CONST          *prcDirty;
} UPDATELAYEREDWINDOWINFO;
#endif

#if defined( __ANDROID__ ) || defined( __LINUX__ )
typedef struct WindowPos
{
	int x, y, cx, cy;
} WINDOWPOS;
#endif

/* Private structure for Vidlib. See PRENDERER. Exposed, but
	applications should use appropriate methods in render
	namespace.																*/
typedef struct HVIDEO_tag
{
//DOM-IGNORE-BEGIN
	KEYBOARD kbd;
	PKEYDEFINE KeyDefs;
	CRITICALSECTION cs;
	struct ImageFile_tag *pImage;
	struct ImageFile_tag *pImageLayeredStretch;
	TEXTCHAR *pTitle; // window title... need this if we draw manually anyhow
	// this is the thread that created the hwndoutput (events get dispatched to this.)
	PTHREAD pThreadWnd;
	struct {
		int32_t x;
		int32_t y;
	} cursor_bias;
	WINDOWPOS pWindowPos;  // should always contain current information.
#ifdef _WIN32
	HWND hWndOutput;
	// do opengl rendering to this... then move from window to window the updated stuff for layered window openGL junk.
	HWND hWndOutputFake;

	PLIST dropped_file_acceptors;
	HDC hDCOutput; // handle to the window....
#elif defined( __LINUX__ )
#define CW_USEDEFAULT 0x40000000
	GLWindow *x11_gl_window;
	struct my_windowpos_clone_tag {
		int32_t x, y;
		uint32_t cx, cy;
	} WindowPos;
#endif
#ifdef _OPENGL_ENABLED
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
		HGLRC	hRC;	 // Permanent Rendering Context
	} *pFractures;
#endif

#  if defined( __3D__ )
	struct display_camera *camera;
	MATRIX fModelView;
	PTRANSFORM transform;
#  endif

#  ifdef _OPENGL_DRIVER
	Image pAppImage; // this is the image returned for the application's reference.  The real image is a larger surface than this.
	GLuint		texture[1]; // texture that is this real image...
#  endif
#ifdef _WIN32
#  ifndef _OPENGL_DRIVER
	HBITMAP hBm;
	HBITMAP hBmFullScreen;
	HDC hDCBitmap; // hdcMem
	HDC hDCBitmapFullScreen; // hdcMem
	/* used for OpenGL Output buffering; not sure it's required...*/
	HDC hDCFakeWindow;
	HDC hDCFakeBitmap; // compatible dc with window, not the window, and selected bitmap
	HBITMAP hOldFakeBm;
	//PPBO_Info PBO;
	UPDATELAYEREDWINDOWINFO ULWInfo;
	SIZE size;  // window size passed to ULWInfo
	POINT topPos;// window pos passed to ULWInfo
#  endif
#endif
	PTHREAD thread;
	struct {
		int32_t x, y;
		uint32_t b;
	} mouse;

	uintptr_t idle_timer_id;
	uintptr_t top_force_timer_id;
	MouseCallback pMouseCallback;
	uintptr_t  dwMouseData;
	HideAndRestoreCallback pHideCallback;
	uintptr_t  dwHideData;
	HideAndRestoreCallback pRestoreCallback;
	uintptr_t  dwRestoreData;

#if !defined( NO_TOUCH )
	TouchCallback pTouchCallback;
	uintptr_t dwTouchData;
#endif
	RedrawCallback pRedrawCallback;
	uintptr_t dwRedrawData;

	CloseCallback pWindowClose;
	uintptr_t dwCloseData;

	KeyProc pKeyProc;
	uintptr_t dwKeyData;

	RenderReadCallback ReadComplete;
	uintptr_t psvRead;
	PLINKQUEUE pInput;

	struct {
		BIT_FIELD  bExternalImage:1; // locks the frame from being resized...
		BIT_FIELD  bShown:1; // can keep the window invisible until we draw
		BIT_FIELD  bFull:1;  // Use the full window space, otherwise use client space inside window.
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
		BIT_FIELD bRendering : 1; // while rendering, set this; prevents destroy-while-draw
		BIT_FIELD bFullScreen : 1; // trigger full screen, don't resize internal surface.
		BIT_FIELD bNotFullScreen : 1; // temporary override for internal mouse handling
	} flags;

	struct HVIDEO_tag *pNext, *pPrior;
	struct HVIDEO_tag *pAbove  // this is above specified window
		, *pBelow;  // this is below specified window

	LoseFocusCallback pLoseFocus;
	uintptr_t dwLoseFocus;


	void *hOldBitmap;
	void *hOldBitmapFullScreen;

#ifdef _OPENGL_ENABLED
	HGLRC	hRC;	 // Permanent Rendering Context
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

	struct video_struct_fullscreen
	{
		int32_t x, y;
		uint32_t width, height;
		int target_display;
	}full_screen;
	struct video_struct_portion_update {
		LOGICAL pending;
		uint32_t x, y, w, h;
	} portion_update;
} VIDEO, *PVIDEO;

RENDER_NAMESPACE_END

#endif
