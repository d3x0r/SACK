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

#include <emscripten/html5.h>

#include "local.h"

RENDER_NAMESPACE
// move local into render namespace.

HWND  GetNativeHandle (PVIDEO hVideo);


int SetActiveGLDisplayView( struct display_camera *camera, int nFracture )
{
	static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE last_active;
	if( last_active )
	{
		emscripten_webgl_commit_frame( last_active );
		last_active = NULL;
	}
	if( camera )
	{
		if( camera->hVidCore )
		{
			emscripten_webgl_make_context_current( last_active = camera->displayWindow );
		}
	}
}

RENDER_PROC( int, SetActiveGLDisplay )( struct display_camera *hDisplay )
{
   return SetActiveGLDisplayView( hDisplay, 0 );
}



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
												 , int32_t *x, int32_t *y
												 , uint32_t *width, uint32_t *height)
{
	while( !l.default_display_x || !l.default_display_y )
	{
		//lprintf( "Didn't have a display yet.. .wait..." );
		Relinquish();
	}
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
               lprintf( "was able to get size from camera..." );
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
		//lprintf( "Asked for size... %d,%d", (*width), (*height) );
}


void SetCameraNativeHandle( struct display_camera *camera )
{
   camera->displayWindow = l.displayWindow;
}

// this is dynamically linked from the loader code to get the window
void SACK_Vidlib_SetNativeWindowHandle( EMSCRIPTEN_WEBGL_CONTEXT_HANDLE    displayWindow )
{
   lprintf( "Setting native window handle... (shouldn't this do something else?)" );
	l.displayWindow = displayWindow;
   // Standard init (was looking more like a common call thing)
	HostSystem_InitDisplayInfo();
	// creates the cameras.

	LoadOptions();

	if( !l.bThreadRunning )
	{
		l.bThreadRunning = TRUE;
		//SACK_Vidlib_OpenCameras();
	}
	//l.flags.disallow_3d = (RCOORD)SACK_GetProfileInt( GetProgramName(), "SACK/Video Render/Disallow 3D", 1 );
}

void SACK_Vidlib_SetAnimationWake( void (*wake_callback)(void))
{
   l.wake_callback = wake_callback;
}

void HostSystem_InitDisplayInfo(void )
{
	// this is passed in from the external world; do nothing, but provide the hook.
	// have to wait for this ....
	//lprintf( "SET size here..." );
   emscripten_webgl_get_drawing_buffer_size( l.displayWindow, &l.default_display_x, &l.default_display_y );
	//default_display_x	ANativeWindow_getFormat( camera->displayWindow)

}

// this is linked to external native activiety shell...
int SACK_Vidlib_DoRenderPass( void )
{
lprintf( "DoRenderPass..." );
	return ProcessGLDraw(TRUE);
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


int Init3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
   // this is called every render pass
	//lprintf( "Init3d" );
#ifdef USE_EGL
	EnableEGLContext( camera );
#else
	SetActiveGLDisplay( camera );
#endif
//	if( !SetActiveGLDisplay( camera ) )
//		return FALSE;
	if( !camera->flags.init )
	{
		lprintf( "First setup" );
		glEnable( GL_BLEND );
		CheckErr();
		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		CheckErr();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		CheckErr();
		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		CheckErr();

		MygluPerspective(90.0f,camera->aspect,1.0f,camera->depth);
		CheckErr();
		//lprintf( "First GL Init Done." );
		camera->flags.init = 1;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.1f);				// Black Background
	CheckErr();
	glClearDepthf(1.0f);									// Depth Buffer Setup
	CheckErr();
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer
	CheckErr();

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


