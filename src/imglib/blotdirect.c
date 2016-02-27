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

#define StartLoop oo /= 4;	 \
	oi /= 4;						 \
	{								  \
		_32 row= 0;				 \
		while( row < hs )		 \
		{							  \
			_32 col=0;			  \
			while( col < ws )	 \
			{						  \
				{

#define EndLoop	}			  \
/*lprintf( "in %08x out %08x", ((CDATA*)pi)[0], ((CDATA*)po)[1] );*/ \
				po++;				 \
				pi++;				 \
				col++;				\
			}						  \
			pi += oi;				\
			po += oo;				\
			row++;					\
		}							  \
	}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void CopyPixelsT0( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
									)
{
	StartLoop
				*po = *pi;
	EndLoop
}

//---------------------------------------------------------------------------

static void CopyPixelsT1( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
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

static void CopyPixelsTA( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
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

static void CopyPixelsTImgA( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
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

static void CopyPixelsTImgAI( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void InvertPixelsT0( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
									)
{
	StartLoop
				*po = INVERTPIXEL(*pi);
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsT1( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
									)
{
	StartLoop
				CDATA cin;
				if( (cin = *pi) )
				{
					*po = INVERTPIXEL(cin);
				}
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsTA( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent )
{
	StartLoop
				CDATA cin;
				if( (cin = INVERTPIXEL(*pi)) )
				{
					*po = DOALPHA2( *po
									  , cin
									  , nTransparent );
				}
	EndLoop
}

//---------------------------------------------------------------------------

static void InvertPixelsTImgA( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent )
{
	StartLoop
				_32 alpha;
				CDATA cin;
				if( (cin = INVERTPIXEL(*pi)) )
				{
					alpha = ( cin & 0xFF000000 ) >> 24;
					alpha += nTransparent;
					*po = DOALPHA2( *po, cin, alpha );
				}
	EndLoop
}
//---------------------------------------------------------------------------

static void InvertPixelsTImgAI( PCDATA po, PCDATA  pi
								  , S_32 oo, S_32 oi
								  , _32 ws, _32 hs
								  , _32 nTransparent )
{
	StartLoop
				S_32 alpha;

				CDATA cin;
				if( (cin = INVERTPIXEL(*pi)) )
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void CopyPixelsShadedT0( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsShadedT1( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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

static void CopyPixelsShadedTA( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsShadedTImgA( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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

static void CopyPixelsMultiT0( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsMultiT1( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsMultiTA( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsMultiTImgA( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
static void CopyPixelsMultiTImgAI( PCDATA po, PCDATA  pi
									 , S_32 oo, S_32 oi
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
	int  hd, wd;
	S_32 oo, oi; // treated as an adder... it is unsigned by math, but still results correct offset?
	static _32 lock;
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

	wd = pifDest->width + pifDest->x;
	hd = pifDest->height + pifDest->y;
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
	if( (S_32)ws <= 0 ||
		 (S_32)hs <= 0 ||
		 (S_32)wd <= 0 ||
		 (S_32)hd <= 0 )
		return;
	if( pifSrc->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		oi = 4*-(int)(ws+pifSrc->pwidth); // adding remaining width...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	}
	if( pifDest->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		po = IMG_ADDRESS( pifDest, xd, yd );
		oo = 4*-(int)(ws+pifDest->pwidth);	  // w is how much we can copy...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		po = IMG_ADDRESS( pifDest, xd, yd );
		oo = 4*(pifDest->pwidth - ws);	  // w is how much we can copy...
	}
	//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	//oo = 4*(pifDest->pwidth - ws);	  // w is how much we can copy...
	//oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	//while( LockedExchange( &lock, 1 ) )
	//	Relinquish();
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
		case BLOT_INVERTED:
			if( !nTransparent )
				InvertPixelsT0( po, pi, oo, oi, ws, hs );
			else if( nTransparent == 1 )
				InvertPixelsT1( po, pi, oo, oi, ws, hs );
			else if( nTransparent & ALPHA_TRANSPARENT )
				InvertPixelsTImgA( po, pi, oo, oi, ws, hs, nTransparent & 0xFF);
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				InvertPixelsTImgAI( po, pi, oo, oi, ws, hs, nTransparent & 0xFF );
			else
				InvertPixelsTA( po, pi, oo, oi, ws, hs, nTransparent );
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
				//lprintf( "r g b %08x %08x %08x", r,g, b );
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
	}
	//lock = 0;
	//lprintf( "Image done.." );
}
// copy all of pifSrc to the destination - placing the upper left
// corner of pifSrc on the point specified.
 void  BlotImageEx ( ImageFile *pifDest, ImageFile *pifSrc, S_32 xd, S_32 yd, _32 nTransparent, _32 method, ... )
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

