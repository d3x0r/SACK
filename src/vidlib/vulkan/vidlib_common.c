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
#endif

#ifdef _MSC_VER
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#endif

#define NO_UNICODE_C
#define FIX_RELEASE_COM_COLLISION

#define NEED_REAL_IMAGE_STRUCTURE
#define USE_IMAGE_INTERFACE l.gl_image_interface

#include <stdhdrs.h>
#include <sqlgetoption.h>
/* again, this should be moved to stdhdrs so we get timeGetTime() */

#undef StrDup

#ifdef _WIN32
#include <shlwapi.h> // have to include this if shellapi.h is included (for mingw)
#include <shellapi.h> // very last though - this is DragAndDrop definitions...
#endif

// this is safe to leave on.
#define LOG_ORDERING_REFOCUS

// move local into render namespace.
#define VIDLIB_MAIN
#include "local.h"

IMAGE_NAMESPACE

struct saved_location
{
	int32_t x, y;
	uint32_t w, h;
};

IMAGE_NAMESPACE_END

RENDER_NAMESPACE

HWND  GetNativeHandle (PVIDEO hVideo);

extern KEYDEFINE KeyDefs[];


// forward declaration - staticness will probably cause compiler errors.

//----------------------------------------------------------------------------

void  EnableLoggingOutput( LOGICAL bEnable )
{
	l.flags.bLogWrites = bEnable;
}

void  UpdateDisplayPortionEx( PVIDEO hVideo
                                          , int32_t x, int32_t y
                                          , uint32_t w, uint32_t h DBG_PASS)
{

	if( hVideo )
		hVideo->flags.bShown = 1;
	l.flags.bUpdateWanted = 1;
	if( l.wake_callback )
		l.wake_callback();
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
	{
		l.top = hVideo->pAbove;
		//lprintf( "set l.top to %p", l.top );
	}	
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
		else		  // nothing points to this - therefore we must find the start
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
		//lprintf( "set l.top to %p", l.top );
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
		{
			l.top = hVideo;
			//lprintf( "set l.top to %p", l.top );
		}

		if( hVideo->pAbove )
		{
			lprintf( WIDE( "Window was over somethign else and now we die." ) );
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
		tnprintf( name, sizeof( name ), WIDE( "render.display.%p" ), hVideo );
		//lprintf( WIDE( "making initial transform" ) );
		hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
	}

	//lprintf( WIDE( "Set transform at %d,%d" ), hVideo->pWindowPos.x, hVideo->pWindowPos.y );
	Translate( hVideo->transform, (RCOORD)hVideo->pWindowPos.x, (RCOORD)hVideo->pWindowPos.y, 0 );

	// additionally indicate that this is a GL render point
	hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
	return TRUE;
}

