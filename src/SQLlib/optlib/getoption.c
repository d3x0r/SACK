#define FORIEGN_KEYS_WORK
#define OPTION_MAIN_SOURCE
#define NO_UNICODE_C
#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif
// we want access to GLOBAL from sqltub
#define SQLLIB_SOURCE
#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sharemem.h>
#include <filesys.h>
#include <network.h>
//#include <controls.h> // INI prompt
#ifdef __WATCOMC__
#include <io.h> // unlink
#endif

#include <pssql.h>
#include <sqlgetoption.h>

#include "../sqlstruc.h"
// define this to show very verbose logging during creation and
// referencing of option tree...
//#define DETAILED_LOGGING

#define DEFAULT_PUBLIC_KEY WIDE( "DEFAULT" )
//#define DEFAULT_PUBLIC_KEY "system"

SQL_NAMESPACE
extern struct pssql_global *global_sqlstub_data;
SQL_NAMESPACE_END
/*
 Dump Option table...
 SELECT oname2.name,oname.name,optionvalues.string,omap.*
 FROM `optionmap` as omap
 join optionname as oname on omap.name_id=oname.name_id
 left join optionvalues on omap.value_id=optionvalues.value_id
 left join optionmap as omap2 on omap2.node_id=omap.parent_node_id
 left join optionname as oname2 on omap2.name_id=oname2.name_id
*/


SACK_OPTION_NAMESPACE

#include "optlib.h"

typedef struct sack_option_global_tag OPTION_GLOBAL;

#ifdef _cut_sql_statments_

SQL table create is in 'mkopttabs.sql'

#endif
#define og (*sack_global_option_data)
	OPTION_GLOBAL *sack_global_option_data;


//------------------------------------------------------------------------

//int mystrcmp( TEXTCHAR *x, TEXTCHAR *y )
//{
//   lprintf( WIDE("Is %s==%s?"), x, y );
//   return strcmp( x, y );
//}

//------------------------------------------------------------------------

#define MKSTR(n,...) #__VA_ARGS__
//char *tablestatements =
#include "makeopts.mysql"
;


SQLGETOPTION_PROC( void, SetOptionStringValueEx )( PODBC odbc, POPTION_TREE_NODE node, CTEXTSTR pValue )
{
	int drop_odbc = FALSE;
	if( !odbc )
	{
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
		drop_odbc = TRUE;
	}
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	OpenWriter( tree );
	New4CreateValue( tree, node, pValue );
	if( drop_odbc )
		DropOptionODBC( odbc );
	
}

LOGICAL SetOptionStringValue( POPTION_TREE tree, POPTION_TREE_NODE optval, CTEXTSTR pValue )
{
	LOGICAL retval = TRUE;
	EnterCriticalSec( &og.cs_option );
	OpenWriter( tree );
	retval = New4CreateValue( tree, optval, pValue );
	LeaveCriticalSec( &og.cs_option );
	return retval;

}


SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionIndexEx )( POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate, int bBypassParsing DBG_PASS );
#define GetOptionIndex(p,f,b,v) GetOptionIndexEx( p,f,b,v,FALSE,FALSE DBG_SRC )
SQLGETOPTION_PROC( void, CreateOptionDatabaseEx )( PODBC odbc, POPTION_TREE tree );

POPTION_TREE GetOptionTreeExxx( PODBC odbc, PFAMILYTREE existing_tree DBG_PASS )
{
	int drop_odbc = FALSE;
	POPTION_TREE tree = NULL;
	INDEX idx;
	if( !odbc )
	{
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
		drop_odbc = TRUE;
	}
	//_lprintf(DBG_RELAY)( "Finding tree for %p", odbc );
	LIST_FORALL( og.trees, idx, struct sack_option_tree_family*, tree )
	{
		if( tree->odbc == odbc )
		{
			//lprintf( "Tree existed." );
			break;
		}
	}
	if( !tree )
	{
		//_lprintf(DBG_RELAY)( WIDE( "need a new option tree for %p" ), odbc );
		tree = New( struct sack_option_tree_family );
		MemSet( tree, 0, sizeof( struct sack_option_tree_family ) );
		tree->root = GetFromSet( OPTION_TREE_NODE, &tree->nodes );
		//MemSet( tree->root, 0, sizeof( struct sack_option_tree_family_node ) );

		tree->root->guid = GuidZero();
		tree->root->flags.bHasValue = 0;

		tree->root->value = NULL;

		// if it's a new optiontree, pass it to create...
		if( existing_tree )
		{
         //_lprintf(DBG_RELAY)( "attaching existing tree..." );
			tree->option_tree = existing_tree;
		}
		else
		{
         //_lprintf(DBG_RELAY)( "attaching NEW tree..." );
			tree->option_tree = CreateFamilyTree( (int(CPROC*)(uintptr_t,uintptr_t))StrCaseCmp, NULL );
		}
		tree->odbc = odbc;
		tree->odbc_writer = NULL;
		// default to the old version... allow other code to select new version.

		tree->flags.bCreated = 0;
		AddLink( &og.trees, tree );
	}
	if( drop_odbc )
		DropOptionODBC( odbc );
	//lprintf( "return tree %p for odbc %p", tree, odbc );
	return tree;
}

POPTION_TREE SetOptionDatabase( PODBC odbc )
{
	if( odbc )
	{
		POPTION_TREE tree;
		// maybe, if previously open with private database, close that connection
#ifdef DETAILED_LOGGING
		lprintf( WIDE( "Set a new option databsae : %p" ), odbc );
#endif
		//lprintf( "new global database is %p", odbc );
		og.Option = odbc;
		og.flags.bInited = FALSE;
		CreateOptionDatabaseEx( odbc, tree = GetOptionTreeExxx( odbc, NULL DBG_SRC ) ); // make sure the option tables exist.
		return tree;
	}
	else
		return GetOptionTreeExxx( og.Option, NULL DBG_SRC );
}

PFAMILYTREE* GetOptionTree( PODBC odbc )
{
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	if( tree )
		return &tree->option_tree;
	return NULL;
}



SQLGETOPTION_PROC( void, CreateOptionDatabaseEx )( PODBC odbc, POPTION_TREE tree )
{
#ifdef DETAILED_LOGGING
	lprintf( "Create Option Database. %d", tree->flags.bCreated );
#endif
	if( !global_sqlstub_data->flags.bLogOptionConnection )
		SetSQLLoggingDisable( tree->odbc, TRUE );
	{
		PTABLE table;
		if( !tree->flags.bCreated )
		{
			table = GetFieldsInSQLEx( option4_name, FALSE DBG_SRC );
			CheckODBCTable( tree->odbc, table, CTO_MERGE );
			DestroySQLTable( table );
			table = GetFieldsInSQLEx( option4_map, FALSE DBG_SRC );
			CheckODBCTable( tree->odbc, table, CTO_MERGE );
			DestroySQLTable( table );
#if THIS_IS_A_GOOD_IDEA_OR_THIS_HAS_A_USE
			table = GetFieldsInSQLEx( option4_exception, FALSE DBG_SRC );
			CheckODBCTable( tree->odbc, table, CTO_MERGE );
			DestroySQLTable( table );
#endif
			table = GetFieldsInSQLEx( option4_values, FALSE DBG_SRC );
			CheckODBCTable( tree->odbc, table, CTO_MATCH );
			DestroySQLTable( table );
			table = GetFieldsInSQLEx( option4_blobs, FALSE DBG_SRC );
			CheckODBCTable( tree->odbc, table, CTO_MERGE );
			DestroySQLTable( table );
			{
				// this needs a self-looped root to satisfy constraints.
				CTEXTSTR result;
				if( !SQLQueryf( tree->odbc, &result, WIDE("select parent_option_id from option4_map where option_id='%s'"), GuidZero() )
					|| !result )
				{
					OpenWriter( tree );
					SQLCommandf( tree->odbc_writer
					           , WIDE("insert into option4_map (option_id,parent_option_id,name_id)values('%s','%s','%s' )")
					           , GuidZero(), GuidZero()
					           , New4ReadOptionNameTable(tree,WIDE("."),OPTION4_NAME,WIDE( "name_id" ),WIDE( "name" ),1 DBG_SRC)
									);
				}
				SQLEndQuery( tree->odbc );
				//for( SQLQueryf( tree->odbc, &result, "select * from option4_map" ); result; FetchSQLResult( tree->odbc, &result ));
				//SQLEndQuery( tree->odbc );
				//for( SQLQueryf( tree->odbc, &result, "select * from option4_name" ); result; FetchSQLResult( tree->odbc, &result ));
				//SQLEndQuery( tree->odbc );
				//for( SQLQueryf( tree->odbc, &result, "select * from option4_values" ); result; FetchSQLResult( tree->odbc, &result ));
				//SQLEndQuery( tree->odbc );
			}
			tree->flags.bCreated = 1;
		}
	}
}

