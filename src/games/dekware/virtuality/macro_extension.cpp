

#include "local.h"

struct macro_info_struct
{
	PMACROSTATE pms;
	PENTITY pe_running;
   LOGICAL running;
   LOGICAL ended;
}

static NATIVE GetMacroRunning( PTRSZVAL psv )
{
	struct macro_info_struct *mis = (struct macro_info_struct *)psv;
   return mis->running;
}


static void StartRunMacro( PTRSZVAL psv, NATIVE value )
{
	struct macro_info_struct *mis = (struct macro_info_struct *)psv;
	if( value > 0 )
	{
		if( !mis->running )
		{
			mis->running = 1;
         mis->pms = InvokeMacro( NULL, mis->pMacro, NULL );
		}
	}
}

static void StopRunMacro( PTRSZVAL psv, NATIVE value )
{
	if( GetMacroRunning( psv ) )
	{

	}
}

static void ObjectMacroCreated( WIDE("Point Label"), WIDE("AddMacro"), WIDE( "Add a invokable macro") )(PENTITY pe_object, PMACRO macro)
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_object->pPlugin, l.extension );
	if( vobj )
	{
		PVARTEXT pvt = VarTextCreate();
		vtprintf( pvt, WIDE("Run Macro %s"), GetText( GetName( macro ) ) );

		PBRAIN_STEM pbs = new BRAIN_STEM( GetText( VarTextPeek( pvt ) ) );
		vobj->brain->AddBrainStem( pbs );
		{
			struct macro_info_struct *mis = New( struct macro_info_struct );
			mis->pms = NULL;
			mis->pMacro = macro;
			mis->pe_running = pe_created;
			mis->running = FALSE;
			mis->stopped = FALSE;
		}
      pbs->AddInput( new value(GetMacroRunning, (PTRSZVAL)mis ), WIDE( "Is Running" ) );
      pbs->AddOutput( new value(StartRunMacro, (PTRSZVAL)mis ), WIDE( "Start" ) );
      pbs->AddOutput( new value(StopRunMacro, (PTRSZVAL)mis ), WIDE( "Stop" ) );
	}
}


//static void ObjectMacroDestroyed( WIDE("Point Label"), WIDE("AddMacro"), WIDE( "Add a invokable macro") )(PENTITY pe_object, PMACRO macro)
//{
//}