void DoDestroy (PVIDEO hVideo)
{
	if (hVideo)
	{
		if (hVideo->pWindowClose)
		{
			hVideo->pWindowClose (hVideo->dwCloseData);
		}

		if( hVideo->over )
			hVideo->over->under = NULL;
		if( hVideo->under )
			hVideo->under->over = NULL;
		Deallocate(TEXTCHAR *, hVideo->pTitle);
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
		if( l.hVirtualCaptured == hVideo )
		{
			l.hVirtualCaptured = NULL;
			l.hVirtualCapturedPrior = NULL; // make sure to clear this so it doesn't get reset to invalid
			l.flags.bVirtualManuallyCapturedMouse = 0;
		}
		if( l.hCameraCaptured == hVideo )
		{
			l.hCameraCaptured = NULL;
			l.hCameraCapturedPrior = NULL; // make sure to clear this so it doesn't get reset to invalid
			l.flags.bCameraManuallyCapturedMouse = 0;
		}
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

void LoadOptions( void )
{
	uint32_t average_width, average_height;
	//int some_width;
	//int some_height;
	//HostSystem_InitDisplayInfo();
#ifndef __ANDROID__
	// these come in courtesy of the android system...
	l.default_display_x = 1024;
	l.default_display_y = 768;
#endif

#ifndef __NO_OPTIONS__
	l.flags.bLogRenderTiming = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Log Render Timing"), 0, TRUE );
	l.flags.bView360 = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/360 view"), 0, TRUE );

	l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Scale"), 10 );
	if( l.scale == 0.0 )
	{
		l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Inverse Scale"), 2 );
		if( l.scale == 0.0 )
			l.scale = 1;
	}
	else
		l.scale = 1.0f / l.scale;
	//lprintf( "LoadOptions" );
	if( !l.cameras )
	{
		struct display_camera *default_camera = NULL;
		uint32_t screen_w, screen_h;
		int nDisplays = SACK_GetProfileIntEx( GetProgramName(), WIDE("SACK/Video Render/Number of Displays"), l.flags.bView360?6:1, TRUE );
		int n;
		lprintf( WIDE("Loading %d displays"), nDisplays );
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
		//lprintf( "Set camera 0 to 1" );
		SetLink( &l.cameras, 0, (POINTER)1 ); // set default here 
		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			camera->nCamera = n + 1;
			camera->depth = 30000.0f;
			camera->origin_camera = CreateTransform();

			tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Display is topmost"), n+1 );
			camera->flags.topmost = SACK_GetProfileIntEx( GetProgramName(), tmp, 0, TRUE );

#if defined( __QNX__ ) || defined( __ANDROID__ )
			custom_pos = 0;
#else
			tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Custom Position"), n+1 );
			custom_pos = SACK_GetProfileIntEx( GetProgramName(), tmp, l.flags.bView360?1:0, TRUE );
#endif
			if( custom_pos )
			{
				camera->display = -1;
				tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/x"), n+1 );
				camera->x = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?400:n==2?0:n==3?400:0
					:nDisplays==6
						?n==0?((screen_w * 0)/4):n==1?((screen_w * 1)/4):n==2?((screen_w * 1)/4):n==3?((screen_w * 1)/4):n==4?((screen_w * 2)/4):n==5?((screen_w * 3)/4):0
					:nDisplays==1
						?0
					:0), TRUE );
				tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/y"), n+1 );
				camera->y = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?n==0?0:n==1?0:n==2?300:n==3?300:0
					:nDisplays==6
						?n==0?((screen_h * 1)/3):n==1?((screen_h * 0)/3):n==2?((screen_h * 1)/3):n==3?((screen_h * 2)/3):n==4?((screen_h * 1)/3):n==5?((screen_h * 1)/3):0
					:nDisplays==1
						?0
					:0), TRUE );
				tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/width"), n+1 );
				camera->w = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?400
					:nDisplays==6
						?( screen_w / 4 )
					:nDisplays==1
						?800
					:0), TRUE );
				tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Position/height"), n+1 );
				camera->h = SACK_GetProfileIntEx( GetProgramName(), tmp, (
					nDisplays==4
						?300
					:nDisplays==6
						?( screen_h / 3 )
					:nDisplays==1
						?600
					:0), TRUE );
				/*
				tnprintf( tmp, sizeof( tmp ), "SACK/Video Render/Display %d/Position/Direction", n+1 );
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
				tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Use Display"), n+1 );
				camera->display = SACK_GetProfileIntEx( GetProgramName(), tmp, nDisplays>1?n+1:0, TRUE );
				GetDisplaySizeEx( camera->display, &camera->x, &camera->y, &camera->w, &camera->h );
			}

			camera->identity_depth = camera->w/2.0f;
			if( l.flags.bForceUnaryAspect )
				camera->aspect = 1.0;
			else
			{
				camera->aspect = ((float)camera->w/(float)camera->h);
			}

			tnprintf( tmp, sizeof( tmp ), WIDE("SACK/Video Render/Display %d/Camera Type"), n+1 );
			camera->type = SACK_GetProfileIntEx( GetProgramName(), tmp, (nDisplays==6)?n:2, TRUE );
			if( camera->type == 2 && !default_camera )
			{
				default_camera = camera;
			}
			//lprintf( "Add camera to list" );
			AddLink( &l.cameras, camera );
			lprintf( WIDE(" camera is %d,%d" ), camera->w, camera->h );
		}
		if( !default_camera )
		{
			default_camera = (struct display_camera *)GetLink( &l.cameras, 1 );
			//lprintf( "Retrieve default as %p", default_camera );
		}
		lprintf( WIDE("Set default to %p"), default_camera );
		SetLink( &l.cameras, 0, default_camera );
	}
	{
		PODBC option = GetOptionODBC( NULL );
		l.flags.bLogMessageDispatch = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log message dispatch"), 0, TRUE );
		l.flags.bLogFocus = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log focus event"), 0, TRUE );
		l.flags.bLogKeyEvent = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log key event"), 0, TRUE );
		l.flags.bLogMouseEvent = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/log mouse event"), 0, TRUE );
#ifdef _D3D_DRIVER
#  define LAYERED_DEFAULT 0
#else
		// opengl output to layered (transparent) windows is impracticle, and requires an extra move from video memory, or render to
		// conventional memroy and then a push to video memory;  tiny surfaces may work (those things... toolbar widgets)
#  define LAYERED_DEFAULT 0
#endif
		l.flags.bLayeredWindowDefault = SACK_GetOptionIntEx( option, GetProgramName(), WIDE( "SACK/Video Render/Default windows are layered" ), LAYERED_DEFAULT, TRUE )?TRUE:FALSE;
		l.flags.bLogWrites = SACK_GetOptionIntEx( option, GetProgramName(), WIDE("SACK/Video Render/Log Video Output"), 0, TRUE );
	}
#else
	l.flags.bLogRenderTiming = 0;
	l.flags.bView360 = 0;

	l.scale = 1.0 / 10;
	//lprintf( "LoadOptions" );
	if( !l.cameras )
	{
		struct display_camera *default_camera = NULL;
		uint32_t screen_w, screen_h;
		int nDisplays = 1;
		int n;
		lprintf( WIDE("Loading %d displays"), nDisplays );
		l.flags.bForceUnaryAspect = 0;
		GetDisplaySizeEx( 0, NULL, NULL, &screen_w, &screen_h );
		//lprintf( "Set camera 0 to 1" );
		SetLink( &l.cameras, 0, (POINTER)1 ); // set default here 
		for( n = 0; n < nDisplays; n++ )
		{
			TEXTCHAR tmp[128];
			int custom_pos;
			struct display_camera *camera = New( struct display_camera );
			MemSet( camera, 0, sizeof( *camera ) );
			camera->nCamera = n + 1;
			camera->depth = 30000.0f;
			camera->origin_camera = CreateTransform();

			camera->flags.topmost = 0;

#if defined( __QNX__ ) || defined( __ANDROID__ )
			custom_pos = 0;
#else
			custom_pos = 0;
#endif
			camera->display = 0;
			GetDisplaySizeEx( camera->display, &camera->x, &camera->y, &camera->w, &camera->h );

			camera->identity_depth = camera->w/2;
			if( l.flags.bForceUnaryAspect )
				camera->aspect = 1.0;
			else
			{
				camera->aspect = ((float)camera->w/(float)camera->h);
			}

			camera->type = 2;
			if( camera->type == 2 && !default_camera )
			{
				default_camera = camera;
			}
			//lprintf( "Add camera to list" );
			AddLink( &l.cameras, camera );
			lprintf( WIDE(" camera is %d,%d" ), camera->w, camera->h );
		}
		if( !default_camera )
		{
			default_camera = (struct display_camera *)GetLink( &l.cameras, 1 );
			//lprintf( "Retrieve default as %p", default_camera );
		}
		lprintf( WIDE("Set default to %p"), default_camera );
		SetLink( &l.cameras, 0, default_camera );
	}
	l.flags.bLogMessageDispatch = 0;
	l.flags.bLogFocus = 0;
	l.flags.bLogKeyEvent = 0;
	l.flags.bLogMouseEvent = 0;
	l.flags.bLayeredWindowDefault = 0;
	l.flags.bLogWrites = 0;
#endif

	if( !l.origin )
	{
		static MATRIX m;
		lprintf( WIDE("Init camera") );
		l.origin = CreateNamedTransform( WIDE("render.camera") );

		Translate( l.origin, l.scale * average_width/2, l.scale * average_height/2, l.scale * average_height/2 );
		RotateAbs( l.origin, M_PI, 0, 0 );

		CreateTransformMotion( l.origin ); // some things like rotate rel

		// spin so we can see if the display is SOMEWHERE
		//SetRotation( l.origin, _Y );

	}


}

static void InvokeExtraInit( struct display_camera *camera, PTRANSFORM view_camera )
{
	uintptr_t (CPROC *Init3d)(PMatrix,PTRANSFORM,RCOORD*,RCOORD*);
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	TEXTCHAR optname[64];
	for( name = GetFirstRegisteredName( WIDE("sack/render/puregl/init3d"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		LOGICAL already_inited;
		tnprintf( optname, 64, WIDE("%s/%d"), name, camera->nCamera );
		already_inited = GetRegisteredIntValueEx( data, optname, WIDE("Executed") );
		if( already_inited )
			continue;
		RegisterIntValueEx( data, optname, WIDE("Executed"), 1 );
		Init3d = GetRegisteredProcedureExx( data,(CTEXTSTR)name,uintptr_t,WIDE("ExtraInit3d"),(PMatrix,PTRANSFORM,RCOORD*,RCOORD*));

		if( Init3d )
		{
			struct plugin_reference *reference;
			uintptr_t psvInit = Init3d( &l.fProjection, view_camera, &camera->identity_depth, &camera->aspect );
			if( psvInit )
			{
				INDEX idx;
				LIST_FORALL( camera->plugins, idx, struct plugin_reference *, reference )
				{
					if( StrCaseCmp( reference->name, name ) == 0 )
						break;
				}
				if( !reference )
				{
					reference = New( struct plugin_reference );
					reference->flags.did_first_draw = 0;
					reference->name = name;
					{
						static PCLASSROOT draw3d;
						if( !draw3d )
							draw3d = GetClassRoot( WIDE("sack/render/puregl/draw3d") );
						reference->Update3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("Update3d"),(PTRANSFORM));
						reference->Resume3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("Resume3d"),(void));
						// add one copy of each update proc to update list.
						if( FindLink( &l.update, reference->Update3d ) == INVALID_INDEX )
							AddLink( &l.update, reference->Update3d );
						reference->Draw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraDraw3d"),(uintptr_t));
						reference->FirstDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("FirstDraw3d"),(uintptr_t));
						reference->ExtraDraw3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraBeginDraw3d"),(uintptr_t,PTRANSFORM));
						reference->ExtraClose3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,void,WIDE("ExtraClose3d"),(uintptr_t));
						reference->Mouse3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("ExtraMouse3d"),(uintptr_t,PRAY,int32_t,int32_t,uint32_t));
						reference->Key3d = GetRegisteredProcedureExx( draw3d,(CTEXTSTR)name,LOGICAL,WIDE("ExtraKey3d"),(uintptr_t,uint32_t));
					}
					if( reference->Draw3d )
						 camera->flags.bDrawablePlugin = 1;
					AddLink( &camera->plugins, reference );
				}

				// update the psv for the new init.
				reference->psv = psvInit;
			}
		}
	}

}

int CPROC OpenGLKey( uintptr_t psv, uint32_t keycode )
{
	struct display_camera *camera = (struct display_camera *)psv;
	int used = 0;
	PRENDERER check = NULL;
	INDEX idx;
	struct plugin_reference *ref;
	if( l.hPluginKeyCapture )
	{
		used = l.hPluginKeyCapture->Key3d( l.hPluginKeyCapture->psv, keycode );
		if( !used )
			l.hPluginKeyCapture = NULL;
		else
			return 1;
	}

	LIST_FORALL( camera->plugins, idx, struct plugin_reference *, ref )
	{
		if( ref->Key3d )
		{
			used = ref->Key3d( ref->psv, keycode );
			if( used )
			{
				// if a thing uses a key, lock to that plugin for future keys until it doesn't want a key
				// (thing like a chat module, first key would lock to it, and it could claim all events;
				// maybe should implement an interface to manually clear this
				l.hPluginKeyCapture = ref;
				break;
			}
		}
	}
	return used;
}

void OpenCamera( struct display_camera *camera )
{
	{
		//lprintf( "Open camera %p", camera);
		if( camera->flags.opening )
			return;

		if( !camera->hVidCore )
		{
			camera->hVidCore = New( VIDEO );
			MemSet (camera->hVidCore, 0, sizeof (VIDEO));
			InitializeCriticalSec( &camera->hVidCore->cs );
			camera->hVidCore->camera = camera;
			camera->hVidCore->flags.bLayeredWindow = l.flags.bLayeredWindowDefault;

			camera->hVidCore->pMouseCallback = OpenGLMouse;
			camera->hVidCore->dwMouseData = (uintptr_t)camera;
			camera->hVidCore->pKeyProc = OpenGLKey;
			camera->hVidCore->dwKeyData = (uintptr_t)camera;
		}
		else if( camera->hVidCore->flags.bReady )
		{
			// already open, and valid.
			return;
		}


		/* CreateWindowEx */
