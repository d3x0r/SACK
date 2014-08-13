#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include "resource.h"

#include <controls.h>

#include "../intershell_export.h"
#include "../intershell_registry.h"

#include <ffmpeg_interface.h>
#include "video_player_local.h"

#define CONTROL_NAME WIDE("Video Player/Seek Slider")
//---------------------------------------------------------------------------

enum {
	CHECKBOX_HORIZONTAL = 42000
	, CHECKBOX_DRAGGING
	, EDIT_BACKGROUND_IMAGE
	, EDIT_MIN 
	, EDIT_MAX 
	, EDIT_CURRENT 

};




//---------------------------------------------------------------------------
static PTRSZVAL CPROC ReloadSliderColor( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC ReloadSliderBackColor( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderHorizontal( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderDragging( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderBackgroundImage( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderMinValue( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderMaxValue( PTRSZVAL psv, arg_list args );
static PTRSZVAL CPROC SetSliderCurrentValue( PTRSZVAL psv, arg_list args );


//---------------------------------------------------------------------------
PRELOAD( RegisterExtraSliderConfig )
{
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, CHECKBOX_HORIZONTAL, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, CHECKBOX_DRAGGING, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, EDIT_BACKGROUND_IMAGE, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, EDIT_MIN, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, EDIT_MAX, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/")CONTROL_NAME, EDIT_CURRENT, EDIT_FIELD_NAME );

}


static void CPROC OnSliderUpdateProc(PTRSZVAL psv, PSI_CONTROL pc, int val)
//Update current value
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;

//	lprintf( "OnSliderUpdateProc: info=%X  val=%d", info, val);

	if(!info)
		return;
	if( info->flags.skip_update )
		return;
	{
		INDEX idx;
		struct my_button *button;
		LIST_FORALL( l.buttons, idx, struct my_button *, button )
		{
			if( button->ID == info->ID )
			{
				if( button->file )
					ffmpeg_SeekFile( button->file, (_64)val * 1000 );
			}
		}
	}
	info->current = val;
	LabelVariableChanged( info->label );
}

static CTEXTSTR GetCurrentValueString( PTRSZVAL psv )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	snprintf( info->value, 32, "%"_32fs".%"_32fs"%%", info->current / 10000, info->current % 10000 );
	return info->value;
}

void UpdatePositionCallback( PTRSZVAL psv, _64 tick )
{
	struct my_button *button = (struct my_button*)psv;
	if( button->slider )
	{
		button->slider->flags.skip_update = 1;
		button->slider->current = tick/1000;
		SetSliderValues( button->slider->control, button->slider->min, (int)(tick / 1000), button->slider->max );
		button->slider->flags.skip_update = 0;
		LabelVariableChanged( button->slider->label );
	}
}

static PTRSZVAL OnCreateControl(CONTROL_NAME)( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{

	PSLIDER_INFO info = New( SLIDER_INFO );
	MemSet( info, 0, sizeof( *info ) );

	info->ID = l.slider_seek_id++;
	 //Default values
	info->color = BASE_COLOR_WHITE;

	info->min = 0;
	info->max = 1000000;
	info->current = 1;
	if( w > h )
		info->flags.bHorizontal = 1;
	else
		info->flags.bHorizontal = 0;


	info->control = MakeControl( frame
												, SLIDER_CONTROL
									  , x
									  , y
									  , w
									  , h
									  , -1
									  );

	{
		INDEX idx;
		struct my_button *button;
		LIST_FORALL( l.buttons, idx, struct my_button *, button )
		{
			if( button->ID == info->ID )
			{
				button->slider = info;
			}
		}
	}

	SetSliderUpdateHandler( info->control, OnSliderUpdateProc, (PTRSZVAL)info );	//info (pointer) will be returned in OnSliderUpdateProc()
	 
	// none of these are accurate values, they are just default WHITE and nothing.
	InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	
	//It seems that this magic line helps in showing slider
	MoveSizeCommon( info->control, x, y, w, h );
	{
		TEXTCHAR tmpname[32];
		tnprintf( tmpname, 32, WIDE( "Player %d position" ), info->ID );
		info->label = CreateLabelVariableEx( tmpname, LABEL_TYPE_PROC_EX, GetCurrentValueString, (PTRSZVAL)info );
	}

	// the result of this will be hidden...
	return (PTRSZVAL)info;
}




static PTRSZVAL OnConfigureControl( CONTROL_NAME )( PTRSZVAL psv, PSI_CONTROL parent_frame )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	{
		PCOMMON frame = NULL; 
		int okay = 0;
		int done = 0;
		if( !frame )
		{
			frame = LoadXMLFrame( WIDE("seek_slider_properties.frame") );
			if( frame )
			{
				 TEXTCHAR buffer[256];

				SetCommonButtonControls( frame );
				
				SetCheckState( GetControl( frame, CHECKBOX_HORIZONTAL ), info->flags.bHorizontal );
				SetCheckState( GetControl( frame, CHECKBOX_DRAGGING ), info->flags.bDragging );
				SetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), info->image_name );

					 snprintf(buffer, sizeof(buffer), WIDE("%d"), info->min);
				SetControlText( GetControl( frame, EDIT_MIN ), buffer );

					 snprintf(buffer, sizeof(buffer), WIDE("%d"), info->max);
				SetControlText( GetControl( frame, EDIT_MAX ), buffer );
				
					 snprintf(buffer, sizeof(buffer), WIDE("%d"), info->current);
				SetControlText( GetControl( frame, EDIT_CURRENT ), buffer );
				
				SetCommonButtons( frame, &done, &okay );
				DisplayFrameOver( frame, parent_frame );
				CommonWait( frame );

				if( okay )
				{
					GetCommonButtonControls( frame );
					info->font = InterShell_GetCurrentButtonFont();
					if( info->font )
						SetCommonFont( info->control, (*info->font ) );
					info->color = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
					info->backcolor = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
					{
						GetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), buffer, sizeof( buffer ) );
						if( info->image_name )
							Release( info->image_name );
						info->image_name = StrDup( buffer );
					}

//					SetSliderColor( info->control, info->color );
//					SetSliderkBackColor( info->control, info->backcolor );

					info->flags.bHorizontal = GetCheckState( GetControl( frame, CHECKBOX_HORIZONTAL ) );
					info->flags.bDragging = GetCheckState( GetControl( frame, CHECKBOX_DRAGGING ) );

					GetControlText( GetControl( frame, EDIT_MIN), buffer, sizeof( buffer ) );
					info->min = atoi(buffer);
					GetControlText( GetControl( frame, EDIT_MAX), buffer, sizeof( buffer ) );
					info->max = atoi(buffer);
					GetControlText( GetControl( frame, EDIT_CURRENT), buffer, sizeof( buffer ) );
					info->current = atoi(buffer);

				}
				DestroyFrame( &frame );
			}
		}
	}
	return psv;
}


