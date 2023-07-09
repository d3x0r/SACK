/// This font cache keeps fonts in char format.

//#define DEBUG_OPENFONTFILE
#define IMAGE_LIBRARY_SOURCE

#ifdef _PSI_INCLUSION_
//#line 2 "../imglib/psi_fntcache.c"
#endif

//#define __CAN_USE_CACHE_DIALOG__
#ifdef SACK_MONOLITHIC_BUILD
#define __CAN_USE_TOPMOST__
#endif
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION

#include <stdhdrs.h>
#include <ctype.h> // isdigit ... hasn't been a problem elsewhere so just add this yere.

#include <timers.h>
#include <sqlgetoption.h>
#include <sharemem.h>

#include <ft2build.h>
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


//-------------------------------------------------------------------------

#define fg (*global_font_data)
#ifdef _PSI_INCLUSION
PRIORITY_PRELOAD( CreatePSIFontCacheGlobal, IMAGE_PRELOAD_PRIORITY )
#else
PRIORITY_PRELOAD( CreateFontCacheGlobal, IMAGE_PRELOAD_PRIORITY + 1 )
#endif
{
	SimpleRegisterAndCreateGlobal( global_font_data );
	if( !fg._build )
	{
		fg._build = New( struct cache_build_tag );
		memset( fg._build, 0, sizeof( struct cache_build_tag ) );
#ifndef __NO_OPTIONS__
		SACK_GetProfileString( GetProgramName(), "SACK/Image Library/Font Cache Path"
#else
	   strcpy( fg.font_cache_path
#endif
#ifdef WIN32
									, "*/../../Fonts.Cache"
#else
									, "Fonts.Cache"
#endif
#ifndef __NO_OPTIONS__
                           , fg.font_cache_path, 256 );
#else
									 );

#endif
	}
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
#ifndef _PSI_INCLUSION_
 struct font_global_tag *  IMGVER(GetGlobalFonts)( void )
{
	return &fg;
}
#endif

//-------------------------------------------------------------------------

#if 0
void LogSHA1Ex( TEXTCHAR *leader, PSIZE_FILE file DBG_PASS)
#define LogSHA1(l,f) LogSHA1Ex(l,f DBG_SRC )
{
	TEXTCHAR msg[256];
	int ofs;
	int n;
	ofs = tnprintf( msg, sizeof( msg ), "%s: ", leader );
	//for( n = 0; n < SHA1HashSize; n++ )
	//	ofs += tnprintf( msg + ofs, sizeof(msg)-ofs*sizeof(TEXTCHAR), "%02X ", ((uint8_t*)file->SHA1)[n] );
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
		tnprintf( filename, sizeof( filename ), "%s/%s", file->path->word, file->file->word );
		memmap = OpenSpace( NULL, filename, (uintptr_t*)&size );
		SHA1Reset( &Sha1Context );
		SHA1Input( &Sha1Context, (uint8_t*)memmap, size );
		SHA1Result( &Sha1Context, file->SHA1 );
		CloseSpace( memmap );
	}
	else
		DebugBreak();
}
#endif
//-------------------------------------------------------------------------

static void CPROC DestroyFontEntry( PCACHE_FONT_ENTRY pfe, TEXTCHAR *key )
{
	INDEX idx, idx2;
	PCACHE_FONT_STYLE pfs;
	PCACHE_SIZE_FILE psf;
	LIST_FORALL( pfe->styles, idx, PCACHE_FONT_STYLE, pfs )
	{
		LIST_FORALL( pfs->files, idx2, PCACHE_SIZE_FILE, psf )
		{
			PCACHE_SIZES size;
			PCACHE_ALT_SIZE_FILE pasf;
			INDEX idx;
			LIST_FORALL( psf->sizes, idx, PCACHE_SIZES, size )
			{
				Deallocate( PCACHE_SIZES, size );
			}
			DeleteListEx( &psf->sizes DBG_SRC );
			LIST_FORALL( psf->pAlternate, idx, PCACHE_ALT_SIZE_FILE, pasf )
			{
				Deallocate( PCACHE_ALT_SIZE_FILE, pasf );
			}
			DeleteListEx( &psf->pAlternate DBG_SRC );
			Deallocate( PCACHE_SIZE_FILE, psf );
		}
		DeleteListEx( &pfs->files DBG_SRC );
		Deallocate( PCACHE_FONT_STYLE, pfs );
	}
	DeleteListEx( &pfe->styles DBG_SRC );
	Deallocate( PCACHE_FONT_ENTRY, pfe );
}

//-------------------------------------------------------------------------

static int UniqueStrCmp( TEXTCHAR *s1, INDEX s1_length, TEXTCHAR *s2 )
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
			num = (int)IntCreateFromText( numstart+1 );
			StrCpyEx( name, s1, 256 );
			tnprintf( s1, sizeof( name ), "%s[%d]", name, num+1 );
		}
		else
		{
			tnprintf( s1, s1_length, "%s[1]", s2 ); // s1 and s2 are equal, so this works...
		}
		return 1;
	}
	return dir;
}

//-------------------------------------------------------------------------

static void CPROC DestroyDictEntry( CPOINTER psvEntry, uintptr_t key )
{
   Deallocate( POINTER, (POINTER)psvEntry );
}

//-------------------------------------------------------------------------

static int CPROC MyStrCmp( uintptr_t s1, uintptr_t s2 )
{
   return strcmp( (TEXTCHAR*)s1, (TEXTCHAR*)s2 );
}

//-------------------------------------------------------------------------

static PCACHE_DICT_ENTRY AddDictEntry( PTREEROOT *root, CTEXTSTR name )
{
	PCACHE_DICT_ENTRY pde;
	size_t len;
	if( !*root )
		*root = CreateBinaryTreeExx( BT_OPT_NODUPLICATES
											, MyStrCmp
											, DestroyDictEntry );

	len = StrLen( name );
	pde = NewPlus( CACHE_DICT_ENTRY, len*sizeof(pde->word[0]));
	StrCpyEx( pde->word, name, len + 1 );
	if( !AddBinaryNode( *root, pde, (uintptr_t)pde->word ) )
	{
		Deallocate( PCACHE_DICT_ENTRY, pde );
		pde = (PCACHE_DICT_ENTRY)FindInBinaryTree( *root, (uintptr_t)name );
	}
	return pde;
}


//-------------------------------------------------------------------------

// if ID != 0 use than rather than custom assignment.
static PCACHE_DICT_ENTRY AddPath( CTEXTSTR filepath, PCACHE_DICT_ENTRY *file )
{
	TEXTSTR tmp = StrDup( filepath );
	TEXTSTR p;
	PCACHE_DICT_ENTRY ppe;
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
	ppe = AddDictEntry( &fg.build.pPaths, tmp );
	*file = AddDictEntry( &fg.build.pFiles, filename );
	p[0] = save; // restore character.
	Deallocate( TEXTSTR, tmp );
	return ppe;
}

