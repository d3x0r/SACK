/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <EGL/egl.h>
//#include <GLES/gl.h>
#include <render.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>

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
#if __USE_NATIVE_APP_EGL_MODULE__
    EGLDisplay display;
    EGLSurface surface;
	 EGLContext context;
#endif
    int32_t width;
    int32_t height;
	 struct saved_state state;
    volatile int wait_for_startup;
	 volatile int wait_for_display_init;
	 struct input_point points[10];
    int nPoints;
};

void (*OpenCameras)(void);
void (*RenderPass)(void);
void (*SetNativeWindowHandle)(NativeWindowType );
void (*SendTouchEvents)( int nPoints, PINPUT_POINT points );

struct engine engine;

#if __USE_NATIVE_APP_EGL_MODULE__
/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
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

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires. In this
     * sample, we have a very simplified selection process, where we pick
     * the first EGLConfig that matches our criteria */
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

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
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

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
#endif
/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event)
{
    struct engine* engine = (struct engine*)app->userData;
	 if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	 {
		 int32_t action = AMotionEvent_getAction(event );
		 int pointer = ( action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		 LOGI( "POINTER %04x %d %d", action, pointer, action & AMOTION_EVENT_ACTION_MASK );

		 {
			 int n;
			 for( n = 0; n < engine->nPoints; n++ )
			 {
				 if( engine->points[n].flags.end_event )
				 {
					 memcpy( engine->points + n, engine->points + n + 1, sizeof( struct input_point ) * (9-pointer) );
					 engine->nPoints--;
                n--;
				 }
			 }
		 }

		 switch( action & AMOTION_EVENT_ACTION_MASK )
		 {
		 case AMOTION_EVENT_ACTION_DOWN:
			 // primary pointer down.
          engine->points[0].x = AMotionEvent_getX( event, pointer );
			 engine->points[0].y = AMotionEvent_getY( event, pointer );
          engine->points[0].flags.new_event = 1;
			 engine->points[0].flags.end_event = 0;
          engine->nPoints++;
          break;
		 case AMOTION_EVENT_ACTION_UP:
			 // primary pointer up.
          engine->points[0].flags.new_event = 0;
			 engine->points[0].flags.end_event = 1;
          break;
		 case AMOTION_EVENT_ACTION_MOVE:
          engine->points[pointer].x = AMotionEvent_getX( event, pointer );
			 engine->points[pointer].y = AMotionEvent_getY( event, pointer );
          engine->points[pointer].flags.new_event = 0;
			 engine->points[pointer].flags.end_event = 0;
			 break;
		 case AMOTION_EVENT_ACTION_POINTER_DOWN:
			 // primary pointer down.
          engine->points[pointer].x = AMotionEvent_getX( event, pointer );
			 engine->points[pointer].y = AMotionEvent_getY( event, pointer );
          engine->points[pointer].flags.new_event = 1;
			 engine->points[pointer].flags.end_event = 0;
          engine->nPoints++;
			 break;
		 case AMOTION_EVENT_ACTION_POINTER_UP:
          engine->points[pointer].flags.new_event = 0;
			 engine->points[pointer].flags.end_event = 1;
			 break;
		 default:
          LOGI( "Motion Event ignored..." );
		 }
		 {
			 int n;
			 for( n = 0; n < engine->nPoints; n++ )
			 {
				 LOGI( "Point : %d %4d %4d %d %d", n, engine->points[n].x , engine->points[n].y, engine->points[n].flags.new_event, engine->points[n].flags.end_event );
			 }
		 }
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
			  engine->animating = 1;
           engine->wait_for_display_init = 1;
			  while( engine->wait_for_startup )
				  sched_yield();
			  OpenCameras();
           engine->wait_for_display_init = 0;
			  sched_yield();
           RenderPass();
#if __USE_NATIVE_APP_EGL_MODULE__
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
				}
#else
            // should do something to allow vidlib to process...
#endif
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
			  engine->animating = 0;
#if __USE_NATIVE_APP_EGL_MODULE__
			  engine_term_display(engine);
#else
			  // do something like unload interface
           // CloseCameras();
#endif
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
#if __USE_NATIVE_APP_EGL_MODULE__
				engine_draw_frame(engine);
#else
            // trigger want update?
#endif
            break;
    }
}

void *LoadLibrary( char *path, char *name )
{
	char buf[256];
   int tries = 0;
	snprintf( buf, 256, "%s/%s", path, name );
	do
	{
      void *result;
      //LOGI( "Open [%s]", buf );
		if( !( result = dlopen( buf, 0 ) ) )
		{
			const char *recurse = dlerror();
			LOGI( "error: %s", recurse );
			if( strstr( recurse, "could not load needed library" ) )
			{
				char *namestart = strchr( recurse, '\'' );
				char *nameend = strchr( namestart+1, '\'' );
				char tmpname[256];
				snprintf( tmpname, 256, "%*.*s", (nameend-namestart)-1,(nameend-namestart)-1,namestart+1 );
            LOGI( "Result was [%s]", tmpname );
				LoadLibrary( path, tmpname );
			}
			else
			{
				LOGI( "Some Other Eror:%s", recurse );
				break;
			}
		}
		else
         return result;
      tries++;
	}
   while( tries < 2 );
   return NULL;
}

void ExportAssets( void )
{
	AAssetManager* mgr;
	mgr = engine.app->activity->assetManager;
	AAssetDir* assetDir = AAssetManager_openDir(engine.app->activity->assetManager, "");
	const char* filename = (const char*)NULL;
	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
		AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING);
		char buf[BUFSIZ];
		int nb_read = 0;
		//LOGI( "Asset:[%s]", filename );
		FILE* out = fopen(filename, "w");
		while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
			fwrite(buf, nb_read, 1, out);
		fclose(out);
		AAsset_close(asset);
	}
	AAssetDir_close(assetDir);
}

