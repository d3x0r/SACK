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
#include <system.h>
#include <network.h>
#include <controls.h> // INI prompt
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

/*
 Dump Option table...
 SELECT oname2.name,oname.name,optionvalues.string,omap.*
 FROM `optionmap` as omap
 join optionname as oname on omap.name_id=oname.name_id
 left join optionvalues on omap.value_id=optionvalues.value_id
 left join optionmap as omap2 on omap2.node_id=omap.parent_node_id
 left join optionname as oname2 on omap2.name_id=oname2.name_id
*/
SQL_NAMESPACE
extern GLOBAL *global_sqlstub_data;
SQL_NAMESPACE_END


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

SQLGETOPTION_PROC( POPTION_TREE_NODE, GetOptionIndexEx )( POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate DBG_PASS );
#define GetOptionIndex(p,f,b,v) GetOptionIndexEx( p,f,b,v,FALSE DBG_SRC )
SQLGETOPTION_PROC( void, CreateOptionDatabaseEx )( PODBC odbc, POPTION_TREE tree );

POPTION_TREE GetOptionTreeExx( PODBC odbc DBG_PASS )
{
	POPTION_TREE tree = NULL;
	INDEX idx;
	if( !odbc )
	{
		if( !og.flags.bInited )
			InitMachine();
		//lprintf( "Ran dead init and get %p", og.Option );
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN(), global_sqlstub_data->OptionVersion );
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
		//lprintf( WIDE( "need a new option tree for %p" ), odbc );
		tree = New( struct sack_option_tree_family );
		tree->root = New( OPTION_TREE_NODE );
		MemSet( tree->root, 0, sizeof( struct sack_option_tree_family_node ) );
		tree->root->name_id = INVALID_INDEX;
		tree->root->value_id = INVALID_INDEX;

		tree->root->guid = GuidZero();
		tree->root->name_guid = NULL;
		tree->root->value_guid = NULL;

		tree->root->value = NULL;

		// if it's a new optiontree, pass it to create...
		tree->option_tree = CreateFamilyTree( (int(CPROC*)(PTRSZVAL,PTRSZVAL))StrCaseCmp, NULL );
		tree->odbc = odbc;
		tree->odbc_writer = NULL;
		// default to the old version... allow other code to select new version.
		if( odbc != &global_sqlstub_data->OptionDb )
		{
			tree->flags.bNewVersion = GetOptionTreeEx( &global_sqlstub_data->OptionDb )->flags.bNewVersion;
			tree->flags.bVersion4 = GetOptionTreeEx( &global_sqlstub_data->OptionDb )->flags.bVersion4;
		}
		else
		{
			tree->flags.bNewVersion = 0;
			tree->flags.bVersion4 = 0;
		}
		tree->flags.bCreated = 0;
		AddLink( &og.trees, tree );
	}
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
		CreateOptionDatabaseEx( odbc, tree = GetOptionTreeEx( odbc ) ); // make sure the option tables exist.
		return tree;
	}
	else
		return GetOptionTreeEx( og.Option );
}

PFAMILYTREE* GetOptionTree( PODBC odbc )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( tree )
		return &tree->option_tree;
	return NULL;
}



SQLGETOPTION_PROC( void, CreateOptionDatabaseEx )( PODBC odbc, POPTION_TREE tree )
{
	if( !global_sqlstub_data->flags.bLogOptionConnection )
		SetSQLLoggingDisable( tree->odbc, TRUE );
	{
		PTABLE table;
		if( !tree->flags.bCreated )
		{
			if( tree->flags.bNewVersion )
			{
				table = GetFieldsInSQLEx( option2_exception, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option2_map, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option2_name, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option2_values, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MATCH );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option2_blobs, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
			}
			else if( tree->flags.bVersion4 )
			{
				table = GetFieldsInSQLEx( option4_exception, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option4_map, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option4_name, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option4_values, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MATCH );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option4_blobs, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				{
               // this needs a self-looped root to satisfy constraints.
               CTEXTSTR result;
					if( !SQLQueryf( tree->odbc, &result, "select parent_option_id from option4_map where option_id='00000000-0000-0000-0000-000000000000'" )
						|| !result )
					{
						SQLCommandf( tree->odbc, "insert into option4_map (option_id,parent_option_id,name_id)values('00000000-0000-0000-0000-000000000000','00000000-0000-0000-0000-000000000000','%s' )"
									  , New4ReadOptionNameTable(tree,WIDE("."),OPTION4_NAME,WIDE( "name_id" ),WIDE( "name" ),1 DBG_SRC)
									  );
					}
				}
			}
         else
			{
				table = GetFieldsInSQLEx( option_exception, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option_map, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option_name, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( option_values, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MATCH );
				DestroySQLTable( table );
				table = GetFieldsInSQLEx( systems, FALSE DBG_SRC );
				CheckODBCTable( tree->odbc, table, CTO_MERGE );
				DestroySQLTable( table );
			}

         //SQLCommit( odbc );
         //SQLCommand( odbc, WIDE( "COMMIT" ) );
			tree->flags.bCreated = 1;
		}
	}
}

void SetOptionDatabaseOption( PODBC odbc, int bNewVersion )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( tree )
	{
		tree->flags.bCreated = FALSE;
		if( bNewVersion == 2 )
		{
			tree->flags.bVersion4 = 1;
			tree->flags.bNewVersion = 0; 
		}
		else if( bNewVersion == 1 )
		{
			tree->flags.bVersion4 = 0;
			tree->flags.bNewVersion = 1;
		}

		//lprintf( "Set tree %p to newversion %d", tree, bNewVersion );
		if( global_sqlstub_data->flags.bInited )
			CreateOptionDatabaseEx( odbc, tree );
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
		//option->odbc_writer = SQLGetODBC( option->odbc?option->odbc->info.pDSN:global_sqlstub_data->Primary.info.pDSN );
		if( option->odbc_writer )
		{
			if( !global_sqlstub_data->flags.bLogOptionConnection )
				SetSQLLoggingDisable( option->odbc_writer, TRUE );
			SetSQLThreadProtect( option->odbc_writer, TRUE );
			SetSQLAutoTransact( option->odbc_writer, TRUE );
			//SetSQLAutoClose( option->odbc_writer, TRUE );
		}
	}
}

