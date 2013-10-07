#include <stdhdrs.h>
#include <idle.h>
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

int main( void )
{
	CDATA result;
	PCOMMON display[2];
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	SetSystemLog( SYSLOG_FILENAME, WIDE("Test3.Log") );
	//SetAllocateLogging( TRUE );

	SetControlImageInterface( GetImageInterface() );
	SetControlInterface( GetDisplayInterface() );

	display[0] = CreateFrame( WIDE("Frame 1"), 50, 50, 150, 150, 0, NULL );
	DisplayFrame( display[0] );

	display[1] = CreateFrame( WIDE("Frame 2"), 100, 100, 150, 150, 0, NULL );
	DisplayFrame( display[1] );

   OpenDisplaySizedAt( 0, 250, 250, 50, 50 );

   while( 1 )
	{
	Idle();
	}      
	return 0;
}

// $Log: test6.c,v $
// Revision 1.4  2004/10/03 10:07:02  d3x0r
// Update for lack of processcontrolmessages
//
// Revision 1.3  2004/03/23 22:48:47  d3x0r
// Mass changes to get test programs to compile...
//
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
