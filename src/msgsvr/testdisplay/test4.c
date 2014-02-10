

#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <image.h>
#include <render.h>

#include <controls.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;

static GLOBAL g;

#include <logging.h>

#ifdef UNDER_CE
int APIENTRY WinMain( HINSTANCE h, HINSTANCE p, LPTSTR cmd, int nCmdShow )
{
#else
	int main( void )
{
#endif
	CDATA result;
   PRENDERER display;
	SetSystemLog( SYSLOG_FILENAME, WIDE("Test3.Log") );
	//SetAllocateLogging( TRUE );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

	SetControlImageInterface( GetImageInterface() );
	SetControlInterface( GetDisplayInterface() );
   display = OpenDisplaySizedAt( 0, 150, 150, 50, 50 );
   result = Color(127,127,127);
	while( PickColorEx( &result, result, 0, 256, 150 ) )
		printf( WIDE("Guess what! We got a color: %08x\n"), result );
	printf( WIDE("Color Dialog was canceled.\n") );
   CloseDisplay( display );
	return 0;
}

// $Log: test4.c,v $
// Revision 1.4  2004/03/23 22:44:36  d3x0r
// Updates to new interface mechanism
//
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
