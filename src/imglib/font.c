/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle usage of 'SFTFont's on 'Image's.
 * 
 *  *  consult doc/image.html
 *
 */

#define FORCE_COLOR_MACROS
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#define IMAGE_LIBRARY_SOURCE
#include <imglib/fontstruct.h>
#include <imglib/imagestruct.h>
#include <image.h>

#ifdef __3D__
#ifdef _OPENGL_DRIVER
#  if defined( USE_GLES )
#    include <GLES/gl.h>
#  elif defined( USE_GLES2 )
#    include <GLES2/gl2.h>
#  else
#   include <GL/glew.h>
#   include <GL/gl.h>         // Header File For The OpenGL32 Library
#  endif
#endif
#ifdef PURE_OPENGL2_ENABLED
#include "puregl2/local.h"
#elif defined( _VULKAN_DRIVER )
#include "vulkan/local.h"
#elif defined( _D3D11_DRIVER )
#include "pured3d11/local.h"
#elif defined( _D3D10_DRIVER )
#include "pured3d10/local.h"
#elif defined( _D3D2_DRIVER )
#include "pured3d2/local.h"
#elif defined( _D3D_DRIVER )
#include "pured3d/local.h"
#else
#include "puregl/local.h"
#endif
#else
#include "local.h"
#endif

#define REQUIRE_GLUINT
#include "image_common.h"

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
}color_defs[21];

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

#if __3D__
#define CharPlotAlpha(pRealImage,x,y,color) ( pRealImage->reverse_interface? pRealImage->reverse_interface->_plotalpha( (Image)pRealImage->reverse_interface_instance, x, y, color ) : plot( pRealImage, x, y, color ) )
#else
#define CharPlotAlpha(pRealImage,x,y,color) ( pRealImage->reverse_interface? pRealImage->reverse_interface->_plotalpha( (Image)pRealImage->reverse_interface_instance, x, y, color ) : plotalpha( pRealImage, x, y, color ) )
#endif

//---------------------------------------------------------------------------
void CharPlotAlpha8( Image pRealImage, int32_t x, int32_t y, uint32_t data, CDATA fore, CDATA back )
{
	if( back )
		CharPlotAlpha( pRealImage, x, y, back );
	if( data )
		CharPlotAlpha( pRealImage, x, y, ( fore & 0xFFFFFF ) | ( (((uint32_t)data*AlphaVal(fore))&0xFF00) << 16 ) );
}
uint32_t CharData8( uint8_t *bits, uint8_t bit )
{
	return bits[bit];
}
//---------------------------------------------------------------------------
void CharPlotAlpha2( Image pRealImage, int32_t x, int32_t y, uint32_t data, CDATA fore, CDATA back )
{
	if( back )
		CharPlotAlpha( pRealImage, x, y, back );
	if( data )
	{
		CharPlotAlpha( pRealImage, x, y, ( fore & 0xFFFFFF ) | ( (((uint32_t)((data&3)+1)*(AlphaVal(fore)))&0x03FC) << 22 ) );
	}
}
uint32_t CharData2( uint8_t *bits, uint8_t bit )
{
	return (bits[bit>>2] >> (2*(bit&3)))&3;
}
//---------------------------------------------------------------------------
void CharPlotAlpha1( Image pRealImage, int32_t x, int32_t y, uint32_t data, CDATA fore, CDATA back )
{
	if( data )
		CharPlotAlpha( pRealImage, x, y, fore );
	else if( back )
		CharPlotAlpha( pRealImage, x, y, back );
}
uint32_t CharData1( uint8_t *bits, uint8_t bit )
{
	return (bits[bit>>3] >> (bit&7))&1;
}
//---------------------------------------------------------------------------

int32_t StepXNormal(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1+delta1;
}
int32_t StepYNormal(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1+delta1;
}
//---------------------------------------------------------------------------

int32_t StepXVertical(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1-delta2;
}
int32_t StepYVertical(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1+delta2;
}
//---------------------------------------------------------------------------

int32_t StepXInvert(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1-delta1;
}
int32_t StepYInvert(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1-delta1;
}
//---------------------------------------------------------------------------

int32_t StepXInvertVertical(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1+delta2;
}
int32_t StepYInvertVertical(int32_t base1,int32_t delta1,int32_t delta2)
{
	return base1-delta2;
}
//---------------------------------------------------------------------------

enum order_type {
	OrderPoints,OrderPointsVertical,OrderPointsInvert,OrderPointsVerticalInvert
};

