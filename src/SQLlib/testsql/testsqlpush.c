
#include <stdhdrs.h>
#include <pssql.h>
//#include <sqlstub.h>
#define TMP_TEST_NAME "tmp_test"

int ReadNameTableTest(  char * table, char * col )
{
	int retval = 0;
	char * pch = NULL;
   CTEXTSTR sequelresult = NULL;

	xlprintf(LOG_NOISE)("Welcome to ReadNameTableTest for %s %s"
			  , table
			  , col
			  );

   ///GetCurrentSystemID? We don't need no stinkin' GetCurrentSystemID!  Heh.

	if(! ( DoSQLQueryf( &sequelresult
							, "SELECT user()"
							)
		  )
	  )
	{
		xlprintf(LOG_NOISE)("Whoa! Could not select user().  Bogus!");
	}
	else
	{
		if( sequelresult )
		{
			xlprintf(LOG_NOISE)("The user is --> %s", sequelresult );
			pch = strchr( sequelresult, '@' );
			pch++; //well move past that silly at sign.
			xlprintf(LOG_NOISE)("Found %s, is that something?"
									 , pch
									 );
         retval = 1;
		}
		else
		{
			xlprintf(LOG_ALWAYS)("Grr. sequelresult was null? ReadNameTableTest fails.");
         retval = 0;
		}
	}


	retval = ReadNameTableExx(  pch
									 , table
									 , col
									 , "name"
                            , TRUE
									 );
   return retval;
}

int TripleRecordQueryTest(void)
{
   int rezultz = 0;
	int results = 0;
	int rslts = 0;
	uint32_t z = 0;
	uint32_t y = 0;
   uint32_t x = 0;
   CTEXTSTR * sequelrezultz = NULL  ;
   CTEXTSTR * sequelresults = NULL;
	CTEXTSTR * sqlrslts = NULL;

   xlprintf(LOG_NOISE)("Welcome to TripleQueryTest");

	for( (   DoSQLRecordQuery( "SELECT program_name,program_id FROM program_identifiers"
									 , &rslts
									 , &sqlrslts
									 , NULL ));
		  sqlrslts  ;
		  GetSQLRecord(  &sqlrslts )
		)
	{
      z++;
		xlprintf(LOG_NOISE)("z is %d"
								 , z
								 );

		if( ( sqlrslts[0] ) && ( strlen( sqlrslts[0] )> 0 )  )
		{
         y++ ;

			PushSQLQuery();

			for( (   DoSQLRecordQuery( "SELECT name,system_id FROM systems"
											 , &results
											 , &sequelresults
											 , NULL ));
				  sequelresults  ;
				  GetSQLRecord(  &sequelresults )
				)
			{
				if( (  sequelresults[0] ) && ( strlen( sequelresults[0] ) > 0 ) )
				{
					//if( !( strcmp(sequelresults[0], duhname ) ) )
					{
                  xlprintf(LOG_NOISE)("Ok, got a good result. Pushing second query");
                  x++;
						PushSQLQuery();

						for( (   DoSQLRecordQuery( "SELECT name,test_id FROM tmp_test"
														 , &rezultz
														 , &sequelrezultz
														 , NULL ));
							  sequelrezultz  ;
							  GetSQLRecord(  &sequelrezultz )
							)
						{

							if( ( sequelrezultz[0] ) && ( strlen( sequelrezultz[0] ) > 1 ) )
							{
								xlprintf(LOG_NOISE)("z:%d y:%d x:%d < %s %s %s >"
														 , z
														 , y
														 , x
														 , sequelrezultz[0]
														 , sequelresults[0]
														 , sqlrslts[0]
                                            );
							}

						}

						PopODBC();

					}
				}
				else
				{
					xlprintf(LOG_NOISE)("Oops.  Got a bad result! Skipping");
				}

			}
			PopODBC();
		}
		else
		{
			xlprintf(LOG_NOISE)("Oops. Sometimes, there is a NULL in the program_name field in the program_identifiers table.  Skip this one.");
		}
	}

   return z;
}


