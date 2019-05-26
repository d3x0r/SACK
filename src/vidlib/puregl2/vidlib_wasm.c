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

static struct display_camera *defaultCamera;
static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE last_active;

int SetActiveGLDisplayView( struct display_camera *camera, int nFracture )
{
	if( last_active )
	{
		//lprintf( "This commits the prior frame %d", last_active );
		//emscripten_webgl_commit_frame( last_active );
		last_active = 0;
	}
	if( camera )
	{
		if( camera->hVidCore )
		{
			//lprintf( "Setting camera display HRC: %p", camera->displayWindow );
			emscripten_webgl_make_context_current( last_active = camera->displayWindow );
		}
	}
	return 1;
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
               				//lprintf( "was able to get size from camera..." );
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

struct display_camera **initializingCamera;

void SetCameraNativeHandle( struct display_camera *camera )
{
	//lprintf( "SetCameraNativeHandle; called to set ahndle: %d", initializingCamera );
	if( initializingCamera )
		initializingCamera[0] = camera;
	//lprintf( "SET CAMERA HRC: %p", l.displayWindow );
	camera->displayWindow = l.displayWindow;
}


void HostSystem_InitDisplayInfo(void )
{
	// this is passed in from the external world; do nothing, but provide the hook.
	// have to wait for this ....
	//lprintf( "SET size here..." );
	emscripten_webgl_get_drawing_buffer_size( l.displayWindow, &l.default_display_x, &l.default_display_y );
	//lprintf( "SET WINDOW SIZE FROM CONTROL: %d %d", l.default_display_x, l.default_display_y );
	//default_display_x	ANativeWindow_getFormat( camera->displayWindow)
}

// this is dynamically linked from the loader code to get the window
void SACK_Vidlib_SetNativeWindowHandle( EMSCRIPTEN_WEBGL_CONTEXT_HANDLE    displayWindow )
{
	//lprintf( "Setting native window handle... (shouldn't this do something else?) %p", displayWindow );

	l.displayWindow = displayWindow;
	// Standard init (was looking more like a common call thing)
	HostSystem_InitDisplayInfo();
	// creates the cameras.

	LoadOptions();

	l.bThreadRunning = TRUE; // it is.
}

void SACK_Vidlib_SetAnimationWake( void (*wake_callback)(void))
{
	// this is called when the  display wants a update
	// but it's not continuous update.
	l.wake_callback = wake_callback;
}

// this is linked to external native activiety shell...
int SACK_Vidlib_DoRenderPass( void )
{
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
		//lprintf( "First setup" );
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
		camera->flags.init = 1;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
	CheckErr();
	glClearDepthf(1.0f);									// Depth Buffer Setup
	CheckErr();
	//lprintf( "glClear" );
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer
	CheckErr();

	return TRUE;										// Initialization Went OK
}



void SetupPositionMatrix( struct display_camera *camera )
{
	// camera->origin_camera is valid eye position matrix
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
#ifdef ALLOW_SETTING_GL1_MATRIX
	lprintf( "IS THIS SET?" );
//	glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );
#endif
}

void EndActive3D( struct display_camera *camera ) // does appropriate EndActiveXXDisplay
{
	//lprintf( "End of rendering... all done." );
#ifdef USE_EGL
	//lprintf( "doing swap buffer..." );
	eglSwapBuffers( camera->egl_display, camera->surface );
#endif
}



#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <emscripten/html5.h>
#include <emscripten.h>

#define NULL ((void*)0)

static EM_BOOL em_touch_callback_handler(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData){
	lprintf( "Touch callback..." );
	return false; // or true consumed
}

static EM_BOOL em_mouse_callback_handler(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
	struct display_camera *camera = ((struct display_camera**)userData)[0];
	double mTick = mouseEvent->timestamp;
	EM_BOOL retval = FALSE;
	// mouseEvent->screenX, mouseEvent->screenY
	// mouseEvent->clientX, mouseEvent->clientY
	// EM_BOOL ctrlKey EM_BOOL shiftKey EM_BOOL altKey EM_BOOL metaKey
	
	//  canvasX long canvasY
	//mouseEvent->button // which one changed.
	//mouseEvent->buttons
	// movementX long movementY;

	l.mouse_b = mouseEvent->buttons;
#if 0
 // click-and drag capture...
		//hWndLastFocus = hWnd;
		if( l.hCameraCaptured )
		{
#ifdef LOG_MOUSE_EVENTS
			lprintf( "Captured mouse already - don't do anything?" );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
				lprintf( "Auto owning mouse to surface which had the mouse clicked DOWN." );
#endif
				if( !l.hCameraCaptured )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( "Auto release mouse from surface which had the mouse unclicked." );
				if( !l.hCameraCaptured )
					ReleaseCapture();
			}
		}
#endif
		{
			l.mouse_x = mouseEvent->targetX;
			l.mouse_y = mouseEvent->targetY;
		}
#if 0
		if( l.last_mouse_update )
		{
			if( ( l.last_mouse_update + 1000 ) < timeGetTime() )
			{
				if( !l.flags.mouse_on ) // mouse is off...
					l.last_mouse_update = 0;
			}
		}
		if( l.mouse_x != l._mouse_x ||
			l.mouse_y != l._mouse_y ||
			l.mouse_b != l._mouse_b ||
			l.mouse_last_vid != hVideo ) // this hvideo!= last hvideo?
		{
			if( (!hVideo->flags.mouse_on || !l.flags.mouse_on ) && !hVideo->flags.bNoMouse)
			{
				int x;
				if (!hCursor)
					hCursor = LoadCursor (NULL, IDC_ARROW);
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "cursor on." );
#endif
				x = ShowCursor( TRUE );
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( "cursor count %d %d", x, hCursor );
#endif
				SetCursor (hCursor);
				l.flags.mouse_on = 1;
				hVideo->flags.mouse_on = 1;
			}
			if( hVideo->flags.bIdleMouse )
			{
				l.last_mouse_update = timeGetTime();
#ifdef LOG_MOUSE_HIDE_IDLE
			lprintf( "Tick ... %d", l.last_mouse_update );
#endif
		}
		l.mouse_last_vid = hVideo;
#endif

		if( l.flags.bRotateLock  )
		{
			lprintf( "rotate lock..." );
			RCOORD delta_x = l.mouse_x - (camera->w/2);
			RCOORD delta_y = l.mouse_y - (camera->h/2);
			//lprintf( "mouse came in we're at %d,%d %g,%g", l.mouse_x, l.mouse_y, delta_x, delta_y );
			if( delta_y || delta_x )
			{
				static int toggle;
				delta_x /= camera->w;
				delta_y /= camera->h;
				if( toggle )
				{
					RotateRel( l.origin, delta_y, 0, 0 );
					RotateRel( l.origin, 0, delta_x, 0 );
				}
				else
				{
					RotateRel( l.origin, 0, delta_x, 0 );
					RotateRel( l.origin, delta_y, 0, 0 );
				}
				toggle = 1-toggle;
				l.mouse_x = camera->w/2;
				l.mouse_y = camera->h/2;
				//lprintf( "Set curorpos.." );

				//SetCursorPos( hVideo->pWindowPos.x + hVideo->pWindowPos.cx/2, hVideo->pWindowPos.y + hVideo->pWindowPos.cy / 2 );
				//lprintf( "Set curorpos Done.." );
			}
		}
		else if (camera->hVidCore->pMouseCallback)
		{
			retval = (EM_BOOL)camera->hVidCore->pMouseCallback (camera->hVidCore->dwMouseData,
											l.mouse_x, l.mouse_y, l.mouse_b);
		}
		l._mouse_x = l.mouse_x;
		l._mouse_y = l.mouse_y;
		// clear scroll buttons...
		// otherwise circumstances of mouse wheel followed by any other event
		// continues to generate scroll clicks.
		l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
		l._mouse_b = l.mouse_b;
	
	return retval;			// don't allow windows to think about this...

}

