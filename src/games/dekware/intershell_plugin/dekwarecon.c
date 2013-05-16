
#include <system.h>
#include <timers.h>
#include <InterShell/intershell_registry.h>

#include <psi/console.h>


PRELOAD( LoadConsole )
{
   TEXTCHAR buffer[256];
	lprintf( WIDE("Loaded the actual console... which loads dekware.core (probably)") );
   snprintf( buffer, 256, "%s/dekware.core", GetProgramPath() );
	LoadFunction( buffer, NULL );
	//LoadFunction( WIDE("psicon.nex"), NULL );
}


void CPROC tickthing( PTRSZVAL psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;

	//pcprintf( pc, WIDE("blah\n") );

}


OnCreateControl( WIDE("Dekware Console") )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, WIDE("Dekware PSI Console"), x, y, w, h, -1 );
   AddTimer( 250, tickthing, (PTRSZVAL)pc );

   return (PTRSZVAL)pc;
}

OnGetControl( WIDE("Dekware Console") )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}

PUBLIC( void, ExportedSymbolToMakeWatcomHappy )( void )
{
}

