/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle usage of 'SFTFont's on 'Image's.
 * 
 *  *  consult doc/image.html
 *
 */
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#define IMAGE_LIBRARY_SOURCE
#include <imglib/fontstruct.h>
#include <imglib/imagestruct.h>
#include <image.h>

#ifdef _OPENGL_DRIVER
#ifdef USE_GLES2
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#ifdef PURE_OPENGL2_ENABLED
#include "puregl2/local.h"
#else
#include "puregl/local.h"
#endif
#endif

#endif
#include "image_common.h"
#ifdef _D3D_DRIVER
#include <d3d11.h>
#include "local.h"
#endif

//#ifdef UNNATURAL_ORDER
// use this as natural - to avoid any confusion about signs...
#define SYMBIT(bit)  ( 1 << (bit&0x7) )
//#else
//#define SYMBIT(bit) ( 0x080 >> (bit&0x7) )
//#endif

IMAGE_NAMESPACE

static TEXTCHAR maxbase1[] = WIDE("0123456789abcdefghijklmnopqrstuvwxyz");
static TEXTCHAR maxbase2[] = WIDE("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");

#define NUM_COLORS sizeof( color_defs ) / sizeof( color_defs[0] )
static struct {
	CDATA color;
	CTEXTSTR name;
}color_defs[4];

#ifdef __cplusplus
	namespace default_font {
extern   PFONT __LucidaConsole13by8;
	}
using namespace default_font;
#define DEFAULTFONT (*__LucidaConsole13by8)
#else
#define DEFAULTFONT _LucidaConsole13by8
extern FONT DEFAULTFONT;
#endif

//PFONT LucidaConsole13by8 = (PFONT)&_LucidaConsole13by8;


PFONT GetDefaultFont( void )
{
	return &DEFAULTFONT;
}

#define CharPlotAlpha(pImage,x,y,color) plotalpha( pImage, x, y, color )

//---------------------------------------------------------------------------
void CharPlotAlpha8( ImageFile *pImage, S_32 x, S_32 y, _32 data, CDATA fore, CDATA back )
{
	if( back )
		CharPlotAlpha( pImage, x, y, back );
	if( data )
		CharPlotAlpha( pImage, x, y, ( fore & 0xFFFFFF ) | ( (((_32)data*AlphaVal(fore))&0xFF00) << 16 ) );
}
_32 CharData8( _8 *bits, _8 bit )
{
	return bits[bit];
}
//---------------------------------------------------------------------------
void CharPlotAlpha2( ImageFile *pImage, S_32 x, S_32 y, _32 data, CDATA fore, CDATA back )
{
	if( back )
		CharPlotAlpha( pImage, x, y, back );
	if( data )
	{
		CharPlotAlpha( pImage, x, y, ( fore & 0xFFFFFF ) | ( (((_32)((data&3)+1)*(AlphaVal(fore)))&0x03FC) << 22 ) );
	}
}
_32 CharData2( _8 *bits, _8 bit )
{
	return (bits[bit>>2] >> (2*(bit&3)))&3;
}
//---------------------------------------------------------------------------
void CharPlotAlpha1( ImageFile *pImage, S_32 x, S_32 y, _32 data, CDATA fore, CDATA back )
{
	if( data )
		CharPlotAlpha( pImage, x, y, fore );
	else if( back )
		CharPlotAlpha( pImage, x, y, back );
}
_32 CharData1( _8 *bits, _8 bit )
{
	return (bits[bit>>3] >> (bit&7))&1;
}
//---------------------------------------------------------------------------

S_32 StepXNormal(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1+delta1;
}
S_32 StepYNormal(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1+delta1;
}
//---------------------------------------------------------------------------

S_32 StepXVertical(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1-delta2;
}
S_32 StepYVertical(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1+delta2;
}
//---------------------------------------------------------------------------

S_32 StepXInvert(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1-delta1;
}
S_32 StepYInvert(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1-delta1;
}
//---------------------------------------------------------------------------

S_32 StepXInvertVertical(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1+delta2;
}
S_32 StepYInvertVertical(S_32 base1,S_32 delta1,S_32 delta2)
{
	return base1-delta2;
}
//---------------------------------------------------------------------------

enum order_type {
	OrderPoints,OrderPointsVertical,OrderPointsInvert,OrderPointsVerticalInvert
};

