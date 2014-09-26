#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif

#include <stdhdrs.h>
#include <sharemem.h>
#include <pssql.h>
#include <sqlgetoption.h>


#include "../sqlstruc.h"
#include "makeopts.mysql"
SACK_OPTION_NAMESPACE

#include "optlib.h"

#define og (*sack_global_option_data)
extern struct sack_option_global_tag *sack_global_option_data;

#define ENUMOPT_FLAG_HAS_VALUE 1
#define ENUMOPT_FLAG_HAS_CHILDREN 2


void NewEnumOptions( PODBC odbc
												  , POPTION_TREE_NODE parent
												  , int (CPROC *Process)(PTRSZVAL psv
																				, CTEXTSTR name
																				, POPTION_TREE_NODE ID
																				, int flags )
												  , PTRSZVAL psvUser )
{
	POPTION_TREE node = GetOptionTreeExxx( odbc, NULL DBG_SRC );
	TEXTCHAR query[256];
	static PODBC pending;
	int first_result, popodbc;
	POPTION_TREE_NODE tmp_node;
	CTEXTSTR *results = NULL;
	if( !odbc )
		odbc = pending;
	if( !odbc )
		return;

	if( !parent )
		parent = node->root;

	pending = odbc;

	// any existing query needs to be saved...
	PushSQLQueryEx( odbc ); // any subqueries will of course clean themselves up.
	snprintf( query
			  , sizeof( query )
			  , WIDE( "select option_id,n.name,n.name_id " )
				WIDE( "from " )OPTION_MAP WIDE( " as m " )
				WIDE( "join " )OPTION_NAME WIDE( " as n on n.name_id=m.name_id " )
				WIDE( "where parent_option_id=%ld " )
				WIDE( "order by n.name" )
			  , parent->id );
	popodbc = 0;
	for( first_result = SQLRecordQuery( odbc, query, NULL, &results, NULL );
		 results;
		  FetchSQLRecord( odbc, &results ) )
	{
		CTEXTSTR optname;
		tmp_node = New( OPTION_TREE_NODE );
		tmp_node->id = IndexCreateFromText( results[0] );
		tmp_node->name_id = IndexCreateFromText( results[2] );
		tmp_node->value_id = INVALID_INDEX;

		popodbc = 1;
		optname = results[1];

      // psv is a pointer to args in some cases...
      //lprintf( WIDE( "Enum %s %ld" ), optname, node );
		//ReadFromNameTable( name, WIDE(""OPTION_NAME""), WIDE("name_id"), &result);
		if( !Process( psvUser, optname, tmp_node
						, ((tmp_node->value_id!=INVALID_INDEX)?1:0)
						) )
		{
			break;
		}
		//lprintf( WIDE("reget: %s"), query );
	}
	PopODBCEx( odbc );
	pending = NULL;

}


struct complex_args
{
	POPTION_TREE_NODE iNewRoot;
	POPTION_TREE_NODE iOldRoot;
	PODBC odbc;
};

static int CPROC CopyRoot( PTRSZVAL psvArgs, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
	struct complex_args *args = (struct complex_args*)psvArgs;
	POPTION_TREE_NODE iCopy = GetOptionIndexEx( args->iNewRoot, NULL, name, NULL, TRUE DBG_SRC );
	NewDuplicateValue( args->odbc, ID, iCopy );

	{
		struct complex_args c_args;
		c_args.iNewRoot = iCopy;
		c_args.odbc = args->odbc;
		EnumOptions( ID, CopyRoot, (PTRSZVAL)&c_args );
	}
	return TRUE;
}

void NewDuplicateOption( PODBC odbc, POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
	CTEXTSTR result = NULL;
	POPTION_TREE_NODE iNewName;
	if( SQLQueryf( odbc, &result, WIDE( "select parent_option_id from " ) OPTION_MAP WIDE( " where option_id=%ld" ), iRoot ) && result )
	{
		POPTION_TREE_NODE tmp_node = New( OPTION_TREE_NODE );
		struct complex_args args;
		tmp_node->id = IntCreateFromText( result );
		tmp_node->name_id = INVALID_INDEX;
		tmp_node->value_id = INVALID_INDEX;
		tmp_node->value = NULL;
		SQLEndQuery( odbc );
		iNewName = GetOptionIndexEx( tmp_node, NULL, pNewName, NULL, TRUE DBG_SRC );
		args.iNewRoot = iNewName;
		args.odbc = odbc;
		NewEnumOptions( args.odbc, iRoot, CopyRoot, (PTRSZVAL)&args );
	}
}


static void NewFixOrphanedBranches( void )
{
	PLIST options = CreateList();
	CTEXTSTR *result = NULL;
	CTEXTSTR result2 = NULL;
	SQLQuery( og.Option, WIDE( "select count(*) from " ) OPTION_MAP, &result2 );
   // expand the options list to max extent real quickk....
	SetLink( &options, (PTRSZVAL)IntCreateFromText( result2 ) + 1, 0 );
	for( SQLRecordQuery( og.Option, WIDE( "select option_id,parent_option_id from " )OPTION_MAP, NULL, &result, NULL );
		  result;
		  FetchSQLRecord( og.Option, &result ) )
	{
		INDEX node_id, parent_option_id;
		node_id = IndexCreateFromText( result[0] );
		parent_option_id = IndexCreateFromText( result[1] );
		//sscanf( result, WIDE("%ld,%ld"), &node_id, &parent_option_id );
		SetLink( &options, node_id, (POINTER)(parent_option_id+1) );
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
					//lprintf( WIDE("node %ld has parent id %ld which does not exist."), idx, parent-1 );
					SetLink( &options, idx, NULL );
					SQLCommandf( og.Option, WIDE( "delete from " )OPTION_MAP WIDE( " where option_id=%ld" ), idx );
				}
			}
		}while( deleted );
	}
	DeleteList( &options );
}


void NewDeleteOption( PODBC odbc, POPTION_TREE_NODE iRoot )
{
	SQLCommandf( odbc, WIDE( "delete from " )OPTION_MAP WIDE( " where option_id=%ld" ), iRoot->id );
	SQLCommandf( odbc, WIDE( "delete from " )OPTION_VALUES WIDE( " where option_id=%ld" ), iRoot->id );
	SQLCommandf( odbc, WIDE( "delete from " )OPTION_BLOBS WIDE( " where option_id=%ld" ), iRoot->id );
	NewFixOrphanedBranches();
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