static LOGICAL CreateOptionDatabase( void )
{
	if( !og.Option )
	{
		if( strlen( global_sqlstub_data->OptionDb.info.pDSN ) == 0 )
			global_sqlstub_data->OptionDb.info.pDSN = WIDE( "@/option.db" );
/*
		{
			if( !og.Option )
			{
#ifdef DETAILED_LOGGING
				lprintf( WIDE( "Option global database gone - connect to %s" ), global_sqlstub_data->OptionDb.info.pDSN );
#endif
				og.Option = GetOptionODBC( global_sqlstub_data->OptionDb.info.pDSN, global_sqlstub_data->OptionVersion );
            DropODBC( og.Option );
				//SetSQLAutoClose( og.Option, TRUE );
			}
 		}
*/
      return TRUE;
	}
	return FALSE;

}

void InitMachine( void )
{
	if( !og.flags.bInited )
	{
      /*
      POPTION_TREE tree;
#if 0
		_32 timeout;
#endif

		if( CreateOptionDatabase() )
		{
			tree = GetOptionTreeEx( og.Option );
			//lprintf( "Setup internal primary database version..." );
			tree->flags.bNewVersion = GetOptionTreeEx( &global_sqlstub_data->OptionDb )->flags.bNewVersion;
			tree->flags.bVersion4 = GetOptionTreeEx( &global_sqlstub_data->OptionDb )->flags.bVersion4;
		}
		else
			tree = GetOptionTreeEx( og.Option );


		CreateOptionDatabaseEx( og.Option, tree );
		// acutlaly init should be called always ....
		if( !IsSQLOpen(og.Option) )
		{
			lprintf( WIDE("Get Option init failed... no database...") );
			return;
		}
		// og.system = GetSYstemID( WIDE("SYSTEMNAME") );
		og.SystemID = 0;  // default - any system...
      */
		og.flags.bInited = 1;
		//og.SystemID = SQLReadNameTable( og.Option, GetSystemName(), WIDE("systems"), WIDE("system_id")  );
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
	snprintf( query, sizeof( query ), WIDE("select %s from %s where %s like '%s'")
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
		snprintf( query, sizeof( query ), WIDE("insert into %s (%s) values( '%s' )"), table, namecol, newval );
		OpenWriterEx( tree DBG_RELAY );
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
		AddBinaryNode( GetTableCache(tree->odbc,table), (POINTER)((PTRSZVAL)(IDName+1))
						 , (PTRSZVAL)SaveText( name ) );
	}
	return IDName;
}

//---------------------------------------------------------------------------

INDEX IndexCreateFromText( CTEXTSTR string )
{
   return (INDEX)IntCreateFromText( string );
}

//---------------------------------------------------------------------------


//#define OPTION_ROOT_VALUE INVALID_INDEX
#define OPTION_ROOT_VALUE 0

static POPTION_TREE_NODE GetOptionIndexExxx( PODBC odbc, POPTION_TREE_NODE parent
														 , const TEXTCHAR *file
														 , const TEXTCHAR *pBranch
														 , const TEXTCHAR *pValue
														 , int bCreate, int bIKnowItDoesntExist DBG_PASS )
//#define GetOptionIndex( f,b,v ) GetOptionIndexEx( OPTION_ROOT_VALUE, f, b, v, FALSE )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( !parent )
		parent = tree->root;

	InitMachine();

	if( tree->flags.bVersion4 )
	{
		//lprintf( "... %p %s %s %ws", parent, file, pBranch, pValue );
		return New4GetOptionIndexExxx( odbc, parent, file, pBranch, pValue, bCreate, bIKnowItDoesntExist DBG_RELAY );
	}
	else if( tree->flags.bNewVersion )
	{
		//lprintf( "... %p %s %s %ws", parent, file, pBranch, pValue );
		return NewGetOptionIndexExxx( odbc, parent, file, pBranch, pValue, bCreate, bIKnowItDoesntExist DBG_RELAY );
	}
	else 
	{
		const TEXTCHAR **start = NULL;
		TEXTCHAR namebuf[256];
		TEXTCHAR query[256];
		const TEXTCHAR *p;
		static const TEXTCHAR *_program = NULL;
		const TEXTCHAR *program = NULL;
		static const TEXTCHAR *_system = NULL;
		const TEXTCHAR *system = NULL;
		CTEXTSTR *result = NULL;
		INDEX ID;
		//, IDName; // Name to lookup
		if( og.flags.bUseProgramDefault )
		{
			if( !_program )
				_program = GetProgramName();
			program = _program;
		}
		if( og.flags.bUseSystemDefault )
		{
			if( !_system )
				_system = GetSystemName();
			system = _system;
		}

		// resets the search/browse cursor... not empty...
		FamilyTreeReset( &tree->option_tree );
		while( system || program || file || pBranch || pValue || start )
		{
#ifdef DETAILED_LOGGING
			lprintf( WIDE("Top of option loop") );
#endif
			if( !start || !(*start) )
			{
				if( program )
					start = &program;
				if( !start && system )
					start = &system;

				if( !start && file )
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Token parsing at FILE") );
#endif
					start = &file;
				}
				if( !start && pBranch )
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Token parsing at branch") );
#endif
					start = &pBranch;
				}
				if( !start && pValue )
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Token parsing at value") );
#endif
					start = &pValue;
				}
				if( !start || !(*start) ) continue;
			}
			p = pathchr( *start );
			if( p )
			{
				if( p-(*start) > 0 )
				{
					MemCpy( namebuf, (*start), p - (*start) );
					namebuf[p-(*start)] = 0;
				}
				else
				{
					(*start) = p + 1;
					continue;
				}
				(*start) = p + 1;
			}
			else
			{
				StrCpyEx( namebuf, (*start), sizeof( namebuf )-1 );
				(*start) = NULL;
				start = NULL;
			}

			// remove references of 'here' during parsing.
			if( strcmp( namebuf, WIDE( "." ) ) == 0 )
				continue;
#ifdef DETAILED_LOGGING
			lprintf( WIDE("First - check local cache for %s"), namebuf );
