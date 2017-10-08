/* provides text translation.
  Primary Usage:
      SetTranslation( "some string" );

	 CTEXTSTR result = TranslateText( "some string to translate" );

	 lprintf( TranslateText( "Some format string %d:%d" ), x, y );

*/

#ifndef TRANSLATIONS_DEFINED
/* Multiple inclusion protection symbol. */
#define TRANSLATIONS_DEFINED

#ifdef __cplusplus
#  define _TRANSLATION_NAMESPACE namespace translation {
#  define _TRANSLATION_NAMESPACE_END }
#  define 	SACK_TRANSLATION_NAMESPACE_END } }
#  define USE_TRANSLATION_NAMESPACE using namespace sack::translation;
#else
#  define _TRANSLATION_NAMESPACE
#  define _TRANSLATION_NAMESPACE_END
#  define 	SACK_TRANSLATION_NAMESPACE_END
#  define USE_TRANSLATION_NAMESPACE 
#endif
#  define TRANSLATION_NAMESPACE SACK_NAMESPACE _TRANSLATION_NAMESPACE
#  define TRANSLATION_NAMESPACE_END _TRANSLATION_NAMESPACE_END  SACK_NAMESPACE_END

SACK_NAMESPACE
	/* Namespace of custom math routines.  Contains operators
	 for Vectors and fractions. */
	_TRANSLATION_NAMESPACE

#define TRANSLATION_API CPROC
#  ifdef TRANSLATION_SOURCE
#    define TRANSLATION_PROC EXPORT_METHOD
#  else
/* Define the library linkage for a these functions. */
#    define TRANSLATION_PROC IMPORT_METHOD
#  endif

struct translation {
	TEXTSTR name;
	PLIST strings;
};

typedef struct translation *PTranslation;


TRANSLATION_PROC LOGICAL TRANSLATION_API SetCurrentTranslation( CTEXTSTR language );
TRANSLATION_PROC CTEXTSTR TRANSLATION_API TranslateText( CTEXTSTR text );



TRANSLATION_PROC PTranslation TRANSLATION_API CreateTranslation( CTEXTSTR language );
TRANSLATION_PROC struct translation * TRANSLATION_API GetTranslation( CTEXTSTR language );
TRANSLATION_PROC void TRANSLATION_API SetTranslatedString( PTranslation translation, INDEX idx, CTEXTSTR string );
TRANSLATION_PROC CTEXTSTR TRANSLATION_API GetTranslationName( PTranslation translation );

TRANSLATION_PROC void TRANSLATION_API SaveTranslationDataEx( const char *filename );
TRANSLATION_PROC void TRANSLATION_API SaveTranslationData( void );
TRANSLATION_PROC void TRANSLATION_API SaveTranslationDataToFile( FILE *output );

TRANSLATION_PROC void TRANSLATION_API LoadTranslationDataEx( const char *filename );
TRANSLATION_PROC void TRANSLATION_API LoadTranslationData( void );
TRANSLATION_PROC void TRANSLATION_API LoadTranslationDataFromMemory( POINTER data, size_t length );
TRANSLATION_PROC void TRANSLATION_API LoadTranslationDataFromFile( FILE *file );

/* 
   return: PLIST is a list of PTranslation
*/
TRANSLATION_PROC PLIST TRANSLATION_API GetTranslations( void ); 

TRANSLATION_PROC CTEXTSTR TRANSLATION_API GetTranslationName( struct translation *translation );
/*
	return: PLIST of CTEXTSTR which are result strings of this translation
*/
TRANSLATION_PROC PLIST TRANSLATION_API GetTranslationStrings( struct translation *translation );
/*
  return: PLIST of CTEXTSTR which are source index strings
  */
TRANSLATION_PROC PLIST TRANSLATION_API GetTranslationIndexStrings( );



SACK_TRANSLATION_NAMESPACE_END
USE_TRANSLATION_NAMESPACE

#endif