static _32 PutCharacterFontX ( ImageFile *pImage
									  , S_32 x, S_32 y
									  , CDATA color, CDATA background
									  , _32 c, PFONT UseFont
									  , enum order_type order
									  , S_32 (*StepX)(S_32 base1,S_32 delta1,S_32 delta2)
									  , S_32 (*StepY)(S_32 base1,S_32 delta1,S_32 delta2)
									  )
{
	int bit, col, inc;
	int line;
	int width;
	int size;
	PCHARACTER pchar;
	P_8 data;
	P_8 dataline;
	void (*CharPlotAlphax)( ImageFile *pImage, S_32 x, S_32 y, _32 data, CDATA fore, CDATA back );
	_32 (*CharDatax)( _8 *bits, _8 bit );
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	if( !pImage || c > UseFont->characters )
		return 0;
	// real quick -
	if( !UseFont->character[c] )
		InternalRenderFontCharacter( NULL, UseFont, c );

	pchar = UseFont->character[c];
	if( !pchar ) return 0;

	//lprintf( "output %c at %d,%d", c, x, y );

#if defined( _OPENGL_DRIVER ) || defined( _D3D_DRIVER )
	if( !UseFont->character[c]->cell && ( pImage->flags & IF_FLAG_FINAL_RENDER ) )
	{
		Image image = AllocateCharacterSpaceByFont( UseFont, UseFont->character[c] );
		// it's the same characteristics... so we should just pass same step XY
		// oh wait - that's like for lines for sideways stuff... uhmm...should get direction and render 4 bitmaps
		//lprintf( "Render to image this character... %p", image );
		PutCharacterFontX( image, 0, 0, BASE_COLOR_WHITE, 0, c, UseFont, OrderPoints, StepXNormal, StepYNormal );
	}

	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		S_32 xd;
		S_32 yd;
		S_32 yd_back;
		S_32 xd_back;
		S_32 xs = 0;
		S_32 ys = 0;
		Image pifSrc = pchar->cell;
		Image pifSrcReal;
		Image pifDest = pImage;
		switch( order )
		{
		default:
		case OrderPoints:
			xd = x;
			yd = y+(UseFont->baseline - pchar->ascent);
			xd_back = xd;
			yd_back = y;
			break;
		case OrderPointsInvert:
			xd = x;
			yd = y- UseFont->baseline + pchar->ascent;
			xd_back = xd;
			yd_back = y;
			break;
		case OrderPointsVertical:
			xd = x - (UseFont->baseline - pchar->ascent);
			yd = y;
			xd_back = x;
			yd_back = yd;
			break;
		case OrderPointsVerticalInvert:
			xd = x + (UseFont->baseline - pchar->ascent);
			yd = y;
			xd_back = x;
			yd_back = yd;
			break;
		}
		
		for( pifSrcReal = pifSrc; pifSrcReal->pParent; pifSrcReal = pifSrcReal->pParent );
#ifdef _OPENGL_DRIVER
		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->glActiveSurface )
		{
			return 0;
		}
#endif
#ifdef _D3D_DRIVER
		ReloadD3DTexture( pifSrc, 0 );
		if( !pifSrc->pActiveSurface )
		{
			return 0;
		}
