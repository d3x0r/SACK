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

//#define DEBUG_TOUCH_INPUT
//#define DEBUG_KEY_INPUT

//BEGIN_INCLUDE(all)
#define NO_UNICODE_C
#include <jni.h>
#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <EGL/egl.h>
#include <linux/ashmem.h>
#include <sys/mman.h>
#include <android/asset_manager_jni.h>
//#include <GLES/gl.h>
//#include <render.h>


#include "engine.h"

// sets the native window; opencameras will use this as the surface to initialize to.
static void (*BagVidlibPureglSetNativeWindowHandle)(NativeWindowType );
static void (*BagVidlibPureglSetKeyboardMetric)( int );
static void (*BagVidlibPureglOpenCameras)(void);
static int (*BagVidlibPureglRenderPass)(void);
static void (*BagVidlibPureglFirstRender)(void);
static void (*BagVidlibPureglSendTouchEvents)( int nPoints, PINPUT_POINT points );
static void (*BagVidlibPureglCloseDisplay)(void);  // do cleanup and suspend processing until a new surface is created.
static void (*BagVidlibPureglSurfaceLost)(void);  // do cleanup and suspend processing until a new surface is created.
static void (*BagVidlibPureglSurfaceGained)(NativeWindowType);  // do cleanup and suspend processing until a new surface is created.
static void (*BagVidlibPureglSetTriggerKeyboard)( void(*show)(void)
																, void(*hide)(void)
																, int(*get_status_metric)(void)
																, int(*get_keyboard_metric)(void)
																, char*(*get_key_text)(void) );  // do cleanup and suspend processing until a new surface is created.
static void (*BagVidlibPureglSetAnimationWake)(void(*wake_animation)(void));  // do cleanup and suspend processing until a new surface is created.
static void (*BagVidlibPureglSetSleepSuspend)(void(*suspend)(int));  // do cleanup and suspend processing until a new surface is created.

static int  (*BagVidlibPureglSendKeyEvents)( int pressed, int key, int mods );
static void (*SACK_Main)(int,char* );
static char *myname;
static char *mypath;

// second generation (native framebuffer hooks)
static void (*BagVidlibPureglPauseDisplay)(void);
static void (*BagVidlibPureglResumeDisplay)(void);

extern int AndroidGetKeyText( AInputEvent *event );
extern void AndroidLoadSharedLibrary( char *libname );


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
#ifdef DEBUG_TOUCH_INPUT
		LOGI( "pointer (full action)%04x (pointer)%d (number points)%d", action, pointer, p );
