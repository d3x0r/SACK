#include "controlstruc.h"

#include <psi.h>

#include <psi/knob.h>

//---------------------------------------------------------------------------------

typedef struct scroll_knob
{
	struct knob_flags
	{
		BIT_FIELD draw_update_arc : 1;  // draw mouse-over updates
	} flags;
	int _b;
	int last_arc;
	KnobEvent event_handler;
	PTRSZVAL psvEvent;

	Image knob_image;
   PSPRITE knob_sprite;
	CTEXTSTR knob_image_file;


	fixed zero_angle;
	fixed width_scale;
   fixed height_scale;

} ScrollKnob, *PScrollKnob;

EasyRegisterControl( CONTROL_SCROLL_KNOB_NAME, sizeof( ScrollKnob ) );

static int OnCreateCommon( CONTROL_SCROLL_KNOB_NAME )( PSI_CONTROL pc )
{
   PScrollKnob knob = ControlData( PScrollKnob, pc );
	knob->width_scale = 0x100000000ULL;
	knob->height_scale = 0x100000000ULL;

	return 1;
}

static int OnMouseCommon( CONTROL_SCROLL_KNOB_NAME )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	int arc;
	int w, h;
	GetFrameSize( pc, &w, &h );
	x = x - (w/2);
	y = y - (h/2);
	// center deadzone
	if( (x < 10) && (x > -10) && (y < 10) && (y > -10) )
		return 1;
	if( x >= 0 && y >= 0 )
	{
		if( y < x/2 )
		{
			arc = 0;
		}
		else if ( y > (x * 2) )
		{
			arc = 2;
		}
		else
			arc = 1;
	}
	if( x < 0 && y >= 0 )
	{
		x = -x;
		if( y < x/2 )
		{
			arc = 5-0;
		}
		else if ( y > (x * 2) )
		{
			arc = 5-2;
		}
		else
			arc = 5-1;
	}
	if( x < 0 && y < 0 )
	{
		x = -x;
		y = -y;
		if( y < x/2 )
		{
			arc = 6 + 0;
		}
		else if ( y > (x * 2) )
		{
			arc = 6 + 2;
		}
		else
			arc = 6 + 1;
	}
	if( x >= 0 && y < 0 )
	{
		y = -y;
		if( y < x/2 )
		{
			arc = 11 - 0;
		}
		else if ( y > (x * 2) )
		{
			arc = 11 - 2;
		}
		else
			arc = 11 - 1;
	}
	
	if( knob->_b & MK_LBUTTON )
	{
		if( b & MK_LBUTTON )
		{
			int forward = arc - knob->last_arc;
			int backward = knob->last_arc - arc;
			if( forward < 0 )
				forward += 12;
			if( backward < 0 )
				backward += 12;
			if( forward < backward )
			{
				if( knob->event_handler )
               knob->event_handler( knob->psvEvent, forward );
			}
			else
			{
				if( knob->event_handler )
               knob->event_handler( knob->psvEvent, -backward );
			}
		}
		else
			knob->_b &= ~MK_LBUTTON;
		knob->last_arc = arc;
	}
	else
	{
		if( b & MK_LBUTTON )
		{
			knob->_b |= MK_LBUTTON;
			knob->last_arc = arc;
			knob->flags.draw_update_arc = 0;
		}
		else
		{
			knob->last_arc = arc;
			knob->flags.draw_update_arc = 1;
		}
	}
	SmudgeCommon( pc );
	return 1;
}

static int OnDrawCommon( CONTROL_SCROLL_KNOB_NAME )( PSI_CONTROL pc )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
		Image surface;
		surface = GetControlSurface( pc );
		if( !knob->knob_sprite )
		{
			ClearImageTo( surface, BASE_COLOR_RED );
			PutString( surface, 0, 0, BASE_COLOR_WHITE, 0, WIDE("No Image") );
			return 1;
		}
		ClearImageTo( surface, 0x1000001 );
		rotate_scaled_sprite( surface
								  , knob->knob_sprite
								  , ( ( 0x100000000LL/12 ) * knob->last_arc ) - knob->zero_angle
								  , knob->width_scale
								  , knob->height_scale
								  );
	}
	return 1;
}

#ifndef NO_TOUCH
static int OnTouchCommon( CONTROL_SCROLL_KNOB_NAME )( PSI_CONTROL pc, PTOUCHINPUT inputs, int input_count )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
	}
	return 0;
}
#endif

void SetScrollKnobEvent( PSI_CONTROL pc, KnobEvent event, PTRSZVAL psvEvent )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
		knob->psvEvent = psvEvent;
      knob->event_handler = event;
	}
}

PSI_PROC( void, SetScrollKnobImageName )( PSI_CONTROL pc, CTEXTSTR image )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
		knob->knob_image_file = StrDup( image );
		UnmakeImageFile( knob->knob_image );
		knob->knob_image = LoadImageFile( image );
		if( knob->knob_sprite )
			UnmakeSprite( knob->knob_sprite, 0 );
		knob->knob_sprite = MakeSpriteImage( knob->knob_image );
		{
			_32 image_width, image_height;
			int control_width, control_height;
			GetFrameSize( pc, &control_width, &control_height );
			GetImageSize( knob->knob_image, &image_width, &image_height );
			knob->width_scale = 0x10000 * control_width / image_width;
			knob->height_scale = 0x10000 * control_height / image_height;
			SetSpritePosition( knob->knob_sprite, control_width / 2, control_height / 2 );
			SetSpriteHotspot( knob->knob_sprite, image_width / 2, image_height / 2 );
		}
	}
}

PSI_PROC( void, SetScrollKnobImage )( PSI_CONTROL pc, Image image )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
		knob->knob_image = image;
		if( knob->knob_sprite )
			UnmakeSprite( knob->knob_sprite, 0 );
		knob->knob_sprite = MakeSpriteImage( knob->knob_image );
		{
			_32 image_width, image_height;
			int control_width, control_height;
			GetFrameSize( pc, &control_width, &control_height );
			GetImageSize( knob->knob_image, &image_width, &image_height );
			knob->width_scale = 0x10000 * control_width / image_width;
			knob->height_scale = 0x10000 * control_height / image_height;
			SetSpritePosition( knob->knob_sprite, control_width / 2, control_height / 2 );
			SetSpriteHotspot( knob->knob_sprite, image_width / 2, image_height / 2 );
		}
	}

}

// fixed is defined in image.h
// angle is a fixed scaled integer with 0x1 0000 0000 being the full circle.
PSI_PROC( void, SetScrollKnobImageZeroAngle )( PSI_CONTROL pc, fixed angle )
{
	PScrollKnob knob = ControlData( PScrollKnob, pc );
	if( knob )
	{
      knob->zero_angle = angle;
	}
}


