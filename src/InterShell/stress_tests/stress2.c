#include <stdlib.h>
#include <milk_export.h>
#include <milk_registry.h>

static struct {
	struct {
		BIT_FIELD initialized : 1;
	} flags;
} l;

typedef struct {
	struct {
		BIT_FIELD showing : 1;
	} flags;
   _32 timer;
   PMENU_BUTTON button;
} MY_BUTTON, *PMY_BUTTON;

OnFinishInit( "Phase 2 Test block update" )( void )
{
   l.flags.initialized = 1; // on blink timer, actually cause an update, otherwise just update status
}

static void CPROC Blink( PTRSZVAL psv )
{
	PMY_BUTTON me = (PMY_BUTTON)psv;
	me->flags.showing = !me->flags.showing;
	if( l.flags.initialized )
	{
		lprintf( "Color Button to update..." );
		UpdateButton( me->button );
		lprintf( "Color Button Changed..." );
	}
   RescheduleTimerEx( 0, 50 + ( rand() % 500 ) );
}

/*
OnQueryShowControl( "Stress Button 2(colors)" )( PTRSZVAL psv )
{
	PMY_BUTTON me = (PMY_BUTTON)psv;
	lprintf( "Blink Button asked for permission to show..." );
   return me->flags.showing;
}
*/
//OnFixupControl
OnShowControl
("Stress Button 2(colors)" )( PTRSZVAL psv )
{
	PMY_BUTTON me = (PMY_BUTTON)psv;
   lprintf( "Getting new colors for button..." );
	MILK_SetButtonColors( me->button
							  , 0xFF000000 | (0x1 << ( rand() & 0x7 ) ) | (0x100 << ( rand() & 0x7 ) )| (0x10000 << ( rand() & 0x7 ) )
							  , 0xFF000000 | (0x1 << ( rand() & 0x7 ) ) | (0x100 << ( rand() & 0x7 ) )| (0x10000 << ( rand() & 0x7 ) )
							  , 0xFF000000 | (0x1 << ( rand() & 0x7 ) ) | (0x100 << ( rand() & 0x7 ) )| (0x10000 << ( rand() & 0x7 ) )
							  , 0xFF000000 | (0x1 << ( rand() & 0x7 ) ) | (0x100 << ( rand() & 0x7 ) )| (0x10000 << ( rand() & 0x7 ) )
                        );
}

OnCreateMenuButton( "Stress Button 2(colors)" )( PMENU_BUTTON button )
{
	PMY_BUTTON me = New( MY_BUTTON );
	me->button = button;
	me->timer = AddTimer( 1000, Blink, (PTRSZVAL)me );
	me->flags.showing = 1;
   return (PTRSZVAL)me;
}


