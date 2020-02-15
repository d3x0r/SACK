
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include <idle.h>
#define IMAGE_LIBRARY_SOURCE
#include <ft2build.h>
#include <../src/contrib/freetype-2.8/include/freetype/tttables.h>
//#include <tttags.h>
#ifdef FT_BEGIN_HEADER

#ifdef _OPENGL_DRIVER
#  if defined( USE_GLES )
#    include <GLES/gl.h>
#  elif defined( USE_GLES2 )
//#include <GLES/gl.h>
#    include <GLES2/gl2.h>
#  else
#    define USE_OPENGL
// why do we need this anyway?
//#    include <GL/glew.h>
#    include <GL/gl.h>         // Header File For The OpenGL32 Library
#  endif
#endif

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
#include <render3d.h>
#define REQUIRE_GLUINT

#include "image_common.h"
#include <render.h> // need definition for OnDisplayConnect to redownload font

IMAGE_NAMESPACE

#define SYMBIT(bit)  ( 1 << (bit&0x7) )

FILE *IMGVER(output);
#define output IMGVER(output)

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
			Log1( "Free type init failed: %d", error );
			return ;
		}
	}
}

//-------------------------------------------------------------------------

static void PrintLeadinJS( CTEXTSTR name, SFTFont font, int bits )
{
	//sack_fprintf( output, "var font = {};\n" );
	sack_fprintf( output, "var font = { \nheight:%d, baseline:%d, flags:%d, characters:["
	       , font->height 
	       , font->baseline
	       , font->flags
	       );

}

static void PrintLeadin( CTEXTSTR name, SFTFont font, int bits )
{
	sack_fprintf( output, "#include <vectlib.h>\n" );
	sack_fprintf( output, "#undef _X\n" );

	if( bits == 2)
		sack_fprintf( output, "#define BITS_GREY2\n" );
	if( bits != 8 )
		sack_fprintf( output, "#include \"symbits.h\"\n" );

	sack_fprintf( output, "IMAGE_NAMESPACE\n" );
	sack_fprintf( output, "	typedef struct font_tag *PFONT ;\n" );
	sack_fprintf( output, "#ifdef __cplusplus \n" );
	sack_fprintf( output, "	namespace default_font {\n" );
	sack_fprintf( output, "#endif\n" );
	sack_fprintf( output, "\n" );
	sack_fprintf( output, "#define EXTRA_STRUCT  struct ImageFile_tag *cell; RCOORD x1, x2, y1, y2; struct font_char_tag *next_in_line;\n" );
	sack_fprintf( output, "#define EXTRA_INIT  0,0,0,0,0,0\n" );

	sack_fprintf( output, "typedef struct font_char_tag *PCHARACTER;\n" );
}

static int PrintCharJS( int bits, int charnum, PCHARACTER character, int height )
{
	int  outwidth;
	TEXTCHAR charid[64];
	char *data = character?(char*)character->data:0;
	tnprintf( charid, sizeof( charid ), "_char_%d", charnum );

	#define LINEPAD "                  "

	if( !character )
		sack_fprintf( output, "%snull\n", charnum?",":""  );
	else
	{
		if( bits == 8 )
			outwidth = character->size; // round up to next byte increments size.
		else if( bits == 2 )
			outwidth = ((character->size+3) & 0xFC )/8; // round up to next byte increments size.
		else
			outwidth = ((character->size+7) & 0xF8 )/8; // round up to next byte increments size.

		sack_fprintf( output, "%s{sz:%d,w:%d,ofs:%d,asc:%d,dsc:%d"
					, charnum?",":""
							, character->size
							, character->width
							, (signed)character->offset
							, character->ascent
							, character->descent
							);

		if( character->size )
			sack_fprintf( output, ",data:new Uint8Array([" );

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
					sack_fprintf( output, "," );
				}
				dataline = data;
				{
					for( bit = 0; bit < outwidth; bit++ )
					{
						if( bit )
							sack_fprintf( output, "," );
						sack_fprintf( output, "%u", (unsigned)((unsigned char*)dataline)[bit] );
					}
				}

				// fill with trailing 0's
				data += outwidth;
			}
		}
		if( character->size )
			sack_fprintf( output, "]) " );
		sack_fprintf( output, "}\n" );
	}

	return 0;
}


static int PrintChar( int bits, int charnum, PCHARACTER character, int height )
{
	int  outwidth;
	TEXTCHAR charid[64];
	char *data;
	if( !character ) return 0;
	data = (char*)character->data;
	tnprintf( charid, sizeof( charid ), "_char_%d", charnum );

	#define LINEPAD "                  "
	if( bits == 8 )
		outwidth = character->size; // round up to next byte increments size.
	else if( bits == 2 )
		outwidth = ((character->size+3) & 0xFC ); // round up to next byte increments size.
	else
		outwidth = ((character->size+7) & 0xF8 ); // round up to next byte increments size.

	if( ((outwidth) / (8 / bits))*((character->ascent - character->descent) + 1) )
		sack_fprintf( output, "static struct{ char s, w, o, j; short a, d; uint32_t rf; EXTRA_STRUCT unsigned char data[%d]; } %s =\n",
						((outwidth)/(8/bits))*( ( character->ascent - character->descent ) + 1 )
						, charid );
	else
		sack_fprintf( output, "static struct{ char s, w, o, j; short a, d; EXTRA_STRUCT } %s =\n"
						, charid );

	sack_fprintf( output, "{ %2d, %2d, %2d, 0, %2d, %2d, %d EXTRA_INIT "
							, character->size
							, character->width
							, (signed)character->offset
							, character->ascent
							, character->descent
							, (bits == 8 ) ?FONT_FLAG_8BIT:(bits==2)?FONT_FLAG_2BIT:FONT_FLAG_MONO
							);

	if( character->size )
		sack_fprintf( output, ", { \n" LINEPAD );

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
					sack_fprintf( output, ", // <---- baseline\n" LINEPAD );
				else
					sack_fprintf( output, ",\n" LINEPAD );
			}
			dataline = data;
			if( bits == 1 )
			{
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit && ((bit % 8) == 0 ) )
						sack_fprintf( output, "," );
					if( dataline[bit >> 3] & SYMBIT(bit) )
					{
						sack_fprintf( output, "X" );
					}
					else
						sack_fprintf( output, "_" );
				}
			}
			else if( bits == 2)
			{
				/*
				for( bit = 0; bit < (character->size+(3))/(8/bits); bit++ )
				{
					sack_fprintf( output, "%02x ", dataline[bit] );
					}
					*/
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit && ((bit % 4) == 0 ) )
						sack_fprintf( output, "," );
					//sack_fprintf( output, "%02x ", (dataline[bit >> 2] >> (2*(bit&0x3))) & 3 );
					switch( (dataline[bit >> 2] >> (2*(bit&0x3))) & 3 )
					{
					case 3:
						sack_fprintf( output, "X" );
						break;
					case 2:
						sack_fprintf( output, "O" );
						break;
					case 1:
						sack_fprintf( output, "o" );
						break;
					case 0:
						sack_fprintf( output, "_" );
						break;
					}
				}
			}
			else if( bits == 8 )
			{
				for( bit = 0; bit < character->size; bit++ )
				{
					if( bit )
						sack_fprintf( output, "," );
					sack_fprintf( output, "%3u", (unsigned)((unsigned char*)dataline)[bit] );
				}

			}

			// fill with trailing 0's
			if( bits < 8 )
			{
				for( ; bit < outwidth; bit++ )
					sack_fprintf( output, "_" );
			}
			data += (character->size+(bits==8?0:bits==2?3:7))/(8/bits);
		}
	}

	if( character->size )
		sack_fprintf( output, "]) " );
	sack_fprintf( output, "};\n\n\n" );
	return 0;
}


