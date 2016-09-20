#ifndef _OPENGL_DRIVER
#define _OPENGL_DRIVER
#endif
#define _OPENGL_ENABLED

#define TEST_REFRESH_WITH_ROTATE

#include <stdhdrs.h>
#include <gl\gl.h>         // Header File For The OpenGL32 Library
#include <gl\glu.h>        // Header File For The GLu32 Library
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
#include <image.h>

//#define LOG_ORDERING_REFOCUS
//#define LOG_MOUSE_EVENTS
//#define LOG_RECT_UPDATE
//#define LOG_DESTRUCTION
//#define LOG_STARTUP
//#define LOG_FOCUSEVENTS

// related symbol needs to be defined in KEYDEFS.C
//#define LOG_KEY_EVENTS

#include <vidlib/vidstruc.h>
#include "VidLib.H"

#include "Keybrd.h"

#define VIDLIB_MAIN
#include "local.h"

//HWND     hWndLastFocus;

// commands to the video thread for non-windows native ....
#define CREATE_VIEW 1


struct saved_location
{
	int32_t x, y;
	uint32_t w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
   Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h );
   uintptr_t psv;
};

RENDER_NAMESPACE

extern KEYDEFINE KeyDefs[];

//----------------------------------------------------------------------------

void CPROC TickDisplay( uintptr_t psv )
{
   HVIDEO hVideo = (HVIDEO)psv;
	/* on a seperate frame timer, output the display image... */
#ifdef TEST_REFRESH_WITH_ROTATE
         static float xrot, yrot, zrot;
#endif
#ifdef LOG_RECT_UPDATE
			//_xlprintf( 1 DBG_RELAY )( WIDE("Draw to Window: %d %d %d %d"), x, y, w, h );
#endif
			EnterCriticalSec( &hVideo->cs );
			//DebugBreak();

         SetActiveGLDisplayView( hVideo, 0 );
			{
				// begin vis persp
				{
					//if( mode != MODE_PERSP )
					{
						//mode = MODE_PERSP;
						glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
						glLoadIdentity();									// Reset The Projection Matrix
						// Calculate The Aspect Ratio Of The Window
						//gluPerspective(90.0f,1.0f,0.1f,3000.0f);
						//glOrtho( 0, hVideo->pAppImage->width, 0, hVideo->pAppImage->height, -100, 100 );
						glOrtho( -1, 1, -1, 1, -100, 100 );
						//glViewport(0, 0, 1024, 768);
						glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
						//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
						//glLoadIdentity();									// Reset The Modelview Matrix
  				}
				}
				{
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear Screen And Depth Buffer
					glLoadIdentity();							// Reset The Current Matrix
#ifdef TEST_REFRESH_WITH_ROTATE
					glTranslatef(-0.0f,-0.0f,-5.0f);						// Move Into The Screen 5 Units
					glRotatef(xrot,1.0f,0.0f,0.0f);						// Rotate On The X Axis
					glRotatef(yrot,0.0f,1.0f,0.0f);						// Rotate On The Y Axis
					glRotatef(zrot,0.0f,0.0f,1.0f);						// Rotate On The Z Axis
					xrot+=0.3f;								// X Axis Rotation
					yrot+=0.2f;								// Y Axis Rotation
					zrot+=0.4f;								// Z Axis Rotation
#endif

				}
			}
			glBindTexture(GL_TEXTURE_2D, hVideo->texture[0]);				// Select Our Texture
			//lprintf( "Texture.. " );
			//do_line( hVideo->pAppImage, 0, 0, 255, 255, BASE_COLOR_WHITE );
			//do_line( hVideo->pAppImage, 255, 0, 0, 255, BASE_COLOR_BLACK );
#if 0
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, hVideo->pImage->width, hVideo->pImage->height
								, GL_RGBA
							, GL_UNSIGNED_BYTE
							, hVideo->pImage->image );
			{
				int err = glGetError();
				lprintf( "GL Err: %d", err );
			}
#endif
         //lprintf( "Texture.. " );
			glBegin(GL_QUADS);
			{
			float x_size, y_size, y_size2;
			/*
			 * only a portion of the image is actually used, the rest is filled with blank space
			 *
          */
         x_size = (double)hVideo->pAppImage->width / (double)hVideo->pImage->width;
         y_size = 1.0 - (double)hVideo->pAppImage->height / (double)hVideo->pImage->height;
         y_size2 = 1.0;// - (double)hVideo->pAppImage->height / (double)hVideo->pImage->height;
			// Front Face
			//glColor4ub( 255,120,32,192 );
			//lprintf( "Texture size is %g,%g", x_size, y_size );

		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		// Back Face
#ifdef TEST_REFRESH_WITH_ROTATE
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		// Top Face
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f,  1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f,  1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		// Bottom Face
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f, -1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f, -1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		// Right face
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		// Left Face
		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
#endif
			}
	glEnd();

	glFlush();
	// make inactive
   SwapBuffers( hVideo->hRC );
         SetActiveGLDisplayView( NULL, 0 );

			// StretchBlt can take the raw data by the way...
#ifdef _OPENGL_ENABLED
			{
				int n;
				for( n = 0; n < hVideo->nFractures; n++ )
				{
					//BitBlt ( (HDC)hVideo->hDCBitmap
					//		 , hVideo->pFractures[n].x, hVideo->pFractures[n].y
					//		 , hVideo->pFractures[n].w, hVideo->pFractures[n].h
					//		 , (HDC)hVideo->pFractures[n].hDCBitmap, 0, 0, SRCCOPY);
				}
			}
#endif
			if( 0 && hVideo->sprites )
			{
				INDEX idx;
				uint32_t _w, _h;
				PSPRITE_METHOD psm;
 
				// set these to set clipping for sprite routine
#if 0
				hVideo->pImage->x = x;
				hVideo->pImage->y = y;
				_w = hVideo->pImage->width;
				hVideo->pImage->width = w;
				_h = hVideo->pImage->height;
				hVideo->pImage->height = h;
#ifdef DEBUG_TIMING
				lprintf( "Save screen..." );
#endif
				LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
				{
					if( ( psm->original_surface->width != psm->renderer->pImage->width ) ||
						( psm->original_surface->height != psm->renderer->pImage->height ) )
					{
						UnmakeImageFile( psm->original_surface );
						psm->original_surface = MakeImageFile( psm->renderer->pImage->width, psm->renderer->pImage->height );
					}
					//lprintf( "save Sprites" );
					BlotImageSized( psm->original_surface, psm->renderer->pImage
									  , x, y, w, h );
#ifdef DEBUG_TIMING
					lprintf( "Render sprites..." );
#endif
					if( psm->RenderSprites )
					{
						// if I exported the PSPRITE_METHOD structure to the image library
						// then it could itself short circuit the drawing...
                  //lprintf( "render Sprites" );
						psm->RenderSprites( psm->psv, hVideo, x, y, w, h );
					}
#ifdef DEBUG_TIMING
					lprintf( "Done render sprites...");
#endif
				}
#ifdef DEBUG_TIMING
				lprintf( "Done save screen and update spritess..." );
#endif
            hVideo->pImage->x = 0;
            hVideo->pImage->y = 0;
            hVideo->pImage->width = _w;
            hVideo->pImage->height = _h;
#endif
			}
         /*
			BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
			(HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
         */
			if( 0 && hVideo->sprites )
			{
				INDEX idx;
				PSPRITE_METHOD psm;
				struct saved_location location;
#ifdef DEBUG_TIMING 
				lprintf( "Restore Original" );
#endif
				LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
				{
					//BlotImage( psm->renderer->pImage, psm->original_surface
					//			, 0, 0 );
					while( DequeData( &psm->saved_spots, &location ) )
					{
						// restore saved data from image to here...
                  //lprintf( "Restore %d,%d %d,%d", location.x, location.y
						//					 , location.w, location.h );

						BlotImageSizedEx( hVideo->pImage, psm->original_surface
											 , location.x, location.y
											 , location.x, location.y
											 , location.w, location.h
											 , 0
											 , BLOT_COPY );

					}
				}
#ifdef DEBUG_TIMING
				lprintf( "Restored Original" );
#endif
			}
			LeaveCriticalSec( &hVideo->cs );

}


//----------------------------------------------------------------------------

