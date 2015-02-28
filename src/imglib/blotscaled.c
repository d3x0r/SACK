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
	erry = i_erry;					 \
	y = 0;								\
	while( y < hd )					\
	{									 \
		/* move first line.... */ \
		errx = i_errx;				\
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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void CPROC cBlotInvertScaledT0( SCALED_BLOT_WORK_PARAMS
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

void CPROC cBlotInvertScaledT1( SCALED_BLOT_WORK_PARAMS
								)
{
	ScaleLoopStart
		if( *pi )  *(po) = INVERTPIXEL(*(pi));
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

void CPROC cBlotInvertScaledTA( SCALED_BLOT_WORK_PARAMS
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

void CPROC cBlotInvertScaledTImgA(SCALED_BLOT_WORK_PARAMS
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

void CPROC cBlotInvertScaledTImgAI( SCALED_BLOT_WORK_PARAMS
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
	S_32  oo;
	S_32 srcwidth;
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
	//	xs, ys, ws, hs, xd, yd, wd, hd );
	/*
	if( xs < (pifSrc->real_x) )
	{
		int tmpdiff;
		int tmpdel = ( (S_64)wd * (S_64)(tmpdiff = xs-(pifSrc->x)) ) / (int)ws;
		wd += tmpdel;
		xd -= tmpdel;
		ws += tmpdiff;
		xs = (pifSrc->x);
	}
	Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
			 xs, ys, ws, hs, xd, yd, wd, hd );
	if( ys < (pifSrc->real_y) )
	{
		int tmpdiff;
		int tmpdel = ( (S_64)hd * (S_64)(tmpdiff = ys-(pifSrc->y)) ) / (int)hs;
		hd += tmpdel;
		yd -= tmpdel;
		hs += tmpdiff;
		ys = (pifSrc->y);
		}
	*/
	 /*
	if( ws > (pifSrc->real_width) )
	{
		int newws;
		newws = (pifSrc->width);
		wd -= (( wd - newws)* wd )/ws;
		ws = newws;
	}
	Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
			 xs, ys, ws, hs, xd, yd, wd, hd );
	if( hs > TOFIXED(pifSrc->real_height) )
	{
		int newhs;
		newhs = TOFIXED(pifSrc->height);
		hd -= (( hs - newhs)* hd )/hs;
		hs = newhs;
		} 
		*/
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

	//while( LockedExchange( &lock, 1 ) )
	//	Relinquish();

	//Log8( WIDE("Do blot work...%d(%d),%d(%d) %d(%d) %d(%d)")
	//	 , ws, FROMFIXED(ws), hs, FROMFIXED(hs) 
	//	 , wd, FROMFIXED(wd), hd, FROMFIXED(hd) );
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
	//lock = 0;
//	Log( WIDE("Blot done") );
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
