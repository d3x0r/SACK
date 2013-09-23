
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>

#define IMAGE_LIBRARY_SOURCE
#include <ft2build.h>
#ifdef FT_BEGIN_HEADER

#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <filesys.h>
#include <sharemem.h>
#include <timers.h>
//#include "global.h"
#include <imglib/fontstruct.h>

#include <image.h>
#include "fntglobal.h"
#include <controls.h>

IMAGE_NAMESPACE

#define SYMBIT(bit)  ( 1 << (bit&0x7) )

FILE *output;

//-------------------------------------------------------------------------

#define fg (*global_font_data)
PRIORITY_PRELOAD( CreateFontRenderGlobal, IMAGE_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( global_font_data );
	if( !fg.library )
	{
		int error;
		InitializeCriticalSec( &fg.cs );
		error = FT_Init_FreeType( &fg.library );
		if( error )
		{
			Log1( WIDE("Free type init failed: %d"), error );
			return ;
		}
	}
}

//-------------------------------------------------------------------------

void PrintLeadin( int bits )
{
	fprintf( output, WIDE( "#include <vectlib.h>\n" ) );
	fprintf( output, WIDE( "#undef _X\n" ) );

	if( bits == 2)
		fprintf( output, WIDE("#define BITS_GREY2\n") );
	if( bits != 8 )
		fprintf( output, WIDE("#include \"symbits.h\"\n") );

	fprintf( output, WIDE( "IMAGE_NAMESPACE\n") );
	fprintf( output, WIDE( "	typedef struct font_tag *PFONT ;\n") );
	fprintf( output, WIDE( "#ifdef __cplusplus \n") );
	fprintf( output, WIDE( "	namespace default_font {\n") );
	fprintf( output, WIDE( "#endif\n") );
	fprintf( output, WIDE( "\n") );
	fprintf( output, WIDE( "#define EXTRA_STRUCT  struct ImageFile_tag *cell; RCOORD x1, x2, y1, y2; struct font_char_tag *next_in_line;\n") );
	fprintf( output, WIDE( "#define EXTRA_INIT  0,0,0,0,0,0\n") );




	fprintf( output, WIDE("typedef char CHARACTER, *PCHARACTER;\n") );
}

int PrintChar( int bits, int charnum, PCHARACTER character, int height )
{
	int  outwidth;
	TEXTCHAR charid[64];
	char *data = (char*)character->data;
	snprintf( charid, sizeof( charid ), WIDE("_char_%d"), charnum );

	#define LINEPAD WIDE("                  ")
	if( bits == 8 )
		outwidth = character->size; // round up to next byte increments size.
	else if( bits == 2 )
		outwidth = ((character->size+3) & 0xFC ); // round up to next byte increments size.
	else
		outwidth = ((character->size+7) & 0xF8 ); // round up to next byte increments size.

	if( ((outwidth)/(8/bits))*( ( character->ascent - character->descent ) + 1 ))
		fprintf( output, WIDE("static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT unsigned char data[%d]; } %s =\n"),
						((outwidth)/(8/bits))*( ( character->ascent - character->descent ) + 1 )
						, charid );
	else
		fprintf( output, WIDE("static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT } %s =\n")
						, charid );

	fprintf( output, WIDE("{ %2d, %2d, %2d, 0, %2d, %2d, EXTRA_INIT ")
							, character->size
							, character->width
							, (signed)character->offset
							, character->ascent
							, character->descent
							);

	if( character->size )
		fprintf( output, WIDE(", { \n") LINEPAD );

	{
		int line, bit;
	   	char *dataline;
		data = (char*)character->data;
		for(line = character->ascent;
			 line >= character->descent;
			 line-- )
		{
			if( line != character->ascent )
			{
				if( !line )
					fprintf( output, WIDE(", // <---- baseline\n") LINEPAD );
				else
					fprintf( output, WIDE(",\n") LINEPAD );
			}
			dataline = data;
			if( bits == 1 )
			{
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit && ((bit % 8) == 0 ) )
						fprintf( output, WIDE(",") );
					if( dataline[bit >> 3] & SYMBIT(bit) )
					{
						fprintf( output, WIDE("X") );
					}
					else
						fprintf( output, WIDE("_") );
				}
			}
			else if( bits == 2)
			{
				/*
				for( bit = 0; bit < (character->size+(3))/(8/bits); bit++ )
				{
					fprintf( output, WIDE("%02x "), dataline[bit] );
					}
					*/
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit && ((bit % 4) == 0 ) )
						fprintf( output, WIDE(",") );
					//fprintf( output, WIDE("%02x "), (dataline[bit >> 2] >> (2*(bit&0x3))) & 3 );
					switch( (dataline[bit >> 2] >> (2*(bit&0x3))) & 3 )
					{
					case 3:
						fprintf( output, WIDE("X") );
						break;
					case 2:
						fprintf( output, WIDE("O") );
						break;
					case 1:
						fprintf( output, WIDE("o") );
						break;
					case 0:
						fprintf( output, WIDE("_") );
						break;
					}
				}
			}
			else if( bits == 8 )
			{
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit )
						fprintf( output, WIDE(",") );
					fprintf( output, WIDE("%3u"), (unsigned)dataline[bit] );
				}

			}

			// fill with trailing 0's
			if( bits < 8 )
			{
				for( ; bit < outwidth; bit++ )
					fprintf( output, WIDE("_") );
			}
			data += (character->size+(bits==8?0:bits==2?3:7))/(8/bits);
		}
	}

	if( character->size )
		fprintf( output, WIDE("} ") );
	fprintf( output, WIDE("};\n\n\n") );
	return 0;
}


void PrintFontTable( CTEXTSTR name, PFONT font )
{
	int idx;
	int i, maxwidth;
	idx = 0;
	maxwidth = 0;
	for( i = 0; i < 256; i++ )
	{
		if( font->character[i] )
			if( font->character[i]->width > maxwidth )
				maxwidth = font->character[i]->width;
	}
	fprintf( output, WIDE("struct { unsigned short height, baseline, chars; unsigned char flags, junk;\n")
				WIDE("         char *fontname;\n")
			  WIDE("         PCHARACTER character[256]; }\n")
			  WIDE("#ifdef __cplusplus\n")
			  WIDE( "       ___%s\n")
			  WIDE("#else\n")
			  WIDE( "       __%s\n")
			  WIDE("#endif\n")
			  WIDE( "= { \n%d, %d, 256, %d, 0, \"%s\", {")
				, name
			 , name
				, font->height 
				, font->baseline
				, font->flags
           , name
				);

	for( i = 0; i < 256; i++ )
	{
		if( font->character[i] )
			fprintf( output, WIDE(" %c(PCHARACTER)&_char_%d\n"), (i)?',':' ', i );

	}
	fprintf( output, WIDE("\n} };") );

	fprintf( output, WIDE("\n") );
	fprintf( output, WIDE("#ifdef __cplusplus\n") );
	fprintf( output, WIDE("PFONT __%s = (PFONT)&___%s;\n"), name, name );
	fprintf( output, WIDE("#endif\n") );

}

