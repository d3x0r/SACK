#include "../intershell_export.h"


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

	uint32_t min, max, current;	

	/* this control may be destroyed and recreated based on other options */
	PSI_CONTROL control;
	 TEXTSTR image_name;
	Image image;
	TEXTCHAR value[32];
	PVARIABLE label;

} SLIDER_INFO, *PSLIDER_INFO;


struct my_button
{
	PMENU_BUTTON button;
	int ID;
	struct ffmpeg_file *file;
	PRENDERER render;
	PSLIDER_INFO slider;
};

#ifndef VIDEO_PLAYER_MAIN_SOURCE
extern
#endif
struct ffmpeg_button_local
{
	uint32_t display_width, display_height;
	PRENDER_INTERFACE pdi;
	PIMAGE_INTERFACE pii;
	int button_id;
	int button_display_id;
	int button_pause_id;
	int button_stop_id;
	int slider_seek_id;
	PLIST buttons;
	PLIST files;
	uint32_t x_ofs;
	uint32_t y_ofs;
	PLIST active_files;
	int full_display;
	PVARIABLE label_active_display;
	int32_t mouse_x, mouse_y;
	uint32_t mouse_b;
	uint32_t mouse_first_click_tick;
}l;

// slider update... (passed the my_button containing the file... )
void UpdatePositionCallback( uintptr_t psv, uint64_t tick );
