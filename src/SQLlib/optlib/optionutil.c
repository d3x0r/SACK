#ifndef GETOPTION_SOURCE
#define GETOPTION_SOURCE
#endif

#include <stdhdrs.h>
#include <sharemem.h>
#include <pssql.h>
#include <sqlgetoption.h>


#include "../sqlstruc.h"

SACK_OPTION_NAMESPACE

#include "optlib.h"

#define og (*sack_global_option_data)
extern struct sack_option_global_tag *sack_global_option_data;

#define ENUMOPT_FLAG_HAS_VALUE 1
#define ENUMOPT_FLAG_HAS_CHILDREN 2


SQLGETOPTION_PROC( void, EnumOptionsEx )( PODBC odbc, POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
											  , PTRSZVAL psvUser )
{
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	InitMachine();
	if( tree->flags.bNewVersion )
	{
		NewEnumOptions( og.Option, parent, Process, psvUser );
	}
	else if( tree->flags.bVersion4 )
	{
		New4EnumOptions( og.Option, parent, Process, psvUser );
	}
	else
	{
		TEXTCHAR query[256];
		int first_result, popodbc;
		CTEXTSTR result = NULL;
		POPTION_TREE_NODE tmp_node = New( struct sack_option_tree_family_node );
		// any existing query needs to be saved...
		InitMachine();
		PushSQLQueryEx( og.Option ); // any subqueries will of course clean themselves up.
		snprintf( query
				  , sizeof( query )
				  , WIDE( "select node_id,m.name_id,value_id,n.name" )
					WIDE( " from option_map as m" )
					WIDE( " join option_name as n on n.name_id=m.name_id" )
					WIDE( " where parent_node_id=%ld" )
               WIDE( " order by name" )
				  , parent?parent->id:0 );
		popodbc = 0;
		for( first_result = SQLQuery( og.Option, query, &result );
			 result;
			  FetchSQLResult( og.Option, &result ) )
		{
			CTEXTSTR optname;
         POPTION_TREE_NODE tmp_node = New( OPTION_TREE_NODE );
			popodbc = 1;
			sscanf( result, WIDE("%lu,%lu,%lu"), &tmp_node->id, &tmp_node->name_id, &tmp_node->value_id );
			optname = strrchr( result, ',' );
			if( optname )
				optname++;
			//ReadFromNameTable( name, WIDE("option_name"), WIDE("name_id"), &result);
			if( !Process( psvUser, optname, tmp_node
							, ((tmp_node->value_id!=INVALID_INDEX)?1:0)
							) )
			{
				break;
			}
			//lprintf( WIDE("reget: %s"), query );
		}
		PopODBCEx( og.Option );
	}
}

SQLGETOPTION_PROC( void, EnumOptions )( POPTION_TREE_NODE parent
					 , int (CPROC *Process)(PTRSZVAL psv, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
											  , PTRSZVAL psvUser )
{
   EnumOptionsEx( og.Option, parent, Process, psvUser );
}
struct copy_data {
	POPTION_TREE_NODE iNewName;
   POPTION_TREE tree;
};

static int CPROC CopyRoot( PTRSZVAL iNewRoot, CTEXTSTR name, POPTION_TREE_NODE ID, int flags )
{
	struct copy_data *copydata = (struct copy_data *)iNewRoot;
	struct copy_data newcopy;
	// iNewRoot is at its source an INDEX
	POPTION_TREE_NODE iCopy = GetOptionIndexEx( copydata->iNewName, NULL, name, NULL, TRUE DBG_SRC );
	POPTION_TREE_NODE iValue = GetOptionValueIndex( ID );
	newcopy.tree = copydata->tree;
	newcopy.iNewName = iCopy;
	if( ID->value_id != INVALID_INDEX )
	{
      DuplicateValue( iValue, iCopy );
		SetOptionValueEx( copydata->tree, iCopy );
	}

	EnumOptions( ID, CopyRoot, (PTRSZVAL)&newcopy );
	return TRUE;
}

SQLGETOPTION_PROC( void, DuplicateOptionEx )( PODBC odbc, POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
	CTEXTSTR result = NULL;
	struct copy_data copydata;
	POPTION_TREE tree = GetOptionTreeEx( odbc );
	copydata.tree = tree;
	if( tree->flags.bNewVersion )
	{
		NewDuplicateOption( og.Option, iRoot, pNewName );
		return;
	}
	if( SQLQueryf( og.Option, &result, WIDE("select parent_node_id from option_map where node_id=%ld"), iRoot ) && result )
	{
		POPTION_TREE_NODE tmp_node = New( OPTION_TREE_NODE );
		tmp_node->id = IndexCreateFromText( result );
		tmp_node->name_id = INVALID_INDEX;
      tmp_node->value_id = INVALID_INDEX;
		copydata.iNewName = GetOptionIndexEx( tmp_node, NULL, pNewName, NULL, TRUE DBG_SRC );
		EnumOptions( iRoot, CopyRoot, (PTRSZVAL)&copydata );
	}
}

SQLGETOPTION_PROC( void, DuplicateOption )( POPTION_TREE_NODE iRoot, CTEXTSTR pNewName )
{
   DuplicateOptionEx( og.Option, iRoot, pNewName );
}

static void FixOrphanedBranches( void )
{
	PLIST options = CreateList();
	CTEXTSTR *result = NULL;
	CTEXTSTR result2 = NULL;
	SQLQuery( og.Option, WIDE("select count(*) from option_map"), &result2 );
	// expand the options list to max extent real quickk....
	SetLink( &options, atoi( result2 ) + 1, 0 );
	for( SQLRecordQuery( og.Option, WIDE("select node_id,parent_node_id from option_map"), NULL, &result, NULL );
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
					lprintf( WIDE("node %ld has parent id %ld which does not exist."), idx, parent-1 );
					SetLink( &options, idx, NULL );
					SQLCommandf( og.Option, WIDE("delete from option_map where node_id=%ld"), idx );
				}
			}
		}while( deleted );
	}
	DeleteList( &options );
}


SQLGETOPTION_PROC( void, DeleteOption )( POPTION_TREE_NODE iRoot )
{
	POPTION_TREE tree = GetOptionTreeEx( og.Option );
	if( tree->flags.bVersion4 )
	{
		New4DeleteOption( og.Option, iRoot );
		return;
	}
	else if( tree->flags.bNewVersion )
	{
		NewDeleteOption( og.Option, iRoot );
		return;
	}
	else
	{
		SQLCommandf( og.Option, WIDE("delete from option_map where node_id=%ld"), iRoot->id );
	   	FixOrphanedBranches();
	}
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

