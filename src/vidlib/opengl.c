#ifndef UNDER_CE
#define NEED_REAL_IMAGE_STRUCTURE

#include <stdhdrs.h>
//#include <windows.h>    // Header File For Windows
#include <sharemem.h>
#include <timers.h>
#include <logging.h>
#ifndef UNDER_CE
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#include <GL/glu.h>        // Header File For The GLu32 Library
#endif
//#include <gl\glaux.h>    // Header File For The Glaux Library
#include <imglib/imagestruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>

#define ENABLE_ON_DC_LAYERED hDCFakeWindow
#define ENABLE_ON_DC_NORMAL hDCOutput


#include "local.h"

RENDER_NAMESPACE

void ReadBuffer( PPBO_Info pbo );
PPBO_Info SetupPBO( Image dest_buffer );

void KillGLWindow( void )
{
   // do something here...
}



int LoadGLImage( Image image, int *result )
{
	static GLuint	texture[5000];
	static int nTextures;

	glGenTextures(1, &texture[nTextures]);			// Create One Texture

	// Create Linear Filtered Texture
	glBindTexture(GL_TEXTURE_2D, texture[nTextures]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, image->real_width, image->real_height
					, 0, GL_RGBA, GL_UNSIGNED_BYTE
					, image->image );
	return nTextures++;
}

int IsVidThread( void );

RENDER_PROC( int, SetActiveGLDisplayView )( PVIDEO hDisplay, int nFracture )
{
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
			if( hDisplay->flags.bLayeredWindow )
            hdcEnable = hDisplay->ENABLE_ON_DC_LAYERED;
			else
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
			lprintf( WIDE( "Prior GL Context being released." ) );
#endif
			lprintf( WIDE( "swapping buffer..." ) );
			glFlush();
			if( _hDisplay->flags.bLayeredWindow )
			{
				SwapBuffers( _hDisplay->hDCFakeWindow );
				//ReadBuffer( _hDisplay->PBO );
			}
         else
				SwapBuffers( _hDisplay->hDCOutput );

			lprintf( WIDE( "Steal surface from fake into my bitmap..." ) );


         lprintf( WIDE( "Read from buffer is how Slow?" ) );
			//BitBlt ((HDC)_hDisplay->hDCFakeBitmap, 0, 0, _hDisplay->pWindowPos.cx, _hDisplay->pWindowPos.cy,
			//		  (HDC)_hDisplay->hDCFakeWindow, 0, 0, SRCCOPY);
			if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
			{
				DebugBreak();
				Log1( WIDE("GetLastERror == %d"), GetLastError() );
				//MessageBox(NULL,"Can't Deactivate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;                       // Return FALSE
			}
         lprintf(WIDE( " Failure %d" ), GetLastError() );
         lprintf( WIDE( "Done with that... shared surface, right? now we can put it out?" ) );

			_hDisplay->_prior_fracture = -1;
			//lprintf( "lock-1" );
			LeaveCriticalSec( &_hDisplay->cs );
			_hDisplay = NULL;
			//LeaveCriticalSec( &cs );
		}
		LeaveCriticalSec( &cs );
	}
	return TRUE;
}

RENDER_PROC( int, SetActiveGLDisplay )( PVIDEO hDisplay )
{
   return SetActiveGLDisplayView( hDisplay, 0 );
}

//----------------------------------------------------------------------------

static int CreatePartialDrawingSurface (PVIDEO hVideo, int x, int y, int w, int h )
{
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
			BITMAPINFO bmInfo;

			bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = w; // size of window...
			bmInfo.bmiHeader.biHeight = h;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 32;   // 24, 16, ...
			bmInfo.bmiHeader.biCompression = BI_RGB;
			bmInfo.bmiHeader.biSizeImage = 0;   // zero for BI_RGB
			bmInfo.bmiHeader.biXPelsPerMeter = 0;
			bmInfo.bmiHeader.biYPelsPerMeter = 0;
			bmInfo.bmiHeader.biClrUsed = 0;
			bmInfo.bmiHeader.biClrImportant = 0;
			{
				PCOLOR pBuffer;
				hVideo->pFractures[nFracture].hBm = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer, NULL,   // hVideo (hMemView)
														  0); // offset DWORD multiple
				//lprintf( WIDE("New drawing surface, remaking the image, dispatch draw event...") );
				hVideo->pFractures[nFracture].pImage =
					RemakeImage (hVideo->pFractures[nFracture].pImage
									, pBuffer, bmInfo.bmiHeader.biWidth,
									 bmInfo.bmiHeader.biHeight);

			}
			if (!hVideo->pFractures[nFracture].hBm)
			{
				//DWORD dwError = GetLastError();
				// this is normal if window minimizes...
				if (bmInfo.bmiHeader.biWidth || bmInfo.bmiHeader.biHeight)  // both are zero on minimization
					MessageBox (hVideo->hWndOutput, WIDE("Failed to create Window DIB"),
									WIDE("ERROR"), MB_OK);
				return FALSE;
			}
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