#endif
		//lprintf( "use regular texture %p (%d,%d)", pifSrc, pifSrc->width, pifSrc->height );

		/*
		 * only a portion of the image is actually used, the rest is filled with blank space
		 *
		 */
		TranslateCoord( pifDest, &xd, &yd );
		TranslateCoord( pifDest, &xd_back, &yd_back );
		TranslateCoord( pifSrc, &xs, &ys );
		{
			int glDepth = 1;
			float x_size, x_size2, y_size, y_size2;
			VECTOR v[2][4], v2[2][4];
			int vi = 0;
			float texture_v[4][2];
#ifdef PURE_OPENGL2_ENABLED
			float _color[4];
			float _back_color[4];
			_color[0] = RedVal( color ) / 255.0f;
			_color[1] = GreenVal( color ) / 255.0f;
			_color[2] = BlueVal( color ) / 255.0f;
			_color[3] = AlphaVal( color ) / 255.0f;
			_back_color[0] = RedVal( background ) / 255.0f;
			_back_color[1] = GreenVal( background ) / 255.0f;
			_back_color[2] = BlueVal( background ) / 255.0f;
			_back_color[3] = AlphaVal( background ) / 255.0f;
#endif
			switch( order )
			{
			default:
			case OrderPoints:
				v[vi][0][0] = xd;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0f;

				v[vi][1][0] = xd+pchar->cell->real_width;
				v[vi][1][1] = yd;
				v[vi][1][2] = 1.0f;

				v[vi][2][0] = xd;
				v[vi][2][1] = yd+pchar->cell->real_height;
				v[vi][2][2] = 1.0f;

				v[vi][3][0] = xd+pchar->cell->real_width;
				v[vi][3][1] = yd+pchar->cell->real_height;
				v[vi][3][2] = 1.0f;

				v2[vi][0][0] = xd_back;
				v2[vi][0][1] = yd_back;
				v2[vi][0][2] = 1.0;

				v2[vi][1][0] = xd_back + pchar->width;
				v2[vi][1][1] = yd_back;
				v2[vi][1][2] = 1.0;

				v2[vi][2][0] = xd_back;
				v2[vi][2][1] = yd_back + UseFont->height;
				v2[vi][2][2] = 1.0;

				v2[vi][3][0] = xd_back + pchar->width;
				v2[vi][3][1] = yd_back + UseFont->height;
				v2[vi][3][2] = 1.0;

 				break;
			case OrderPointsInvert:
				v[vi][0][0] = xd;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0f;

				v[vi][1][0] = xd - pchar->cell->real_width;
				v[vi][1][1] = yd;
				v[vi][1][2] = 1.0f;

				v[vi][2][0] = xd;
				v[vi][2][1] = yd - pchar->cell->real_height;
				v[vi][2][2] = 1.0f;

				v[vi][3][0] = xd - pchar->cell->real_width;
				v[vi][3][1] = yd - pchar->cell->real_height;
				v[vi][3][2] = 1.0f;

				v2[vi][0][0] = xd_back;
				v2[vi][0][1] = yd_back;
				v2[vi][0][2] = 1.0f;

				v2[vi][1][0] = xd_back - pchar->cell->real_width;
				v2[vi][1][1] = yd_back;
				v2[vi][1][2] = 1.0f;

				v2[vi][2][0] = xd_back;
				v2[vi][2][1] = yd_back - UseFont->height;
				v2[vi][2][2] = 1.0f;

				v2[vi][3][0] = xd_back - pchar->cell->real_width;
				v2[vi][3][1] = yd_back - UseFont->height;
				v2[vi][3][2] = 1.0f;

				break;
			case OrderPointsVertical:
				v[vi][0][0] = xd;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0;

				v[vi][1][0] = xd;
				v[vi][1][1] = yd+pchar->cell->real_width;
				v[vi][1][2] = 1.0;

				v[vi][2][0] = xd-pchar->cell->real_height;
				v[vi][2][1] = yd;
				v[vi][2][2] = 1.0;


				v[vi][3][0] = xd-pchar->cell->real_height;
				v[vi][3][1] = yd+pchar->cell->real_width;
				v[vi][3][2] = 1.0;

				v2[vi][0][0] = xd_back;
				v2[vi][0][1] = yd_back;
				v2[vi][0][2] = 1.0;

				v2[vi][1][0] = xd_back;
				v2[vi][1][1] = yd_back+pchar->cell->real_width;
				v2[vi][1][2] = 1.0;

				v2[vi][2][0] = xd_back-UseFont->height;
				v2[vi][2][1] = yd_back;
				v2[vi][2][2] = 1.0;

				v2[vi][3][0] = xd_back-UseFont->height;
				v2[vi][3][1] = yd_back+pchar->cell->real_width;
				v2[vi][3][2] = 1.0;

				break;
			case OrderPointsVerticalInvert:
				v[vi][0][0] = xd;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0;

				v[vi][1][0] = xd;
				v[vi][1][1] = yd-pchar->cell->real_width;
				v[vi][1][2] = 1.0;

				v[vi][2][0] = xd+pchar->cell->real_height;
				v[vi][2][1] = yd;
				v[vi][2][2] = 1.0;

				v[vi][3][0] = xd+pchar->cell->real_height;
				v[vi][3][1] = yd-pchar->cell->real_width;
				v[vi][3][2] = 1.0;

				v2[vi][0][0] = xd_back;
				v2[vi][0][1] = yd_back;
				v2[vi][0][2] = 1.0;

				v2[vi][1][0] = xd_back;
				v2[vi][1][1] = yd_back-pchar->cell->real_width;
				v2[vi][1][2] = 1.0;

				v2[vi][2][0] = xd_back+UseFont->height;
				v2[vi][2][1] = yd_back;
				v2[vi][2][2] = 1.0;

				v2[vi][3][0] = xd_back+UseFont->height;
				v2[vi][3][1] = yd_back-pchar->cell->real_width;
				v2[vi][3][2] = 1.0;
				break;
			}

			x_size = (double) xs/ (double)pifSrcReal->width;
			x_size2 = (double) (xs+pchar->cell->real_width)/ (double)pifSrcReal->width;
			y_size = (double) ys/ (double)pifSrcReal->height;
			y_size2 = (double) (ys+pchar->cell->real_height)/ (double)pifSrcReal->height;
			texture_v[0][0] = x_size;
			texture_v[0][1] = y_size;
			texture_v[1][0] = x_size2;
			texture_v[1][1] = y_size;
			texture_v[2][0] = x_size;
			texture_v[2][1] = y_size2;
			texture_v[3][0] = x_size2;
			texture_v[3][1] = y_size2;

			//lprintf( "Texture size is %g,%g to %g,%g", x_size, y_size, x_size2, y_size2 );
			while( pifDest && pifDest->pParent )
			{
				glDepth = 0;
				if( pifDest->transform )
				{
					Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
					Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
					Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
					Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
					if( background )
					{
						Apply( pifDest->transform, v2[1-vi][0], v2[vi][0] );
						Apply( pifDest->transform, v2[1-vi][1], v2[vi][1] );
						Apply( pifDest->transform, v2[1-vi][2], v2[vi][2] );
						Apply( pifDest->transform, v2[1-vi][3], v2[vi][3] );
					}
					vi = 1 - vi;
				}
				pifDest = pifDest->pParent;
			}
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
				Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
				Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
				Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
				if( background )
				{
					Apply( pifDest->transform, v2[1-vi][0], v2[vi][0] );
					Apply( pifDest->transform, v2[1-vi][1], v2[vi][1] );
					Apply( pifDest->transform, v2[1-vi][2], v2[vi][2] );
					Apply( pifDest->transform, v2[1-vi][3], v2[vi][3] );
				}
				vi = 1 - vi;
			}

			scale( v[vi][0], v[vi][0], l.scale );
			scale( v[vi][1], v[vi][1], l.scale );
			scale( v[vi][2], v[vi][2], l.scale );
			scale( v[vi][3], v[vi][3], l.scale );
			if( background )
			{
				scale( v2[vi][0], v2[vi][0], l.scale );
				scale( v2[vi][1], v2[vi][1], l.scale );
				scale( v2[vi][2], v2[vi][2], l.scale );
				scale( v2[vi][3], v2[vi][3], l.scale );
			}


