#ifndef l
#define l local_opengl_video_common
#endif

#ifndef USE_IMAGE_INTERFACE
#define USE_IMAGE_INTERFACE l.gl_image_interface
#endif

#ifdef __LINUX__
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#else
#include "../glext.h"
#endif

#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>
#include <image.h>

RENDER_NAMESPACE

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

typedef void (CPROC *Update3dProc)(PTRANSFORM);


struct plugin_reference
{
	CTEXTSTR name;
	PTRSZVAL psv;
	void (CPROC *Update3d)(PTRANSFORM origin);
	void (CPROC *FirstDraw3d)(PTRSZVAL);
	void (CPROC *ExtraDraw3d)(PTRSZVAL,PTRANSFORM camera);
	void (CPROC *Draw3d)(PTRSZVAL);
	LOGICAL (CPROC *Mouse3d)(PTRSZVAL,PRAY,_32);
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
#elif defined( __WINDOWS__ )
	HWND hWndInstance;
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

	RAY mouse_ray;

#if defined( __WINDOWS__ )
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
	_32 dwThreadID;  // thread that receives events from windows queues...
	_32 dwEventThreadID; // thread that handles dispatch to application
	VIDEO hDialogVid[16];
	int nControlVid;
	const TEXTCHAR *gpTitle;
	PVIDEO hVideoPool;      // linked list of active windows
	PVIDEO hVidFocused;
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
	_32 redraw_timer_id;

	RCOORD fProjection[16];
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

} l;



RENDER_NAMESPACE_END
