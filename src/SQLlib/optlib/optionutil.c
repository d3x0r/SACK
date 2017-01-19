#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif

#include <stdhdrs.h>
#include <sharemem.h>
#include <pssql.h>
#include <sqlgetoption.h>

// we want access to GLOBAL from sqltub
#define SQLLIB_SOURCE
#include "../sqlstruc.h"

SQL_NAMESPACE
extern struct pssql_global *global_sqlstub_data;
SQL_NAMESPACE_END

SACK_OPTION_NAMESPACE

#include "optlib.h"

#define og (*sack_global_option_data)
extern struct sack_option_global_tag *sack_global_option_data;

#define ENUMOPT_FLAG_HAS_VALUE 1
#define ENUMOPT_FLAG_HAS_CHILDREN 2


SQLGETOPTION_PROC( void, EnumOptionsEx )( PODBC odbc, POPTION_TREE_NODE parent
					 , int (CPROC *Process)(uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
											  , uintptr_t psvUser )
{
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	New4EnumOptions( odbc, parent, Process, psvUser );
}

SQLGETOPTION_PROC( void, EnumOptions )( POPTION_TREE_NODE parent
					 , int (CPROC *Process)(uintptr_t psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
											  , uintptr_t psvUser )
{
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	EnumOptionsEx( odbc, parent, Process, psvUser );
	DropOptionODBC( odbc );
}

SQLGETOPTION_PROC( void, DuplicateOptionEx )( PODBC odbc, POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	New4DuplicateOption( odbc, iRoot, pNewName );
}

SQLGETOPTION_PROC( void, DuplicateOption )( POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
   	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	DuplicateOptionEx( odbc, iRoot, pNewName );
   	DropOptionODBC( odbc );
}

static void FixOrphanedBranches( void )
{
	PLIST options = CreateList();
	CTEXTSTR *result = NULL;
	CTEXTSTR result2 = NULL;
   	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	SQLQuery( odbc, WIDE("select count(*) from option_map"), &result2 );
	// expand the options list to max extent real quickk....
	SetLink( &options, IntCreateFromText( result2 ) + 1, 0 );
	for( SQLRecordQuery( odbc, WIDE("select node_id,parent_node_id from option_map"), NULL, &result, NULL );
		  result;
		  GetSQLRecord( &result ) )
	{
		INDEX node_id, parent_node_id;
		node_id = IndexCreateFromText( result[0] );
		parent_node_id = IndexCreateFromText( result[1] );
		//sscanf( result, WIDE("%ld,%ld"), &node_id, &parent_node_id );
		SetLink( &options, node_id, (POINTER)(parent_node_id+1) );
	}
	{
		INDEX idx;
   		int deleted;
		INDEX parent;
		do
		{
			deleted = 0;
			LIST_FORALL( options, idx, INDEX, parent )
			{
				//lprintf( WIDE("parent node is...%ld"), parent );
				// node ID parent of 0 or -1 is a parent node...
				// so nodeID+1 of 0 is 1
				if( (parent > 1) && !GetLink( &options, parent-1 ) )
				{
					deleted = 1;
					lprintf( WIDE("node %")_size_f WIDE(" has parent id %")_size_f WIDE(" which does not exist."), idx, parent-1 );
					SetLink( &options, idx, NULL );
					SQLCommandf( odbc, WIDE("delete from option_map where node_id=%")_size_f , idx );
				}
			}
		}while( deleted );
	}
	DeleteList( &options );
   DropOptionODBC( odbc );
}


SQLGETOPTION_PROC( void, DeleteOption )( POPTION_TREE_NODE iRoot )
{
	PODBC odbc = GetOptionODBC( GetDefaultOptionDatabaseDSN() );
	POPTION_TREE tree = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	New4DeleteOption( odbc, iRoot );
	DropOptionODBC( odbc );
}

SACK_OPTION_NAMESPACE_END

//--------------------------------------------------------------------------
//
// $Log: optionutil.c,v $
// Revision 1.3  2005/03/21 19:37:08  jim
// Update to reflect newest database standardized changes
//
// Revision 1.2  2004/07/08 18:23:26  jim
// roughly works now.
//
// Revision 1.1  2004/04/15 00:12:56  jim
// Checkpoint
//
//