static void OnSaveControl( WIDE( CONTROL_NAME ) )( FILE *file,PTRSZVAL psv )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	fprintf( file, WIDE("Seek Slider color=$%02X%02X%02X%02X\n")
			 , AlphaVal( info->color )
			 , RedVal( info->color )
			 , GreenVal( info->color )
			 , BlueVal( info->color )
			 );
	fprintf( file, WIDE("Seek Slider back color=$%02X%02X%02X%02X\n")
			 , AlphaVal( info->backcolor )
			 , RedVal( info->backcolor )
			 , GreenVal( info->backcolor )
			 , BlueVal( info->backcolor )
			 );
	fprintf( file, WIDE("Seek Slider background image=%s\n" ), info->image_name?info->image_name:WIDE("") );
	fprintf( file, WIDE("Seek Slider is horizontal?%s\n"), info->flags.bHorizontal?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("Seek Slider is draggable?%s\n"), info->flags.bDragging?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("Seek Slider min value=%d\n"), info->min);
	fprintf( file, WIDE("Seek Slider max value=%d\n"), info->max);
	fprintf( file, WIDE("Seek Slider current value=%d\n"), info->current);


	InterShell_SaveCommonButtonParameters( file );

}


static PTRSZVAL CPROC ReloadSliderColor( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, CDATA, color );
	info->color = color;
	return psv;
}

