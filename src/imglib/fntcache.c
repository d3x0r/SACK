/// This font cache keeps fonts in char format.


#define IMAGE_LIBRARY_SOURCE

#ifdef _PSI_INCLUSION_
//#line 2 "../imglib/psi_fntcache.c"
#endif

#define __CAN_USE_CACHE_DIALOG__
#ifdef SACK_MONOLITHIC_BUILD
#define __CAN_USE_TOPMOST__
#endif
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include <timers.h>
#include <sharemem.h>
#include <ft2build.h>
#ifdef __cplusplus
// isdigit ... hasn't been a problem elsewhere so just add this yere.
#include <ctype.h>
#endif
#ifdef FT_FREETYPE_H
#include FT_FREETYPE_H

#include <sack_types.h>
#include <filedotnet.h>
#include <filesys.h>
#include <sharemem.h>
#include <timers.h>

#include <sha1.h>

#include <controls.h>
//#include "global.h"

#ifndef SHA1HashSize
#define SHA1Context SHA1_CTX
#define SHA1HashSize SHA1_DIGEST_LENGTH
#define SHA1Reset SHA1Init
#define SHA1Input SHA1Update
#define SHA1Result(a,b) SHA1Final(b,a)
#endif

#define FONT_MAIN_SOURCE
#include "cache.h"
#include "fntglobal.h"

#ifdef __cplusplus_cli
#  define atoi IntCreateFromText
#  define atol IntCreateFromText
#endif

IMAGE_NAMESPACE

typedef struct cache_build_tag
{
	struct {
		_32 initialized : 1;
		_32 show_mono_only : 1;
		_32 show_prop_only : 1;
	} flags;
	// this really has no sorting
	// just a list of FONT_ENTRYs
	//SHA1Context Sha1Build;
	// PDICTs... (scratch build)
	PTREEROOT pPaths;
	PTREEROOT pFiles;
	PTREEROOT pFamilies;
	PTREEROOT pStyles;
	PTREEROOT pFontCache;

   // when we read the cache from disk - use these....
	_32 nPaths;
	TEXTCHAR* *pPathList;
	TEXTCHAR *pPathNames; // slab of ALL names?
	_32 nFiles;
	TEXTCHAR* *pFileList;
	TEXTCHAR *pFileNames; // slab of ALL names?
	_32 nStyles;
	TEXTCHAR* *pStyleList;
	TEXTCHAR *pStyleNames; // slab of ALL names?
	_32 nFamilies;
	TEXTCHAR* *pFamilyList;
	TEXTCHAR *pFamilyNames; // slab of ALL names?

	PFONT_STYLE pStyleSlab;
	_32 nStyle;
	PAPP_SIZE_FILE pSizeFileSlab;
	_32 nSizeFile;
	PSIZES pSizeSlab;
   _32 nSize;
	PALT_SIZE_FILE pAltSlab;
   _32 nAlt;

} CACHE_BUILD, *PCACHE_BUILD;

_64 fontcachetime;
static CACHE_BUILD build;

//-------------------------------------------------------------------------

#define fg (*global_font_data)
#ifdef _PSI_INCLUSION
PRIORITY_PRELOAD( CreatePSIFontCacheGlobal, IMAGE_PRELOAD_PRIORITY )
#else
PRIORITY_PRELOAD( CreateFontCacheGlobal, IMAGE_PRELOAD_PRIORITY + 1 )
#endif
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
#ifndef _PSI_INCLUSION_
 struct font_global_tag *  GetGlobalFonts( void )
{
   return &fg;
}
#endif

//-------------------------------------------------------------------------
#if 0
#if !defined( NO_FONT_GLOBAL_DECLARATION ) || defined(__LINUX__)
EXPORT_METHOD int InitFont( void )
{
	if( !&fg )
	{
		lprintf( WIDE("Image interface has not been loaded... bailing on InitFont()explain why %p is NULL!"), &fg );
		return 0;
	}
	if( !fg.library )
	{
//#ifdef IMAGE_LIBRARY_SOURCE
		int error = FT_Init_FreeType( &fg.library );
		if( error )
		{
			Log1( WIDE("Free type init failed: %d"), error );
			return 0;
		}
//#endif
	}
   return 1;
}
#else
//IMPORT_METHOD int InitFont(void);
#endif
#endif
//-------------------------------------------------------------------------

void LogSHA1Ex( TEXTCHAR *leader, PSIZE_FILE file DBG_PASS)
#define LogSHA1(l,f) LogSHA1Ex(l,f DBG_SRC )
{
	TEXTCHAR msg[256];
	int ofs;
	int n;
	ofs = snprintf( msg, sizeof( msg ), WIDE("%s: "), leader );
	for( n = 0; n < SHA1HashSize; n++ )
		ofs += snprintf( msg + ofs, sizeof(msg)-ofs*sizeof(TEXTCHAR), WIDE("%02X "), ((P_8)file->SHA1)[n] );
	SystemLogEx( msg DBG_RELAY );
}


void DoSHA1( PSIZE_FILE file )
{
	size_t size = 0;
	TEXTCHAR filename[256];
	SHA1Context Sha1Context;
	POINTER memmap;
	if( file->path && file->file )
	{
		snprintf( filename, sizeof( filename ), WIDE("%s/%s"), file->path->word, file->file->word );
		memmap = OpenSpace( NULL, filename, (PTRSZVAL*)&size );
		SHA1Reset( &Sha1Context );
		SHA1Input( &Sha1Context, (uint8_t*)memmap, size );
		SHA1Result( &Sha1Context, file->SHA1 );
		CloseSpace( memmap );
	}
	else
      DebugBreak();
}

//-------------------------------------------------------------------------

void CPROC DestroyFontEntry( PFONT_ENTRY pfe, TEXTCHAR *key )
{
	INDEX idx, idx2;
	PFONT_STYLE pfs;
	PSIZE_FILE psf;
	LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfs )
	{
		LIST_FORALL( pfs->files, idx2, PSIZE_FILE, psf )
		{
			PSIZES size;
			PALT_SIZE_FILE pasf;
			INDEX idx;
			LIST_FORALL( psf->sizes, idx, PSIZES, size )
			{
				Deallocate( PSIZES, size );
			}
			DeleteList( &psf->sizes );
			LIST_FORALL( psf->pAlternate, idx, PALT_SIZE_FILE, pasf )
			{
				Deallocate( PALT_SIZE_FILE, pasf );
			}
			DeleteList( &psf->pAlternate );
			Deallocate( PSIZE_FILE, psf );
		}
		DeleteList( &pfs->files );
		Deallocate( PFONT_STYLE, pfs );
	}
	DeleteList( &pfe->styles );
	Deallocate( PFONT_ENTRY, pfe );
}

//-------------------------------------------------------------------------

int UniqueStrCmp( TEXTCHAR *s1, INDEX s1_length, TEXTCHAR *s2 )
{
	int dir;
	int num;
	TEXTCHAR name[256];
	dir = strcmp( s1, s2 );
	if( dir == 0 )
	{
		TEXTCHAR *numstart;
#ifndef __cplusplus
		// strchr results with a CTEXTSTR type, override to remove warning
		// we're passing a non ctextstr in.. so result will be a non ctextstr
      // C++ has an overloaded function that returns the same type as what's passed
#define SANECAST (TEXTSTR)
#else
#define SANECAST
#endif
		if( ( numstart = SANECAST StrChr( s1, '[' ) ) )
		{
			numstart[0] = 0;
			num = atoi( numstart+1 );
			StrCpyEx( name, s1, 256 );
			snprintf( s1, sizeof( name ), WIDE("%s[%d]"), name, num+1 );
		}
		else
		{
			snprintf( s1, s1_length, WIDE("%s[1]"), s2 ); // s1 and s2 are equal, so this works...
		}
		return 1;
	}
	return dir;
}

