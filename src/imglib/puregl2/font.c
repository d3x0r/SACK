/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle usage of 'SFTFont's on 'Image's.
 * 
 *  *  consult doc/image.html
 *
 */
#include <stdhdrs.h>
#include <string.h>

#ifdef USE_GLES2
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

#define LIBRARY_DEF
#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#include <imglib/fontstruct.h>
#include <imglib/imagestruct.h>
#define SFTFont struct font_tag *
#include <image.h>
#include "local.h"

#define TRUE (!FALSE)

//#ifdef UNNATURAL_ORDER
// use this as natural - to avoid any confusion about signs...
#define SYMBIT(bit)  ( 1 << (bit&0x7) )
//#else
//#define SYMBIT(bit) ( 0x080 >> (bit&0x7) )
//#endif

IMAGE_NAMESPACE


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
   //lprintf( "character %c(%d)", c, c );
   if( !pImage || c > UseFont->characters || (!pImage->image && !(pImage->flags &IF_FLAG_FINAL_RENDER)) )
		return 0;
   // real quick -
	if( !UseFont->character[c] )
	{
		InternalRenderFontCharacter( NULL, UseFont, c );
		if( !UseFont->character[c] )
         return 0;
	}
	if( !UseFont->character[c]->cell && ( pImage->flags & IF_FLAG_FINAL_RENDER ) )
	{
		Image image = AllocateCharacterSpaceByFont( UseFont, UseFont->character[c] );
		// it's the same characteristics... so we should just pass same step XY
		// oh wait - that's like for lines for sideways stuff... uhmm...should get direction and render 4 bitmaps
		//lprintf( "Render to image this character... %p", image );
		PutCharacterFontX( image, 0, 0, BASE_COLOR_WHITE, 0, c, UseFont, OrderPoints, StepXNormal, StepYNormal );
	}

	pchar = UseFont->character[c];

	if( pImage->flags & IF_FLAG_FINAL_RENDER )
	{
		S_32 xd = x;
		S_32 yd = y+(UseFont->baseline - pchar->ascent);
		S_32 xs = 0;
		S_32 ys = 0;
		Image pifSrc = pchar->cell;
		Image pifSrcReal;
		Image pifDest = pImage;
		
		for( pifSrcReal = pifSrc; pifSrcReal->pParent; pifSrcReal = pifSrcReal->pParent );
		ReloadOpenGlTexture( pifSrc, 0 );
		if( !pifSrc->glActiveSurface )
		{
			return 0;
		}
		//lprintf( "use regular texture %p (%d,%d)", pifSrc, pifSrc->width, pifSrc->height );
		//DebugBreak();        g

		/*
		 * only a portion of the image is actually used, the rest is filled with blank space
		 *
		 */
		TranslateCoord( pifDest, &xd, &yd );
		TranslateCoord( pifSrc, &xs, &ys );
		{
			int glDepth = 1;
			float x_size, x_size2, y_size, y_size2;
			VECTOR v[2][4];
			int vi = 0;
			float texture_v[4][2];
			float _color[4];
			_color[0] = RedVal( color ) / 255.0f;
			_color[1] = GreenVal( color ) / 255.0f;
			_color[2] = BlueVal( color ) / 255.0f;
			_color[3] = AlphaVal( color ) / 255.0f;

			switch( order )
			{
			case OrderPoints:
		   		v[vi][0][0] = xd;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0f;

				v[vi][1][0] = xd;
				v[vi][1][1] = yd+pchar->cell->real_height;
				v[vi][1][2] = 1.0f;

				v[vi][2][0] = xd+pchar->cell->real_width;
				v[vi][2][1] = yd;
				v[vi][2][2] = 1.0f;

				v[vi][3][0] = xd+pchar->cell->real_width;
				v[vi][3][1] = yd+pchar->cell->real_height;
				v[vi][3][2] = 1.0f;
				break;
			case OrderPointsInvert:
		   		v[vi][0][0] = xd - pchar->cell->real_width;
				v[vi][0][1] = yd - pchar->cell->real_height;
				v[vi][0][2] = 1.0f;

				v[vi][1][0] = xd - pchar->cell->real_width;
				v[vi][1][1] = yd+pchar->cell->real_height - pchar->cell->real_height;
				v[vi][1][2] = 1.0f;

				v[vi][2][0] = xd+pchar->cell->real_width - pchar->cell->real_width;
				v[vi][2][1] = yd - pchar->cell->real_height;
				v[vi][2][2] = 1.0f;

				v[vi][3][0] = xd+pchar->cell->real_width - pchar->cell->real_width;
				v[vi][3][1] = yd+pchar->cell->real_height - pchar->cell->real_height;
				v[vi][3][2] = 1.0f;
				break;
			case OrderPointsVertical:
				v[vi][0][0] = xd-pchar->cell->real_height;
				v[vi][0][1] = yd;
				v[vi][0][2] = 1.0;

				v[vi][1][0] = xd+pchar->cell->real_height-pchar->cell->real_height;
				v[vi][1][1] = yd;
				v[vi][1][2] = 1.0;

				v[vi][2][0] = xd-pchar->cell->real_height;
				v[vi][2][1] = yd+pchar->cell->real_width;
				v[vi][2][2] = 1.0;


				v[vi][3][0] = xd+pchar->cell->real_height-pchar->cell->real_height;
				v[vi][3][1] = yd+pchar->cell->real_width;
				v[vi][3][2] = 1.0;
				break;
		   case OrderPointsVerticalInvert:
				v[vi][0][0] = xd;
				v[vi][0][1] = yd-pchar->cell->real_width;
				v[vi][0][2] = 1.0;

				v[vi][1][0] = xd+pchar->cell->real_height;
				v[vi][1][1] = yd-pchar->cell->real_width;
				v[vi][1][2] = 1.0;

				v[vi][2][0] = xd;
				v[vi][2][1] = yd+pchar->cell->real_width-pchar->cell->real_width;
				v[vi][2][2] = 1.0;


				v[vi][3][0] = xd+pchar->cell->real_height;
				v[vi][3][1] = yd+pchar->cell->real_width-pchar->cell->real_width;
				v[vi][3][2] = 1.0;
				break;
			}

			x_size = (double) xs/ (double)pifSrcReal->width;
			x_size2 = (double) (xs+pchar->cell->real_width)/ (double)pifSrcReal->width;
			y_size = (double) ys/ (double)pifSrcReal->height;
			y_size2 = (double) (ys+pchar->cell->real_height)/ (double)pifSrcReal->height;

			//x_size = (double) xs/ (double)pifSrc->width;
			//x_size2 = (double) (xs+pchar->cell->real_width)/ (double)pifSrc->width;
			//y_size = (double) ys/ (double)pifSrc->height;
			//y_size2 = (double) (ys+pchar->cell->real_height)/ (double)pifSrc->height;

			// Front Face
			//glColor4ub( 255,120,32,192 );
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
				vi = 1 - vi;
			}

			scale( v[vi][0], v[vi][0], l.scale );
			scale( v[vi][1], v[vi][1], l.scale );
			scale( v[vi][2], v[vi][2], l.scale );
			scale( v[vi][3], v[vi][3], l.scale );
			switch( order )
			{
			case OrderPoints:
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size, y_size); glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
				texture_v[1][0] = x_size;
				texture_v[1][1] = y_size2;
				/**///glTexCoord2d(x_size, y_size2); glVertex3dv(v[vi][1]);	// Top Left Of The Texture and Quad
				texture_v[2][0] = x_size2;
				texture_v[2][1] = y_size;
				/**///glTexCoord2d(x_size2, y_size); glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
				texture_v[3][0] = x_size2;
				texture_v[3][1] = y_size2;
				/**///glTexCoord2d(x_size2, y_size2); glVertex3dv(v[vi][3]);	// Top Right Of The Texture and Quad
				break;
			case OrderPointsInvert:
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size, y_size2); glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size, y_size); glVertex3dv(v[vi][1]);	// Top Left Of The Texture and Quad
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size2, y_size2); glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size2, y_size); glVertex3dv(v[vi][3]);	// Top Right Of The Texture and Quad
            break;
			case OrderPointsVertical:
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size, y_size2); glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size, y_size); glVertex3dv(v[vi][1]);	// Top Left Of The Texture and Quad
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size2, y_size2); glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size2, y_size); glVertex3dv(v[vi][3]);	// Top Right Of The Texture and Quad
            break;
			case OrderPointsVerticalInvert:
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size2, y_size); glVertex3dv(v[vi][0]);	// Bottom Left Of The Texture and Quad
				texture_v[0][0] = x_size2;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size2, y_size2); glVertex3dv(v[vi][1]);	// Top Left Of The Texture and Quad
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size;
				/**///glTexCoord2d(x_size, y_size); glVertex3dv(v[vi][2]);	// Bottom Right Of The Texture and Quad
				texture_v[0][0] = x_size;
				texture_v[0][1] = y_size2;
				/**///glTexCoord2d(x_size, y_size2); glVertex3dv(v[vi][3]);	// Top Right Of The Texture and Quad
				break;
			}
			EnableShader( "Simple Shaded Texture", v[vi], pifSrc->glActiveSurface, texture_v, _color );
			glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
			// Back Face
		}
	}
	else
	{
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
			// bias the left edge of the character
			for(line = 0;
				 line <= pchar->ascent - pchar->descent;
				 line++ )
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
		MarkImageUpdated( pImage );
	}
	return pchar->width;
}

