/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 *
 *  Support for putting one image on another without scaling.
 *
 *
 *
 *  consult doc/image.html
 *
 */

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif

#include <stdhdrs.h>
#include <sharemem.h>

#ifdef __ANDROID__
#include <gles/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

#include <imglib/imagestruct.h>
#include <image.h>
#include "../image_common.h"
#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"

#ifdef __cplusplus
namespace sack {
namespace image {
#endif


//---------------------------------------------------------------------------

#define StartLoop oo /= 4;    \
   oi /= 4;                   \
   {                          \
      uint32_t row= 0;             \
      while( row < hs )       \
      {                       \
         uint32_t col=0;           \
         while( col < ws )    \
         {                    \
            {

#define EndLoop   }           \
/*lprintf( "in %08x out %08x", ((CDATA*)pi)[0], ((CDATA*)po)[1] );*/ \
            po++;             \
            pi++;             \
            col++;            \
         }                    \
         pi += oi;            \
         po += oo;            \
         row++;               \
      }                       \
   }

 void CPROC cCopyPixelsT0( PCDATA po, PCDATA  pi
                          , uint32_t oo, uint32_t oi
                          , uint32_t ws, uint32_t hs
                           )
{
   StartLoop
            *po = *pi;
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsT1( PCDATA po, PCDATA  pi
                          , uint32_t oo, uint32_t oi
                          , uint32_t ws, uint32_t hs
                           )
{
   StartLoop
            CDATA cin;
            if( (cin = *pi) )
            {
               *po = cin;
            }
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsTA( PCDATA po, PCDATA  pi
                          , uint32_t oo, uint32_t oi
                          , uint32_t ws, uint32_t hs
                          , uint32_t nTransparent )
{
   StartLoop
            CDATA cin;
            if( (cin = *pi) )
				{
					*po = DOALPHA2( *po
									  , cin
									  , nTransparent );
            }
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsTImgA( PCDATA po, PCDATA  pi
                          , uint32_t oo, uint32_t oi
                          , uint32_t ws, uint32_t hs
                          , uint32_t nTransparent )
{
   StartLoop
            uint32_t alpha;
            CDATA cin;
            if( (cin = *pi) )
            {
               alpha = ( cin & 0xFF000000 ) >> 24;
               alpha += nTransparent;
               *po = DOALPHA2( *po, cin, alpha );
            }
   EndLoop
}
//---------------------------------------------------------------------------

 void CPROC cCopyPixelsTImgAI( PCDATA po, PCDATA  pi
                          , uint32_t oo, uint32_t oi
                          , uint32_t ws, uint32_t hs
                          , uint32_t nTransparent )
{
   StartLoop
            int32_t alpha;

            CDATA cin;
            if( (cin = *pi) )
            {
               alpha = ( cin & 0xFF000000 ) >> 24;
               alpha -= nTransparent;
               if( alpha > 1 )
               {
                  *po = DOALPHA2( *po, cin, alpha );
               }
            }
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsShadedT0( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , CDATA c )
{
   StartLoop
            uint32_t pixel;
            pixel = *pi;
            *po = SHADEPIXEL(pixel, c);
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedT1( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , CDATA c )
{
   StartLoop
            uint32_t pixel;
            if( (pixel = *pi) )
            {
               *po = SHADEPIXEL(pixel, c);
            }
   EndLoop
}
//---------------------------------------------------------------------------

 void CPROC cCopyPixelsShadedTA( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA c )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               pixout = SHADEPIXEL(pixel, c);
               *po = DOALPHA2( *po, pixout, nTransparent );
            }
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedTImgA( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA c )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t alpha;
               pixout = SHADEPIXEL(pixel, c);
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha += nTransparent;
               *po = DOALPHA2( *po, pixout, alpha );
            }
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA c )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha -= nTransparent;
               if( alpha > 1 )
               {
                  pixout = SHADEPIXEL(pixel, c);
                  *po = DOALPHA2( *po, pixout, alpha );
               }
            }
   EndLoop
}


//---------------------------------------------------------------------------

 void CPROC cCopyPixelsMultiT0( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            uint32_t pixel, pixout;
            {
               uint32_t rout, gout, bout;
               pixel = *pi;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);
            }
            *po = pixout;
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiT1( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);

               *po = pixout;
            }
   EndLoop
}
//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiTA( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);
               *po = DOALPHA2( *po, pixout, nTransparent );
            }
   EndLoop
}
//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiTImgA( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha += nTransparent;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);
               //lprintf( "pixel %08x pixout %08x r %08x g %08x b %08x", pixel, pixout, r,g,b);
               *po = DOALPHA2( *po, pixout, alpha );
            }
   EndLoop
}
//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiTImgAI( PCDATA po, PCDATA  pi
                            , uint32_t oo, uint32_t oi
                            , uint32_t ws, uint32_t hs
                            , uint32_t nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha -= nTransparent;
               if( alpha > 1 )
               {
                  pixout = MULTISHADEPIXEL( pixel, r,g,b);
                  *po = DOALPHA2( *po, pixout, alpha );
               }
            }
   EndLoop
}


