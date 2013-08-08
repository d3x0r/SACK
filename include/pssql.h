/* more documentation at end */

/*
 *
 *   Creator: Panther   #implemented in Dekware
 *   Modified by: Jim Buckeyne #ported to service SQL via proxy.
 *   Returned to sack by: Jim Buckeyne
 *                  # stripped application specific
 *                  # features, returned to SACK.
 *
 *  Provides a simple, intuitive interface to SQL.  Used sensibly,
 *  provides garbage collection of resources.
 *
 *  Commands without an ODBC specifier are the perferred method to
 *  use this interface.  This allows the internal system to maintain
 *  a primary and a redundant backup connection to provide transparent
 *  reliability to the application.
 *
 *  Provides some slick table creation routines
 *     - check for existance, and drop  (CTO_DROP)
 *     - check for existance, and match (CTO_MATCH)
 *     - check for existance, and merge (CTO_MERGE)
 *     - create table if not exist.
 *
 *  Latest additions provide ...RecordQuery... functions which
 *  result with a const CTEXTSTR * of results;  (ie, result[0] = (CTEXTSTR)result1 )
 *  also available are the column names from the query.
 *  I strongly recommend passing NULL always to the field names, and
 *  using sensible enumerators that follow the query definition.
 *
 *  (c)Freedom Collective (Jim Buckeyne <2000-2006)
 *
 */


#ifndef PSSQL_STUB_DEFINED
/* multiple inclusion protection symbol */
#define PSSQL_STUB_DEFINED
#include <sack_types.h>

#if defined( SQLSTUB_SOURCE ) || defined( SQLPROXY_LIBRARY_SOURCE )
#define PSSQL_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define PSSQL_PROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifdef __cplusplus
#define _SQL_NAMESPACE   namespace sql {
#define _SQL_NAMESPACE_END   }
#define SQL_NAMESPACE   namespace sack { namespace sql {
#define SQL_NAMESPACE_END } }
#else
#define _SQL_NAMESPACE   
#define _SQL_NAMESPACE_END
#define SQL_NAMESPACE   
#define SQL_NAMESPACE_END 
#endif

SACK_NAMESPACE
/* SQL access library. This provides a simple access to ODBC
   connections, and to sqlite. If no database is specified,
   there is an internal database that can be used. These methods
   on the PODBC connection are NOT thread safe. Multiple threads
   shall never use the same PODBC; they can use seperate PODBC
   connections. Under linux this links to unixODBC.
   
   
   
   DoSQLCommandf
   
   DoSQLRecordQueryf
   
   GetSQLRecord
   
   
   
   ConnectToDatabase
   
   DoSQLCommandf
   
   DoSQLRecordQueryf
   
   FetchSQLRecord
   
   
   
   There is a configuration file for the default SQL connection,
   this is kept in a file 'sql.config' which is processed with
   ProcessConfigurationFile(); If this file does not exist, it
   will be automatically created with default values.
   
   
   
   (Need to describe this sql.config file)                       */
_SQL_NAMESPACE


/* <combine PSSQL_PROC>
   
   \ \                    */
#define SQLPROXY_PROC PSSQL_PROC
/* This is the connection object that provides interface to the
   database. Can be NULL to specify the default connection
   interface. See namespace <link sack::sql, sql>.
   
   
   
   An ODBC connection handles commands as a stack. Each command
   is done as a temporary entry on the stack. A query is done as
   an entry on the stack, but the entry remains on the stack
   until the final result is retrieved or an early PopODBC is
   called.
   
   
   
   The structure of this is such that if a command is slow to a
   database, it would be possible to stack commands that are
   temporary and pending until the database connection is
   restored.
   
   
   Example
   <code lang="c++">
   int f( void )
   {
       // results from the query
       CTEXTSTR *results;
       // connect.
       PODBC odbc = ConnectToDatabase( "system_dsn_name" );
       // do a command, does a temporary entry on the stack, unless the database is slow
       SQLCommandf( odbc, "create temporary table my_test_table( ID int, value int )" );
       // start a new entry on the command stack.
       SQLRecordQueryf( odbc, NULL, &amp;results, NULL, "select 1+1" );
       // when this command is done, it is stacked on the query.
       SQLCommandf( odbc, "insert into my_test_table (value) values(%d)", 1234 );
       // at this point there is technically 2 entries on the command stack until the next
       // FetchSQLResult( odbc, &amp;results );
   }
	</code>                                                                                 */

#if !defined( ALTANIK_SQL_STUB_DEFINED ) || !defined( __GNUC__ )
   // GCC doesn't identify this as exactly the same declaration
	typedef struct odbc_handle_tag *PODBC;
#endif
typedef struct odbc_handle_tag ODBC;

// recently added {} container braces for structure element
#define FIELDS(n) {( sizeof( n ) / sizeof( FIELD ) ), n}

/* a field definition can be a rename, and contain prior names,
   so that the rename can be tracked and migrated appropraitely.
   Unfortuntaly this sort of operation only affects this code,
   and not all auxiliary code.                                   */
#define MAX_PREVIOUS_FIELD_NAMES 4
/* <combine sack::sql::required_field_tag>
   
   <code lang="c++">
     FIELD fields[] = { { "ID", WIDE("int") }, ... };
   </code>                                            */
typedef struct required_field_tag
{
	/* This is the name of the column described in this table. */
	CTEXTSTR name;
	/* pointer to a string describing the type of this column.  */
	CTEXTSTR type;
	/* extra information about the field... grab all addtional
	   information like 'NOT NULL' "default 'zxa'" to describe a
	   field. Sometimes target databases don't understand extra
	   \parameters, and these can be translated as required or
	   ignored.                                                  */
	CTEXTSTR extra;
	// if you have renamed this column more than 1
	// times - you really need to stop messing around
	// and get a life.
	CTEXTSTR previous_names[MAX_PREVIOUS_FIELD_NAMES];
} FIELD, *PFIELD;

#if !defined( _MSC_VER ) || ( _MSC_VER >= 800 )
/* A macro to append a NULL automatically to a list of strings.
   Example
   <code lang="c++">
   CTEXTSTR strings[] = { KEY_COLUMNS( "one", "two", "three" ) };
   </code>
   strings will be set to 4 elements with the 3 strings listed
   in KEY_COLUMNS plus a NULL string.                             */
#define KEY_COLUMNS(...) { __VA_ARGS__, NULL }
#endif
/* sets the count and the array of a statically declared
   required_table_tag.
   
   
   Example
   <code lang="c++">
   
   </code>
   <code>
   FIELD fields[5];
   DB_KEY_DEF keys[3];
   TABLE table = { "table_name", FIELDS( fields ), TABLE_KEYS( keys ) };
   </code>
   
   This creates a static table definition with the name
   "table_name" and 5 fields with 3 keys. fields[] = { } is
   usally the declartion. Also DB_KEY_DEF keys[] = { ... }; for
   keys.
                                                                         */
#define TABLE_KEYS(n) {( sizeof( n ) / sizeof( DB_KEY_DEF ) ), n}
/* maximum columns that can be specified for a multicolumn index
   in required_key_def.                                          */
#define MAX_KEY_COLUMNS 8

/* <combine sack::sql::required_key_def>
   
   \ \                                   */
typedef struct required_key_def  DB_KEY_DEF;
/* <combine sack::sql::required_key_def>
   
   \ \                                   */
