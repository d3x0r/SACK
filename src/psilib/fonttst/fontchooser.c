
#include <ft2build.h>
#include FT_FREETYPE_H

#include <sack_types.h>
#include <filesys.h>
#include <sharemem.h>

#include <image.h>
#include <render.h>
#include <controls.h>

typedef struct dictionary_entry_tag
{
	int ID;
   //PLIST pReferences; // entries which reference this.
   char word[1];
} DICT_ENTRY, *PDICT_ENTRY;

typedef struct size_file_tag
{
   PDICT_ENTRY path;
	int16_t width;
   int16_t height;
   char file[1];
} SIZE_FILE, *PSIZE_FILE;

typedef struct font_style_t
{
	struct {
		uint32_t mono : 1;
	} flags;
	PDICT_ENTRY name;
   PLIST  files; // list of PSIZE_FILEs (equate size->file)
} FONT_STYLE, *PFONT_STYLE;

typedef struct font_entry_tag
{
	PDICT_ENTRY name;
   PLIST styles; // List of PFONT_STYLEs
} FONT_ENTRY, *PFONT_ENTRY;

typedef struct global_tag
{
	struct {
		uint32_t initialized : 1;
		uint32_t show_mono_only : 1;
		uint32_t show_prop_only : 1;
	} flags;
	// this really has no sorting
   // just a list of FONT_ENTRYs
   PTREEROOT pFontCache;

   // PDICTs... (scratch build)
	PTREEROOT pPaths;
	PTREEROOT pFamilies;
	PTREEROOT pStyleNames;
	FT_Library library;
   PFRAME pFrame;
} GLOBAL;

static GLOBAL g;

//-------------------------------------------------------------------------

static void InitFont( void )
{
	if( !g.flags.initialized )
	{
		int error = FT_Init_FreeType( &g.library );
		if( error )
		{
			Log1( WIDE("Free type init failed: %d"), error );
		}
		else
         g.flags.initialized = 1;
	}
}

//-------------------------------------------------------------------------

void DestroyFontEntry( PFONT_ENTRY pfe, char *key )
{
	INDEX idx, idx2;
	PFONT_STYLE pfs;
	PSIZE_FILE psf;
	LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfs )
	{

		LIST_FORALL( pfs->files, idx2, PSIZE_FILE, psf )
		{
         Release( psf );
		}
		DeleteList( &pfs->files );
      Release( pfs );
	}
   DeleteList( &pfe->styles );
   Release( pfe );
}

//-------------------------------------------------------------------------

int UniqueStrCmp( char *s1, char *s2 )
{
	int dir;
	int num;
   char name[256];
	dir = strcmp( s1, s2 );
	if( dir == 0 )
	{
      char *numstart;
		if( ( numstart = strchr( s1, '[' ) ) )
		{
         numstart[0] = 0;
         num = atoi( numstart+1 );
         strcpy( name, s1 );
			sprintf( s1, WIDE("%s[%d]"), name, num+1 );
		}
		else
         strcat( s1, WIDE("[1]") );
      return 1;
	}
   return dir;
}

//-------------------------------------------------------------------------

void DestroyDictEntry( uintptr_t psvEntry, uintptr_t key )
{
   Release( (POINTER)psvEntry );
}

//-------------------------------------------------------------------------

PDICT_ENTRY AddDictEntry( PTREEROOT *root, char *name, uint32_t ID )
{
	PDICT_ENTRY pde;
	int len;
   if( !*root )
		*root = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
											, (int(*)(uintptr_t,uintptr_t))strcmp
											, DestroyDictEntry );

   len = strlen( name );
	pde = Allocate( sizeof( DICT_ENTRY ) + len );
	strcpy( pde->word, name );
	if( ID == INVALID_INDEX )
	{
		if( !AddBinaryNode( *root, pde, (uintptr_t)pde->word ) )
		{
			Release( pde );
			pde = (PDICT_ENTRY)FindInBinaryTree( *root, (uintptr_t)name );
		}
	}
   else
	{
      pde->ID = ID;
		if( !AddBinaryNode( *root, pde, ID ) )
		{
         Log( WIDE("Already built this path!?!?!") );
		}
	}
	return pde;
}


//-------------------------------------------------------------------------

// if ID != 0 use than rather than custom assignment.
PDICT_ENTRY AddPath( char *filepath, uint32_t ID, char **file )
{
	char *p;
	PDICT_ENTRY ppe;
	char save;
	if( file )
	{
		p = pathrchr( filepath );
		if( !p )
			return NULL;
		if( file )
			*file = p+1;
      save = p[0];
		p[0] = 0;
	}
	ppe = AddDictEntry( &g.pPaths, filepath, ID );
   p[0] = save; // restore character.
	return ppe;
}

