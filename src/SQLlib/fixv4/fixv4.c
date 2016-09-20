#include <stdhdrs.h>
#include <pssql.h>


struct name_entry
{
	TEXTSTR guid;
	TEXTSTR name;
	PLIST replace;
   int count;
};

PTREEROOT names;
PLIST options;

int CPROC MyCompare( uintptr_t a, uintptr_t b )
{
	lprintf( "is %s==%s", a, b );
	return StrCaseCmp( (CTEXTSTR)a, (CTEXTSTR)b );
}


int main( int argc, char **argv )
{
   int n = 0;
	TEXTSTR *results;
	PODBC odbc;
	if( argc > 1 )
		odbc = SQLGetODBC( argv[1] );
	else
		odbc = SQLGetODBC( "MySQL" );
	names = CreateBinaryTreeExtended( BT_OPT_NODUPLICATES, MyCompare, NULL DBG_SRC );
	for( SQLRecordQueryf( odbc, NULL, &results, NULL, "select name_id,name from option4_name" );
		 results;
		  FetchSQLRecord( odbc, &results ) )
	{
		LOGICAL changed = FALSE;
		struct name_entry *name = New( struct name_entry );
		name->guid = StrDup( results[0] );
		name->name = StrDup( results[1] );
		while( name->name[StrLen( name->name )-1] == ' ')
		{
			changed = TRUE;
			name->name[StrLen( name->name )-1] = 0;
		}
		if( changed )
			SQLCommandf( odbc, "update option4_name set name='%s' where name='%s'", name->name, results[1] );

		name->replace = NULL;
		if( !AddBinaryNode( names, name, name->name ) )
		{
			struct name_entry * oldname = (struct name_entry *)FindInBinaryTree( names, name->name );
			if( StrCaseCmp( oldname->guid, name->guid ) ==0 )
				continue;
			AddLink( &oldname->replace, name );
         // duplicated.
		}
		BalanceBinaryTree( names );
	}

	{
		struct name_entry *name;
		for( name = GetLeastNode( names );
			 name;
			  name = GetGreaterNode( names ) )
		{
			if( name->replace )
			{
				INDEX idx;
				struct name_entry *replace;
				LIST_FORALL( name->replace, idx, struct name_entry *, replace )
				{
					SQLCommandf( odbc, "update option4_name set name_id='%s' where name_id='%s'", name->guid, replace->guid );
				}
			}
		}
	}


	{
		struct name_entry *name;
		SQLCommandf( odbc, "delete from option4_name" );
		for( name = GetLeastNode( names );
			 name;
			  name = GetGreaterNode( names ) )
		{
			SQLCommandf( odbc, "insert into option4_name (name_id,name) values ('%s','%s')", name->guid, name->name );
		}
	}

	return 0;
}
