#include <stdhdrs.h>
#include <procreg.h>
#include <filesys.h>
#define TRANSLATION_SOURCE
#include <translation.h>

TRANSLATION_NAMESPACE 

struct translation {
	TEXTSTR name;
	PLIST strings;
};

#define STRING_INDEX_TYPE uint32_t

#define STRINGSPACE_SIZE (8192 - sizeof( uint32_t ) - 2 * sizeof( POINTER ))

typedef struct stringspace_tag
{
	uint32_t nextname;
	TEXTCHAR buffer[STRINGSPACE_SIZE];
	DeclareLink( struct stringspace_tag );
} STRINGSPACE, *PSTRINGSPACE;



static struct translation_local_tag
{
	PTREEROOT index;
	PLIST index_list;
	PSTRINGSPACE index_strings;
	PSTRINGSPACE translated_strings;
	uint32_t string_count;
	PLIST translations;
	struct translation *current_translation;
} *_translate_local;
#define translate_local (*_translate_local)

//---------------------------------------------------------------------------

PRELOAD( InitTrasnlationLocal )
{
	SimpleRegisterAndCreateGlobal( _translate_local );
	translate_local.index = CreateBinaryTreeEx( (GenericCompare)(int (MEM_API *)(CTEXTSTR , CTEXTSTR ))StrCmp, NULL );
}

//---------------------------------------------------------------------------

static CTEXTSTR SaveString( PSTRINGSPACE *root, uint32_t index, CTEXTSTR text )
{
	PSTRINGSPACE space = root[0];
	TEXTCHAR *p;
	size_t len = StrLen( text ) + sizeof( TEXTCHAR );
	for( ; space; space = space->next )
	{
		//lprintf( "Finding next name space free %p %p %p", l.NameSpace, space, space->next );
		if( ( space->nextname + len ) < ( STRINGSPACE_SIZE - ( index?5:1 ) ) )
		{
			break;
		}
	}

	if( !space )
	{
		space = New( STRINGSPACE );
		space->nextname = 0;
		//lprintf( "Adding new namespace %p", space );
		if( space->next = root[0] )
			root[0]->me = &space->next;
		space->me = root;
		root[0] = space;
	}
	if( index )
	{
		MemCpy( p = space->buffer + space->nextname + sizeof( STRING_INDEX_TYPE )
			, text,(uint32_t)(sizeof( TEXTCHAR)*(len)) );
		((STRING_INDEX_TYPE*)(space->buffer + space->nextname))[0] = index;
		space->nextname += (uint32_t)len + sizeof( STRING_INDEX_TYPE );
	}
	else
	{
		MemCpy( p = space->buffer + space->nextname, text,(uint32_t)(sizeof( TEXTCHAR)*(len)) );
		space->nextname += (uint32_t)len;
	}
	// +2 1 for byte of len, 1 for nul at end.
#if defined( __ARM__ ) || defined( UNDER_CE )
	space->nextname = ( space->nextname + 3 ) & 0xFFFFC;
	// +3&0xFC rounds to next full dword segment
	// arm requires this name be aligned on a dword boundry
	// because later code references this as a DWORD value.
#endif
	return (CTEXTSTR)p;
}

//---------------------------------------------------------------------------