void PrintFooter( void )
{
	fprintf( output, WIDE("#ifdef __cplusplus\n" ) );
	fprintf( output, WIDE("      }; // default_font namespace\n" ) );
	fprintf( output, WIDE("#endif\n" ) );
	fprintf( output, WIDE( "IMAGE_NAMESPACE_END\n") );

}

//-------------------------------------------------------------------------

void DumpFontFile( CTEXTSTR name, SFTFont font_to_dump )
{
	PFONT font = (PFONT)font_to_dump;
	if( font )
	{
		output = sack_fopen( 0, name, WIDE("wt") );
		if( output )
		{
			PrintLeadin(  (font->flags&3)==1?2
							: (font->flags&3)==2?8
							: 1 );
			{
				int charid;
				for	( charid = 0; charid < 256; charid++ )
				{
					PCHARACTER character;
					void InternalRenderFontCharacter( PFONT_RENDERER renderer, PFONT font, INDEX idx );
               InternalRenderFontCharacter( NULL, font_to_dump, charid );
					character = font->character[charid];
					if( character )
						PrintChar( (font->flags&3) == FONT_FLAG_8BIT?8
									 :(font->flags&3) == FONT_FLAG_2BIT?2
									 :1
									,charid, character, font->height );
				}
			}
			PrintFontTable( font->name, font );
         PrintFooter();
			sack_fclose( output );
		}
	}
	else
		Log( WIDE("No font to dump...") );
}

//-------------------------------------------------------------------------

struct font_renderer_tag {
	TEXTCHAR *file;
	S_16 nWidth;
	S_16 nHeight;
	FRACTION width_scale;
   FRACTION height_scale;
	_32 flags; // whether to render 1(0/1), 2(2), 8(3) bits, ...
	POINTER ResultData; // data that was resulted for creating this font.
	size_t ResultDataLen;
	POINTER font_memory;
	FT_Face face;
	FT_Glyph glyph;
	FT_GlyphSlot slot;
	SFTFont ResultFont;
	PFONT font;
	TEXTCHAR fontname[256];

   int max_ascent;
   int min_descent;
	// -- - - - -- - - This bit of structure is for character renedering to an image which is downloaded for bitmap fonts
   //
	// full surface, contains all characters.  Starts as 3 times the first character (9x vertical and horizontal)
   // after that it is expanded 2x (4x total, 2x horizontal 2x rows )
	Image surface;
   int nLinesUsed;
   int nLinesAvail;
	int *pLineStarts; // counts highest character in a line, each line walks across
   PCHARACTER *ppCharacterLineStarts; // beginning of chain of characters on this image.
} ;
typedef struct font_renderer_tag FONT_RENDERER;

static PLIST fonts;

//-------------------------------------------------------------------------

void UpdateRendererImage( PFONT_RENDERER renderer, int char_width, int char_height )
{
	if( !renderer->surface )
	{
		renderer->surface = MakeImageFile( char_width * 3, char_height * 3 );
		ClearImage( renderer->surface );
	}
	else
	{
		Image tmp;
		Image new_surface = MakeImageFile( renderer->surface->real_width * 2, renderer->surface->real_height * 2 );
		ClearImage( new_surface );
		BlotImage( new_surface, renderer->surface, 0, 0 );
		while( tmp = renderer->surface->pChild )
		{
			OrphanSubImage( tmp );
			AdoptSubImage( new_surface, tmp );
		}
		UnmakeImageFile( renderer->surface );
		renderer->surface = new_surface;
      // need to recompute character floating point values for existing characters.
	}
}

Image AllocateCharacterSpace( PFONT_RENDERER renderer, PCHARACTER character )
{
	int height;
	int line;
	int last_line_top;

	height = (character->ascent - character->descent)+1;
	//if( height < renderer->font->height )
	//   height = renderer->font->height;
	//lprintf( "GetImage for character %p %d %d %d,%d,%d  h:%d,c:%d", renderer
	//		 , character->width
	//		 , renderer->font->baseline
	//		 , character->descent
	//		 , character->ascent
	//		 , character->descent-character->ascent
	//		 , height, character->junk );
	do
	{
		last_line_top = 0;
		for( line = 0; line < renderer->nLinesUsed; line++ )
		{
         //lprintf( "Is %d in %d?", height, renderer->pLineStarts[line] );
			if( height <= renderer->pLineStarts[line] )
			{
				// fits within the current line height...
				PCHARACTER check = renderer->ppCharacterLineStarts[line];
				int new_line_start = 0;
				// seek to last character on line, accumulating widths
				while( check )
				{
					new_line_start += check->width+2;
					if( !check->next_in_line )
						break;
					check = check->next_in_line;
				}
				if( ( new_line_start + character->width + 2 ) <= renderer->surface->real_width )
				{
					check->next_in_line = character;
					character->cell = MakeSubImage( renderer->surface
															, new_line_start + 1, last_line_top + 1
															, character->width, height );
					//lprintf( "Result %d,%d", character->cell->real_x, character->cell->real_y );
					return character->cell;
				}
			}
			last_line_top += renderer->pLineStarts[line];
		}

		// if we got here, didn't find room on any line.
		if( !renderer->surface )
		{
			lprintf( WIDE( "make new image" ) );
			// gonna need lines and character chain starts too... don't continue
			UpdateRendererImage( renderer, character->width, height );
		}

		// this should NOT go more than once.... but it could with nasty fonts...
		if( ( last_line_top + height ) > renderer->surface->real_height )
		{
			lprintf( WIDE( "expand font image %d %d %d" ), last_line_top, height, renderer->surface->real_height );
			UpdateRendererImage( renderer, character->width, height );
			continue; // go to top of while loop;
		}

		if( renderer->nLinesUsed == renderer->nLinesAvail )
		{
         lprintf( WIDE( "make new lines" ) );
         renderer->nLinesAvail += 16; // another 16 lines... (16 ever should be enough)
         renderer->pLineStarts = (int*)HeapReallocate( 0, renderer->pLineStarts, sizeof( int ) * renderer->nLinesAvail );
         renderer->ppCharacterLineStarts = (PCHARACTER*)HeapReallocate( 0, renderer->ppCharacterLineStarts, sizeof( PCHARACTER ) * renderer->nLinesAvail );
		}

		renderer->nLinesUsed++;

		renderer->pLineStarts[line] = height+2;
		renderer->ppCharacterLineStarts[line] = character;

		character->cell = MakeSubImage( renderer->surface
												, 0+1, last_line_top+1
												, character->width, height
												);
		break;
	} while( 1 );
   lprintf( WIDE( "Result %d,%d" ), character->cell->real_x, character->cell->real_y );
   return character->cell;
}

Image AllocateCharacterSpaceByFont( SFTFont font, PCHARACTER character )
{
	PFONT_RENDERER renderer;
	INDEX idx;
	if( font )
	{
		LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
		{
			if( renderer->font == (PFONT)font )
				break;
		}
		if( !renderer )
		{
			renderer = New( FONT_RENDERER );
			MemSet( renderer, 0, sizeof( FONT_RENDERER ) );
			renderer->nWidth = (S_16)0;
			renderer->nHeight = (S_16)0;
			renderer->flags = 0;
			renderer->file = NULL;
			renderer->font = font;
			AddLink( &fonts, renderer );
		}
		return AllocateCharacterSpace( renderer, character );
	}
	return NULL;
}

