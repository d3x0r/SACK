#include <stdhdrs.h>
#include <sha1.h>
#ifndef STRUCT_ONLY
#include <ft2build.h>
#ifdef FT_FREETYPE_H
#include FT_FREETYPE_H
#endif
#endif

#include <deadstart.h>
#define fg (*global_font_data)

SACK_NAMESPACE
_IMAGE_NAMESPACE

#ifndef FONT_CACHE_STRUCTS

/* Describes an alternate file location for this font. During
   the building of the cache, the same fonts may exist on a
   system multiple times.                                     */
/* <combine sack::image::alt_size_file_tag>
   
   \ \                                      */
typedef struct alt_size_file_tag
{
   /* path of the file. */
   TEXTCHAR *path;
	/* more alternate files. */
	struct alt_size_file_tag *pAlternate;
   /* the filename appended to this structure. */
   TEXTCHAR file[1];
} ALT_SIZE_FILE, *PALT_SIZE_FILE;

/* Describes size information about a font family, style, and
   the file instance specified.                               */
/* <combine sack::image::size_tag>
   
   \ \                             */
/* <combine sack::image::size_tag>
   
   \ \                             */
typedef struct size_tag
{
	/* <combine sack::image::size_tag::flags>
	   
	   \ \                                    */
	/* flags to describe the state of this size entry. Sometimes
	   specific sizes of a font in a file cannot be rendered.    */
	struct {
		/* This font file was found to be unusable. This is mostly used
		   during the building of the font cache.                       */
		_32 unusable : 1;
	} flags;
	/* The height of the character (in uhmm something like PELS or
	   dots)                                                       */
	S_16 width;
	/* The height of the character (in uhmm something like PELS or
	   dots)                                                       */
	S_16 height;
   /* Next size. A SizeFile actually contains an array of lists of
      size entries. There may be more than one size entry base, but
      then each might point at another. (like if a font had an 8x8,
      9x12, 11x15 sizes, but also has -1x-1 scalable size, the
      first three will be a list, and then the scalable indicator)  */
   struct size_tag *next;
} SIZES, *PSIZES;

/* <link sack::image::font_global_tag::pFontCache, GetGlobalFont()-\>pFontCache> */
/* <combine sack::image::file_size_tag>
   
   \ \                                  */
/* <combine sack::image::file_size_tag>
   
   \ \                                  */
typedef struct file_size_tag
{
	/* Flags to indicate statuses about this font cache entry. */
	/* <combine sack::image::file_size_tag::flags>
	   
	   \ \                                         */
	struct {
		/* This font file was found to be unusable. This is mostly used
		   during the building of the font cache.                       */
		_32 unusable : 1;
	} flags;
   /* pointer to the path where the file is found */
	TEXTCHAR *path;
   /* name of the file itself */
	TEXTCHAR *file;
   /* number of alternate files specified. */
	_32   nAlt;
   /* list of alternates for this font */
	PALT_SIZE_FILE pAlternate;
   /* sizes that are in this file */
   _32   nSizes;
   PSIZES sizes; // scaled and fixed sizes available in this file.
} SIZE_FILE, *PSIZE_FILE;

/* describes a style of a font family. */
/* <combine sack::image::font_style_t>
   
   \ \                                 */
typedef struct font_style_t
{
	/* <combine sack::image::font_style_t::flags>
	   
	   \ \                                        */
	/* Flags describing characteristics of this style, and all sizes
	   contained.                                                    */
	struct {
		/* The font from here and all sizes is mono-spaced. */
		_32 mono : 1;
		/* This font file was found to be unusable. This is mostly used
		   during the building of the font cache.                       */
		_32 unusable : 1;
		/* set italic attribute when rendering. */
		_32 italic : 1;
		/* set bold attribute when rendering. */
		_32 bold : 1;
	} flags;
	/* name of this style. This is appended directly to the
	   structure to avoid any allocation overhead.          */
	TEXTCHAR        *name;
   /* number of files in the array of files. */
   _32          nFiles;
   /* pointer to array of SIZE_FILE s. */
   PSIZE_FILE   files;
} FONT_STYLE, *PFONT_STYLE;

/* <combine sack::image::font_entry_tag>
   
   \ \                                   */
/* <combine sack::image::font_entry_tag>
   
   \ \                                   */
typedef struct font_entry_tag
{
	/* Flags about this font family. Currently only 'unusable' that
	   the font failed to load... or there are no styles that
	   loaded?                                                      */
	/* <combine sack::image::font_entry_tag::flags@1>
	   
	   \ \                                            */
	struct {
		/* This font file was found to be unusable. This is mostly used
		   during the building of the font cache.                       */
		_32 unusable : 1;
	} flags;
	TEXTCHAR   *name;  // name of this font family.
   _32          nStyles; // number of styles in the styles array.
   PFONT_STYLE  styles; // array of nStyles
} FONT_ENTRY, *PFONT_ENTRY;

#endif

#if defined( NO_FONT_GLOBAL )
#else
/* Global information kept by the specific font subsystem. This
   was seperated from the guts of Image Global, because PSI SFTFont
   Picker dialog wants to get information from here (to browse
   the font cache for instance).                                 */