int EnableOpenGL( PVIDEO hVideo )
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

	if( hVideo->flags.bLayeredWindow )
	{
		if( !hVideo->hWndOutputFake )
		{
			Image i = GetDisplayImage( hVideo );
			static HMODULE hMe;
			if( hMe == NULL )
				hMe = GetModuleHandle (_WIDE(TARGETNAME));
			hVideo->hWndOutputFake = CreateWindowEx( 0
																, (TEXTCHAR *) l.aClass2
																, WIDE("InvisiGlWindow")
																, WS_POPUP
																, 10000, 0
																, i->width
																, i->height
																, NULL //hVideo->hWndContainer //(HWND)l.hWndInstance  // Parent
																, NULL     // Menu
																, hMe
																, (void *) hVideo);

			hVideo->hDCFakeWindow = GetWindowDC (hVideo->hWndOutputFake );
			//hVideo->hDCFakeBitmap = CreateCompatibleDC( hVideo->hDCFakeWindow );
			//hVideo->hOldFakeBm = SelectObject( hVideo->hDCFakeBitmap
			//											, hVideo->hBm );
			ShowWindow( hVideo->hWndOutputFake, SW_NORMAL );
		}
		hdcEnable = hVideo->ENABLE_ON_DC_LAYERED;
	}
	else
	{
      hdcEnable = hVideo->ENABLE_ON_DC_NORMAL;
		if( !hVideo->ENABLE_ON_DC_NORMAL )
			UpdateDisplay( hVideo );
	}
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

	if( hVideo->flags.bLayeredWindow )
	{
		//hVideo->PBO = SetupPBO( hVideo->pImage );
	}
	if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
	{
	}

	hVideo->flags.bOpenGL = 1;
	LeaveCriticalSec( &hVideo->cs );
   return TRUE;
}

RENDER_PROC( int, EnableOpenGLView )( PVIDEO hVideo, int x, int y, int w, int h )
{

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

	if( !hVideo->hWndOutputFake )
	{
		static HMODULE hMe;
		if( hMe == NULL )
			hMe = GetModuleHandle (_WIDE(TARGETNAME));
		hVideo->hWndOutputFake = CreateWindowEx( 0
						  , (TEXTCHAR*)l.aClass2
						  , WIDE("InvisiGlWindow")
						  , WS_POPUP
						  , x, y
						  , w
						  , h
						  , NULL //hVideo->hWndContainer //(HWND)l.hWndInstance  // Parent
						  , NULL     // Menu
						  , hMe
						  , (void *) hVideo);

		hVideo->hDCFakeWindow = GetWindowDC (hVideo->hWndOutputFake );
		hVideo->hDCFakeBitmap = CreateCompatibleDC( hVideo->hDCFakeWindow );
		hVideo->hOldFakeBm = (HBITMAP)SelectObject( hVideo->hDCFakeBitmap
													, hVideo->hBm );
	}

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

	 LOGICAL pboSupported;
	 LOGICAL pboUsed;
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
    if(str) glInfo.vendor = DupCStr( str );                  // check NULL return value
    else return FALSE;

    // get renderer string
    str = (char*)glGetString(GL_RENDERER);
    if(str) glInfo.renderer = DupCStr( str );                // check NULL return value
    else return FALSE;

    // get version string
    str = (char*)glGetString(GL_VERSION);
    if(str) glInfo.version = DupCStr( str );                 // check NULL return value
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

    // get max number of lights allowed
    glGetIntegerv(GL_MAX_LIGHTS, &glInfo.maxLights);

    // get max texture resolution
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glInfo.maxTextureSize);

    // get max number of clipping planes
    glGetIntegerv(GL_MAX_CLIP_PLANES, &glInfo.maxClipPlanes);

    // get max modelview and projection matrix stacks
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &glInfo.maxModelViewStacks);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &glInfo.maxProjectionStacks);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &glInfo.maxAttribStacks);

    // get max texture stacks
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH, &glInfo.maxTextureStacks);

    return TRUE;

}

