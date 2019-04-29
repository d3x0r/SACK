
#include <sack_system.h>
#include <timers.h>
#include <InterShell/intershell_registry.h>

#include <psi/console.h>


PRELOAD( LoadConsole )
{
	TEXTCHAR buffer[256];
	lprintf( "Loaded the actual console... which loads dekware.core (probably)" );
	snprintf( buffer, 256, "%s/dekware.core", GetProgramPath() );
	LoadFunction( buffer, NULL );
	//LoadFunction( "psicon.nex", NULL );
}


void CPROC tickthing( uintptr_t psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;

	//pcprintf( pc, "blah\n" );

}


static uintptr_t OnCreateControl( "Dekware Console" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, "Dekware PSI Console", x, y, w, h, -1 );
	AddTimer( 250, tickthing, (uintptr_t)pc );

	return (uintptr_t)pc;
}

static PSI_CONTROL OnGetControl( "Dekware Console" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

PUBLIC( void, ExportedSymbolToMakeWatcomHappy )( void )
{
}

