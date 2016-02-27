// changes colordef
#define IMAGE_LIBRARY_SOURCE_MAIN
#define IMAGE_LIBRARY_SOURCE
#define IMAGE_MAIN
#ifdef _MSC_VER
#ifndef UNDER_CE
#include <emmintrin.h>
#endif
// intrinsics
#endif

#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
//#define NO_LOGGING
#include <logging.h>
#include <sharemem.h>
#include <filesys.h>

#include <imglib/imagestruct.h>
#include <image.h>

// alpha macro is here
#include "blotproto.h"
#include "image_common.h"

ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE
//---------------------------------------------------------------------------
// a(alpha) parameter value 0 : in is clear, over opaque
//                         255: in is solid, over is clear

//---------------------------------------------------------------------------


void CPROC MarkImageUpdated( Image child_image )
{
	Image image;
	for( image = child_image; image && image->pParent; image = image->pParent );

	{
		if( image_common_local.tint_cache )
		{
			CPOINTER node = FindInBinaryTree( image_common_local.tint_cache, (PTRSZVAL)image );
			struct shade_cache_image *ci = (struct shade_cache_image *)node;
			struct shade_cache_element *ce;
			if( node )
			{
				INDEX idx;
				LIST_FORALL( ci->elements, idx, struct shade_cache_element *, ce )
				{
					ce->flags.parent_was_dirty = 1;
				}
			}
		}
		if( image_common_local.shade_cache )
		{
			CPOINTER node = FindInBinaryTree( image_common_local.shade_cache, (PTRSZVAL)image );
			struct shade_cache_image *ci = (struct shade_cache_image *)node;
			struct shade_cache_element *ce;
			if( node )
			{
				INDEX idx;
				LIST_FORALL( ci->elements, idx, struct shade_cache_element *, ce )
				{
					ce->flags.parent_was_dirty = 1;
				}
			}
		}
		image->flags |= IF_FLAG_UPDATED;
	}
	;;

	{
		// release foriegn resources
	}
}

#if 0
IMAGE_NAMESPACE_END
ASM_IMAGE_NAMESPACE
void  CPROC asmBlatColor( PCDATA po, int oo, int w, int h
                  , CDATA color );

void  CPROC asmBlatColorAlpha( PCDATA po, int oo, int w, int h
                  , CDATA color );
void  CPROC mmxBlatColorAlpha( PCDATA po, int oo, int w, int h
                  , CDATA color );
extern void  (CPROC*BlatPixels)( PCDATA po, int oo, int w, int h
                  , CDATA color );
extern void  (CPROC*BlatPixelsAlpha)( PCDATA po, int oo, int w, int h
                  , CDATA color );

ASM_IMAGE_NAMESPACE_END
IMAGE_NAMESPACE
#endif

