// see also http://docs.nvidia.com/tegra/data/Android_Application_Lifecycle_in_Practice_A_Developer_s_Guide.html
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


#include "engine.h"

// sets the native window; opencameras will use this as the surface to initialize to.
void (*BagVidlibPureglSetNativeWindowHandle)(NativeWindowType );
void (*BagVidlibPureglOpenCameras)(void);
void (*BagVidlibPureglRenderPass)(void);
void (*BagVidlibPureglSendTouchEvents)( int nPoints, PINPUT_POINT points );
void (*BagVidlibPureglCloseDisplay)(void);  // do cleanup and suspend processing until a new surface is created.
void (*BagVidlibPureglSurfaceLost)(void);  // do cleanup and suspend processing until a new surface is created.
void (*BagVidlibPureglSurfaceGained)(NativeWindowType);  // do cleanup and suspend processing until a new surface is created.
void (*BagVidlibPureglSetTriggerKeyboard)(void(*show)(void),void(*hide)(void));  // do cleanup and suspend processing until a new surface is created.

int (*BagVidlibPureglSendKeyEvents)( int pressed, int key, int mods );

struct engine engine;

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event)
{
    struct engine* engine = (struct engine*)app->userData;
	 switch(AInputEvent_getType(event))
	 {
	 case AINPUT_EVENT_TYPE_MOTION:
		 {
		 int32_t action = AMotionEvent_getAction(event );
		 int pointer = ( action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK ) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		 int p = AMotionEvent_getPointerCount( event );

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
			 {
				 int n;
				 for( n = 0; n < p; n++ )
				 {
					 engine->points[n].x = AMotionEvent_getX( event, n );
					 engine->points[n].y = AMotionEvent_getY( event, n );
					 engine->points[n].flags.new_event = 0;
					 engine->points[n].flags.end_event = 0;
				 }
			 }
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
       /*
		 {
			 int n;
			 for( n = 0; n < engine->nPoints; n++ )
			 {
				 LOGI( "Point : %d %4d %4d %d %d", n, engine->points[n].x , engine->points[n].y, engine->points[n].flags.new_event, engine->points[n].flags.end_event );
			 }
		 }
       */
		 BagVidlibPureglSendTouchEvents( engine->nPoints, engine->points );
		 //engine->animating = 1;
        //engine->state.x = AMotionEvent_getX(event, 0);
        //engine->state.y = AMotionEvent_getY(event, 0);
		 return 1;
		 }
	 case AINPUT_EVENT_TYPE_KEY:
		 {
			 int32_t key_val = AKeyEvent_getKeyCode(event);
			 int32_t key_mods = AKeyEvent_getMetaState( event );
			 int32_t key_pressed = AKeyEvent_getAction( event );
			 LOGI("Received key event: %d %d %d\n", key_pressed, key_val, key_mods );
			 if( key_val )
			 {
             int used;
				 if( key_pressed == AKEY_EVENT_ACTION_MULTIPLE )
				 {
					 int count = AKeyEvent_getRepeatCount( event );
					 int n;
					 for( n = 0; n < count; n++ )
					 {
						 used = BagVidlibPureglSendKeyEvents( 1, key_val, key_mods );
						 used = BagVidlibPureglSendKeyEvents( 0, key_val, key_mods );
					 }
				 }
				 else
					 used = BagVidlibPureglSendKeyEvents( (key_pressed==AKEY_EVENT_ACTION_DOWN)?1:0, key_val, key_mods );
				 return used;
			 }
          break;
		 }
    }
    return 0;
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

extern void displayKeyboard(int pShow);

void show_keyboard( void )
{
	LOGI( "ShowSoftInput" );
   displayKeyboard( 1 );
	//ANativeActivity_showSoftInput( engine.app->activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT );

}

void hide_keyboard( void )
{
   LOGI( "HideSoftInput" );
   displayKeyboard( 0 );
   //ANativeActivity_hideSoftInput( engine.app->activity, ANATIVEACTIVITY_HIDE_SOFT_INPUT_IMPLICIT_ONLY );
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
// do not auto load libraries
#ifndef BUILD_PORTABLE_EXECUTABLE
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
					void *lib = LoadLibrary( mypath, "libbag.video.puregl2.so" );
					if( !lib )
						LOGI( "Failed to load lib:%s", dlerror() );

				}
#endif
				{
					void *lib;
               void (*SACK_Main)(int,char* );
					snprintf( buf, 256, "%s.code.so", myname );
					lib = LoadLibrary( mypath, buf );
               // assume we need to init this; it's probably a portable target
					if( !InvokeDeadstart )
					{
                  // this normally comes from bag; but a static/portable application may have it linked as part of its code
						InvokeDeadstart = dlsym( lib, "InvokeDeadstart" );
						{
							void (*f)(char*);
							f = dlsym( lib, "SACKSystemSetProgramPath" );
							f( mypath );
							f = dlsym( lib, "SACKSystemSetProgramName" );
							f( myname );
							f = dlsym( lib, "SACKSystemSetWorkingPath" );
							f( buf );
						}
						MarkRootDeadstartComplete = dlsym( lib, "MarkRootDeadstartComplete" );
					}

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

					// somehow these will be loaded
					// but we don't know where, but it's pretty safe to assume the names are unique, or
               // first-come-first-serve is appropriate
					BagVidlibPureglSetNativeWindowHandle = (void (*)(NativeWindowType ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetNativeWindowHandle" );
					if( !BagVidlibPureglSetNativeWindowHandle )
						LOGI( "Failed to get SetNativeWindowHandle:%s", dlerror() );

					BagVidlibPureglRenderPass = (void (*)(void ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_DoRenderPass" );
					if( !BagVidlibPureglRenderPass )
						LOGI( "Failed to get DoRenderPass:%s", dlerror() );

					BagVidlibPureglOpenCameras = (void (*)(void ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_OpenCameras" );
					if( !BagVidlibPureglOpenCameras )
						LOGI( "Failed to get OpenCameras:%s", dlerror() );

               BagVidlibPureglSendKeyEvents = (int(*)(int,int,int))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SendKeyEvents" );
					BagVidlibPureglSendTouchEvents = (void (*)(int,PINPUT_POINT ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SendTouchEvents" );
					BagVidlibPureglCloseDisplay = (void(*)(void))dlsym( RTLD_DEFAULT, "SACK_Vidlib_CloseDisplay" );
               BagVidlibPureglSurfaceLost = (void(*)(void))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SurfaceLost" );  // egl event
               BagVidlibPureglSurfaceGained = (void(*)(NativeWindowType))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SurfaceGained" );  // egl event
					BagVidlibPureglSetTriggerKeyboard = (void(*)(void(*)(void),void(*)(void)))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetTriggerKeyboard" );
               if( BagVidlibPureglSetTriggerKeyboard )
						BagVidlibPureglSetTriggerKeyboard( show_keyboard, hide_keyboard );

               // shouldn't need this shortly; was more about doing things my way than the android way
					engine.wait_for_startup = 0;
					// resume other threads so potentially the display is the next thing initialized.
					while( engine.wait_for_display_init )
						sched_yield();

					SACK_Main( 0, NULL );
               engine.closed = 1;
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
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
	 switch (cmd)
	 {
	 case APP_CMD_START:
		 {
			 pthread_t thread;
			 engine->wait_for_startup = 1;
			 pthread_create( &thread, NULL, BeginNormalProcess, NULL );
          // wait for core initilization to complete, and soft symbols to be loaded.
			 while( engine->wait_for_startup )
				  sched_yield();
		 }
       break;
	 case APP_CMD_SAVE_STATE:
		 // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
	 case APP_CMD_INIT_WINDOW:
		 // don't realy want to do anything, this is legal to bind to the egl context, but the size is invalid.
       // after init will get a changed anyway
		 //break;
	 //case APP_CMD_WINDOW_RESIZED:
      // LOGI( "Resized received..." );
		 // The window is being shown, get it ready.
		 engine->wait_for_display_init = 1;
		 while( engine->wait_for_startup )
		 {
          LOGI( "wait for deadstart to finish (load interfaces)" );
			 sched_yield();
		 }
		 BagVidlibPureglSetNativeWindowHandle( engine->app->pendingWindow );
		 engine->wait_for_display_init = 0;
		 // reopen cameras...
		 BagVidlibPureglOpenCameras();
		 break;
	 case APP_CMD_TERM_WINDOW:
		 // The window is being hidden or closed, clean it up.
		 engine->animating = 0;
		 BagVidlibPureglCloseDisplay();
		 break;
	 case APP_CMD_GAINED_FOCUS:
		 // first resume is not valid until gained focus (else resume during lock screen)
		 //case APP_CMD_RESUME:
		 // resume physics from now
		 engine->animating = 1;

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
		 // need to suspend physics at this point; aka next move is time 0, until the next-next

		 // When our app loses focus, we stop monitoring the accelerometer.
		 // This is to avoid consuming battery while not being used.
		 if (engine->accelerometerSensor != NULL) {
			 ASensorEventQueue_disableSensor(engine->sensorEventQueue,
														engine->accelerometerSensor);
		 }
		 // Also stop animating.
		 engine->animating = 0;
		 break;
    }
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

    // loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
		  while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		  {
			  // Process this event.
			  if (source != NULL)
			  {
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
				  //state->activity->vm->DetachCurrentThread();
				  BagVidlibPureglCloseDisplay();
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
            BagVidlibPureglRenderPass();
            // trigger want draw?
        }
		  if(engine.closed)
		  {
			  ANativeActivity_finish(state->activity);
		  }
	 }
}
//END_INCLUDE(all)