#ifdef __QNX__
		CreateQNXOutputForCamera( camera );
#endif
#ifdef USE_EGL
		camera->displayWindow = l.displayWindow;
		OpenEGL( camera, camera->displayWindow );
#else
		camera->flags.opening = 1;
		//lprintf( "Open win32 camera..." );
#  if !defined( __LINUX__ )
		OpenWin32Camera( camera );
#  endif
#endif

#ifdef __ANDROID__
		if( l.displayWindow )
			camera->hVidCore->flags.bReady = 1;
#endif
#ifdef LOG_OPEN_TIMING
		lprintf( WIDE( "Created Real window...Stuff.. %d,%d %dx%d" ),camera->x,camera->y,camera->w,camera->h );
#endif
		// trigger first draw logic for camera
		camera->flags.first_draw = 1;
		camera->flags.opening = 0;
		// extra init iterates through registered plugins and
		// loads their initial callbacks; the actual OnIni3d() has many more params
		camera->flags.init= 0; // make sure we do first setup on context...
		lprintf( WIDE("Invoke init on camera (should be a valid device by here?") );
		InvokeExtraInit( camera, camera->origin_camera );

		// first draw allows loading textures and shaders; so reset that we did a first draw.
		// render loop short circuits if camera is not ready
		camera->hVidCore->flags.bReady = TRUE;
	}

}