static void PrintFontTable( CTEXTSTR name, PFONT font )
{
	int idx;
	uint32_t i, maxwidth;
	idx = 0;
	maxwidth = 0;
	for( i = 0; i < font->characters; i++ )
	{
		if( font->character[i] )
			if( font->character[i]->width > maxwidth )
				maxwidth = font->character[i]->width;
	}
	sack_fprintf( output, "struct { unsigned short height, baseline, chars; unsigned char flags, junk;\n"
				"         char *fontname;\n"
			  "         PCHARACTER character[%d]; }\n"
			  "#ifdef __cplusplus\n"
			  "       ___%s\n"
			  "#else\n"
			  "       __%s\n"
			  "#endif\n"
			  "= { \n%d, %d, %d, %d, 0, \"%s\", {"
	       , font->characters
	       , name
	       , name
	       , font->height 
	       , font->baseline
	       , font->characters
	       , font->flags
	       , name
	       );

	for( i = 0; i < font->characters; i++ )
	{
		if( font->character[i] )
			sack_fprintf( output, " %c(PCHARACTER)&_char_%d\n", (i)?',':' ', i );
		else
			sack_fprintf( output, " %cNULL\n", (i)?',':' ' );

	}
	sack_fprintf( output, "\n} };" );

	sack_fprintf( output, "\n" );
	sack_fprintf( output, "#ifdef __cplusplus\n" );
	sack_fprintf( output, "PFONT __%s = (PFONT)&___%s;\n", name, name );
	sack_fprintf( output, "#endif\n" );

}


static void PrintFontTableJS( CTEXTSTR name, PFONT font )
{

}

static void PrintFooterJS( void )
{
	sack_fprintf( output, "\n] };" );
	sack_fprintf( output, "\n" );

}

static void PrintFooter( void )
{
	sack_fprintf( output, "#ifdef __cplusplus\n" );
	sack_fprintf( output, "      }; // default_font namespace\n" );
	sack_fprintf( output, "#endif\n" );
	sack_fprintf( output, "IMAGE_NAMESPACE_END\n" );

}

//-------------------------------------------------------------------------

void IMGVER(DumpFontFile)( CTEXTSTR name, SFTFont font_to_dump )
{
	PFONT font = (PFONT)font_to_dump;
	int JS = 0;
	void( *printLeadin )(CTEXTSTR name, SFTFont font, int bits);
	int( *printChar )(int bits, int charnum, PCHARACTER character, int height);
	void (*printFooter)( void );
	void( *printFontTable )(CTEXTSTR name, PFONT font);

		if( JS )
		{
			printLeadin = PrintLeadinJS;
			printChar = PrintCharJS;
			printFooter = PrintFooterJS;
			printFontTable = PrintFontTableJS;
		}
		else {
			printLeadin = PrintLeadin;
			printChar = PrintChar;
			printFooter = PrintFooter;
			printFontTable = PrintFontTable;
		}
	if( font )
	{
		output = sack_fopen( 0, name, "wt" );
		if( output )
		{
			uint32_t charid;
			for	( charid = 0; charid < font_to_dump->characters; charid++ )
			{
				void IMGVER(InternalRenderFontCharacter)( PFONT_RENDERER renderer, PFONT font, INDEX idx );
				IMGVER(InternalRenderFontCharacter)( NULL, font_to_dump, charid );
			}
			printLeadin(  name, font, (font->flags&3)==FONT_FLAG_2BIT?2
							: (font->flags&3)==FONT_FLAG_8BIT?8
							: 1  );
					//		(font->flags&3)==FONT_FLAG_2BIT?2
					//		: (font->flags&3)==FONT_FLAG_8BIT?8
				//			: 1 );
			{
				for	( charid = 0; charid < font_to_dump->characters; charid++ )
				{
					PCHARACTER character;
					void IMGVER(InternalRenderFontCharacter)( PFONT_RENDERER renderer, PFONT font, INDEX idx );
					IMGVER(InternalRenderFontCharacter)( NULL, font_to_dump, charid );
					character = font->character[charid];
						PrintChar( (font->flags&3) == FONT_FLAG_8BIT?8
									 :(font->flags&3) == FONT_FLAG_2BIT?2
									 :1
									, charid, character, font->height );
						//if( charid > 40 )
						//	DebugDumpMem();
				}
			}
			printFooter();
			printFontTable( font->name, font );
			sack_fclose( output );
		}
	}
	else
		Log( "No font to dump..." );
}

//-------------------------------------------------------------------------

struct font_renderer_tag {
	TEXTCHAR *file;
	int32_t nWidth;
	int32_t nHeight;
	FRACTION width_scale;
	FRACTION height_scale;
	uint32_t flags; // whether to render 1(0/1), 2(2), 8(3) bits, ...
	POINTER ResultData; // data that was resulted for creating this font.
	size_t ResultDataLen;
	POINTER font_memory;
	size_t font_memory_size;
	FT_Face face;
	FT_Glyph glyph;
	FT_GlyphSlot slot;
	SFTFont ResultFont;
	PFONT font;
	char fontname[256];

	int max_ascent;
	int min_descent;
	// -- - - - -- - - This bit of structure is for character renedering to an image which is downloaded for bitmap fonts
	//
	// full surface, contains all characters.  Starts as 3 times the first character (9x vertical and horizontal)
	// after that it is expanded 2x (4x total, 2x horizontal 2x rows )
	Image surface;
	PLIST priorSurfaces;
	int nLinesUsed;
	int nLinesAvail;
	int *pLineStarts; // counts highest character in a line, each line walks across
	PCHARACTER *ppCharacterLineStarts; // beginning of chain of characters on this image.
	PIMAGE_INTERFACE reverse_interface;
} ;
typedef struct font_renderer_tag FONT_RENDERER;

static PLIST fonts;
static LOGICAL cleanFonts;

#ifdef USE_API_ALIAS_PREFIX
#  define FNTRENDER_DISPLAYCONNECT_NAME__(a) a
#  define FNTRENDER_DISPLAYCONNECT_NAME_(n)   FNTRENDER_DISPLAYCONNECT_NAME__("@00 Image Core" #n)
#  define FNTRENDER_DISPLAYCONNECT_NAME FNTRENDER_DISPLAYCONNECT_NAME_(USE_API_ALIAS_PREFIX)
#else
#  define FNTRENDER_DISPLAYCONNECT_NAME
#endif

static void OnDisplayConnect( FNTRENDER_DISPLAYCONNECT_NAME )( struct display_app*app, struct display_app_local ***pppLocal )
{
	INDEX idx;
	PFONT_RENDERER renderer;
	LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
	{
		int n;
		// this isn't set until an image is created to reuse..
		if( renderer->reverse_interface )
			renderer->reverse_interface->_ReuseImage( (Image)renderer->surface );
		for( n = 0; n < renderer->nLinesAvail; n++ )
		{
			PCHARACTER check = renderer->ppCharacterLineStarts[n];
			while( check )
			{
				renderer->reverse_interface->_ReuseImage( (Image)check->cell );
				check = check->next_in_line;
			}
		}
	}
	{
		POINTER node;
		for( node = (Image)GetLeastNode( image_common_local.tint_cache ); 
			node; 
			node = (Image)GetGreaterNode( image_common_local.tint_cache ) )
		{
			struct shade_cache_image *ci = ( struct shade_cache_image *)node;
			struct shade_cache_element *ce;
			INDEX idx2;
			LIST_FORALL( ci->elements, idx2, struct shade_cache_element *, ce )
			{
				ce->image->reverse_interface->_ReuseImage( (Image)ce->image->reverse_interface_instance );
			}
		}
	}
}

//-------------------------------------------------------------------------

void IMGVER(CleanupFontSurfaces)( void ) {
	INDEX idx;
	PFONT_RENDERER renderer;
	if( !cleanFonts ) return;
	LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
	{
		INDEX idx;
		Image oldImage;
		LIST_FORALL( renderer->priorSurfaces, idx, Image, oldImage ) {
			if( oldImage->reverse_interface )
				oldImage->reverse_interface->_UnmakeImageFileEx( oldImage DBG_SRC );
			else
				IMGVER(UnmakeImageFileEx)( oldImage DBG_SRC );
		}
		DeleteListEx( &renderer->priorSurfaces DBG_SRC ); // this doesn't have THAT often...
	}
	cleanFonts = FALSE;
}

