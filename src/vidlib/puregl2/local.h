#ifndef l
#define l local_opengl_video_common
#endif

#ifndef USE_IMAGE_INTERFACE
#define USE_IMAGE_INTERFACE l.gl_image_interface
#endif

#if defined( __QNX__ )
#include <gf/gf.h>
#endif

#ifdef __LINUX__
#if defined( __ANDROID__ ) || defined( __QNX__ )
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#else
//#include "../glext.h"
#endif

#define NEED_VECTLIB_COMPARE
#define FORCE_NO_RENDER_INTERFACE

#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>
#include <render3d.h>
#include <image.h>
#include <vectlib.h>

RENDER_NAMESPACE


#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d ",err ); \
				}                               
#define CheckErrf(f,...)  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
					lprintf( "err=%d "f,err,##__VA_ARGS__ ); \
				}                               

#ifdef MINGW_SUX
typedef struct tagUPDATELAYEREDWINDOWINFO {
    _32               cbSize;
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

typedef LOGICAL (CPROC *Update3dProc)(PTRANSFORM);


struct plugin_reference
{
	CTEXTSTR name;
	PTRSZVAL psv;
	LOGICAL (CPROC *Update3d)(PTRANSFORM origin);
	void (CPROC *FirstDraw3d)(PTRSZVAL);
	void (CPROC *ExtraDraw3d)(PTRSZVAL,PTRANSFORM camera);
	void (CPROC *Draw3d)(PTRSZVAL);
	LOGICAL (CPROC *Mouse3d)(PTRSZVAL,PRAY,_32);
   void (CPROC *ExtraClose3d)(PTRSZVAL);
};

struct display_camera
{
	S_32 x, y;
	_32 w, h;
	int display;
	RCOORD aspect;
	RCOORD identity_depth;
	PTRANSFORM origin_camera;
	PRENDERER hVidCore; // common, invisible surface behind all windows (application container)
#if defined( __LINUX__ )
#elif defined( _WIN32 )
	HWND hWndInstance;
#endif
#if defined( USE_EGL )
   NativeWindowType displayWindow;
#endif
	RAY mouse_ray;
	int viewport[4];
	struct {
		BIT_FIELD extra_init : 1;
		BIT_FIELD init : 1;
		BIT_FIELD did_first_draw : 1;  // init but in the opengl context....
	} flags;
	PLIST plugins; // each camera has plugins that might attach more render and mouse methods
	int type;
};

#ifndef VIDLIB_MAIN
extern
#endif
	struct vidlib_opengl_local_tag
{
	struct {
		//-------- these should be removed! and moved to comments to save
      // string space! (and related logging statements these protect should be eliminated.)
		BIT_FIELD bLogRegister : 1;
		BIT_FIELD bLogFocus : 1;
		BIT_FIELD bLogWrites : 1; // log when surfaces are written to real space
		BIT_FIELD bThreadCreated : 1;
		BIT_FIELD bPostedInvalidate : 1;
		BIT_FIELD bLogKeyEvent : 1;
		BIT_FIELD bWhatever : 1;
		BIT_FIELD bLayeredWindowDefault : 1;
		BIT_FIELD mouse_on : 1;
		BIT_FIELD bOptimizeHide : 1;
		BIT_FIELD bUseLLKeyhook : 1;
		BIT_FIELD bUpdateWanted : 1; // can idle unless there's an update
		BIT_FIELD bLogMessageDispatch : 1;
		BIT_FIELD bRotateLock : 1;
		BIT_FIELD bView360 : 1;
		BIT_FIELD bLogMouseEvent : 1;
		//---------- see comment above
		BIT_FIELD bForceUnaryAspect : 1;
		BIT_FIELD bLogRenderTiming : 1;
	} flags;
	PRENDERER mouse_last_vid;

	int mouse_b, mouse_y, mouse_x;
	int _mouse_b, _mouse_y, _mouse_x;
	int real_mouse_x, real_mouse_y;

	int WindowBorder_X, WindowBorder_Y;
   int default_display_x, default_display_y;

	RAY mouse_ray;

#if defined( _WIN32 )
	ATOM aClass;      // keep reference of window class....
	ATOM aClass2;      // keep reference of window class.... (opengl minimal)
#endif
#if defined( __QNX__ )
   int nDevices;
	gf_dev_t qnx_dev[64];
	gf_dev_info_t qnx_dev_info[64];
	gf_display_t* qnx_display[64];
	gf_display_info_t* qnx_display_info[64];
#endif
#if defined( USE_EGL )
   NativeWindowType displayWindow;
#endif
	int bCreatedhWndInstance;

// thread synchronization variables...
	unsigned char bThreadRunning, bExitThread;
	PLIST pActiveList;
	PRENDERER bottom;
	PRENDERER top;
	PLIST pInactiveList;
	PLIST threads;
	PTHREAD actual_thread; // this is the one that creates windows surfaces...
	_32 dwThreadID;  // thread that receives events from windows queues...
	_32 dwEventThreadID; // thread that handles dispatch to application
	VIDEO hDialogVid[16];
	int nControlVid;
	const TEXTCHAR *gpTitle;
	PVIDEO hVideoPool;      // linked list of active windows
	PVIDEO hVidPhysicalFocused;  // this is the physical renderer with focus
	PVIDEO hVidVirtualFocused;   // this is the virtual window (application surface) with focus
	PVIDEO hCaptured;
	// kbd.key == KeyboardState
	KEYBOARD kbd;
	_32 dwMsgBase;
	//_32 pid;
	//char KeyboardState[256];   // export for key procs to reference...
	PLIST keyhooks;
	PLIST ll_keyhooks;
	CRITICALSECTION csList;
	//HHOOK hKeyHook;
#ifndef _ARM_
#ifdef WIN32
	BOOL (WINAPI *UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
	BOOL (WINAPI *UpdateLayeredWindowIndirect )(HWND hWnd, const UPDATELAYEREDWINDOWINFO *pULWInfo);
#if !defined( __WATCOMC__ ) && !defined( __GNUC__ )
	BOOL (WINAPI *GetTouchInputInfo )( HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize );
	BOOL (WINAPI *CloseTouchInputHandle )( HTOUCHINPUT hTouchInput );
	BOOL (WINAPI *RegisterTouchWindow )( HWND hWnd, ULONG ulFlags );
#endif
#endif
#endif
	_32 last_mouse_update; // last tick the mouse moved.
	_32 mouse_timer_id;
#ifdef WIN32
	_32 redraw_timer_id;
#endif

	RCOORD fProjection[4][4];
	int multi_shader;
	struct {
		struct {
			BIT_FIELD checked_multitexture : 1;
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;
	} glext;
	PLIST cameras; // list of struct display_camera *
	PLIST update; // list of updates from plugins that have registered.
	PTRANSFORM origin;
	VECTOR mouse_ray_slope;
	VECTOR mouse_ray_origin;
	VECTOR mouse_ray_target;
	RCOORD scale;

	struct display_camera *current_mouse_event_camera;  // setcursorpos can only happen during a mouse event, please.

	PIMAGE_INTERFACE gl_image_interface;
	CTEXTSTR current_key_text;
} l;

void CPROC SACK_Vidlib_HideInputDevice( void );
void CPROC SACK_Vidlib_ShowInputDevice( void );

void	HostSystem_InitDisplayInfo( void );
int Handle3DTouches( struct display_camera *camera, PINPUT_POINT touches, int nTouches );


/// ---------------- Interface ---------------
POINTER  CPROC GetDisplayInterface (void);
void  CPROC DropDisplayInterface (POINTER p);
POINTER CPROC GetDisplay3dInterface (void);
void  CPROC DropDisplay3dInterface (POINTER p);

// ------------- Interface to system key event interface
void SACK_Vidlib_ShowInputDevice( void );
void SACK_Vidlib_HideInputDevice( void );

// ---------------- Common --------------------
PTRANSFORM CPROC GetRenderTransform( PRENDERER r );
struct display_camera *SACK_Vidlib_OpenCameras( void );
LOGICAL CreateDrawingSurface (PVIDEO hVideo);
void DoDestroy (PVIDEO hVideo);
LOGICAL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy);

// --------------- Mouse 3d ------------
void UpdateMouseRay( struct display_camera * camera );
int InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y );
PRENDERER CPROC OpenGLMouse( PTRSZVAL psvMouse, S_32 x, S_32 y, _32 b );
int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
								, RCOORD *pT2, PVECTOR s2, PVECTOR o2 );
// this uses coordiantes in l.mouse_x and l.mouse_y and computes the current mouse ray in all displays
void UpdateMouseRays( void );

//-------------------  render utility ------------
void RenderGL( struct display_camera *camera );
void WantRenderGL( void );

// -------------  physical interface - WIN32 ------------
void OpenWin32Camera( struct display_camera *camera );
int InitGL( struct display_camera *camera );										// All Setup For OpenGL Goes Here



// ---------- vidlib win32 - share dsymbols for keymap win32
#define WD_HVIDEO   0   // WindowData_HVIDEO



RENDER_NAMESPACE_END
