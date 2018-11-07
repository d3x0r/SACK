/*
 *  Crafted by Jim Buckeyne
 *	(c)1999-2006++ Freedom Collective
 *
 *  Simple Line operations on Images.  Single pixel, no anti aliasing.
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

#ifdef USE_GLES2
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
//#include <GL/glew.h>
#include <GL/gl.h>   // Header File For The OpenGL32 Library
#endif

#include <imglib/imagestruct.h>
#include <image.h>

#include "shaders.h"
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


void CPROC do_line( ImageFile *pImage, int32_t x1, int32_t y1
						 , int32_t x2, int32_t y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v[2][4];
		VECTOR slope;
		float _color[4];
		RCOORD tmp;
		int vi = 0;

		TranslateCoord( pImage, (int32_t*)&x1, (int32_t*)&y1 );
		TranslateCoord( pImage, (int32_t*)&x2, (int32_t*)&y2 );
		v[vi][0][0] = x1;
		v[vi][0][1] = y1;
		v[vi][0][2] = 0.0;
		v[vi][1][0] = x2;
		v[vi][1][1] = y2;
		v[vi][1][2] = 0.0;

		sub( slope, v[vi][1], v[vi][0] );
		normalize( slope );
		tmp = slope[0];
		slope[0] = -slope[1];
		slope[1] = tmp;

		addscaled( v[vi][0], v[vi][0], slope, -0.5 );
		addscaled( v[vi][2], v[vi][0], slope, 1.0 );
		addscaled( v[vi][1], v[vi][1], slope, -0.5 );
		addscaled( v[vi][3], v[vi][1], slope, 1.0 );

		while( pImage && pImage->pParent )
		{
			glDepth = 0;
			if(pImage->transform )
			{
				Apply( pImage->transform, v[1-vi][0], v[vi][0] );
				Apply( pImage->transform, v[1-vi][1], v[vi][1] );
				Apply( pImage->transform, v[1-vi][2], v[vi][2] );
				Apply( pImage->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}
			pImage = pImage->pParent;
		}
		if( pImage->transform )
		{
			Apply( pImage->transform, v[1-vi][0], v[vi][0] );
			Apply( pImage->transform, v[1-vi][1], v[vi][1] );
			Apply( pImage->transform, v[1-vi][2], v[vi][2] );
			Apply( pImage->transform, v[1-vi][3], v[vi][3] );
			vi = 1-vi;
		}
#if 0
		if( glDepth )
			glEnable( GL_DEPTH_TEST );
		else
			glDisable( GL_DEPTH_TEST );
#endif
		/**///glBegin( GL_TRIANGLE_STRIP );
		_color[0] = RedVal(d) / 255.0f;
		_color[1] = GreenVal(d) / 255.0f;
		_color[2] = BlueVal(d) / 255.0f;
		_color[3] = AlphaVal(d) / 255.0f;
		;/**///glColor4ub( RedVal(d), GreenVal(d),BlueVal(d), 255 );
		scale( v[vi][0], v[vi][0], l.scale );
		scale( v[vi][1], v[vi][1], l.scale );
		scale( v[vi][2], v[vi][2], l.scale );
		scale( v[vi][3], v[vi][3], l.scale );

		{
			struct image_shader_op *op;
			op = BeginImageShaderOp( GetShader( WIDE("Simple Shader") ), pImage, _color  );
			AppendImageShaderOpTristrip( op, 2, v[vi] );
		}
		//glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		CheckErr();

		/**///glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
		/**///glVertex3dv(v[vi][1]);	// Bottom Right Of The Texture and Quad
		/**///glVertex3dv(v[vi][3]);	// Bottom Left Of The Texture and Quad
		/**///glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
		/**///glEnd();

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

