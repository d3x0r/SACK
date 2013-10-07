#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <stdhdrs.h> //DebugBreak()
#include "resource.h"

#include <controls.h>
#include <psi/clock.h>

#include "intershell_export.h"
#include "intershell_registry.h"

//---------------------------------------------------------------------------
USE_PSI_CLOCK_NAMESPACE

enum {
	CHECKBOX_ANALOG = 4000
	  , CHECKBOX_DATE
	  , CHECKBOX_SINGLE_LINE
	  , CHECKBOX_DAY_OF_WEEK
	  , EDIT_BACKGROUND_IMAGE
     , EDIT_ANALOG_IMAGE
	  , CHECKBOX_AM_PM
};

typedef struct clock_info_tag
{
	struct {
		BIT_FIELD bAnalog : 1;  // we want to set analog, first show will enable
		BIT_FIELD bSetAnalog : 1; // did we already set analog?
		BIT_FIELD bDate : 1;
		BIT_FIELD bDayOfWeek : 1;
		BIT_FIELD bSingleLine : 1;
		BIT_FIELD bAmPm : 1;
	} flags;

	CDATA backcolor, color;

	//SFTFont *font;

   /* this control may be destroyed and recreated based on other options */
	PSI_CONTROL control;
   TEXTSTR image_name;
	Image image;
   TEXTSTR analog_image_name;
   struct clock_image_thing image_desc;
} CLOCK_INFO, *PCLOCK_INFO;

PRELOAD( RegisterExtraClockConfig )
{
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), CHECKBOX_DATE, RADIO_BUTTON_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), CHECKBOX_DAY_OF_WEEK, RADIO_BUTTON_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), CHECKBOX_ANALOG, RADIO_BUTTON_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), CHECKBOX_DATE, RADIO_BUTTON_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), EDIT_BACKGROUND_IMAGE, EDIT_FIELD_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), EDIT_ANALOG_IMAGE, EDIT_FIELD_NAME );
   EasyRegisterResource( WIDE("InterShell/") WIDE("Clock"), CHECKBOX_AM_PM, RADIO_BUTTON_NAME );
}

