/*
 *  Crafted by Jim Buckeyne
 *  (c)1999-2006++ Freedom Collective
 * 
 *  Handle putting out one image scaled onto another image.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */
#define FORCE_COLOR_MACROS
#define NO_TIMING_LOGGING
#ifndef NO_TIMING_LOGGING
#include <stdhdrs.h>
#include <logging.h>
#endif

#define IMAGE_LIBRARY_SOURCE
#include <stdhdrs.h>
#include <sharemem.h>
#include <imglib/imagestruct.h>
#include <colordef.h>
#include "image.h"
#define NEED_ALPHA2
#include "blotproto.h"

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

#define TOFIXED(n)	((n)<<FIXED_SHIFT)
#define FROMFIXED(n)	((n)>>FIXED_SHIFT)
#define TOPFROMFIXED(n) (((n)+(FIXED-1))>>FIXED_SHIFT)
#define FIXEDPART(n)	 ((n)&(FIXED-1))
#define FIXED		  1024
#define FIXED_SHIFT  10

//---------------------------------------------------------------------------

#define ScaleLoopStart int errx, erry; \
	_32 x, y;							\
	PCDATA _pi = pi;				  \
	PCDATA __pi = pi;				  \
	erry = i_erry/2;					 \
	y = 0;								\
	while( y < hd )					\
	{									 \
		/* move first line.... */ \
		errx = i_errx/2;				\
		x = 0;						  \
		pi = _pi;					  \
		while( x < wd )			  \
		{								 \
			{

#define ScaleLoopEnd  }				 \
			po++;							 \
			x++;							  \
			errx += (signed)dws; /* add source width */\
			while( errx >= 0 )					\
			{										  \
				errx -= (signed)dwd; /* fix backwards the width we're copying*/\
				pi++;								 \
			}										  \
		}											  \
		po = (CDATA*)(((char*)po) + oo);	 \
		y++;										  \
		erry += (signed)dhs;								\
		while( erry >= 0 )						\
		{											  \
			erry -= (signed)dhd;							 \
			_pi = (CDATA*)(((char*)_pi) + srcpwidth); /* go to next line start*/\
			{ \
				int psvDel = ( __pi - _pi ); \
				psvDel /= (-srcpwidth/sizeof(CDATA)); \
				if( psvDel > dhs ) return; \
			} \
		}											  \
	}

//---------------------------------------------------------------------------

#define PIXCOPY	

#define TCOPY  if( *pi )  *(po) = *(pi);

#define ACOPY  CDATA cin;  if( cin = *pi )		  \
		{														 \
			*po = DOALPHA2( *po, cin, nTransparent ); \
		}

#define IMGACOPY	  CDATA cin;						  \
		int alpha;											  \
		if( cin = *pi )										\
		{														  \
			alpha = ( cin & 0xFF000000 ) >> 24;		 \
			alpha += nTransparent;						  \
			*po = DOALPHA2( *po, cin, alpha );			\
		}

#define IMGINVACOPY  CDATA cin;						  \
		_32 alpha;											  \
		if( (cin = *pi) )										\
		{														  \
			alpha = ( cin & 0xFF000000 ) >> 24;		 \
			alpha -= nTransparent;						  \
			if( alpha > 1 )									\
				*po = DOALPHA2( *po, cin, alpha );		\
		}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
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

static void BlotScaledTImgA(SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledTImgAI( SCALED_BLOT_WORK_PARAMS
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void BlotInvertScaledT0( SCALED_BLOT_WORK_PARAMS
											)
{
	ScaleLoopStart
		if( AlphaVal(*pi) == 0 )
			(*po) = 0;
		else
			(*po) = INVERTPIXEL(*pi);
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledT1( SCALED_BLOT_WORK_PARAMS
								)
{
	ScaleLoopStart
		if( *pi )  *(po) = INVERTPIXEL(*(pi));
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTA( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent )
{
	ScaleLoopStart
		CDATA cin;  
		if( (cin = INVERTPIXEL(*pi)) )
		{
			*po = DOALPHA2( *po, cin, nTransparent ); 
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTImgA(SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent )
{
	ScaleLoopStart
		CDATA cin;											 
		_32 alpha;											 
		if( (cin = INVERTPIXEL(*pi)) )
		{														 
			alpha = ( cin & 0xFF000000 ) >> 24;		
			alpha += nTransparent;
			*po = DOALPHA2( *po, cin, alpha );		  
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTImgAI( SCALED_BLOT_WORK_PARAMS
							 , _32 nTransparent )
{

	ScaleLoopStart
		CDATA cin;									  
		S_32 alpha;
		if( (cin = INVERTPIXEL(*pi)) )
		{												  
			alpha = ( cin & 0xFF000000 ) >> 24; 
			alpha -= nTransparent;				  
			if( alpha > 1 )							
				*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
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
static void BlotScaledShadedTImgA( SCALED_BLOT_WORK_PARAMS
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
static void BlotScaledShadedTImgAI( SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledMultiT0( SCALED_BLOT_WORK_PARAMS
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

void BlotScaledMultiT1(  SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledMultiTA(  SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledMultiTImgA( SCALED_BLOT_WORK_PARAMS
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

static void BlotScaledMultiTImgAI( SCALED_BLOT_WORK_PARAMS
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
	S_32  oo;
	S_32 srcwidth;
	int errx, erry;
	_32 dhd, dwd, dhs, dws;
	va_list colors;
	va_start( colors, method );
	//lprintf( WIDE("Blot enter %p %p (%d,%d)"), pifDest, pifSrc, wd, hd );
	if( nTransparent > ALPHA_TRANSPARENT_MAX )
	{
		return;
	}
	if( !pifSrc ||  !pifDest
	  || !pifSrc->image || !pifDest->image
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
	//	lprintf(" begin scaled output..." );
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
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
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
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( xd + (signed)wd ) > ( pifDest->x + pifDest->width) )
	{
		//int newwd = TOFIXED(pifDest->width);
		//ws -= ((S_64)( (int)wd - newwd)* (S_64)ws )/(int)wd;
		wd = ( pifDest->x + pifDest->width ) - xd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( yd + (signed)hd ) > (pifDest->y + pifDest->height) )
	{
		//int newhd = TOFIXED(pifDest->height);
		//hs -= ((S_64)( hd - newhd)* hs )/hd;
		hd = (pifDest->y + pifDest->height) - yd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( (S_32)wd <= 0 ||
		 (S_32)hd <= 0 ||
		 (S_32)ws <= 0 ||
		 (S_32)hs <= 0 )
	{
		return;
	}
	
	//Log9( WIDE("Image locations: %d(%d %d) %d(%d) %d(%d) %d(%d)")
	//			 , xs, FROMFIXED(xs), FIXEDPART(xs)
	//			 , ys, FROMFIXED(ys)
	//			 , xd, FROMFIXED(xd)
	//			 , yd, FROMFIXED(yd) );

	if( pifSrc->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		srcwidth = 4*-(int)(pifSrc->pwidth); // adding remaining width...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		srcwidth = 4*(pifSrc->pwidth); // adding remaining width...
	}
	if( pifDest->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		po = IMG_ADDRESS( pifDest, xd, yd );
		oo = 4*(-((signed)wd) - (pifDest->pwidth) );	  // w is how much we can copy...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		po = IMG_ADDRESS( pifDest, xd, yd );
		oo = 4*(pifDest->pwidth - (wd));;	  // w is how much we can copy...
	}

	//lprintf( WIDE("Do blot work... %p %p  %d  %d  %d,%d %d,%d")
	//		 , pi, po, srcwidth, oo
	//		 , ws, hs
	//		 , wd, hd );
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
	case BLOT_INVERTED:
		if( !nTransparent )
			BlotInvertScaledT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );
		else if( nTransparent == 1 )
			BlotInvertScaledT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );		 
		else if( nTransparent & ALPHA_TRANSPARENT )
			BlotInvertScaledTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			BlotInvertScaledTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );		  
		else
			BlotInvertScaledTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent );		  
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
	//lprintf( WIDE("Blot done") );
}



#ifdef __cplusplus 
}//namespace sack {
}//namespace image {
#endif