//---------------------------------------------------------------------------
// x, y is position
// xs, ys is starting position on source bitmap (x, y is upper left) + xs, ys )
// w, h is height and width of the image to use.
// default behavior is to omit copying 0 pixels for transparency
// overlays....
 void  BlotImageSizedEx ( ImageFile *pifDest, ImageFile *pifSrc
                              , int32_t xd, int32_t yd
                              , int32_t xs, int32_t ys
                              , uint32_t ws, uint32_t hs
                              , uint32_t nTransparent
                              , uint32_t method
                              , ... )
{
#define BROKEN_CODE
	PCDATA po, pi;
	//int  hd, wd;
	uint32_t oo, oi; // treated as an adder... it is unsigned by math, but still results correct offset?
	static uint32_t lock;
	va_list colors;
	va_start( colors, method );
	if( nTransparent > ALPHA_TRANSPARENT_MAX )
		return;

	if(  !pifSrc
		|| !pifSrc->image
		|| !pifDest
		//|| !pifDest->image

	  )
	{
		// this will happen when mixing modes...
		lprintf( WIDE( "sanity check failed %p %p %p" ), pifSrc, pifDest, pifSrc?pifSrc->image:NULL );
		return;
	}
	//lprintf( WIDE( "BlotImageSized %d,%d to %d,%d by %d,%d" ), xs, ys, xd, yd, ws, hs );

	{
		IMAGE_RECTANGLE r1;
		IMAGE_RECTANGLE r2;
		IMAGE_RECTANGLE rs;
		IMAGE_RECTANGLE rd;
		int tmp;
		//IMAGE_RECTANGLE r3;
		r1.x = xd;
		r1.y = yd;
		r1.width = ws;
		r1.height = hs;
		r2.x = pifDest->eff_x;
		r2.y = pifDest->eff_y;
		tmp = (pifDest->eff_maxx - pifDest->eff_x) + 1;
		if( tmp < 0 )
			r2.width = 0;
		else
			r2.width = (IMAGE_SIZE_COORDINATE)tmp;
		tmp = (pifDest->eff_maxy - pifDest->eff_y) + 1;
		if( tmp < 0 )
			r2.height = 0;
		else
			r2.height = (IMAGE_SIZE_COORDINATE)tmp;
		if( !IntersectRectangle( &rd, &r1, &r2 ) )
		{
			//lprintf( WIDE( "Images do not overlap. %d,%d %d,%d vs %d,%d %d,%d" ), r1.x,r1.y,r1.width,r1.height
			//		 , r2.x,r2.y,r2.width,r2.height);
			return;
		}

		//lprintf( WIDE( "Correcting coordinates by %d,%d" )
		//		 , rd.x - xd
		//		 , rd.y - yd
		//		 );

		xs += rd.x - xd;
		ys += rd.y - yd;
		tmp = rd.x - xd;
		if( tmp > 0 && (unsigned)tmp > ws )
			ws = 0;
		else
		{
			if( tmp < 0 )
				ws += (unsigned)-tmp;
			else
				ws -= (unsigned)tmp;
		}
		tmp = rd.y - yd;
		if( tmp > 0 && (unsigned)tmp > hs )
			hs = 0;
		else
		{
			if( tmp < 0 )
				hs += (unsigned)-tmp;
			else
				hs -= (unsigned)tmp;
		}
		//lprintf( WIDE( "Resulting dest is %d,%d %d,%d" ), rd.x,rd.y,rd.width,rd.height );
		xd = rd.x;
		yd = rd.y;
		r1.x = xs;
		r1.y = ys;
		r1.width = ws;
		r1.height = hs;
		r2.x = pifSrc->eff_x;
		r2.y = pifSrc->eff_y;
		tmp = (pifSrc->eff_maxx - pifSrc->eff_x) + 1;
		if( tmp < 0 )
			r2.width = 0;
		else
			r2.width = (IMAGE_SIZE_COORDINATE)tmp;
		tmp = (pifSrc->eff_maxy - pifSrc->eff_y) + 1;
		if( tmp < 0 )
			r2.height = 0;
		else
			r2.height = (IMAGE_SIZE_COORDINATE)tmp;
		if( !IntersectRectangle( &rs, &r1, &r2 ) )
		{
			lprintf( WIDE( "Desired Output does not overlap..." ) );
			return;
		}
		//lprintf( WIDE( "Resulting dest is %d,%d %d,%d" ), rs.x,rs.y,rs.width,rs.height );
		ws = rs.width<rd.width?rs.width:rd.width;
		hs = rs.height<rd.height?rs.height:rd.height;
		xs = rs.x;
		ys = rs.y;
		//lprintf( WIDE( "Resulting rect is %d,%d to %d,%d dim: %d,%d" ), rs.x, rs.y, rd.x, rd.y, rs.width, rs.height );
		//lprintf( WIDE( "Resulting rect is %d,%d to %d,%d dim: %d,%d" ), xs, ys, xd, yd, ws, hs );
	}
		//lprintf( WIDE(WIDE( "Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)" )), xs, ys, ws, hs, xd, yd, wd, hd );
	if( (int32_t)ws <= 0 ||
        (int32_t)hs <= 0 /*||
        (int32_t)wd <= 0 ||
		(int32_t)hd <= 0 */ )
	{
		lprintf( WIDE( "out of bounds" ) );
		return;
	}
#ifdef _INVERT_IMAGE
	// set pointer in to the starting x pixel
	// on the last line of the image to be copied
	//pi = IMG_ADDRESS( pifSrc, xs, ys );
	//po = IMG_ADDRESS( pifDest, xd, yd );
	pi = IMG_ADDRESS( pifSrc, xs, ys );
	po = IMG_ADDRESS( pifDest, xd, yd );
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(492) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(493) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
	oo = 4*-(int)(ws+pifDest->pwidth);     // w is how much we can copy...
	oi = 4*-(int)(ws+pifSrc->pwidth); // adding remaining width...
#else
	// set pointer in to the starting x pixel
	// on the first line of the image to be copied...
	pi = IMG_ADDRESS( pifSrc, xs, ys );
	po = IMG_ADDRESS( pifDest, xd, yd );
	oo = 4*(pifDest->pwidth - ws);     // w is how much we can copy...
	oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
#endif
	//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	//oo = 4*(pifDest->pwidth - ws);     // w is how much we can copy...
	//oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	while( LockedExchange( &lock, 1 ) )
		Relinquish();

	if( ( pifDest->flags & IF_FLAG_FINAL_RENDER )
		&& !( pifDest->flags & IF_FLAG_IN_MEMORY ) )
	{
		Image topmost_parent;

		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->glActiveSurface )
		{
	        //lprintf( WIDE( "gl texture hasn't updated or went away?" ) );
		    lock = 0;
			return;
		}
		//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );
      //DebugBreak();        g

		// closed loop to get the top imgae size.
		for( topmost_parent = pifSrc; topmost_parent->pParent; topmost_parent = topmost_parent->pParent );

		/*
		 * only a portion of the image is actually used, the rest is filled with blank space
		 *
		 */
		TranslateCoord( pifDest, &xd, &yd );
		TranslateCoord( pifSrc, &xs, &ys );
		{
			int glDepth = 1;
			double x_size, x_size2, y_size, y_size2;
			VECTOR v1[2], v3[2],v4[2],v2[2];
			int v = 0;

			v1[v][0] = xd;
			v1[v][1] = yd;
			v1[v][2] = 0.0;

			v2[v][0] = xd;
			v2[v][1] = yd+hs;
			v2[v][2] = 0.0;

			v3[v][0] = xd+ws;
			v3[v][1] = yd+hs;
			v3[v][2] = 0.0;

			v4[v][0] = xd+ws;
			v4[v][1] = yd;
			v4[v][2] = 0.0;

			x_size = (double) xs/ (double)topmost_parent->width;
			x_size2 = (double) (xs+ws)/ (double)topmost_parent->width;
			y_size = (double) ys/ (double)topmost_parent->height;
			y_size2 = (double) (ys+hs)/ (double)topmost_parent->height;
			// Front Face
			//glColor4ub( 255,120,32,192 );
			//lprintf( WIDE( "Texture size is %g,%g to %g,%g" ), x_size, y_size, x_size2, y_size2 );
			while( pifDest && pifDest->pParent )
			{
				glDepth = 0;
				if( pifDest->transform )
				{
					Apply( pifDest->transform, v1[1-v], v1[v] );
					Apply( pifDest->transform, v2[1-v], v2[v] );
					Apply( pifDest->transform, v3[1-v], v3[v] );
					Apply( pifDest->transform, v4[1-v], v4[v] );
					v = 1-v;
				}
				pifDest = pifDest->pParent;
			}
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v1[1-v], v1[v] );
				Apply( pifDest->transform, v2[1-v], v2[v] );
				Apply( pifDest->transform, v3[1-v], v3[v] );
				Apply( pifDest->transform, v4[1-v], v4[v] );
				v = 1-v;
			}