// returns the forward view camera (or default camera)
struct display_camera *SACK_Vidlib_OpenCameras( void )
{
	struct display_camera *camera;
	INDEX idx;
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
		if( !idx ) // default camera is a duplicate of another camera
			continue;
		//lprintf( "Open camera %d", idx);
		OpenCamera( camera );
	}

	return (struct display_camera *)GetLink( &l.cameras, 0 );
}

#ifndef __ANDROID__
// when a library is loaded later, invoke it's Init3d with existing cameras
static void OnLibraryLoad( WIDE("Video Render PureGL 2") )( void )
{
#  if defined( WIN32 )
	if( l.bThreadRunning )
		PostThreadMessage( l.dwThreadID, WM_USER_OPEN_CAMERAS, 0, 0);
#  endif
}
#endif

LOGICAL  CreateWindowStuffSizedAt (PVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
		if( hVideo )
		{
			if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
			{
				uint32_t w, h;
				GetDisplaySize( &w, &h );
				wx = w * 7 / 10;
				wy = h * 7 / 10;
			}
			if( x == CW_USEDEFAULT )
				x = 10;
			if( y == CW_USEDEFAULT )
				y = 10;

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
				//lprintf( WIDE("%d %d"), x, y );
				{
					hVideo->pImage =
						RemakeImage( hVideo->pImage, NULL, hVideo->pWindowPos.cx,
										hVideo->pWindowPos.cy );
					if( !hVideo->transform )
					{
						TEXTCHAR name[64];
						tnprintf( name, sizeof( name ), WIDE( "render.display.%p" ), hVideo );
						//lprintf( WIDE( "making initial transform" ) );
						hVideo->transform = hVideo->pImage->transform = CreateTransformMotion( CreateNamedTransform( name ) );
					}

					//lprintf( WIDE( "Set transform at %d,%d" ), hVideo->pWindowPos.x, hVideo->pWindowPos.y );
					Translate( hVideo->transform, (RCOORD)hVideo->pWindowPos.x, (RCOORD)hVideo->pWindowPos.y, 0 );

					// additionally indicate that this is a GL render point
					hVideo->pImage->flags |= IF_FLAG_FINAL_RENDER;
				}
				hVideo->flags.bReady = 1;
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
}

//----------------------------------------------------------------------------

LOGICAL DoOpenDisplay( PVIDEO hNextVideo )
{
	// starts our message thread if there is one...
	if( !l.top )
		l.hVidVirtualFocused = hNextVideo;

	PutDisplayAbove( hNextVideo, l.top );

	AddLink( &l.pActiveList, hNextVideo );
	hNextVideo->KeyDefs = CreateKeyBinder();
#ifdef LOG_OPEN_TIMING
	lprintf( WIDE( "Doing open of a display... %p" ) , hNextVideo);
#endif
	//if( ( GetCurrentThreadId () == l.dwThreadID )  )
	{
#ifdef LOG_OPEN_TIMING
		lprintf( WIDE( "About to create my own stuff..." ) );
#endif
		CreateWindowStuffSizedAt( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										, hNextVideo->pWindowPos.cy);
	}
#ifdef LOG_STARTUP
	lprintf( WIDE("Resulting new window %p %p"), hNextVideo, GetNativeHandle( hNextVideo ) );
#endif
	return TRUE;
}


PVIDEO  OpenDisplaySizedAt (uint32_t attr, uint32_t wx, uint32_t wy, int32_t x, int32_t y) // if native - we can return and let the messages dispatch...
{
	PVIDEO hNextVideo;
	//lprintf( "open display..." );
	hNextVideo = New(VIDEO);
	MemSet (hNextVideo, 0, sizeof (VIDEO));
	InitializeCriticalSec( &hNextVideo->cs );

	lprintf( WIDE("(don't know from where)CreateWindow at %d,%d %dx%d"), x, y, wx, wy );
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
	Deallocate( PRENDERER, hNextVideo );
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
				//_SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, _GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) | WS_EX_TRANSPARENT );
			}
			else
				;//_SetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE, _GetWindowLong( hVideo->hWndOutput, GWL_EXSTYLE ) & ~WS_EX_TRANSPARENT );
#endif
		}

	}
}

