// Per-window state for the macOS (Cocoa) video backend.
//
// The Cocoa/AppKit objects are kept as opaque `void*` (Objective-C `id`s) so
// that this header - and the rest of the C code that includes it - pulls in no
// Objective-C headers.  All AppKit access is done through the Objective-C
// runtime (<objc/message.h>) from plain C; see cocoa_objc.h / vidlib.cocoa.c.

#ifndef SACK_VIDLIB_COCOA_LOCAL_H
#define SACK_VIDLIB_COCOA_LOCAL_H

RENDER_NAMESPACE

struct HVIDEO_tag {
	KEYBOARD kbd;
	RenderReadCallback ReadComplete;
	uintptr_t psvRead;
	PLINKQUEUE pInput;
};

typedef struct cocoa_video_tag {
	struct {
		uint32_t bShown : 1;
		uint32_t bDestroy : 1;
		uint32_t bFocused : 1;
		uint32_t hidden : 1;       // window is currently ordered out
		uint32_t bLayerReady : 1;  // content view is layer backed
	} flags;

	int32_t  x, y;       // window origin (top-left, SACK coordinates)
	uint32_t w, h;       // window/client size

	struct cocoa_video_tag *under; // this window sits under `under`
	struct cocoa_video_tag *above; // this window sits above `above`

	void *nsWindow;        // NSWindow*
	void *nsView;          // NSView* (layer-backed content view)
	void *caLayer;         // CALayer* that displays the framebuffer image
	void *windowDelegate;  // delegate object: close / resize / focus

	Image  pImageDraw;     // framebuffer the application draws into
	PCOLOR framebuffer;    // backing pixels for pImageDraw (we own these)

	uint32_t mouse_b;      // current button mask (MK_LBUTTON ...)
	int32_t  mouse_x, mouse_y;

	RedrawCallback         pRedrawCallback; uintptr_t dwRedrawData;
	MouseCallback          mouse;           uintptr_t psvMouse;
	LoseFocusCallback      pLoseFocus;      uintptr_t dwLoseFocus;
	CloseCallback          pWindowClose;    uintptr_t dwCloseData;
	KeyProc                pKeyProc;        uintptr_t dwKeyData;
	HideAndRestoreCallback pHideCallback;   uintptr_t dwHideData;
	HideAndRestoreCallback pRestoreCallback;uintptr_t dwRestoreData;
	PKEYDEFINE             pKeyDefs;
} XPANEL, *PXPANEL;

struct cocoa_local_tag {
	struct {
		volatile uint32_t bInited : 1;     // backend initialized
		volatile uint32_t bAppInited : 1;  // NSApplication created
	} flags;
	PIMAGE_INTERFACE pii;
	PLIST    pActiveList;     // all live PXPANELs
	PXPANEL  hVidFocused;     // keyboard events target
	PXPANEL  hCaptured;       // mouse capture target
	int      opens;           // count of opened displays (cascade offset)
	uint32_t default_display_x, default_display_y;
	void    *nsApp;           // NSApplication*
	void    *viewClass;       // runtime-built NSView subclass
	void    *delegateClass;   // runtime-built window-delegate subclass
	CRITICALSECTION cs;
};

extern struct cocoa_local_tag cocoa_l;

// key binder helpers (keydefs.c)
PKEYDEFINE co_CreateKeyBinder ( void );
void       co_DestroyKeyBinder ( PKEYDEFINE pKeyDef );
int        co_BindEventToKey ( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv );
int        co_UnbindKey ( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier );
int        co_HandleKeyEvents ( PKEYDEFINE pKeyDefs, uint32_t key );

RENDER_NAMESPACE_END

#endif