#endif
			{
				// return is UserData, assume I DO store this as an index.
				POPTION_TREE_NODE node = (POPTION_TREE_NODE)FamilyTreeFindChild( tree->option_tree, (PTRSZVAL)namebuf );
				if( node )
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("Which is found, and new parent ID result...%d"), node->name_id );
#endif
					parent = node;
					continue;
				}
			}
			{
				INDEX IDName = ReadOptionNameTable(tree,namebuf,WIDE( "option_name" ),WIDE( "name_id" ),WIDE( "name" ),1 DBG_RELAY);

				if( !bIKnowItDoesntExist )
				{
					PushSQLQueryEx(odbc);
					snprintf( query, sizeof( query )
							  , WIDE("select node_id,value_id from option_map where parent_node_id=%ld and name_id=%d")
							  , parent->id
							  , IDName );
				}

				if( bIKnowItDoesntExist || !SQLRecordQuery( odbc, query, NULL, &result, NULL ) || !result )
				{
					if( bCreate )
					{
						// this is the only place where ID must be set explicit...
						// otherwise our root node creation failes if said root is gone.
						//lprintf( "New entry... create it..." );
						snprintf( query, sizeof( query ), WIDE("Insert into option_map(`parent_node_id`,`name_id`) values (%ld,%lu)"), parent->id, IDName );
						OpenWriterEx( tree DBG_RELAY );
						if( SQLCommand( tree->odbc_writer, query ) )
						{
							ID = FetchLastInsertID( tree->odbc_writer, WIDE("option_map"), WIDE("node_id") );
						}
						else
						{
							CTEXTSTR error;
							FetchSQLError( tree->odbc_writer, &error );
#ifdef DETAILED_LOGGING
							lprintf( WIDE("Error inserting option: %s"), error );
#endif
						}
#ifdef DETAILED_LOGGING
						lprintf( WIDE("Created option root...") );
#endif
						//lprintf( WIDE("Adding new option to family tree... ") );
						{
							POPTION_TREE_NODE new_node = New( struct sack_option_tree_family_node );
                     MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
							new_node->id = ID;
							new_node->value_id = INVALID_INDEX;
							new_node->name_id = IDName;
							new_node->value = NULL;
							new_node->node = FamilyTreeAddChild( &tree->option_tree, new_node, (PTRSZVAL)SaveText( namebuf ) );
							parent = new_node;
						}
						PopODBCEx( odbc );
						continue; // get out of this loop, continue outer.
					}
#ifdef DETAILED_LOGGING
					_lprintf(DBG_RELAY)( WIDE("Option tree corrupt.  No option node_id=%ld"), ID );
#endif
					if( !bIKnowItDoesntExist )
						PopODBCEx( odbc );
					return NULL;
				}
				else
				{
#ifdef DETAILED_LOGGING
					lprintf( WIDE("found the node which has the name specified...") );
#endif
					// might as well fetch the value ID associated here alsos.
					//if( result[1] )
					//	value = atoi( result[1] );
					//else
					//   value = INVALID_INDEX;
					//sscanf( result, WIDE("%lu"), &parent );
					{
						POPTION_TREE_NODE new_node = New( struct sack_option_tree_family_node );
						MemSet( new_node, 0, sizeof( struct sack_option_tree_family_node ) );
						new_node->id = IndexCreateFromText( result[0] );
						new_node->value = NULL;
						new_node->node = FamilyTreeAddChild( &tree->option_tree, new_node, (PTRSZVAL)SaveText( namebuf ) );
						parent = new_node;
					}
				}
				if( !bIKnowItDoesntExist )
					PopODBCEx( odbc );
			}
		}
	}
	return parent;
}

POPTION_TREE_NODE GetOptionIndexExx( PODBC odbc, POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate DBG_PASS )
{
   return GetOptionIndexExxx( odbc, parent, file, pBranch, pValue, bCreate, FALSE DBG_RELAY );
}

POPTION_TREE_NODE GetOptionIndexEx( POPTION_TREE_NODE parent, const TEXTCHAR *file, const TEXTCHAR *pBranch, const TEXTCHAR *pValue, int bCreate DBG_PASS )
{
   return GetOptionIndexExxx( og.Option, parent, file, pBranch, pValue, bCreate, FALSE DBG_RELAY );
}
//------------------------------------------------------------------------

INDEX GetSystemIndex( CTEXTSTR pSystemName )
{
   if( pSystemName )
      return ReadNameTable( pSystemName, WIDE("systems"), WIDE("system_id") );
	else
	{
		if( !og.SystemID )
			og.SystemID = SQLReadNameTable( og.Option, pSystemName, WIDE("systems"), WIDE("system_id")  );

		return og.SystemID;
	}
}

//------------------------------------------------------------------------

POPTION_TREE_NODE GetOptionValueIndexEx( PODBC odbc, POPTION_TREE_NODE ID )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( tree->flags.bNewVersion || tree->flags.bVersion4 )
	{
		return ID;
	}

	{
		TEXTCHAR query[256];
		CTEXTSTR result = NULL;
		if( ID )
		{
			snprintf( query, sizeof( query ), WIDE("select value_id from option_map where node_id=%ld"), ID->id );
			//lprintf( WIDE("push get value index.") );
			PushSQLQueryEx( odbc );
			if( !SQLQuery( odbc, query, &result )
				|| !result )
			{
				lprintf( WIDE("Option tree corrupt.  No option node_id=%ld") );
				return NULL;
			}
			//lprintf( WIDE("okay and then we pop!?") );
			ID->value_id = IndexCreateFromText( result );
			PopODBCEx( odbc);
			//lprintf( WIDE("and then by the time done...") );
		}
		return ID;
	}
}


POPTION_TREE_NODE GetOptionValueIndex( POPTION_TREE_NODE ID )
{
   return GetOptionValueIndexEx( og.Option, ID );
}

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
		snprintf( query, sizeof( query )
			  , WIDE( "replace into " )OPTION4_VALUES WIDE( " (option_id,`string`) values ('%s',%s)" )
				  , iNewOption->guid, tmp = EscapeSQLBinaryOpt( odbc, results[0], strlen( results[0] ), TRUE ) );
		Release( tmp );
		SQLEndQuery( odbc );
		SQLCommand( odbc, query );
	}

	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE( "select `binary` from " )OPTION4_BLOBS WIDE( " where option_id='%s'" ), iOriginalOption->guid );

	if( results && results[0] )
	{
		snprintf( query, sizeof( query )
				  , WIDE( "replace into " )OPTION4_BLOBS WIDE( " (option_id,`binary`) values ('%s',%s)" )
				  , iNewOption->guid, tmp = EscapeSQLBinaryOpt( odbc, results[0], strlen( results[0] ), TRUE ) );
		Release( tmp );
		SQLEndQuery( odbc );
		SQLCommand( odbc, query );
	}
	PopODBCEx( odbc );
	return iNewOption;
}

