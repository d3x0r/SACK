/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle putting out one image scaled onto another image; clips to bounds of sub-image
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

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include <d3d10_1.h>
#include <d3d10.h>
#include <D3DX10math.h>

#include <sharemem.h>
#include <imglib/imagestruct.h>
#include <colordef.h>
#include "image.h"
#include "local.h"
#define NEED_ALPHA2
#include "blotproto.h"
#include "../image_common.h"
IMAGE_NAMESPACE

#if !defined( _WIN32 ) && !defined( NO_TIMING_LOGGING )
	// as long as I don't include windows.h...
typedef struct rect_tag {
   _32 left;
   _32 right;
   _32 top;
   _32 bottom;
} RECT;
#endif

#define TOFIXED(n)   ((n)<<FIXED_SHIFT)
#define FROMFIXED(n)   ((n)>>FIXED_SHIFT)
#define TOPFROMFIXED(n) (((n)+(FIXED-1))>>FIXED_SHIFT)
#define FIXEDPART(n)    ((n)&(FIXED-1))
#define FIXED        1024
#define FIXED_SHIFT  10

//---------------------------------------------------------------------------

#define ScaleLoopStart int errx, erry; \
   _32 x, y;                     \
   PCDATA _pi = pi;              \
   erry = i_erry;                \
   y = 0;                        \
   while( y < hd )               \
   {                            \
      /* move first line.... */ \
      errx = i_errx;            \
      x = 0;                    \
      pi = _pi;                 \
      while( x < wd )           \
      {                         \
         {

#define ScaleLoopEnd  }             \
         po++;                      \
         x++;                       \
         errx += (signed)dws; /* add source width */\
         while( errx >= 0 )               \
         {                                \
            errx -= (signed)dwd; /* fix backwards the width we're copying*/\
            pi++;                         \
         }                                \
      }                                   \
      po = (CDATA*)(((char*)po) + oo);    \
      y++;                                \
      erry += (signed)dhs;                        \
      while( erry >= 0 )                  \
      {                                   \
         erry -= (signed)dhd;                      \
         _pi = (CDATA*)(((char*)_pi) + srcpwidth); /* go to next line start*/\
      }                                   \
   }

//---------------------------------------------------------------------------

#define PIXCOPY   

#define TCOPY  if( *pi )  *(po) = *(pi);

#define ACOPY  CDATA cin;  if( cin = *pi )        \
      {                                           \
         *po = DOALPHA2( *po, cin, nTransparent ); \
      }

#define IMGACOPY     CDATA cin;                    \
      int alpha;                                   \
      if( cin = *pi )                              \
      {                                            \
         alpha = ( cin & 0xFF000000 ) >> 24;       \
         alpha += nTransparent;                    \
         *po = DOALPHA2( *po, cin, alpha );         \
      }

#define IMGINVACOPY  CDATA cin;                    \
      _32 alpha;                                   \
      if( (cin = *pi) )                              \
      {                                            \
         alpha = ( cin & 0xFF000000 ) >> 24;       \
         alpha -= nTransparent;                    \
         if( alpha > 1 )                           \
            *po = DOALPHA2( *po, cin, alpha );      \
      }


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
	_32  oo;
	_32 srcwidth;
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
	   || !pifSrc->image //|| !pifDest->image
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
	//lprintf(" begin scaled output..." );
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
	//       xs, ys, ws, hs, xd, yd, wd, hd );
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
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( xd + (signed)wd ) > ( pifDest->x + pifDest->width) )
	{
		//int newwd = TOFIXED(pifDest->width);
		//ws -= ((S_64)( (int)wd - newwd)* (S_64)ws )/(int)wd;
		wd = ( pifDest->x + pifDest->width ) - xd;
	}
	//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
	//       xs, ys, ws, hs, xd, yd, wd, hd );
	if( ( yd + (signed)hd ) > (pifDest->y + pifDest->height) )
	{
		//int newhd = TOFIXED(pifDest->height);
		//hs -= ((S_64)( hd - newhd)* hs )/hd;
		hd = (pifDest->y + pifDest->height) - yd;
	}
	if( (S_32)wd <= 0 ||
       (S_32)hd <= 0 ||
       (S_32)ws <= 0 ||
		 (S_32)hs <= 0 )
	{
		return;
	}
   
	while( LockedExchange( &lock, 1 ) )
		Relinquish();
   //Log8( WIDE("Do blot work...%d(%d),%d(%d) %d(%d) %d(%d)")
   //    , ws, FROMFIXED(ws), hs, FROMFIXED(hs) 
	//    , wd, FROMFIXED(wd), hd, FROMFIXED(hd) );

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
	{
		int updated = 0;
		Image topmost_parent;

		// closed loop to get the top imgae size.
		for( topmost_parent = pifSrc; topmost_parent->pParent; topmost_parent = topmost_parent->pParent );
		//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );

		{
			_32 color = 0xffffffff;
			int glDepth = 1;
			VECTOR v1[2], v3[2],v4[2],v2[2];
			int v = 0;
			double x_size, x_size2, y_size, y_size2;
			/*
			 * only a portion of the image is actually used, the rest is filled with blank space
			 *
			 */
			TranslateCoord( pifDest, &xd, &yd );
			TranslateCoord( pifSrc, &xs, &ys );

			v1[v][0] = xd;
			v1[v][1] = yd;
			v1[v][2] = 0.0;

			v2[v][0] = xd+wd;
			v2[v][1] = yd;
			v2[v][2] = 0.0;

			v3[v][0] = xd;
			v3[v][1] = yd+hd;
			v3[v][2] = 0.0;

			v4[v][0] = xd+wd;
			v4[v][1] = yd+hd;
			v4[v][2] = 0.0;

			x_size = (RCOORD) xs/ (RCOORD)topmost_parent->width;
			x_size2 = (RCOORD) (xs+ws)/ (RCOORD)topmost_parent->width;
			y_size = (RCOORD) ys/ (RCOORD)topmost_parent->height;
			y_size2 = (RCOORD) (ys+hs)/ (RCOORD)topmost_parent->height;
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

			// Lock the vertex buffer.
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
			// Unlock the vertex buffer.
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
		//Log9( WIDE("Image locations: %d(%d %d) %d(%d) %d(%d) %d(%d)")
		//          , xs, FROMFIXED(xs), FIXEDPART(xs)
		//          , ys, FROMFIXED(ys)
		//          , xd, FROMFIXED(xd)
		//          , yd, FROMFIXED(yd) );
		if( pifSrc->flags & IF_FLAG_INVERTED )
		{
			// set pointer in to the starting x pixel
			// on the last line of the image to be copied
			pi = IMG_ADDRESS( pifSrc, (xs), (ys) );
			po = IMG_ADDRESS( pifDest, (xd), (yd) );
			oo = 4*(-((signed)wd) - (pifDest->pwidth) ); // w is how much we can copy...
			// adding in multiple of 4 because it's C...
			srcwidth = -(4* pifSrc->pwidth);
		}
		else
		{
			// set pointer in to the starting x pixel
			// on the first line of the image to be copied...
			pi = IMG_ADDRESS( pifSrc, (xs), (ys) );
			po = IMG_ADDRESS( pifDest, (xd), (yd) );
			oo = 4*(pifDest->pwidth - (wd)); // w is how much we can copy...
			// adding in multiple of 4 because it's C...
			srcwidth = 4* pifSrc->pwidth;
		}
		switch( method )
		{
		case BLOT_COPY:
			if( !nTransparent )
				cBlotScaledT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );
			else if( nTransparent == 1 )
				cBlotScaledT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth );
			else if( nTransparent & ALPHA_TRANSPARENT )
				cBlotScaledTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				cBlotScaledTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF );
			else
				cBlotScaledTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent );
			break;
		case BLOT_SHADED:
			if( !nTransparent )
				cBlotScaledShadedT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
			else if( nTransparent == 1 )
				cBlotScaledShadedT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT )
				cBlotScaledShadedTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
			else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
				cBlotScaledShadedTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent&0xFF, va_arg( colors, CDATA ) );
			else
				cBlotScaledShadedTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth, nTransparent, va_arg( colors, CDATA ) );
			break;
		case BLOT_MULTISHADE:
			{
				CDATA r,g,b;
				r = va_arg( colors, CDATA );
				g = va_arg( colors, CDATA );
				b = va_arg( colors, CDATA );
				if( !nTransparent )
					cBlotScaledMultiT0( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
										  , r, g, b );
				else if( nTransparent == 1 )
					cBlotScaledMultiT1( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
										  , r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT )
					cBlotScaledMultiTImgA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
											  , nTransparent & 0xFF
											  , r, g, b );
				else if( nTransparent & ALPHA_TRANSPARENT_INVERT )
					cBlotScaledMultiTImgAI( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
												, nTransparent & 0xFF
												, r, g, b );
				else
					cBlotScaledMultiTA( po, pi, errx, erry, wd, hd, dwd, dhd, dws, dhs, oo, srcwidth
										  , nTransparent
										  , r, g, b );
			}
			break;
		}
	}
	lock = 0;
//   Log( WIDE("Blot done") );
}


IMAGE_NAMESPACE_END

