#include <stdhdrs.h>
#include <procreg.h>
#include <filesys.h>
#include <json_emitter.h>
#define TRANSLATION_SOURCE
#include <translation.h>

TRANSLATION_NAMESPACE 

//struct translation {
//	TEXTSTR name;
//	PLIST strings;
//};

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
	LOGICAL updated;
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
	translate_local.updated = TRUE;
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

static FILE *dumpOutput;
static LOGICAL dumpFirst;
static int Dump( CPOINTER user, uintptr_t key )
{
	sack_fprintf( dumpOutput, "\t\t%s\"%d\":\"%s\"\n", dumpFirst?"":",", (uint32_t)(uintptr_t)user, key );
	dumpFirst = FALSE;
	return 0;
}

//---------------------------------------------------------------------------

void SaveTranslationDataToFile( FILE *output )
{
	//uint32_t n;
	//PSTRINGSPACE strings;
	/*
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
	*/
	sack_fprintf( output, "{\n" );
	{
		INDEX idx;
		LOGICAL firstTranslation = TRUE;
		LOGICAL first = TRUE;
		//TEXTCHAR *string;
		struct translation *translation;
		sack_fprintf( output, "\t\"DEFAULT\":{\n" );
		dumpOutput = output;
		dumpFirst = TRUE;
		DumpTree( translate_local.index, Dump );

		sack_fprintf( output, "\t}\n" );
		LIST_FORALL( translate_local.translations, idx, struct translation *, translation )
		{
			//size_t len;
			INDEX string_idx;
			//uint32_t tmp;
			//uint8_t length[3];
			CTEXTSTR string;
			first = TRUE;
			sack_fprintf( output,"\t,\"%s\":{\n", translation->name );
			//len = StrLen( translation->name );
			//sack_fwrite( &len, sizeof( len ), 1, output );
			//sack_fwrite( translation->name, len, 1, output );
			LIST_FORALL( translation->strings, string_idx, CTEXTSTR, string )
			{
				char *tmp;
				sack_fprintf( output,"\t\t%s\"%d\":\"%s\"\n", (first)?"":",", string_idx, tmp = json_escape_string(string) );
				first = FALSE;
				Deallocate( char *, tmp );
/*
				tmp = (uint32_t)(string_idx + 1);
				sack_fwrite( &tmp, 1, 4, output );
				tmp = (uint32_t)StrLen( string );
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
*/
			}
			sack_fprintf( output, "\t}\n");
			//tmp = 0;
			//sack_fwrite( &tmp, 1, 4, output );
		}
	}
	sack_fprintf( output, "}\n");
}

//---------------------------------------------------------------------------

void SaveTranslationDataEx( const char *filename )
{
	if( translate_local.updated ) {
		FILE *output = sack_fopen( 0, filename, WIDE("wb") );
		if( !output )
			return;
		SaveTranslationDataToFile( output );
		sack_fclose( output );
	}
}

//---------------------------------------------------------------------------

void SaveTranslationData( void )
{
	SaveTranslationDataEx( "strings.json" );
}

//---------------------------------------------------------------------------

void LoadTranslationDataFromMemory( POINTER input, size_t length )
{
	PDATALIST data = NULL;
	char *_input = NewArray( char, length );
	memcpy( _input, input, length );
	if( json_parse_message( (char*)_input, length, &data ) ) {
		struct json_value_container *val;
		INDEX idx;
		struct json_value_container *val3;
		INDEX idx3;
		DATA_FORALL( data, idx3, struct json_value_container *, val3 ) {
			if( val3->value_type == VALUE_OBJECT ) {
				DATA_FORALL( val3->contains, idx, struct json_value_container *, val ) {
					if( val->value_type != VALUE_OBJECT ) 
						continue;
					struct json_value_container *val2;
					INDEX idx2;
					if( StrCmp( val->name, "DEFAULT" ) == 0 ) {

						DATA_FORALL( val->contains, idx2, struct json_value_container *, val2 ) {
							if( val2->value_type != VALUE_STRING ) continue;
							CTEXTSTR index_text = SaveString( &translate_local.index_strings, (uint32_t)IntCreateFromText( val2->name ), val2->string );
							PLIST list = SetLink( &translate_local.index_list, translate_local.string_count, index_text );
							AddBinaryNode( translate_local.index, (CPOINTER)((uintptr_t)translate_local.string_count+1), (uintptr_t)index_text );
							translate_local.string_count++;
						}
					
					} else {
						struct translation *translation = CreateTranslation( val->name );
						DATA_FORALL( val->contains, idx2, struct json_value_container *, val2 ) {
							uint64_t index = IntCreateFromText( val2->name );
							SetTranslatedString( translation, index, val2->string ); 
						}
					
					}
					
				}
			}
		}
		json_dispose_message( &data );
	}
	Deallocate( char *, _input );
	translate_local.updated = FALSE;
}

//---------------------------------------------------------------------------
void LoadTranslationDataEx( const char *filename )
{
	size_t size = 0;
	char *tmp = ExpandPath( filename );
	POINTER file = OpenSpace( NULL, tmp, &size );
	Deallocate( char *, tmp );
	//lprintf( "load file:%p", file );
	if( file )
	{
		LoadTranslationDataFromMemory( file, size );
		CloseSpace( file );
	}
}

void LoadTranslationDataFromFile( FILE *file ) {
	size_t size;
	char *data;
	size = sack_fsize( file );
	data = NewArray( char, size );
	size = sack_fread( data, size, 1, file );
	LoadTranslationDataFromMemory( data, size );
}

//---------------------------------------------------------------------------

void LoadTranslationData( void )
{
	LoadTranslationDataEx( "strings.json" );
}
//---------------------------------------------------------------------------

CTEXTSTR TranslateText( CTEXTSTR text )
{
	uintptr_t psvIndex = (uintptr_t)FindInBinaryTree( translate_local.index, (uintptr_t)text );
	if( !psvIndex )
	{
		text = SaveString( &translate_local.index_strings, translate_local.string_count+1, text );
		SetLink( &translate_local.index_list, translate_local.string_count, text );
		AddBinaryNode( translate_local.index, (CPOINTER)((uintptr_t)translate_local.string_count+1), (uintptr_t)text );
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
	translate_local.updated = TRUE;
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

