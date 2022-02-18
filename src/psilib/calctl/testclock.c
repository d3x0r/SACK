
#include <psi/clock.h>

SaneWinMain( argc, argv )
{
	PSI_CONTROL frame;
	PSI_CONTROL clock;
	if( argc > 1 )
		frame = NULL;
	else
		frame = CreateFrame( "blah", 0, 0, 500, 500, BORDER_RESIZABLE|BORDER_NORMAL, NULL );
	clock = MakeNamedControl( frame, "Basic Clock Widget", 0, 0, 350, 350, -1 );
	if( frame )
		SetControlText( frame, "Clock Test Widget" );
	SetControlText( clock, "Clock Test Widget" );
	if( !frame )
		SetCommonBorder( clock,BORDER_RESIZABLE|BORDER_NORMAL );
	DisplayFrame( frame?frame:clock );
	MakeClockAnalog( clock );
	while( 1 )
		WakeableSleep( 10000 );

}
EndSaneWinMain()