POPTION_TREE_NODE NewDuplicateValue( PODBC odbc, POPTION_TREE_NODE iOriginalOption, POPTION_TREE_NODE iNewOption )
{
	TEXTCHAR query[256];
	CTEXTSTR *results;
	TEXTSTR tmp;
	PushSQLQueryEx( odbc );
	// my nested parent may have a select state in a condition that I think it's mine.
	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE( "select `string` from " )OPTION_VALUES WIDE( " where option_id=%ld" ), iOriginalOption->guid );

	if( results && results[0] )
	{
		snprintf( query, sizeof( query )
			  , WIDE( "replace into " )OPTION_VALUES WIDE( " (option_id,`string`) values (%ld,%s)" )
				  , iNewOption->id, tmp = EscapeSQLBinaryOpt( odbc, results[0], strlen( results[0] ), TRUE ) );
		Release( tmp );
		SQLEndQuery( odbc );
		SQLCommand( odbc, query );
	}

	SQLRecordQueryf( odbc, NULL, &results, NULL, WIDE( "select `binary` from " )OPTION_BLOBS WIDE( " where option_id=%ld" ), iOriginalOption->guid );

	if( results && results[0] )
	{
		snprintf( query, sizeof( query )
				  , WIDE( "replace into " )OPTION_BLOBS WIDE( " (option_id,`binary`) values (%ld,%s)" )
				  , iNewOption->id, tmp = EscapeSQLBinaryOpt( odbc, results[0], strlen( results[0] ), TRUE ) );
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
	POPTION_TREE tree = GetOptionTreeEx( og.Option );
	if( tree->flags.bVersion4 )
	{
		return New4DuplicateValue( og.Option, iOriginalValue, iNewValue );
	}
	else if( tree->flags.bNewVersion )
	{
		return NewDuplicateValue( og.Option, iOriginalValue, iNewValue );
	}
	else
	{
		TEXTCHAR query[256];
		snprintf( query, sizeof( query )
				  , WIDE( "insert into option_values select 0,`string`,`binary` from option_values where value_id=%ld" )
				  , iOriginalValue->value_id );
		OpenWriter( tree );
		SQLCommand( tree->odbc_writer, query );
		iNewValue->value_id = FetchLastInsertID(tree->odbc_writer, NULL,NULL);
		return iNewValue;
	}
}

//------------------------------------------------------------------------

size_t GetOptionStringValueEx( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len DBG_PASS )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( tree->flags.bVersion4 )
	{
		//_lprintf( DBG_RELAY )( "GetOptionString for %p", odbc );
		return New4GetOptionStringValue( odbc, optval, buffer, len DBG_RELAY );
	}
	else if( tree->flags.bNewVersion )
	{
		//_lprintf( DBG_RELAY )( "GetOptionString for %p", odbc );
		return NewGetOptionStringValue( odbc, optval, buffer, len DBG_RELAY );
	}
	else
	{
		TEXTCHAR query[256];
		CTEXTSTR result = NULL;
		int last_was_session, last_was_system;
		POPTION_TREE_NODE _optval;
		size_t result_len = 0;
		len--;

		snprintf( query, sizeof( query ), WIDE( "select override_value_id from option_exception " )
				WIDE( "where ( apply_from<=now() or apply_from=0 )" )
				WIDE( "and ( apply_until>now() or apply_until=0 )" )
				WIDE( "and ( system_id=%d or system_id=0 )" )
				WIDE( "and value_id=%d " )
			   , og.SystemID
			   , optval );
		last_was_session = 0;
		last_was_system = 0;
		PushSQLQueryEx( odbc );
		for( SQLQuery( odbc, query, &result ); result; FetchSQLResult( odbc, &result ) )
		{
			_optval = optval;
			if( (!optval) )
				optval = _optval;
         optval->value_id = IndexCreateFromText( result );
		}
		snprintf( query, sizeof( query ), WIDE("select string from option_values where value_id=%ld"), optval->value_id );
		// have to push here, the result of the prior is kept outstanding
		// if this was not pushed, the prior result would evaporate.
		PushSQLQueryEx( odbc );
		buffer[0] = 0;
		//lprintf( WIDE("do query for value string...") );
		if( SQLQuery( odbc, query, &result ) )
		{
			//lprintf( WIDE(" query succeeded....") );
			if( result )
			{
				result_len = StrLen( result );
				StrCpyEx( buffer, result, len );
				optval->value = StrDup( result );
			}
			else
			{
				buffer[0] = 0;
            result_len = (size_t)-1;
			}
		}
		PopODBCEx( odbc );
		PopODBCEx( odbc );
		return result_len;
	}
}

size_t GetOptionStringValue( POPTION_TREE_NODE optval, TEXTCHAR *buffer, size_t len )
{
	return GetOptionStringValueEx( og.Option, optval, buffer, len DBG_SRC );
}

