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
/* again, this should be moved to stdhdrs so we get timeGetTime() */
#ifdef USE_GLES2
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
//#include "glues/source/glues.h"
#else
#include <GL/gl.h>
#include "../glext.h"
#endif

#ifdef __USE_FREEGLUT__
#define FREEGLUT_GLES2
#include <GL/freeglut.h>
#endif

#include <sqlgetoption.h>
#include <deadstart.h>
#include <stdio.h>
#include <string.h>
#include <timers.h>
#include <sharemem.h>
//#define NO_LOGGING
#include <logging.h>
#include <msgclient.h>
#include <idle.h>
//#include <imglib/imagestruct.h>
#define NEED_VECTLIB_COMPARE
#include <image.h>
#include <vectlib.h>
#undef StrDup

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
#include <vidlib/vidstruc.h>
#include <render3d.h>
//#include "vidlib.H"

#include <keybrd.h>

static int stop;
//HWND     hWndLastFocus;

// commands to the video thread for non-windows native ....
#define CREATE_VIEW 1

#define WM_USER_CREATE_WINDOW  WM_USER+512
#define WM_USER_DESTROY_WINDOW  WM_USER+513
#define WM_USER_SHOW_WINDOW  WM_USER+514
#define WM_USER_HIDE_WINDOW  WM_USER+515
#define WM_USER_SHUTDOWN     WM_USER+516
#define WM_USER_MOUSE_CHANGE     WM_USER+517
#define WM_USER_OPEN_CAMERAS    WM_USER+518

#define WM_RUALIVE 5000 // lparam = pointer to alive variable expected to set true
#define WD_HVIDEO   0   // WindowData_HVIDEO


IMAGE_NAMESPACE

struct saved_location
{
	S_32 x, y;
	_32 w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
	Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h );
	PTRSZVAL psv;
};
IMAGE_NAMESPACE_END

RENDER_NAMESPACE

// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

	HWND  GetNativeHandle (PVIDEO hVideo);

extern KEYDEFINE KeyDefs[];

#ifdef USE_EGL

void OpenEGL( struct display_camera *camera )
{
	/*
	 * The layer settings haven't taken effect yet since we haven't
	 * called gf_layer_update() yet.  This is exactly what we want,
	 * since we haven't supplied a valid surface to display yet.
	 * Later, the OpenGL ES library calls will call gf_layer_update()
	 * internally, when  displaying the rendered 3D content.
	 */
		const EGLint config32bpp[] =
	{
		EGL_ALPHA_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
	 const EGLint config24bpp[] =
	 {
		 EGL_RED_SIZE, 8,
		 EGL_GREEN_SIZE, 8,
		 EGL_BLUE_SIZE, 8,
		 EGL_NONE
	 };
	 const EGLint config16bpp[] =
	 {
		 EGL_RED_SIZE, 5,
		 EGL_GREEN_SIZE, 6,
		 EGL_BLUE_SIZE, 5,
		 EGL_NONE
	 };
	 const EGLint* configXbpp;
	 EGLint majorVersion, minorVersion;
	 int numConfigs;

	 if( camera->hVidCore->display )
	 {
		 lprintf( "EGL Context already initialized for camera" );
       return;
	 }

	 camera->hVidCore->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

#ifdef __ANDROID__
	 // Window surface that covers the entire screen, from libui.
	 {
		 NativeWindowType (*android_cds)(void) = (NativeWindowType (*)(void))LoadFunction( "libui.so", "android_createDisplaySurface" );
		 if( android_cds )
		 {
			 camera->hVidCore->displayWindow = android_cds();
			 lprintf( "native window %p", camera->hVidCore->displayWindow );
			 lprintf("Window specs: %d*%d format=%d",
						ANativeWindow_getWidth( camera->hVidCore->displayWindow),
						ANativeWindow_getHeight( camera->hVidCore->displayWindow),
						ANativeWindow_getFormat( camera->hVidCore->displayWindow)
					  );
			 switch( ANativeWindow_getFormat( camera->hVidCore->displayWindow) )
			 {
			 case 1:
				 configXbpp = config32bpp;
				 break;
			 case 2:
				 configXbpp = config24bpp;
				 break;
			 case 4:
				 configXbpp = config16bpp;
				 break;
			 }

		 }
		 else
          lprintf( "Fatal error; cannot get android_createDisplaySurface from libui" );
	 }
#endif

	 eglInitialize(camera->hVidCore->display, &majorVersion, &minorVersion);
	 lprintf("GL version: %d.%d",majorVersion,minorVersion);

	 if (!eglChooseConfig(camera->hVidCore->display, configXbpp, &camera->hVidCore->config, 1, &numConfigs))
	 {
		 lprintf("eglChooseConfig failed");
		 if (camera->hVidCore->econtext==0) lprintf("Error code: %x", eglGetError());
	 }

	 camera->hVidCore->econtext = eglCreateContext(camera->hVidCore->display,
																  camera->hVidCore->config,
																  EGL_NO_CONTEXT,
																  NULL);
	 lprintf("GL context: %x", camera->hVidCore->econtext);
	 if (camera->hVidCore->econtext==0) lprintf("Error code: %x", eglGetError());

	 camera->hVidCore->surface = eglCreateWindowSurface(camera->hVidCore->display,
																		 camera->hVidCore->config,
																		 camera->hVidCore->displayWindow,
																		 NULL);
	 lprintf("GL surface: %x", camera->hVidCore->surface);
	 if (camera->hVidCore->surface==0) lprintf("Error code: %x", eglGetError());

	 // makes it go black as soon as ready
	 eglSwapBuffers(camera->hVidCore->display, camera->hVidCore->surface);
}

void EnableEGLContext( PRENDERER hVidCore )
{
	lprintf( "Enable context %p", hVidCore );
	if( hVidCore )
	{
		/* connect the context to the surface */
         lprintf( "make current..." );
		if (eglMakeCurrent(hVidCore->display, hVidCore->surface, hVidCore->surface, hVidCore->econtext)==EGL_FALSE)
		{
			lprintf( "Make current failed: 0x%x\n", eglGetError());
			return;
		}
      lprintf( "made current..." );
	}
	else
	{
		//glFinish(); //swapbuffers will do implied glFlush
		//eglWaitGL(); // same as glFinish();

		// swap should be done at end of render phase.
		//eglSwapBuffers(hVidCore->display,hVidCore->surface);
	}
}

#endif

#ifdef __ANDROID__
// from ndk sample opengl
#include <jni.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an camera->hVidCore->config with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
	 EGLint numConfigs;
	 EGLConfig config;
	 EGLSurface surface;
	 EGLContext context;
    EGLint error;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    lprintf( "display = %p", display );
	 error = eglInitialize(display, 0, 0);
    lprintf( "... %d",error );

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first camera->hVidCore->config that matches our criteria */
    error = eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    lprintf( "... %d",error );

    /* EGL_NATIVE_VISUAL_ID is an attribute of the camera->hVidCore->config that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a camera->hVidCore->config, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    error = eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    lprintf( "... %d",error );

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // Initialize GL state.
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    //glEnable(GL_CULL_FACE);
    //glShadeModel(GL_SMOOTH);
    //glDisable(GL_DEPTH_TEST);

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }

    // Just fill the screen with a color.
    glClearColor(((float)engine->state.x)/engine->width, engine->state.angle,
            ((float)engine->state.y)/engine->height, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void do_android_main( void ) {
    struct engine engine;
	 struct android_app* state;
	 RegisterAndCreateGlobal( (POINTER*)&state, sizeof( struct android_app ), "Android/android_app/main" );

    // Make sure glue isn't stripped.
    app_dummy();

    lprintf( "Begin main? ");
    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
                        LOGI("accelerometer: x=%f y=%f z=%f",
                                event.acceleration.x, event.acceleration.y,
                                event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}
//END_INCLUDE(all)
//------------------------------------------
// sources included from http://jiggawatt.org/badc0de/android/index.html
// backported into qnx egl structures 


#endif

#ifdef __QNX__
//----------------------------------------------------------------------------
//-------------------- QNX specific display code --------------------------
//----------------------------------------------------------------------------
#include <gf/gf.h>
#include <gf/gf3d.h>

void InitQNXDisplays( void )
{
	int n;
	int nDisplay = 1;
	lprintf( "Init QNX DIsplays" );
	for( n = 0; n < 63; n++ )
	{
		int err = gf_dev_attach( &l.qnx_dev[n], GF_DEVICE_INDEX(n), &l.qnx_dev_info[n] );
		if( err == GF_ERR_DEVICE )
		{
			lprintf( "no device at %d", n );
			break;
		}
		if( err != GF_ERR_OK )
		{
			lprintf( "Error attaching device(%d): %d", n, err );
		}
		else
		{
			int m;
			l.qnx_display[n] = NewArray( gf_display_t, l.qnx_dev_info[n].ndisplays );
			l.qnx_display_info[n] = NewArray( gf_display_info_t, l.qnx_dev_info[n].ndisplays );
			for( m = 0; m < l.qnx_dev_info[n].ndisplays; m++ )
			{
				err = gf_display_attach( &l.qnx_display[n][m],
												l.qnx_dev[n],
												m,
												&l.qnx_display_info[n][m] );

				if( err != GF_ERR_OK )
					lprintf( "Error attaching display(%d,%d): %d", n, m, err );
				else
				{
					lprintf( "Display %d=(%d,%d) has %d layers and is %dx%d", nDisplay, n, m
							 , l.qnx_display_info[n][m].nlayers
							 , l.qnx_display_info[n][m].xres
							 , l.qnx_display_info[n][m].yres );
					nDisplay++;
				}
			}
		}
	}
	l.nDevices = n;
	if( n == 0 )
	{
		lprintf( "No Displays available." );
		DebugBreak();
	}
}



void ShutdownQNXDisplays( void )
{
	int n, m;
	for( n= 0; n < l.nDevices; n++ )
	{
		for( m = 0; m < l.qnx_dev_info[n].ndisplays; m++ )
		{
         gf_display_detach( l.qnx_display[n][m] );
		}
		gf_dev_detach( l.qnx_dev[n] );
	}
	l.nDevices = 0;
}

// takes a 1 based display number and results with device and display index.
void ResolveDeviceID( int nDisplay, int *device, int *display )
{
	int n;
	// try and find display number N and return that display size.
	if( nDisplay )
		nDisplay--; // zero bias this.
	for( n = 0; n < l.nDevices; n++ )
	{
		if( nDisplay >= l.qnx_dev_info[n].ndisplays )
		{
			nDisplay -= l.qnx_dev_info[n].ndisplays;
			continue;
		}
		else
		{
			break;
		}
	}
	if( device )
		(*device)=n;
	if( display )
		(*display)=nDisplay;
}

/**************************************************************************************

Utility Functions to implement...


int gf_display_set_mode( gf_display_t display,
                         int xres,
                         int yres,
                         int refresh,
                         gf_format_t format,
                         unsigned flags );

*******************************************************************************************/

void FindPixelFormat( struct display_camera *camera )
{
	int device, display;
	static EGLint attribute_list[]=
	{
	   EGL_NATIVE_VISUAL_ID, 0,
	   EGL_NATIVE_RENDERABLE, EGL_TRUE,
	   EGL_RED_SIZE, 5,
	   EGL_GREEN_SIZE, 5,
	   EGL_BLUE_SIZE, 5,
	   EGL_DEPTH_SIZE, 16,
	   EGL_NONE
	};
	int i;
	ResolveDeviceID( camera->display, &device, &display );
	if (gf_layer_query(camera->hVidCore->pLayer, l.qnx_display_info[device][display].main_layer_index
				, &camera->hVidCore->layer_info) == GF_ERR_OK) 
	{
		lprintf("found a compatible frame "
				"buffer configuration on layer %d\n", l.qnx_display_info[device][display].main_layer_index);
	}    
	for (i = 0; ; i++) 
	{
		/* Walk through all possible pixel formats for this layer */
		if (gf_layer_query(camera->hVidCore->pLayer, i, &camera->hVidCore->layer_info) != GF_ERR_OK) 
		{
			lprintf("Couldn't find a compatible frame "
					"buffer configuration on layer %d\n", i);
			break;
		}    

		/*
		 * We want the color buffer format to match the layer format,
		 * so request the layer format through EGL_NATIVE_VISUAL_ID.
		 */
		attribute_list[1] = camera->hVidCore->layer_info.format;

		/* Look for a compatible EGL frame buffer configuration */
		if (eglChooseConfig(camera->hVidCore->display
			,attribute_list, &camera->hVidCore->config, 1, &camera->hVidCore->num_config) == EGL_TRUE)
		{
			if (camera->hVidCore->num_config > 0) 
			{
				lprintf( "multiple configs? %d", camera->hVidCore->num_config );
				break;
			}
			else
				lprintf( "Config is no good for eglChooseConfig?" );
		}
		else
			lprintf( "Failed to eglChooseConfig? %d", eglGetError() );
	}
}

void CreateQNXOutputForCamera( struct display_camera *camera )
{

	//layer, surface, ...
	// camera has all information about display number, 
	// it has a hvideo structure ready to populate that the 
	// camera owns.  Just create the real drawing surface
	// and keep handles to enable opengl context for later rendering
	int device,display;
	int err;

	ResolveDeviceID( camera->display, &device, &display );
	lprintf( "create output surface for ..." );

	/* get an EGL display connection */
	camera->hVidCore->display=eglGetDisplay((EGLNativeDisplayType)l.qnx_display[device][display]);

	if(camera->hVidCore->display == EGL_NO_DISPLAY)
	{
		lprintf("ERROR: eglGetDisplay()\n");
		return;
	}
	else
	{
		lprintf("SUCCESS: eglGetDisplay()\n");
	}

	
	/* initialize the EGL display connection */
	if(eglInitialize(camera->hVidCore->display, NULL, NULL) != EGL_TRUE)
	{
		lprintf( "ERROR: eglInitialize: error 0x%x\n", eglGetError());
		return;
	}

	else
	{
		lprintf( "SUCCESS: eglInitialize()\n");
	};

	err = gf_layer_attach( &camera->hVidCore->pLayer, l.qnx_display[device][display], 0, GF_LAYER_ATTACH_PASSIVE );
	if( err != GF_ERR_OK )
	{
		lprintf( "Error attaching layer(%d): %d", camera->display, err );
		return;
	}
	else
	{
		gf_layer_enable( camera->hVidCore->pLayer );
		//gf_surface_create_layer( &camera->hVidCore->pSurface
		//						, &camera->hVidCore->pLayer, 1, 0
		//						, camera->w, camera->h
		//						, 
	}

	FindPixelFormat( camera );


	/* create a 3D rendering target */
	// the list of surfacs should be 2 if manually created
	// cann pass null so surfaces are automatically created
	// (should remove necessicty to get pixel mode?
	if ( ( err = gf_3d_target_create(&camera->hVidCore->pTarget, camera->hVidCore->pLayer,
		NULL, 0, camera->w, camera->h, camera->hVidCore->layer_info.format) ) != GF_ERR_OK) 
	{
		lprintf("Unable to create rendering target:%d\n",err );
	}



	/* create an EGL window surface */
	camera->hVidCore->surface = eglCreateWindowSurface(camera->hVidCore->display
		, camera->hVidCore->config, camera->hVidCore->pTarget, NULL);

	if (camera->hVidCore->surface == EGL_NO_SURFACE) 
	{
		lprintf("Create surface failed: 0x%x\n", eglGetError());
		return;
	}

	// icing?
	gf_layer_set_src_viewport(camera->hVidCore->pLayer, 0, 0, camera->w-1, camera->h-1);
	gf_layer_set_dst_viewport(camera->hVidCore->pLayer, 0, 0, camera->w-1, camera->h-1);
   
}



ATEXIT( ExitTest )
{
   ShutdownQNXDisplays();
}

#endif


// forward declaration - staticness will probably cause compiler errors.
static int CPROC ProcessDisplayMessages(void );

//----------------------------------------------------------------------------

void  EnableLoggingOutput( LOGICAL bEnable )
{
   l.flags.bLogWrites = bEnable;
}

void  UpdateDisplayPortionEx( PVIDEO hVideo
                                          , S_32 x, S_32 y
                                          , _32 w, _32 h DBG_PASS)
{

   if( hVideo )
		hVideo->flags.bShown = 1;
	l.flags.bUpdateWanted = 1;
}

//----------------------------------------------------------------------------

void
UnlinkVideo (PVIDEO hVideo)
{
	// yes this logging is correct, to say what I am below, is to know what IS above me
	// and to say what I am above means I nkow what IS below me
//#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( " -- UNLINK Video %p from which is below %p and above %p" ), hVideo, hVideo->pAbove, hVideo->pBelow );
//#endif
	//if( hVideo->pBelow || hVideo->pAbove )
	//   DebugBreak();
	if (hVideo->pBelow)
	{
		hVideo->pBelow->pAbove = hVideo->pAbove;
	}
	else
		l.top = hVideo->pAbove;
	if (hVideo->pAbove)
	{
		hVideo->pAbove->pBelow = hVideo->pBelow;
	}
	else
		l.bottom = hVideo->pBelow;


	hVideo->pPrior = hVideo->pNext = hVideo->pAbove = hVideo->pBelow = NULL;
}

//----------------------------------------------------------------------------

void
FocusInLevel (PVIDEO hVideo)
{
   lprintf( WIDE( "Focus IN level" ) );
   if (hVideo->pPrior)
   {
      hVideo->pPrior->pNext = hVideo->pNext;
      if (hVideo->pNext)
         hVideo->pNext->pPrior = hVideo->pPrior;

      hVideo->pPrior = NULL;

      if (hVideo->pAbove)
      {
         hVideo->pNext = hVideo->pAbove->pBelow;
         hVideo->pAbove->pBelow->pPrior = hVideo;
         hVideo->pAbove->pBelow = hVideo;
      }
      else        // nothing points to this - therefore we must find the start
      {
         PVIDEO pCur = hVideo->pPrior;
         while (pCur->pPrior)
            pCur = pCur->pPrior;
         pCur->pPrior = hVideo;
         hVideo->pNext = pCur;
       }
		hVideo->pPrior = NULL;
   }
   // else we were the first in this level's chain...
}

//----------------------------------------------------------------------------

void  PutDisplayAbove (PVIDEO hVideo, PVIDEO hAbove)
{
	//  this above that...
	//  this->below is now that // points at things below
	// that->above = this // points at things above
#ifdef LOG_ORDERING_REFOCUS
	PVIDEO original_below = hVideo->pBelow;
	PVIDEO original_above = hVideo->pAbove;
#endif
	PVIDEO topmost = hAbove;

	if( !l.bottom )
	{
		if( hAbove )
			lprintf( WIDE( "Failure, no bottom, but somehow a second display is already known?" ) );
		l.bottom = hVideo;
		l.top = hVideo;
		return;
	}

#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "Begin Put Display Above..." ) );
#endif
	if( hVideo->pAbove == hAbove )
		return;
	if( hVideo == hAbove )
		DebugBreak();

	// unlink the video from the stack first.
	if( hVideo->pBelow )
		hVideo->pBelow->pAbove = hVideo->pAbove;
	hVideo->pBelow = NULL;
	if( hVideo->pAbove )
		hVideo->pAbove->pBelow = hVideo->pBelow;
	hVideo->pAbove = NULL;

	// if this is already in a list (like it has pBelow things)
	// I want to insert hAbove between hVideo and pBelow...
	// if if above already has things above it, I want to put those above hvideo
	if( hVideo && hAbove )
	{
		if( hVideo->pBelow = hAbove->pBelow )
		{
			hAbove->pBelow->pAbove = hVideo;
		}
		else
			l.top = hVideo;

		if( hVideo->pAbove )
		{
			lprintf( WIDE( "Windwo was over somethign else and now we die." ) );
			DebugBreak();
		}

		hAbove->pBelow = hVideo;
		hVideo->pAbove = hAbove;

		LeaveCriticalSec( &l.csList );
		return;
	}
	{

		EnterCriticalSec( &l.csList );
		UnlinkVideo (hVideo);      // make sure it's isolated...

		if( ( hVideo->pAbove = topmost ) )
		{
			//HWND hWndOver = GetNextWindow( topmost->hWndOutput, GW_HWNDPREV );
			if( hVideo->pBelow = topmost->pBelow )
			{
				hVideo->pBelow->pAbove = hVideo;
			}
			topmost->pBelow = hVideo;
		}
		LeaveCriticalSec( &l.csList );
	}
#ifdef LOG_ORDERING_REFOCUS
	if( l.flags.bLogFocus )
		lprintf( WIDE( "End Put Display Above..." ) );
#endif
}

void  PutDisplayIn (PVIDEO hVideo, PVIDEO hIn)
{
   lprintf( WIDE( "Relate hVideo as a child of hIn..." ) );
}

//----------------------------------------------------------------------------

LOGICAL CreateDrawingSurface (PVIDEO hVideo)
{
	if (!hVideo)
		return FALSE;

	hVideo->pImage =
		RemakeImage( hVideo->pImage, NULL, hVideo->pWindowPos.cx,
						hVideo->pWindowPos.cy );
	if( !hVideo->transform )
	{
		TEXTCHAR name[64];
		snprintf( name, sizeof( name ), WIDE( "render.display.%p" ), hVideo );
		lprintf( WIDE( "making initial transform" ) );
		hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
	}

	lprintf( WIDE( "Set transform at %d,%d" ), hVideo->pWindowPos.x, hVideo->pWindowPos.y );
	Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );

	// additionally indicate that this is a GL render point
	hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
	return TRUE;
}