void SaveTranslationDataToFile( FILE *output )
{
	uint32_t n;
	PSTRINGSPACE strings;
	for( n = 0, strings = translate_local.index_strings;
		 strings;
		  n++, strings = strings->next );

	sack_fwrite( &n, 1, sizeof( n ), output );
	for( strings = translate_local.index_strings;
		 strings;
		  strings = strings->next )
	{
		sack_fwrite( strings, 1, sizeof( strings[0] ), output );
	}
	{
		INDEX idx;
		struct translation *translation;
		LIST_FORALL( translate_local.translations, idx, struct translation *, translation )
		{
			int len;
			INDEX string_idx;
			uint32_t tmp;
			uint8_t length[3];
			CTEXTSTR string;
			len = StrLen( translation->name );
			sack_fwrite( &len, 1, sizeof( len ), output );
			sack_fwrite( translation->name, 1, len, output );
			LIST_FORALL( translation->strings, string_idx, CTEXTSTR, string )
			{
				tmp = string_idx + 1;
				sack_fwrite( &tmp, 1, 4, output );
				tmp = StrLen( string );
				if( tmp > 127 )
				{
					if( tmp > 32768 )
					{
						length[0] = 0x80 | ( ( tmp & 0x3FC000 ) >> 14 );
						length[1] = 0x80 | ( ( tmp & 0x3F80 ) >> 7 );
						length[2] =  ( tmp & 0x7F );
						sack_fwrite( length, 1, 3, output );
					}
					else
					{
						length[0] = 0x80 | ( ( tmp & 0x3F80 ) >> 7 );
						length[1] =  ( tmp & 0x7F );
						sack_fwrite( length, 1, 2, output );
					}
				}
				else
					sack_fwrite( &tmp, 1, 1, output );
				sack_fwrite( string, 1, tmp, output );
			}
			tmp = 0;
			sack_fwrite( &tmp, 1, 4, output );
		}
	}
}

//---------------------------------------------------------------------------

void SaveTranslationData( void )
{
	FILE *output = sack_fopen( 0, WIDE("strings.dat"), WIDE("wb") );
	if( !output )
		return;
	SaveTranslationDataToFile( output );
	sack_fclose( output );
}

//---------------------------------------------------------------------------

void LoadTranslationDataFromFile( FILE *input )
{
	uint32_t n;
	uint32_t b;
	PSTRINGSPACE strings;
	PSTRINGSPACE next;
	PSTRINGSPACE last;
	for( n = 0, strings = translate_local.index_strings;
		 strings;
		  n++, strings = next )
	{
		next = strings->next;
		Release( strings );
	}
	translate_local.index_strings = NULL;

	sack_fread( &n, 1, sizeof( n ), input );
	for( b = 0; b < n; b++ )
	{
		strings = New( STRINGSPACE );
		sack_fread( strings, 1, sizeof( strings[0] ), input );
		if( !translate_local.index_strings )
		{
			translate_local.index_strings = strings;
			strings->me = &translate_local.index_strings;
			strings->next = NULL;
		}
		else
		{
			last->next = strings;
			strings->me = &last->next;
			strings->next = NULL;
		}
		last = strings;
	}
	{
		INDEX idx;
		uint32_t len;
		TEXTSTR string = NULL; // buffer used to read string into
		uint32_t maxlen = 0; // length of buffer to use to read string
		struct translation *translation;
		while( sack_fread( &len, 1, sizeof( len ), input ) )
		{
			INDEX string_idx;
			uint32_t tmp;
			uint8_t length[3];
			translation = New( struct translation );
			AddLink( &translate_local.translations, translation );
			translation->strings = NULL;
			translation->name = NewArray( TEXTCHAR, len + 1 );

			sack_fread( translation->name, 1, len, input );
			translation->name[len] = 0;

			while( sack_fread( &string_idx, 1, 4, input ) && string_idx )
			{
				fread( length, 1, 1, input );
				if( length[0] & 0x80 )
				{
					fread( length + 1, 1, 1, input );
					if( length[0] & 0x80 )
					{
						fread( length + 2, 1, 1, input );
						tmp = ( ( ( length[0] & 0x7f ) << 7 ) | ( length[1] & 0x7f ) ) << 7 | length[2];
					}
					else
						tmp = ( ( length[0] & 0x7f ) << 7 ) | length[1];
				}
				else
					tmp = length[0];
				if( tmp > maxlen )
				{
					if( string )
						Release( string );
					string = NewArray( TEXTCHAR, tmp + 16 );
					maxlen = tmp + 16;
				}
				sack_fread( string, 1, tmp, input );
				string[tmp] = 0;
				SetLink( &translation->strings, string_idx - 1
					, SaveString( &translate_local.translated_strings, 0, string ) );
			}
		}
		Deallocate( TEXTSTR, string );
	}

	{
		PSTRINGSPACE space;
		for( space = translate_local.index_strings; space; space = space->next )
		{
			uint32_t offset = 0;
			uint32_t index;
			CTEXTSTR string;
			for(offset = 0; offset < space->nextname; offset++ )
			{
				index = ((STRING_INDEX_TYPE*)( space->buffer + offset ))[0];
				SetLink( &translate_local.index_list, index - 1, string = ( space->buffer + offset + 4 ) );
				AddBinaryNode( translate_local.index, (CPOINTER)((uintptr_t)index), (uintptr_t)string );

				offset += StrLen( string ) + 4;
			}
		}
	}
}