void SetOptionDatabaseOption( PODBC odbc )
{
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	if( tree )
	{
		if( global_sqlstub_data->flags.bInited )
			CreateOptionDatabaseEx( odbc, tree );
	}
}

static void CPROC OptionsCommited( uintptr_t psv, PODBC odbc )
{
	INDEX idx;
	POPTION_TREE_NODE optval;
	POPTION_TREE option = (POPTION_TREE)psv;
	LIST_FORALL( option->uncommited, idx, POPTION_TREE_NODE, optval )
	{
		if( optval->uncommited_write == odbc )
		{
			Deallocate( CTEXTSTR, optval->value );
			optval->value = NULL;
			optval->uncommited_write = NULL;
		}
	}
}

void OpenWriterEx( POPTION_TREE option DBG_PASS )
{
	if( !option->odbc_writer )
	{
#ifdef DETAILED_LOGGING
		_lprintf(DBG_RELAY)( WIDE( "Connect to writer database for tree %p odbc %p" ), option, option->odbc );
#endif
		option->odbc_writer = ConnectToDatabaseExx( option->odbc?option->odbc->info.pDSN:global_sqlstub_data->Primary.info.pDSN, FALSE DBG_RELAY );
		SQLCommand( option->odbc_writer, "pragma foreign_keys=on" );
      /*
		SQLCommand( option->odbc_writer, "pragma integrity_check" );
		{
			CTEXTSTR res = NULL;
			for( SQLQuery( option->odbc_writer, "select * from option4_name where name = 'system Settings'", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) );
			for( SQLQuery( option->odbc_writer, "PRAGMA foreign_key_check", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;// lprintf( "result:%s", res );;
			for( SQLQuery( option->odbc_writer, "PRAGMA foreign_key_check(option4_map)", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;//lprintf( "result:%s", res );;
			for( SQLQuery( option->odbc_writer, "PRAGMA foreign_key_list(option4_map)", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;//lprintf( "result:%s", res );;
			for( SQLQuery( option->odbc_writer, "PRAGMA foreign_key_list(option4_name)", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;//lprintf( "result:%s", res );;
			for( SQLQuery( option->odbc_writer, "PRAGMA foreign_key_list(option4_value)", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;//lprintf( "result:%s", res );;
			for( SQLQuery( option->odbc_writer, "select * from sqlite_master", &res );
				res;
				FetchSQLResult( option->odbc_writer, &res ) )
				;//lprintf( "result:%s", res );;
		}
*/
		//option->odbc_writer = SQLGetODBC( option->odbc?option->odbc->info.pDSN:global_sqlstub_data->Primary.info.pDSN );
		if( option->odbc_writer )
		{
			if( !global_sqlstub_data->flags.bLogOptionConnection )
				SetSQLLoggingDisable( option->odbc_writer, TRUE );
			SetSQLThreadProtect( option->odbc_writer, TRUE );
			SetSQLAutoTransactCallback( option->odbc_writer, OptionsCommited, (uintptr_t)option );
			//SetSQLAutoClose( option->odbc_writer, TRUE );
		}
	}
}

//------------------------------------------------------------------------

#define CreateName(o,n) SQLReadNameTable(o,n,OPTION_MAP,WIDE( "name_id" ))

//------------------------------------------------------------------------

INDEX ReadOptionNameTable( POPTION_TREE tree, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS )
{
	int first_try = 1;
	TEXTCHAR query[256];
	TEXTCHAR *tmp;
	CTEXTSTR result = NULL;
	INDEX IDName = INVALID_INDEX;
	if( !table || !name )
		return INVALID_INDEX;

	// look in internal cache first...
	IDName = GetIndexOfName(tree->odbc,table,name);
	if( IDName != INVALID_INDEX )
		return IDName;

	if( !tree->odbc )
		DebugBreak();

	CreateOptionDatabaseEx( tree->odbc, tree );
	PushSQLQueryEx( tree->odbc );
retry:
	tmp = EscapeSQLStringEx( tree->odbc, name DBG_RELAY );
	tnprintf( query, sizeof( query ), WIDE("select %s from %s where %s like '%s'")
			  , col?col:WIDE("id")
			  , table, namecol, tmp );
	Release( tmp );
	if( SQLQueryEx( tree->odbc, query, &result DBG_RELAY) && result )
	{
		IDName = (INDEX)IntCreateFromText( result );
		SQLEndQuery( tree->odbc );
	}
	else if( bCreate )
	{
		TEXTSTR newval = EscapeSQLString( tree->odbc, name );
		tnprintf( query, sizeof( query ), WIDE("insert into %s (%s) values( '%s' )"), table, namecol, newval );
      //lprintf( "openwriter..." );
		OpenWriterEx( tree DBG_RELAY );
      //lprintf( "and the command..." );
		if( !SQLCommandEx( tree->odbc_writer, query DBG_RELAY ) )
		{
			// insert failed;  assume it's a duplicate key now, and retry.
			// on an option connection, maybe the name has been inserted, and is waiting in a commit-on-idle
			// ... if we try this a few times, the commit will happen; then we can re-select and continue as normal
			//lprintf( WIDE("insert failed, how can we define name %s?"), name );
			if( first_try )
			{
				first_try = 0;
				goto retry;
			}
         else
				lprintf( WIDE("insert failed, and retry again failed, how can we define name %s?"), name );
		}
		else
		{
			// all is well.
			IDName = FetchLastInsertIDEx( tree->odbc_writer, table, col?col:WIDE("id") DBG_RELAY );
		}
		Release( newval );
	}
	else
		IDName = INVALID_INDEX;

	PopODBCEx(tree->odbc);

	if( IDName != INVALID_INDEX )
	{
		AddBinaryNode( GetTableCache(tree->odbc,table), (POINTER)((uintptr_t)(IDName+1))
						 , (uintptr_t)SaveText( name ) );
	}
	return IDName;
}

//---------------------------------------------------------------------------

INDEX IndexCreateFromText( CTEXTSTR string )
{
   return (INDEX)IntCreateFromText( string );
}

//---------------------------------------------------------------------------

void ResetOptionMap( PODBC odbc )
{
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	if( tree )
		FamilyTreeClear( tree->option_tree );
}

//#define OPTION_ROOT_VALUE INVALID_INDEX
#define OPTION_ROOT_VALUE 0

