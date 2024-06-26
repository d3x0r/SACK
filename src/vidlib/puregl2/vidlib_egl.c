#include <stdhdrs.h>



// move local into render namespace.
#include "local.h"



#ifdef USE_EGL

void DumpSurface( void )
{
/*
#define Logit(n) lprintf( #n " = ", eglQuerySurface( n ) );
#define EGL_HEIGHT			        
#define EGL_WIDTH			
#define EGL_LARGEST_PBUFFER	
#define EGL_TEXTURE_FORMAT	
#define EGL_TEXTURE_TARGET	
#define EGL_MIPMAP_TEXTURE	
#define EGL_MIPMAP_LEVEL	
#define EGL_RENDER_BUFFER	
#define EGL_VG_COLORSPACE	
#define EGL_VG_ALPHA_FORMAT	
#define EGL_HORIZONTAL_RESOLUTION
#define EGL_VERTICAL_RESOLUTION	
#define EGL_PIXEL_ASPECT_RATIO	
#define EGL_SWAP_BEHAVIOR		
#define EGL_MULTISAMPLE_RESOLVE	
*/
}

void OpenEGL( struct display_camera *camera, NativeWindowType displayWindow )
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
		EGL_CONFORMANT,EGL_OPENGL_ES2_BIT,
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
		EGL_ALPHA_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
	const EGLint config24bpp[] =
	{
		EGL_CONFORMANT,EGL_OPENGL_ES2_BIT,
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
	const EGLint config16bpp[] =
	{
		EGL_CONFORMANT,EGL_OPENGL_ES2_BIT,
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_NONE
	};
	const EGLint* configXbpp;
	EGLint majorVersion, minorVersion;
	int numConfigs;

		lprintf( "handles: %08x %08x %08x", camera->econtext, camera->surface, camera->egl_display );

	if( camera->egl_display && camera->egl_display != EGL_NO_DISPLAY )
	{
		lprintf( "EGL Context already initialized for camera" );
		return;
	}

	camera->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	lprintf("GL display: %p %x", camera, camera->egl_display);

#ifdef __ANDROID__
	// Window surface that covers the entire screen, from libui.
	{
		//NativeWindowType (*android_cds)(void) = (NativeWindowType (*)(void))LoadFunction( "libui.so", "android_createDisplaySurface" );
		//if( android_cds )
		{
			camera->displayWindow = displayWindow;//android_cds();
			lprintf( "native window %p", camera->displayWindow );
			lprintf("Window specs: %d*%d format=%d",
					  ANativeWindow_getWidth( camera->displayWindow),
					  ANativeWindow_getHeight( camera->displayWindow),
					  ANativeWindow_getFormat( camera->displayWindow)
					 );
			camera->hVidCore->pWindowPos.cx
				= camera->w = ANativeWindow_getWidth( camera->displayWindow);
			camera->hVidCore->pWindowPos.cy
				= camera->h = ANativeWindow_getHeight( camera->displayWindow);
			camera->hVidCore->pImage =
				RemakeImage( camera->hVidCore->pImage, NULL, camera->hVidCore->pWindowPos.cx,
								camera->hVidCore->pWindowPos.cy );

			camera->identity_depth = camera->h/2;
			lprintf( "Used to mangle the transform here..." );
			//Translate( l.origin, l.scale * camera->w/2, l.scale * camera->h/2, l.scale * camera->h/2 );

			if( l.flags.bForceUnaryAspect )
				camera->aspect = 1.0;
			else
			{
				camera->aspect = ((float)camera->w/(float)camera->h);
			}
         // reload all default settings and options too.
			camera->flags.init = 0;

			switch( ANativeWindow_getFormat( camera->displayWindow) )
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
		//else
		//	lprintf( "Fatal error; cannot get android_createDisplaySurface from libui" );
	}
#endif

	eglInitialize(camera->egl_display, &majorVersion, &minorVersion);
	lprintf("GL version: %d.%d",majorVersion,minorVersion);

	if (!eglChooseConfig(camera->egl_display, configXbpp, &camera->config, 1, &numConfigs))
	{
		lprintf("eglChooseConfig failed");
		if (camera->econtext==0) lprintf("Error code: %x", eglGetError());
	}
	lprintf( "configs = %d", numConfigs );
	{
		EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

		camera->econtext = eglCreateContext(camera->egl_display,
																	 camera->config,
																	 EGL_NO_CONTEXT,
																	 context_attribs);
		lprintf("GL context: %x", camera->econtext);
		if (camera->econtext==0) lprintf("Error code: %x", eglGetError());
	}
	//ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	camera->surface = eglCreateWindowSurface(camera->egl_display,
																		camera->config,
																		camera->displayWindow,
																		NULL);
	lprintf("GL surface: %x", camera->surface);
	if (camera->surface==0) lprintf("Error code: %x", eglGetError());

   lprintf( "First swap..." );
	// makes it go black as soon as ready
	eglSwapBuffers(camera->egl_display, camera->surface);

}

