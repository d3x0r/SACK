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
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include <d3d10_1.h>
#include <d3d10.h>
#include <imglib/imagestruct.h>
#include <image.h>

#include "local.h"
#include "blotproto.h"

/* void do_line(BITMAP *bmp, int x1, y1, x2, y2, int d, void (*proc)())
 *  Calculates all the points along a line between x1, y1 and x2, y2,
 *  calling the supplied function for each one. This will be passed a
 *  copy of the bmp parameter, the x and y position, and a copy of the
 *  d parameter (so do_line() can be used with putpixel()).
 */
IMAGE_NAMESPACE
#define int S_32
//unsigned long DOALPHA( unsigned long over, unsigned long in, unsigned long a );

#define FIX_SHIFT 18
#define ROUND_ERROR ( ( 1<< ( FIX_SHIFT - 1 ) ) - 1 )


void CPROC do_line( ImageFile *pImage, int x1, int y1
						 , int x2, int y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		VECTOR slope;
		RCOORD tmp;
		VECTOR normal;
		int v = 0;

		TranslateCoord( pImage, (S_32*)&x1, (S_32*)&y1 );
		TranslateCoord( pImage, (S_32*)&x2, (S_32*)&y2 );
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

			static ID3D10Buffer *pQuadVB;
			if( !pQuadVB )
			{
				D3D10_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D10_USAGE_DEFAULT;
				bufferDesc.ByteWidth        = sizeof( D3DPOSVERTEX ) * 4;
				bufferDesc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = 0;
				bufferDesc.MiscFlags        = 0;
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB);
			}
			D3DPOSVERTEX* pData;
			//lock buffer (NEW)
			pQuadVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&pData);
			//copy data to buffer (NEW)
			{
				pData[0].fX = v1[v][vRight] * l.scale;
				pData[0].fY = v1[v][vUp] * l.scale;
				pData[0].fZ = v1[v][vForward] * l.scale;
				pData[1].fX = v2[v][vRight] * l.scale;
				pData[1].fY = v2[v][vUp] * l.scale;
				pData[1].fZ = v2[v][vForward] * l.scale;
				pData[2].fX = v4[v][vRight] * l.scale;
				pData[2].fY = v4[v][vUp] * l.scale;
				pData[2].fZ = v4[v][vForward] * l.scale;
				pData[3].fX = v3[v][vRight] * l.scale;
				pData[3].fY = v3[v][vUp] * l.scale;
				pData[3].fZ = v3[v][vForward] * l.scale;
			}
			//unlock buffer (NEW)
			pQuadVB->Unmap();
			float _color[4];
			_color[0] = RedVal( d ) / 255.0f;
			_color[1] = GreenVal( d ) / 255.0f;
			_color[2] = BlueVal( d ) / 255.0f;
			_color[3] = AlphaVal( d ) / 255.0f;
			EnableShader( l.simple_shader, pQuadVB, _color );
			//g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
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

void CPROC do_lineAlpha( ImageFile *pImage, int x1, int y1
				                , int x2, int y2, CDATA d )
{
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		int glDepth = 1;
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		VECTOR slope;
		RCOORD tmp;
		VECTOR normal;
		int v = 0;

		TranslateCoord( pImage, (S_32*)&x1, (S_32*)&y1 );
		TranslateCoord( pImage, (S_32*)&x2, (S_32*)&y2 );
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


			static ID3D10Buffer *pQuadVB;
			if( !pQuadVB )
			{
				D3D10_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D10_USAGE_DEFAULT;
				bufferDesc.ByteWidth        = sizeof( D3DPOSVERTEX ) * 4;
				bufferDesc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = 0;
				bufferDesc.MiscFlags        = 0;
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB);
			}
			D3DPOSVERTEX* pData;
			//lock buffer (NEW)
			pQuadVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&pData);
			//copy data to buffer (NEW)
			{
				pData[0].fX = v1[v][vRight] * l.scale;
				pData[0].fY = v1[v][vUp] * l.scale;
				pData[0].fZ = v1[v][vForward] * l.scale;
				pData[1].fX = v2[v][vRight] * l.scale;
				pData[1].fY = v2[v][vUp] * l.scale;
				pData[1].fZ = v2[v][vForward] * l.scale;
				pData[2].fX = v4[v][vRight] * l.scale;
				pData[2].fY = v4[v][vUp] * l.scale;
				pData[2].fZ = v4[v][vForward] * l.scale;
				pData[3].fX = v3[v][vRight] * l.scale;
				pData[3].fY = v3[v][vUp] * l.scale;
				pData[3].fZ = v3[v][vForward] * l.scale;
			}
			//unlock buffer (NEW)
			pQuadVB->Unmap();

			float _color[4];
			_color[0] = RedVal( d ) / 255.0f;
			_color[1] = GreenVal( d ) / 255.0f;
			_color[2] = BlueVal( d ) / 255.0f;
			_color[3] = AlphaVal( d ) / 255.0f;
			EnableShader( l.simple_shader, pQuadVB, _color );

			//g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
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

void CPROC do_lineExV( ImageFile *pImage, int x1, int y1
				                , int x2, int y2, PTRSZVAL d
				                , void (*func)(ImageFile *pif, int x, int y, PTRSZVAL d ) )
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
	}
}

void CPROC do_hline( ImageFile *pImage, int y, int xfrom, int xto, CDATA color )
{
	if( xfrom < xto )
		BlatColor( pImage, xfrom, y, xto-xfrom, 1, color );
	else
		BlatColor( pImage, xfrom, y, xfrom-xto, 1, color );
}

void CPROC do_vline( ImageFile *pImage, int x, int yfrom, int yto, CDATA color )
{
	if( yto > yfrom )
		BlatColor( pImage, x, yfrom, 1, yto-yfrom, color );
	else
		BlatColor( pImage, x, yfrom, 1, yfrom-yto, color );
}

void CPROC do_hlineAlpha( ImageFile *pImage, int y, int xfrom, int xto, CDATA color )
{
	if( xfrom < xto )
		BlatColorAlpha( pImage, xfrom, y, xto-xfrom, 1, color );
	else
		BlatColorAlpha( pImage, xfrom, y, xfrom-xto, 1, color );
}

void CPROC do_vlineAlpha( ImageFile *pImage, int x, int yfrom, int yto, CDATA color )
{
	if( yto > yfrom )
		BlatColorAlpha( pImage, x, yfrom, 1, yto-yfrom, color );
	else
		BlatColorAlpha( pImage, x, yfrom, 1, yfrom-yto, color );
}

IMAGE_NAMESPACE_END

// $Log: line.c,v $
// Revision 1.14  2004/06/21 07:47:13  d3x0r
// Account for newly moved structure files.
//
// Revision 1.13  2003/07/24 16:56:41  panther
// Updates to expliclity define C procedure model for callbacks and assembly modules - incomplete
//
// Revision 1.12  2003/03/31 01:11:28  panther
// Tweaks to work better under service application
//
// Revision 1.11  2003/03/30 18:18:02  panther
// More clip fixes
//
// Revision 1.10  2003/03/30 06:24:56  panther
// Turns out I had badly implemented clipping...
//
// Revision 1.9  2003/03/27 10:50:59  panther
// Display - enable resize that works.  Image - remove hline failed message.  Display - Removed some logging messages.
//
// Revision 1.8  2003/03/25 23:35:59  panther
// Base INVERT_IMAGE off off real_height.  Also updated to use more base ComputeImageData
//
// Revision 1.7  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