typedef struct required_key_def  *PDB_KEY_DEF;
struct required_key_def
{
	/* Flags describing attributes of this key */
	/* <combine sack::sql::required_key_def::flags@1>
	   
	   \ \                                            */
	struct {
		/* this defines the primary key for the table */
		BIT_FIELD bPrimary : 1;
		/* the key is meant to be unique. */
		BIT_FIELD bUnique : 1;
	} flags;
	/* Name of the key column. Can be NULL if primary. */
	CTEXTSTR name;
   CTEXTSTR colnames[MAX_KEY_COLUMNS]; // uhm up to 5 colnames...
   CTEXTSTR null; // if not null, broken structure...
#ifdef __cplusplus
   /* <combine sack::sql::required_key_def>
      
      This is used when actually building C++ for providing an
      initializer to this structure.                           */
   required_key_def( int bPrimary, int bUnique, CTEXTSTR _name, CTEXTSTR colname1 ) { flags.bPrimary = bPrimary; flags.bUnique = bUnique; name = _name; colnames[0] = colname1; colnames[1] = NULL; }
   /* <combine sack::sql::required_key_def>
      
      This is used when actually building C++ for providing an
      initializer to this structure.                           */
   required_key_def( int bPrimary, int bUnique, CTEXTSTR _name, CTEXTSTR colname1, CTEXTSTR colname2 ) { flags.bPrimary = bPrimary; flags.bUnique = bUnique; name = _name; colnames[0] = colname1; colnames[1] = colname2; colnames[2] = 0; }
	/* Just another required_key_def constructor. */
	required_key_def( int bPrimary, int bUnique, CTEXTSTR _name, CTEXTSTR colname1, CTEXTSTR colname2, CTEXTSTR colname3 ) { flags.bPrimary = bPrimary; flags.bUnique = bUnique; name = _name; colnames[0] = colname1; colnames[1] = colname2; colnames[2] = colname3; colnames[3] = 0; }
#else
#define required_key_def( a,b,c,...) { {a,b}, c, {__VA_ARGS__} }
#endif
}; /* Describes a key column of a table.
      <code lang="c++">
      
      DB_KEY_DEF keys[] = { { "lockey", KEY_COLUMNS("hall_id","charity_id") } };
      
      </code>                                                                    */

/* <combine sack::sql::required_constraint_def>
   
   \ \                                   */
typedef struct required_constraint_def  DB_CONSTRAINT_DEF;
/* <combine sack::sql::required_constraint_def>
   
   \ \                                   */
typedef struct required_constraint_def  *PDB_CONSTRAINT_DEF;
struct required_constraint_def
{
	struct {
		BIT_FIELD cascade_on_delete : 1;
		BIT_FIELD cascade_on_update : 1;
		BIT_FIELD restrict_on_delete : 1;
		BIT_FIELD restrict_on_update : 1;
		BIT_FIELD noaction_on_delete : 1;
		BIT_FIELD noaction_on_update : 1;
		BIT_FIELD setnull_on_delete : 1;
		BIT_FIELD setnull_on_update : 1;
		BIT_FIELD setdefault_on_delete : 1;
		BIT_FIELD setdefault_on_update : 1;
	} flags;

	CTEXTSTR name;
    CTEXTSTR colnames[MAX_KEY_COLUMNS]; // uhm up to 5 colnames...
	CTEXTSTR references;
    CTEXTSTR foriegn_colnames[MAX_KEY_COLUMNS]; // uhm up to 5 colnames...
    CTEXTSTR null; // if not null, broken structure...
}; // Describes a constraint clause

/* Example
   By default, CreateTable( CTEXTSTR tablename, CTEXTSTR
   filename ) which reads a 'create table' statement from a file
   to create a table, this now parses the create table structure
   into an internal structure TABLE which has FIELDs and
   DB_KEY_DEFs. This structure is now passed to CheckODBCTable
   which is able to compare the structure with the table
   definition available from the database via DESCRIBE TABLE,
   and then update the table in the database to match the TABLE
   definition.
   
   One can use the table structure to define tables instead of
   maintaining external files... and without having to create a
   temporary external file which could then contain a create
   table statement to create the table.
   <code>
   // declare some fields...
   FIELD some_table_field_array_name[] = { { "field one", "int", NULL }
   , { "field two", "varchar(100)", NULL }
   , { "ID field", "int", "auto_increment" }
   , { "some other field", "int", "NOT NULL default '8'" }
   };
   
   // define some keys...
   DB_KEY_DEF some_table_key_array_name[] = { { .flags = { .bPrimary = 1 }, NULL, {"ID Field"} }
   , { {0}, "namekey", { "field two", NULL } }
   };
   </code>
   
   // the structure for DB_KEY_DEF takes an array of column
   names used to define the key, there should be a NULL to end
   the list. The value after the array of field names is called
   'null' which should always be set to NULL. If these are
   declared in global data space, then any unset value will be
   initialized to zero.
   
   <code>
   TABLE some_table_var_name = { "table name", FIELDS( some_table_field_array_name ), TABLE_KEYS( some_table_key_array_name ), 1 );
   
    LOGICAL CheckODBCTable( PODBC odbc, PTABLE table, _32 options )
        PODBC odbc - may be left NULL to use the default database connection.
        PTABLE table - a pointer to a TABLE structure which has been initialized.
        _32 options - zero or more of  the following symbols or'ed together.
                   \#define CTO_MATCH 4  // attempt to figure out alter statements to drop or add columns to exact match definition
                   \#define CTO_MERGE 8  // attempt to figure out alter statements to add missing columns, do not drop.  Rename?
   
   </code>
   
   Then some routine later
   <code>
   
   {
      ...
      CheckODBCTable( NULL, &amp;some_table_var_name, CTO_MERGE );
      ..
   }
   </code>
   
   * ---------------------------------------------------------- *
   
   alternatively tables may be checked and updated using the
   following code, given an internal constant text string that
   is the create table statement, this may be parsed into a
   PTABLE structure which the resulting table can be used in
   CheckODBCTable();
   <code>
   
   static CTEXTSTR create_player_info = "CREATE TABLE `players_info` ("
         "  `player_id` int(11) NOT NULL auto_increment,           "
         "  PRIMARY KEY  (`player_id`),                            "
         ")                               ";
   
   PTABLE table = GetFieldsInSQL( create_player_info, FALSE );
   CheckODBCTable( NULL, table, CTO_MERGE );
   DestroySQLTable( table );
   </code>                                                                                                                          */
struct required_table_tag
{
	/* This is the name of the table. */
	CTEXTSTR name;
	/* describes the columns (fields) in a table. */
	struct pssql_table_fields {
		/* number of fields in the array pointed at by field. */
		int count;
		/* pointer to an array of FIELD. */
		PFIELD field;
	} fields;
	/* Describes the keys on the table.  */
	/* <combine sack::sql::required_table_tag::keys@1>
	   
	   \ \                                             */
	struct pssql_table_key {
		/* number of keys pointed at by key. */
		int count;
      /* pointer to an array of DB_REQ_KEY. */
      PDB_KEY_DEF key;
	} keys;

	struct pssql_table_constraint {
		int count;
		PDB_CONSTRAINT_DEF constraint;
	} constraints;
	/* <combine sack::sql::required_table_tag::flags@1>
	   
	   \ \                                              */
	/* flags controlling the table. */
		struct pssql_table_flags {
         // set this if defined dynamically (from getfields in SQL)
		BIT_FIELD bDynamic : 1; 
		/* This is a table that is allocated in memory, static table
		   definitions should leave this 0.                          */
		BIT_FIELD bTemporary : 1;
		/* Issue the create statement always, but include 'if not
		   exists'. Don't try and compare the table structure.    */
		BIT_FIELD bIfNotExist : 1;
	} flags;
   /* name of another table that already exists. Creates this table
      using that table's description.                               */
   CTEXTSTR create_like_table_name;
   /* name of the database that contains this table. */
   CTEXTSTR database;
   /* an additional field that can specify the database storage
      engine to use. (Hmm maybe use this to specify sqlite target?) */
   CTEXTSTR type;
   /* This is an additional field to add as a description to the
      database if supported by the target database.              */
   CTEXTSTR comment;
};
/* <combine sack::sql::required_table_tag>
   
   \ \                                     */
typedef struct required_table_tag TABLE;
/* <combine sack::sql::required_table_tag>
   
   \ \                                     */