RENDER_PROC (void, UpdateDisplayPortionEx)( HVIDEO hVideo
                                          , int32_t x, int32_t y
                                          , uint32_t w, uint32_t h DBG_PASS)
{
   ImageFile *pImage;
   if (hVideo
		 && (pImage = hVideo->pImage)
#ifndef _OPENGL_DRIVER
		 && hVideo->hDCBitmap && hVideo->hDCOutput
#endif
		)
	{
#ifdef LOG_RECT_UPDATE
		lprintf( "Entering from %s(%d)", pFile, nLine );
#endif
      if (!h)
         h = pImage->height;
      if (!w)
         w = pImage->width;

      if( l.flags.bLogWrites )
			_xlprintf( 1 DBG_RELAY )( WIDE("Write to Window: %d %d %d %d"), x, y, w, h );

      if (!hVideo->flags.bShown)
		{
			if( l.flags.bLogWrites )
				lprintf( WIDE("Setting shown...") );
			hVideo->flags.bShown = TRUE;
#ifdef LOG_RECT_UPDATE
			_xlprintf( 1 DBG_RELAY )( WIDE("Show Window: %d %d %d %d"), x, y, w, h );
#endif
			ShowWindow (hVideo->hWndOutput, SW_NORMAL);
         AddTimer( 33, TickDisplay, (uintptr_t)hVideo );
         //SetActiveGLDisplayView( hVideo, 0 );

			if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
				SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
			// show will generate a paint...
			// and on the paint we will draw, otherwise
         // we'll do it twice.
         //if( bCreatedhWndInstance )
         {
				SetWindowPos (hVideo->hWndOutput, hVideo->flags.bTopmost?HWND_TOPMOST:HWND_TOP, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE);
				SetForegroundWindow (hVideo->hWndOutput);
				// these things should invoke losefocus method messages...
				SetFocus( hVideo->hWndOutput );
			}
		}
      else
		{
#if 0
#ifdef TEST_REFRESH_WITH_ROTATE
         static float xrot, yrot, zrot;
#endif
#ifdef LOG_RECT_UPDATE
			_xlprintf( 1 DBG_RELAY )( WIDE("Draw to Window: %d %d %d %d"), x, y, w, h );
#endif
			EnterCriticalSec( &hVideo->cs );
			//DebugBreak();

         SetActiveGLDisplayView( hVideo, 0 );
			{
				// begin vis persp
				{
					//if( mode != MODE_PERSP )
					{
						//mode = MODE_PERSP;
						glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
						glLoadIdentity();									// Reset The Projection Matrix
						// Calculate The Aspect Ratio Of The Window
						//gluPerspective(90.0f,1.0f,0.1f,3000.0f);
						//glOrtho( 0, hVideo->pAppImage->width, 0, hVideo->pAppImage->height, -100, 100 );
						glOrtho( -1, 1, -1, 1, -100, 100 );
						//glViewport(0, 0, 1024, 768);
						glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
						//glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
						//glLoadIdentity();									// Reset The Modelview Matrix
  				}
				}
				{
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear Screen And Depth Buffer
					glLoadIdentity();							// Reset The Current Matrix
#ifdef TEST_REFRESH_WITH_ROTATE
					glTranslatef(-0.0f,-0.0f,-5.0f);						// Move Into The Screen 5 Units
					glRotatef(xrot,1.0f,0.0f,0.0f);						// Rotate On The X Axis
					glRotatef(yrot,0.0f,1.0f,0.0f);						// Rotate On The Y Axis
					glRotatef(zrot,0.0f,0.0f,1.0f);						// Rotate On The Z Axis
					xrot+=0.3f;								// X Axis Rotation
					yrot+=0.2f;								// Y Axis Rotation
					zrot+=0.4f;								// Z Axis Rotation
#endif

				}
			}
#endif
         SetActiveGLDisplayView( hVideo, 0 );
			glBindTexture(GL_TEXTURE_2D, hVideo->texture[0]);				// Select Our Texture
			lprintf( "Texture.. " );
			//do_line( hVideo->pAppImage, 0, 0, 255, 255, BASE_COLOR_WHITE );
			//do_line( hVideo->pAppImage, 255, 0, 0, 255, BASE_COLOR_BLACK );
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, hVideo->pImage->width, hVideo->pImage->height
								, GL_RGBA
							, GL_UNSIGNED_BYTE
							, hVideo->pImage->image );
			{
				int err = glGetError();
				lprintf( "GL Err: %d", err );
			}
         SetActiveGLDisplayView( NULL, 0 );
#if 0
         lprintf( "Texture.. " );
			glBegin(GL_QUADS);
			{
			float x_size, y_size, y_size2;
         x_size = (double)hVideo->pAppImage->width / (double)hVideo->pImage->width;
         y_size = 1.0 - (double)hVideo->pAppImage->height / (double)hVideo->pImage->height;
         y_size2 = 1.0;// - (double)hVideo->pAppImage->height / (double)hVideo->pImage->height;
			// Front Face
			//glColor4ub( 255,120,32,192 );
			lprintf( "Texture size is %g,%g", x_size, y_size );

		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		// Back Face
#ifdef TEST_REFRESH_WITH_ROTATE
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		// Top Face
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f,  1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f,  1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		// Bottom Face
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f, -1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f, -1.0f, -1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		// Right face
		glTexCoord2f(x_size, y_size); glVertex3f( 1.0f, -1.0f, -1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f( 1.0f,  1.0f, -1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f( 1.0f,  1.0f,  1.0f);	// Top Left Of The Texture and Quad
		glTexCoord2f(0.0f, y_size); glVertex3f( 1.0f, -1.0f,  1.0f);	// Bottom Left Of The Texture and Quad
		// Left Face
		glTexCoord2f(0.0f, y_size); glVertex3f(-1.0f, -1.0f, -1.0f);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(x_size, y_size); glVertex3f(-1.0f, -1.0f,  1.0f);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(x_size, y_size2); glVertex3f(-1.0f,  1.0f,  1.0f);	// Top Right Of The Texture and Quad
		glTexCoord2f(0.0f, y_size2); glVertex3f(-1.0f,  1.0f, -1.0f);	// Top Left Of The Texture and Quad
#endif
			}
	glEnd();

	glFlush();
	// make inactive
   SwapBuffers( hVideo->hRC );
         SetActiveGLDisplayView( NULL, 0 );

			// StretchBlt can take the raw data by the way...
#ifdef _OPENGL_ENABLED
			{
				int n;
				for( n = 0; n < hVideo->nFractures; n++ )
				{
					//BitBlt ( (HDC)hVideo->hDCBitmap
					//		 , hVideo->pFractures[n].x, hVideo->pFractures[n].y
					//		 , hVideo->pFractures[n].w, hVideo->pFractures[n].h
					//		 , (HDC)hVideo->pFractures[n].hDCBitmap, 0, 0, SRCCOPY);
				}
			}
#endif
			if( 0 && hVideo->sprites )
			{
				INDEX idx;
				uint32_t _w, _h;
				PSPRITE_METHOD psm;
 
				// set these to set clipping for sprite routine
				hVideo->pImage->x = x;
				hVideo->pImage->y = y;
				_w = hVideo->pImage->width;
				hVideo->pImage->width = w;
				_h = hVideo->pImage->height;
				hVideo->pImage->height = h;
#ifdef DEBUG_TIMING
				lprintf( "Save screen..." );
#endif
				LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
				{
					if( ( psm->original_surface->width != psm->renderer->pImage->width ) ||
						( psm->original_surface->height != psm->renderer->pImage->height ) )
					{
						UnmakeImageFile( psm->original_surface );
						psm->original_surface = MakeImageFile( psm->renderer->pImage->width, psm->renderer->pImage->height );
					}
					//lprintf( "save Sprites" );
					BlotImageSized( psm->original_surface, psm->renderer->pImage
									  , x, y, w, h );
#ifdef DEBUG_TIMING
					lprintf( "Render sprites..." );
#endif
					if( psm->RenderSprites )
					{
						// if I exported the PSPRITE_METHOD structure to the image library
						// then it could itself short circuit the drawing...
                  //lprintf( "render Sprites" );
						psm->RenderSprites( psm->psv, hVideo, x, y, w, h );
					}
#ifdef DEBUG_TIMING
					lprintf( "Done render sprites...");
#endif
				}
#ifdef DEBUG_TIMING
				lprintf( "Done save screen and update spritess..." );
#endif
            hVideo->pImage->x = 0;
            hVideo->pImage->y = 0;
            hVideo->pImage->width = _w;
            hVideo->pImage->height = _h;
			}
         /*
			BitBlt ((HDC)hVideo->hDCOutput, x, y, w, h,
			(HDC)hVideo->hDCBitmap, x, y, SRCCOPY);
         */
			if( 0 && hVideo->sprites )
			{
				INDEX idx;
				PSPRITE_METHOD psm;
				struct saved_location location;
#ifdef DEBUG_TIMING 
				lprintf( "Restore Original" );
#endif
				LIST_FORALL( hVideo->sprites, idx, PSPRITE_METHOD, psm )
				{
					//BlotImage( psm->renderer->pImage, psm->original_surface
					//			, 0, 0 );
					while( DequeData( &psm->saved_spots, &location ) )
					{
						// restore saved data from image to here...
                  //lprintf( "Restore %d,%d %d,%d", location.x, location.y
						//					 , location.w, location.h );

						BlotImageSizedEx( hVideo->pImage, psm->original_surface
											 , location.x, location.y
											 , location.x, location.y
											 , location.w, location.h
											 , 0
											 , BLOT_COPY );

					}
				}
#ifdef DEBUG_TIMING
				lprintf( "Restored Original" );
#endif
			}
			LeaveCriticalSec( &hVideo->cs );
#endif
      }
	}
	else
	{
      //lprintf( WIDE("Rendering surface is not able to be updated (no surface, hdc, bitmap, etc)") );
	}
}

//----------------------------------------------------------------------------

void
UnlinkVideo (HVIDEO hVideo)
{
   if (hVideo->pBelow)
   {
      HVIDEO pBelow = hVideo->pBelow;
      while (pBelow)
      {
         pBelow->pAbove = hVideo->pAbove;
         pBelow = pBelow->pNext;
      }
   }
   if (hVideo->pAbove)
   {
      if (hVideo->pAbove->pBelow == hVideo)
         hVideo->pAbove->pBelow = hVideo->pNext;
   }
   if (hVideo->pNext)
      hVideo->pNext->pPrior = hVideo->pPrior;
   if (hVideo->pPrior)
      hVideo->pPrior->pNext = hVideo->pNext;

   hVideo->pPrior = hVideo->pNext = hVideo->pAbove = hVideo->pBelow = NULL;
}

//----------------------------------------------------------------------------

void  FocusInLevel (HVIDEO hVideo)
{
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
         HVIDEO pCur = hVideo->pPrior;
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

RENDER_PROC (void, PutDisplayAbove) (HVIDEO hVideo, HVIDEO hAbove)
{
	UnlinkVideo (hVideo);      // make sure it's isolated...
	// all links are NULL if unlinked.
	//if( !hAbove )
   //   hAbove = l.hVidCore;
   if( ( hVideo->pAbove = hAbove ) )
   {
		if( hVideo->pNext = hAbove->pBelow )
			hVideo->pNext->pPrior = hAbove;
      hAbove->pBelow = hVideo;
   }
#ifdef LOG_ORDERING_REFOCUS
   lprintf("Put %08x above %08x and before %08x", hVideo, hAbove, hVideo->pNext );
#endif
}

uint32_t power2( uint32_t val )
{
	int n;
	for( n = 0; n < 32; n++ )
		if( val & (0x80000000UL>>n) )
			break;
   return (0x80000000 >> (n-1));
}

//----------------------------------------------------------------------------

BOOL CreateDrawingSurface (HVIDEO hVideo)
{
   // can use handle from memory allocation level.....
   if (!hVideo)         // wait......
      return FALSE;

#ifndef _OPENGL_DRIVER
   if (hVideo->hBm && hVideo->hWndOutput)
   {
      if (SelectObject ((HDC)hVideo->hDCBitmap, hVideo->hOldBitmap) != hVideo->hBm)
      {
         Log ("Hmm Somewhere we lost track of which bitmap is selected?!");
      }
      ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCBitmap);
      //hVideo->pImage = NULL;
      DeleteObject (hVideo->hBm);
   }
#endif

   {
#ifndef _OPENGL_DRIVER
		BITMAPINFO bmInfo;
#endif
      RECT r;
      if (hVideo->flags.bFull)
      {
         GetWindowRect (hVideo->hWndOutput, &r);
         r.right -= r.left;
         r.bottom -= r.top;
      }
      else
      {
         GetClientRect (hVideo->hWndOutput, &r);
      }

#ifndef _OPENGL_DRIVER
      bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
      bmInfo.bmiHeader.biWidth = r.right; // size of window...
      bmInfo.bmiHeader.biHeight = r.bottom;
      bmInfo.bmiHeader.biPlanes = 1;
      bmInfo.bmiHeader.biBitCount = 32;   // 24, 16, ...
      bmInfo.bmiHeader.biCompression = BI_RGB;
      bmInfo.bmiHeader.biSizeImage = 0;   // zero for BI_RGB
      bmInfo.bmiHeader.biXPelsPerMeter = 0;
      bmInfo.bmiHeader.biYPelsPerMeter = 0;
      bmInfo.bmiHeader.biClrUsed = 0;
		bmInfo.bmiHeader.biClrImportant = 0;
#endif
      {
#ifndef _OPENGL_DRIVER
         PCOLOR pBuffer;
			hVideo->hBm = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer
													 , NULL,   // hVideo (hMemView)
													  0); // offset DWORD multiple
#endif
			//lprintf( WIDE("New drawing surface, remaking the image, dispatch draw event...") );
			if( hVideo->pImage )
            UnmakeImageFile( hVideo->pImage );
         hVideo->pImage =
            MakeImageFile ( power2(r.right), power2(r.bottom));
			hVideo->pAppImage = MakeSubImage( hVideo->pImage, 0, 0, r.right, r.bottom );

			EnableOpenGL( hVideo );
         //SetActiveGLDisplayView( hVideo, 1 );
			glGenTextures(1, &hVideo->texture[0]);					// Create The Texture
			{
				int err = glGetError();
				if( err )
				{
					DebugBreak();
               lprintf( "Error is %d", err );
				}
			}
         glBindTexture(GL_TEXTURE_2D, hVideo->texture[0]);
			{
				int err = glGetError();
				if( err )
				{
					DebugBreak();
               lprintf( "Error is %d", err );
				}
			}
			glTexImage2D(GL_TEXTURE_2D, 0, 4, hVideo->pImage->width, hVideo->pImage->height, 0
							, GL_RGBA
							, GL_UNSIGNED_BYTE
							, hVideo->pImage->image );
			{
				int err = glGetError();
				if( err )
				{
					DebugBreak();
               lprintf( "Error is %d", err );
				}
			}
			// Generate The Texture
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	// Linear Filtering
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	// Linear Filtering

         SetActiveGLDisplay( NULL );

      }
#ifndef _OPENGL_DRIVER
      if (!hVideo->hBm)
      {
         //DWORD dwError = GetLastError();
         // this is normal if window minimizes...
         if (bmInfo.bmiHeader.biWidth || bmInfo.bmiHeader.biHeight)  // both are zero on minimization
            MessageBox (hVideo->hWndOutput, WIDE("Failed to create Window DIB"),
                        WIDE("ERROR"), MB_OK);
         return FALSE;
      }
#endif
   }
#ifndef _OPENGL_DRIVER
   if (!hVideo->hDCBitmap) // first time ONLY...
      hVideo->hDCBitmap = CreateCompatibleDC ((HDC)hVideo->hDCOutput);
   hVideo->hOldBitmap = SelectObject ((HDC)hVideo->hDCBitmap, hVideo->hBm);
#endif
   //if (hVideo->pRedrawCallback)
   //{
   //   hVideo->pRedrawCallback(hVideo->dwRedrawData, (PRENDERER) hVideo);
	//}
	if( hVideo->flags.bReady && !hVideo->flags.bHidden && hVideo->pRedrawCallback )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
		SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
	}
   //lprintf( WIDE("And here I might want to update the video, hope someone else does for me.") );
	return TRUE;
}

