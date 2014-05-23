#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <deadstart.h>
#ifndef __ANDROID__
#include <systray.h>
#include <render.h>
#endif

SaneWinMain( argc, argv )
{
	InvokeDeadstart();
#ifndef __ANDROID__
	TerminateIcon();

	RegisterIcon( NULL );

	RestoreDisplay( NULL );
#endif
	while( 1 )
		WakeableSleep( 100000 );

	return 0;
}
EndSaneWinMain()
