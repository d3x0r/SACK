
#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;

GLOBAL g;


#ifdef UNDER_CE
int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, int cmd, int nCmdShow )
{
#else
	int main( void )
{
#endif
	PRENDERER display[3];
	Image image[3];

	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

   //SetSystemLog( SYSLOG_FILE, stdout );
	display[0] = OpenDisplaySizedAt( 0, 150, 150, 50, 50 );
	if( !display[0] )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
	image[0] = GetDisplayImage( display[0] );

	display[1] = OpenDisplaySizedAt( 0, 250, 250, 50, 50 );
	if( !display[1] )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
	image[1] = GetDisplayImage( display[1] );

	display[2] = OpenDisplaySizedAt( 0, 350, 350, 50, 50 );
	if( !display[2] )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
	image[2] = GetDisplayImage( display[2] );


	while( 1 )
	{
      int i;
		for( i = 0; i < 100; i+= 10 )
			do_hline( image[0], i, 0, 100, Color( 255,255,255 ) );
		UpdateDisplay( display[0] );
		Sleep( 5000 );
      break;
		//Relinquish(); // hmm what do we have to do here?
	}
   CloseDisplay( display[0] );
   CloseDisplay( display[1] );
   CloseDisplay( display[2] );
}


// $Log: test2.c,v $
// Revision 1.5  2005/04/18 15:54:53  jim
// Message service nearly seems functional, failings may be service handling errors.  Much debugging still remains.
//
// Revision 1.4  2004/03/23 22:44:36  d3x0r
// Updates to new interface mechanism
//
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
