
#include <stdhdrs.h>

#include <brain.hpp>
#include <board.hpp>



void SetupIntershellInteface( void )
{
	void (*Main)(int, CTEXTSTR *, int );
	Main = (void (*)(int,CTEXTSTR*, int ))LoadFunction( WIDE( "InterShell.core" ), WIDE( "Main" ) );
	if( Main )
	{
		static CTEXTSTR args[] = { WIDE( "InterShell.core" ), WIDE( "-tsr" ), NULL };
		lprintf( WIDE( "Calling InterShell..." ) );
		Main( 2, args, 0 );
	}
}

void SetupDekwareInteface( void )
{
	void (*Main)( CTEXTSTR, int );
	Main = (void (*)( CTEXTSTR, int ))LoadFunction( WIDE( "dekware.core" ), WIDE( "Begin" ) );
	if( Main )
	{
		lprintf( WIDE( "Calling dekware..." ) );
		Main( WIDE("-tsr"), 0 );
	}
}


SaneWinMain( argc, argv )
{
	// need to load dekware first, so it registers it's intershell controls
	SetupDekwareInteface();
	SetupIntershellInteface();
	while( 1 )
		WakeableSleep( 10000 );
   return 0;
}
EndSaneWinMain()