void DoDestroy (PVIDEO hVideo)
{
   if (hVideo)
   {
#ifdef _WIN32
      hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
      SetWindowLong (hVideo->hWndOutput, WD_HVIDEO, 0);
#endif
      if (hVideo->pWindowClose)
      {
         hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
#ifdef _WIN32
		ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
#endif
		Release (hVideo->pTitle);
		DestroyKeyBinder( hVideo->KeyDefs );
		// Image library tracks now that someone else gave it memory
		// and it does not deallocate something it didn't allocate...
		UnmakeImageFile (hVideo->pImage);

#ifdef LOG_DESTRUCTION
		lprintf( WIDE( "In DoDestroy, destroyed a good bit already..." ) );
#endif

      // this will be cleared at the next statement....
      // which indicates we will be ready to be released anyhow...
		//hVideo->flags.bReady = FALSE;
      // unlink from the stack of windows...
		UnlinkVideo (hVideo);
		if( l.hCaptured == hVideo )
			l.hCaptured = NULL;
		//Log (WIDE( "Cleared hVideo - is NOW !bReady" ));
		if( !hVideo->flags.event_dispatched )
		{
			int bInDestroy = hVideo->flags.bInDestroy;
			MemSet (hVideo, 0, sizeof (VIDEO));
			// restore this flag... need to keep this so
			// we don't release the structure cuasing a
			// infinite hang while the bit is checked through
			// a released memory pointer.
			hVideo->flags.bInDestroy = bInDestroy;
		}
		else
			hVideo->flags.bReady = 0; // leave as much as we can if in a key...
   }
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------


int IsVidThread( void )
{
   // used by opengl to allow selecting context.
	if( IsThisThread( l.actual_thread ) )
		return TRUE;
	return FALSE;
}

void Redraw( PVIDEO hVideo )
{
	if( hVideo )
		hVideo->flags.bUpdated = 1;
	else
		l.flags.bUpdateWanted = 1;
}


#define MODE_UNKNOWN 0
#define MODE_PERSP 1
#define MODE_ORTHO 2
static int mode = MODE_UNKNOWN;

static void BeginVisPersp( struct display_camera *camera )
{
	//if( mode != MODE_PERSP )
	{
		mode = MODE_PERSP;
		//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		//glLoadIdentity();									// Reset The Projection Matrix
		//gluPerspective(90.0f,camera->aspect,1.0f,30000.0f);
		//glGetFloatv( GL_PROJECTION_MATRIX, l.fProjection );

		//glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	}
}


static int InitGL( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !camera->flags.init )
	{
		//glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

		//glEnable( GL_ALPHA_TEST );
		glEnable( GL_BLEND );
 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glEnable( GL_TEXTURE_2D );
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		//glEnable(GL_NORMALIZE); // glNormal is normalized automatically....
#ifndef __ANDROID__
		//glEnable( GL_POLYGON_SMOOTH );
#endif
 		//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
 
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 
		BeginVisPersp( camera );
		lprintf( WIDE("First GL Init Done.") );
		camera->flags.init = 1;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepthf(1.0f);									// Depth Buffer Setup
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}


static void InvokeExtraInit( struct display_camera *camera, PTRANSFORM view_camera )
{
	PTRSZVAL (CPROC *Init3d)(PTRANSFORM,RCOORD*,RCOORD*);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( WIDE("sack/render/puregl/init3d"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		Init3d = GetRegisteredProcedureExx( data,(CTEXTSTR)name,PTRSZVAL,WIDE("ExtraInit3d"),(PTRANSFORM,RCOORD*,RCOORD*));

		if( Init3d )
		{
			struct plugin_reference *reference;
			PTRSZVAL psvInit = Init3d( view_camera, &camera->identity_depth, &camera->aspect );
			if( psvInit )
			{
				reference = New( struct plugin_reference );
				reference->psv = psvInit;
				reference->name = name;
				{
					static PCLASSROOT draw3d;
					if( !draw3d )
						draw3d = GetClassRoot( WIDE("sack/render/puregl/draw3d") );
					reference->Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraDraw3d"),(PTRSZVAL));
					reference->FirstDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("FirstDraw3d"),(PTRSZVAL));
					reference->ExtraDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraBeginDraw3d"),(PTRSZVAL,PTRANSFORM));
					reference->Mouse3d = GetRegisteredProcedureExx( GetClassRoot( WIDE("sack/render/puregl/mouse3d") ),(CTEXTSTR)name,LOGICAL,WIDE("ExtraMouse3d"),(PTRSZVAL,PRAY,_32));
				}
				AddLink( &camera->plugins, reference );
			}
		}
	}

}

static void InvokeExtraBeginDraw( CTEXTSTR name, PTRSZVAL psvInit )
{
	static PCLASSROOT draw3d;
	void (CPROC *Draw3d)(PTRSZVAL);
	if( !draw3d )
		draw3d = GetClassRoot( WIDE("sack/render/puregl/draw3d") );
	Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraBeginDraw3d"),(PTRSZVAL));
	if( Draw3d )
	{
		Draw3d( psvInit );
	}
}

static void InvokeExtraDraw( CTEXTSTR name, PTRSZVAL psvInit )
{
	static PCLASSROOT draw3d;
	void (CPROC *Draw3d)(PTRSZVAL);
	if( !draw3d )
		draw3d = GetClassRoot( WIDE("sack/render/puregl/draw3d") );
	Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraDraw3d"),(PTRSZVAL));
	if( Draw3d )
	{
		Draw3d( psvInit );
	}
}

static LOGICAL InvokeExtraMouse( CTEXTSTR name, PTRSZVAL psvInit, PRAY mouse_ray, _32 b )
{
	static PCLASSROOT mouse3d;
	LOGICAL (CPROC *Mouse3d)(PTRSZVAL,PRAY,_32);
	LOGICAL used;
	if( !mouse3d )
		mouse3d = GetClassRoot( WIDE("sack/render/puregl/mouse3d") );
	Mouse3d = GetRegisteredProcedureExx( mouse3d, (CTEXTSTR)name,LOGICAL,WIDE("ExtraMouse3d" ),(PTRSZVAL,PRAY,_32));
	if( Mouse3d )
	{
		used = Mouse3d( psvInit, mouse_ray, b );
		if( used )
			return used;
	}
   return FALSE;
}

#ifndef NO_TOUCH
static int CPROC Handle3DTouches( PRENDERER hVideo, PTOUCHINPUT touches, int nTouches )
{
	static struct touch_event_state
	{
		struct {
			struct {
				BIT_FIELD bDrag : 1;
			} flags;
			int x;
         int y;
		} one;
		struct {
			int x;
         int y;
		} two;
	} touch_info;
	if( l.flags.bRotateLock )
	{
		int t;
		for( t = 0; t < nTouches; t++ )
		{
			lprintf( WIDE( "%d %5d %5d " ), t, touches[t].x, touches[t].y );
		}
		lprintf( WIDE( "touch event" ) );
		if( nTouches == 2 )
		{
         // begin rotate lock
			if( touches[1].dwFlags & TOUCHEVENTF_DOWN )
			{
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
			else if( touches[1].dwFlags & TOUCHEVENTF_UP )
			{
			}
			else
			{
				// drag
				int delx, dely;
				int delx2, dely2;
				int delxt, delyt;
				int delx2t, dely2t;
				lprintf( WIDE("drag") );
            delxt = touches[1].x - touches[0].x;
				delyt = touches[1].y - touches[0].y;
            delx2t = touch_info.two.x - touch_info.one.x;
            dely2t = touch_info.two.y - touch_info.one.y;
				delx = -touch_info.one.x + touches[0].x;
            dely = -touch_info.one.y + touches[0].y;
				delx2 = -touch_info.two.x + touches[1].x;
            dely2 = -touch_info.two.y + touches[1].y;
				{
               VECTOR v1,v2/*,vr*/;
					RCOORD delta_x = delx / 40.0;
					RCOORD delta_y = dely / 40.0;
					static int toggle;
					v1[vUp] = delyt;
					v1[vRight] = delxt;
               v1[vForward] = 0;
					v2[vUp] = dely2t;
					v2[vRight] = delx2t;
					v2[vForward] = 0;
               normalize( v1 );
               normalize( v2 );
					lprintf( WIDE("angle %g"), atan2( v2[vUp], v2[vRight] ) - atan2( v1[vUp], v1[vRight] ) );
					RotateRel( l.origin, 0, 0, - atan2( v2[vUp], v2[vRight] ) + atan2( v1[vUp], v1[vRight] ) );
					delta_x /= hVideo->pWindowPos.cx;
					delta_y /= hVideo->pWindowPos.cy;
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
				}

            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
            touch_info.two.x = touches[1].x;
            touch_info.two.y = touches[1].y;
			}
		}
		else if( nTouches == 1 )
		{
			if( touches[0].dwFlags & TOUCHEVENTF_DOWN )
			{
            lprintf( WIDE("begin") );
				// begin touch
            touch_info.one.x = touches[0].x;
            touch_info.one.y = touches[0].y;
			}
			else if( touches[0].dwFlags & TOUCHEVENTF_UP )
			{
				// release
            lprintf( WIDE("done") );
			}
			else
			{
				// drag
				int delx, dely;
				lprintf( WIDE("drag") );
				delx = -touch_info.one.x + touches[0].x;
            dely = -touch_info.one.y + touches[0].y;
				{
					RCOORD delta_x = -delx / 40.0;
					RCOORD delta_y = -dely / 40.0;
					static int toggle;
					delta_x /= hVideo->pWindowPos.cx;
					delta_y /= hVideo->pWindowPos.cy;
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
				}


				touch_info.one.x = touches[0].x;
				touch_info.one.y = touches[0].y;
			}
		}
		return 1;
	}
	return 0;
}
#endif

static void WantRenderGL( void )
{
	INDEX idx;
	PRENDERER hVideo;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Render" );

	{
		PRENDERER other = NULL;
		PRENDERER hVideo;
		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( other == hVideo )
				DebugBreak();
			other = hVideo;
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("Have a video in stack...") );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( WIDE("But it's nto exposed...") );
				continue;
			}
			if( hVideo->flags.bUpdated )
			{
				// any one window with an update draws all.
				l.flags.bUpdateWanted = 1;
				break;
			}
		}
	}
}
static void RenderGL( struct display_camera *camera )
{
	INDEX idx;
	PRENDERER hVideo;
	struct plugin_reference *reference;
	int first_draw;
	if( l.flags.bLogRenderTiming )
		lprintf( "Begin Render" );

	if( !camera->flags.did_first_draw )
	{
		first_draw = 1;
		camera->flags.did_first_draw = 1;
	}
	else
		first_draw = 0;


	// do OpenGL Frame
#ifdef USE_EGL
	lprintf( "enable context" );
	EnableEGLContext( camera->hVidCore );
#else
	//SetActiveEGLDisplay( camera->hVidCore );
#endif

	InitGL( camera );
   lprintf( "Called init for camera.." );
	{
		PRENDERER hVideo = camera->hVidCore;

		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			// setup initial state, like every time so it's a known state?
			{
				// copy l.origin to the camera
				ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );

				if( first_draw )
				{
					if( reference->FirstDraw3d )
						reference->FirstDraw3d( reference->psv );
				}
				if( reference->ExtraDraw3d )
					reference->ExtraDraw3d( reference->psv, camera->origin_camera );
			}
		}

		switch( camera->type )
		{
		case 0:
			RotateRight( camera->origin_camera, vRight, vForward );
			break;
		case 1:
			RotateRight( camera->origin_camera, vForward, vUp );
			break;
		case 2:
			break;
		case 3:
			RotateRight( camera->origin_camera, vUp, vForward );
			break;
		case 4:
			RotateRight( camera->origin_camera, vForward, vRight );
			break;
		case 5:
			RotateRight( camera->origin_camera, -1, -1 );
			break;
		}

		//GetGLMatrix( camera->origin_camera, camera->hVidCore->fModelView );
		//glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				InvokeExtraBeginDraw( ref->name, ref->psv );
			}
		}

		for( hVideo = l.bottom; hVideo; hVideo = hVideo->pBelow )
		{
			if( l.flags.bLogMessageDispatch )
				lprintf( WIDE("Have a video in stack...") );
			if( hVideo->flags.bDestroy )
				continue;
			if( hVideo->flags.bHidden || !hVideo->flags.bShown )
			{
				if( l.flags.bLogMessageDispatch )
					lprintf( WIDE("But it's nto exposed...") );
				continue;
			}

			hVideo->flags.bUpdated = 0;

			if( l.flags.bLogWrites )
				lprintf( WIDE("------ BEGIN A REAL DRAW -----------") );

			glEnable( GL_DEPTH_TEST );
      lprintf( "..." );
			// put out a black rectangle
			// should clear stensil buffer here so we can do remaining drawing only on polygon that's visible.
			ClearImageTo( hVideo->pImage, 0 );
      lprintf( "..." );
			glDisable(GL_DEPTH_TEST);							// Enables Depth Testing

			if( hVideo->pRedrawCallback )
			{
				hVideo->pRedrawCallback( hVideo->dwRedrawData, (PRENDERER)hVideo );
			}

			// allow draw3d code to assume depth testing 
			glEnable( GL_DEPTH_TEST );
		}

		{
#if 0
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, camera->mouse_ray.o, camera->mouse_ray.n, 1.0 );
				SetPoint( origin, camera->mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,0,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,255,0,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
			// render a ray that we use for mouse..
			{
				VECTOR target;
				VECTOR origin;
				addscaled( target, l.mouse_ray.o, l.mouse_ray.n, 1.0 );
				SetPoint( origin, l.mouse_ray.o );
				origin[0] += 0.001;
				origin[1] += 0.001;
				//mouse_ray_origin[2] += 0.01;
				glBegin( GL_LINES );
				glColor4ub( 255,255,255,128 );
				glVertex3dv(origin);	// Bottom Left Of The Texture and Quad
				glColor4ub( 255,56,255,128 );
 				glVertex3dv(target);	// Bottom Left Of The Texture and Quad
				glEnd();
			}
#endif
		}

		LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
		{
			// setup initial state, like every time so it's a known state?
			{
				// copy l.origin to the camera
				ApplyTranslationT( VectorConst_I, camera->origin_camera, l.origin );

				if( first_draw )
				{
					if( reference->FirstDraw3d )
						reference->FirstDraw3d( reference->psv );
				}
				if( reference->ExtraDraw3d )
					reference->ExtraDraw3d( reference->psv, camera->origin_camera );
			}
		}

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				InvokeExtraDraw( ref->name, ref->psv );
			}
		}
	}
	// using eglLayer, doesn't really have to be cleared
	// each beginning flushes a previous... oh (well at end of all should call a clear)
	//SetActiveEGLDisplay( NULL );
}



