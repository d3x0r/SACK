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
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
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
	uint32_t x, y;							\
	PCDATA _pi = pi;				  \
	PCDATA __pi = pi;				  \
	erry = params->erry;					 \
	y = 0;								\
	while( y < params->hd )					\
	{									 \
		/* move first line.... */ \
		errx = params->errx;				\
		x = 0;						  \
		pi = _pi;					  \
		while( x < params->wd )			  \
		{								 \
			{

#define ScaleLoopEnd  }				 \
			po++;							 \
			x++;							  \
			errx += (signed)params->dws; /* add source width */\
			while( errx >= 0 )					\
			{										  \
				errx -= (signed)params->dwd; /* fix backwards the width we're copying*/\
				pi++;								 \
			}										  \
		}											  \
		po = (CDATA*)(((char*)po) + params->oo);	 \
		y++;										  \
		erry += (signed)params->dhs;								\
		while( erry >= 0 )						\
		{											  \
			erry -= (signed)params->dhd;							 \
			_pi = (CDATA*)(((char*)_pi) + params->srcpwidth); /* go to next line start*/\
			{ \
				int psvDel = (int)( __pi - _pi ); \
				psvDel /= (-params->srcpwidth/sizeof(CDATA)); \
				if( psvDel > params->dhs ) return; \
			} \
		}											  \
	}

#define SCALED_DIRECT_WORK_PARAMS  

struct bsParams {
	PCDATA po; PCDATA  pi;
	int errx; int erry;
		uint32_t wd; uint32_t hd;
		int32_t dwd; int32_t dhd;
		int32_t dws; int32_t dhs;
		int32_t oo; int32_t srcpwidth;
		uint32_t nTransparent;
		CDATA c;
		CDATA r, g, b;
};

//---------------------------------------------------------------------------

#define PIXCOPY	

#define TCOPY  if( *pi )  *(po) = *(pi);

#define ACOPY  CDATA cin;  if( cin = *pi )		  \
		{														 \
			*po = DOALPHA2( *po, cin, params->nTransparent ); \
		}

#define IMGACOPY	  CDATA cin;						  \
		int alpha;											  \
		if( cin = *pi )										\
		{														  \
			alpha = ( cin & 0xFF000000 ) >> 24;		 \
			alpha += params->nTransparent;						  \
			*po = DOALPHA2( *po, cin, alpha );			\
		}

#define IMGINVACOPY  CDATA cin;						  \
		uint32_t alpha;											  \
		if( (cin = *pi) )										\
		{														  \
			alpha = ( cin & 0xFF000000 ) >> 24;		 \
			alpha -= params->nTransparent;						  \
			if( alpha > 1 )									\
				*po = DOALPHA2( *po, cin, alpha );		\
		}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void BlotScaledT0( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( AlphaVal(*pi) == 0 )
			(*po) = 0;
		else
			(*po) = (*pi);
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledT1( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( *pi )  *(po) = *(pi);
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledTA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;  
		if( (cin = *pi) )
		{
			*po = DOALPHA2( *po, cin, params->nTransparent );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledTImgA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;											 
		uint32_t alpha;											 
		if( (cin = *pi) )
		{														 
			alpha = ( cin & 0xFF000000 ) >> 24;		
			alpha += params->nTransparent;
			*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledTImgAI( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;

	ScaleLoopStart
		CDATA cin;									  
		int32_t alpha;
		if( (cin = *pi) )
		{												  
			alpha = ( cin & 0xFF000000 ) >> 24; 
			alpha -= params->nTransparent;
			if( alpha > 1 )							
				*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
static void BlotInvertScaledT0( struct bsParams *params)
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( AlphaVal(*pi) == 0 )
			(*po) = 0;
		else
			(*po) = INVERTPIXEL(*pi);
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledT1( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( *pi )  *(po) = INVERTPIXEL(*(pi));
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;  
		if( (cin = INVERTPIXEL(*pi)) )
		{
			*po = DOALPHA2( *po, cin, params->nTransparent );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTImgA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;											 
		uint32_t alpha;											 
		if( (cin = INVERTPIXEL(*pi)) )
		{														 
			alpha = ( cin & 0xFF000000 ) >> 24;		
			alpha += params->nTransparent;
			*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotInvertScaledTImgAI( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;									  
		int32_t alpha;
		if( (cin = INVERTPIXEL(*pi)) )
		{												  
			alpha = ( cin & 0xFF000000 ) >> 24; 
			alpha -= params->nTransparent;
			if( alpha > 1 )							
				*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static void BlotScaledShadedT0( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart

		*(po) = SHADEPIXEL( *pi, params->c );

	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledShadedT1( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( *pi )
			*(po) = SHADEPIXEL( *pi, params->c );
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledShadedTA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		if( (cin = *pi) )
		{
			cin = SHADEPIXEL( cin, params->c );
			*po = DOALPHA2( *po, cin, params->nTransparent );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------
static void BlotScaledShadedTImgA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		uint32_t alpha;
		if( (cin = *pi) )
		{
			alpha = ( cin & 0xFF000000 ) >> 24;
			alpha += params->nTransparent;
			cin = SHADEPIXEL( cin, params->c );
			*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------
static void BlotScaledShadedTImgAI( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		uint32_t alpha;
		if( (cin = *pi) )
		{
			alpha = ( cin & 0xFF000000 ) >> 24;
			alpha -= params->nTransparent;
			if( alpha > 1 )
			{
				cin = SHADEPIXEL( cin, params->c );
				*po = DOALPHA2( *po, cin, alpha );
			}
		}
	ScaleLoopEnd
}						  

//---------------------------------------------------------------------------

static void BlotScaledMultiT0( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		uint32_t rout, gout, bout;
		*(po) = MULTISHADEPIXEL( *pi, params->r, params->g, params->b );
	ScaleLoopEnd

}
				
//---------------------------------------------------------------------------

void BlotScaledMultiT1( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		if( *pi )
		{
			uint32_t rout, gout, bout;
			*(po) = MULTISHADEPIXEL( *pi, params->r, params->g, params->b );
		}
	ScaleLoopEnd

}
//---------------------------------------------------------------------------

static void BlotScaledMultiTA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		if( (cin = *pi) )
		{
			uint32_t rout, gout, bout;
			cin = MULTISHADEPIXEL( cin, params->r, params->g, params->b );
			*po = DOALPHA2( *po, cin, params->nTransparent );
		}
	ScaleLoopEnd

}

//---------------------------------------------------------------------------

static void BlotScaledMultiTImgA( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		uint32_t alpha;
		if( (cin = *pi) )
		{
			uint32_t rout, gout, bout;
			cin = MULTISHADEPIXEL( cin, params->r, params->g, params->b );
			alpha = ( cin & 0xFF000000 ) >> 24;
			alpha += params->nTransparent;
			*po = DOALPHA2( *po, cin, alpha );
		}
	ScaleLoopEnd

}

//---------------------------------------------------------------------------

static void BlotScaledMultiTImgAI( struct bsParams *params )
{
	CDATA *pi, *po;
	pi = params->pi; po = params->po;
	ScaleLoopStart
		CDATA cin;
		uint32_t alpha;
		if( (cin = *pi) )
		{
			uint32_t rout, gout, bout;
			cin = MULTISHADEPIXEL( cin, params->r, params->g, params->b );
			alpha = ( cin & 0xFF000000 ) >> 24;
			alpha -= params->nTransparent;
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
	struct bsParams params;
	//CDATA *po, *pi;
	static uint32_t lock;
	//int32_t  oo;
	//int32_t srcwidth;
	//int errx, erry;
	//uint32_t dhd, dwd, dhs, dws;
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
	params.dhd = hd;
	params.dhs = hs;
	params.dwd = wd;
	params.dws = ws;

	// ok - how to figure out how to do this
	// need to update the position and width to be within the 
	// the bounds of pifDest....
	//	lprintf(" begin scaled output..." );
	params.errx = -(signed)params.dwd;
	params.erry = -(signed)params.dhd;

	if( xd < pifDest->x )
	{
		while( xd < pifDest->x )
		{
			params.errx += (signed)params.dws;
			while( params.errx >= 0 )
			{
				params.errx -= (signed)params.dwd;
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
			params.erry += (signed)params.dhs;
			while( params.erry >= 0 )
			{
				params.erry -= (signed)params.dhd;
				hs--;
				ys++;
			}
			hd--;
			yd++;
		}
	}

	params.errx = (params.dws>params.dwd) ? params.errx / 2 : params.errx;				\

	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( xd + (signed)wd ) > ( pifDest->x + pifDest->width) )
	{
		//int newwd = TOFIXED(pifDest->width);
		//ws -= ((int64_t)( (int)wd - newwd)* (int64_t)ws )/(int)wd;
		wd = ( pifDest->x + pifDest->width ) - xd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( yd + (signed)hd ) > (pifDest->y + pifDest->height) )
	{
		//int newhd = TOFIXED(pifDest->height);
		//hs -= ((int64_t)( hd - newhd)* hs )/hd;
		hd = (pifDest->y + pifDest->height) - yd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//		 xs, ys, ws, hs, xd, yd, wd, hd );
	if( (int32_t)wd <= 0 ||
		 (int32_t)hd <= 0 ||
		 (int32_t)ws <= 0 ||
		 (int32_t)hs <= 0 )
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
		params.pi = IMG_ADDRESS( pifSrc, xs, ys );
		params.srcpwidth = 4*-(int)(pifSrc->pwidth); // adding remaining width...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		params.pi = IMG_ADDRESS( pifSrc, xs, ys );
		params.srcpwidth = 4*(pifSrc->pwidth); // adding remaining width...
	}
	if( pifDest->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		//pi = IMG_ADDRESS( pifSrc, xs, ys );
		//po = IMG_ADDRESS( pifDest, xd, yd );
		params.po = IMG_ADDRESS( pifDest, xd, yd );
		params.oo = 4*(-((signed)wd) - (pifDest->pwidth) );	  // w is how much we can copy...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		params.po = IMG_ADDRESS( pifDest, xd, yd );
		params.oo = 4*(pifDest->pwidth - (wd));;	  // w is how much we can copy...
	}

	//lprintf( WIDE("Do blot work... %p %p  %d  %d  %d,%d %d,%d")
	//		 , pi, po, srcwidth, oo
	//		 , ws, hs
	//		 , wd, hd );
	params.wd = wd;
	params.hd = hd;
	params.nTransparent = nTransparent & 0xFF;
	switch( method )
	{
	case BLOT_COPY:
		if( !nTransparent )
			BlotScaledT0( &params );		 
		else if( nTransparent == 1 )
			BlotScaledT1( &params );
		else if( nTransparent & ALPHA_TRANSPARENT )
			BlotScaledTImgA( &params );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			BlotScaledTImgAI( &params );
		else
			BlotScaledTA( &params );
		break;
	case BLOT_INVERTED:
		if( !nTransparent )
			BlotInvertScaledT0( &params );
		else if( nTransparent == 1 )
			BlotInvertScaledT1( &params );
		else if( nTransparent & ALPHA_TRANSPARENT )
			BlotInvertScaledTImgA( &params );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			BlotInvertScaledTImgAI( &params );
		else
			BlotInvertScaledTA( &params );
		break;
	case BLOT_SHADED:
		params.c = va_arg( colors, CDATA );
		if( !nTransparent )
			BlotScaledShadedT0( &params );
		else if( nTransparent == 1 )
			BlotScaledShadedT1( &params );
		else if( nTransparent & ALPHA_TRANSPARENT )
			BlotScaledShadedTImgA( &params );
		else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
			BlotScaledShadedTImgAI( &params );
		else
			BlotScaledShadedTA( &params );
		break;
	case BLOT_MULTISHADE:
		{
			//CDATA r,g,b;
			params.r = va_arg( colors, CDATA );
			params.g = va_arg( colors, CDATA );
			params.b = va_arg( colors, CDATA );
			if( !nTransparent )
				BlotScaledMultiT0( &params );
			else if( nTransparent == 1 )
				BlotScaledMultiT1( &params );
			else if( nTransparent & ALPHA_TRANSPARENT )
				BlotScaledMultiTImgA( &params );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				BlotScaledMultiTImgAI( &params );
			else
				BlotScaledMultiTA( &params );
		}
		break;
	}
	//lprintf( WIDE("Blot done") );
}



#ifdef __cplusplus 
}//namespace sack {
}//namespace image {
#endif

