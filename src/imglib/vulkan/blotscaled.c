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
   uint32_t left;
   uint32_t right;
   uint32_t top;
   uint32_t bottom;
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
   uint32_t x, y;                     \
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
      uint32_t alpha;                                   \
      if( (cin = *pi) )                              \
      {                                            \
         alpha = ( cin & 0xFF000000 ) >> 24;       \
         alpha -= nTransparent;                    \
         if( alpha > 1 )                           \
            *po = DOALPHA2( *po, cin, alpha );      \
      }


//---------------------------------------------------------------------------

         static void BlotScaledT0( SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledT1( SCALED_BLOT_WORK_PARAMS
                        )
{
   ScaleLoopStart
      if( *pi )  *(po) = *(pi);
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

static void BlotScaledTA( SCALED_BLOT_WORK_PARAMS
                      , uint32_t nTransparent )
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

static void BlotScaledTImgA(SCALED_BLOT_WORK_PARAMS
                      , uint32_t nTransparent )
{
   ScaleLoopStart
      CDATA cin;                                  
      uint32_t alpha;                                  
      if( (cin = *pi) )                             
      {                                           
         alpha = ( cin & 0xFF000000 ) >> 24;      
         alpha += nTransparent;
         *po = DOALPHA2( *po, cin, alpha );        
      }
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

static void BlotScaledTImgAI( SCALED_BLOT_WORK_PARAMS
                      , uint32_t nTransparent )
{

   ScaleLoopStart
      CDATA cin;                             
      int32_t alpha;
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

static void BlotScaledShadedT0( SCALED_BLOT_WORK_PARAMS
                       , CDATA shade )
{
   ScaleLoopStart

      *(po) = SHADEPIXEL( *pi, shade );

   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

static void BlotScaledShadedT1( SCALED_BLOT_WORK_PARAMS
                       , CDATA shade )
{
   ScaleLoopStart
      if( *pi )
         *(po) = SHADEPIXEL( *pi, shade );
   ScaleLoopEnd
}                    

//---------------------------------------------------------------------------

static void BlotScaledShadedTA( SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
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
static void BlotScaledShadedTImgA( SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
                       , CDATA shade )
{
   ScaleLoopStart
      CDATA cin;
      uint32_t alpha;
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
static void BlotScaledShadedTImgAI( SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
                       , CDATA shade )
{
   ScaleLoopStart
      CDATA cin;
      uint32_t alpha;
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

static void BlotScaledMultiT0( SCALED_BLOT_WORK_PARAMS
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
		uint32_t rout, gout, bout;
	   *(po) = MULTISHADEPIXEL( *pi, r, g, b );
   ScaleLoopEnd

}
            
//---------------------------------------------------------------------------

static void BlotScaledMultiT1(  SCALED_BLOT_WORK_PARAMS
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      if( *pi )
      {
         uint32_t rout, gout, bout;
         *(po) = MULTISHADEPIXEL( *pi, r, g, b );
      }
   ScaleLoopEnd

}
//---------------------------------------------------------------------------

static void BlotScaledMultiTA(  SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      if( (cin = *pi) )
      {
         uint32_t rout, gout, bout;
         cin = MULTISHADEPIXEL( cin, r, g, b );
         *po = DOALPHA2( *po, cin, nTransparent );
      }
   ScaleLoopEnd

}

//---------------------------------------------------------------------------

static void BlotScaledMultiTImgA( SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      uint32_t alpha;
      if( (cin = *pi) )
      {
         uint32_t rout, gout, bout;
         cin = MULTISHADEPIXEL( cin, r, g, b );
         alpha = ( cin & 0xFF000000 ) >> 24;
         alpha += nTransparent;
         *po = DOALPHA2( *po, cin, alpha );
      }
   ScaleLoopEnd

}

//---------------------------------------------------------------------------

static void BlotScaledMultiTImgAI( SCALED_BLOT_WORK_PARAMS
                       , uint32_t nTransparent 
                       , CDATA r
                       , CDATA g
                       , CDATA b )
{
   ScaleLoopStart
      CDATA cin;
      uint32_t alpha;
      if( (cin = *pi) )
      {
         uint32_t rout, gout, bout;
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
                                    , int32_t xd, int32_t yd
                                    , uint32_t wd, uint32_t hd
                                    , int32_t xs, int32_t ys
                                    , uint32_t ws, uint32_t hs
                                    , uint32_t nTransparent
                                    , uint32_t method, ... )
     // integer scalar... 0x10000 = 1
{
	CDATA *po, *pi;
	static uint32_t lock;
	uintptr_t  oo;
	uintptr_t srcwidth;
	int errx, erry;
	uint32_t dhd, dwd, dhs, dws;
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
		//ws -= ((int64_t)( (int)wd - newwd)* (int64_t)ws )/(int)wd;
		wd = ( pifDest->x + pifDest->width ) - xd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( yd + (signed)hd ) > (pifDest->y + pifDest->height) )
	{
		//int newhd = TOFIXED(pifDest->height);
		//hs -= ((int64_t)( hd - newhd)* hs )/hd;
		hd = (pifDest->y + pifDest->height) - yd;
	}
	if( (int32_t)wd <= 0 ||
       (int32_t)hd <= 0 ||
       (int32_t)ws <= 0 ||
		 (int32_t)hs <= 0 )
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
		if( !pifSrc->vkActiveSurface )
		{
			lprintf( WIDE( "gl texture hasn't downloaded or went away?" ) );
			lock = 0;
			return;
		}
		//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );

		{
			struct image_shader_op *op;// = BeginImageShaderOp( GetShader( WIDE("Simple Texture") ), pifDest, pifSrc->glActiveSurface  );
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
				op = BeginImageShaderOp( GetShader( WIDE("Simple Texture") ), pifDest, pifSrc->vkActiveSurface );
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

				op = BeginImageShaderOp( GetShader( WIDE("Simple MultiShaded Texture") ), pifDest, pifSrc->vkActiveSurface );
				AppendShaderTristripQuad( op, v[vi], pifSrc->vkActiveSurface, texture_v, r_color, g_color, b_color );
			}
			else if( method == BLOT_INVERTED )
			{
#if !defined( __ANDROID__ ) && !defined( __QNX__ )
				if( l.vkActiveSurface->shader.inverse_shader )
				{
					int err;
					//lprintf( WIDE( "HAVE SHADER %d" ), l.glActiveSurface->shader.inverse_shader );
					//////glEnable(GL_FRAGMENT_PROGRAM_ARB);
					//////glUseProgram( l.vkActiveSurface->shader.inverse_shader );
					//err = glGetError();
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

			//glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		}
	}
	else
	{
		switch( method )
		{
		case BLOT_COPY:
			if( !nTransparent )
				BlotScaledT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );
			else if( nTransparent == 1 )
				BlotScaledT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );
			else if( nTransparent & ALPHA_TRANSPARENT )
				BlotScaledTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				BlotScaledTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
			else
				BlotScaledTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent );
			break;
		case BLOT_SHADED:
			if( !nTransparent )
				BlotScaledShadedT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
			else if( nTransparent == 1 )
				BlotScaledShadedT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT )
				BlotScaledShadedTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				BlotScaledShadedTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
			else
				BlotScaledShadedTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent, va_arg( colors, CDATA ) );
			break;
		case BLOT_MULTISHADE:
			{
				CDATA r,g,b;
				r = va_arg( colors, CDATA );
				g = va_arg( colors, CDATA );
				b = va_arg( colors, CDATA );
				if( !nTransparent )
					BlotScaledMultiT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
											, r, g, b );
				else if( nTransparent == 1 )
					BlotScaledMultiT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
											, r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT )
					BlotScaledMultiTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
												, nTransparent & 0xFF
												, r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
					BlotScaledMultiTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
												 , nTransparent & 0xFF
												 , r, g, b );
				else
					BlotScaledMultiTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
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
