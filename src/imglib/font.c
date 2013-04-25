/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 *   Handle usage of 'SFTFont's on 'Image's.
 * 
 *  *  consult doc/image.html
 *
 */

#include <string.h>
#include <sharemem.h>
#define LIBRARY_DEF
#define IMAGE_LIBRARY_SOURCE
#include <imglib/fontstruct.h>
#include <imglib/imagestruct.h>
#include <image.h>

#define TRUE (!FALSE)

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


static _32 PutCharacterFontX ( ImageFile *pImage
                             , S_32 x, S_32 y
                             , CDATA color, CDATA background
                             , _32 c, PFONT UseFont
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
   if( !pImage || c > UseFont->characters || !pImage->image )
		return 0;
   // real quick -
	if( !UseFont->character[c] )
		InternalRenderFontCharacter( NULL, UseFont, c );

	pchar = UseFont->character[c];
   if( !pchar ) return 0;
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
      for(line = UseFont->baseline - pchar->ascent;
          line <= UseFont->baseline - pchar->descent;
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
   return pchar->width;
}

static _32 _PutCharacterFont( ImageFile *pImage
                                   , S_32 x, S_32 y
                                   , CDATA color, CDATA background
                                   , _32 c, PFONT UseFont )
{
   return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , StepXNormal, StepYNormal );
}

static _32 _PutCharacterVerticalFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , StepXVertical, StepYVertical );
}


static _32 _PutCharacterInvertFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	return PutCharacterFontX( pImage, x, y, color, background, c, UseFont
									, StepXInvert, StepYInvert );
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
									, StepXInvertVertical, StepYInvertVertical );
}

void PutCharacterFont( ImageFile *pImage
                                   , S_32 x, S_32 y
                                   , CDATA color, CDATA background
                                   , _32 c, PFONT UseFont )
{
   PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , StepXNormal, StepYNormal );
}

void PutCharacterVerticalFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	//return
		PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , StepXVertical, StepYVertical );
}


void PutCharacterInvertFont( ImageFile *pImage
                                           , S_32 x, S_32 y
                                           , CDATA color, CDATA background
                                           , _32 c, PFONT UseFont )
{
	//return
		PutCharacterFontX( pImage, x, y, color, background, c, UseFont
                            , StepXInvert, StepYInvert );
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
                            , StepXInvertVertical, StepYInvertVertical );
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
