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
#include <stdbool.h>
#include <emscripten/html5.h>

#include "local.h"

#include <emscripten/html5.h>
#include <emscripten.h>


RENDER_NAMESPACE
// move local into render namespace.
	const char *displayName = "[id='SACK Display 1']";

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
			//lprintf( "Setting camera displayName HRC: %p", camera->displayWindow );
			emscripten_webgl_make_context_current( last_active = camera->displayWindow );
		}
	}
	return 1;
}

RENDER_PROC( int, SetActiveGLDisplay )( struct display_camera *hDisplay )
{
	return SetActiveGLDisplayView( hDisplay, 0 );
}





void  GetDisplaySizeEx ( int nDisplay
												 , int32_t *x, int32_t *y
												 , uint32_t *width, uint32_t *height)
{
	while( !l.default_display_x || !l.default_display_y )
	{
		//lprintf( "Didn't have a displayName yet.. .wait..." );
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
	//emscripten_webgl_get_drawing_buffer_size( l.displayWindow, &l.default_display_x, &l.default_display_y );
   emscripten_get_canvas_element_size( displayName, &l.default_display_x, &l.default_display_y );
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
	// this is called when the  displayName wants a update
	// but it's not continuous update.
	l.wake_callback = wake_callback;
}

// this is linked to external native activiety shell...
int SACK_Vidlib_DoRenderPass( void )
{	int wantDraw;
	void (*wake_callback)(void);
	wake_callback = l.wake_callback;
	l.wake_callback= NULL;

#if 0
	{
		EmscriptenDeviceOrientationEvent state;
		EmscriptenDeviceOrientationEvent *deviceOrientationEvent = &state;
		emscripten_get_deviceorientation_status(&state);
		RCOORD a = deviceOrientationEvent->alpha;  // z is up (0 is long along y)
		RCOORD b = deviceOrientationEvent->beta;  // x is the narrow side
		RCOORD c = deviceOrientationEvent->gamma;  // y is the long forward along base?
		lprintf( "Orientation.  %g %g %g", a, b, c  );
	}
#endif
	wantDraw = ProcessGLDraw(TRUE);



	l.wake_callback = wake_callback;
	return wantDraw;
}



int Init3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
   // this is called every render pass
	//lprintf( "Init3d" );
	SetActiveGLDisplay( camera );
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

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	//CheckErr();
	glClearDepthf(1.0f);									// Depth Buffer Setup
	//CheckErr();
	//lprintf( "glClear" );
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer
	//CheckErr();

	return TRUE;										// Initialization Went OK
}



void SetupPositionMatrix( struct display_camera *camera )
{
	// camera->origin_camera is valid eye position matrix
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
}

void EndActive3D( struct display_camera *camera ) // does appropriate EndActiveXXDisplay
{
}

static struct input_point touch_points[32];
static struct {
	int id;
	int deleted;
} touch_point_state[32];
static int used_touch_points;

static EM_BOOL em_touch_callback_handler(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData){
	struct display_camera *camera = ((struct display_camera**)userData)[0];
	int n, m;
	int point;
	if( !camera ) return false;
	
#if 0
	lprintf( "Touch callback... %d %d",eventType,  touchEvent->numTouches );
	for( point = 0; point < touchEvent->numTouches; point++ ) {
		lprintf( "touches: %d  %d %d", point, touchEvent->touches[point].targetX, touchEvent->touches[point].targetY );
	}
#endif
	for( n = 0; n < used_touch_points; n++ )  {
		// any old event is not new.
		touch_points[n].flags.new_event = 0;
		// end events would have been sent, remove them now.
		if( touch_points[n].flags.end_event ) 
			while( n < used_touch_points && touch_point_state[n].deleted ) {
				for( m = n+1; m < used_touch_points; m++ ) {
					touch_points[m-1] = touch_points[m];
					touch_point_state[m-1] = touch_point_state[m];
				}
				used_touch_points--;
			}		
		touch_points[used_touch_points].flags.end_event = 0;
	}

	if( eventType == 23 ){ // end
		for( point = 0; point < used_touch_points; point++ ) {
			touch_point_state[point].deleted = 1;
		}
			
		for( n = 0; n < touchEvent->numTouches; n++ ) {
			int tid;
			if( touchEvent->touches[n].isChanged ) continue; // this is the one to delete, definatly don't undetlete
			tid = touchEvent->touches[n].identifier;
			for( point = 0; point < used_touch_points; point++ ) {
				if( touch_point_state[point].id == tid ) {
					break;
				}
			}
			if( point < used_touch_points )
				touch_point_state[point].deleted = 0;
		}
		for( n = 0; n < used_touch_points; n++ ) {
			if( touch_point_state[n].deleted )
				if( !touch_points[n].flags.end_event )
					touch_points[n].flags.end_event = 1;
		}
	}
	//lprintf( "..." );
	if( eventType == 22 || eventType == 24 ) {
		 // start
		for( n = 0; n < touchEvent->numTouches; n++ ) {
			int tid = touchEvent->touches[n].identifier;
			int point;
			for( point = 0; point < used_touch_points; point++ ) {
				if( touch_point_state[point].id == tid ) {
					break;
				}
			}
			if(  point == used_touch_points ) {
				//lprintf( "Add a new point." );
				used_touch_points++;
				touch_point_state[point].id = tid;
				touch_points[point].flags.new_event = 1;
				touch_points[point].flags.end_event = 0;
				touch_point_state[point].deleted = 0;
			} 

			if( !l.real_display_x ) {
				//lprintf( "Bad Paramters..." );
				l.real_display_x = l.default_display_x;
				l.real_display_y = l.default_display_y;
			}
			touch_points[point].x = touchEvent->touches[n].targetX *l.default_display_x/ l.real_display_x;
			touch_points[point].y = touchEvent->touches[n].targetY *l.default_display_y/ l.real_display_y;
		}
	}
	if( l.wake_callback ) {
		l.wake_callback();
	}
	//lprintf( "Send events:%d", used_touch_points );
	return Handle3DTouches( camera, touch_points, used_touch_points );
	//lprintf( "")
	
	//return true; // or true consumed
}





