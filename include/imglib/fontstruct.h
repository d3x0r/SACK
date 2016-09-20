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
	uint16_t size;   // size of the character data (length of bitstream)
	uint16_t width;  // width to adjust position by (returned from putchar)
	int16_t offset; // minor width adjustment (leadin)
	uint16_t junk;   // I lost this junk padding?!
	int16_t ascent; // ascent can be negative also..
	int16_t descent;

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
	uint16_t height; 
	 // distance from top-left origin to character baseline.
	 // the top of a character is now (y + baseline - ascent)
	 // if this is more than (y) the remainder must be
	 // filled with the background color.
	 // the bottom is ( y + baseline - descent ) - if this is
	// less than height the remainder must be background filled.
	uint16_t baseline;
	/* if 0 - characters will be 1 - old font - please compensate
	   for change.... */
	uint32_t characters;
	uint8_t flags;
	uint8_t bias;
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