void DoDestroy (HVIDEO hVideo)
{
   if (hVideo)
   {
      hVideo->hWndOutput = NULL; // release window... (disallows FreeVideo in user call)
      SetWindowLong (hVideo->hWndOutput, WD_HVIDEO, 0);
      if (hVideo->pWindowClose)
      {
         hVideo->pWindowClose (hVideo->dwCloseData);
      }

#ifndef _OPENGL_DRIVER
      if (SelectObject ((HDC)hVideo->hDCBitmap, hVideo->hOldBitmap) != hVideo->hBm)
      {
         Log ("Don't think we deselected the bm right");
      }
      ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCOutput);
      ReleaseDC (hVideo->hWndOutput, (HDC)hVideo->hDCBitmap);
      if (!DeleteObject (hVideo->hBm))
      {
         Log ("Yup this bitmap is expanding...");
		}
#endif
      Release (hVideo->pTitle);
		DestroyKeyBinder( hVideo->KeyDefs );
      //hVideo->pImage->image = NULL; // cheat this out for now...
      // Image library tracks now that someone else gave it memory
      // and it does not deallocate something it didn't allocate...
      UnmakeImageFile (hVideo->pImage);

      // this will be cleared at the next statement....
      // which indicates we will be ready to be released anyhow...
		//hVideo->flags.bReady = FALSE;
      // unlink from the stack of windows...
		UnlinkVideo (hVideo);
		if( l.hCaptured == hVideo )
         l.hCaptured = NULL;
		//Log ("Cleared hVideo - is NOW !bReady");
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
#ifndef __NO_WIN32API__
int CALLBACK
VideoWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HVIDEO hVideo;
   uint32_t _mouse_b = l.mouse_b;
	//static UINT uLastMouseMsg;
   switch (uMsg)
   {
   case WM_SETFOCUS:
      {
			if( hWnd == l.hWndInstance )
			{
				HVIDEO hVidPrior = (HVIDEO)GetWindowLong( (HWND)wParam, WD_HVIDEO );
				if( hVidPrior )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("-------------------------------- GOT FOCUS --------------------------") );
					lprintf( WIDE("Instance is %p"), hWnd );
					lprintf( WIDE("prior is %p"), hVidPrior->hWndOutput );
#endif
					if( hVidPrior->pBelow )
					{
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Set to window prior was below...") );
#endif
						SetFocus( hVidPrior->pBelow->hWndOutput );
					}
					else if( hVidPrior->pNext )
					{
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Set to window prior last interrupted..") );
#endif
						SetFocus( hVidPrior->pNext->hWndOutput );
					}
					else if( hVidPrior->pPrior )
					{
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Set to window prior which interrupted us...") );
#endif
						SetFocus( hVidPrior->pPrior->hWndOutput );
					}
					else if( hVidPrior->pAbove )
					{
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Set to a window which prior was above.") );
#endif
						SetFocus( hVidPrior->pAbove->hWndOutput );
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
                  lprintf( WIDE("prior window is not around anythign!?") );
					}
#endif
				}
			}
#ifdef LOG_ORDERING_REFOCUS
         Log ("Got setfocus...");
#endif
			//SetWindowPos( l.hWndInstance, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			//SetWindowPos( hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
			hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
			l.hVidFocused = hVideo;
			if (hVideo)
			{
				if( hVideo->flags.bDestroy )
					return FALSE;
				if( !hVideo->flags.bFocused )
				{
					hVideo->flags.bFocused = 1;
					{
						static POINTER _NULL = NULL;
						SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
													, &hVideo, sizeof( hVideo )
													, &_NULL, sizeof( _NULL ) );
					}
				}
			}
			//SetFocus( l.hWndInstance );
		}
		return 1;
#ifndef __cplusplus
		break;
#endif
   case WM_KILLFOCUS:
      {
#ifdef LOG_ORDERING_REFOCUS
         lprintf("Got Killfocus new focus to %p %p", hWnd, wParam);
#endif
         l.hVidFocused = NULL;
         hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
			if (hVideo)
			{
				if( hVideo->flags.bDestroy )
					return FALSE;
				if( hVideo->flags.bFocused )
				{
					hVideo->flags.bFocused = 0;
					// clear keyboard state...
					{
						MemSet( &l.kbd, 0, sizeof( l.kbd ) );
						MemSet( &hVideo->kbd, 0, sizeof( hVideo->kbd ) );
					}
					if ( hVideo->pLoseFocus && !hVideo->flags.bHidden )
					{
						// don't really have a purpose for secondary argument...
						HVIDEO hVidRecv =
							(HVIDEO) GetWindowLong ((HWND) wParam, WD_HVIDEO);
						if (!hVidRecv)
							hVidRecv = (HVIDEO) 1;
						else
						{
							HVIDEO me = hVideo;
							//lprintf( WIDE("killfocus window thing %d"), hVidRecv );
							while( me )
							{
								if( me && me->pAbove == hVidRecv )
								{
#ifdef LOG_ORDERING_REFOCUS
									lprintf( WIDE("And - we need to stop this from happening, I'm stacked on the window getting the focus... restore it back to me") );
#endif
									//SetFocus( hVideo->hWndOutput );
									return 0;
								}
								me = me->pAbove;
							}
						}
#ifdef LOG_ORDERING_REFOCUS
						lprintf( WIDE("Dispatch lose focus callback....") );
#endif
						SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
													, &hVideo, sizeof( hVideo )
													, &hVidRecv, sizeof( hVidRecv ) );
						//hVideo->pLoseFocus (hVideo->dwLoseFocus, hVidRecv);
					}
#ifdef LOG_ORDERING_REFOCUS
					else
					{
                  lprintf( WIDE("Hidden window or lose focus was not active.") );
					}
#endif
				}
				else
				{
					lprintf( WIDE("this video window was not focused.") );
				}
			}
		}
		return 1;
#ifndef __cplusplus
		break;
#endif
   case WM_ACTIVATE:
		if( hWnd == l.hWndInstance ) {
#ifdef LOG_ORDERING_REFOCUS
			Log2 ("Activate: %08x %08x", wParam, lParam);
#endif
			if (LOWORD (wParam) == WA_ACTIVE || LOWORD (wParam) == WA_CLICKACTIVE)
			{
				hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
				if (hVideo)
				{
#ifdef LOG_ORDERING_REFOCUS
					Log2 ("Window %08x is below %08x", hVideo, hVideo->pBelow);
#endif
					if (hVideo->pBelow && hVideo->pBelow->flags.bShown)
					{
#ifdef LOG_ORDERING_REFOCUS
						Log ("Setting active window the the lower(upper?) one...");
#endif
						SetActiveWindow (hVideo->pBelow->hWndOutput);
					}
					else
					{
#ifdef LOG_ORDERING_REFOCUS
						Log ("Within same level do focus...");
#endif
						FocusInLevel (hVideo);
					}
				}
			}
		}
		return 1;
   case WM_RUALIVE:
      {
         int *alive = (int *) lParam;
         *alive = TRUE;
      }
      break;
   case WM_NCPAINT:
      hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
      if (hVideo && hVideo->flags.bFull)   // do not allow system draw...
         return 0;
      break;
   case WM_WINDOWPOSCHANGED:
      {
         // global hVideo is pPrimary Video...
         LPWINDOWPOS pwp;
         pwp = (LPWINDOWPOS) lParam;
			hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE("Being inserted after %x %x"), pwp->hwndInsertAfter, hWnd );