typedef struct required_table_tag *PTABLE;

/* Checks a table in a database to see if it exists, and that
   all the columns in the table definition passed exist as
   column in the database. Will generate alter statements to the
   table as appropriate.
   Parameters
   odbc :     connection to check the table on
   table :    a table which was created with GetFieldsInSQL, or
              created by filling in a structure.
   options :  Options from CreateTableOptions.                   */
PSSQL_PROC( LOGICAL, CheckODBCTableEx)( PODBC odbc, PTABLE table, _32 options DBG_PASS );
/* Checks a table in a database to see if it exists, and that
   all the columns in the table definition passed exist as
   column in the database. Will generate alter statements to the
   table as appropriate.
   Parameters
   odbc :     connection to check the table on
   table :    a table which was created with GetFieldsInSQL, or
              created by filling in a structure.
   options :  Options from CreateTableOptions.                   */
PSSQL_PROC( LOGICAL, CheckODBCTable)( PODBC odbc, PTABLE table, _32 options );
/* <combine sack::sql::CheckODBCTableEx@PODBC@PTABLE@_32 options>
   
   \ \                                                            */
#define CheckODBCTable(odbc,t,opt) CheckODBCTableEx(odbc,t,opt DBG_SRC )
/* Enable or disable logging SQL to the sql.log file and to the
   application's log.
   Parameters
   odbc :      connection to disable logging on
   bDisable :  if TRUE disables logging, else restores logging. */
PSSQL_PROC( void, SetSQLLoggingDisable )( PODBC odbc, LOGICAL bDisable );



#ifndef SQLPROXY_INCLUDE
// result is FALSE on error
// result is TRUE on success
PSSQL_PROC( int, DoSQLCommandEx )( CTEXTSTR command DBG_PASS);
#endif
/* <combine sack::sql::DoSQLCommandEx@CTEXTSTR command>
   
   \ \                                                  */
#define DoSQLCommand(c) DoSQLCommandEx(c DBG_SRC )

/* Generate a commit for any outstanding transactions. Commit
   syntax is variable depending on the connection. Connections
   also have the feature to auto generate begin transaction, and
   flush after a period of idle.
   Parameters
   odbc :  connection to database to commit                      */
PSSQL_PROC( void, SQLCommit )( PODBC odbc );


// parameters to this are pairs of "name", type, WIDE("value")
//  type == 0 - value is text, do not quote
//  type == 1 - value is text, add quotes appropriate for database
//  type == 2 - value is an integer, do not quote
// the last pair's name is NULL, and value does not matter.
// insert values into said table.
PSSQL_PROC( int, DoSQLInsert )( CTEXTSTR table, ... );

#ifndef SQLPROXY_INCLUDE
/* This opens or re-opens a database connection. Mostly an
   \internal function(?)
   Parameters
   odbc :  connection to open.                             */
PSSQL_PROC( int, OpenSQLConnection )( PODBC );
#endif
/* This opens or re-opens a database connection. Mostly an
   \internal function(?)
   Parameters
   odbc :  connection to open.                             */
PSSQL_PROC( int, OpenSQLConnectionEx )( PODBC DBG_PASS );
/* <combine sack::sql::OpenSQLConnectionEx@PODBC>
   
   \ \                                            */
#define OpenSQLConnect(o) OpenSQLConnectionEx( o DBG_SRC )


// should pass to this a &(CTEXTSTR) which starts as NULL for result.
// result is FALSE on error
// result is TRUE on success, and **result is updated to 
// contain the resulting data.
PSSQL_PROC( int, DoSQLQueryEx )( CTEXTSTR query, CTEXTSTR *result DBG_PASS);
/* <combine sack::sql::DoSQLQueryEx@CTEXTSTR@CTEXTSTR *result>
   
   \ \                                                         */
#define DoSQLQuery(q,r) DoSQLQueryEx( q,r DBG_SRC )
/* <combine sack::sql::DoSQLRecordQueryf@int *@CTEXTSTR **@CTEXTSTR **@CTEXTSTR@...>
   
   \ \                                                                               */
#define DoSQLRecordQuery(q,r,c,f) SQLRecordQueryEx( NULL,q,r,c,f DBG_SRC )
/* <combine sack::sql::SQLRecordQueryEx@PODBC@CTEXTSTR@int *@CTEXTSTR **@CTEXTSTR **fields>
   
   \ \                                                                                      */
#define DoSQLQueryRecord(q,r,c)   DoSQLRecordQuery(q,r,c,NULL)
/* <combine sack::sql::SQLRecordQueryEx@PODBC@CTEXTSTR@int *@CTEXTSTR **@CTEXTSTR **fields>
   
   \ \                                                                                      */
#define SQLQueryRecord(o,q,r,c)   SQLRecordQuery(o,q,r,c,NULL)
/* <combine sack::sql::GetSQLRecord@CTEXTSTR **>
   
   \ \                                           */
#define GetSQLResultRecord(r,c)   GetSQLRecord(c)

/* <combine sack::sql::FetchSQLResult@PODBC@CTEXTSTR *>
   
   \ \                                                  */
PSSQL_PROC( int, GetSQLResult )( CTEXTSTR *result );
/* <combine sack::sql::FetchSQLRecord@PODBC@CTEXTSTR **>
   
   \ \                                                   */
PSSQL_PROC( int, GetSQLRecord )( CTEXTSTR **result );
/* Gets the last result on the default ODBC connection.
   Parameters
   result\ :  address of a string pointer to get set to the error
              string.
   
   Example
   <code>
   {
      CTEXTSTR error;
      GetSQLError( &amp;error );
      printf( "Error: %s", error );
   }
   </code>                                                        */
PSSQL_PROC( int, GetSQLError )( CTEXTSTR *result );
/* This is a test command that tests to see if the default
   database connection is able to work.                    */
PSSQL_PROC( int, IsSQLReady )( void );

/* <combine sack::sql::PushSQLQueryExEx@PODBC>
   
   \ \                                         */
PSSQL_PROC( int, PushSQLQuery )( void );
/* <combine sack::sql::PopODBCEx@PODBC>
   
   \ \                                  */
PSSQL_PROC( void, PopODBC )( void );
#ifndef SQLPROXY_INCLUDE
/* Clear the top non temporary sql statement from the PODBC
   stack.
   Parameters
   odbc :  connection to remove the statement from.
   
   Remarks
   A SQLCommand is temporary, a SQLQuery or a PushODBC is not. Pop
   MAY be used to clear a query early, but it is recommended to
   read to the end of it instead.                                  */
PSSQL_PROC( void, PopODBCExx )( PODBC, LOGICAL DBG_PASS );
PSSQL_PROC( void, PopODBCEx )( PODBC );
/* <combine sack::sql::PopODBCExx@PODBC@LOGICAL>
   
   \ \                                           */
#define PopODBCEx(o) PopODBCExx(o,FALSE DBG_SRC)
/* <combine sack::sql::PopODBCEx>
   
   \ \                            */
#define PopODBC() PopODBCExx(NULL,FALSE DBG_SRC)
#endif
/* This terminates a query on the PODBC stack. (It was mentioned
   in pop odbc that it could be used to terminate a query, but
   that will log that a pop is being done without a push. This
   is the proper way to prematurely end a query.)
   Parameters
   odbc :  connection to end a query on.                         */
PSSQL_PROC( void, SQLEndQuery )( PODBC odbc );
// release any open queries on the database... all result
// sets are now invalid... uhmm what about things like fields?
// could be messy...
PSSQL_PROC( void, ReleaseODBC )( PODBC odbc );

// does a query responce kinda thing returning types.
// if( GetSQLTypes() ) while( GetSQLResult( &result ) && result )
PSSQL_PROC( int, GetSQLTypes )( void );