//-------------------------------------------------------------------------

#define TOSCALED(n) ((n)<<6)
#define CEIL(n)  ((S_16)((((n) + 63 )&(-64)) >> 6))
#define FLOOR(n) ((S_16)(((n) & (-64)) >> 6))

#define BITMAP_MASK(buf,n)  (buf[((n)/8)] & (0x80U >> ( (n) & 7 )))

void RenderMonoChar( PFONT font
					, INDEX idx
					, FT_Bitmap *bitmap
					, FT_Glyph_Metrics *metrics
					, FT_Int left_ofs
					, FT_Int width
					)
{
	/*
	Log2( WIDE("Character: %d(%c)"), idx, idx>32&&idx<127?idx:0 );
	//Log1( WIDE("Font Height %d"), font->height );
	//Log5( WIDE("Bitmap information: (%d by %d) %d %d %d")
	//	 , bitmap->width
	//	 , bitmap->rows
	//	 , bitmap->pitch
	//	 , bitmap->pixel_mode
	//	 , bitmap->palette
	//	 );
	Log2( WIDE("Metrics: height %d bearing Y %d"), metrics->height, metrics->horiBearingY );
	Log2( WIDE("Metrics: width %d bearing X %d"), metrics->width, metrics->horiBearingX );
	Log2( WIDE("Metrics: advance %d %d"), metrics->horiAdvance, metrics->vertAdvance );
	*/
	if( bitmap->width > 1000 ||
	    bitmap->width < 0 ||
	    bitmap->rows > 1000 ||
	    bitmap->rows < 0 )
	{
		Log3( WIDE("Failing character %") _32fs WIDE(" rows: %d  width: %d"), idx, bitmap->width, bitmap->rows );
		font->character[idx] = NULL;
		return;
	}

	{
		PCHARACTER character = NewPlus( CHARACTER,
												  ( bitmap->rows
													* ((bitmap->width+7)/8) ) );
		INDEX bit, line, linetop, linebottom;
		INDEX charleft, charright;
		INDEX lines;

		char *data, *chartop;
		font->character[idx] = character;
		font->character[idx]->next_in_line = NULL;
		font->character[idx]->cell = NULL;
		if( !character )
		{
			Log( WIDE("Failed to allocate character") );
			Log2( WIDE("rows: %d width: %d"), bitmap->rows, bitmap->width );
			return;
		}
		character->width = width /*CEIL(metrics->horiAdvance)*/;
		if( !bitmap->rows && !bitmap->width )
		{
			//Log( WIDE("No physical data for this...") );
			character->ascent = 0;
			character->descent = 0;
			character->offset = 0;
			character->size = 0;
			return;
		}

		if( CEIL( metrics->width ) != bitmap->width )
		{
			//Log4( WIDE("%d(%c) metric and bitmap width mismatch : %d %d")
			//		, idx, (idx>=32 && idx<=127)?idx:'.'
			//	 , CEIL( metrics->width ), bitmap->width );
			if( !CEIL( metrics->width ) && bitmap->width )
				metrics->width = TOSCALED( bitmap->width );
		}
		if( CEIL( metrics->height ) != bitmap->rows )
		{
			//lprintf( WIDE("%d(%c) metric and bitmap height mismatch : %d %d using height %d")
			//		, idx, (idx>=32 && idx<=127)?idx:'.'
			//	 , CEIL( metrics->height )
			//	 , bitmap->rows
			//	 , TOSCALED( bitmap->rows) );
			metrics->height = TOSCALED( bitmap->rows );
		}

		character->ascent = CEIL(metrics->horiBearingY);
		if( metrics->height )
			character->descent = -CEIL(metrics->height - metrics->horiBearingY) + 1;
		else
			character->descent = CEIL( metrics->horiBearingY ) - font->height + 1;
		character->offset = CEIL(metrics->horiBearingX);
		character->size = bitmap->width;
		//charleft = charleft;
		//charheight = CEIL( metrics->height );
      if( 0 )
		lprintf( WIDE("(%d(%c)) Character parameters: %d %d %d %d %d")
				 , idx, idx< 32 ? ' ':idx
				 , character->width
				 , character->offset
				 , character->size
				 , character->ascent
				 , character->descent );

		// for all horizontal lines which are blank - decrement ascent...
		chartop = NULL;
		if( bitmap->buffer )
		{
			data = (char*)bitmap->buffer;

			linetop = 0;
			linebottom = character->ascent - character->descent;
			//Log2( WIDE("Linetop: %d linebottom: %d"), linetop, linebottom );

			lines = character->ascent - character->descent;

			for( line = linetop; line <= linebottom; line++ )
			{
				data = (char*)bitmap->buffer + line * bitmap->pitch;
				for( bit = 0; bit < character->size; bit++ )
				{
					if( BITMAP_MASK(data,bit) )
						break;
				}
				if( bit == character->size )
				{
					//Log( WIDE("Dropping a line...") );
					character->ascent--;
				}
				else
				{
					linetop = line;
					chartop = data;
					break;
				}
			}
			if( line == linebottom + 1 )
			{
				Deallocate( POINTER, font->character[idx] );
				font->character[idx] = NULL;
				//Log( WIDE("No character data usable...") );
				return;
			}

			for( line = linebottom; line > linetop; line-- )
			{
				data = (char*)bitmap->buffer + line * bitmap->pitch;
				for( bit = 0; bit < character->size; bit++ )
				{
					if( BITMAP_MASK(data,bit) )
						break;
				}
				if( bit == character->size )
				{
					//Log( WIDE("Dropping a line...") );
					character->descent++;
				}
				else
				{
					break;
				}
			}

			linebottom = linetop + character->ascent - character->descent;
			// find character left offset...
			charleft = character->size;
			for( line = linetop; line <= linebottom; line++ )
			{
				data = (char*)bitmap->buffer + (line) * bitmap->pitch;
				for( bit = 0; bit < character->size; bit++ )
				{
					if( BITMAP_MASK( data, bit ) )
					{
						if( bit < charleft )
							charleft = bit;
					}
				}
				if( bit < character->size )
					break;
			}
			if( charleft )
			{
				//Log3( WIDE("Reduced char(%c) size by %d to %d"), idx>32&&idx<127?idx:'.', charleft, character->size-charleft );
			}
			// if zero no change...

			// find character right extent
			charright = 0;
			for( line = linetop; line <= linebottom; line++ )
			{
				data = (char*)bitmap->buffer + (line) * bitmap->pitch;
				for( bit = charleft; bit < character->size; bit++ )
				{
					if( BITMAP_MASK( data, bit ) )
						if( bit > charright )
							charright = bit;
				}
			}
			charright ++; // add one back in... so we have a delta between left and right
			//Log2( WIDE("Reduced char right size %d to %d"), charright, charright - charleft );

			character->offset = (_8)charleft;
			character->size = (_8)(charright - charleft);

			//Log7( WIDE("(%d(%c)) Character parameters: %d %d %d %d %d")
			//	 , idx, idx< 32 ? ' ':idx
			//	 , character->width
			//	 , character->offset
			//	 , character->size
			//	 , character->ascent
			//	 , character->descent );
			{
				unsigned char *outdata;
				// now - to copy the pixels...
				for( line = linetop; line <= linebottom; line++ )
				{
					outdata = character->data +
						((line - linetop) * ((character->size + 7)/8));
					data = (char*)bitmap->buffer + (line) * bitmap->pitch;
					for( bit = 0; bit < ((character->size + 7U)/8U); bit++)
						outdata[bit] = 0;
					for( bit = 0; bit < character->size; bit++ )
					{
						if( BITMAP_MASK( data, bit + character->offset ) )
						{
							outdata[(bit)>>3] |= 0x01 << (bit&7);
						}
					}
				}
			}
			character->offset += left_ofs;
		}
	}
}