#endif
         if (!hVideo)      // not attached to anything...
				return 0;
			if( hVideo->flags.bHidden )
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Hmm don't really care about the motion of a hidden window...") );
#endif
				hVideo->pWindowPos = *pwp; // save always....!!!
            return 0;
			}
         if (pwp->cx != hVideo->pWindowPos.cx ||
             pwp->cy != hVideo->pWindowPos.cy)
         {
            CreateDrawingSurface (hVideo);
			}
         //lprintf( WIDE("window pos changed - new ordering includes %p for %p(%p)"), pwp->hwndInsertAfter, pwp->hwnd, hWnd );
			if( hVideo->pBelow )
			{
				HVIDEO below = hVideo->pBelow;
				if( below->hWndOutput != pwp->hwndInsertAfter )
				{
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("Steal position behind what I'm below") );
#endif
					SetWindowPos( below->hWndOutput, pwp->hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
					SetWindowPos( hWnd, below->hWndOutput, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
#ifdef LOG_ORDERING_REFOCUS
					lprintf( WIDE("Stole position am now behind my child") );
#endif
				}
#ifdef LOG_ORDERING_REFOCUS
            else
					lprintf( WIDE("Uhmm relationship is already fine.") );
#endif
            //return 0;
			}
			if( hVideo->pAbove )
			{
#ifdef LOG_ORDERING_REFOCUS
				lprintf( WIDE("Putting the window I am above below myself.") );
#endif
            SetWindowPos( hVideo->pAbove->hWndOutput, hVideo->hWndOutput
						, 0, 0, 0, 0
						, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
			}
#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE("window pos changed - new ordering includes %p for %p(%p)"), pwp->hwndInsertAfter, pwp->hwnd, hWnd );
#endif
         hVideo->pWindowPos = *pwp; // save always....!!!
#ifdef LOG_ORDERING_REFOCUS
			lprintf( WIDE("Window pos %d,%d %d,%d")
					 , hVideo->pWindowPos.x
					 , hVideo->pWindowPos.y
					 , hVideo->pWindowPos.cx
					 , hVideo->pWindowPos.cy );
#endif
		}
		return 0;         // indicate handled message... no WM_MOVE/WM_SIZE generated.

	case WM_NCMOUSEMOVE:
      // normal fall through without processing button states
		if (0)
		{
			int16_t wheel;
	case WM_MOUSEWHEEL:
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			wheel = (int16_t)(( wParam & 0xFFFF0000 ) >> 16);
			if( wheel >= 120 )
				l.mouse_b |= MK_SCROLL_UP;
			if( wheel <= -120 )
				l.mouse_b |= MK_SCROLL_DOWN;
		}
		else if (0)
		{
   case WM_NCLBUTTONDOWN:
			l.mouse_b |= MK_LBUTTON;
		}
		else if (0)
		{
   case WM_NCMBUTTONDOWN:
			l.mouse_b |= MK_MBUTTON;
		}
		else if (0)
		{
   case WM_NCRBUTTONDOWN:
			l.mouse_b |= MK_RBUTTON;
		}
		else if (0)
		{
   case WM_NCLBUTTONUP:
			l.mouse_b &= ~MK_LBUTTON;
		}
		else if (0)
		{
   case WM_NCMBUTTONUP:
			l.mouse_b &= ~MK_MBUTTON;
		}
		else if (0)
		{
   case WM_NCRBUTTONUP:
			l.mouse_b &= ~MK_RBUTTON;
		}
		else if (0)
		{
   case WM_LBUTTONDOWN:
   case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | wParam;
		}
		else if (0)
		{
   case WM_LBUTTONUP:
   case WM_MBUTTONUP:
	case WM_RBUTTONUP:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | wParam;
		}
		else if (0)
		{
   case WM_MOUSEMOVE:
			l.mouse_b = ( l.mouse_b & ~(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) | wParam;
		}

		//hWndLastFocus = hWnd;
		hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
		if (!hVideo)
			return 0;

		if( l.hCaptured )
		{
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Captured mouse already - don't do anything?") );
#endif
		}
		else
		{
			if( ( ( _mouse_b ^ l.mouse_b ) & l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON) ) )
			{
#ifdef LOG_MOUSE_EVENTS
				lprintf( WIDE("Auto owning mouse to surface which had the mouse clicked DOWN.") );
#endif
				if( !l.hCaptured )
					SetCapture( hWnd );
			}
			else if( ( (l.mouse_b & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == 0 ) )
			{
				//lprintf( WIDE("Auto release mouse from surface which had the mouse unclicked.") );
				if( !l.hCaptured )
					ReleaseCapture();
			}
		}

		{
			POINT p;
			int dx, dy;
			GetCursorPos (&p);
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position %d,%d"), p.x, p.y );
#endif
			p.x -= (dx =(l.hCaptured?l.hCaptured:hVideo)->pWindowPos.x);
			p.y -= (dy=(l.hCaptured?l.hCaptured:hVideo)->pWindowPos.y);
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Mouse position results %d,%d %d,%d"), dx, dy, p.x, p.y );
#endif
			if (!(l.hCaptured?l.hCaptured:hVideo)->flags.bFull)
			{
				p.x -= l.WindowBorder_X;
				p.y -= l.WindowBorder_Y;
			}
			l.mouse_x = p.x;
			l.mouse_y = p.y;
		}
		{
			static HCURSOR hCursor;
			if (!hCursor)
				hCursor = LoadCursor (NULL, IDC_ARROW);
			SetCursor (hCursor);
		}
		if( l.mouse_x != l._mouse_x ||
			l.mouse_y != l._mouse_y ||
			l.mouse_b != l._mouse_b ||
		   l.mouse_last_vid != hVideo ) // this hvideo!= last hvideo?
		{
			uint32_t msg[4];
			l.mouse_last_vid = hVideo;
			msg[0] = (uint32_t)(l.hCaptured?l.hCaptured:hVideo);
			msg[1] = l.mouse_x;
			msg[2] = l.mouse_y;
			msg[3] = l.mouse_b;
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("Generate mouse message %p(%p?) %d %d,%08x %d+%d=%d"), l.hCaptured, hVideo, l.mouse_x, l.mouse_y, l.mouse_b, l.dwMsgBase, MSG_MouseMethod, l.dwMsgBase + MSG_MouseMethod );
#endif
			SendServiceEvent( 0, l.dwMsgBase + MSG_MouseMethod
								 , msg
								 , sizeof( msg ) );
			l._mouse_x = l.mouse_x;
			l._mouse_y = l.mouse_y;
			// clear scroll buttons...
			// otherwise circumstances of mouse wheel followed by any other event
         // continues to generate scroll clicks.
			l.mouse_b &= ~( MK_SCROLL_UP|MK_SCROLL_DOWN);
			l._mouse_b = l.mouse_b;
		}
      /*
      if (hVideo->pMouseCallback)
      {
         if( l.mouse_x != l._mouse_x ||
            l.mouse_y != l._mouse_y ||
            l.mouse_b != l._mouse_b )
         {
            // align mouse to be in bitmap coordinates...
            //lprintf( WIDE("Mouse event from original source. %d %d %08x"), l.mouse_x, l.mouse_y, l.mouse_b );
            hVideo->pMouseCallback (hVideo->dwMouseData,
                                    l.mouse_x, l.mouse_y, l.mouse_b);
            l._mouse_x = l.mouse_x;
            l._mouse_y = l.mouse_y;
            l._mouse_b = l.mouse_b;
         }
			}
         */
      return 0;         // don't allow windows to think about this...
   case WM_PAINT:
      Log( WIDE("Paint Message!") );
      hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
      SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
      //UpdateDisplayPortion (hVideo, 0, 0, 0, 0);
      ValidateRect (hWnd, NULL);
      break;
   case WM_SYSCOMMAND:
      switch (wParam)
      {
      case SC_CLOSE:

         //DestroyWindow( hWnd );
         return TRUE;
      }
      break;
   case WM_QUERYENDSESSION:
   	return TRUE; // uhmm okay.
	case WM_DESTROY:
#ifdef LOG_DESTRUCTION
		Log( WIDE("Destroying a window...") );
#endif
      hVideo = (HVIDEO) GetWindowLong (hWnd, WD_HVIDEO);
      if (hVideo)
		{
#ifdef LOG_DESTRUCTION
			Log ("Killing the window...");
#endif
         DoDestroy (hVideo);
      }
		break;
	case WM_ERASEBKGND:
		// LIE! we don't care to ever erase the background...
		// thanx for the invite though.
      //lprintf( WIDE("Erase background, and we just say NO") );
      return TRUE;
   case WM_CREATE:
      {
         LPCREATESTRUCT pcs;
         pcs = (LPCREATESTRUCT) lParam;
         if (pcs)
         {
            hVideo = (HVIDEO)pcs->lpCreateParams; // user passed param...

            if ((LONG) hVideo == 1)
               break;

            if (!hVideo)
            {
               // directly created without parameter (Dialog control
               // probably )... grab a static HVIDEO for this...
               hVideo = &l.hDialogVid[(l.nControlVid++) & 0xf];
            }

            SetWindowLong (hWnd, WD_HVIDEO, (long) hVideo);

            if (hVideo->flags.bFull)
               hVideo->hDCOutput = GetWindowDC (hWnd);
            else
               hVideo->hDCOutput = GetDC (hWnd);

            GetWindowRect (hWnd, (RECT *) & (hVideo->pWindowPos.x));
            hVideo->pWindowPos.cx -= hVideo->pWindowPos.x;
            hVideo->pWindowPos.cy -= hVideo->pWindowPos.y;
            hVideo->hWndOutput = hWnd;

            CreateDrawingSurface (hVideo);
				EnableOpenGLView( hVideo, 0, 0, hVideo->pImage->width, hVideo->pImage->height );
				hVideo->flags.bReady = TRUE;
            WakeThreadID( hVideo->thread );
			}
      }
      break;
   }

   return DefWindowProc (hWnd, uMsg, wParam, lParam);
}
#endif
//----------------------------------------------------------------------------
#ifndef HWND_MESSAGE
#define HWND_MESSAGE     ((HWND)-3)
#endif

#define WINDOW_STYLE (WS_OVERLAPPEDWINDOW)

RENDER_PROC (BOOL, CreateWindowStuffSizedAt) (HVIDEO hVideo, int x, int y,
                                              int wx, int wy)
{
#ifndef __NO_WIN32API__
   l.bCreatedhWndInstance = 0;
   if (!hVideo->flags.bFull)
   {
      // may need to adjust window frame parameters...
      // used to have this code but we just deleted it!
   }
   if (!l.hWndInstance)
   {
      l.bCreatedhWndInstance = 1;
      //Log1 ("Creating container window named: %s",
      //    (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo->pTitle);
      l.hWndInstance = CreateWindowEx (0, (char *) l.aClass, (l.gpTitle && l.gpTitle[0]) ? l.gpTitle : hVideo->pTitle
                                      , WS_OVERLAPPEDWINDOW|WS_VISIBLE
                                      , 0, 0, 0, 0
                                      , HWND_MESSAGE  // (GetDesktopWindow()), // Parent
                                      , NULL // Menu
												  , GetModuleHandle (NULL), (void *) 1);
		{
			l.hVidCore = (HVIDEO)Allocate (sizeof (VIDEO));
			MemSet (l.hVidCore, 0, sizeof (VIDEO));
			l.hVidCore->hWndOutput = (HWND)l.hWndInstance;
		}
      //Log ("Created master window...");
   }
   if (wx == CW_USEDEFAULT || wy == CW_USEDEFAULT)
   {
      uint32_t w, h;
      GetDisplaySize (&w, &h);
      wx = w * 7 / 10;
      wy = h * 7 / 10;
   }
   // hWndOutput is set within the create window proc...
	CreateWindowEx( 0 //| WS_EX_LAYERED // WS_EX_NOPARENTNOTIFY
					  , (char *) l.aClass
					  , hVideo->pTitle
					  , hVideo->flags.bFull ? (WS_POPUP) : (WINDOW_STYLE)
					  , x, y
					  , hVideo->flags.bFull ?wx:(wx + l.WindowBorder_X)
					  , hVideo->flags.bFull ?wy:(wy + l.WindowBorder_Y)
					  , NULL //(HWND)l.hWndInstance  // Parent
					  , NULL     // Menu
					  , GetModuleHandle (NULL), (void *) hVideo);

	// save original screen image for initialized state...
#ifndef _OPENGL_DRIVER
	BitBlt ((HDC)hVideo->hDCBitmap, 0, 0, wx, wy
			 , (HDC)hVideo->hDCOutput, 0, 0, SRCCOPY);
#endif

	// generate an event to dispatch pending...
	// there is a good chance that a window event caused a window
	// and it will be sleeping until the next event...
	SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
	//Log ("Created window in module...");
	if (!hVideo->hWndOutput)
		return FALSE;
	//SetParent( hVideo->hWndOutput, l.hWndInstance );
	return TRUE;
#else
	// need a .NET window here...
	return FALSE;
#endif
}