static uint32_t PutCharacterFontX ( ImageFile *pImage
                             , int32_t x, int32_t y
                             , CDATA color, CDATA background
                             , uint32_t c, PFONT UseFont
                             , enum order_type order
                             , int32_t (*StepX)(int32_t base1,int32_t delta1,int32_t delta2)
                             , int32_t (*StepY)(int32_t base1,int32_t delta1,int32_t delta2)
                             , LOGICAL internal_render
                             )
{
	int bit, col, inc;
	int line;
	int width;
	int size;
	PCHARACTER pchar;
	uint8_t* data;
	uint8_t* dataline;
	void (*CharPlotAlphax)( Image pRealImage, int32_t x, int32_t y, uint32_t data, CDATA fore, CDATA back );
	uint32_t (*CharDatax)( uint8_t *bits, uint8_t bit );
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	if( !pImage || c > UseFont->characters )
		return 0;
	// real quick -
	if( !UseFont->character[c] )
		InternalRenderFontCharacter( NULL, UseFont, c );

	pchar = UseFont->character[c];
	if( !pchar ) return 0;

	if( !UseFont->character[c]->cell 
		&& ( pImage->flags & IF_FLAG_FINAL_RENDER )
		&& !( pImage->flags & IF_FLAG_IN_MEMORY ) )
	{
		Image image = AllocateCharacterSpaceByFont( pImage, UseFont, UseFont->character[c] );
		// it's the same characteristics... so we should just pass same step XY
		// oh wait - that's like for lines for sideways stuff... uhmm...should get direction and render 4 bitmaps
		//lprintf( "Render to image this character... %p", image );
		if( pImage->reverse_interface )
			PutCharacterFontX( pImage->reverse_interface->_GetNativeImage( image ), 0, 0, BASE_COLOR_WHITE, 0, c, UseFont, OrderPoints, StepXNormal, StepYNormal, TRUE );
		else
			PutCharacterFontX( image, 0, 0, BASE_COLOR_WHITE, 0, c, UseFont, OrderPoints, StepXNormal, StepYNormal, TRUE );
	}

	if( ( pImage->flags & IF_FLAG_FINAL_RENDER )
		&& !( pImage->flags & IF_FLAG_IN_MEMORY ) )
	{
		int orientation = 0;
		int32_t xd;
		int32_t yd;
		int32_t yd_back;
		int32_t xd_back;
		int32_t xs = 0;
		int32_t ys = 0;
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
			orientation = BLOT_ORIENT_INVERT;
			xd = x;
			yd = y- UseFont->baseline + pchar->ascent;
			xd_back = xd;
			yd_back = y;
			break;
		case OrderPointsVertical:
			orientation = BLOT_ORIENT_VERTICAL;
			xd = x - (UseFont->baseline - pchar->ascent);
			yd = y;
			xd_back = x;
			yd_back = yd;
			break;
		case OrderPointsVerticalInvert:
			orientation = BLOT_ORIENT_VERTICAL_INVERT;
			xd = x + (UseFont->baseline - pchar->ascent);
			yd = y;
			xd_back = x;
			yd_back = yd;
			break;
		}
#if !defined( __3D__ )

		if( pImage->reverse_interface && !(pImage->flags & IF_FLAG_HAS_PUTSTRING ) )
		{
			if( background )
				pImage->reverse_interface->_BlatColorAlpha( (Image)pImage->reverse_interface_instance, xd_back, yd_back, pchar->width, UseFont->height, background );
			pImage->reverse_interface->_BlotImageSizedEx( (Image)pImage->reverse_interface_instance, pifSrc, xd, yd, xs, ys, pchar->cell->real_width, pchar->cell->real_height, TRUE, BLOT_SHADED|orientation, color );
		}
		else
		{
			if( background )
				BlatColorAlpha( pImage, xd, yd, pchar->width, UseFont->height, background );
			BlotImageSizedEx( pImage, pifSrc, xd, yd, xs, ys, pchar->cell->real_width, pchar->cell->real_height, TRUE, BLOT_SHADED|orientation, color );
		}

#else

		for( pifSrcReal = pifSrc; pifSrcReal->pParent; pifSrcReal = pifSrcReal->pParent );

#ifdef _OPENGL_DRIVER
		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->glActiveSurface )
		{
			return 0;
		}
#endif
#if defined( _D3D_DRIVER ) || defined( _D3D10_DRIVER ) || defined( _D3D11_DRIVER )
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
			{
				struct image_shader_op *op;
				if( background )
				{
					op = BeginImageShaderOp( GetShader( WIDE("Simple Shader") ), pifDest, _back_color );
					AppendImageShaderOpTristrip( op, 2, v2[vi] );
				}

				op = BeginImageShaderOp( GetShader( WIDE("Simple Shaded Texture") ), pifDest, pifSrc->glActiveSurface, _color  );
				AppendImageShaderOpTristrip( op, 2, v[vi], texture_v );
			}
			// Back Face