static POPTION_TREE_NODE GetOptionIndexExxx( PODBC odbc, POPTION_TREE_NODE parent
														 , const TEXTCHAR *program_override
														 , const TEXTCHAR *file
														 , const TEXTCHAR *pBranch
														 , const TEXTCHAR *pValue
														 , int bCreate
                                           , int bBypassParsing
														 , int bIKnowItDoesntExist DBG_PASS )
{
	const TEXTCHAR *_program = program_override;
	size_t _program_length = _program?StrLen( _program ):0;
	const TEXTCHAR *program = NULL;
	static const TEXTCHAR *_system = NULL;
	const TEXTCHAR *system = NULL;
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	if( !parent )
		parent = tree->root;

	if( og.flags.bUseProgramDefault )
	{
		if( !_program )
		_program_length = StrLen( _program = GetProgramName() );
		if( ( StrCaseCmp( file, DEFAULT_PUBLIC_KEY ) == 0 )
			&& ( StrCaseCmpEx( pBranch, _program, _program_length ) == 0 ) )
		{
		}
		else if( ( StrCaseCmp( file, DEFAULT_PUBLIC_KEY ) != 0 )
			|| ( StrCaseCmpEx( pBranch, _program, _program_length ) != 0 ) )
		{
			program = file;
			file = _program;
		}
		else
		{
			if( !program )
				program = _program;
			// default options were SACK_GetProfileXX( GetProgramName(), Branch, ... )
			// so this should be that condition, with aw NULL file, which gwets promoted to DEFAULT_PUBLIC_KEY
			// before being called, and section will already be programname, and pValue will
			// just be the string default.
		}
	}
	if( og.flags.bUseSystemDefault )
	{
#ifndef __NO_NETWORK__
		if( !_system )
			_system = GetSystemName();
#else
		if( !_system )
			_system = GetSystemID();
#endif
		system = _system;
	}
	//lprintf( WIDE("GetOptionIndex for %s %s %s"), program?program:WIDE("NO PROG"), file, pBranch );
	return New4GetOptionIndexExxx( odbc, tree, parent, system, program, file, pBranch, pValue, bCreate, bBypassParsing, bIKnowItDoesntExist DBG_RELAY );
}

POPTION_TREE_NODE GetOptionIndexExx( PODBC odbc, POPTION_TREE_NODE parent, CTEXTSTR program, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate, int bBypassParsing DBG_PASS )
{
   return GetOptionIndexExxx( odbc, parent, program, file, pBranch, pValue, bCreate, bBypassParsing, FALSE DBG_RELAY );
}

POPTION_TREE_NODE GetOptionIndexEx( POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate, int bBypassParsing DBG_PASS )
{
   return GetOptionIndexExxx( og.Option, parent, NULL, file, pBranch, pValue, bCreate, bBypassParsing, FALSE DBG_RELAY );
}
//------------------------------------------------------------------------

INDEX GetSystemIndex( CTEXTSTR pSystemName )
{
	if( pSystemName )
		return SQLReadNameTable( og.Option, pSystemName, WIDE("systems"), WIDE("system_id") );
	else
	{
		if( !og.SystemID )
			og.SystemID = SQLReadNameTable( og.Option, pSystemName, WIDE("systems"), WIDE("system_id")  );
		return og.SystemID;
	}
}

//------------------------------------------------------------------------

POPTION_TREE_NODE New4DuplicateValue( PODBC odbc, POPTION_TREE_NODE iOriginalOption, POPTION_TREE_NODE iNewOption )
{
	TEXTCHAR query[256];
	CTEXTSTR *results;
	TEXTSTR tmp;
	PushSQLQueryEx( odbc );
	// my nested parent may have a select state in a condition that I think it's mine.
	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE( "select `string` from " )OPTION4_VALUES WIDE( " where option_id='%s'" ), iOriginalOption->guid );

	if( results && results[0] )
	{
		tnprintf( query, sizeof( query )
			  , WIDE( "replace into " )OPTION4_VALUES WIDE( " (option_id,`string`) values ('%s',%s)" )
				  , iNewOption->guid, tmp = EscapeSQLBinaryOpt( odbc, results[0], StrLen( results[0] ), TRUE ) );
		Release( tmp );
		SQLEndQuery( odbc );
		SQLCommand( odbc, query );
	}

	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE( "select `binary` from " )OPTION4_BLOBS WIDE( " where option_id='%s'" ), iOriginalOption->guid );

	if( results && results[0] )
	{
		tnprintf( query, sizeof( query )
				  , WIDE( "replace into " )OPTION4_BLOBS WIDE( " (option_id,`binary`) values ('%s',%s)" )
				  , iNewOption->guid, tmp = EscapeSQLBinaryOpt( odbc, results[0], StrLen( results[0] ), TRUE ) );
		Release( tmp );
		SQLEndQuery( odbc );
		SQLCommand( odbc, query );
	}
	PopODBCEx( odbc );
	return iNewOption;
}


// this changes in the new code...
POPTION_TREE_NODE DuplicateValue( POPTION_TREE_NODE iOriginalValue, POPTION_TREE_NODE iNewValue )
{
	POPTION_TREE_NODE result;
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	result = New4DuplicateValue( odbc, iOriginalValue, iNewValue );
	DropOptionODBC( odbc );
	return result;
}

//------------------------------------------------------------------------

size_t GetOptionStringValueEx( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len DBG_PASS )
{
	size_t res = New4GetOptionStringValue( odbc, optval, buffer, len DBG_RELAY );
	return res;
}

size_t GetOptionStringValue( POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	size_t result;
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	result = GetOptionStringValueEx( odbc, optval, buffer, len DBG_SRC );
	DropOptionODBC( odbc );
	return result;
}

int GetOptionBlobValueOdbc( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	CTEXTSTR *result = NULL;
	size_t tmplen;
	if( !len )
		len = &tmplen;
	PushSQLQueryEx( odbc );
	if( SQLRecordQueryf( odbc, NULL, &result, NULL
								, WIDE("select `binary`,length(`binary`) from ")OPTION4_BLOBS WIDE(" where option_id='%s'")
								, optval->guid ) )
	{
		int success = FALSE;
		//lprintf( WIDE(" query succeeded....") );
		if( buffer && result && result[0] && result[1] )
		{
			success = TRUE;
			(*buffer) = NewArray( TEXTCHAR, (*len)=(size_t)IntCreateFromText( result[1] ));
			MemCpy( (*buffer), result[0], (uintptr_t)(*len) );
		}
		PopODBCEx( odbc );
		return success;
	}
   return FALSE;
}


int GetOptionBlobValue( POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	return GetOptionBlobValueOdbc( og.Option, optval, buffer, len );
}

//------------------------------------------------------------------------

LOGICAL GetOptionIntValue( POPTION_TREE_NODE optval, int *result_value DBG_PASS )
{
	TEXTCHAR *value;
	if( GetOptionStringValueEx( og.Option, optval, &value, NULL DBG_RELAY ) != INVALID_INDEX )
	{
		if( value[0] == 'y' || value[0] == 'Y' || ( value[0] == 't' || value[0] == 'T' ) )
			*result_value = 1;
		else
			*result_value = (int)IntCreateFromText( value );
		return TRUE;
	}
	return FALSE;
}

//------------------------------------------------------------------------

/*
static LOGICAL CreateValue( POPTION_TREE tree, POPTION_TREE_NODE iOption, CTEXTSTR pValue )
{
	OpenWriter( tree );
	return New4CreateValue( tree, iOption, pValue );
}
*/


//------------------------------------------------------------------------
// result with option value ID

//------------------------------------------------------------------------
// result with option value ID
static LOGICAL SetOptionBlobValueEx( POPTION_TREE tree, POPTION_TREE_NODE optval, POINTER buffer, size_t length )
{
	OpenWriter( tree );
	{
		TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, (CTEXTSTR)buffer, length, TRUE );
		LOGICAL retval =
			SQLCommandf( tree->odbc_writer, WIDE( "replace into " )OPTION4_BLOBS WIDE( " (`option_id`,`binary` ) values ('%s',%s)" )
							, optval->guid
							, newval
							);
		Release( newval );
		return retval;
	}
}