static EM_BOOL em_mouse_callback_handler(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
	struct display_camera *camera = ((struct display_camera**)userData)[0];
   if( !camera ) return false;
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
	/* do we need to do mouse capture fo out of window events? didn't seem to.*/

	{
		if( !l.real_display_x ) {
			//lprintf( "Bad Paramters..." );
			l.real_display_x = l.default_display_x;
			l.real_display_y = l.default_display_y;
		}
		l.mouse_x = mouseEvent->targetX *l.default_display_x/ l.real_display_x;
		l.mouse_y = mouseEvent->targetY *l.default_display_y/ l.real_display_y;
		//lprintf( "mouse is %d %d  %d %d  %g %d", l.mouse_x, l.mouse_y, mouseEvent->targetX, mouseEvent->targetY, l.real_display_x, l.default_display_x );
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

		if( l.flags.bRotateLock && !l.mouse_b )
		{
			//lprintf( "rotate lock..." );
			RCOORD delta_x = l.mouse_x - l._mouse_x;//(camera->w/2);
			RCOORD delta_y = l.mouse_y - l._mouse_y;//(camera->h/2);
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
            // TEMPORARY DON'T WARP MOUSEl
				//l.mouse_x = camera->w/2;
				//l.mouse_y = camera->h/2;

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

      //lprintf( "Mouse Event used? %d", retval );
		return (retval!=0 );			// don't allow windows to think about this...

}

static EM_BOOL em_key_callback_handler(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData, int pressed, const char *key ){
	struct display_camera *camera = ((struct display_camera**)userData)[0];
	if( !camera ) return false;
	switch( keyEvent->keyCode )
	{
	case KEY_F1: // shift
	case KEY_F2: // shift
	case KEY_F3: // shift
	case KEY_F4: // shift
	case KEY_F12: // shift
	case KEY_SCROLL_LOCK: // shift
	case KEY_ALT: // shift
	case KEY_CONTROL: // shift
	case KEY_SHIFT: // shift
	case KEY_LEFT: // shift
	case KEY_RIGHT: // shift
	case KEY_UP: // shift
	case KEY_DOWN: // shift
	case 1: // shift
		key = NULL;
		break;
	case KEY_ENTER: //backspace // Windows VK_BACKSPACE
		//lprintf( "we like to send newline?" );
		key = "\n";
		break;
	case KEY_BACKSPACE: //backspace // Windows VK_BACKSPACE
		lprintf( "is a backspace... swapping key to \\b");
		key = "\b";
		break;
	}
	if( l.current_key_text )
		free( (char*)l.current_key_text );
	if( key )
		l.current_key_text = strdup( key );
	else
		l.current_key_text = NULL;
	if( pressed < 2 ) {
		int key_mods = (keyEvent->ctrlKey?KEY_MOD_CTRL:0)
			+(keyEvent->shiftKey?KEY_MOD_SHIFT:0)
			+(keyEvent->altKey?KEY_MOD_ALT:0)
			+(keyEvent->metaKey?KEY_MOD_META:0);
		int key_index = keyEvent->keyCode;
		uint32_t normal_key = (pressed?KEY_PRESSED:0)
			| ( key_mods & 7 ) << 28
			| ( key_index & 0xFF ) << 16
			| ( key_index )
			;
		//lprintf( "CODE: %08x  %d %d", normal_key, key_mods, key_index );
		{
			int used = DispatchKeyEvent( camera->hVidCore, normal_key );
			//lprintf( "USED? %d", used );
			return ( used != 0 );
		}
	}
	return false;
	//EM_BOOL ctrlKey EM_BOOL shiftKey EM_BOOL altKey EM_BOOL metaKey
	// 
}


static EM_BOOL em_key_callback_handler_dn(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	//lprintf( "Key dn callback... %s %s", keyEvent->key, keyEvent->code  );
	//lprintf( "Key dn callback... %d %d %d", keyEvent->charCode, keyEvent->keyCode , keyEvent->which );
	return em_key_callback_handler( eventType, keyEvent, userData, 1, keyEvent->key );
}
static EM_BOOL em_key_callback_handler_up(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	//lprintf( "Key up callback... %s %s", keyEvent->key, keyEvent->code  );
	//lprintf( "Key up callback... %d %d %d", keyEvent->charCode, keyEvent->keyCode , keyEvent->which );
	return em_key_callback_handler( eventType, keyEvent, userData, 0, keyEvent->key );
}
static EM_BOOL em_key_callback_handler_pr(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData){
	//lprintf( "Key pr callback... %s %s", keyEvent->key, keyEvent->code  );
	//lprintf( "Key pr callback... %d %d %d", keyEvent->charCode, keyEvent->keyCode , keyEvent->which );
	return em_key_callback_handler( eventType, keyEvent, userData, 2, keyEvent->key );
}

static EM_BOOL em_webgl_context_handler(int eventType, const void *reserved, void *userData){

	struct display_camera *camera = ((struct display_camera**)userData)[0];
	lprintf( "webgl context event");
   if( !camera ) return false;
	return false;
}


static EM_BOOL em_resize_callback_handler( int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
	lprintf( "some sort of resize event: %d", eventType );
	switch( eventType ) {
	default:
		 emscripten_get_element_css_size( displayName, &l.real_display_x, &l.real_display_y );
	}
	return true;
}


static EM_BOOL em_deviceorientation_callback_handler(int eventType, const EmscriptenDeviceOrientationEvent *deviceOrientationEvent, void *userData){
	// EMSCRIPTEN_EVENT_DEVICEORIENTATION
	RCOORD a = deviceOrientationEvent->alpha;  // z is up (0 is long along y)
	RCOORD b = deviceOrientationEvent->beta;  // x is the narrow side
	RCOORD c = deviceOrientationEvent->gamma;  // y is the long forward along base?
	lprintf( "%d   Orientation.  %g %g %g", eventType, a, b, c  );
	if( deviceOrientationEvent->absolute ) {
		RotateAbs( l.origin, b, c, a );
      if( l.wake_callback ) l.wake_callback();
	} else {
		lprintf( "Ignore orientation" );
	}
	
}

PRELOAD( do_init_display ) {
//void initDisplay() {
	EMSCRIPTEN_RESULT r;
	struct display_camera ** defaultCamera = New( struct display_camera * );
	void *myState = (void*)defaultCamera;
	defaultCamera[0] = NULL;
	initializingCamera = defaultCamera;

	r = emscripten_get_element_css_size( displayName, &l.real_display_x, &l.real_display_y );

	r = emscripten_set_resize_callback( displayName, myState, true, em_resize_callback_handler );

	r = emscripten_set_touchstart_callback( displayName, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchend_callback(displayName, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchmove_callback(displayName, myState, true, em_touch_callback_handler);
	r = emscripten_set_touchcancel_callback(displayName, myState, true, em_touch_callback_handler);

	r = emscripten_set_click_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mousedown_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseup_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_dblclick_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mousemove_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseenter_callback(displayName, myState, true, em_mouse_callback_handler);
	r = emscripten_set_mouseleave_callback(displayName, myState, true, em_mouse_callback_handler);


	//r = emscripten_set_keypress_callback(displayName, myState, true, em_key_callback_handler_pr);
	r = emscripten_set_keydown_callback(displayName, myState, true, em_key_callback_handler_dn);
	r = emscripten_set_keyup_callback(displayName, myState, true, em_key_callback_handler_up);

	r = emscripten_set_webglcontextlost_callback(displayName, myState, true, em_webgl_context_handler);
	r = emscripten_set_webglcontextrestored_callback(displayName, myState, true, em_webgl_context_handler);

	r = emscripten_set_deviceorientation_callback( myState, true, em_deviceorientation_callback_handler );

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

	hGL = emscripten_webgl_create_context(displayName, &attribs );
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