//----------------------------------------------------------------------------

int CPROC VideoEventHandler( uint32_t MsgID, uint32_t *params, uint32_t paramlen )
{
#if defined( LOG_MOUSE_EVENTS ) || defined( OTHER_EVENTS_HERE )
	lprintf( WIDE("Received message %d"), MsgID );
#endif
   l.dwEventThreadID = GetCurrentThreadId();
	//LogBinary( (POINTER)params, paramlen );
	switch( MsgID )
	{
	case MSG_DispatchPending:
		{
			INDEX idx;
			HVIDEO hVideo;
         //lprintf( WIDE("dispatching outstanding events...") );
			LIST_FORALL( l.pInactiveList, idx, HVIDEO, hVideo )
			{
				if( !hVideo->flags.bReady )
				{
					if( !hVideo->flags.bInDestroy )
					{
						SetWindowLong( hVideo->hWndOutput, WD_HVIDEO, 0 );
						Release( hVideo ); // last event in queue, should be safe to go away now...
					}
					SetLink( &l.pInactiveList, idx, NULL );
				}
			}
			LIST_FORALL( l.pActiveList, idx, HVIDEO, hVideo )
			{
            if( hVideo->flags.mouse_pending )
				{
					if( hVideo && hVideo->pMouseCallback)
					{
#ifdef LOG_MOUSE_EVENTS
						lprintf( WIDE("Pending dispatch mouse %p (%d,%d) %08x")
								 , hVideo
								 , hVideo->mouse.x
								 , hVideo->mouse.y
								 , hVideo->mouse.b );
#endif
						if( !hVideo->flags.event_dispatched )
						{
							hVideo->flags.mouse_pending = 0;
							hVideo->flags.event_dispatched = 1;
							hVideo->pMouseCallback( hVideo->dwMouseData
														 , hVideo->mouse.x
														 , hVideo->mouse.y
														 , hVideo->mouse.b
														 );
							//lprintf( "Resulted..." );
							hVideo->mouse.x = l.mouse_x;
							hVideo->mouse.y = l.mouse_y;
							hVideo->mouse.b = l.mouse_b;
							hVideo->flags.event_dispatched = 0;
						}
					}
				}
			}
		}
		break;
	case MSG_LoseFocusMethod:
		{
			HVIDEO hVideo = (HVIDEO)params[0];
         if( l.flags.bLogFocus )
				lprintf( WIDE("Got a losefocus for %p at %P"), params[0], params[1] );
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo && hVideo->pLoseFocus )
					hVideo->pLoseFocus (hVideo->dwLoseFocus, (HVIDEO)params[1] );
			}
		}
		break;
	case MSG_RedrawMethod:
		{
			HVIDEO hVideo = (HVIDEO)params[0];
			//lprintf( WIDE("Show video %p"), hVideo );
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				EnterCriticalSec( &hVideo->cs );
				if( hVideo && hVideo->pRedrawCallback )
				{
					hVideo->flags.event_dispatched = 1;
					hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
					hVideo->flags.event_dispatched = 0;
					UpdateDisplayPortion (hVideo, 0, 0, 0, 0);
					//InvalidateRect( hVideo->hWndOutput, NULL, FALSE );
				}
				LeaveCriticalSec( &hVideo->cs );
			}
			else
            lprintf( WIDE("Failed to find window to show?") );
		}
		break;
	case MSG_CloseMethod:
		break;
	case MSG_MouseMethod:
		{
			HVIDEO hVideo = (HVIDEO)params[0];
#ifdef LOG_MOUSE_EVENTS
			lprintf( WIDE("mouse method... forward to application please...") );
         lprintf( "params %ld %ld %ld", params[1], params[2], params[3] );
#endif
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
			{
				if( hVideo->flags.mouse_pending )
				{
					if( params[3] != hVideo->mouse.b ||
						( params[3] & MK_OBUTTON ) )
					{
#ifdef LOG_MOUSE_EVENTS
						lprintf( WIDE("delta dispatching mouse (%d,%d) %08x")
								 , hVideo->mouse.x
								 , hVideo->mouse.y
								 , hVideo->mouse.b );
#endif
						if( !hVideo->flags.event_dispatched )
						{
							hVideo->flags.event_dispatched = 1;
							if( hVideo->pMouseCallback )
								hVideo->pMouseCallback( hVideo->dwMouseData
															 , hVideo->mouse.x
															 , hVideo->mouse.y
															 , hVideo->mouse.b
															 );
							hVideo->mouse.x = l.mouse_x;
							hVideo->mouse.y = l.mouse_y;
							hVideo->mouse.b = l.mouse_b;
							hVideo->flags.event_dispatched = 0;
						}
					}
					if( params[3] & MK_OBUTTON )
					{
						// copy keyboard state table for the client's use...
					}
				}
#ifdef LOG_MOUSE_EVENTS
				lprintf( "*** Set Mouse Pending on %p %ld,%ld %08x!", hVideo, params[1], params[2], params[3]  );
#endif
				hVideo->flags.mouse_pending = 1;
				hVideo->mouse.x = params[1];
				hVideo->mouse.y = params[2];
				hVideo->mouse.b = params[3];
				// indicate that we want to receive eventdispatch
				return EVENT_WAIT_DISPATCH;
			}
 		}
		break;
	case MSG_KeyMethod:
		{
			int dispatch_handled = 0;
			HVIDEO hVideo = (HVIDEO)params[0];
			if( FindLink( &l.pActiveList, hVideo ) != INVALID_INDEX )
				if( hVideo && hVideo->pKeyProc )
				{
					hVideo->flags.event_dispatched = 1;
					//lprintf( WIDE("Dispatched KEY!") );
					if( hVideo->flags.key_dispatched )
						EnqueLink( &hVideo->pInput, (POINTER)params[1] );
					else
					{
						hVideo->flags.key_dispatched = 1;
						do
						{
							//lprintf( "Dispatching key %08lx", params[1] );
							dispatch_handled = hVideo->pKeyProc( hVideo->dwKeyData, params[1] );
							//lprintf( "Result of dispatch was %ld", dispatch_handled );
							if( !dispatch_handled )
							{
#ifdef LOG_KEY_EVENTS
								lprintf( WIDE("Local Keydefs Dispatch key : %p %08lx"), hVideo, params[1] );
#endif
								if( hVideo && !HandleKeyEvents( hVideo->KeyDefs, params[1] ) )
								{
#ifdef LOG_KEY_EVENTS
									lprintf( WIDE("Global Keydefs Dispatch key : %08lx"), params[1] );
#endif
									if( !HandleKeyEvents( KeyDefs, params[1] ) )
									{
										// previously this would then dispatch the key event...
										// but we want to give priority to handled keys.
									}
								}
							}
							params[1] = (uint32_t)DequeLink( &hVideo->pInput );
						} while( params[1] );
						hVideo->flags.key_dispatched = 0;
					}
					hVideo->flags.event_dispatched = 0;
				}
		}
		break;
	default:
      lprintf( WIDE("Got a unknown message %d with %d data"), MsgID, paramlen );
      break;
	}
   return 0;
}

//----------------------------------------------------------------------------

void HandleDestroyMessage( HVIDEO hVidDestroy )
{
   {
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("To destroy! %p %d"), Msg.lParam, hVidDestroy->hWndOutput );
#endif
		// hide the window! then it can't be focused or active or anything!

		//EnableWindow( hVidDestroy->hWndOutput, FALSE );
		if( GetActiveWindow() == hVidDestroy->hWndOutput)
		{
#ifdef LOG_DESTRUCTION
			lprintf( WIDE("Set ourselves inactive...") );
#endif
			//SetActiveWindow( l.hWndInstance );
			SetForegroundWindow( (HWND)l.hWndInstance );
		}
		if( GetFocus() == hVidDestroy->hWndOutput)
		{
#ifdef LOG_DESTRUCTION
			lprintf( WIDE("Fixed focus away from ourselves before destroy.") );
#endif
			SetFocus( GetDesktopWindow() );
		}
		//ShowWindow( hVidDestroy->hWndOutput, SW_HIDE );
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("------------ DESTROY! -----------") );
#endif
 		DestroyWindow (hVidDestroy->hWndOutput);
		//UnlinkVideo (hVidDestroy);
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("From destroy") );
#endif
	}
}
//----------------------------------------------------------------------------

static void HandleMessage (MSG Msg)
{
#ifdef USE_XP_RAW_INPUT
	//#if(_WIN32_WINNT >= 0x0501)
#define WM_INPUT                        0x00FF
	//#endif /* _WIN32_WINNT >= 0x0501 */
	if( Msg.message ==  WM_INPUT )
	{
		UINT dwSize;
		WPARAM wParam = Msg.wParam;
		LPARAM lParam = Msg.lParam;
		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
							 sizeof(RAWINPUTHEADER));
		{
			LPBYTE lpb = Allocate( dwSize );
			if (lpb == NULL)
			{
				return;
			}

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize,
									  sizeof(RAWINPUTHEADER)) != dwSize )
				OutputDebugString (TEXT("GetRawInputData doesn't return correct size !\n"));

			{
				RAWINPUT* raw = (RAWINPUT*)lpb;
				char szTempOutput[256];
				HRESULT hResult;

				if (raw->header.dwType == RIM_TYPEKEYBOARD)
				{
               /*
					snprintf(szTempOutput, sizeof( szTempOutput ), TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
											 raw->data.keyboard.MakeCode,
											 raw->data.keyboard.Flags,
											 raw->data.keyboard.Reserved,
											 raw->data.keyboard.ExtraInformation,
											 raw->data.keyboard.Message,
											 raw->data.keyboard.VKey);
											 lprintf(szTempOutput);
                                  */
				}
				else if (raw->header.dwType == RIM_TYPEMOUSE)
				{

					snprintf(szTempOutput,sizeof( szTempOutput ) , TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
											 raw->data.mouse.usFlags,
											 raw->data.mouse.ulButtons,
											 raw->data.mouse.usButtonFlags,
											 raw->data.mouse.usButtonData,
											 raw->data.mouse.ulRawButtons,
											 raw->data.mouse.lLastX,
											 raw->data.mouse.lLastY,
											 raw->data.mouse.ulExtraInformation);

											 lprintf(szTempOutput);

				}
			}
			Release( lpb );
		}
		DispatchMessage (&Msg);
	}

	else
