
#include <stdhdrs.h>

// #define DEFINE_RENDER_INTERFACE
 #define USE_RENDER_INTERFACE g.Render
#include <render.h>
//#include <image.h>

typedef struct global_tag
{
	PRENDER_INTERFACE Render;
   char unused;
} GLOBAL;

GLOBAL g;

#ifdef UNDER_CE
int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, LPCSTR cmd, int nCmdShow )
{
#else
int main( void )
{
#endif
	PRENDERER display;
   g.Render = GetDisplayInterface();
   //SetSystemLog( SYSLOG_FILE, stdout );
	display = OpenDisplaySizedAt( 0, 150, 150, 50, 50 );
	if( !display )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
   UpdateDisplay( display );
	display = OpenDisplaySizedAt( 0, 250, 250, 50, 50 );
	if( !display )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
   UpdateDisplay( display );
	display = OpenDisplaySizedAt( 0, 350, 350, 50, 50 );
	if( !display )
	{
		fprintf( stderr, WIDE("Failed to open a display") );
		return 0;
	}
   UpdateDisplay( display );

   WakeableSleep( 5000 );
	//while( 1 )
	//	Relinquish(); // hmm what do we have to do here?
}

// $Log: test.c,v $
// Revision 1.7  2005/05/12 21:12:45  jim
// Merge process_service_branch into trunk development.
//
// Revision 1.6.2.1  2005/05/05 19:05:12  jim
// Modified simple test program to exit after 5 seconds.  Making progress on this whole message service thing...
//
// Revision 1.6  2004/08/16 07:13:42  d3x0r
// Extend tests, enable on windows, msgsvr fully functional?
//
// Revision 1.5  2004/03/23 22:44:36  d3x0r
// Updates to new interface mechanism
//
// Revision 1.4  2004/03/23 22:20:21  d3x0r
// Use correct define for display
//
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
