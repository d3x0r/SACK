#ifndef l
#define l local_video_common
#endif
#include <imglib/imagestruct.h>
#include <imglib/fontstruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>
#include <image.h>

RENDER_NAMESPACE
#ifdef MINGW_SUX

typedef HANDLE HTOUCHINPUT;
#define WM_TOUCH 0x0240
/*
 * RegisterTouchWindow flag values
 */
#define TWF_FINETOUCH       (0x00000001)
#define TWF_WANTPALM        (0x00000002)
#endif

#ifndef VIDLIB_MAIN
extern
#endif
	struct vidlib_local_tag
{
	struct {
		//-------- these should be removed! and moved to comments to save
		// string space! (and related logging statements these protect should be eliminated.)
		BIT_FIELD bLogMessages : 1;
		BIT_FIELD bLogRegister : 1;
		BIT_FIELD bLogFocus : 1;
		BIT_FIELD bLogWrites : 1; // log when surfaces are written to real space
		BIT_FIELD bLogDisplayEnumTest : 1;
		BIT_FIELD bThreadCreated : 1;
		BIT_FIELD bPostedInvalidate : 1;
		BIT_FIELD bLogKeyEvent : 1;
		BIT_FIELD bWhatever : 1;
		BIT_FIELD bLayeredWindowDefault : 1;
		BIT_FIELD mouse_on : 1;
		BIT_FIELD bOptimizeHide : 1;
		BIT_FIELD bUseLLKeyhook : 1;
		BIT_FIELD bLogMouseEvents : 1;
		BIT_FIELD bHookTouchEvents : 1;
		//---------- see comment above
	} flags;
	PRENDERER mouse_last_vid;
#if defined( WIN32 )
	LPCTSTR new_cursor;
	LPCTSTR old_cursor;
#endif
	int mouse_b, mouse_y, mouse_x;
	int _mouse_b, _mouse_y, _mouse_x;

	int WindowBorder_X, WindowBorder_Y;
#if defined( WIN32 )
	ATOM aClass;		// keep reference of window class....
	ATOM aClass2;      // keep reference of window class.... (opengl minimal)

	HANDLE hWndInstance;
#endif
	int bCreatedhWndInstance;

// thread synchronization variables...
	unsigned char bThreadRunning, bExitThread;
	PLIST pActiveList;
	PLIST pInactiveList;
	PLIST threads;
	PTHREAD actual_thread; // this is the one that creates windows surfaces...
#if defined( WIN32 )
	DWORD dwThreadID;  // thread that receives events from windows queues...
	DWORD dwEventThreadID; // thread that handles dispatch to application
#endif
	VIDEO hDialogVid[16];
	int nControlVid;
	const TEXTCHAR *gpTitle;
	PVIDEO hVideoPool;      // linked list of active windows
	PVIDEO hVidFocused;
	PVIDEO hVidCore; // common, invisible surface behind all windows (application container)
	PVIDEO hCaptured;
	PVIDEO hCapturedPrior;
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
#ifndef NO_TOUCH
	BOOL (WINAPI *GetTouchInputInfo )( HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize );
	BOOL (WINAPI *CloseTouchInputHandle )( HTOUCHINPUT hTouchInput );
	BOOL (WINAPI *RegisterTouchWindow )( HWND hWnd, ULONG ulFlags );
#endif
#endif
#endif
	_32 last_mouse_update; // last tick the mouse moved.
	PRENDERER last_mouse_update_display;
	PRENDERER invalidated_window;
} l;

RENDER_NAMESPACE_END