int GetOptionBlobValueOdbc( PODBC odbc, POPTION_TREE_NODE optval, TEXTCHAR **buffer, size_t *len )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	if( tree->flags.bNewVersion )
	{
		CTEXTSTR *result = NULL;
		size_t tmplen;
		if( !len )
			len = &tmplen;
		PushSQLQueryEx( odbc );
		if( SQLRecordQueryf( odbc, NULL, &result, NULL
								 , WIDE("select `binary`,length(`binary`) from ")OPTION_BLOBS WIDE(" where option_id=%lu")
								 , optval->id ) )
		{
			int success = FALSE;
			//lprintf( WIDE(" query succeeded....") );
			if( buffer && result && result[0] && result[1] )
			{
				success = TRUE;
				(*buffer) = NewArray( TEXTCHAR, (*len)=(size_t)IntCreateFromText( result[1] ));
				MemCpy( (*buffer), result[0], (PTRSZVAL)(*len) );
			}
			PopODBCEx( odbc );
			return success;
		}
	}
	else if( tree->flags.bVersion4 )
	{
		CTEXTSTR *result = NULL;
		size_t tmplen;
		if( !len )
			len = &tmplen;
		PushSQLQueryEx( odbc );
		if( SQLRecordQueryf( odbc, NULL, &result, NULL
								 , WIDE("select `binary`,length(`binary`) from ")OPTION4_BLOBS WIDE(" where option_id=%lu")
								 , optval->id ) )
		{
			int success = FALSE;
			//lprintf( WIDE(" query succeeded....") );
			if( buffer && result && result[0] && result[1] )
			{
				success = TRUE;
				(*buffer) = NewArray( TEXTCHAR, (*len)=(size_t)IntCreateFromText( result[1] ));
				MemCpy( (*buffer), result[0], (PTRSZVAL)(*len) );
			}
			PopODBCEx( odbc );
			return success;
		}
	}
	else
	{
		CTEXTSTR *result = NULL;
		size_t tmplen;
		if( !len )
			len = &tmplen;
		PushSQLQueryEx( odbc );
#ifdef DETAILED_LOGGING
		lprintf( WIDE("do query for value string...") );
#endif
		if( SQLRecordQueryf( odbc, NULL, &result, NULL
								 , WIDE("select `binary`,length(`binary`) from option_values where value_id=%ld")
								 , optval->value_id ) )
		{
			int success = FALSE;
			//lprintf( WIDE(" query succeeded....") );
			if( buffer && result && result[0] && result[1] )
			{
				success = TRUE;
				(*buffer) = NewArray( TEXTCHAR, (*len)=(size_t)IntCreateFromText( result[1] ));
				MemCpy( (*buffer), result[0], (PTRSZVAL)(*len) );
			}
			PopODBCEx( odbc );
			return success;
		}
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
	TEXTCHAR value[3];
	if( GetOptionStringValueEx( og.Option, optval, value, sizeof( value ) DBG_RELAY ) != INVALID_INDEX )
	{
		if( value[0] == 'y' || value[0] == 'Y' || ( value[0] == 't' || value[0] == 'T' ) )
			*result_value = 1;
		else
			*result_value = atoi( value );
		return TRUE;
	}
	return FALSE;
}

//------------------------------------------------------------------------

static LOGICAL CreateValue( POPTION_TREE tree, POPTION_TREE_NODE iOption, CTEXTSTR pValue )
{
	if( tree->flags.bNewVersion )
	{
		return NewCreateValue( tree, iOption, pValue );
	}
	else if( tree->flags.bVersion4 )
	{
		return New4CreateValue( tree, iOption, pValue );
	}
	else
	{
		TEXTCHAR insert[256];
		CTEXTSTR result=NULL;
		LOGICAL retval = TRUE;
		TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc, pValue, StrLen( pValue ), TRUE );
		if( pValue == NULL )
			snprintf( insert, sizeof( insert ), WIDE("insert into option_values (`binary` ) values ('')")
					  );
		else
			snprintf( insert, sizeof( insert ), WIDE("insert into option_values (`string` ) values (%s)")
					  , pValue?newval:WIDE( "NULL" )
					  );
		if( SQLCommand( tree->odbc_writer, insert ) )
		{
			iOption->value_id = FetchLastInsertID( tree->odbc_writer, WIDE("option_values"), WIDE("value_id") );
		}
		else
		{
			FetchSQLError( tree->odbc_writer, &result );
			lprintf( WIDE("Insert value failed: %s"), result );
			retval = FALSE;
		}
		Release( newval );
		return retval;
	}
}


//------------------------------------------------------------------------
// result with option value ID
LOGICAL SetOptionValueEx( POPTION_TREE tree, POPTION_TREE_NODE optval )
{
	if( tree->flags.bNewVersion )
	{
		return TRUE;
	}
	else if( tree->flags.bVersion4 )
	{
		return TRUE;
	}
	else
	{
		TEXTCHAR update[128];
		CTEXTSTR result = NULL;
		// should escape quotes passed in....
		snprintf( update, sizeof( update ), WIDE("update option_map set value_id=%ld where node_id=%ld"), optval->value_id, optval->id );
		if( !SQLCommand( tree->odbc_writer, update ) )
		{
			FetchSQLResult( tree->odbc_writer, &result );
			lprintf( WIDE("Update value failed: %s"), result );
			return FALSE;
		}
		return TRUE;
	}
}

//------------------------------------------------------------------------
// result with option value ID
LOGICAL SetOptionStringValue( POPTION_TREE tree, POPTION_TREE_NODE optval, CTEXTSTR pValue )
{
	// update value.
	TEXTCHAR update[256];
	//TEXTCHAR value[256]; // SQL friendly string...
	CTEXTSTR result = NULL;
	LOGICAL retval = TRUE;
	TEXTSTR newval;
	EnterCriticalSec( &og.cs_option );

	if( !pValue )
	{
		if( optval->value_id == INVALID_INDEX )
		{
			if( tree->flags.bNewVersion )
				optval->value_id = optval->id;
			GetOptionValueIndexEx( tree->odbc, optval );
         //lprintf( "option id was invalid, is now %d", optval->value_id );
		}

		if( ( optval->value_id != INVALID_INDEX ) || ( optval->value_guid != NULL ) )
		{
			snprintf( update, sizeof( update )
					  , tree->flags.bVersion4?WIDE("delete from %s where %s='%s'")
						:WIDE("delete from %s where %s=%ld")

					  ,tree->flags.bVersion4?OPTION4_VALUES:
						tree->flags.bNewVersion?OPTION_VALUES:WIDE( "option_values" )
					  , (tree->flags.bVersion4||tree->flags.bNewVersion)?WIDE( "option_id" ):WIDE( "value_id" )
					  , tree->flags.bVersion4?optval->value_guid:(CPOINTER)optval->value_id );
			OpenWriter( tree );
			if( !SQLCommand( tree->odbc_writer, update ) )
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( WIDE("Delete value failed: %s"), result );
				retval = FALSE;
			}

			snprintf( update, sizeof( update ), WIDE("delete from %s where %s=%ld")
					  , tree->flags.bVersion4?OPTION4_MAP
						:tree->flags.bNewVersion?OPTION_MAP:WIDE( "option_map" )
					  , (tree->flags.bVersion4||tree->flags.bNewVersion)?WIDE( "option_id" ):WIDE( "option_id" )
					  , tree->flags.bVersion4?optval->value_guid:(CPOINTER)optval->value_id  );
			if( !SQLCommand( tree->odbc_writer, update ) )
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( WIDE("Delete option failed: %s"), result );
				retval = FALSE;
			}
		}
		LeaveCriticalSec( &og.cs_option );
		return retval;
	}
	if( tree->flags.bVersion4 )
	{
		New4CreateValue( tree, optval, pValue );
		LeaveCriticalSec( &og.cs_option );
      return TRUE;
	}

	{
		// should escape quotes passed in....
		//if( IDValue && IDValue != INVALID_INDEX )
		//	snprintf( update, sizeof( update ), WIDE("select string from %s where %s=%lu")
		//			  , tree->flags.bNewVersion?OPTION_VALUES:"option_values"
		//			  , tree->flags.bNewVersion?"option_id":"value_id"
		//			  , IDValue );
		//StrCpyEx( value, pValue, sizeof( value )-1 );
		if( pValue && optval->value_id != INVALID_INDEX )
		{
         //lprintf( "Loaded it from database, and have value..." );
			if( StrCmp( pValue, optval->value ) == 0 )
			{
				LeaveCriticalSec( &og.cs_option );
				return retval;
			}
		}

		if( optval->value_id == INVALID_INDEX )
		{
			TEXTCHAR old_value[256];
			size_t result = GetOptionStringValueEx( tree->odbc, optval, old_value, sizeof( old_value ) DBG_SRC );
			if( result && ( StrCmp( old_value, pValue ) == 0 ) )
			{
				LeaveCriticalSec( &og.cs_option );
				return retval;
			}
		}

		if( tree->flags.bNewVersion )
			optval->value_id = optval->id;
		optval->value = StrDup( pValue );

		newval = EscapeSQLBinaryOpt( tree->odbc, pValue, strlen( pValue ), TRUE );
		//lprintf( "ID is %d", IDValue );
		if( optval && ( optval->value_id != INVALID_INDEX ) )
		{
			snprintf( update, sizeof( update ), WIDE("replace into %s (string,%s) values (%s,%ld)")
					  , tree->flags.bNewVersion?OPTION_VALUES:WIDE( "option_values" )
					  , tree->flags.bNewVersion?WIDE( "option_id" ):WIDE( "value_id" )
					  , newval
					  , optval->value_id );
			SQLEndQuery( tree->odbc );
			OpenWriter( tree );
			if( !SQLCommand( tree->odbc_writer, update ) )
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( WIDE("Update value failed: %s"), result );
				retval = FALSE;
			}
		}
		else
		{
			//lprintf( "Really create option value" );
			retval = CreateValue( tree, optval, pValue );
			if( retval )
			{
				// setoption might fail, resulting in an invalid index ID
				retval = SetOptionValueEx( tree, optval );
			}
		}
	}
	LeaveCriticalSec( &og.cs_option );

	Release( newval );
	return retval;
}