//---------------------------------------------------------------------------
// This routine fills a rectangle with a solid color
// it is used for clear image, clear image to
// and for arbitrary rectangles - the direction
// of images does not matter.
 void  BlatColor ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PCDATA po;
	int  oo;
	if( !pifDest || !pifDest->image )
	{
		lprintf( WIDE( "No dest, or no dest image." ) );
		return;
	}

	if( !w )
		w = pifDest->width;
	if( !h )
		h = pifDest->height;
	{
		IMAGE_RECTANGLE r, r1, r2;
		// build rectangle of what we want to show
		r1.x      = x;
		r1.width  = w;
		r1.y      = y;
		r1.height = h;
					 // build rectangle which is presently visible on image
		r2.x      = pifDest->x;
		r2.width  = pifDest->width;
		r2.y      = pifDest->y;
		r2.height = pifDest->height;
		if( !IntersectRectangle( &r, &r1, &r2 ) )
		{
			lprintf( WIDE("blat color is out of bounds (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(") (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(")")
				, x, y, w, h
				, r2.x, r2.y, r2.width, r2.height );
			return;
		}
#ifdef DEBUG_BLATCOLOR
		// trying to figure out why there are stray lines for DISPLAY surfaces
		// apparently it's a logic in space node min/max to region conversion
		lprintf( WIDE("Rects %d,%d %d,%d/%d,%d %d,%d/ %d,%d %d,%d ofs %d,%d %d,%d")
				 , r1.x, r1.y

				 ,r1.width, r1.height
				 , r2.x, r2.y
				 ,r2.width, r2.height
				 , r.x, r.y, r.width, r.height
				 , pifDest->eff_x, pifDest->eff_y
				 , pifDest->eff_maxx, pifDest->eff_maxy

				 );
#endif
		x = r.x;
		w = r.width;
		y = r.y;
		h = r.height;
	}
	//lprintf( WIDE("Blotting %d,%d - %d,%d"), x, y, w, h );
	// start at origin on destination....
	if( pifDest->flags & IF_FLAG_INVERTED )
		oo = 4*( (-(S_32)w) - pifDest->pwidth);     // w is how much we can copy...
   else
		oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...
	po = IMG_ADDRESS(pifDest,x,y);
	SetColor( po, oo, w, h, color );
}

 void  BlatColorAlpha ( ImageFile *pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PCDATA po;
	int  oo;

	if( !pifDest || !pifDest->image )
	{
		lprintf( WIDE( "No dest, or no dest image." ) );

		return;
	}
	if( !w )
		w = pifDest->width;
	if( !h )
		h = pifDest->height;
	{
		IMAGE_RECTANGLE r, r1, r2;
		r1.x = x,
		r1.width = w;
		r1.y = y;
		r1.height = h;
		r2.x = pifDest->x;
		r2.width = pifDest->width;
		r2.y = pifDest->y;
		r2.height = pifDest->height;
		if( !IntersectRectangle( &r, &r1, &r2 ) )
		{
			lprintf( WIDE("blat color alpha is out of bounds (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(") (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(")")
				, x, y, w, h
				, r2.x, r2.y, r2.width, r2.height );
			return;
		}
#ifdef DEBUG_BLATCOLOR
		lprintf( WIDE("Rects %d,%d %d,%d/%d,%d %d,%d/ %d,%d %d,%d")
				 , r1.x, r1.y
				 ,r1.width, r1.height
				 , r2.x, r2.y
				 ,r2.width, r2.height
				 , r.x, r.y, r.width, r.height );
#endif
		// it's a same rectangle, it's safe to delete the comparison to <= 0 that was here
		x = r.x;
		w = r.width;
		y = r.y;
		h = r.height;
	}
   // start at origin on destination....
	if( pifDest->flags & IF_FLAG_INVERTED )
		y += h-1;
	po = IMG_ADDRESS(pifDest,x,y);
	oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...

	SetColorAlpha( po, oo, w, h, color );
}

void SetImageTransformRelation( Image pImage, enum image_translation_relation relation, PRCOORD aux )
{

}
//---------------------------------------------------------------------------

IMAGE_NAMESPACE_END
ASM_IMAGE_NAMESPACE

void CPROC cplot( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c );
CDATA CPROC cgetpixel( ImageFile *pi, S_32 x, S_32 y );

#ifdef HAS_ASSEMBLY
void CPROC asmplot( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC asmplotalphaMMX( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
CDATA CPROC asmgetpixel( ImageFile *pi, S_32 x, S_32 y );
#endif

//---------------------------------------------------------------------------

void CPROC plotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
   *IMG_ADDRESS(pi,x,y) = c;
}

void CPROC plot( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
   if( !pi || !pi->image ) return;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height )) )
   {
      *IMG_ADDRESS(pi,x,y)= c;
   }
}

//---------------------------------------------------------------------------

CDATA CPROC getpixel( ImageFile *pi, S_32 x, S_32 y )
{
   if( !pi || !pi->image ) return 0;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height )) )
   {
      return *IMG_ADDRESS(pi,x,y);
   }
   return 0;
}

//---------------------------------------------------------------------------

void CPROC plotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
   CDATA *po;
   if( !pi || !pi->image ) return;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height) ) )
   {
      po = IMG_ADDRESS(pi,x,y);
      *po = DOALPHA( *po, c, AlphaVal(c) );
   }
}

//---------------------------------------------------------------------------
#if 0

void CPROC do_linec( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineasm( ImageFile *pImage, S_32 x, S_32 y
               , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineAlphac( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineAlphaasm( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
void CPROC do_lineAlphaMMX( ImageFile *pImage, S_32 x, S_32 y
                    , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineExVc( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( ImageFile*pif, S_32 x, S_32 y, int d ) );
#ifdef HAS_ASSEMBLY
void CPROC do_lineExVasm( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( ImageFile*pif, S_32 x, S_32 y, int d ) );
#endif

void CPROC do_hlinec( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineasm( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlinec( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineasm( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

void CPROC do_hlineAlphac( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineAlphaasm( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
void CPROC do_hlineAlphaMMX( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlineAlphac( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineAlphaasm( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
void CPROC do_vlineAlphaMMX( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

#endif

ASM_IMAGE_NAMESPACE_END
IMAGE_NAMESPACE


IMAGE_NAMESPACE_END


