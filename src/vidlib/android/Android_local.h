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
} *PVPRENDER;

#define l vidlib_proxy_server_local
struct vidlib_proxy_local
{
	TEXTSTR application_title;
	PLIST renderers;
	PLIST images;
   PVPRENDER bottom;
	PIMAGE_INTERFACE real_interface;
	_8 key_states[256];
	CRITICALSECTION message_formatter;
	ANativeWindow *displayWindow;
	_32 default_display_x, default_display_y;
	PVPRENDER hVidVirtualFocused;
} l;

// linux_keymap.c
CTEXTSTR SACK_Vidlib_GetKeyText( int pressed, int key_index, int *used );
void SACK_Vidlib_ProcessKeyState( int pressed, int key_index, int *used );


// vidlib_touch.c
int HandleTouches( PVPRENDER r, PINPUT_POINT touches, int nTouches );

// android_keymap.c
void SACK_Vidlib_ShowInputDevice( void );
void SACK_Vidlib_HideInputDevice( void );
void SACK_Vidlib_ToggleInputDevice( void );

TEXTCHAR  AndroidANW_GetKeyText(int key);
