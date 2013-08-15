#ifndef FONT_CACHE_STRUCTS
#define FONT_CACHE_STRUCTS


typedef struct dictionary_entry_tag
{
	_32 ID;
	//PLIST pReferences; // entries which reference this.
	TEXTCHAR word[1];
} DICT_ENTRY, *PDICT_ENTRY;

typedef struct alt_size_file_tag
{
	PDICT_ENTRY path;
	PDICT_ENTRY file;
} ALT_SIZE_FILE, *PALT_SIZE_FILE;

typedef struct size_tag
{
	struct {
		_32 unusable : 1;
	} flags;
	S_16 width;
	S_16 height;
} SIZES, *PSIZES;

typedef struct size_file_tag
{
	struct {
		_32 unusable : 1;
	} flags;
	// and this type of size-file is only used in building the cache...
	PDICT_ENTRY path; // path name reference
	PDICT_ENTRY file; // file name reference
	_32   nAlternate;
	PLIST pAlternate; // list of alternate files
	_32   nSizes;
	PLIST sizes; // may be more than one size per size-file
	_8 SHA1[SHA1HashSize]; // only need on base file...
} SIZE_FILE, *PSIZE_FILE;

typedef struct app_size_file_tag
{
	struct {
		_32 unusable : 1;
	} flags;
	// and this type of size-file is only used in building the cache...
	TEXTCHAR * path; // path name reference
	TEXTCHAR * file; // file name reference
	_32   nAlternate;
	PALT_SIZE_FILE pAlternate; // list of alternate files
	_32   nSizes;
	PSIZES sizes; // may be more than one size per size-file
} APP_SIZE_FILE, *PAPP_SIZE_FILE;

//	_8 SHA1[SHA1HashSize];
typedef struct font_style_t
{
	struct {
		_32 mono : 1;
		_32 unusable : 1;
	} flags;
	PDICT_ENTRY name; // style name
	_32    nFiles;
	union {
		PLIST  files;     // list of PSIZE_FILEs (equate size->file)
		PAPP_SIZE_FILE appfiles;
	};
} FONT_STYLE, *PFONT_STYLE;

typedef struct font_entry_tag
{
	struct {
		_32 unusable : 1;
	} flags;
	PDICT_ENTRY name;
	_32   nStyles;
	PLIST styles; // List of PFONT_STYLEs
} FONT_ENTRY, *PFONT_ENTRY;

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