void CPROC do_lineAlpha( ImageFile *pImage, int32_t x1, int32_t y1
                            , int32_t x2, int32_t y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v[2][4];
		VECTOR slope;
		RCOORD tmp;
		float _color[4];
		int vi = 0;

		TranslateCoord( pImage, (int32_t*)&x1, (int32_t*)&y1 );
		TranslateCoord( pImage, (int32_t*)&x2, (int32_t*)&y2 );
		v[vi][0][0] = x1;
		v[vi][0][1] = y1;
		v[vi][0][2] = 0.0;
		v[vi][1][0] = x2;
		v[vi][1][1] = y2;
		v[vi][1][2] = 0.0;

		sub( slope, v[vi][1], v[vi][0] );
		normalize( slope );
		tmp = slope[0];
		slope[0] = -slope[1];
		slope[1] = tmp;

		addscaled( v[vi][0], v[vi][0], slope, -0.5 );
		addscaled( v[vi][2], v[vi][0], slope, 1.0 );
		addscaled( v[vi][1], v[vi][1], slope, -0.5 );
		addscaled( v[vi][3], v[vi][1], slope, 1.0 );

		while( pImage && pImage->pParent )
		{
			glDepth = 0;
			if(pImage->transform )
			{
				Apply( pImage->transform, v[1-vi][0], v[vi][0] );
				Apply( pImage->transform, v[1-vi][1], v[vi][1] );
				Apply( pImage->transform, v[1-vi][2], v[vi][2] );
				Apply( pImage->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}
			pImage = pImage->pParent;
		}
		if(pImage->transform )
		{
			Apply( pImage->transform, v[1-vi][0], v[vi][0] );
			Apply( pImage->transform, v[1-vi][1], v[vi][1] );
			Apply( pImage->transform, v[1-vi][2], v[vi][2] );
			Apply( pImage->transform, v[1-vi][3], v[vi][3] );
			vi = 1-vi;
		}
#if 0
		if( glDepth )
			glEnable( GL_DEPTH_TEST );
		else
			glDisable( GL_DEPTH_TEST );
#endif
		/**///glBegin( GL_TRIANGLE_STRIP );
		;/**///glColor4ub( RedVal(d), GreenVal(d),BlueVal(d), AlphaVal( d ) );
		_color[0] = RedVal(d) / 255.0f;
		_color[1] = GreenVal(d) / 255.0f;
		_color[2] = BlueVal(d) / 255.0f;
		_color[3] = AlphaVal(d) / 255.0f;
		scale( v[vi][0], v[vi][0], l.scale );
		scale( v[vi][1], v[vi][1], l.scale );
		scale( v[vi][2], v[vi][2], l.scale );
		scale( v[vi][3], v[vi][3], l.scale );

		{
			struct image_shader_op *op;
			op = BeginImageShaderOp( GetShader( WIDE("Simple Shader") ), pImage, _color  );
			AppendImageShaderOpTristrip( op, 2, v[vi] );
		}
		//EnableShader( GetShader( WIDE("Simple Shader"), NULL ), v[vi], _color );
		//glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		//CheckErr();

		/**///glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
		/**///glVertex3dv(v[vi][1]);	// Bottom Right Of The Texture and Quad
		/**///glVertex3dv(v[vi][3]);	// Bottom Left Of The Texture and Quad
		/**///glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
		/**///`glEnd();
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

void CPROC do_lineExV( ImageFile *pImage, int32_t x1, int32_t y1
                            , int32_t x2, int32_t y2, uintptr_t d
                            , void (*func)(ImageFile *pif, int32_t x, int32_t y, uintptr_t d ) )
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

void CPROC do_hline( ImageFile *pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color )
{
	BlatColor( pImage, xfrom, y, xto-xfrom, 1, color );
	//do_linec( pImage, xfrom, y, xto, y, color );
}

void CPROC do_vline( ImageFile *pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color )
{
	BlatColor( pImage, x, yfrom, 1, yto-yfrom, color );
	//do_linec( pImage, x, yfrom, x, yto, color );
}

void CPROC do_hlineAlpha( ImageFile *pImage, int32_t y, int32_t xfrom, int32_t xto, CDATA color )
{
	BlatColorAlpha( pImage, xfrom, y, xto-xfrom, 1, color );
	//do_lineAlphac( pImage, xfrom, y, xto, y, color );
}

void CPROC do_vlineAlpha( ImageFile *pImage, int32_t x, int32_t yfrom, int32_t yto, CDATA color )
{
	BlatColorAlpha( pImage, x, yfrom, 1, yto-yfrom, color );
	//do_lineAlphac( pImage, x, yfrom, x, yto, color );

}

#ifdef __cplusplus
		} //extern "C" {
	} //	namespace image {
} //namespace sack {
#endif