//---------------------------------------------------------------------------
void LoadTranslationData( void )
{
	FILE *input;
	input = sack_fopen( 0, WIDE("strings.dat"), WIDE("rb") );
	if( input )
	{
		LoadTranslationDataFromFile( input );
		sack_fclose( input );
	}
}
//---------------------------------------------------------------------------


CTEXTSTR TranslateText( CTEXTSTR text )
{
	uintptr_t psvIndex = (uintptr_t)FindInBinaryTree( translate_local.index, (uintptr_t)text );
	if( !psvIndex )
	{
		CTEXTSTR index_text = SaveString( &translate_local.index_strings, translate_local.string_count+1, text );
		PLIST list = SetLink( &translate_local.index_list, translate_local.string_count, index_text );
		AddBinaryNode( translate_local.index, (CPOINTER)((uintptr_t)translate_local.string_count+1), (uintptr_t)index_text );
		translate_local.string_count++;
		psvIndex = translate_local.string_count;
	}
	if( translate_local.current_translation )
	{
		CTEXTSTR output = (CTEXTSTR)GetLink( &translate_local.current_translation->strings, psvIndex-1 );
		if( output )
			return output;
	}

	return text;
}

//---------------------------------------------------------------------------

LOGICAL SetCurrentTranslation( CTEXTSTR language )
{
	INDEX idx;
	struct translation *translation;
	LIST_FORALL( translate_local.translations, idx, struct translation *, translation )
	{
		if( StrCaseCmp( translation->name, language ) == 0 )
		{
			translate_local.current_translation = translation;
			return TRUE;
		}
	}
	return FALSE;
}

//---------------------------------------------------------------------------

struct translation * GetTranslation( CTEXTSTR language )
{
	INDEX idx;
	struct translation *translation;
	LIST_FORALL( translate_local.translations, idx, struct translation *, translation )
	{
		if( StrCaseCmp( translation->name, language ) == 0 )
		{
			return translation;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------

struct translation * CreateTranslation( CTEXTSTR language )
{
	INDEX idx;
	struct translation *translation;
	LIST_FORALL( translate_local.translations, idx, struct translation *, translation )
	{
		if( StrCaseCmp( translation->name, language ) == 0 )
		{
			translate_local.current_translation = translation;
			return translation;
		}
	}
	translation = New( struct translation );
	translation->name = StrDup( language );
	translation->strings = NULL;
	AddLink( &translate_local.translations, translation );
	return translation;
}

//---------------------------------------------------------------------------

void SetTranslatedString( struct translation *translation, INDEX idx, CTEXTSTR string )
{
	if( translation )
	{
		SetLink( &translation->strings, idx - 1, SaveString( &translate_local.translated_strings, 0, string ) );
	}
}

//---------------------------------------------------------------------------

CTEXTSTR GetTranslationName( struct translation *translation )
{
	return translation->name;
}

//---------------------------------------------------------------------------

PLIST GetTranslations( void )
{
	return translate_local.translations;
}

//---------------------------------------------------------------------------

PLIST GetTranslationStrings( struct translation *translation )
{
	return translation->strings;
}

//---------------------------------------------------------------------------

PLIST GetTranslationIndexStrings( void )
{
	return translate_local.index_list;
}

TRANSLATION_NAMESPACE_END