OnCreateControl(WIDE("Clock"))
/*PTRSZVAL CPROC CreateClock*/( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{
	PCLOCK_INFO info = New( CLOCK_INFO );
   MemSet( info, 0, sizeof( *info ) );
	info->color = BASE_COLOR_WHITE;

	info->control = MakeNamedControl( frame
											  , WIDE("Basic Clock Widget")
											  , x
											  , y
											  , w
											  , h
											  , -1
											  );

   // none of these are accurate values, they are just default WHITE and nothing.
	InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	SetClockColor( info->control, info->color );
	SetClockBackColor( info->control, info->backcolor );
	// need to supply extra information about the image, location of hands and face in image
   // and the spots...
	//MakeClockAnalog( info->control );
	//info->font = InterShell_GetCurrentButtonFont();
	//if( info->font )
	//	SetCommonFont( info->control, (*info->font ) );
	// the result of this will be hidden...
	return (PTRSZVAL)info;
}

OnShowControl( WIDE("Clock") )( PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
   SetClockAmPm( info->control, info->flags.bAmPm );
   SetClockDate( info->control, info->flags.bDate );
   SetClockDayOfWeek( info->control, info->flags.bDayOfWeek );
   SetClockSingleLine( info->control, info->flags.bSingleLine );
	StartClock( info->control );
}

OnConfigureControl( WIDE("Clock") )( PTRSZVAL psv, PSI_CONTROL parent_frame )
//PTRSZVAL CPROC ConfigureClock( PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	{
		PCOMMON frame = NULL; 
		int okay = 0;
		int done = 0;
		if( !frame )
		{
			frame = LoadXMLFrameOver( parent_frame, WIDE("Clock_Properties.isFrame") );
			//frame = CreateFrame( WIDE("Clock Properties"), 0, 0, 420, 250, 0, NULL );
			if( frame )
			{
				//MakeTextControl( frame, 5, 15, 120, 18, TXT_STATIC, WIDE("Text Color"), 0 );
				//EnableColorWellPick( MakeColorWell( frame, 130, 15, 18, 18, CLR_TEXT_COLOR, info->color ), TRUE );
				SetCommonButtonControls( frame );
				SetCheckState( GetControl( frame, CHECKBOX_DATE ), info->flags.bDate );
				SetCheckState( GetControl( frame, CHECKBOX_SINGLE_LINE ), info->flags.bSingleLine );
				SetCheckState( GetControl( frame, CHECKBOX_DAY_OF_WEEK ), info->flags.bDayOfWeek );
				SetCheckState( GetControl( frame, CHECKBOX_ANALOG ), info->flags.bAnalog );
				SetCheckState( GetControl( frame, CHECKBOX_AM_PM ), info->flags.bAmPm );
				SetControlText( GetControl( frame, EDIT_ANALOG_IMAGE ), info->analog_image_name );
				SetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), info->image_name );
				//EnableColorWellPick( SetColorWell( GetControl( frame, CLR_TEXT_COLOR), page_label->color ), TRUE );
				//SetButtonPushMethod( GetControl( frame, CHECKBOX_ANALOG ), ChangeClockStyle );
				SetCommonButtons( frame, &done, &okay );
				DisplayFrameOver( frame, parent_frame );
				CommonWait( frame );
				if( okay )
				{
               		TEXTCHAR buffer[256];
					GetCommonButtonControls( frame );
					//info->font = InterShell_GetCurrentButtonFont();
					//if( info->font )
                  			//SetCommonFont( info->control, (*info->font ) );
					info->color = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
					info->backcolor = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
					{
						GetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), buffer, sizeof( buffer ) );
						if( info->image_name )
							Release( info->image_name );
						info->image_name = StrDup( buffer );
					}
					{
						GetControlText( GetControl( frame, EDIT_ANALOG_IMAGE ), buffer, sizeof( buffer ) );
						if( info->analog_image_name )
							Release( info->analog_image_name );
						info->analog_image_name = StrDup( buffer );
					}
					SetClockColor( info->control, info->color );
					SetClockBackColor( info->control, info->backcolor );
					info->flags.bAmPm = GetCheckState( GetControl( frame, CHECKBOX_AM_PM ) );
					info->flags.bAnalog = GetCheckState( GetControl( frame, CHECKBOX_ANALOG ) );
					info->flags.bDate = GetCheckState( GetControl( frame, CHECKBOX_DATE ) );
					info->flags.bSingleLine = GetCheckState( GetControl( frame, CHECKBOX_SINGLE_LINE ) );
					info->flags.bDayOfWeek = GetCheckState( GetControl( frame, CHECKBOX_DAY_OF_WEEK ) );
					if( info->image )
						SetClockBackImage( info->control, info->image );
				}
				DestroyFrame( &frame );
			}
		}
	}
   return psv;
}


OnSaveControl( WIDE( "Clock" ) )( FILE *file,PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	fprintf( file, WIDE("%sClock color=$%02lX%02lX%02lX%02lX\n")
			 , InterShell_GetSaveIndent()
			 , AlphaVal( info->color )
			 , RedVal( info->color )
			 , GreenVal( info->color )
			 , BlueVal( info->color )
			 );
	fprintf( file, WIDE("%sClock back color=$%02lX%02lX%02lX%02lX\n")
			 , InterShell_GetSaveIndent()
			 , AlphaVal( info->backcolor )
			 , RedVal( info->backcolor )
			 , GreenVal( info->backcolor )
			 , BlueVal( info->backcolor )
			 );
	fprintf( file, WIDE("%sClock background image=%s\n" ), InterShell_GetSaveIndent(), info->image_name?info->image_name:WIDE("") );
	fprintf( file, WIDE("%sClock is analog?%s\n"), InterShell_GetSaveIndent(), info->flags.bAnalog?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("%sClock is military time?%s\n"), InterShell_GetSaveIndent(), (!info->flags.bAmPm)?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("%sClock show date?%s\n"), InterShell_GetSaveIndent(), info->flags.bDate?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("%sClock is single line?%s\n"), InterShell_GetSaveIndent(), info->flags.bSingleLine?WIDE("Yes"):WIDE("No") );
	fprintf( file, WIDE("%sClock show day of week?%s\n"), InterShell_GetSaveIndent(), info->flags.bDayOfWeek?WIDE("Yes"):WIDE("No") );

	fprintf( file, WIDE("%sClock analog image=%s\n" ), InterShell_GetSaveIndent(), info->analog_image_name?info->analog_image_name:WIDE("images/Clock.png") );
	{
		TEXTSTR out;
      EncodeBinaryConfig( &out, &info->image_desc, sizeof( info->image_desc ) );
		fprintf( file, WIDE("%sClock analog description=%s\n" ), InterShell_GetSaveIndent(), out );
		Release( out );
	}

	InterShell_SaveCommonButtonParameters( file );

}