#endif
		if (!Msg.hwnd && (Msg.message == (WM_USER + 512)))
   {
      HVIDEO hVidCreate = (HVIDEO) Msg.lParam;
      CreateWindowStuffSizedAt (hVidCreate, hVidCreate->pWindowPos.x,
                                hVidCreate->pWindowPos.y,
                                hVidCreate->pWindowPos.cx,
                                hVidCreate->pWindowPos.cy);
   }
   else if (!Msg.hwnd && (Msg.message == (WM_USER + 513)))
	{
      HandleDestroyMessage( (HVIDEO) Msg.lParam );
   }
	else if (Msg.message == WM_QUIT)
		l.bExitThread = TRUE;
	else
	{
		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}
}

//----------------------------------------------------------------------------
int internal;
static int CPROC ProcessDisplayMessages(void )
{
   MSG Msg;
   if (GetCurrentThreadId () == l.dwThreadID)
   {
      if (l.bExitThread)
         return -1;
      if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
		{
			lprintf( WIDE("(E)Got message:%d"), Msg.message );
         HandleMessage (Msg);
         if (l.bExitThread)
            return -1;
         return TRUE;
      }
      return FALSE;
   }
   return -1;
}

//----------------------------------------------------------------------------
uintptr_t CPROC VideoThreadProc (PTHREAD thread)
{
#ifdef LOG_STARTUP
	Log( WIDE("Video thread...") );
#endif
   if (l.bThreadRunning)
   {
#ifdef LOG_STARTUP
		Log( WIDE("Already exists - leaving.") );
#endif
      return 0;
   }
   {
      // creat the thread's message queue so that when we set
      // dwthread, it's going to be a valid target for
      // setwindowshookex
		MSG msg;
#ifdef LOG_STARTUP
		Log( WIDE("reading a message to create a message queue") );
#endif
		PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	}
	l.dwThreadID = GetCurrentThreadId ();
	//l.pid = (uint32_t)l.dwThreadID;
	l.bThreadRunning = TRUE;
	AddIdleProc ( (int(CPROC*)(uintptr_t))ProcessDisplayMessages, 0);
	//AddIdleProc ( ProcessClientMessages, 0);
#ifdef LOG_STARTUP
	Log( WIDE("Registered Idle, and starting message loop") );
#endif
	{
		MSG Msg;
		while( !l.bExitThread && GetMessage (&Msg, NULL, 0, 0) )
		{
			//lprintf( "Dispatched..." );
			HandleMessage (Msg);
		}
	}
	l.bThreadRunning = FALSE;
	//ExitThread( 0 );
	return 0;
}

//----------------------------------------------------------------------------

RENDER_PROC (int, InitDisplay) (void)
{
#ifndef __NO_WIN32API__
   WNDCLASS wc;
   if (!l.aClass)
   {
      memset (&wc, 0, sizeof (WNDCLASS));
      wc.style = CS_OWNDC | CS_GLOBALCLASS;

      wc.lpfnWndProc = (WNDPROC) VideoWindowProc;
      wc.hInstance = GetModuleHandle (NULL);
      wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
      wc.lpszClassName = "VideoOutputClass";
      wc.cbWndExtra = 4;   // one extra DWORD


      l.aClass = RegisterClass (&wc);
		if (!l.aClass)
		{
         lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
			return FALSE;
		}
		InitMessageService();
		l.dwMsgBase = LoadService( NULL, VideoEventHandler );
      //l.flags.bOpenGL = TRUE; // make sure we force opengl surface, and issue soft surfaces to applications...
	}
#endif
   return TRUE;
}

//----------------------------------------------------------------------------

#ifndef __NO_WIN32API__
LRESULT CALLBACK
   KeyHook (int code,      // hook code
            WPARAM wParam,    // virtual-key code
            LPARAM lParam     // keystroke-message information
           )
{
   {
      HVIDEO hVid;
      int key, scancode, keymod = 0;
      HWND hWndFocus = GetFocus ();
      ATOM aThisClass;
      aThisClass = (ATOM) GetClassLong (hWndFocus, GCW_ATOM);
      if (aThisClass != l.aClass && hWndFocus != l.hWndInstance )
			return CallNextHookEx (l.hKeyHook, code, wParam, lParam);
		//lprintf( WIDE("Keyhook mesasage... %08x %08x"), wParam, lParam );
      //lprintf( WIDE("hWndFocus is %p"), hWndFocus );
		if( hWndFocus == l.hWndInstance )
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hwndfocus is something...") );
#endif
			hVid = l.hVidFocused;
		}
		else
		{
#ifdef LOG_FOCUSEVENTS
			lprintf( WIDE("hVid from focus") );
#endif
			hVid = (HVIDEO) GetWindowLong (hWndFocus, WD_HVIDEO);
			if( hVid && !hVid->flags.bReady )
			{
            DebugBreak();
				hVid = NULL;
			}
		}
      //lprintf( WIDE("hvid is %p"), hVid );
      //if(hVid)
      {
         /*
          // I think this is upside down...
          struct {
          uint32_t base : 8;
          uint32_t extended : 1;
          uint32_t nothing : 7;
          uint32_t scancode : 8;
          uint32_t morenothing : 4;
          uint32_t shift : 1;
          uint32_t control : 1;
          uint32_t alt : 1;
          uint32_t down : 1;
          } keycode;
          */
         key = ( scancode = (wParam & 0xFF) ) // base keystroke
            | ((lParam & 0x1000000) >> 16)   // extended key
            | (lParam & 0x80FF0000) // repeat count? and down status
            ^ (0x80000000) // not's the single top bit... becomes 'press'
            ;
         // lparam MSB is keyup status (strange)

         if (key & 0x80000000)   // test keydown...
         {
            l.kbd.key[wParam & 0xFF] |= 0x80;   // set this bit (pressed)
            l.kbd.key[wParam & 0xFF] ^= 1;   // toggle this bit...
         }
         else
         {
            l.kbd.key[wParam & 0xFF] &= ~0x80;  //(unpressed)
			}
         //lprintf( WIDE("Set local keyboard %d to %d"), wParam& 0xFF, l.kbd.key[wParam&0xFF]);
			if( hVid )
			{
				hVid->kbd.key[wParam & 0xFF] = l.kbd.key[wParam & 0xFF];
			}

         if (l.kbd.key[KEY_SHIFT] & 0x80)
         {
            key |= 0x10000000;
            l.mouse_b |= MK_SHIFT;
            keymod |= 1;
         }
         else
            l.mouse_b &= ~MK_SHIFT;
         if (l.kbd.key[KEY_CTRL] & 0x80)
         {
            key |= 0x20000000;
            l.mouse_b |= MK_CONTROL;
            keymod |= 2;
         }
         else
            l.mouse_b &= ~MK_CONTROL;
         if (l.kbd.key[KEY_ALT] & 0x80)
         {
            key |= 0x40000000;
            l.mouse_b |= MK_ALT;
            keymod |= 4;
         }
         else
            l.mouse_b &= ~MK_ALT;
				{
					uint32_t Msg[2];
					Msg[0] = (uint32_t)hVid;
					Msg[1] = key;
               //lprintf( WIDE("Dispatch key from raw handler into event system.") );
					SendServiceEvent( 0, l.dwMsgBase + MSG_KeyMethod, Msg, sizeof( Msg ) );
				}
		}
	}
   // do we REALLY have to call the next hook?!
   // I mean windows will just fuck us in the next layer....
   return CallNextHookEx (l.hKeyHook, code, wParam, lParam);
}
#endif
//----------------------------------------------------------------------------

RENDER_PROC (char, GetKeyText) (int key)
{
   int c;
   char ch[5];
   key ^= 0x80000000;
   c =
      ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
               l.kbd.key, (unsigned short *) ch, 0);
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

RENDER_PROC (HVIDEO, OpenDisplaySizedAt) (uint32_t attr, uint32_t wx, uint32_t wy, int32_t x, int32_t y) // if native - we can return and let the messages dispatch...
{
   HVIDEO hNextVideo;
   hNextVideo = (HVIDEO)Allocate (sizeof (VIDEO));
	MemSet (hNextVideo, 0, sizeof (VIDEO));
#ifdef _OPENGL_ENABLED
	hNextVideo->_prior_fracture = -1;
#endif
   hNextVideo->KeyDefs = CreateKeyBinder();
   if (wx == -1)
      wx = CW_USEDEFAULT;
   if (wy == -1)
      wy = CW_USEDEFAULT;
   if (x == -1)
      x = CW_USEDEFAULT;
   if (y == -1)
      y = CW_USEDEFAULT;
   l.WindowBorder_X = GetSystemMetrics (SM_CXFRAME);
   l.WindowBorder_Y = GetSystemMetrics (SM_CYFRAME)
      + GetSystemMetrics (SM_CYCAPTION) + GetSystemMetrics (SM_CYBORDER);
   // NOT MULTI-THREAD SAFE ANYMORE!
   InitDisplay ();
   if (!l.pThread)
   {
      int failcount = 0;
      //Log( WIDE("Starting video thread...") );
      l.pThread = ThreadTo (VideoThreadProc, 0);
#ifdef LOG_STARTUP
		Log( WIDE("Started video thread...") );
#endif
      do
      {
         failcount++;
         do
         {
            Sleep (0);
         }
         while (!l.bThreadRunning);
#ifndef __NO_WIN32API__
         l.hKeyHook =
				SetWindowsHookEx (WH_KEYBOARD, (HOOKPROC)KeyHook, NULL, l.dwThreadID);
#endif
      } while( !l.hKeyHook && (failcount < 100) );
#ifdef USE_XP_RAW_INPUT
		{
			RAWINPUTDEVICE Rid[2];

			Rid[0].usUsagePage = 0x01;
			Rid[0].usUsage = 0x02;
			Rid[0].dwFlags = 0 /*RIDEV_NOLEGACY*/;   // adds HID mouse and also ignores legacy mouse messages

			Rid[1].usUsagePage = 0x01;
			Rid[1].usUsage = 0x06;
			Rid[1].dwFlags = 0 /*RIDEV_NOLEGACY*/;   // adds HID keyboard and also ignores legacy keyboard messages

			if (RegisterRawInputDevices(Rid, 2, sizeof (Rid [0])) == FALSE)
			{
				lprintf( WIDE("Registration failed!?") );
				//registration failed. Call GetLastError for the cause of the error
			}
		}
#endif
	}
   lprintf( WIDE("Hardcoded right here for FULL window surfaces, no native borders.") );
   hNextVideo->flags.bFull = TRUE; //FALSE;
   hNextVideo->pWindowPos.x = x;
   hNextVideo->pWindowPos.y = y;
   hNextVideo->pWindowPos.cx = wx;
	hNextVideo->pWindowPos.cy = wy;
   AddLink( &l.pActiveList, hNextVideo );
	//hNextVideo->pid = l.pid;
   if (GetCurrentThreadId () == l.dwThreadID)
	{
		CreateWindowStuffSizedAt ( hNextVideo
										 , hNextVideo->pWindowPos.x
										 , hNextVideo->pWindowPos.y
										 , hNextVideo->pWindowPos.cx
										 , hNextVideo->pWindowPos.cy);
	}
	else
	{
		int d = 1;
		int cnt = 25;
		do
		{
			//SendServiceEvent( l.pid, WM_USER + 512, &hNextVideo, sizeof( hNextVideo ) );
			d = PostThreadMessage (l.dwThreadID, WM_USER + 512, 0,
										  (LPARAM) hNextVideo);
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
			uint32_t timeout = GetTickCount() + 5000;
			hNextVideo->thread = GetMyThreadID();
			while (!hNextVideo->flags.bReady && timeout > GetTickCount())
			{
				// need to do this on the possibility that
				// thIS thread did create this window...
				if( !Idle() )
				{
					//lprintf( WIDE("Sleeping until the window is created.") );
					//WakeableSleep( SLEEP_FOREVER );
					Relinquish();
				}
			}
			if( !hNextVideo->flags.bReady )
			{
            CloseDisplay( hNextVideo );
				lprintf( WIDE("Fatality.. window creation did not complete in a timely manner.") );
				// hnextvideo is null anyhow, but this is explicit.
				return NULL;
			}
		}
	}
#ifdef LOG_STARTUP
	lprintf( WIDE("Resulting new window %p %d"), hNextVideo, hNextVideo->hWndOutput );
#endif
   return hNextVideo;      // can return a-synchronously with creation... but WILL be valid...
}

