#ifndef UNDER_CE
#define NEED_REAL_IMAGE_STRUCTURE

#include <stdhdrs.h>
//#include <windows.h>    // Header File For Windows
#include <sharemem.h>
#include <timers.h>
#include <logging.h>
#include <vectlib.h>

#define ENABLE_ON_DC_NORMAL hDCOutput

#include "local.h"

RENDER_NAMESPACE

int SetActiveGLDisplayView( struct display_camera *camera, int nFracture )
{
#ifdef _WIN32 
	PRENDERER hDisplay = camera?camera->hVidCore:NULL;
	static PVIDEO _hDisplay; // last display with a lock.
	if( hDisplay )
	{
		HDC hdcEnable;
		if( nFracture )
		{
			nFracture -= 1;
			if( hDisplay->_prior_fracture != nFracture )
			{
				if(!wglMakeCurrent( (HDC)hDisplay->pFractures[nFracture].hDCBitmap
									, hDisplay->pFractures[nFracture].hRC))               // Try To Activate The Rendering Context
				{
					Log1( "GetLastERror == %d", GetLastError() );
					//MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
					return FALSE;                       // Return FALSE
				}

				if( hDisplay->pFractures[nFracture].pImage->height==0 )         // Prevent A Divide By Zero By
				{
					hDisplay->pFractures[nFracture].pImage->height=1;            // Making Height Equal One
				}
				glViewport(0,0
					,hDisplay->pFractures[nFracture].pImage->real_width
					,hDisplay->pFractures[nFracture].pImage->real_height);                // Reset The Current Viewport
				hDisplay->_prior_fracture = nFracture;
				//lprintf( "lock+1" );
				_hDisplay = hDisplay;
				//if( hDisplay->_prior_fracture == -1 )
				//	EnterCriticalSec( &cs );
			}
			else
			{
				lprintf( "Last fracture is the same as current." );
			}
		}
		else
		{
			hdcEnable = hDisplay->ENABLE_ON_DC_NORMAL;
			if( _hDisplay )
			{
				LeaveCriticalSec( &_hDisplay->cs );
				//LeaveCriticalSec( &cs ); // leave once?!
				_hDisplay = NULL;
			}
			if( !_hDisplay )
			{
				if(!wglMakeCurrent(hdcEnable,hDisplay->hRC))               // Try To Activate The Rendering Context
				{
					DebugBreak();

					Log1( "GetLastERror == %d", GetLastError() );
					//MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
					return FALSE;                       // Return FALSE
				}

				if( hDisplay->pImage->height==0 )         // Prevent A Divide By Zero By
				{
					hDisplay->pImage->height=1;            // Making Height Equal One
				}
				glViewport(0,0,hDisplay->pImage->real_width,hDisplay->pImage->real_height);                // Reset The Current Viewport

				//lprintf( "lock+1");
				//EnterCriticalSec( &cs );
				_hDisplay = hDisplay;
			}
			else
			{
				if( hDisplay == _hDisplay )
				{
					lprintf( "Active GL Context already on this renderer..." );
				}
				else
				{
					DebugBreak();
					lprintf( "Active GL context is on another Display... please wait?!" );
				}
			}
		}
	}
	else
	{
		if( _hDisplay )
		{
#ifdef LOG_OPENGL_CONTEXT
			lprintf( "Prior GL Context being released." );
#endif
			//lprintf( "swapping buffer..." );
			glFlush();
			SwapBuffers( _hDisplay->hDCOutput );

			if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
			{
				DebugBreak();
				Log1( "GetLastERror == %d", glGetError() );
				//MessageBox(NULL,"Can't Deactivate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;                       // Return FALSE
			}
			 //  lprintf(" Failure %d %d", glGetError(), GetLastError() );

			_hDisplay->_prior_fracture = -1;
			//lprintf( "lock-1" );
			_hDisplay = NULL;
			//LeaveCriticalSec( &cs );
		}
	}
	return TRUE;
#elif defined( __LINUX__ )
	{
		static struct display_camera *last_active;
		GLWindow *GLWin;
		if( last_active )
		{
			GLWin = last_active->hVidCore->x11_gl_window;
			glXSwapBuffers( GLWin->dpy, GLWin->win);
			last_active = NULL;
		}
		if( camera )
		{
			if( camera->hVidCore )
			{
				last_active = camera;
				GLWin = camera->hVidCore->x11_gl_window;
				glXMakeCurrent(GLWin->dpy, GLWin->win, GLWin->ctx);
			}
		}
	}
#endif
}

int SetActiveGLDisplay( struct display_camera *hDisplay )
{
   return SetActiveGLDisplayView( hDisplay, 0 );
}