#define GREY_MASK(buf,n)  ((bits==2)?(\
	(((buf[n]&0xE0)?2:0)|   \
	 ((buf[n]&0x3F)?1:0))       \
	):(buf[n]))

void RenderGreyChar( PFONT font
						 , INDEX idx
						 , FT_Bitmap *bitmap
						 , FT_Glyph_Metrics *metrics
						 , FT_Int left_ofs
						 , FT_Int width
						 , _32 bits // 2 or 8
						 )
{
   //lprintf( WIDE("Rending char %d bits %d"), idx, bits );
	/*
	 Log2( WIDE("Character: %d(%c)"), idx, idx>32&&idx<127?idx:0 );
	 //Log1( WIDE("Font Height %d"), font->height );
	 //Log5( WIDE("Bitmap information: (%d by %d) %d %d %d")
	 //	 , bitmap->width
	//	 , bitmap->rows
	//	 , bitmap->pitch
	//	 , bitmap->pixel_mode
	//	 , bitmap->palette
	//	 );
	Log2( WIDE("Metrics: height %d bearing Y %d"), metrics->height, metrics->horiBearingY );
	Log2( WIDE("Metrics: width %d bearing X %d"), metrics->width, metrics->horiBearingX );
	Log2( WIDE("Metrics: advance %d %d"), metrics->horiAdvance, metrics->vertAdvance );
   */
	if( bitmap->width > 1000 ||
	    bitmap->width < 0 ||
	    bitmap->rows > 1000 ||
	    bitmap->rows < 0 )
	{
		Log3( WIDE("Failing character %") _32fs WIDE(" rows: %d  width: %d"), idx, bitmap->width, bitmap->rows );
		font->character[idx] = NULL;
		return;
	}

	{
		PCHARACTER character = NewPlus( CHARACTER,
												  + ( bitmap->rows
													  * (bitmap->width+(bits==8?0:bits==2?3:7)/(8/bits)) ) );
		INDEX bit, line, linetop, linebottom;
		INDEX charleft, charright;
		INDEX lines;

		char *data, *chartop;
		font->character[idx] = character;
		font->character[idx]->next_in_line = NULL;
		font->character[idx]->cell = NULL;
		if( !character )
		{
			Log( WIDE("Failed to allocate character") );
			Log2( WIDE("rows: %d width: %d"), bitmap->rows, bitmap->width );
			return;
		}
		character->width = width /*CEIL(metrics->horiAdvance)*/;
		if( !bitmap->rows && !bitmap->width )
		{
			//Log( WIDE("No physical data for this...") );
			character->ascent = 0;
			character->descent = 0;
			character->offset = 0;
			character->size = 0;
			return;
		}

		if( CEIL( metrics->width ) != bitmap->width )
		{
			Log4( WIDE("%") _32fs WIDE("(%c) metric and bitmap width mismatch : %d %d")
					, idx, (int)((idx>=32 && idx<=127)?idx:'.')
					, CEIL( metrics->width ), bitmap->width );
		}
		if( CEIL( metrics->height ) != bitmap->rows )
		{
			Log4( WIDE("%") _32fs WIDE("(%c) metric and bitmap height mismatch : %d %d")
					, idx, (int)((idx>=32 && idx<=127)?idx:'.')
					, CEIL( metrics->height ), bitmap->rows );
			metrics->height = TOSCALED( bitmap->rows );
		}

		character->ascent = CEIL(metrics->horiBearingY);
		if( metrics->height )
			character->descent = -CEIL(metrics->height - metrics->horiBearingY) + 1;
		else
			character->descent = CEIL( metrics->horiBearingY ) - font->height + 1;
		character->offset = CEIL(metrics->horiBearingX);
		character->size = bitmap->width;

		if( 0 )
			lprintf( WIDE("(%d(%c)) Character parameters: %d %d %d %d %d")
					 , idx, idx< 32 ? ' ':idx
					 , character->width
					 , character->offset
					 , character->size
					 , character->ascent
					 , character->descent );

		// for all horizontal lines which are blank - decrement ascent...
		chartop = NULL;
		data = (char*)bitmap->buffer;

		linetop = 0;
		linebottom = character->ascent - character->descent;
		//Log2( WIDE("Linetop: %d linebottom: %d"), linetop, linebottom );

		lines = character->ascent - character->descent;
		// reduce ascent to character data minimum....
		for( line = linetop; line <= linebottom; line++ )
		{
			data = (char*)bitmap->buffer + line * bitmap->pitch;
			for( bit = 0; bit < character->size; bit++ )
			{
				if( GREY_MASK(data,bit) )
					break;
			}
			if( bit == character->size )
			{
				//Log( WIDE("Dropping a line...") );
				character->ascent--;
			}
			else
			{
				linetop = line;
				chartop = data;
				break;
			}
		}
		if( line == linebottom + 1 )
		{
			character->size = 0;
			//Log( WIDE("No character data usable...") );
			return;
		}

		for( line = linebottom; line > linetop; line-- )
		{
			data = (char*)bitmap->buffer + line * bitmap->pitch;
			for( bit = 0; bit < character->size; bit++ )
			{
				if( GREY_MASK(data,bit) )
					break;
			}
			if( bit == character->size )
			{
				//Log( WIDE("Dropping a line...") );
				character->descent++;
			}
			else
			{
				break;
			}
		}

		linebottom = linetop + character->ascent - character->descent;
		// find character left offset...
		charleft = character->size;
		for( line = linetop; line <= linebottom; line++ )
		{
			data = (char*)bitmap->buffer + (line) * bitmap->pitch;
			for( bit = 0; bit < character->size; bit++ )
			{
				if( GREY_MASK( data, bit ) )
				{
					if( bit < charleft )
						charleft = bit;
				}
			}
			if( bit < character->size )
				break;
		}

		// find character right extent
		charright = 0;
		for( line = linetop; line <= linebottom; line++ )
		{
			data = (char*)bitmap->buffer + (line) * bitmap->pitch;
			for( bit = charleft; bit < character->size; bit++ )
			{
				if( GREY_MASK( data, bit ) )
					if( bit > charright )
						charright = bit;
			}
		}
		charright ++; // add one back in... so we have a delta between left and right
		//Log2( WIDE("Reduced char right size %d to %d"), charright, charright - charleft );

		character->offset = (_8)charleft;
		character->size = (_8)(charright - charleft);

		{
			unsigned char *outdata;
			// now - to copy the pixels...
			for( line = linetop; line <= linebottom; line++ )
			{
				outdata = character->data +
					((line - linetop) * ((character->size + (bits==8?0:bits==2?3:7))/(8/bits)));
				data = (char*)bitmap->buffer + (line) * bitmap->pitch;
				for( bit = 0; bit < ((character->size + (bits==8?0:bits==2?3:7))/(8/bits)); bit++)
					outdata[bit] = 0;
				for( bit = 0; bit < (character->size); bit++ )
				{
					int grey = GREY_MASK( data, bit + character->offset );
					//printf( WIDE("(%02x)"), grey );
					if( grey )
					{
						if( bits == 2 )
						{
							outdata[bit>>2] |= grey << ((bit % 4) * 2);
							//printf( WIDE("%02x "), outdata[bit>>2] );
							/*
							switch( grey )
							{
							case 3: printf( WIDE("X") ); break;
							case 2: printf( WIDE("O") ); break;
							case 1: printf( WIDE("o") ); break;
							case 0: printf( WIDE("_") ); break;
							default: printf( WIDE("%02X"), grey );
							}
							*/
						}
						else
							outdata[bit] = grey;
					}
					//else
					//	printf( WIDE("_") );
				}
				//printf( WIDE("\n") );
			}
		}
		character->offset += left_ofs;
	}
}