//-------------------------------------------------------------------------

void CPROC DestroyDictEntry( POINTER psvEntry, PTRSZVAL key )
{
   Deallocate( POINTER, psvEntry );
}

//-------------------------------------------------------------------------

static int CPROC MyStrCmp( PTRSZVAL s1, PTRSZVAL s2 )
{
   return strcmp( (TEXTCHAR*)s1, (TEXTCHAR*)s2 );
}

//-------------------------------------------------------------------------

PDICT_ENTRY AddDictEntry( PTREEROOT *root, CTEXTSTR name )
{
	PDICT_ENTRY pde;
	size_t len;
	if( !*root )
		*root = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
											, MyStrCmp
											, DestroyDictEntry );

	len = strlen( name );
	pde = NewPlus( DICT_ENTRY, len*sizeof(pde->word[0]));
	StrCpyEx( pde->word, name, len + 1 );
	if( !AddBinaryNode( *root, pde, (PTRSZVAL)pde->word ) )
	{
		Deallocate( PDICT_ENTRY, pde );
		pde = (PDICT_ENTRY)FindInBinaryTree( *root, (PTRSZVAL)name );
	}
	return pde;
}


//-------------------------------------------------------------------------

// if ID != 0 use than rather than custom assignment.
PDICT_ENTRY AddPath( CTEXTSTR filepath, PDICT_ENTRY *file )
{
	TEXTSTR tmp = StrDup( filepath );
	TEXTSTR p;
	PDICT_ENTRY ppe;
	TEXTCHAR *filename;
	TEXTCHAR save;
	if( file )
	{
		p = SANECAST pathrchr( tmp );
		if( !p )
			return NULL;
		filename= p+1;
		save = p[0];
		p[0] = 0;
	}
	ppe = AddDictEntry( &build.pPaths, tmp );
	*file = AddDictEntry( &build.pFiles, filename );
	p[0] = save; // restore character.
	Deallocate( TEXTSTR, tmp );
	return ppe;
}

//-------------------------------------------------------------------------


void SetIDs( PTREEROOT root )
{
	PDICT_ENTRY pde;
	_32 ID = 0;
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
	if( !build.pFontCache )
	{
		build.pFontCache = CreateBinaryTreeEx( MyStrCmp
														 , (void(CPROC *)(POINTER,PTRSZVAL))DestroyFontEntry );
	}

	pfe = (PFONT_ENTRY)FindInBinaryTree( build.pFontCache, (PTRSZVAL)name->word );
	if( !pfe )
	{
		pfe = New( FONT_ENTRY );
		pfe->flags.unusable = 0;
		pfe->name = name;
		pfe->nStyles = 0;
		pfe->styles = NULL;
		AddBinaryNode( build.pFontCache, pfe, (PTRSZVAL)name->word );
	}
	return pfe;
}

//-------------------------------------------------------------------------

void AddAlternateSizeFile( PSIZE_FILE psfBase, PDICT_ENTRY path, PDICT_ENTRY file )
{
	PALT_SIZE_FILE psf = New( ALT_SIZE_FILE );
	psf->path = path;
	psf->file = file;
	psfBase->nAlternate++;
	AddLink( &psfBase->pAlternate, psf );
}

//-------------------------------------------------------------------------

void AddSizeToFile( PSIZE_FILE psf, S_16 width, S_16 height )
{
	if( psf )
	{
		PSIZES size = New( SIZES );
		size->flags.unusable = 0;
		size->width = width;
		size->height = height;
		psf->nSizes++;
		AddLink( &psf->sizes, size );
	}
}

//-------------------------------------------------------------------------

PSIZE_FILE AddSizeFileEx( PFONT_STYLE pfs
								, PDICT_ENTRY path
								, PDICT_ENTRY file
								, LOGICAL bTest
								DBG_PASS )
#define AddSizeFile(pfs,p,f,t) AddSizeFileEx(pfs,p,f,t DBG_SRC )
{
	PSIZE_FILE psf = New( SIZE_FILE );
	PSIZE_FILE psfCheck = NULL;
	INDEX idx;
	psf->flags.unusable = 0;
	psf->path = path;
	psf->nSizes = 0;
	psf->sizes = NULL;
	psf->nAlternate = 0;
	psf->pAlternate = NULL;
	psf->file = file;
	if( bTest )
		DoSHA1( psf );
	// Log( WIDE("Commpare sha1!") );
	if( bTest )
	{
		LIST_FORALL( pfs->files, idx, PSIZE_FILE, psfCheck )
		{
			//LogSHA1( WIDE("file: "), psf );
			//LogSHA1( WIDE("vs  : "), psfCheck );
			if( MemCmp( psf->SHA1
						 , psfCheck->SHA1
						 , SHA1HashSize ) == 0 )
			{
				//Log3( DBG_FILELINEFMT "File 1: (%d)%s/%s" DBG_RELAY
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->path->ID
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->path->word
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->file );
				//Log3( DBG_FILELINEFMT "File 2: (%d)%s/%s" DBG_RELAY , path->ID, path->word, file );
				//Log( WIDE("Duplicate found - storing as alterate.") );
				Deallocate( PSIZE_FILE, psf );
				AddAlternateSizeFile( psfCheck, path, file );
				return NULL;
			}
		}
	}
	if( !psfCheck )
	{
		pfs->nFiles++;
		AddLink( &pfs->files, psf );
	}
	return psf;
}

//-------------------------------------------------------------------------

#ifndef _PSI_INCLUSION_
void DumpFontCache( void )
{
	LoadAllFonts();
	{
		INDEX idx;
		for( idx = 0; idx < fg.nFonts; idx++ )
		{
			PFONT_ENTRY pfe;
			_32 s;
			PFONT_STYLE pfs;
			pfe = fg.pFontCache + idx;
			if( pfe->flags.unusable )
				continue;
			for( s = 0; s < pfe->nStyles; s++ )
			{
				PSHORT_SIZE_FILE file;
				_32 f;
				pfs = ((PFONT_STYLE)pfe->styles) + s;
				for( f = 0; f < pfs->nFiles; f++ )
				{
					_32 sz;
					file = ((PSHORT_SIZE_FILE)pfs->files) + f;
					for( sz = 0; sz < file->nSizes; sz++ )
					{
						PSIZES size = ((PSIZES)file->sizes) + sz;
						lprintf( WIDE("%s[%s] = %s/%s (%dx%d)"), (TEXTCHAR*)pfe->name
							, (TEXTCHAR*)pfs->name
							, (TEXTCHAR*)file->path
							, (TEXTCHAR*)file->file, size->width, size->height );
					}
				}
			}
		}
	}
}
#endif