int Init3D( struct display_camera *camera )										// All Setup For OpenGL Goes Here
{
	if( !SetActiveGLDisplay( camera ) )
		return FALSE;


	if( !camera->flags.init )
	{
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading

		glEnable( GL_ALPHA_TEST );
		glEnable( GL_BLEND );
 		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glEnable( GL_TEXTURE_2D );
 		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		glEnable(GL_NORMALIZE); // glNormal is normalized automatically....
#ifndef __ANDROID__
		//glEnable( GL_POLYGON_SMOOTH );
		//glEnable( GL_POLYGON_SMOOTH_HINT );
		glEnable( GL_LINE_SMOOTH );
		//glEnable( GL_LINE_SMOOTH_HINT );
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
#endif
 		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
 
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 
		MygluPerspective( 90.0f, camera->aspect, 1.0f, camera->depth );
		lprintf( "First GL Init Done." );
		camera->flags.init = 1;
		camera->hVidCore->flags.bReady = TRUE;
	}


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glClear(GL_COLOR_BUFFER_BIT
			  | GL_DEPTH_BUFFER_BIT
			 );	// Clear Screen And Depth Buffer

	return TRUE;										// Initialization Went OK
}


void SetupPositionMatrix( struct display_camera *camera )
{
	// camera->origin_camera is valid eye position matrix
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
#ifdef ALLOW_SETTING_GL1_MATRIX
	GetGLCameraMatrix( camera->origin_camera, camera->hVidCore->fModelView );
	glLoadMatrixf( (RCOORD*)camera->hVidCore->fModelView );
#endif
}



//----------------------------------------------------------------------------

static int CreatePartialDrawingSurface (PVIDEO hVideo, int x, int y, int w, int h )
{
#ifdef _WIN32
	int nFracture = -1;
	// can use handle from memory allocation level.....
	if (!hVideo)         // wait......
		return FALSE;
	{
		nFracture = hVideo->nFractures;
		if( !hVideo->pFractures || hVideo->nFractures == hVideo->nFracturesAvail )
		{
			hVideo->nFracturesAvail += 16;
			hVideo->pFractures =
				NewArray( struct hvideo_fracture_tag, hVideo->nFracturesAvail );
			MemSet( hVideo->pFractures + nFracture, 0, sizeof( hVideo->pFractures[0] ) * 16 );
		}
		hVideo->pFractures[nFracture].x = x;
		hVideo->pFractures[nFracture].y = y;
		hVideo->pFractures[nFracture].w = w;
		hVideo->pFractures[nFracture].h = h;

		if (!hVideo->pFractures[nFracture].hDCBitmap) // first time ONLY...
			hVideo->pFractures[nFracture].hDCBitmap = CreateCompatibleDC ((HDC)hVideo->hDCOutput);
		hVideo->pFractures[nFracture].hOldBitmap = SelectObject( (HDC)hVideo->pFractures[nFracture].hDCBitmap
																				 , hVideo->pFractures[nFracture].hBm);
		if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
		{
			struct {
				PVIDEO hVideo;
				int nFracture;
			} msg;
			msg.hVideo = hVideo;
			msg.nFracture = nFracture;
			//lprintf( "Sending redraw for fracture %d on vid %p", nFracture, hVideo );
			//SendServiceEvent( local_vidlib.dwMsgBase + MSG_RedrawFractureMethod, &msg, sizeof( msg ) );
		}
	}
	hVideo->nFractures++;
	//lprintf( "And here I might want to update the video, hope someone else does for me." );
	return nFracture + 1;
#else
	return 0;
#endif
}

//GetDisplayImageView


int ogl_EnableOpenGL( struct display_camera *camera )
{
   PVIDEO hVideo = camera->hVidCore;
	GLuint      PixelFormat;         // Holds The Results After Searching For A Match
	HDC hdcEnable;
#ifdef _WIN32
	static   PIXELFORMATDESCRIPTOR pfd=          // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR)          // Size Of This Pixel Format Descriptor
		,1                               // Version Number
		, 0  // dwFlags - set later.
		, PFD_TYPE_RGBA                        // Request An RGBA Format
		,0,                              // Select Our Color Depth
		0, 0, 0, 0, 0, 0,                   // Color Bits Ignored
		8,                               // No Alpha Buffer
		0,                               // Shift Bit Ignored
		0,                               // No Accumulation Buffer
		0, 0, 0, 0,                         // Accumulation Bits Ignored
		0,                                 // 16Bit Z-Buffer (Depth Buffer)
		0,                               // No Stencil Buffer
		0,                               // No Auxiliary Buffer
		PFD_MAIN_PLANE,                        // Main Drawing Layer
		0,                               // Reserved
		0, 0, 0                             // Layer Masks Ignored
	};
#endif
	if( !hVideo->camera )
	   return TRUE;