//-------------------------------------------------------------------------


void SetIDs( PTREEROOT root )
{
	PDICT_ENTRY pde;
   uint32_t ID = 0;
	for( pde = (PDICT_ENTRY)GetLeastNode( root );
		 pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( root ) )
	{
      pde->ID = ID++;
	}
}

//-------------------------------------------------------------------------


PFONT_ENTRY AddFontEntry( PDICT_ENTRY name )
{
	PFONT_ENTRY pfe;
	if( !g.pFontCache )
	{
		g.pFontCache = CreateBinaryTreeEx( (int(*)(uintptr_t,uintptr_t))strcmp
													, (void(*)(uintptr_t,uintptr_t))DestroyFontEntry );
	}

	pfe = (PFONT_ENTRY)FindInBinaryTree( g.pFontCache, (uintptr_t)name->word );
	if( !pfe )
	{
		pfe = Allocate( sizeof( FONT_ENTRY ) );
		pfe->name = name;
		pfe->styles = NULL;
      AddBinaryNode( g.pFontCache, pfe, (uintptr_t)name->word );
	}
   return pfe;
}

//-------------------------------------------------------------------------

void AddSizeFile( PFONT_STYLE pfs, int16_t width, int16_t height, PDICT_ENTRY path, char *file )
{
	PSIZE_FILE psf = Allocate( sizeof( SIZE_FILE ) + strlen ( file ) );
	psf->width = width;
   psf->height = height;
	psf->path = path;
	strcpy( psf->file, file );
   AddLink( &pfs->files, psf );
}

//-------------------------------------------------------------------------

void ListFontFile( uintptr_t psv, char *name, int flags )
{
	FT_Face face;
	int error;
	PDICT_ENTRY ppe;
	PFONT_ENTRY pfe;
   PFONT_STYLE pfs;
   int32_t size;
   PDICT_ENTRY pFamilyEntry, pStyle;
   char *filename;
	InitFont();
	//Log1( WIDE("Try font: %s"), name );
	error = FT_New_Face( g.library
							 , name
							 , 0, &face );
	if( error == FT_Err_Unknown_File_Format )
	{
		Log1( WIDE("Unsupported font: %s"), name );
		return;
	}
	else if( error )
	{
		Log1( WIDE("Error loading font: %d"), error );
      return;
	}
	ppe = AddPath( name, INVALID_INDEX, &filename );

	pFamilyEntry = AddDictEntry( &g.pFamilies
										, face->family_name?face->family_name:"no-name"
										, INVALID_INDEX );

	pStyle = AddDictEntry( &g.pStyleNames
								, face->style_name, INVALID_INDEX);


	pfe = AddFontEntry( pFamilyEntry );
	{
      PFONT_STYLE pfsCheck = NULL;
		pfs = Allocate( sizeof( FONT_STYLE ) );
      pfs->files = NULL;
		pfs->name = pStyle;
      if( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
			pfs->flags.mono = 1;
      else
			pfs->flags.mono = 0;
      INDEX idx;
		LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfsCheck )
		{
			if( pfsCheck->name == pfs->name &&
				pfsCheck->flags.mono == pfs->flags.mono )
			{
            Release( pfs );
				pfs = pfsCheck;
            break;
			}
		}
		LIST_ENDFORALL();
		if( !pfsCheck )
         AddLink( &pfe->styles, pfs );
	}
   if(0)
	{
      char tmp[32];
		Log7( WIDE("%s family: %s style: %s %s %s(%d) %s")
			 , ((face->num_faces!=1)?sprintf( tmp, WIDE("Faces %d"), face->num_faces ), tmp:"")
			 , face->family_name
			 , face->style_name
			 , face->face_flags & FT_FACE_FLAG_SCALABLE?"scalable":""

			 , face->face_flags & FT_FACE_FLAG_FIXED_SIZES?"fixed":""
			 , face->num_fixed_sizes
			 , face->face_flags & FT_FACE_FLAG_FIXED_WIDTH?"mono":"var"
			 );
	}

	//if( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
	//	pfe->flags.mono = 1;
	size = -1;
	if( face->face_flags & FT_FACE_FLAG_FIXED_SIZES &&
	    face->available_sizes )
	{
		int n;
		if( face->face_flags & FT_FACE_FLAG_SCALABLE )
			AddSizeFile( pfs, -1, -1, ppe, filename );
      if( face->num_fixed_sizes )
			for( n = 0; n < face->num_fixed_sizes; n++ )
			{
				//Log2( WIDE("Font Size: %dx%d")
				//	 , face->available_sizes[n].width
				//	 , face->available_sizes[n].height );
				size = face->available_sizes[n].width << 16
					| face->available_sizes[n].height ;
				AddSizeFile( pfs
							  , face->available_sizes[n].width
							  , face->available_sizes[n].height
							  , ppe, filename );
			}
		else
		{
         Log( WIDE("Error adding font - it's fixed with no fixed defined") );
			//AddSizeFile( pfe, -2, -2, ppe, filename );
		}
	}
	else
		AddSizeFile( pfs, -1, -1, ppe, filename );

   FT_Done_Face( face );
   return;
}

