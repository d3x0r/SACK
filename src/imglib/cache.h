#ifndef FONT_CACHE_STRUCTS
#define FONT_CACHE_STRUCTS


typedef struct cache_dictionary_entry_tag
{
	uint32_t ID;
	//PLIST pReferences; // entries which reference this.
	TEXTCHAR word[1];
} CACHE_DICT_ENTRY, *PCACHE_DICT_ENTRY;

typedef struct cache_alt_size_file_tag
{
	PCACHE_DICT_ENTRY path;
	PCACHE_DICT_ENTRY file;
} CACHE_ALT_SIZE_FILE, *PCACHE_ALT_SIZE_FILE;

typedef struct cache_size_tag
{
	struct {
		uint32_t unusable : 1;
	} flags;
	int16_t width;
	int16_t height;
} CACHE_SIZES, *PCACHE_SIZES;

typedef struct cache_short_size_file_tag
{
	struct {
		uint32_t unusable : 1;
	} flags;
	// and this type of size-file is only used in building the cache...
	PCACHE_DICT_ENTRY path; // path name reference
	PCACHE_DICT_ENTRY file; // file name reference
	uint32_t   nAlternate;
	PLIST pAlternate; // list of alternate files
	uint32_t   nSizes;
	PLIST sizes; // may be more than one size per size-file
} CACHE_SHORT_SIZE_FILE, *PCACHE_SHORT_SIZE_FILE;

typedef struct cache_size_file_tag
{
	struct {
		uint32_t unusable : 1;
	} flags;
	// and this type of size-file is only used in building the cache...
	PCACHE_DICT_ENTRY path; // path name reference
	PCACHE_DICT_ENTRY file; // file name reference
	uint32_t   nAlternate;
	PLIST pAlternate; // list of alternate files
	uint32_t   nSizes;
	PLIST sizes; // may be more than one size per size-file
	//uint8_t SHA1[SHA1HashSize]; // only need on base file...
} CACHE_SIZE_FILE, *PCACHE_SIZE_FILE;

typedef struct cache_app_size_file_tag
{
	struct {
		uint32_t unusable : 1;
	} flags;
	// and this type of size-file is only used in building the cache...
	TEXTCHAR * path; // path name reference
	TEXTCHAR * file; // file name reference
	uint32_t   nAlternate;
	PCACHE_ALT_SIZE_FILE pAlternate; // list of alternate files
	uint32_t   nSizes;
	PCACHE_SIZES sizes; // may be more than one size per size-file
} CACHE_APP_SIZE_FILE, *PCACHE_APP_SIZE_FILE;

//	uint8_t SHA1[SHA1HashSize];
typedef struct font_cache_style_t
{
	struct {
		uint32_t mono : 1;
		uint32_t unusable : 1;
		uint32_t italic : 1;
		uint32_t bold : 1;
	} flags;
	PCACHE_DICT_ENTRY name; // style name
	uint32_t    nFiles;
	union {
		PLIST  files;     // list of PSIZE_FILEs (equate size->file)
		PCACHE_APP_SIZE_FILE appfiles;
	};
} CACHE_FONT_STYLE, *PCACHE_FONT_STYLE;

typedef struct font_cache_entry_tag
{
	struct {
		uint32_t unusable : 1;
	} flags;
	PCACHE_DICT_ENTRY name;
	uint32_t   nStyles;
	PLIST styles; // List of PFONT_STYLEs
} CACHE_FONT_ENTRY, *PCACHE_FONT_ENTRY;

void LoadAllFonts( void );

#endif
// $Log: cache.h,v $
// Revision 1.4  2004/12/15 03:00:19  panther
// Begin coding to only show valid, renderable fonts in dialog, and update cache, turns out that we'll have to postprocess the cache to remove unused dictionary entries
//
// Revision 1.3  2004/10/24 20:09:46  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.1  2004/09/19 19:22:30  d3x0r
// Begin version 2 psilib...
//
// Revision 1.2  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
