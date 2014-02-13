#include <controls.h>
#include <timers.h>

int main( void )
{
	PCOMMON pc = CreateFrame( WIDE("Test Dynamic edits"), 256, 256, 0, 0, 0, NULL );
	EditFrame( pc, TRUE );
	DisplayFrame( pc );
	WakeableSleep( SLEEP_FOREVER );
	return 0;
}