#  endif  // ifdef OPENGL2
#endif
#if defined( _D3D_DRIVER ) || defined( _D3D11_DRIVER )
#  ifdef _D3D2_DRIVER
			static LPDIRECT3DVERTEXBUFFER9 pQuadVB_back;
			static LPDIRECT3DVERTEXBUFFER9 pQuadVB;
			if( !pQuadVB_back )
				g_d3d_device->CreateVertexBuffer(sizeof( D3DPOSVERTEX )*4,
															D3DUSAGE_WRITEONLY,
															D3DFVF_XYZ,
															D3DPOOL_MANAGED,
															&pQuadVB_back,
															NULL);
			if( !pQuadVB )
				g_d3d_device->CreateVertexBuffer(sizeof( D3DTEXTUREDVERTEX )*4,
															D3DUSAGE_WRITEONLY,
															D3DFVF_CUSTOMTEXTUREDVERTEX,
															D3DPOOL_MANAGED,
															&pQuadVB,
															NULL);
			D3DPOSVERTEX* pData_back;
			D3DTEXTUREDVERTEX* pData;
			//lock buffer (NEW)
			if( background )
			{
				pQuadVB->Lock(0,0,(void**)&pData_back,0);
				{
					int n;
					for( n = 0; n < 4; n++ )
					{
						pData_back[n].fX = v2[vi][n][vRight];
						pData_back[n].fY = v2[vi][n][vUp];
						pData_back[n].fZ = v2[vi][n][vForward];
					}
				}
				pQuadVB_back->Unlock();
				float _color[4];
				_color[0] = RedVal( background ) / 255.0f;
				_color[1] = GreenVal( background ) / 255.0f;
				_color[2] = BlueVal( background ) / 255.0f;
				_color[3] = AlphaVal( background ) / 255.0f;
				EnableShader( l.simple_shader, pQuadVB_back, _color );
				g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			}
			pQuadVB->Lock(0,0,(void**)&pData,0);
			{
				int n;
				for( n = 0; n < 4; n++ )
				{
					pData[n].fX = v[vi][n][vRight];
					pData[n].fY = v[vi][n][vUp];
					pData[n].fZ = v[vi][n][vForward];

					pData[n].fU1 = texture_v[n][vRight];
					pData[n].fV1 = texture_v[n][vUp];
					//lprintf( WIDE("point %d  %g,%g,%g   %g,%g  %08x")
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
			float _color[4];
			_color[0] = RedVal( color ) / 255.0f;
			_color[1] = GreenVal( color ) / 255.0f;
			_color[2] = BlueVal( color ) / 255.0f;
			_color[3] = AlphaVal( color ) / 255.0f;
			EnableShader( l.simple_texture_shader, pQuadVB_back, pifSrc, _color );

			g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			//pQuadVB->Release();
#  elif defined( _D3D10_DRIVER )
#  elif defined( _D3D11_DRIVER )
			D3DPOSVERTEX* pData_back;
			D3DTEXTUREDVERTEX* pData;
			static ID3D11Buffer *pQuadVB_back;
			static ID3D11Buffer *pQuadVB;
			if( !pQuadVB_back )
			{
				D3D11_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
				bufferDesc.ByteWidth        = sizeof( D3DPOSVERTEX ) * 4;
				bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
				bufferDesc.MiscFlags        = 0;
				bufferDesc.StructureByteStride = sizeof( D3DPOSVERTEX );
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB_back);
			}
			if( !pQuadVB )
			{
				D3D11_BUFFER_DESC bufferDesc;
				bufferDesc.Usage            = D3D11_USAGE_DYNAMIC;
				bufferDesc.ByteWidth        = sizeof( D3DTEXTUREDVERTEX ) * 4;
				bufferDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
				bufferDesc.MiscFlags        = 0;
				bufferDesc.StructureByteStride = sizeof( D3DTEXTUREDVERTEX );
	
				g_d3d_device->CreateBuffer( &bufferDesc, NULL/*&InitData*/, &pQuadVB);
			}
			D3D11_MAPPED_SUBRESOURCE resource;
			g_d3d_device_context->Map( pQuadVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
			pData = (D3DTEXTUREDVERTEX*)resource.pData;

			g_d3d_device_context->Map( pQuadVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
			pData_back = (D3DPOSVERTEX*)resource.pData;

			unsigned int stride = sizeof( pData[0] );
			unsigned int offset = 0;
			float _color[4];
			if( background )
			{
				_color[0] = RedVal( background ) / 255.0f;
				_color[1] = GreenVal( background ) / 255.0f;
				_color[2] = BlueVal( background ) / 255.0f;
				_color[3] = AlphaVal( background ) / 255.0f;
				EnableShader( l.simple_shader, _color );
			}
			_color[0] = RedVal( color ) / 255.0f;
			_color[1] = GreenVal( color ) / 255.0f;
			_color[2] = BlueVal( color ) / 255.0f;
			_color[3] = AlphaVal( color ) / 255.0f;
			
			EnableShader( l.simple_texture_shader, pifSrc, _color );

			g_d3d_device_context->IASetVertexBuffers(0, 1, &pQuadVB, &stride, &offset);
			g_d3d_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			g_d3d_device_context->Draw( 4, 0 );
#  else
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
					//lprintf( WIDE("point %d  %g,%g,%g   %g,%g  %08x")
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
#  endif // _D3D11_DRIVER elseif
#endif // __OPENGLDriver__ if/else
		}
#endif // __3D__
	}
	else
	{
		// need a physical color buffer attached...
		if( !pImage->image )
			return 0;

		data  = pchar->data;
		// background may have an alpha value -
		// but we should still assume that black is transparent...
		size = pchar->size;
		width = pchar->width;
		if( ( pchar->render_flags & 3 ) == FONT_FLAG_MONO )
		{
			CharPlotAlphax = CharPlotAlpha1;
			CharDatax = CharData1;
			inc = (pchar->size+7)/8;
		}
		else if( (pchar->render_flags & 3 ) == FONT_FLAG_2BIT )
		{
			CharPlotAlphax = CharPlotAlpha2;
			CharDatax = CharData2;
			inc = (pchar->size+3)/4;
		}
		else if( (pchar->render_flags & 3 ) == FONT_FLAG_8BIT /*|| ( UseFont->flags & 3 )  == FONT_FLAG_8BIT*/ )
		{
			CharPlotAlphax = CharPlotAlpha8;
			CharDatax = CharData8;
			inc = (pchar->size);
		}
		//lprintf( "Output Character %c %d %d", c, pchar->ascent, pchar->descent );
		if( background /*&0xFFFFFFFF*/ )
		{
			for( line = 0; line < ((int16_t)UseFont->baseline - pchar->ascent); line++ )
			{
				for( col = 0; col < pchar->width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
			}
			for(; line <= ((int16_t)UseFont->baseline - pchar->descent); line++ )
			{
				dataline = data;
				col = 0;
				for( col = 0; col < pchar->offset; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
				for( bit = 0; bit < size; col++, bit++ )
					CharPlotAlphax( pImage, StepX(x,col,line), StepY(y,line,col), CharDatax( data, bit ), color, background );
				for(; col < width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col),background );
				data += inc;
			}
			for(; line < UseFont->height; line++ )
			{
				for( col = 0; col < pchar->width; col++ )
					CharPlotAlpha( pImage, StepX(x,col,line), StepY(y,line,col), background );
			}
		}
		else
		{
			int line_target;
			//if( 0 )
			//	lprintf( WIDE("%d  %d %d %d %d %d"), UseFont->baseline, UseFont->baseline - pchar->ascent, y, UseFont->baseline - pchar->descent, pchar->descent, pchar->ascent );
			// bias the left edge of the character

			if( internal_render )
			{
				//y = 0;
				line = 0;
				line_target = ( pchar->ascent - pchar->descent );
			}
			else
			{
				y += ( UseFont->baseline - pchar->ascent );
				line = 0;//UseFont->baseline - pchar->ascent;
				line_target =  pchar->ascent - pchar->descent;//(UseFont->baseline - pchar->ascent) + ( UseFont->baseline - pchar->descent );
			}
			for(;
				 line <= line_target;
				 line++ )
			{
				dataline = data;
				col = pchar->offset;
				for( bit = 0; bit < size; col++, bit++ )
				{
					uint8_t chardata = (uint8_t)CharDatax( data, bit );
					if( chardata )
						CharPlotAlphax( pImage, StepX(x,col,line), StepY(y,line,col)
										  , chardata
										  , color, 0 );
				}
				data += inc;
			}
		}
		//lprintf( "Set dirty on image...." );
		if( pImage->reverse_interface )
			pImage->reverse_interface->_MarkImageDirty( (Image)pImage->reverse_interface_instance );
		else
			MarkImageUpdated( pImage );
	}
	return pchar->width;
}

static uint32_t _PutCharacterFont( ImageFile *pImage
											  , int32_t x, int32_t y
											  , CDATA color, CDATA background
											  , uint32_t c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont, OrderPoints
									 , StepXNormal, StepYNormal, FALSE );
}