#ifdef _WIN32
	pfd.dwFlags =  (hVideo->flags.bLayeredWindow? PFD_DRAW_TO_BITMAP :PFD_DRAW_TO_WINDOW)                // Format Must Support Window
		|PFD_SUPPORT_OPENGL                 // Format Must Support OpenGL
		| (hVideo->flags.bLayeredWindow? 0 : PFD_DOUBLEBUFFER )// Must Support Double Buffering
      ;
	pfd.dwFlags =  PFD_DRAW_TO_WINDOW               // Format Must Support Window
		|PFD_SUPPORT_OPENGL                 // Format Must Support OpenGL
		|PFD_DOUBLEBUFFER  // Must Support Double Buffering
      ;
   // there is no choice but to use 32 bits here (64?)
	pfd.cColorBits = 32;

	hdcEnable = hVideo->ENABLE_ON_DC_NORMAL;

	if( !hVideo->ENABLE_ON_DC_NORMAL )
		ogl_UpdateDisplayEx( hVideo DBG_SRC );

	EnterCriticalSec( &hVideo->cs );
	if (!(PixelFormat=ChoosePixelFormat(hdcEnable,&pfd)))  // Did Windows Find A Matching Pixel Format?
	{
		DebugBreak();
		LeaveCriticalSec( &hVideo->cs );
		//KillGLWindow();                        // Reset The Display
		//MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if(!SetPixelFormat(hdcEnable,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
	{
		DebugBreak();
		LeaveCriticalSec( &hVideo->cs );

		//KillGLWindow();                        // Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if (!(hVideo->hRC=wglCreateContext(hdcEnable)))           // Are We Able To Get A Rendering Context?
	{
		DebugBreak();

		LeaveCriticalSec( &hVideo->cs );
		//KillGLWindow();                        // Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if(!wglMakeCurrent(hdcEnable,hVideo->hRC))               // Try To Activate The Rendering Context
	{
		lprintf( "Error: %d", GetLastError() );
	}

	if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
	{
	}

	hVideo->flags.bOpenGL = 1;
	LeaveCriticalSec( &hVideo->cs );
#endif
	return TRUE;
}


int EnableOpenGLView( struct display_camera *camera, int x, int y, int w, int h )
{
   PVIDEO hVideo = camera->hVidCore;
#ifdef _WIN32
	// enable a partial opengl area on a single window surface
	// actually turns out it's just a memory context anyhow...
	int nFracture;
	static GLuint      PixelFormat;         // Holds The Results After Searching For A Match
	static   PIXELFORMATDESCRIPTOR pfd=          // pfd Tells Windows How We Want Things To Be
	{
      sizeof(PIXELFORMATDESCRIPTOR)          // Size Of This Pixel Format Descriptor
      ,1                               // Version Number
      ,PFD_DRAW_TO_BITMAP                 // Format Must Support Window
      |PFD_SUPPORT_OPENGL                 // Format Must Support OpenGL
      //| PFD_DOUBLEBUFFER                   // Must Support Double Buffering
      , PFD_TYPE_RGBA                        // Request An RGBA Format
      ,0,                              // Select Our Color Depth
      0, 0, 0, 0, 0, 0,                   // Color Bits Ignored
      8,                               // No Alpha Buffer
      0,                               // Shift Bit Ignored
      0,                               // No Accumulation Buffer
      0, 0, 0, 0,                         // Accumulation Bits Ignored
      0,                                 // 16Bit Z-Buffer (Depth Buffer)  
      0,                               // No Stencil Buffer
      0,                               // No Auxiliary Buffer
      PFD_MAIN_PLANE,                        // Main Drawing Layer
      0,                               // Reserved
      0, 0, 0                             // Layer Masks Ignored
	};
	// there is no choice but to use 32 bits here (64?)

	if( !hVideo->flags.bOpenGL )
	{
		if( !ogl_EnableOpenGL( camera ) )
         return 0;
	}
	nFracture = CreatePartialDrawingSurface( hVideo, x, y, w, h );
	if( nFracture )
	{
		nFracture -= 1;
		if( !pfd.cColorBits )
		{
			pfd.cColorBits = 32;
			if (!(PixelFormat=ChoosePixelFormat((HDC)hVideo->pFractures[nFracture].hDCBitmap,&pfd)))  // Did Windows Find A Matching Pixel Format?
			{
				DebugBreak();
				//KillGLWindow();                        // Reset The Display
				MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
				return nFracture + 1;                       // Return FALSE
			}
		}
		if(!SetPixelFormat((HDC)hVideo->pFractures[nFracture].hDCBitmap,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
		{
			DebugBreak();
				
			//KillGLWindow();                        // Reset The Display
			MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;                       // Return FALSE
		}

		if (!(hVideo->pFractures[nFracture].hRC=wglCreateContext((HDC)hVideo->pFractures[nFracture].hDCBitmap)))           // Are We Able To Get A Rendering Context?
		{
			DebugBreak();
				
			//KillGLWindow();                        // Reset The Display
			MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;                       // Return FALSE
		}
		return nFracture + 1;
	}
#endif
	return 0;
}


void EndActive3D( struct display_camera *camera ) // does appropriate EndActiveXXDisplay
{
	SetActiveGLDisplay( NULL );
}


RENDER_NAMESPACE_END
#endif