#ifndef SQLPROXY_INCLUDE
/* parse the string passed as a date/time as returned from a
   MySQL database.
   Parameters
   date :    string to parse
   year :    pointer to an int that will receive the year portion
             of the date
   month :   pointer to an int that will receive the month
             portion of the date
   day :     pointer to an int that will receive the day portion
             of the date
   hour :    pointer to an int that will receive the hours
             portion of the date
   minute :  pointer to an int that will receive the minutes
             portion of the date
   second :  pointer to an int that will receive the second
             portion of the date
   msec :    pointer to an int that will receive the milli\-second
             portion of the date
   nsec :    pointer to an int that will receive the nano second portion
             of the date                                                 */
PSSQL_PROC( void, ConvertSQLDateEx )( CTEXTSTR date
												  , int *year, int *month, int *day
												  , int *hour, int *minute, int *second
												  , int *msec, S_32 *nsec );
#endif
/* <combine sack::sql::ConvertSQLDateEx@CTEXTSTR@int *@int *@int *@int *@int *@int *@int *@S_32 *>
   
   \ \                                                                                             */
#define ConvertSQLDate( date, y,m,d) ConvertSQLDateEx( date,y,m,d,NULL,NULL,NULL,NULL,NULL)
/* <combine sack::sql::ConvertSQLDateEx@CTEXTSTR@int *@int *@int *@int *@int *@int *@int *@S_32 *>
   
   \ \                                                                                             */
#define ConvertSQLDateTime( date, y,mo,d,h,mn,s) ConvertSQLDateEx( date,y,mo,d,h,mn,s,NULL,NULL)


//------------------------------
// this set of functions will auto create a suitable name table
// providing table_name_id and table_name_name as the columns to query by standard
// previous defaults where "id" and "name" which results in inability to use natural join
//
PSSQL_PROC( INDEX, FetchSQLNameID )( PODBC odbc, CTEXTSTR table_name, CTEXTSTR name );
/* A specialized function which takes a name, looks in a SQL
   table on the default database connection for in column
   'name', and returns the value in the 'ID' column. This
   function may create a table with the required fields. This
   table is very bad, if you have 3 tables all with the same
   'name' column reverse engineering and natural join clauses
   fail.
   
   
   Parameters
   table_name :  name of the table to get the name's ID from.
   name :        name to lookup its ID for.
   
   Returns
   the ID of the name or INVALID_INDEX if not found.          */
PSSQL_PROC( INDEX, GetSQLNameID )( CTEXTSTR table_name, CTEXTSTR name );

/* Still a bad function to use.... just don't.
   
   
   Parameters
   odbc :        _nt_
   table_name :  _nt_
   iName :       _nt_                          */
PSSQL_PROC( CTEXTSTR, FetchSQLName )( PODBC odbc, CTEXTSTR table_name, INDEX iName );
/* A specialized function which takes an ID, looks in a SQL
   table on the default database connection for in column 'ID',
   and returns the value in the 'name' column. This function may
   create a table with the required fields. This table is very
   bad, if you have 3 tables all with the same 'name' column
   reverse engineering and natural join clauses fail.
   Parameters
   table_name :  name of the database table to read from
   iName :       ID of the name to get                           */
PSSQL_PROC( CTEXTSTR, GetSQLName )( CTEXTSTR table_name, INDEX iName );

/* <combine sack::sql::SQLReadNameTableExEx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \ 
   Note
   If database connection is not specified or is NULL, uses the
   default SQL connection.                                                                         */
PSSQL_PROC( INDEX, ReadNameTableExEx)( CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS );
/* <combine sack::sql::ReadNameTableExEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                    */
#define ReadNameTableExx( name,table,col,namecol,bCreate) ReadNameTableExEx( name,table,col,namecol,bCreate DBG_SRC )
//column name if NOT specified will be 'ID'
PSSQL_PROC( INDEX, ReadNameTableEx)( CTEXTSTR name, CTEXTSTR table, CTEXTSTR col DBG_PASS );
/* <combine sack::sql::ReadNameTableExEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                    */
#define ReadNameTable(n,t,c) ReadNameTableExEx( n,t,c, WIDE("name"),TRUE DBG_SRC )
/* <combine sack::sql::ReadFromNameTableExEx@INDEX@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR *result>
   
   \ \                                                                                          */
PSSQL_PROC( int, ReadFromNameTableEx )( INDEX id, CTEXTSTR table, CTEXTSTR id_colname, CTEXTSTR name_colname, CTEXTSTR *result DBG_PASS);
/* TRUE if name in result... again if !colname colname = 'ID'
   Parameters
   odbc :       connection to use 
   id :         ID of the name to read
   table :      table to read from
   id_column :  name of the column that contains the ID
   colname :    name of the column that is where the name is
   result\ :    pointer to a CTEXTSTR which will be filled with
                the name in the table                           */
PSSQL_PROC( int, ReadFromNameTableExEx )( INDEX id, CTEXTSTR table, CTEXTSTR id_column, CTEXTSTR colname, CTEXTSTR *result DBG_PASS);
/* <combine sack::sql::ReadFromNameTableExEx@INDEX@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR *result>
   
   \ \                                                                                          */
#define ReadFromNameTableExx(id,t,ic,nc,r) ReadFromNameTableExEx(id,t,ic,nc,r DBG_SRC )
/* <combine sack::sql::ReadFromNameTableEx@INDEX@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR *result>
   
   \ \                                                                                        */
#define ReadFromNameTable(id,t,c,r) ReadFromNameTableEx(id,t,c,WIDE("name"),r DBG_SRC )

/* This is a better name resolution function. It will also
   create a table that contains the required columns, but the
   column names may be more intelligent than 'ID' and 'name'.
   Parameters
   odbc :     database connection to read from
   name :     the name to lookup the ID for
   table :    table the name column is in
   col :      name of the key column(s) to read.
   namecol :  name of column containing the name to lookup.
   bCreate :  if TRUE, will insert the name into the table, and
              return the resulting columns.                     */
PSSQL_PROC( INDEX, SQLReadNameTableExEx)( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS );
/* <combine sack::sql::SQLReadNameTableExEx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                             */
#define SQLReadNameTableExx( odbc,name,table,col,namecol,bCreate) SQLReadNameTableExEx( odbc,name,table,col,namecol,bCreate DBG_SRC )
/* <combine sack::sql::SQLReadNameTableExEx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                             */
#define SQLReadNameTable(o,n,t,c) SQLReadNameTableExEx( o,n,t,c,WIDE( "name" ),TRUE DBG_SRC )

/* Reads a table that's assumed to be a primary key ID and a
   name sort of dictionary table. This also maintains an
   \internal cache of names queried, since it is assumed words
   in a dictionary don't move or change.
   
   
   Parameters
   odbc :      odbc connection to use
   name :      name to get the index of
   table :     table to get the index from
   col :       column name of the ID columns (macros allow this to
               be defaulted)
   namecol :   column name of the name column (macros allow this to
               be defaulted)
   bCreate :   If the name doesn't exist, setting this to TRUE will
               insert the new name, else return will be
               INVALID_INDEX.
   bQuote :    Indicates if the name should be quoted (else use no
               quotes)
   DBG_PASS :  _nt_                                                 */
PSSQL_PROC( INDEX, GetNameIndexExtended)( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate, int bQuote DBG_PASS );
/* <combine sack::sql::GetNameIndexExtended@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int@int bQuote>
   
   \ \                                                                                                */
PSSQL_PROC( INDEX, GetNameIndexExx)( PODBC odbc, CTEXTSTR name, CTEXTSTR table, CTEXTSTR col, CTEXTSTR namecol, int bCreate DBG_PASS );
/* <combine sack::sql::GetNameIndexExx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                        */
#define GetNameIndexEx( odbc,name,table,col,namecol,bCreate) GetNameIndexExx( odbc,name,table,col,namecol,bCreate DBG_SRC )
/* <combine sack::sql::GetNameIndexExx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@CTEXTSTR@int bCreate>
   
   \ \                                                                                        */