//----------------------------------------------------------------------------

RENDER_PROC (HVIDEO, OpenDisplayAboveSizedAt) (uint32_t attr, uint32_t wx, uint32_t wy,
                                               int32_t x, int32_t y, HVIDEO parent)
{
   HVIDEO newvid = OpenDisplaySizedAt (attr, wx, wy, x, y);
   if (parent)
      PutDisplayAbove (newvid, parent);
   return newvid;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, CloseDisplay) (HVIDEO hVideo)
{
   int bEventThread;
   // just kills this video handle....
   if (!hVideo)         // must already be destroyed, eh?
      return;
#ifdef LOG_DESTRUCTION
   Log ("Unlinking destroyed window...");
#endif
   // take this out of the list of active windows...
   DeleteLink( &l.pActiveList, hVideo );
	hVideo->flags.bDestroy = 1;
	// this isn't the therad to worry about...
   bEventThread = ( l.dwEventThreadID == GetCurrentThreadId() );
	if (GetCurrentThreadId () == l.dwThreadID )
   {
#ifdef LOG_DESTRUCTION
		lprintf( WIDE("Scheduled for deletion") );
#endif
      HandleDestroyMessage( hVideo );
   }
   else
   {
		int d = 1;
#ifdef LOG_DESTRUCTION
      Log ("Dispatching destroy and resulting...");
#endif
		//SendServiceEvent( l.pid, WM_USER + 513, &hVideo, sizeof( hVideo ) );
      d = PostThreadMessage (l.dwThreadID, WM_USER + 513, 0, (LPARAM) hVideo);
      if (!d)
      {
         Log ("Failed to post create new window message...");
         DebugBreak ();
      }
	}
	{
#ifdef LOG_DESTRUCTION
		int logged = 0;
#endif
      hVideo->flags.bInDestroy = 1;
#ifdef LOG_DESTRUCTION
		while (hVideo->flags.bReady && !bEventThread )
		{
			if( !logged )
			{
				lprintf( WIDE("Wait for window to go unready.") );
				logged = 1;
			}
			Sleep (0);
		}
#else
		while (hVideo->flags.bReady && !bEventThread)
         Idle();
#endif
		hVideo->flags.bInDestroy = 0;
      // the scan of inactive windows releases the hVideo...
		AddLink( &l.pInactiveList, hVideo );
		// generate an event to dispatch pending...
		// there is a good chance that a window event caused a window
		// and it will be sleeping until the next event...
		SendServiceEvent( 0, l.dwMsgBase + MSG_DispatchPending, NULL, 0 );
	}
   return;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SizeDisplay) (HVIDEO hVideo, uint32_t w, uint32_t h)
{
	SetWindowPos (hVideo->hWndOutput, NULL
				, 0, 0
				, hVideo->flags.bFull ?w:(w+l.WindowBorder_X)
				, hVideo->flags.bFull ?h:(h + l.WindowBorder_Y)
                , SWP_NOZORDER | SWP_NOMOVE);
}


//----------------------------------------------------------------------------

RENDER_PROC (void, SizeDisplayRel) (HVIDEO hVideo, int32_t delw, int32_t delh)
{
   if (delw || delh)
   {
      int32_t cx, cy;
      cx = hVideo->pWindowPos.cx + delw;
      cy = hVideo->pWindowPos.cy + delh;
      if (hVideo->pWindowPos.cx < 50)
         hVideo->pWindowPos.cx = 50;
      if (hVideo->pWindowPos.cy < 20)
         hVideo->pWindowPos.cy = 20;
#ifdef LOG_RESIZE
      Log2 ("Resized display to %d,%d", hVideo->pWindowPos.cx,
            hVideo->pWindowPos.cy);
#endif

      SetWindowPos (hVideo->hWndOutput, NULL, 0, 0, cx, cy,
                    SWP_NOZORDER | SWP_NOMOVE);
   }
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveDisplay) (HVIDEO hVideo, int32_t x, int32_t y)
{
   SetWindowPos (hVideo->hWndOutput, NULL, x, y, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveDisplayRel) (HVIDEO hVideo, int32_t x, int32_t y)
{
   if (x || y)
   {
      hVideo->pWindowPos.x += x;
      hVideo->pWindowPos.y += y;
      SetWindowPos (hVideo->hWndOutput, NULL
					, hVideo->pWindowPos.x
                    , hVideo->pWindowPos.y
					, 0, 0
					, SWP_NOZORDER | SWP_NOSIZE);
   }
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplay) (HVIDEO hVideo, int32_t x, int32_t y, int32_t w,
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
   SetWindowPos (hVideo->hWndOutput, NULL
				, hVideo->pWindowPos.x
				, hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				 , SWP_NOZORDER);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MoveSizeDisplayRel) (HVIDEO hVideo, int32_t delx, int32_t dely,
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
  SetWindowPos (hVideo->hWndOutput, NULL
				, hVideo->pWindowPos.x
                , hVideo->pWindowPos.y
				, hVideo->flags.bFull ?cx:(cx+l.WindowBorder_X)
				, hVideo->flags.bFull ?cy:(cy + l.WindowBorder_Y)
				, SWP_NOZORDER);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, UpdateDisplayEx) (HVIDEO hVideo DBG_PASS )
{
   // copy hVideo->lpBuffer to hVideo->hDCOutput
	if (hVideo
#ifndef _OPENGL_DRIVER
		 && hVideo->hWndOutput && hVideo->hBm
#endif
		)
   {
      UpdateDisplayPortionEx (hVideo, 0, 0, 0, 0 DBG_RELAY);
   }
   return;
}

//----------------------------------------------------------------------------


RENDER_PROC (void, SetMousePosition) (HVIDEO hVid, int32_t x, int32_t y)
{

   if (hVid->flags.bFull)
      SetCursorPos (x + hVid->pWindowPos.x, y + hVid->pWindowPos.y);
   else
      SetCursorPos (x + l.WindowBorder_X + hVid->pWindowPos.x,
                    y + l.WindowBorder_Y + hVid->pWindowPos.y);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetMousePosition) (int32_t * x, int32_t * y)
{
   POINT p;
   GetCursorPos (&p);
   if (x)
      (*x) = p.x;
   if (y)
      (*y) = p.y;
}

//----------------------------------------------------------------------------

void CPROC GetMouseState(int32_t * x, int32_t * y, uint32_t *b)
{
   GetMousePosition( x, y );
	if( b )
      (*b) = l.mouse_b;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetCloseHandler) (HVIDEO hVideo,
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

RENDER_PROC (void, SetMouseHandler) (HVIDEO hVideo,
                                     MouseCallback pMouseCallback,
                                     uintptr_t dwUser)
{
   hVideo->dwMouseData = dwUser;
   hVideo->pMouseCallback = pMouseCallback;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetRedrawHandler) (HVIDEO hVideo,
                                      RedrawCallback pRedrawCallback,
                                      uintptr_t dwUser)
{
   hVideo->dwRedrawData = dwUser;
	if( (hVideo->pRedrawCallback = pRedrawCallback ) )
	{
		//lprintf( WIDE("Sending redraw for %p"), hVideo );
      if( hVideo->flags.bShown )
			SendServiceEvent( 0, l.dwMsgBase + MSG_RedrawMethod, &hVideo, sizeof( hVideo ) );
		//hVideo->pRedrawCallback (hVideo->dwRedrawData, (PRENDERER) hVideo);
	}

}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetKeyboardHandler) (HVIDEO hVideo, KeyProc pKeyProc,
                                        uintptr_t dwUser)
{
   hVideo->dwKeyData = dwUser;
   hVideo->pKeyProc = pKeyProc;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetLoseFocusHandler) (HVIDEO hVideo,
                                         LoseFocusCallback pLoseFocus,
                                         uintptr_t dwUser)
{
   hVideo->dwLoseFocus = dwUser;
   hVideo->pLoseFocus = pLoseFocus;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetApplicationTitle) (const TEXTCHAR *pTitle)
{
   l.gpTitle = pTitle;
   if (l.hWndInstance)
      SetWindowText ((HWND)l.hWndInstance, l.gpTitle);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetRendererTitle) (HVIDEO hVideo, const TEXTCHAR *pTitle)
{
	//local_vidlib.gpTitle = pTitle;
	//if (local_vidlib.hWndInstance)
	{
		//DebugBreak();
		if( hVideo->pTitle )
         Release( hVideo->pTitle );
		hVideo->pTitle = StrDup( pTitle );
		SetWindowText( hVideo->hWndOutput, pTitle );
	}
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetApplicationIcon) (ImageFile * hIcon)
{
#ifdef _WIN32
   //HICON hIcon = CreateIcon();
#endif
}

//----------------------------------------------------------------------------

RENDER_PROC (void, MakeTopmost) (HVIDEO hVideo)
{
	hVideo->flags.bTopmost = 1;
   if( hVideo->flags.bShown )
		SetWindowPos (hVideo->hWndOutput, HWND_TOPMOST, 0, 0, 0, 0,
						  SWP_NOMOVE | SWP_NOSIZE);
}

//----------------------------------------------------------------------------

RENDER_PROC( int, IsTopmost )( HVIDEO hVideo )
{
   return hVideo->flags.bTopmost;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, HideDisplay) (HVIDEO hVideo)
{
	//Log ("Hiding the window!");
	if( hVideo )
	{
		hVideo->flags.bShown = 0;
		hVideo->flags.bHidden = 1;
		ShowWindow (hVideo->hWndOutput, SW_HIDE);
		if (hVideo->pAbove)
		{
			lprintf( WIDE("Focusing and activating my parent window.") );
			{
				static POINTER _NULL = NULL;
				SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
											, &hVideo, sizeof( hVideo )
											, &hVideo->pAbove, sizeof( hVideo->pAbove ) );
				SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
											, &hVideo->pAbove, sizeof( hVideo->pAbove )
											, &_NULL, sizeof( _NULL ) );
				//if( hVideo->pLoseFocus )
				//	hVideo->pLoseFocus(hVideo->dwLoseFocus, hVideo->pAbove);
				//if( hVideo->pAbove->pLoseFocus )
				//	hVideo->pAbove->pLoseFocus(hVideo->pAbove->dwLoseFocus, NULL );
			}

			SetFocus( hVideo->pAbove->hWndOutput );
			SetActiveWindow (hVideo->pAbove->hWndOutput);
		}
		if (hVideo->pBelow)
		{
			{
				static POINTER _NULL = NULL;
				SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
											, &hVideo, sizeof( hVideo )
											, &hVideo->pBelow, sizeof( hVideo->pBelow ) );
				SendMultiServiceEvent( 0, l.dwMsgBase + MSG_LoseFocusMethod, 2
											, &hVideo->pBelow, sizeof( hVideo->pBelow )
											, &_NULL, sizeof( _NULL ) );
				//if( hVideo->pLoseFocus )
				//	hVideo->pLoseFocus(hVideo->dwLoseFocus, hVideo->pBelow);
				//if( hVideo->pBelow->pLoseFocus )
				//	hVideo->pBelow->pLoseFocus(hVideo->pBelow->dwLoseFocus, NULL );
			}
			SetFocus( hVideo->pBelow->hWndOutput );
			SetActiveWindow (hVideo->pBelow->hWndOutput);
		}
	}
}