#ifdef _OPENGL_DRIVER
#  ifndef PURE_OPENGL2_ENABLED
			if( background )
			{
				glColor4ubv( (GLubyte*)&background );
				glBegin(GL_TRIANGLE_STRIP);
				{
					int n;
					for( n = 0; n < 4; n++ )
					{
						//lprintf( "background vert %g,%d", v2[vi][n][0],v2[vi][n][1] );
						glVertex3fv(v2[vi][n]);
					}
				}
				glEnd();
			}
			glBindTexture(GL_TEXTURE_2D, pifSrc->glActiveSurface);				// Select Our Texture
			glColor4ubv( (GLubyte*)&color );
			glBegin(GL_TRIANGLE_STRIP);
			{
			 	int n;
				for( n = 0; n < 4; n++ )
				{
					//lprintf( "fore vert %g,%d", v[vi][n][0],v[vi][n][1] );
					glTexCoord2f( texture_v[n][0], texture_v[n][1]  ); 
					glVertex3fv(v[vi][n]);	// Bottom Left Of The Texture and Quad
				}
			}
			glEnd();
			glBindTexture(GL_TEXTURE_2D, 0);				// Select Our Texture
#  endif  // ifndef OPENGL2  (OPENGl1?)

#  ifdef PURE_OPENGL2_ENABLED
			if( background )
			{
				EnableShader( GetShader( "Simple Shader", NULL ), v2[vi], _back_color );
				glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
			}

			EnableShader( GetShader( "Simple Shaded Texture", NULL ), v[vi], pifSrc->glActiveSurface, texture_v, _color );
			glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
			// Back Face
#  endif  // ifdef OPENGL2
#endif
#ifdef _D3D_DRIVER

			static LPDIRECT3DVERTEXBUFFER9 pQuadVB;
         if( !pQuadVB )
				g_d3d_device->CreateVertexBuffer(sizeof( D3DTEXTUREDVERTEX )*4,
															D3DUSAGE_WRITEONLY,
															D3DFVF_CUSTOMTEXTUREDVERTEX,
															D3DPOOL_MANAGED,
															&pQuadVB,
															NULL);
			D3DTEXTUREDVERTEX* pData;
			//lock buffer (NEW)
			if( background )
			{
				pQuadVB->Lock(0,0,(void**)&pData,0);
				{
					int n;
					for( n = 0; n < 4; n++ )
					{
						pData[n].dwColor = background;
						pData[n].fX = v2[vi][n][vRight];
						pData[n].fY = v2[vi][n][vUp];
						pData[n].fZ = v2[vi][n][vForward];
					}
				}
				pQuadVB->Unlock();
				g_d3d_device->SetStreamSource(0,pQuadVB,0,sizeof(D3DTEXTUREDVERTEX));
				g_d3d_device->SetFVF( D3DFVF_CUSTOMTEXTUREDVERTEX );
				g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			}
			pQuadVB->Lock(0,0,(void**)&pData,0);
			{
				int n;
				for( n = 0; n < 4; n++ )
				{
					pData[n].dwColor = color;
					pData[n].fX = v[vi][n][vRight];
					pData[n].fY = v[vi][n][vUp];
					pData[n].fZ = v[vi][n][vForward];

					pData[n].fU1 = texture_v[n][vRight];
					pData[n].fV1 = texture_v[n][vUp];
					//lprintf( "point %d  %g,%g,%g   %g,%g  %08x"
					//		 , n
					//		 , pData[n].fX
					//		 , pData[n].fY
					//		 , pData[n].fZ
					//		 , pData[n].fU1
					//		 , pData[n].fV1
					//  	 , pData[n].dwColor );
				}
			}
			//copy data to buffer (NEW)
			//unlock buffer (NEW)
			pQuadVB->Unlock();
			g_d3d_device->SetTexture( 0, pifSrc->pActiveSurface );
			g_d3d_device->SetFVF( D3DFVF_CUSTOMTEXTUREDVERTEX );
			g_d3d_device->SetStreamSource(0,pQuadVB,0,sizeof(D3DTEXTUREDVERTEX));

			//draw quad (NEW)
			g_d3d_device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			g_d3d_device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			g_d3d_device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
			g_d3d_device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
			g_d3d_device->SetTexture( 0, NULL );
			//pQuadVB->Release();
#endif  // _D3D_DRIVER
		}
	}
	else