#define GetNameIndex(o,n,t,c) GetNameIndexExx( o,n,t,c,WIDE( "name" ),TRUE DBG_SRC )



// table and col are not used if a MySQL backend is used...
// they are needed to get the last ID from a postgresql backend.
PSSQL_PROC( INDEX, GetLastInsertIDEx)( CTEXTSTR table, CTEXTSTR col DBG_PASS );
/* <combine sack::sql::GetLastInsertIDEx@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                          */
#define GetLastInsertID(t,c) GetLastInsertIDEx(t,c DBG_SRC )
/* Gets the ID of the primary key from the prior insert. This
   value can be used in subsequent inserts to relate detail
   records to a master.
   Parameters
   odbc :    database connection
   table :   if NULL, just get's the connection's last insert
             into whatever table. PostgreSQL requires a table
             name and column name to get the last insert for. So,
             proper portability for certain databases may use
             this parameter.
   column :  if NULL, just get's the connection's last insert id
             from the auto increment primary key. PostgreSQL
             requires a table name and column name to get the
             last insert for. So, proper portability for certain
             databases may use this parameter.
   
   Returns
   a 64 bit row identifier.                                       */
PSSQL_PROC( INDEX, FetchLastInsertIDEx)( PODBC odbc, CTEXTSTR table, CTEXTSTR col DBG_PASS );
/* <combine sack::sql::FetchLastInsertIDEx@PODBC@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                                  */
#define FetchLastInsertID(o,t,c) FetchLastInsertIDEx(o,t,c DBG_SRC )
/* <combine sack::sql::FetchLastInsertIDEx@PODBC@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                                  */
#define FetchLastInsertKey(o,t,c) FetchLastInsertKeyEx(o,t,c DBG_SRC )
/* <combine sack::sql::FetchLastInsertIDEx@PODBC@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                                  */
PSSQL_PROC( CTEXTSTR, FetchLastInsertKeyEx)( PODBC odbc, CTEXTSTR table, CTEXTSTR col DBG_PASS );

/* <combine sack::sql::GetLastInsertIDEx@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                          */
PSSQL_PROC( CTEXTSTR, GetLastInsertKeyEx)( CTEXTSTR table, CTEXTSTR col DBG_PASS );
/* <combine sack::sql::GetLastInsertIDEx@CTEXTSTR@CTEXTSTR col>
   
   \ \                                                          */
#define GetLastInsertKey(t,c) GetLastInsertKeyEx(t,c DBG_SRC )


// CreateTable Options (CTO_)
enum CreateTableOptions {
 CTO_DROP  = 1,   // drop old table before create.
 CTO_MATCH = 4,  // attempt to figure out alter statements to drop or add columns to exact match definition
 CTO_MERGE = 8,  // attempt to figure out alter statements to add missing columns, do not drop.  Rename?
		CTO_LOG_CHANGES = 16 // log changes to "changes.sql"
};

/* \ \ 
   Parameters
   odbc :          database connection to check table in
   filename :      name of file containing sql CREATE TABLE
                   statements.
   templatename :  name of the table specified by the CREATE
                   TABLE statement.
   tablename :     table name to use when actually creating this.
                   May be different from template table name.
   options :       Options from CreateTableOptions.               */
PSSQL_PROC( int, SQLCreateTableEx )(PODBC odbc, CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, _32 options );
/* <combine sack::sql::SQLCreateTableEx@PODBC@CTEXTSTR@CTEXTSTR@CTEXTSTR@_32>
   
   \ \                                                                        */
#define SQLCreateTable( odbc, file, table ) SQLCreateTableEx(odbc,file,table,table,0)
/* Creates a table in a database by reading an external file
   containing the table definition. It can also perform
   iterative updates to table structure if the template
   definition adds or deletes columns.
   Parameters
   filename :      filename to read the template from
   templatename :  name of the table in the create table template
                   statement.
   tablename :     the name of the table to create (may be
                   different than template)
   options :       Options from CreateTableOptions.
   
   Returns
   TRUE if success.
   
   FALSE if failure. (No further information)                     */
PSSQL_PROC( int, CreateTableEx )( CTEXTSTR filename, CTEXTSTR templatename, CTEXTSTR tablename, _32 options );
/* <combine sack::sql::CreateTableEx@CTEXTSTR@CTEXTSTR@CTEXTSTR@_32>
   
   \ \                                                               */
#define CreateTable( file, table ) CreateTableEx(file,table,table,0)

// results in a static buffer with escapes filled in for characterws
// which would otherwise conflict with string punctuators.
PSSQL_PROC( TEXTSTR ,EscapeStringEx )( CTEXTSTR name DBG_PASS );
/* <combine sack::sql::EscapeStringEx@CTEXTSTR name>
   
   \ \                                               */
#define EscapeString(s) EscapeStringEx( s DBG_SRC )
/* <combine sack::sql::EscapeStringEx@CTEXTSTR name>
   
   \ \                                               */
#define EscapeStringOpt(s,q) EscapeSQLBinaryExx( NULL,s,StrLen(s),q DBG_SRC )
/* \ \ 
   Parameters
   odbc :  connection to escape the string appropriately for. Different
           database engines require different string escapes.
   name :  string to escape
   
   Returns
   a TEXTSTR that is the content of the string passed properly
   escaped.
   
   it is appropriate to Release( result );
   Example
   This is difficult to describe coorectly, since in C, you have
   to do escaping on the parameters anyhow....
   <code lang="c++">
   {
       TEXTSTR result = EscapeSQLString( "\\"test \\'escape\\'" );
       printf( "original : %s\\n"
               "result   : %s\\n"
             , "\\"test \\'escape\\'"
             , \result );
   }
   </code>
   \Output
   <code lang="c++">
   original : "test 'escape'
   \result   : \\"test \\'escape\\'
   </code>                                                              */
PSSQL_PROC( TEXTCHAR *,EscapeSQLStringEx )( PODBC odbc, CTEXTSTR name DBG_PASS );
/* <combine sack::sql::EscapeSQLStringEx@PODBC@CTEXTSTR name>
   
   \ \                                                        */
#define EscapeSQLString(odbc, s) EscapeSQLStringEx( odbc, s DBG_SRC )

// the following functions return an allcoated buffer which the application must Release()
PSSQL_PROC( TEXTSTR ,EscapeBinaryEx )( CTEXTSTR blob, PTRSZVAL bloblen DBG_PASS );
/* <combine sack::sql::EscapeBinaryEx@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                           */
#define EscapeBinary(b,bl) EscapeBinaryEx(b,bl DBG_SRC )
/* <combine sack::sql::EscapeBinaryEx@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                           */
#define EscapeBinaryOpt(b,bl,q) EscapeSQLBinaryExx(NULL,b,bl,q DBG_SRC )

/* <combine sack::sql::EscapeBinaryEx@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                           */
PSSQL_PROC( TEXTSTR,EscapeSQLBinaryExx )( PODBC odbc, CTEXTSTR blob, PTRSZVAL bloblen, LOGICAL bQuote DBG_PASS );
/* <combine sack::sql::EscapeBinaryEx@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                           */
PSSQL_PROC( TEXTSTR,EscapeSQLBinaryEx )( PODBC odbc, CTEXTSTR blob, PTRSZVAL bloblen DBG_PASS );
/* <combine sack::sql::EscapeSQLBinaryEx@PODBC@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                                    */
#define EscapeSQLBinary(odbc,blob,len) EscapeSQLBinaryEx( odbc,blob,len DBG_SRC )
/* <combine sack::sql::EscapeSQLBinaryEx@PODBC@CTEXTSTR@PTRSZVAL bloblen>
   
   \ \                                                                    */
#define EscapeSQLBinaryOpt(odbc,blob,len,q) EscapeSQLBinaryExx( odbc,blob,len,q DBG_SRC )

