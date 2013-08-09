/****************
 * Some performance concerns regarding high numbers of layered windows...
 * every 1000 windows causes 2 - 12 millisecond delays...
 *    the first delay is between CreateWindow and WM_NCCREATE
 *    the second delay is after ShowWindow after WM_POSCHANGING and before POSCHANGED (window manager?)
 * this means each window is +24 milliseconds at least at 1000, +48 at 2000, etc...
 * it does seem to be a linear progression, and the lost time is
 * somewhere within the windows system, and not user code.
 ************************************************/



//#define _OPENGL_ENABLED
/* this must have been done for some other collision in some other bit of code...
 * probably the update queue? the mosue queue ?
 */
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_EVENTS
//#define USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
#if defined( UNDER_CE )
#define NO_MOUSE_TRANSPARENCY
#define NO_ENUM_DISPLAY
#define NO_DRAG_DROP
#define NO_TRANSPARENCY
#undef _OPENGL_ENABLED
#else
#  if defined( __WINDOWS__ )
#    define USE_KEYHOOK
#  endif
#endif

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

#ifdef _WIN32
#include <shlwapi.h> // have to include this if shellapi.h is included (for mingw)
#include <shellapi.h> // very last though - this is DragAndDrop definitions...
#endif

// this is safe to leave on.
#define LOG_ORDERING_REFOCUS

//#define LOG_MOUSE_EVENTS
//#define LOG_RECT_UPDATE
//#define LOG_DESTRUCTION
#define LOG_STARTUP
//#define LOG_FOCUSEVENTS
//#define OTHER_EVENTS_HERE
//#define LOG_SHOW_HIDE
//#define LOG_DISPLAY_RESIZE
//#define NOISY_LOGGING
// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS
#define LOG_OPEN_TIMING
//#define LOG_MOUSE_HIDE_IDLE
//#define LOG_OPENGL_CONTEXT

#include "local.h"

RENDER_NAMESPACE
// move local into render namespace.

HWND  GetNativeHandle (PVIDEO hVideo);
void MygluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);

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
			struct display_camera *camera = (struct display_camera *)GetLink( &l.cameras, 0 );
			if( camera && ( camera != (struct display_camera*)1 ) )
			{
				if( width )
					(*width) = camera->w;
				if( height )
					(*height) = camera->h;
			}
			else
			{
				if( width )
					(*width) = 640;
				if( height )
					(*height) = 480;
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
   l.displayWindow = displayWindow;
}

void HostSystem_InitDisplayInfo(void )
{
   // this is passed in from the external world; do nothing, but provide the hook.
}

// this is linked to external native activiety shell...
void SACK_Vidlib_DoRenderPass( void )
{

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
				WantRenderGL();
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
					EnableEGLContext( camera->hVidCore );
#else
					SetActiveGLDisplay( camera->hVidCore );
#endif
					RenderGL( camera );
#ifdef USE_EGL
					//lprintf( "doing swap buffer..." );
					eglSwapBuffers( camera->hVidCore->display, camera->hVidCore->surface );
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



RENDER_NAMESPACE_END