static EM_BOOL em_key_callback_handler(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	lprintf( "Key callback..." );
	return false;
}

static EM_BOOL em_webgl_context_handler(int eventType, const void *reserved, void *userData){

	return false;
}


PRELOAD( do_init_display ) {
//void initDisplay() {
	EMSCRIPTEN_RESULT r;
	struct display_camera ** defaultCamera = New( struct display_camera * );
	void *myState = (void*)defaultCamera;
	defaultCamera[0] = NULL;
	initializingCamera = defaultCamera;
	const char *display = "SACK Display 1";
	r = emscripten_set_touchstart_callback( display, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchend_callback(display, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchmove_callback(display, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchcancel_callback(display, myState, true, em_touch_callback_handler);

	r = emscripten_set_click_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mousedown_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseup_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_dblclick_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mousemove_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseenter_callback(display, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseleave_callback(display, myState, true, em_mouse_callback_handler);


	r = emscripten_set_keypress_callback(display, myState, true, em_key_callback_handler);
	r = emscripten_set_keydown_callback(display, myState, true, em_key_callback_handler);
	r = emscripten_set_keyup_callback(display, myState, true, em_key_callback_handler);

	r = emscripten_set_webglcontextlost_callback(display, myState, true, em_webgl_context_handler);
	r = emscripten_set_webglcontextrestored_callback(display, myState, true, em_webgl_context_handler);

        {
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE hGL;
	EmscriptenWebGLContextAttributes attribs = {};
	emscripten_webgl_init_context_attributes( & attribs );
//	attribs.alpha = true; // default
//	attribs.depth = true; // default
//	attribs.stencil = false; // default
	attribs.antialias = false; // not default
//	attribs.premultipliedAlpha = true; // defalt
//	attribs.preserveDrawingBuffer = false; // default
//	attribs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT; //  EM_WEBGL_POWER_PREFERENCE_DEFAULT, EM_WEBGL_POWER_PREFERENCE_LOW_POWER, EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE.
	attribs.failIfMajorPerformanceCaveat = false; // allow slow fallback paths
	attribs.majorVersion = 2;
	attribs.minorVersion = 0; // 1.0 webgl.; ther eis 2.0 but ....

//	attribs.enableExtensionsByDefault = true;
//	attribs.explicitSwapControl = false; // requires other compiler options to enable this.
//	attribs.renderViaOffscreenBackBuffer = false;  //
	//   attribs.proxyContextToMainThread =

	hGL = emscripten_webgl_create_context(display, &attribs );
	{
		extern void SACK_Vidlib_SetNativeWindowHandle( EMSCRIPTEN_WEBGL_CONTEXT_HANDLE    displayWindow );
		//lprintf( "specify HRC %d", hGL );
		SACK_Vidlib_SetNativeWindowHandle( hGL );
	}
	}
//	emscripten_set_main_loop( renderLoop, 0, 1 );

	/* Other usefule things
    *
    * emscripten_webgl_get_drawing_buffer_size(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, int *width, int *height)

    EM_BOOL emscripten_webgl_enable_extension(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, const char *extension)
   EM_BOOL emscripten_webgl_enable_extension(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, const char *extension)
	EMSCRIPTEN_RESULT emscripten_get_canvas_element_size(const char *target, int *width, int *height)

   dictionary WebGLContextAttributes {
    boolean alpha = true;
    boolean depth = true;
    boolean stencil = false;
    boolean antialias = true;
    boolean premultipliedAlpha = true;
    boolean preserveDrawingBuffer = false;
    WebGLPowerPreference powerPreference = "default";
    boolean failIfMajorPerformanceCaveat = false;
    boolean desynchronized = false;
};

    */

	/* focus */
  //typedef EM_BOOL (*em_webgl_context_callback)(int eventType, const void *reserved, void *userData);
//   r = emscripten_webgl_make_context_current( EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context );

}


RENDER_NAMESPACE_END


