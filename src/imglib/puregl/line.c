/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 *
 * Simple Line operations on Images.  Single pixel, no anti aliasing.
 *  There is a line routine which steps through each point and calls
 *  a user defined function, which may be used to extend straight lines
 *  with a smarter antialiasing renderer. (isntead of plot/putpixel)
 *
 *
 *
 *  consult doc/image.html
 *
 */


#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define LIBRARY_DEF
#include <stdhdrs.h>

#ifdef __ANDROID__
#include <gles/gl.h>
#else
//#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

#include <imglib/imagestruct.h>
#include <image.h>
#include "local.h"

#include "blotproto.h"

/* void do_line(BITMAP *bmp, int x1, y1, x2, y2, CDATA d, void (*proc)())
 *  Calculates all the points along a line between x1, y1 and x2, y2,
 *  calling the supplied function for each one. This will be passed a
 *  copy of the bmp parameter, the x and y position, and a copy of the
 *  d parameter (so do_line() can be used with putpixel()).
 */
#ifdef __cplusplus
namespace sack {
	namespace image {
extern "C" {
#endif

//unsigned long DOALPHA( unsigned long over, unsigned long in, unsigned long a );

#define FIX_SHIFT 18
#define ROUND_ERROR ( ( 1<< ( FIX_SHIFT - 1 ) ) - 1 )


void CPROC do_line( Image pImage, int32_t x1, int32_t y1
						 , int32_t x2, int32_t y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		VECTOR slope;
		RCOORD tmp;
		int v = 0;

		TranslateCoord( pImage, (int32_t*)&x1, (int32_t*)&y1 );
		TranslateCoord( pImage, (int32_t*)&x2, (int32_t*)&y2 );
		v1[v][0] = x1;
		v1[v][1] = y1;
		v1[v][2] = 0.0;
		v2[v][0] = x2;
		v2[v][1] = y2;
		v2[v][2] = 0.0;

		sub( slope, v2[v], v1[v] );
		normalize( slope );
		tmp = slope[0];
		slope[0] = -slope[1];
		slope[1] = tmp;

		addscaled( v1[v], v1[v], slope, -0.5 );
		addscaled( v4[v], v1[v], slope, 1.0 );
		addscaled( v2[v], v2[v], slope, -0.5 );
		addscaled( v3[v], v2[v], slope, 1.0 );

		while( pImage && pImage->pParent )
		{
			glDepth = 0;
			if(pImage->transform )
			{
				Apply( pImage->transform, v1[1-v], v1[v] );
				Apply( pImage->transform, v2[1-v], v2[v] );
				Apply( pImage->transform, v3[1-v], v3[v] );
				Apply( pImage->transform, v4[1-v], v4[v] );
				v = 1-v;
			}
			pImage = pImage->pParent;
		}
		if( pImage->transform )
		{
			Apply( pImage->transform, v1[1-v], v1[v] );
			Apply( pImage->transform, v2[1-v], v2[v] );
			Apply( pImage->transform, v3[1-v], v3[v] );
			Apply( pImage->transform, v4[1-v], v4[v] );
			v = 1-v;
		}
#if 0
		if( glDepth )
			glEnable( GL_DEPTH_TEST );
		else
         glDisable( GL_DEPTH_TEST );
#endif
      glBegin( GL_TRIANGLE_STRIP );
		glColor4ub( RedVal(d), GreenVal(d),BlueVal(d), 255 );
		scale( v1[v], v1[v], l.scale );
		scale( v2[v], v2[v], l.scale );
		scale( v3[v], v3[v], l.scale );
		scale( v4[v], v4[v], l.scale );
		glVertex3fv(v1[v]);	// Bottom Left Of The Texture and Quad
		glVertex3fv(v2[v]);	// Bottom Right Of The Texture and Quad
		glVertex3fv(v4[v]);	// Bottom Left Of The Texture and Quad
		glVertex3fv(v3[v]);	// Bottom Right Of The Texture and Quad
		glEnd();

	}
   else
	{
   int err, delx, dely, len, inc;
   if( !pImage || !pImage->image ) return;
   delx = x2 - x1;
   if( delx < 0 )
      delx = -delx;

   dely = y2 - y1;
   if( dely < 0 )
      dely = -dely;

   if( dely > delx ) // length for y is more than length for x
   {
      len = dely;
      if( y1 > y2 )
      {
         int tmp = x1;
         x1 = x2;
         x2 = tmp;
         y1 = y2; // x1 is start...
      }
      if( x2 > x1 )
         inc = 1;
      else
         inc = -1;

      err = -(dely / 2);
      while( len >= 0 )
      {
         plot( pImage, x1, y1, d );
         y1++;
         err += delx;
         while( err >= 0 )
         {
            err -= dely;
            x1 += inc;
         }
         len--;
      }
   }
   else
   {
      if( !delx ) // 0 length line
         return;
      len = delx;
      if( x1 > x2 )
      {
         int tmp = y1;
         y1 = y2;
         y2 = tmp;
         x1 = x2; // x1 is start...
      }
      if( y2 > y1 )
         inc = 1;
      else
         inc = -1;

      err = -(delx / 2);
      while( len >= 0 )
      {
         plot( pImage, x1, y1, d );
         x1++;
         err += dely;
         while( err >= 0 )
         {
            err -= delx;
            y1 += inc;
         }
         len--;
      }
   }
   MarkImageUpdated( pImage );
	}
}

void CPROC do_lineAlpha( Image pImage, int32_t x1, int32_t y1
                            , int32_t x2, int32_t y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		VECTOR slope;
		RCOORD tmp;
		int v = 0;

		TranslateCoord( pImage, (int32_t*)&x1, (int32_t*)&y1 );
		TranslateCoord( pImage, (int32_t*)&x2, (int32_t*)&y2 );
		v1[v][0] = x1;
		v1[v][1] = y1;
		v1[v][2] = 0.0;
		v2[v][0] = x2;
		v2[v][1] = y2;
		v2[v][2] = 0.0;

		sub( slope, v2[v], v1[v] );
		normalize( slope );
		tmp = slope[0];
		slope[0] = -slope[1];
		slope[1] = tmp;

		addscaled( v1[v], v1[v], slope, -0.5 );
		addscaled( v4[v], v1[v], slope, 1.0 );
		addscaled( v2[v], v2[v], slope, -0.5 );
		addscaled( v3[v], v2[v], slope, 1.0 );

		while( pImage && pImage->pParent )
		{
			glDepth = 0;
			if(pImage->transform )
			{
				Apply( pImage->transform, v1[1-v], v1[v] );
				Apply( pImage->transform, v2[1-v], v2[v] );
				Apply( pImage->transform, v3[1-v], v3[v] );
				Apply( pImage->transform, v4[1-v], v4[v] );
				v = 1-v;
			}
			pImage = pImage->pParent;
		}
		if(pImage->transform )
		{
			Apply( pImage->transform, v1[1-v], v1[v] );
			Apply( pImage->transform, v2[1-v], v2[v] );
			Apply( pImage->transform, v3[1-v], v3[v] );
			Apply( pImage->transform, v4[1-v], v4[v] );
			v = 1-v;
		}
#if 0
		if( glDepth )
			glEnable( GL_DEPTH_TEST );
		else
			glDisable( GL_DEPTH_TEST );
#endif
		glBegin( GL_TRIANGLE_STRIP );
		glColor4ub( RedVal(d), GreenVal(d),BlueVal(d), AlphaVal( d ) );
		scale( v1[v], v1[v], l.scale );
		scale( v2[v], v2[v], l.scale );
		scale( v3[v], v3[v], l.scale );
		scale( v4[v], v4[v], l.scale );
		glVertex3fv(v1[v]);	// Bottom Left Of The Texture and Quad
		glVertex3fv(v2[v]);	// Bottom Right Of The Texture and Quad
		glVertex3fv(v4[v]);	// Bottom Left Of The Texture and Quad
		glVertex3fv(v3[v]);	// Bottom Right Of The Texture and Quad
		glEnd();
	}
	else
	{
		int err, delx, dely, len, inc;
		if( !pImage || !pImage->image ) return;
   delx = x2 - x1;
   if( delx < 0 )
      delx = -delx;

   dely = y2 - y1;
   if( dely < 0 )
      dely = -dely;

   if( dely > delx ) // length for y is more than length for x
   {
      len = dely;
      if( y1 > y2 )
      {
         int tmp = x1;
         x1 = x2;
         x2 = tmp;
         y1 = y2; // x1 is start...
      }
      if( x2 > x1 )
         inc = 1;
      else
         inc = -1;

      err = -(dely / 2);
      while( len >= 0 )
      {
         plotalpha( pImage, x1, y1, d );
         y1++;
         err += delx;
         while( err >= 0 )
         {
            err -= dely;
            x1 += inc;
         }
         len--;
      }
   }
   else
   {
      if( !delx ) // 0 length line
         return;
      len = delx;
      if( x1 > x2 )
      {
         int tmp = y1;
         y1 = y2;
         y2 = tmp;
         x1 = x2; // x1 is start...
      }
      if( y2 > y1 )
         inc = 1;
      else
         inc = -1;

      err = -(delx / 2);
      while( len >= 0 )
      {
         plotalpha( pImage, x1, y1, d );
         x1++;
         err += dely;
         while( err >= 0 )
         {
            err -= delx;
            y1 += inc;
         }
         len--;
      }
   }
   MarkImageUpdated( pImage );
	}
}

void CPROC do_lineExV( Image pImage, int32_t x1, int32_t y1
                            , int32_t x2, int32_t y2, uintptr_t d
                            , void (*func)(Image pif, int32_t x, int32_t y, uintptr_t d ) )
{
   int err, delx, dely, len, inc;
   //if( !pImage || !pImage->image ) return;
   delx = x2 - x1;
   if( delx < 0 )
      delx = -delx;

   dely = y2 - y1;
   if( dely < 0 )
      dely = -dely;

   if( dely > delx ) // length for y is more than length for x
   {
      len = dely;
      if( y1 > y2 )
      {
         int tmp = x1;
         x1 = x2;
         x2 = tmp;
         y1 = y2; // x1 is start...
      }
      if( x2 > x1 )
         inc = 1;
      else
         inc = -1;

      err = -(dely / 2);
      while( len >= 0 )
      {
         func( pImage, x1, y1, d );
         y1++;
         err += delx;
         while( err >= 0 )
         {
            err -= dely;
            x1 += inc;
         }
         len--;
      }
   }
   else
   {
      if( !delx ) // 0 length line
         return;
      len = delx;
      if( x1 > x2 )
      {
         int tmp = y1;
         y1 = y2;
         y2 = tmp;
         x1 = x2; // x1 is start...
      }
      if( y2 > y1 )
         inc = 1;
      else
         inc = -1;

      err = -(delx / 2);
      while( len >= 0 )
      {
         func( pImage, x1, y1, d );
         x1++;
         err += dely;
         while( err >= 0 )
         {
            err -= delx;
            y1 += inc;
         }
         len--;
      }
   // pImageFile is not nesseciarily an image; do not set updated.
	}
}

void CPROC do_hline( Image pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color )
{
   BlatColor( pImage, xfrom, y, xto-xfrom, 1, color );
   //do_linec( pImage, xfrom, y, xto, y, color );
}

void CPROC do_vline( Image pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color )
{
   BlatColor( pImage, x, yfrom, 1, yto-yfrom, color );
   //do_linec( pImage, x, yfrom, x, yto, color );
}

void CPROC do_hlineAlpha( Image pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color )
{
   BlatColorAlpha( pImage, xfrom, y, xto-xfrom, 1, color );
   //do_lineAlphac( pImage, xfrom, y, xto, y, color );
}

void CPROC do_vlineAlpha( Image pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color )
{
   BlatColorAlpha( pImage, x, yfrom, 1, yto-yfrom, color );
	//do_lineAlphac( pImage, x, yfrom, x, yto, color );

}

#ifdef __cplusplus
		} //extern "C" {
	} //	namespace image {
} //namespace sack {
#endif

