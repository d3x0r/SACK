/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle putting out one image scaled onto another image.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */





#define NO_TIMING_LOGGING
#ifndef NO_TIMING_LOGGING
#include <stdhdrs.h>
#include <logging.h>
#endif

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#include <stdhdrs.h>

#if defined( __ANDROID__ )
#include <gles\gl.h>         // Header File For The OpenGL32 Library
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
//#include <gl\glu.h>        // Header File For The GLu32 Library
#endif

#include <sharemem.h>
#include <imglib/imagestruct.h>
#include <colordef.h>
#include "image.h"
#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"
#include "../image_common.h"

#ifdef __cplusplus 
namespace sack {
namespace image {
#endif


#if !defined( _WIN32 ) && !defined( NO_TIMING_LOGGING )
	// as long as I don't include windows.h...
typedef struct rect_tag {
   _32 left;
   _32 right;
   _32 top;
   _32 bottom;
} RECT;
#endif

#define TOFIXED(n)   ((n)<<FIXED_SHIFT)
#define FROMFIXED(n)   ((n)>>FIXED_SHIFT)
#define TOPFROMFIXED(n) (((n)+(FIXED-1))>>FIXED_SHIFT)
#define FIXEDPART(n)    ((n)&(FIXED-1))
#define FIXED        1024
#define FIXED_SHIFT  10

//---------------------------------------------------------------------------

#define ScaleLoopStart int errx, erry; \
   _32 x, y;                     \
   PCDATA _pi = pi;              \
   erry = i_erry;                \
   y = 0;                        \
   while( y < hd )               \
   {                            \
      /* move first line.... */ \
      errx = i_errx;            \
      x = 0;                    \
      pi = _pi;                 \
      while( x < wd )           \
      {                         \
         {

#define ScaleLoopEnd  }             \
         po++;                      \
         x++;                       \
         errx += (signed)dws; /* add source width */\
         while( errx >= 0 )               \
         {                                \
            errx -= (signed)dwd; /* fix backwards the width we're copying*/\
            pi++;                         \
         }                                \
      }                                   \
      po = (CDATA*)(((char*)po) + oo);    \
      y++;                                \
      erry += (signed)dhs;                        \
      while( erry >= 0 )                  \
      {                                   \
         erry -= (signed)dhd;                      \
         _pi = (CDATA*)(((char*)_pi) + srcpwidth); /* go to next line start*/\
      }                                   \
   }

//---------------------------------------------------------------------------

#define PIXCOPY   

#define TCOPY  if( *pi )  *(po) = *(pi);

#define ACOPY  CDATA cin;  if( cin = *pi )        \
      {                                           \
         *po = DOALPHA2( *po, cin, nTransparent ); \
      }

#define IMGACOPY     CDATA cin;                    \
      int alpha;                                   \
      if( cin = *pi )                              \
      {                                            \
         alpha = ( cin & 0xFF000000 ) >> 24;       \
         alpha += nTransparent;                    \
         *po = DOALPHA2( *po, cin, alpha );         \
      }

#define IMGINVACOPY  CDATA cin;                    \
      _32 alpha;                                   \
      if( (cin = *pi) )                              \
      {                                            \
         alpha = ( cin & 0xFF000000 ) >> 24;       \
         alpha -= nTransparent;                    \
         if( alpha > 1 )                           \
            *po = DOALPHA2( *po, cin, alpha );      \
      }


//---------------------------------------------------------------------------

         void CPROC cBlotScaledT0( SCALED_BLOT_WORK_PARAMS
                                 )
{
	ScaleLoopStart
		if( AlphaVal(*pi) == 0 )
			(*po) = 0;
		else
			(*po) = (*pi);
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledT1( SCALED_BLOT_WORK_PARAMS
                        )
{
   ScaleLoopStart
      if( *pi )  *(po) = *(pi);
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledTA( SCALED_BLOT_WORK_PARAMS
                      , _32 nTransparent )
{
   ScaleLoopStart
      CDATA cin;  
      if( (cin = *pi) )        
		{
         *po = DOALPHA2( *po, cin, nTransparent ); 
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledTImgA(SCALED_BLOT_WORK_PARAMS
                      , _32 nTransparent )
{
   ScaleLoopStart
      CDATA cin;                                  
      _32 alpha;                                  
      if( (cin = *pi) )                             
      {                                           
         alpha = ( cin & 0xFF000000 ) >> 24;      
         alpha += nTransparent;
         *po = DOALPHA2( *po, cin, alpha );        
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledTImgAI( SCALED_BLOT_WORK_PARAMS
                      , _32 nTransparent )
{

   ScaleLoopStart
      CDATA cin;                             
      S_32 alpha;
      if( (cin = *pi) )                        
      {                                      
         alpha = ( cin & 0xFF000000 ) >> 24; 
         alpha -= nTransparent;              
         if( alpha > 1 )                     
            *po = DOALPHA2( *po, cin, alpha );
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledShadedT0( SCALED_BLOT_WORK_PARAMS
                       , CDATA shade )
{
   ScaleLoopStart

      *(po) = SHADEPIXEL( *pi, shade );

   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledShadedT1( SCALED_BLOT_WORK_PARAMS
                       , CDATA shade )
{
   ScaleLoopStart
      if( *pi )
         *(po) = SHADEPIXEL( *pi, shade );
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledShadedTA( SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA shade )
{
   ScaleLoopStart
      CDATA cin;
      if( (cin = *pi) )
      {
         cin = SHADEPIXEL( cin, shade );
         *po = DOALPHA2( *po, cin, nTransparent );
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------
void CPROC cBlotScaledShadedTImgA( SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA shade )
{
   ScaleLoopStart
      CDATA cin;
      _32 alpha;
      if( (cin = *pi) )
      {
         alpha = ( cin & 0xFF000000 ) >> 24;
         alpha += nTransparent;
         cin = SHADEPIXEL( cin, shade );
         *po = DOALPHA2( *po, cin, alpha );
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------
void CPROC cBlotScaledShadedTImgAI( SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA shade )
{
   ScaleLoopStart
      CDATA cin;
      _32 alpha;
      if( (cin = *pi) )
      {
         alpha = ( cin & 0xFF000000 ) >> 24;
         alpha -= nTransparent;
         if( alpha > 1 )
         {
            cin = SHADEPIXEL( cin, shade );
            *po = DOALPHA2( *po, cin, alpha );
         }
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

void CPROC cBlotScaledMultiT0( SCALED_BLOT_WORK_PARAMS
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
		_32 rout, gout, bout;
	   *(po) = MULTISHADEPIXEL( *pi, r, g, b );
   ScaleLoopEnd

}
            
//---------------------------------------------------------------------------

void CPROC cBlotScaledMultiT1(  SCALED_BLOT_WORK_PARAMS
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      if( *pi )
      {
         _32 rout, gout, bout;
         *(po) = MULTISHADEPIXEL( *pi, r, g, b );
      }
   ScaleLoopEnd

}
//---------------------------------------------------------------------------

void CPROC cBlotScaledMultiTA(  SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      if( (cin = *pi) )
      {
         _32 rout, gout, bout;
         cin = MULTISHADEPIXEL( cin, r, g, b );
         *po = DOALPHA2( *po, cin, nTransparent );
      }
   ScaleLoopEnd

}

//---------------------------------------------------------------------------

void CPROC cBlotScaledMultiTImgA( SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      _32 alpha;
      if( (cin = *pi) )
      {
         _32 rout, gout, bout;
         cin = MULTISHADEPIXEL( cin, r, g, b );
         alpha = ( cin & 0xFF000000 ) >> 24;
         alpha += nTransparent;
         *po = DOALPHA2( *po, cin, alpha );
      }
   ScaleLoopEnd

}

//---------------------------------------------------------------------------

void CPROC cBlotScaledMultiTImgAI( SCALED_BLOT_WORK_PARAMS
                       , _32 nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      _32 alpha;
      if( (cin = *pi) )
      {
         _32 rout, gout, bout;
			cin = MULTISHADEPIXEL( cin, r, g, b );
			alpha = ( cin & 0xFF000000 ) >> 24;
			alpha -= nTransparent;
			if( alpha > 1 )
			{
				*po = DOALPHA2( *po, cin, alpha );
         }
      }
   ScaleLoopEnd

}

//---------------------------------------------------------------------------
// x, y location on dest
// w, h are actual width and height to span...

 void  BlotScaledImageSizedEx ( ImageFile *pifDest, ImageFile *pifSrc
                                    , S_32 xd, S_32 yd
                                    , _32 wd, _32 hd
                                    , S_32 xs, S_32 ys
                                    , _32 ws, _32 hs
                                    , _32 nTransparent
                                    , _32 method, ... )
     // integer scalar... 0x10000 = 1
{
	CDATA *po, *pi;
	static _32 lock;
	_32  oo;
	_32 srcwidth;
	int errx, erry;
	_32 dhd, dwd, dhs, dws;
	va_list colors;
	va_start( colors, method );
	//lprintf( WIDE("Blot enter (%d,%d)"), _wd, _hd );
	if( nTransparent > ALPHA_TRANSPARENT_MAX )
	{
		return;
	}
	if( !pifSrc ||  !pifDest
	   || !pifSrc->image //|| !pifDest->image
	   || !wd || !hd || !ws || !hs )
	{
		return;
	}

	if( ( xd > ( pifDest->x + pifDest->width ) )
	  || ( yd > ( pifDest->y + pifDest->height ) )
	  || ( ( xd + (signed)wd ) < pifDest->x )
		|| ( ( yd + (signed)hd ) < pifDest->y ) )
	{
		return;
	}
	dhd = hd;
	dhs = hs;
	dwd = wd;
	dws = ws;

	// ok - how to figure out how to do this
	// need to update the position and width to be within the 
	// the bounds of pifDest....
	//lprintf(" begin scaled output..." );
	errx = -(signed)dwd;
	erry = -(signed)dhd;

	if( xd < pifDest->x )
	{
		while( xd < pifDest->x )
		{
			errx += (signed)dws;
			while( errx >= 0 )
			{
	            errx -= (signed)dwd;
				ws--;
				xs++;
			}
			wd--;
			xd++;
		}
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( yd < pifDest->y )
	{
		while( yd < pifDest->y )
		{
			erry += (signed)dhs;
			while( erry >= 0 )
			{
				erry -= (signed)dhd;
				hs--;
				ys++;
			}
			hd--;
			yd++;
		}
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( xd + (signed)wd ) > ( pifDest->x + pifDest->width) )
	{
		//int newwd = TOFIXED(pifDest->width);
		//ws -= ((S_64)( (int)wd - newwd)* (S_64)ws )/(int)wd;
		wd = ( pifDest->x + pifDest->width ) - xd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( yd + (signed)hd ) > (pifDest->y + pifDest->height) )
	{
		//int newhd = TOFIXED(pifDest->height);
		//hs -= ((S_64)( hd - newhd)* hs )/hd;
		hd = (pifDest->y + pifDest->height) - yd;
	}
	if( (S_32)wd <= 0 ||
       (S_32)hd <= 0 ||
       (S_32)ws <= 0 ||
		 (S_32)hs <= 0 )
	{
		return;
	}
   
	//Log9( WIDE("Image locations: %d(%d %d) %d(%d) %d(%d) %d(%d)")
	//          , xs, FROMFIXED(xs), FIXEDPART(xs)
	//          , ys, FROMFIXED(ys)
	//          , xd, FROMFIXED(xd)
	//          , yd, FROMFIXED(yd) );
#ifdef _INVERT_IMAGE
	// set pointer in to the starting x pixel
	// on the last line of the image to be copied 
	pi = IMG_ADDRESS( pifSrc, (xs), (ys) );
	po = IMG_ADDRESS( pifDest, (xd), (yd) );
	oo = 4*(-((signed)wd) - (pifDest->pwidth) ); // w is how much we can copy...
	// adding in multiple of 4 because it's C...
	srcwidth = -(4* pifSrc->pwidth);
#else
	// set pointer in to the starting x pixel
	// on the first line of the image to be copied...
	pi = IMG_ADDRESS( pifSrc, (xs), (ys) );
	po = IMG_ADDRESS( pifDest, (xd), (yd) );
	oo = 4*(pifDest->pwidth - (wd)); // w is how much we can copy...
	// adding in multiple of 4 because it's C...
	srcwidth = 4* pifSrc->pwidth;
#endif
	while( LockedExchange( &lock, 1 ) )
		Relinquish();
   //Log8( WIDE("Do blot work...%d(%d),%d(%d) %d(%d) %d(%d)")
   //    , ws, FROMFIXED(ws), hs, FROMFIXED(hs) 
	//    , wd, FROMFIXED(wd), hd, FROMFIXED(hd) );

	if( ( pifDest->flags & IF_FLAG_FINAL_RENDER )
		&& !( pifDest->flags & IF_FLAG_IN_MEMORY ) )
	{
		int updated = 0;
		Image topmost_parent;

		// closed loop to get the top imgae size.
		for( topmost_parent = pifSrc; topmost_parent->pParent; topmost_parent = topmost_parent->pParent );

		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->glActiveSurface )
		{
			lprintf( WIDE( "gl texture hasn't downloaded or went away?" ) );
			lock = 0;
			return;
		}
		//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );

		{
			int glDepth = 1;
			VECTOR v1[2], v3[2], v4[2], v2[2];
			int v = 0;
			double x_size, x_size2, y_size, y_size2;
			/*
			 * only a portion of the image is actually used, the rest is filled with blank space
			 *
			 */
			TranslateCoord( pifDest, &xd, &yd );
			TranslateCoord( pifSrc, &xs, &ys );

			v1[v][0] = xd;
			v1[v][1] = yd;
			v1[v][2] = 0.0;

			v2[v][0] = xd;
			v2[v][1] = yd+hd;
			v2[v][2] = 0.0;

			v3[v][0] = xd+wd;
			v3[v][1] = yd+hd;
			v3[v][2] = 0.0;

			v4[v][0] = xd+wd;
			v4[v][1] = yd;
			v4[v][2] = 0.0;

			x_size = (double) xs/ (double)topmost_parent->width;
			x_size2 = (double) (xs+ws)/ (double)topmost_parent->width;
			y_size = (double) ys/ (double)topmost_parent->height;
			y_size2 = (double) (ys+hs)/ (double)topmost_parent->height;

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
					//lprintf( WIDE( "HAVE SHADER %d" ), l.glActiveSurface->shader.inverse_shader );
					glEnable(GL_FRAGMENT_PROGRAM_ARB);
					glUseProgram( l.glActiveSurface->shader.inverse_shader );
					err = glGetError();
				}
				else
#endif
				{
					Image output_image;
					//lprintf( WIDE( "DID NOT HAVE SHADER" ) );
					output_image = GetInvertedImage( pifSrc );
					glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					glColor4ub( 255,255,255,255 );
				}
			}

			glBegin(GL_TRIANGLE_STRIP);
			//glBegin(GL_QUADS);
			// Front Face
			//glColor4ub( 255,120,32,192 );
			scale( v1[v], v1[v], l.scale );
			scale( v2[v], v2[v], l.scale );
			scale( v3[v], v3[v], l.scale );
			scale( v4[v], v4[v], l.scale );
			glTexCoord2f(x_size, y_size); glVertex3fv(v1[v]);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(x_size, y_size2); glVertex3fv(v2[v]);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(x_size2, y_size); glVertex3fv(v4[v]);	// Top Left Of The Texture and Quad
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

	else switch( method )
	{
	case BLOT_COPY:
		if( !nTransparent )
			cBlotScaledT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );       
		else if( nTransparent == 1 )
			cBlotScaledT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );       
		else if( nTransparent & ALPHA_TRANSPARENT )
			cBlotScaledTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			cBlotScaledTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );        
		else
			cBlotScaledTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent );        
		break;
	case BLOT_SHADED:
		if( !nTransparent )
			cBlotScaledShadedT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
		else if( nTransparent == 1 )
			cBlotScaledShadedT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
		else if( nTransparent & ALPHA_TRANSPARENT )
			cBlotScaledShadedTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			cBlotScaledShadedTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
		else
			cBlotScaledShadedTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent, va_arg( colors, CDATA ) );
		break;
	case BLOT_MULTISHADE:
		{
			CDATA r,g,b;
			r = va_arg( colors, CDATA );
			g = va_arg( colors, CDATA );
			b = va_arg( colors, CDATA );
			if( !nTransparent )
				cBlotScaledMultiT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
									  , r, g, b );
			else if( nTransparent == 1 )
				cBlotScaledMultiT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
									  , r, g, b );
			else if( nTransparent & ALPHA_TRANSPARENT )
				cBlotScaledMultiTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
										  , nTransparent & 0xFF
										  , r, g, b );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				cBlotScaledMultiTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
											, nTransparent & 0xFF
											, r, g, b );
			else
				cBlotScaledMultiTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
									  , nTransparent
									  , r, g, b );
		}
		break;
	}
	lock = 0;
//   Log( WIDE("Blot done") );
}



#ifdef __cplusplus 
}//namespace sack {
}//namespace image {
#endif

//---------------------------------------------------------------------------

// $Log: blotscaled.c,v $
// Revision 1.29  2005/06/21 00:45:41  jim
// Fix image bound issue with scaled image blotting... Also add a custom error handler to png image loader
//
// Revision 1.30  2005/05/30 20:05:53  d3x0r
// okay right/bottom edge adjustment was wrong... corrected.
//
// Revision 1.29  2005/05/30 19:56:37  d3x0r
// Make blotscaled behave a lot better... respecting image boundrys much better...
//
// Revision 1.28  2004/09/01 03:27:20  d3x0r
// Control updates video display issues?  Image blot message go away...
//
// Revision 1.27  2004/08/11 12:52:36  d3x0r
// Should figure out where they hide flag isn't being set... vline had to check for height<0
//
// Revision 1.26  2004/06/21 07:47:08  d3x0r
// Account for newly moved structure files.
//
// Revision 1.25  2004/03/29 20:07:25  d3x0r
// Remove benchmark logging
//
// Revision 1.24  2004/01/12 00:34:54  panther
// Fix error really of always 0 comparison vs wd, ht
//
// Revision 1.23  2003/09/15 17:06:37  panther
// Fixed to image, display, controls, support user defined clipping , nearly clearing correct portions of frame when clearing hotspots...
//
// Revision 1.22  2003/08/20 15:53:31  panther
// Okay and assembly loops have been updated accordingly
//
// Revision 1.21  2003/08/20 14:22:23  panther
// Remove excess logging, unused parameters
//
// Revision 1.20  2003/08/20 13:59:13  panther
// Okay looks like the C layer blotscaled works...
//
// Revision 1.19  2003/08/20 08:07:12  panther
// some fixes to blot scaled... fixed to makefiles test projects... fixes to export containters lib funcs
//
// Revision 1.18  2003/08/12 15:11:08  panther
// Test fixed point bias for scaled clipping
//
// Revision 1.17  2003/08/12 15:09:32  panther
// Test fixed point bias for scaled clipping
//
// Revision 1.16  2003/08/01 07:56:12  panther
// Commit changes for logging...
//
// Revision 1.15  2003/08/01 00:17:34  panther
// minor cleanup for watcom compile
//
// Revision 1.14  2003/07/31 08:55:30  panther
// Fix blotscaled boundry calculations - perhaps do same to blotdirect
//
// Revision 1.13  2003/07/25 00:08:59  panther
// Fixeup all copyies, scaled and direct for watcom
//
// Revision 1.12  2003/04/25 08:33:09  panther
// Okay move the -1's back out of IMG_ADDRESS
//
// Revision 1.11  2003/03/30 21:17:40  panther
// Used wrong image names..
//
// Revision 1.10  2003/03/30 18:39:03  panther
// Update image blotters to use IMG_ADDRESS
//
// Revision 1.9  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
