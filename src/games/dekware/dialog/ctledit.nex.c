#include <stdhdrs.h>
// ooOo fancy text parsing... good thing
// we get fed fancy text segments :)
#include <configscript.h> // sack
#include <plugin.h> // dekware
#include <controls.h> // psi

#include "global.h" // common for this plugin

static void InitControlObject( PENTITY pe, PCOMMON pc )
{
	//AddBehavior();
	//AddMethod();
	//AddVolatileVar();
	//connect PSI events to behaviors...
}

static int CPROC CustomEditButton( PCOMMON pc )
{
	InitControlObject( CommonInitControl( pc ), pc );
   return 1;
}

PRELOAD( EdtRegisterExtraInits )
{
	SimpleRegisterMethod( WIDE("psi/control/") EDIT_FIELD_NAME WIDE("/rtti")
							  , CustomEditButton, WIDE("int"), WIDE("extra init"), WIDE("(PCOMMON)") );
}

