#ifndef FONT_TYPES_DEFINED
#define FONT_TYPES_DEFINED

#include <sack_types.h>
#if defined( _OPENGL_DRIVER ) || defined( _D3D_DRIVER )
#include <vectlib.h>
#endif
#include <image.h>

#ifdef __cplusplus 
IMAGE_NAMESPACE

	namespace default_font { };
#endif


typedef struct font_char_tag
{
	_16 size;   // size of the character data (length of bitstream)
	_16 width;  // width to adjust position by (returned from putchar)
	S_16 offset; // minor width adjustment (leadin)
	_16 junk;   // I lost this junk padding?!
	S_16 ascent; // ascent can be negative also..
	S_16 descent;

	/* *** this bit of structure is for dyanmic rendering on surfaces *** */
	// data is byte aligned - count of bytes is (size/8) for next line...
	struct ImageFile_tag *cell;
	RCOORD x1, x2, y1, y2;
	struct font_char_tag *next_in_line; // NULL at end of line

	unsigned char data[1];
} CHARACTER, *PCHARACTER;

typedef struct font_tag
{
   // distance between 'lines' of text - the font may render above and below this height.
	_16 height; 
	 // distance from top-left origin to character baseline.
	 // the top of a character is now (y + baseline - ascent)
	 // if this is more than (y) the remainder must be
	 // filled with the background color.
	 // the bottom is ( y + baseline - descent ) - if this is
	// less than height the remainder must be background filled.
	_16 baseline;
	/* if 0 - characters will be 1 - old font - please compensate
	   for change.... */
	_32 characters;
	_8 flags;
	_8 bias;
	TEXTCHAR *name;
	PCHARACTER character[1];
} FONT, *PFONT;

enum FontFlags {
	FONT_FLAG_MONO = 0,
	FONT_FLAG_2BIT = 1,
	FONT_FLAG_8BIT = 2,
	FONT_FLAG_ITALIC = 0x10,
	FONT_FLAG_BOLD   = 0x20,
	FONT_FLAG_UPDATED = 0x40, // a character has been added to it since this was last cleared
	FONT_FLAG_COLOR = 0x80, // font is a full color font
};

typedef struct font_renderer_tag *PFONT_RENDERER;

void InternalRenderFontCharacter( PFONT_RENDERER renderer, PFONT font, INDEX idx );

IMAGE_NAMESPACE_END


#endif