void* BeginNormalProcess( void*param )
{
	char buf[256];
	{
		FILE *maps = fopen( "/proc/self/maps", "rt" );
		while( fgets( buf, 256, maps ) )
		{
         unsigned long start;
			unsigned long end;
         sscanf( buf, "%lx", &start );
			sscanf( buf+9, "%lx", &end );
			if( ((unsigned long)BeginNormalProcess >= start ) && ((unsigned long)BeginNormalProcess <= end ) )
			{
				char *mypath;
				char *myname;
				void *lib;
            char *myext;
				void (*InvokeDeadstart)(void );
				void (*MarkRootDeadstartComplete)(void );
				if( strlen( buf ) > 49 )
				mypath = strdup( buf + 49 );
				myext = strrchr( mypath, '.' );
				myname = strrchr( mypath, '/' );
				if( myname )
				{
					myname[0] = 0;
					myname++;
				}
				else
					myname = mypath;
				if( myext )
				{
               myext[0] = 0;
				}
            //LOGI( "my path [%s][%s]", mypath, myname );
				if( chdir( mypath ) )
               LOGI( "path change failed to [%s]", mypath );
				if( chdir( "../files" ) )
				{
					if( mkdir( "../files" ) )
					{
						LOGI( "path change failed to [%s]", mypath );
					}
					if( chdir( "../files" ) )
					{
                  getcwd( buf, 256 );
					}
				}
				{
					FILE *assets_saved = fopen( "assets.exported", "rb" );
					if( !assets_saved )
					{
						ExportAssets();
						fopen( "assets.exported", "wb" );
					}
				}
				LoadLibrary( mypath, "libbag.externals.so" );
				{
					void (*RunExits)(void );
					lib = LoadLibrary( mypath, "libbag.so" );
					RunExits = dlsym( lib, "RunExits" );
					if( RunExits )
						atexit( RunExits );
					else
						LOGI( "Failed to get symbol RunExits:%s", dlerror() );
					InvokeDeadstart = dlsym( lib, "InvokeDeadstart" );
					if( !InvokeDeadstart )
                  LOGI( "Failed to get InvokeDeadstart entry" );
               MarkRootDeadstartComplete = dlsym( lib, "MarkRootDeadstartComplete" );
					if( !MarkRootDeadstartComplete )
						LOGI( "Failed to get MarkRootDeadstartComplete entry" );
					{
						void (*f)(char*);
						f = dlsym( lib, "SACKSystemSetProgramPath" );
                  f( mypath );
						f = dlsym( lib, "SACKSystemSetProgramName" );
                  f( myname );
						f = dlsym( lib, "SACKSystemSetWorkingPath" );
                  f( buf );
					}
				}
				LoadLibrary( mypath, "libbag.psi.so" );

				{
					int n;
					for( n = 0; n < 1000; n++ )
					{
						if( !engine.app->pendingWindow )
							LOGI( "Pending window will fail!" );
						else
                     break;
						usleep( 10000 );
					}
				}
				{
					void *lib = LoadLibrary( mypath, "libbag.video.puregl.so" );
					if( !lib )
						LOGI( "Failed to load lib:%s", dlerror() );
					SetNativeWindowHandle = (void (*)(NativeWindowType ))dlsym( lib, "SetNativeWindowHandle" );
					if( !SetNativeWindowHandle )
						LOGI( "Failed to get SetNativeWindowHandle:%s", dlerror() );
					else
					{
						SetNativeWindowHandle( engine.app->pendingWindow );
					}
					RenderPass = (void (*)(void ))dlsym( lib, "DoRenderPass" );
					if( !RenderPass )
						LOGI( "Failed to get DoRenderPass:%s", dlerror() );
					OpenCameras = (void (*)(void ))dlsym( lib, "OpenCameras" );
					if( !OpenCameras )
						LOGI( "Failed to get OpenCameras:%s", dlerror() );
               SendTouchEvents = (void (*)(int,PINPUT_POINT ))dlsym( lib, "SendTouchEvents" );
				}
				{
					void *lib;
               void (*SACK_Main)(int,char* );
					snprintf( buf, 256, "%s.code.so", myname );
					lib = LoadLibrary( mypath, buf );
					SACK_Main = dlsym( lib, "SACK_Main" );
					if( !SACK_Main )
					{
						LOGI( "Failed to get entry point" );
                  return 0;
					}
               LOGI( "Invoke Deadstart..." );
					InvokeDeadstart();
               LOGI( "Deadstart Completed..." );
					MarkRootDeadstartComplete();
               engine.wait_for_startup = 0;
					sched_yield();
               while( engine.wait_for_display_init )
						sched_yield();
					SACK_Main( 0, NULL );
               LOGI( "Main exited... so we should all..." );
               break;
				}
			}
		}
      fclose( maps );
	}
   return NULL;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {

    // Make sure glue isn't stripped.
    app_dummy();

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

	 {
		 pthread_t thread;
       engine.wait_for_startup = 1;
		 pthread_create( &thread, NULL, BeginNormalProcess, NULL );
	 }
	 //ThreadTo( BeginNormalProcess, 0 );
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
                        //LOGI("accelerometer: x=%f y=%f z=%f",
                         //       event.acceleration.x, event.acceleration.y,
                         //       event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
				if (state->destroyRequested != 0) {
               LOGI( "Destroy Requested..." );
#if __USE_NATIVE_APP_EGL_MODULE__
					engine_term_display(&engine);
#endif
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
#if __USE_NATIVE_APP_EGL_MODULE__
				engine_draw_frame(&engine);
#else
            RenderPass();
            // trigger want draw?
#endif
        }
    }
}
//END_INCLUDE(all)
