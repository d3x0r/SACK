

#include "global.h"
#include <sharemem.h>
#include <controls.h>
#include "controlstruc.h"
#include <psi.h>

PSI_NAMESPACE 

//---------------------------------------------------------------------------

static void CPROC StopThisProgram( PSI_CONTROL pc )
{
	PSI_CONTROL button;
	if( pc->device && pc->parent  && ( !(pc->BorderType & BORDER_WITHIN ) ) )
	{
		button = GetControl( pc, pc->device->nIDDefaultCancel );
		if( button )
			InvokeButton( button );
		else
		{
			button = GetControl( pc, pc->device->nIDDefaultOK );
			if( button )
				InvokeButton( button );
			else
				HideCommon( pc );
		}
	}
	else
		BAG_Exit( (int)'exit' );
}

//---------------------------------------------------------------------------

PCAPTION_BUTTON AddCaptionButton( PSI_CONTROL frame, Image normal, Image pressed, int extra_pad, void (CPROC*event)(PSI_CONTROL) )
{
	if( !( frame->BorderType & BORDER_CAPTION_NO_CLOSE_BUTTON ) )
	{
		if( !frame->flags.bCloseButtonAdded 
			&& ( frame->BorderType & BORDER_CAPTION_CLOSE_BUTTON ) )
		{
			frame->flags.bCloseButtonAdded = 1;
			AddCaptionButton( frame, (!event&&normal)?normal:g.StopButton, (!event&&normal)?normal:g.StopButtonPressed, (!event&&normal)?extra_pad:g.StopButtonPad, StopThisProgram );
		}
		else if( !event && frame->flags.bCloseButtonAdded )
		{
			PCAPTION_BUTTON close_button = (PCAPTION_BUTTON)GetLink( &frame->caption_buttons, 0 );
			close_button->normal = normal;
			close_button->pressed = pressed;
			close_button->extra_pad = extra_pad;
		}
	}
	if( frame && event )
	{
		PCAPTION_BUTTON button = New( CAPTION_BUTTON );
		button->normal = normal;
		button->pressed = pressed;
		button->pressed_event = event;
		button->flags.hidden = FALSE;
		button->is_pressed = FALSE;
		button->extra_pad = extra_pad;
		button->pc = frame;
		AddLink( &frame->caption_buttons, button );
		return button;
	}
	return NULL;
}

void SetCaptionButtonImages( struct physical_device_caption_button *caption_button, Image normal, Image pressed )
{
	if( caption_button )
	{
		caption_button->pressed = pressed;
		caption_button->normal = normal;
		if( caption_button->pc->device )
		{
			int y = FrameCaptionYOfs( caption_button->pc, caption_button->pc->BorderType );
			DrawFrameCaption( caption_button->pc );
			UpdateDisplayPortion( caption_button->pc->device->pActImg, caption_button->pc->surface_rect.x - 1, y
										, caption_button->pc->surface_rect.width + 2
										, caption_button->pc->surface_rect.y - y );
		}
	}
}

void HideCaptionButton ( struct physical_device_caption_button *caption_button )
{
	if( caption_button )
	{
		caption_button->flags.hidden = TRUE;
		if( caption_button->pc->device )
		{
			int y = FrameCaptionYOfs( caption_button->pc, caption_button->pc->BorderType );
			DrawFrameCaption( caption_button->pc );
			if( !caption_button->pc->flags.bResizedDirty ) // set during resize operation; don't output now.
				UpdateDisplayPortion( caption_button->pc->device->pActImg, caption_button->pc->surface_rect.x - 1, y
											, caption_button->pc->surface_rect.width + 2
											, caption_button->pc->surface_rect.y - y );
		}
	}
}
void ShowCaptionButton ( struct physical_device_caption_button *caption_button )
{
	if( caption_button )
	{
		caption_button->flags.hidden = FALSE;
		if( caption_button->pc->device )
		{
			int y = FrameCaptionYOfs( caption_button->pc, caption_button->pc->BorderType );
			DrawFrameCaption( caption_button->pc );
			if( !caption_button->pc->flags.bResizedDirty ) // set during resize operation; don't output now.
				UpdateDisplayPortion( caption_button->pc->device->pActImg, caption_button->pc->surface_rect.x - 1, y
											, caption_button->pc->surface_rect.width + 2
											, caption_button->pc->surface_rect.y - y );
		}
	}
}
//---------------------------------------------------------------------------


PSI_NAMESPACE_END