//----------------------------------------------------------------------------

static void LoadOptions( void )
{
	_32 average_width, average_height;
	//int some_width;
	//int some_height;
#ifndef __NO_OPTIONS__
	l.flags.bView360 = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/360 view"), 0, TRUE );
	if( !l.cameras )
	{
		struct display_camera *default_camera = NULL;
		_32 screen_w, screen_h;
		int nDisplays = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Number of Displays"), l.flags.bView360?6:1, TRUE );
		int n;
		l.flags.bForceUnaryAspect = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Force Aspect 1.0"), (nDisplays==1)?0:1, TRUE );
		GetDisplaySizeEx( 0, NULL, NULL, &screen_w, &screen_h );
		switch( nDisplays )
		{
		default:
		case 0:
			average_width = screen_w;
			average_height = screen_h;
			break;
		case 6:
			average_width = screen_w/4;
			average_height = screen_h/3;
			break;
		}
		SetLink( &l.cameras, 0, (POINTER)1 ); // set default here 
		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			camera->origin_camera = CreateTransform();

#ifdef __QNX__
			custom_pos = 0;
#else
			snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Custom Position"), n+1 );
			custom_pos = SACK_GetProfileIntEx( GetProgramName(), tmp, l.flags.bView360?1:0, TRUE );
#endif
			if( custom_pos )
			{
				camera->display = -1;
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/x"), n+1 );
				camera->x = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?400:n==2?0:n==3?400:0
					:nDisplays==6
						?n==0?((screen_w * 0)/4):n==1?((screen_w * 1)/4):n==2?((screen_w * 1)/4):n==3?((screen_w * 1)/4):n==4?((screen_w * 2)/4):n==5?((screen_w * 3)/4):0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/y"), n+1 );
				camera->y = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?0:n==2?300:n==3?300:0
					:nDisplays==6
						?n==0?((screen_h * 1)/3):n==1?((screen_h * 0)/3):n==2?((screen_h * 1)/3):n==3?((screen_h * 2)/3):n==4?((screen_h * 1)/3):n==5?((screen_h * 1)/3):0
					:nDisplays==1
						?0
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/width"), n+1 );
				camera->w = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?400
					:nDisplays==6
						?( screen_w / 4 )
					:nDisplays==1
						?800
					:0), TRUE );
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/height"), n+1 );
				camera->h = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?300
					:nDisplays==6
						?( screen_h / 3 )
					:nDisplays==1
						?600
					:0), TRUE );
				/*
				snprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/Direction", n+1 );
				camera->direction = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?1:n==2?2:n==3?3:0
					:nDisplays==6
						?n // this is natural 0=left, 2=forward, 1=up, 3=down, 4=right, 5=back
					:nDisplays==1
						?0
					:0), TRUE );
					*/
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Display"), n+1 );
				camera->display = SACK_GetProfileIntEx( GetProgramName(), tmp, nDisplays>1?n+1:0, TRUE );
				GetDisplaySizeEx( camera->display, &camera->x, &camera->y, &camera->w, &camera->h );
			}

			if( l.flags.bForceUnaryAspect )
				camera->aspect = 1.0;
			else
			{
				camera->aspect = ((float)camera->w/(float)camera->h);
			}

			snprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Camera Type"), n+1 );
			camera->type = SACK_GetProfileIntEx( GetProgramName(), tmp, (nDisplays==6)?n:2, TRUE );
			if( camera->type == 2 && !default_camera )
			{
				default_camera = camera;
			}
			AddLink( &l.cameras, camera );
		}
		if( !default_camera )
			default_camera = (struct display_camera *)GetLink( &l.cameras, 1 );
		SetLink( &l.cameras, 0, default_camera );
	}
	l.flags.bLogMessageDispatch = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/log message dispatch"), 0, TRUE );
	l.flags.bLogFocus = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/log focus event"), 0, TRUE );
	l.flags.bLogKeyEvent = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/log key event"), 0, TRUE );
	l.flags.bLogMouseEvent = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/log mouse event"), 0, TRUE );
	l.flags.bOptimizeHide = SACK_GetProfileIntEx( WIDE("SACK"), WIDE("Video Render/Optimize Hide with SWP_NOCOPYBITS"), 0, TRUE );
	l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogWrites = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Log Video Output"), 0, TRUE );
	l.flags.bUseLLKeyhook = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Use Low Level Keyhook"), 0, TRUE );