//------------------------------------------------------------------------
// result with option value ID
static LOGICAL SetOptionBlobValueEx( POPTION_TREE tree, POPTION_TREE_NODE optval, POINTER buffer, size_t length )
{
	OpenWriter( tree );
	if( tree->flags.bNewVersion )
	{
		{
			TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, (CTEXTSTR)buffer, length, TRUE );
			LOGICAL retval =
				SQLCommandf( tree->odbc_writer, WIDE( "replace into " )OPTION_BLOBS WIDE( " (`option_id`,`binary` ) values (%lu,%s)" )
							  , optval->id
							  , newval
							  );
			Release( newval );
			return retval;
		}
	}
	else if( tree->flags.bVersion4 )
	{
		{
			TEXTSTR newval = EscapeSQLBinaryOpt( tree->odbc_writer, (CTEXTSTR)buffer, length, TRUE );
			LOGICAL retval =
				SQLCommandf( tree->odbc_writer, WIDE( "replace into " )OPTION4_BLOBS WIDE( " (`option_id`,`binary` ) values (%lu,%s)" )
							  , optval->id
							  , newval
							  );
			Release( newval );
			return retval;
		}
	}
	else
	{
		// update value.
		CTEXTSTR result = NULL;
		LOGICAL retval = TRUE;
		//INDEX IDValue = GetOptionValueIndexEx( tree->odbc, optval );
		// should escape quotes passed in....
		if( optval->value_id == INVALID_INDEX )
		{
			retval = CreateValue( tree, optval, NULL );
			if( retval )
				retval = SetOptionValueEx( tree, optval );
		}

		if( retval )
		{
			TEXTCHAR *tmp = EscapeSQLBinaryOpt( tree->odbc_writer, (CTEXTSTR)buffer, length, TRUE );
			if( !SQLCommandf( tree->odbc_writer, WIDE("update option_values set `binary`=%s where value_id=%ld"), tmp, optval->value_id ) )
			{
				FetchSQLError( tree->odbc_writer, &result );
				lprintf( WIDE("Update value failed: %s"), result );
				retval = FALSE;
			}
			Release( tmp );
		}
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
	return _SQLPromptINIValue(lpszSection, lpszEntry, lpszDefault, lpszReturnBuffer, cbReturnBuffer, filename );
#endif
#endif
#if prompt_stdout
	fprintf( stdout, WIDE( "[%s]%s=%s?\nor enter new value:" ), lpszSection, lpszEntry, lpszDefault );
	fflush( stdout );
	if( fgets( lpszReturnBuffer, cbReturnBuffer, stdin ) && lpszReturnBuffer[0] != '\n' && lpszReturnBuffer[0] )
	{
      return strlen( lpszReturnBuffer );
	}
#endif
	StrCpyEx( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
	lpszReturnBuffer[cbReturnBuffer-1] = 0;
	return strlen( lpszReturnBuffer );
}

struct check_mask_param
{
	LOGICAL is_found;
	LOGICAL is_mapped;
	CTEXTSTR section_name;
	CTEXTSTR file_name;
	PODBC odbc;
};

static int CPROC CheckMasks( PTRSZVAL psv_params, CTEXTSTR name, POPTION_TREE_NODE this_node, int flags )
{
   struct check_mask_param *params = (struct check_mask_param*)psv_params;
	// return 0 to break loop.
   lprintf( "Had mask to check [%s]", name );
	if( CompareMask( name, params->section_name, FALSE ) )
	{
		params->is_found = TRUE;
		//GetOptionStringValue( ... );
		{
			TEXTCHAR resultbuf[12];
			TEXTCHAR key[256];
			snprintf( key, 256, WIDE("System Settings/Map INI Local/%s"), params->file_name );
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
		while( pINIFile[0] == '/' || pINIFile[0] == '\\' )
			pINIFile++;
		if( !pathchr( pINIFile ) )
		{
			if( ( pINIFile != DEFAULT_PUBLIC_KEY )
				&& ( StrCaseCmp( pINIFile, DEFAULT_PUBLIC_KEY ) != 0 ) )
			{
				lprintf( "(Convert %s)", pINIFile );
				if( og.flags.bEnableSystemMapping == 2 )
					og.flags.bEnableSystemMapping = SACK_GetProfileIntEx( WIDE( "System Settings"), WIDE( "Enable System Mapping" ), 0, TRUE );
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
                  lprintf( "FILE is not mapped entirly, check enumerated options..." );
						snprintf( buf, 128, WIDE("System Settings/Map INI Local Masks/%s"), pINIFile );
                  lprintf( "buf is %s", buf );
						node = GetOptionIndexExxx( odbc, NULL, DEFAULT_PUBLIC_KEY, NULL, buf, FALSE, FALSE DBG_SRC );
						if( node )
						{
							lprintf( "Node is %p?", node );
							EnumOptionsEx( odbc, node, CheckMasks, (PTRSZVAL)&params );
							lprintf( "Done enumerating..." );
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
						snprintf( buf, 128, WIDE("System Settings/Map INI Local/%s"), pINIFile );
						SACK_GetPrivateProfileStringExxx( odbc, buf, pSection, WIDE("0"), resultbuf, 12, NULL, TRUE DBG_SRC );
						if( resultbuf[0] != '0' )
							params.is_mapped = TRUE;
					}

					if( params.is_mapped )
					{
						snprintf( buf, 128, WIDE("System Settings/%s/%s"), GetSystemName(), pINIFile );
						buf[127] = 0;
						pINIFile = buf;
					}
				}
            lprintf( "(result %s)", pINIFile );
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
	//lprintf( "Getting {%s}[%s]%s=%s", pINIFile, pSection, pOptname, pDefaultbuf );
	if( !odbc )
	{
		if( !og.Option )
			InitMachine();
		odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN(), global_sqlstub_data->OptionVersion );
		drop_odbc = TRUE;
	}

	if( !pINIFile )
		pINIFile = DEFAULT_PUBLIC_KEY;
	else
	{
		char buf[128];
		pINIFile = ResolveININame( odbc, pSection, buf, pINIFile );
	}

	{
		// first try, do it as false, so we can fill in default values.
		POPTION_TREE_NODE opt_node;
		// maybe do an if( l.flags.bLogOptionsRead )
		if( global_sqlstub_data->flags.bLogOptionConnection )
			_lprintf(DBG_RELAY)( WIDE( "Getting option {%s}[%s]%s=%s" ), pINIFile, pSection, pOptname, pDefaultbuf );
		opt_node = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, pINIFile, pSection, pOptname, FALSE DBG_RELAY );
		if( !opt_node )
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
		do_defaulting:
			if( !bQuiet && og.flags.bPromptDefault )
			{
				SQLPromptINIValue( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, pINIFile );
			}
			else
			{
				if( pDefaultbuf )
					StrCpyEx( pBuffer, pDefaultbuf, nBuffer/sizeof( TEXTCHAR ) );
				else
					pBuffer[0] = 0;
			}
			// create the option branch since it doesn't exist...
			opt_node = GetOptionIndexExxx( odbc, OPTION_ROOT_VALUE, pINIFile, pSection, pOptname, TRUE, TRUE DBG_RELAY );
			{
				int x;
				if( SetOptionStringValue( GetOptionTreeEx( odbc ), opt_node, pBuffer ) )
					x = (int)StrLen( pBuffer );
				else
					x = 0;
				if( global_sqlstub_data->flags.bLogOptionConnection )
					lprintf( "Result [%s]", pBuffer );
				if( drop_odbc )
					DropOptionODBC( odbc );
				LeaveCriticalSec( &og.cs_option );
				return x;
			}
			//strcpy( pBuffer, pDefaultbuf );
		}
		else
		{
			size_t x = GetOptionStringValueEx( odbc, GetOptionValueIndexEx( odbc, opt_node ), pBuffer, nBuffer DBG_RELAY );
			if( (x == (size_t)-1) && pDefaultbuf && pDefaultbuf[0] )
			{
				if( global_sqlstub_data->flags.bLogOptionConnection )
					lprintf( WIDE( "No value result, get or set default..." ) );
				// if there's no default, doesn't matter if it's set or not.
				goto do_defaulting;
			}
			if( global_sqlstub_data->flags.bLogOptionConnection )
				lprintf( WIDE( "buffer result is [%s]" ), pBuffer );
			if( drop_odbc )
				DropOptionODBC( odbc );
			LeaveCriticalSec( &og.cs_option );
			return x;
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
   return SACK_GetPrivateProfileStringExxx( og.Option,    pSection
																		  , pOptname
																		  , pDefaultbuf
																		  , pBuffer
																		  , nBuffer
																		  , pINIFile
																		  , bQuiet
																			DBG_RELAY
																		  );

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

SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileIntExx )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pINIFile, LOGICAL bQuiet DBG_PASS )
{
	TEXTCHAR buffer[32];
	TEXTCHAR defaultbuf[32];
	snprintf( defaultbuf, sizeof( defaultbuf ), WIDE("%ld"), nDefault );
	if( SACK_GetPrivateProfileStringExxx( odbc, pSection, pOptname, defaultbuf, buffer, sizeof( buffer )/sizeof(TEXTCHAR), pINIFile, bQuiet DBG_RELAY ) )
	{
		if( buffer[0] == 'Y' || buffer[0] == 'y' )
			return 1;
		return atoi( buffer );
	}
	return nDefault;
}

SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pINIFile, LOGICAL bQuiet )
{
   return SACK_GetPrivateProfileIntExx( og.Option, pSection, pOptname, nDefault, pINIFile, bQuiet DBG_SRC );
}


SQLGETOPTION_PROC( S_32, SACK_GetPrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 nDefault, CTEXTSTR pINIFile )
{
	return SACK_GetPrivateProfileIntEx( pSection, pOptname, nDefault, pINIFile, FALSE );
}

//------------------------------------------------------------------------


SQLGETOPTION_PROC( size_t, SACK_GetProfileStringEx )( CTEXTSTR pSection, CTEXTSTR pOptname, CTEXTSTR pDefaultbuf, TEXTCHAR *pBuffer, size_t nBuffer, LOGICAL bQuiet )
{
	return SACK_GetPrivateProfileStringEx( pSection, pOptname, pDefaultbuf, pBuffer, nBuffer, NULL, bQuiet );
}

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
	optval = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, NULL, pSection, pOptname, FALSE DBG_SRC );
	if( !optval )
	{
		return FALSE;
	}
	else
	{
		return GetOptionBlobValueOdbc( odbc, GetOptionValueIndexEx( odbc, optval ), pBuffer, pnBuffer );
	}
	return FALSE;