#if 0
			if( glDepth )
			{
				//lprintf( WIDE( "enqable depth..." ) );
				glEnable( GL_DEPTH_TEST );
			}
			else
			{
				//lprintf( WIDE( "disable depth..." ) );
				glDisable( GL_DEPTH_TEST );
			}
#endif
			glBindTexture(GL_TEXTURE_2D, pifSrc->glActiveSurface);				// Select Our Texture
			if( method == BLOT_COPY )
				glColor4ub( 255,255,255,255 );
			else if( method == BLOT_SHADED )
			{
				CDATA tmp = va_arg( colors, CDATA );
				glColor4ubv( (GLubyte*)&tmp );
			}
			else if( method == BLOT_MULTISHADE )
			{
#if !defined( __ANDROID__ )
				InitShader();
				if( glUseProgram && l.glActiveSurface->shader.multi_shader )
				{
					int err;
					CDATA r = va_arg( colors, CDATA );
					CDATA g = va_arg( colors, CDATA );
					CDATA b = va_arg( colors, CDATA );
		 			glEnable(GL_FRAGMENT_PROGRAM_ARB);
					glUseProgram( l.glActiveSurface->shader.multi_shader );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, (float)GetRedValue( r )/255.0f, (float)GetGreenValue( r )/255.0f, (float)GetBlueValue( r )/255.0f, (float)GetAlphaValue( r )/255.0f );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, (float)GetRedValue( g )/255.0f, (float)GetGreenValue( g )/255.0f, (float)GetBlueValue( g )/255.0f, (float)GetAlphaValue( g )/255.0f );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2, (float)GetRedValue( b )/255.0f, (float)GetGreenValue( b )/255.0f, (float)GetBlueValue( b )/255.0f, (float)GetAlphaValue( b )/255.0f );					
					err = glGetError();
				}
				else