#define DIA_X(x) x * 2
#define DIA_Y(y) y * 2
#define DIA_W(w) w * 2
#define DIA_H(h) h * 2

typedef int (CPROC *_F)(
									  CTEXTSTR lpszSection,
									  CTEXTSTR lpszEntry,
									  CTEXTSTR lpszDefault,
									  TEXTSTR lpszReturnBuffer,
									  size_t cbReturnBuffer,
									  CTEXTSTR filename
											  );


size_t SQLPromptINIValue(
													 CTEXTSTR lpszSection,
													 CTEXTSTR lpszEntry,
													 CTEXTSTR lpszDefault,
													 TEXTSTR lpszReturnBuffer,
													 size_t cbReturnBuffer,
													 CTEXTSTR filename
													)
{
#ifndef __NO_GUI__
#ifndef __STATIC__
	static _F _SQLPromptINIValue;
	if( !_SQLPromptINIValue )
		_SQLPromptINIValue = (_F)LoadFunction( _WIDE( TARGETNAME ), WIDE( "_SQLPromptINIValue" ) );
	if( !_SQLPromptINIValue )
		_SQLPromptINIValue = (_F)LoadFunction( WIDE( "bag.psi.dll" ), WIDE( "_SQLPromptINIValue" ) );
	if( !_SQLPromptINIValue )
		_SQLPromptINIValue =  (_F)LoadFunction( WIDE( "libbag.psi.so" ), WIDE( "_SQLPromptINIValue" ) );
	if( !_SQLPromptINIValue )
		_SQLPromptINIValue =  (_F)LoadFunction( WIDE( "sack_bag.dll" ), WIDE( "_SQLPromptINIValue" ) );
	if( _SQLPromptINIValue )
		return _SQLPromptINIValue(lpszSection, lpszEntry, lpszDefault, lpszReturnBuffer, cbReturnBuffer, filename );
#else
	//return _SQLPromptINIValue(lpszSection, lpszEntry, lpszDefault, lpszReturnBuffer, cbReturnBuffer, filename );
#endif
#endif
#if prompt_stdout
	fprintf( stdout, WIDE( "[%s]%s=%s?\nor enter new value:" ), lpszSection, lpszEntry, lpszDefault );
	fflush( stdout );
	if( fgets( lpszReturnBuffer, cbReturnBuffer, stdin ) && lpszReturnBuffer[0] != '\n' && lpszReturnBuffer[0] )
	{
      return StrLen( lpszReturnBuffer );
	}
#endif
	StrCpyEx( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
	lpszReturnBuffer[cbReturnBuffer-1] = 0;
	return StrLen( lpszReturnBuffer );
}

struct check_mask_param
{
	LOGICAL is_found;
	LOGICAL is_mapped;
	CTEXTSTR section_name;
	CTEXTSTR file_name;
	PODBC odbc;
};

static int CPROC CheckMasks( uintptr_t psv_params, CTEXTSTR name, POPTION_TREE_NODE this_node, int flags )
{
	struct check_mask_param *params = (struct check_mask_param*)psv_params;
	// return 0 to break loop.
	//lprintf( "Had mask to check [%s]", name );
	if( CompareMask( name, params->section_name, FALSE ) )
	{
		params->is_found = TRUE;
		//GetOptionStringValue( ... );
		{
			TEXTCHAR resultbuf[12];
			TEXTCHAR key[256];
			tnprintf( key, 256, WIDE("System Settings/Map INI Local/%s"), params->file_name );
			SACK_GetPrivateProfileStringExxx( params->odbc, key, name, WIDE("0"), resultbuf, 12, NULL, TRUE DBG_SRC );
			if( resultbuf[0] != '0' )
				params->is_mapped = TRUE;
		}
		return 0;
	}
	return TRUE;
}

//------------------------------------------------------------------------

static CTEXTSTR CPROC ResolveININame( PODBC odbc, CTEXTSTR pSection, TEXTCHAR *buf, CTEXTSTR pINIFile )
{
	if( pINIFile[0] == '.' && ( pINIFile[1] == '/' || pINIFile[1] == '\\' ) )
		pINIFile += 2;

	while( pINIFile[0] == '/' || pINIFile[0] == '\\' )
	pINIFile++;
	if( !pathchr( pINIFile ) )
	{
		if( ( pINIFile != DEFAULT_PUBLIC_KEY )
			&& ( StrCaseCmp( pINIFile, DEFAULT_PUBLIC_KEY ) != 0 ) )
		{
			//lprintf( "(Convert %s)", pINIFile );
			if( og.flags.bEnableSystemMapping == 2 )
				og.flags.bEnableSystemMapping = SACK_GetPrivateProfileIntExx( odbc, WIDE( "System Settings")
																									 , WIDE( "Enable System Mapping" ), 0, NULL, TRUE DBG_SRC );
			if( og.flags.bEnableSystemMapping )
			{
				TEXTCHAR resultbuf[12];
				struct check_mask_param params;
				params.is_mapped = FALSE;
				params.is_found = FALSE;

				// check masks first for wildcarded relocations.
				{
					POPTION_TREE_NODE node;
					params.section_name = pSection;
					params.file_name = pINIFile;
					params.odbc = odbc;
					//lprintf( "FILE is not mapped entirly, check enumerated options..." );
					tnprintf( buf, 128, WIDE("System Settings/Map INI Local Masks/%s"), pINIFile );
					//lprintf( "buf is %s", buf );
					node = GetOptionIndexExxx( odbc, NULL, DEFAULT_PUBLIC_KEY, NULL, NULL, buf, TRUE, FALSE, FALSE DBG_SRC );
					if( node )
					{
						//lprintf( "Node is %p?", node );
						EnumOptionsEx( odbc, node, CheckMasks, (uintptr_t)&params );
						//lprintf( "Done enumerating..." );
					}
				}
				if( !params.is_found )
				{
					SACK_GetPrivateProfileStringExxx( odbc, WIDE("System Settings/Map INI Local"), pINIFile, WIDE("0"), resultbuf, 12, NULL, TRUE DBG_SRC );
					if( resultbuf[0] != '0' )
					{
						params.is_found = 1;
						params.is_mapped = 1;
					}
				}
				if( !params.is_found )
				{
					tnprintf( buf, 128, WIDE("System Settings/Map INI Local/%s"), pINIFile );
					SACK_GetPrivateProfileStringExxx( odbc, buf, pSection, WIDE("0"), resultbuf, 12, NULL, TRUE DBG_SRC );
					if( resultbuf[0] != '0' )
						params.is_mapped = TRUE;
				}

#ifndef __NO_NETWORK__
				if( params.is_mapped )
				{
					tnprintf( buf, 128, WIDE("System Settings/%s/%s"), GetSystemName(), pINIFile );
					buf[127] = 0;
					pINIFile = buf;
				}
#endif
  			}
  			//lprintf( "(result %s)", pINIFile );
  		}
  	}
   return pINIFile;
}


//------------------------------------------------------------------------

SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileStringExxx )( PODBC odbc
																				, CTEXTSTR pSection
																				, CTEXTSTR pOptname
																				, CTEXTSTR pDefaultbuf
																				, TEXTCHAR *pBuffer
																				, size_t nBuffer
																				, CTEXTSTR pINIFile
																				, LOGICAL bQuiet
																				 DBG_PASS
																				)
{
	LOGICAL drop_odbc = FALSE;
	EnterCriticalSec( &og.cs_option );
	if( !odbc )
	{
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
		drop_odbc = TRUE;
	}

	if( !pINIFile )
		pINIFile = DEFAULT_PUBLIC_KEY;
	else
	{
		TEXTCHAR buf[128];
		pINIFile = ResolveININame( odbc, pSection, buf, pINIFile );
	}

	{
		// first try, do it as false, so we can fill in default values.
		POPTION_TREE_NODE opt_node;
		char *buffer;
		size_t buflen;
		// maybe do an if( l.flags.bLogOptionsRead )
#if defined( _DEBUG )
		if( global_sqlstub_data->flags.bLogOptionConnection )
			_lprintf(DBG_RELAY)( WIDE( "Getting option {%s}[%s]%s=%s" ), pINIFile, pSection, pOptname, pDefaultbuf );
#endif
		opt_node = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, NULL, pINIFile, pSection, pOptname, TRUE, FALSE DBG_RELAY );
		// used to have a test - get option value index; but option index == node_id
		// so it just returned the same node; but not quite, huh?
		GetOptionStringValueEx( odbc, opt_node, &buffer, &buflen DBG_RELAY );
		if( !buffer )
		{
			// this actually implies to delete the entry... but since it doesn't exist no worries...
			if( !pDefaultbuf )
			{
				if( drop_odbc )
					DropOptionODBC( odbc );
				LeaveCriticalSec( &og.cs_option );
				return 0;
			}
			// issue dialog
		//do_defaulting:
			//if( !bQuiet && og.flags.bPromptDefault )
			//{
			//	SQLPromptINIValue( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, pINIFile );
			//}
			//else
			{
				if( pDefaultbuf ){
					StrCpyEx( pBuffer, pDefaultbuf, nBuffer/sizeof( TEXTCHAR ) );
					buflen = StrLen( pBuffer );
				} else {
					pBuffer[0] = 0;
					buflen = 0;
				}
			}
			// create the option branch since it doesn't exist...
			{
				SetOptionStringValue( GetOptionTreeExxx( odbc, NULL DBG_SRC ), opt_node, pBuffer );
#if defined( _DEBUG )
				if( global_sqlstub_data->flags.bLogOptionConnection )
					lprintf( WIDE("default Result [%s]"), pBuffer );
#endif
				if( drop_odbc )
					DropOptionODBC( odbc );
				LeaveCriticalSec( &og.cs_option );
				return buflen;
			}
			//strcpy( pBuffer, pDefaultbuf );
		}
		else
		{
			MemCpy( pBuffer, buffer, buflen = ((buflen+1<(nBuffer) )?(buflen+1):nBuffer) );
			buflen--;
			pBuffer[buflen] = 0;
#if defined( _DEBUG )
			if( global_sqlstub_data->flags.bLogOptionConnection )
				lprintf( WIDE( "buffer result is [%s]" ), pBuffer );
#endif
			if( drop_odbc )
				DropOptionODBC( odbc );
			LeaveCriticalSec( &og.cs_option );
			return buflen;
		}
	}
}

SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileStringExx )( CTEXTSTR pSection
																		  , CTEXTSTR pOptname
																		  , CTEXTSTR pDefaultbuf
																		  , TEXTCHAR *pBuffer
																		  , size_t nBuffer
																		  , CTEXTSTR pINIFile
																		  , LOGICAL bQuiet
																			DBG_PASS
																				)
{
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	size_t result;
	result = SACK_GetPrivateProfileStringExxx( odbc,    pSection
																		  , pOptname
																		  , pDefaultbuf
																		  , pBuffer
																		  , nBuffer
																		  , pINIFile
																		  , bQuiet
																			DBG_RELAY
																		  );
	DropOptionODBC( odbc );
	return result;
}

SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileStringEx )( CTEXTSTR pSection
																		  , CTEXTSTR pOptname
																		  , CTEXTSTR pDefaultbuf
																		  , TEXTCHAR *pBuffer
																		  , size_t nBuffer
																		  , CTEXTSTR pINIFile
																		  , LOGICAL bQuiet
																		  )
{
	return SACK_GetPrivateProfileStringExx( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, pINIFile, bQuiet DBG_SRC );
}

SQLGETOPTION_PROC( size_t, SACK_GetPrivateProfileString )( CTEXTSTR pSection
                                                 , CTEXTSTR pOptname
                                                 , CTEXTSTR pDefaultbuf
                                                 , TEXTCHAR *pBuffer
                                                 , size_t nBuffer
                                                 , CTEXTSTR pINIFile )
{
	return SACK_GetPrivateProfileStringEx( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, pINIFile, FALSE );
}
//------------------------------------------------------------------------

SQLGETOPTION_PROC( int32_t, SACK_GetPrivateProfileIntExx )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, int32_t nDefault, CTEXTSTR pINIFile, LOGICAL bQuiet DBG_PASS )
{
	TEXTCHAR buffer[32];
	TEXTCHAR defaultbuf[32];
	tnprintf( defaultbuf, sizeof( defaultbuf ), WIDE("%") _32fs, nDefault );
	if( SACK_GetPrivateProfileStringExxx( odbc, pSection, pOptname, defaultbuf, buffer, sizeof( buffer )/sizeof(TEXTCHAR), pINIFile, bQuiet DBG_RELAY ) )
	{
		if( buffer[0] == 'Y' || buffer[0] == 'y' )
			return 1;
		return (int32_t)IntCreateFromText( buffer );
	}
	return nDefault;
}

SQLGETOPTION_PROC( int32_t, SACK_GetPrivateProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t nDefault, CTEXTSTR pINIFile, LOGICAL bQuiet )
{
	int32_t result;
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	result = SACK_GetPrivateProfileIntExx( odbc, pSection, pOptname, nDefault, pINIFile, bQuiet DBG_SRC );
	DropOptionODBC( odbc );
	return result;
}


SQLGETOPTION_PROC( int32_t, SACK_GetPrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t nDefault, CTEXTSTR pINIFile )
{
	return SACK_GetPrivateProfileIntEx( pSection, pOptname, nDefault, pINIFile, FALSE );
}

//------------------------------------------------------------------------


SQLGETOPTION_PROC( size_t, SACK_GetProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer, LOGICAL bQuiet )
{
	return SACK_GetPrivateProfileStringEx( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, NULL, bQuiet );
}

#undef SACK_GetProfileString
SQLGETOPTION_PROC( size_t, SACK_GetProfileString )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer )
{
	return SACK_GetPrivateProfileString( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, NULL );
}

//------------------------------------------------------------------------


SQLGETOPTION_PROC( int, SACK_GetProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR **pBuffer, size_t *pnBuffer )
{
	POPTION_TREE_NODE optval;
#ifdef DETAILED_LOGGING
	lprintf( WIDE( "Only single odbc available here." ) );
#endif
	optval = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, NULL, NULL, pSection, pOptname, FALSE, FALSE DBG_SRC );
	if( !optval )
	{
		return FALSE;
	}
	else
	{
		return GetOptionBlobValueOdbc( odbc, optval, pBuffer, pnBuffer );
	}
	return FALSE;
//   int status = SACK_GetProfileString( );
}

SQLGETOPTION_PROC( int, SACK_GetProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR **pBuffer, size_t *pnBuffer )
{
   return SACK_GetProfileBlobOdbc( og.Option, pSection, pOptname, pBuffer, pnBuffer );
}

//------------------------------------------------------------------------

SQLGETOPTION_PROC( int32_t, SACK_GetProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t defaultval, LOGICAL bQuiet )
{
   return SACK_GetPrivateProfileIntEx( pSection, pOptname, defaultval, NULL, bQuiet );
}