//----------------------------------------------------------------------------
RENDER_PROC (void, RestoreDisplay) (HVIDEO hVideo)
{
		hVideo->flags.bHidden = 0;
   ShowWindow (hVideo->hWndOutput, SW_RESTORE);
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetDisplaySize) (uint32_t * width, uint32_t * height)
{
   RECT r;
   GetWindowRect (GetDesktopWindow (), &r);
	//Log4( WIDE("Desktop rect is: %d, %d, %d, %d"), r.left, r.right, r.top, r.bottom );

	if (width)
      //1024;//
      *width = r.right - r.left;
	if (height)
      // 760;//
      *height =r.bottom - r.top;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, GetDisplayPosition) (HVIDEO hVid, int32_t * x, int32_t * y,
                                        uint32_t * width, uint32_t * height)
{
   if (!hVid)
      return;
   if (width)
      *width = hVid->pWindowPos.cx;
   if (height)
      *height = hVid->pWindowPos.cy;
   if (x)
      *x = hVid->pWindowPos.x;
   if (y)
      *y = hVid->pWindowPos.y;
}

//----------------------------------------------------------------------------
RENDER_PROC (LOGICAL, DisplayIsValid) (HVIDEO hVid)
{
   return hVid->flags.bReady;
}

//----------------------------------------------------------------------------

RENDER_PROC (void, SetDisplaySize) (uint32_t width, uint32_t height)
{
   SizeDisplay (l.hVideoPool, width, height);
}

//----------------------------------------------------------------------------

RENDER_PROC (ImageFile *,GetDisplayImage )(HVIDEO hVideo)
{
#ifdef _OPENGL_DRIVER
	return hVideo->pAppImage;
#else
	return hVideo->pImage;
#endif
}

//----------------------------------------------------------------------------

RENDER_PROC (PKEYBOARD, GetDisplayKeyboard) (HVIDEO hVideo)
{
   return &hVideo->kbd;
}

//----------------------------------------------------------------------------

RENDER_PROC (LOGICAL, HasFocus) (PRENDERER hVideo)
{
   return hVideo->flags.bFocused;
}

//----------------------------------------------------------------------------

RENDER_PROC (int, SendActiveMessage) (PRENDERER dest, PACTIVEMESSAGE msg)
{
   return 0;
}

RENDER_PROC (PACTIVEMESSAGE, CreateActiveMessage) (int ID, int size,...)
{
   return NULL;
}

RENDER_PROC (void, SetDefaultHandler) (PRENDERER hVideo,
                                       GeneralCallback general, uintptr_t psv)
{
}

//----------------------------------------------------------------------------

RENDER_PROC (void, OwnMouseEx) (HVIDEO hVideo, uintptr_t own DBG_PASS)
{
	if (own)
	{
		lprintf( WIDE("Capture is set on %p"),hVideo );
		if( !l.hCaptured )
		{
			l.hCaptured = hVideo;
			hVideo->flags.bCaptured = 1;
			SetCapture (hVideo->hWndOutput);
		}
		else
		{
			if( l.hCaptured != hVideo )
			{
				lprintf( WIDE("Another window now wants to capture the mouse... the prior window will ahve the capture stolen.") );
				l.hCaptured = hVideo;
				hVideo->flags.bCaptured = 1;
				SetCapture (hVideo->hWndOutput);
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

RENDER_PROC (uintptr_t, GetNativeHandle) (HVIDEO hVideo)
{
   return (uintptr_t) hVideo->hWndOutput;
}

RENDER_PROC (int, BeginCalibration) (uint32_t nPoints)
{
   return 1;
}

//----------------------------------------------------------------------------
RENDER_PROC (void, SyncRender)( HVIDEO hVideo )
{
   // sync has no consequence...
   return;
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayFocus )( PRENDERER pRender )
{
	//SetActiveWindow( GetParent( pRender->hWndOutput ) );
   //SetForegroundWindow( GetParent( pRender->hWndOutput ) );
   //SetFocus( GetParent( pRender->hWndOutput ) );
	SetActiveWindow( pRender->hWndOutput );
	SetForegroundWindow( pRender->hWndOutput );
   SetFocus( pRender->hWndOutput );
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayForward )( PRENDERER pRender )
{
   if( !SetWindowPos( pRender->hWndOutput, pRender->flags.bTopmost?HWND_TOPMOST:HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE ) )
      lprintf( WIDE("Window Pos set failed: %d"), GetLastError() );
   if( !SetActiveWindow( pRender->hWndOutput ) )
      lprintf( WIDE("active window failed: %d"), GetLastError() );

   if( !SetForegroundWindow( pRender->hWndOutput ) )
      lprintf( WIDE("okay well foreground failed...?") );
}

//----------------------------------------------------------------------------

RENDER_PROC( void, ForceDisplayBack )( PRENDERER pRender )
{
   // uhmm...
   SetWindowPos( pRender->hWndOutput, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );

}
//----------------------------------------------------------------------------

#undef UpdateDisplay
RENDER_PROC (void, UpdateDisplay) (HVIDEO hVideo )
{
   //DebugBreak();
   UpdateDisplayEx( hVideo DBG_SRC );
}

#include <render.h>

static RENDER_INTERFACE VidInterface = { InitDisplay
                                       , SetApplicationTitle
                                       , (void CPROC (*)(Image)) SetApplicationIcon
                                       , GetDisplaySize
                                       , SetDisplaySize
                                       , (PRENDERER CPROC (*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t)) OpenDisplaySizedAt
                                       , (PRENDERER CPROC (*)(uint32_t, uint32_t, uint32_t, int32_t, int32_t, PRENDERER)) OpenDisplayAboveSizedAt
                                       , (void CPROC (*)(PRENDERER)) CloseDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t, uint32_t, uint32_t DBG_PASS)) UpdateDisplayPortionEx
                                       , (void CPROC (*)(PRENDERER DBG_PASS)) UpdateDisplayEx
                                       , GetDisplayPosition
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) MoveDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) MoveDisplayRel
                                       , (void CPROC (*)(PRENDERER, uint32_t, uint32_t)) SizeDisplay
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) SizeDisplayRel
                                       , MoveSizeDisplayRel
                                       , (void CPROC (*)(PRENDERER, PRENDERER)) PutDisplayAbove
                                       , (Image CPROC (*)(PRENDERER)) GetDisplayImage
                                       , (void CPROC (*)(PRENDERER, CloseCallback, uintptr_t)) SetCloseHandler
                                       , (void CPROC (*)(PRENDERER, MouseCallback, uintptr_t)) SetMouseHandler
                                       , (void CPROC (*)(PRENDERER, RedrawCallback, uintptr_t)) SetRedrawHandler
                                       , (void CPROC (*)(PRENDERER, KeyProc, uintptr_t)) SetKeyboardHandler
                                       , (void CPROC (*)(PRENDERER, LoseFocusCallback, uintptr_t)) SetLoseFocusHandler, NULL   // default callback
                                       , (void CPROC (*)(int32_t *, int32_t *)) GetMousePosition
                                       , (void CPROC (*)(PRENDERER, int32_t, int32_t)) SetMousePosition
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
													, EnableOpenGL
                                       , SetActiveGLDisplay
#else
                                       , NULL
                                       , NULL
#endif
                                       , MoveSizeDisplay
                                       , MakeTopmost
                                       , HideDisplay
                                       , RestoreDisplay
                                       , ForceDisplayFocus
                                       , ForceDisplayForward
                                       , ForceDisplayBack
                                       , BindEventToKey
													, UnbindKey
													, IsTopmost
													, NULL // OkaySyncRender is internal.
													, IsTouchDisplay
													, GetMouseState
													, EnableSpriteMethod
													, NULL
													, NULL
													, NULL
                                       , SetRendererTitle
};

#undef GetDisplayInterface
#undef DropDisplayInterface

RENDER_PROC (POINTER, GetDisplayInterface) (void)
{
   InitDisplay();
   return (POINTER)&VidInterface;
}

RENDER_PROC (void, DropDisplayInterface) (POINTER p)
{
}

static void CPROC DefaultExit( uintptr_t psv, uint32_t keycode )
{
   BAG_Exit(0);
}

int IsTouchDisplay( void )
{
   return 0;
}

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 46
#endif
PRIORITY_PRELOAD( VideoRegisterInterface, VIDLIB_PRELOAD_PRIORITY )
{
   if( l.flags.bLogRegister )
		lprintf( WIDE("Regstering video interface...") );
   RegisterInterface( 
#ifdef SACK_BAG_EXPORTS  // symbol defined by visual studio sack_bag.vcproj
	   WIDE("render")
#else
	   WIDE("video")
#endif
	   , GetDisplayInterface, DropDisplayInterface );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_ALT, DefaultExit, 0 );
}

typedef struct sprite_method_tag *PSPRITE_METHOD;

RENDER_PROC( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(uintptr_t psv, PRENDERER renderer, int32_t x, int32_t y, uint32_t w, uint32_t h ), uintptr_t psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = (PSPRITE_METHOD)Allocate( sizeof( *psm ) );
	psm->renderer = render;
	psm->original_surface = MakeImageFile( render->pImage->width, render->pImage->height );
	//psm->original_surface = MakeImageFile( render->pImage->width, render->pImage->height );
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
   /*
	BlotImageSizedEx( psm->original_surface, psm->renderer->pImage
						 , x, y
						 , x, y
						 , w, h
						 , 0
						 , BLOT_COPY );
                   */
}

PRELOAD( InitSetSavePortion )
{
   SetSavePortion( SavePortion );
}

RENDER_NAMESPACE_END

//--------------------------------------------------------------------------
//
