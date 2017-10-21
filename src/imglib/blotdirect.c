/*
 *  Crafted by Jim Buckeyne
 *	(c)1999-2006++ Freedom Collective
 *
 *  Support for putting one image on another without scaling.
 *
 *
 *
 *  consult doc/image.html
 *
 */
#define FORCE_COLOR_MACROS

#define IMAGE_LIBRARY_SOURCE

#include <stdhdrs.h>
#include <sharemem.h>
#include <imglib/imagestruct.h>
#include <image.h>

#define NEED_ALPHA2
#include "blotproto.h"

#ifdef __cplusplus
namespace sack {
namespace image {
#endif


//---------------------------------------------------------------------------

#define StartLoop CDATA *po = params->po; CDATA *pi = params->pi; \
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

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

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
			alpha = (cin & 0xFF000000) >> 24;
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
			alpha = (cin & 0xFF000000) >> 24;
			alpha -= params->nTransparent;
			if( alpha > 1 )
			{
				*po = DOALPHA2( *po, cin, alpha );
			}
		}
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsT0( struct bdParams *params )
{
	StartLoop
		*po = INVERTPIXEL( *pi );
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsT1( struct bdParams *params )
{
	StartLoop
		CDATA cin;
		if( (cin = *pi) )
		{
			*po = INVERTPIXEL( cin );
		}
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsTA( struct bdParams *params )
{
	StartLoop
		CDATA cin;
		if( (cin = INVERTPIXEL( *pi )) )
		{
			*po = DOALPHA2( *po
				, cin
				, params->nTransparent );
		}
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsTImgA( struct bdParams *params )
{
	StartLoop
		uint32_t alpha;
		CDATA cin;
		if( (cin = INVERTPIXEL( *pi )) )
		{
			alpha = (cin & 0xFF000000) >> 24;
			alpha += params->nTransparent;
			*po = DOALPHA2( *po, cin, alpha );
		}
	EndLoop
}
//--	-------------------------------------------------------------------------

static void InvertPixelsTImgAI( struct bdParams *params )
{
	StartLoop
		int32_t alpha;
		CDATA cin;
		if( (cin = INVERTPIXEL( *pi )) )
		{
			alpha = (cin & 0xFF000000) >> 24;
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
		*po = SHADEPIXEL( pixel, params->c );
	EndLoop
}

//---------------------------------------------------------------------------
void CPROC CopyPixelsShadedT1( struct bdParams *params )
{
	StartLoop
		uint32_t pixel;
	if( (pixel = *pi) )
	{
		*po = SHADEPIXEL( pixel, params->c );
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
		pixout = SHADEPIXEL( pixel, params->c );
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
		pixout = SHADEPIXEL( pixel, params->c );
		alpha = (pixel & 0xFF000000) >> 24;
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
		alpha = (pixel & 0xFF000000) >> 24;
		alpha -= params->nTransparent;
		if( alpha > 1 )
		{
			pixout = SHADEPIXEL( pixel, params->c );
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
		alpha = (pixel & 0xFF000000) >> 24;
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
		alpha = (pixel & 0xFF000000) >> 24;
		alpha -= params->nTransparent;
		if( alpha > 1 )
		{
			pixout = MULTISHADEPIXEL( pixel, params->r, params->g, params->b );
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
	static uint32_t lock;
	va_list colors;
	va_start( colors, method );
	if( nTransparent > ALPHA_TRANSPARENT_MAX )
		return;

	if(  !pifSrc
	  || !pifSrc->image
	  || !pifDest
	  || !pifDest->image )
		return;
	//lprintf( "BlotImageSized %d,%d to %d,%d by %d,%d", xs, ys, xd, yd, ws, hs );

	{
//cpg26dec2006 c:\work\sack\src\imglib\blotdirect.c(348): Warning! W202: Symbol 'r' has been defined, but not referenced
//cpg26dec2006  	IMAGE_RECTANGLE r;
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
			//lprintf( "Images do not overlap. %d,%d %d,%d vs %d,%d %d,%d", r1.x,r1.y,r1.width,r1.height
			//		 , r2.x,r2.y,r2.width,r2.height);
			return;
		}

		//lprintf( "Correcting coordinates by %d,%d"
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
		//lprintf( "Resulting dest is %d,%d %d,%d", rd.x,rd.y,rd.width,rd.height );
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
			//lprintf( "Desired Output does not overlap..." );
			return;
		}
		//lprintf( "Resulting dest is %d,%d %d,%d", rs.x,rs.y,rs.width,rs.height );
		ws = rs.width<rd.width?rs.width:rd.width;
		hs = rs.height<rd.height?rs.height:rd.height;
		xs = rs.x;
		ys = rs.y;
		//lprintf( "Resulting rect is %d,%d to %d,%d dim: %d,%d", rs.x, rs.y, rd.x, rd.y, rs.width, rs.height );
		//lprintf( "Resulting rect is %d,%d to %d,%d dim: %d,%d", xs, ys, xd, yd, ws, hs );
	}
	//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	if( (int32_t)ws <= 0 ||
		 (int32_t)hs <= 0 )
		return;
	if( pifSrc->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		bd.pi = IMG_ADDRESS( pifSrc, xs, ys );
		bd.oi = 4*-(int)(ws+pifSrc->pwidth); // adding remaining width...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		bd.pi = IMG_ADDRESS( pifSrc, xs, ys );
		bd.oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	}
	if( pifDest->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		bd.po = IMG_ADDRESS( pifDest, xd, yd );
		bd.oo = 4*-(int)(ws+pifDest->pwidth);	  // w is how much we can copy...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		bd.po = IMG_ADDRESS( pifDest, xd, yd );
		bd.oo = 4*(pifDest->pwidth - ws);	  // w is how much we can copy...
	}
	//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	//oo = 4*(pifDest->pwidth - ws);	  // w is how much we can copy...
	//oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	//while( LockedExchange( &lock, 1 ) )
	//	Relinquish();
	{
		bd.nTransparent = nTransparent & 0xFF;
		switch( method )
		{
		case BLOT_COPY:
			if( !nTransparent )
				CopyPixelsT0( &bd );
			else if( nTransparent == 1 )
				CopyPixelsT1( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT )
				CopyPixelsTImgA( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				CopyPixelsTImgAI( &bd );
			else
				CopyPixelsTA( &bd );
			break;
		case BLOT_INVERTED:
			if( !nTransparent )
				InvertPixelsT0( &bd );
			else if( nTransparent == 1 )
				InvertPixelsT1( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT )
				InvertPixelsTImgA( &bd );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				InvertPixelsTImgAI( &bd );
			else
				InvertPixelsTA( &bd );
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
			} else if( nTransparent & ALPHA_TRANSPARENT_INVERT ) {
				bd.nTransparent = nTransparent & 0xFF;
				CopyPixelsShadedTImgAI( &bd );
			} else {
				bd.nTransparent = nTransparent;
				CopyPixelsShadedTA( &bd );
			}
			break;
		case BLOT_MULTISHADE:
			{
				bd.r = va_arg( colors, CDATA );
				bd.g = va_arg( colors, CDATA );
				bd.b = va_arg( colors, CDATA );
				//lprintf( "r g b %08x %08x %08x", r,g, b );
				if( !nTransparent )
					CopyPixelsMultiT0( &bd );
				else if( nTransparent == 1 )
					CopyPixelsMultiT1( &bd );
				else if( nTransparent & ALPHA_TRANSPARENT ) {
					bd.nTransparent = nTransparent & 0xFF;
					CopyPixelsMultiTImgA( &bd );
				} else if( nTransparent & ALPHA_TRANSPARENT_INVERT ) {
					bd.nTransparent = nTransparent & 0xFF;
					CopyPixelsMultiTImgAI( &bd );
				} else {
					bd.nTransparent = nTransparent;
					CopyPixelsMultiTA( &bd );
				}
			}
			break;
		}
	}
	//lock = 0;
	//lprintf( "Image done.." );
}
// copy all of pifSrc to the destination - placing the upper left
// corner of pifSrc on the point specified.
 void  BlotImageEx ( ImageFile *pifDest, ImageFile *pifSrc, int32_t xd, int32_t yd, uint32_t nTransparent, uint32_t method, ... )
{
	va_list colors;
	CDATA r;
	CDATA g;
	CDATA b;
	if( pifSrc )
	{
		va_start( colors, method );
		r = va_arg( colors, CDATA );
		g = va_arg( colors, CDATA );
		b = va_arg( colors, CDATA );
		BlotImageSizedEx( pifDest, pifSrc, xd, yd, 0, 0
						, pifSrc->real_width, pifSrc->real_height, nTransparent, method
										  , r,g,b
										);
	}
}
#ifdef __cplusplus
}; //namespace sack::image {
}; //namespace sack::image {
#endif

