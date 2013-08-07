#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <deadstart.h>
#include <systray.h>
#include <render.h>

SaneWinMain( argc, argv )
{
	InvokeDeadstart();

	TerminateIcon();

	RegisterIcon( NULL );

	RestoreDisplay( NULL );

	while( 1 )
		WakeableSleep( 100000 );

	return 0;
}
EndSaneWinMain()
