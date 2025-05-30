

#include "controlstruc.h"
#include <psi.h>

PSI_NAMESPACE

   struct progress_bar_data
   {
       // declare some data that you want to have associated with your control.
       CDATA bar_color;
       CDATA back_color;
       uint32_t range; // total range of progress bar
	   uint32_t progress; // current progress portion
   };
   
//EasyRegisterControl( PROGRESS_BAR_CONTROL_NAME, sizeof( struct progress_bar_data ) );

static CONTROL_REGISTRATION progressBarControl= { PROGRESS_BAR_CONTROL_NAME
			, { 32, 32, sizeof( struct progress_bar_data ), BORDER_THINNER } };
PRELOAD( registerProgressBarcontrol ){ DoRegisterControl( &progressBarControl ); }
static uint32_t* pb_MyControlID = &progressBarControl.TypeID;

static int OnCreateCommon( PROGRESS_BAR_CONTROL_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( struct progress_bar_data *, pb_MyControlID[0], data, pc );
	if( data )
	{
		data->range = 100;
		data->progress = 0;
		data->bar_color = BASE_COLOR_BLUE;
		data->back_color = BASE_COLOR_DARKGREY;
	}
	return 1;
}

static int OnDrawCommon( PROGRESS_BAR_CONTROL_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( struct progress_bar_data *, pb_MyControlID[0], data, pc );
	if( data )
	{
		Image surface = GetControlSurface( pc );
		uint32_t c = (data->progress > 0)?(data->progress < data->range)?data->progress:data->range:0;
		uint32_t w = ( surface->width * c ) / data->range;
		BlatColor( surface, 0, 0, w, surface->height, data->bar_color );
		BlatColor( surface, w, 0, surface->width - w, surface->height, data->back_color );

		return 1;
	}
	return 0;
}

void ProgressBar_SetRange( PSI_CONTROL pc, int range )
{
	ValidatedControlData( struct progress_bar_data *, pb_MyControlID[0], data, pc );
	if( data )
	{
		data->range = range;
	}
}

void ProgressBar_SetProgress( PSI_CONTROL pc, int progress )
{
	ValidatedControlData( struct progress_bar_data *, pb_MyControlID[0], data, pc );
	if( data )
	{
		data->progress = progress;

		SmudgeCommon( pc );
	}
}

void ProgressBar_SetColors( PSI_CONTROL pc, CDATA background, CDATA foreground )
{
	ValidatedControlData( struct progress_bar_data *, pb_MyControlID[0], data, pc );
	if( data )
	{
		data->back_color = background;
		data->bar_color = foreground;
	}
}

void ProgressBar_EnableText( PSI_CONTROL pc, LOGICAL enable )
{
}

PSI_NAMESPACE_END