#ifdef __TEST_BUFFERS___
//#include "glext.h"
struct {
   PFNGLGENBUFFERSARBPROC pglGenBuffersARB;                     // VBO Name Generation Procedure
	PFNGLBINDBUFFERARBPROC pglBindBufferARB;                     // VBO Bind Procedure
	PFNGLBUFFERDATAARBPROC pglBufferDataARB;                     // VBO Data Loading Procedure
	PFNGLBUFFERSUBDATAARBPROC pglBufferSubDataARB;               // VBO Sub Data Loading Procedure
	PFNGLDELETEBUFFERSARBPROC pglDeleteBuffersARB;               // VBO Deletion Procedure
	PFNGLGETBUFFERPARAMETERIVARBPROC pglGetBufferParameterivARB; // return various parameters of VBO
	PFNGLMAPBUFFERARBPROC pglMapBufferARB;                       // map VBO procedure
	PFNGLUNMAPBUFFERARBPROC pglUnmapBufferARB;                   // unmap VBO procedure
#define glGenBuffersARB           ARB_Interface.pglGenBuffersARB
#define glBindBufferARB           ARB_Interface.pglBindBufferARB
#define glBufferDataARB           ARB_Interface.pglBufferDataARB
#define glBufferSubDataARB        ARB_Interface.pglBufferSubDataARB
#define glDeleteBuffersARB        ARB_Interface.pglDeleteBuffersARB
#define glGetBufferParameterivARB ARB_Interface.pglGetBufferParameterivARB
#define glMapBufferARB            ARB_Interface.pglMapBufferARB
#define glUnmapBufferARB          ARB_Interface.pglUnmapBufferARB

} ARB_Interface;

LOGICAL IsExtensionSupported( CTEXTSTR name )
{
   CTEXTSTR exten;
	INDEX idx;
	LIST_FORALL( glInfo.extensions, idx, CTEXTSTR, exten )
	{
		if( StrCmp( name, exten ) == 0 )
         return TRUE;
	}
   return FALSE;
}

PPBO_Info SetupPBO( Image dest_buffer )
{
    //initSharedMem();

    // init GLUT and GL
    //initGLUT(argc, argv);
    //initGL();
		SetupInfo();
    // get OpenGL info
    //glInfo glInfo;
    //glInfo.getInfo();
    //glInfo.printSelf();

#ifdef _WIN32
   if( !glGenBuffersARB )
    // check PBO is supported by your video card
    if(IsExtensionSupported(WIDE("GL_ARB_pixel_buffer_object")))
    {
        // get pointers to GL functions
        glGenBuffersARB = (PFNGLGENBUFFERSARBPROC)wglGetProcAddress("glGenBuffersARB");
        glBindBufferARB = (PFNGLBINDBUFFERARBPROC)wglGetProcAddress("glBindBufferARB");
        glBufferDataARB = (PFNGLBUFFERDATAARBPROC)wglGetProcAddress("glBufferDataARB");
        glBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)wglGetProcAddress("glBufferSubDataARB");
        glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)wglGetProcAddress("glDeleteBuffersARB");
        glGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
        glMapBufferARB = (PFNGLMAPBUFFERARBPROC)wglGetProcAddress("glMapBufferARB");
        glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)wglGetProcAddress("glUnmapBufferARB");

        // check once again PBO extension
        if(glGenBuffersARB && glBindBufferARB && glBufferDataARB && glBufferSubDataARB &&
           glMapBufferARB && glUnmapBufferARB && glDeleteBuffersARB && glGetBufferParameterivARB)
        {
            glInfo.pboSupported = glInfo.pboUsed = TRUE;
            //cout << "Video card supports GL_ARB_pixel_buffer_object." << endl;
        }
        else
        {
            glInfo.pboSupported = glInfo.pboUsed = FALSE;
            //cout << "Video card does NOT support GL_ARB_pixel_buffer_object." << endl;
        }
    }

#else // for linux, do not need to get function pointers, it is up-to-date
    if(glInfo.isExtensionSupported("GL_ARB_pixel_buffer_object"))
    {
        glInfo.pboSupported = glInfo.pboUsed = TRUE;
        //cout << "Video card supports GL_ARB_pixel_buffer_object." << endl;
    }
    else
    {
        glInfo.pboSupported = glInfo.pboUsed = FALSE;
        //cout << "Video card does NOT support GL_ARB_pixel_buffer_object." << endl;
    }
