#ifndef SQL_GET_OPTION_DEFINED
#define SQL_GET_OPTION_DEFINED
#include <sack_types.h>
#include <pssql.h>
#include <procreg.h>

#ifdef __cplusplus
#define _OPTION_NAMESPACE namespace options {
#define _OPTION_NAMESPACE_END }
#define USE_OPTION_NAMESPACE 	using namespace sack::sql::options;

#else
#define _OPTION_NAMESPACE
#define _OPTION_NAMESPACE_END
#define USE_OPTION_NAMESPACE
#endif

SACK_NAMESPACE
   _SQL_NAMESPACE
	/* Contains methods for saving and recovering options from a
	   database. If enabled, will use a local option.db sqlite
	   database. Use EditOptions application to modify options. Can
	   use any database connection, but sql.config file will specify
	   'option.db' to start.                                         */
	_OPTION_NAMESPACE

#define SACK_OPTION_NAMESPACE SACK_NAMESPACE _SQL_NAMESPACE _OPTION_NAMESPACE
#define SACK_OPTION_NAMESPACE_END _OPTION_NAMESPACE_END _SQL_NAMESPACE_END SACK_NAMESPACE_END



#ifdef SQLGETOPTION_SOURCE
#define SQLGETOPTION_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SQLGETOPTION_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifndef __NO_INTERFACES__
   _INTERFACE_NAMESPACE
/* Defines a set of functions that can be registered as an
   interface, and the interface can be used for saving options. Module
   ideas might be to save into the windows registry system or
   into INI files.                                                     */
typedef struct option_interface_tag
{
   // these provide simple section, key, value queries.
	METHOD_PTR( size_t, GetPrivateProfileString )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTSTR pBuffer, size_t nBuffer, CTEXTSTR pININame );
	METHOD_PTR( int32_t, GetPrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t nDefault, CTEXTSTR pININame );
	METHOD_PTR( size_t, GetProfileString )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTSTR pBuffer, size_t nBuffer );
	METHOD_PTR( int32_t, GetProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t defaultval );

   // these provide an additional level of abstraction - the ini file
	METHOD_PTR( LOGICAL, WritePrivateProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile );
	METHOD_PTR( int32_t, WritePrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, int32_t value, CTEXTSTR pINIFile );
	METHOD_PTR( LOGICAL, WriteProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue );
	METHOD_PTR( int32_t, WriteProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, int32_t value );

   // these offer(expose) the option to be quiet
	METHOD_PTR( size_t, GetPrivateProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTSTR pBuffer, size_t nBuffer, CTEXTSTR pININame, LOGICAL bQuiet );
	METHOD_PTR( int32_t, GetPrivateProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t nDefault, CTEXTSTR pININame, LOGICAL bQuiet );
	METHOD_PTR( size_t, GetProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTSTR pBuffer, size_t nBuffer, LOGICAL bQuiet );
	METHOD_PTR( int32_t, GetProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t defaultval, LOGICAL bQuiet );

	METHOD_PTR( LOGICAL, WriteProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush );
	METHOD_PTR( LOGICAL, WritePrivateProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL commit );

} *POPTION_INTERFACE;

#define GetOptionInterface() ((POPTION_INTERFACE)GetInterface( WIDE("options") ))
//POPTION_INTERFACE GetOptionInterface( void );
//void DropOptionInterface( POPTION_INTERFACE );
#ifndef DEFAULT_OPTION_INTERFACE
#define DEFAULT_OPTION_INTERFACE ((!pOptionInterface)?(pOptionInterface=GetOptionInterface()):pOptionInterface)
#ifdef USES_OPTION_INTERFACE
static POPTION_INTERFACE pOptionInterface;
#ifdef __WATCOMC__
static void UseInterface( void )
{
	// use the value of this function and set pOptionInterface with it
	// makes pOptionInterface marked as used so is UseInterface.
	// Visual Studio pucked on this because converting a function pointer to data pointer
   // but this function should never be called.
   pOptionInterface = (POPTION_INTERFACE)UseInterface;
}
#endif
#endif

#endif
   _INTERFACE_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::sql::options::Interface;
#endif
#endif

#define OptGetPrivateProfileString   METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetPrivateProfileString)
#define OptGetPrivateProfileInt      METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetPrivateProfileInt)
#define OptGetProfileString          METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetProfileString)
#define OptGetProfileInt             METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetProfileInt)
#define OptWritePrivateProfileString METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WritePrivateProfileString)
#define OptWritePrivateProfileInt    METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WritePrivateProfileInt)
#define OptWriteProfileString        METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WriteProfileString)
#define OptWriteProfileInt           METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WriteProfileInt)
#define OptGetPrivateProfileStringEx   METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetPrivateProfileStringEx)
#define OptGetPrivateProfileIntEx      METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetPrivateProfileIntEx)
#define OptGetProfileStringEx          METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetProfileStringEx)
#define OptGetProfileIntEx             METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),GetProfileIntEx)
#define OptWritePrivateProfileStringEx     METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WritePrivateProfileStringEx)
#define OptWriteProfileStringEx     METHOD_ALIAS((DEFAULT_OPTION_INTERFACE),WriteProfileStringEx)

SACK_OPTION_NAMESPACE_END
#endif 