static uint32_t _PutCharacterVerticalFont( ImageFile *pImage
														 , int32_t x, int32_t y
														 , CDATA color, CDATA background
														 , uint32_t c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									 , OrderPointsVertical, StepXVertical, StepYVertical, FALSE );
}


static uint32_t _PutCharacterInvertFont( ImageFile *pImage
													, int32_t x, int32_t y
													, CDATA color, CDATA background
													, uint32_t c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, OrderPointsInvert, StepXInvert, StepYInvert, FALSE );
}

static uint32_t _PutCharacterVerticalInvertFont( ImageFile *pImage
													, int32_t x, int32_t y
													, CDATA color, CDATA background
													, uint32_t c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, OrderPointsVerticalInvert, StepXInvertVertical, StepYInvertVertical, FALSE );
}

void PutCharacterFont( ImageFile *pImage
											  , int32_t x, int32_t y
											  , CDATA color, CDATA background
											  , TEXTCHAR c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsVerticalInvert, StepXNormal, StepYNormal, FALSE );
}

void PutCharacterVerticalFont( ImageFile *pImage
                             , int32_t x, int32_t y
                             , CDATA color, CDATA background
                             , TEXTCHAR c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPoints, StepXVertical, StepYVertical, FALSE );
}


void PutCharacterInvertFont( ImageFile *pImage
                           , int32_t x, int32_t y
                           , CDATA color, CDATA background
                           , TEXTCHAR c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsInvert, StepXInvert, StepYInvert, FALSE );
}