int OpenFontFile( CTEXTSTR name, POINTER *font_memory, FT_Face *face, int face_idx, LOGICAL fallback_to_cache )
{
	int error;
	POINTER _font_memory = NULL;
	TEXTSTR temp_filename = NULL;
	TEXTSTR font_style;
	LOGICAL style_set = FALSE;
	size_t temp_filename_len = 0;
	PTRSZVAL size = 0;
	LOGICAL logged_error;
	if( !font_memory )
		font_memory = &_font_memory;

	if( font_style = (TEXTSTR)StrChr( name, '[' ) )
	{
		TEXTSTR end = (TEXTSTR)StrChr( font_style, ']' );
		if( end )
		{
			font_style++;
			end[0] = 0;
			font_style = StrDup( font_style );
			style_set = TRUE;
			end[0] = ']';
		}
		else
			font_style = WIDE("regular");
	}
	else
		font_style = WIDE("regular");

	if( !(*font_memory) )
	{
		(*font_memory) =
#ifdef UNDER_CE
			NULL;
#else
			OpenSpace( NULL, name, &size );
#endif
	}
#ifndef _PSI_INCLUSION_
	if( !(*font_memory) && fallback_to_cache )
	{
		CTEXTSTR base_name = pathrchr( name );
		INDEX idx;
		if( base_name )
			base_name++;
		else
			base_name = name;
		//lprintf( "Direct open failed... try seaching..." );
		// only open this if there's not a direct file ready.
		LoadAllFonts();
		for( idx = 0; idx < fg.nFonts; idx++ )
		{
			PFONT_ENTRY pfe;
			PFONT_STYLE pfs;
			_32 s;
			pfe = fg.pFontCache + idx;
			if( pfe->flags.unusable )
				continue;
			for( s = 0; s < pfe->nStyles; s++ )
			{
				PSHORT_SIZE_FILE file;
				_32 f;
				pfs = ((PFONT_STYLE)pfe->styles) + s;
				for( f = 0; f < pfs->nFiles; f++ )
				{
					file = ((PSHORT_SIZE_FILE)pfs->files) + f;
					if( StrCaseCmp( (TEXTCHAR*)file->file, base_name ) == 0 )
					{
						//lprintf( "Found font file matchint %s", file->file );
						temp_filename_len = (StrLen( (TEXTCHAR*)file->path ) + StrLen( (TEXTCHAR*)file->file ) + 2);
						temp_filename = NewArray( TEXTCHAR, temp_filename_len );
						snprintf( temp_filename, temp_filename_len, WIDE("%s/%s"), (TEXTCHAR*)file->path, (TEXTCHAR*)file->file );
						break;
					}
					if( StrCaseCmp( base_name, (TEXTCHAR*)pfe->name ) == 0 )
					{
						if( StrCaseCmp( (TEXTCHAR*)pfs->name, font_style ) )
						{
							temp_filename_len = (StrLen( (TEXTCHAR*)file->path ) + StrLen( (TEXTCHAR*)file->file ) + 2);
							temp_filename = NewArray( TEXTCHAR, temp_filename_len );
							snprintf( temp_filename, temp_filename_len, WIDE("%s/%s"), (TEXTCHAR*)file->path, (TEXTCHAR*)file->file );
							break;
						}
					}
				}
				if( f < pfs->nFiles )
					break;
			}
			if( s < pfe->nStyles )
				break;
		}
		if( temp_filename )
		{
			(*font_memory) =
#ifdef UNDER_CE
				NULL;
#else
				OpenSpace( NULL, temp_filename, &size );
#endif
		}
	}
#else
   lprintf( WIDE("!!!!!!!! PSI SHOULD NOT BE USING OPENFONTFILE !!!!!!!!!!!!") );
#endif
	if( *font_memory )
	{
		POINTER p = NewArray( _8, size );
		MemCpy( p, (*font_memory), size );
		Deallocate( POINTER, (*font_memory) );
		(*font_memory) = p;

		//lprintf( WIDE("Using memory mapped space...") );
		error = FT_New_Memory_Face( fg.library
										  , (FT_Byte*)(*font_memory)
										  , (FT_Long)size
										  , face_idx
										  , face );
	}
	else
	{
		char *file = DupTextToChar( name );
		//lprintf( WIDE("Using file access font... for %s"), name );
		error = FT_New_Face( fg.library
								 , file
								 , face_idx
								 , face );
		if( error )
		{
			char *file2 = DupTextToChar( temp_filename );
			error = FT_New_Face( fg.library
									 , file2
									 , face_idx
									 , face );
			Deallocate( char *, file2 );
		}
		if( error )
		{
			logged_error = 1;
			lprintf( WIDE("Failed to open font %s or %s Result %d")
					 , name?name:WIDE("<nofile>")
					 , temp_filename?temp_filename:WIDE("<nofile>")
					 , error );
		}
		Deallocate( char *, file );
	}
	if( style_set )
		Deallocate( TEXTCHAR*, font_style );
	if( temp_filename )
		Deallocate( TEXTCHAR*, temp_filename );
	return error;
}

static PFONT_STYLE AddFontStyle( PFONT_ENTRY pfe, PDICT_ENTRY name )
{
	PFONT_STYLE pfsCheck = NULL;
	INDEX idx;
	LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfsCheck )
	{
		if( pfsCheck->name == name )
		{
			break;
		}
	}
	if( !pfsCheck )
	{
		PFONT_STYLE pfs = New( FONT_STYLE );
		memset( pfs, 0, sizeof( FONT_STYLE ) );
		pfs->name = name;
		pfs->nFiles = 0;
		pfs->files = NULL;

		pfe->nStyles++;
		AddLink( &pfe->styles, pfs );
		return pfs;
	}
	return pfsCheck; // already exists
}

//-------------------------------------------------------------------------
static _32 fonts_checked;