#endif

    if(glInfo.pboSupported)
	 {
		 PPBO_Info pbo = New(PBO_Info );
		 int DATA_SIZE = dest_buffer->height * dest_buffer->width * sizeof( CDATA );
		 pbo->index = 0;
		 pbo->dest_buffer = dest_buffer;
        // create 2 pixel buffer objects, you need to delete them when program exits.
		 // glBufferDataARB with NULL pointer reserves only memory space.
        glGenBuffersARB(PBO_COUNT, pbo->pboIds);
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo->pboIds[0]);
        glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_READ_ARB);
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo->pboIds[1]);
        glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_READ_ARB);

			  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
        return pbo;
	 }
return NULL;

}


void ReadBuffer( PPBO_Info pbo )
{

	glReadBuffer(GL_FRONT);

    if(glInfo.pboUsed) // with PBO
	 {
       int size = pbo->dest_buffer->width * pbo->dest_buffer->height * 4;
        // read framebuffer ///////////////////////////////
        //t1.start();
   //glColorMask( 1, 1, 1, 1 );
	 pbo->index = (pbo->index + 1) % 2;
	 pbo->nextIndex = (pbo->index + 1) % 2;
		 
#define PIXEL_FORMAT GL_RGBA
        // copy pixels from framebuffer to PBO
        // Use offset instead of ponter.
        // OpenGL should perform asynch DMA transfer, so glReadPixels() will return immediately.
        //lprintf( "_ Update hdisplay buffer image" )          ;
        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo->pboIds[pbo->index]);
		  lprintf( WIDE("_ Update hdisplay buffer image") )          ;


		  glReadPixels(0, 0
						  , pbo->dest_buffer->width, pbo->dest_buffer->height
						  , PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

        // measure the time reading framebuffer
        //t1.stop();
        //readTime = t1.getElapsedTimeInMilliSec();
        ///////////////////////////////////////////////////

        // process pixel data /////////////////////////////
        //t1.start();

        // map the PBO that contain framebuffer pixels before processing it
           lprintf( WIDE("a Update hdisplay buffer image") )          ;
			  glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo->pboIds[pbo->nextIndex]);
           //lprintf( "b Update hdisplay buffer image" )          ;
			  {
				  PCDATA out = (PCDATA)pbo->dest_buffer->image;
				  pbo->raw = (PCDATA)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
				  //lprintf( "c Update hdisplay buffer image" )          ;
				  if(pbo->raw)
				  {
                 /*
					  uint8_t* tmp = (uint8_t*) pbo->raw;
					  tmp+= 3;
					  {
						  int n;
						  int m = 1920 * 1200;
						  for( n = 0; n < m; n++ )
						  {
                       int a;
							  if( ( a = tmp[-3]+tmp[-2]+tmp[-1] ) >255 )
                          a = 255;
							  //tmp[0] = a;
							  out[0] = ((PCDATA)(tmp-3))[0];
							  out++;
							  tmp += 4;
						  }
					  }
                 */
					  // change brightness
					  //add(src, SCREEN_WIDTH, SCREEN_HEIGHT, shift, colorBuffer);
					  //lprintf( "Update hdisplay buffer image" )          ;
					  memcpy( pbo->dest_buffer->image,  pbo->raw, size );
					  lprintf( WIDE("Output to display") );
					  glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);     // release pointer to the mapped buffer
				  }
			  }
			  // measure the time reading framebuffer
        //t1.stop();
        //processTime = t1.getElapsedTimeInMilliSec();
        ///////////////////////////////////////////////////

        glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
    }

    else        // without PBO
    {
        // read framebuffer ///////////////////////////////
        //t1.start();

        //glReadPixels(0, 0, pbo->dest_buffer->width, pbo->dest_buffer->height, PIXEL_FORMAT, GL_UNSIGNED_BYTE, pbo->dest_buffer->image);

        // measure the time reading framebuffer
        //t1.stop();
        //readTime = t1.getElapsedTimeInMilliSec();
        ///////////////////////////////////////////////////

        // covert to greyscale ////////////////////////////
        //t1.start();

        // change brightness
        //add(colorBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, shift, colorBuffer);

        // measure the time reading framebuffer
        //t1.stop();
        //processTime = t1.getElapsedTimeInMilliSec();
        ///////////////////////////////////////////////////
    }
}

#endif

RENDER_NAMESPACE_END
#endif