static void KillSpaces( TEXTCHAR *string )
{
	int ofs = 0;
	if( !string )
		return;
	while( *string )
	{
		string[ofs] = string[0];
		if( *string == ' ' )
			ofs--;
		string++;
	}
	string[ofs] = 0;
}

extern int InitFont( void );

// everything eventually comes to here to render a font...
// this should therefore cache this information with the fonts
// it has created...


void InternalRenderFontCharacter( PFONT_RENDERER renderer, PFONT font, INDEX idx )
{
	if( font && font->character[idx] )
		return;
	if( !renderer )
	{
		INDEX idx;
		if( font )
			LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
			{
				if( renderer->font == font )
					break;
			}
		if( !renderer )
		{
			lprintf( WIDE("Couldn't find by font either...") );
			DebugBreak();
			return;
		}
	}
	EnterCriticalSec( &fg.cs );
	if( !renderer->font->character[idx] )
	{
		FT_Face face = renderer->face;
		int glyph_index = FT_Get_Char_Index( face, (FT_ULong)idx );
		//lprintf( WIDE("Character %d is glyph %d"), idx, glyph_index );
		FT_Load_Glyph( face
						 , glyph_index
						 , 0
						  | FT_LOAD_FORCE_AUTOHINT
						 );

		//lprintf( "advance and height... %d %d", ( face->glyph->linearVertAdvance>>16 ), renderer->font->height );
		if( ( face->glyph->linearVertAdvance>>16 ) > renderer->font->height )
		{
			renderer->font->height = (short)(face->glyph->linearVertAdvance>>16);
			renderer->font->baseline = renderer->max_ascent +
				( ( renderer->font->height
					- ( renderer->max_ascent - renderer->min_descent ) )
				 / 2 );
			if( 0 )
				lprintf( WIDE("Result baseline %d   %d,%d"), renderer->font->baseline, renderer->max_ascent, renderer->min_descent );

		}
			switch( face->glyph->format )
			{
			case FT_GLYPH_FORMAT_OUTLINE:
				//Log( WIDE("Outline render glyph....") );
				if( (( renderer->flags & 3 )<2) )
					FT_Render_Glyph( face->glyph, FT_RENDER_MODE_MONO );
				else
					FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
#ifdef ft_glyph_format_bitmap
			case FT_GLYPH_FORMAT_BITMAP:
#else
			case ft_glyph_format_bitmap:
#endif
				/*
				Log2( WIDE("bitmap params: left %d top %d"), face->glyph->bitmap_left, face->glyph->bitmap_top );
				Log6( WIDE("Glyph Params: %d(%d) %d(%d) %d %d")
					 , face->glyph->linearHoriAdvance
					 , face->glyph->linearHoriAdvance>>16
					 , face->glyph->linearVertAdvance
					 , face->glyph->linearVertAdvance>>16
					  , face->glyph->advance.x
					 , face->glyph->advance.y );
					 Log3( WIDE("bitmap height: %d ascent: %d descent: %d"), face->height>>6, face->ascender>>6, face->descender>>6);
					 */
				//Log( WIDE("Bitmap") );
				if( ( renderer->flags & 3 )<2 )
				{
					RenderMonoChar( renderer->font
								 , idx
								 , &face->glyph->bitmap
								 , &face->glyph->metrics
								 , face->glyph->bitmap_left // left offset?
								 , face->glyph->linearHoriAdvance>>16
									  );
				}
				else
				{
					RenderGreyChar( renderer->font
									  , idx
									  , &face->glyph->bitmap
									  , &face->glyph->metrics
									  , face->glyph->bitmap_left // left offset?
									  , face->glyph->linearHoriAdvance>>16
									  , ((renderer->flags & 3)==3)?8:2 );
				}
				break;
			//case FT_GLYPH_FORMAT_COMPOSITE:
			//	Log( WIDE("unsupported Composit") );
			//	break;
			default:
				renderer->font->character[idx] = NULL;
				Log1( WIDE("unsupported Unknown format(%4.4s)"), (char*)&face->glyph->format );
				break;
			}
		}
		for( idx = 0; idx < 256; idx++ )
			if( renderer->font->character[idx] )
				break;
		if( idx == 256 )
		{
			renderer->ResultFont = NULL;
		}
		else
		{
			renderer->ResultFont = LoadFont( (SFTFont)renderer->font );
		}
		LeaveCriticalSec( &fg.cs );
	}