/* Remove escape sequences which are inserted into a text
   string. (for things like quotes and binary characters?)
   
   
   Parameters
   name :  string to remove string escapes from
   
   Returns
   a copy of the string without quotes. This result should be
   freed with Release when user is done with it.              */
PSSQL_PROC( TEXTSTR ,RevertEscapeString )( CTEXTSTR name );
/* Remove escape sequences which are inserted into a binary
   string.
   Parameters
   blob :     pointer to data to remove binary escape sequences
              from
   bloblen :  length of the data block to handle
   Returns
   a pointer to the string without escapes. (Even though it says
   binary, it's still to and from text?) This result should be
   freed with Release when user is done with it.                 */
PSSQL_PROC( TEXTSTR ,RevertEscapeBinary )( CTEXTSTR blob, PTRSZVAL *bloblen );
/* Parse a Blob string stored as hex... that is text character
   0-9 and A-F.
   Parameters
   blob :    pointer to the string containing the blob string
   buffer :  target buffer for data
   buflen :  length of target buffer                           */
PSSQL_PROC( TEXTSTR , DeblobifyString )( CTEXTSTR blob, TEXTSTR buffer, int buflen );

/* parse the string passed as a date/time as returned from a
   MySQL database.
   Parameters
   timestring :     string to parse
   endtimestring :  pointer to a pointer to a string to receive
                    the position of the character after the
                    timestring.
   year :           pointer to an int that will receive the year
                    portion of the date
   month :          pointer to an int that will receive the month
                    portion of the date
   day :            pointer to an int that will receive the day
                    portion of the date
   hour :           pointer to an int that will receive the hours
                    portion of the date
   minute :         pointer to an int that will receive the
                    minutes portion of the date
   second :         pointer to an int that will receive the
                    second portion of the date
   
   Returns
   A true/false status whether the string passed was a valid
   time string (?).                                               */
PSSQL_PROC( int, ConvertDBTimeString )( CTEXTSTR timestring
                                        , CTEXTSTR *endtimestring
													 , int *pyr, int *pmo, int *pdy
													 , int *phr, int *pmn, int *psc );


#ifndef SQLPROXY_INCLUDE
/* Issue a command to a SQL database. Things like Update and
   Insert are commands.
   Parameters
   odbc :     database connection to perform the command on. If
              NULL uses the default global connection.
   command :  text string to send to the database to execute. 
   
   Returns
   TRUE if the statement succeeds.
   
   FALSE if the statement fails. See FetchSQLError.             */
PSSQL_PROC( int, SQLCommandEx )( PODBC odbc, CTEXTSTR command DBG_PASS);
#endif
/* <combine sack::sql::SQLCommandEx@PODBC@CTEXTSTR command>
   
   \ \                                                      */
#define SQLCommand(o,c) SQLCommandEx(o,c DBG_SRC )
/* Begin collecting insert statements for batch output.
   Parameters
   odbc :  database connection to start collecting inserts for */
PSSQL_PROC( int, SQLInsertBegin )( PODBC odbc );
/* Generate a SQL insert statement from a variable parameter
   list.
   Parameters
   odbc :   connection to generate an insert on
   table :  table to insert into
   args :   a list of fields.
   
   Remarks
   args each column is a set of 3 parameters; the first
   parameter is the name of the column to insert into, the
   second is a value 0 or 1 whether to quote the value or not,
   and a string pointer.
   
   Inserts may be batched together and flushed as a whole to the
   database connection.                                          */
PSSQL_PROC( int, vSQLInsert )( PODBC odbc, CTEXTSTR table, va_list args );
/* Generate an insert to the database. Inserts to a single table
   can be cached internally and flushed.
   Parameters
   odbc :   database connection to use
   table :  name of table to insert into
   ... :    sets of column paramters.                            */
PSSQL_PROC( int, SQLInsert )( PODBC odbc, CTEXTSTR table, ... );
PSSQL_PROC( int, DoSQLInsert )( CTEXTSTR table, ... );
/* Flushes all cached inserts collected on a database
   connection.
   Parameters
   odbc :  database connection to flush inserts       */
PSSQL_PROC( int, SQLInsertFlush )( PODBC odbc );

/* This was the original implementation, it returned the results
   as a comma separated list, with quotes around results that
   had commas in them, and quotes around empty strings to
   distinguish NULL result which is just ',,'.
   Parameters
   odbc :     database connection to do the query
   query :    the string query to do
   result\ :  address of a CTEXTSTR to get a comma seperated
              \result of the query in.
   Returns
   TRUE if the query succeeded
   
   FALSE if the query was in error. See FetchSQLError.
   Example
   <code lang="c++">
   PODBC odbc = NULL; // just use the default connection...
   CTEXTSTR result;
   DoSQLQuery( odbc, "select 1,2,3", &amp;result );
   printf( "result : %s" );
   </code>
   \Output
   <code lang="c++">
   \result : 1,2,3
   
   </code>
   See Also
   SQLRecordQuery                                                */
PSSQL_PROC( int, SQLQueryEx )( PODBC odbc, CTEXTSTR query, CTEXTSTR *result DBG_PASS);
/* <combine sack::sql::SQLQueryEx@PODBC@CTEXTSTR@CTEXTSTR *result>
   
   \ \                                                             */
#define SQLQuery(o,q,r) SQLQueryEx( o,q,r DBG_SRC )
/* <combine sack::sql::DoSQLRecordQueryf@int *@CTEXTSTR **@CTEXTSTR **@CTEXTSTR@...>
   
   \ \                                                                               */
PSSQL_PROC( int, SQLRecordQueryEx )( PODBC odbc
												 , CTEXTSTR query
												 , int *pnResult
												 , CTEXTSTR **result
												 , CTEXTSTR **fields DBG_PASS);
/* <combine sack::sql::SQLRecordQueryEx@PODBC@CTEXTSTR@int *@CTEXTSTR **@CTEXTSTR **fields>
   
   \ \                                                                                      */
#define SQLRecordQuery(o,q,prn,r,f) SQLRecordQueryEx( o,q,prn,r,f DBG_SRC )
/* Gets the next result from a query.
   Parameters
   odbc :     database connection that the query was executed on
   result\ :  address of the result variable.
   
   Example
   See SQLRecordQueryf.                                          */
PSSQL_PROC( int, FetchSQLResult )( PODBC, CTEXTSTR *result );
/* Gets the next record result from the connection.
   Parameters
   odbc :     connection to get the result from; if NULL, uses
              \internal static connection.
   result\ :  address of a CTEXTSTR *; to set to an array of
              CTEXTSTR results.
   
   Remarks
   Values received are invalid after the next FetchSQLRecord or
   possibly other query.                                        */
PSSQL_PROC( int, FetchSQLRecord )( PODBC, CTEXTSTR **result ); 
/* Gets the last result on the specified ODBC connection.
   Parameters
   odbc :     connection to get the last error of
   result\ :  address of a string pointer to receive the error
              \result.
   Example
   <code lang="c++">
   {
      CTEXTSTR error;
      FetchSQLError( NULL, &amp;error );
   </code>
   <code>
      printf( "Error: %s", error );
   </code>
   <code lang="c++">
   }
   </code>                                                     */
PSSQL_PROC( int, FetchSQLError )( PODBC, CTEXTSTR *result );
#ifndef SQLPROXY_INCLUDE
/* Test if a database connection is open
   Parameters
   odbc :  database connection to check
   
   Returns
   TRUE if the connection is open and works.
   
   FALSE if the connection would not work because it is not
   connected.                                               */
PSSQL_PROC( int, IsSQLOpenEx )( PODBC DBG_PASS );
/* Test if a database connection is open
   Parameters
   odbc :  database connection to check
   
   Returns
   TRUE if the connection is open and works.
   
   FALSE if the connection would not work because it is not
   connected.                                               */
PSSQL_PROC( int, IsSQLOpen )( PODBC );
/* <combine sack::sql::IsSQLOpenEx@PODBC>
   
   \ \                                    */