#else
#  ifndef UNDER_CE
#  endif
#endif
	if( !l.origin )
	{
		static MATRIX m;
		l.origin = CreateNamedTransform( WIDE("render.camera") );

		Translate( l.origin, average_width/2, average_height/2, average_height/2 );
		RotateAbs( l.origin, M_PI, 0, 0 );

		CreateTransformMotion( l.origin ); // some things like rotate rel
	}
}


// intersection of lines - assuming lines are 
// relative on the same plane....

//int FindIntersectionTime( RCOORD *pT1, LINESEG pL1, RCOORD *pT2, PLINE pL2 )

int FindIntersectionTime( RCOORD *pT1, PVECTOR s1, PVECTOR o1
                        , RCOORD *pT2, PVECTOR s2, PVECTOR o2 )
{
   VECTOR R1, R2, denoms;
   RCOORD t1, t2, denom;

#define a (o1[0])
#define b (o1[1])
#define c (o1[2])

#define d (o2[0])
#define e (o2[1])
#define f (o2[2])

#define na (s1[0])
#define nb (s1[1])
#define nc (s1[2])

#define nd (s2[0])
#define ne (s2[1])
#define nf (s2[2])

   crossproduct(denoms, s1, s2 ); // - result...
   denom = denoms[2];
//   denom = ( nd * nb ) - ( ne * na );
   if( NearZero( denom ) )
   {
      denom = denoms[1];
//      denom = ( nd * nc ) - (nf * na );
      if( NearZero( denom ) )
      {
         denom = denoms[0];
//         denom = ( ne * nc ) - ( nb * nf );
         if( NearZero( denom ) )
         {
#ifdef FULL_DEBUG
            lprintf("Bad!-------------------------------------------\n");
#endif
            return FALSE;
         }
         else
         {
            DebugBreak();
            t1 = ( ne * ( c - f ) + nf * ( b - e ) ) / denom;
            t2 = ( nb * ( c - f ) + nc * ( b - e ) ) / denom;
         }
      }
      else
      {
         DebugBreak();
         t1 = ( nd * ( c - f ) + nf * ( d - a ) ) / denom;
         t2 = ( na * ( c - f ) + nc * ( d - a ) ) / denom;
      }
   }
   else
   {
      // this one has been tested.......
      t1 = ( nd * ( b - e ) + ne * ( d - a ) ) / denom;
      t2 = ( na * ( b - e ) + nb * ( d - a ) ) / denom;
   }

   R1[0] = a + na * t1;
   R1[1] = b + nb * t1;
   R1[2] = c + nc * t1;

   R2[0] = d + nd * t2;
   R2[1] = e + ne * t2;
   R2[2] = f + nf * t2;

   if( ( !COMPARE(R1[0],R2[0]) ) ||
       ( !COMPARE(R1[1],R2[1]) ) ||
       ( !COMPARE(R1[2],R2[2]) ) )
   {
      return FALSE;
   }
   *pT2 = t2;
   *pT1 = t1;
   return TRUE;
#undef a
#undef b
#undef c
#undef d
#undef e
#undef f
#undef na
#undef nb
#undef nc
#undef nd
#undef ne
#undef nf
}


int Parallel( PVECTOR pv1, PVECTOR pv2 )
{
   RCOORD a,b,c,cosTheta; // time of intersection

   // intersect a line with a plane.

//   v <DOT> w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v <DOT> w)/(|v| |w|) = cos <theta>     

   a = dotproduct( pv1, pv2 );

   if( a < 0.0001 &&
       a > -0.0001 )  // near zero is sufficient...
	{
#ifdef DEBUG_PLANE_INTERSECTION
		Log( "Planes are not parallel" );
#endif
      return FALSE; // not parallel..
   }

   b = Length( pv1 );
   c = Length( pv2 );

   if( !b || !c )
      return TRUE;  // parallel ..... assumption...

   cosTheta = a / ( b * c );
#ifdef FULL_DEBUG
   lprintf( " a: %g b: %g c: %g cos: %g \n", a, b, c, cosTheta );
#endif
   if( cosTheta > 0.99999 ||
       cosTheta < -0.999999 ) // not near 0degrees or 180degrees (aligned or opposed)
   {
      return TRUE;  // near 1 is 0 or 180... so IS parallel...
   }
   return FALSE;
}

// slope and origin of line, 
// normal of plane, origin of plane, result time from origin along slope...
RCOORD IntersectLineWithPlane( PCVECTOR Slope, PCVECTOR Origin,  // line m, b
                            PCVECTOR n, PCVECTOR o,  // plane n, o
										RCOORD *time DBG_PASS )
#define IntersectLineWithPlane( s,o,n,o2,t ) IntersectLineWithPlane(s,o,n,o2,t DBG_SRC )
{
   RCOORD a,b,c,cosPhi, t; // time of intersection

   // intersect a line with a plane.

//   v <DOT> w = (1/2)(|v + w|2 - |v|2 - |w|2) 
//  (v <DOT> w)/(|v| |w|) = cos <theta>     

	//cosPhi = CosAngle( Slope, n );

   a = ( Slope[0] * n[0] +
         Slope[1] * n[1] +
         Slope[2] * n[2] );

   if( !a )
	{
		//Log1( DBG_FILELINEFMT "Bad choice - slope vs normal is 0" DBG_RELAY, 0 );
		//PrintVector( Slope );
      //PrintVector( n );
      return FALSE;
   }

   b = Length( Slope );
   c = Length( n );
	if( !b || !c )
	{
      Log( WIDE("Slope and or n are near 0") );
		return FALSE; // bad vector choice - if near zero length...
	}

   cosPhi = a / ( b * c );

   t = ( n[0] * ( o[0] - Origin[0] ) +
         n[1] * ( o[1] - Origin[1] ) +
         n[2] * ( o[2] - Origin[2] ) ) / a;

//   lprintf( " a: %g b: %g c: %g t: %g cos: %g pldF: %g pldT: %g \n", a, b, c, t, cosTheta,
//                  pl->dFrom, pl->dTo );

//   if( cosTheta > e1 ) //global epsilon... probably something custom

//#define 

   if( cosPhi > 0 ||
       cosPhi < 0 ) // at least some degree of insident angle
	{
		*time = t;
		return cosPhi;
	}
	else
	{
		Log1( WIDE("Parallel... %g\n"), cosPhi );
		PrintVector( Slope );
		PrintVector( n );
      // plane and line are parallel if slope and normal are perpendicular
//      lprintf("Parallel...\n");
		return 0;
	}
}

void UpdateMouseRay( struct display_camera * camera )
{
#define BEGIN_SCALE 1
#define COMMON_SCALE ( 2*camera->aspect)
#define END_SCALE 1000
#define tmp_param1 (END_SCALE*COMMON_SCALE)
	if( camera->origin_camera )
	{
		VECTOR tmp1;
		PTRANSFORM t = camera->origin_camera;

		addscaled( l.mouse_ray_origin, _0, _Z, BEGIN_SCALE );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _X, ((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )*COMMON_SCALE/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_origin, l.mouse_ray_origin, _Y, -((float)l.mouse_y-((float)camera->viewport[3]/2.0f) )*(COMMON_SCALE/camera->aspect)/(float)camera->viewport[3] );

		addscaled( l.mouse_ray_target, _0, _Z, END_SCALE );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _X, tmp_param1*((float)l.mouse_x-((float)camera->viewport[2]/2.0f) )/(float)camera->viewport[2] );
		addscaled( l.mouse_ray_target, l.mouse_ray_target, _Y, -(tmp_param1/camera->aspect)*((float)l.mouse_y-((float)camera->viewport[3]/2.0f))/(float)camera->viewport[3] );

		// this is definaly the correct rotation
		Apply( t, tmp1, l.mouse_ray_origin );
      SetPoint( l.mouse_ray_origin, tmp1 );
		Apply( t, tmp1, l.mouse_ray_target );
		SetPoint( l.mouse_ray_target, tmp1 );

		sub( l.mouse_ray_slope, l.mouse_ray_target, l.mouse_ray_origin );
		
		SetPoint( camera->mouse_ray.n, l.mouse_ray_slope );
		SetPoint( camera->mouse_ray.o, l.mouse_ray_origin );
		SetRay( &l.mouse_ray, &camera->mouse_ray );
	}
}

void UpdateMouseRays( void )
{
	struct display_camera *camera;
	INDEX idx;
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		UpdateMouseRay( camera );
	}
}

// h
static int CPROC InverseOpenGLMouse( struct display_camera *camera, PRENDERER hVideo, RCOORD x, RCOORD y, int *result_x, int *result_y )
{
	if( camera->origin_camera )
	{
		VECTOR v1, v2;
		int v = 0;

		v2[0] = x;
		v2[1] = y;
		v2[2] = 1.0;
		//ApplyInverse( l.origin, v
		if( hVideo )
			Apply( hVideo->transform, v1, v2 );
		else
			SetPoint( v1, v2 );
		ApplyInverse( camera->origin_camera, v2, v1 );
		ApplyInverse( l.origin, v1, v2 );

		//lprintf( "%g,%g,%g  from %g,%g,%g ", v1[0], v1[1], v1[2], v2[0], v2[1] , v2[2] );

		// so this puts the point back in world space
		{
			RCOORD t;
			RCOORD cosphi;
			VECTOR v4;
			SetPoint( v4, _0 );
			v4[2] = 1.0;

			cosphi = IntersectLineWithPlane( v2, _0, _Z, v4, &t );
			//lprintf( "t is %g  cosph = %g", t, cosphi );
			if( cosphi != 0 )
				addscaled( v2, _0, v1, t );

			//lprintf( "%g,%g,%g  ", v1[0], v1[1],v1[2] );

			//surface_x * l.viewport[2] +l.viewport[2] =   ((float)l.mouse_x-((float)l.viewport[2]/2.0f) )*0.25f/(float)l.viewport[2]
		}

		//lprintf( "%g,%g became like %g,%g,%g or %g,%g", x, y
   		//		 , v1[0], v1[1], v1[2]
		//		 , (v1[0]/2.5 * l.viewport[2]) + (l.viewport[2]/2)
		//		 , (l.viewport[3]/2) - (v1[1]/(2.5/l.aspect) * l.viewport[3])
		//   	 );
		if( result_x )
			(*result_x) = (int)((v2[0]/COMMON_SCALE * camera->viewport[2]) + (camera->viewport[2]/2));
		if( result_y )
			(*result_y) = (camera->viewport[3]/2) - (v2[1]/(COMMON_SCALE/camera->aspect) * camera->viewport[3]);
	}
	return 1;
}



