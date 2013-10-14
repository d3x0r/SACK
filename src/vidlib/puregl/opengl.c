#ifndef UNDER_CE
#define NEED_REAL_IMAGE_STRUCTURE

#include <stdhdrs.h>
//#include <windows.h>    // Header File For Windows
#include <sharemem.h>
#include <timers.h>
#include <logging.h>
#include <vectlib.h>

//#include <gl\glaux.h>    // Header File For The Glaux Library

//#define LOG_OPENGL_CONTEXT

//#define ENABLE_ON_DC_LAYERED hDCFakeWindow
//#define ENABLE_ON_DC_LAYERED hDCFakeWindow
#define ENABLE_ON_DC_NORMAL hDCOutput

//#define ENABLE_ON_DC hDCBitmap
//#define ENABLE_ON_DC hDCOutput
//hDCBitmap


#include "local.h"

RENDER_NAMESPACE


void KillGLWindow( void )
{
   // do something here...
}

int IsVidThread( void );



RENDER_PROC( int, SetActiveGLDisplayView )( PVIDEO hDisplay, int nFracture )
{
#ifdef _WIN32 
	static CRITICALSECTION cs;
	static int first = 1;
	static PVIDEO _hDisplay; // last display with a lock.
	if( first )
	{
		InitializeCriticalSec( &cs );
		first = 0;
	}
	if( hDisplay )
	{
		HDC hdcEnable;
		//if( !IsVidThread() )
		//	return 0;
		EnterCriticalSec( &cs );
		EnterCriticalSec( &hDisplay->cs );
		if( nFracture )
		{
			nFracture -= 1;
			if( hDisplay->_prior_fracture != nFracture )
			{
				if(!wglMakeCurrent( (HDC)hDisplay->pFractures[nFracture].hDCBitmap
									, hDisplay->pFractures[nFracture].hRC))               // Try To Activate The Rendering Context
				{
					Log1( WIDE("GetLastERror == %d"), GetLastError() );
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
				//lprintf( WIDE("lock+1") );
				_hDisplay = hDisplay;
				//if( hDisplay->_prior_fracture == -1 )
				//	EnterCriticalSec( &cs );
			}
			else
			{
				lprintf( WIDE( "Last fracture is the same as current." ) );
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

					Log1( WIDE("GetLastERror == %d"), GetLastError() );
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
					lprintf( WIDE("Active GL Context already on this renderer...") );
				}
				else
				{
					DebugBreak();
					lprintf( WIDE("Active GL context is on another Display... please wait?!") );
				}
			}
		}
	}
	else
	{
		//__try {
		//glFlush();
		//}
		//__except ( EXCEPTION_EXECUTE_HANDLER ) {
		//	lprintf( "gl stack fucked.  Wonder where we can recover." );
		//}
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
				Log1( WIDE("GetLastERror == %d"), glGetError() );
				//MessageBox(NULL,"Can't Deactivate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;                       // Return FALSE
			}
			 //  lprintf(" Failure %d %d", glGetError(), GetLastError() );

			_hDisplay->_prior_fracture = -1;
			//lprintf( "lock-1" );
			LeaveCriticalSec( &_hDisplay->cs );
			_hDisplay = NULL;
			//LeaveCriticalSec( &cs );
		}
		LeaveCriticalSec( &cs );
	}
	return TRUE;
#elif defined( __LINUX__ )
    
#endif
}

int SetActiveGLDisplay( PVIDEO hDisplay )
{
   return SetActiveGLDisplayView( hDisplay, 0 );
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
			hVideo->pFractures =
#ifdef __cplusplus
				(struct HVIDEO_tag::fracture_tag*)
#endif
				Allocate( sizeof( hVideo->pFractures[0] ) * ( hVideo->nFracturesAvail += 16 ) );
			MemSet( hVideo->pFractures + nFracture, 0, sizeof( hVideo->pFractures[0] ) * 16 );
		}
		hVideo->pFractures[nFracture].x = x;
		hVideo->pFractures[nFracture].y = y;
		hVideo->pFractures[nFracture].w = w;
		hVideo->pFractures[nFracture].h = h;
		{
		}
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
			//lprintf( WIDE("Sending redraw for fracture %d on vid %p"), nFracture, hVideo );
			//SendServiceEvent( local_vidlib.dwMsgBase + MSG_RedrawFractureMethod, &msg, sizeof( msg ) );
		}
	}
	hVideo->nFractures++;
	//lprintf( WIDE("And here I might want to update the video, hope someone else does for me.") );
	return nFracture + 1;
