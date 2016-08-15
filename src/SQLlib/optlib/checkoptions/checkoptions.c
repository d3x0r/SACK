
#include <stdhdrs.h>
#include <sharemem.h>
#include <pssql.h>

// enumerate all options in the option map, validate that a parent exitst...


  int do_fix = 0;
	
typedef struct name_tag
{
	char *name;
   INDEX id;
} NAME, *PNAME;

void FixOptionNames( void )
{
   PLIST names = CreateList();
	CTEXTSTR result = NULL;
   PNAME name;
	for( DoSQLQuery( WIDE("select name_id,name from option_name"), &result )
		 ; result
		  ; GetSQLResult( &result ) )
	{
		INDEX name_idx;
      INDEX idx;
		char *p = strchr( result, ',' );
		p++;
      idx = atol( result );

		LIST_FORALL( names, name_idx, PNAME, name )
		{
			if( stricmp( name->name, p ) == 0 )
			{
				DoSQLCommandf( WIDE("update option_map set name_id=%ld where name_id=%ld")
							  , name->id
								 , idx );
			if( !do_fix )
			{
				lprintf( WIDE("Delete option name (unused in option map") );
			}
                         else
            DoSQLCommandf( WIDE("delete from option_name where name_id=%ld"), idx );
            break;
			}
		}
		if( !name )
		{
			name = New( NAME );//Allocate( sizeof( NAME ) );
			name->name = StrDup( p );
			name->id = idx;
         AddLink( &names, name );
		}
	}
	{
		PNAME name;
      INDEX name_idx;
		LIST_FORALL( names, name_idx, PNAME, name )
		{
			Release( name->name );
         Release( name );
		}
      DeleteList( &names );
	}
}

void FixDuplicateOptionNodes( void )
{
	CTEXTSTR result = NULL;
	INDEX original_parent = 0; //cpg27dec2006 , fix_parent = 0;
	INDEX new_root_node_id;
	int graphted;
	do
	{
      graphted = 0;
		for( DoSQLQuery( WIDE("SELECT a.node_id,b.parent_node_id ")
							 "FROM `option_map` as a "
							 "join option_map as b on "
							 "a.name_id=b.name_id "
							 "and a.node_id != b.node_id "
							 "and a.parent_node_id=b.parent_node_id "
							 "order by a.parent_node_id", &result )
			 ; result
			  ; GetSQLResult( &result ) )
		{
			INDEX id_parent;
			INDEX id = atol( result );
			sscanf( result, WIDE("%ld,%ld"), &id, &id_parent );
			if( !original_parent )
			{
				original_parent = id_parent;
				new_root_node_id = id;
				continue;
			}
			if( id_parent != original_parent )
			{
				original_parent = id_parent;
				new_root_node_id = id;
				continue;
			}

				graphted = 1;
			if( !do_fix )
			{
            lprintf( WIDE("....") );
			}
			else

			{
				DoSQLCommandf( WIDE("update option_map set parent_node_id=%ld where parent_node_id=%ld")
								 , new_root_node_id
								 , id );

				DoSQLCommandf( WIDE("delete from option_map where node_id=%ld"), id );
			}

									  //lprintf( WIDE("update option_map set parent_node_id=%ld where parent_node_id=%ld")
									  //			  , new_root_node_id
									  //			  , id );
									  //lprintf( WIDE("delete from option_map where node_id=%ld"), id );
		}
	} while( graphted );

}



void FixOrphanedBranches( void )
{
	PLIST options = CreateList();
CTEXTSTR *result = NULL;
   CTEXTSTR singleresult;
	DoSQLQuery( WIDE("select count(*) from option_map"), &singleresult );
   // expand the options list to max extent real quickk....
	SetLink( &options, atoi( singleresult ) + 1, 0 );
	for( DoSQLRecordQuery( WIDE("select node_id,parent_node_id from option_map"), NULL,&result,NULL );
		  result;
		  GetSQLRecord( &result ) )
	{
		INDEX node_id, parent_node_id;
	   node_id=atol( result[0] );
      parent_node_id=atol(result[1]);
		//sscanf( result, WIDE("%ld,%ld"), &node_id, &parent_node_id );
      SetLink( &options, node_id, (POINTER)(parent_node_id+1) );
	}
	{
		INDEX idx;
      int deleted;
		uint32_t parent;
		do
		{
			deleted = 0;
			LIST_FORALL( options, idx, uint32_t, parent )
			{
				if( (parent > 1) && !GetLink( &options, parent-1 ) )
				{
					deleted = 1;
					lprintf( WIDE("node %ld has parent id %ld which does not exist."), idx, parent-1 );
					SetLink( &options, idx, NULL );
               if( do_fix )
						DoSQLCommandf( WIDE("delete from option_map where node_id=%ld"), idx );
				}
			}
		}while( deleted );
	}
}

void FixOrphanedValues( void )
{
	CTEXTSTR result;
	for( DoSQLQuery( "select value_id from option_values left  join option_map using (value_id) where node_id is NULL"
						, &result )
		 ; result
		  ; GetSQLResult( &result ) )
	{
      printf( "Deleting unused value %s\n", result );
      DoSQLCommandf( "delete from option_values where value_id=%s", result );
	}
}

int main( int argc, char *argv[] )
{
   do_fix=(argc>1);
	FixOptionNames();
	FixDuplicateOptionNodes();
	FixOrphanedBranches();
   FixOrphanedValues();
   return 0;
}