#endif
				{
					Image output_image;
					CDATA r = va_arg( colors, CDATA );
					CDATA g = va_arg( colors, CDATA );
					CDATA b = va_arg( colors, CDATA );
					output_image = GetShadedImage( pifSrc, r, g, b );
					glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					glColor4ub( 255,255,255,255 );
				}
			}
			else if( method == BLOT_INVERTED )
			{
#if !defined( __ANDROID__ )
				InitShader();
				if( l.glActiveSurface->shader.inverse_shader )
				{
					int err;
		 			glEnable(GL_FRAGMENT_PROGRAM_ARB);
					glUseProgram( l.glActiveSurface->shader.inverse_shader );
					err = glGetError();
				}
				else
#endif
				{
					Image output_image;
					output_image = GetInvertedImage( pifSrc );
					glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					glColor4ub( 255,255,255,255 );
				}
			}
			glBegin(GL_TRIANGLE_STRIP);
			//glBegin(GL_QUADS);
			scale( v1[v], v1[v], l.scale );
			scale( v2[v], v2[v], l.scale );
			scale( v3[v], v3[v], l.scale );
			scale( v4[v], v4[v], l.scale );
			glTexCoord2f(x_size, y_size); glVertex3fv(v1[v]);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(x_size, y_size2); glVertex3fv(v2[v]);	// Top Left Of The Texture and Quad
			glTexCoord2f(x_size2, y_size); glVertex3fv(v4[v]);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(x_size2, y_size2); glVertex3fv(v3[v]);	// Top Right Of The Texture and Quad
			// Back Face
			glEnd();
