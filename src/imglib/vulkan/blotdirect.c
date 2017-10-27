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

#include <imglib/imagestruct.h>
#include <image.h>

#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"
#include "shaders.h"
#include "../image_common.h"
IMAGE_NAMESPACE

//---------------------------------------------------------------------------

#define StartLoop CDATA *pi = params->pi; CDATA *po = params->po; \
   params->oo /= 4;    \
   params->oi /= 4;                   \
   {                          \
      uint32_t row= 0;             \
      while( row < params->hs )       \
      {                       \
         uint32_t col=0;           \
         while( col < params->ws )    \
         {                    \
            {

#define EndLoop   }           \
/*lprintf( "in %08x out %08x", ((CDATA*)pi)[0], ((CDATA*)po)[1] );*/ \
            po++;             \
            pi++;             \
            col++;            \
         }                    \
         pi += params->oi;            \
         po += params->oo;            \
         row++;               \
      }                       \
   }

struct bdParams {
	PCDATA po; PCDATA  pi;
	uintptr_t oo; uintptr_t oi;
	uint32_t ws; uint32_t hs;
	uint32_t nTransparent;
	CDATA c;
	CDATA r, g, b;
};

static void CopyPixelsT0( struct bdParams *params )
{
   StartLoop
            *po = *pi;
   EndLoop
}

//---------------------------------------------------------------------------

static void CopyPixelsT1( struct bdParams *params )
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

static void CopyPixelsTA( struct bdParams *params )
{
   StartLoop
            CDATA cin;
            if( (cin = *pi) )
				{
					*po = DOALPHA2( *po
									  , cin
									  , params->nTransparent );
            }
   EndLoop
}

//---------------------------------------------------------------------------

static void CopyPixelsTImgA( struct bdParams *params )
{
   StartLoop
            uint32_t alpha;
            CDATA cin;
            if( (cin = *pi) )
            {
               alpha = ( cin & 0xFF000000 ) >> 24;
               alpha += params->nTransparent;
               *po = DOALPHA2( *po, cin, alpha );
            }
   EndLoop
}
//---------------------------------------------------------------------------

static void CopyPixelsTImgAI( struct bdParams *params )
{
   StartLoop
            int32_t alpha;

            CDATA cin;
            if( (cin = *pi) )
            {
               alpha = ( cin & 0xFF000000 ) >> 24;
               alpha -= params->nTransparent;
               if( alpha > 1 )
               {
                  *po = DOALPHA2( *po, cin, alpha );
               }
            }
   EndLoop
}

//---------------------------------------------------------------------------

static void CopyPixelsShadedT0( struct bdParams *params )
{
   StartLoop
            uint32_t pixel;
            pixel = *pi;
            *po = SHADEPIXEL(pixel, params->c);
   EndLoop
}

//---------------------------------------------------------------------------
 void CPROC CopyPixelsShadedT1( struct bdParams *params )
{
   StartLoop
            uint32_t pixel;
            if( (pixel = *pi) )
            {
               *po = SHADEPIXEL(pixel, params->c);
            }
   EndLoop
}
//---------------------------------------------------------------------------

static void CopyPixelsShadedTA( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               pixout = SHADEPIXEL(pixel, params->c);
               *po = DOALPHA2( *po, pixout, params->nTransparent );
            }
   EndLoop
}

//---------------------------------------------------------------------------
static void CopyPixelsShadedTImgA( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t alpha;
               pixout = SHADEPIXEL(pixel, params->c);
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha += params->nTransparent;
               *po = DOALPHA2( *po, pixout, alpha );
            }
   EndLoop
}

//---------------------------------------------------------------------------
static void CopyPixelsShadedTImgAI( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha -= params->nTransparent;
               if( alpha > 1 )
               {
                  pixout = SHADEPIXEL(pixel, params->c);
                  *po = DOALPHA2( *po, pixout, alpha );
               }
            }
   EndLoop
}


//---------------------------------------------------------------------------

static void CopyPixelsMultiT0( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            {
               uint32_t rout, gout, bout;
               pixel = *pi;
			   pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b );
   }
            *po = pixout;
   EndLoop
}

