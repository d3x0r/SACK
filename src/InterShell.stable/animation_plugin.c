#if 0
#include <sack_types.h>
#include <deadstart.h>
#include "intershell_local.h"
#include "intershell_registry.h"
#include "menu_real_button.h"
#include "resource.h"
#include "fonts.h"
#include "animation.h"

#define ANIMATION_BUTTON_NAME "Animation"


typedef struct {

	TEXTSTR animation_name;

	_32 x;				//position and size of played animation
	_32 y;
	_32 w;
	_32 h;

	PMNG_ANIMATION 	animation;
	
	PMENU_BUTTON pmb;
   PLIST buttons;

} ANIMATION_INFO, *PANIMATION_INFO;


ANIMATION_INFO l;


enum {
	EDIT_ANIMATION_NAME = 1200
	, EDIT_X
	, EDIT_Y 
	, EDIT_W 
	, EDIT_H 
};

//---------------------------------------------------------------------------
PRELOAD( RegisterAnimationDialogResources )
{

	EasyRegisterResource( "intershell/animation", EDIT_ANIMATION_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_X, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_Y, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_W, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_H, EDIT_FIELD_NAME );

}

//---------------------------------------------------------------------------
OnCreateMenuButton( ANIMATION_BUTTON_NAME )( PMENU_BUTTON common )
{
   PANIMATION_INFO info = ( PANIMATION_INFO )Allocate( sizeof( *info ) );
   MemSet(info, 0, sizeof(*info));
  
	info->pmb = common;
	{
		//_32 w, h;
		//GetFrameSize( InterShell_GetButtonControl( common ), &w, &h );
		info->w = common->w;
		info->h = common->h;
	}
//   lprintf("OnCreateMenuButton I am here!!!");
   AddLink( &l.buttons, info );
   return (PTRSZVAL)info;
}


//---------------------------------------------------------------------------
static void ConfigureAnimationButton( PANIMATION_INFO info, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "ConfigureAnimationButton.isFrame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		char buffer[256];

		SetCommonButtons( frame, &done, &okay );

		SetControlText( GetControl( frame, EDIT_ANIMATION_NAME ), info->animation_name );

		snprintf(buffer, sizeof(buffer), "%d", info->x);
		SetControlText( GetControl( frame, EDIT_X ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->y);
		SetControlText( GetControl( frame, EDIT_Y ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->w);
		SetControlText( GetControl( frame, EDIT_W ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->h);
		SetControlText( GetControl( frame, EDIT_H ), buffer );

		SetCommonButtonControls( frame );

		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			char buffer[256];
			GetCommonButtonControls( frame );
			GetControlText( GetControl( frame, EDIT_ANIMATION_NAME ), buffer, sizeof( buffer ) );
			if( info->animation_name )
				Release( info->animation_name );
			info->animation_name = StrDup( buffer );

			GetControlText( GetControl( frame, EDIT_X), buffer, sizeof( buffer ) );
			info->x = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_Y), buffer, sizeof( buffer ) );
			info->y = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_W), buffer, sizeof( buffer ) );
			info->w = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_H), buffer, sizeof( buffer ) );
			info->h = atoi(buffer);


		}
		DestroyFrame( &frame );
	}
}

//---------------------------------------------------------------------------
OnEditControl( ANIMATION_BUTTON_NAME )( PTRSZVAL psv, PSI_CONTROL parent )
{
	ConfigureAnimationButton( (PANIMATION_INFO)psv, parent );
	return psv;
}


//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetAnimationName( PTRSZVAL psv, arg_list args )
{
//	PANIMATION_INFO info = (PANIMATION_INFO)psv;


	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	//lprintf("SetAnimationName i am here %s %X", name, name);
if( info )
	info->animation_name = StrDup( name );

	return psv;
}

//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetAnimationXValue( PTRSZVAL psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, S_32, x );
if( info )
	info->x = x;

	return psv;
}

//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetAnimationYValue( PTRSZVAL psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, S_32, y );
if( info )
	info->y = y;
	return psv;
}
//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetAnimationWValue( PTRSZVAL psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, S_32, w );
if( info )
	info->w = w;
	return psv;
}
//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetAnimationHValue( PTRSZVAL psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, S_32, h );
if( info )
	info->h = h;
	return psv;
}


//---------------------------------------------------------------------------
void WriteAnimationButton( CTEXTSTR leader, FILE *file, PTRSZVAL psv )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;

	sack_fprintf( file, WIDE("Animation name =%s\n" ), info->animation_name?info->animation_name:"" );
	sack_fprintf( file, WIDE("Animation X =%d\n" ), info->x );
	sack_fprintf( file, WIDE("Animation Y =%d\n" ), info->y );
	sack_fprintf( file, WIDE("Animation W =%d\n" ), info->w );
	sack_fprintf( file, WIDE("Animation H =%d\n" ), info->h );

}

//---------------------------------------------------------------------------
void ReadAnimationButton( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
	AddConfigurationMethod( pch, WIDE("Animation name =%m"), SetAnimationName );
	AddConfigurationMethod( pch, WIDE("Animation X =%i"), SetAnimationXValue );
	AddConfigurationMethod( pch, WIDE("Animation Y =%i"), SetAnimationYValue );
	AddConfigurationMethod( pch, WIDE("Animation W =%i"), SetAnimationWValue );
	AddConfigurationMethod( pch, WIDE("Animation H =%i"), SetAnimationHValue );
}



//---------------------------------------------------------------------------
OnSaveControl( ANIMATION_BUTTON_NAME )( FILE *file, PTRSZVAL psv )
{
	WriteAnimationButton( NULL, file, psv );
}


//---------------------------------------------------------------------------
OnLoadControl( ANIMATION_BUTTON_NAME )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{

//   lprintf("OnLoadControl I am here!!!");

	ReadAnimationButton( pch, psv);
}

//---------------------------------------------------------------------------
OnGlobalPropertyEdit( ANIMATION_BUTTON_NAME )( PSI_CONTROL parent )
{

	ConfigureAnimationButton( &l, parent );
}


//---------------------------------------------------------------------------
OnSaveCommon( ANIMATION_BUTTON_NAME )( FILE *file )
{
	WriteAnimationButton( NULL, file, (PTRSZVAL)&l );
}

//---------------------------------------------------------------------------
OnLoadCommon( ANIMATION_BUTTON_NAME )( PCONFIG_HANDLER pch )
{
//   lprintf("OnLoadCommon I am here!!!   (sizeof(l)) is %lu", (sizeof(l)));

	MemSet( &l, 0, sizeof(l) );
	ReadAnimationButton( pch, (PTRSZVAL)&l);
}


//---------------------------------------------------------------------------
void PlayAnimation(PSI_CONTROL control)
/*
   Play animations 
*/
{
   //DebugBreak();
	if( l.animation )
	{
		DeInitAnimationEngine( l.animation );
		l.animation = NULL;
	}

	{
		INDEX idx;
		PANIMATION_INFO info;
		LIST_FORALL( l.buttons, idx, PANIMATION_INFO, info )
		{
			if( info->animation_name && info->animation_name[0] )
			{
				PCanvasData canvas = GetCanvas( NULL );

				info->animation = InitAnimationEngine();

				//		lprintf("PlayAnimation : %s %d %d %d %d", l.animation_name, l.x, l.y, l.w, l.h);

				if(info->animation)
					GenerateAnimation( info->animation, control, info->animation_name, PARTX(info->x), PARTY(info->y), PARTW( info->x, info->w ), PARTH( info->y, info->h ));
			}
		}
	}
}

#endif