static SFTFont DoInternalRenderFontFile( PFONT_RENDERER renderer )
{
	int error;

	{
		if( !renderer->font )
		{
			lprintf( WIDE("Need font resource (once)") );
			renderer->font = NewPlus( FONT, 255 * sizeof(PCHARACTER) );
			// this is okay it's a kludge so some other code worked magically
			// it's redundant and should be dleete...
			renderer->ResultFont = (SFTFont)renderer->font;
			MemSet( renderer->font, 0, sizeof( FONT )+ 255 * sizeof(PCHARACTER) );
		}

		{
			INDEX idx;
			PFONT font;
			font = (PFONT)renderer->ResultFont;

			renderer->max_ascent = 0;
			renderer->min_descent = 0;

			font->characters = 256;
			font->baseline = 0;
			if( ( renderer->flags & 3 ) == 3 )
				font->flags = FONT_FLAG_8BIT;
			else if( ( renderer->flags & 3 ) == 2 )
				font->flags = FONT_FLAG_2BIT;
			else
				font->flags = FONT_FLAG_MONO;
			// default rendering for one-to-one pixel sizes.
			/*
			 error = FT_Set_Char_Size( face
			 , nWidth << 6
			 , nHeight << 6
			 , 96
			 , 96 );
			 */
			if( 0 )
				lprintf( WIDE("Setting pixel size %d %d"), ScaleValue( &renderer->width_scale, renderer->nWidth )
						 , ScaleValue( &renderer->height_scale, renderer->nHeight ) );

			error = FT_Set_Pixel_Sizes( renderer->face
											  , ScaleValue( &renderer->width_scale, renderer->nWidth )
											  , ScaleValue( &renderer->height_scale, renderer->nHeight ) );
			if( error )
			{
				Log1( WIDE("Fault setting char size - %d"), error );
			}
			font->height = 0; //CEIL(face->size->metrics.height);
			font->name = StrDup( renderer->fontname );
			InternalRenderFontCharacter( renderer, NULL, 0 );
			for( idx = 0; idx < 256; idx++ )
			{
				FT_Face face = renderer->face;
				int glyph_index = FT_Get_Char_Index( face, (FT_ULong)idx );
				//lprintf( WIDE("Character %d is glyph %d"), idx, glyph_index );
				FT_Load_Glyph( face
								 , glyph_index
								 , 0
								  | FT_LOAD_FORCE_AUTOHINT
								 );
				{
					int ascent = CEIL(face->glyph->metrics.horiBearingY);
					int descent;
					if( face->glyph->metrics.height )
					{
						descent = -CEIL(face->glyph->metrics.height - face->glyph->metrics.horiBearingY) + 1;
					}
					else
					{
						descent = CEIL( face->glyph->metrics.horiBearingY ) /*- font->height + */ + 1;
					}

					// done when the font is initially loaded
					// loading face characteristics shouldn't matter
					if( ascent > renderer->max_ascent )
					{
						renderer->max_ascent = ascent;
						renderer->font->baseline = renderer->max_ascent +
							( ( renderer->font->height
								- ( renderer->max_ascent - renderer->min_descent ) )
							 / 2 );
						if( 0 )
							lprintf( WIDE("Result baseline %d   %d,%d"), renderer->font->baseline, renderer->max_ascent, renderer->min_descent );
					}
					if( descent < renderer->min_descent )
					{
						renderer->min_descent = descent;
						renderer->font->baseline = renderer->max_ascent +
							( ( renderer->font->height
								- ( renderer->max_ascent - renderer->min_descent ) )
							 / 2 );
						if( 0 )
							lprintf( WIDE("Result baseline %d   %d,%d"), renderer->font->baseline, renderer->max_ascent, renderer->min_descent );
					}
				}
				{
					if( ( face->glyph->linearVertAdvance>>16 ) > renderer->font->height )
					{
						renderer->font->height = (short)(face->glyph->linearVertAdvance>>16);
						renderer->font->baseline = renderer->max_ascent +
							( ( renderer->font->height
								- ( renderer->max_ascent - renderer->min_descent ) )
							 / 2 );
						if( 0 )
							lprintf( WIDE("Result baseline %d   %d,%d"), renderer->font->baseline, renderer->max_ascent, renderer->min_descent );

					}
				}
			}
		}
	}
	return renderer->ResultFont;
}

SFTFont InternalRenderFontFile( CTEXTSTR file
										, S_32 nWidth
										, S_32 nHeight
										, PFRACTION width_scale
										, PFRACTION height_scale
										, _32 flags // whether to render 1(0/1), 2(2), 8(3) bits, ...
										)
{
	int error;
	int bDefaultFile = 0;
	int face_idx = 0;
	int num_faces = 0;
	PFONT_RENDERER renderer;

	//static CRITICALSECTION fg.cs;
	EnterCriticalSec( &fg.cs );
	//if( !InitFont() )
	{
		//lprintf( WIDE("Failed to init fonts.. failing font render.") );
		//LeaveCriticalSec( &fg.cs );
	//	return NULL;
	}

	// with the current freetype library which has been grabbed
	// width and height more than these cause errors.
	if( nWidth > 227 ) nWidth = 227;
	if( nHeight > 227 ) nHeight = 227;

try_another_default:	
	if( !file || bDefaultFile )
	{
		switch( bDefaultFile )
		{
		case 0:  	
			file = WIDE("arialbd.ttf"); 	  	
			break;
		case 1:
			file = WIDE("fonts/arialbd.ttf");
			break;
		case 2:
			 lprintf( WIDE("default font arialbd.ttf or fonts/arialbd.ttf did not load.") );
			LeaveCriticalSec( &fg.cs );
			return NULL;
		}
		bDefaultFile++;
	}
	{
		INDEX idx;
		LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
		{
			if( renderer->nWidth == nWidth
				&& renderer->nHeight == nHeight
				&& renderer->flags == flags
				&& strcmp( renderer->file, file ) == 0 )
			{
				break;
			}
		}
		if( !renderer )
		{
			renderer = New( FONT_RENDERER );
			MemSet( renderer, 0, sizeof( FONT_RENDERER ) );

			renderer->nWidth = (S_16)nWidth;
			renderer->nHeight = (S_16)nHeight;

			if( width_scale)
				renderer->width_scale = width_scale[0];
			else
				SetFraction( renderer->width_scale, 1, 1 );

			if( height_scale)
				renderer->height_scale = height_scale[0];
			else
            SetFraction( renderer->height_scale, 1, 1 );
			renderer->flags = flags;
			renderer->file = sack_prepend_path( 0, file );
			AddLink( &fonts, renderer );
		}
		else
		{
			//lprintf( WIDE("using existing renderer for this font...") );
		}
	}

	do
	{
		// this needs to be post processed to
		// center all characters in a font - favoring the
		// bottom...

		if( !renderer->face )
		{
			error = OpenFontFile( renderer->file, &renderer->font_memory, &renderer->face, face_idx, TRUE );
#if rotation_was_italic
			if( renderer->flags & FONT_FLAG_ITALIC )
			{         
				FT_Matrix matrix;         
				const float angle = 30 * (-M_PI) / 180.0f;         
				matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );         
				matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );         
				matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );         
				matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );         
				FT_Set_Transform(renderer->face,&matrix,0);     
			} 
			else 
			{         
				//FT_Set_Transform(renderer->face,0,0);     
			} 