//-------------------------------------------------------------------------

void IMGVER(UpdateRendererImage)( Image image, PFONT_RENDERER renderer, int char_width, int char_height )
{
	if( !renderer->surface )
	{
		if( image->reverse_interface )
		{
			renderer->reverse_interface = image->reverse_interface;
			renderer->surface = image->reverse_interface->_MakeImageFileEx( char_width * 3, char_height * 3 DBG_SRC );
			image->reverse_interface->_BlatColor(renderer->surface,0,0,(renderer->surface)->width,(renderer->surface)->height, 0 );
		}
		else
		{
			renderer->surface = IMGVER(MakeImageFileEx)( char_width * 3, char_height * 3 DBG_SRC );
			IMGVER(ClearImage( renderer->surface ));
		}
	}
	else
	{
		Image new_surface;
		if( image->reverse_interface )
		{
			new_surface = image->reverse_interface->_MakeImageFileEx( renderer->surface->real_width * 2, renderer->surface->real_height * 2 DBG_SRC );
			image->reverse_interface->_BlatColor( new_surface,0,0,new_surface->width,new_surface->height, 0 );
			image->reverse_interface->_BlotImageEx( new_surface, renderer->surface, 0, 0, 1, BLOT_COPY );
			image->reverse_interface->_TransferSubImages( new_surface, renderer->surface );
#if defined( __3D__ ) 
			cleanFonts = TRUE;
			AddLink( &renderer->priorSurfaces, renderer->surface );
#else
			image->reverse_interface->_UnmakeImageFileEx( renderer->surface DBG_SRC );
#endif
		}
		else
		{
			new_surface = IMGVER(MakeImageFileEx)( renderer->surface->real_width * 2, renderer->surface->real_height * 2 DBG_SRC);
			IMGVER(ClearImage( new_surface ));
			IMGVER(BlotImage)( new_surface, renderer->surface, 0, 0 );
			IMGVER(TransferSubImages)( new_surface, renderer->surface );
#if defined( __3D__ ) 
			cleanFonts = TRUE;
			AddLink( &renderer->priorSurfaces, renderer->surface );
#else
			IMGVER(UnmakeImageFileEx)( renderer->surface DBG_SRC);
#endif
		}
		renderer->surface = new_surface;
		// need to recompute character floating point values for existing characters.
	}
}

Image IMGVER(AllocateCharacterSpace)( Image target_image, PFONT_RENDERER renderer, PCHARACTER character )
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
			//lprintf( "Is %d in %d?", height, (renderer->pLineStarts[line]) );
			if( height <= ( renderer->pLineStarts[line] ) )
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
					//lprintf( "Fits in this line, create at %d,%d   %d width", new_line_start + 1, last_line_top + 1
					//										, character->width );
					check->next_in_line = character;
					if( target_image->reverse_interface )
						character->cell = target_image->reverse_interface->_MakeSubImageEx( renderer->surface
															, new_line_start + 1, last_line_top + 1
															, character->width, height DBG_SRC );
					else
						character->cell = IMGVER(MakeSubImageEx)( renderer->surface
															, new_line_start + 1, last_line_top + 1
															, character->width, height DBG_SRC );
					//lprintf( "Result %d,%d", character->cell->real_x, character->cell->real_y );
					return character->cell;
				}
			}
			last_line_top += renderer->pLineStarts[line] + 2;
		}

		// if we got here, didn't find room on any line.
		if( !renderer->surface )
		{
			//lprintf( "make new image" );
			// gonna need lines and character chain starts too... don't continue
			IMGVER(UpdateRendererImage)( target_image, renderer, character->width, height );
		}

		// this should NOT go more than once.... but it could with nasty fonts...
		if( ( last_line_top + height ) > renderer->surface->real_height )
		{
			//lprintf( "expand font image %d %d %d", last_line_top, height, renderer->surface->real_height );
			IMGVER(UpdateRendererImage)( target_image, renderer, character->width, height );
			continue; // go to top of while loop;
		}

		if( renderer->nLinesUsed == renderer->nLinesAvail )
		{
			//lprintf( "make new lines" );
			renderer->nLinesAvail += 16; // another 16 lines... (16 ever should be enough)
			renderer->pLineStarts = (int*)HeapReallocate( 0, renderer->pLineStarts, sizeof( int ) * renderer->nLinesAvail );
			renderer->ppCharacterLineStarts = (PCHARACTER*)HeapReallocate( 0, renderer->ppCharacterLineStarts, sizeof( PCHARACTER ) * renderer->nLinesAvail );
		}

		renderer->nLinesUsed++;

		renderer->pLineStarts[line] = height;
		renderer->ppCharacterLineStarts[line] = character;
		//lprintf( "added new line; this is at %d,%d high", 1, last_line_top + 1 );
		if( target_image->reverse_interface )
			character->cell = target_image->reverse_interface->_MakeSubImageEx( renderer->surface
													, 0+1, last_line_top+1
													, character->width, height
													DBG_SRC
													);
		else
			character->cell = IMGVER(MakeSubImageEx)( renderer->surface
													, 0+1, last_line_top+1
													, character->width, height
													DBG_SRC );
		break;
	} while( 1 );
	//lprintf( "Result %d,%d", character->cell->real_x, character->cell->real_y );
	return character->cell;
}

Image IMGVER(AllocateCharacterSpaceByFont)( Image image, SFTFont font, PCHARACTER character )
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
			renderer->nWidth = 0;
			renderer->nHeight = 0;
			renderer->flags = 0 | FONT_FLAG_MONO;
			renderer->file = NULL;
			renderer->font = font;
			AddLink( &fonts, renderer );
		}
		return IMGVER(AllocateCharacterSpace)( image, renderer, character );
	}
	return NULL;
}

//-------------------------------------------------------------------------

#define TOSCALED(n) ((n)<<6)
#define CEIL(n)  ((int16_t)((((n) + 63 )&(-64)) >> 6))
#define FLOOR(n) ((int16_t)(((n) & (-64)) >> 6))

#define BITMAP_MASK(buf,n)  (buf[((n)/8)] & (0x80U >> ( (n) & 7 )))

