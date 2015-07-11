

#include "controlstruc.h"
#include <psi.h>


   struct progress_bar_data
   {
       // declare some data that you want to have associated with your control.
       CDATA bar_color;
       CDATA back_color;
       _32 range; // total range of progress bar
	   _32 progress; // current progress portion
   };
   
   EasyRegisterControl( PROGRESS_BAR_CONTROL_NAME, sizeof( struct progress_bar_data ) );


static int OnCreateCommon( PROGRESS_BAR_CONTROL_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( struct progress_bar_data *, MyControlID, data, pc );
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
	ValidatedControlData( struct progress_bar_data *, MyControlID, data, pc );
	if( data )
	{
		Image surface = GetControlSurface( pc );
		_32 c = (data->progress > 0)?(data->progress < data->range)?data->progress:data->range:0;
		_32 w = ( surface->width * c ) / data->range;
		BlatColor( surface, 0, 0, w, surface->height, data->bar_color );
		BlatColor( surface, w, 0, surface->width - w, surface->height, data->back_color );

		return 1;
	}
	return 0;
}

void ProgressBar_SetRange( PSI_CONTROL pc, int range )
{
	ValidatedControlData( struct progress_bar_data *, MyControlID, data, pc );
	if( data )
	{
		data->range = range;
	}
}

void ProgressBar_SetProgress( PSI_CONTROL pc, int progress )
{
	ValidatedControlData( struct progress_bar_data *, MyControlID, data, pc );
	if( data )
	{
		data->progress = progress;

		SmudgeCommon( pc );
	}
}

void ProgressBar_SetColors( PSI_CONTROL pc, CDATA background, CDATA foreground )
{
	ValidatedControlData( struct progress_bar_data *, MyControlID, data, pc );
	if( data )
	{
		data->back_color = background;
		data->bar_color = foreground;
	}
}

void ProgressBar_EnableText( PSI_CONTROL pc, LOGICAL enable )
{
}