//----------------------------------------------------------------------------

PVIDEO  OpenDisplayAboveSizedAt (uint32_t attr, uint32_t wx, uint32_t wy,
                                               int32_t x, int32_t y, PVIDEO parent)
{
	PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
	if (parent)
	{
		lprintf( WIDE("Want to reposition; had a parent to put this window above") );
		PutDisplayAbove (newvid, parent);
	}
	return newvid;
}

PVIDEO  OpenDisplayAboveUnderSizedAt (uint32_t attr, uint32_t wx, uint32_t wy,
                                               int32_t x, int32_t y, PVIDEO parent, PVIDEO barrier)
{
	PVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
	if( barrier )
	{
		// use initial SW_RESTORE instead of SW_NORMAL
		newvid->flags.bOpenedBehind = 1;
		newvid->under = barrier;

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
	}
	if (parent)
	{
		lprintf( WIDE("Want to reposition; had a parent to put this window above") );
		PutDisplayAbove (newvid, parent);
	}
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
	if( l.hVidVirtualFocused == hVideo )
	{
		l.hVidVirtualFocused = 0; // and noone gets it yet either.
	}
	DeleteLink( &l.pActiveList, hVideo );
	UnlinkVideo( hVideo );
	lprintf( WIDE("and we should be ok?") );
	hVideo->flags.bDestroy = 1;
	while( hVideo->flags.bRendering )
		Relinquish();
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

void  SizeDisplay (PVIDEO hVideo, uint32_t w, uint32_t h)
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

void  SizeDisplayRel (PVIDEO hVideo, int32_t delw, int32_t delh)
{
	if (delw || delh)
	{
		int32_t cx, cy;
		cx = hVideo->pWindowPos.cx + delw;
		cy = hVideo->pWindowPos.cy + delh;
		if (cx < 50)
			cx = 50;
		if (cy < 20)
			cy = 20;
		hVideo->pWindowPos.cx = cx;
		hVideo->pWindowPos.cy = cy;

#ifdef LOG_RESIZE
		Log2 (WIDE( "Resized display to %d,%d" ), hVideo->pWindowPos.cx,
		hVideo->pWindowPos.cy);
#endif
#ifdef LOG_ORDERING_REFOCUS
		lprintf( WIDE( "size display relative" ) );
#endif
		CreateDrawingSurface( hVideo );
	}
}

//----------------------------------------------------------------------------

void  MoveDisplay (PVIDEO hVideo, int32_t x, int32_t y)
{
#ifdef LOG_ORDERING_REFOCUS
	//lprintf( WIDE( "Move display %d,%d" ), x, y );
#endif
	if( hVideo )
	{
		if( ( hVideo->pWindowPos.x != x ) || ( hVideo->pWindowPos.y != y ) )
		{
			hVideo->pWindowPos.x = x;
			hVideo->pWindowPos.y = y;
			Translate( hVideo->transform, (RCOORD)x, (RCOORD)y, 0.0 );
			if( hVideo->flags.bShown )
			{
				// layered window requires layered output to be called to move the display.
				UpdateDisplay( hVideo );
			}
		}
	}
}

//----------------------------------------------------------------------------

void  MoveDisplayRel (PVIDEO hVideo, int32_t x, int32_t y)
{
	if (x || y)
	{
		hVideo->pWindowPos.x += x;
		hVideo->pWindowPos.y += y;
		Translate( hVideo->transform, hVideo->pWindowPos.x, hVideo->pWindowPos.y, 0 );
	}
}

//----------------------------------------------------------------------------

void  MoveSizeDisplay (PVIDEO hVideo, int32_t x, int32_t y, int32_t w,
                                     int32_t h)
{
	int32_t cx, cy;
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

void  MoveSizeDisplayRel (PVIDEO hVideo, int32_t delx, int32_t dely,
                                        int32_t delw, int32_t delh)
{
	int32_t cx, cy;
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

void  SetMousePosition (PVIDEO hVid, int32_t x, int32_t y)
{
	if( !hVid )
	{
		int newx, newy;
		hVid = l.mouse_last_vid;
			
		//lprintf( WIDE("TAGHERE") );
		if( hVid )
		{
			//InverseOpenGLMouse( hVid->camera, hVid, (RCOORD)x, (RCOORD)y, &newx, &newy );
			newx = x + hVid->pWindowPos.x;
			newy = y + hVid->pWindowPos.y;
		}
		else
		{
			newx = x;
			newy = y;
		}
		//lprintf( WIDE("%d,%d became %d,%d"), x, y, newx, newy );
#if defined( WIN32 )
		SetCursorPos( newx, newy );
#endif
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

void  GetMousePosition (int32_t * x, int32_t * y)
{
	lprintf( WIDE("This is really relative to what is looking at it ") );
	//DebugBreak();
	if (x)
		(*x) = l.real_mouse_x;
	if (y)
		(*y) = l.real_mouse_y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(int32_t * x, int32_t * y, uint32_t *b)
{
	GetMousePosition( x, y );
	if( b )
		(*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

void  SetCloseHandler (PVIDEO hVideo,
                                     CloseCallback pWindowClose,
                                     uintptr_t dwUser)
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
                                     uintptr_t dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}

void  SetHideHandler (PVIDEO hVideo,
                                     HideAndRestoreCallback pHideCallback,
                                     uintptr_t dwUser)
{
   hVideo->dwHideData = dwUser;
   hVideo->pHideCallback = pHideCallback;
}

void  SetRestoreHandler (PVIDEO hVideo,
                                     HideAndRestoreCallback pRestoreCallback,
                                     uintptr_t dwUser)
{
   hVideo->dwRestoreData = dwUser;
   hVideo->pRestoreCallback = pRestoreCallback;
}


//----------------------------------------------------------------------------
#if !defined( NO_TOUCH )
RENDER_PROC (void, SetTouchHandler) (PVIDEO hVideo,
                                     TouchCallback pTouchCallback,
                                     uintptr_t dwUser)
{
   hVideo->dwTouchData = dwUser;
   hVideo->pTouchCallback = pTouchCallback;
}
#endif
//----------------------------------------------------------------------------


void  SetRedrawHandler (PVIDEO hVideo,
                                      RedrawCallback pRedrawCallback,
                                      uintptr_t dwUser)
{
	hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		if( hVideo->flags.bShown )
		{
			l.flags.bUpdateWanted = 1;
			if( l.wake_callback )
				l.wake_callback();
		}
	}

}

//----------------------------------------------------------------------------

void  SetKeyboardHandler (PVIDEO hVideo, KeyProc pKeyProc,
                                        uintptr_t dwUser)
{
	hVideo->dwKeyData = dwUser;
	hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

void  SetLoseFocusHandler (PVIDEO hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         uintptr_t dwUser)
{
	hVideo->dwLoseFocus = dwUser;
	hVideo->pLoseFocus = pLoseFocus;
	// window will need the initial set focus if it is focused.
	if( pLoseFocus && hVideo == l.hVidVirtualFocused )
		pLoseFocus( dwUser, l.hVidVirtualFocused );
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
			Deallocate( POINTER, hVideo->pTitle );
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
	lprintf(WIDE( "Hiding the window! %p %p %p" ), hVideo, hVideo?hVideo->pAbove:0, hVideo?hVideo->pBelow:0 );
//#endif
	if( hVideo )
	{
		if( l.hVirtualCaptured == hVideo )
		{
			l.hVirtualCaptured = NULL;
			l.hVirtualCapturedPrior = NULL; // make sure to clear this so it doesn't get reset to invalid
			l.flags.bVirtualManuallyCapturedMouse = 0;
		}
		if( l.hCameraCaptured == hVideo )
		{
			l.hCameraCaptured = NULL;
			l.hCameraCapturedPrior = NULL; // make sure to clear this so it doesn't get reset to invalid
			l.flags.bCameraManuallyCapturedMouse = 0;
		}
		hVideo->flags.bHidden = 1;
		UnlinkVideo( hVideo );  // might as well take it out of the list, no keys, mouse or output allowed.
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
#ifdef WIN32
	PostThreadMessage (l.dwThreadID, WM_USER_OPEN_CAMERAS, 0, 0 );
#endif

	if( hVideo )
	{
		if( hVideo->flags.bHidden )
			PutDisplayAbove( hVideo, l.top );  // might as well take it out of the list, no keys, mouse or output allowed.
		hVideo->flags.bShown = 1;
		hVideo->flags.bHidden = 0;
	}
}

//----------------------------------------------------------------------------

void  GetDisplaySize (uint32_t * width, uint32_t * height)
{
   lprintf( WIDE("GetDisplaySize (this will pause for a display to be given to us...)") );
   GetDisplaySizeEx( 0, NULL, NULL, width, height );
}

//----------------------------------------------------------------------------

void  GetDisplayPosition (PVIDEO hVid, int32_t * x, int32_t * y,
                                        uint32_t * width, uint32_t * height)
{
	if (!hVid)
		return;
	if (width)
		*width = hVid->pWindowPos.cx;
	if (height)
		*height = hVid->pWindowPos.cy;
	if( x )
		*x = hVid->pWindowPos.x;
	if( y )
		*y = hVid->pWindowPos.y;
}

//----------------------------------------------------------------------------
LOGICAL  DisplayIsValid (PVIDEO hVid)
{
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

void  SetDisplaySize (uint32_t width, uint32_t height)
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
                                       GeneralCallback general, uintptr_t psv)
{
}
#endif
//----------------------------------------------------------------------------

void  OwnMouseEx (PVIDEO hVideo, uint32_t own DBG_PASS)
{
	if (own)
	{
		//lprintf( WIDE("Capture is set on %p %p"),l.hVirtualCaptured, hVideo );
		if( !hVideo )
		{
			hVideo = l.mouse_last_vid;
			l.hCapturedMousePhysical = hVideo;
			hVideo->flags.bCaptured = 1;
#ifdef WIN32
			SetCapture (hVideo->hWndOutput);
#endif
		}
		else
		if( hVideo->camera )
		{
			lprintf( WIDE("set physical capture here") );
		}
		else
		{
			if( !l.hVirtualCaptured )
			{
				//lprintf( "no virtual capture; set capture on %p", hVideo );
				l.hVirtualCaptured = hVideo;
				l.hVirtualCapturedPrior = hVideo; // make sure to set this so it doesn't get reset to invalid
				hVideo->flags.bCaptured = 1;
				l.flags.bVirtualManuallyCapturedMouse = 1;
			}
			else
			{
				if( l.hVirtualCaptured != hVideo )
				{
					//lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
					l.hVirtualCaptured = hVideo;
					l.hVirtualCapturedPrior = hVideo; // make sure to set this so it doesn't get reset to invalid
					hVideo->flags.bCaptured = 1;
					l.flags.bVirtualManuallyCapturedMouse = 1;
				}
				else
				{
					if( !hVideo->flags.bCaptured )
					{
						// mgiht be auto captured from display but not 
						// captured by application....
						//lprintf( WIDE("This should NEVER happen!") );
						//*(int*)0 = 0;
					}
					// should already have the capture...
				}
			}
		}
	}
	else
	{
		if( !hVideo )
		{
			l.hCapturedMousePhysical = NULL;
#if defined( WIN32 )
			ReleaseCapture();
#endif
		}
		else
		{
			if( l.hVirtualCaptured == hVideo )
			{
				if( hVideo->flags.bCaptured )
				{
					//lprintf( WIDE("No more capture.") );
					//ReleaseCapture ();
					hVideo->flags.bCaptured = 0;
					l.hVirtualCaptured = NULL;
					l.hVirtualCapturedPrior = hVideo; // make sure to set this so it doesn't get reset to invalid
					l.flags.bVirtualManuallyCapturedMouse = 0;
				}
			}
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

int  BeginCalibration (uint32_t nPoints)
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
	if( !l.hVidVirtualFocused ||
		l.hVidVirtualFocused != pRender )
	{
		if( l.hVidVirtualFocused )
		{
			if( l.hVidVirtualFocused->pLoseFocus )
				l.hVidVirtualFocused->pLoseFocus( l.hVidVirtualFocused->dwLoseFocus, l.hVidVirtualFocused );
		}
		l.hVidVirtualFocused = pRender;
	}
}

//----------------------------------------------------------------------------

 void  ForceDisplayFront ( PRENDERER pRender )
{
	if( pRender != l.top )
	{
		//lprintf( "Force some display forward %p", pRender );
		PutDisplayAbove( pRender, l.top );
	}

}

//----------------------------------------------------------------------------

 void  ForceDisplayBack ( PRENDERER pRender )
{
	// uhmm...
	lprintf( WIDE( "Force display backward." ) );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
void  UpdateDisplay (PRENDERER hVideo )
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
			//l.mouse_timer_id = (uint32_t)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)2, 100, NULL );
			//hVideo->idle_timer_id = (uint32_t)SetTimer( (HWND)hVideo->hWndOutput, (UINT_PTR)3, 100, NULL );
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
	if( l.wake_callback )
		l.wake_callback();
}

static LOGICAL CPROC DefaultExit( uintptr_t psv, uint32_t keycode )
{
	lprintf( WIDE( "Default Exit..." ) );
	BAG_Exit(0);
	return 1;
}

static LOGICAL CPROC EnableRotation( uintptr_t psv, uint32_t keycode )
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
#ifdef _WIN32
			SetCursorPos( default_camera->hVidCore->pWindowPos.x
				+ default_camera->hVidCore->pWindowPos.cx/2
				, default_camera->hVidCore->pWindowPos.y
				+ default_camera->hVidCore->pWindowPos.cy / 2 );
#endif
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

static LOGICAL CPROC CameraForward( uintptr_t psv, uint32_t keycode )
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
		UpdateMouseRays( l.mouse_x, l.mouse_y );
//      return 1;
	}
//   return 0;
	return 1;
}

static LOGICAL CPROC CameraLeft( uintptr_t psv, uint32_t keycode )
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
		UpdateMouseRays( l.mouse_x, l.mouse_y );
//      return 1;
	}
//   return 0;
	return 1;
}

static LOGICAL CPROC CameraRight( uintptr_t psv, uint32_t keycode )
{
	if( l.flags.bRotateLock )
	{
		if( IsKeyPressed( keycode ) )
			Right( l.origin, SPEED_CONSTANT );
		else
			Right( l.origin, 0.0 );
//		return 1;
		UpdateMouseRays( l.mouse_x, l.mouse_y );
	}
//	return 0;
	return 1;
}

static LOGICAL CPROC CameraRollRight( uintptr_t psv, uint32_t keycode )
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
//		return 1;
		UpdateMouseRays( l.mouse_x, l.mouse_y );
	}
//	return 0;
	return 1;
}

static LOGICAL CPROC CameraRollLeft( uintptr_t psv, uint32_t keycode )
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
//		return 1;
		UpdateMouseRays( l.mouse_x, l.mouse_y );
	}
//	return 0;
	return 1;
}

static LOGICAL CPROC CameraDown( uintptr_t psv, uint32_t keycode )
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
		UpdateMouseRays( l.mouse_x, l.mouse_y );
//		return 1;
	}
//	return 0;
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

static LOGICAL OnKey3d( WIDE("Video Render Common") )( uintptr_t psv, uint32_t key )
{
	if( IsKeyPressed( key ) )
	{
		if( ( KEY_CODE( key ) == KEY_SCROLL_LOCK ) || ( KEY_CODE( key ) == KEY_F12 ) )
		{
			EnableRotation( 0, key );
			return 1;
		}
	}
	if( l.flags.bRotateLock )
	{
		if( KEY_CODE( key ) == KEY_A )
		{
			CameraLeft( 0, key );
			return 1;
		}
		else if( KEY_CODE( key ) == KEY_D )
		{
			CameraRight( 0, key );
			return 1;
		}
		else if( KEY_CODE( key ) == KEY_W )
		{
			CameraForward( 0, key );
			return 1;
		}
		else if( KEY_CODE( key ) == KEY_S )
		{
			CameraDown( 0, key );
			return 1;
		}
		else if( KEY_CODE( key ) == KEY_Q )
		{
			CameraRollLeft( 0, key );
			return 1;
		}
		else if( KEY_CODE( key ) == KEY_E )
		{
			CameraRollRight( 0, key );
			return 1;
		}
	}
	return 0;
}

static uintptr_t OnInit3d( WIDE("Video Render Common") )(PMatrix m,PTRANSFORM c,RCOORD*identity_dept,RCOORD*aspect)
{
	// provide one of these so key can get called.
	return 1;
}



#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 46
#endif
PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
	if( l.flags.bLogRegister )
		lprintf( WIDE("Regstering video interface...") );
	RegisterInterface( 
		WIDE("vulkan.render")
		, GetDisplayInterface, DropDisplayInterface );
	RegisterInterface( 
		WIDE("vulkan.render.3d")
		, GetDisplay3dInterface, DropDisplay3dInterface );
	l.gl_image_interface = (PIMAGE_INTERFACE)GetInterface( WIDE("vulkan.image") );  // common in-memory interface...

#ifndef __ANDROID__
#ifndef UNDER_CE
#ifndef NO_TOUCH
	l.GetTouchInputInfo = (BOOL (WINAPI *)( HTOUCHINPUT, UINT, PTOUCHINPUT, int) )LoadFunction( WIDE("user32.dll"), WIDE("GetTouchInputInfo") );
	l.CloseTouchInputHandle =(BOOL (WINAPI *)( HTOUCHINPUT ))LoadFunction( WIDE("user32.dll"), WIDE("CloseTouchInputHandle") );
	l.RegisterTouchWindow = (BOOL (WINAPI *)( HWND, ULONG  ))LoadFunction( WIDE("user32.dll"), WIDE("RegisterTouchWindow") );
#endif
#endif
#endif
	// loads options describing cameras; creates camera structures and math structures

	//LoadOptions();
#ifdef __QNX__
	// gets handles to low level device information
	InitQNXDisplays();
#endif

#ifndef __ANDROID__
	BindEventToKey( NULL, KEY_F4, KEY_MOD_RELEASE|KEY_MOD_ALT, DefaultExit, 0 );
	BindEventToKey( NULL, KEY_SCROLL_LOCK, 0, EnableRotation, 0 );
	BindEventToKey( NULL, KEY_F12, 0, EnableRotation, 0 );
#endif
	//EnableLoggingOutput( TRUE );
}

//typedef struct sprite_method_tag *PSPRITE_METHOD;

PSPRITE_METHOD  EnableSpriteMethod (PRENDERER render, void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h ), uintptr_t psv )
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
static void CPROC SavePortion( PSPRITE_METHOD psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h )
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

LOGICAL CPROC PureGL2_Vidlib_AllowsAnyThreadToUpdate( void )
{
	// require draw all makes this irrelavent.
	return FALSE;
}

void CPROC PureGL2_Vidlib_SuspendSystemSleep( int suspend )
{
#ifdef WIN32
	if( suspend )
		SetThreadExecutionState( ES_DISPLAY_REQUIRED | ES_CONTINUOUS );
	else
		SetThreadExecutionState( ES_USER_PRESENT | ES_CONTINUOUS );
#endif
}

void CPROC PureGL2_Vidlib_SetDisplayCursor( CTEXTSTR cursor )
{
	/* suppose we should do something about the cursor? */
#ifdef WIN32
	SetCursor( (HCURSOR)cursor );
#endif
}

RENDER_NAMESPACE_END

