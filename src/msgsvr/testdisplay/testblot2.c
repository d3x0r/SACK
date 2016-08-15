#include <stdio.h>
#include <stdlib.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>
#include <timers.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;
static GLOBAL g;
Image back;
Image surface;

void CPROC Output( uintptr_t psv, PRENDERER display )
{
   surface = GetDisplayImage( display );
	BlotScaledImage( surface, back );
		  
}

int main( int argc, char **argv )
{
	uint32_t width, height;
	SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	if(argc<3)lprintf( WIDE("Usage: %s <background image> <daubing image>\n"), argv[0] );
	GetDisplaySize( &width, &height );;
	{
		PRENDERER display = OpenDisplaySizedAt( 0, width, height, 0, 0 );
      SetRedrawHandler( display, Output, 0 );
		back = LoadImageFile( argc > 1?argv[1]:"sky.jpg" );
		UpdateDisplay( display );
		while( 1 )
			WakeableSleep( 10000 );
	}
   return 0;
}