//------------------------------------------------------------------------
#undef SACK_GetProfileInt
SQLGETOPTION_PROC( int32_t, SACK_GetProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, int32_t defaultval )
{
   return SACK_GetPrivateProfileInt( pSection, pOptname, defaultval, NULL );
}

//------------------------------------------------------------------------
SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateOptionStringEx )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush )
{
	POPTION_TREE_NODE optval;
	if( !pINIFile )
		pINIFile = DEFAULT_PUBLIC_KEY;
	else
	{
      TEXTCHAR buf[128];
      pINIFile = ResolveININame( odbc, pSection, buf, pINIFile );
	}
#if defined( _DEBUG )
	if( global_sqlstub_data->flags.bLogOptionConnection )
		_lprintf( DBG_SRC )( WIDE( "Setting option {%s}[%s]%s=%s" ), pINIFile, pSection, pName, pValue );
#endif
	optval = GetOptionIndexExxx( odbc, NULL, NULL, pINIFile, pSection, pName, TRUE, FALSE, FALSE DBG_SRC );
	if( !optval )
	{
		lprintf( WIDE("Creation of path failed!") );
		return FALSE;
	}
	else
	{
		POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
		LOGICAL result = SetOptionStringValue( tree, optval, pValue );
		if( flush && tree->odbc_writer )
			SQLCommit( tree->odbc_writer );
#ifdef DETAILED_LOGGING
		lprintf( WIDE( "Set option value %d [%d]" ), optval, pValue );
#endif
		return result;
	}
}
//------------------------------------------------------------------------
SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush )
{
	PODBC odbc;
	LOGICAL result;
	odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	result = SACK_WritePrivateOptionStringEx( odbc, pSection, pName, pValue, pINIFile, flush );
	DropOptionODBC( odbc );
	return result;
}

SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile )
{
	return SACK_WritePrivateProfileStringEx( pSection, pName, pValue, pINIFile, FALSE );
}

SQLGETOPTION_PROC( LOGICAL, SACK_WritePrivateOptionString )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile )
{
	return SACK_WritePrivateOptionStringEx( odbc, pSection, pName, pValue, pINIFile, FALSE );
}

//------------------------------------------------------------------------

SQLGETOPTION_PROC( int32_t, SACK_WritePrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, int32_t value, CTEXTSTR pINIFile )
{
	TEXTCHAR valbuf[32];
	tnprintf( valbuf, sizeof( valbuf ), WIDE("%") _32fs, value );
	return SACK_WritePrivateProfileString( pSection, pName, valbuf, pINIFile );
}


//------------------------------------------------------------------------
SQLGETOPTION_PROC( LOGICAL, SACK_WriteProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue, CTEXTSTR pINIFile, LOGICAL flush )
{
	return SACK_WritePrivateProfileStringEx( pSection, pName, pValue, DEFAULT_PUBLIC_KEY, flush );
}

SQLGETOPTION_PROC( LOGICAL, SACK_WriteProfileString )( CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue )
{
	return SACK_WritePrivateProfileString( pSection, pName, pValue, DEFAULT_PUBLIC_KEY );
}

SQLGETOPTION_PROC( LOGICAL, SACK_WriteOptionString )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pName, CTEXTSTR pValue )
{
	return SACK_WritePrivateOptionString( odbc, pSection, pName, pValue, DEFAULT_PUBLIC_KEY );
}

//------------------------------------------------------------------------

SQLGETOPTION_PROC( int32_t, SACK_WriteProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, int32_t value )
{
	return SACK_WritePrivateProfileInt( pSection, pName, value, NULL );
}

SQLGETOPTION_PROC( int, SACK_WritePrivateProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer, CTEXTSTR app )
{
	POPTION_TREE_NODE optval;
	optval = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, app, NULL, pSection, pOptname, TRUE, FALSE DBG_SRC );
	if( !optval )
	{
		lprintf( WIDE("Creation of path failed!") );
		return FALSE;
	}
	else
	{
		POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
		return SetOptionBlobValueEx( tree, optval, pBuffer, nBuffer );
	}
	return 0;
}


SQLGETOPTION_PROC( int, SACK_WritePrivateProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer, CTEXTSTR app )
{
	return SACK_WritePrivateProfileBlobOdbc( og.Option, pSection, pOptname, pBuffer, nBuffer, app );
}

SQLGETOPTION_PROC( int, SACK_WriteProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer )
{
   return SACK_WritePrivateProfileBlobOdbc( odbc, pSection, pOptname, pBuffer, nBuffer, NULL );
}


SQLGETOPTION_PROC( int, SACK_WriteProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer )
{
   return SACK_WritePrivateProfileBlobOdbc( og.Option, pSection, pOptname, pBuffer, nBuffer, NULL );
}

//------------------------------------------------------------------------

#if 0
/// this still needs a way to communicate the time from and time until.
SQLGETOPTION_PROC( int, SACK_WritePrivateProfileExceptionString )( CTEXTSTR pSection
                                                            , CTEXTSTR pName
                                                            , CTEXTSTR pValue
                                                            , CTEXTSTR pINIFile
                                                            , uint32_t from // SQLTIME
                                                            , uint32_t to   // SQLTIME
                                                            , CTEXTSTR pSystemName )
{
	INDEX optval = GetOptionIndexEx( OPTION_ROOT_VALUE, pINIFile, pSection, pName, TRUE DBG_SRC );
	if( optval == INVALID_INDEX )
	{
		lprintf( WIDE("Creating of path failed!") );
		return FALSE;
	}
	else
	{
		//CTEXTSTR result = NULL;
		TEXTCHAR exception[256];
		INDEX system;
		INDEX IDValue = CreateValue( og.Option, optval,pValue );
		system = GetSystemIndex( pSystemName );

		tnprintf( exception, sizeof( exception ), WIDE("insert into option_exception (`apply_from`,`apply_to`,`value_id`,`override_value_id`,`system`) ")
																	  WIDE( "values( \'%04d%02d%02d%02d%02d\', \'%04d%02d%02d%02d%02d\', %")_size_f WIDE(", %")_size_f WIDE(",%" _size_f )
             , wYrFrom, wMoFrom, wDyFrom
             , wHrFrom, wMnFrom,wScFrom
             , wYrTo, wMoTo, wDyTo
             , wHrTo, wMnTo,wScTo
             , optval
              , IDValue
              , system // system
              );
		if( !SQLCommand( og.Option, exception ) )
		{
			CTEXTSTR result = NULL;
			GetSQLResult( &result );
			lprintf( WIDE("Insert exception failed: %s"), result );
		}
		else
		{
			if( system || session )
			{
	            INDEX IDTime = FetchLastInsertID( og.Option, WIDE("option_exception"), WIDE("exception_id") );
				// lookup system name... provide detail record
			}
		}
	}
	return 1;
}
#endif

struct option_interface_tag DefaultInterface =
{
   SACK_GetPrivateProfileString
   , SACK_GetPrivateProfileInt
   , SACK_GetProfileString
   , SACK_GetProfileInt
   , SACK_WritePrivateProfileString
   , SACK_WritePrivateProfileInt
   , SACK_WriteProfileString
   , SACK_WriteProfileInt
   , SACK_GetPrivateProfileStringEx
   , SACK_GetPrivateProfileIntEx
   , SACK_GetProfileStringEx
   , SACK_GetProfileIntEx
   , SACK_WritePrivateProfileStringEx
};

#undef GetOptionInterface
SQLGETOPTION_PROC( POPTION_INTERFACE, GetOptionInterface )( void )
{
   return &DefaultInterface;
}

