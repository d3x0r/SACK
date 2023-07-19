#ifndef l
#define l local_opengl_video_common
#endif
#ifndef USE_IMAGE_INTERFACE
#define USE_IMAGE_INTERFACE l.gl_image_interface
#endif

#define USE_IMAGE_3D_INTERFACE l._3d_image_interface
#if WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#if __LINUX__
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <stdhdrs.h>
#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#endif


#define NEED_VECTLIB_COMPARE
#define FORCE_NO_RENDER_INTERFACE

#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>
#include <render3d.h>
#include <image.h>
#include <image3d.h>
#include <vectlib.h>

#include "vulkaninfo.h"

#if defined( __64__ ) && defined( _WIN32 )
#define _SetWindowLong(a,b,c)   SetWindowLongPtr(a,b,(LONG_PTR)(c))
#define _GetWindowLong   GetWindowLongPtr
#else
#define _SetWindowLong(a,b,c)   SetWindowLong(a,b,(long)(c))
#define _GetWindowLong(a,b)   GetWindowLong(a,b)
#endif

IMAGE_NAMESPACE
struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
	Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h );
	uintptr_t psv;
};
IMAGE_NAMESPACE_END



RENDER_NAMESPACE

#define WM_USER_OPEN_CAMERAS    WM_USER+518

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


typedef LOGICAL (CPROC *Update3dProc)(PTRANSFORM);


struct plugin_reference
{
	struct plugin_refernce_flags
	{
		BIT_FIELD did_first_draw : 1;
	} flags;
	CTEXTSTR name;
	uintptr_t psv;
	LOGICAL (CPROC *Update3d)(PTRANSFORM origin);
	void (CPROC *FirstDraw3d)(uintptr_t);
	void (CPROC *ExtraDraw3d)(uintptr_t,PTRANSFORM camera);
	void (CPROC *Draw3d)(uintptr_t);
	LOGICAL (CPROC *Mouse3d)(uintptr_t,PRAY,int32_t,int32_t,uint32_t);
	void (CPROC *ExtraClose3d)(uintptr_t);
	void (CPROC *Resume3d)(void);
   LOGICAL (CPROC *Key3d)(uintptr_t,uint32_t);
};

#if defined( _D3D11_DRIVER )
struct dxgi_adapter_output
{
	unsigned int ID;
	IDXGIOutput* adapterOutput;
	DXGI_OUTPUT_DESC adapterOutputDesc;
	unsigned int numModes;
	DXGI_MODE_DESC* displayModeList;
};
struct dxgi_adapter
{
	unsigned int ID;
	IDXGIAdapter *adapter;
	DXGI_ADAPTER_DESC adapterDesc;
	PLIST adapter_outputs; // list of struct dxgi_adapter_output
};
#endif

struct display_camera
{
	int32_t x, y;
	uint32_t w, h;
	int display;
	RCOORD aspect;
	RCOORD depth; // far field
	RCOORD identity_depth;
	PTRANSFORM origin_camera;
	PRENDERER hVidCore; // common, invisible surface behind all windows (application container)
#if defined( __LINUX__ )
	// X11 handle?
#elif defined( _WIN32 )
	HWND hWndInstance;
#endif

	struct SwapChain chain; // chain owns the device.

	RAY mouse_ray;
	struct {
		BIT_FIELD extra_init : 1;
		BIT_FIELD init : 1;
		BIT_FIELD topmost : 1;
		BIT_FIELD first_draw : 1;
		BIT_FIELD vsync : 1; 
		BIT_FIELD opening : 1; // prevent re-opening the same camera
		BIT_FIELD bDrawablePlugin : 1;
	} flags;
	PLIST plugins; // each camera has plugins that might attach more render and mouse methods
	int type;
   	int nCamera;
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
		BIT_FIELD bHookTouchEvents : 1;
		BIT_FIELD bCameraManuallyCapturedMouse : 1;
		BIT_FIELD bVirtualManuallyCapturedMouse : 1;
		BIT_FIELD bViewVolumeUpdated : 1;
		BIT_FIELD disallow_3d : 1; // I know; it's a puregl shell... but android can be flat
	} flags;
	PRENDERER mouse_last_vid;

	int mouse_b, mouse_y, mouse_x;
	int _mouse_b, _mouse_y, _mouse_x;
	int real_mouse_x, real_mouse_y;

	int WindowBorder_X, WindowBorder_Y;
	int default_display_x, default_display_y;

