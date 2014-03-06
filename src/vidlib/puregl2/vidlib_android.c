/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *    the first delay is between CreateWindow and WM_NCCREATE
 *    the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/



/* this must have been done for some other collision in some other bit of code...
 * probably the update queue? the mosue queue ?
 */

#ifdef _MSC_VER
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#endif

#define NEED_REAL_IMAGE_STRUCTURE
#define USE_IMAGE_INTERFACE l.gl_image_interface

#include <stdhdrs.h>


#include "local.h"

RENDER_NAMESPACE
// move local into render namespace.

HWND  GetNativeHandle (PVIDEO hVideo);

extern KEYDEFINE KeyDefs[];

	static struct touch_event_state
	{
		struct touch_event_flags
		{
			BIT_FIELD owned_by_surface : 1;
		}flags;
		PRENDERER owning_surface;

		struct touch_event_one{
			struct touch_event_one_flags {
				BIT_FIELD bDrag : 1;
			} flags;
			int x;
         int y;
		} one;
		struct touch_event_two{
			int x;
         int y;
		} two;
		struct touch_event_three{
			int x;
         int y;
		} three;
	} touch_info;



void  GetDisplaySizeEx ( int nDisplay
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
{
		if( x )
         (*x) = 0;
		if( y )
			(*y) = 0;


		{
         int set = 0;
			if( l.cameras )
			{
				struct display_camera *camera = (struct display_camera *)GetLink( &l.cameras, 0 );
				if( camera && ( camera != (struct display_camera*)1 ) )
				{
               set = 1;
					if( width )
						(*width) = camera->w;
					if( height )
						(*height) = camera->h;
				}
			}
         if( !set )
			{
				if( width )
					(*width) = l.default_display_x;
				if( height )
					(*height) = l.default_display_y;
			}
		}
}


void SetCameraNativeHandle( struct display_camera *camera )
{
   camera->displayWindow = l.displayWindow;
}

// this is dynamically linked from the loader code to get the window
void SACK_Vidlib_SetNativeWindowHandle( NativeWindowType displayWindow )
{
   lprintf( "Setting native window handle... (shouldn't this do something else?)" );
	l.displayWindow = displayWindow;

   // Standard init (was looking more like a common call thing)
	HostSystem_InitDisplayInfo();
	// creates the cameras.

	LoadOptions();
	l.flags.disallow_3d = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Video Render/Disallow 3D"), 1 );
}

void HostSystem_InitDisplayInfo(void )
{
	// this is passed in from the external world; do nothing, but provide the hook.
	// have to wait for this ....
	l.default_display_x = ANativeWindow_getWidth( l.displayWindow);
	l.default_display_y = ANativeWindow_getHeight( l.displayWindow);
	//default_display_x	ANativeWindow_getFormat( camera->displayWindow)

}

// this is linked to external native activiety shell...
void SACK_Vidlib_DoRenderPass( void )
{

	if( l.flags.disallow_3d )
		return;

	ProcessGLDraw(TRUE);
   return;

			Move( l.origin );

			{
				INDEX idx;
				Update3dProc proc;
				LIST_FORALL( l.update, idx, Update3dProc, proc )
				{
					if( proc( l.origin ) )
                  l.flags.bUpdateWanted = TRUE;
				}
			}

			// no reason to check this if an update is already wanted.
			if( !l.flags.bUpdateWanted )
			{
				// set l.flags.bUpdateWanted for window surfaces.
				WantRender3D();
			}

			{
				struct display_camera *camera;
				INDEX idx;
				l.flags.bUpdateWanted = 0;
				LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
				{
               // skip 'default_camera'
					if( !idx )
                  continue;
					if( !camera->hVidCore->flags.bReady )
						return;
					// if plugins or want update, don't continue.
					if( !camera->plugins && !l.flags.bUpdateWanted )
					{
						lprintf( "no update" );
						//continue;
					}
					
					if( !camera->hVidCore || !camera->hVidCore->flags.bReady )
					{
						lprintf( "not ready" );
		
						continue;
					}
					// drawing may cause subsequent draws; so clear this first
					// do OpenGL Frame
#ifdef USE_EGL
					EnableEGLContext( camera );
#else
					SetActiveGLDisplay( camera );
#endif
					Render3D( camera );
#ifdef USE_EGL
					//lprintf( "doing swap buffer..." );
					eglSwapBuffers( camera->egl_display, camera->surface );
#endif
				}
			}

}

int SACK_Vidlib_SendTouchEvents( int nPoints, PINPUT_POINT points )
{
	{
		//if( hVideo )
		{
			int handled = 0;
			//if( hVideo->pTouchCallback )
			{
			//	handled = hVideo->pTouchCallback( hVideo->dwTouchData, inputs, count );
			}

			if( !handled )
			{
				// this will be like a hvid core
				handled = Handle3DTouches( ((struct display_camera *)GetLink( &l.cameras, 0 )), points, nPoints );
			}
			return handled;
		}
	}
}

static void BeginVisPersp( struct display_camera *camera )
{
	//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	//glLoadIdentity();									// Reset The Projection Matrix
	MygluPerspective(90.0f,camera->aspect,1.0f,camera->depth);
	//glGetFloatv( GL_PROJECTION_MATRIX, (GLfloat*)l.fProjection );
	//PrintMatrix( l.fProjection );
	//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
}


int Init3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
#ifdef USE_EGL
	EnableEGLContext( camera );
#else
	SetActiveGLDisplay( camera );
#endif
//	if( !SetActiveGLDisplay( camera ) )
//		return FALSE;


	if( !camera->flags.init )
	{

		glEnable( GL_BLEND );
 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable( GL_TEXTURE_2D );
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
#ifndef __ANDROID__
		glEnable(GL_NORMALIZE); // glNormal is normalized automatically....
		glEnable( GL_ALPHA_TEST );
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
		//glEnable( GL_POLYGON_SMOOTH );
		//glEnable( GL_POLYGON_SMOOTH_HINT );
		glEnable( GL_LINE_SMOOTH );
		//glEnable( GL_LINE_SMOOTH_HINT );
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
 		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
#endif
 
 
		BeginVisPersp( camera );
		lprintf( WIDE("First GL Init Done.") );
		camera->flags.init = 1;
		camera->hVidCore->flags.bReady = TRUE;
	}


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepthf(1.0f);									// Depth Buffer Setup
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}



void SetupPositionMatrix( struct display_camera *camera )
{
	// camera->origin_camera is valid eye position matrix
#ifdef ALLOW_SETTING_GL1_MATRIX
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
	glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );
#endif
}

void EndActive3D( struct display_camera *camera ) // does appropriate EndActiveXXDisplay
{
#ifdef USE_EGL
	//lprintf( "doing swap buffer..." );
	eglSwapBuffers( camera->egl_display, camera->surface );
#endif
}


RENDER_NAMESPACE_END