#endif  // defined ( D3D OR OPENGL )
	{
		// need a physical color buffer attached...
		if( !pImage->image )
			return 0;

		data  = pchar->data;
		// background may have an alpha value -
		// but we should still assume that black is transparent...
		size = pchar->size;
		width = pchar->width;
		if( ( UseFont->flags & 3 ) == FONT_FLAG_MONO )
		{
			CharPlotAlphax = CharPlotAlpha1;
			CharDatax = CharData1;
			inc = (pchar->size+7)/8;
		}
		else if( ( UseFont->flags & 3 ) == FONT_FLAG_2BIT )
		{
			CharPlotAlphax = CharPlotAlpha2;
			CharDatax = CharData2;
			inc = (pchar->size+3)/4;
		}
		else if( ( UseFont->flags & 3 ) == FONT_FLAG_8BIT )
		{
			CharPlotAlphax = CharPlotAlpha8;
			CharDatax = CharData8;
			inc = (pchar->size);
		}
		if( background &0xFFFFFF )
		{
			for( line = 0; line < UseFont->baseline - pchar->ascent; line++ )
			{
				for( col = 0; col < pchar->width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
			}
			for( ; line <= UseFont->baseline - pchar->descent ; line++ )
			{
				dataline = data;
				col = 0;
				for( col = 0; col < pchar->offset; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
				for( bit = 0; bit < size; col++, bit++ )
					CharPlotAlphax( pImage, StepX(x,col,line), StepY(y,line,col), CharDatax( data, bit ), color, background );
				for( ; col < width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col),background );
				data += inc;
			}
			for( ; line < UseFont->height; line++ )
			{
				for( col = 0; col < pchar->width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
			}
		}
		else
		{
			if( 0 )
				lprintf( WIDE("%d %d %d"), UseFont->baseline - pchar->ascent, y, UseFont->baseline - pchar->descent );
			// bias the left edge of the character
#if defined( _D3D_DRIVER ) || defined( _OPENGL_DRIVER )
			for(line = 0;
				 line <= UseFont->baseline - pchar->descent;
				 line++ )
#else
			for(line = UseFont->baseline - pchar->ascent;
				 line <= UseFont->baseline - pchar->descent;
				 line++ )
#endif
			{
				dataline = data;
				col = pchar->offset;
				for( bit = 0; bit < size; col++, bit++ )
				{
					_8 chardata = (_8)CharDatax( data, bit );
					if( chardata )
						CharPlotAlphax( pImage, StepX(x,col,line), StepY(y,line,col)
										  , chardata
										  , color, 0 );
				}
				data += inc;
			}
		}
#if defined( _D3D_DRIVER ) || defined( _OPENGL_DRIVER )
		MarkImageUpdated( pImage );
#endif
	}
	return pchar->width;
}

static _32 _PutCharacterFont( ImageFile *pImage
											  , S_32 x, S_32 y
											  , CDATA color, CDATA background
											  , _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont, OrderPoints
									 , StepXNormal, StepYNormal );
}

static _32 _PutCharacterVerticalFont( ImageFile *pImage
														 , S_32 x, S_32 y
														 , CDATA color, CDATA background
														 , _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									 , OrderPointsVertical, StepXVertical, StepYVertical );
}


static _32 _PutCharacterInvertFont( ImageFile *pImage
													, S_32 x, S_32 y
													, CDATA color, CDATA background
													, _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, OrderPointsInvert, StepXInvert, StepYInvert );
}

static _32 _PutCharacterVerticalInvertFont( ImageFile *pImage
													, S_32 x, S_32 y
													, CDATA color, CDATA background
													, _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, OrderPointsVerticalInvert, StepXInvertVertical, StepYInvertVertical );
}

void PutCharacterFont( ImageFile *pImage
											  , S_32 x, S_32 y
											  , CDATA color, CDATA background
											  , _32 c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsVerticalInvert, StepXNormal, StepYNormal );
}

void PutCharacterVerticalFont( ImageFile *pImage
														 , S_32 x, S_32 y
														 , CDATA color, CDATA background
														 , _32 c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPoints, StepXVertical, StepYVertical );
}


void PutCharacterInvertFont( ImageFile *pImage
														 , S_32 x, S_32 y
														 , CDATA color, CDATA background
														 , _32 c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsInvert, StepXInvert, StepYInvert );
}

void PutCharacterVerticalInvertFont( ImageFile *pImage
														 , S_32 x, S_32 y
														 , CDATA color, CDATA background
															    , _32 c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsVerticalInvert, StepXInvertVertical, StepYInvertVertical );
}


typedef struct string_state_tag
{
	struct {
		_32 bEscape : 1;
		_32 bLiteral : 1;
	} flags;
} STRING_STATE, *PSTRING_STATE;

/*
static void ClearStringState( PSTRING_STATE pss )
{
	pss->flags.bEscape = 0;
}
*/
typedef _32 (*CharPut)(ImageFile *pImage
							 , S_32 x, S_32 y
							 , CDATA fore, CDATA back
							 , char pc
							 , PFONT font );
