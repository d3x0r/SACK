

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
	if( ( pc->device && pc->parent  && ( !(pc->BorderType & BORDER_WITHIN ) ) )
		|| ( pc->BorderType & BORDER_CAPTION_CLOSE_IS_DONE ) )
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
				HideControl( pc );
		}
	}
	else
		BAG_Exit( (int)'exit' );
}

//---------------------------------------------------------------------------

PCAPTION_BUTTON AddCaptionButton( PSI_CONTROL frame, Image normal, Image pressed, Image highlight, int extra_pad, void (CPROC*event)(PSI_CONTROL) )
{
	if( !( frame->BorderType & BORDER_CAPTION_NO_CLOSE_BUTTON ) )
	{
		if( !frame->flags.bCloseButtonAdded 
			&& ( frame->BorderType & BORDER_CAPTION_CLOSE_BUTTON ) )
		{
			frame->flags.bCloseButtonAdded = 1;
			AddCaptionButton( frame, (!event&&normal)?normal:g.StopButton
							, (!event&&pressed)?pressed:g.StopButtonPressed
							, (!event&&highlight)?highlight:NULL
							, (!event&&normal)?extra_pad:g.StopButtonPad, StopThisProgram );
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
		button->highlight = highlight;
		button->pressed_event = event;
		button->flags.hidden = FALSE;
		button->flags.rollover = FALSE;
		button->is_pressed = FALSE;
		button->extra_pad = extra_pad;
		button->pc = frame;
		AddLink( &frame->caption_buttons, button );
		return button;
	}
	return NULL;
}

void SetCaptionButtonImages( struct physical_device_caption_button *caption_button, Image normal, Image pressed, Image rollover )
{
	if( caption_button )
	{
		caption_button->pressed = pressed;
		caption_button->normal = normal;
		caption_button->highlight = rollover;
		if( caption_button->pc->device )
		{
			int y = FrameCaptionYOfs( caption_button->pc, caption_button->pc->BorderType );
			LockRenderer( caption_button->pc->device->pActImg );
			DrawFrameCaption( caption_button->pc );
			UpdateDisplayPortion( caption_button->pc->device->pActImg, caption_button->pc->surface_rect.x - 1, y
										, caption_button->pc->surface_rect.width + 2
										, caption_button->pc->surface_rect.y - y );
			UnlockRenderer( caption_button->pc->device->pActImg );
		}
	}
}

static void RefreshCaption( PSI_CONTROL pc, PPHYSICAL_DEVICE device )
{
	//if( !g.flags.allow_threaded_draw )
	if( device->pActImg && !pc->flags.bOpeningFrameDisplay )
	{
		int y = FrameCaptionYOfs( pc, pc->BorderType );
		LockRenderer( device->pActImg );
		DrawFrameCaption( pc );
		if( !pc->flags.bResizedDirty ) // set during resize operation; don't output now.
			UpdateDisplayPortion( device->pActImg, pc->surface_rect.x - 1, y
										, pc->surface_rect.width + 2
										, pc->surface_rect.y - y );
		UnlockRenderer( device->pActImg );
	}
	/*
	else
	{
		pc->flags.bResizedDirty = 1;
		Redraw( device->pActImg );
	}
	*/
}

void HideCaptionButton ( struct physical_device_caption_button *caption_button )
{
	if( caption_button )
	{
		caption_button->flags.hidden = TRUE;
		if( caption_button->pc->device )
		{
			RefreshCaption( caption_button->pc, caption_button->pc->device );

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
			RefreshCaption( caption_button->pc, caption_button->pc->device );
		}
	}
}
//---------------------------------------------------------------------------

void SetCaptionButtonOffset( PSI_CONTROL frame, int32_t x, int32_t y )
{
	if( frame )
	{
		frame->caption_button_x_ofs = x;
		frame->caption_button_y_ofs = y;
	}
}

//---------------------------------------------------------------------------

PSI_NAMESPACE_END