#endif
			if( renderer->flags & FONT_FLAG_ITALIC )
			{
				FT_Matrix mat;
				mat.xx = (FT_Fixed)(1 * (1<<16));
				mat.xy = (FT_Fixed)(0.5 * (1<<16));
				mat.yx = 0;
				mat.yy = 1 << 16;
				FT_Set_Transform(renderer->face, &mat, 0 );
			}

			//if( renderer->flags & FONT_FLAG_ITALIC )
			//$	((FT_FaceRec*)renderer->face)->style_flags |= FT_STYLE_FLAG_ITALIC;
			if( renderer->flags & FONT_FLAG_BOLD )
				((FT_FaceRec*)renderer->face)->style_flags |= FT_STYLE_FLAG_BOLD;

		}
		else
		{
			error = 0;
		}

		if( !renderer->face || error )
		{
		  //DebugBreak();
			//lprintf( WIDE("Failed to load font...Err:%d %s %d %d"), error, file, nWidth, nHeight );
			DeleteLink( &fonts, renderer );
			Deallocate( POINTER, renderer->file );
			Deallocate( POINTER, renderer );
			if( bDefaultFile )
					goto try_another_default;
			LeaveCriticalSec( &fg.cs );
			return NULL;
		}

		if( !renderer->fontname[0] )
		{
			FT_Face face = renderer->face;
#ifdef _UNICODE
			{
				TEXTSTR tmp1 = CharWConvert( face->family_name?face->family_name:"No-Name" );
				TEXTSTR tmp2 = CharWConvert( face->style_name?face->style_name:"normal" );
				snprintf( renderer->fontname, 256, WIDE("%s%s%s%dBy%d")
						  , tmp1
						  , tmp2
						  , (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)?WIDE("fixed"):WIDE("")
						  , nWidth, nHeight
						  );
				Deallocate( TEXTSTR, tmp1 );
				Deallocate( TEXTSTR, tmp2 );
			}
#else
			snprintf( renderer->fontname, 256, WIDE("%s%s%s%dBy%d")
					  , face->family_name?face->family_name:WIDE("No-Name")
					  , face->style_name?face->style_name:WIDE("normal")
					  , (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)?WIDE("fixed"):WIDE("")
					  , nWidth, nHeight
					  );
#endif
			KillSpaces( renderer->fontname );
		}

		DoInternalRenderFontFile( renderer );

	}
	while( ( face_idx < num_faces ) && !renderer->font );
	//FT_Done_Face( face );
	//if( renderer->font_memory )
	//	Deallocate( POINTER, renderer->font_memory );

	// just for grins - check all characters defined for font
	// to see if their ascent - descent is less than height
	// and that ascent is less than baseline.
	// however, if we have used a LoadFont function
	// which has replaced the font thing with a handle...
	//DumpFontFile( WIDE("font.h"), font );
	LeaveCriticalSec( &fg.cs );
	return renderer->ResultFont;;
}


int RecheckCache( CTEXTSTR entry, _32 *pe
					 , CTEXTSTR style, _32 *ps
					 , CTEXTSTR filename, _32 *psf );

SFTFont RenderScaledFontData( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale )
{
#define fname(base) base
#define sname(base) fname(base) + (strlen(base)+1)
#define fsname(base) sname(base) + strlen(sname(base)+1)
	extern _64 fontcachetime;
	if( !pfd )
		return NULL;
	LoadAllFonts(); // load cache data so we can resolve user data
	if( pfd->magic == MAGIC_PICK_FONT )
	{
		CTEXTSTR name1 = pfd->names;
		CTEXTSTR name2 = pfd->names + StrLen( pfd->names ) + 1;
		CTEXTSTR name3 = name2 + StrLen( name2 ) + 1;

		lprintf( WIDE("[%s][%s][%s]"), name1, name2, name3 );

		if( pfd->cachefile_time == fontcachetime ||
			RecheckCache( name1/*fname(pfd->names)*/, &pfd->nFamily
							, name2/*sname(pfd->names)*/, &pfd->nStyle
							, name3/*fsname(pfd->names)*/, &pfd->nFile ) )
		{
			SFTFont return_font = InternalRenderFont( pfd->nFamily
																 , pfd->nStyle
																 , pfd->nFile
																 , pfd->nWidth
																 , pfd->nHeight
																 , width_scale
																 , height_scale
																 , pfd->flags );
			pfd->cachefile_time = fontcachetime;
			return return_font;
		}
	}
	else if( pfd->magic == MAGIC_RENDER_FONT )
	{
		PRENDER_FONTDATA prfd = (PRENDER_FONTDATA)pfd;
		PFONT_ENTRY pfe = NULL;
		int family_idx;
		lprintf( WIDE("Rendered font... not picked.") );
		for( family_idx = 0; SUS_LT( family_idx, int, fg.nFonts, _32 ); family_idx++ )
		{
			pfe = fg.pFontCache + family_idx;
			if( StrCaseCmp( pfe->name, prfd->filename ) == 0 )
				break;
		}
		if( pfe )
			return InternalRenderFont( family_idx, 0, 0
											 , prfd->nWidth
											 , prfd->nHeight
											 , width_scale
											 , height_scale
											 , prfd->flags );
		else
			return InternalRenderFontFile( prfd->filename
												  , prfd->nWidth
												  , prfd->nHeight
												  , width_scale
												  , height_scale
												  , prfd->flags );
	}
	LogBinary( pfd, 32 );
	lprintf( WIDE("Font parameters are no longer valid.... could not be found in cache issue font dialog here?") );
	return NULL;
}

SFTFont InternalRenderFont( _32 nFamily
								  , _32 nStyle
								  , _32 nFile
								  , S_32 nWidth
								  , S_32 nHeight
								  , PFRACTION width_scale
								  , PFRACTION height_scale
								  , _32 flags
								  )
{
	PFONT_ENTRY pfe = fg.pFontCache + nFamily;
	TEXTCHAR name[256];
	POINTER result = NULL;
	INDEX nAlt = 0;
	//if( !InitFont() )
	//	return NULL;
	if( nFamily < fg.nFonts &&
		nStyle < pfe->nStyles &&
		nFile < pfe->styles[nStyle].nFiles )
	{
	snprintf( name, sizeof( name ), WIDE("%s/%s")
			  , pfe->styles[nStyle].files[nFile].path
			  , pfe->styles[nStyle].files[nFile].file );

	//Log1( WIDE("Font: %s"), name );
	do
	{
		if( nAlt )
		{
			snprintf( name, sizeof( name ), WIDE("%s/%s")
					  , pfe->styles[nStyle].files[nFile].pAlternate[nAlt-1].path
					  , pfe->styles[nStyle].files[nFile].pAlternate[nAlt-1].file );
		}
		// this is called from pickfont, and we dont' have font space to pass here...
		// it's taken from dialog parameters...
		if( pfe->styles[nStyle].flags.italic )
			flags |= FONT_FLAG_ITALIC;
		if( pfe->styles[nStyle].flags.bold )
			flags |= FONT_FLAG_BOLD;

		result = InternalRenderFontFile( name, nWidth, nHeight, width_scale, height_scale, flags );
		// this may not be 'wide' enough...
		// there may be characters which overflow the
		// top/bottomm of the cell... probably will
		// ignore that - but if we DO find such a font,
		// maybe this can be adjusted -- NORMALLY
		// this will be sufficient...
	} while( (nAlt++ < pfe->styles[nStyle].files[nFile].nAlt) && !result );
	}
	return (SFTFont)result;
}

void DestroyFont( SFTFont *font )
{
	if( font )
	{
		if( *font )
		{
			xlprintf( LOG_ALWAYS )( WIDE("font destruction is NOT implemented, this WILL result in a memory leak!!!!!!!") );
			(*font) = NULL;
		}
	}

	// need to find this resource...
	//if( renderer->font_memory )
	//	Deallocate( POINTER, renderer->font_memory );

// release resources of this font...
	// (also in the image library which may be a network service thing)
}