#endif
		{
			int n;
			for( n = 0; n < engine->nPoints; n++ )
			{
				engine->points[n].flags.new_event = 0;
				if( engine->points[n].flags.end_event )
				{
					int m;
					for( m = n; m < (engine->nPoints-1); m++ )
					{
						if( engine->input_point_map[m+1] == m+1 )
							engine->input_point_map[m] = m;
						else
						{
							if( engine->input_point_map[m+1] < n )
								engine->input_point_map[m] = engine->input_point_map[m+1];
							else
								engine->input_point_map[m] = engine->input_point_map[m+1] - 1;

						}
						engine->points[m] = engine->points[m+1];
					}
					engine->nPoints--;
					n--;
				}
			}
		}

		switch( action & AMOTION_EVENT_ACTION_MASK )
		{
		case AMOTION_EVENT_ACTION_DOWN:
			// primary pointer down.
			//if( engine->nPoints )
			//{
			//	LOGI( "Pointer Event Down (pointer0) and there's already pointers..." );
			//}
			engine->points[0].x = AMotionEvent_getX( event, pointer );
			engine->points[0].y = AMotionEvent_getY( event, pointer );
			engine->points[0].flags.new_event = 1;
			engine->points[0].flags.end_event = 0;
			engine->nPoints++;
			engine->input_point_map[0] = 0;
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
					// points may have come in as 'new' in the wrong order,
					// reference the input point map to fill in the correct point location
					int actual = engine->input_point_map[n];
					engine->points[actual].x = AMotionEvent_getX( event, n );
					engine->points[actual].y = AMotionEvent_getY( event, n );
					engine->points[actual].flags.new_event = 0;
					engine->points[actual].flags.end_event = 0;
				}
			}
			break;
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			// the new pointer might not be the last one, so we insert it.
			// at the end, before dispatch, new points are moved to the end
			// and mapping begins.  this code should not reference the map
			if( pointer < engine->nPoints )
			{
				int c;
#ifdef DEBUG_TOUCH_INPUT
				LOGI( "insert point new. %d", engine->nPoints-1 );
#endif
				for( c = engine->nPoints; c >= pointer; c-- )
				{
#ifdef DEBUG_TOUCH_INPUT
					LOGI( "Set %d to %d", c, engine->input_point_map[c-1] );
#endif
					engine->input_point_map[c] = engine->input_point_map[c-1]; // save still in the same target...
				}
#ifdef DEBUG_TOUCH_INPUT
				LOGI( "Set %d to %d", pointer, engine->nPoints );
#endif
				engine->input_point_map[pointer] = engine->nPoints; // and the new one maps to the last.
				// now just save in last and don't swap twice.
				engine->points[engine->nPoints].x = AMotionEvent_getX( event, pointer );
				engine->points[engine->nPoints].y = AMotionEvent_getY( event, pointer );
				pointer = engine->nPoints;
			}
			else
			{
				engine->points[pointer].x = AMotionEvent_getX( event, pointer );
				engine->points[pointer].y = AMotionEvent_getY( event, pointer );
				engine->input_point_map[pointer] = pointer;
			}
			// primary pointer down.
			engine->points[pointer].flags.new_event = 1;
			engine->points[pointer].flags.end_event = 0;
			// always initialize the
			engine->nPoints++;
			break;
		case AMOTION_EVENT_ACTION_POINTER_UP:
			{
				// a up pointer may be remapped already, set the actual entry for the point
				int actual = engine->input_point_map[pointer];
				int n;
				engine->points[actual].flags.new_event = 0;
				engine->points[actual].flags.end_event = 1;

#ifdef DEBUG_TOUCH_INPUT
				LOGI( "Set point %d (map %d) to ended", pointer, actual );
#endif
				// any release event will reset the other input points appropriately(?)
				for( n = 0; n < engine->nPoints; n++ )
				{
					int other;
					if( engine->input_point_map[n] != n )
					{
						int m;
#ifdef DEBUG_TOUCH_INPUT
						LOGI( "reorder to natural input order" );
#endif
						memcpy( engine->tmp_points, engine->points, engine->nPoints * sizeof( struct input_point ) );
						// m is the point currently mapped to this position.
						// data from engine[n] and engine[m] need to swap
						for( m = 0; m < engine->nPoints; m++ )
						{
							engine->points[m] = engine->tmp_points[other = engine->input_point_map[m]];
							engine->input_point_map[m] = m;
#ifdef DEBUG_TOUCH_INPUT
							LOGI( "move point %d to %d", other, m );
#endif
						}
						break;
					}
				}
			}
			break;
		default:
#ifdef DEBUG_TOUCH_INPUT
			LOGI( "Motion Event ignored..." );
#endif
			break;
		}

		{
			int n;
#ifdef DEBUG_TOUCH_INPUT
			for( n = 0; n < engine->nPoints; n++ )
			{
				LOGI( "Point : %d %d %g %g %d %d", n, engine->input_point_map[n], engine->points[n].x , engine->points[n].y, engine->points[n].flags.new_event, engine->points[n].flags.end_event );
			}
#endif
		}

		BagVidlibPureglSendTouchEvents( engine->nPoints, engine->points );
		//engine->state.animating = 1;
		//engine->state.x = AMotionEvent_getX(event, 0);
		//engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
		}
	case AINPUT_EVENT_TYPE_KEY:
		{
			int32_t key_val = AKeyEvent_getKeyCode(event);
			//int32_t key_char = AKeyEvent_getKeyChar(event);
			int32_t key_mods = AKeyEvent_getMetaState( event );
			int32_t key_pressed = AKeyEvent_getAction( event );
			int realmod = 0;
         //lprintf( "key char is %d (%c)", key_char, key_char );
			if( ( key_mods & 0x3000 ) == 0x3000 )
				realmod |= KEY_MOD_CTRL;
			if( ( key_mods & 0x12 ) == 0x12 )
				realmod |= KEY_MOD_ALT;
			if( ( key_mods & 0x41 ) == 0x41 )
				realmod |= KEY_MOD_SHIFT;
			key_mods = realmod;
#ifdef DEBUG_KEY_INPUT
			LOGI("Received key event: %d %d %d\n", key_pressed, key_val, key_mods );
#endif
			{
				engine->key_text = AndroidGetKeyText( event );
            lprintf( "Event translates to %d(%04x)%c", engine->key_text, engine->key_text ,engine->key_text );
			}

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
	default:
		LOGI( "Unhandled Motion Event ignored..." );
		break;
	}
	return 0;
}

