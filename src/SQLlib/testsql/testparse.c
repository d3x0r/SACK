#include <stdhdrs.h>
#include <sack_types.h>
#ifdef __cplusplus
#ifdef __WATCOMC__
#include <io.h>
#endif
#endif
#include <pssql.h>

char *strings[] = { "CRAETE TABLE blah"
						, "Create TaBlE if not exists blah"
                  , "Create tempor table if not exi db.blah"
                  , "Create table db.blah"
                  , "Create table db.blah (column type(stuf) extra)"
                  , "Create table `db test.blah` (column type(stuf) extra)"
                  , "Create table `table name` (column type(stuf) extra)"
                  , "Create table `table name` (ID int(11) PRIMARY KEY, data int)"
                  , "Create table `table name` (ID int(11), data int)"
                  , "Create table `table name` (ID int(11), text varchar(100) default 'abc,123)' )"
                  , "Create table `table name` (column type(stuf) extra, PRIMARY KEY ( `column` ) )"
						, "Create table `table name` (column type(stuf) extra, unique KeY ( `col1`,`col2`,`col3`))"
						, "CrEaTE TaBlE `another table name` ( thing enum('a','b','c') NOT NULL default 'a')"
						, "CREATE TABLE `brain_info` ( `brain_info_id` int(11) NOT NULL auto_increment, `brain_name` varchar(100) NOT NULL default '', `version` int(11) NOT NULL default '0', `k` double NOT NULL default '0', PRIMARY KEY  (`brain_info_id`) ) TYPE=MyISAM;"
						, NULL };


static void DumpSQLTable( PTABLE table )
{
	int n;
	int m;
// don't release tables created statically in C files...
	printf( "Table name: %s\n", table->name );
	for( n = 0; n < table->fields.count; n++ )
	{
		printf( "   Column %d '%s' [%s] [%s]\n"
              , n
				 ,( table->fields.field[n].name )
				 ,( table->fields.field[n].type )
				 ,( table->fields.field[n].extra )
				 );
		for( m = 0; table->fields.field[n].previous_names[m] && m < MAX_PREVIOUS_FIELD_NAMES; m++ )
		{
         //Release( (POINTER)table->fields.field[n].previous_names[m] );
		}
	}
	for( n = 0; n < table->keys.count; n++ )
	{
		printf( "    Key %s", table->keys.key[n].name?table->keys.key[n].name:"<NONAME>" );
		for( m = 0; table->keys.key[n].colnames[m] && m < MAX_KEY_COLUMNS; m++ )
		{
			printf( "Key part = %s"
					 , ( table->keys.key[n].colnames[m] )
					 );
		}
      printf( "\n" );
	}
}

void TestAllInFile( char *filename )
{
	FILE *file;
		file = fopen( filename, WIDE("rt") );
		if( file )
		{
			int done = FALSE;
			int gathering = FALSE;
			TEXTCHAR *buf;
			char fgets_buf[4096];
			TEXTCHAR cmd[4096];
			int nOfs = 0;
			lprintf( WIDE("Opened %s to read"), filename );
			while( fgets( fgets_buf, sizeof( fgets_buf ), file ) )
			{
				TEXTCHAR *p;
#ifdef __cplusplus_cli
				buf = DupCStr( fgets_buf );
#else
				buf = fgets_buf;
#endif
				p = buf + strlen( buf ) - 1;
				while( p[0] == ' ' || p[0] == '\t' || p[0] == '\n' || p[0] == '\r')
				{
					p[0] = 0;
					p--;
				}
				p = buf;
				while( p[0] )
				{
					if ( p[0] == '#' )
					{
						p[0] = 0;
						break;
					}
					p++;
				}
				p = buf;
				while( p[0] == ' ' || p[0] == '\t' )
					p++;
				if( p[0] )
				{
					// have content on the line...
					TEXTCHAR *end = p + strlen( p ) - 1;
					if( gathering && end[0] == ';' )
					{
						done = TRUE;
						end[0] = 0;
					}
					if( !gathering )
					{
						if( strnicmp( p, WIDE("CREATE"), 6 ) == 0 )
						{
							gathering = TRUE;
						}
					}
					if( gathering )
					{
						nOfs += sprintf( cmd + nOfs, WIDE("%s "), p );
						if( done )
						{
							// result is set with the first describe result
							// the matching done in CheckMySQLTable should
							// be done here, after parsing the line from the file (cmd)
							// into a TABLE structure.
							//DebugBreak();
							{
								PTABLE table;
								lprintf( "parse: %s", cmd );
								table = GetFieldsInSQL( cmd, 1 );
                        CheckODBCTable( NULL, table, CTO_MERGE|CTO_LOG_CHANGES );
								DestroySQLTable( table );
							}
							done = FALSE;
							gathering = FALSE;
                     nOfs = 0;
						}
					}
				}
#ifdef __cplusplus_cli
				Release( buf );
#endif
			}
			lprintf( WIDE("Done with create...") );
			fclose( file );
  	}

}


int main( int argc, char **argv )
{
	PTABLE result;
	int n;
	if( argc < 2 )
	{
	}
	else
	{
      unlink( "changes.sql" );
      TestAllInFile( argv[1] );
	}
   /*
	for( n = 0; strings[n]; n++ )
	{
		fprintf( stdout, "Parsing: %s\n", strings[n] );
		result = GetFieldsInSQL( strings[n], 0 );
		if( result )
		{
			fprintf( stdout, "Success... (dump table?)" );
         DumpSQLTable( result );
			DestroySQLTable( result );
		}
      else
			fprintf( stderr, "Failed parsing.. \n"
					 );
					 }
               */
   //TestAllInFile( "mkdb.mysql" );
   return 0;
}

