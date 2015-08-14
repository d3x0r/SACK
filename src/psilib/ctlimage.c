/* basic image control - show a scaled image in the control space, no border */

#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "controlstruc.h"
#include <controls.h>
#include <keybrd.h>
#include <psi.h>

struct image_display {
	Image image;
};
typedef struct image_display IMAGE_DISPLAY, *PIMAGE_DISPLAY;

CONTROL_REGISTRATION
image_display_control = { IMAGE_DISPLAY_CONTROL_NAME
					, { { 64, 64}, sizeof( IMAGE_DISPLAY ), BORDER_NONE }
};

PRIORITY_PRELOAD( RegisterImageDisplay, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &image_display_control );
}

static int OnCreateCommon( IMAGE_DISPLAY_CONTROL_NAME )( PSI_CONTROL pc )
{
	SetCommonTransparent( pc, TRUE );
	return 1;
}

static int OnDrawCommon( IMAGE_DISPLAY_CONTROL_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PIMAGE_DISPLAY, image_display_control.TypeID, image, pc );
	if( image )
	{
		Image surface = GetControlSurface( pc );
		BlotScaledImage( surface, image->image );
	}
	return 1;
}

void SetImageControlImage( PSI_CONTROL pc, Image show_image )
{
	ValidatedControlData( PIMAGE_DISPLAY, image_display_control.TypeID, image, pc );
	if( image )
	{
		image->image = show_image;
	}
}

