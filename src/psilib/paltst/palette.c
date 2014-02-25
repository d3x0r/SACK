#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>
#include <sharemem.h>
#include <logging.h>
#include <timers.h> //Sleep()
//DEADSTART_LINK;

int main( void )
{
	CDATA result;
	result = Color(127,127,127);
	while( PickColorEx( &result, result, 0, 256, 150 ) )
	{
		printf( WIDE("Guess what! We got a color: %08") _32fX WIDE("\n"), result );
		{
			_32 free,used,chunks,freechunks;
			Sleep( 1000 );// wait a moment for the dialog to really go away.
			GetMemStats( &free, &used, &chunks, &freechunks );
			printf( WIDE("Debug: used:%") _32f WIDE(" free:%") _32f WIDE(" chunks:%") _32f WIDE(" freechunks:%") _32f WIDE("\n")
					, used,free,chunks,freechunks );
			DebugDumpMem();
		}
	}
	printf( WIDE("Color Dialog was canceled.\n") );
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
