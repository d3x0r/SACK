#ifndef SQL_OPTIONS_DEFINED
#define SQL_OPTIONS_DEFINED
#include <pssql.h>
// sqloptint.h leaves namespace open.
// these headers should really be collapsed.
#include <sqloptint.h>


SACK_OPTION_NAMESPACE

typedef struct sack_option_tree_family_node *POPTION_TREE_NODE;
typedef struct sack_option_tree_family *POPTION_TREE;

/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileString )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer, CTEXTSTR pININame );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pININame );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( size_t, SACK_GetProfileString )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( int, SACK_GetProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR **pBuffer, size_t *pnBuffer );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( int, SACK_GetProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR **pBuffer, size_t *pnBuffer );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( S_32, SACK_GetProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 defaultval );

/* All gets eventually end up here. This function gets a value
   from a database. Functions which return an 'int' use this
   function, but has extra processing to convert the text into a
   number; also if the text is 'Y', or 'y' then the option's int
   value is 1.
   Parameters
   pSection :     Path of the option to retrieve.
   pOptname :     Actual option name to retrieve.
   pDefaultbuf :  Default value if the option doesn't exist
                  already.
   pBuffer :      Pointer to the buffer to get the result
   nBuffer :      size of the result buffer in characters (not
                  bytes).
   pININame :     This is the upper level name. If a function
                  does not have a pININame, then the name
                  "DEFAULT' is used. (pass NULL here for
                  non\-private)
   bQuiet :       Boolean, if configured to prompt the user for
                  option values, this overrides the default to
                  disable prompting.                             */
SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer, CTEXTSTR pININame, LOGICAL bQuiet );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pININame, LOGICAL bQuiet );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( size_t, SACK_GetProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer, LOGICAL bQuiet );
/* <combine sack::sql::options::SACK_GetPrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@TEXTCHAR *@size_t@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                                        */
SQLGETOPTION_PROC( S_32, SACK_GetProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 defaultval, LOGICAL bQuiet );

/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( S_32, SACK_WritePrivateProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value, CTEXTSTR pINIFile, LOGICAL bQuiet );
SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL bFlush );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( LOGICAL, SACK_WriteProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIfile, LOGICAL flush );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( S_32, SACK_WriteProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value, LOGICAL bQuiet );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( S_32, SACK_WritePrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value, CTEXTSTR pINIFile );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( LOGICAL, SACK_WriteProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( LOGICAL, SACK_WriteOptionString )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( int, SACK_WriteProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( int, SACK_WriteProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer );
/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( S_32, SACK_WriteProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value );


/* <combinewith sack::sql::options::SACK_WritePrivateProfileStringEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@LOGICAL>
   
   \ \                                                                                                            */
SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileStringExxx )( PODBC odbc
																				, CTEXTSTR pSection
																				, CTEXTSTR pOptname
																				, CTEXTSTR pDefaultbuf
																				, TEXTCHAR *pBuffer
																				, size_t nBuffer
																				, CTEXTSTR pININame
																				, LOGICAL bQuiet
																				 DBG_PASS
																				);


#ifdef __NO_OPTIONS__
#define SACK_GetProfileInt( s,e,d ) (d)
#define SACK_GetProfileString( s,e,d,b,n ) ((d)?StrCpyEx( b,d,n ):0)
#endif


#define SACK_GetPrivateOptionString( odbc, section, option, default_buf, buf, buf_size, ini_name )   \
	SACK_GetPrivateProfileStringExxx( odbc, section, option, default_buf, buf, buf_size, ini_name, FALSE DBG_SRC )

#define SACK_GetPrivateOptionStringEx( odbc, section, option, default_buf, buf, buf_size, ini_name, quiet )   \
   SACK_GetPrivateProfileStringExxx( odbc, section, option, default_buf, buf, buf_size, ini_name, quiet DBG_SRC )

#define SACK_GetOptionString( odbc, section, option, default_buf, buf, buf_size )   \
   SACK_GetPrivateProfileStringExxx( odbc, section, option, default_buf, buf, buf_size, NULL, FALSE DBG_SRC )

#define SACK_GetOptionStringEx( odbc, section, option, default_buf, buf, buf_size, quiet )   \
   SACK_GetPrivateProfileStringExxx( odbc, section, option, default_buf, buf, buf_size, NULL, quiet DBG_SRC )

SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileIntExx )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pININame, LOGICAL bQuiet DBG_PASS );