void PutCharacterVerticalInvertFont( ImageFile *pImage
                                   , int32_t x, int32_t y
                                   , CDATA color, CDATA background
                                   , TEXTCHAR c, PFONT UseFont )
{
	PutCharacterFontX( pImage, x, y, color, background, c, UseFont
						  , OrderPointsVerticalInvert, StepXInvertVertical, StepYInvertVertical, FALSE );
}


typedef struct string_state_tag
{
	struct {
		uint32_t bEscape : 1;
		uint32_t bLiteral : 1;
	} flags;
} STRING_STATE, *PSTRING_STATE;

/*
static void ClearStringState( PSTRING_STATE pss )
{
	pss->flags.bEscape = 0;
}
*/
typedef uint32_t (*CharPut)(ImageFile *pImage
							 , int32_t x, int32_t y
							 , CDATA fore, CDATA back
							 , char pc
							 , PFONT font );
/*
static uint32_t FilterMenuStrings( ImageFile *pImage
									 , int32_t *x, int32_t *y
									 , int32_t xdel, int32_t ydel
									 , CDATA fore, CDATA back
									 , char pc, PFONT font
									 , PSTRING_STATE pss
									 , CharPut out)
{
	uint32_t w;
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


// Return the integer character from the string
// using utf-8 or utf-16 decoding appropriately.  No more extended-ascii.

static int Step( CTEXTSTR *pc, size_t *nLen, CDATA *fore_original, CDATA *back_original, CDATA *fore, CDATA *back )
{
	CTEXTSTR _pc = (*pc);
	int ch;
	//lprintf( "Step (%s[%*.*s])", (*pc), nLen,nLen, (*pc) );
	if( nLen && !*nLen )
		return 0;

	ch = GetUtfChar( pc );
	if( ch & 0xFFE00000 )
		DebugBreak();
	if( nLen )
		(*nLen) -= (*pc) - _pc;
	_pc = (*pc);

	if( fore_original && back_original && !fore_original[0] && !back_original[0] )
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
		//(*pc)++;
		//(*nLen)--;
	}
	if( ch )
	{
		while( ch == (unsigned char)'\x9F' )
		{
			// use a long unique code for color sequencing...
         // so we can encode other things like images..
#define PREFIX WIDE("org.d3x0r.sack.image:color")
  			if( StrCmp( (*pc), PREFIX ) != 0 )
			{
				while( ch && ( ch != (unsigned char)'\x9C' ) )
				{
					ch = GetUtfChar( pc );
					if( nLen )
						(*nLen) -= (*pc) - _pc;
					_pc = (*pc);
				}
			}
			else
			{
				(*pc) += 26;

					ch = GetUtfChar( pc );
					if( nLen )
						(*nLen) -= (*pc) - _pc;
					_pc = (*pc);

			while( ch && ( ch != WIDE( '\x9C' ) ) )
			{
				int code;
				ch = GetUtfChar( pc );
				(*nLen) -= (*pc) - _pc;
				_pc = (*pc);
				if( fore_original && back_original && fore && back )
				switch( code = ch )
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
					ch = GetUtfChar( pc );
					(*nLen) -= (*pc) - _pc;
					_pc = (*pc);
					if( ch == '$' )
					{
						uint32_t accum = 0;
						CTEXTSTR digit = (*pc);
						ch = GetUtfChar( &digit );
						//lprintf( WIDE("COlor testing: %s"), digit );
						// less than or equal - have to count the $
						while( digit && digit[0] && ( ( digit - (*pc) ) <= 8 ) )
						{
							int n;
							CTEXTSTR p;
							n = 16;
							p = strchr( maxbase1, ch );
							if( p )
								n = (uint32_t)(p-maxbase1);
							else
							{
								p = strchr( maxbase2, ch );
								if( p )
									n = (uint32_t)(p-maxbase2);
							}
							if( n < 16 )
							{
								accum *= 16;
								accum += n;
							}
							else
								break;
							ch = GetUtfChar( &digit );
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
							uint32_t file_color = accum;
							COLOR_CHANNEL a = (COLOR_CHANNEL)( file_color >> 24 ) & 0xFF;
							COLOR_CHANNEL r = (COLOR_CHANNEL)( file_color >> 16 ) & 0xFF;
							COLOR_CHANNEL grn = (COLOR_CHANNEL)( file_color >> 8 ) & 0xFF;
							COLOR_CHANNEL b = (COLOR_CHANNEL)( file_color >> 0 ) & 0xFF;
							accum = AColor( r,grn,b,a );
						}
						//Log4( WIDE("Color is: $%08X/(%d,%d,%d)")
						//		, pce->data[0].Color
						//		, RedVal( pce->data[0].Color ), GreenVal(pce->data[0].Color), BlueVal(pce->data[0].Color) );

						//(*nLen) -= ( digit - (*pc) ) - 1;
						//(*pc) = digit - 1;

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
					uint32_t text_width, text_height;
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
			}

			// if the string ended...
			if( !ch )
			{
				// this is done.  There's nothing left... command with no data is bad form, but not illegal.
				return FALSE;
			}
			else  // pc is now on the stop command, advance one....
			{
				// this is in a loop, and the next character may be another command....
				ch = GetUtfChar( pc );
				if( nLen )
					(*nLen) -= (*pc) - _pc;
				_pc = (*pc);
			}
		}
	}
	return ch;
}

void PutStringVerticalFontEx( ImageFile *pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	uint32_t _y = y;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	int ch;
	int32_t bias_x;
	int32_t bias_y;
	if( !font )
		font = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( font->bias & 0xF );
		bias_y = ( ( font->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	if( !pImage || !pc ) return;// y;
	while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( ch == '\n' )
		{
			x -= font->height;
			y = _y;
		}
		else
			y += _PutCharacterVerticalFont( pImage, x, y, color, background, ch, font );
	}
	return;// y;
}

void PutStringInvertVerticalFontEx( ImageFile *pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	uint32_t _y = y;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	int ch;
	int32_t bias_x;
	int32_t bias_y;
	if( !font )
		font = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( font->bias & 0xF );
		bias_y = ( ( font->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	if( !pImage || !pc ) return;// y;
	while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( ch == '\n' )
		{
			x += font->height;
			y = _y;
		}
		else
			y -= _PutCharacterVerticalInvertFont( pImage, x, y, color, background, ch, font );
	}
	return;// y;
}

void PutStringFontEx( ImageFile *pImage
											 , int32_t x, int32_t y
											 , CDATA color, CDATA background
											 , CTEXTSTR pc, size_t nLen, PFONT font )
{
	uint32_t _x = x;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	int ch;
	int32_t bias_x;
	int32_t bias_y;
	if( !font )
		font = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( font->bias & 0xF );
		bias_y = ( ( font->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	_x = x;
	if( !pImage || !pc ) return;// x;
	{
		while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
		{
			if( ch == '\n' )
			{
				y += font->height;
				x = _x;
			}
			else
				x += _PutCharacterFont( pImage, x, y, color, background, ch, font );
		}
	}
	return;// x;
}

void PutStringFontExx( ImageFile *pImage
											 , int32_t x, int32_t y
											 , CDATA color, CDATA background
											 , CTEXTSTR pc, size_t nLen, PFONT font, int justification, uint32_t _width )
{
	uint32_t length = 0;
	uint32_t _x = x;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	CTEXTSTR start = pc;
	int32_t bias_x;
	int32_t bias_y;
	if( !font )
		font = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( font->bias & 0xF );
		bias_y = ( ( font->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	_x = x;
	if( !pImage || !pc ) return;// x;
	{
		if( justification )
		{
			int ch;
			start = pc;
			while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
			{
				PCHARACTER pchar;
				pchar = font->character[ ch ];
				if( ch == '\n' )
				{
					if( justification == 2 )				
						x = ( _x + _width ) - length;
					else
						x = ( ( _x + _width ) - length ) / 2;
					while( start != pc )
					{
						int ch2 = GetUtfChar( &start );
						x += _PutCharacterFont( pImage, x, y, color, background, ch2, font );
					}
					start++;
					y += font->height;
					length = 0;
				}
				else
				{
					length += pchar->width;
				}
			}
		}
		else
		{
			int ch;
			while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
			{
				if( ch == '\n' )
				{
					y += font->height;
					x = _x;
				}
				else
					x += _PutCharacterFont( pImage, x, y, color, background, ch, font );
			}
		}
	}
	return;// x;
}

void PutStringInvertFontEx( ImageFile *pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
	uint32_t _x = x;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	int ch;
	int32_t bias_x;
	int32_t bias_y;
	if( !font )
		font = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( font->bias & 0xF );
		bias_y = ( ( font->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	_x = x;
	if( !pImage || !pc ) return;// x;
	while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		if( ch == '\n' )
		{
			y -= font->height;
			x = _x;
		}
		else
			x -= _PutCharacterInvertFont( pImage, x, y, color, background, ch, font );
	}
	return;// x;
}

uint32_t PutMenuStringFontEx( ImageFile *pImage, int32_t x, int32_t y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT UseFont )
{
	int bUnderline;
	CDATA tmp1 = 0;
	CDATA tmp2 = 0;
	int32_t bias_x;
	int32_t bias_y;
	int ch;
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		bias_x = ( UseFont->bias & 0xF );
		bias_y = ( ( UseFont->bias >> 4 ) & 0xF );
	}
	else
		bias_x = bias_y = 0;
	if( bias_x & 0x8 ) bias_x |= 0xFFFFFFF0;
	if( bias_y & 0x8 ) bias_y |= 0xFFFFFFF0;
	x += bias_x;
	y += bias_y;
	bUnderline = FALSE;
	while( ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background ) )
	{
		int w;
		bUnderline = FALSE;
		if( ch == '&' )
		{
			bUnderline = TRUE;
			ch = Step( &pc, &nLen, &tmp1, &tmp2, &color, &background );
			if( ch == '&' )
				bUnderline = FALSE;
		}
		if( !ch ) // just in case '&' was end of string...
			break;
		w = _PutCharacterFont( pImage, x, y, color, background, ch, UseFont );
		if( bUnderline )
			do_line( pImage, x, y + UseFont->height -1,
								  x + w, y + UseFont->height -1, color );
		x += w;
	}
	return x;
}

 uint32_t  GetMenuStringSizeFontEx ( CTEXTSTR string, size_t len, int *width, int *height, PFONT font )
{
	int _width;
	CDATA tmp1 = 0;
	int ch;
	if( !font )
		font = &DEFAULTFONT;
	if( height )
		*height = font->height;
	if( !width )
		width = &_width;
	*width = 0;
	while( ch = Step( &string, &len, &tmp1, &tmp1, &tmp1, &tmp1 ) )
	{
		if( ch == '&' )
		{
			ch = Step( &string, &len, &tmp1, &tmp1, &tmp1, &tmp1 );
			if( !ch )
				break;
		}
		if( !font->character[ch] )
			InternalRenderFontCharacter( NULL, font, ch );
		*width += font->character[ch]->width;
	}
	return *width;
}

 uint32_t  GetMenuStringSizeFont ( CTEXTSTR string, int *width, int *height, PFONT font )
{
	return GetMenuStringSizeFontEx( string, StrLen( string ), width, height, font );
}

 uint32_t  GetMenuStringSizeEx ( CTEXTSTR string, int len, int *width, int *height )
{
	return GetMenuStringSizeFontEx( string, len, width, height, &DEFAULTFONT );
}

 uint32_t  GetMenuStringSize ( CTEXTSTR string, int *width, int *height )
{
	return GetMenuStringSizeFontEx( string, StrLen( string ), width, height, &DEFAULTFONT );
}

 uint32_t  GetStringSizeFontEx ( CTEXTSTR pString, size_t nLen, uint32_t *width, uint32_t *height, PFONT UseFont )
{
	uint32_t _width, max_width, _height;
	PCHARACTER *chars;
	CDATA tmp1 = 0;
	if( !pString || !pString[0] )
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
	if( !width )
		width = &_width;
	max_width = *width = 0;
	if( !height )
		height = &_height;
	// default to one line of height...
	// later, if there's no characters to make width, this resets to 0
	// otherwise, height increments with newlines (adding a line is
	// another height... so starting with 1 is appropriate.
	*height = UseFont->height;
	chars = UseFont->character;
	if( pString )
	{
		unsigned int character;
		// color is irrelavent, safe to use a 0 initialized variable...
		// Step serves to do some computation and work to update colors, but also to step application commands
		// application commands should be non-printed.
		while( character = Step( &pString, &nLen, &tmp1, &tmp1, &tmp1, &tmp1 ) )
		{
			if( ( nLen & ( (size_t)1 << ( sizeof( nLen ) * CHAR_BIT - 1 ) ) ) )
				break;
			if( character == '\n' )
			{
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[character] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, character );
				// the NUL character may not have a height associated with it...
				// so keep trying to set intial height as new characters are added.
				if( !(*height) )
					(*height) = UseFont->height;
			}
			if( ( character < UseFont->characters ) && chars[character] )
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
	//if( !max_width && !(*width) )
	//	(*height) = 0; // zero length is also zero height.
	if( max_width > *width )
		*width = max_width;
	return *width;
}

// used to compute the actual output size
//
 uint32_t  GetStringRenderSizeFontEx ( CTEXTSTR pString, size_t nLen, uint32_t *width, uint32_t *height, uint32_t *charheight, PFONT UseFont )
{
	uint32_t _width, max_width, _height;
	uint32_t maxheight = 0;
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
		int ch;
		while( ch = Step( &pString, &nLen, &tmp1, &tmp1, &tmp1, &tmp1 ) )
		{
			if( ch == '\n' )
			{
				maxheight = 0;
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[ch] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, ch );
			}
			if( chars[ch] )
			{
				*width += chars[ch]->width;
				if( SUS_GT( (UseFont->baseline - chars[ch]->descent ),int32_t,maxheight,uint32_t) )
					maxheight = UseFont->baseline - chars[ch]->descent;
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


 uint32_t  GetMaxStringLengthFont ( uint32_t width, PFONT UseFont )
{
	if( !UseFont )
		UseFont = &DEFAULTFONT;
	if( width > 0 )
		// character 0 should contain the average width of a character or is it max?
		return width/UseFont->character[0]->width; 
	return 0;
}  

 uint32_t  GetFontHeight ( PFONT font )
{
	if( font )
	{
		if( !font->height )
		{
			InternalRenderFontCharacter( NULL, font, ' ' );
		}
		//lprintf( WIDE("Resulting with %d height"), font->height );
		return font->height;
	}
	return DEFAULTFONT.height;
}

void SetFontBias( SFTFont font, int32_t x, int32_t y )
{
	if( font )
	{
		font->bias = ( x + ( y << 4 ) ) & 0xFF;
		//font->bias_x = x;
		//font->bias_y = y;
	}
}


PRELOAD( InitColorDefaults )
{
	int n;
	color_defs[0].color = BASE_COLOR_BLACK;
	color_defs[0].name = WIDE("black");
	color_defs[1].color = BASE_COLOR_BLUE;
	color_defs[1].name = WIDE("blue");
	color_defs[2].color = BASE_COLOR_GREEN;
	color_defs[2].name = WIDE("green");
	color_defs[3].color = BASE_COLOR_RED;
	color_defs[3].name = WIDE("red");		
	n = 4;
	color_defs[n].name = WIDE("dark blue");		
	color_defs[n++].color = BASE_COLOR_DARKBLUE; 
	color_defs[n].name = WIDE("cyan");		
	color_defs[n++].color = BASE_COLOR_CYAN; 
	color_defs[n].name = WIDE("brown");		
	color_defs[n++].color = BASE_COLOR_BROWN;
	color_defs[n].name = WIDE("brown");		
	color_defs[n++].color = BASE_COLOR_LIGHTBROWN;
	color_defs[n].name = WIDE("light brown");		
	color_defs[n++].color =BASE_COLOR_MAGENTA;
	color_defs[n].name = WIDE("magenta");		
	color_defs[n++].color =BASE_COLOR_LIGHTGREY;
	color_defs[n].name = WIDE("light grey");		
	color_defs[n++].color =BASE_COLOR_DARKGREY;
	color_defs[n].name = WIDE("dark grey");		
	color_defs[n++].color =BASE_COLOR_LIGHTBLUE;
	color_defs[n].name = WIDE("light blue");		
	color_defs[n++].color =BASE_COLOR_LIGHTGREEN;
	color_defs[n].name = WIDE("light green");		
	color_defs[n++].color =BASE_COLOR_LIGHTCYAN;
	color_defs[n].name = WIDE("light cyan");		
	color_defs[n++].color =BASE_COLOR_LIGHTRED;
	color_defs[n].name = WIDE("light red");		
	color_defs[n++].color =BASE_COLOR_LIGHTMAGENTA;
	color_defs[n].name = WIDE("light magenta");		
	color_defs[n++].color =BASE_COLOR_YELLOW;
	color_defs[n].name = WIDE("yellow");		
	color_defs[n++].color =BASE_COLOR_WHITE;
	color_defs[n].name = WIDE("white");		
	color_defs[n++].color =BASE_COLOR_ORANGE;
	color_defs[n].name = WIDE("orange");		
	color_defs[n++].color =BASE_COLOR_NICE_ORANGE;
	color_defs[n].name = WIDE("purple");		
	color_defs[n++].color =BASE_COLOR_PURPLE;
}

IMAGE_NAMESPACE_END

