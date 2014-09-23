
#include <render.h>
#include <image.h>


void CPROC RenderBoard( void )
{
	// this will rely on the veiwpoint having been
	// set before rending this board...
	// in fact this count on everything being setup
   // and have to do nothing in this.
}

void InitShell(void)
{
	PRENDERER render = OpenDisplaySizeAt( 0, 800, 600, 0, 0 );
	EnableOpenGL( render );
   SetActiveGLDisplay( renderer );
}


#ifdef TEST_PROGRAM
int main( void )
{
   InitShell();
   return 0;
}

