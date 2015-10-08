/* this should be used as a top-most mouse-hover... it should capture the mouse(?) and disappear on motion */


#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "controlstruc.h"
#include <controls.h>
#include <keybrd.h>
#include <psi.h>

struct tool_tip_display {
	CTEXTSTR text;
};
typedef struct tool_tip_display TOOL_TIP_DISPLAY, *PTOOL_TIP_DISPLAY;

CONTROL_REGISTRATION
tool_tip_display_control = { IMAGE_DISPLAY_CONTROL_NAME
					, { { 256, 32}, sizeof( TOOL_TIP_DISPLAY ), BORDER_THINNER }
};

PRIORITY_PRELOAD( RegisterImageDisplay, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &tool_tip_display_control );
}

void SetControlHoverTip( PSI_CONTROL pc, CTEXTSTR text )
{
}