/*
static _32 FilterMenuStrings( ImageFile *pImage
									 , S_32 *x, S_32 *y
									 , S_32 xdel, S_32 ydel
									 , CDATA fore, CDATA back
									 , char pc, PFONT font
									 , PSTRING_STATE pss
									 , CharPut out)
{
	_32 w;
	if( pc == '&' )
	{
		if( pss->flags.bEscape )
		{
			if( pss->flags.bLiteral )
			{
				if( out )
					w = out( pImage, *x, *y, fore, back, '&', font );
				w |= 0x80000000;
				pss->flags.bLiteral = 0;
			}
			else
			{
				pss->flags.bLiteral = 1;
				pss->flags.bEscape = 0;
			}
		}
		else
			pss->flags.bEscape = 1;
	}
	else
	{
		if( out )
			w = out( pImage, *x, *y, fore, back, pc, font );
		if( pss->flags.bEscape )
		{
			w |= 0x80000000;
			pss->flags.bEscape = 0;
		}
	}
	return w;
}
*/


static LOGICAL Step( CTEXTSTR *pc, size_t *nLen, CDATA *fore_original, CDATA *back_original, CDATA *fore, CDATA *back )
{
	LOGICAL have_char;
	if( !fore_original[0] && !back_original[0] )
	{
		// This serves as a initial condition.  If the colors were 0 and 0, then make them not zero.
		// Anything that's 0 alpha will still be effectively 0.
		if( !( fore_original[0] = fore[0] ) )
			fore_original[0] = 1;
		if( !( back_original[0] = back[0] ) )
			back_original[0] = 0x100;  // use a different value (diagnosing inverse colors)
	}
	else
	{
		// if not the first invoke, move to next character first.
		(*pc)++;
		(*nLen)--;
	}
	if( *(unsigned char*)(*pc) && (*nLen) )
	{
		while( (*(*pc)) == WIDE('\x9F') )
		{
			while( (*(*pc)) && (*(*pc)) != WIDE( '\x9C' ) )
			{
				int code;
				(*pc)++;
				(*nLen)--;
				switch( code = (*pc)[0] )
				{
				case 'r':
				case 'R':
					back[0] = fore_original[0];
					fore[0] = back_original[0];
					break;
				case 'n':
				case 'N':
					fore[0] = fore_original[0];
					back[0] = back_original[0];
					break;

				case 'B':
				case 'b':
				case 'F':
				case 'f':
					(*pc)++;
					(*nLen)--;
					if( (*(*pc)) == '$' )
					{
						_32 accum = 0;
						CTEXTSTR digit;
						digit = (*pc) + 1;
						//lprintf( WIDE("COlor testing: %s"), digit );
						// less than or equal - have to count the $
						while( digit && digit[0] && ( ( digit - (*pc) ) <= 8 ) )
						{
							int n;
							CTEXTSTR p;
							n = 16;
							p = strchr( maxbase1, digit[0] );
							if( p )
								n = (_32)(p-maxbase1);
							else
							{
								p = strchr( maxbase2, digit[0] );
								if( p )
									n = (_32)(p-maxbase2);
							}
							if( n < 16 )
							{
								accum *= 16;
								accum += n;
							}
							else
								break;
							digit++;
						}
						if( ( digit - (*pc) ) < 6 )
						{
							lprintf( WIDE("Perhaps an error in color variable...") );
							accum = accum | 0xFF000000;
						}
						else
						{
							if( ( digit - (*pc) ) == 6 )
								accum = accum | 0xFF000000;
							else
								accum = accum;
						}

						// constants may need reording (OpenGL vs Windows)
						{
							_32 file_color = accum;
							COLOR_CHANNEL a = (COLOR_CHANNEL)( file_color >> 24 ) & 0xFF;
							COLOR_CHANNEL r = (COLOR_CHANNEL)( file_color >> 16 ) & 0xFF;
							COLOR_CHANNEL grn = (COLOR_CHANNEL)( file_color >> 8 ) & 0xFF;
							COLOR_CHANNEL b = (COLOR_CHANNEL)( file_color >> 0 ) & 0xFF;
							accum = AColor( r,grn,b,a );
						}
						//Log4( WIDE("Color is: $%08X/(%d,%d,%d)")
						//		, pce->data[0].Color
						//		, RedVal( pce->data[0].Color ), GreenVal(pce->data[0].Color), BlueVal(pce->data[0].Color) );

						(*nLen) -= ( digit - (*pc) ) - 1;
						(*pc) = digit - 1;

						if( code == 'b' || code == 'B' )
							back[0] = accum;
						else
							fore[0] = accum;
					}
					else
					{
						int n;
						size_t len;
						for( n = 0; n < NUM_COLORS; n++ )
						{
							//lprintf( WIDE("(*pc) %s =%s?"), (*pc)
							//		 , color_defs[n].name );
							if( StrCaseCmpEx( (*pc)
											, color_defs[n].name
											, len = StrLen( color_defs[n].name ) ) == 0 )
							{
								break;
							}
						}
						if( n < NUM_COLORS )
						{
							(*pc) += len-1;
							(*nLen) -= len-1;
							if( code == 'b' || code == 'B' )
								back[0] = color_defs[n].color;
							else if( code == 'f' || code == 'F' )
								fore[0] = color_defs[n].color;
						}
					}
					break;
				}
				/*
			 // handle justification codes...
			{
				int textx = 0, texty = 0, first = 1, lines = 0, line = 0;
				CTEXTSTR p;
				p = (*pc);
				while( p && p[0] )
				{
					lines++;
					p += strlen( p ) + 1;
				}
				while( (*pc) && (*pc)[0] )
				{
					SFTFont font = GetCommonFont( key->button );
					size_t len = StrLen( (*pc) );
					_32 text_width, text_height;
					// handle content formatting.
					(*pc)++;
					GetStringSizeFontEx( (*pc), len-1, &text_width, &text_height, font );
					if( 0 )
						lprintf( WIDE("%d lines %d line  width %d  height %d"), lines, line, text_width, text_height );
					switch( (*pc)[-1] )
					{
					case 'A':
					case 'a':
						textx = ( key->width - text_width ) / 2;
						texty = ( ( key->height - ( text_height * lines ) ) / 2 ) + ( text_height * line );;
						break;
					case 'c':
						textx = ( key->width - text_width ) / 2;
						break;
					case 'C':
						textx = ( key->width - text_width ) / 2;
						if( !first )
							texty += text_height;
						break;
					default:
						break;
					}
					if( 0 )
						lprintf( WIDE("Finally string is %s at %d,%d max %d"), (*pc), textx, texty, key->height );
					PutStringFontEx( surface
										, textx, texty
										, key->flags.bGreyed?BASE_COLOR_WHITE:key->text_color
										, 0
										, (*pc)
										, len
										, font );
					first = 0;
					line++;
					(*pc) += len; // next string if any...
				}
			}
			*/
			}

			// if the string ended...
			if( !(*(*pc)) )
			{
				// this is done.  There's nothing left... command with no data is bad form, but not illegal.
				return FALSE;
			}
			else  // pc is now on the stop command, advance one....
			{
				// this is in a loop, and the next character may be another command....
				(*pc)++;
				(*nLen)--;
			}
		}
		return TRUE;
	}
	else
		return FALSE;
}

void PutStringVerticalFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	_32 _y = y;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	if( !font )
		font = &DEFAULTFONT;
	if( !pImage || !pc ) return;// y;
	while( Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( *(unsigned char*)pc == '\n' )
		{
			x -= font->height;
			y = _y;
		}
		else
			y += _PutCharacterVerticalFont( pImage, x, y, color, background, *(unsigned char*)pc, font );
	}
	return;// y;
}

void PutStringInvertVerticalFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	_32 _y = y;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	if( !font )
		font = &DEFAULTFONT;
	if( !pImage || !pc ) return;// y;
	while( Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( *(unsigned char*)pc == '\n' )
		{
			x += font->height;
			y = _y;
		}
		else
			y -= _PutCharacterVerticalInvertFont( pImage, x, y, color, background, *(unsigned char*)pc, font );
	}
	return;// y;
}

void PutStringFontEx( ImageFile *pImage
											 , S_32 x, S_32 y
											 , CDATA color, CDATA background
											 , CTEXTSTR pc, size_t nLen, PFONT font )
{
	_32 _x = x;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	if( !font )
		font = &DEFAULTFONT;
	if( !pImage || !pc ) return;// x;
	{
		while( Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
		{
			if( *(unsigned char*)pc == '\n' )
			{
				y += font->height;
				x = _x;
			}
			else
				x += _PutCharacterFont( pImage, x, y, color, background, *(unsigned char*)pc, font );
		}
	}
	return;// x;
}

void PutStringInvertFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	_32 _x = x;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	if( !font )
		font = &DEFAULTFONT;
	if( !pImage || !pc ) return;// x;
	while( Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( *(unsigned char*)pc == '\n' )
		{
			y -= font->height;
			x = _x;
		}
		else
			x -= _PutCharacterInvertFont( pImage, x, y, color, background, *(unsigned char*)pc, font );
	}
	return;// x;
}

