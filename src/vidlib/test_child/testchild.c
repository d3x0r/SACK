
#include <render.h>

void CPROC Draw( uintptr_t psv, PRENDERER render )
{
	Image image = GetDisplayImage( render );
	ClearImageTo( image, psv );

}


int main( void )

{
	PRENDERER render = OpenDisplay( "whatever" );
	PRENDERER newchild = OpenDisplay( "asdf" );
   SetRedrawHandler( render, Draw, BASE_COLOR_GREEN );
   SetRedrawHandler( newchild, Draw, BASE_COLOR_RED );
	PutDisplayIn( newchild, render );
   UpdateDisplay( render );
	UpdateDisplay( newchild );
   while( 1 )
   WakeableSleep( SLEEP_FOREVER );
}