typedef struct font_global_tag
{
#if defined( STRUCT_ONLY )
	PTRSZVAL library;
#else
	/* This is the freetype instance variable that font rendering
	   uses. Thread access to rendering is controlled, only a single
	   thread may render using this instance at a time.              */
	FT_Library   library;
#endif
	/* \ \  */
	_32          nFonts;
	/* \ \ 
	   Description
	   This is an overview of the internal font cache. The
	   Fonts.cache file is a semi-compressed quick reference table
	   of compressed strings.
	   
	   
	   
	   The cache is an array of PFONT_ENTRY. Each entry has a name;
	   it is the name of the font as described by the font file.
	   
	   
	   
	   each PFONT_ENTRY has an array of PFONT_STYLE. Each style has
	   a name; it is usually something like Regular, Bold, Italic...
	   Styles also have a flag whether it is mono spaced.
	   
	   each PFONT_STYLE has an array of PSIZE_FILE. Each size_file entry
	   has an array of PSIZES. Each size_file also has a path and
	   filename associated. At this point, the file may be
	   different. It may also have a link to alternate files which
	   are the same font (style name and family name match...).
	   
	   each PSIZES describes a possible rendering size of the font. If
	   it is -1,-1, the font is scalable, otherwise specific x by y
	   needs to be specified.
	   
	   
	   
	   So To Reiterate
	   <code lang="c++">
	      FONT_ENTRY
	         FONT_STYLE
	             SIZE_FILE
	                SIZES
	   </code>
	   I really don't want to give you the loop to iterate this, but
	   I must if anyone else is to write font choice methods.
	   
	   
	   Example
	   See src/psilib/fntdlg.c                                           */
	FONT_ENTRY  *pFontCache;
	/* Critical section protecting global */
	CRITICALSECTION cs;
} FONT_GLOBAL;
#endif

/* These are symbols used for 'magic' in PFONTDATA and
   PRENDER_FONTDATA.                                   */
enum FontMagicIdentifiers{
   MAGIC_PICK_FONT = 'PICK', /* These are symbols used for 'magic' in PFONTDATA. */
   MAGIC_RENDER_FONT = 'FONT' /* These are symbols used for 'magic' in PRENDER_FONTDATA. */
};
/* This is the internal structure used to define the font cache,
   and data structures used to track renderings of fonts. Structures
   include desired size of the font in pixels. This data
   structure can be requested from the image library and can be
   used to recreate the font again in the future. This font came
   from a font choice dialog and includes all the IDs of the
   options selected.                                                 */
struct font_data_tag {
   /* A magic identifier from FontMagicIdentifiers. This structure
      might be pointed to as a RENDER_FONTDATA instead, and its
      magic identifier would be different.                         */
   _32 magic;
	/* this is the family index in the internal font cache. */
	_32 nFamily;
	/* This is the index into the font cache of this style. */
	_32 nStyle;
	/* This is the file index in the font cache. */
	_32 nFile;
	/* How wide to render the font in pixels. */
	_32 nWidth;
	/* Height of the font to render output in pixels. */
	_32 nHeight;
   /* This is the flags that the font was created with. Think only
      the low 2 bits are used to determine resolution of the font
      as 1, 2, or 8 bits.                                          */
   _32 flags;
	/* This is the timestamp of the cache file. If these don't
	   match, the names are used to create the font.           */
	_64 cachefile_time;
   /* these is a list of names to create the font if the indexes
      are different. Think all names are concated together with a
      single '\\0' between and a double '\\0\\0' at the end.      */
   TEXTCHAR names[];
};
// defines FONTDATA for internal usage.
typedef struct font_data_tag  FONTDATA;

/* This is used to define the data usable to recreate a font
   being rendered right now. This is the data that would result
   from calling
   
   <link sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32, RenderFontFile>,
   and would be used in future uses of <link sack::image::RenderScaledFontData@PFONTDATA@PFRACTION@PFRACTION, RenderScaledFontData>. */
typedef struct render_font_data_tag {
   /* A magic identifier from FontMagicIdentifiers. This structure
      might be pointed to as a FONTDATA instead, and its
      magic identifier would be different.                         */
   _32 magic;
	/* How wide to render the font in pixels. */
	_32 nWidth;
	/* Height of the font to render output in pixels. */
	_32 nHeight;
   /* This is the flags that the font was created with. Think only
      the low 2 bits are used to determine resolution of the font
      as 1, 2, or 8 bits.                                          */
	_32 flags;
   /* this is the filename that was used to create the font.  The filename is relative to where the image service is running from. */
	TEXTCHAR filename[];
} RENDER_FONTDATA;

/* internal function to load fonts */
void LoadAllFonts( void );
/* internal function to unload fonts */
void UnloadAllFonts( void );

#ifndef STRUCT_ONLY

int OpenFontFile( CTEXTSTR name, POINTER *font_memory, FT_Face *face, int face_idx, LOGICAL fallback_to_cache );
#endif

#if !defined( NO_FONT_GLOBAL )
static FONT_GLOBAL *global_font_data;
//#define fg (*global_font_data)
#endif
IMAGE_NAMESPACE_END

// $Log: fntglobal.h,v $
// Revision 1.8  2004/12/15 03:00:19  panther
// Begin coding to only show valid, renderable fonts in dialog, and update cache, turns out that we'll have to postprocess the cache to remove unused dictionary entries
//
// Revision 1.7  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.6  2003/10/07 00:37:34  panther
// Prior commit in error - Begin render fonts in multi-alpha.
//
// Revision 1.5  2003/10/07 00:32:08  panther
// Fix default font.  Add bit size flag to font
//
// Revision 1.4  2003/06/16 10:17:42  panther
// Export nearly usable renderfont routine... filename, size
//
// Revision 1.3  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
