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

struct new4_enum_params
{
	int (CPROC *Process)(uintptr_t psv
							  , CTEXTSTR name
							  , POPTION_TREE_NODE ID
							  , int flags );
	uintptr_t psvEnum;
};


static LOGICAL CPROC New4CheckOption( uintptr_t psvForeach, uintptr_t psvNode )
{
	POPTION_TREE_NODE option_node = (POPTION_TREE_NODE)psvNode;
	struct new4_enum_params *params = (struct new4_enum_params *)psvForeach;
	return params->Process( params->psvEnum, option_node->name, option_node
								 , ((option_node->value_id)?1:0) );
}

void New4EnumOptions( PODBC odbc
												  , POPTION_TREE_NODE parent
												  , int (CPROC *Process)(uintptr_t psv
																				, CTEXTSTR name
																				, POPTION_TREE_NODE ID
																				, int flags )
												  , uintptr_t psvUser )
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

	// first should check the exisiting loaded family tree....
	//lprintf( WIDE("Enumerating for %p %p %s %s"), parent, parent->guid, parent->guid, parent->name_guid );

	{
		struct new4_enum_params params;
		params.Process = Process;
		params.psvEnum = psvUser;
		FamilyTreeForEachChild( node->option_tree, parent->node, New4CheckOption, (uintptr_t)&params );
	}

	if( !parent->flags.bExpanded || ( ( timeGetTime() - 5000 ) > parent->expansion_tick ) )
	{
		parent->flags.bExpanded = 1;
		parent->expansion_tick = timeGetTime();
		// any existing query needs to be saved...
		PushSQLQueryEx( odbc ); // any subqueries will of course clean themselves up.
		tnprintf( query
			  , sizeof( query )
			  , WIDE( "select option_id,n.name,n.name_id " )
				WIDE( "from " )OPTION4_MAP WIDE( " as m " )
				WIDE( "join " )OPTION4_NAME WIDE( " as n on n.name_id=m.name_id " )
				WIDE( "where parent_option_id='%s' " )
				WIDE( "order by n.name" )
			  , parent->guid?parent->guid:GuidZero() );
		popodbc = 0;
		for( first_result = SQLRecordQuery( odbc, query, NULL, &results, NULL );
			 results;
			  FetchSQLRecord( odbc, &results ) )
		{
			// if it was already in the tree, it was processed in the loop above.
			// there is no actual sorting on insert of family nodes, it's more about their relations
			// then their stats...

			// try adding this node into the tree.
			POPTION_TREE_NODE existing = (POPTION_TREE_NODE)FamilyTreeFindChildEx( node->option_tree, parent->node, (uintptr_t)results[1] );
			if( !existing )
			{
				tmp_node = New( OPTION_TREE_NODE );
				MemSet( tmp_node, 0, sizeof( struct sack_option_tree_family_node ) );
				tmp_node->guid = StrDup( results[0] );
				tmp_node->name_guid = StrDup( results[2] );
				tmp_node->value_guid = NULL;

				popodbc = 1;
				tmp_node->name = SaveText( results[1] );
				tmp_node->node = FamilyTreeAddChild( &node->option_tree, tmp_node, (uintptr_t)tmp_node->name );

				// psv is a pointer to args in some cases...
				//lprintf( WIDE( "Enum %s %ld" ), optname, node );
				//ReadFromNameTable( name, WIDE(""OPTION_NAME""), WIDE("name_id"), &result);
				if( !Process( psvUser, tmp_node->name, tmp_node
								, ((tmp_node->value_id)?1:0)
								) )
				{
					break;
				}
			}
			//lprintf( WIDE("reget: %s"), query );
		}
		PopODBCEx( odbc );
	}
	pending = NULL;
}


struct complex_args
{
	POPTION_TREE_NODE iNewRoot;
	POPTION_TREE_NODE iOldRoot;
	PODBC odbc;
};