void CPROC ListFontFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	FT_Face face;
	int face_idx;
	int num_faces;
	int error;
	PDICT_ENTRY ppe;
	PFONT_ENTRY pfe;
	PFONT_STYLE pfs[4];
	PDICT_ENTRY pFamilyEntry, pStyle, pFileName;
	fonts_checked++;
	//if( !InitFont() )
	//	return;
	//lprintf( WIDE("Try font: %s"), name );
	//#ifdef IMAGE_LIBRARY_SOURCE
	face_idx = 0;
	do
	{
		error = OpenFontFile( name, NULL, &face, face_idx, FALSE );
		if( error == FT_Err_Unknown_File_Format )
		{
			//Log1( WIDE("Unsupported font: %s"), name );
			return;
		}
		else if( error )
		{
			Log2( WIDE("Error loading font: %s(%d)"), name, error );
			return;
		}

		num_faces = face->num_faces;
		ppe = AddPath( name, &pFileName );

		{
			TEXTSTR name = DupCharToText( face->family_name?face->family_name:"no-name");
			pFamilyEntry = AddDictEntry( &build.pFamilies
												, name
												);
			Deallocate( TEXTSTR, name );
		}
		{
			TEXTSTR name = DupCharToText( face->style_name );
			pStyle = AddDictEntry( &build.pStyles
										, name );
			Deallocate( TEXTSTR, name );
		}

		pfe = AddFontEntry( pFamilyEntry );

		{
			pfs[0] = AddFontStyle( pfe, pStyle );
			if( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
				pfs[0]->flags.mono = 1;
			//else
         //   pfs->flags.variable = 1;
		}

		{
			PDICT_ENTRY pStyle;
			TEXTSTR style_name = DupCharToText( face->style_name?face->style_name:"no-style-name");
			TEXTCHAR buffer[256];
			
			snprintf( buffer, sizeof( buffer ), WIDE("%s+Italic"), style_name );
			pStyle = AddDictEntry( &build.pStyles, buffer );
			pfs[1] = AddFontStyle( pfe, pStyle );
			pfs[1]->flags.mono = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );
			pfs[1]->flags.italic = 1;

			snprintf( buffer, sizeof( buffer ), WIDE("%s+Bold"), style_name );
			pStyle = AddDictEntry( &build.pStyles, buffer );
			pfs[2] = AddFontStyle( pfe, pStyle );
			pfs[2]->flags.mono = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );
			pfs[2]->flags.bold = 1;

			snprintf( buffer, sizeof( buffer ), WIDE("%s+Bold-Italic"), style_name );
			pStyle = AddDictEntry( &build.pStyles, buffer );
			pfs[3] = AddFontStyle( pfe, pStyle );
			pfs[3]->flags.mono = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );
			pfs[3]->flags.italic = 1;
			pfs[3]->flags.bold = 1;
         Deallocate( TEXTSTR, style_name );
		}

		if( face->face_flags & FT_FACE_FLAG_FIXED_SIZES &&
			face->available_sizes )
		{
			int style;
			int n;
			for( style = 0; style < 4; style++ )
			{
				PSIZE_FILE psf = AddSizeFile( pfs[style], ppe, pFileName, TRUE );
				// if( !psf ) we already had the file...
				// no reason to add these sizes...
				if( psf )
				{
					if( face->face_flags & FT_FACE_FLAG_SCALABLE )
						AddSizeToFile( psf, -1, -1 );
					if( face->num_fixed_sizes )
					{
						//Log( WIDE("Adding fixed sizes") );
						for( n = 0; n < face->num_fixed_sizes; n++ )
						{
							//Log2( WIDE("Added size %d,%d")
							//	 , face->available_sizes[n].width
							//	 , face->available_sizes[n].height
							//	 );
							AddSizeToFile( psf
											 , face->available_sizes[n].width
											 , face->available_sizes[n].height
											 );
						}
					}
					else
					{
						Log( WIDE("Error adding font - it's fixed with no fixed defined") );
						//AddSizeFile( pfe, -2, -2, ppe, filename, TRUE );
					}
				}
				else
				{
					//Log1( WIDE("Alternate font of %s"), pfe->name->word );
				}
			}
		}
		else
		{
			int style;
			for( style = 0; style < 4; style++ )
			{
				PSIZE_FILE psf = AddSizeFile( pfs[style], ppe, pFileName, TRUE );
				if( psf )
					AddSizeToFile( psf, -1, -1 );
				else
				{
					//Log1( WIDE("(Scaalable only)Alternate font of %s"), pfe->name->word );
				}
			}
		}

		//#ifdef IMAGE_LIBRARY_SOURCE
		FT_Done_Face( face );
		//#endif
		face_idx++;
	} while( face_idx < num_faces );
	return;
}

//-------------------------------------------------------------------------

void OutputFontCache( void )
{
	FILE *out;
	PFONT_ENTRY pfe;
	PDICT_ENTRY pde;
	size_t size;
	Fopen( out, WIDE("Fonts.Cache"), WIDE("wt") );
	if( !out )
		return; // no point.
//	for( pfe = (PFONT_ENTRY)GetLeastNode( build.pFontCache );
//		  pfe;
//		  pfe = (PFONT_ENTRY)GetGreaterNode( build.pFontCache ) )
//	{
//		if( pfe->flags.unusable )
 //        RemoveBinaryNode( build.pFontCache, pfe, pfe->name->word );
 //  }


	if( build.pPaths )
	{
		for( size = 0, pde = (PDICT_ENTRY)GetLeastNode( build.pPaths );
	        pde;
			  (size += strlen( pde->word ) + 1), pde = (PDICT_ENTRY)GetGreaterNode( build.pPaths ) );
		fprintf( out, WIDE("%%@%") _32f WIDE(",%") _size_f WIDE("\n"), GetNodeCount( build.pPaths ), size );
	}
	if( build.pFamilies )
	{
		for( size = 0, pde = (PDICT_ENTRY)GetLeastNode( build.pFamilies );
   	     pde;
			  (size += strlen( pde->word ) + 1),pde = (PDICT_ENTRY)GetGreaterNode( build.pFamilies ) );
		fprintf( out, WIDE("%%$%") _32f WIDE(",%") _size_f WIDE("\n"), GetNodeCount( build.pFamilies ), size );
	}
	if( build.pStyles )
	{
		for( size = 0,pde = (PDICT_ENTRY)GetLeastNode( build.pStyles );
   	     pde;
			  (size += strlen( pde->word ) + 1),pde = (PDICT_ENTRY)GetGreaterNode( build.pStyles ) );
	   fprintf( out, WIDE("%%*%") _32f WIDE(",%") _size_f WIDE("\n"), GetNodeCount( build.pStyles ), size );
	}
	if( build.pFiles )
	{
		for( size = 0,pde = (PDICT_ENTRY)GetLeastNode( build.pFiles );
   	     pde;
			  (size += strlen( pde->word ) + 1),pde = (PDICT_ENTRY)GetGreaterNode( build.pFiles ) );
	   fprintf( out, WIDE("%%&%") _32f WIDE(",%") _size_f WIDE("\n"), GetNodeCount( build.pFiles ), size );
	}

	// slightly complex loop to scan cache as it is, and
   // write out total styles, alt files, and sizes...
	{
		_32 nStyles = 0;
		_32 nSizeFiles = 0;
		_32 nAltFiles = 0;
		_32 nSizes = 0;
		for( pfe = (PFONT_ENTRY)GetLeastNode( build.pFontCache );
			  pfe;
			  pfe = (PFONT_ENTRY)GetGreaterNode( build.pFontCache ) )
		{
			INDEX idx;
			PFONT_STYLE pfs;
			if( pfe->flags.unusable )
				continue;
			LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfs )
			{
				INDEX idx;
				PSIZE_FILE psf;
				nStyles++;
				LIST_FORALL(pfs->files, idx, PSIZE_FILE, psf )
				{
					if( psf->flags.unusable )
						continue;
					nSizeFiles++;
					{
						INDEX idx;
						PSIZES size;
						LIST_FORALL( psf->sizes, idx, PSIZES, size )
						{
							if( size->flags.unusable )
								continue;
							nSizes++;
						}
					}
					{
						INDEX idx;
						PALT_SIZE_FILE pasf;
						LIST_FORALL( psf->pAlternate, idx, PALT_SIZE_FILE, pasf )
						{
							nAltFiles++;
						}
					}
				}
			}
		}
		fprintf( out, WIDE("%%#%") _32f WIDE(",%") _32f WIDE(",%") _32f WIDE(",%") _32f WIDE("\n"), nStyles, nSizeFiles, nSizes, nAltFiles );
	}

	for( pde = (PDICT_ENTRY)GetLeastNode( build.pPaths );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( build.pPaths ) )
	{
      fprintf( out, WIDE("@%s\n"), pde->word );
	}

	for( pde = (PDICT_ENTRY)GetLeastNode( build.pFamilies );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( build.pFamilies ) )
	{
      fprintf( out, WIDE("$%s\n"), pde->word );
	}

	for( pde = (PDICT_ENTRY)GetLeastNode( build.pStyles );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( build.pStyles ) )
	{
		fprintf( out, WIDE("*%s\n"), pde->word );
	}

	for( pde = (PDICT_ENTRY)GetLeastNode( build.pFiles );
        pde;
		  pde = (PDICT_ENTRY)GetGreaterNode( build.pFiles ) )
	{
		fprintf( out, WIDE("&%s\n"), pde->word );
	}



	for( pfe = (PFONT_ENTRY)GetLeastNode( build.pFontCache );
		  pfe;
        pfe = (PFONT_ENTRY)GetGreaterNode( build.pFontCache ) )
	{
		int linelen;
		int mono = 2;
		INDEX idx;
		PSIZE_FILE psf;
		PFONT_STYLE pfs;
		TEXTCHAR outbuf[80];
		int  newlen;
		// should dump name also...
		linelen = fprintf( out, WIDE("%") _32f WIDE(",%") _32f WIDE("")
							  , pfe->name->ID
							  , pfe->nStyles
							  );
		LIST_FORALL( pfe->styles, idx, PFONT_STYLE, pfs )
		{
			INDEX idx;
			newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("!%") _32f WIDE("*%s%s%s,%") _32f WIDE("")
								 , pfs->name->ID
								 , pfs->flags.mono?WIDE("m"):WIDE("")
								 , pfs->flags.italic?WIDE("i"):WIDE("")
								 , pfs->flags.bold?WIDE("b"):WIDE("")
								 , pfs->nFiles );
			if( ( newlen + linelen ) >= 80 )
			{
				fprintf( out, WIDE("\n") );
				linelen = 0;
			}
			linelen += newlen;
			fputs( outbuf, out );

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
				PSIZES size;
				INDEX idx;
				newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("@%") _32f WIDE(",%") _32f WIDE(",%") _32f WIDE(":%") _32f WIDE("")
									 , psf->nAlternate
                            , psf->nSizes
									 , psf->path->ID
									 , psf->file->ID
									 );

				if( linelen + newlen >= 80 )
				{
					fprintf( out, WIDE("\n\\") );
					linelen = 1;
				}
				linelen += newlen;
				fputs( outbuf, out );

				LIST_FORALL( psf->sizes, idx, PSIZES, size )
				{
					if( size->width < 0 )
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("#%d")
											 , size->width );
					else
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("#%d,%d")
											 , size->width
											 , size->height );
					if( linelen + newlen >= 80 )
					{
						fprintf( out, WIDE("\n") );
						linelen = newlen;
					}
					else
						linelen += newlen;
					fputs( outbuf, out );
				}

				{
					INDEX idx;
					PALT_SIZE_FILE pasf;
					LIST_FORALL( psf->pAlternate, idx, PALT_SIZE_FILE, pasf )
					{
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("^%") _32fs WIDE(":%") _32fs WIDE("")
											 , pasf->path->ID
											 , pasf->file->ID );
						if( linelen + newlen >= 80 )
						{
							fprintf( out, WIDE("\n|") );
							linelen = newlen + 1;
						}
						else
							linelen += newlen;
						fputs( outbuf, out );
					}
				}
				linelen += fprintf( out, WIDE("^:") );
			}
		}
		fprintf( out, WIDE("\n") );
		linelen = 0;
	}
	sack_fclose( out );
}

