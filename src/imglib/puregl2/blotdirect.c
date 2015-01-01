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

#if defined( USE_GLES2 )
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

#include <imglib/imagestruct.h>
#include <image.h>

#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"
#include "shaders.h"
#include "../image_common.h"
IMAGE_NAMESPACE

//---------------------------------------------------------------------------

#define StartLoop oo /= 4;    \
   oi /= 4;                   \
   {                          \
      _32 row= 0;             \
      while( row < hs )       \
      {                       \
         _32 col=0;           \
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
                          , _32 oo, _32 oi
                          , _32 ws, _32 hs
                           )
{
   StartLoop
            *po = *pi;
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsT1( PCDATA po, PCDATA  pi
                          , _32 oo, _32 oi
                          , _32 ws, _32 hs
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
                          , _32 oo, _32 oi
                          , _32 ws, _32 hs
                          , _32 nTransparent )
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
                          , _32 oo, _32 oi
                          , _32 ws, _32 hs
                          , _32 nTransparent )
{
   StartLoop
            _32 alpha;
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
                          , _32 oo, _32 oi
                          , _32 ws, _32 hs
                          , _32 nTransparent )
{
   StartLoop
            S_32 alpha;

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
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , CDATA c )
{
   StartLoop
            _32 pixel;
            pixel = *pi;
            *po = SHADEPIXEL(pixel, c);
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedT1( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , CDATA c )
{
   StartLoop
            _32 pixel;
            if( (pixel = *pi) )
            {
               *po = SHADEPIXEL(pixel, c);
            }
   EndLoop
}
//---------------------------------------------------------------------------

 void CPROC cCopyPixelsShadedTA( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA c )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               pixout = SHADEPIXEL(pixel, c);
               *po = DOALPHA2( *po, pixout, nTransparent );
            }
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedTImgA( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA c )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 alpha;
               pixout = SHADEPIXEL(pixel, c);
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha += nTransparent;
               *po = DOALPHA2( *po, pixout, alpha );
            }
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA c )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 alpha;
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
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            _32 pixel, pixout;
            {
               _32 rout, gout, bout;
               pixel = *pi;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);
            }
            *po = pixout;
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiT1( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 rout, gout, bout;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);

               *po = pixout;
            }
   EndLoop
}
//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiTA( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 rout, gout, bout;
               pixout = MULTISHADEPIXEL( pixel, r,g,b);
               *po = DOALPHA2( *po, pixout, nTransparent );
            }
   EndLoop
}
//---------------------------------------------------------------------------
 void CPROC cCopyPixelsMultiTImgA( PCDATA po, PCDATA  pi
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 rout, gout, bout;
               _32 alpha;
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
                            , _32 oo, _32 oi
                            , _32 ws, _32 hs
                            , _32 nTransparent
                            , CDATA r, CDATA g, CDATA b )
{
   StartLoop
            _32 pixel, pixout;
            if( (pixel = *pi) )
            {
               _32 rout, gout, bout;
               _32 alpha;
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
                              , S_32 xd, S_32 yd
                              , S_32 xs, S_32 ys
                              , _32 ws, _32 hs
                              , _32 nTransparent
                              , _32 method
                              , ... )
{
#define BROKEN_CODE
	PCDATA po, pi;
	//int  hd, wd;
	_32 oo, oi; // treated as an adder... it is unsigned by math, but still results correct offset?
	static _32 lock;
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
	if( (S_32)ws <= 0 ||
        (S_32)hs <= 0 /*||
        (S_32)wd <= 0 ||
		(S_32)hd <= 0 */ )
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

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
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
			struct image_shader_op *op;// = BeginImageShaderOp( GetShader( WIDE("Simple Texture") ), pifDest, pifSrc->glActiveSurface  );
			int glDepth = 1;
			float x_size, x_size2, y_size, y_size2;
			float texture_v[4][2];
			VECTOR v[2][4];
			int vi = 0;

			v[vi][0][0] = xd;
			v[vi][0][1] = yd;
			v[vi][0][2] = 0.0;

			v[vi][1][0] = xd+ws;
			v[vi][1][1] = yd;
			v[vi][1][2] = 0.0;

			v[vi][2][0] = xd;
			v[vi][2][1] = yd+hs;
			v[vi][2][2] = 0.0;

			v[vi][3][0] = xd+ws;
			v[vi][3][1] = yd+hs;
			v[vi][3][2] = 0.0;

			x_size = (float) xs/ (float)topmost_parent->width;
			x_size2 = (float) (xs+ws)/ (float)topmost_parent->width;
			y_size = (float) ys/ (float)topmost_parent->height;
			y_size2 = (float) (ys+hs)/ (float)topmost_parent->height;
			// Front Face
			//glColor4ub( 255,120,32,192 );
			//lprintf( WIDE( "Texture size is %g,%g to %g,%g" ), x_size, y_size, x_size2, y_size2 );
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

         texture_v[0][0] = x_size;
         texture_v[0][1] = y_size;
         texture_v[1][0] = x_size2;
         texture_v[1][1] = y_size;
         texture_v[2][0] = x_size;
         texture_v[2][1] = y_size2;
         texture_v[3][0] = x_size2;
         texture_v[3][1] = y_size2;

			/**///glBindTexture(GL_TEXTURE_2D, pifSrc->glActiveSurface);				// Select Our Texture
			if( method == BLOT_COPY )
			{
				op = BeginImageShaderOp( GetShader( WIDE("Simple Texture") ), pifDest, pifSrc->glActiveSurface  );
				AppendImageShaderOpTristrip( op, 2, v[vi], texture_v );
			}
			else if( method == BLOT_SHADED )
			{
				CDATA tmp = va_arg( colors, CDATA );
				float _color[4];
				_color[0] = RedVal( tmp ) / 255.0f;
				_color[1] = GreenVal( tmp ) / 255.0f;
				_color[2] = BlueVal( tmp ) / 255.0f;
				_color[3] = AlphaVal( tmp ) / 255.0f;

				op = BeginImageShaderOp( GetShader( WIDE("Simple Shaded Texture") ), pifDest, pifSrc->glActiveSurface, _color  );
				AppendImageShaderOpTristrip( op, 2, v[vi], texture_v );
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

				op = BeginImageShaderOp( GetShader( WIDE("Simple MultiShaded Texture") ), pifDest, pifSrc->glActiveSurface, r_color, g_color, b_color );
				AppendImageShaderOpTristrip( op, 2, v[vi], texture_v );
			}
			else if( method == BLOT_INVERTED )
			{
#if !defined( __ANDROID__ ) && !defined( __QNX__ )
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
					/**///glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					;/**///glColor4ub( 255,255,255,255 );
				}
			}

			//glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

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
void  BlotImageEx ( ImageFile *pifDest, ImageFile *pifSrc, S_32 xd, S_32 yd, _32 nTransparent, _32 method, ... )
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
IMAGE_NAMESPACE_END