//-------------------------------------------------------------------------


static void SetIDs( PTREEROOT root )
{
	PCACHE_DICT_ENTRY pde;
	uint32_t ID = 0;
	for( pde = (PCACHE_DICT_ENTRY)GetLeastNode( root );
		 pde;
		  pde = (PCACHE_DICT_ENTRY)GetGreaterNode( root ) )
	{
		pde->ID = ID++;
	}
}

//-------------------------------------------------------------------------


static PCACHE_FONT_ENTRY AddFontEntry( PCACHE_DICT_ENTRY name )
{
	PCACHE_FONT_ENTRY pfe;
	if( !fg.build.pFontCache )
	{
		fg.build.pFontCache = CreateBinaryTreeEx( MyStrCmp
														 , (void(CPROC *)(CPOINTER,uintptr_t))DestroyFontEntry );
	}

	pfe = (PCACHE_FONT_ENTRY)FindInBinaryTree( fg.build.pFontCache, (uintptr_t)name->word );
	if( !pfe )
	{
		pfe = New( CACHE_FONT_ENTRY );
		pfe->flags.unusable = 0;
		pfe->name = name;
		pfe->nStyles = 0;
		pfe->styles = NULL;
		AddBinaryNode( fg.build.pFontCache, pfe, (uintptr_t)name->word );
	}
	return pfe;
}

//-------------------------------------------------------------------------

static void AddAlternateSizeFile( PCACHE_SIZE_FILE psfBase, PCACHE_DICT_ENTRY path, PCACHE_DICT_ENTRY file )
{
	PCACHE_ALT_SIZE_FILE psf = New( CACHE_ALT_SIZE_FILE );
	psf->path = path;
	psf->file = file;
	psfBase->nAlternate++;
	AddLink( &psfBase->pAlternate, psf );
}

//-------------------------------------------------------------------------

static void AddSizeToFile( PCACHE_SIZE_FILE psf, int16_t width, int16_t height )
{
	if( psf )
	{
		PCACHE_SIZES size = New( CACHE_SIZES );
		size->flags.unusable = 0;
		size->width = width;
		size->height = height;
		psf->nSizes++;
		AddLink( &psf->sizes, size );
	}
}

//-------------------------------------------------------------------------

static PCACHE_SIZE_FILE AddSizeFileEx( PCACHE_FONT_STYLE pfs
								, PCACHE_DICT_ENTRY path
								, PCACHE_DICT_ENTRY file
								, LOGICAL bTest
								DBG_PASS )