SQLGETOPTION_PROC( void, DropOptionInterface )( POPTION_INTERFACE interface_drop )
{

}
PRIORITY_UNLOAD( AllocateOptionGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY )
{
   // other data to destroy?	
	DeleteCriticalSec( &og.cs_option );
}
PRIORITY_PRELOAD( AllocateOptionGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( sack_global_option_data );
	InitializeCriticalSec( &og.cs_option );
}

PRIORITY_PRELOAD(RegisterSQLOptionInterface, SQL_PRELOAD_PRIORITY + 1 )
{
   // have a multiple test because of C++ and C playing together with shared global; not threading issue
	if( !og.flags.bRegistered )
	{
		og.flags.bRegistered = 1;
		RegisterInterface( WIDE("SACK_SQL_Options"), (POINTER(CPROC *)(void))GetOptionInterface, (void(CPROC *)(POINTER))DropOptionInterface );
		og.flags.bEnableSystemMapping = 2;
	}
}

// delay reading options until after interface configuration is processed which has option defaults.
PRIORITY_PRELOAD( ReadOptionOptions, NAMESPACE_PRELOAD_PRIORITY + 1 )
{
#ifndef __NO_OPTIONS__
	og.flags.bUseProgramDefault = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/SQL/Options/Options Use Program Name Default" ), 1, TRUE );
	og.flags.bUseSystemDefault = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/SQL/Options/Options Use System Name Default" ), 0, TRUE );
#else
	og.flags.bUseProgramDefault = 1;
	og.flags.bUseSystemDefault = 0;
#endif
}


SQLGETOPTION_PROC( CTEXTSTR, GetSystemID )( void )
{
#ifndef __NO_NETWORK__
	static TEXTCHAR result[12];
	tnprintf( result, 12, WIDE("%")_size_f, GetSystemIndex( GetSystemName() ) );
	return result;
#else
	{
      static TEXTCHAR buf[42];
		SACK_GetPrivateProfileStringExxx( NULL, "SACK/System", "Name", GetSeqGUID(), buf, 42, NULL, TRUE DBG_SRC );
		return buf;
	}
#endif
}

SQLGETOPTION_PROC( void, BeginBatchUpdate )( void )
{
	//   SQLCommand(
   //SQLCommand( og.Option, WIDE( "BEGIN TRANSACTION" ) );
}

SQLGETOPTION_PROC( void, EndBatchUpdate )( void )
{
   //SQLCommand( og.Option, WIDE( "COMMIT" ) );
}

ATEXIT( CommitOptions )
{
	INDEX idx;
	POPTION_TREE tree;
#ifdef DETAILED_LOGGING
	lprintf( WIDE( "Running Option cleanup..." ) );
#endif
	if( sack_global_option_data )
	{
		LIST_FORALL( og.trees, idx, POPTION_TREE, tree )
		{
			if( tree->odbc_writer )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE( "flushing a write on %p" ), tree->odbc_writer );
#endif
				SQLCommit( tree->odbc_writer );
			}
		}
	}
}

static void CloseAllODBC( CTEXTSTR dsn ) {
	INDEX idx;
	LOGICAL new_tracker = FALSE;
	struct option_odbc_tracker *tracker;
	if( !dsn )
		dsn = GetDefaultOptionDatabaseDSN();

	LIST_FORALL( og.odbc_list, idx, struct option_odbc_tracker *, tracker )
	{
		//lprintf( "Check %s(%d) vs %s(%d)", dsn, version, tracker->name, tracker->version );
		if( StrCaseCmp( dsn, tracker->name ) == 0 ) {
			//lprintf( "yes, it matched." );
			break;
		}
	}

	if( tracker ) {
		PODBC odbc;
		PLIST list = NULL;
		LIST_FORALL( tracker->outstanding, idx, PODBC, odbc ) {
			CloseDatabaseEx( odbc, FALSE );
		}
		while( odbc = (PODBC)DequeLink( &tracker->available ) ) {
			CloseDatabaseEx( odbc, FALSE );
			AddLink( &list, odbc );
		}
		LIST_FORALL( list, idx, PODBC, odbc ) {
			EnqueLink( &tracker->available, odbc );
		}
	}
}


static void repairOptionDb( uintptr_t psv, PODBC odbc ) {
	static int fixing = 0;
	CTEXTSTR *results;
	size_t *lengths;
	int cols;
	int n;
	if( fixing ) {
		lprintf( "Already fixing, must have been selecting something?" );
		return;
	}
	fixing = 1;
	//lprintf( "Check:" );
	for( SQLRecordQueryf_v2( odbc, &cols, &results, &lengths, NULL, "pragma integrity_check" );
		results;
		FetchSQLRecord( odbc, &results ) ) {
		for( n = 0; n < cols; n++ ) {
			lprintf( "Rsult:%s", results[n] );
		}
	}

	{
		char newDb[256];
		char newDbFile[256];
		PODBC newOdbc;
		CTEXTSTR *fields;
		int row = 0;
		struct file_system_mounted_interface *mount = NULL;
		PVARTEXT pvtCmd = VarTextCreate();
		char *pDbOrigFile;
		{
			char * pVfs, *pVfsInfo, *pDbFile;
			//size_t nVfs, nVfsInfo, nDbFile;
			//ParseDSN( odbc->info.pDSN, &pVfs, &nVfs, &pVfsInfo, &nVfsInfo, &pDbFile, &nDbFile );
			ParseDSN( odbc->info.pDSN, &pVfs, &pVfsInfo, &pDbFile );
			pDbOrigFile = StrDup( pDbFile );
			snprintf( newDbFile, 256, "%s-r2", pDbFile );
			if( pVfsInfo )
				mount = sack_get_mounted_filesystem( pVfsInfo );
			else
				mount = sack_get_default_mount();
			sack_unlinkEx( 0, newDbFile, mount );
			snprintf( newDb, 256, "%s-r2", odbc->info.pDSN );

		}
		//snprintf( newDb, 256, "%s-r2", odbc->info.pDSN );
		newOdbc = GetOptionODBC( newDb );
		SetOptionDatabaseOption( newOdbc );
		SQLCommand( odbc, "delete from option4_map" );
		SQLCommand( odbc, "delete from option4_name" );
		for( row=0, SQLRecordQueryf_v2( odbc, &cols, &results, &lengths, &fields, "select * from option4_map" );
			results;
			row++, FetchSQLRecord( odbc, &results ) ) {
			if( !row ) {
				vtprintf( pvtCmd, "insert into option4_map(" );
				for( n = 0; n < cols; n++ ) {
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", fields[n] );
				}
				vtprintf( pvtCmd, ")VALUES" );
			}
			vtprintf( pvtCmd, "%s(", row?",":"" );
			for( n = 0; n < cols; n++ ) {
				if( results[n] )
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", EscapeSQLBinary( newOdbc, results[n], lengths[n] ) );
				else
					vtprintf( pvtCmd, "%sNULL", n ? "," : "" );
			}
			vtprintf( pvtCmd, ")" );
			
		}
		{
			PTEXT cmd = VarTextPeek( pvtCmd );
			if( cmd )
				SQLCommand( newOdbc, GetText( cmd ) );
		}
		VarTextEmpty( pvtCmd );


		for( row = 0, SQLRecordQueryf_v2( odbc, &cols, &results, &lengths, &fields, "select * from option4_name" );
			results;
			row++, FetchSQLRecord( odbc, &results ) ) {
			if( !row ) {
				vtprintf( pvtCmd, "insert into option4_name(" );
				for( n = 0; n < cols; n++ ) {
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", fields[n] );
				}
				vtprintf( pvtCmd, ")VALUES" );
			}
			vtprintf( pvtCmd, "%s(", row ? "," : "" );
			for( n = 0; n < cols; n++ ) {
				if( results[n] )
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", EscapeSQLBinary( newOdbc, results[n], lengths[n] ) );
				else
					vtprintf( pvtCmd, "%sNULL", n ? "," : "" );
			}
			vtprintf( pvtCmd, ")" );

		}
		{
			PTEXT cmd = VarTextPeek( pvtCmd );
			if( cmd )
				SQLCommand( newOdbc, GetText( cmd ) );
		}
		VarTextEmpty( pvtCmd );


		for( row = 0, SQLRecordQueryf_v2( odbc, &cols, &results, &lengths, &fields, "select * from option4_values" );
			results;
			row++, FetchSQLRecord( odbc, &results ) ) {
			if( !row ) {
				vtprintf( pvtCmd, "insert into option4_values(" );
				for( n = 0; n < cols; n++ ) {
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", fields[n] );
				}
				vtprintf( pvtCmd, ")VALUES" );
			}
			vtprintf( pvtCmd, "%s(", row ? "," : "" );
			for( n = 0; n < cols; n++ ) {
				if( results[n] )
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", EscapeSQLBinary( newOdbc, results[n], lengths[n] ) );
				else
					vtprintf( pvtCmd, "%sNULL", n ? "," : "" );
			}
			vtprintf( pvtCmd, ")" );
		}
		{
			PTEXT cmd = VarTextPeek( pvtCmd );
			if( cmd )
				SQLCommand( newOdbc, GetText( cmd ) );
		}
		VarTextEmpty( pvtCmd );

		for( row=0, SQLRecordQueryf_v2( odbc, &cols, &results, &lengths, &fields, "select * from option4_blobs" );
			results;
			row++, FetchSQLRecord( odbc, &results ) ) {
			if( !row ) {
				vtprintf( pvtCmd, "insert into option4_blobs(" );
				for( n = 0; n < cols; n++ ) {
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", fields[n] );
				}
				vtprintf( pvtCmd, ")VALUES" );
			}
			vtprintf( pvtCmd, "%s(", row ? "," : "" );
			for( n = 0; n < cols; n++ ) {
				if( results[n] )
					vtprintf( pvtCmd, "%s'%s'", n ? "," : "", EscapeSQLBinary( newOdbc, results[n], lengths[n] ) );
				else
					vtprintf( pvtCmd, "%sNULL", n ? "," : "" );
			}
			vtprintf( pvtCmd, ")" );
		}
		{
			PTEXT cmd = VarTextPeek( pvtCmd );
			if( cmd )
				SQLCommand( newOdbc, GetText( cmd ) );
		}
		VarTextDestroy( &pvtCmd );
		CloseDatabaseEx( newOdbc, FALSE );
		CloseAllODBC( odbc->info.pDSN );
		sack_unlinkEx( 0, pDbOrigFile, mount );
		sack_renameEx( pDbOrigFile, newDbFile, mount );
		Release( pDbOrigFile );
	}
	fixing = 0;
}