int PushSQLQueryTest(void)
{
   int retval = 1;
	int rslts = 0;
   int z = 0;
   CTEXTSTR * sequelresults = NULL;
	CTEXTSTR * sqlrslts = NULL;
	char duhname[128];
   char duhaddress[128];

   xlprintf(LOG_NOISE)("Welcome to PushSQLQueryTest");

	for( (   DoSQLRecordQuery( "SELECT name,system_id FROM systems"
									 , &rslts
									 , &sqlrslts
									 , NULL ));
		  sqlrslts  ;
		  GetSQLRecord(  &sqlrslts )
		)
	{
      z++;
		xlprintf(LOG_NOISE)("z is %d"
								 , z
								 );

		if( ( sqlrslts[0] ) && ( strlen( sqlrslts[0] )> 0 )  )
		{
         uint32_t tick;
			xlprintf(LOG_NOISE)("sqlrslts[0] is %s", sqlrslts[0] );
         tick = GetTickCount();
			strcpy( duhname, sqlrslts[0] );
			snprintf(duhaddress, (sizeof( duhaddress )), "%ld"
					  , tick
					  );
			xlprintf(LOG_NOISE)("Got %s and made %s", duhname, duhaddress );

			PushSQLQuery();

			if (! ( DoSQLRecordQueryf( NULL
											 , &sequelresults
											 , NULL
											 , "SELECT address,test_id FROM tmp_test WHERE name='%s'"
											 , duhname
											 ) &&  sequelresults
					)
				)
			{
				if( !( DoSQLCommandf( "INSERT INTO `tmp_test` (name,address) VALUES('%s','%s' )"
										  , duhname
										  , duhaddress
										  )
					  )
               )
				{

					xlprintf(LOG_NOISE)("Fubar plain and simple. Returning zero");
               retval = 0;
				}
				else
				{
					xlprintf(LOG_NOISE)("Inserted I guess,  for %s got test_id of %d ?"
                                   , duhname
											 , ( GetLastInsertID( "tmp_test", "name" ) )
											 );
				}

			}
			else
			{

				xlprintf(LOG_NOISE)("The test_id for %s is %s", duhname, sqlrslts[1] );

			}
			PopODBC();
		}
	}

   return retval;

}

int ShowOrDeleteTableTest( LOGICAL torf)
{
	int retval = 0;

   if( torf )
	{

		// watcom C requires these to be static so that they are constant.
		// otherwise, the address would actually be populated on the stack, and initialized
      // with data on function entrance... instead of being static in program data space.
		static FIELD test_fields[] = { { "test_id", "int","NOT NULL auto_increment" }
									 , { "name", "varchar(65)", "NOT NULL" }
									 , { "address", "varchar(65)", "NOT NULL" }
		};


		static DB_KEY_DEF keys[] = {
#ifdef __cplusplus
			required_key_def( TRUE, FALSE, NULL, "test_id" )
#else
			{  .flags={.bPrimary=1}, NULL, KEY_COLUMNS("test_id") }
#endif
		};
		TABLE test_table = {	"tmp_test"
								 , FIELDS( test_fields )
								 , TABLE_KEYS( keys )
		};
		xlprintf(LOG_NOISE)("About to CheckODBTable for test_table");
//		PRELOAD( CreateTables )
		{
			CheckODBCTable( NULL, &test_table, 0 );
		}


      xlprintf(LOG_NOISE)("Finished with CheckODBTable for test_table");
      retval = 1;

	}
	else
	{
		if( !( DoSQLCommand( "DROP TABLE tmp_test")
			  )
		  )
		{
         retval = 0;
		}
		else
		{
         retval = 1;
		}

	}

   return retval;

}

int CreateTableTest( void )
{
	int retval = 0;

	if( !( DoSQLCommandf( "CREATE TABLE `%s` ( \
								`test_id` int(11) NOT NULL auto_increment,      \
								`name` varchar(64) NOT NULL default '',           \
								`address` varchar(64) NOT NULL default '',        \
								PRIMARY KEY  (`test_id`),                       \
								KEY `namekey` (`name`)                            \
							  ) ENGINE=MyISAM DEFAULT CHARSET=latin1"
							  , TMP_TEST_NAME
							  )
		  )
	  )
	{
		retval = 0;
	}
	else
	{
		retval = 1;
	}

	return retval;
}



void DoEvenSomethingElse( PODBC odbc )
{
   CTEXTSTR result = NULL;
	PushSQLQueryEx( odbc );
#define stuff "12 + 5 / 2"
	for( SQLQuery( odbc, WIDE("select ")stuff, &result );
		 result;
		  FetchSQLResult( odbc, &result ) )
	{
      lprintf( WIDE("result of")stuff" = %s", result );
	}
   PopODBCEx( odbc );
}