#ifdef _DEBUG

INDEX IndexOf( TEXTCHAR **list, _32 count, POINTER item )
{
	INDEX idx;
	for( idx = 0; idx < count; idx++)
	{
		if( list[idx] == item )
         return idx;
	}
	return INVALID_INDEX;
}

//-------------------------------------------------------------------------

void DumpLoadedFontCache( void )
{
	FILE *out;
	PFONT_ENTRY pfe;
	_32 fontidx, idx;
	Fopen( out, WIDE("Fonts.Cache1"), WIDE("wt") );
	if( !out )
		return;
	fprintf( out, WIDE("%%@%") _32f WIDE(",%") _size_f WIDE("\n"), build.nPaths, SizeOfMemBlock( build.pPathNames ) );
	fprintf( out, WIDE("%%$%") _32f WIDE(",%") _size_f WIDE("\n"), build.nFamilies, SizeOfMemBlock( build.pFamilyNames ) );
	fprintf( out, WIDE("%%*%") _32f WIDE(",%") _size_f WIDE("\n"), build.nStyles, SizeOfMemBlock( build.pStyleNames ) );
	fprintf( out, WIDE("%%&%") _32f WIDE(",%") _size_f WIDE("\n"), build.nFiles, SizeOfMemBlock( build.pFileNames ) );

	{
		INDEX fontidx, styleidx, idx;
		PFONT_ENTRY pfe;
		_32 nStyles = 0;
		_32 nSizeFiles = 0;
		_32 nAltFiles = 0;
		_32 nSizes = 0;
		for( fontidx = 0; pfe = fg.pFontCache + fontidx, fontidx < fg.nFonts; fontidx++ )
		{
			PAPP_SIZE_FILE psf;
			PFONT_STYLE pfs;
			nStyles += pfe->nStyles;
			for( styleidx = 0; pfs = ((PFONT_STYLE)pfe->styles) + styleidx, styleidx < pfe->nStyles; styleidx++ )
			{
				nSizeFiles += pfs->nFiles;
				for( idx = 0; psf = pfs->appfiles + idx, idx < pfs->nFiles; idx++ )
				{
					nSizes    += psf->nSizes;
					nAltFiles += psf->nAlternate;
				}
			}
		}
		fprintf( out, WIDE("%%#%") _32f WIDE(",%") _32f WIDE(",%") _32f WIDE(",%") _32f WIDE("\n"), nStyles, nSizeFiles, nSizes, nAltFiles );
	}

	for( idx = 0; idx < build.nPaths; idx++ )
      fprintf( out, WIDE("@%s\n"), build.pPathList[idx] );
	for( idx = 0; idx < build.nFamilies; idx++ )
      fprintf( out, WIDE("$%s\n"), build.pFamilyList[idx] );
	for( idx = 0; idx < build.nStyles; idx++ )
      fprintf( out, WIDE("*%s\n"), build.pStyleList[idx] );
	for( idx = 0; idx < build.nFiles; idx++ )
      fprintf( out, WIDE("&%s\n"), build.pFileList[idx] );


   for( fontidx = 0; pfe = fg.pFontCache + fontidx, fontidx < fg.nFonts; fontidx++ )
	{
		int linelen;
		PAPP_SIZE_FILE psf;
		PFONT_STYLE pfs;
		TEXTCHAR outbuf[80];
		int  newlen;
		INDEX idx, styleidx;
		if( pfe->flags.unusable )
			continue;
		// should dump name also...
		linelen = fprintf( out, WIDE("%") _size_f WIDE(",%") _32f WIDE("")
						, IndexOf( build.pFamilyList, build.nFamilies, pfe->name )
                        , pfe->nStyles
							  );
		for( styleidx = 0; styleidx < pfe->nStyles; styleidx++ )
		{
			pfs = (PFONT_STYLE)pfe->styles + styleidx;
			newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("!%") _size_f WIDE("*%s,%") _32f WIDE("")
								 , IndexOf( build.pStyleList, build.nStyles, pfs->name )
								 , pfs->flags.mono?"m":""
								 , pfs->nFiles
								 );
			if( ( linelen + newlen ) >= 80 )
			{
				fprintf( out, WIDE("\n") );
				linelen = 0;
			}
			linelen += newlen;
			fputs( outbuf, out );
			for( idx = 0; psf = pfs->appfiles + idx, idx < pfs->nFiles; idx++ )
			{
				INDEX idx;
				PSIZES size;
				newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("@%") _32f WIDE(",%") _32f WIDE(",%") _size_f WIDE(":%") _size_f
                            , psf->nAlternate
                            , psf->nSizes
									 , IndexOf( build.pPathList, build.nPaths, psf->path )
									 , IndexOf( build.pFileList, build.nFiles, psf->file )
									 );
				if( linelen + newlen >= 80 )
				{
					fprintf( out, WIDE("\n\\") );
					linelen = newlen + 1;
				}
				else
					linelen += newlen;
				fputs( outbuf, out );
				for( idx = 0; size = ((PSIZES)psf->sizes) + idx, idx < psf->nSizes; idx++ )
				{
					if( size->width < 0 )
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("#%d")
											 , size->width );
					else
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("#%d,%d")
											 , size->width
											 , size->height );
					if( linelen + newlen >= 80 )
					{
						fprintf( out, WIDE("\n") );
						linelen = newlen;
					}
					else
						linelen += newlen;
					fputs( outbuf, out );
				}

				{
					PALT_SIZE_FILE pasf;
					INDEX idx;
					for( idx = 0; pasf = ((PALT_SIZE_FILE)psf->pAlternate) + idx, idx < psf->nAlternate; idx++ )
					{
						newlen = snprintf( outbuf, sizeof( outbuf ), WIDE("^%") _size_f WIDE(":%") _size_f 
											 , IndexOf( build.pPathList, build.nPaths, pasf->path )
											 , IndexOf( build.pFileList, build.nFiles, pasf->file )
											 );
						if( linelen + newlen >= 80 )
						{
							fprintf( out, WIDE("\n|") );
							linelen = newlen + 1;
						}
						else
							linelen += newlen;
						fputs( outbuf, out );
					}
				}
				linelen += fprintf( out, WIDE("^:") );
			}
		}
		fprintf( out, WIDE("\n") );
		linelen = 0;
	}
	sack_fclose( out );
}
#endif

