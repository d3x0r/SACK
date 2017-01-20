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

