#include <stdhdrs.h>
#include <timers.h>
#include <system.h>
#ifdef WIN32
#include <systray.h>
#endif
#include <procreg.h>

#ifdef _MSC_VER
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow )
#else
int main( void )
#endif
{
   // doesn't matter...
	if( LoadPrivateFunction( WIDE("sack.msgsvr.service.plugin"), NULL ) )
	{
#ifdef WIN32
#ifndef __NO_GUI__
		RegisterIcon( NULL );
#endif
#endif
      while( 1 )
			WakeableSleep( SLEEP_FOREVER );
	}
   else
		printf( WIDE("Failed to load message core service.\n") );
   return 0;
}