#define IsSQLOpen(odbc) IsSQLOpenEx(odbc DBG_SRC )

/* An PODBC connection handles commands as a stack, this saves
   the current query state (that you want to still get results
   from), so you can start a new query within the outer query.
   Parameters
   odbc :  database connection to save the current query state. */
PSSQL_PROC( int, PushSQLQueryExEx )(PODBC DBG_PASS);
PSSQL_PROC( int, PushSQLQueryEx )(PODBC);
/* <combine sack::sql::PushSQLQueryExEx@PODBC>
   
   \ \                                         */
#define PushSQLQueryEx(odbc) PushSQLQueryExEx(odbc DBG_SRC )

// no application support for username/password, sorry, trust thy odbc layer, please
PSSQL_PROC( PODBC, ConnectToDatabase )( CTEXTSTR dsn );
PSSQL_PROC( PODBC, SQLGetODBC )( CTEXTSTR dsn );
PSSQL_PROC( void, SQLDropODBC )( PODBC odbc );

#endif
// default parameter to require is the global flag RequireConnection from sql.config....
PSSQL_PROC( PODBC, ConnectToDatabaseExx )( CTEXTSTR DSN, LOGICAL bRequireConnection DBG_PASS );
PSSQL_PROC( PODBC, ConnectToDatabaseEx )( CTEXTSTR DSN, LOGICAL bRequireConnection );

#define ConnectToDatabaseEx( dsn, required ) ConnectToDatabaseExx( dsn, required DBG_SRC )
#define ConnectToDatabase( dsn ) ConnectToDatabaseExx( dsn, FALSE DBG_SRC )

/* Close a database connection. Releases all resources
   associated with the odbc connection.
   Parameters
   odbc :  connection to database to close. Should not be NULL.  */
PSSQL_PROC( void, CloseDatabase)(PODBC odbc );

// does a query responce kinda thing returning types.
// if( GetSQLTypes() ) while( GetSQLResult( &result ) && result )
PSSQL_PROC( int, GetSQLTypes )( void );
/* ODBC only (sqlite no support?). Gets the types of data that
   the ODBC connection supports.
   Parameters
   odbc :  database connection to get the types from.
   
   Example
   <code>
   PODBC odbc = NULL; // or do a ConnectToDatabsae
   CTEXTSTR result; // the singular line result
   
   if( FetchSQLTypes(odbc) )
       while( FetchSQLResult( &amp;result ) &amp;&amp; result )
       {
           printf( "Supported Type: %s\\n", result );
       }
   
   </code>
   <code lang="c++">
   
   if( GetSQLTypes() )
       while( GetSQLResult( &amp;result ) &amp;&amp; result )
   </code>
   <code>
       {
           printf( "Supported Type: %s\\n", result );
       }
   
   </code>                                                      */
PSSQL_PROC( int, FetchSQLTypes )( PODBC );

#define PSSQL_VARARG_PROC(a,b,c)  PSSQL_PROC(a,b)c; typedef a(CPROC * __f_##b)c; PSSQL_PROC( __f_##b, __##b )(DBG_VOIDPASS);

/* Do a SQL query on the default odbc connection. The first
   record results immediately if there are any records. Returns
   the results as an array of strings. If you know the select
   you are using .... "select a,b,c from xyz" then you know that
   this will have 3 columns resulting.
   Parameters
   columns :  pointer to an int to receive the number of columns
              in the result. (the user will know this based on
              the query issued usually, so it can be NULL to
              ignore parameter)
   result\ :  pointer to a pointer to strings... see example
   fields :   address of a pointer to strings which will get the
              field names
   fmt :      format string as is appropriate for vsnprintf
   .... :     extra arguments to pass to format string
   
   Example
   See SQLRecordQueryf, but omit the database parameter.         */
PSSQL_VARARG_PROC( int, DoSQLRecordQueryf ,( int *columns, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... ) );
#define DoSQLRecordQueryf   (__DoSQLRecordQueryf( DBG_VOIDSRC ))
/* <combine sack::sql::SQLQueryf@PODBC@CTEXTSTR *@CTEXTSTR@...>
   
   \ \                                                          */
PSSQL_VARARG_PROC( int, DoSQLQueryf, ( CTEXTSTR *result, CTEXTSTR fmt, ... ) );
#define DoSQLQueryf   (__DoSQLQueryf( DBG_VOIDSRC ))
/* This does a command to the database as a formatted command.
   This allows the user to simply specify the command and
   \parameters, and not also maintain a buffer to build the
   string into before passing the string to the ODBC connection
   as a command.
   Parameters
   fmt :  format string appropriate for vsnprintf. ... \: extra
          \parameters to fill the format string.
   See Also
   SQLCommandf
   Returns
   TRUE if command success, else FALSE.
   
   if FALSE, can get the error from GetSQLError.
	*/
PSSQL_VARARG_PROC( int, DoSQLCommandf, ( CTEXTSTR fmt, ... ) );
#define DoSQLCommandf   (__DoSQLCommandf( DBG_VOIDSRC ))


/* Do a SQL query on the default odbc connection. The first
   record results immediately if there are any records. Returns
   the results as an array of strings. If you know the select
   you are using .... "select a,b,c from xyz" then you know that
   this will have 3 columns resulting.
   Parameters
   odbc :     database connection to perform the query on
   columns :  pointer to an int to receive the number of columns
              in the result. (the user will know this based on
              the query issued usually, so it can be NULL to
              ignore parameter)
   result\ :  pointer to a pointer to strings... see example
   fields :   address of a pointer to strings which will get the
              field names. May be ommited if you don't want to
              know the names. (is less work internally if this is
              not built).
   fmt :      format string as is appropriate for vsnprintf
   .... :     extra arguments to pass to format string
   
   Example
   <code lang="c++">
   PODBC odbc = ConnectToDatabase( "MySQL" );
   CTEXTSTR *results;
   CTEXTSTR *column_names;
   int columns;
   for( SQLRecordQueryf( odbc, &amp;columns, &amp;results, &amp;column_names
                       , "select a,b,c from %s where %s=%s"
                       , "table_name"
                       , "column_name"
                       , "'value'"
                       );
        results;
        FetchSQLRecord( odbc, &amp;results ) )
   {
      int n;
       // draw a seperator between rows returned
      printf( " ----- record data ----- \\n" );
      for( n = 0; n \< columns; n++ )
      {
         printf( "Result column '%s' = '%s'\\n", column_name[n], results[n] );
      }
   }
   
   CloseDatabase( odbc );
   
   </code>
   If the default connection is used, odbc can be NULL in the
   prior example, or the for staement could be
   <code>
   for( DoSQLRecordQueryf( &amp;columns, &amp;results, &amp;column_names
                         , "select a,b,c from %s where %s=%s"
                         , "table_name"
                         , "column_name"
                         , "'value'"
                         );
        results;
        GetSQLRecord( &amp;results ) )
   {
   }
   </code>                                                                     */
//PSSQL_PROC( int, SQLRecordQueryf )( PODBC odbc, int *columns, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... );
PSSQL_VARARG_PROC( int, SQLRecordQueryf, ( PODBC odbc, int *columns, CTEXTSTR **result, CTEXTSTR **fields, CTEXTSTR fmt, ... ) );
#define SQLRecordQueryf   (__SQLRecordQueryf( DBG_VOIDSRC ))
/* This was the original implementation, it returned the results
   as a comma separated list, with quotes around results that
   had commas in them, and quotes around empty strings to
   distinguish NULL result which is just ',,'.
   Parameters
   odbc :     database connection to do the query
   result\ :  address of a CTEXTSTR to get a comma seperated
              \result of the query in.
   query :    the string query to do
   ... :      extra parameters for the query format string
   
   Returns
   TRUE if the query succeeded
   
   FALSE if the query was in error. See FetchSQLError.
   Example
   <code>
   PODBC odbc = NULL; // just use the default connection...
   CTEXTSTR result;
   DoSQLQueryf( odbc, &amp;result, "select %d,%d,%d", 1, 2, 3 );
   printf( "result : %s" );
   </code>
   \Output
   <code>
   \result : 1,2,3
   
   </code>
   See Also
   SQLRecordQueryf                                               */