#if defined( _WIN32 )
	ATOM aClass;      // keep reference of window class....
	ATOM aClass2;      // keep reference of window class.... (opengl minimal)
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
	uint32_t dwThreadID;  // thread that receives events from windows queues...
	uint32_t dwEventThreadID; // thread that handles dispatch to application
	VIDEO hDialogVid[16];
	int nControlVid;
	const TEXTCHAR *gpTitle;
	PVIDEO hVideoPool;      // linked list of active windows
	PVIDEO hVidPhysicalFocused;  // this is the physical renderer with focus
	PVIDEO hVidVirtualFocused;   // this is the virtual window (application surface) with focus
	PVIDEO hCameraCaptured;
	PVIDEO hCameraCapturedPrior; // reset on unused event
	PVIDEO hVirtualCaptured;
	PVIDEO hVirtualCapturedPrior; // reset on unused event

	PVIDEO hCapturedMousePhysical;

   struct plugin_reference *hPluginKeyCapture; // used to track focus of key events to plugin modules
	// kbd.key == KeyboardState
	KEYBOARD kbd;
	uint32_t dwMsgBase;
	//char KeyboardState[256];   // export for key procs to reference...
	PLIST keyhooks;
	PLIST ll_keyhooks;
	CRITICALSECTION csList;
	//HHOOK hKeyHook;
#ifndef _ARM_
#ifdef WIN32
	BOOL (WINAPI *UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
	BOOL (WINAPI *UpdateLayeredWindowIndirect )(HWND hWnd, const UPDATELAYEREDWINDOWINFO *pULWInfo);
#ifndef NO_TOUCH
	BOOL (WINAPI *GetTouchInputInfo )( HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize );
	BOOL (WINAPI *CloseTouchInputHandle )( HTOUCHINPUT hTouchInput );
	BOOL (WINAPI *RegisterTouchWindow )( HWND hWnd, ULONG ulFlags );
#endif
#endif
#endif
	uint32_t last_mouse_update; // last tick the mouse moved.
	uint32_t mouse_timer_id;
#ifdef WIN32
	UINT_PTR redraw_timer_id;
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
	RCOORD scale;

	struct display_camera *current_render_camera;  // setcursorpos can only happen during a mouse event, please.
	struct display_camera *current_mouse_event_camera;  // setcursorpos can only happen during a mouse event, please.

	PIMAGE_INTERFACE gl_image_interface;
	PIMAGE_3D_INTERFACE _3d_image_interface;
	CTEXTSTR current_key_text;
	void (*wake_callback)(void );
} l;

void CPROC SACK_Vidlib_HideInputDevice( void );
void CPROC SACK_Vidlib_ShowInputDevice( void );

#ifndef _WIN32
// this is external for android, it's preload for windows
void	HostSystem_InitDisplayInfo( void );
#endif
int Handle3DTouches( struct display_camera *camera, PINPUT_POINT touches, int nTouches );


/// ---------------- Interface ---------------
#undef GetDisplayInterface
POINTER  CPROC GetDisplayInterface (void);
#undef DropDisplayInterface
void  CPROC DropDisplayInterface (POINTER p);
POINTER CPROC GetDisplay3dInterface (void);
void  CPROC DropDisplay3dInterface (POINTER p);

// ------------- Interface to system key event interface
void SACK_Vidlib_ShowInputDevice( void );
void SACK_Vidlib_HideInputDevice( void );

// ---------------- Common --------------------
void LoadOptions( void );
#undef GetRenderTransform
PTRANSFORM CPROC GetRenderTransform( PRENDERER r );
struct display_camera *SACK_Vidlib_OpenCameras( void );
LOGICAL CreateDrawingSurface (PVIDEO hVideo);
void DoDestroy (PVIDEO hVideo);
LOGICAL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy);
void CPROC PureGL2_Vidlib_SuspendSystemSleep( int suspend );
void CPROC PureGL2_Vidlib_SetDisplayCursor( CTEXTSTR cursor );
LOGICAL CPROC PureGL2_Vidlib_AllowsAnyThreadToUpdate( void );
void OpenCamera( struct display_camera *camera );


