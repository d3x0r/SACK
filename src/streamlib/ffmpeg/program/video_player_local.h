
#define USES_INTERSHELL_INTERFACE
#define USE_RENDER_INTERFACE l.pdi
#define USE_IMAGE_INTERFACE l.pii
#include <render.h>

#include <psi.h>
//#include "../intershell_export.h"
#include <ffmpeg_interface.h>


typedef struct slider_info_tag
{
	int ID; // mate with player button
	struct {
		BIT_FIELD bHorizontal : 1; // vertical if not horizontal
		BIT_FIELD bDragging	: 1; // clicked and held...
		BIT_FIELD skip_update : 1;
	} flags;

	CDATA backcolor, color;

	SFTFont *font;

	_32 min, max, current;	

	/* this control may be destroyed and recreated based on other options */
	PSI_CONTROL control;
	 TEXTSTR image_name;
	Image image;
	TEXTCHAR value[32];
	//PVARIABLE label;

} SLIDER_INFO, *PSLIDER_INFO;


struct my_button
{
	struct {
		BIT_FIELD showing_panel : 1;
	} flags;
	//PMENU_BUTTON button;
	int ID;
	struct ffmpeg_file *file;
	PRENDERER render;
	//PSLIDER_INFO slider;
};

#ifndef VIDEO_PLAYER_MAIN_SOURCE
extern
#endif
struct ffmpeg_button_local
{
	_32 display_width, display_height;
	PRENDER_INTERFACE pdi;
	PIMAGE_INTERFACE pii;
	int button_id;
	int button_display_id;
	int button_pause_id;
	int button_stop_id;
	int slider_seek_id;
	PLIST buttons;
	PLIST files;
	_32 x_ofs;
	_32 y_ofs;
	PLIST active_files;
	int full_display;
	//PVARIABLE label_active_display;
  S_32 mouse_x, mouse_y;
	_32 mouse_b;
	_32 mouse_first_click_tick;
	PLIST videos_played; //list of CTEXTSTR
   PTHREAD main_thread;
   PTHREAD player_thread;
	LOGICAL stopped;
	LOGICAL quit;

	struct my_button me;
   PRENDERER renderer;
	//--- control_panel.c shared
	PLIST controls; // list of struct media_control_panel

	LOGICAL loop; // controls whether to play once or continuously
	LOGICAL stretch_full_display ;
	LOGICAL touch_to_pause;
	LOGICAL topmost;

	int touch_step;
	int last_touch; // when touch began.
	int exit_code; // what to return when done.
}l;

// slider update... (passed the my_button containing the file... )
void UpdatePositionCallback( PTRSZVAL psv, _64 tick );


void ShowMediaPanel( struct my_button *media );
void HideMediaPanel( struct my_button *media );


