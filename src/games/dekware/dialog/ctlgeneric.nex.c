// A blank skeleton control from which other controls may be based.

#include <stdhdrs.h>
// ooOo fancy text parsing... good thing
// we get fed fancy text segments :)
#include <configscript.h> // sack
#include <space.h> // dekware
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

PRELOAD( RegisterExtraInits )
{
	SimpleRegisterMethod( "psi/control/" "generic control name" "/rtti"
							  , CustomEditButton, "int", "extra init", "(PCOMMON)" );
}