//-------------------------------------------------------------------------

int RecheckCache( CTEXTSTR entry, _32 *pe
					 , CTEXTSTR style, _32 *ps
					 , CTEXTSTR filename, _32 *psf )
{
	INDEX n;
	for( n = 0; n < fg.nFonts; n++ )
	{
		//lprintf( "(1) is %s==%s", fg.pFontCache[n].name, entry );
		if( strcmp( fg.pFontCache[n].name, entry ) == 0 )
		{
			INDEX s;
			for( s = 0; s < fg.pFontCache[n].nStyles; s++ )
			{
				//lprintf( "  (2) is %s==%s", fg.pFontCache[n].styles[s].name, style );
				if( strcmp( fg.pFontCache[n].styles[s].name, style ) == 0 )
				{
					INDEX sf;
					for( sf = 0; sf < fg.pFontCache[n].styles[s].nFiles; sf++ )
					{
						CTEXTSTR file = pathrchr( filename );
						if( file )
							file++;
						else
							file = filename;
						//lprintf( "    (3) is %s==%s (%s)", fg.pFontCache[n].styles[s].files[sf].file, file, filename );
						if( strcmp( fg.pFontCache[n].styles[s].files[sf].file, file ) == 0 )
						{
							(*pe) = (_32)n;
							(*ps) = (_32)s;
							(*psf) = (_32)sf;
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

void SetFontRendererData( SFTFont font, POINTER pResult, size_t size )
{
	PFONT_RENDERER renderer;
	INDEX idx;
	LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
	{
		if( renderer->ResultFont == font )
			break;
	}
	if( renderer )
	{
		Hold( pResult ); // make sure we hang onto this... I think applications are going to release it.
		renderer->ResultData = pResult;
		renderer->ResultDataLen = size;
	}
}

SFTFont RenderFontFileScaledEx( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t * size, POINTER *pFontData )
{
	SFTFont font = InternalRenderFontFile( file, (S_16)(width&0x7FFF), (S_16)(height&0x7FFF), width_scale, height_scale, flags );
	if( font && size && pFontData )
	{
		size_t chars;
		PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*size) =
																			  (chars = strlen( file ) + 1)*sizeof(TEXTCHAR) );
      (*size) += sizeof( RENDER_FONTDATA );
		pResult->magic = MAGIC_RENDER_FONT;
		pResult->nHeight = height;
		pResult->nWidth = width;
		pResult->flags = flags;
		StrCpyEx( pResult->filename, file, chars );
		(*pFontData) = (POINTER)pResult;
		SetFontRendererData( font, pResult, (*size) );
	}
	return font;
}
#undef RenderFontFileEx
SFTFont RenderFontFileEx( CTEXTSTR file, _32 width, _32 height, _32 flags, size_t * size, POINTER *pFontData )
{
   return RenderFontFileScaledEx( file, width, height, NULL, NULL, flags, size, pFontData );
}

#undef RenderFontFile
SFTFont RenderFontFile( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags )
{
	return InternalRenderFontFile( file, width, height, width_scale, height_scale, flags );
}

SFTFont RenderScaledFontEx( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData )
{
	SFTFont font;
	PFONT_ENTRY pfe;
	INDEX family_idx;

	LoadAllFonts(); // load cache data so we can resolve user data
	for( family_idx = 0; family_idx < fg.nFonts; family_idx++ )
	{
		pfe = fg.pFontCache + family_idx;
		if( StrCaseCmp( pfe->name, name ) == 0 )
			break;
	}
	if( family_idx < fg.nFonts )
	{
		SFTFont return_font = InternalRenderFont( (_32)family_idx
														 , 0
															 , 0
															 , width
															 , height
														 , width_scale
														 , height_scale
														 , flags );
		{
			size_t chars;
			PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*pnFontDataSize)
																					= 
															    (chars = strlen( name ) + 1)*sizeof(TEXTCHAR) );
         (*pnFontDataSize) += sizeof( RENDER_FONTDATA );
			pResult->magic = MAGIC_RENDER_FONT;
			pResult->nHeight = height;
			pResult->nWidth = width;
			pResult->flags = flags;
			StrCpyEx( pResult->filename, name, chars );
			(*pFontData) = (POINTER)pResult;
			SetFontRendererData( return_font, pResult, (*pnFontDataSize) );
		}
		return return_font;
	}

	font = InternalRenderFontFile( name
										  , width
										  , height
										  , width_scale
										  , height_scale
										  , flags );
	{
		size_t chars;
		PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*pnFontDataSize) =
																			   (chars = strlen( name ) + 1)*sizeof(TEXTCHAR) );
      (*pnFontDataSize) += sizeof( RENDER_FONTDATA );
		pResult->magic = MAGIC_RENDER_FONT;
		pResult->nHeight = height;
		pResult->nWidth = width;
		pResult->flags = flags;
		StrCpyEx( pResult->filename, name, chars );
		(*pFontData) = (POINTER)pResult;
		SetFontRendererData( font, pResult, (*pnFontDataSize) );
	}
	return font;
}

#undef RenderScaledFont
SFTFont RenderScaledFont( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags )
{
	return RenderScaledFontEx(name,width,height,width_scale,height_scale,flags,NULL,NULL);
}


int GetFontRenderData( SFTFont font, POINTER *fontdata, size_t *fontdatalen )
{
	// set pointer and _32 datalen passed by address
	// with the font descriptor block that was used to
	// create the font (there's always something like that)
	PFONT_RENDERER renderer = NULL;
	INDEX idx;
	if( font )
	{
		LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
		{
			if( renderer->font == (PFONT)font )
				break;
		}
	}
	if( renderer )
	{
		(*fontdata) = renderer->ResultData;
		(*fontdatalen) = renderer->ResultDataLen;
		return TRUE;
	}
	return FALSE;
}

void RerenderFont( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale )
{
	PFONT_RENDERER renderer;
	INDEX idx;
	EnterCriticalSec( &fg.cs );

	LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
	{
		if( renderer->font == font )
		{
			int n;
			for( n = 0; n < 256; n++ )
			{
				if( renderer->font->character[n] )
				{
					Deallocate( PCHARACTER, renderer->font->character[n] );
					renderer->font->character[n] = NULL;
				}
			}

			renderer->nLinesUsed = 0;

			if( width )
				renderer->nWidth = (S_16)width;
			if( height )
				renderer->nHeight = (S_16)height;

			if( width_scale)
				renderer->width_scale = width_scale[0];
			else
				SetFraction( renderer->width_scale, 1, 1 );

			if( height_scale)
				renderer->height_scale = height_scale[0];
			else
				SetFraction( renderer->height_scale, 1, 1 );

			DoInternalRenderFontFile( renderer );
			LeaveCriticalSec( &fg.cs );
			return;
		}
	}
	LeaveCriticalSec( &fg.cs );
}

#endif

IMAGE_NAMESPACE_END