//-------------------------------------------------------------------------

#define SafeRelease(n)  if(build.n) { Deallocate( POINTER, build.n ); build.n = NULL; }
void UnloadFontBuilder( void )
{
	if( build.pFontCache )
	{
		DestroyBinaryTree( build.pFontCache );
		build.pFontCache = NULL;
	}
	if( build.pPaths )
	{
		DestroyBinaryTree( build.pPaths );
		build.pPaths = NULL;
	}
	if( build.pFamilies )
	{
		DestroyBinaryTree( build.pFamilies );
		build.pFamilies = NULL;
	}
	if( build.pStyles )
	{
		DestroyBinaryTree( build.pStyles );
		build.pStyles = NULL;
	}
	if( build.pFiles )
	{
		DestroyBinaryTree( build.pFiles );
		build.pFiles = NULL;
	}
}

//-------------------------------------------------------------------------

#define TXT_STATUS  100
#define TXT_TIME_STATUS 101
#define TXT_COUNT_STATUS 102
static _32 StartTime;
static int TimeElapsed;

void CPROC UpdateStatus( PTRSZVAL psvFrame )
{
	TEXTCHAR msg[256];
	if( !StartTime )
	{
		StartTime = GetTickCount();
	}
	TimeElapsed = GetTickCount() - StartTime;
	snprintf( msg, sizeof( msg ), WIDE("Elapsed time: %d:%02d")
			 , (TimeElapsed/1000) / 60
			 , (TimeElapsed/1000) % 60
			 );
	SystemLog( msg );
#ifdef __CAN_USE_CACHE_DIALOG__
   // in the positiion of image library, there is no controls to do...
	SetCommonText( GetControl( (PCOMMON)psvFrame, TXT_TIME_STATUS ), msg );
#endif
	snprintf( msg, sizeof( msg ), WIDE("Checked Fonts: %d")
			 , fonts_checked
			 );
	SystemLog( msg );
#ifdef __CAN_USE_CACHE_DIALOG__
	SetCommonText( GetControl( (PCOMMON)psvFrame, TXT_COUNT_STATUS ), msg );
#endif
}

//-------------------------------------------------------------------------

void CPROC ScanDrive( PTRSZVAL user, TEXTCHAR *letter, int flags )
{
	TEXTCHAR base[5];
	void *data = NULL;
	snprintf( base, sizeof( base ), WIDE("%c:"), letter[0] );
	if( letter[0] != 'c' && letter[0] != 'C' )
		return;
	while( ScanFiles( base
						 , WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz")
						 , &data
						 , ListFontFile
						 , SFF_SUBCURSE, 0 ) );

}

