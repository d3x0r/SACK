#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>
#include <sharemem.h>
#include <logging.h>
#include <timers.h> //Sleep()
//DEADSTART_LINK;

PTHREAD mainThread;
int done;

void newColor( uintptr_t psv, CDATA color, LOGICAL confirm ) {
	if( confirm) {
		printf ( "Guess what! We got a color: %08" _32fX "\n", color );
	}else {
		done = TRUE;
		WakeThread( mainThread );
	}

 	{
		int32_t free,used,chunks,freechunks;
		Sleep( 1000 ) ;// w ait a m oment for the dialog to  reall y go aw ay.
		GetMemStats( &free, &used, &chunks, &freechunks );
		printf( "Debug: used:%" _32f " free:%" _32f " chunks:%" _32f " freechunks:%" _32f "\n"
				, used,free,chunks,freechunks );
						DumpMem ();
	}
	if( confirm ) {
		PickColorEx( NULL, color, 0, 256, 158, newColor, 0 );
	}
}

int main( void )
{
	CDATA result;
	mainThread = MakeThread();
	result = Color(127,127,127);
	PickColorEx( NULL, result, 0, 256, 150, newColor, 0 );
	// don't know when cancel happens.
	// after an OK can't do another one...
	while( !done ) WakeableSleep( 1000 );
	printf( "Color Dialog was canceled.\n" );
	return 0;
}

// $Log: palette.c,v $
// Revision 1.18  2005/03/22 12:41:58  panther
// Wow this transparency thing is going to rock! :) It was much closer than I had originally thought.  Need a new class of controls though to support click-masks.... oh yeah and buttons which have roundable scaleable edged based off of a dot/circle
//
// Revision 1.17  2004/12/05 15:32:06  panther
// Some minor cleanups fixed a couple memory leaks
//
// Revision 1.16  2004/09/07 07:05:46  d3x0r
// Stablized up to palette dialog, which is internal... may require recompile binary upgrade may not work.
//
// Revision 1.15  2004/08/25 15:01:06  d3x0r
// Checkpoint - more vc compat fixes
//
// Revision 1.14  2004/04/25 10:15:01  d3x0r
// Cleaned up project back to simple status
//
//
// PRIOR LOG REMOVED.
