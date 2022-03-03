
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>
#include <wayland-cursor.h>

typedef uint32_t* PCOLOR;

struct HVIDEO_tag {
	//KEYBOARD kbd;
	//RenderReadCallback ReadComplete;
	uintptr_t psvRead;
	//PLINKQUEUE pInput;
};

struct drawRequest {
	//PTHREAD thread;
	struct wvideo_tag *r;

};

struct damageInfo {
	int32_t x, y;
	uint32_t w, h;
};

typedef struct wvideo_tag
{
	struct {
		uint32_t bShown : 1;
		uint32_t bDestroy : 1;
		uint32_t bFocused : 1;
		volatile uint32_t dirty : 1;
		volatile uint32_t wantBuffer : 1; // cannot be 'canCommit', because no new buffer can be used.
		volatile uint32_t canCommit : 1;
		uint32_t wantDraw : 1;
		uint32_t hidden : 1; // tracks whether the window is visible or not.
		uint32_t commited : 1; // if a redraw did not update anything, update everything.
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
	uint32_t bufw, bufh;
	struct wl_buffer *buff; // current buffer pointer
	int curBuffer;         // current buffer index
	//enum sizeDisplayValues changedEdge; // when resized, this is how to copy the existing image.
	#define MAX_OUTSTANDING_FRAMES 2
	int freeBuffer[MAX_OUTSTANDING_FRAMES];
	struct wl_buffer * buffers[MAX_OUTSTANDING_FRAMES];
	PCOLOR  color_buffers[MAX_OUTSTANDING_FRAMES];
	size_t buffer_sizes[MAX_OUTSTANDING_FRAMES];
	void* buffer_images[MAX_OUTSTANDING_FRAMES];
	//PLIST damage; // backing buffer was in use while damage happened...
	struct wl_shm_pool *pool;
	struct wl_callback *frame_callback;  // request for WM_PAINT
	//Image pImage;
} XPANEL, *PXPANEL;


struct mouseEvent {
	int32_t x, y;
	uint32_t b;
	PXPANEL r;
};

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
	wis_xdg_shell3,
	wis_xdg_base,
	wis_xdg_shell,
#ifdef ALLOW_KDE
	wis_kde_shell,
#endif
	max_interface_versions
};

struct pendingKey {
	uint rawKey;
	uint32_t key;
	uint32_t tick;
	PXPANEL r;
	int repeating;
};

struct wayland_local_tag
{
	int waylandThread;
	int drawThread;
	struct {
		volatile uint32_t bInited : 1;
		volatile uint32_t bFailed : 1;
		uint32_t bLogKeyEvent:1;
	} flags;
	//PLIST wantDraw;

	struct wl_event_queue* queue;
	struct wl_display* display;
	struct wl_compositor* compositor;
	struct output_data* outputSurfaces[12];  // contains wl_output's
	int nOutputSurfaces;
	struct wl_subcompositor* subcompositor;
	struct wl_shm *shm;
	volatile int registering; // valid pixel format found.
	int canDraw; // valid pixel format found.
	struct wl_shell *shell;
	struct xdg_wm_base *xdg_wm_base;
	struct xdg_shell *xdg_shell;
	struct wl_seat *seat;
	struct wl_keyboard_config {
		uint32_t repeat;
		uint32_t delay;
		//PLIST pendingKeys;
		//struct pendingKey pendingKey;
	} keyRepeat;
	struct wl_pointer *pointer;
	//PDATAQUEUE pdlMouseEvents;
	int32_t mouseSerial;
	struct pointer_data pointer_data ;
	struct wl_keyboard *keyboard;
	struct xkb_context *xkb_context;
	struct xkb_keymap *keymap;
	struct xkb_state *xkb_state;
	int versions[max_interface_versions]; // reported versions

	struct wl_surface *cursor_surface;
	struct wl_cursor_image *cursor_image;
	struct wl_cursor_theme *cursor_theme;

	//-------------- Global seat/input tracking
	struct mouseEvent mouse_;
	int keyMods;
	uint32_t  utfKeyCode;
	xkb_keysym_t keysym;

	PXPANEL hVidFocused; // keyboard events go here
	PXPANEL hCaptured; // send all mouse events here (probalby should-no-op this)
	//PLIST pActiveList; // non-destroyed windows are here

};


//----------------------------