//---------------------------------------------------------------------------
static void CopyPixelsMultiT1( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
			   pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b );

               *po = pixout;
            }
   EndLoop
}
//---------------------------------------------------------------------------
static void CopyPixelsMultiTA( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
			   pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b );
			   *po = DOALPHA2( *po, pixout, params->nTransparent );
            }
   EndLoop
}
//---------------------------------------------------------------------------
static void CopyPixelsMultiTImgA( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha += params->nTransparent;
			   pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b );
			   //lprintf( "pixel %08x pixout %08x r %08x g %08x b %08x", pixel, pixout, r,g,b);
               *po = DOALPHA2( *po, pixout, alpha );
            }
   EndLoop
}
//---------------------------------------------------------------------------
static void CopyPixelsMultiTImgAI( struct bdParams *params )
{
   StartLoop
            uint32_t pixel, pixout;
            if( (pixel = *pi) )
            {
               uint32_t rout, gout, bout;
               uint32_t alpha;
               alpha = ( pixel & 0xFF000000 ) >> 24;
               alpha -= params->nTransparent;
               if( alpha > 1 )
               {
                  pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b);
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
	struct bdParams bd;
	//int  hd, wd;
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
	bd.pi = IMG_ADDRESS( pifSrc, xs, ys );
	bd.po = IMG_ADDRESS( pifDest, xd, yd );
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(492) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(493) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
	bd.oo = 4*-(int)(ws+pifDest->pwidth);     // w is how much we can copy...
	bd.oi = 4*-(int)(ws+pifSrc->pwidth); // adding remaining width...
#else
	// set pointer in to the starting x pixel
	// on the first line of the image to be copied...
	bd.pi = IMG_ADDRESS( pifSrc, xs, ys );
	bd.po = IMG_ADDRESS( pifDest, xd, yd );
	bd.oo = 4*(pifDest->pwidth - ws);     // w is how much we can copy...
	bd.oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
#endif
	while( LockedExchange( &lock, 1 ) )
		Relinquish();

	if( ( pifDest->flags & IF_FLAG_FINAL_RENDER )
		&& !( pifDest->flags & IF_FLAG_IN_MEMORY ) )

	{
		Image topmost_parent;

		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->vkActiveSurface )
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
				op = BeginImageShaderOp( GetShader( WIDE("Simple Texture") ), pifDest, pifSrc->vkActiveSurface  );
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

				op = BeginImageShaderOp( GetShader( WIDE("Simple Shaded Texture") ), pifDest, pifSrc->vkActiveSurface, _color  );
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

				op = BeginImageShaderOp( GetShader( WIDE("Simple MultiShaded Texture") ), pifDest, pifSrc->vkActiveSurface, r_color, g_color, b_color );
				AppendImageShaderOpTristrip( op, 2, v[vi], texture_v );
			}
			else if( method == BLOT_INVERTED )
			{
#if PORTED
#if !defined( __ANDROID__ ) && !defined( __QNX__ )
				if( l.vkActiveSurface->shader.inverse_shader )
				{
					int err;
					lprintf( "need to add this to command stream....")
		 			//glEnable(GL_FRAGMENT_PROGRAM_ARB);
					//glUseProgram( l.vkActiveSurface->shader.inverse_shader );
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
#endif
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
				CopyPixelsT0( &bd );
			else if( nTransparent == 1 )
				CopyPixelsT1( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT ) {
				bd.nTransparent = nTransparent & 0xFF;
				CopyPixelsTImgA( &bd );
			}
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT ) {
				bd.nTransparent = nTransparent & 0xFF;
				CopyPixelsTImgAI( &bd );
			}
			else {
				bd.nTransparent = nTransparent;// &0xFF;
				CopyPixelsTA( &bd );
			}
			break;
		case BLOT_SHADED:
			bd.c = va_arg( colors, CDATA );
			if( !nTransparent )
				CopyPixelsShadedT0( &bd );
			else if( nTransparent == 1 )
				CopyPixelsShadedT1( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT ) {
				bd.nTransparent = nTransparent & 0xFF;
				CopyPixelsShadedTImgA( &bd );
			}
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT ) {
				bd.nTransparent = nTransparent & 0xFF;
				CopyPixelsShadedTImgAI( &bd );
			}
			else {
				bd.nTransparent = nTransparent;// &0xFF;
				CopyPixelsShadedTA( &bd );
			}
			break;
		case BLOT_MULTISHADE:
			{
				CDATA r,g,b;
				bd.r = va_arg( colors, CDATA );
				bd.g = va_arg( colors, CDATA );
				bd.b = va_arg( colors, CDATA );
				//lprintf( WIDE( "r g b %08x %08x %08x" ), r,g, b );
				if( !nTransparent )
					CopyPixelsMultiT0( &bd );
				else if( nTransparent == 1 )
					CopyPixelsMultiT1( &bd );
				else if( nTransparent & ALPHA_TRANSPARENT ) {
					bd.nTransparent = nTransparent & 0xFF;
					CopyPixelsMultiTImgA( &bd );
				}
				else if( nTransparent & ALPHA_TRANSPARENT_INVERT ) {
					bd.nTransparent = nTransparent & 0xFF;
					CopyPixelsMultiTImgAI( &bd );
				}
				else {
					bd.nTransparent = nTransparent;// &0xFF;
					CopyPixelsMultiTA( &bd );
				}
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
IMAGE_NAMESPACE_END