#define SACK_GetPrivateOptionInt( odbc, section, option, default_val, ini_name )   \
	SACK_GetPrivateProfileIntExx( odbc, section, option, default_val, ini_name, FALSE DBG_SRC )

#define SACK_GetPrivateOptionIntEx( odbc, section, option, default_val, ini_name, quiet )   \
   SACK_GetPrivateProfileIntExx( odbc, section, option, default_val, ini_name, quiet DBG_SRC )

#define SACK_GetOptionInt( odbc, section, option, default_val )   \
   SACK_GetPrivateProfileIntExx( odbc, section, option, default_val, NULL, FALSE DBG_SRC )

#define SACK_GetOptionIntEx( odbc, section, option, default_val, quiet )   \
   SACK_GetPrivateProfileIntExx( odbc, section, option, default_val, NULL, quiet DBG_SRC )


SQLGETOPTION_PROC( CTEXTSTR, GetSystemID )( void );

SQLGETOPTION_PROC( void, EnumOptions )( POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
                , PTRSZVAL psvUser );
SQLGETOPTION_PROC( void, EnumOptionsEx )( PODBC odbc, POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
                , PTRSZVAL psvUser );
SQLGETOPTION_PROC( POPTION_TREE, GetOptionTreeExxx )( PODBC odbc, PFAMILYTREE existing_tree DBG_PASS );


/* Sets the option database to use (does not prevent
   preload/deadstart code from using the old database) but this
   can be used for comparison utilities.
   Parameters
   odbc :  The PODBC connection to use. 
   
   See Also
   PODBC                                                        */
SQLGETOPTION_PROC( POPTION_TREE, SetOptionDatabase )( PODBC odbc );
SQLGETOPTION_PROC( CTEXTSTR, GetDefaultOptionDatabaseDSN )( void );
SQLGETOPTION_PROC( void, SetOptionDatabaseOption )( PODBC odbc, int bNewVersion );

SQLGETOPTION_PROC( void, BeginBatchUpdate )( void );
SQLGETOPTION_PROC( void, EndBatchUpdate )( void );

SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionIndexEx )( POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate DBG_PASS );
SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionIndexExx )( PODBC odbc, POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate DBG_PASS );
#define GetOptionIndex(p,f,b,v) GetOptionIndexEx( p,f,b,v,FALSE DBG_SRC )
SQLGETOPTION_PROC( size_t, GetOptionStringValueEx )( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len DBG_PASS );
SQLGETOPTION_PROC( size_t, GetOptionStringValue )( POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len );
SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionValueIndex )( POPTION_TREE_NODE ID );
SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionValueIndexEx )( PODBC odbc, POPTION_TREE_NODE ID );
SQLGETOPTION_PROC( LOGICAL, SetOptionStringValue )( POPTION_TREE tree, POPTION_TREE_NODE optval, CTEXTSTR pValue );
SQLGETOPTION_PROC( void, DeleteOption )( POPTION_TREE_NODE iRoot );
SQLGETOPTION_PROC( void, DuplicateOption )( POPTION_TREE_NODE iRoot, CTEXTSTR pNewName );
SQLGETOPTION_PROC( void, ResetOptionMap )( PODBC odbc ); // flush the map cache.  

SQLGETOPTION_PROC( PODBC, GetOptionODBCEx )( CTEXTSTR dsn, int version DBG_PASS );
SQLGETOPTION_PROC( void, DropOptionODBCEx )( PODBC odbc DBG_PASS );
SQLGETOPTION_PROC( PODBC, GetOptionODBC )( CTEXTSTR dsn, int version );
SQLGETOPTION_PROC( void, DropOptionODBC )( PODBC odbc );
#define GetOptionODBC( a,b) GetOptionODBCEx( a,b DBG_SRC )
#define DropOptionODBC(a) DropOptionODBCEx( a DBG_SRC )

SQLGETOPTION_PROC( void, FindOptions )( PODBC odbc, PLIST *result_list, CTEXTSTR name );

_OPTION_NAMESPACE_END _SQL_NAMESPACE_END SACK_NAMESPACE_END

	USE_OPTION_NAMESPACE
#endif
