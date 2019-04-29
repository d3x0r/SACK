#include <stdhdrs.h>
// ooOo fancy text parsing... good thing
// we get fed fancy text segments :)
#include <configscript.h> // sack
#include <controls.h> // psi


#include <plugin.h> // dekware

#include "global.h" // common for this plugin

// this is a variable in dialog.nex

static PTEXT ObjectVolatileVariableGet( "psi_button", "check_state", "Get the button's check state" )
//	static PTEXT CPROC GetCheckStateVar
	( struct entity_tag *pe
								  , PTEXT *lastvalue )
{
	// result is *lastvalue
	if(*lastvalue)
	{
		LineRelease( *lastvalue );
      *lastvalue = NULL;
	}
	{

		PSI_CONTROL pc = (PSI_CONTROL)GetLink( &pe->pPlugin, g.iCommon );
		if( pc )
		{
		}
	}
   return (*lastvalue);
}


static PTEXT ObjectVolatileVariableSet( "psi_button", "check_state", "Set the button's check state" )
//PTEXT CPROC SetCheckStateVar
(struct entity_tag *pe
						 , PTEXT newvalue )
{
	{
		PSI_CONTROL pc = (PSI_CONTROL)GetLink( &pe->pPlugin, g.iCommon );
		if( pc )
		{
			PTEXT tmp = newvalue;
			LOGICAL val;
			if( GetBooleanVar( &tmp, &val ) )
				SetCheckState( pc, val );
			return newvalue;
		}
		else
			return NULL;
	}
   return newvalue; // is more like true/false
}


static PTEXT ObjectVolatileVariableGet( "psi_button", "pressed", "Is button pressed" )
//static PTEXT CPROC GetButtonPressed
( struct entity_tag *pe
								  , PTEXT *lastvalue )
{
	// result is *lastvalue
	if(*lastvalue)
	{
		LineRelease( *lastvalue );
      *lastvalue = NULL;
	}
	{
		PSI_CONTROL pc = (PSI_CONTROL)GetLink( &pe->pPlugin, g.iCommon );
		if( pc )
		{
			DECLTEXT( yes, "yes" );
			DECLTEXT( no, "no" );
			if( IsButtonPressed( pc ) )
				return (PTEXT)&yes;
			else
				return (PTEXT)&no;
		}
	}
   return (*lastvalue);
}


static PTEXT ObjectVolatileVariableSet( "psi_button", "pressed", "Set button press state" )
//PTEXT CPROC SetButtonPressed
( struct entity_tag *pe
						 , PTEXT newvalue )
{
	{
		PSI_CONTROL pc = (PSI_CONTROL)GetLink( &pe->pPlugin, g.iCommon );
		if( pc )
		{
			PTEXT tmp = newvalue;
			LOGICAL val;
			if( GetBooleanVar( &tmp, &val ) )
				PressButton( pc, val );
			return newvalue;
		}
		else
			return NULL;
	}
   return newvalue; // is more like true/false
}

static PTEXT ObjectVolatileVariableSet( "psi_button", "color", "Set the button's current color" )
//PTEXT CPROC SetButtonColorVar
(  struct entity_tag *pe
						 , PTEXT newvalue )
{
	{
		PSI_CONTROL pc = (PSI_CONTROL)GetLink( &pe->pPlugin, g.iCommon );
		if( pc )
		{
			PTEXT tmp = newvalue;
			CDATA color;
         DebugBreak();
			if( GetColorVar( &tmp, &color ) )
				SetButtonColor( pc, color );
			return newvalue;
		}
		else
			return NULL;
	}
   return newvalue; // is more like true/false
}


