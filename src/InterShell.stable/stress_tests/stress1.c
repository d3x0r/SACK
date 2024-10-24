#include <stdlib.h>
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <InterShell_export.h>
#include <InterShell_registry.h>

static struct {
	struct {
		BIT_FIELD initialized : 1;
	} flags;
} l;

typedef struct {
	struct {
		BIT_FIELD showing : 1;
	} flags;
   uint32_t timer;
   PMENU_BUTTON button;
} MY_BUTTON, *PMY_BUTTON;

OnFinishInit( "Phase 1 Test block update" )( void )
{
   l.flags.initialized = 1; // on blink timer, actually cause an update, otherwise just update status
}

static void CPROC Blink( uintptr_t psv )
{
	PMY_BUTTON me = (PMY_BUTTON)psv;
	me->flags.showing = !me->flags.showing;
	if( l.flags.initialized )
	{
		//lprintf( "Blink Button to update..." );
		UpdateButton( me->button );
		//lprintf( "Blink Button Changed..." );
	}
   RescheduleTimerEx( 0, 50 + ( rand() % 500 ) );
}

OnQueryShowControl( "Stress Button 1(Blink)" )( uintptr_t psv )
{
	PMY_BUTTON me = (PMY_BUTTON)psv;
	//lprintf( "Blink Button asked for permission to show...: %s", me->flags.showing?"Yes":"No" );
   return me->flags.showing;
}

OnCreateMenuButton( "Stress Button 1(Blink)" )( PMENU_BUTTON button )
{
	PMY_BUTTON me = New( MY_BUTTON );
	me->button = button;
	me->timer = AddTimer( 1000, Blink, (uintptr_t)me );
	me->flags.showing = 1;
   return (uintptr_t)me;
}



