#include  <android/native_window.h>

#define USE_IMAGE_INTERFACE l.real_interface
#define NEED_REAL_IMAGE_STRUCTURE
#include <imglib/imagestruct.h>

#include <render.h>
#include <render3d.h>
#include <image3d.h>


typedef struct vidlib_proxy_renderer
{
	_32 w, h;
	S_32 x, y;
	_32 attributes;
	struct vidlib_proxy_renderer *above, *under;
	Image image;  // representation of the output surface
	struct vidlib_proxy_renderer_flags
	{
		BIT_FIELD hidden : 1;
		BIT_FIELD fullscreen : 1;
		BIT_FIELD not_fullscreen : 1;
		BIT_FIELD mouse_transparent : 1; // hidden to mouse events; overlays
	} flags;
	INDEX id;
	MouseCallback mouse_callback;
	PTRSZVAL psv_mouse_callback;
	KeyProc key_callback;
	PTRSZVAL psv_key_callback;
	RedrawCallback redraw;
	PTRSZVAL psv_redraw;
	TouchCallback touch_callback;
	PTRSZVAL psv_touch_callback;
	struct touch_event_state
	{
		S_32 mouse_x, mouse_y;
		struct touch_event_one{
			struct touch_event_one_flags {
				BIT_FIELD bDrag : 1;
			} flags;
			RCOORD x;
			RCOORD y;
		} one;
		struct touch_event_two{
			RCOORD x;
			RCOORD y;
			//RCOORD begin_length;
		} two;
		struct touch_event_three{
			RCOORD x;
			RCOORD y;
			RCOORD begin_lengths[3]; //3 lengths for segments 1->2, 2->3, 1->3
		} three;
	} touch_info;
	struct default_mouse_info
	{
      S_32 lock_x, lock_y;
	} mouse_info;
} *PVPRENDER;

#define l vidlib_proxy_server_local
struct vidlib_android_local
{
	TEXTSTR application_title;
	PLIST renderers;
	PLIST images;
	PVPRENDER bottom;
	PVPRENDER top;
	CRITICALSECTION cs_update;
	PIMAGE_INTERFACE real_interface;
	_8 key_states[256];
	CRITICALSECTION message_formatter;
	ANativeWindow *displayWindow;
	S_32 default_display_x, default_display_y;
	PVPRENDER hVidVirtualFocused;
	struct vidlib_android_local_flags {
		BIT_FIELD paused : 1;
		BIT_FIELD full_screen_renderer : 1;
		BIT_FIELD display_closed : 1; // prevent drawing until it re-opens
	} flags;
	PVPRENDER full_screen_display;
	void(*SuspendSleep)(int);
	S_32 mouse_x, mouse_y;
	_32 mouse_b;
	_32 mouse_first_click_tick;
} l;

// linux_keymap.c
CTEXTSTR SACK_Vidlib_GetKeyText( int pressed, int key_index, int *used );
void SACK_Vidlib_ProcessKeyState( int pressed, int key_index, int *used );


// vidlib_touch.c
int HandleTouches( PVPRENDER r, PINPUT_POINT touches, int nTouches );
void TouchWindowClose( PVPRENDER r );

// android_keymap.c
void SACK_Vidlib_ShowInputDevice( void );
void SACK_Vidlib_HideInputDevice( void );
void SACK_Vidlib_ToggleInputDevice( void );

TEXTCHAR  AndroidANW_GetKeyText(int key);

// ANdroid_nativewindow.c
void SACK_Vidlib_SetSleepSuspend( void(*)(int));