static _32 _PutCharacterFont( ImageFile *pImage
                                   , S_32 x, S_32 y
                                   , CDATA color, CDATA background
                                   , _32 c, PFONT UseFont )
{
   return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , OrderPoints, StepXNormal, StepYNormal );
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
   PCHARACTER pchar;
   if( !UseFont )
      UseFont = &DEFAULTFONT;
	if( !UseFont->character[c] )
		InternalRenderFontCharacter( NULL, UseFont, c );
   pchar = UseFont->character[c];
   if( !pchar ) return 0;
   x -= pchar->offset;
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, OrderPointsVerticalInvert, StepXInvertVertical, StepYInvertVertical );
}

void PutCharacterFont( ImageFile *pImage
                                   , S_32 x, S_32 y
                                   , CDATA color, CDATA background
                                   , _32 c, PFONT UseFont )
{
   PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , OrderPoints, StepXNormal, StepYNormal );
}

void PutCharacterVerticalFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	//return
		PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , OrderPointsVertical, StepXVertical, StepYVertical );
}


void PutCharacterInvertFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	//return
		PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , OrderPointsInvert, StepXInvert, StepYInvert );
}

void PutCharacterVerticalInvertFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                                 , _32 c, PFONT UseFont )
{
   PCHARACTER pchar;
   if( !UseFont )
      UseFont = &DEFAULTFONT;
	if( !UseFont->character[c] )
		InternalRenderFontCharacter( NULL, UseFont, c );
   pchar = UseFont->character[c];
   if( !pchar ) return;// 0;
   x -= pchar->offset;
	//return
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


void PutStringVerticalFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
   _32 _y = y;
   if( !font )
      font = &DEFAULTFONT;
	if( !pImage || !pc ) return;// y;
   while( *pc && nLen-- )
	{
		if( *pc == '\n' )
		{
			x -= font->height;
         y = _y;
		}
      else
			y += _PutCharacterVerticalFont( pImage, x, y, color, background, *pc, font );
      pc++;
   }
   return;// y;
}

void PutStringInvertVerticalFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
   _32 _y = y;
   if( !font )
      font = &DEFAULTFONT;
   if( !pImage || !pc ) return;// y;
   while( *pc && nLen-- )
   {
		if( *pc == '\n' )
		{
			x += font->height;
         y = _y;
		}
      else
			y -= _PutCharacterVerticalInvertFont( pImage, x, y, color, background, *pc, font );
      pc++;
   }
   return;// y;
}

void PutStringFontEx( ImageFile *pImage
                                  , S_32 x, S_32 y
                                  , CDATA color, CDATA background
                                  , CTEXTSTR pc, size_t nLen, PFONT font )
{
   _32 _x = x;
   if( !font )
		font = &DEFAULTFONT;
   if( !pImage || !pc ) return;// x;
   {
      while( *pc && nLen-- )
      {
			if( *pc == '\n' )
			{
				y += font->height;
				x = _x;
			}
			else
			{
				x += _PutCharacterFont( pImage, x, y, color, background, *pc, font );
			}
         pc++;
      }
   }
   return;// x;
}

void PutStringInvertFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT font )
{
   _32 _x = x;
   if( !font )
      font = &DEFAULTFONT;
   if( !pImage || !pc ) return;// x;
   while( *pc && nLen-- )
   {
		if( *pc == '\n' )
		{
			y -= font->height;
         x = _x;
		}
      else
			x -= _PutCharacterInvertFont( pImage, x, y, color, background, *pc, font );
      pc++;
   }
   return;// x;
}