static void RenderMonoChar( PFONT font
					, INDEX idx
					, FT_Bitmap *bitmap
					, FT_Glyph_Metrics *metrics
					, FT_Int left_ofs
					, FT_Int width
					)
{
	/*
	Log2( "Character: %d(%c)", idx, idx>32&&idx<127?idx:0 );
	//Log1( "Font Height %d", font->height );
	//Log5( "Bitmap information: (%d by %d) %d %d %d"
	//	 , bitmap->width
	//	 , bitmap->rows
	//	 , bitmap->pitch
	//	 , bitmap->pixel_mode
	//	 , bitmap->palette
	//	 );
	Log2( "Metrics: height %d bearing Y %d", metrics->height, metrics->horiBearingY );
	Log2( "Metrics: width %d bearing X %d", metrics->width, metrics->horiBearingX );
	Log2( "Metrics: advance %d %d", metrics->horiAdvance, metrics->vertAdvance );
	*/
	if( bitmap->width > 1000 ||
	    bitmap->rows > 1000 )
	{
		Log3( "Failing character %" _size_f " rows: %" _32f "  width: %" _32f, idx, bitmap->width, bitmap->rows );
		font->character[idx] = NULL;
		return;
	}

	{
		PCHARACTER character = NewPlus( CHARACTER,
												  ( bitmap->rows
													* ((bitmap->width+7)/8) ) );
		INDEX bit, line, linetop, linebottom;
		INDEX charleft, charright;

		char *data, *chartop;
		//lprintf( "character %c(%d) is %dx%d = %d", idx, idx, bitmap->width, bitmap->rows, ( bitmap->rows
		//											* ((bitmap->width+7)/8) ) );
		character->render_flags = 0;
		font->flags |= FONT_FLAG_UPDATED;
		font->character[idx] = character;
		font->character[idx]->next_in_line = NULL;
		font->character[idx]->cell = NULL;
		if( !character )
		{
			Log( "Failed to allocate character" );
			Log2( "rows: %d width: %d", bitmap->rows, bitmap->width );
			return;
		}
		character->width = width /*CEIL(metrics->horiAdvance)*/;
		if( !bitmap->rows && !bitmap->width )
		{
			//Log( "No physical data for this..." );
			character->ascent = 0;
			character->descent = 0;
			character->offset = 0;
			character->size = 0;
			return;
		}

		if( CEIL( metrics->width ) != bitmap->width )
		{
			//Log4( "%d(%c) metric and bitmap width mismatch : %d %d"
			//		, idx, (idx>=32 && idx<=127)?idx:'.'
			//	 , CEIL( metrics->width ), bitmap->width );
			if( !CEIL( metrics->width ) && bitmap->width )
				metrics->width = TOSCALED( bitmap->width );
		}
		if( CEIL( metrics->height ) != bitmap->rows )
		{
			//lprintf( "%d(%c) metric and bitmap height mismatch : %d %d using height %d"
			//		, idx, (idx>=32 && idx<=127)?idx:'.'
			//	 , CEIL( metrics->height )
			//	 , bitmap->rows
			//	 , TOSCALED( bitmap->rows) );
			metrics->height = TOSCALED( bitmap->rows );
		}

		character->ascent = CEIL(metrics->horiBearingY);
		//lprintf( "heights %d %d %d", font->height, bitmap->rows, metrics->height );
		if( metrics->height )
			character->descent = -CEIL(metrics->height - metrics->horiBearingY) + 1;
		else
			character->descent = CEIL( metrics->horiBearingY ) - font->height + 1;
		character->offset = CEIL(metrics->horiBearingX);
		character->size = bitmap->width;
		//charleft = charleft;
		//charheight = CEIL( metrics->height );
		//if( 0 )
		//lprintf( "(%" _size_f "(%c)) Character parameters: %d %d %d %d %d"
		//		 , idx, (char)(idx< 32 ? ' ':idx)
		//		 , character->width
		//		 , character->offset
		//		 , character->size
		//		 , character->ascent
		//		 , character->descent );

		// for all horizontal lines which are blank - decrement ascent...
		chartop = NULL;
		if( bitmap->buffer )
		{
			data = (char*)bitmap->buffer;

			linetop = 0;
			linebottom = character->ascent - character->descent;
			//lprintf( "Linetop: %d linebottom: %d", linetop, linebottom );

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
					//Log( "Dropping a line..." );
					character->ascent--;
				}
				else
				{
					//lprintf( "New top will be %d", line );
					linetop = line;
					chartop = data;
					break;
				}
			}
			if( line == linebottom + 1 )
			{
				//Log( "No character data... might still have a width though" );
				character->size = 0;
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
					//Log( "Dropping a line..." );
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
				//Log3( "Reduced char(%c) size by %d to %d", idx>32&&idx<127?idx:'.', charleft, character->size-charleft );
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
			//Log2( "Reduced char right size %d to %d", charright, charright - charleft );

			character->offset = (uint8_t)charleft;
			character->size = (uint8_t)(charright - charleft);

			//Log7( "(%d(%c)) Character parameters: %d %d %d %d %d"
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
	  (((buf[n]&0xE0)?2:0)   \
	  |((buf[n]&0x0F)?1:0))       \
	):(buf[n]))

static void RenderGreyChar( PFONT font
						 , INDEX idx
						 , FT_Bitmap *bitmap
						 , FT_Glyph_Metrics *metrics
						 , FT_Int left_ofs
						 , FT_Int width
						 , uint32_t bits // 2 or 8
						 )
{
	//lprintf( "Rending char %d bits %d", idx, bits );
	/*
	 Log2( "Character: %d(%c)", idx, idx>32&&idx<127?idx:0 );
	 //Log1( "Font Height %d", font->height );
	 //Log5( "Bitmap information: (%d by %d) %d %d %d"
	 //	 , bitmap->width
	//	 , bitmap->rows
	//	 , bitmap->pitch
	//	 , bitmap->pixel_mode
	//	 , bitmap->palette
	//	 );
	Log2( "Metrics: height %d bearing Y %d", metrics->height, metrics->horiBearingY );
	Log2( "Metrics: width %d bearing X %d", metrics->width, metrics->horiBearingX );
	Log2( "Metrics: advance %d %d", metrics->horiAdvance, metrics->vertAdvance );
	*/
	if( bitmap->width > 2000  ||
	    bitmap->rows > 2000 )
	{
		Log3( "Failing character %" _size_f " rows: %d  width: %d", idx, bitmap->width, bitmap->rows );
		font->character[idx] = NULL;
		return;
	}

	{
		PCHARACTER character = NewPlus( CHARACTER,
												  + ( bitmap->rows
													  * (bitmap->width+(bits==8?0:bits==2?3:7)/(8/bits)) )
												  + 512 );
		INDEX bit, line, linetop, linebottom;
		INDEX charleft, charright;

		char *data, *chartop;
		//lprintf( "character %c(%d) is %dx%d = %d", idx, idx, bitmap->width, bitmap->rows, ( bitmap->rows
		//											  * (bitmap->width+(bits==8?0:bits==2?3:7)/(8/bits)) )
		//										  + 512 );
		character->render_flags = (bits==8)?FONT_FLAG_8BIT:FONT_FLAG_2BIT;
		font->flags |= FONT_FLAG_UPDATED;
		font->character[idx] = character;
		font->character[idx]->next_in_line = NULL;
		font->character[idx]->cell = NULL;
		if( !character )
		{
			Log( "Failed to allocate character" );
			Log2( "rows: %d width: %d", bitmap->rows, bitmap->width );
			return;
		}
		character->width = width /*CEIL(metrics->horiAdvance)*/;
		if( !bitmap->rows && !bitmap->width )
		{
			//Log( "No physical data for this..." );
			character->ascent = 0;
			character->descent = 0;
			character->offset = 0;
			character->size = 0;
			return;
		}

		if( CEIL( metrics->width ) != bitmap->width )
		{
			Log4( "%" _size_f "(%c) metric and bitmap width mismatch : %d %d"
					, idx, (int)((idx>=32 && idx<=127)?idx:'.')
					, CEIL( metrics->width ), bitmap->width );
		}
		if( CEIL( metrics->height ) != bitmap->rows )
		{
			Log4( "%" _size_f "(%c) metric and bitmap height mismatch : %d %d"
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

		//lprintf( "heights %d %d %d", font->height, bitmap->rows, CEIL(metrics->height) );
		//if( 0 )
		//	lprintf( "(%" _size_f "(%c)) Character parameters: %d %d %d %d %d"
		//			 , idx, (char)(idx< 32 ? ' ':idx)
		//			 , character->width
		//			 , character->offset
		//			 , character->size
		//			 , character->ascent
		//			 , character->descent );

		// for all horizontal lines which are blank - decrement ascent...
		chartop = NULL;
		data = (char*)bitmap->buffer;

		linetop = 0;
		linebottom = character->ascent - character->descent;
		//Log2( "Linetop: %d linebottom: %d", linetop, linebottom );

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
				//Log( "Dropping a line..." );
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
			//Log( "No character data usable..." );
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
				//Log( "Dropping a line..." );
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
		//Log2( "Reduced char right size %d to %d", charright, charright - charleft );

		character->offset = (uint8_t)charleft;
		character->size = (uint8_t)(charright - charleft);

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
					//printf( "(%02x)", grey );
					if( grey )
					{
						if( bits == 2 )
						{
							outdata[bit>>2] |= grey << ((bit % 4) * 2);
							//printf( "%02x ", outdata[bit>>2] );
							/*
							switch( grey )
							{
							case 3: printf( "X" ); break;
							case 2: printf( "O" ); break;
							case 1: printf( "o" ); break;
							case 0: printf( "_" ); break;
							default: printf( "%02X", grey );
							}
							*/
						}
						else
							outdata[bit] = grey;
					}
					//else
					//	printf( "_" );
				}
				//printf( "\n" );
			}
		}
		character->offset += left_ofs;
	}
}