void DoSomethingElse( PODBC odbc )
{
   CTEXTSTR result = NULL;
	PushSQLQueryEx( odbc );
	for( SQLQuery( odbc, WIDE("select 5 * 3"), &result );
		 result;
		  FetchSQLResult( odbc, &result ) )
	{
      DoEvenSomethingElse( odbc );
      lprintf( WIDE("result of 5*3 = %s"), result );
	}
   PopODBCEx( odbc );
}

void DoSomething( PODBC odbc )
{
   CTEXTSTR result = NULL;
	PushSQLQueryEx( odbc );
	for( SQLQuery( odbc, WIDE("select * from systems"), &result );
		 result;
		  FetchSQLResult( odbc, &result ) )
	{
      char cmd[256];
      lprintf( WIDE("Adding a new system address into the table...") );
      sprintf( cmd, WIDE("insert into systems (address) values ('%d')"), GetTickCount() );
      SQLCommand( odbc, cmd );
      lprintf( WIDE("Got %s from system"), result );
      DoSomethingElse( odbc );
	}
   PopODBCEx( odbc );
}



int main ( int argc, char **argv )
{
   /// do stuff

// PODBC odbc = ConnectToDatabase( WIDE("lMySQL") );
	PODBC odbc = ConnectToDatabase( WIDE("MySQL") );

	xlprintf(LOG_ADVISORY)("Welcome to testsql3");

	if( odbc )
	{
      int retval = 0;

		if( argc > 2 )
		{
			if (!( strcmp( argv[1], "fresh" ) ) )
			{
            xlprintf(LOG_NOISE)("Dropping the table because fresh is needed");
				retval = ShowOrDeleteTableTest(FALSE);
				xlprintf(LOG_NOISE)("ShowOrDeleteTableTest(FALSE) returned a %d", retval );

			}
			if (!( strcmp( argv[1], "old" ) ) )
			{

            xlprintf(LOG_NOISE)("Using the old table, the existing one.");
				retval = ShowOrDeleteTableTest(TRUE);
				xlprintf(LOG_NOISE)("ShowOrDeleteTableTest(TRUE) returned a %d", retval );

			}

			if(  (! ( strcmp( argv[2], "all" )))
			  || (! ( strcmp( argv[2], "queryf" )))
			  )
			{
				if (retval )
				{

					retval = PushSQLQueryTest();
					xlprintf(LOG_NOISE)(" PushSQLQueryTest returned a %d", retval );

				}
				else
				{
					xlprintf(LOG_NOISE)("Wow.  PushSQLQueryfTest failed, let's try to manually create the table..");
					retval = CreateTableTest( );
					xlprintf(LOG_NOISE)("CreateTableTest returned a %d, did it work?", retval);
				}
			}
			if(  (! ( strcmp( argv[2], "all" )))
			  || (! ( strcmp( argv[2], "readname" )))
			  )
			{


				xlprintf(LOG_NOISE)("Testing for ReadNameTable");

				retval = ReadNameTableTest(  TMP_TEST_NAME, "test_id");
            xlprintf(LOG_NOISE)(" ReadNameTableTest returned a %d", retval);
			}
			if(  (! ( strcmp( argv[2], "all" )))
			  || (! ( strcmp( argv[2], "triple" )))
			  )
			{

				if(!( strcmp( argv[1], "old" ) ) )
				{
					xlprintf(LOG_NOISE)("Testing for TripleRecordQuery");

					retval = TripleRecordQueryTest();
					xlprintf(LOG_NOISE)(" TripleRecordQueryTest returned a %d", retval);
				}
				else
				{
               xlprintf(LOG_NOISE)("Oops.  Can't do fresh and triple at the same time.  Sorry it didn't work out.");
				}
			}

		}
		else
		{
			xlprintf(LOG_ALWAYS)("Please use the following syntax: testsql3 [fresh|old] [all|queryf|readname|triple]");
			printf("\nPlease use the following syntax: testsql3 [fresh|old] [all|queryf|readname|triple]\n");
         return 0;

		}

	}

   xlprintf(LOG_ADVISORY)("Thank you for using testsql3.  Goodbye.");
   return 0;
}