#define AddSizeFile(pfs,p,f,t) AddSizeFileEx(pfs,p,f,t DBG_SRC )
{
	PCACHE_SIZE_FILE psf = New( CACHE_SIZE_FILE );
	PCACHE_SIZE_FILE psfCheck = NULL;
	psf->flags.unusable = 0;
	psf->path = path;
	psf->nSizes = 0;
	psf->sizes = NULL;
	psf->nAlternate = 0;
	psf->pAlternate = NULL;
	psf->file = file;
#if 0
	if( bTest )
		DoSHA1( psf );
#endif
	// Log( "Commpare sha1!" );
	if( bTest )
	{
#if 0
		LIST_FORALL( pfs->files, idx, PCACHE_SIZE_FILE, psfCheck )
		{
			//LogSHA1( "file: ", psf );
			//LogSHA1( "vs  : ", psfCheck );
			if( MemCmp( psf->SHA1
						 , psfCheck->SHA1
						 , SHA1HashSize ) == 0 )
			{
				//Log3( DBG_FILELINEFMT "File 1: (%d)%s/%s" DBG_RELAY
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->path->ID
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->path->word
				//	 , ((PSIZE_FILE)GetLink( &pfs->files, 0 ))->file );
				//Log3( DBG_FILELINEFMT "File 2: (%d)%s/%s" DBG_RELAY , path->ID, path->word, file );
				//Log( "Duplicate found - storing as alterate." );
				Deallocate( PCACHE_SIZE_FILE, psf );
				AddAlternateSizeFile( psfCheck, path, file );
				return NULL;
			}
		}
#endif
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
void IMGVER(DumpFontCache)( void )
{
	IMGVER(LoadAllFonts)();
	{
		INDEX idx;
		for( idx = 0; idx < fg.nFonts; idx++ )
		{
			PFONT_ENTRY pfe;
			uint32_t s;
			PFONT_STYLE pfs;
			pfe = fg.pFontCache + idx;
			if( pfe->flags.unusable )
				continue;
			for( s = 0; s < pfe->nStyles; s++ )
			{
				PCACHE_SHORT_SIZE_FILE file;
				uint32_t f;
				pfs = pfe->styles + s;// ((PCACHE_FONT_STYLE)pfe->styles) + s;
				for( f = 0; f < pfs->nFiles; f++ )
				{
					uint32_t sz;
					file = ((PCACHE_SHORT_SIZE_FILE)pfs->files) + f;
					for( sz = 0; sz < file->nSizes; sz++ )
					{
						PCACHE_SIZES size = ((PCACHE_SIZES)file->sizes) + sz;
						lprintf( "%s[%s] = %s/%s (%dx%d)", (TEXTCHAR*)pfe->name
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

int IMGVER(OpenFontFile)( CTEXTSTR name, POINTER *font_memory, size_t *font_memory_size, FT_Face *face, int face_idx, LOGICAL fallback_to_cache )
{
	int error;
	POINTER _font_memory = NULL;
	TEXTSTR temp_filename = NULL;
	TEXTSTR font_style;
	LOGICAL style_set = FALSE;
	size_t temp_filename_len = 0;
	uintptr_t size = 0;
	size_t _font_memory_size;
	LOGICAL logged_error;
	TEXTSTR name_ = ExpandPath( name );
	lprintf( "Path converted from %s to %s", name, name_ );
	if( !font_memory )
		font_memory = &_font_memory;
	if( !font_memory_size )
		font_memory_size = &_font_memory_size;

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
		{
			size_t len = strlen( name );
			TEXTCHAR *tmp = NewArray( TEXTCHAR, len );
			snprintf( tmp, len, "%.*s", font_style-name, name_ );
			name_ = ExpandPath( tmp );
			ReleaseEx( tmp DBG_SRC );
		}
		if( !end )
			font_style = (TEXTCHAR*)"regular";
	}
	else
		font_style = (TEXTCHAR*)"regular";

	if( !(*font_memory) )
	{
		(*font_memory) =
#ifdef UNDER_CE
			NULL;
#else
			OpenSpace( NULL, name_, &size );
#endif
#ifdef DEBUG_OPENFONTFILE
		lprintf( "open space result is %p %d", (*font_memory), size );
#endif
		if( !(*font_memory) )
		{
			FILE *file;
#ifdef DEBUG_OPENFONTFILE
			lprintf( "open by memory map failed for %s", name_ );
#endif
			file = sack_fopen( 0, name_, "rb" );
			if( file )				
			{
#ifdef DEBUG_OPENFONTFILE
				lprintf( "did manage to open the file. %p", file );
#endif
				size = sack_fsize( file );
				(*font_memory) = NewArray( uint8_t, size );
				(*font_memory_size) = size;
				sack_fread( (*font_memory), size, 1, file );
				sack_fclose( file );
			}
		}
		else
			(*font_memory_size) = size;
	}
	else
		(*font_memory_size) = size;

#ifdef DEBUG_OPENFONTFILE
	lprintf( "fallback is %d", fallback_to_cache );
#endif
#ifndef _PSI_INCLUSION_
	if( !(*font_memory) && fallback_to_cache )
	{
		CTEXTSTR base_name = pathrchr( name_ );
		INDEX idx;
		if( base_name )
			base_name++;
		else
			base_name = name_;
		//lprintf( "Direct open failed... try seaching..." );
		// only open this if there's not a direct file ready.
		IMGVER(LoadAllFonts)();
		for( idx = 0; idx < fg.nFonts; idx++ )
		{
			PFONT_ENTRY pfe;
			PFONT_STYLE pfs;
			uint32_t s;
			pfe = fg.pFontCache + idx;
#ifdef DEBUG_OPENFONTFILE
			lprintf( "check font %d", idx );
#endif
			if( pfe->flags.unusable )
			{
#ifdef DEBUG_OPENFONTFILE
				lprintf( "was marked unusable." );
#endif
				continue;
			}
			for( s = 0; s < pfe->nStyles; s++ )
			{
				PSIZE_FILE file;
				uint32_t f;
				pfs = pfe->styles + s;
				for( f = 0; f < pfs->nFiles; f++ )
				{
					file = (pfs->files) + f;
#ifdef DEBUG_OPENFONTFILE
					lprintf( "file is %s %s vs %s", pfs->name, file->file, base_name );
#endif
					if( StrCaseCmp( (TEXTCHAR*)file->file, base_name ) == 0 )
					{
						//lprintf( "Found font file matchint %s", file->file );
						temp_filename_len = (StrLen( (TEXTCHAR*)file->path ) + StrLen( (TEXTCHAR*)file->file ) + 2);
						temp_filename = NewArray( TEXTCHAR, temp_filename_len );
						tnprintf( temp_filename, temp_filename_len, "%s/%s", (TEXTCHAR*)file->path, (TEXTCHAR*)file->file );
#ifdef DEBUG_OPENFONTFILE
						lprintf( "full path is %s", temp_filename );
#endif
						break;
					}
					if( StrCaseCmp( base_name, (TEXTCHAR*)pfe->name ) == 0 )
					{
						if( StrCaseCmp( (TEXTCHAR*)pfs->name, font_style ) )
						{
							temp_filename_len = (StrLen( (TEXTCHAR*)file->path ) + StrLen( (TEXTCHAR*)file->file ) + 2);
							temp_filename = NewArray( TEXTCHAR, temp_filename_len );
							tnprintf( temp_filename, temp_filename_len, "%s/%s", (TEXTCHAR*)file->path, (TEXTCHAR*)file->file );
#ifdef DEBUG_OPENFONTFILE
							lprintf( "full path is %s", temp_filename );
#endif
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
			if( !(*font_memory) )
			{
				FILE *file;
#ifdef DEBUG_OPENFONTFILE
				lprintf( "open by memory map failed for %s", temp_filename );
#endif
				file = sack_fopen( 0, temp_filename, "rb" );
				if( file )				
				{
#ifdef DEBUG_OPENFONTFILE
					lprintf( "did manage to open the file. %p", file );
#endif
					size = sack_fsize( file );
					(*font_memory) = NewArray( uint8_t, size );
					sack_fread( (*font_memory), size, 1, file );
					sack_fclose( file );
				}
			}

		}
	}
#else
	lprintf( "!!!!!!!! PSI SHOULD NOT BE USING OPENFONTFILE !!!!!!!!!!!!" );
#endif
	if( *font_memory && size < 0xd000000 )
	{
		//POINTER p = NewArray( uint8_t, size );
		//MemCpy( p, (*font_memory), size );
		//Deallocate( POINTER, (*font_memory) );
		//(*font_memory) = p;
#ifdef DEBUG_OPENFONTFILE
		lprintf( "re-copied the font memory." );
#endif
		//lprintf( "Using memory mapped space..." );
		error = FT_New_Memory_Face( fg.library
										  , (FT_Byte*)(*font_memory)
										  , (FT_Long)size
										  , face_idx
										  , face );
	}
	else
	{
		char *file = DupTextToChar( name_ );
		//lprintf( "Using file access font... for %s", name );
#ifdef DEBUG_OPENFONTFILE
		lprintf( "using FT_New_Face direct file access... (should never happen now)" );
#endif
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
#ifdef DEBUG_OPENFONTFILE
			lprintf( "Failed to open font %s or %s Result %d"
					 , name?name:"<nofile>"
					 , temp_filename?temp_filename:"<nofile>"
					 , error );
#endif
		}
		Deallocate( char *, file );
	}
	if( style_set )
		Deallocate( TEXTCHAR*, font_style );
	if( temp_filename )
		Deallocate( TEXTCHAR*, temp_filename );
	Deallocate( TEXTSTR, name_ );
	return error;
}

static PCACHE_FONT_STYLE AddFontStyle( PCACHE_FONT_ENTRY pfe, PCACHE_DICT_ENTRY name )
{
	PCACHE_FONT_STYLE pfsCheck = NULL;
	INDEX idx;
	LIST_FORALL( pfe->styles, idx, PCACHE_FONT_STYLE, pfsCheck )
	{
		if( pfsCheck->name == name )
		{
			break;
		}
	}
	if( !pfsCheck )
	{
		PCACHE_FONT_STYLE pfs = New( CACHE_FONT_STYLE );
		memset( pfs, 0, sizeof( CACHE_FONT_STYLE ) );
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
static uint32_t fonts_checked;

static void CPROC ListFontFile( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags )
{
	FT_Face face;
	int face_idx;
	int num_faces;
	int error;
	PCACHE_DICT_ENTRY ppe;
	PCACHE_FONT_ENTRY pfe;
	PCACHE_FONT_STYLE pfs[4];
	PCACHE_DICT_ENTRY pFamilyEntry, pStyle, pFileName;
	fonts_checked++;
	//if( !InitFont() )
	//	return;
#ifdef DEBUG_OPENFONTFILE
	lprintf( "Try font: %s", name );
#endif
	//#ifdef IMAGE_LIBRARY_SOURCE
	face_idx = 0;
	do
	{
		POINTER font_memory = NULL;
		error = OpenFontFile( name, &font_memory, NULL, &face, face_idx, FALSE );
		Deallocate( POINTER, font_memory );
		if( error == FT_Err_Unknown_File_Format )
		{
			//Log1( "Unsupported font: %s", name );
			return;
		}
		else if( error )
		{
			Log2( "Error loading font: %s(%d)", name, error );
			return;
		}

		num_faces = face->num_faces;
		ppe = AddPath( name, &pFileName );

		{
			TEXTSTR name = DupCharToText( face->family_name?face->family_name:"no-name");
			pFamilyEntry = AddDictEntry( &fg.build.pFamilies
												, name
												);
			Deallocate( TEXTSTR, name );
		}
		{
			TEXTSTR name = DupCharToText( face->style_name );
			pStyle = AddDictEntry( &fg.build.pStyles
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
			PCACHE_DICT_ENTRY pStyle;
			TEXTSTR style_name = DupCharToText( face->style_name?face->style_name:"no-style-name");
			TEXTCHAR buffer[256];
			
			tnprintf( buffer, sizeof( buffer ), "%s+Italic", style_name );
			pStyle = AddDictEntry( &fg.build.pStyles, buffer );
			pfs[1] = AddFontStyle( pfe, pStyle );
			pfs[1]->flags.mono = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );
			pfs[1]->flags.italic = 1;

			tnprintf( buffer, sizeof( buffer ), "%s+Bold", style_name );
			pStyle = AddDictEntry( &fg.build.pStyles, buffer );
			pfs[2] = AddFontStyle( pfe, pStyle );
			pfs[2]->flags.mono = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );
			pfs[2]->flags.bold = 1;

			tnprintf( buffer, sizeof( buffer ), "%s+Bold-Italic", style_name );
			pStyle = AddDictEntry( &fg.build.pStyles, buffer );
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
				PCACHE_SIZE_FILE psf = AddSizeFile( pfs[style], ppe, pFileName, FALSE );
				// if( !psf ) we already had the file...
				// no reason to add these sizes...
				if( psf )
				{
					if( face->face_flags & FT_FACE_FLAG_SCALABLE )
						AddSizeToFile( psf, -1, -1 );
					if( face->num_fixed_sizes )
					{
						//Log( "Adding fixed sizes" );
						for( n = 0; n < face->num_fixed_sizes; n++ )
						{
							//Log2( "Added size %d,%d"
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
						Log( "Error adding font - it's fixed with no fixed defined" );
						//AddSizeFile( pfe, -2, -2, ppe, filename, TRUE );
					}
				}
				else
				{
					//Log1( "Alternate font of %s", pfe->name->word );
				}
			}
		}
		else
		{
			int style;
			for( style = 0; style < 4; style++ )
			{
				PCACHE_SIZE_FILE psf = AddSizeFile( pfs[style], ppe, pFileName, FALSE );
				if( psf )
					AddSizeToFile( psf, -1, -1 );
				else
				{
					//Log1( "(Scaalable only)Alternate font of %s", pfe->name->word );
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

static void OutputFontCache( void )
{
	FILE *out;
	PCACHE_FONT_ENTRY pfe;
	PCACHE_DICT_ENTRY pde;
	size_t size;
	out = sack_fopen( 0, fg.font_cache_path, "wt" );
	if( !out )
		return; // no point.
//	for( pfe = (PFONT_ENTRY)GetLeastNode( fg.build.pFontCache );
//		  pfe;
//		  pfe = (PFONT_ENTRY)GetGreaterNode( fg.build.pFontCache ) )
//	{
//		if( pfe->flags.unusable )
 //        RemoveBinaryNode( fg.build.pFontCache, pfe, pfe->name->word );
 //  }


	if( fg.build.pPaths )
	{
		for( size = 0, pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pPaths );
	        pde;
			  (size += StrLen( pde->word ) + 1), pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pPaths ) ){}
		sack_fprintf( out, "%%@%" c_32f ",%" c_size_f "\n", GetNodeCount( fg.build.pPaths ), size );
	}
	if( fg.build.pFamilies )
	{
		for( size = 0, pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pFamilies );
   	     pde;
			  (size += StrLen( pde->word ) + 1),pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pFamilies ) ){}
		sack_fprintf( out, "%%$%" c_32f ",%" c_size_f "\n", GetNodeCount( fg.build.pFamilies ), size );
	}
	if( fg.build.pStyles )
	{
		for( size = 0,pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pStyles );
   	     pde;
			  (size += StrLen( pde->word ) + 1),pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pStyles ) ){}
	   sack_fprintf( out, "%%*%" c_32f ",%" c_size_f "\n", GetNodeCount( fg.build.pStyles ), size );
	}
	if( fg.build.pFiles )
	{
		for( size = 0,pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pFiles );
   	     pde;
			  (size += StrLen( pde->word ) + 1),pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pFiles ) )
		{}
	   sack_fprintf( out, "%%&%" c_32f ",%" c_size_f "\n", GetNodeCount( fg.build.pFiles ), size );
	}

	// slightly complex loop to scan cache as it is, and
   // write out total styles, alt files, and sizes...
	{
		uint32_t nStyles = 0;
		uint32_t nSizeFiles = 0;
		uint32_t nAltFiles = 0;
		uint32_t nSizes = 0;
		for( pfe = (PCACHE_FONT_ENTRY)GetLeastNode( fg.build.pFontCache );
			  pfe;
			  pfe = (PCACHE_FONT_ENTRY)GetGreaterNode( fg.build.pFontCache ) )
		{
			INDEX idx;
			PCACHE_FONT_STYLE pfs;
			if( pfe->flags.unusable )
				continue;
			LIST_FORALL( pfe->styles, idx, PCACHE_FONT_STYLE, pfs )
			{
				INDEX idx;
				PCACHE_SIZE_FILE psf;
				nStyles++;
				LIST_FORALL(pfs->files, idx, PCACHE_SIZE_FILE, psf )
				{
					if( psf->flags.unusable )
						continue;
					nSizeFiles++;
					{
						INDEX idx;
						PCACHE_SIZES size;
						LIST_FORALL( psf->sizes, idx, PCACHE_SIZES, size )
						{
							if( size->flags.unusable )
								continue;
							nSizes++;
						}
					}
					{
						INDEX idx;
						PCACHE_ALT_SIZE_FILE pasf;
						LIST_FORALL( psf->pAlternate, idx, PCACHE_ALT_SIZE_FILE, pasf )
						{
							nAltFiles++;
						}
					}
				}
			}
		}
		sack_fprintf( out, "%%#%" c_32f ",%" c_32f ",%" c_32f ",%" c_32f "\n", nStyles, nSizeFiles, nSizes, nAltFiles );
	}

	for( pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pPaths );
	    pde;
		  pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pPaths ) )
	{
		sack_fprintf( out, "@%s\n", pde->word );
	}

	for( pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pFamilies );
	    pde;
		  pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pFamilies ) )
	{
		sack_fprintf( out, "$%s\n", pde->word );
	}

	for( pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pStyles );
        pde;
		  pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pStyles ) )
	{
		sack_fprintf( out, "*%s\n", pde->word );
	}

	for( pde = (PCACHE_DICT_ENTRY)GetLeastNode( fg.build.pFiles );
        pde;
		  pde = (PCACHE_DICT_ENTRY)GetGreaterNode( fg.build.pFiles ) )
	{
		sack_fprintf( out, "&%s\n", pde->word );
	}



	for( pfe = (PCACHE_FONT_ENTRY)GetLeastNode( fg.build.pFontCache );
		  pfe;
        pfe = (PCACHE_FONT_ENTRY)GetGreaterNode( fg.build.pFontCache ) )
	{
		int linelen;
		int mono = 2;
		INDEX idx;
		PCACHE_SIZE_FILE psf;
		PCACHE_FONT_STYLE pfs;
		char outbuf[80];
		int  newlen;
		// should dump name also...
		linelen = sack_fprintf( out, "%" c_32f ",%" c_32f 
							  , pfe->name->ID
							  , pfe->nStyles
							  );
		LIST_FORALL( pfe->styles, idx, PCACHE_FONT_STYLE, pfs )
		{
			INDEX idx;
			newlen = snprintf( outbuf, sizeof( outbuf ), "!%" c_32f "*%s%s%s,%" c_32f 
								 , pfs->name->ID
								 , pfs->flags.mono?"m":""
								 , pfs->flags.italic?"i":""
								 , pfs->flags.bold?"b":""
								 , pfs->nFiles );
			if( ( newlen + linelen ) >= 80 )
			{
				sack_fprintf( out, "\n" );
				linelen = 0;
			}
			linelen += newlen;
			sack_fputs( outbuf, out );

			if( pfs->flags.mono )
			{
				if( !mono )
					Log2( "Mono-spaced and var spaced together:%s %s"
						, pfe->name->word, pfs->name->word );
				mono = 1;
			}
			else
			{
				if( mono == 1 )
				{
					Log2( "Mono-spaced and var spaced together:%s %s"
						, pfe->name->word, pfs->name->word );
				}
			}

			LIST_FORALL(pfs->files, idx, PCACHE_SIZE_FILE, psf )
			{
				PCACHE_SIZES size;
				INDEX idx;
				newlen = snprintf( outbuf, sizeof( outbuf ), "@%" c_32f ",%" c_32f ",%" c_32f ":%" c_32f
									 , psf->nAlternate
                            , psf->nSizes
									 , psf->path->ID
									 , psf->file->ID
									 );

				if( linelen + newlen >= 80 )
				{
					sack_fprintf( out, "\n\\" );
					linelen = 1;
				}
				linelen += newlen;
				sack_fputs( outbuf, out );

				LIST_FORALL( psf->sizes, idx, PCACHE_SIZES, size )
				{
					if( size->width < 0 )
						newlen = snprintf( outbuf, sizeof( outbuf ), "#%d"
											 , size->width );
					else
						newlen = snprintf( outbuf, sizeof( outbuf ), "#%d,%d"
											 , size->width
											 , size->height );
					if( linelen + newlen >= 80 )
					{
						sack_fprintf( out, "\n" );
						linelen = newlen;
					}
					else
						linelen += newlen;
					sack_fputs( outbuf, out );
				}

				{
					INDEX idx;
					PCACHE_ALT_SIZE_FILE pasf;
					LIST_FORALL( psf->pAlternate, idx, PCACHE_ALT_SIZE_FILE, pasf )
					{
						newlen = snprintf( outbuf, sizeof( outbuf ), "^%" c_32fs ":%" c_32fs
											 , pasf->path->ID
											 , pasf->file->ID );
						if( linelen + newlen >= 80 )
						{
							sack_fprintf( out, "\n|" );
							linelen = newlen + 1;
						}
						else
							linelen += newlen;
						sack_fputs( outbuf, out );
					}
				}
				linelen += sack_fprintf( out, "^:" );
			}
		}
		sack_fprintf( out, "\n" );
		linelen = 0;
	}
	sack_fclose( out );
}

#ifdef _DEBUG

static INDEX IndexOf( TEXTCHAR **list, uint32_t count, POINTER item )
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
/// this was for debugging the font cache load/unload to make sure we could save the same thing again from a loaded one
#  if 0 
void DumpLoadedFontCache( void )
{
	FILE *out;
	PFONT_ENTRY pfe;
	uint32_t fontidx, idx;
	out = sack_fopen( 0, fg.font_cache_path, "wt" );
	if( !out )
		return;
	sack_fprintf( out,"%%@%" c_32f ",%" c_size_f "\n", fg.build.nPaths, SizeOfMemBlock( fg.build.pPathNames ) );
	sack_fprintf( out,"%%$%" c_32f ",%" c_size_f "\n", fg.build.nFamilies, SizeOfMemBlock( fg.build.pFamilyNames ) );
	sack_fprintf( out,"%%*%" c_32f ",%" c_size_f "\n", fg.build.nStyles, SizeOfMemBlock( fg.build.pStyleNames ) );
	sack_fprintf( out,"%%&%" c_32f ",%" c_size_f "\n", fg.build.nFiles, SizeOfMemBlock( fg.build.pFileNames ) );

	{
		INDEX fontidx, styleidx, idx;
		PFONT_ENTRY pfe;
		uint32_t nStyles = 0;
		uint32_t nSizeFiles = 0;
		uint32_t nAltFiles = 0;
		uint32_t nSizes = 0;
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
		sack_fprintf( out, "%%#%" c_32f ",%" c_32f ",%" c_32f ",%" c_32f "\n", nStyles, nSizeFiles, nSizes, nAltFiles );
	}

	for( idx = 0; idx < fg.build.nPaths; idx++ )
      sack_fprintf( out, "@%s\n", fg.build.pPathList[idx] );
	for( idx = 0; idx < fg.build.nFamilies; idx++ )
      sack_fprintf( out, "$%s\n", fg.build.pFamilyList[idx] );
	for( idx = 0; idx < fg.build.nStyles; idx++ )
      sack_fprintf( out, "*%s\n", fg.build.pStyleList[idx] );
	for( idx = 0; idx < fg.build.nFiles; idx++ )
      sack_fprintf( out, "&%s\n", fg.build.pFileList[idx] );


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
		linelen = sack_fprintf( out, "%" _size_f ",%" _32f ""
						, IndexOf( fg.build.pFamilyList, fg.build.nFamilies, pfe->name )
                        , pfe->nStyles
							  );
		for( styleidx = 0; styleidx < pfe->nStyles; styleidx++ )
		{
			pfs = (PFONT_STYLE)pfe->styles + styleidx;
			newlen = tnprintf( outbuf, sizeof( outbuf ), "!%" _size_f "*%s,%" _32f ""
								 , IndexOf( fg.build.pStyleList, fg.build.nStyles, pfs->name )
								 , pfs->flags.mono?"m":""
								 , pfs->nFiles
								 );
			if( ( linelen + newlen ) >= 80 )
			{
				sack_fprintf( out, "\n" );
				linelen = 0;
			}
			linelen += newlen;
			sack_fputs( outbuf, out );
			for( idx = 0; psf = pfs->appfiles + idx, idx < pfs->nFiles; idx++ )
			{
				INDEX idx;
				PSIZES size;
				newlen = tnprintf( outbuf, sizeof( outbuf ), "@%" _32f ",%" _32f ",%" _size_f ":%" _size_f
                            , psf->nAlternate
                            , psf->nSizes
									 , IndexOf( fg.build.pPathList, fg.build.nPaths, psf->path )
									 , IndexOf( fg.build.pFileList, fg.build.nFiles, psf->file )
									 );
				if( linelen + newlen >= 80 )
				{
					sack_fprintf( out, "\n\\" );
					linelen = newlen + 1;
				}
				else
					linelen += newlen;
				sack_fputs( outbuf, out );
				for( idx = 0; size = ((PSIZES)psf->sizes) + idx, idx < psf->nSizes; idx++ )
				{
					if( size->width < 0 )
						newlen = tnprintf( outbuf, sizeof( outbuf ), "#%d"
											 , size->width );
					else
						newlen = tnprintf( outbuf, sizeof( outbuf ), "#%d,%d"
											 , size->width
											 , size->height );
					if( linelen + newlen >= 80 )
					{
						sack_fprintf( out, "\n" );
						linelen = newlen;
					}
					else
						linelen += newlen;
					sack_fputs( outbuf, out );
				}

				{
					PALT_SIZE_FILE pasf;
					INDEX idx;
					for( idx = 0; pasf = ((PALT_SIZE_FILE)psf->pAlternate) + idx, idx < psf->nAlternate; idx++ )
					{
						newlen = tnprintf( outbuf, sizeof( outbuf ), "^%" _size_f ":%" _size_f 
											 , IndexOf( fg.build.pPathList, fg.build.nPaths, pasf->path )
											 , IndexOf( fg.build.pFileList, fg.build.nFiles, pasf->file )
											 );
						if( linelen + newlen >= 80 )
						{
							sack_fprintf( out, "\n|" );
							linelen = newlen + 1;
						}
						else
							linelen += newlen;
						sack_fputs( outbuf, out );
					}
				}
				linelen += sack_fprintf( out, "^:" );
			}
		}
		sack_fprintf( out, "\n" );
		linelen = 0;
	}
	sack_fclose( out );
}
#  endif
#endif  // _DEBUG
//-------------------------------------------------------------------------

#define SafeRelease(n)  if(fg.build.n) { Deallocate( POINTER, fg.build.n ); fg.build.n = NULL; }
static void UnloadFontBuilder( void )
{
	if( fg.build.pFontCache )
	{
		DestroyBinaryTree( fg.build.pFontCache );
		fg.build.pFontCache = NULL;
	}
	if( fg.build.pPaths )
	{
		DestroyBinaryTree( fg.build.pPaths );
		fg.build.pPaths = NULL;
	}
	if( fg.build.pFamilies )
	{
		DestroyBinaryTree( fg.build.pFamilies );
		fg.build.pFamilies = NULL;
	}
	if( fg.build.pStyles )
	{
		DestroyBinaryTree( fg.build.pStyles );
		fg.build.pStyles = NULL;
	}
	if( fg.build.pFiles )
	{
		DestroyBinaryTree( fg.build.pFiles );
		fg.build.pFiles = NULL;
	}
}

//-------------------------------------------------------------------------

#define TXT_STATUS  100
#define TXT_TIME_STATUS 101
#define TXT_COUNT_STATUS 102
static uint32_t StartTime;
static int TimeElapsed;

static void CPROC UpdateStatus( uintptr_t psvFrame )
{
	TEXTCHAR msg[256];
	fg.font_status_timer_thread = MakeThread();
	if( !StartTime )
	{
		StartTime = GetTickCount();
	}
	TimeElapsed = GetTickCount() - StartTime;
	tnprintf( msg, sizeof( msg ), "Elapsed time: %d:%02d"
			 , (TimeElapsed/1000) / 60
			 , (TimeElapsed/1000) % 60
			 );
	SystemLog( msg );
#ifdef __CAN_USE_CACHE_DIALOG__
   // in the positiion of image library, there is no controls to do...
	SetControlText( GetControl( (PCOMMON)psvFrame, TXT_TIME_STATUS ), msg );
#endif
	tnprintf( msg, sizeof( msg ), "Checked Fonts: %d"
			 , fonts_checked
			 );
	SystemLog( msg );
#ifdef __CAN_USE_CACHE_DIALOG__
	SetControlText( GetControl( (PCOMMON)psvFrame, TXT_COUNT_STATUS ), msg );
#endif
}

//-------------------------------------------------------------------------
#if 0
// unrefernced?
void CPROC ScanDrive( uintptr_t user, TEXTCHAR *letter, int flags )
{
	TEXTCHAR base[5];
	void *data = NULL;
	tnprintf( base, sizeof( base ), "%c:", letter[0] );
	if( letter[0] != 'c' && letter[0] != 'C' )
		return;
	while( ScanFiles( base
						 , "*.ttf\t*.ttc\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz"
						 , &data
						 , ListFontFile
						 , SFF_SUBCURSE, 0 ) );

}
#endif

static void BuildFontCache( void )
{
	void *data = NULL;
	uint32_t timer;
#ifdef __CAN_USE_CACHE_DIALOG__
	PCOMMON status;
	status = CreateFrame( "Font Cache Status"
							  , 0, 0
							  , 250, 60
							  , 0, 0 );
	MakeTextControl( status, 5, 5, 240, 19, TXT_STATUS, "Building font cache...", 0 );
	MakeTextControl( status, 5, 25, 240, 19, TXT_TIME_STATUS, "", 0 );
	MakeTextControl( status, 5, 45, 240, 19, TXT_COUNT_STATUS, "", 0 );

#endif
#ifdef DEBUG_OPENFONTFILE
	lprintf( "Building cache..." );
#endif
	StartTime = 0;
#ifdef __CAN_USE_CACHE_DIALOG__
	{
		fg.flags.OpeningFontStatus = 1;
		fg.font_status_open_thread = MakeThread();
		LeaveCriticalSec( &fg.cs );
		DisplayFrame( status );
#  ifdef __CAN_USE_TOPMOST__
		MakeTopmost( GetFrameRenderer( status ) );
#  endif
		EnterCriticalSec( &fg.cs );
		fg.flags.OpeningFontStatus = 0;
	}
#endif
#ifdef __CAN_USE_CACHE_DIALOG__
	timer = AddTimer( 100, UpdateStatus
		, (uintptr_t)status
		);
#else
	timer = AddTimer( 100, UpdateStatus
		, 0
		);
#endif

	// .psf.gz doesn't load directly.... 
	while( ScanFiles( ".", "*.ttf\t*.ttc\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz", &data
						 , ListFontFile, SFF_SUBCURSE, 0 ) ){}
#ifndef __ANDROID__

	while( ScanFiles( "%resources%", "*.ttf\t*.ttc\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz", &data
						 , ListFontFile, SFF_SUBCURSE, 0 ) ){}
#endif

	// scan windows/fonts directory
#ifndef __NO_OPTIONS__
   if( SACK_GetPrivateProfileIntEx( "SACK/Image Library", "Scan Windows Fonts", 1, NULL, TRUE ) )
#endif
	{
#ifdef HAVE_ENVIRONMENT
		CTEXTSTR name
#ifdef WIN32
			= OSALOT_GetEnvironmentVariable( "windir" );
#else
#  ifdef __ANDROID__
		   = "/system/fonts";
#  else
		   = "/usr/share/fonts";
#  endif
#endif
			{
				size_t len;
#ifdef WIN32
				TEXTSTR tmp = NewArray( TEXTCHAR, len = StrLen( name ) + 10 );
				tnprintf( tmp, len * sizeof( TEXTCHAR ), "%s\\fonts", name );
#else
				CTEXTSTR tmp = name;
#endif
				while( ScanFiles( tmp, "*.ttf\t*.ttc\t*.fon\t*.TTF\t*.pcf.gz\t*.pf?\t*.fnt\t*.psf.gz", &data
									 , ListFontFile, SFF_SUBCURSE, 0 ) );
#ifdef WIN32
				Deallocate( TEXTSTR, tmp );
#endif
			}
#endif
	}

	SetIDs( fg.build.pPaths );
	SetIDs( fg.build.pFamilies );
	SetIDs( fg.build.pStyles );
	SetIDs( fg.build.pFiles );
	OutputFontCache();
	// delete all memory associated with building the cache.
	UnloadFontBuilder();
	//DebugDumpMemFile( "cache.built" );
   // it might be dispatched...
	RemoveTimer( timer );
#ifdef __CAN_USE_CACHE_DIALOG__
	DestroyFrame( &status );
#endif
}

//-------------------------------------------------------------------------

static int IsNumber( char *num )
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

void IMGVER(LoadAllFonts)( void )
{
	FILE *in = NULL;
	//if( !InitFont() )
	//return;
	if( fg.pFontCache )
	{
      // if it was loaded, don't re-load it.
		return;
	}
	in = sack_fopen( 0, fg.font_cache_path, "rt" );
	if( !in )
	{
		fg.flags.bScanningFonts = 1;
		BuildFontCache(); // destroys the trees used to create the cache...
		fg.flags.bScanningFonts = 0;
		in = sack_fopen( 0, fg.font_cache_path, "rt" );
	}
	if( in )
	{
		// lines in cache are built to be limited to 80...
		TEXTCHAR fgets_buf[128];
		TEXTCHAR *buf;
		size_t len;
		uint32_t PathID = 0;
		size_t PathOfs = 0;
		uint32_t FamilyID = 0;
		size_t FamilyOfs = 0;
		uint32_t StyleID = 0;
		size_t StyleOfs = 0;
		uint32_t FileID = 0;
		size_t FileOfs = 0;
		//uint32_t line = 0;
		uint32_t nFont = 0; // which font we're currently reading for.
		uint64_t tmpfiletime;
		tmpfiletime = GetFileWriteTime( fg.font_cache_path );
		//lprintf( "font cache is %p", in );
		if( tmpfiletime && fg.fontcachetime == tmpfiletime )
		{
			sack_fclose( in );
			return;  // already read.
		}
		fg.fontcachetime = tmpfiletime;

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
		{
			TEXTCHAR *style, *flags, *next, *count;
			PFONT_ENTRY pfe; // kept for contined things;
			PFONT_STYLE pfs;
			PSIZE_FILE  psfCurrent;
			while( sack_fgets( fgets_buf, sizeof( fgets_buf ), in ) )
			{
				buf = fgets_buf;
				len = StrLen( buf );
				buf[len-1] = 0; // kill \n on line.
				//lprintf( "Process: (%d)%s", ++line, buf );
				switch( buf[0] )
				{
					size_t len;
				case '%':
					switch( buf[1] )
					{
					case '@':
						fg.build.nPaths = (uint32_t)IntCreateFromText( buf + 2 );
						fg.build.pPathNames = NewArray( TEXTCHAR, (size_t)IntCreateFromText( strchr( buf, ',' ) + 1 ) );
						fg.build.pPathList = NewArray( TEXTCHAR*, fg.build.nPaths );
						break;
					case '$':
						fg.build.nStyle = 0;
						fg.build.nFamilies = (uint32_t)IntCreateFromText( buf + 2 );
						fg.build.pFamilyNames = NewArray( TEXTCHAR, (size_t)IntCreateFromText( strchr( buf, ',' ) + 1 ) );
						fg.build.pFamilyList = NewArray( TEXTCHAR*, fg.build.nFamilies );

						fg.nFonts     = fg.build.nFamilies;
						fg.pFontCache = NewArray( FONT_ENTRY, fg.build.nFamilies );
						{
							uint32_t n;
							for( n = 0; n < fg.build.nFamilies; n++ ) {
								fg.pFontCache[n].styles = NULL;
								fg.pFontCache[n].nStyles = 0;
								fg.pFontCache[n].flags.unusable = 1;
							}
						}
						break;
					case '*':
						fg.build.nStyles = (uint32_t)IntCreateFromText( buf + 2 );
						fg.build.pStyleNames = NewArray( TEXTCHAR, (size_t)IntCreateFromText( strchr( buf, ',' ) + 1 ) );
						fg.build.pStyleList = NewArray( TEXTCHAR*, fg.build.nStyles );
						break;
					case '&':
						fg.build.nFiles = (int)IntCreateFromText( buf + 2 );
						fg.build.pFileNames = NewArray( TEXTCHAR, (size_t)IntCreateFromText( strchr( buf, ',' ) + 1 ) );
						fg.build.pFileList = NewArray( TEXTCHAR*, fg.build.nFiles );
						break;
					case '#':
						{
							uint32_t nStyles;
							uint32_t nSizeFiles;
							uint32_t nAltFiles;
							uint32_t nSizes;
#ifdef __cplusplus_cli
#define SCANBUF mybuf
							char *mybuf = CStrDup( buf + 2 );
#else
#define SCANBUF buf+2
#endif
							if( tscanf( buf + 2, "%d,%d,%d,%d", &nStyles, &nSizeFiles, &nSizes, &nAltFiles ) == 4 )
							{
								fg.build.nStyle = 0;
								fg.build.pStyleSlab = NewArray( FONT_STYLE, nStyles );
								MemSet( fg.build.pStyleSlab, 0, sizeof( FONT_STYLE ) * nStyles );
								fg.build.nSizeFile = 0;
								fg.build.pSizeFileSlab = NewArray( SIZE_FILE, nSizeFiles );
								MemSet( fg.build.pSizeFileSlab, 0, sizeof( SIZE_FILE ) * nSizeFiles );
								fg.build.nSize = 0;
								fg.build.pSizeSlab = NewArray( SIZES, nSizes );
								MemSet( fg.build.pSizeSlab, 0, sizeof( SIZES ) * nSizes );
								fg.build.nAlt = 0;
								fg.build.pAltSlab = NewArray( ALT_SIZE_FILE, nAltFiles );
								MemSet( fg.build.pAltSlab, 0, sizeof( ALT_SIZE_FILE ) * nAltFiles );
							}
							else
							{
								Log( "Error loading slab sizes!" );
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
						fg.build.pPathList[PathID] = fg.build.pPathNames + PathOfs;
						PathOfs += (len=StrLen( buf + 1 ) + 1);
						StrCpyEx( fg.build.pPathList[PathID++], buf + 1, len );
					}
					break;
				case '$':
					{
						fg.build.pFamilyList[FamilyID] = fg.build.pFamilyNames + FamilyOfs;
						FamilyOfs += (len=StrLen( buf + 1 ) + 1);
						StrCpyEx( fg.build.pFamilyList[FamilyID++], buf + 1, len );
					}
					break;
				case '*':
					{
						fg.build.pStyleList[StyleID] = fg.build.pStyleNames + StyleOfs;
						StyleOfs += (len=StrLen( buf + 1 ) + 1);
						StrCpyEx( fg.build.pStyleList[StyleID++], buf + 1, len );
					}
					break;
				case '&':
					{
						fg.build.pFileList[FileID] = fg.build.pFileNames + FileOfs;
						FileOfs += (len=StrLen( buf + 1 ) + 1);
						StrCpyEx( fg.build.pFileList[FileID++], buf + 1, len );
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
							uint32_t nStyles = (uint32_t)IntCreateFromText( count );
							pfe->flags.unusable = 0;
							pfe->styles = (PFONT_STYLE)(fg.build.pStyleSlab + fg.build.nStyle);
							fg.build.nStyle+=nStyles;
							//MemSet( pfe->styles, 0, nStyles * sizeof( FONT_STYLE ) );
						}
						pfe->name = fg.build.pFamilyList[IntCreateFromText( family )];
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
						pfs = pfe->styles + pfe->nStyles++;
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
						pfs->name = fg.build.pStyleList[IntCreateFromText( style )];

						pfs->nFiles = 0;
						pfs->files = (PSIZE_FILE)(fg.build.pSizeFileSlab + fg.build.nSizeFile);
						// demote from int64_t to uint32_t
						fg.build.nSizeFile += (uint32_t)IntCreateFromText( count );
						if(0)
						{
				case '\\':
							next = buf + 2;
						}
						{
							// continue size-fonts... (on style)
							TEXTCHAR *width, *height;
							int16_t nWidth, nHeight;
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

									psfCurrent = pfs->files + pfs->nFiles++;

									psfCurrent->path =fg.build.pPathList[IntCreateFromText( PathID )];
									psfCurrent->file =fg.build.pFileList[IntCreateFromText(file)];
									psfCurrent->nSizes = 0;
									psfCurrent->sizes = fg.build.pSizeSlab + fg.build.nSize;
									fg.build.nSize += (uint32_t)IntCreateFromText( count2 );
									//Allocate( sizeof( SIZES ) * IntCreateFromText( count2 ) );
									psfCurrent->nAlternate = 0;
									psfCurrent->pAlternate = fg.build.pAltSlab + fg.build.nAlt;
									fg.build.nAlt += (uint32_t)IntCreateFromText( count );
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
									nWidth = (int16_t)IntCreateFromText( width );
									if( nWidth >= 0 )
									{
										height = strchr( width, ',' );
										if( !height )
										{
											Log( "Fatality - Cache loses!" );
										}
										height++;
										nHeight = (int16_t)IntCreateFromText( height );
									}
									else
									{
										nHeight = -1;
										height = width + 1;
									}
									//Log2( "Add size to file: %d,%d", nWidth, nHeight );
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
									PCACHE_ALT_SIZE_FILE pasf = (PCACHE_ALT_SIZE_FILE)psfCurrent->pAlternate
										+ psfCurrent->nAlternate++;
									file = strchr( PathID, ':' );
									next = strchr( file, '^' );
									//*(PathID++) = 0;
									*(file++) = 0;
									if( next )
										*(next++) = 0;
									pasf->path =(PCACHE_DICT_ENTRY)fg.build.pPathList[IntCreateFromText(PathID)];
									pasf->file =(PCACHE_DICT_ENTRY)fg.build.pFileList[IntCreateFromText(file)];
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
			}
#ifdef __cplusplus_cli
			Deallocate( POINTER, buf );
#endif

		}
		sack_fclose( in );
#ifdef _DEBUG
		//DumpLoadedFontCache();
#endif
		Defragment( (POINTER*)&fg.pFontCache );
		//DebugDumpMemFile( "cache.loaded" );
	}
}

void IMGVER(UnloadAllFonts)( void )
{
   // Release all cached data...
}
#endif

IMAGE_NAMESPACE_END