#if !defined( __ANDROID__ )
			if( method == BLOT_MULTISHADE )
			{
				if( l.glActiveSurface->shader.multi_shader )
				{
 					glDisable(GL_FRAGMENT_PROGRAM_ARB);
				}
			}
			else if( method == BLOT_INVERTED )
			{
				if( l.glActiveSurface->shader.inverse_shader )
				{
 					glDisable(GL_FRAGMENT_PROGRAM_ARB);
				}
			}
#endif
			glBindTexture(GL_TEXTURE_2D, 0);				// Select Our Texture
		}
	}
	else
	{
		switch( method )
		{
		case BLOT_COPY:
			if( !nTransparent )
				CopyPixelsT0( po, pi, oo, oi, ws, hs );
			else if( nTransparent == 1 )
				CopyPixelsT1( po, pi, oo, oi, ws, hs );
			else if( nTransparent & ALPHA_TRANSPARENT )
				CopyPixelsTImgA( po, pi, oo, oi, ws, hs, nTransparent & 0xFF);
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				CopyPixelsTImgAI( po, pi, oo, oi, ws, hs, nTransparent & 0xFF );
			else
				CopyPixelsTA( po, pi, oo, oi, ws, hs, nTransparent );
			break;
		case BLOT_SHADED:
			if( !nTransparent )
				CopyPixelsShadedT0( po, pi, oo, oi, ws, hs
                           , va_arg( colors, CDATA ) );
			else if( nTransparent == 1 )
				CopyPixelsShadedT1( po, pi, oo, oi, ws, hs
                           , va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT )
				CopyPixelsShadedTImgA( po, pi, oo, oi, ws, hs, nTransparent & 0xFF
                           , va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				CopyPixelsShadedTImgAI( po, pi, oo, oi, ws, hs, nTransparent & 0xFF
                           , va_arg( colors, CDATA ) );
			else
				CopyPixelsShadedTA( po, pi, oo, oi, ws, hs, nTransparent
                           , va_arg( colors, CDATA ) );
			break;
		case BLOT_MULTISHADE:
			{
				CDATA r,g,b;
				r = va_arg( colors, CDATA );
				g = va_arg( colors, CDATA );
				b = va_arg( colors, CDATA );
				//lprintf( WIDE( "r g b %08x %08x %08x" ), r,g, b );
				if( !nTransparent )
					CopyPixelsMultiT0( po, pi, oo, oi, ws, hs
										  , r, g, b );
				else if( nTransparent == 1 )
					CopyPixelsMultiT1( po, pi, oo, oi, ws, hs
										  , r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT )
					CopyPixelsMultiTImgA( po, pi, oo, oi, ws, hs, nTransparent & 0xFF
											  , r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
					CopyPixelsMultiTImgAI( po, pi, oo, oi, ws, hs, nTransparent & 0xFF
												, r, g, b );
				else
					CopyPixelsMultiTA( po, pi, oo, oi, ws, hs, nTransparent
										  , r, g, b );
			}
			break;
		}
		MarkImageUpdated( pifDest );
	}
	lock = 0;
	//lprintf( WIDE( "Image done.." ) );
}
// copy all of pifSrc to the destination - placing the upper left
// corner of pifSrc on the point specified.
void  BlotImageEx ( ImageFile *pifDest, ImageFile *pifSrc, int32_t xd, int32_t yd, uint32_t nTransparent, uint32_t method, ... )
{
	va_list colors;
	CDATA r;
	CDATA g;
	CDATA b;
	va_start( colors, method );
	r = va_arg( colors, CDATA );
	g = va_arg( colors, CDATA );
	b = va_arg( colors, CDATA );
	BlotImageSizedEx( pifDest, pifSrc, xd, yd, 0, 0
                   , pifSrc->real_width, pifSrc->real_height, nTransparent, method
                                      , r,g,b
                                    );
}
#ifdef __cplusplus
}; //namespace sack::image {
}; //namespace sack::image {
#endif