// --------------- Mouse 3d ------------
void ComputeMouseRay( struct display_camera *camera, LOGICAL bUniverseSpace, PRAY mouse_ray, int32_t x, int32_t y );
int InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y );
uintptr_t/*PRENDERER*/ CPROC OpenGLMouse( uintptr_t psvMouse, int32_t x, int32_t y, uint32_t b );
int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
								, RCOORD *pT2, PVECTOR s2, PVECTOR o2 );
// this uses coordiantes in l.mouse_x and l.mouse_y and computes the current mouse ray in all displays
void UpdateMouseRays( int32_t x, int32_t y );
#undef GetViewVolume
void CPROC GetViewVolume( PRAY *planes );

// -------- keymap_win32.c ----------------
int CPROC OpenGLKey( uintptr_t psv, uint32_t keycode ); // should move this

// ------- keymap_linux.c
CTEXTSTR SACK_Vidlib_GetKeyText( int pressed, int key_index, int *used );
void SACK_Vidlib_ProcessKeyState( int pressed, int key_index, int *used );


//-------------------  render utility ------------
void Render3D( struct display_camera *camera );
void WantRender3D( void );
void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
LOGICAL ProcessGLDraw( LOGICAL draw_all ); // returns want update also...
void drawCamera( struct display_camera *camera );


// -------------  physical interface - WIN32 ------------
void OpenWin32Camera( struct display_camera *camera );
int InitGL( struct display_camera *camera );										// All Setup For OpenGL Goes Here

// ------ keydefs ------------
int DispatchKeyEvent( PRENDERER render, uint32_t key );


// ------ android keymap -------------
void SACK_Vidlib_ToggleInputDevice( void );

#ifdef __3D__
// this has the ability to fail, becuase it enables the context
int Init3D( struct display_camera *camera ); // begin drawing (setup projection); also does SetActiveXXXDIsplay
void EndActive3D( struct display_camera *camera ); // does appropriate EndActiveXXDisplay
void SetupPositionMatrix( struct display_camera *camera );
#endif

#if defined( _OPENGL_DRIVER )
int EnableOpenGL( struct display_camera *camera );
int SetActiveGLDisplayView( struct display_camera *camera, int nFracture );
#endif
#if defined( USE_EGL )
void EnableEGLContext( struct display_camera *camera );
#endif

#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER ) || defined( _D3D11_DRIVER )
int EnableD3d( struct display_camera *camera );
int EnableD3dView( struct display_camera *camera, int x, int y, int w, int h );
void DisableD3d( struct display_camera *camera );

int SetActiveD3DDisplay( struct display_camera *hVideo );
int SetActiveD3DDisplayView( struct display_camera *hVideo, int nFracture );


#  ifndef VIDLIB_INTERFACE_DEFINED
extern
	  RENDER3D_INTERFACE Render3d;
#  endif
#endif

//--------------------------vulkan_interface.c 

#if defined(_WIN32)
void EnableVulkan( HINSTANCE hInstance, struct display_camera *camera );
#elif defined(__ANDROID__)
void EnableVulkan( ANativeWindow *window, struct display_camera *camera );
#else
void EnableVulkan( xcb_connection_t *connection,
	xcb_window_t *window, struct display_camera *camera );
#endif



// ---------- vidlib win32 - share dsymbols for keymap win32
#define WD_HVIDEO   0   // WindowData_HVIDEO

// --------------- win32 keymap ------------------------
#ifdef _WIN32
LRESULT CALLBACK
   KeyHook (int code,      // hook code
				WPARAM wParam,    // virtual-key code
				LPARAM lParam     // keystroke-message information
			  );
LRESULT CALLBACK
   KeyHook2 (int code,      // hook code
				WPARAM wParam,    // virtual-key code
				LPARAM lParam     // keystroke-message information
			  );
#endif

RENDER_NAMESPACE_END
