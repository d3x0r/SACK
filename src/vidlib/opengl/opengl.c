#ifndef _OPENGL_DRIVER
#define _OPENGL_DRIVER
#endif
#define _OPENGL_ENABLED

#define NEED_REAL_IMAGE_STRUCTURE

#include <windows.h>    // Header File For Windows
#include <sharemem.h>
#include <timers.h>
#include <logging.h>
#include <gl\gl.h>         // Header File For The OpenGL32 Library
#include <gl\glu.h>        // Header File For The GLu32 Library
//#include <gl\glaux.h>    // Header File For The Glaux Library
#include <imglib/imagestruct.h>
#include <vidlib/vidstruc.h>
#include <render.h>

#define ENABLE_ON_DC hDCOutput
//hDCBitmap

RENDER_NAMESPACE

void KillGLWindow( void )
{
   // do something here...
}

#if 0
void junk( void )
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


void InitStuff( void )
{
	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping ( NEW )
	glShadeModel(GL_SMOOTH);						// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);					// Black Background
	glClearDepth(1.0f);							// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);							// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Really Nice Perspective Calculations
   //junk();
   //glEnable( GL_ARB_texture_non_power_of_two );
}

RENDER_PROC( int, SetActiveGLDisplayView )( HVIDEO hDisplay, int nFracture )
{
	static CRITICALSECTION cs;
	static HVIDEO _hDisplay; // last display with a lock.
	int first = 1;
	if( first )
	{
		InitializeCriticalSec( &cs );
		first = 0;
	}
	if( hDisplay )
	{
		EnterCriticalSec( &cs );
		EnterCriticalSec( &hDisplay->cs );
		if( nFracture )
		{
			nFracture -= 1;
			if( hDisplay->_prior_fracture != nFracture )
			{
				if(!wglMakeCurrent( (HDC)hDisplay->/*pFractures[nFracture].*/ENABLE_ON_DC
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
				//glViewport(0,0
				//	,hDisplay->pFractures[nFracture].pImage->real_width
				//	,hDisplay->pFractures[nFracture].pImage->real_height);                // Reset The Current Viewport
				hDisplay->_prior_fracture = nFracture;
				lprintf( WIDE("lock+1") );
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
			if( _hDisplay )
			{
				LeaveCriticalSec( &_hDisplay->cs );
				//LeaveCriticalSec( &cs ); // leave once?!
            _hDisplay = NULL;
			}
			if( !_hDisplay )
			{
				if(!wglMakeCurrent((HDC)hDisplay->ENABLE_ON_DC,hDisplay->hRC))               // Try To Activate The Rendering Context
				{
					DebugBreak();

					Log1( WIDE("GetLastERror == %d"), GetLastError() );
					//MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
					return FALSE;                       // Return FALSE
				}
            InitStuff();
				if( hDisplay->pImage->height==0 )         // Prevent A Divide By Zero By
				{
					hDisplay->pImage->height=1;            // Making Height Equal One
				}
				//glViewport(0,0,hDisplay->pImage->real_width,hDisplay->pImage->real_height);                // Reset The Current Viewport
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
			//SwapBuffers( _hDisplay->hRC );
			if(!wglMakeCurrent( NULL, NULL) )               // Try To Deactivate The Rendering Context
			{
				DebugBreak();
				Log1( WIDE("GetLastERror == %d"), GetLastError() );
				//MessageBox(NULL,"Can't Deactivate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;                       // Return FALSE
			}
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

RENDER_PROC( int, SetActiveGLDisplay )( HVIDEO hDisplay )
{
   return SetActiveGLDisplayView( hDisplay, 0 );
}

//----------------------------------------------------------------------------

static int CreatePartialDrawingSurface (HVIDEO hVideo, int x, int y, int w, int h )
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
		InitStuff();
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
				HVIDEO hVideo;
				int nFracture;
			} msg;
			msg.hVideo = hVideo;
			msg.nFracture = nFracture;
			//lprintf( WIDE("Sending redraw for fracture %d on vid %p"), nFracture, hVideo );
			//SendServiceEvent( l.dwMsgBase + MSG_RedrawFractureMethod, &msg, sizeof( msg ) );
		}
	}
   hVideo->nFractures++;
   //lprintf( WIDE("And here I might want to update the video, hope someone else does for me.") );
	return nFracture + 1;
}

//GetDisplayImageView


RENDER_PROC( int, EnableOpenGLView )( HVIDEO hVideo, int x, int y, int w, int h )
{

	// enable a partial opengl area on a single window surface
	// actually turns out it's just a memory context anyhow...
   int nFracture;
   static GLuint      PixelFormat;         // Holds The Results After Searching For A Match
   static   PIXELFORMATDESCRIPTOR pfd=          // pfd Tells Windows How We Want Things To Be
   {
      sizeof(PIXELFORMATDESCRIPTOR)          // Size Of This Pixel Format Descriptor
      ,1                               // Version Number
      ,PFD_DRAW_TO_WINDOW                 // Format Must Support Window
      |PFD_SUPPORT_OPENGL                 // Format Must Support OpenGL
      //| PFD_DOUBLEBUFFER                   // Must Support Double Buffering
      , PFD_TYPE_RGBA                        // Request An RGBA Format
      ,0,                              // Select Our Color Depth
      0, 0, 0, 0, 0, 0,                   // Color Bits Ignored
      0,                               // No Alpha Buffer
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

	nFracture = CreatePartialDrawingSurface( hVideo, x, y, w, h );
	if( nFracture )
	{
      nFracture -= 1;
		if( !pfd.cColorBits )
		{
			pfd.cColorBits = 32;
			if (!(PixelFormat=ChoosePixelFormat((HDC)hVideo->/*pFractures[nFracture].*/ENABLE_ON_DC,&pfd)))  // Did Windows Find A Matching Pixel Format?
			{
				DebugBreak();
				KillGLWindow();                        // Reset The Display
				MessageBox(NULL,WIDE("Can't Find A Suitable PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
				return FALSE;                       // Return FALSE
			}
		}
		if(!SetPixelFormat((HDC)hVideo->/*pFractures[nFracture].*/ENABLE_ON_DC,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
		{
			DebugBreak();
				
			KillGLWindow();                        // Reset The Display
			MessageBox(NULL,WIDE("Can't Set The PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
			return FALSE;                       // Return FALSE
		}

		if (!(hVideo->pFractures[nFracture].hRC=wglCreateContext((HDC)hVideo->/*pFractures[nFracture].*/ENABLE_ON_DC)))           // Are We Able To Get A Rendering Context?
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

RENDER_PROC( int, EnableOpenGL )( HVIDEO hVideo )
{
   GLuint      PixelFormat;         // Holds The Results After Searching For A Match
   static   PIXELFORMATDESCRIPTOR pfd=          // pfd Tells Windows How We Want Things To Be
   {
      sizeof(PIXELFORMATDESCRIPTOR)          // Size Of This Pixel Format Descriptor
      ,1                               // Version Number
      ,PFD_DRAW_TO_WINDOW                 // Format Must Support Window
      |PFD_SUPPORT_OPENGL                 // Format Must Support OpenGL
      //| PFD_DOUBLEBUFFER                   // Must Support Double Buffering
      //, PFD_TYPE_RGBA                        // Request An RGBA Format
      , PFD_TYPE_RGBA                        // Request An RGBA Format
      ,0,                              // Select Our Color Depth
      0, 0, 0, 0, 0, 0,                   // Color Bits Ignored
      0,                               // No Alpha Buffer
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
   pfd.cColorBits = 32;

	if( !hVideo->ENABLE_ON_DC )
      UpdateDisplay( hVideo );
	EnterCriticalSec( &hVideo->cs );
   if (!(PixelFormat=ChoosePixelFormat((HDC)hVideo->ENABLE_ON_DC,&pfd)))  // Did Windows Find A Matching Pixel Format?
   {
	   DebugBreak();
LeaveCriticalSec( &hVideo->cs );				
      KillGLWindow();                        // Reset The Display
      MessageBox(NULL,WIDE("Can't Find A Suitable PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
      return FALSE;                       // Return FALSE
   }

   if(!SetPixelFormat((HDC)hVideo->ENABLE_ON_DC,PixelFormat,&pfd))     // Are We Able To Set The Pixel Format?
   {
	   DebugBreak();
LeaveCriticalSec( &hVideo->cs );				
				
      KillGLWindow();                        // Reset The Display
      MessageBox(NULL,WIDE("Can't Set The PixelFormat."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
      return FALSE;                       // Return FALSE
   }

   if (!(hVideo->hRC=wglCreateContext((HDC)hVideo->ENABLE_ON_DC)))           // Are We Able To Get A Rendering Context?
   {
	   DebugBreak();
				
LeaveCriticalSec( &hVideo->cs );				
      KillGLWindow();                        // Reset The Display
      MessageBox(NULL,WIDE("Can't Create A GL Rendering Context."),WIDE("ERROR"),MB_OK|MB_ICONEXCLAMATION);
      return FALSE;                       // Return FALSE
	}
   SetActiveGLDisplay( hVideo );
LeaveCriticalSec( &hVideo->cs );				
   return TRUE;
}

RENDER_NAMESPACE_END