static void *LoadLibrary( char *path, char *name )
{
	char buf[256];
	int tries = 0;
	snprintf( buf, 256, "%s/%s", path, name );
   //AndroidLoadSharedLibrary( name );
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


FILE *OpenFile( const char *filename )
{
	const char *start;
	const char *end;
	char *tmp;
	int len;
	FILE *result = NULL;
	int ofs = 0;
	tmp = malloc( sizeof( char ) * ( len = ( strlen(filename) + 1 ) ) );
	start = filename;
	do
	{
		end = strchr( start, '~' );
		if( end )
		{
			DIR* dir;
         //LOGI( "attempt (%d)%s", end-start, start );
			ofs += snprintf( tmp + ofs, len, "%*.*s", end-start, end-start, start );
			dir = opendir( tmp );
			if (dir)
			{
				/* Directory exists. */
            //lprintf( "directory already exists... moving on..." );
				closedir(dir);
			}
			else if (ENOENT == errno)
			{
				/* Directory does not exist. */
				if( mkdir( tmp, 0700 ) )
					LOGI( "Failed to make directory %s(%d)", tmp, errno );
			}
			else
			{
				/* opendir() failed for some other reason. */
			}
			start = end + 1;
			tmp[ofs++] = '/';
		}
		else
		{
			snprintf( tmp + ofs, len - ofs, "%s", start );
			result = fopen( tmp, "wb" );
		}
	}while( end );
	free( tmp );
	return result;
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
		FILE* out = OpenFile(filename);
		if( out )
		{
			while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
				fwrite(buf, nb_read, 1, out);
			fclose(out);
		}
		else
			LOGI( "Failed to open asset copy: %s", filename );
		AAsset_close(asset);
	}
	AAssetDir_close(assetDir);
}

extern void displayKeyboard(int pShow);
extern void SuspendSleep(int bSuspend);
extern int AndroidGetStatusMetric( void );

char * AndroidGetCurrentKeyText( void )
{
   return (char*)&engine.key_text;
}


int  AndroidGetKeyboardMetric( void )
{
	LOGI( "Result with one of the sizes." );
   return 0;
}

void show_keyboard( void )
{										
#ifdef DEBUG_KEY_INPUT
	LOGI( "ShowSoftInput" );
#endif
	displayKeyboard( 1 );
	//ANativeActivity_showSoftInput( engine.app->activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT );

}

void hide_keyboard( void )
{
#ifdef DEBUG_KEY_INPUT
	LOGI( "HideSoftInput" );
#endif
	displayKeyboard( 0 );
	//ANativeActivity_hideSoftInput( engine.app->activity, ANATIVEACTIVITY_HIDE_SOFT_INPUT_IMPLICIT_ONLY );
}

void wake_animation( void )
{
	if( engine.have_focus )
	{
		if( !engine.state.animating )
		{
			engine.state.animating = 1;
			android_app_write_cmd( engine.app, APP_CMD_WAKE);
		}	
	}
}

static int findself( void )
{
	if( !engine.state.restarting && !myname )
	{
		char buf[256];
		FILE *maps;
		//LOGI( "Finding self in /proc/self/maps,,," );
		maps = fopen( "/proc/self/maps", "rt" );
		while( maps && fgets( buf, 256, maps ) )
		{
			unsigned long start;
			unsigned long end;
			sscanf( buf, "%lx", &start );
			sscanf( buf+9, "%lx", &end );
			//LOGI( "Map includes: %s", buf );
			if( ((unsigned long)findself >= start ) && ((unsigned long)findself <= end ) )
			{
            const char *filepath;
				char *myext;
				//LOGI( "THIS IS ME" );
				fclose( maps );
				maps = NULL;

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
				filepath = engine.data_path;//malloc( 256 );
            //snprintf( filepath, 256, "/data/data/%s/files", engine.data_path );
				LOGI( "my path [%s][%s][%s]", filepath, mypath, myname );
            return 1;
			}
		}
		if( maps )
			fclose( maps );
	}
   return 0;
}