static int CPROC CopyRoot( uintptr_t psvArgs, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
	struct complex_args *args = (struct complex_args*)psvArgs;
	POPTION_TREE_NODE iCopy = GetOptionIndexEx( args->iNewRoot, NULL, name, NULL, TRUE DBG_SRC );
	New4DuplicateValue( args->odbc, ID, iCopy );

	{
		struct complex_args c_args;
		c_args.iNewRoot = iCopy;
		c_args.odbc = args->odbc;
		EnumOptions( ID, CopyRoot, (uintptr_t)&c_args );
	}
	return TRUE;
}

void New4DuplicateOption( PODBC odbc, POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
	CTEXTSTR result = NULL;
	POPTION_TREE_NODE iNewName;
	if( SQLQueryf( odbc, &result, WIDE( "select parent_option_id from " ) OPTION4_MAP WIDE( " where option_id=%ld" ), iRoot ) && result )
	{
		POPTION_TREE_NODE tmp_node = New( OPTION_TREE_NODE );
		struct complex_args args;
		tmp_node->guid = StrDup( result );
		tmp_node->name_guid = NULL;
		tmp_node->value_guid = NULL;
		tmp_node->value = NULL;
		SQLEndQuery( odbc );
		iNewName = GetOptionIndexEx( tmp_node, NULL, pNewName, NULL, TRUE DBG_SRC );
		args.iNewRoot = iNewName;
		args.odbc = odbc;
		New4EnumOptions( args.odbc, iRoot, CopyRoot, (uintptr_t)&args );
	}
}


static void New4FixOrphanedBranches( void )
{
	PLIST options = CreateList();
	PLIST options2 = CreateList();
	CTEXTSTR *result = NULL;
	CTEXTSTR result2 = NULL;
	lprintf( WIDE("Orphan branches not fixable at this time.") );
	return;

	SQLQuery( og.Option, WIDE( "select count(*) from " ) OPTION_MAP, &result2 );
	// expand the options list to max extent real quickk....
	SetLink( &options, (uintptr_t)IntCreateFromText( result2 ) + 1, 0 );
	SetLink( &options2, (uintptr_t)IntCreateFromText( result2 ) + 1, 0 );

	for( SQLRecordQuery( og.Option, WIDE( "select option_id,parent_option_id from " ) OPTION4_MAP, NULL, &result, NULL );
		  result;
		  FetchSQLRecord( og.Option, &result ) )
	{
		CTEXTSTR node_id, parent_option_id;
		node_id = StrDup( result[0] );
		parent_option_id = StrDup( result[1] );
		AddLink( &options2, parent_option_id );
		//sscanf( result, WIDE("%ld,%ld"), &node_id, &parent_option_id );
		AddLink( &options, node_id );
	}
	{
		INDEX idx;
		int deleted;
		CTEXTSTR parent;
		do
		{
			deleted = 0;
			LIST_FORALL( options, idx, CTEXTSTR, parent )
			{
				//lprintf( WIDE("parent node is...%ld"), parent );
				// node ID parent of 0 or -1 is a parent node...
				// so nodeID+1 of 0 is 1
				//if( (parent > 1) && !GetLink( &options, parent-1 ) )
				{
					deleted = 1;
					//lprintf( WIDE("node %ld has parent id %ld which does not exist."), idx, parent-1 );
					SetLink( &options, idx, NULL );
					SQLCommandf( og.Option, WIDE( "delete from " )OPTION4_MAP WIDE( " where option_id='%s'" ), "some-guid"  );
				}
			}
		}while( deleted );
	}
	DeleteList( &options );
}


void New4DeleteOption( PODBC odbc, POPTION_TREE_NODE iRoot )
{
	SQLCommandf( odbc, WIDE( "delete from " )OPTION4_MAP WIDE( " where option_id='%s'" ), iRoot->guid );
	//   foriegn keys should be cascade, so these will disappear without specifically removing.
	//SQLCommandf( odbc, WIDE( "delete from " )OPTION4_VALUES WIDE( " where option_id='%s'" ), iRoot->guid );
	//SQLCommandf( odbc, WIDE( "delete from " )OPTION4_BLOBS WIDE( " where option_id='%s'" ), iRoot->guid );
	//New4FixOrphanedBranches();
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

