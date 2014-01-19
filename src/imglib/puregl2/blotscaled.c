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

#if defined( USE_GLES2 )
//#include <GLES/gl.h>         // Header File For The OpenGL32 Library
#include <GLES2/gl2.h>         // Header File For The OpenGL32 Library
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
#include "shaders.h"
#include "../image_common.h"

IMAGE_NAMESPACE

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

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
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
			VECTOR v[2][4];
			float texture_v[4][2];
			int vi = 0;
			float x_size, x_size2, y_size, y_size2;
			/*
			 * only a portion of the image is actually used, the rest is filled with blank space
			 *
			 */
			TranslateCoord( pifDest, &xd, &yd );
			TranslateCoord( pifSrc, &xs, &ys );

			v[vi][0][0] = xd;
			v[vi][0][1] = yd;
			v[vi][0][2] = 0.0;

			v[vi][1][0] = xd+wd;
			v[vi][1][1] = yd;
			v[vi][1][2] = 0.0;

			v[vi][2][0] = xd;
			v[vi][2][1] = yd+hd;
			v[vi][2][2] = 0.0;

			v[vi][3][0] = xd+wd;
			v[vi][3][1] = yd+hd;
			v[vi][3][2] = 0.0;

			x_size = (float) xs/ (float)topmost_parent->width;
			x_size2 = (float) (xs+ws)/ (float)topmost_parent->width;
			y_size = (float) ys/ (float)topmost_parent->height;
			y_size2 = (float) (ys+hs)/ (float)topmost_parent->height;

         texture_v[0][0] = x_size;
         texture_v[0][1] = y_size;
         texture_v[1][0] = x_size2;
         texture_v[1][1] = y_size;
         texture_v[2][0] = x_size;
         texture_v[2][1] = y_size2;
         texture_v[3][0] = x_size2;
         texture_v[3][1] = y_size2;



			while( pifDest && pifDest->pParent )
			{
				glDepth = 0;
				if( pifDest->transform )
				{
					Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
					Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
					Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
					Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
					vi = 1-vi;
				}
				pifDest = pifDest->pParent;
			}
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
				Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
				Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
				Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}

			scale( v[vi][0], v[vi][0], l.scale );
			scale( v[vi][1], v[vi][1], l.scale );
			scale( v[vi][2], v[vi][2], l.scale );
			scale( v[vi][3], v[vi][3], l.scale );

			if( method == BLOT_COPY )
			{
				EnableShader( GetShader( WIDE("Simple Texture"), NULL ), v[vi], pifSrc->glActiveSurface, texture_v );
			}
			else if( method == BLOT_SHADED )
			{
				CDATA tmp = va_arg( colors, CDATA );
				float _color[4];
				_color[0] = RedVal( tmp ) / 255.0f;
				_color[1] = GreenVal( tmp ) / 255.0f;
				_color[2] = BlueVal( tmp ) / 255.0f;
				_color[3] = AlphaVal( tmp ) / 255.0f;

				EnableShader( GetShader( WIDE("Simple Shaded Texture"), NULL ), v[vi], pifSrc->glActiveSurface, texture_v, _color );
			}
			else if( method == BLOT_MULTISHADE )
			{
				CDATA r = va_arg( colors, CDATA );
				CDATA g = va_arg( colors, CDATA );
				CDATA b = va_arg( colors, CDATA );
				float r_color[4], g_color[4], b_color[4];
				r_color[0] = RedVal( r) / 255.0f;
				r_color[1] = GreenVal( r ) / 255.0f;
				r_color[2] = BlueVal( r ) / 255.0f;
				r_color[3] = AlphaVal( r ) / 255.0f;

				g_color[0] = RedVal( g ) / 255.0f;
				g_color[1] = GreenVal( g ) / 255.0f;
				g_color[2] = BlueVal( g ) / 255.0f;
				g_color[3] = AlphaVal( g ) / 255.0f;

				b_color[0] = RedVal( b ) / 255.0f;
				b_color[1] = GreenVal( b ) / 255.0f;
				b_color[2] = BlueVal( b ) / 255.0f;
				b_color[3] = AlphaVal( b ) / 255.0f;

				EnableShader( GetShader( WIDE("Simple MultiShaded Texture"), NULL ), v[vi], pifSrc->glActiveSurface, texture_v, r_color, g_color, b_color );
			}
			else if( method == BLOT_INVERTED )
			{
#if !defined( __ANDROID__ ) && !defined( __QNX__ )
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
					/**///glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					;/**///glColor4ub( 255,255,255,255 );
				}
			}

			glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		}
	}
	else
	{
		switch( method )
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
	}
	lock = 0;
	//   Log( WIDE("Blot done") );
}

IMAGE_NAMESPACE_END