static void RenderColorChar( PFONT font
						 , INDEX idx
						 , FT_Bitmap *bitmap
						 , FT_Glyph_Metrics *metrics
						 , FT_Int left_ofs
						 , FT_Int width
						 )
{
	//lprintf( "Rending char %d bits %d", idx, bits );
	/*
	 Log2( "Character: %d(%c)", idx, idx>32&&idx<127?idx:0 );
	 //Log1( "Font Height %d", font->height );
	 //Log5( "Bitmap information: (%d by %d) %d %d %d"
	 //	 , bitmap->width
	//	 , bitmap->rows
	//	 , bitmap->pitch
	//	 , bitmap->pixel_mode
	//	 , bitmap->palette
	//	 );
	Log2( "Metrics: height %d bearing Y %d", metrics->height, metrics->horiBearingY );
	Log2( "Metrics: width %d bearing X %d", metrics->width, metrics->horiBearingX );
	Log2( "Metrics: advance %d %d", metrics->horiAdvance, metrics->vertAdvance );
	*/
	if( bitmap->width > 1000 ||
	    bitmap->rows > 1000 )
	{
		Log3( "Failing character %" _size_f " rows: %d  width: %d", idx, bitmap->width, bitmap->rows );
		font->character[idx] = NULL;
		return;
	}

	{
		PCHARACTER character = NewPlus( CHARACTER,
												  + ( bitmap->rows
													  * (bitmap->width*4) )
												  + 512 );
		INDEX bit, line, linetop, linebottom;
		INDEX charleft, charright;

		char *data, *chartop;
		//lprintf( "character %c(%d) is %dx%d = %d", idx, idx, bitmap->width, bitmap->rows, ( bitmap->rows
		//											  * (bitmap->width+(bits==8?0:bits==2?3:7)/(8/bits)) )
		//										  + 512 );
		font->flags |= FONT_FLAG_UPDATED;
		font->character[idx] = character;
		font->character[idx]->next_in_line = NULL;
		font->character[idx]->cell = NULL;
		if( !character )
		{
			Log( "Failed to allocate character" );
			Log2( "rows: %d width: %d", bitmap->rows, bitmap->width );
			return;
		}
		character->width = width /*CEIL(metrics->horiAdvance)*/;
		if( !bitmap->rows && !bitmap->width )
		{
			//Log( "No physical data for this..." );
			character->ascent = 0;
			character->descent = 0;
			character->offset = 0;
			character->size = 0;
			return;
		}

		if( CEIL( metrics->width ) != bitmap->width )
		{
			Log4( "%" _size_f "(%c) metric and bitmap width mismatch : %d %d"
					, idx, (int)((idx>=32 && idx<=127)?idx:'.')
					, CEIL( metrics->width ), bitmap->width );
		}
		if( CEIL( metrics->height ) != bitmap->rows )
		{
			Log4( "%" _size_f "(%c) metric and bitmap height mismatch : %d %d"
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

		//lprintf( "heights %d %d %d", font->height, bitmap->rows, CEIL(metrics->height) );
		//if( 0 )
		//	lprintf( "(%" _size_f "(%c)) Character parameters: %d %d %d %d %d"
		//			 , idx, (char)(idx< 32 ? ' ':idx)
		//			 , character->width
		//			 , character->offset
		//			 , character->size
		//			 , character->ascent
		//			 , character->descent );

		// for all horizontal lines which are blank - decrement ascent...
		chartop = NULL;
		data = (char*)bitmap->buffer;

		linetop = 0;
		linebottom = character->ascent - character->descent;
		//Log2( "Linetop: %d linebottom: %d", linetop, linebottom );

		// reduce ascent to character data minimum....
		for( line = linetop; line <= linebottom; line++ )
		{
			data = (char*)bitmap->buffer + line * bitmap->pitch;
			for( bit = 0; bit < character->size; bit++ )
			{
				if( data[0] || data[1] || data[2] )
					break;
			}
			if( bit == character->size )
			{
				//Log( "Dropping a line..." );
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
			//Log( "No character data usable..." );
			return;
		}

		for( line = linebottom; line > linetop; line-- )
		{
			data = (char*)bitmap->buffer + line * bitmap->pitch;
			for( bit = 0; bit < character->size; bit++ )
			{
				if( data[0] || data[1] || data[2] )
					break;
			}
			if( bit == character->size )
			{
				//Log( "Dropping a line..." );
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
				if( data[0] || data[1] || data[2] )
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
				if( data[0] || data[1] || data[2] )
					if( bit > charright )
						charright = bit;
			}
		}
		charright ++; // add one back in... so we have a delta between left and right
		//Log2( "Reduced char right size %d to %d", charright, charright - charleft );

		character->offset = (uint8_t)charleft;
		character->size = (uint8_t)(charright - charleft);

		{
			unsigned char *outdata;
			// now - to copy the pixels...
			for( line = linetop; line <= linebottom; line++ )
			{
				outdata = character->data +
					((line - linetop) * ((character->size*4) ));
				data = (char*)bitmap->buffer + (line) * bitmap->pitch;
				for( bit = 0; bit < ( character->size ); bit++)
					outdata[bit] = 0;
				for( bit = 0; bit < (character->size); bit++ )
				{
					//int grey = data[0] || data[1] || data[2];
					//printf( "(%02x)", grey );
					{
						outdata[bit + 0] = data[0];
						outdata[bit + 1] = data[1];
						outdata[bit + 2] = data[2];
					}
					//else
					//	printf( "_" );
				}
				//printf( "\n" );
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

/* test if a face is full-color */
static LOGICAL IsColorEmojiFont( FT_Face face ) {
	static const uint32_t tag = FT_MAKE_TAG('C', 'B', 'D', 'T');
	FT_ULong length = 0;
	FT_Load_Sfnt_Table( face, tag, 0, NULL, &length);
	if (length) {
		//std::cout << font_file_ << " is color font" << std::endl;
		return TRUE;
	}
	return FALSE;
}

// everything eventually comes to here to render a font...
// this should therefore cache this information with the fonts
// it has created...

void IMGVER(InternalRenderFontCharacter)( PFONT_RENDERER renderer, PFONT font, INDEX idx )
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
			lprintf( "Couldn't find by font either..." );
			//DebugBreak();
			return;
		}
	}
	if( fg.flags.OpeningFontStatus && ( fg.font_status_open_thread != MakeThread() ) )
		return;
	if( fg.flags.bScanningFonts && fg.font_status_timer_thread )
		;
	else
		EnterCriticalSec( &fg.cs );
	if( !renderer->font->character[idx] )
	{
		FT_Face face = renderer->face;
		FT_Error status;
		int glyph_index = FT_Get_Char_Index( face, (FT_ULong)idx );
		if( glyph_index <= 0 ) {
			LeaveCriticalSec( &fg.cs );
			return;
		}
		//lprintf( "Character %d is glyph %d", idx, glyph_index );
		status = FT_Load_Glyph( face
						 , glyph_index
						 , 0
						 // | FT_LOAD_FORCE_AUTOHINT
						 );
		if( status ) {
			LeaveCriticalSec( &fg.cs );
			return;
		}

		{
			//int ascent = CEIL(face->glyph->metrics.horiBearingY);
			int ascent = CEIL( face->size->metrics.ascender );
			int descent = CEIL( face->size->metrics.descender );
			int height = CEIL( face->size->metrics.height );
			if( height > renderer->font->height ) {
				renderer->font->height = height;
				renderer->font->baseline = ascent +
					((height - (ascent - descent)) / 2);
			}
			else if( height < renderer->font->height ) {
				lprintf( "Reset font height from %d to %d", renderer->font->height, height );
			}
		}
		//lprintf( "advance and height... %d %d", ( face->glyph->linearVertAdvance>>16 ), renderer->font->height );
		switch( face->glyph->format )
		{
		case FT_GLYPH_FORMAT_OUTLINE:
			//Log( "Outline render glyph...." );
			if( (( renderer->flags & 3 )<1) )
				FT_Render_Glyph( face->glyph, FT_RENDER_MODE_MONO );
			else
				FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
		case FT_GLYPH_FORMAT_BITMAP:
			/*
			Log2( "bitmap params: left %d top %d", face->glyph->bitmap_left, face->glyph->bitmap_top );
			Log6( "Glyph Params: %d(%d) %d(%d) %d %d"
					, face->glyph->linearHoriAdvance
					, face->glyph->linearHoriAdvance>>16
					, face->glyph->linearVertAdvance
					, face->glyph->linearVertAdvance>>16
					, face->glyph->advance.x
					, face->glyph->advance.y );
					Log3( "bitmap height: %d ascent: %d descent: %d", face->height>>6, face->ascender>>6, face->descender>>6);
					*/
			//Log( "Bitmap" );
			if( renderer->flags & FONT_FLAG_COLOR )
			{
				RenderColorChar( renderer->font
								, idx
								, &face->glyph->bitmap
								, &face->glyph->metrics
								, face->glyph->bitmap_left // left offset?
					, (face->face_flags & FT_FACE_FLAG_SCALABLE)
					? (face->glyph->linearHoriAdvance >> 16)
					: (face->glyph->advance.x >> 6)
				);
			}
			else if( ( face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO )
				|| ( ( renderer->flags & 3 ) == 0 ) 
				)
			{
				RenderMonoChar( renderer->font
								, idx
								, &face->glyph->bitmap
								, &face->glyph->metrics
								, face->glyph->bitmap_left // left offset?
								, (face->face_flags & FT_FACE_FLAG_SCALABLE) 
							? (face->glyph->linearHoriAdvance >> 16) 
							:( face->glyph->advance.x >> 6 )
								//face->glyph->linearHoriAdvance>>16
									);
			}
			else
			{
				RenderGreyChar( renderer->font
									, idx
									, &face->glyph->bitmap
									, &face->glyph->metrics
									, face->glyph->bitmap_left // left offset?
					, (face->face_flags & FT_FACE_FLAG_SCALABLE)
					? (face->glyph->linearHoriAdvance >> 16)
					: (face->glyph->advance.x >> 6)
					, ((renderer->flags & 3)>=2)?8:2 );
			}
			break;
		//case FT_GLYPH_FORMAT_COMPOSITE:
		//	Log( "unsupported Composit" );
		//	break;
		default:
			renderer->font->character[idx] = NULL;
			Log1( "unsupported Unknown format(%4.4s)", (char*)&face->glyph->format );
			break;
		}
	}
	for( idx = 0; idx < renderer->font->characters; idx++ )
		if( renderer->font->character[idx] )
			break;
	if( idx == renderer->font->characters )
	{
		renderer->ResultFont = NULL;
	}
	else
	{
		renderer->ResultFont = IMGVER(LoadFont)( (SFTFont)renderer->font );
	}
	if( fg.flags.bScanningFonts && fg.font_status_timer_thread )
		;
	else
		LeaveCriticalSec( &fg.cs );
}

static SFTFont DoInternalRenderFontFile( PFONT_RENDERER renderer )
{
	int error;

	{
		int charcount;
		if( !renderer->font )
		{
			//lprintf( "Need font resource (once)" );
			renderer->font = NewPlus( FONT, (charcount=65535) * sizeof(PCHARACTER) );
			// this is okay it's a kludge so some other code worked magically
			// it's redundant and should be dleete...
			renderer->ResultFont = (SFTFont)renderer->font;
			MemSet( renderer->font, 0, sizeof( FONT )+ charcount * sizeof(PCHARACTER) );
		}

		{
			INDEX idx;
			PFONT font;
			font = (PFONT)renderer->ResultFont;
			if( !font ) {
				return NULL;
			}
			renderer->max_ascent = 0;
			renderer->min_descent = 0;
			if( !font->characters )
				font->characters = charcount + 1;
			font->baseline = 0;
			font->flags = ( renderer->flags & 3 );
			if( 0 )
				lprintf( "Setting pixel size %d %d", ScaleValue( &renderer->width_scale, renderer->nWidth )
						 , ScaleValue( &renderer->height_scale, renderer->nHeight ) );

			{
				FT_Size_RequestRec req;
				req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
				req.width = (FT_Long)(ScaleValue( &renderer->width_scale, renderer->nWidth ) << 6);
				req.height = (FT_Long)(ScaleValue( &renderer->height_scale, renderer->nHeight ) << 6);
				req.horiResolution = 0;
				req.vertResolution = 0;

				error = FT_Request_Size( renderer->face, &req );
			}
			/*
			error = FT_Set_Pixel_Sizes( renderer->face
											  , ScaleValue( &renderer->width_scale, renderer->nWidth )
											  , ScaleValue( &renderer->height_scale, renderer->nHeight ) );
			*/
			if( error )
			{
				Log1( "Fault setting char size - %d", error );
			}
			font->character[0] = NULL;
			font->height = 0; //CEIL(face->size->metrics.height);
			font->name = StrDup( renderer->fontname );
			IMGVER(InternalRenderFontCharacter)( renderer, NULL, 0 );
		}
	}
	return renderer->ResultFont;
}

SFTFont IMGVER(InternalRenderFontFile)( CTEXTSTR file
										, int32_t nWidth
										, int32_t nHeight
										, PFRACTION width_scale
										, PFRACTION height_scale
										, uint32_t flags // whether to render 1(0/1), 2(2), 8(3) bits, ...
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
		//lprintf( "Failed to init fonts.. failing font render." );
		//LeaveCriticalSec( &fg.cs );
	//	return NULL;
	}

	// with the current freetype library which has been grabbed
	// width and height more than these cause errors.
	if( nWidth > 255 ) nWidth = 255;
	//if( nHeight > 227 ) nHeight = 227;

try_another_default:	
	if( !file || bDefaultFile )
	{
		switch( bDefaultFile )
		{
		case 0:  	
			file = "arialbd.ttf"; 	  	
			break;
		case 1:
			file = "fonts/arialbd.ttf";
			break;
		case 2:
			 lprintf( "default font arialbd.ttf or fonts/arialbd.ttf did not load." );
			LeaveCriticalSec( &fg.cs );
			return NULL;
		}
		bDefaultFile++;
	}
	{
		INDEX idx;
		PFONT_RENDERER fileDataRenderer = NULL;
		LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
		{
			if( strcmp( renderer->file, file ) == 0 ) {
				fileDataRenderer = renderer;
				if( renderer->nWidth == nWidth
					&& renderer->nHeight == nHeight
					&& renderer->flags == flags )
				{
					break;
				}
			}
		}
		if( !renderer )
		{
			renderer = New( FONT_RENDERER );
			MemSet( renderer, 0, sizeof( FONT_RENDERER ) );

			renderer->nWidth = nWidth;
			renderer->nHeight = nHeight;

			if( width_scale)
				renderer->width_scale = width_scale[0];
			else
				SetFraction( renderer->width_scale, 1, 1 );

			if( height_scale)
				renderer->height_scale = height_scale[0];
			else
				SetFraction( renderer->height_scale, 1, 1 );
			renderer->flags = flags;
			renderer->file = StrDup( file );
			AddLink( &fonts, renderer );
			do
			{
				// this needs to be post processed to
				// center all characters in a font - favoring the
				// bottom...

				if( !renderer->face )
				{
					if( fileDataRenderer ) {
						renderer->font_memory = fileDataRenderer->font_memory;
						renderer->font_memory_size = fileDataRenderer->font_memory_size;
						error = FT_New_Memory_Face( fg.library
							, (FT_Byte*)(renderer->font_memory)
							, (FT_Long)(renderer->font_memory_size)
							, face_idx
							, &renderer->face );

					}
					else {
						//lprintf( "memopen %s", renderer->file );
						error = OpenFontFile( renderer->file, &renderer->font_memory, &renderer->font_memory_size, &renderer->face, face_idx, TRUE );
					}
					if( IsColorEmojiFont( renderer->face ) )
						renderer->flags |= FONT_FLAG_COLOR;
#if rotation_was_italic
					if( renderer->flags & FONT_FLAG_ITALIC )
					{
						FT_Matrix matrix
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
					//lprintf( "Failed to load font...Err:%d %s %d %d", error, file, nWidth, nHeight );
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
					snprintf( renderer->fontname, 256, "%s%s%s%dBy%d"
							  , face->family_name?face->family_name:"No-Name"
							  , face->style_name?face->style_name:"normal"
							  , (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)?"fixed":""
							  , nWidth, nHeight
							  );
					KillSpaces( renderer->fontname );
				}

				DoInternalRenderFontFile( renderer );

			}
			while( ( face_idx < num_faces ) && !renderer->font );
		}
		else
		{
			//lprintf( "using existing renderer for this font..." );
		}
	}


	// just for grins - check all characters defined for font
	// to see if their ascent - descent is less than height
	// and that ascent is less than baseline.
	// however, if we have used a LoadFont function
	// which has replaced the font thing with a handle...
	//DumpFontFile( "font.h", font );
	LeaveCriticalSec( &fg.cs );
	return renderer->ResultFont;;
}


static int RecheckCache( CTEXTSTR entry, uint32_t *pe
					 , CTEXTSTR style, uint32_t *ps
					 , CTEXTSTR filename, uint32_t *psf );

SFTFont IMGVER(RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale )
{
	LOGICAL delete_pfd = FALSE;
#define fname(base) base
#define sname(base) fname(base) + (StrLen(base)+1)
#define fsname(base) sname(base) + StrLen(sname(base)+1)
	if( !pfd )
		return NULL;

	IMGVER(LoadAllFonts)(); // load cache data so we can resolve user data
	if( pfd->magic != MAGIC_PICK_FONT && pfd->magic != MAGIC_RENDER_FONT )
	{
		//lprintf( "Attempt to decode %s", pfd );
		delete_pfd = TRUE;
		if( strncmp( (char*)pfd, "pick,", 4 ) == 0 )
		{
			PFONTDATA newpfd;
			newpfd = NewPlus( FONTDATA, 256 );

			newpfd->magic = MAGIC_PICK_FONT;
			{
				int family, style, file, width, height, flags;
				char *buf = newpfd->names;
				int ofs;
				//lprintf( "Attempting scan of %s", (((char*)pfd) + 5) );
				sscanf( ( ((char*)pfd) + 5), "%d,%d,%d,%d,%d,%d"
						, &family, &style, &file
						, &width, &height, &flags );
				//lprintf( "%d,%d,%d,%d,%d,%d", family, style, file, width, height, flags );
				if( family < (int)fg.nFonts )
				{
					//lprintf( "family %s", fg.pFontCache[family].name );
					ofs = snprintf( buf, 256, "%s", fg.pFontCache[family].name );
					ofs += 1;
					if( style < (int)fg.pFontCache[family].nStyles )
					{
						//lprintf( "style %s", fg.pFontCache[family].styles[style].name );
						ofs += snprintf( buf + ofs, 256-ofs,"%s", fg.pFontCache[family].styles[style].name );
						ofs += 1;
						if( file < (int)fg.pFontCache[family].styles[style].nFiles )
						{
							//lprintf( "file %s", fg.pFontCache[family].styles[style].files[file].file );
							ofs += snprintf( buf + ofs, 256-ofs,"%s", fg.pFontCache[family].styles[style].files[file].file );
						}
					}
				}
				else
					lprintf( "overflow, will be invalid." );
				newpfd->nWidth = width;
				newpfd->nHeight = height;
				newpfd->flags = flags;
				newpfd->nFamily = family;
				newpfd->nStyle = style;
				newpfd->nFile = file;
				pfd = newpfd;
			}
		}
		else
		{
			PRENDER_FONTDATA prfd;
			CTEXTSTR tail = StrRChr( (TEXTSTR)pfd, ',' );
			if( tail )
				tail++;
			else
				tail = "";
			prfd = NewPlus( RENDER_FONTDATA, (StrLen( tail ) + 1)*sizeof(TEXTCHAR) );
			StrCpy( prfd->filename, tail );
			prfd->magic = MAGIC_RENDER_FONT;
			{
				int width, height, flags;
				sscanf( (char*)pfd, "%d,%d,%d", &width, &height, &flags );
				prfd->nWidth = width;
				prfd->nHeight = height;
				prfd->flags = flags;
			}
			pfd = (PFONTDATA)prfd;
		}
	}
	if( pfd->magic == MAGIC_PICK_FONT )
	{
		CTEXTSTR name1 = pfd->names;
		CTEXTSTR name2 = pfd->names + StrLen( pfd->names ) + 1;
		CTEXTSTR name3 = name2 + StrLen( name2 ) + 1;

		// picked from the current cache... otherwise search for it in this cache.
		if( RecheckCache( name1/*fname(pfd->names)*/, &pfd->nFamily
							, name2/*sname(pfd->names)*/, &pfd->nStyle
							, name3/*fsname(pfd->names)*/, &pfd->nFile ) )
		{
			SFTFont return_font = IMGVER(InternalRenderFont)( pfd->nFamily
																 , pfd->nStyle
																 , pfd->nFile
																 , pfd->nWidth
																 , pfd->nHeight
																 , width_scale
																 , height_scale
																 , pfd->flags );
			if( delete_pfd )
				Deallocate( PFONTDATA, pfd );
			return return_font;
		}
		//lprintf( "[%s][%s][%s]", name1, name2, name3 );
	}
	else if( pfd->magic == MAGIC_RENDER_FONT )
	{
		PRENDER_FONTDATA prfd = (PRENDER_FONTDATA)pfd;
		PFONT_ENTRY pfe = NULL;
		int family_idx;
		SFTFont result_font;
		for( family_idx = 0; SUS_LT( family_idx, int, fg.nFonts, uint32_t ); family_idx++ )
		{
			pfe = fg.pFontCache + family_idx;
			if( StrCaseCmp( pfe->name, prfd->filename ) == 0 )
				break;
		}
		if( pfe )
			result_font = IMGVER(InternalRenderFont)( family_idx, 0, 0
											 , prfd->nWidth
											 , prfd->nHeight
											 , width_scale
											 , height_scale
											 , prfd->flags );
		else
			result_font = IMGVER(InternalRenderFontFile)( prfd->filename
												  , prfd->nWidth
												  , prfd->nHeight
												  , width_scale
												  , height_scale
												  , prfd->flags );
		if( delete_pfd )
			Deallocate( PFONTDATA, pfd );
		return result_font;
	}
	LogBinary( pfd, 32 );
	lprintf( "Font parameters are no longer valid.... could not be found in cache issue font dialog here?" );
	return NULL;
}

SFTFont IMGVER(InternalRenderFont)( uint32_t nFamily
								  , uint32_t nStyle
								  , uint32_t nFile
								  , int32_t nWidth
								  , int32_t nHeight
								  , PFRACTION width_scale
								  , PFRACTION height_scale
								  , uint32_t flags
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
	tnprintf( name, sizeof( name ), "%s/%s"
			  , pfe->styles[nStyle].files[nFile].path
			  , pfe->styles[nStyle].files[nFile].file );

	//Log1( "Font: %s", name );
	do
	{
		if( nAlt )
		{
			tnprintf( name, sizeof( name ), "%s/%s"
					  , pfe->styles[nStyle].files[nFile].pAlternate[nAlt-1].path
					  , pfe->styles[nStyle].files[nFile].pAlternate[nAlt-1].file );
		}
		// this is called from pickfont, and we dont' have font space to pass here...
		// it's taken from dialog parameters...
		if( pfe->styles[nStyle].flags.italic )
			flags |= FONT_FLAG_ITALIC;
		if( pfe->styles[nStyle].flags.bold )
			flags |= FONT_FLAG_BOLD;

		result = IMGVER(InternalRenderFontFile)( name, nWidth, nHeight, width_scale, height_scale, flags );
		// this may not be 'wide' enough...
		// there may be characters which overflow the
		// top/bottomm of the cell... probably will
		// ignore that - but if we DO find such a font,
		// maybe this can be adjusted -- NORMALLY
		// this will be sufficient...
	} while( (nAlt++ < pfe->styles[nStyle].files[nFile].nAlternate) && !result );
	}
	return (SFTFont)result;
}

void IMGVER(DestroyFont)( SFTFont *font )
{
	if( font )
	{
		if( *font )
		{
			xlprintf( LOG_ALWAYS )( "font destruction is NOT implemented, this WILL result in a memory leak!!!!!!!" );
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

/*static declared above*/
int RecheckCache( CTEXTSTR entry, uint32_t *pe
					 , CTEXTSTR style, uint32_t *ps
					 , CTEXTSTR filename, uint32_t *psf )
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
						//lprintf( "	 (3) is %s==%s (%s)", fg.pFontCache[n].styles[s].files[sf].file, file, filename );
						if( strcmp( fg.pFontCache[n].styles[s].files[sf].file, file ) == 0 )
						{
							(*pe) = (uint32_t)n;
							(*ps) = (uint32_t)s;
							(*psf) = (uint32_t)sf;
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

void IMGVER(SetFontRendererData)( SFTFont font, POINTER pResult, size_t size )
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

SFTFont IMGVER(RenderFontFileScaledEx)( CTEXTSTR file, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags, size_t * size, POINTER *pFontData )
{
	POINTER save_data;
	size_t save_size;
	SFTFont font;
	if( !pFontData )
	{
		// save this anyway so we can get font data later.
		pFontData = &save_data;
		size = &save_size;
	}
	font = IMGVER(InternalRenderFontFile)( file, (width&0x7FFF), (height&0x7FFF), width_scale, height_scale, flags );
	if( font && size && pFontData )
	{
		TEXTCHAR buf[256];
		(*size) = snprintf( buf, 256, "%d,%d,%d,%s", width, height, flags, file );
		(*size) += 1; // save the null in the binary.
		(*pFontData) = StrDup( buf );
		IMGVER(SetFontRendererData)( font, (*pFontData), (*size ) );
		/*
		PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*size) =
																			  (chars = StrLen( file ) + 1)*sizeof(TEXTCHAR) );
		(*size) += sizeof( RENDER_FONTDATA );
		pResult->magic = MAGIC_RENDER_FONT;
		pResult->nHeight = height;
		pResult->nWidth = width;
		pResult->flags = flags;
		StrCpyEx( pResult->filename, file, chars );
		(*pFontData) = (POINTER)pResult;
		SetFontRendererData( font, pResult, (*size) );
		*/
	}
	return font;
}
#undef RenderFontFileEx
SFTFont IMGVER(RenderFontFileEx)( CTEXTSTR file, uint32_t width, uint32_t height, uint32_t flags, size_t * size, POINTER *pFontData )
{
	return IMGVER(RenderFontFileScaledEx)( file, width, height, NULL, NULL, flags, size, pFontData );
}

#undef RenderFontFile
SFTFont IMGVER(RenderFontFile)( CTEXTSTR file, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags )
{
	return IMGVER(InternalRenderFontFile)( file, width, height, width_scale, height_scale, flags );
}

SFTFont IMGVER(RenderScaledFontEx)( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags, size_t *pnFontDataSize, POINTER *pFontData )
{
	SFTFont font;
	PFONT_ENTRY pfe;
	INDEX family_idx;
	POINTER save_data;
	size_t save_size;
	if( !pFontData )
	{
		// save this anyway so we can get font data later.
		pFontData = &save_data;
		pnFontDataSize = &save_size;
	}

	IMGVER(LoadAllFonts)(); // load cache data so we can resolve user data
	for( family_idx = 0; family_idx < fg.nFonts; family_idx++ )
	{
		pfe = fg.pFontCache + family_idx;
		if( StrCaseCmp( pfe->name, name ) == 0 )
			break;
	}
	if( family_idx < fg.nFonts )
	{
		SFTFont return_font = IMGVER(InternalRenderFont)( (uint32_t)family_idx
														 , 0
															 , 0
															 , width
															 , height
														 , width_scale
														 , height_scale
														 , flags );
		{
			TEXTCHAR buf[256];
			(*pnFontDataSize) = snprintf( buf, 256, "%d,%d,%d,%s", width, height, flags, name );
			(*pnFontDataSize) += 1; // save the null in the binary.
			(*pFontData) = StrDup( buf );
			IMGVER(SetFontRendererData)( return_font, (*pFontData), (*pnFontDataSize) );
			/*
			PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*pnFontDataSize)
																					= 
																 (chars = StrLen( name ) + 1)*sizeof(TEXTCHAR) );
			(*pnFontDataSizec) += sizeof( RENDER_FONTDATA );
			pResult->magic = MAGIC_RENDER_FONT;
			pResult->nHeight = height;
			pResult->nWidth = width;
			pResult->flags = flags;
			StrCpyEx( pResult->filename, name, chars );
			(*pFontData) = (POINTER)pResult;
			SetFontRendererData( return_font, pResult, (*pnFontDataSize) );
			*/
		}
		return return_font;
	}

	font = IMGVER(InternalRenderFontFile)( name
										  , width
										  , height
										  , width_scale
										  , height_scale
										  , flags );
	{
		TEXTCHAR buf[256];
		(*pnFontDataSize) = snprintf( buf, 256, "%d,%d,%d,%s", width, height, flags, name );
		(*pnFontDataSize) += 1; // save the null in the binary.
		(*pFontData) = StrDup( buf );
		IMGVER(SetFontRendererData)( font, (*pFontData), (*pnFontDataSize ) );
		/*
		PRENDER_FONTDATA pResult = NewPlus( RENDER_FONTDATA, (*pnFontDataSize) =
																				(chars = StrLen( name ) + 1)*sizeof(TEXTCHAR) );
		(*pnFontDataSize) += sizeof( RENDER_FONTDATA );
		pResult->magic = MAGIC_RENDER_FONT;
		pResult->nHeight = height;
		pResult->nWidth = width;
		pResult->flags = flags;
		StrCpyEx( pResult->filename, name, chars );
		(*pFontData) = (POINTER)pResult;
		SetFontRendererData( font, pResult, (*pnFontDataSize) );
		*/
	}
	return font;
}

#undef RenderScaledFont
SFTFont IMGVER(RenderScaledFont)( CTEXTSTR name, uint32_t width, uint32_t height, PFRACTION width_scale, PFRACTION height_scale, uint32_t flags )
{
	return IMGVER(RenderScaledFontEx)(name,width,height,width_scale,height_scale,flags,NULL,NULL);
}


int IMGVER(GetFontRenderData)( SFTFont font, POINTER *fontdata, size_t *fontdatalen )
{
	// set pointer and uint32_t datalen passed by address
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

void IMGVER(RerenderFont)( SFTFont font, int32_t width, int32_t height, PFRACTION width_scale, PFRACTION height_scale )
{
	PFONT_RENDERER renderer;
	INDEX idx;
	EnterCriticalSec( &fg.cs );

	LIST_FORALL( fonts, idx, PFONT_RENDERER, renderer )
	{
		if( renderer->font == font )
		{
			uint32_t n;
			for( n = 0; n < font->characters; n++ )
			{
				if( renderer->font->character[n] )
				{
					Deallocate( PCHARACTER, renderer->font->character[n] );
					renderer->font->character[n] = NULL;
				}
			}

			renderer->nLinesUsed = 0;

			if( width )
				renderer->nWidth = width;
			if( height )
				renderer->nHeight = height;

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

#undef output

IMAGE_NAMESPACE_END