// $Log: blotdirect.c,v $
// Revision 1.21  2003/12/13 08:26:57  panther
// Fix blot direct for image bound having been set (break natural?)
//
// Revision 1.20  2003/09/21 20:47:26  panther
// Removed noisy logging messages.
//
// Revision 1.19  2003/09/21 16:25:28  panther
// Removed much noisy logging, all in the interest of sheet controls.
// Fixed some linking of services.
// Fixed service close on dead client.
//
// Revision 1.18  2003/09/12 14:37:55  panther
// Fix another overflow in drawing sources rects greater than source image
//
// Revision 1.17  2003/09/12 14:12:54  panther
// Fix apparently blotting image problem...
//
// Revision 1.16  2003/08/30 10:05:01  panther
// Fix clipping blotted images beyond dest boundries
//
// Revision 1.15  2003/08/14 11:57:48  panther
// Okay - so this blotdirect code definatly works....
//
// Revision 1.14  2003/08/13 16:24:22  panther
// Well - found what was broken...
//
// Revision 1.13  2003/08/13 16:14:11  panther
// Remove stupid timing...
//
// Revision 1.12  2003/07/31 08:55:30  panther
// Fix blotscaled boundry calculations - perhaps do same to blotdirect
//
// Revision 1.11  2003/07/25 00:08:31  panther
// Fixeup all copyies, scaled and direct for watcom
//
// Revision 1.10  2003/07/01 08:54:13  panther
// Fix seg fault when blotting soft cursor over bottom of screen
//
// Revision 1.9  2003/04/25 08:33:09  panther
// Okay move the -1's back out of IMG_ADDRESS
//
// Revision 1.8  2003/04/24 00:03:49  panther
// Added ColorAverage to image... Fixed a couple macros
//
// Revision 1.7  2003/03/30 18:39:03  panther
// Update image blotters to use IMG_ADDRESS
//
// Revision 1.6  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
