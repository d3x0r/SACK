#include <stdhdrs.h>
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <render.h>

SaneWinMain( argc, argv )
{
	// just create window, so we get the shutdown event.
	OpenDisplay( 0/*PANEL_ATTRIBUTE_NONE*/ );

	// okay we get the query end session first, but reschedule for getting the endsession
	SetProcessShutdownParameters( 0x2a0, 0 );

	while( 1 )
		WakeableSleep( 100000 );
	return 0;
}
EndSaneWinMain()