static int CPROC OpenGLMouse( PTRSZVAL psvMouse, S_32 x, S_32 y, _32 b )
{
	int used = 0;
	PRENDERER check;
	struct display_camera *camera = (struct display_camera *)psvMouse;
	if( camera->origin_camera )
	{
      UpdateMouseRay( camera );

		{
			INDEX idx;
			struct plugin_reference *ref;
			LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
			{
				used = InvokeExtraMouse( ref->name, ref->psv, &camera->mouse_ray, b );
				if( used )
					break;
			}
		}

		if( !used )
		for( check = l.top; check ;check = check->pAbove)
		{
			VECTOR target_point;
			if( l.hCaptured )
				if( check != l.hCaptured )
					continue;
			if( check->flags.bHidden || (!check->flags.bShown) )
				continue;
			{
				RCOORD t;
				RCOORD cosphi;

				//PrintVector( GetOrigin( check->transform ) );
				cosphi = IntersectLineWithPlane( camera->mouse_ray.n, camera->mouse_ray.o
												, GetAxis( check->transform, vForward )
												, GetOrigin( check->transform ), &t );
				if( cosphi != 0 )
					addscaled( target_point, l.mouse_ray_origin, l.mouse_ray_slope, t );
				//PrintVector( target_point );
			}
			// okay so here's the theory now
			//   there's an origin where the camera is, I know where that is
			//   I don't nkow how the model projection is used...
			//   can reverse enginner it I guess... but then we need to break it into
			//    resolution spots,
			{
				VECTOR target_surface_point;
				int newx;
				int newy;

				l.real_mouse_x = target_point[0];
				l.real_mouse_y = target_point[1];

				ApplyInverse( check->transform, target_surface_point, target_point );
				//PrintVector( target_surface_point );
				newx = (int)target_surface_point[0];
				newy = (int)target_surface_point[1];
				//lprintf( "Is %d,%d in %d,%d %dx%d"
				 //  	 ,newx, newy
				 //  	 ,check->pWindowPos.x, check->pWindowPos.y
				 //  	 , check->pWindowPos.x+ check->pWindowPos.cx
				 //  	 , check->pWindowPos.y+ check->pWindowPos.cy );

				if( check == l.hCaptured ||
					( ( newx >= 0 && newx < (check->pWindowPos.cx ) )
					 && ( newy >= 0 && newy < (check->pWindowPos.cy ) ) ) )
				{
					if( check && check->pMouseCallback)
					{
						if( l.flags.bLogMouseEvent )
							lprintf( WIDE("Sent Mouse Proper. %d,%d %08x"), newx, newy, l.mouse_b );
						//InverseOpenGLMouse( camera, check, newx, newy, NULL, NULL );
						l.current_mouse_event_camera = camera;
						used = check->pMouseCallback( check->dwMouseData
													, newx
													, newy
													, l.mouse_b );
						l.current_mouse_event_camera = NULL;
						if( used )
							break;
					}
				}
			}
		}
	}
	return used;
}

//static void HandleMessage (MSG Msg);

#ifdef UNDER_CE
#define WINDOW_STYLE 0
#else
#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW)
#endif

// returns the forward view camera (or default camera)
static struct display_camera *OpenCameras( void )
{
	struct display_camera *camera;
	INDEX idx;
	//lprintf( WIDE( "-----Create WIndow Stuff----- %s %s" ), hVideo->flags.bLayeredWindow?WIDE( "layered" ):WIDE( "solid" )
	//		 , hVideo->flags.bChildWindow?WIDE( "Child(tool)" ):WIDE( "user-selectable" ) );
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		if( camera->hVidCore )
		{
         lprintf( "Camera is already open, skipping..." );
			continue;
		}
		//if( !camera->hWndInstance )
		{
			int UseCoords = camera->display == -1;
			int DefaultScreen = camera->display;
			S_32 x, y;
			_32 w, h;

			// if we don't do it this way, size_t overflows in camera definition.
			if( !UseCoords && !l.flags.bView360 )
			{
				GetDisplaySizeEx( DefaultScreen, &camera->x, &camera->y, &w, &h );
				camera->w = w;
				camera->h = h;
			}
			else
			{
				w = (_32)camera->w;
				h = (_32)camera->h;
			}


			x = camera->x;
			y = camera->y;


	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.. %d,%d %dx%d" ),x,y,w,h );
	#endif
			camera->hVidCore = New( VIDEO );
			MemSet (camera->hVidCore, 0, sizeof (VIDEO));
			InitializeCriticalSec( &camera->hVidCore->cs );
			camera->hVidCore->camera = camera;
			camera->hVidCore->pMouseCallback = OpenGLMouse;
			camera->hVidCore->dwMouseData = (PTRSZVAL)camera;
			camera->viewport[0] = x;
			camera->viewport[1] = y;
			camera->viewport[2] = (int)w;
			camera->viewport[3] = (int)h;

			/* CreateWindowEx */
#ifdef __QNX__
			CreateQNXOutputForCamera( camera );
#endif
#ifdef USE_EGL
			OpenEGL( camera );
#endif

	#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created Real window...Stuff.." ) );
	#endif
 			//camera->hVidCore->hWndOutput = (HWND)camera->hWndInstance;
			//if (!camera->hWndInstance)
			{
			//	return FALSE;
			}
			camera->flags.extra_init = 1;
			camera->identity_depth = camera->w/2;

			InvokeExtraInit( camera, camera->origin_camera );

			camera->hVidCore->flags.bReady = TRUE;
		}
	}


	return (struct display_camera *)GetLink( &l.cameras, 0 );
}

LOGICAL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
#ifndef __NO_WIN32API__
	struct display_camera *camera;

	lprintf(WIDE( "Creating container window named: %s" ),
			(l.gpTitle && l.gpTitle[0]) ? l.gpTitle : (hVideo&&hVideo->pTitle)?hVideo->pTitle:WIDE("No Name"));

	camera = (struct display_camera *)GetLink( &l.cameras, 0 ); // returns the forward(default) camera

		if( hVideo )
		{
			if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
			{
				wx = camera->viewport[2] * 7 / 10;
				wy = camera->viewport[3] * 7 / 10;
			}
			if( x == CW_USEDEFAULT )
				x = 10;
			if( y == CW_USEDEFAULT )
				y = 10;
			//if( hVideo->hWndContainer )
			{
				//RECT r;
				//GetClientRect( hVideo->hWndContainer, &r );
				x = 0;
				y = 0;
				//wx = r.right-r.left + 1;
				//wy = r.top-r.bottom + 1;
			}
			//if( !hVideo->hWndOutput )
			{
				// hWndOutput is set within the create window proc...
		#ifdef LOG_OPEN_TIMING
				lprintf( WIDE( "Create Real Window (In CreateWindowStuff).." ) );
		#endif

				//hVideo->hWndOutput = (HWND)1;
				hVideo->pWindowPos.x = x;
				hVideo->pWindowPos.y = y;
				hVideo->pWindowPos.cx = wx;
				hVideo->pWindowPos.cy = wy;
				lprintf( WIDE("%d %d"), x, y );
				CreateDrawingSurface (hVideo);

				hVideo->flags.bReady = 1;
				WakeThreadID( hVideo->thread );
			  //CreateWindowEx used to be here
			}
			//else
			{

			}


		#ifdef LOG_OPEN_TIMING
			lprintf( WIDE( "Created window stuff..." ) );
		#endif
			// generate an event to dispatch pending...
			// there is a good chance that a window event caused a window
			// and it will be sleeping until the next event...
		#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
			SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
		#endif
			//Log (WIDE( "Created window in module..." ));

		}

	return TRUE;
#else
	// need a .NET window here...
	return FALSE;
#endif
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

void HandleDestroyMessage( PVIDEO hVidDestroy )
{
	{
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("To destroy! %p %d"), 0 /*Msg.lParam*/, hVidDestroy->hWndOutput );
#endif
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("------------ DESTROY! -----------") );
#endif
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("From destroy") );
#endif
	}
}
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
int internal;
static int CPROC ProcessDisplayMessages(void )
{
	return -1;
}

//----------------------------------------------------------------------------
PTRSZVAL CPROC VideoThreadProc (PTHREAD thread)
{
#ifdef LOG_STARTUP
	Log( WIDE("Video thread...") );
#endif
   if (l.bThreadRunning)
   {
		Log( WIDE("Thread Already exists - leaving.") );
		return 0;
	}

#ifdef LOG_STARTUP
	lprintf( WIDE( "Video Thread Proc %x, adding hook and thread." ), GetCurrentThreadId() );
#endif
	{
      // creat the thread's message queue so that when we set
      // dwthread, it's going to be a valid target for
      // setwindowshookex
		//MSG msg;
#ifdef LOG_STARTUP
		Log( WIDE("reading a message to create a message queue") );
#endif
		//PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	}
   l.actual_thread = thread;
	l.dwThreadID = GetCurrentThreadId ();
	//l.pid = (_32)l.dwThreadID;


	l.bThreadRunning = TRUE;

	AddIdleProc( (int(CPROC*)(PTRSZVAL))ProcessDisplayMessages, 0 );

	while( !IsRootDeadstartComplete() )
	{
      lprintf( "Wait for deadstart to complete..." );
		WakeableSleep( 10 );
	}

   // have to wait for inits to be regsitered.
	OpenCameras(); // returns the forward camera
	//AddIdleProc ( ProcessClientMessages, 0);
#ifdef LOG_STARTUP
	Log( WIDE("Registered Idle, and starting message loop") );
#endif
	{
		//MSG Msg;
      // Message Loop
		while( 1 )
		{
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
					RenderGL( camera );
#ifdef USE_EGL
					eglSwapBuffers( camera->hVidCore->display, camera->hVidCore->surface );
#endif
				}
			}
			WakeableSleep( 250 );
		}
	}
	l.bThreadRunning = FALSE;
	lprintf( WIDE( "Video Exited volentarily" ) );
	//ExitThread( 0 );
	return 0;
}

//----------------------------------------------------------------------------

int  InitDisplay (void)
{
	//if (!l.aClass)
	{
		if (!l.flags.bThreadCreated )
		{
			int failcount = 0;
			l.flags.bThreadCreated = 1;
			AddLink( &l.threads, ThreadTo( VideoThreadProc, 0 ) );
#ifdef LOG_STARTUP
			Log( WIDE("Started video thread...") );
#endif
			{
				do
				{
					failcount++;
					do
					{
						Relinquish();
					}
					while (!l.bThreadRunning);
				} while( (failcount < 100) );
			}
		}
	}
	{
		//GLboolean params = 0;
		//if( glGetBooleanv(GL_STEREO, &params), params )
		//{
		//	lprintf( WIDE("yes stereo") );
		//}
		//else
		//	lprintf( WIDE("no stereo") );
	}


	return TRUE;
}

//----------------------------------------------------------------------------


TEXTCHAR  GetKeyText (int key)
{
   int c;
	char ch[5];
	if( key & KEY_MOD_DOWN )
      return 0;
	key ^= 0x80000000;

   c =  
#if !defined( UNDER_CE ) && !defined( __ANDROID__ ) && !defined( __QNX__ )
      ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
               l.kbd.key, (unsigned short *) ch, 0);
#else
	   key;
#endif
   if (!c)
   {
      // check prior key bindings...
      //printf( WIDE("no translation\n") );
      return 0;
   }
   else if (c == 2)
   {
      //printf( WIDE("Key Translated: %d %d\n"), ch[0], ch[1] );
      return 0;
   }
   else if (c < 0)
   {
      //printf( WIDE("Key Translation less than 0\n") );
      return 0;
   }
   //printf( WIDE("Key Translated: %d(%c)\n"), ch[0], ch[0] );
   return ch[0];
}

//----------------------------------------------------------------------------

LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
{
	InitDisplay();

	PutDisplayAbove( hNextVideo, l.top );

	AddLink( &l.pActiveList, hNextVideo );
	//hNextVideo->pid = l.pid;
	hNextVideo->KeyDefs = CreateKeyBinder();
#ifdef LOG_OPEN_TIMING
	lprintf( WIDE( "Doing open of a display..." ) );
#endif
	if( ( GetCurrentThreadId () == l.dwThreadID )  )
	{
#ifdef LOG_OPEN_TIMING
		lprintf( WIDE( "Allowed to create my own stuff..." ) );
#endif
		lprintf( WIDE("about to Create some window stuff") );
		CreateWindowStuffSizedAt( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										, hNextVideo->pWindowPos.cy);
		lprintf( WIDE("Created some window stuff") );
	}
	else
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			lprintf( WIDE("posting create to thred.") );
			//d = PostThreadMessage (l.dwThreadID, WM_USER_CREATE_WINDOW, 0,
			//							  (LPARAM) hNextVideo);
			if (!d)
			{
				Log1( WIDE("Failed to post create new window message...%d" ),
						GetLastError ());
				cnt--;
			}
#ifdef LOG_STARTUP
			else
			{
				lprintf( WIDE("Posted create new window message...") );
			}
#endif
			Relinquish();
		}
		while (!d && cnt);
		if (!d)
		{
			DebugBreak ();
		}
		if( hNextVideo )
		{
			//_32 timeout = timeGetTime() + 500000;
			hNextVideo->thread = GetMyThreadID();
			//while (!hNextVideo->flags.bReady && timeout > timeGetTime())
			{
				// need to do this on the possibility that
				// thIS thread did create this window...
				if( !Idle() )
				{
#ifdef NOISY_LOGGING
					lprintf( WIDE("Sleeping until the window is created.") );
#endif
					WakeableSleep( SLEEP_FOREVER );
					//Relinquish();
				}
			}
			//if( !hNextVideo->flags.bReady )
			{
				//CloseDisplay( hNextVideo );
				//lprintf( WIDE("Fatality.. window creation did not complete in a timely manner.") );
				// hnextvideo is null anyhow, but this is explicit.
				//return FALSE;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( WIDE("Resulting new window %p %p"), hNextVideo, GetNativeHandle( hNextVideo ) );
#endif
	return TRUE;
}


PVIDEO  OpenDisplaySizedAt (_32 attr, _32 wx, _32 wy, S_32 x, S_32 y) // if native - we can return and let the messages dispatch...
{
	PVIDEO hNextVideo;
	//lprintf( "open display..." );
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
	InitializeCriticalSec( &hNextVideo->cs );
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
	if (wx == -1)
		wx = CW_USEDEFAULT;
	if (wy == -1)
		wy = CW_USEDEFAULT;
	if (x == -1)
		x = CW_USEDEFAULT;
	if (y == -1)
		y = CW_USEDEFAULT;
#ifdef UNDER_CE
	l.WindowBorder_X = 0;
	l.WindowBorder_Y = 0;
#else
	l.WindowBorder_X = 0;//GetSystemMetrics (SM_CXFRAME);
	l.WindowBorder_Y = 0;//GetSystemMetrics (SM_CYFRAME)
	  // + GetSystemMetrics (SM_CYCAPTION) + GetSystemMetrics (SM_CYBORDER);
#endif
	// NOT MULTI-THREAD SAFE ANYMORE!
	//lprintf( WIDE("Hardcoded right here for FULL window surfaces, no native borders.") );
	hNextVideo->flags.bFull = TRUE;
	hNextVideo->flags.bLayeredWindow = 0;
	hNextVideo->flags.bNoAutoFocus = (attr & DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS)?TRUE:FALSE;
	hNextVideo->flags.bChildWindow = (attr & DISPLAY_ATTRIBUTE_CHILD)?TRUE:FALSE;
	hNextVideo->flags.bNoMouse = (attr & DISPLAY_ATTRIBUTE_NO_MOUSE)?TRUE:FALSE;
	hNextVideo->pWindowPos.x = x;
	hNextVideo->pWindowPos.y = y;
	hNextVideo->pWindowPos.cx = wx;
	hNextVideo->pWindowPos.cy = wy;

	if( DoOpenDisplay( hNextVideo ) )
	{
      lprintf( WIDE("New bottom is %p"), l.bottom );
      return hNextVideo;
	}
	Release( hNextVideo );
	return NULL;
}

 void  SetDisplayNoMouse ( PVIDEO hVideo, int bNoMouse )
{
	if( hVideo ) 
	{
		if( hVideo->flags.bNoMouse != bNoMouse )
		{
			hVideo->flags.bNoMouse = bNoMouse;
#ifndef NO_MOUSE_TRANSPARENCY
			if( bNoMouse )
			{
				//SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
				;//SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
#endif
		}

	}
}

//----------------------------------------------------------------------------

PVIDEO  OpenDisplayAboveSizedAt (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PVIDEO parent)
{
   PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

PVIDEO  OpenDisplayAboveUnderSizedAt (_32 attr, _32 wx, _32 wy,
                                               S_32 x, S_32 y, PVIDEO parent, PVIDEO barrier)
{
   PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if( barrier )
   {
	   // use initial SW_RESTORE instead of SW_NORMAL
		newvid->flags.bOpenedBehind = 1;
		newvid->under = barrier;
		//if( barrier->over )
		{
         //if( barrier->over != parent )
			//	DebugBreak();
		}
		barrier->over = newvid;
		if( barrier )
		{
			if( l.bottom == barrier )
			{
				l.bottom = newvid;
			}

			if( newvid->pBelow )
				newvid->pBelow->pAbove = newvid->pAbove;
			if( newvid->pAbove )
				newvid->pAbove->pBelow = newvid->pBelow;

			newvid->pBelow = barrier;
			if( newvid->pAbove = barrier->pAbove )
				barrier->pAbove->pBelow = newvid;
			barrier->pAbove = newvid;
		}
		//lprintf( "Opening window behind another." );
		//lprintf( "--- before SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );

		//SetWindowPos( newvid->hWndOutput, barrier->hWndOutput, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	   //lprintf( "--- after SWP --- " );
		//DumpChainAbove( newvid, newvid->hWndOutput );
		//DumpChainBelow( newvid, newvid->hWndOutput );
   }
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

//----------------------------------------------------------------------------

void  CloseDisplay (PVIDEO hVideo)
{
	lprintf( WIDE("close display %p"), hVideo );
	// just kills this video handle....
	if (!hVideo)         // must already be destroyed, eh?
		return;
#ifdef LOG_DESTRUCTION
	Log (WIDE( "Unlinking destroyed window..." ));
#endif
	// take this out of the list of active windows...
	DeleteLink( &l.pActiveList, hVideo );
	UnlinkVideo( hVideo );
	lprintf( WIDE("and we should be ok?") );
	hVideo->flags.bDestroy = 1;

	// the scan of inactive windows releases the hVideo...
	AddLink( &l.pInactiveList, hVideo );
	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
#ifdef USE_IPC_MESSAGE_QUEUE_TO_GATHER_MOUSE_EVENTS
	SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
#endif
   return;
}

//----------------------------------------------------------------------------

void  SizeDisplay (PVIDEO hVideo, _32 w, _32 h)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Size Display..." ) );
#endif
	if( w == hVideo->pWindowPos.cx && h == hVideo->pWindowPos.cy )
		return;
	if( hVideo->flags.bLayeredWindow )
	{
		// need to remake image surface too...
		hVideo->pWindowPos.cx = w;
		hVideo->pWindowPos.cy = h;
		CreateDrawingSurface (hVideo);
		if( hVideo->flags.bShown )
			UpdateDisplay( hVideo );
	}
}


//----------------------------------------------------------------------------

void  SizeDisplayRel (PVIDEO hVideo, S_32 delw, S_32 delh)
{
	if (delw || delh)
	{
		S_32 cx, cy;
		cx = hVideo->pWindowPos.cx + delw;
		cy = hVideo->pWindowPos.cy + delh;
		if (hVideo->pWindowPos.cx < 50)
			hVideo->pWindowPos.cx = 50;
		if (hVideo->pWindowPos.cy < 20)
			hVideo->pWindowPos.cy = 20;
#ifdef LOG_RESIZE
		Log2 (WIDE( "Resized display to %d,%d" ), hVideo->pWindowPos.cx,
            hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "size display relative" ) );
#endif
   }
}

//----------------------------------------------------------------------------

void  MoveDisplay (PVIDEO hVideo, S_32 x, S_32 y)
{
#ifdef LOG_ORDERING_REFOCUS
	lprintf( WIDE( "Move display %d,%d" ), x, y );
#endif
   if( hVideo )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
			Translate( hVideo->transform, x, y, 0.0 );
			if( hVideo->flags.bShown )
			{
				// layered window requires layered output to be called to move the display.
				UpdateDisplay( hVideo );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  MoveDisplayRel (PVIDEO hVideo, S_32 x, S_32 y)
{
   if (x || y)
   {
		lprintf( WIDE("Moving display %d,%d"), x, y );
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
		Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "Move display relative" ) );
#endif
   }
}

//----------------------------------------------------------------------------

void  MoveSizeDisplay (PVIDEO hVideo, S_32 x, S_32 y, S_32 w,
                                     S_32 h)
{
   S_32 cx, cy;
   hVideo->pWindowPos.x = x;
	hVideo->pWindowPos.y = y;
   cx = w;
   cy = h;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->pWindowPos.cx = cx;
   hVideo->pWindowPos.cy = cy;
#ifdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size display." ) );
#endif
   // updates window translation
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  MoveSizeDisplayRel (PVIDEO hVideo, S_32 delx, S_32 dely,
                                        S_32 delw, S_32 delh)
{
	S_32 cx, cy;
   hVideo->pWindowPos.x += delx;
   hVideo->pWindowPos.y += dely;
   cx = hVideo->pWindowPos.cx + delw;
   cy = hVideo->pWindowPos.cy + delh;
   if (cx < 50)
      cx = 50;
   if (cy < 20)
		cy = 20;
   hVideo->pWindowPos.cx = cx;
   hVideo->pWindowPos.cy = cy;
//fdef LOG_DISPLAY_RESIZE
	lprintf( WIDE( "move and size relative %d,%d %d,%d" ), delx, dely, delw, delh );
//ndif
   CreateDrawingSurface( hVideo );
}

//----------------------------------------------------------------------------

void  UpdateDisplayEx (PVIDEO hVideo DBG_PASS )
{
   // copy hVideo->lpBuffer to hVideo->hDCOutput
   if (hVideo )
   {
      UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
   }
   return;
}

//----------------------------------------------------------------------------

void  ClearDisplay (PVIDEO hVideo)
{
   lprintf( WIDE("Who's calling clear display? it's assumed clear") );
   //ClearImage( hVideo->pImage );
}

//----------------------------------------------------------------------------

void  SetMousePosition (PVIDEO hVid, S_32 x, S_32 y)
{
	if( !hVid )
	{
		int newx, newy;
		lprintf( WIDE("Moving Mouse Not Implemented") );
		InverseOpenGLMouse( hVid->camera, hVid, (RCOORD)x, (RCOORD)y, &newx, &newy );
		lprintf( WIDE("%d,%d (should)became %d,%d"), x, y, newx, newy );
		//SetCursorPos( newx, newy );
	}
	else
	{
		if( hVid->camera && hVid->flags.bFull)
		{
			int newx, newy;
			//lprintf( "TAGHERE" );
		lprintf( WIDE("Moving Mouse Not Implemented") );
			InverseOpenGLMouse( hVid->camera, hVid, x+ hVid->cursor_bias.x, y, &newx, &newy );
			//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
			//SetCursorPos (newx,newy);
		}
		else
		{
			if( l.current_mouse_event_camera )
			{
				int newx, newy;
				//lprintf( "TAGHERE" );
		lprintf( WIDE("Moving Mouse Not Implemented") );
				InverseOpenGLMouse( l.current_mouse_event_camera, hVid, x, y, &newx, &newy );
				//lprintf( "%d,%d became %d,%d", x, y, newx, newy );
				//SetCursorPos (newx + l.WindowBorder_X + hVid->cursor_bias.x + l.current_mouse_event_camera->x ,
				//				  newy + l.WindowBorder_Y + hVid->cursor_bias.y + l.current_mouse_event_camera->y );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  GetMousePosition (S_32 * x, S_32 * y)
{
	lprintf( WIDE("This is really relative to what is looking at it ") );
	//DebugBreak();
	if (x)
		(*x) = l.real_mouse_x;
	if (y)
		(*y) = l.real_mouse_y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
	GetMousePosition( x, y );
	if( b )
		(*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

void  SetCloseHandler (PVIDEO hVideo,
                                     CloseCallback pWindowClose,
                                     PTRSZVAL dwUser)
{
	if( hVideo )
	{
		hVideo->dwCloseData = dwUser;
		hVideo->pWindowClose = pWindowClose;
	}
}

//----------------------------------------------------------------------------

void  SetMouseHandler (PVIDEO hVideo,
                                     MouseCallback pMouseCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}

void  SetHideHandler (PVIDEO hVideo,
                                     HideAndRestoreCallback pHideCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwHideData = dwUser;
   hVideo->pHideCallback = pHideCallback;
}

void  SetRestoreHandler (PVIDEO hVideo,
                                     HideAndRestoreCallback pRestoreCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwRestoreData = dwUser;
   hVideo->pRestoreCallback = pRestoreCallback;
}


//----------------------------------------------------------------------------
#if !defined( NO_TOUCH )
RENDER_PROC (void, SetTouchHandler) (PVIDEO hVideo,
                                     TouchCallback pTouchCallback,
                                     PTRSZVAL dwUser)
{
   hVideo->dwTouchData = dwUser;
   hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------


void  SetRedrawHandler (PVIDEO hVideo,
                                      RedrawCallback pRedrawCallback,
                                      PTRSZVAL dwUser)
{
	hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		if( hVideo->flags.bShown )
		{
         l.flags.bUpdateWanted = 1;
		}
	}

}

//----------------------------------------------------------------------------

void  SetKeyboardHandler (PVIDEO hVideo, KeyProc pKeyProc,
                                        PTRSZVAL dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

void  SetLoseFocusHandler (PVIDEO hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         PTRSZVAL dwUser)
{
	hVideo->dwLoseFocus = dwUser;
	hVideo->pLoseFocus = pLoseFocus;
}

//----------------------------------------------------------------------------

void  SetApplicationTitle (const TEXTCHAR *pTitle)
{
	l.gpTitle = pTitle;
	if (l.cameras)
	{
      //DebugBreak();
		//SetWindowText((((struct display_camera *)GetLink( &l.cameras, 0 ))->hWndInstance), l.gpTitle);
	}
}

//----------------------------------------------------------------------------

void  SetRendererTitle (PVIDEO hVideo, const TEXTCHAR *pTitle)
{
	//l.gpTitle = pTitle;
	//if (l.hWndInstance)
	{
		if( hVideo->pTitle )
         Release( hVideo->pTitle );
		hVideo->pTitle = StrDupEx( pTitle DBG_SRC );
	}
}

//----------------------------------------------------------------------------

void  SetApplicationIcon (ImageFile * hIcon)
{
#ifdef _WIN32
   //HICON hIcon = CreateIcon();
#endif
}

//----------------------------------------------------------------------------

void  MakeTopmost (PVIDEO hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		if( hVideo->flags.bShown )
		{
			//lprintf( WIDE( "Forcing topmost" ) );
		}
		else
		{
		}
	}
}

//----------------------------------------------------------------------------

void  MakeAbsoluteTopmost (PVIDEO hVideo)
{
	if( hVideo )
	{
		hVideo->flags.bTopmost = 1;
		hVideo->flags.bAbsoluteTopmost = 1;
		if( hVideo->flags.bShown )
		{
		}
	}
}

//----------------------------------------------------------------------------

 int  IsTopmost ( PVIDEO hVideo )
{
   return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
void  HideDisplay (PVIDEO hVideo)
{
//#ifdef LOG_SHOW_HIDE
	lprintf(WIDE( "Hiding the window! %p %p %p" ), hVideo, hVideo->pAbove, hVideo->pBelow );
//#endif
	if( hVideo )
	{
		if( l.hCaptured == hVideo )
			l.hCaptured = NULL;
		hVideo->flags.bHidden = 1;
		/* handle lose focus */
	}
}

//----------------------------------------------------------------------------
#undef RestoreDisplay
void  RestoreDisplay (PVIDEO hVideo)
{
	RestoreDisplayEx( hVideo DBG_SRC );
}
void RestoreDisplayEx(PVIDEO hVideo DBG_PASS )
{
#ifdef __WINDOWS__
	PostThreadMessage (l.dwThreadID, WM_USER_OPEN_CAMERAS, 0, 0 );
#endif

#ifdef LOG_SHOW_HIDE
	lprintf( WIDE( "Restore display. %p %p" ), hVideo, hVideo?hVideo->hWndOutput:0 );
#endif

	if( hVideo )
	{
		hVideo->flags.bHidden = 0;
		hVideo->flags.bShown = 1;
	}
}

//----------------------------------------------------------------------------

void  GetDisplaySizeEx ( int nDisplay
												 , S_32 *x, S_32 *y
												 , _32 *width, _32 *height)
{
#if defined( __WINDOWS__ )
#ifndef NO_ENUM_DISPLAY
		if( nDisplay > 0 )
		{
			TEXTSTR teststring = NewArray( TEXTCHAR, 20 );
			//int idx;
			int v_test = 0;
			int i;
			int found = 0;
			DISPLAY_DEVICE dev;
			DEVMODE dm;
			dm.dmSize = sizeof( DEVMODE );
			dev.cb = sizeof( DISPLAY_DEVICE );
			for( v_test = 0; !found && ( v_test < 2 ); v_test++ )
			{
            // go ahead and try to find V devices too... not sure what they are, but probably won't get to use them.
				snprintf( teststring, 20, WIDE( "\\\\.\\DISPLAY%s%d" ), (v_test==1)?"V":"", nDisplay );
				for( i = 0;
					 !found && EnumDisplayDevices( NULL // all devices
														  , i
														  , &dev
														  , 0 // dwFlags
														  ); i++ )
				{
					if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
					{
						lprintf( WIDE("display %s is at %d,%d %dx%d"), dev.DeviceName, dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight );
					}
					else
						lprintf( WIDE("Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					lprintf( WIDE( "[%s] might be [%s]" ), teststring, dev.DeviceName );
					if( StrCaseCmp( teststring, dev.DeviceName ) == 0 )
					{
						if( EnumDisplaySettings( dev.DeviceName, ENUM_CURRENT_SETTINGS, &dm ) )
						{
							if( x )
								(*x) = dm.dmPosition.x;
							if( y )
								(*y) = dm.dmPosition.y;
							if( width )
								(*width) = dm.dmPelsWidth;
							if( height )
								(*height) = dm.dmPelsHeight;
							found = 1;
							break;
						}
						else
							lprintf( WIDE("Found display name, but enum current settings failed? %s %d" ), teststring, GetLastError() );
					}
					else
					{
						//lprintf( "[%s] is not [%s]", teststring, dev.DeviceName );
					}
				}
			}
		}
		else
#endif
		{
         // Ex version should return real screen size for screen 0.
			if( x )
				(*x)= 0;
			if( y )
				(*y)= 0;
			{
				RECT r;
				GetWindowRect (GetDesktopWindow (), &r);
				//Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );
				if (width)
					*width = r.right - r.left;
				if (height)
					*height = r.bottom - r.top;
			}
		}
#elseif defined( __QNX__ )
		if( nDisplay > 0 )
		{
			int n, m;
			ResolveDeviceID( nDisplay, &n, &m );
				// try and find display number N and return that display size.
			if( x )
				(*x) = 0;
			if( y )
				(*y) = 0;
			if( width )
				(*width) = l.qnx_display_info[n][m].xres;
			if( height )
				(*height) = l.qnx_display_info[n][m].yres;
		}
		else
		{
			// if any displays, otherwise, arrays are unallocated.
			// application should set insane source values which remain
			// unchanged.
			if( l.nDisplays )
			{
				if( x )
					(*x) = 0;
				if( y )
					(*y) = 0;
				if( width )
					(*width) = l.qnx_display_info[0][0].xres;
				if( height )
					(*height) = l.qnx_display_info[0][0].yres;
			}
			// return default display size
		}

#else
		if( x )
         (*x) = 0;
		if( y )
         (*y) = 0;
		if( width )
         (*width) = 640;
		if( height )
         (*height) = 480;
#endif

}

void  GetDisplaySize (_32 * width, _32 * height)
{
   GetDisplaySizeEx( 0, NULL, NULL, width, height );
}

//----------------------------------------------------------------------------

void  GetDisplayPosition (PVIDEO hVid, S_32 * x, S_32 * y,
                                        _32 * width, _32 * height)
{
	if (!hVid)
		return;
	if (width)
		*width = hVid->pWindowPos.cx;
	if (height)
		*height = hVid->pWindowPos.cy;
#ifdef __WINDOWS__
#ifndef NO_ENUM_DISPLAY
	{
		int posx = 0;
		int posy = 0;
		{
			WINDOWINFO wi;
			wi.cbSize = sizeof( wi);
			
			GetWindowInfo( hVid->hWndOutput, &wi ); 
			posx += wi.rcClient.left;
			posy += wi.rcClient.top;
		}
		if (x)
			*x = posx;
		if (y)
			*y = posy;
	}
#endif
#endif
}

//----------------------------------------------------------------------------
LOGICAL  DisplayIsValid (PVIDEO hVid)
{
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

void  SetDisplaySize (_32 width, _32 height)
{
   SizeDisplay (l.hVideoPool, width, height);
}

//----------------------------------------------------------------------------

ImageFile * GetDisplayImage (PVIDEO hVideo)
{
   return hVideo->pImage;
}

//----------------------------------------------------------------------------

PKEYBOARD  GetDisplayKeyboard (PVIDEO hVideo)
{
   return &hVideo->kbd;
}

//----------------------------------------------------------------------------

LOGICAL  HasFocus (PRENDERER hVideo)
{
   return hVideo->flags.bFocused;
}

//----------------------------------------------------------------------------

#if ACTIVE_MESSAGE_IMPLEMENTED
int  SendActiveMessage (PRENDERER dest, PACTIVEMESSAGE msg)
{
   return 0;
}

PACTIVEMESSAGE  CreateActiveMessage (int ID, int size,...)
{
   return NULL;
}

void  SetDefaultHandler (PRENDERER hVideo,
                                       GeneralCallback general, PTRSZVAL psv)
{
}
#endif
//----------------------------------------------------------------------------

void  OwnMouseEx (PVIDEO hVideo, _32 own DBG_PASS)
{
	if (own)
	{
		lprintf( WIDE("Capture is set on %p"),hVideo );
		if( !l.hCaptured )
		{
			l.hCaptured = hVideo;
			hVideo->flags.bCaptured = 1;
		}
		else
		{
			if( l.hCaptured != hVideo )
			{
				lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
				l.hCaptured = hVideo;
				hVideo->flags.bCaptured = 1;
			}
			else
			{
				if( !hVideo->flags.bCaptured )
				{
					lprintf( WIDE("This should NEVER happen!") );
               *(int*)0 = 0;
				}
				// should already have the capture...
			}
		}
	}
	else
	{
		if( l.hCaptured == hVideo )
		{
			lprintf( WIDE("No more capture.") );
			//ReleaseCapture ();
			hVideo->flags.bCaptured = 0;
			l.hCaptured = NULL;
		}
	}
}

//----------------------------------------------------------------------------
void
NoProc (void)
{
   // empty do nothing prodecudure for unimplemented features
}

//----------------------------------------------------------------------------
#undef GetNativeHandle
	HWND  GetNativeHandle (PVIDEO hVideo)
	{
		return NULL; //hVideo->hWndOutput;
	}

int  BeginCalibration (_32 nPoints)
{
   return 1;
}

//----------------------------------------------------------------------------
void  SyncRender( PVIDEO hVideo )
{
   // sync has no consequence...
   return;
}

//----------------------------------------------------------------------------

 void  ForceDisplayFocus ( PRENDERER pRender )
{
	//SetActiveWindow( GetParent( pRender->hWndOutput ) );
	//SetForegroundWindow( GetParent( pRender->hWndOutput ) );
	//SetFocus( GetParent( pRender->hWndOutput ) );
	//lprintf( WIDE( "... 3 step?" ) );
	//SetActiveWindow( pRender->hWndOutput );
	//SetForegroundWindow( pRender->hWndOutput );
	//if( pRender )
	//	SafeSetFocus( pRender->hWndOutput );
}

//----------------------------------------------------------------------------

 void  ForceDisplayFront ( PRENDERER pRender )
{
	if( pRender != l.top )
		PutDisplayAbove( pRender, l.top );

}

//----------------------------------------------------------------------------

 void  ForceDisplayBack ( PRENDERER pRender )
{
	// uhmm...
   lprintf( WIDE( "Force display backward." ) );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
void  UpdateDisplay (PVIDEO hVideo )
{
   //DebugBreak();
   UpdateDisplayEx( hVideo DBG_SRC );
}

void  DisableMouseOnIdle (PVIDEO hVideo, LOGICAL bEnable )
{
	if( hVideo->flags.bIdleMouse != bEnable )
	{
		if( bEnable )
		{
			//l.mouse_timer_id = (_32)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)2, 100, NULL );
			//hVideo->idle_timer_id = (_32)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)3, 100, NULL );
			l.last_mouse_update = GetTickCount(); // prime the hider.
			hVideo->flags.bIdleMouse = bEnable;
		}
		else // disabling...
		{
			hVideo->flags.bIdleMouse = bEnable;
			if( !l.flags.mouse_on )
			{
#ifdef LOG_MOUSE_HIDE_IDLE
				lprintf( WIDE( "Mouse was off... want it on..." ) );
#endif
				//SendMessage( hVideo->hWndOutput, WM_USER_MOUSE_CHANGE, 0, 0 );
			}
			if( hVideo->idle_timer_id )
			{
				//KillTimer( hVideo->hWndOutput, hVideo->idle_timer_id );
            hVideo->idle_timer_id = 0;
			}
		}
	}
}


 void  SetDisplayFade ( PVIDEO hVideo, int level )
{
	if( hVideo )
	{
		if( level < 0 )
         level = 0;
		if( level > 254 )
			level = 254;
		hVideo->fade_alpha = 255 - level;

		if( l.flags.bLogWrites )
			lprintf( WIDE( "Output fade %d" ), hVideo->fade_alpha );
	}
}

#undef GetRenderTransform
PTRANSFORM CPROC GetRenderTransform       ( PRENDERER r )
{
	return r->transform;
}

LOGICAL RequiresDrawAll ( void )
{
	return TRUE;
}

void MarkDisplayUpdated( PRENDERER r )
{
   l.flags.bUpdateWanted = 1;
	if( r )
      r->flags.bUpdated = 1;
}

#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void (CPROC*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , ProcessDisplayMessages
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32)) OpenDisplaySizedAt
                                       , (PRENDERER (CPROC*)(_32, _32, _32, S_32, S_32, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void (CPROC*)(PRENDERER)) CloseDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32, _32, _32 DBG_PASS)) UpdateDisplayPortionEx
                                       , (void (CPROC*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , (void (CPROC*)(PRENDERER)) ClearDisplay
                                       , GetDisplayPosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) MoveDisplayRel
                                       , (void (CPROC*)(PRENDERER, _32, _32)) SizeDisplay
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void (CPROC*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image (CPROC*)(PRENDERER)) GetDisplayImage
                                       , (void (CPROC*)(PRENDERER, CloseCallback, PTRSZVAL)) SetCloseHandler
                                       , (void (CPROC*)(PRENDERER, MouseCallback, PTRSZVAL)) SetMouseHandler
                                       , (void (CPROC*)(PRENDERER, RedrawCallback, PTRSZVAL)) SetRedrawHandler
                                       , (void (CPROC*)(PRENDERER, KeyProc, PTRSZVAL)) SetKeyboardHandler
													,  SetLoseFocusHandler
                                          , NULL
                                       , (void (CPROC*)(S_32 *, S_32 *)) GetMousePosition
                                       , (void (CPROC*)(PRENDERER, S_32, S_32)) SetMousePosition
                                       , HasFocus  // has focus
                                       , NULL         // SendMessage
													, NULL         // CrateMessage
                                       , GetKeyText
                                       , IsKeyDown
                                       , KeyDown
                                       , DisplayIsValid
                                       , OwnMouseEx
                                       , BeginCalibration
													, SyncRender   // sync
#ifdef _OPENGL_ENABLED
													, NULL //EnableOpenGL
                                       , NULL //SetActiveEGLDisplay
#else
                                       , NULL
                                       , NULL
#endif
                                       , MoveSizeDisplay
                                       , MakeTopmost
                                       , HideDisplay
                                       , RestoreDisplay
                                       , ForceDisplayFocus
                                       , ForceDisplayFront
                                       , ForceDisplayBack
                                       , BindEventToKey
													, UnbindKey
													, IsTopmost
													, NULL // OkaySyncRender is internal.
													, IsTouchDisplay
													, GetMouseState
                                       , EnableSpriteMethod
													, NULL// WinShell_AcceptDroppedFiles
													, PutDisplayIn
                                       , NULL //MakeDisplayFrom
													, SetRendererTitle
													, DisableMouseOnIdle
													, OpenDisplayAboveUnderSizedAt
													, SetDisplayNoMouse
													, Redraw
													, MakeAbsoluteTopmost
													, SetDisplayFade
													, IsDisplayHidden
#ifdef WIN32
													, GetNativeHandle
#endif
                                       , GetDisplaySizeEx
													, LockRenderer
													, UnlockRenderer
													, NULL  //IssueUpdateLayeredEx
                                       , RequiresDrawAll
#ifndef NO_TOUCH
													, SetTouchHandler
#endif
                                       , MarkDisplayUpdated
									   , SetHideHandler
									   , SetRestoreHandler
									   , RestoreDisplayEx
};

RENDER3D_INTERFACE Render3d = {
	GetRenderTransform

};

#undef GetDisplayInterface
#undef DropDisplayInterface

POINTER  CPROC GetDisplayInterface (void)
{
	InitDisplay();
   return (POINTER)&VidInterface;
}

void  CPROC DropDisplayInterface (POINTER p)
{
}

#undef GetDisplay3dInterface
POINTER CPROC GetDisplay3dInterface (void)
{
	InitDisplay();
	return (POINTER)&Render3d;
}

void  CPROC DropDisplay3dInterface (POINTER p)
{
}

static LOGICAL CPROC DefaultExit( PTRSZVAL psv, _32 keycode )
{
   lprintf( WIDE( "Default Exit..." ) );
	BAG_Exit(0);
   return 1;
}

static LOGICAL CPROC EnableRotation( PTRSZVAL psv, _32 keycode )
{
	lprintf( WIDE("Enable Rotation...") );
	if( IsKeyPressed( keycode ) )
	{
		l.flags.bRotateLock = 1 - l.flags.bRotateLock;
		if( l.flags.bRotateLock )
		{
			struct display_camera *default_camera = (struct display_camera *)GetLink( &l.cameras, 0 );
			l.mouse_x = default_camera->hVidCore->pWindowPos.cx/2;
			l.mouse_y = default_camera->hVidCore->pWindowPos.cy/2;
			lprintf( WIDE("Moving Mouse Not Implemented") );
			//SetCursorPos( default_camera->hVidCore->pWindowPos.x
			//	+ default_camera->hVidCore->pWindowPos.cx/2
			//	, default_camera->hVidCore->pWindowPos.y
			//	+ default_camera->hVidCore->pWindowPos.cy / 2 );
		}
		lprintf( WIDE("ALLOW ROTATE") );
	}
	else
		lprintf( WIDE("DISABLE ROTATE") );
	if( l.flags.bRotateLock )
		lprintf( WIDE("lock rotate") );
	else
		lprintf(WIDE("unlock rotate") );
   return 1;
}

static LOGICAL CPROC CameraForward( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
#define SPEED_CONSTANT 250
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
				Forward( l.origin, -SPEED_CONSTANT );
			else
				Forward( l.origin, SPEED_CONSTANT );
		}
      else
			Forward( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			if( keycode & KEY_SHIFT_DOWN )
			{
				Right( l.origin, SPEED_CONSTANT );
			}
			else
			{
				Right( l.origin, -SPEED_CONSTANT );
			}
		}
      else
			Right( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
      if( IsKeyPressed( keycode ) )
			Right( l.origin, SPEED_CONSTANT );
      else
			Right( l.origin, 0.0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRollRight( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			VECTOR tmp;
			scale( tmp, _Z, -1.0 );
			SetRotation( l.origin, tmp );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraRollLeft( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
			//VECTOR tmp;
         //scale(tmp,_Z,1.0 );
			SetRotation( l.origin, _Z );
		}
      else
			SetRotation( l.origin, _0 );
//      return 1;
      UpdateMouseRays( );
	}
//   return 0;
   return 1;
}

static LOGICAL CPROC CameraDown( PTRSZVAL psv, _32 keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
		{
 			if( keycode & KEY_SHIFT_DOWN )
				Up( l.origin, SPEED_CONSTANT );
			else
				Up( l.origin, -SPEED_CONSTANT );
		}
		else
			Up( l.origin, 0.0 );
      UpdateMouseRays( );
//      return 1;
	}
//   return 0;
   return 1;
}

int IsTouchDisplay( void )
{
   return 0;
}

LOGICAL IsDisplayHidden( PVIDEO video )
{
   if( video )
		return video->flags.bHidden;
   return 0;
}

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 46
#endif
PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	if( l.flags.bLogRegister )
		lprintf( WIDE("Regstering video interface...") );
#ifndef __ANDROID__
#ifndef UNDER_CE
#if !defined( __WATCOMC__ ) && !defined( __GNUC__ )
	l.GetTouchInputInfo = (BOOL (WINAPI *)( HTOUCHINPUT, UINT, PTOUCHINPUT, int) )LoadFunction( WIDE("user32.dll"), WIDE("GetTouchInputInfo") );
	l.CloseTouchInputHandle =(BOOL (WINAPI *)( HTOUCHINPUT ))LoadFunction( WIDE("user32.dll"), WIDE("CloseTouchInputHandle") );
	l.RegisterTouchWindow = (BOOL (WINAPI *)( HWND, ULONG  ))LoadFunction( WIDE("user32.dll"), WIDE("RegisterTouchWindow") );
#endif
#endif
#endif
	// loads options describing cameras; creates camera structures and math structures
	LoadOptions();
#ifdef __QNX__
   // gets handles to low level device information
	InitQNXDisplays();
#endif

	RegisterInterface( 
	   WIDE("puregl.render")
	   , GetDisplayInterface, DropDisplayInterface );
	RegisterInterface( 
	   WIDE("puregl.render.3d")
	   , GetDisplay3dInterface, DropDisplay3dInterface );
	l.gl_image_interface = (PIMAGE_INTERFACE)GetInterface( "puregl.image" );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	BindEventToKey( NULL, KEY_SCROLL_LOCK, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_F12, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_A, KEY_MOD_ALL_CHANGES, CameraLeft, 0 );
	BindEventToKey( NULL, KEY_S, KEY_MOD_ALL_CHANGES, CameraDown, 0 );
	BindEventToKey( NULL, KEY_D, KEY_MOD_ALL_CHANGES, CameraRight, 0 );
	BindEventToKey( NULL, KEY_W, KEY_MOD_ALL_CHANGES, CameraForward, 0 );
	BindEventToKey( NULL, KEY_Q, KEY_MOD_ALL_CHANGES, CameraRollLeft, 0 );
	BindEventToKey( NULL, KEY_E, KEY_MOD_ALL_CHANGES, CameraRollRight, 0 );
	//EnableLoggingOutput( TRUE );
}

//typedef struct sprite_method_tag *PSPRITE_METHOD;

PSPRITE_METHOD  EnableSpriteMethod (PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = New( struct sprite_method_tag );
	psm->renderer = render;
	psm->saved_spots = CreateDataQueue( sizeof( struct saved_location ) );
	psm->RenderSprites = RenderSprites;
	psm->psv = psv;
	AddLink( &render->sprites, psm );
   return psm; // the sprite should assign this...
}

// this is a magic routine, and should only be called by sprite itself
// and therefore this is handed to the image library via an export into image library
// this is done this way, because the image library MUST exist before this library
// therefore relying on the linker to handle this export is not possible.
static void CPROC SavePortion( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h )
{
	struct saved_location location;
   location.x = x;
   location.y = y;
   location.w = w;
	location.h = h;
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
	EnqueData( &psm->saved_spots, &location );
	//lprintf( "Save Portion %d,%d %d,%d", x, y, w, h );
}

PRELOAD( InitSetSavePortion )
{
   //SetSavePortion( SavePortion );
}

void LockRenderer( PRENDERER render )
{
	EnterCriticalSec( &render->cs );
}

void UnlockRenderer( PRENDERER render )
{
	LeaveCriticalSec( &render->cs );
}

PUBLIC( void, InvokePreloads )( void )
{
}

#if 0
#ifdef __ANDROID__

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#include <android/input.h>

#ifndef _PACKT_EVENTLOOP_HPP_
#define _PACKT_EVENTLOOP_HPP_
//#include "ActivityHandler.hpp"
//#include "InputHandler.hpp"
//#include "Types.hpp"
//#include <android_native_app_glue.h>
namespace packt {
class EventLoop {
public:
EventLoop(android_app* pApplication);
void run(ActivityHandler* pActivityHandler,
InputHandler* pInputHandler);
protected:
...
void processAppEvent(int32_t pCommand);
int32_t processInputEvent(AInputEvent* pEvent);
void processSensorEvent();
private:
...
static void callback_event(android_app* pApplication,
int32_t pCommand);
static int32_t callback_input(android_app* pApplication,
AInputEvent* pEvent);
private:
...
android_app* mApplication;
ActivityHandler* mActivityHandler;
InputHandler* mInputHandler;
};
}
#endif

namespace packt {
class InputHandler {
public:
virtual ~InputHandler() {};
virtual bool onTouchEvent(AInputEvent* pEvent) = 0;
};
}


static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

static const char gVertexShader[] = 
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char gFragmentShader[] = 
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

void renderFrame() {
    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
	 checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}


extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}

#endif
#endif
RENDER_NAMESPACE_END

