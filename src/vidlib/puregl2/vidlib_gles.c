/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *    the first delay is between CreateWindow and WM_NCCREATE
 *    the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/

#include <stdhdrs.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */

// move local into render namespace.
#include "local.h"

RENDER_NAMESPACE

#define __glPi 3.14159265358979323846


int InitGL( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !camera->flags.init )
	{
		glEnable( GL_BLEND );
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do

      // this just fills l.fProjection
		MygluPerspective(90.0f,camera->aspect,1.0f,30000.0f);

		lprintf( WIDE("First GL Init Done.") );
		camera->flags.init = 1;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
		glClearDepthf(1.0f);									// Depth Buffer Setup
	}
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}




RENDER_NAMESPACE_END