SQLGETOPTION_PROC( CTEXTSTR, GetDefaultOptionDatabaseDSN )( void )
{
	return global_sqlstub_data->OptionDb.info.pDSN;
}

PODBC GetOptionODBCEx( CTEXTSTR dsn  DBG_PASS )
{
	INDEX idx;
	LOGICAL new_tracker = FALSE;
	struct option_odbc_tracker *tracker;
	if( !dsn )
		dsn = GetDefaultOptionDatabaseDSN();

	LIST_FORALL( og.odbc_list, idx, struct option_odbc_tracker *, tracker )
	{
		//lprintf( "Check %s(%d) vs %s(%d)", dsn, version, tracker->name, tracker->version );
		if( StrCaseCmp( dsn, tracker->name ) == 0 )
		{
			//lprintf( "yes, it matched." );
			break;
		}
	}
	if( !tracker )
	{
		//lprintf( "Needed a new tracker." );
		tracker = New( struct option_odbc_tracker );
		tracker->name = StrDup( dsn );
		tracker->shared_option_tree = NULL;
		tracker->available = CreateLinkQueue();
		tracker->outstanding = NULL;
		AddLink( &og.odbc_list, tracker );
		new_tracker = TRUE;
	}
	{
		PODBC odbc = (PODBC)DequeLink( &tracker->available );
		if( !odbc )
		{
#ifdef DETAILED_LOGGING
			lprintf( "none available, create new connection." );
#endif
			odbc = ConnectToDatabaseExx( tracker->name, TRUE DBG_RELAY );
			SetSQLCorruptionHandler( odbc, repairOptionDb, (uintptr_t)odbc );
			SQLCommand( odbc, "pragma foreign_keys=on" );

			{
				INDEX idx;
				CTEXTSTR cmd;
				CTEXTSTR result;
				LIST_FORALL( global_sqlstub_data->option_database_init, idx, CTEXTSTR, cmd ) {
					SQLQueryf( odbc, &result, cmd );
					//if( result )
					//	lprintf( WIDE( " %s" ), result );
					SQLEndQuery( odbc );
				}
			}

			//SetSQLAutoClose( odbc, TRUE );
			if( !tracker->shared_option_tree )
			{
				POPTION_TREE option = GetOptionTreeExxx( odbc, NULL DBG_RELAY );
				//lprintf( "setting tracker shared to %p", option->option_tree );
				tracker->shared_option_tree =  option->option_tree;
			}
			else
			{
#ifdef DETAILED_LOGGING
				lprintf( "get the tree...." );
#endif
				GetOptionTreeExxx( odbc, tracker->shared_option_tree DBG_RELAY );
			}
			// only if it's a the first connection should we leave created as false.
			if( !new_tracker )
			{
				POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
				tree->flags.bCreated = 1;
			}
			SetOptionDatabaseOption( odbc );
		}
		AddLink( &tracker->outstanding, odbc );
#ifdef DETAILED_LOGGING
		_lprintf( DBG_RELAY )( "%p result...", odbc );
#endif
		return odbc;
	}
}

#undef GetOptionODBC
PODBC GetOptionODBC( CTEXTSTR dsn )
{
	return GetOptionODBCEx( dsn DBG_SRC );
}

void DropOptionODBCEx( PODBC odbc DBG_PASS )
{
	INDEX idx;
	struct option_odbc_tracker *tracker;
	//_lprintf( DBG_RELAY )( "%p Drop...", odbc );
	LIST_FORALL( og.odbc_list, idx, struct option_odbc_tracker *, tracker )
	{
		INDEX idx2;
		PODBC connection;
		LIST_FORALL( tracker->outstanding, idx2, PODBC, connection )
		{
			if( connection == odbc )
			{
				SetLink( &tracker->outstanding, idx2, NULL );
				EnqueLink( &tracker->available, odbc );
				break;
			}
		}
		if( !connection )
		{
			lprintf( WIDE("Failed to find the thing to drop.") );
		}
		if( connection )
			break;
	}
}

#undef DropOptionODBC
void DropOptionODBC( PODBC odbc )
{
	DropOptionODBCEx( odbc DBG_SRC );
}

PRIORITY_PRELOAD( CommitOptionsLoad, 150 )
{
	INDEX idx;
	POPTION_TREE tree;
#ifdef DETAILED_LOGGING
	lprintf( WIDE( "Running Option cleanup..." ) );
#endif
	LIST_FORALL( og.trees, idx, POPTION_TREE, tree )
	{
		if( tree->odbc_writer )
		{
#ifdef DETAILED_LOGGING
			lprintf( WIDE( "flushing a write on %p" ), tree->odbc_writer );
#endif
			SQLCommit( tree->odbc_writer );
		}
	}
}


void FindOptions( PODBC odbc, PLIST *result_list, CTEXTSTR name )
{
	POPTION_TREE tree;
	if( !odbc )
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );

	New4FindOptions( tree, result_list, name );
}


SACK_OPTION_NAMESPACE_END