static PTRSZVAL CPROC ReloadClockColor( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CDATA, color );
   info->color = color;
   return psv;
}

static PTRSZVAL CPROC ReloadClockBackColor( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CDATA, color );
   info->backcolor = color;
   return psv;
}

static PTRSZVAL CPROC SetClockAnalog( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, bAnalog );
	info->flags.bAnalog = bAnalog;
   return psv;
}
static PTRSZVAL CPROC ConfigSetClockAmPm( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bAmPm = !yes_no;
   return psv;
}
static PTRSZVAL CPROC ConfigSetClockDate( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bDate = yes_no;
   return psv;
}
static PTRSZVAL CPROC ConfigSetClockDayOfWeek( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bDayOfWeek = yes_no;
   return psv;
}
static PTRSZVAL CPROC ConfigSetClockSingleLine( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bSingleLine = yes_no;
   return psv;
}

static PTRSZVAL CPROC SetClockAnalogImage( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	info->analog_image_name = StrDup( name );

   return psv;
}
static PTRSZVAL CPROC SetClockBackgroundImage( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	info->image_name = StrDup( name );

   return psv;
}
static PTRSZVAL CPROC SetClockAnalogImageDesc( PTRSZVAL psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, size_t, size );
	PARAM( args, struct clock_image_thing *, desc );
	if( size == sizeof( struct clock_image_thing ) )
	{
		info->image_desc = (*desc);
	}
	else
	{
      		lprintf( WIDE("size of struct was %d not %d"), size, sizeof( struct clock_image_thing ) );
	}
   return psv;
}

OnLoadControl( WIDE( "Clock" ) )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	InterShell_AddCommonButtonConfig( pch );

	AddConfigurationMethod( pch, WIDE("Clock color=%c"), ReloadClockColor );
	AddConfigurationMethod( pch, WIDE("Clock back color=%c"), ReloadClockBackColor );
	AddConfigurationMethod( pch, WIDE("Clock is analog?%b"), SetClockAnalog );
	AddConfigurationMethod( pch, WIDE("Clock is military time?%b"), ConfigSetClockAmPm );
	AddConfigurationMethod( pch, WIDE("Clock show day of week?%b"), ConfigSetClockDayOfWeek );
	AddConfigurationMethod( pch, WIDE("Clock show date?%b"), ConfigSetClockDate );
	AddConfigurationMethod( pch, WIDE("Clock is single line?%b"), ConfigSetClockSingleLine );
	AddConfigurationMethod( pch, WIDE("Clock analog image=%m" ), SetClockAnalogImage );
	AddConfigurationMethod( pch, WIDE("Clock background image=%m" ), SetClockBackgroundImage );
	AddConfigurationMethod( pch, WIDE("Clock analog description=%B" ), SetClockAnalogImageDesc );
}

OnFixupControl( WIDE("Clock") )(  PTRSZVAL psv )
//void CPROC FixupClock(  PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	if( info )
	{
		SetClockColor( info->control, info->color );
		SetClockBackColor( info->control, info->backcolor );
		InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	}
}

OnGetControl( WIDE("Clock") )( PTRSZVAL psv )
//PSI_CONTROL CPROC GetClockControl( PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
   return info->control;
}

OnEditEnd( WIDE("Clock") )(PTRSZVAL psv )
//void CPROC ResumeClock( PTRSZVAL psv)
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	lprintf( WIDE("Break.") );
	StartClock( info->control );
}

OnEditBegin( WIDE("Clock") )( PTRSZVAL psv )
//void CPROC PauseClock( PTRSZVAL psv)
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	//HideCommon( info->control );
	StopClock( info->control );
}

OnQueryShowControl( WIDE("Clock") )( PTRSZVAL psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	if( info->flags.bAnalog )
	{
		MakeClockAnalogEx( info->control, info->analog_image_name, &info->image_desc );
	}
	else
		MakeClockAnalogEx( info->control, NULL, NULL ); // hrm this should turn off the analog feature...
	return TRUE;
}

//---------------------------------------------------------------------------
#if ( __WATCOMC__ < 1291 )
PUBLIC( void, NeedAtLeastOneExport )( void )
{
}
#endif