void EnableEGLContext( struct display_camera *camera )
{
   static EGLDisplay prior_display;
	if( camera )
	{
		/* connect the context to the surface */
		prior_display = camera->egl_display;
		if (eglMakeCurrent(camera->egl_display
								, camera->surface
								, camera->surface
								, camera->econtext)==EGL_FALSE)
		{
			lprintf( "Make current failed: 0x%x\n", eglGetError());
			return;
		}
	}
	else
	{
		//glFinish(); //swapbuffers will do implied glFlush
		//eglWaitGL(); // same as glFinish();

		// swap should be done at end of render phase.
		//eglSwapBuffers(camera->egl_display,camera->surface);
		if( prior_display )
		{
			if (eglMakeCurrent(prior_display, EGL_NO_SURFACE, EGL_NO_SURFACE,  EGL_NO_CONTEXT)==EGL_FALSE)
			{
				lprintf( "Make current failed: 0x%x\n", eglGetError());
				return;
			}
			prior_display = NULL;
		}
	}
}

// suspend display does not notify plugins
// ; plugin notification would be to release resources on the context, but we get to keep the context this way.
void SACK_Vidlib_SuspendDisplayEx( INDEX idx )
{
   struct display_camera *camera = ( struct display_camera *)GetLink( &l.cameras, idx );
	EnableEGLContext( NULL );

	//lprintf( "Ready flag on camera: %d", camera->hVidCore->flags.bReady );
   if( camera->hVidCore->flags.bReady )
	{
      // default camera is listed twice.
		camera->hVidCore->flags.bReady = 0;
		{
			//lprintf( "handles: %08x %08x %08x", camera->econtext, camera->surface, camera->egl_display );
			if (camera->surface != EGL_NO_SURFACE)
			{
				eglDestroySurface(camera->egl_display, camera->surface);
				camera->surface = EGL_NO_SURFACE;
			}
			//lprintf( "handles: %08x %08x %08x", camera->econtext, camera->surface, camera->egl_display );
			if (camera->egl_display != EGL_NO_DISPLAY)
			{
				eglTerminate(camera->egl_display);
				camera->egl_display = EGL_NO_DISPLAY;
			}
			//lprintf( "handles: %08x %08x %08x", camera->econtext, camera->surface, camera->egl_display );
		}
	}
}

void SACK_Vidlib_SuspendDisplay( void )
{
   SACK_Vidlib_SuspendDisplayEx( 0 );

}


void SACK_Vidlib_ResumeDisplay( NativeWindowType displayWindow  )
{
	
}


void SACK_Vidlib_CloseDisplay( void )
{
	INDEX idx;
	struct display_camera *camera;
	// suspend display will have already
	//    1) set NULL render context for this thread
	//    2) destroyed the surface and display.
	//  so all that's left is to tell imagelib to release its resources in the context
	//  and release the context.
   lprintf( "Closing display (temporary); physical device no longer available..." );
	SACK_Vidlib_SuspendDisplay();
	LIST_FORALL( l.cameras, idx, struct display_camera *, camera )
	{
      // default camera is listed twice.
		if( !idx )
			continue;

      //lprintf( "Closing camera %p", camera );
		{
			struct plugin_reference *reference;
			INDEX idx2;
			LIST_FORALL( camera->plugins, idx2, struct plugin_reference *, reference )
			{
				if( reference->ExtraClose3d )
               reference->ExtraClose3d( reference->psv );
			}
		}

		// so then the context CAN exist without the display or surface...
		// but you can't put anything into it without one.
      // what happens if you put a context on a dissimilar surface?
		if (camera->econtext != EGL_NO_CONTEXT)
		{
			eglDestroyContext(camera->egl_display, camera->econtext);
			camera->econtext = EGL_NO_CONTEXT;
		}

		// this is redundant; unless something freak occurs,
		// this will already be destroyed by suspend surface
		if (camera->surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(camera->egl_display, camera->surface);
			camera->surface = EGL_NO_SURFACE;
		}

		if (camera->egl_display != EGL_NO_DISPLAY)
		{
			// this will already be destroyed by suspend surface
			eglTerminate(camera->egl_display);
			camera->egl_display = EGL_NO_DISPLAY;
		}

		camera->flags.init = 0;
	}
}

#endif