PSSQL_VARARG_PROC( int, SQLQueryf ,( PODBC odbc, CTEXTSTR *result, CTEXTSTR fmt, ... ) );
#define SQLQueryf   (__SQLQueryf( DBG_VOIDSRC ))

/* This performs a command on a SQL connection.
   Parameters
   odbc :  database connection to do the command on
   fmt :   format string as appropriate for vsnprintf
   ... :   extra arguments as required by the format string
   
   Returns
   TRUE if command success, else FALSE.
   
   if FALSE, can get the error from FetchSQLError.
                                                            */
PSSQL_VARARG_PROC( int, SQLCommandf, ( PODBC odbc, CTEXTSTR fmt, ... ) );
#define SQLCommandf   (__SQLCommandf( DBG_VOIDSRC ))


/* Function signature for the callback when the SQL layer can
   log a status about a database connection (connection,
   disconnected, failed...) See SQLSetFeedbackHandler.        */
typedef void (CPROC *HandleSQLFeedback)(CTEXTSTR message);
// register a feedback message for startup messages
//  allows external bannering of status... perhaps this can handle failures
//  and disconnects also...
PSSQL_PROC( void, SQLSetFeedbackHandler )( HandleSQLFeedback handler );

/* Parses a CREATE TABLE statement and builds a PTABLE from it.
   Parameters
   cmd :         a CREATE TABLE sql command. It is a little
                 sqlite/mysql centric, and may fail on column
                 types for SQL Server.
   writestate :  if writestate is TRUE, a file called
                 'sparse.txt' will be generated with a C
                 structure of the Create Table statement passed. This
                 \file could then be used to copy into code, and
                 have a code\-static definition instead of going
                 from the create table statement.
   
   Returns
   a PTABLE which represents the create table statement.              */
PSSQL_PROC( PTABLE, GetFieldsInSQLEx )( CTEXTSTR cmd, int writestate DBG_PASS );
/* <combine sack::sql::GetFieldsInSQLEx@CTEXTSTR@int writestate>
   
   \ \                                                           */
#define GetFieldsInSQL(c,w) GetFieldsInSQLEx( c, w DBG_SRC )
//PSSQL_PROC( PTABLE, GetFieldsInSQL )( CTEXTSTR cmd, int writestate);
// this is used to destroy the table returned by GetFieldsInSQL
PSSQL_PROC( void, DestroySQLTable )( PTABLE table );

// allow setting and getting of a bit of user data associated with the PODBC...
// though this can result in memory losses at the moment, cause there is no notification
// that the PODBC has gone away, and that the user needs to remove his data...
PSSQL_PROC( PTRSZVAL, SQLGetUserData )( PODBC odbc );
/* A PODBC may have a user data assigned to it.
   Parameters
   odbc :  connection to set the data for; shouldn't be NULL.
   psv :   user data to assign to the database connection.
   
   See Also
   SQLGetUserData                                             */
PSSQL_PROC( void, SQLSetUserData )( PODBC odbc, PTRSZVAL );


/* Returns a text string GUID, the guid is saved in psersistant text space and will 
   not be released or overwritten.  */
PSSQL_PROC( CTEXTSTR, GetGUID )( void );

/* Returns a text string GUID, This uses UuidCreateSequential  */
PSSQL_PROC( CTEXTSTR, GetSeqGUID )( void );

/* Returns a text string GUID, the guid is saved in psersistant text space and will 
   not be released or overwritten.  This tring is the constant 0 guid */
PSSQL_PROC( CTEXTSTR, GuidZero )( void );

/* some internal stub-proxy linkage for generating remote
   responders..
   
   This was work in progress for providing a msgsvr service to
   SQL. One of the implementations of this library was across a
   windows message queue using ATOM types to transport results
   and commands. Was going to implement this on the abstract
   msgqueue interface.                                          */
typedef struct responce_tag
{
	struct {
		BIT_FIELD bSingleLine : 1;
		BIT_FIELD bMultiLine : 1;
		BIT_FIELD bFields : 1;
	} flags;
	PVARTEXT result_single_line;
   int nLines;
	CTEXTSTR *pLines;
   CTEXTSTR *pFields;
} SQL_RESPONCE, *PSQL_RESPONCE;

/* *WORK IN PROGRESS* function call signature for callback method passed to
   RegisterResponceHandler.                              */
typedef void (CPROC *result_responder)( int responce
									  , PSQL_RESPONCE result );


/* *WORK IN PROGRESS* 
   result_responder :  callback function to get called with sql
                       global status messages.
   
   See Also
   <link sack::sql::result_responder, Result Responder Type>    */
PSSQL_PROC( void, RegisterResponceHandler )( result_responder );

/* Thread protect means to use critical sections to protect this
   connection against multiple thread access. Recommended usage
   is to not use a PODBC with more than one thread in the first
   place.
   Parameters
   odbc :     connection to enable; if null, references the
              \internal static connection.
   bEnable :  TRUE to enable, FALSE to disable.                  */
PSSQL_PROC( void, SetSQLThreadProtect )( PODBC odbc, LOGICAL bEnable );
/* Enable using 'BEGIN TRANSACTION' and 'COMMIT' commands automatically
   around commands. If there is a lull of 500ms (1/2 second),
   then the commit automatically fires. SQLCommit can be called
   to trigger this process early.
   
   
   Parameters
   odbc :     connection to set auto transact on
   bEnable :  TRUE to enable, FALSE to disable.                         */
PSSQL_PROC( void, SetSQLAutoTransact )( PODBC odbc, LOGICAL bEnable );
/* Enable using 'BEGIN TRANSACTION' and 'COMMIT' commands automatically
   around commands. If there is a lull of 500ms (1/2 second),
   then the commit automatically fires. SQLCommit can be called
   to trigger this process early.
   
   
   Parameters
   odbc :     connection to set auto transact on
   callback :  not NULL to enable, NULL to disable.                         */
PSSQL_PROC( void, SetSQLAutoTransactCallback )( PODBC odbc, void (CPROC*callback)(PTRSZVAL,PODBC), PTRSZVAL psv );
/* Relevant for SQLite databases. After a certain period of
   inactivity the database is closed (allowing the file to be
   not-in-use during idle). PODBC odject remains valid, and
   connection to database is re-enabled on next usage.
   
   
   Parameters
   odbc :     connection to enable auto close behavior on
   bEnable :  TRUE to enable auto close FALSE to disable.     */
PSSQL_PROC( void, SetSQLAutoClose )( PODBC odbc, LOGICAL bEnable );

/* A function to apply a time offset for fiscal time
   calculations; sometimes the day doesn't end at midnight, but
   a shift might last until 5 in the morning.
   Parameters
   odbc :            connection to get the appropriate SQL
                     expression for
   BeginOfDayType :  name of the type of beginning of the day
   default_begin :   the default time when a day begins.
   
   Note
   default_begin is a format sort of like a time. If this is a
   simple integer 5 then it's 5:00am, if it's more than 100,
   then it's assumed to be hours and minutes so 530 would be
   5:30 in the monring. this is also stored in the option
   databse, so the default value can be overridden; if the SQL
   value has a ':' in it then it is parsed as hours and minutes.
   Negative time may be used to indicate that the day begins
   before the day ends (-2 would be day end at 10pm).            */
PSSQL_PROC( CTEXTSTR, GetSQLOffsetDate )( PODBC odbc, CTEXTSTR BeginOfDayType, int default_begin );

SQL_NAMESPACE_END
#ifdef __cplusplus
	using namespace sack::sql;
#endif
#endif



#if 0
#endif
