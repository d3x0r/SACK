#include <timers.h>
#define l local_video_common

typedef struct vidlib_local_tag
{
	struct {
		//-------- these should be removed! and moved to comments to save
      // string space! (and related logging statements these protect should be eliminated.)
		_32 bLogRegister : 1;
		_32 bLogFocus : 1;
		_32 bLogWrites : 1; // log when surfaces are written to real space
      //---------- see comment above
	} flags;
   PRENDERER mouse_last_vid;
   int mouse_b, mouse_y, mouse_x;
   int _mouse_b, _mouse_y, _mouse_x;

   int WindowBorder_X, WindowBorder_Y;

   ATOM aClass;      // keep reference of window class....

   HANDLE hWndInstance;
   int bCreatedhWndInstance;

// thread synchronization variables...
	unsigned char bThreadRunning, bExitThread;
	PLIST pActiveList;
	PLIST pInactiveList;
	PTHREAD pThread;
	DWORD dwThreadID;  // thread that receives events from windows queues...
	DWORD dwEventThreadID; // thread that handles dispatch to application
	VIDEO hDialogVid[16];
	int nControlVid;
	const TEXTCHAR *gpTitle;
	HVIDEO hVideoPool;      // linked list of active windows
	HVIDEO hVidFocused;
	HVIDEO hVidCore; // common, invisible surface behind all windows (application container)
	HVIDEO hCaptured;
	// kbd.key == KeyboardState
	KEYBOARD kbd;
	_32 dwMsgBase;
	//_32 pid;
	//char KeyboardState[256];   // export for key procs to reference...
	HHOOK hKeyHook;
} LOCAL;

#ifndef VIDLIB_MAIN
extern
#endif
 LOCAL l;