static PTRSZVAL CPROC ReloadSliderBackColor( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, CDATA, color );
	info->backcolor = color;
	return psv;
}

static PTRSZVAL CPROC SetSliderHorizontal( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, LOGICAL, bHorizontal );
	info->flags.bHorizontal = bHorizontal;
	SetSliderOptions( info->control, info->flags.bHorizontal ? SLIDER_HORIZ : SLIDER_VERT );

	return psv;
}

static PTRSZVAL CPROC SetSliderDragging( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, LOGICAL, bDragging );
	info->flags.bDragging = bDragging;
	return psv;
}


static PTRSZVAL CPROC SetSliderBackgroundImage( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	info->image_name = StrDup( name );

	return psv;
}

static PTRSZVAL CPROC SetSliderMinValue( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, S_32, min );
	info->min = min;
	return psv;
}

static PTRSZVAL CPROC SetSliderMaxValue( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, S_32, max );
	info->max = max;
	return psv;
}
static PTRSZVAL CPROC SetSliderCurrentValue( PTRSZVAL psv, arg_list args )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	PARAM( args, S_32, current );
	info->current = current;
	return psv;
}

static void OnLoadControl( WIDE( CONTROL_NAME ) )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	AddConfigurationMethod( pch, WIDE("Seek Slider color=%c"), ReloadSliderColor );
	AddConfigurationMethod( pch, WIDE("Seek Slider back color=%c"), ReloadSliderBackColor );
	AddConfigurationMethod( pch, WIDE("Seek Slider is horizontal?%b"), SetSliderHorizontal );
	AddConfigurationMethod( pch, WIDE("Seek Slider is draggable?%b"), SetSliderDragging );
	AddConfigurationMethod( pch, WIDE("Seek Slider background image=%m" ), SetSliderBackgroundImage );
	AddConfigurationMethod( pch, WIDE("Seek Slider min value=%i"), SetSliderMinValue );
	AddConfigurationMethod( pch, WIDE("Seek Slider max value=%i"), SetSliderMaxValue );
	AddConfigurationMethod( pch, WIDE("Seek Slider current value=%i"), SetSliderCurrentValue );

}

static void OnFixupControl( CONTROL_NAME )(  PTRSZVAL psv )
{
	_32 opt;
	PSLIDER_INFO info = (PSLIDER_INFO)psv;

	if( info )
	{
//		SetSliderColor( info->control, info->color );
//		SetSliderBackColor( info->control, info->backcolor );
		 opt = info->flags.bHorizontal ? SLIDER_HORIZ : SLIDER_VERT;
		SetSliderValues( info->control, info->min, info->current, info->max );
		SetSliderOptions( info->control, opt );
		InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	}
}

static PSI_CONTROL OnGetControl( CONTROL_NAME )( PTRSZVAL psv )
{
	PSLIDER_INFO info = (PSLIDER_INFO)psv;
	return info->control;
}


static LOGICAL OnQueryShowControl( CONTROL_NAME )( PTRSZVAL psv )
{
	_32 opt;
	PSLIDER_INFO info = (PSLIDER_INFO)psv;

	if( info ) 
	{	
		if(info->min == 0 && info->max == 0 && info->current == 0)
		{
			info->min = 0;
			info->max = 100;
			info->current = 10;
		}

		opt = info->flags.bHorizontal ? SLIDER_HORIZ : SLIDER_VERT;
		SetSliderValues( info->control, info->min, info->current, info->max );
		SetSliderOptions( info->control, opt );

	}

	return TRUE;
}

//---------------------------------------------------------------------------

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