#else
	return 0;
#endif
}

//GetDisplayImageView



#if 0
{
	// sample copied from
	// http://developer.nvidia.com/attach/6533
	// which is a PDF detailing pbuffer creation on an NVIDIA system

	PFNWGLEXTENSIONSSTRINGARBPROC
		wglGetExtensionsStringARB =(PFNWGLEXTENSIONSSTRINGARBPROC)
		wglGetProcAddress( WIDE("wglGetExtensionsStringARB") );
	if( wglGetExtensionsStringARB )
	{
		const Glubyte *extensions = (const Glubyte *)
			wglGetExtensionsStringARB( wglGetCurrentDC() );
		if( strstr( extensions, WIDE("WGL_ARB_pixel_format") ) &&
			strstr( extension, WIDE("WGL_ARB_pbuffer") ) )
		{
#define INIT_ENTRY_POINT( funcname, type ) \
	{ type funcname = (type) wglGetProcAddress(#funcname); \
	if ( !funcname ) \
	frintf( stderr, #funcname"() not initialized\n" ); \
			return 0;                                    \
		}
			/* Initialize WGL_ARB_pbuffer entry points. */
			INIT_ENTRY_POINT(
								  wglCreatePbufferARB, PFNWGLCREATEPBUFFERARBPROC );
			INIT_ENTRY_POINT(
								  wglGetPbufferDCARB, PFNWGLGETPBUFFERDCARBPROC );
			INIT_ENTRY_POINT(
								  wglReleasePbufferDCARB, PFNWGLRELEASEPBUFFERDCARBPROC );
			INIT_ENTRY_POINT(
								  wglDestroyPbufferARB, PFNWGLDESTROYPBUFFERARBPROC );
			INIT_ENTRY_POINT(
								  wglQueryPbufferARB, PFNWGLQUERYPBUFFERARBPROC );
			/* Initialize WGL_ARB_pixel_format entry points. */
			INIT_ENTRY_POINT(
								  wglGetPixelFormatAttribivARB,
								  PFNWGLGETPIXELFORMATATTRIBIVARBPROC );
			INIT_ENTRY_POINT(
								  wglGetPixelFormatAttribfvARB,
								  PFNWGLGETPIXELFORMATATTRIBFVARBPROC );
			INIT_ENTRY_POINT(
								  wglChoosePixelFormatARB,
								  PFNWGLCHOOSEPIXELFORMATARBPROC );

			{
				HDC hdc = wglGetCurrentDC();
				// Get ready to query for a suitable pixel format that meets our
				// minimum requirements.
				int iattributes[2*MAX_ATTRIBS];
				float fattributes[2*MAX_ATTRIBS];
				int nfattribs = 0;
				int niattribs = 0;
				// Attribute arrays must be “0” terminated – for simplicity, first
				// just zero-out the array then fill from left to right.
				for ( int a = 0; a < 2*MAX_ATTRIBS; a++ )
				{
					iattributes[a] = 0;
					fattributes[a] = 0;
				}
				// Since we are trying to create a pbuffer, the pixel format we
				// request (and subsequently use) must be “p-buffer capable”.
				iattributes[2*niattribs ] = WGL_DRAW_TO_PBUFFER_ARB;
				iattributes[2*niattribs+1] = true;
				niattribs++;
				// We require a minimum of 24-bit depth.
				iattributes[2*niattribs ] = WGL_DEPTH_BITS_ARB;
				iattributes[2*niattribs+1] = 24;
				niattribs++;
				// We require a minimum of 8-bits for each R, G, B, and A.
				iattributes[2*niattribs ] = WGL_RED_BITS_ARB;
				iattributes[2*niattribs+1] = 8;
				niattribs++;
				iattributes[2*niattribs ] = WGL_GREEN_BITS_ARB;
				iattributes[2*niattribs+1] = 8;
				niattribs++;
				iattributes[2*niattribs ] = WGL_BLUE_BITS_ARB;
				iattributes[2*niattribs+1] = 8;
				niattribs++;
				iattributes[2*niattribs ] = WGL_ALPHA_BITS_ARB;
				iattributes[2*niattribs+1] = 8;
				niattribs++;
				// Now obtain a list of pixel formats that meet these minimum
				// requirements.
				{
					int pformat[MAX_PFORMATS];
					unsigned int nformats;
					if ( !wglChoosePixelFormatARB( hdc, iattributes, fattributes,
															MAX_PFORMATS, pformat, &nformats ) )
					{
						fprintf( stderr, "pbuffer creation error: Couldn't find a \
								  suitable pixel format.\n" );
						exit( -1 );
					}
					{
						HPBUFFERARB hbuffer =
							wglCreatePbufferARB( hdc, pformat[0], iwidth,
													  iheight, iattribs );
						HDC hpbufdc = wglGetPbufferDCARB( hbuffer );
						{
                     // get size of image created.
							wglQueryPbufferARB( hbuffer, WGL_PBUFFER_WIDTH_ARB, &h );
							wglQueryPbufferARB( hbuffer, WGL_PBUFFER_WIDTH_ARB, &w );
						}
						HRC pbufglctx = wglCreateContext( hpbufdc );
                  //wglMakeCurrent( hpbufdc, pbufglctx );
					}
				}
			}
		}
	}

	{
		int flag = 0;
		wglQueryPbufferARB( hbuffer, WGL_PBUFFER_LOST_ARB,
								 &flag);
		if( flag )
		{
			// rebuild context
		}
	}

	//wglDeleteContext( pbufglctx );
	//wglReleasePbufferDCARB( hbuffer, hpbufdc );
	//wglDestroyPbufferARB( hbuffer );

}

#endif
RENDER_PROC( int, EnableOpenGLView )( PVIDEO hVideo, int x, int y, int w, int h )
{
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
		if( !EnableOpenGL( hVideo ) )
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
				KillGLWindow();                        // Reset The Display
				MessageBox(NULL,WIDE("Can't Find A Suitable PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
				return nFracture + 1;                       // Return FALSE
			}
		}
		if(!SetPixelFormat((HDC)hVideo->pFractures[nFracture].hDCBitmap,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
		{
			DebugBreak();
				
			KillGLWindow();                        // Reset The Display
			MessageBox(NULL,WIDE("Can't Set The PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
			return FALSE;                       // Return FALSE
		}

		if (!(hVideo->pFractures[nFracture].hRC=wglCreateContext((HDC)hVideo->pFractures[nFracture].hDCBitmap)))           // Are We Able To Get A Rendering Context?
		{
			DebugBreak();
				
			KillGLWindow();                        // Reset The Display
			MessageBox(NULL,WIDE("Can't Create A GL Rendering Context."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
			return FALSE;                       // Return FALSE
		}
		return nFracture + 1;
	}
	return 0;
}

RENDER_PROC( int, EnableOpenGL )( PVIDEO hVideo )
{
	GLuint      PixelFormat;         // Holds The Results After Searching For A Match
	HDC hdcEnable;
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

	if( !hVideo->camera )
	   return TRUE;
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
		UpdateDisplay( hVideo );

	EnterCriticalSec( &hVideo->cs );
	if (!(PixelFormat=ChoosePixelFormat(hdcEnable,&pfd)))  // Did Windows Find A Matching Pixel Format?
	{
		DebugBreak();
		LeaveCriticalSec( &hVideo->cs );
		//KillGLWindow();                        // Reset The Display
		//MessageBox(NULL,WIDE("Can't Find A Suitable PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if(!SetPixelFormat(hdcEnable,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
	{
		DebugBreak();
		LeaveCriticalSec( &hVideo->cs );

		KillGLWindow();                        // Reset The Display
		MessageBox(NULL,WIDE("Can't Set The PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if (!(hVideo->hRC=wglCreateContext(hdcEnable)))           // Are We Able To Get A Rendering Context?
	{
		DebugBreak();

		LeaveCriticalSec( &hVideo->cs );
		KillGLWindow();                        // Reset The Display
		MessageBox(NULL,WIDE("Can't Create A GL Rendering Context."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                       // Return FALSE
	}

	if(!wglMakeCurrent(hdcEnable,hVideo->hRC))               // Try To Activate The Rendering Context
	{
		lprintf( WIDE("Error: %d"), GetLastError() );
	}

	if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
	{
	}

	hVideo->flags.bOpenGL = 1;
	LeaveCriticalSec( &hVideo->cs );
#endif
	return TRUE;
}

struct {
	CTEXTSTR vendor;
    CTEXTSTR renderer;
    CTEXTSTR version;
    PLIST extensions;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int maxTextureSize;
    int maxLights;
    int maxAttribStacks;
    int maxModelViewStacks;
    int maxProjectionStacks;
    int maxClipPlanes;
    int maxTextureStacks;

    // ctor, init all members
    //glInfo() : redBits(0), greenBits(0), blueBits(0), alphaBits(0), depthBits(0),
    //           stencilBits(0), maxTextureSize(0), maxLights(0), maxAttribStacks(0),
    //           maxModelViewStacks(0), maxClipPlanes(0), maxTextureStacks(0) {}

    //bool getInfo();                             // extract info
    //void printSelf();                           // print itself
    //bool isExtensionSupported(const char* ext); // check if a extension is supported

} glInfo;

LOGICAL SetupInfo( void )
{
    char* str = 0;
    char* tok = 0;
	 
       // already setup.
		 if( glInfo.vendor )
          return TRUE;


    // get vendor string
    str = (char*)glGetString(GL_VENDOR);
    if(str) glInfo.vendor = DupCharToText( str );                  // check NULL return value
    else return FALSE;

    // get renderer string
    str = (char*)glGetString(GL_RENDERER);
    if(str) glInfo.renderer = DupCharToText( str );                // check NULL return value
    else return FALSE;

    // get version string
    str = (char*)glGetString(GL_VERSION);
    if(str) glInfo.version = DupCharToText( str );                 // check NULL return value
    else return FALSE;

    // get all extensions as a string
    str = (char*)glGetString(GL_EXTENSIONS);

    // split extensions
    if(str)
	{
		char *tmp = strdup( str );
		tok = strtok((char*)tmp, " ");
		while(tok)
		{
			AddLink( &glInfo.extensions, tok );
            tok = strtok(0, " ");               // next token
        }
    }
    else
    {
		return FALSE;
	}
		
    // sort extension by alphabetical order
    //std::sort(glInfo.extensions.begin(), glInfo.extensions.end());

    // get number of color bits
    glGetIntegerv(GL_RED_BITS, &glInfo.redBits);
    glGetIntegerv(GL_GREEN_BITS, &glInfo.greenBits);
    glGetIntegerv(GL_BLUE_BITS, &glInfo.blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &glInfo.alphaBits);

    // get depth bits
    glGetIntegerv(GL_DEPTH_BITS, &glInfo.depthBits);

    // get stecil bits
    glGetIntegerv(GL_STENCIL_BITS, &glInfo.stencilBits);
	 
#ifndef USE_GLES2
    // get max number of lights allowed
		 glGetIntegerv(GL_MAX_LIGHTS, &glInfo.maxLights);
#endif

    // get max texture resolution
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glInfo.maxTextureSize);

#ifndef USE_GLES2
    // get max number of clipping planes
    glGetIntegerv(GL_MAX_CLIP_PLANES, &glInfo.maxClipPlanes);

    // get max modelview and projection matrix stacks
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &glInfo.maxModelViewStacks);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &glInfo.maxProjectionStacks);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &glInfo.maxAttribStacks);

    // get max texture stacks
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &glInfo.maxTextureStacks);

#endif
	return TRUE;

}

RENDER_NAMESPACE_END
#endif