void* BeginNormalProcess( void*param )
{
	engine.wait_for_startup = 0;
	sched_yield();
	LOGI( "BeginNormalProcess: %d %d %d  %d %p %p", engine.state.restarting, engine.state.closed, engine.state.opened, engine.wait_for_startup, myname, BeginNormalProcess );
	while( !engine.data_path )
      sched_yield();
	if( !engine.state.restarting && !myname )
	{
		if( findself() )
		{
			do
			{
            char buf[256];
				void (*InvokeDeadstart)(void );
				void (*MarkRootDeadstartComplete)(void );
				const char * filepath = engine.data_path;//malloc( 256 );
				//snprintf( filepath, 256, "/data/data/%s/files", engine.data_path );
				LOGI( "my path [%s][%s][%s]", filepath, mypath, myname );
				if( chdir( filepath ) )
				//if( chdir( "../files" ) )
				{
					LOGI( "path change failed to [%s]", mypath );
					if( mkdir( filepath /*"../files"*/, 0700 ) )
					{
						LOGI( "path change failed to [%s]", mypath );
					}
					chdir( filepath );
				}
				getcwd( buf, 256 );
				LOGI( "ended up in directory:%s", buf );

				{
					FILE *assets_saved = fopen( "assets.exported", "rb" );
					if( !assets_saved )
					{
						ExportAssets();
						assets_saved = fopen( "assets.exported", "wb" );
					}
               fclose( assets_saved );
				}
// do not auto load libraries
#ifndef BUILD_PORTABLE_EXECUTABLE
				LoadLibrary( mypath, "libbag.externals.so" );
				{
					void *lib;
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
#endif
				{
					void *lib;
					snprintf( buf, 256, "%s.code.so", myname );
					//LOGI( "Load library... %s", buf );
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
						// allow normal main fail processing
						break;
					}
					//LOGI( "Invoke Deadstart..." );
					InvokeDeadstart();
					//LOGI( "Deadstart Completed..." );
					MarkRootDeadstartComplete();

					// somehow these will be loaded
					// but we don't know where, but it's pretty safe to assume the names are unique, or
					// first-come-first-serve is appropriate
					BagVidlibPureglSetNativeWindowHandle = (void (*)(NativeWindowType ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetNativeWindowHandle" );
					if( !BagVidlibPureglSetNativeWindowHandle )
						LOGI( "Failed to get SetNativeWindowHandle:%s", dlerror() );

					BagVidlibPureglSetKeyboardMetric = (void (*)(int ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetKeyboardMetric" );
					BagVidlibPureglFirstRender = (void (*)(void ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_DoFirstRender" );
					//if( !BagVidlibPureglFirstRender )
					//	LOGI( "Failed to get BagVidlibPureglFirstRender:%s", dlerror() );

					BagVidlibPureglRenderPass = (int (*)(void ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_DoRenderPass" );
					//if( !BagVidlibPureglRenderPass )
					//	LOGI( "Failed to get DoRenderPass:%s", dlerror() );

					BagVidlibPureglOpenCameras = (void (*)(void ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_OpenCameras" );
					if( !BagVidlibPureglOpenCameras )
						LOGI( "Failed to get OpenCameras:%s", dlerror() );

					BagVidlibPureglSendKeyEvents = (int(*)(int,int,int))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SendKeyEvents" );
					BagVidlibPureglSendTouchEvents = (void (*)(int,PINPUT_POINT ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SendTouchEvents" );
					BagVidlibPureglCloseDisplay = (void(*)(void))dlsym( RTLD_DEFAULT, "SACK_Vidlib_CloseDisplay" );
					BagVidlibPureglSurfaceLost = (void(*)(void))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SurfaceLost" );  // egl event
					BagVidlibPureglSurfaceGained = (void(*)(NativeWindowType))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SurfaceGained" );  // egl event

					BagVidlibPureglPauseDisplay = (void(*)(void))dlsym( RTLD_DEFAULT,"SACK_Vidlib_PauseDisplay" );
					BagVidlibPureglResumeDisplay = (void(*)(void))dlsym( RTLD_DEFAULT,"SACK_Vidlib_ResumeDisplay" );

					BagVidlibPureglSetTriggerKeyboard = (void(*)(void(*)(void),void(*)(void),int(*get_status_metric)(void)
																			  ,int(*get_keyboard_metric)(void)
																			  ,char*(*get_key_text)(void)
																			  ))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetTriggerKeyboard" );
					if( BagVidlibPureglSetTriggerKeyboard )
						BagVidlibPureglSetTriggerKeyboard( show_keyboard, hide_keyboard, AndroidGetStatusMetric, AndroidGetKeyboardMetric, AndroidGetCurrentKeyText );

					BagVidlibPureglSetAnimationWake = (void(*)(void(*)(void)))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetAnimationWake" );
					if( BagVidlibPureglSetAnimationWake )
						BagVidlibPureglSetAnimationWake( wake_animation );

					BagVidlibPureglSetSleepSuspend = (void(*)(void(*)(int)))dlsym( RTLD_DEFAULT, "SACK_Vidlib_SetSleepSuspend" );
					if( BagVidlibPureglSetSleepSuspend )
						BagVidlibPureglSetSleepSuspend( SuspendSleep );

					// shouldn't need this shortly; was more about doing things my way than the android way
					//engine.wait_for_display_init = 1;
					engine.wait_for_startup = 0;
 					// resume other threads so potentially the display is the next thing initialized.
					//while( engine.wait_for_display_init )
					sched_yield();
					//break;
				}
			}
			while( 0 );
		}
		else if( !myname )
		{
			LOGI( "Failed to open procself" );
			myname = "Reading /proc/self/maps failed";
		}
	}
	else
		engine.wait_for_startup = 0;

	if( !SACK_Main )
	{
		LOGI( "(still)Failed to get SACK_Main entry point; I am [%s]", myname );
		return 0;
	}
	LOGI( "Start Application..." );
	if( engine.app->pendingWindow && BagVidlibPureglSetNativeWindowHandle )
		BagVidlibPureglSetNativeWindowHandle( engine.app->pendingWindow );
	if( engine.resumed )
	{
		if( BagVidlibPureglResumeDisplay )
			BagVidlibPureglResumeDisplay();
      engine.state.animating = 1;
		android_app_write_cmd( engine.app, APP_CMD_WAKE);
	}
	SACK_Main( 0, NULL );
	engine.state.closed = 1;
	LOGI( "Main exited... and so should we all..." );
	return NULL;
}


/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd)
	{
	default:
		LOGI( "Other Command: %d", cmd );
		break;
	case APP_CMD_WAKE:
      break;
	case APP_CMD_DESTROY:
		// need to end normal process here
		// unload everything....
      //
      break;
	case APP_CMD_START:
		{
			if( !engine->state.restarting )
			{
            LOGI( "No; start is just a new display...." );
			}
			else
				LOGI( "App already started... just going to get a new display..." );
		}
		break;
	case APP_CMD_SAVE_STATE:
      LOGI( "*** Save my isntance data... do things change?" );
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
		while( engine->wait_for_startup )
		{
			LOGI( "wait for deadstart to finish (load interfaces)" );
			sched_yield();
		}
		if( BagVidlibPureglSetNativeWindowHandle )
		{
			BagVidlibPureglSetNativeWindowHandle( engine->app->pendingWindow );
			// reopen cameras...  (wait for animate to trigger that)
			// BagVidlibPureglOpenCameras();
		engine->state.animating = 1;

			LOGI( "Clear wait for display init..." );
		}
		if( engine->have_focus )
			engine->state.animating = 1;

		//engine->wait_for_display_init = 0;
      engine->state.opened = 1;
		//sched_yield();
		break;
	case APP_CMD_CONFIG_CHANGED:
		{
			int new_orient = AConfiguration_getOrientation(engine->app->config);
			if( engine->state.orientation != new_orient )
			{
            engine->state.orientation = new_orient;
				if( BagVidlibPureglPauseDisplay )
					BagVidlibPureglPauseDisplay();
			}
		}
      break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
      engine->state.opened = 0;
		engine->state.animating = 0;
		engine->app->pendingWindow = NULL;
      if( BagVidlibPureglCloseDisplay )
			BagVidlibPureglCloseDisplay();
		break;
	case APP_CMD_GAINED_FOCUS:
		// first resume is not valid until gained focus (else resume during lock screen)
		//case APP_CMD_RESUME:
		// resume physics from now
		LOGI( "Gain focus" );
      engine->have_focus = 1;
      if( engine->state.opened )
			engine->state.animating = 1;
		else
         LOGI( "but we don't have a window?!" );

		// When our app gains focus, we start monitoring the accelerometer.
		//if (engine->accelerometerSensor != NULL) {
		//	ASensorEventQueue_enableSensor(engine->sensorEventQueue,
		//						engine->accelerometerSensor);
			// We'd like to get 60 events per second (in us).
		 //  ASensorEventQueue_setEventRate(engine->sensorEventQueue,
		 //  										engine->accelerometerSensor, (1000L/60)*1000);
		//}
		break;
	case APP_CMD_PAUSE:
      engine->have_focus = 0;
		engine->state.animating = 0;
		while( engine->state.rendering )
         sched_yield();
		if( BagVidlibPureglPauseDisplay )
			BagVidlibPureglPauseDisplay();
      engine->resumed = 0;
		break;
	case APP_CMD_RESUME:
		lprintf( "Resume..." );
		if( BagVidlibPureglResumeDisplay )
			BagVidlibPureglResumeDisplay();
      engine->resumed = 1;
		break;
	case APP_CMD_LOST_FOCUS:
		// need to suspend physics at this point; aka next move is time 0, until the next-next

		// Also stop animating.
		engine->state.animating = 0;
		break;
	}
}

void BeginNativeProcess( struct engine* engine )
{
			{
				pthread_t thread;
				engine->wait_for_startup = 1;
				LOGI( "Create A thread start! *** " );
				pthread_create( &thread, NULL, BeginNormalProcess, NULL );
				// wait for core initilization to complete, and soft symbols to be loaded.
				//while( engine->wait_for_startup )
				//	sched_yield();
			}
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
   const char *data_path = engine.data_path;
	// Make sure glue isn't stripped.
	app_dummy();

	memset(&engine, 0, sizeof(engine));
   engine.data_path = data_path;
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

   LOGI( "BEGIN ANDROID_MAIN" );
	// Prepare to monitor accelerometer
 	//engine.sensorManager = ASensorManager_getInstance();
	//engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
	//			ASENSOR_TYPE_ACCELEROMETER);
	//engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
	//			state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
		engine.state.closed = 0;
      engine.state.animating = 0; // won't be ready to animate until later...
		engine.state.restarting = 1;
		LOGI( "Recover prior saved state... %d %d", engine.state.closed, engine.state.restarting );
	}
	else
	{
      LOGI( "No prior state" );
		engine.state.orientation = AConfiguration_getOrientation(engine.app->config);
		BeginNativeProcess( &engine );
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
		while ((ident=ALooper_pollAll(engine.state.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		{
			// Process this event.
			if (source != NULL)
			{
				source->process(state, source);
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				LOGI( "Destroy Requested... %d", engine.state.closed );
				//state->activity->vm->DetachCurrentThread();
				BagVidlibPureglCloseDisplay();
				engine.did_finish_once = 0;
				return;
			}

			// if not animating, this will get missed...
			if(engine.state.closed && !engine.did_finish_once )
			{
				engine.did_finish_once = 1;
				LOGI( "Do final activity" );
				ANativeActivity_finish(state->activity);
			}
		}

		if (engine.state.animating) {
				// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			// trigger want draw?
         //LOGI( "Animating..." );
			engine.state.rendering = 1;
			if( BagVidlibPureglRenderPass )
				engine.state.animating = BagVidlibPureglRenderPass();
			else
			{
				if( BagVidlibPureglFirstRender )
					BagVidlibPureglFirstRender();
				engine.state.animating = 0;
			}
         engine.state.rendering = 0;
		}
	}
}

#include <jni.h>
#ifdef __cplusplus
extern "C"
{
#endif
	JNIEXPORT void JNICALL

		//Java_InterShell_1console_update_1keyboard_1size
		//Java_org_d3x0r_InterShell_Chatment_InterShell_1console_update_1keyboard_1size
		//Java_org_d3x0r_InterShell_Chatment_InterShell_1console_update_1keyboard_1size(JNIEnv * env, jobject obj, jint keyboard_metric)
		//Java_org_d3x0r_sack_InterShell_InterShell_1window_update_1keyboard_1size(JNIEnv * env, jobject obj, jint keyboard_metric)
		Java_org_d3x0r_sack_core_NativeStaticLib_update_1keyboard_1size(JNIEnv * env, jobject obj, jint keyboard_metric)
	{
		if( BagVidlibPureglSetKeyboardMetric )
		{
			BagVidlibPureglSetKeyboardMetric( keyboard_metric );
			//LOGI( "SUCCESS GETTING TO CODE! %d", keyboard_metric );
		}

	}

	JNIEXPORT void JNICALL

		//Java_InterShell_1console_update_1keyboard_1size
		//Java_org_d3x0r_InterShell_Chatment_InterShell_1console_update_1keyboard_1size
		//Java_org_d3x0r_InterShell_Chatment_InterShell_1console_update_1keyboard_1size(JNIEnv * env, jobject obj, jint keyboard_metric)
		//Java_org_d3x0r_sack_InterShell_InterShell_1window_update_1keyboard_1size(JNIEnv * env, jobject obj, jint keyboard_metric)
		Java_org_d3x0r_sack_core_NativeStaticLib_start(JNIEnv * env, jobject obj, jobject assetObject )
	{
			{
				pthread_t thread;
				engine.wait_for_startup = 1;
				{
               struct android_app* state = (struct android_app*)malloc( sizeof( struct android_app ) );
					state->userData = &engine;
					state->onAppCmd = engine_handle_cmd;
					state->onInputEvent = engine_handle_input;
					state->activity = (ANativeActivity*)malloc( sizeof( struct ANativeActivity ) );
               state->activity->assetManager =  AAssetManager_fromJava(env, assetObject);;
					engine.app = state;
				}
				LOGI( "Create A thread start! *** " );
				pthread_create( &thread, NULL, BeginNormalProcess, NULL );
				// wait for core initilization to complete, and soft symbols to be loaded.
				while( engine.wait_for_startup )
					sched_yield();
			}

	}

	JNIEXPORT void JNICALL
		Java_org_d3x0r_sack_core_NativeStaticLib_setDataPath(JNIEnv * env, jobject obj, jstring package)
	{
		//LOGI( "SETTING PACKAGE: %s", package );
		// there is a release function for this... so this will be always valid.
		// restart should change this?
      if( !engine.data_path )
			engine.data_path = (*env)->GetStringUTFChars( env, package , NULL );
		//LOGI( "resulted with : %s", engine.data_path );

	}

	JNIEXPORT jint JNICALL
		Java_org_d3x0r_sack_core_NativeStaticLib_loadSharedLibrary(JNIEnv * env, jobject obj, jstring library )
	{
      int fd = -1;
		//LOGI( "SETTING PACKAGE: %s", package );
		// there is a release function for this... so this will be always valid.
		// restart should change this?
		const char *file = (*env)->GetStringUTFChars( env, library, NULL );
		char filename[256];
      FILE *filefile;
		void *filedata;
		size_t filesize;
		int ret;
		if( !mypath )
         findself();
		LOGI( "Got a library to load... : %p %p", mypath, file );
		snprintf( filename, 256, "%s/%s", mypath, file );
		filefile = fopen( filename, "rb" );
		if( filefile )
		{
			fseek( filefile, 0, SEEK_END );
			filesize = ftell( filefile );

					fd = open("/dev/ashmem", O_RDWR);
					if( fd < 0 )
					{
                  lprintf( "Failed to open core device..." );
						return -1;
					}

					ret = ioctl(fd, ASHMEM_SET_SIZE, filesize );
					if (ret < 0)
					{
						lprintf( "Failed to set IOCTL size to %d", filesize );
						//goto error;
					}

			//fd = ashmem_create_region( "meaningless name", filesize );
			{
				char *map = mmap(NULL, filesize, PROT_READ|PROT_WRITE,
									  MAP_SHARED, fd, 0);
				fread( map, 1, filesize, filefile );
			}
         fclose( filefile );
		}
		(*env)->ReleaseStringUTFChars(env, library, file);
		//LOGI( "resulted with : %s", engine.data_path );
		return fd;
	}

	JNIEXPORT int JNICALL
		Java_org_d3x0r_sack_core_NativeStaticLib_mapSharedLibrary(JNIEnv * env, jobject obj, jint share_fd)
	{

	}

#ifdef __cplusplus
}
#endif


//END_INCLUDE(all)