void BuildFontCache( void )
{
	void *data = NULL;
	_32 timer;
#ifdef __CAN_USE_CACHE_DIALOG__
	PCOMMON status;
	status = CreateFrame( WIDE("Font Cache Status")
							  , 0, 0
							  , 250, 60
							  , 0, 0 );
	MakeTextControl( status, 5, 5, 240, 19, TXT_STATUS, WIDE("Building font cache..."), 0 );
	MakeTextControl( status, 5, 25, 240, 19, TXT_TIME_STATUS, WIDE(""), 0 );
	MakeTextControl( status, 5, 45, 240, 19, TXT_COUNT_STATUS, WIDE(""), 0 );
#endif
	lprintf( WIDE("Building cache...") );
	StartTime = 0;
#ifdef __CAN_USE_CACHE_DIALOG__
	DisplayFrame( status );
#  ifdef __CAN_USE_TOPMOST__
	MakeTopmost( GetFrameRenderer( status ) );
#  endif
#endif
#ifdef __CAN_USE_CACHE_DIALOG__
	timer = AddTimer( 100, UpdateStatus
		, (PTRSZVAL)status
		);
#else
	timer = AddTimer( 100, UpdateStatus
		, 0
		);
#endif

	// .psf.gz doesn't load directly.... 
	while( ScanFiles( WIDE("."), WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"), &data
						 , ListFontFile, SFF_SUBCURSE, 0 ) );

   // scan windows/fonts directory
	{
#ifdef HAVE_ENVIRONMENT
            CTEXTSTR name
#ifdef WIN32
                = OSALOT_GetEnvironmentVariable( WIDE( "windir" ) );
#else
            = "/usr/share/fonts";
#endif
            {
		size_t len;
		TEXTSTR tmp = NewArray( TEXTCHAR, len = strlen( name ) + 10 );
		snprintf( tmp, len * sizeof( TEXTCHAR ), WIDE( "%s\\fonts" ), name );
		while( ScanFiles( tmp, WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"), &data
							 , ListFontFile, SFF_SUBCURSE, 0 ) );
                Deallocate( TEXTSTR, tmp );
            }
#endif
	}
#ifdef __LINUX__	                     	
	//while( ScanFiles( WIDE("/."), WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"), &data
	//					 , ListFontFile, SFF_SUBCURSE, 0 ) );
#else
	//ScanDrives( ScanDrive, (PTRSZVAL)NULL );
	//while( ScanFiles( WIDE("/."), WIDE("*.ttf\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"), &data
	//					 , ListFontFile, SFF_SUBCURSE, 0 ) );
#endif

	SetIDs( build.pPaths );
	SetIDs( build.pFamilies );
	SetIDs( build.pStyles );
	SetIDs( build.pFiles );
	OutputFontCache();
	// delete all memory associated with building the cache.
	UnloadFontBuilder();
	//DebugDumpMemFile( WIDE("cache.built") );
   // it might be dispatched...
	RemoveTimer( timer );
#ifdef __CAN_USE_CACHE_DIALOG__
	DestroyFrame( &status );
#endif
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
	FILE *in = NULL;
	//if( !InitFont() )
	//return;
	if( fg.pFontCache )
	{
      // if it was loaded, don't re-load it.
		return;
	}
	Fopen( in, WIDE("Fonts.Cache"), WIDE("rt") );
	if( !in )
	{
		BuildFontCache(); // destroys the trees used to create the cache...
		Fopen( in, WIDE("Fonts.Cache"), WIDE("rt") );
	}
	if( in )
	{
		// lines in cache are built to be limited to 80...
		TEXTCHAR fgets_buf[128];
		TEXTCHAR *buf;
		size_t len;
		_32 PathID = 0;
		size_t PathOfs = 0;
		_32 FamilyID = 0;
		size_t FamilyOfs = 0;
		_32 StyleID = 0;
		size_t StyleOfs = 0;
		_32 FileID = 0;
		size_t FileOfs = 0;
		//_32 line = 0;
		_32 nFont = 0; // which font we're currently reading for.
		_64 tmpfiletime = GetFileWriteTime( WIDE("Fonts.Cache") );
		if( fontcachetime == tmpfiletime )
		{
			sack_fclose( in );
			return;  // already read.
		}
		fontcachetime = tmpfiletime;

		SafeRelease( pPathNames );
		SafeRelease( pFamilyNames );
		SafeRelease( pStyleNames );
		SafeRelease( pFileNames );
		SafeRelease( pStyleSlab );
		SafeRelease( pSizeFileSlab );
		SafeRelease( pSizeSlab );
		SafeRelease( pAltSlab );
		SafeRelease( pPathList );
		SafeRelease( pFamilyList );
		SafeRelease( pStyleList );
		SafeRelease( pFileList );
		if( fg.pFontCache )
		{
			Deallocate( POINTER, fg.pFontCache );
			fg.pFontCache = NULL;
		}

		while( fgets( fgets_buf, sizeof( fgets_buf ), in ) )
		{
			TEXTCHAR *style, *flags, *next, *count;
			PFONT_ENTRY pfe; // kept for contined things;
			PFONT_STYLE pfs;
			PAPP_SIZE_FILE  psfCurrent;
			buf = fgets_buf;
			len = strlen( buf );
			buf[len-1] = 0; // kill \n on line.
			//Log2( WIDE("Process: (%d)%s"), ++line, buf );
			switch( buf[0] )
			{
            size_t len;
			case '%':
				switch( buf[1] )
				{
				case '@':
					build.nPaths = atoi( buf + 2 );
					build.pPathNames = NewArray( TEXTCHAR, atoi( strchr( buf, ',' ) + 1 ) );
					build.pPathList = NewArray( TEXTCHAR*, build.nPaths );
					break;
				case '$':
					build.nStyle = 0;
					build.nFamilies = atoi( buf + 2 );
					build.pFamilyNames = NewArray( TEXTCHAR, atoi( strchr( buf, ',' ) + 1 ) );
					build.pFamilyList = NewArray( TEXTCHAR*, build.nFamilies );

					fg.nFonts     = build.nFamilies;
					fg.pFontCache = NewArray( FONT_ENTRY, build.nFamilies );
					break;
				case '*':
					build.nStyles = atoi( buf + 2 );
					build.pStyleNames = NewArray( TEXTCHAR, atoi( strchr( buf, ',' ) + 1 ) );
					build.pStyleList = NewArray( TEXTCHAR*, build.nStyles );
					break;
				case '&':
					build.nFiles = atoi( buf + 2 );
					build.pFileNames = NewArray( TEXTCHAR, atoi( strchr( buf, ',' ) + 1 ) );
					build.pFileList = NewArray( TEXTCHAR*, build.nFiles );
					break;
				case '#':
					{
						_32 nStyles;
						_32 nSizeFiles;
						_32 nAltFiles;
						_32 nSizes;
#ifdef __cplusplus_cli
#define SCANBUF mybuf
						char *mybuf = CStrDup( buf + 2 );
#else
#define SCANBUF buf+2
#endif
						if( sscanf( buf + 2, WIDE("%d,%d,%d,%d"), &nStyles, &nSizeFiles, &nSizes, &nAltFiles ) == 4 )
						{
							build.nStyle = 0;
							build.pStyleSlab = NewArray( FONT_STYLE, nStyles );
							MemSet( build.pStyleSlab, 0, sizeof( FONT_STYLE ) * nStyles );
							build.nSizeFile = 0;
							build.pSizeFileSlab = NewArray( APP_SIZE_FILE, nSizeFiles );
							MemSet( build.pSizeFileSlab, 0, sizeof( APP_SIZE_FILE ) * nSizeFiles );
							build.nSize = 0;
							build.pSizeSlab = NewArray( SIZES, nSizes );
							MemSet( build.pSizeSlab, 0, sizeof( SIZES ) * nSizes );
							build.nAlt = 0;
							build.pAltSlab = NewArray( ALT_SIZE_FILE, nAltFiles );
							MemSet( build.pAltSlab, 0, sizeof( ALT_SIZE_FILE ) * nAltFiles );
						}
						else
						{
							Log( WIDE("Error loading slab sizes!") );
						}
#ifdef __cplusplus_cli
						Deallocate( POINTER, mybuf );
#endif
					}
					break;
				}
				break;
			case '@':
				{
					build.pPathList[PathID] = build.pPathNames + PathOfs;
					PathOfs += (len=strlen( buf + 1 ) + 1);
					StrCpyEx( build.pPathList[PathID++], buf + 1, len );
				}
				break;
			case '$':
				{
					build.pFamilyList[FamilyID] = build.pFamilyNames + FamilyOfs;
					FamilyOfs += (len=strlen( buf + 1 ) + 1);
					StrCpyEx( build.pFamilyList[FamilyID++], buf + 1, len );
				}
				break;
			case '*':
				{
					build.pStyleList[StyleID] = build.pStyleNames + StyleOfs;
					StyleOfs += (len=strlen( buf + 1 ) + 1);
					StrCpyEx( build.pStyleList[StyleID++], buf + 1, len );
				}
				break;
			case '&':
				{
					build.pFileList[FileID] = build.pFileNames + FileOfs;
					FileOfs += (len=strlen( buf + 1 ) + 1);
					StrCpyEx( build.pFileList[FileID++], buf + 1, len );
				}
				break;
			default:
				{
					TEXTCHAR *family = buf;
					// there WILL be at least one of these....
					count = (TEXTCHAR*)StrChr( buf, ',' );
					next = (TEXTCHAR*)StrChr( buf, '!' );
					if( next )
						*(next++) = 0;
					if( count )
						*(count++) = 0;
					pfe = fg.pFontCache + nFont++;
					{
						_32 nStyles = atoi( count );
						pfe->flags.unusable = 0;
						pfe->nStyles = 0; // use this to count...
						pfe->styles = (PLIST)(build.pStyleSlab + build.nStyle);
						build.nStyle += nStyles;
						//MemSet( pfe->styles, 0, nStyles * sizeof( FONT_STYLE ) );
					}
					pfe->name = (PDICT_ENTRY)build.pFamilyList[IntCreateFromText( family )];
           			// add all the files/sizes associated on this line...
				}
				if( 0 )
				{
			case '!':
					next = buf + 1;
				}
				while( ( style = next ) && next[0] )
				{
					// new font style here...
					flags = strchr( style, '*' );
					count = strchr( flags, ',' );
					next = strchr( flags, '@' );
					if( next )
						*(next++) = 0;
					*(count++) = 0;
					*(flags++) = 0;
					pfs = ((PFONT_STYLE)pfe->styles) + pfe->nStyles++;
					while( flags[0] )
					{
						switch( flags[0] )
						{
						case 'm':
							pfs->flags.mono = 1;
							break;
						case 'i':
							pfs->flags.italic = 1;
							break;
						case 'b':
							pfs->flags.bold = 1;
							break;
						}
						flags++;
					}
					pfs->name = (PDICT_ENTRY)build.pStyleList[IntCreateFromText( style )];

					pfs->nFiles = 0;
					pfs->appfiles = build.pSizeFileSlab + build.nSizeFile;
					// demote from S_64 to _32
					build.nSizeFile += (_32)IntCreateFromText( count );
					if(0)
					{
			case '\\':
				      next = buf + 2;
					}
					{
						// continue size-fonts... (on style)
						TEXTCHAR *width, *height;
						S_16 nWidth, nHeight;
						TEXTCHAR *PathID;
						TEXTCHAR *file;
						while( ( count = next ) && next[0] )
						{
							if( count[0] )
							{
								TEXTCHAR *count2 = strchr( count, ',' );
								if( count2 )
								{
									*(count2++) = 0;
									PathID = strchr( count2, ',' );
								}
								else
								{
									PathID = NULL;
								}
								file = strchr( PathID, ':' );
								next = strchr( file, '^' );
								*(file++) = 0;
								*(PathID++) = 0;
								// terminates next file (even if empty thing)
								// so now between 'file' and \0 are file sizes.
								if( next )
									*(next++) = 0;

								psfCurrent = pfs->appfiles + pfs->nFiles++;
	
								psfCurrent->path =build.pPathList[IntCreateFromText( PathID )];
								psfCurrent->file =build.pFileList[IntCreateFromText(file)];
								psfCurrent->nSizes = 0;
								psfCurrent->sizes = build.pSizeSlab + build.nSize;
								build.nSize += (_32)IntCreateFromText( count2 );
									//Allocate( sizeof( SIZES ) * IntCreateFromText( count2 ) );
								psfCurrent->nAlternate = 0;
								psfCurrent->pAlternate = build.pAltSlab + build.nAlt;
								build.nAlt += (_32)IntCreateFromText( count );
							}
							//Allocate( sizeof( ALT_SIZE_FILE ) * IntCreateFromText(count) );

							height = file;
							if(0)
							{
			case '#':
								height = buf;
								// on a new line - set next correctly.
								next = strchr( height, '^' );
								if( next )
									*(next++) = 0; // setup next...
							}
							while( ( width = strchr( height, '#' ) ) )
							{
								PSIZES newsize = ((PSIZES)psfCurrent->sizes) + psfCurrent->nSizes++;
								width++;
								// safe to play with these as numbers
								// even without proper null terimination...
								nWidth = atoi( width );
								if( nWidth >= 0 )
								{
									height = strchr( width, ',' );
									if( !height )
									{
										Log( WIDE("Fatality - Cache loses!") );
									}
									height++;
									nHeight = atoi( height );
								}
								else
								{
									nHeight = -1;
									height = width + 1;
								}
								//Log2( WIDE("Add size to file: %d,%d"), nWidth, nHeight );
								newsize->width = nWidth;
								newsize->height = nHeight;
							}
							if(0)
							{
			case '|':
								next = buf + 2; // |^: worst case...
							}
							while( ( PathID = next ) && PathID[0] != ':' )
							{
								PALT_SIZE_FILE pasf = (PALT_SIZE_FILE)psfCurrent->pAlternate
									+ psfCurrent->nAlternate++;
								file = strchr( PathID, ':' );
								next = strchr( file, '^' );
								//*(PathID++) = 0;
								*(file++) = 0;
								if( next )
									*(next++) = 0;
								pasf->path =(PDICT_ENTRY)build.pPathList[IntCreateFromText(PathID)];
								pasf->file =(PDICT_ENTRY)build.pFileList[IntCreateFromText(file)];
							}
							if( PathID )
							{
								// +1 == ':' +2 == ... char type...
								// could be a ! (new style)
								// could be a @ (new file/sizes)
								if( PathID[1] )
								{
									next = PathID + 2;
									if( PathID[1] == '!' )
										break; // otherwise will be a @ which
									// the current loop is another file...
								}
								else
									next = NULL;
							}
						}
					}
				}
			}
#ifdef __cplusplus_cli
			Deallocate( POINTER, buf );
#endif

		}
		sack_fclose( in );
#ifdef _DEBUG
		//DumpLoadedFontCache();
#endif
		/*
       // need to keep this so we might re-generate the cache with further validations
		if( build.pPathList )
		{
			Deallocate( POINTER, build.pPathList );
			build.pPathList = NULL;
		}
		if( build.pFamilyList )
		{
			Deallocate( POINTER, build.pFamilyList );
			build.pFamilyList = NULL;
		}
		if( build.pStyleList )
		{
			Deallocate( POINTER, build.pStyleList );
			build.pStyleList = NULL;
		}
		if( build.pFileList )
		{
			Deallocate( POINTER, build.pFileList );
			build.pFileList = NULL;
		}
      */
		Defragment( (POINTER*)&fg.pFontCache );
		//DebugDumpMemFile( WIDE("cache.loaded") );
	}
}

