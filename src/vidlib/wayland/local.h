
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>


struct HVIDEO_tag {
	KEYBOARD kbd;
	RenderReadCallback ReadComplete;
	uintptr_t psvRead;
	PLINKQUEUE pInput;
};

typedef struct wvideo_tag
{
	struct {
		uint32_t bShown : 1;
		uint32_t bDestroy : 1;
		uint32_t bFocused : 1;
		uint32_t dirty : 1;
		uint32_t canCommit : 1;
		uint32_t wantDraw : 1;
		uint32_t hidden : 1; // tracks whether the window is visible or not.
	} flags;

	int32_t x, y;
	uint32_t w, h;

	struct wvideo_tag* under;  // this window is under this reference
	struct wvideo_tag* above;  // this window is above the one pointed here

	struct wl_surface* surface;
	struct wl_shell_surface *shell_surface;
	struct wl_subsurface *sub_surface;
	struct xdg_toplevel *xdg_toplevel;
	PCOLOR shm_data;
	struct wl_buffer *buff;
	int curBuffer;
	uint32_t bufw, bufh;
	//enum sizeDisplayValues changedEdge; // when resized, this is how to copy the existing image.
	int freeBuffer[2];
	struct wl_buffer * buffers[2];
	PCOLOR  color_buffers[2];
	size_t buffer_sizes[2];
	Image buffer_images[2];
	struct wl_shm_pool *pool;
	struct wl_callback *frame_callback;  // request for WM_PAINT
	Image pImage;
	RedrawCallback pRedrawCallback;
	intptr_t dwRedrawData;

	MouseCallback mouse; uintptr_t psvMouse;
	LoseFocusCallback pLoseFocus;
	uintptr_t dwLoseFocus;

	CloseCallback pWindowClose;
	uintptr_t dwCloseData;

	KeyProc pKeyProc;
	uintptr_t dwKeyData;

	HideAndRestoreCallback pHideCallback;
	uintptr_t  dwHideData;
	PKEYDEFINE pKeyDefs;
} XPANEL, *PXPANEL;

struct pointer_data {
    struct wl_surface *surface;
    struct wl_buffer *buffer;
    int32_t hot_spot_x;
    int32_t hot_spot_y;
    struct wl_surface *target_surface;
};

struct output_data {
	struct wl_output *wl_output;
	int x, y;
	int w, h;
	int w_mm, h_mm;

	int32_t scale;
	int32_t pending_scale;
};

enum WAYLAND_INTERFACE_STRING {
	wis_compositor,
	wis_output,
	wis_subcompositor,
	wis_shell,
	wis_seat,
	wis_shm,
	wis_xdg_base,
	wis_xdg_shell,
#ifdef ALLOW_KDE
	wis_kde_shell,
#endif
	max_interface_versions
};



struct wayland_local_tag
{
	PTHREAD waylandThread;
	PIMAGE_INTERFACE pii;
	struct {
		volatile uint32_t bInited : 1;
		uint32_t bLogKeyEvent:1;
	} flags;
	PLIST damaged;
	int dirty;
	int commit;

	struct wl_event_queue* queue;
	struct wl_display* display;
	struct wl_compositor* compositor;
	PLIST outputSurfaces;  // contains wl_output's
	struct wl_subcompositor* subcompositor;
	struct wl_shm *shm;
	volatile int registering; // valid pixel format found.
	int canDraw; // valid pixel format found.
	struct wl_shell *shell;
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_shell *xdg_shell;
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	int32_t mouseSerial;
	struct pointer_data pointer_data ;
	struct wl_keyboard *keyboard;
	struct xkb_context *xkb_context;
	struct xkb_keymap *keymap;
	struct xkb_state *xkb_state;
	int versions[max_interface_versions]; // reported versions


	int screen_width;
	int screen_height;
	int screen_num;
/* these variables will be used to store the IDs of the black and white */
/* colors of the given screen. More on this will be explained later.    */
	unsigned long white_pixel;
	unsigned long black_pixel;

	//-------------- Global seat/input tracking
	struct mouse {
		int32_t x, y;
		uint32_t b;
	}mouse_;
	int keyMods;
	uint32_t  utfKeyCode;
	xkb_keysym_t keysym;

	PXPANEL hVidFocused; // keyboard events go here
	PXPANEL hCaptured; // send all mouse events here (probalby should-no-op this)
	PLIST pActiveList; // non-destroyed windows are here

};


//----------------------------

PKEYDEFINE wl_CreateKeyBinder ( void );
void wl_DestroyKeyBinder ( PKEYDEFINE pKeyDef );
// keycode is JUST a KEY_ symbol
int wl_BindEventToKey ( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier, KeyTriggerHandler trigger, uintptr_t psv );
int wl_UnbindKey ( PKEYDEFINE pKeyDefs, uint32_t keycode, uint32_t modifier );
// key is a key event with key sumbol and modifiers with pressed/released status
int wl_HandleKeyEvents ( PKEYDEFINE pKeyDefs, uint32_t key );
