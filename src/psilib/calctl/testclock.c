
#include <psi/clock.h>

int main( int argc, char **argv )
{

	PSI_CONTROL frame;
	PSI_CONTROL clock;
	if( argc > 1 )
		frame = NULL;
   else
		frame = CreateFrame( "blah", 0, 0, 500, 500, BORDER_RESIZABLE|BORDER_NORMAL, NULL );
	clock = MakeNamedControl( frame, "Basic Clock Widget", 0, 0, 350, 350, -1 );
	if( frame )
		SetCommonText( frame, "CLock Test Widet" );
	SetCommonText( clock, "CLock Test Widet" );
   if( !frame )
   SetCommonBorder( clock,BORDER_RESIZABLE|BORDER_NORMAL );
	DisplayFrame( frame?frame:clock );
   MakeClockAnalog( clock );
	while( 1 )
      WakeableSleep( 1000 );

}