void UnloadAllFonts( void )
{
   // Release all cached data...
}
#endif

IMAGE_NAMESPACE_END


// $Log: fntcache.c,v $
// Revision 1.14  2005/03/23 02:43:07  panther
// Okay probably a couple more badly initialized 'unusable' flags.. but font rendering/picking seems to work again.
//
// Revision 1.13  2004/12/15 03:00:19  panther
// Begin coding to only show valid, renderable fonts in dialog, and update cache, turns out that we'll have to postprocess the cache to remove unused dictionary entries
//
// Revision 1.12  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.3  2004/10/13 11:13:53  d3x0r
// Looks like this is cleaning up very nicely... couple more rough edges and it'll be good to go.
//
// Revision 1.2  2004/10/07 04:37:16  d3x0r
// Okay palette and listbox seem to nearly work... controls draw, now about that mouse... looks like my prior way of cheating is harder to step away from than I thought.
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.11  2004/04/26 09:47:26  d3x0r
// Cleanup some C++ problems, and standard C issues even...
//
// Revision 1.10  2003/11/29 00:10:28  panther
// Minor fixes for typecast equation
//
// Revision 1.9  2003/11/28 18:43:56  panther
// Fix LIST_ENDFORALL
//
// Revision 1.8  2003/09/26 16:44:48  panther
// Extend font test program
//
// Revision 1.7  2003/09/25 09:47:15  panther
// Clean up for LCC compilation
//
// Revision 1.6  2003/08/27 07:58:39  panther
// Lots of fixes from testing null pointers in listbox, font generation exception protection
//
// Revision 1.5  2003/08/21 13:34:42  panther
// include font render project with windows since there's now freetype
//
// Revision 1.4  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
