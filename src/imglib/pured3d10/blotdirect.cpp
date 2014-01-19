/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 *
 *  Support for putting one image on another without scaling.
 *
 *
 *
 *  consult doc/image.html
 *
 */

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif

#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#include <sharemem.h>

#include <d3d10_1.h>
#include <d3d10.h>

#include <imglib/imagestruct.h>
#include <image.h>

#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"
#include "../image_common.h"
IMAGE_NAMESPACE


//---------------------------------------------------------------------------

#define StartLoop oo /= 4;    \
   oi /= 4;                   \
   {                          \
      _32 row= 0;             \
      while( row < hs )       \
      {                       \
         _32 col=0;           \
         while( col < ws )    \
         {                    \
            {

#define EndLoop   }           \
/*lprintf( "in %08x out %08x", ((CDATA*)pi)[0], ((CDATA*)po)[1] );*/ \
            po++;             \
            pi++;             \
            col++;            \
         }                    \
         pi += oi;            \
         po += oo;            \
         row++;               \
      }                       \
   }

 void CPROC cCopyPixelsT0( PCDATA po, PCDATA  pi
                          , S_32 oo, S_32 oi
                          , _32 ws, _32 hs
                           )
{
   StartLoop
            *po = *pi;
   EndLoop
}

//---------------------------------------------------------------------------

 void CPROC cCopyPixelsT1( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsTA( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsTImgA( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsTImgAI( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsShadedT0( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsShadedT1( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsShadedTA( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsShadedTImgA( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsShadedTImgAI( PCDATA po, PCDATA  pi
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

 void CPROC cCopyPixelsMultiT0( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsMultiT1( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsMultiTA( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsMultiTImgA( PCDATA po, PCDATA  pi
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
 void CPROC cCopyPixelsMultiTImgAI( PCDATA po, PCDATA  pi
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
	//int  hd, wd;
	S_32 oo, oi; // treated as an adder... it is unsigned by math, but still results correct offset?
	static _32 lock;
	va_list colors;
	va_start( colors, method );
	if( nTransparent > ALPHA_TRANSPARENT_MAX )
		return;

	if(  !pifSrc
		|| !pifSrc->image
		|| !pifDest
		//|| !pifDest->image

	  )
	{
		// this will happen when mixing modes...
		lprintf( WIDE( "sanity check failed %p %p %p" ), pifSrc, pifDest, pifSrc?pifSrc->image:NULL );
		return;
	}
	//lprintf( WIDE( "BlotImageSized %d,%d to %d,%d by %d,%d" ), xs, ys, xd, yd, ws, hs );

	{
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
			//lprintf( WIDE( "Images do not overlap. %d,%d %d,%d vs %d,%d %d,%d" ), r1.x,r1.y,r1.width,r1.height
			//		 , r2.x,r2.y,r2.width,r2.height);
			return;
		}

		//lprintf( WIDE( "Correcting coordinates by %d,%d" )
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
		//lprintf( WIDE( "Resulting dest is %d,%d %d,%d" ), rd.x,rd.y,rd.width,rd.height );
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
			lprintf( WIDE( "Desired Output does not overlap..." ) );
			return;
		}
		//lprintf( WIDE( "Resulting dest is %d,%d %d,%d" ), rs.x,rs.y,rs.width,rs.height );
		ws = rs.width<rd.width?rs.width:rd.width;
		hs = rs.height<rd.height?rs.height:rd.height;
		xs = rs.x;
		ys = rs.y;
		//lprintf( WIDE( "Resulting rect is %d,%d to %d,%d dim: %d,%d" ), rs.x, rs.y, rd.x, rd.y, rs.width, rs.height );
		//lprintf( WIDE( "Resulting rect is %d,%d to %d,%d dim: %d,%d" ), xs, ys, xd, yd, ws, hs );
	}
		//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	if( (S_32)ws <= 0 ||
        (S_32)hs <= 0 /*||
        (S_32)wd <= 0 ||
		(S_32)hd <= 0 */ )
	{
		lprintf( WIDE( "out of bounds" ) );
		return;
	}

	if( pifSrc->flags & IF_FLAG_INVERTED )
	{
		// set pointer in to the starting x pixel
		// on the last line of the image to be copied
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		po = IMG_ADDRESS( pifDest, xd, yd );
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(492) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
//cpg 19 Jan 2007 2>c:\work\sack\src\imglib\blotdirect.c(493) : warning C4146: unary minus operator applied to unsigned type, result still unsigned
		oo = 4*-(int)(ws+pifDest->pwidth);     // w is how much we can copy...
		oi = 4*-(int)(ws+pifSrc->pwidth); // adding remaining width...
	}
	else
	{
		// set pointer in to the starting x pixel
		// on the first line of the image to be copied...
		pi = IMG_ADDRESS( pifSrc, xs, ys );
		po = IMG_ADDRESS( pifDest, xd, yd );
		oo = 4*(pifDest->pwidth - ws);     // w is how much we can copy...
		oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	}
	//lprintf( WIDE("Doing image (%d,%d)-(%d,%d) (%d,%d)-(%d,%d)"), xs, ys, ws, hs, xd, yd, wd, hd );
	//oo = 4*(pifDest->pwidth - ws);     // w is how much we can copy...
	//oi = 4*(pifSrc->pwidth - ws); // adding remaining width...
	while( LockedExchange( &lock, 1 ) )
		Relinquish();

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
	{
		//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );
      //DebugBreak();        g

		/*
		 * only a portion of the image is actually used, the rest is filled with blank space
		 *
		 */
		{
			int glDepth = 1;
			RCOORD x_size, x_size2, y_size, y_size2;
			VECTOR v1[2], v3[2],v4[2],v2[2];
			CDATA color = 0xffffffff;
			int v = 0;

			TranslateCoord( pifDest, &xd, &yd );
			TranslateCoord( pifSrc, &xs, &ys );

			v1[v][0] = xd;
			v1[v][1] = yd;
			v1[v][2] = 0.0;

			v2[v][0] = xd+ws;
			v2[v][1] = yd;
			v2[v][2] = 0.0;

			v3[v][0] = xd;
			v3[v][1] = yd+hs;
			v3[v][2] = 0.0;

			v4[v][0] = xd+ws;
			v4[v][1] = yd+hs;
			v4[v][2] = 0.0;


			Image pifSrcReal;
			for( pifSrcReal = pifSrc; pifSrcReal->pParent; pifSrcReal = pifSrcReal->pParent );
			x_size = (RCOORD) xs/ (RCOORD)pifSrcReal->width;
			x_size2 = (RCOORD) (xs+ws)/ (RCOORD)pifSrcReal->width;
			y_size = (RCOORD) ys/ (RCOORD)pifSrcReal->height;
			y_size2 = (RCOORD) (ys+hs)/ (RCOORD)pifSrcReal->height;

			//lprintf( WIDE( "Texture size is %g,%g to %g,%g" ), x_size, y_size, x_size2, y_size2 );
			while( pifDest && pifDest->pParent )
			{
				glDepth = 0;
				if( pifDest->transform )
				{
					Apply( pifDest->transform, v1[1-v], v1[v] );
					Apply( pifDest->transform, v2[1-v], v2[v] );
					Apply( pifDest->transform, v3[1-v], v3[v] );
					Apply( pifDest->transform, v4[1-v], v4[v] );
					v = 1-v;
				}
				pifDest = pifDest->pParent;
			}
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v1[1-v], v1[v] );
				Apply( pifDest->transform, v2[1-v], v2[v] );
				Apply( pifDest->transform, v3[1-v], v3[v] );
				Apply( pifDest->transform, v4[1-v], v4[v] );
				v = 1-v;
			}

			static ID3D10Buffer *pQuadVB;
			if( !pQuadVB )
			{
				D3D10_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D10_USAGE_DEFAULT;
				bufferDesc.ByteWidth        = sizeof( D3DTEXTUREDVERTEX ) * 4;
				bufferDesc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = 0;
				bufferDesc.MiscFlags        = 0;
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB);
			}
			D3DTEXTUREDVERTEX* pData;

			//lock buffer (NEW)
			pQuadVB->Map(D3D10_MAP_WRITE_DISCARD, 0, (void**)&pData);
			//copy data to buffer (NEW)
			{
				pData[0].fX = v1[v][vRight] * l.scale;
				pData[0].fY = v1[v][vUp] * l.scale;
				pData[0].fZ = v1[v][vForward] * l.scale;
				pData[0].fU1 = x_size;
				pData[0].fV1 = y_size;
				pData[1].fX = v2[v][vRight] * l.scale;
				pData[1].fY = v2[v][vUp] * l.scale;
				pData[1].fZ = v2[v][vForward] * l.scale;
				pData[1].fU1 = x_size2;
				pData[1].fV1 = y_size;
				pData[2].fX = v3[v][vRight] * l.scale;
				pData[2].fY = v3[v][vUp] * l.scale;
				pData[2].fZ = v3[v][vForward] * l.scale;
				pData[2].fU1 = x_size;
				pData[2].fV1 = y_size2;
				pData[3].fX = v4[v][vRight] * l.scale;
				pData[3].fY = v4[v][vUp] * l.scale;
				pData[3].fZ = v4[v][vForward] * l.scale;
				pData[3].fU1 = x_size2;
				pData[3].fV1 = y_size2;
			}
			//unlock buffer (NEW)
			pQuadVB->Unmap();

			if( method == BLOT_COPY )
			{
				EnableShader( l.simple_texture_shader, pQuadVB, pifSrc );
			}
			else if( method == BLOT_SHADED )
			{
				color = va_arg( colors, CDATA );
				float _color[4];
				_color[0] = RedVal( color ) / 255.0f;
				_color[1] = GreenVal( color ) / 255.0f;
				_color[2] = BlueVal( color ) / 255.0f;
				_color[3] = AlphaVal( color ) / 255.0f;
				EnableShader( l.simple_shaded_texture_shader, pQuadVB, pifSrc, _color );
			}
			else if( method == BLOT_MULTISHADE )
			{
				Image output_image;
				CDATA r = va_arg( colors, CDATA );
				CDATA g = va_arg( colors, CDATA );
				CDATA b = va_arg( colors, CDATA );
				float r_color[4];
				float g_color[4];
				float b_color[4];
				r_color[0] = RedVal( r ) / 255.0f;
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
				EnableShader( l.simple_multi_shaded_texture_shader, pQuadVB, pifSrc, r_color, g_color, b_color );
			}
			else if( method == BLOT_INVERTED )
			{
				//EnableShader( l.simple_inverted_texture_shader, pQuadVB, pifSrc );
			}

			{
				unsigned int stride;
				unsigned int offset;


				// Set vertex buffer stride and offset.
				stride = sizeof(D3DTEXTUREDVERTEX); 
				offset = 0;
			
				// Set the vertex buffer to active in the input assembler so it can be rendered.
				g_d3d_device->IASetVertexBuffers(0, 1, &pQuadVB, &stride, &offset);

				// Set the index buffer to active in the input assembler so it can be rendered.
				g_d3d_device->IASetIndexBuffer(pQuadVB, DXGI_FORMAT_R32_UINT, 0);

				// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
				g_d3d_device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
		}
	}
	else
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
				//lprintf( WIDE( "r g b %08x %08x %08x" ), r,g, b );
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
		MarkImageUpdated( pifDest );
	}
	lock = 0;
	//lprintf( WIDE( "Image done.." ) );
}
// copy all of pifSrc to the destination - placing the upper left
// corner of pifSrc on the point specified.
void  BlotImageEx ( ImageFile *pifDest, ImageFile *pifSrc, S_32 xd, S_32 yd, _32 nTransparent, _32 method, ... )
{
	va_list colors;
	CDATA r;
	CDATA g;
	CDATA b;
	va_start( colors, method );
	r = va_arg( colors, CDATA );
	g = va_arg( colors, CDATA );
	b = va_arg( colors, CDATA );
	BlotImageSizedEx( pifDest, pifSrc, xd, yd, 0, 0
                   , pifSrc->real_width, pifSrc->real_height, nTransparent, method
                                      , r,g,b
                                    );
}

IMAGE_NAMESPACE_END