#if 0
volatile_variable_entry normal_button_vars[] = { { DEFTEXT("color"), NULL /*get button color*/
																 , SetButtonColorVar }
															  , { DEFTEXT("pressed")
																 , GetButtonPressed
																 , SetButtonPressed }
															  , { DEFTEXT("check_state")
																 , GetCheckStateVar
																 , SetCheckStateVar }
};
#endif
static void CPROC ButtonClick( uintptr_t psv, PSI_CONTROL pc )
{
	PENTITY pe = (PENTITY)psv;
   InvokeBehavior( "click", pe, pe->pControlledBy, NULL );
}

//-- draw will require a command interface to the image library
//-- are NOT ready for this.
//void CPROC ButtonDraw(


static void InitControlObject( PENTITY pe, PSI_CONTROL pc )
{
	if( !pe )
      return;
	AddBehavior( pe, "click", "Button has been clicked." );
	AddBehavior( pe, "draw", "Button needs to be drawn." );
   Assimilate( pe, NULL, "psi_button", NULL );
   /*
	AddVolatileVariable( pe, normal_button_vars, (uintptr_t)pc );
	AddVolatileVariable( pe, normal_button_vars+1, (uintptr_t)pc );
	AddVolatileVariable( pe, normal_button_vars+2, (uintptr_t)pc );
   */
   //SetButtonPushMethod( pc, ButtonClick, (uintptr_t)pe );
}

int CPROC CustomPressButton( PSI_CONTROL pc )
{
	PENTITY pe = GetOneOfMyFrames( pc );
	if( pe )
      InvokeBehavior( "click", pe, pe->pControlledBy, NULL );
   return 0;
}

int CPROC CustomDrawButton( PSI_CONTROL pc )
{
	PENTITY pe = GetOneOfMyFrames( pc );
	if( pe )
      InvokeBehavior( "draw", pe, pe->pControlledBy, NULL );
   return 0;
}

int CPROC CustomInitButton( PSI_CONTROL pc )
{
	InitControlObject( CommonInitControl( pc ), pc );
   // if we returned a zero - what would YOu expect?  the caller does not care ATM.
   return 1;
}

PRELOAD( BtnRegisterExtraInits )
{
	SimpleRegisterMethod( "psi/control/" NORMAL_BUTTON_NAME "/rtti/extra init"
							  , CustomInitButton, "int", "extra init", "(PSI_CONTROL)" );
	SimpleRegisterMethod( "psi/control/" CUSTOM_BUTTON_NAME "/rtti/extra init"
							  , CustomInitButton, "int", "extra init", "(PSI_CONTROL)" );
	SimpleRegisterMethod( "psi/control/" IMAGE_BUTTON_NAME  "/rtti/extra init"
							  , CustomInitButton, "int", "extra init", "(PSI_CONTROL)" );
	SimpleRegisterMethod( "psi/control/" RADIO_BUTTON_NAME  "/rtti/extra init"
							  , CustomInitButton, "int", "extra init", "(PSI_CONTROL)" );

   // register methods which result in on-click events
   SimpleRegisterMethod( "psi/control/" NORMAL_BUTTON_NAME  "/rtti/extra press"
							  , CustomPressButton, "int", "extra dekware press", "(PSI_CONTROL)" );
   SimpleRegisterMethod( "psi/control/" CUSTOM_BUTTON_NAME  "/rtti/extra press"
							  , CustomPressButton, "int", "extra dekware press", "(PSI_CONTROL)" );
   SimpleRegisterMethod( "psi/control/" IMAGE_BUTTON_NAME  "/rtti/extra press"
							  , CustomPressButton, "int", "extra dekware press", "(PSI_CONTROL)" );

   // register methods which result in on-draw events.
   SimpleRegisterMethod( "psi/control/" CUSTOM_BUTTON_NAME  "/rtti/extra draw"
							  , CustomDrawButton, "int", "extra dekware draw", "(PSI_CONTROL)" );


	SimpleRegisterMethod( "psi/control/" NORMAL_BUTTON_NAME "/rtti"
							  , SaveCommonMacroData, "int", "extra save", "(PSI_CONTROL,PVARTEXT)" );
}

