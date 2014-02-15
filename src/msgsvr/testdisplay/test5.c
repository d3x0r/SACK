
#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <image.h>
#include <render.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;
GLOBAL g;


void CPROC RedrawTwo( PTRSZVAL psvUser, PRENDERER renderer )
{
	Image image = GetDisplayImage( renderer );
	Log2( WIDE("Update image two (0,0)-(%d,%d)"), image->width, image->height );
	BlatColor( image, 0, 0, image->width, image->height, psvUser );
}

int main( int argc, char **argv )
{
	PRENDERER displays[10];
	int n;
   int nDisplays = 10;
	if( argc > 1 )
	{
		nDisplays = atoi( argv[1] );
		if( !nDisplays )
		{
         fprintf( stderr, WIDE("Invalid number specified, defaulting to 10 panels.") );
			nDisplays = 10;
		}
	}
	SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

	for( n = 0; n < nDisplays; n++ )
	{
		displays[n] = OpenDisplaySizedAt( 0, 100+n, 100+n, 100 + n * 20, 100 + n * 20 );
		if( !displays[n] )
		{
			printf( WIDE("Failed to open display 0\n") );
			return 0;
		}
      else
			lprintf( WIDE("Managed to create display %d"), n );
		{
         CDATA color = ColorAverage( Color( 72, 83, 10 )
																				, Color( 219, 142, 153 )
																				, n
																				, 10 ) ;
			SetRedrawHandler( displays[n], RedrawTwo, color);
		}
		UpdateDisplay( displays[n] );
	}
	//while( 1 )
      Sleep( 300000 );

   return 0;
}

// $Log: test5.c,v $
// Revision 1.7  2005/05/25 16:50:15  d3x0r
// Synch with working repository.
//
// Revision 1.8  2005/05/12 21:12:45  jim
// Merge process_service_branch into trunk development.
//
// Revision 1.7.2.1  2005/05/12 21:03:12  jim
// Fixed up several issues with syncronization, detection of lost services... various functionaly improvements
//
// Revision 1.7  2005/04/19 22:49:30  jim
// Looks like the display module technology nearly works... at least exits graceful are handled somewhat gracefully.
//
// Revision 1.6  2004/08/17 04:17:27  d3x0r
// Extend test5...
//
// Revision 1.5  2004/08/16 07:13:42  d3x0r
// Extend tests, enable on windows, msgsvr fully functional?
//
// Revision 1.4  2004/03/23 22:48:47  d3x0r
// Mass changes to get test programs to compile...
//
// Revision 1.3  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