//-------------------------------------------------------------------------

void DumpFontCache( void )
{
	FILE *out = fopen( WIDE("Fonts.Cache"), WIDE("wt") );
	PFONT_ENTRY pfe;
	PDICT_ENTRY pde;
	for( pde = (PDICT_ENTRY)GetLeastNode( g.pPaths );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( g.pPaths ) )
	{
      fprintf( out, WIDE("@%s\n"), pde->word );
	}
	for( pde = (PDICT_ENTRY)GetLeastNode( g.pFamilies );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( g.pFamilies ) )
	{
      fprintf( out, WIDE("$%s\n"), pde->word );
	}
	for( pde = (PDICT_ENTRY)GetLeastNode( g.pStyleNames );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( g.pStyleNames ) )
	{
		fprintf( out, WIDE("#%s\n"), pde->word );
	}

	for( pfe = (PFONT_ENTRY)GetLeastNode( g.pFontCache );
		  pfe;
        pfe = (PFONT_ENTRY)GetGreaterNode( g.pFontCache ) )
	{
		int linelen;
      int first = 1;
      int mono = 2;
      // should dump name also...
		linelen = fprintf( out, WIDE("%d")
							  , pfe->name->ID
							  );
		INDEX idx;
		PSIZE_FILE psf;
		PFONT_STYLE pfs;
		LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfs )
		{
			INDEX idx;
			if( first )
				linelen += fprintf( out, WIDE("!%d*%s")
										, pfs->name->ID
										, pfs->flags.mono?"m":"" );
			else
				linelen = fprintf( out, WIDE("\n!%d*%s")
										, pfs->name->ID
										, pfs->flags.mono?"m":"" );
         first = 0;
			if( pfs->flags.mono )
			{
				if( !mono )
					Log2( WIDE("Mono-spaced and var spaced together:%s %s")
						, pfe->name->word, pfs->name->word );
				mono = 1;
			}
			else
			{
				if( mono == 1 )
				{
					Log2( WIDE("Mono-spaced and var spaced together:%s %s")
						, pfe->name->word, pfs->name->word );
				}
			}
			LIST_FORALL(pfs->files, idx, PSIZE_FILE, psf )
			{
				char outbuf[80];
				int  newlen;
				if( psf->width < 0 )
					newlen = sprintf( outbuf, WIDE("#%d@%d:%s")
										 , psf->width
										 , psf->path->ID
										 , psf->file );
				else
					newlen = sprintf( outbuf, WIDE("#%d|%d@%d:%s")
										 , psf->width
										 , psf->height
										 , psf->path->ID
										 , psf->file );
				if( linelen + newlen > 80 )
				{
					fprintf( out, WIDE("\n\\") );
					linelen = newlen + 1;
				}
				else
					linelen += newlen;
				fwrite( outbuf, 1, newlen, out );
			}
			LIST_ENDFORALL();
		}
      LIST_ENDFORALL();
		fprintf( out, WIDE("\n") );
      linelen = 0;
	}
   LIST_ENDFORALL();
   fclose( out );
}

//-------------------------------------------------------------------------

void BuildFontCache( void )
{
	void *data = NULL;
   // .psf.gz doesn't load directly....
	while( ScanFiles( WIDE("/"), WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"), &data
						 , ListFontFile, SFF_SUBCURSE, 0 ) );

	SetIDs( g.pPaths );
	SetIDs( g.pFamilies );
   SetIDs( g.pStyleNames );
	DumpFontCache();
	// delete all memory associated with building the cache.
   DestroyBinaryTree( g.pFontCache );
	DestroyBinaryTree( g.pPaths );
	DestroyBinaryTree( g.pFamilies );
	DestroyBinaryTree( g.pStyleNames );
   g.pPaths = NULL;
   g.pFamilies = NULL;
	g.pStyleNames = NULL;
   g.pFontCache = NULL;
   DebugDumpMemFile( WIDE("cache.built") );
}

//-------------------------------------------------------------------------

int IsNumber( char *num )
{
	while( num[0] )
	{
		if( !isdigit( num[0] ) )
         return FALSE;
      num++;
	}
   return TRUE;
}