//   int status = SACK_GetProfileString( );
}

SQLGETOPTION_PROC( int, SACK_GetProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR **pBuffer, size_t *pnBuffer )
{
   return SACK_GetProfileBlobOdbc( og.Option, pSection, pOptname, pBuffer, pnBuffer );
}

//------------------------------------------------------------------------

SQLGETOPTION_PROC( S_32, SACK_GetProfileIntEx )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 defaultval, LOGICAL bQuiet )
{
   return SACK_GetPrivateProfileIntEx( pSection, pOptname, defaultval, NULL, bQuiet );
}

//------------------------------------------------------------------------

SQLGETOPTION_PROC( S_32, SACK_GetProfileInt )( CTEXTSTR pSection, CTEXTSTR pOptname, S_32 defaultval )
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
      char buf[128];
      pINIFile = ResolveININame( odbc, pSection, buf, pINIFile );
	}
	optval = GetOptionIndexExxx( odbc, NULL, pINIFile, pSection, pName, TRUE, FALSE DBG_SRC );
	if( !optval )
	{
		lprintf( WIDE("Creation of path failed!") );
		return FALSE;
	}
	else
	{
		POPTION_TREE tree = GetOptionTreeEx( odbc );
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
	return SACK_WritePrivateOptionStringEx( og.Option, pSection, pName, pValue, pINIFile, flush );
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

SQLGETOPTION_PROC( S_32, SACK_WritePrivateProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value, CTEXTSTR pINIFile )
{
	TEXTCHAR valbuf[32];
	snprintf( valbuf, sizeof( valbuf ), WIDE("%ld"), value );
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

SQLGETOPTION_PROC( S_32, SACK_WriteProfileInt )( CTEXTSTR pSection, CTEXTSTR pName, S_32 value )
{
	return SACK_WritePrivateProfileInt( pSection, pName, value, NULL );
}

SQLGETOPTION_PROC( int, SACK_WriteProfileBlobOdbc )( PODBC odbc, CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer )
{
	POPTION_TREE_NODE optval;
	optval = GetOptionIndexExx( odbc, OPTION_ROOT_VALUE, NULL, pSection, pOptname, TRUE DBG_SRC );
	if( !optval )
	{
		lprintf( WIDE("Creation of path failed!") );
		return FALSE;
	}
	else
	{
		POPTION_TREE tree = GetOptionTreeEx( odbc );
		return SetOptionBlobValueEx( tree, optval, pBuffer, nBuffer ) != INVALID_INDEX;
	}
	return 0;
}


SQLGETOPTION_PROC( int, SACK_WriteProfileBlob )( CTEXTSTR pSection, CTEXTSTR pOptname, TEXTCHAR *pBuffer, size_t nBuffer )
{
   return SACK_WriteProfileBlobOdbc( og.Option, pSection, pOptname, pBuffer, nBuffer );
}

//------------------------------------------------------------------------

#if 0
/// this still needs a way to communicate the time from and time until.
SQLGETOPTION_PROC( int, SACK_WritePrivateProfileExceptionString )( CTEXTSTR pSection
                                                            , CTEXTSTR pName
                                                            , CTEXTSTR pValue
                                                            , CTEXTSTR pINIFile
                                                            , _32 from // SQLTIME
                                                            , _32 to   // SQLTIME
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

		snprintf( exception, sizeof( exception ), WIDE("insert into option_exception (`apply_from`,`apply_to`,`value_id`,`override_value_idvalue_id`,`system`) ")
																	  WIDE( "values( \'%04d%02d%02d%02d%02d\', \'%04d%02d%02d%02d%02d\', %ld, %ld,%d")
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

PRIORITY_PRELOAD( AllocateOptionGlobal, CONFIG_SCRIPT_PRELOAD_PRIORITY )
{
	SimpleRegisterAndCreateGlobal( sack_global_option_data );
	InitializeCriticalSec( &og.cs_option );
}

PRIORITY_PRELOAD(RegisterSQLOptionInterface, SQL_PRELOAD_PRIORITY + 1 )
{
	if( !og.flags.bRegistered )
	{
		og.flags.bRegistered = 1;

		RegisterInterface( WIDE("SACK_SQL_Options"), (POINTER(CPROC *)(void))GetOptionInterface, (void(CPROC *)(POINTER))DropOptionInterface );
		og.flags.bUseProgramDefault = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/SQL/Options/Options Use Program Name Default" ), 0, TRUE );
		og.flags.bUseSystemDefault = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/SQL/Options/Options Use System Name Default" ), 0, TRUE );
		og.flags.bEnableSystemMapping = 2;
	}
}


SQLGETOPTION_PROC( INDEX, GetSystemID )( void )
{
   InitMachine();
   return og.SystemID;
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

SQLGETOPTION_PROC( CTEXTSTR, GetDefaultOptionDatabaseDSN )( void )
{
   return global_sqlstub_data->OptionDb.info.pDSN;
}

PODBC GetOptionODBC( CTEXTSTR dsn, int version )
{
	INDEX idx;
	struct option_odbc_tracker *tracker;
	LIST_FORALL( og.odbc_list, idx, struct option_odbc_tracker *, tracker )
	{
		if( StrCaseCmp( dsn, tracker->name ) == 0 )
		{
			if( version == tracker->version )
				break;
		}
	}
	if( !tracker )
	{
		tracker = New( struct option_odbc_tracker );
		tracker->name = StrDup( dsn );
		tracker->version = version;
		tracker->available = CreateLinkQueue();
        tracker->outstanding = NULL;
        AddLink( &og.odbc_list, tracker );
	}
	{
		PODBC odbc = (PODBC)DequeLink( &tracker->available );
		if( !odbc )
		{
			odbc = ConnectToDatabase( tracker->name );
			SetOptionDatabaseOption( odbc, version==1?0:version==2?1:2 );
		}
        AddLink( &tracker->outstanding, odbc );
        return odbc;
	}
}


void DropOptionODBC( PODBC odbc )
{
	INDEX idx;
	struct option_odbc_tracker *tracker;
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
		if( connection )
			break;
	}
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

SACK_OPTION_NAMESPACE_END