_32 PutMenuStringFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT UseFont )
{
	int bUnderline;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	bUnderline = FALSE;
	while( Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		int w;
		bUnderline = FALSE;
		if( *(unsigned char*)pc == '&' )
		{
			bUnderline = TRUE;
			pc++;
			if( *(unsigned char*)pc == '&' )
				bUnderline = FALSE;
		}
		if( !(*(unsigned char*)pc) ) // just in case '&' was end of string...
			break;
		w = _PutCharacterFont( pImage, x, y, color, background, *(unsigned char*)pc, UseFont );
		if( bUnderline )
			do_line( pImage, x, y + UseFont->height -1,
								  x + w, y + UseFont->height -1, color );
		x += w;
	}
	return x;
}

 _32  GetMenuStringSizeFontEx ( CTEXTSTR string, size_t len, int *width, int *height, PFONT font )
{
	int _width;
	CDATA tmp1 = 0;
	if( !font )
		font = &DEFAULTFONT;
	if( height )
		*height = font->height;
	if( !width )
		width = &_width;
	*width = 0;
	while( Step( &string, &len, &tmp1, &tmp1, &tmp1, &tmp1 ) )
	{
		if( *(unsigned char*)string == '&' )
		{
			string++;
			len--;
			if( !*(unsigned char*)string )
				break;
		}
		if( !font->character[*(unsigned char*)string] )
			InternalRenderFontCharacter( NULL, font, *(unsigned char*)string );
		*width += font->character[*(unsigned char*)string]->width;
	}
	return *width;
}

 _32  GetMenuStringSizeFont ( CTEXTSTR string, int *width, int *height, PFONT font )
{
	return GetMenuStringSizeFontEx( string, StrLen( string ), width, height, font );
}

 _32  GetMenuStringSizeEx ( CTEXTSTR string, int len, int *width, int *height )
{
	return GetMenuStringSizeFontEx( string, len, width, height, &DEFAULTFONT );
}

 _32  GetMenuStringSize ( CTEXTSTR string, int *width, int *height )
{
	return GetMenuStringSizeFontEx( string, StrLen( string ), width, height, &DEFAULTFONT );
}

 _32  GetStringSizeFontEx ( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, PFONT UseFont )
{
	_32 _width, max_width, _height;
	PCHARACTER *chars;
	CDATA tmp1 = 0;
	if( !pString )
	{
		if( width )
			*width = 0;
		if( height )
					*height = 0;
		return 0;
	}
	if( !UseFont )
		UseFont = &DEFAULTFONT;
		// a character must be rendered before height can be computed.
	if( !UseFont->character[0] )
	{
		InternalRenderFontCharacter(  NULL, UseFont, pString?pString[0]:0 );
	}
	if( !height )
		height = &_height;
	*height = UseFont->height;
	if( !width )
		width = &_width;
	max_width = *width = 0;
	chars = UseFont->character;
	if( pString )
	{
		unsigned int character;
		// color is irrelavent, safe to use a 0 initialized variable...
		// Step serves to do some computation and work to update colors, but also to step application commands
		// application commands should be non-printed.
		while( Step( &pString, &nLen, &tmp1, &tmp1, &tmp1, &tmp1 ) )
		{
			character = (*(unsigned char*)pString) & 0xFF;
			if( *pString == '\n' )
			{
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[character] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, *(unsigned char*)pString );
			}
			if( chars[character] )
				*width += chars[character]->width;
		}
	}
	else
	{
		if( UseFont->character[0] )
			*width = UseFont->character[0]->width;
		else
			*width = 0;
	}

	if( max_width > *width )
		*width = max_width;
	return *width;
}

// used to compute the actual output size
//
 _32  GetStringRenderSizeFontEx ( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, PFONT UseFont )
{
	_32 _width, max_width, _height;
	_32 maxheight = 0;
	CDATA tmp1 = 0;
	PCHARACTER *chars;
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	// a character must be rendered before height can be computed.
	if( !UseFont->character[0] )
	{
		InternalRenderFontCharacter(  NULL, UseFont, pString?pString[0]:0 );
	}
	if( !height )
		height = &_height;
	*height = UseFont->height;
	if( !width )
		width = &_width;
	max_width = *width = 0;
	chars = UseFont->character;
	if( pString )
	{
		while( Step( &pString, &nLen, &tmp1, &tmp1, &tmp1, &tmp1 ) )
		{
			if( *pString == '\n' )
			{
				maxheight = 0;
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[*(unsigned char*)pString] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, *(unsigned char*)pString );
			}
			if( chars[*(unsigned char*)pString] )
			{
				*width += chars[*(unsigned char*)pString]->width;
				if( SUS_GT( (UseFont->baseline - chars[*(unsigned char*)pString]->descent ),S_32,maxheight,_32) )
					maxheight = UseFont->baseline - chars[*(unsigned char*)pString]->descent;
			}
		}
		if( charheight )
			(*charheight) = (*height);
		(*height) += maxheight - UseFont->height;
	}
	else
	{
		if( UseFont->character[0] )
			*width = UseFont->character[0]->width;
		else
			*width = 0;
	}

	if( max_width > *width )
		*width = max_width;
	return *width;
}


 _32  GetMaxStringLengthFont ( _32 width, PFONT UseFont )
{
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	if( width > 0 )
		// character 0 should contain the average width of a character or is it max?
		return width/UseFont->character[0]->width; 
	return 0;
}  

 _32  GetFontHeight ( PFONT font )
{
	if( font )
	{
		//lprintf( WIDE("Resulting with %d height"), font->height );
		return font->height;
	}
	return DEFAULTFONT.height;
}

PRELOAD( InitColorDefaults )
{
		color_defs[0].color = BASE_COLOR_BLACK;
		color_defs[0].name = WIDE("black");
		color_defs[1].color = BASE_COLOR_BLUE;
		color_defs[1].name = WIDE("blue");
		color_defs[2].color = BASE_COLOR_GREEN;
		color_defs[2].name = WIDE("green");
		color_defs[3].color = BASE_COLOR_RED;
		color_defs[3].name = WIDE("red");		
}

IMAGE_NAMESPACE_END