//-------------------------------------------------------------------------

void LoadAllFonts( void )
{
	FILE *in = fopen( WIDE("Fonts.Cache"), WIDE("rt") );
	if( !in )
	{
		BuildFontCache();
      in = fopen( WIDE("Fonts.Cache"), WIDE("rt") );
	}

   if( in )
	{
		char buf[256];
		int len;
		uint32_t PathID = 0;
		uint32_t FamilyID = 0;
		uint32_t StyleID = 0;
		while( fgets( buf, sizeof( buf ), in ) )
		{
			char *style, *flags, *next;
			PFONT_ENTRY pfe; // kept for contined things;
			PFONT_STYLE pfs;

			len = strlen( buf );
			buf[len-1] = 0; // kill \n on line.
			switch( buf[0] )
			{
			case '@':
				{
					if( !g.pPaths )
						g.pPaths = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
																, NULL
																, DestroyDictEntry );
					AddDictEntry( &g.pPaths, buf + 1, PathID++ );
				}
				break;
			case '$':
				{
					if( !g.pFamilies )
						g.pFamilies = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
																	, NULL, DestroyDictEntry );
					AddDictEntry( &g.pFamilies, buf + 1, FamilyID++ );
				}
				break;
			case '#':
				{
					if( !g.pStyleNames )
						g.pStyleNames = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
																	  , NULL
																	  , DestroyDictEntry );
					AddDictEntry( &g.pStyleNames, buf + 1, StyleID++ );
				}
            break;
			default:
				{
					char *family = buf;
					style = strchr( buf, '!' );
					PDICT_ENTRY pFamily;
					*(style++) = 0;
					pFamily = (PDICT_ENTRY)FindInBinaryTree( g.pFamilies, atol( family ) );
					pfe = Allocate( sizeof( FONT_ENTRY ) );
					pfe->name = pFamily;
					pfe->styles = NULL;
					if( !g.pFontCache )
						g.pFontCache = CreateBinaryTreeEx( NULL
																	, (void(*)(uintptr_t,uintptr_t))DestroyFontEntry );
               AddBinaryNode( g.pFontCache, pfe, pFamily->ID );
           		// add all the files/sizes associated on this line...
				}
				if( 0 )
				{
			case '!':
					style = buf + 1;
				}
				{
					// new font style here...
					flags = strchr( style, '*' );
					next = strchr( flags, '#' );
					if( next )
						*(next++) = 0;
					*(flags++) = 0;
					pfs = Allocate( sizeof(FONT_STYLE ) );
               MemSet( pfs, 0, sizeof( FONT_STYLE ) );
					while( flags[0] )
					{
						switch( flags[0] )
						{
						case 'm':
							pfs->flags.mono = 1;
                     break;
						}
                  flags++;
					}
					pfs->name = (PDICT_ENTRY)FindInBinaryTree( g.pStyleNames, atol( style ) );
               AddLink( &pfe->styles, pfs );
				}
				if(0)
				{
			case '\\':
               next = buf + 2;
				}
				{
					// continue size-fonts... (on style)
					char *width, *height;
					char *PathID;
					char *file;
					while( ( width = next ) )
					{
						PDICT_ENTRY ppe;
						int16_t nWidth, nHeight;
						nWidth = atoi( width );
						if( nWidth >= 0 )
						{
							height = strchr( width, '|' );
							*(height++) = 0;
							nHeight = atoi( height );
						}
						else
						{
							height = width;
							nHeight = -1;
						}
						PathID = strchr( height, '@' );
						file = strchr( PathID, ':' );
						next = strchr( file, '#' );
						*(PathID++) = 0;
						*(file++) = 0;
						if( next )
							*(next++) = 0;
						ppe = (PDICT_ENTRY)FindInBinaryTree( g.pPaths, atol( PathID ) );
						AddSizeFile( pfs, nWidth, nHeight, ppe, file );
					}
				}
			}
		}
		fclose( in );
		DumpFontCache();
	}
}

//-------------------------------------------------------------------------

int main( void )
{
	SetSystemLog( SYSLOG_FILE, stdout );
	SetAllocateDebug( TRUE );
   //SetAllocateLogging( TRUE );
	LoadAllFonts();
	{
		uint32_t a, b, c, d;
		GetMemStats( &a, &b, &c, &d );
		Log4( WIDE("Mem stats: Free: %ld Used: %ld Chunks: %ld FreeChunks: %ld"), a, b, c, d );
	}
	{
		extern void PickFont( int32_t x, int32_t y );
		PickFont( 0, 0 );
	}
   //DebugDumpMemFile( WIDE("memory.dump") );
   return 0;
}
// $Log: $
