#include  <android/native_window.h>


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
} *PVPRENDER;

#define l vidlib_proxy_server_local
struct vidlib_proxy_local
{
	TEXTSTR application_title;
	PLIST renderers;
	PLIST images;
	PIMAGE_INTERFACE real_interface;
	_8 key_states[256];
	CRITICALSECTION message_formatter;
	ANativeWindow *displayWindow;
   _32 default_display_x, default_display_y;

} l;

CTEXTSTR SACK_Vidlib_GetKeyText( int pressed, int key_index, int *used );
void SACK_Vidlib_ProcessKeyState( int pressed, int key_index, int *used );


