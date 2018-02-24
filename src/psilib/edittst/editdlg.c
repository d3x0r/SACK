#include <controls.h>
#include <timers.h>

SaneWinMain( argc, argv )
{
	PSI_CONTROL pc = CreateFrame( WIDE("Test Dynamic edits"), 256, 256, 0, 0, 0, NULL );
	EditFrame( pc, TRUE );
	DisplayFrame( pc );
	WakeableSleep( SLEEP_FOREVER );
	return 0;
}
EndSaneWinMain()

