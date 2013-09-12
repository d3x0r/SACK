#define l local_video_common

//#include "../glext.h"

#ifdef MINGW_SUX
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
	int x, y;
	size_t w, h;
	int display;
	RCOORD aspect;
	RCOORD identity_depth;
	PTRANSFORM T_camera_origin_offset;
	PTRANSFORM origin_camera;
	PVIDEO hVidCore; // common, invisible surface behind all windows (application container)
	HWND hWndInstance;
	RAY mouse_ray;
	int viewport[4];
	struct {
		BIT_FIELD init : 1;
		BIT_FIELD did_first_draw : 1;
	} flags;
	PLIST plugins; // each camera has plugins that might attach more render and mouse methods
	int type;

};

typedef struct vidlib_local_tag
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
		BIT_FIELD bUseLLKeyhook : 1;
		BIT_FIELD bUpdateWanted : 1; // can idle unless there's an update
		BIT_FIELD bLogMessageDispatch : 1;
		BIT_FIELD bRotateLock : 1;
		BIT_FIELD bView360 : 1;
      //---------- see comment above
	} flags;
	PRENDERER mouse_last_vid;
	int mouse_b, mouse_y, mouse_x;
	int _mouse_b, _mouse_y, _mouse_x;
	int real_mouse_x, real_mouse_y;

	int WindowBorder_X, WindowBorder_Y;

	RAY mouse_ray;


	ATOM aClass;      // keep reference of window class....
	ATOM aClass2;      // keep reference of window class.... (opengl minimal)

	int bCreatedhWndInstance;

// thread synchronization variables...
	unsigned char bThreadRunning, bExitThread;
	PLIST pActiveList;
	PRENDERER bottom;
	PRENDERER top;
	PLIST pInactiveList;
	PLIST threads;
	PTHREAD actual_thread; // this is the one that creates windows surfaces...
	DWORD dwThreadID;  // thread that receives events from windows queues...
	DWORD dwEventThreadID; // thread that handles dispatch to application
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
#endif
#endif
	_32 last_mouse_update; // last tick the mouse moved.
	UINT_PTR mouse_timer_id;
	UINT_PTR redraw_timer_id;

	RCOORD fProjection[16];
	int multi_shader;
	struct {
		struct {
			BIT_FIELD init_ok : 1;
			BIT_FIELD shader_ok : 1;
		} flags;
	} glext;
	PLIST cameras; // list of struct display_camera *
	PLIST update; // list of updates from plugins that have registered.
	PTRANSFORM origin;
} LOCAL;

int SetActiveD3DDisplayView( PVIDEO hDisplay, int nFracture );
int SetActiveD3DDisplay( PVIDEO hDisplay );

// in D3D.cpp
int InitD3D( struct display_camera *camera );										// All Setup For OpenGL Goes Here

#ifndef VIDLIB_MAIN
extern
#endif
 LOCAL l;