_32 PutMenuStringFontEx( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, PFONT UseFont )
{
   int bUnderline;
   if( !UseFont )
      UseFont = &DEFAULTFONT;
   bUnderline = FALSE;
   while( *pc && nLen-- )
   {
      int w;
      bUnderline = FALSE;
      if( *pc == '&' )
      {
         bUnderline = TRUE;
         pc++;
         if( *pc == '&' )
            bUnderline = FALSE;
      }
      if( !(*pc) ) // just in case '&' was end of string...
         break;
      w = _PutCharacterFont( pImage, x, y, color, background, *pc, UseFont );
      if( bUnderline )
         do_line( pImage, x, y + UseFont->height -1,
                          x + w, y + UseFont->height -1, color );
      x += w;
      pc++;
   }
   return x;
}

 _32  GetMenuStringSizeFontEx ( CTEXTSTR string, size_t len, int *width, int *height, PFONT font )
{
   int _width;
   if( !font )
      font = &DEFAULTFONT;
   if( height )
      *height = font->height;
   if( !width )
      width = &_width;
   *width = 0;
   while( string && *string && ( len-- > 0 ) )
   {
      if( *string == '&' )
      {
         string++;
         len--;
         if( !*string )
            break;
      }
		if( !font->character[(unsigned)*string] )
			InternalRenderFontCharacter( NULL, font, *string );
      *width += font->character[(unsigned)*string]->width;
      string++;
   }
   return *width;
}

 _32  GetMenuStringSizeFont ( CTEXTSTR string, int *width, int *height, PFONT font )
{
   return GetMenuStringSizeFontEx( string, strlen( string ), width, height, font );
}

 _32  GetMenuStringSizeEx ( CTEXTSTR string, int len, int *width, int *height )
{
   return GetMenuStringSizeFontEx( string, len, width, height, &DEFAULTFONT );
}

 _32  GetMenuStringSize ( CTEXTSTR string, int *width, int *height )
{
   return GetMenuStringSizeFontEx( string, strlen( string ), width, height, &DEFAULTFONT );
}

 _32  GetStringSizeFontEx ( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, PFONT UseFont )
{
   _32 _width, max_width, _height;
   PCHARACTER *chars;
   if( !UseFont )
		UseFont = &DEFAULTFONT;
   // a character must be rendered before height can be computed.
	if( !UseFont->character[0] )
	{
		InternalRenderFontCharacter(  NULL, UseFont, *pString );
	}
   if( height )
		*height = UseFont->height;
	else
      height = &_height;
   if( !width )
		width = &_width;
	max_width = *width = 0;
	chars = UseFont->character;
	if( pString )
	{
		while( pString && *pString && nLen-- )
		{
			if( *pString == '\n' )
			{
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[(unsigned)*pString] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, *pString );
			}
			if( chars[ (int)((unsigned char)*pString)] )
				*width += chars[((unsigned char)*pString)]->width;
			pString++;
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
   PCHARACTER *chars;
   if( !UseFont )
		UseFont = &DEFAULTFONT;
   // a character must be rendered before height can be computed.
	if( !UseFont->character[0] )
	{
		InternalRenderFontCharacter(  NULL, UseFont, *pString );
	}
   if( height )
		*height = UseFont->height;
	else
      height = &_height;
   if( !width )
		width = &_width;
	max_width = *width = 0;
	chars = UseFont->character;
	if( pString )
	{
		while( pString && *pString && nLen-- )
		{
			if( *pString == '\n' )
			{
            maxheight = 0;
				*height += UseFont->height;
				if( *width > max_width )
					max_width = *width;
				*width = 0;
			}
			else if( !chars[(unsigned)*pString] )
			{
				InternalRenderFontCharacter(  NULL, UseFont, *pString );
			}
			if( chars[(int)*pString] )
			{
				*width += chars[*pString]->width;
				if( SUS_GT( (UseFont->baseline - chars[*pString]->descent ),S_32,maxheight,_32) )
					maxheight = UseFont->baseline - chars[*pString]->descent;
			}
			pString++;
		}
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

IMAGE_NAMESPACE_END

// $Log: font.c,v $
// Revision 1.36  2005/01/27 08:20:57  panther
// These should be cleaned up soon... but they're messy and sprite acutally used them at one time.
//
// Revision 1.35  2004/12/15 03:17:11  panther
// Minor remanants of fixes already commited.
//
// Revision 1.34  2004/10/13 18:53:18  d3x0r
// protect against images which have no surface
//
// Revision 1.33  2004/10/02 23:41:06  d3x0r
// Protect against NULL height result.
//
// Revision 1.32  2004/08/11 12:52:36  d3x0r
// Should figure out where they hide flag isn't being set... vline had to check for height<0
//
// Revision 1.32  2004/08/09 03:28:27  jim
// Make strings by default handle \n binary characters.
//
// Revision 1.31  2004/06/21 07:47:12  d3x0r
// Account for newly moved structure files.
//
// Revision 1.30  2004/03/26 06:27:33  d3x0r
// Fix 2-bit font alpha computation some (not perfect, but who is?)
//
// Revision 1.29  2003/10/13 05:04:16  panther
// Hmm wonder how that minor thing got through... font alpha addition was obliterated.
//
// Revision 1.28  2003/10/08 09:28:48  panther
// Simplify some work drawing characters...
//
// Revision 1.27  2003/10/07 20:29:17  panther
// Generalize font render to one routine, handle multi-bit fonts.
//
// Revision 1.26  2003/10/07 02:12:50  panther
// Ug - it's all terribly broken
//
// Revision 1.25  2003/10/07 01:53:52  panther
// Quick and dirty patch to support alpha fonts
//
// Revision 1.24  2003/08/27 07:57:21  panther
// Fix calling of plot in lineasm.  Fix no character data in font
//
// Revision 1.23  2003/05/01 21:37:30  panther
// Fix string to const string
//
// Revision 1.22  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
