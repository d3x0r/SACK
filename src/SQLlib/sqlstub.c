
//-----------------------------------------------------------------------
// SQLSTUB.C - core sql abstractin library
//   Provides, essentially, 2 functions...
//     SQLQuery
//     SQLCommand
//
//-----------------------------------------------------------------------

/* this is a logging around the ODBC open itself... may be used for timing... */
//#define LOG_ACTUAL_CONNECTION
//#define LOG_COLLECTOR_STATES
//#define LOG_EVERYTHING
#define SQLLIB_SOURCE
#define DO_LOGGING
#ifdef USE_SQLITE_INTERFACE
#define USES_SQLITE_INTERFACE
//#define DEFINES_SQLITE_INTERFACE
#endif
#include <stdhdrs.h>
#include <deadstart.h>
#include <sack_types.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <idle.h>
#include <sqlgetoption.h>
#include <configscript.h>
#include <procreg.h>
#include <filesys.h>
#include <sqlgetoption.h>
#include <salty_generator.h>
#include <json_emitter.h>  // uses generic 'json_value_container'
#include <sha1.h>
// please remove this reference ASAP
//#include <controls.h> // temp graphic interface for debugging....
#include <systray.h>
#include <msgclient.h>
#ifdef SQL_PROXY_SERVER
#include <construct.h>
#endif

#ifdef __cplusplus
using namespace sack::logging;
using namespace sack::memory;
using namespace sack::timers;
using namespace sack::config;
#endif

#include "sqlstruc.h"

#include <pssql.h>


SQL_NAMESPACE

#ifdef _cplusplus_cli
using namespace CORE::Database;

public ref class export_this_class_PLEASE
{
	~export_this_class_PLEASE() // implements/overrides the IDisposable::Dispose method
    {
        // free managed and unmanaged resources
    }

    !export_this_class_PLEASE() // implements/overrides the Object::Finalize method
    {
        // free unmanaged resources only
    }

};

//It also works on generics:
#if 0
generic <typename T>
ref class MyGeneric
{
public:
   void PrintTypes()
   {
      Type^ tType = T::typeid;
      Console::WriteLine(tType->FullName);
   }
};
#endif
/*
PRELOAD( test1 )
//void f( void )
{
CORE::Database::SingleService^ ss = gcnew CORE::Database::SingleService();
DECLTEXT( cmd, WIDE("Select 1+1") );
System::Data::Common::DbDataReader^ db = ss->RunQuery( gcnew System::String( GetText( (PTEXT)&cmd ) ) );
for( int n = db->FieldCount; n; n-- )
{

	lprintf( WIDE("field %d is %s"), n, db->GetString(n-1) );
}
delete db;
delete ss;
	//for( db.Read();
	//CORE::Database::SingleService::
	}
*/
#endif


#define PROXY_FULL     WIDE( "PROXY_FULL" )
#define PROXY_PRIMARY  WIDE( "PROXY_PRIMARY" )
#define PROXY_BACKUP   WIDE( "PROXY_BACKUP" )
#define PROXY_DOWN     WIDE( "PROXY_DOWN" )


#define SQL_INI WIDE( "SQLPROXY.INI" )

static uintptr_t CPROC AutoCloseThread( PTHREAD thread );
static uintptr_t CPROC AutoCheckpointThread( PTHREAD thread );
void CloseDatabaseEx( PODBC odbc, LOGICAL ReleaseConnection );

static int __DoSQLQueryEx(  PODBC odbc, PCOLLECT collection, CTEXTSTR query DBG_PASS );
#define __DoSQLQuery( o,c,q ) __DoSQLQueryEx(o,c,q DBG_SRC )
static int __DoSQLCommandEx( PODBC odbc, PCOLLECT collection/*, uint32_t MyID*/ DBG_PASS );
#define __DoSQLCommand(o,c) __DoSQLCommandEx(o,c DBG_SRC )
static int __GetSQLResult( PODBC odbc, PCOLLECT collection, int bMore/*, uint32_t MyID*/ );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
static int DumpInfo2( PVARTEXT pvt, SQLSMALLINT type, struct odbc_handle_tag *odbc, LOGICAL bNoLog );
#endif
#ifdef USE_ODBC
static int DumpInfoEx( PODBC odbc, PVARTEXT pvt, SQLSMALLINT type, SQLHANDLE *handle, LOGICAL bNoLog DBG_PASS );
#define DumpInfo(o,a,b,c,d) DumpInfoEx(o,a,b,c,d DBG_SRC )
//int DumpInfo( PVARTEXT pvt, SQLSMALLINT type, SQLHANDLE *handle );
#endif

enum SQL_DelayOperations {
   LAST_NONE
     , LAST_COMMAND
     , LAST_QUERY
     , LAST_RESULT
}; // LastOperations;


#include <configscript.h>

#include "sqlstruc.h"
//#include <sqlstub.h>

typedef struct update_task_def UPDATE_TASK, *PUPDATE_TASK;
struct update_task_def
{
	TEXTCHAR name[256];
	// perhaps this should be PODBC primary PODBC backup
	// so we know where we're copying to/from on the update task.
	void (CPROC *PrimaryRecovered)( PODBC,PODBC );
	void (CPROC *CheckTables)( PODBC );
	DeclareLink( struct update_task_def );
};

struct pssql_global *global_sqlstub_data;
#ifdef g
#  undef g
#endif
#define g (*global_sqlstub_data)

//----------------------------------------------------------------------

static void SqlStubInitLibrary( void );


PRIORITY_PRELOAD( InitGlobalData, SQL_PRELOAD_PRIORITY )
{
	// is null initialized.
	SimpleRegisterAndCreateGlobal( global_sqlstub_data );
	SqlStubInitLibrary();

}

ATEXIT_PRIORITY( CloseConnections, ATEXIT_PRIORITY_SYSLOG - 3 )
{
	PODBC odbc;
	INDEX idx;
	if( global_sqlstub_data )
		LIST_FORALL( global_sqlstub_data->pOpenODBC, idx, PODBC, odbc  )
		{
			CloseDatabaseEx( odbc, FALSE );
		}
}

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )

static void GetColumnSize(sqlite3_context*onwhat,int n,sqlite3_value**something) {
	switch( sqlite3_value_type( something[0] ) ) {
	case SQLITE_TEXT :
	case SQLITE_BLOB :
		sqlite3_result_int( onwhat, sqlite3_value_bytes(something[0] ) );
		break;
	default :
		// do a text conversion on it.
		sqlite3_value_text( something[0] );
		sqlite3_result_int( onwhat, sqlite3_value_bytes(something[0] ) );
		break;
	}
}

static void GetNowFunc(sqlite3_context*onwhat,int n,sqlite3_value**something)
{
	CTEXTSTR str = GetPackedTime();
#ifdef _UNICODE
	char *tmp_str = WcharConvert( str );
	sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
	Deallocate( char *, tmp_str );
#else
	sqlite3_result_text( onwhat, str, (int)(sizeof( str[0] ) * StrLen( str )), 0 );
#endif
}
static void GetCurUser(sqlite3_context*onwhat,int n,sqlite3_value**something)
{
	CTEXTSTR str = OSALOT_GetEnvironmentVariable( WIDE("USERNAME") );
	static TEXTCHAR tmp[256];
	tnprintf( tmp, 256, WIDE("%s@127.0.0.1"), str );
#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( tmp );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, tmp, (int)(sizeof( tmp[0] ) * StrLen( tmp )), 0 );
#endif
}
static void GetCurDateFunc(sqlite3_context*onwhat,int n,sqlite3_value**something)
{
	CTEXTSTR str = GetPackedTime();
	static TEXTSTR tmp;
	if( !tmp )
		tmp = StrDup( str );
	else
		StrCpyEx( tmp, str, 8 );
	tmp[8] = 0;
#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( tmp );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, tmp, (int)(sizeof( tmp[0] ) * StrLen( tmp )), 0 );
#endif
}
static void SQLiteGetLastInsertID(sqlite3_context*onwhat,int n,sqlite3_value**something)
{
	static TEXTCHAR str[20];
	PODBC odbc = (PODBC)sqlite3_user_data(onwhat);
	tnprintf( str, sizeof( str ), WIDE( "%") _64fs, sqlite3_last_insert_rowid( odbc->db ) );
#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( str );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, str, (int)(sizeof( str[0] ) * StrLen( str )), 0 );
#endif
}

#ifndef NO_CRYPT
static void computeSha1(sqlite3_context*onwhat,int argc,sqlite3_value**argv)
{
	PVARTEXT pvt = VarTextCreate();
	PODBC odbc = (PODBC)sqlite3_user_data(onwhat);
	const unsigned char *val = sqlite3_value_text( argv[0] );
	SHA1Context sha;
	uint8_t digest[SHA1HashSize];
	int n;
	static PTEXT result;
   if( result ) LineRelease( result );
	SHA1Reset( &sha );
	SHA1Input( &sha, val, strlen( (CTEXTSTR)val ) );
	SHA1Result( &sha, digest );
	for( n = 0; n < SHA1HashSize; n++ )
		vtprintf( pvt, "%02X", digest[n] );
	result = VarTextGet( pvt );

#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( GetText( result ) );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, GetText( result ), (int)GetTextSize( result ), 0 );
#endif
}

static void computePassword(sqlite3_context*onwhat,int argc,sqlite3_value**argv)
{
   const unsigned char *val = sqlite3_value_text( argv[0] );
	static TEXTCHAR *result;
	if( result ) Release( result );
	result = SRG_EncryptString( (CTEXTSTR)val );
#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( result );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, result, (int)StrLen( result ), 0 );
#endif
	//Release( result );
}

static void decomputePassword(sqlite3_context*onwhat,int n,sqlite3_value**argv)
{
	const unsigned char *val = sqlite3_value_text( argv[0] );
	static TEXTCHAR *result;
	if( result ) Release( result );
	result = SRG_DecryptString( (CTEXTSTR)val );
#ifdef _UNICODE
	{
		char *tmp_str = WcharConvert( result );
		sqlite3_result_text( onwhat, tmp_str, (int)(sizeof( tmp_str[0] ) * StrLen( str )), 0 );
		Deallocate( char *, tmp_str );
	}
#else
	sqlite3_result_text( onwhat, result, (int)StrLen( result ), 0 );
#endif
	//Release( result );
}

#endif

int PSSQL_AddSqliteFunction( PODBC odbc
	, const char *name
	, void( *callUserFunction )( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv )
	, void( *callDestroy )( void* )
	, int args
	, void *userData ) {
	return sqlite3_create_function_v2(
	    odbc->db //sqlite3 *,
	    , name  //const char *zFunctionName,
	    , args //int nArg,
	    , SQLITE_UTF8 //int eTextRep,
	    , userData //void*,
	    , callUserFunction //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
	    , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
	    , NULL //void (*xFinal)(sqlite3_context*)
	    , callDestroy
	);
}

int PSSQL_AddSqliteProcedure( PODBC odbc
	, const char *name
	, void( *callUserFunction )( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv )
	, void( *callDestroy )( void* )
	, int args
	, void *userData ) {
	return sqlite3_create_function_v2(
	    odbc->db //sqlite3 *,
	    , name  //const char *zFunctionName,
	    , args //int nArg,
	    , SQLITE_UTF8|SQLITE_DETERMINISTIC //int eTextRep,
	    , userData //void*,
	    , callUserFunction //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
	    , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
	    , NULL //void (*xFinal)(sqlite3_context*)
	    , callDestroy
	);
}

int PSSQL_AddSqliteAggregate( PODBC odbc
	, const char *name
	, void( *callStep )( struct sqlite3_context*onwhat, int argc, struct sqlite3_value**argv )
	, void( *callFinal )( struct sqlite3_context*onwhat )
	, void( *callDestroy )( void* )
	, int args
	, void *userData ) {
	return sqlite3_create_function_v2(
	    odbc->db //sqlite3 *,
	    , name  //const char *zFunctionName,
	    , args //int nArg,
	    , SQLITE_UTF8 //int eTextRep,
	    , userData //void*,
	    , NULL //callUserFunction //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
	    , callStep //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
	    , callFinal //void (*xFinal)(sqlite3_context*)
	    , callDestroy
	);
}


POINTER PSSQL_GetSqliteFunctionData( struct sqlite3_context*context ) {
	return sqlite3_user_data( context );
}
void PSSQL_ResultSqliteText( struct sqlite3_context*context, const char *data, int dataLen, void (*done)(void*) ) {
	sqlite3_result_text( context, data, dataLen, done );
}
void PSSQL_ResultSqliteBlob( struct sqlite3_context*context, const char *data, int dataLen, void (*done)(void*) ) {
	sqlite3_result_blob( context, data, dataLen, done );
}
void PSSQL_ResultSqliteDouble( struct sqlite3_context*context, double val ) {
	sqlite3_result_double( context, val );
}
void PSSQL_ResultSqliteInt( struct sqlite3_context*context, int val ) {
	sqlite3_result_int( context, val );
}
void PSSQL_ResultSqliteInt64( struct sqlite3_context*context, int64_t val ) {
	sqlite3_result_int64( context, val );
}
void PSSQL_ResultSqliteNull( struct sqlite3_context*context ) {
	sqlite3_result_null( context );
}
void PSSQL_GetSqliteValueText( struct sqlite3_value *val, const char **text, int *textLen ) {
	(*text) = (const char *)sqlite3_value_text( val ); // sqlite function is 'unsigned' result
	(*textLen) = sqlite3_value_bytes( val );
}

enum sqlite_data_types PSSQL_GetSqliteValueType( struct sqlite3_value *val ){
	return (enum sqlite_data_types)sqlite3_value_type( val );
}
void PSSQL_GetSqliteValueBlob( struct sqlite3_value *val, const char **text, int *textLen ){
	(*text) = (const char *)sqlite3_value_text( val ); // sqlite function is 'unsigned' result
	(*textLen) = sqlite3_value_bytes( val );
}
void PSSQL_GetSqliteValueDouble( struct sqlite3_value *val, double *result ){
	(*result) = sqlite3_value_double( val ); // sqlite function is 'unsigned' result
}
void PSSQL_GetSqliteValueInt( struct sqlite3_value *val, int *result ){
	(*result) = sqlite3_value_int( val ); // sqlite function is 'unsigned' result
}
void PSSQL_GetSqliteValueInt64( struct sqlite3_value *val, int64_t *result ){
	(*result) = sqlite3_value_int64( val ); // sqlite function is 'unsigned' result
}
const char * PSSQL_GetColumnTableName( PODBC odbc, int col) {
	if( odbc->flags.bSQLite_native ) {
		PCOLLECT pCollect;
		pCollect = odbc ? odbc->collection : NULL;
		if( pCollect ) {
			const char *tmp;
			//tmp = sqlite3_column_table_name( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			//tmp = sqlite3_column_origin_name( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			tmp = sqlite3_column_table_name( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			return tmp;
		}
	}
	else
		return "?";
	return NULL;
}
const char * PSSQL_GetColumnTableAliasName( PODBC odbc, int col ) {
	if( odbc->flags.bSQLite_native ) {
		PCOLLECT pCollect;
		pCollect = odbc ? odbc->collection : NULL;
		if( pCollect ) {
			const char *tmp;
			//tmp = sqlite3_column_table_name( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			//tmp = sqlite3_column_origin_name( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			tmp = sqlite3_column_table_alias( pCollect->stmt, col ); // sqlite function is 'unsigned' result
			return tmp;
		}
	}
	else {
		PCOLLECT pCollect;
		pCollect = odbc ? odbc->collection : NULL;
		if( pCollect ) {
		}
		return "?";
	}
	return NULL;
}

void ExtendConnection( PODBC odbc )
{
	int rc;
	if( odbc->flags.bAutoClose )
	{
		lprintf( "Extned found autoclose" );
		if( !odbc->auto_close_thread )
			odbc->auto_close_thread = ThreadTo( AutoCloseThread, (uintptr_t)odbc );
	}
	rc = sqlite3_create_function(
												odbc->db //sqlite3 *,
											  , "now"  //const char *zFunctionName,
											  , 0 //int nArg,
											  , SQLITE_UTF8 //int eTextRep,
											  , (void*)odbc //void*,
											  , GetNowFunc //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
											  , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
											  , NULL //void (*xFinal)(sqlite3_context*)
											  );
	if( rc )
	{
		// error..
	}
	rc = sqlite3_create_function(
												odbc->db //sqlite3 *,
											  , "bytes"  //const char *zFunctionName,
											  , 0 //int nArg,
											  , SQLITE_INTEGER //int eTextRep,
											  , (void*)odbc //void*,
											  , GetColumnSize //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
											  , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
											  , NULL //void (*xFinal)(sqlite3_context*)
											  );
	if( rc )
	{
		// error..
	}
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "curdate"  //const char *zFunctionName,
										 , 0 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , GetCurDateFunc //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "user"  //const char *zFunctionName,
										 , 0 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , GetCurUser //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}
#ifndef NO_CRYPT
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "sha1"  //const char *zFunctionName,
										 , 1 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , computeSha1 //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "encrypt"  //const char *zFunctionName,
										 , 1 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , computePassword //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "decrypt"  //const char *zFunctionName,
										 , 1 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , decomputePassword //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}
#endif
	rc = sqlite3_create_function(
										  odbc->db //sqlite3 *,
										 , "LAST_INSERT_ID"  //const char *zFunctionName,
										 , 0 //int nArg,
										 , SQLITE_UTF8 //int eTextRep,
										 , (void*)odbc //void*,
										 , SQLiteGetLastInsertID //void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xStep)(sqlite3_context*,int,sqlite3_value**),
										 , NULL //void (*xFinal)(sqlite3_context*)
										 );
	if( rc )
	{
		// error..
	}

	if( !sqlite3_get_autocommit(odbc->db) )
	{
		lprintf( WIDE( "auto commit off?" ) );
		//DebugBreak();
	}

	//SQLCommandf( odbc, "PRAGMA read_uncommitted=True" );
	{
		CTEXTSTR result;
		int n = odbc->flags.bNoLogging;
		int m = odbc->flags.bAutoCheckpoint;
		odbc->flags.bNoLogging = 0;
		odbc->flags.bAutoCheckpoint = 0;
		{
			INDEX idx;
			CTEXTSTR cmd;
			LIST_FORALL( g.database_init, idx, CTEXTSTR, cmd ) {
				SQLQueryf( odbc, &result, cmd );
				//if( result )
				//	lprintf( WIDE( " %s" ), result );
				SQLEndQuery( odbc );
			}
		}
		//SQLQueryf( odbc, &result, WIDE( "PRAGMA journal_mode=WAL;" ) );
		//if( result )
		//	lprintf( WIDE( "Journal is now %s" ), result );
		//SQLEndQuery( odbc );

		if( g.flags.bAutoCheckpointRecover ) {
			SQLQueryf( odbc, &result, WIDE( "PRAGMA wal_checkpoint;" ) );
			SQLEndQuery( odbc );
			g.flags.bAutoCheckpointRecover = 0;
		}
		odbc->flags.bNoLogging = n;
		odbc->flags.bAutoCheckpoint = m;
		//SQLQueryf( odbc, &result, WIDE( "PRAGMA journal_mode" ) );
		//lprintf( WIDE( "Journal is now %s" ), result );
	}
}

#endif
//----------------------------------------------------------------------

static void DumpODBCInfo( PODBC odbc )
{
	if( g.odbc && odbc == g.odbc )
	{
		lprintf( WIDE( "GLOBAL ODBC:" ) );
	}
	if( !odbc )
		return;
	lprintf( WIDE( "odbc = %p" ), odbc );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	lprintf( WIDE( "odbc->db = %p" ), odbc->db );
#endif
#ifdef USE_ODBC
	lprintf( WIDE( "odbc->env = %p" ), odbc->env );
	lprintf( WIDE( "odbc->hdbc = %p" ), odbc->hdbc );
#endif
	lprintf( WIDE( "Last native error code: %d" ), odbc->native );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
#  define NativeOption odbc->flags.bSQLite_native?WIDE("[Native SQLite]"):WIDE("[Not Native SQLite]")
#else
#  define NativeOption WIDE("[]")
#endif
#ifdef USE_ODBC
#  define ODBCOption odbc->flags.bODBC?WIDE("[ODBC]"):WIDE("[Not ODBC]")
#else
#  define ODBCOption WIDE("[]")
#endif

	lprintf( WIDE( "odbc flags = %s %s %s %s %s %s" )
		, odbc->flags.bConnected?WIDE("[Connected]"):WIDE("[Not Connected]")
		, odbc->flags.bAccess ?WIDE("[MS Access]"):WIDE("[]")
		, odbc->flags.bSQLite?WIDE("[SQLite]"):WIDE("[]")
			 , odbc->flags.bPushed?WIDE("[PendingPush]"):WIDE("[]")
			 , NativeOption
			 , ODBCOption
		);
	lprintf( WIDE("Collection(s)...") );
	{
		PCOLLECT c;
		for( c = odbc->collection; c; c = c->next )
		{
			lprintf( WIDE( "----------" ) );
			lprintf( WIDE( "\tflags: %s %s %s %s %s" )
					 , c->flags.bBuildResultArray?WIDE( "Result Array" ):WIDE( "Result String" )
					 , c->flags.bDynamic?WIDE( "Dynamic" ):WIDE( "Static" )
					 , c->flags.bTemporary?WIDE( "Temporary" ):WIDE( "QueryResult" )
					 , c->flags.bPushed?WIDE( "Pushed" ):WIDE( "Auto" )
					 , c->flags.bEndOfFile?WIDE( "EOF" ):WIDE( "more" )
					 );
			lprintf( WIDE( "\tCommand: %s" )
					 , GetText( VarTextPeek( c->pvt_out ) )
					 );
			lprintf( WIDE( "\tResult: %s" )
					 , GetText( VarTextPeek( c->pvt_result ) )
					 );
			lprintf( WIDE( "\tErr Info: %s" )
					 , GetText( VarTextPeek( c->pvt_errorinfo ) )
					 );
		}
	}
}

void DumpAllODBCInfo( void )
{
	INDEX idx;
	PODBC odbc;
	LIST_FORALL( g.pOpenODBC, idx, PODBC, odbc )
		DumpODBCInfo( odbc );
}

//----------------------------------------------------------------------

void SQLSetFeedbackHandler( void (CPROC*HandleSQLFeedback)(CTEXTSTR message) )
{
	g.feedback_handler = HandleSQLFeedback;
}
//----------------------------------------------------------------------

static LOGICAL IsOdbcIdle( PODBC odbc ) {
	PCOLLECT pCollect;
	pCollect = odbc?odbc->collection:NULL;
	for( ; pCollect; pCollect = NextThing( pCollect ) ) {
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		// still has an active statement.
		if( pCollect->stmt )
			return FALSE;
#endif
#if defined( USE_ODBC )
		// still has an active statement.
		if( pCollect->hstmt )
			return FALSE;
#endif
	}
	return TRUE;
}

static void startAutoCheckpoint( PODBC odbc ) {
	if( odbc->flags.bAutoCheckpoint )
	{
		//lprintf( "enabling oneshot idle chckpoint generator" );
		//DumpODBCInfo( odbc );
		if( !odbc->auto_checkpoint_thread )
			odbc->auto_checkpoint_thread = ThreadTo( AutoCheckpointThread, (uintptr_t)odbc );
	}
}


#ifdef LOG_COLLECTOR_STATES
		static int collectors;
#endif
static PCOLLECT CreateCollectorEx( PSERVICE_ROUTE SourceID, PODBC odbc, LOGICAL bTemporary DBG_PASS )
#define CreateCollector(s,o,t) CreateCollectorEx( s,o,t DBG_SRC )
{
	PCOLLECT pCollect;
	LOGICAL pushed;
	if( !odbc )
		odbc = g.odbc;
	if( !odbc )
	{
		lprintf( WIDE("No certain odbc to create collector ON...") );
		DebugBreak();
		return NULL;
	}
	pushed = odbc->collection?odbc->collection->flags.bPushed:odbc->flags.bPushed;
#ifdef LOG_COLLECTOR_STATES
	lprintf( WIDE( "Creating [%s][%s] collector" ), bTemporary?WIDE( "temp" ):WIDE( "" ), odbc->collection?odbc->collection->flags.bPushed?WIDE( "pushed" ):WIDE( "" ):WIDE( "" ) );
#endif
	pCollect = odbc->collection;
	if( pushed && pCollect && pCollect->flags.bPushed )
	{
#ifdef LOG_COLLECTOR_STATES
		lprintf( WIDE( "New collector should be 'pushed', and prior is pushed (might be end of file query temp)" ) );
#endif
		// don't do anything, but definatly don't do temproary promotions.
	}
	else
	{
		if( pCollect && pCollect->flags.bTemporary && bTemporary )
		{
			// it's a created collector, empty prior command... (query)
			VarTextEmpty( pCollect->pvt_out );
			VarTextEmpty( pCollect->pvt_errorinfo );
			if( odbc->flags.bPushed )
			{
				pCollect->flags.bPushed = 1;
				pCollect->flags.bTemporary = 1;
				odbc->flags.bPushed = 0;
			}
			return pCollect; // already have a temp available.. use it.
		}
		else if( pCollect && pCollect->flags.bTemporary && !bTemporary )
		{
#ifdef LOG_COLLECTOR_STATES
			lprintf( WIDE( "Promoting temporary %p to non temp (query)" ), pCollect );
#endif
			VarTextEmpty( pCollect->pvt_out );
			VarTextEmpty( pCollect->pvt_errorinfo );
			pCollect->flags.bTemporary = 0;
			if( odbc->flags.bPushed )
			{
				pCollect->flags.bPushed = 1;
				pCollect->flags.bTemporary = 1;
				odbc->flags.bPushed = 0;
			}
			return pCollect;
		}
	}
	pCollect = (PCOLLECT)AllocateEx( sizeof( COLLECT ) DBG_RELAY );
#ifdef LOG_COLLECTOR_STATES
	lprintf( WIDE( "New collector is %p" ), pCollect );
#endif
	MemSet( pCollect, 0, sizeof( COLLECT ) );
	// there are a couple uninitialized values in thiws...
	pCollect->fields = NULL;
	pCollect->result_len = NULL;
	pCollect->lastop = LAST_NONE;
	pCollect->odbc = odbc;
	pCollect->pvt_out = VarTextCreateEx( DBG_VOIDRELAY );
	pCollect->pvt_result = VarTextCreate();
	pCollect->pvt_errorinfo = VarTextCreate();
	pCollect->SourceID = SourceID;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	pCollect->stmt = NULL;
#endif
#ifdef USE_ODBC
	pCollect->hstmt = NULL;
#endif
	pCollect->flags.bTemporary = bTemporary;
	pCollect->flags.bDynamic = TRUE;
	//lprintf( WIDE("Adding %p to %p at %p"), pCollect, odbc, &odbc->collection );
	if( odbc )
	{
		LinkThing( odbc->collection, pCollect );
	}
	else
		LinkThing( g.collections, pCollect );
#ifdef LOG_COLLECTOR_STATES
	{
		collectors++;
		lprintf( WIDE( "Collectors: %d" ), collectors );
	}
#endif
	if( odbc->flags.bPushed )
	{
		pCollect->flags.bPushed = 1;
		pCollect->flags.bTemporary = 1;
		odbc->flags.bPushed = 0;
	}
	return pCollect;
}

//----------------------------------------------------------------------

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static uintptr_t CPROC AddDatabaseInit( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, initSQL );
	if( initSQL && strlen( initSQL ) )
		AddLink( &g.option_database_init, StrDup( initSQL ) );
	return psv;
}

static uintptr_t CPROC AddOptionDatabaseInit( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, initSQL );
	if( initSQL && strlen( initSQL ) )
		AddLink( &g.database_init, StrDup( initSQL ) );
	return psv;
}

static uintptr_t CPROC SetPrimaryDSN( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pDSN );
	g.Primary.info.pDSN = StrDup( pDSN );
	return psv;
}

static uintptr_t CPROC SetOptionDSN( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pDSN );
	g.OptionDb.info.pDSN = StrDup( pDSN );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetBackupDSN( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pDSN );
	g.Backup.info.pDSN = StrDup( pDSN );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetConnString( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pUser );
	StrCpyEx( g.Primary.info.pConnString, pUser, sizeof( g.Primary.info.pConnString ) / sizeof( TEXTCHAR ) );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetUser( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pUser );
	StrCpyEx( g.Primary.info.pID, pUser, sizeof( g.Primary.info.pID ) / sizeof( TEXTCHAR ) );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetPassword( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pPassword );
	StrCpyEx( g.Primary.info.pPASSWORD, pPassword, sizeof( g.Primary.info.pPASSWORD ) );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetFallback( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bFallback );
	g.flags.bFallback = bFallback;
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetRequirePrimaryConnection( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bRequired );
	g.Primary.flags.bForceConnection = bRequired;
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetRequireConnection( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bRequired );
	g.flags.bRequireConnection = bRequired;
	return psv;
}

static uintptr_t CPROC SetRequireBackupConnection( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bRequired );
	g.Backup.flags.bForceConnection = bRequired;
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetBackupUser( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pUser );
	StrCpyEx( g.Backup.info.pID, pUser, sizeof( g.Primary.info.pID ) );
	return psv;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uintptr_t CPROC SetBackupPassword( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, pPassword );
	StrCpyEx( g.Backup.info.pPASSWORD, pPassword, sizeof( g.Backup.info.pPASSWORD ) );
	return psv;
}

static uintptr_t CPROC SetLoggingEnabled( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bEnable );
	g.flags.bLogging = bEnable;
	return psv;
}
static uintptr_t CPROC SetLoggingEnabled2( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bEnable );
	g.flags.bNoLog = !bEnable;
	return psv;
}

static uintptr_t CPROC SetLoggingEnabled3( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bEnable );
	g.flags.bLogData = bEnable;
	return psv;
}

static uintptr_t CPROC SetAutoCheckpoint( uintptr_t psv, arg_list args ) {
	PARAM( args, LOGICAL, bEnable );
	g.flags.bAutoCheckpoint = bEnable;
	return psv;
}

static uintptr_t CPROC SetLogOptions( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bEnable );
	g.flags.bLogOptionConnection = bEnable;
	return psv;
}

void SetSQLLoggingDisable( PODBC odbc, LOGICAL bDisable )
{
	//lprintf( WIDE( "%s SQL logging on %p" ), bDisable?WIDE( "disabling" ):WIDE( "enabling" ), odbc );
	if( odbc )
	{
		odbc->flags.bNoLogging = bDisable;
	}
}

void SetSQLThreadProtect( PODBC odbc, LOGICAL bEnable )
{
	if( odbc )
	{
		odbc->flags.bThreadProtect = bEnable;
		if( bEnable )
		{
			InitializeCriticalSec( &odbc->cs );
		}
		else
			DeleteCriticalSec( &odbc->cs );

	}
}

void SetSQLAutoTransact( PODBC odbc, LOGICAL bEnable )
{
	if( odbc )
		odbc->flags.bAutoTransact = bEnable;
}

void SetSQLAutoTransactCallback( PODBC odbc, void (CPROC*callback)(uintptr_t,PODBC), uintptr_t psv )
{
	if( odbc )
	{
		if( callback )
			odbc->flags.bAutoTransact = 1;
		else
			odbc->flags.bAutoTransact = 0;
		odbc->auto_commit_callback = callback;
		odbc->auto_commit_callback_psv = psv;
	}
}

void SetSQLAutoClose( PODBC odbc, LOGICAL bEnable )
{
	if( odbc )
	{
		odbc->flags.bAutoClose = bEnable;
		if( bEnable )
		{
			// no thread already, and it is connected (otherwise later, connection will be made, and this scheduled)
			if( !odbc->auto_close_thread && odbc->flags.bConnected )
				odbc->auto_close_thread = ThreadTo( AutoCloseThread, (uintptr_t)odbc );
		}
		else
		{
			if( odbc->auto_close_thread )
			{
				WakeThread( odbc->auto_close_thread );
			}
		}
	}
}

void SetSQLAutoCheckpoint( PODBC odbc, LOGICAL bEnable )
{
	if( odbc )
	{
		odbc->flags.bAutoCheckpoint = bEnable;
		if( bEnable )
		{
			// next command will enable checkpoint thread.
		}
		else
		{
			// if disabling, wake up an in-progress thread so it can quit
			if( odbc->auto_checkpoint_thread )
			{
				WakeThread( odbc->auto_checkpoint_thread );
			}
		}
	}
}

LOGICAL GetSQLAutoCheckpoint( PODBC odbc )
{
	if( odbc )
		return odbc->flags.bAutoCheckpoint;
	return 0;
}

LOGICAL EnsureLogOpen( PODBC odbc )
{
	if( odbc && odbc->flags.bNoLogging )
		return FALSE;
	if( g.flags.bLogging )
	{
		if( !g.pSQLLog )
		{
			TEXTCHAR logname[64];
			int attempt = 0;
			do
			{
				// this should be an option...
				if( attempt )
					tnprintf( logname, sizeof( logname ), WIDE( "sql%d.log" ), attempt );
				else
					StrCpyEx( logname, WIDE("sql.log"), sizeof( logname ) );
				attempt++;
				// this is going to be more hassle to conserve
				// than benefit merits.
				g.pSQLLog = sack_fopen( 0, logname, WIDE("at+") );
				if( !g.pSQLLog )
					g.pSQLLog = sack_fopen( 0, logname, WIDE("wt") );
			}
			while( !g.pSQLLog );
		}
		return TRUE;
	}
	return FALSE;
}

void SqlStubInitLibrary( void )
{
	if( !g.flags.bCriticalSectionInited )
	{
		InitializeCriticalSec( &g.Init );
		g.flags.bCriticalSectionInited = 1;
	}
	EnterCriticalSec( &g.Init );
	if( !g.flags.bInited )
	{
		// since they will never have priors, they will
		// never get deleted....as only those with priors
		// deleted when popped.
		if( g.feedback_handler ) g.feedback_handler( WIDE("Loading ODBC") );
		//CreateCollector( 0, &g.Primary, FALSE );
		//CreateCollector( 0, &g.Backup, FALSE );
#ifdef __ANDROID__
		g.OptionDb.info.pDSN = StrDup( WIDE( "./option.db" ) );
#else
#   ifdef __LINUX__
		g.OptionDb.info.pDSN = StrDup( WIDE( "~/.option.db" ) );
#   else
		g.OptionDb.info.pDSN = StrDup( WIDE( "*/../option.db" ) );
#   endif
#endif
		// default to new option database.
#ifndef __NO_OPTIONS__
		//SetOptionDatabaseOption( &g.OptionDb, TRUE );
#endif
  		g.flags.bAutoCheckpoint = 1;
		{
			LOGICAL success = FALSE;
			PCONFIG_HANDLER pch = CreateConfigurationHandler();
			AddConfigurationMethod( pch, WIDE("Auto Checkpoint=%b"), SetAutoCheckpoint );
			AddConfigurationMethod( pch, WIDE("Option DSN=%m"), SetOptionDSN );
			AddConfigurationMethod( pch, WIDE("Primary DSN=%m"), SetPrimaryDSN );
			AddConfigurationMethod( pch, WIDE("Primary User=%m"), SetUser );
			AddConfigurationMethod( pch, WIDE("Primary Connection String=%m"), SetConnString );
			AddConfigurationMethod( pch, WIDE("Primary Password=%m"), SetPassword );
			AddConfigurationMethod( pch, WIDE("Backup DSN=%m"), SetBackupDSN );
			AddConfigurationMethod( pch, WIDE("Backup User=%m"), SetBackupUser );
			AddConfigurationMethod( pch, WIDE("Backup Password=%m"), SetBackupPassword );
			AddConfigurationMethod( pch, WIDE("Log enable=%b"), SetLoggingEnabled );
			AddConfigurationMethod( pch, WIDE("LogFile enable=%b"), SetLoggingEnabled2 );
			AddConfigurationMethod( pch, WIDE("LogFile enable dump data=%b"), SetLoggingEnabled3 );
			AddConfigurationMethod( pch, WIDE("Log Option Connection=%b"), SetLogOptions );
			AddConfigurationMethod( pch, WIDE("Fallback on failure=%b"), SetFallback );
			AddConfigurationMethod( pch, WIDE("Require Connection=%b"), SetRequireConnection );
			AddConfigurationMethod( pch, WIDE("Require Primary Connection=%b"), SetRequirePrimaryConnection );
			AddConfigurationMethod( pch, WIDE("Require Backup Connection=%b"), SetRequireBackupConnection );
			AddConfigurationMethod( pch, WIDE("Database Init SQL=%m"), AddDatabaseInit );
			AddConfigurationMethod( pch, WIDE("Option Database Init SQL=%m"), AddOptionDatabaseInit );
			// If source is encrypted enable tranlation
			//AddConfigurationFilter( pch, TranslateCrypt );
			{
				TEXTCHAR tmp[256];
				tnprintf( tmp, 256, WIDE("%s.sql.config"), GetProgramName() );
				success = ProcessConfigurationFile( pch, tmp, 0 );
			}

			if( !success && !ProcessConfigurationFile( pch, WIDE("sql.config"), 0 ) )
			{
				FILE *file;
				file = sack_fopen( 1
					, WIDE("*/sql.config")
					, WIDE("wt")
#ifdef _UNICODE
					WIDE(", ccs=UNICODE")
#endif
					);
				if( file )
				{
					sack_fprintf( file, "Auto Checkpoint=No\n" );
					sack_fprintf( file, "Option DSN=%s\n", g.OptionDb.info.pDSN );
					sack_fprintf( file, "Primary DSN=MySQL\n" );
					sack_fprintf( file, "#Primary User=\n" );
					sack_fprintf( file, "#Primary Password=\n" );
					sack_fprintf( file, "Backup DSN=MySQL2\n" );
					sack_fprintf( file, "#Backup User=\n" );
					sack_fprintf( file, "#Backup Password=\n" );
					sack_fprintf( file, "Log enable=No\n"  );
					sack_fprintf( file, "LogFile enable=No\n"  );
					sack_fprintf( file, "LogFile enable dump data=No\n"  );
					sack_fprintf( file, "Log Option Connection=No\n"  );
					sack_fprintf( file, "Fallback on failure=No\n" );
					sack_fprintf( file, "Require Connection=No\n" );
					sack_fprintf( file, "Require Primary Connection=Yes\n" );
					sack_fprintf( file, "Require Backup Connection=No\n" );
					sack_fprintf( file, "Database Init SQL=\n" );
					sack_fprintf( file, "Option Database Init SQL=\n" );
					sack_fclose( file );
				}
				ProcessConfigurationFile( pch, WIDE("sql.config"), 0 );
			}
			DestroyConfigurationEvaluator( pch );
		}
		if( !g.Backup.info.pDSN || !g.Backup.info.pDSN[0] )
		{
			g.flags.bNoBackup = 1;
		}
		// allow log to be delayed opened only when something needs to be written the first time.
		g.TimerCollect.pvt_out = VarTextCreate();
		g.TimerCollect.pvt_result = VarTextCreate();
		g.TimerCollect.pvt_errorinfo = VarTextCreate();
		g.flags.bInited = TRUE;
		if( g.feedback_handler ) g.feedback_handler( WIDE("SQL Connecting...") );
	}
	LeaveCriticalSec( &g.Init );
}

#if 0
#undef EnterCriticalSec
#undef LeaveCriticalSec

#define EnterCriticalSec(a) lprintf( "Enter section %p %d", a, odbc->nProtect ); EnterCriticalSection( a ); lprintf( "In Section %p %d", a, odbc->nProtect );
#define LeaveCriticalSec(a) lprintf( "Leave section %p %d", a, odbc->nProtect ); LeaveCriticalSection( a ); lprintf( "Out Section %p %d", a, odbc->nProtect );
#endif

//-----------------------------------------------------------------------
static int bOpening; // expose this so dump info doesn't open on failed open.

PRIORITY_PRELOAD( FinalDeadstart, SYSLOG_PRELOAD_PRIORITY + 1 )
{
	g.flags.bDeadstartCompleted = 1;
}

void ParseDSN( CTEXTSTR dsn, char **vfs, char **vfsInfo, char **dbFile ) {

	static TEXTCHAR *tmpvfsvfs;
	static TEXTCHAR *tmp_name;
	static char *tmp;
	if( tmpvfsvfs ) { Release( tmpvfsvfs ); tmpvfsvfs = NULL; }
	if( tmp_name ) { Release( tmp_name ); tmp_name = NULL; }
	if( tmp ) { Release( tmp ); tmp = NULL; }

	if( dsn[0] == '$' ) {
		char *vfs_name;
		CTEXTSTR vfs_end = StrChr( dsn + 1, '@' );
		CTEXTSTR vfs_vfs_end = StrRChr( vfs_end ? vfs_end : (dsn + 1), '$' );
		TEXTCHAR *tmpvfs;

		if( vfs_vfs_end ) {
			tmp_name = ExpandPath( vfs_vfs_end + 1 );
			if( vfs_end && vfs_end < vfs_vfs_end ) {
				tmpvfs = NewArray( TEXTCHAR, (vfs_end - dsn) + 1 );
				tmpvfsvfs = NewArray( TEXTCHAR, (vfs_vfs_end - vfs_end) + 1 );
				StrCpyEx( tmpvfs, dsn + 1, vfs_end - dsn );
				StrCpyEx( tmpvfsvfs, dsn + 1, vfs_vfs_end - vfs_end );
				vfs_name = NewArray( TEXTCHAR, vfs_vfs_end - dsn );
				StrCpyEx( vfs_name, dsn + 1, vfs_vfs_end - dsn );
				StrCpyEx( tmpvfsvfs, vfs_end + 1, vfs_vfs_end - vfs_end );
			}
			else {
				tmpvfsvfs = NULL;
				tmpvfs = NewArray( TEXTCHAR, (vfs_vfs_end - dsn) + 1 );
				StrCpyEx( tmpvfs, dsn + 1, vfs_vfs_end - dsn );
				vfs_name = CStrDup( tmpvfs );
			}
			Deallocate( TEXTCHAR*, tmpvfs );
		}
		else {
			tmp_name = ExpandPath( dsn + 1 );
			vfs_name = NULL;
			tmpvfsvfs = NULL;
		}
		tmp = CStrDup( tmp_name );
		(*vfs) = vfs_name;
		(*vfsInfo) = tmpvfsvfs;
		(*dbFile) = tmp;

		Deallocate( TEXTCHAR *, tmp_name );
		tmp_name = NULL;
	}
	else {
		if( StrCaseCmpEx( dsn, "file:", 5 ) == 0
			|| StrCaseCmpEx( dsn, ":memory:", 8 ) == 0 ) {
			//lprintf( "open:%s", dsn );
			(*vfs) = NULL;
			(*vfsInfo) = NULL;
			(*dbFile) =(char*) dsn;
		}
		else {
			tmp_name = ExpandPath( dsn );
			tmp = CStrDup( tmp_name );
			(*vfs) = NULL;
			(*vfsInfo) = NULL;
			(*dbFile) = tmp;
			Deallocate( TEXTCHAR *, tmp_name );
			tmp_name = NULL;
		}
	}


}

int OpenSQLConnectionEx( PODBC odbc DBG_PASS )
{
	int state = 0;
#ifdef USE_ODBC
	RETCODE rc;
	PTEXT pConnect;
#endif
	if( !odbc->info.pDSN )
	{
		return FALSE;
	}
	if( odbc->flags.bConnected )
	{
		return TRUE;
	}
#ifdef SQLPROXY_LIBRARY_SOURCE
	SqlStubInitLibrary();
#endif

	bOpening = TRUE;

	if( StrStr( odbc->info.pDSN, WIDE(".db") ) || ( StrCmp( odbc->info.pDSN, ":memory:" ) == 0 ) )
		odbc->flags.bSkipODBC = 1;
	else
		odbc->flags.bSkipODBC = 0; // make sure it's set to something...
	/*
	 * should fix this someday to check just .mdb files...
	 if( strstr( odbc->info.pDSN, ".mdb" ) )
	 ;
	 */
#if defined( USE_ODBC ) || defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	do
	{
		if( g.flags.bDeadstartCompleted )
		{
			TEXTCHAR msg[80];
			tnprintf( msg, sizeof( msg )/sizeof(TEXTCHAR), WIDE( "Connecting to [%s]%*.*s" ), odbc->info.pDSN, 3+state, 3+state,WIDE( "........." ) );
			msg[79] = 0; // stupid tnprintf, this is going to SUCK.  What the hell microsoft?!
			if( g.feedback_handler )
			{
				//_lprintf(DBG_VOIDRELAY)( WIDE( "%s" ), msg );
				g.feedback_handler( msg );
			}

			state++;
			if( state == 5 )
				state = 0;
		}

#ifdef USE_ODBC
		if( !odbc->env && !odbc->flags.bSkipODBC )
		{
#ifdef LOG_EVERYTHING
			lprintf( WIDE("get env") );
#endif
			rc = SQLAllocEnv( &odbc->env );
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
			{
				bOpening = FALSE;
				lprintf( WIDE("Fatal error, Could not allocate SQL resource.") );
				exit(1);
				return FALSE;
			}
#ifdef LOG_EVERYTHING
			lprintf( WIDE( "%p new ENV rc %d = %p" ), odbc, rc, odbc->env );
#endif
		}

		if( !odbc->hdbc && !odbc->flags.bSkipODBC )
		{
#ifdef LOG_EVERYTHING
			lprintf( WIDE("get hdbc") );
#endif
			if( rc = SQLAllocConnect( odbc->env,
									  &odbc->hdbc ) )
			{
				lprintf( WIDE("Fatal error, Could not allocate SQL resource.") );
				SQLFreeEnv( odbc->env );
				odbc->env = 0;
				bOpening = FALSE;
				return FALSE;
			}
#ifdef LOG_EVERYTHING
			lprintf( WIDE( "%p new HDBC rc %d = %p" ), odbc, rc, odbc->hdbc );
#endif
		}
		//lprintf( WIDE("connect...%d"), odbc->flags.bConnected );

		if( !odbc->flags.bConnected && !odbc->flags.bSkipODBC )
		{
			int variation;
			PVARTEXT pvt = VarTextCreate();
#ifdef LOG_EVERYTHING
			lprintf( WIDE("Begin ODBC Driver Connect... (may take a LONG time.)") );
#endif
			for( variation = 0; variation < 3; variation++ )
			{
				VarTextEmpty( pvt );
				if( variation == 0 && odbc->info.pConnString[0] == 0 )
					vtprintf( pvt
							  , WIDE("DSN=%s%s%s%s%s")
							  , odbc->info.pDSN
							  , odbc->info.pID[0]?WIDE(";UID="):WIDE("")
							  , odbc->info.pID[0]?odbc->info.pID:WIDE("")
							  , odbc->info.pPASSWORD[0]?WIDE(";PWD="):WIDE("")
							  , odbc->info.pPASSWORD[0]?odbc->info.pPASSWORD:WIDE("")
							  );
				else if( variation == 1 )
					vtprintf( pvt
							  , WIDE("DSN=%s%s%s%s%s%s%s")
							  , odbc->info.pDSN
							  , odbc->info.pConnString[0]?WIDE(";DBQ="):WIDE("")
							  , odbc->info.pConnString[0]?odbc->info.pConnString:WIDE("")
							  , odbc->info.pID[0]?WIDE(";UID="):WIDE("")
							  , odbc->info.pID[0]?odbc->info.pID:WIDE("")
							  , odbc->info.pPASSWORD[0]?WIDE(";PWD="):WIDE("")
							  , odbc->info.pPASSWORD[0]?odbc->info.pPASSWORD:WIDE("")
							  );
				else if( variation == 2 )
					vtprintf( pvt
							  , WIDE("DRIVER=Microsoft Access Driver (*.mdb); UID=admin; UserCommitSync=Yes; Threads=30; SafeTransactions=0; PageTimeout=5; MaxScanRows=8; MaxBufferSize=2048; FIL=MS Access; DriverId=25; DefaultDir=.; DBQ=%s; ")
							  , odbc->info.pDSN
								//, odbc->info.pID[0]?WIDE(";UID="):WIDE("")
								//, odbc->info.pID[0]?odbc->info.pID:WIDE("")
								//, odbc->info.pPASSWORD[0]?WIDE(";PWD="):WIDE("")
								//, odbc->info.pPASSWORD[0]?odbc->info.pPASSWORD:WIDE("")
							  );
				else
					continue;

				pConnect = VarTextGet( pvt );
				if( g.flags.bDeadstartCompleted && (!g.flags.bNoLog) && (~odbc->flags.bNoLogging) )
					_lprintf(DBG_RELAY)( WIDE("Begin ODBC Driver Connect to [%s]"), GetText(pConnect) );

				rc = SQLDriverConnect( odbc->hdbc
											, NULL // window handle - do not show dialogs
											,
#ifdef _UNICODE
											 (SQLWCHAR*)GetText( pConnect )
#else
											 (SQLCHAR*)GetText( pConnect )
#endif
											, (SQLSMALLINT)GetTextSize( pConnect )
											, NULL
											, 0
											, NULL
											, SQL_DRIVER_NOPROMPT );

#ifdef LOG_EVERYTHING
				lprintf( WIDE("How long was that?") );
#endif
				LineRelease( pConnect );
				lprintf( WIDE("rc == %d"), rc );
				if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
				{
					//PTEXT result;
					if( !odbc->info.pID[0] && !odbc->info.pPASSWORD[0] )
					{
						odbc->info.flags.bAutoUser = 1;
						// can set odbc->info.pID == USER_TEXT_NAME
						// and odbc->info->pPASSWORD == USER_PASSWORD
						// however, this implementatino trusts implicitly the odbc connection
						// to grant my permissions.
					}
					else
					{
						if( odbc->info.flags.bAutoUser )
						{
							// reset this back to initial conditions, so
							// we rotate to default and retry with user/password.
							odbc->info.pID[0] = 0;
							odbc->info.pPASSWORD[0] = 0;
						}
					}
					DumpInfoEx( odbc, pvt, SQL_HANDLE_DBC, &odbc->hdbc, odbc->flags.bNoLogging DBG_RELAY );
				}
				else
				{
					// hmm something odd here looks like
					// someone suspects that update tasks (while processing)
					// may result with a different primary odbc than they were
					// processing?!
					PODBC SaveOdbc = g.odbc;
					g.odbc = odbc;
					{
						TEXTCHAR buffer[256];
						SQLSMALLINT reslen;
						SQLGetInfo( odbc->hdbc, SQL_DRIVER_NAME, buffer, (SQLSMALLINT)sizeof( buffer ), &reslen );
						if( g.flags.bDeadstartCompleted && (!g.flags.bNoLog) && (~odbc->flags.bNoLogging) )
							lprintf( WIDE("Driver name = %s"), buffer );
						if( strcmp( buffer, WIDE("odbcjt32.dll") ) == 0 )
							odbc->flags.bAccess = 1;
						if( strcmp( buffer, WIDE("sqlite3odbc.dll") ) == 0 || strcmp( buffer, WIDE("sqliteodbc.dll") ) == 0 ) {
							odbc->flags.bSQLite = 1;
						}
						if( !odbc->flags.bSQLite )
							odbc->flags.bAutoCheckpoint = 0;
						SQLGetInfo( odbc->hdbc, SQL_DRIVER_ODBC_VER, buffer, (SQLSMALLINT)sizeof( buffer ), &reslen );
#ifdef LOG_EVERYTHING
						lprintf( WIDE("Driver name = %s"), buffer );
#endif
						SQLGetInfo( odbc->hdbc, SQL_DRIVER_VER, buffer, (SQLSMALLINT)sizeof( buffer ), &reslen );
#ifdef LOG_EVERYTHING
						lprintf( WIDE("Driver name = %s"), buffer );
#endif
						if( StrCaseCmp( buffer, WIDE("05.01.0005") ) == 0 )
						{
							odbc->flags.bFailEnvOnDbcFail = 1;
						}
					}
					odbc->flags.bConnected = TRUE;
					odbc->flags.bODBC = TRUE;
					if( odbc->flags.bAutoClose )
					{
						if( !odbc->auto_close_thread )
							odbc->auto_close_thread = ThreadTo( AutoCloseThread, (uintptr_t)odbc );
					}
					VarTextDestroy( &pvt );
					{
						PUPDATE_TASK task;
						for( task = g.UpdateTasks; task; task = task->next )
						{
							if( task->CheckTables )
							{
								Log( WIDE("invoking plugin to check tables...") );
								task->CheckTables( odbc );
							}
						}
					}
					g.odbc = SaveOdbc;
					bOpening = FALSE;
					if( g.feedback_handler ) g.feedback_handler( WIDE("SQL Connect OK") );
					VarTextDestroy( &pvt );
					return TRUE;
				}
			}
			VarTextDestroy( &pvt );
		}
#endif

		if( !odbc->flags.bConnected &&
			( g.flags.bFallback || odbc->flags.bSkipODBC ) )
		{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
			int rc3;
#ifndef USE_ODBC
			//lprintf( "ODBC not compiled, attempting to use SQLITE which is enabled..." );
#else
			//lprintf( "ODBC Failed, attempting to use SQLITE which is enabled..." );
#endif
			// suffix of dsn was not '.db'
			// and - we REQUIRE connection...
			if( !( odbc->flags.bForceConnection && !odbc->flags.bSkipODBC ) )
			{
				char *tmp;
				char *vfs_name;
				TEXTCHAR *tmpvfsvfs;

				ParseDSN( odbc->info.pDSN, &vfs_name, &tmpvfsvfs, &tmp );
				if( vfs_name )
				//if( odbc->info.pDSN[0] == '$' )
				{
					if( vfs_name )
					{
#ifdef USE_SQLITE_INTERFACE
#  ifdef UNICODE
						TEXTCHAR *_vfs_name = DupCStr( vfs_name );
						char *_tmpvfsvfs = CStrDup( tmpvfsvfs );
#    define vfs_name _vfs_name
#    define tmpvfsvfs _tmpvfsvfs
#  endif
						sqlite_iface->InitVFS( vfs_name, sack_get_mounted_filesystem( tmpvfsvfs ) );
#  ifdef UNICODE
						Deallocate( TEXTCHAR *,_vfs_name );
						Deallocate( char *, _tmpvfsvfs );
#    undef vfs_name
#    undef tmpvfsvsf
#  endif
#endif
					}
					rc3 = sqlite3_open_v2( tmp, &odbc->db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_URI, vfs_name );
				}
				else
				{
					if( StrCaseCmpEx( odbc->info.pDSN, "file:", 5 ) == 0 
						|| StrCaseCmpEx( odbc->info.pDSN, ":memory:", 8 ) == 0 ) {
						//lprintf( "open:%s", odbc->info.pDSN );
						rc3 = sqlite3_open_v2( odbc->info.pDSN, &odbc->db, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_URI, NULL );
					}
					else {
						rc3 = sqlite3_open( tmp, &odbc->db );
					}
				}
				if( rc3 )
				{
					lprintf( WIDE("Failed to connect[%s]: %s")
							 , odbc->info.pDSN
							 , sqlite3_errmsg(odbc->db) );
				}
				else
				{
					//lprintf( "Success using SQLITE" );
					// register some new handlers like now()
					odbc->flags.bConnected = TRUE;
					odbc->flags.bSQLite_native = 1;
					ExtendConnection( odbc );
				}
			}
#endif
		}
		if( !odbc->flags.bConnected )
		{
			// finally, should we retry if it's failed to gain connect?
			if( odbc->flags.bForceConnection )
				WakeableSleep( 1000 ); // wait a second, then retry...
			else
			{
				bOpening = FALSE;
				return FALSE;
			}
		}
	}
	while( odbc->flags.bForceConnection && !odbc->flags.bConnected );
#else
 	bOpening = FALSE;
	// already open, and all is good...
	return FALSE;
#endif
 	bOpening = FALSE;
	// already open, and all is good...
	return TRUE;
}

#undef OpenSQLConnection
int OpenSQLConnection( PODBC odbc )
{
	return OpenSQLConnectionEx( odbc DBG_SRC );
}
//----------------------------------------------------------------------

void SQLCommit( PODBC odbc )
{
	// someone might not want it now, but we already started a thread for it....
	//if( odbc->flags.bAutoTransact )
	{
		if( odbc->flags.bThreadProtect )
		{
			EnterCriticalSec( &odbc->cs );
			odbc->nProtect++;
		}
		// we will own the odbc here, so the timer will either block, or
		// have completed, releasing this.

		// maybe we don't have a pending commit.... (wouldn't if the timer hit just before we ran)
		if( odbc->last_command_tick ) // otherwise we won't need a commit
		{
			int n = odbc->flags.bAutoTransact;
			odbc->last_command_tick = 0;
			if( odbc->auto_commit_thread )
			{
				uint32_t start = timeGetTime();
				WakeThread( odbc->auto_commit_thread );
				while( odbc->auto_commit_thread && ( ( start + 500 )> timeGetTime() ) )
					Relinquish();
				if( odbc->auto_commit_thread )
					lprintf( WIDE( "Auto commit thread stalled." ) );
			}
			// need to end the thread here too....
			odbc->flags.bAutoTransact = 0;
			// the commit command itself will cause SQLCommit to be called - so we turn off autotransact and would create a transaction thread etc...
			SQLCommand( odbc, WIDE( "COMMIT" ) );
			odbc->flags.bAutoTransact = n;
			if( odbc->auto_commit_callback )
				odbc->auto_commit_callback( odbc->auto_commit_callback_psv, odbc );
		}
		if( odbc->flags.bThreadProtect )
		{
			odbc->nProtect--;
			LeaveCriticalSec( &odbc->cs );
		}
	}
}

//----------------------------------------------------------------------

// had to create this thread - stalling out in the timer thread prevents
// all further commit action (which may unlock the Sqlite database this is
// intended for.
uintptr_t CPROC AutoCloseThread( PTHREAD thread )
{
	uintptr_t psv = GetThreadParam( thread );
	PODBC odbc = (PODBC)psv;
	int tick = 0;
	// initialize this to something; this is called when a connection opens...
	// so that makes the first operation the database open tick.
	odbc->last_command_tick_ = timeGetTime();
	while( odbc->flags.bAutoClose )
	{
		// if it expires, set tick and get out of loop
		// clearing last_command_tick will also end the thread (a manual sqlcommit on the connection)
		if( !odbc->auto_commit_thread // let commits finish first...
		  &&( odbc->last_command_tick_ < ( timeGetTime() - 1000 ) )
		  // is idle; no statements active in the collections on the connection
		  && IsOdbcIdle( odbc ) )
		{
			// make sure we still need to close (modeled more after commit; this could be less paranoid)
			if( ( !odbc->flags.bAutoClose )
			// and then claim the thread protection to prevent more statements from starting while we close this
			  || (( odbc->flags.bThreadProtect )?EnterCriticalSecNoWait( &odbc->cs, NULL ):1) )
			{
				tick = 1;
				break;
			}
		}
		WakeableSleep( 250 );
	}
	// a SQLCommit may have happened outside of this, which cleas last_command_tick
	if( tick && odbc->flags.bAutoClose )
	{
		// this will close any in progress result sets... still need a check
		CloseDatabaseEx( odbc, FALSE );
		// release our lock allowing any statement that started JUST as this ticked to resume.
		if( odbc->flags.bThreadProtect )
			LeaveCriticalSec( &odbc->cs );
	}
	odbc->auto_close_thread = NULL;
	return 0;
}

//----------------------------------------------------------------------
// had to create this thread - stalling out in the timer thread prevents
// all further commit action (which may unlock the Sqlite database this is
// intended for.
uintptr_t CPROC AutoCheckpointThread( PTHREAD thread )
{
	uintptr_t psv = GetThreadParam( thread );
	PODBC odbc = (PODBC)psv;
	int tick = 0;
	// initialize this to something; this is called when a connection opens...
	// so that makes the first operation the database open tick.
	odbc->last_command_tick_ = timeGetTime();
	while( odbc->flags.bAutoCheckpoint )
	{
		// if it expires, set tick and get out of loop
		// clearing last_command_tick will also end the thread (a manual sqlcommit on the connection)
		if( !odbc->auto_commit_thread // let commits finish first...
		  &&( odbc->last_command_tick_ < ( timeGetTime() - 250 ) )
		  // is idle; no statements active in the collections on the connection
		  && IsOdbcIdle( odbc ) )
		{
			tick = 1;
			break;
		}
		WakeableSleep( 125 );
	}
	// a SQLCommit may have happened outside of this, which cleas last_command_tick
	if( tick && odbc->flags.bAutoCheckpoint )
	{
		int oldCommit = odbc->flags.bAutoTransact;
		int oldCheckpoint = odbc->flags.bAutoCheckpoint;
		if( odbc->flags.bThreadProtect )
			EnterCriticalSec( &odbc->cs );
		odbc->flags.bAutoTransact = 0;
		odbc->flags.bAutoCheckpoint = 0;
		// this will checkpoint any in progress result sets... still need a check
		SQLCommand( odbc, WIDE( "PRAGMA wal_checkpoint" ) );
		odbc->flags.bAutoTransact = oldCommit;
		odbc->flags.bAutoCheckpoint = oldCheckpoint;
		// release our lock allowing any statement that started JUST as this ticked to resume.
		odbc->auto_checkpoint_thread = NULL;
		if( odbc->flags.bThreadProtect )
			LeaveCriticalSec( &odbc->cs );
	}// redundant; because I want to claer this before unlocking the connection
	odbc->auto_checkpoint_thread = NULL;
	return 0;
}

//----------------------------------------------------------------------

// had to create this thread - stalling out in the timer thread prevents
// all further commit action (which may unlock the Sqlite database this is
// intended for.
static uintptr_t CPROC CommitThread( PTHREAD thread )
{
	uintptr_t psv = GetThreadParam( thread );
	PODBC odbc = (PODBC)psv;
	int tick = 0;
	//lprintf( WIDE( "begin..." ) );
	while( odbc->last_command_tick )
	{
		//lprintf( WIDE( "waiting..." ) );
		// if it expires, set tick and get out of loop
		// clearing last_command_tick will also end the thread (a manual sqlcommit on the connection)
		if( odbc->last_command_tick < ( timeGetTime() - 500 ) )
		{
			if( ( !odbc->flags.bThreadProtect )
				|| EnterCriticalSecNoWait( &odbc->cs, NULL ) )
			{
				//lprintf( WIDE( "tick and out " ));
				tick = 1;
				break;
			}
		}
		WakeableSleep( 250 );
	}

	// a SQLCommit may have happened outside of this, which cleas last_command_tick
	if( odbc->last_command_tick && tick )
	{
		odbc->auto_commit_thread = NULL;
		SQLCommit( odbc );
		if( odbc->flags.bThreadProtect )
			LeaveCriticalSec( &odbc->cs );
	}
	else
	{
		odbc->auto_commit_thread = NULL;
	}
	return 0;
}

//----------------------------------------------------------------------

static void BeginTransactEx( PODBC odbc, int force )
{
	// I Only test this for SQLITE, specifically the optiondb.
	// this transaction phrase is not really as important on server based systems.
	//lprintf( WIDE( "BeginTransact." ) );
	if( !odbc )
		odbc = g.odbc;
	if( !odbc )
		return;
	if( odbc->flags.bAutoTransact || force )
	{
		uint32_t newtick = timeGetTime();
		//lprintf( WIDE( "Allowed. %lu" ), odbc->last_command_tick );

		// again with the tricky expressions....
		// if there is a last tick, then we don't do anything, and fail the expression
		//    okay if there IS a tick, then the OR is triggered, which sets the time, and it will be non zero,
		//    so that will fail the IF, but set the time.
		// if there is NOT a last command tick, then we add the timer
		odbc->last_command_tick_ = newtick;
		if( force || !odbc->last_command_tick )
		{
			int prior;
			odbc->last_command_tick = newtick;
			if( !force && !odbc->auto_commit_thread )
			{
				odbc->auto_commit_thread = ThreadTo( CommitThread, (uintptr_t)odbc );
			}
			prior = odbc->flags.bAutoTransact;
			odbc->flags.bAutoTransact = 0;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
			if( odbc->flags.bSQLite_native )
			{
				SQLCommand( odbc, WIDE( "BEGIN TRANSACTION" ) );
			}
			else
#endif
				if( odbc->flags.bAccess )
				{
					lprintf( "Unhandled; access driver, begintransaction..." );
			}
			else
			{
				SQLCommand( odbc, WIDE( "START TRANSACTION" ) );
			}
			odbc->flags.bAutoTransact = prior;
		}
		else // update the tick.
			odbc->last_command_tick = newtick;
		//lprintf( WIDE( "%p gets %lu" ), odbc, odbc->last_command_tick );
	}
	//else
	//   lprintf( WIDE( "No auto transact here." ) );
}

//----------------------------------------------------------------------

void SQLBeginTransact( PODBC odbc ) {
	BeginTransactEx( odbc, 1 );
}

//----------------------------------------------------------------------


void DispatchPriorRequests( PODBC odbc )
{
	PCOLLECT collection, next;
	if( !odbc )
	{
		collection = g.collections;
	}
	else
	{
		collection = odbc->collection;
	}
	while( collection )
	{
		// can now associate with this...
		//if( collection->odbc = g.odbc )
		if( !odbc )
		{
			if( !g.odbc )
			{
				collection->odbc = g.odbc;
				UnlinkThing( collection );
				lprintf( WIDE("Adding %p to %p at %p"), collection, odbc, &odbc->collection );
				LinkThing( odbc->collection, collection );
			}
			else
			{
				// didn't open? yet you're calling me?!" );
				return;
			}
		}
		{
			Log( WIDE("Dispach prior reqeust...") );
			next = collection->next;
			switch( collection->lastop )
			{
			case LAST_COMMAND:
				Log( WIDE("Dispach prior COMMAND") );
				__DoSQLCommand( g.odbc, collection );
				break;
			case LAST_QUERY:
				Log( WIDE("Dispach prior QUERY") );
				{
					PTEXT tmp = VarTextPeek( collection->pvt_out );
					__DoSQLQuery( g.odbc, collection, GetText( tmp ) );
					if( collection->lastop != LAST_QUERY )
					{
					// if the command is now completed... (no longer a query)
						VarTextEmpty( collection->pvt_out );
					}
				}
				break;
			case LAST_RESULT:
				Log( WIDE("Dispach prior RESULT") );
				__GetSQLResult( g.odbc, collection, TRUE );
				break;
			}
			collection = next;
		}
	}
}

//-----------------------------------------------------------------------

void GenerateResponce( PCOLLECT collection, int responce )
{
	collection->responce = responce;
#ifdef SQL_PROXY_SERVER
	//lprintf( WIDE("Generate responce.... %d"), collection->SourceID );
	// otherwise it's locally created...
	if( collection && collection->SourceID )
	{
		int first = 1;
		PTEXT cmd;
		TEXTSTR outdata;
		int len;
		if( collection->flags.bBuildResultArray )
		{
			BUFFER_LENGTH_PAIR *pairs;
			BUFFER_LENGTH_PAIR *tmp_pairs;
			INDEX idx;
			if( collection->columns )
			{
				pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * 2 * collection->columns + 1);
				pairs[0].len = sizeof( collection->columns );
				pairs[0].buffer = &collection->columns;
				tmp_pairs = pairs + 1;
				for( idx = 0; idx < collection->columns; idx++ )
				{
					tmp_pairs[idx].len = strlen( collection->results[idx] ) + 1;
					tmp_pairs[idx].buffer = (POINTER)collection->results[idx];
				}
				tmp_pairs = pairs + 1 + collection->columns;;
				for( idx = 0; idx < collection->columns; idx++ )
				{
					tmp_pairs[idx].len = strlen( collection->fields[idx] ) + 1;
					tmp_pairs[idx].buffer = (POINTER)collection->fields[idx];
				}
				SendMultiServiceEventPairs( collection->SourceID, responce, collection->columns * 2 + 1, pairs );
			}
			else
			{
				SendMultiServiceEventPairs( collection->SourceID, responce, 0, NULL );
			}
		}
		else
		{
			PTEXT cmd;
			if( responce == WM_SQL_RESULT_ERROR )
				cmd = VarTextPeek( collection->pvt_errorinfo );
			else
				cmd = VarTextPeek( collection->pvt_result );
			outdata = GetText( cmd );
			len = GetTextSize( cmd ) + 1;
			// lie for now... so we don't have to change 16 bit client yet.
			if( responce == WM_SQL_RESULT_NO_DATA )
				responce = WM_SQL_RESULT_DATA;
			lprintf( WIDE("Responce: %d %s"), responce, outdata );
			while( len > 4096 )
			{
				//MemCpy( collection->result, strlen( ofs ) );
				if( first )
					SendServiceEvent( collection->SourceID, WM_SQL_DATA_START, outdata, 4096 );
				//PostMessage( collection->hWnd, WM_SQL_DATA_START, (WPARAM)hWnd, (LPARAM)outatom );
				else
					SendServiceEvent( collection->SourceID, WM_SQL_DATA_MORE, outdata, 4096 );
				//PostMessage( collection->hWnd, WM_SQL_DATA_MORE, (WPARAM)hWnd, (LPARAM)outatom );
				outdata += 4096;
				len -= 4096;
			}
			//PostMessage( collection->hWnd, responce, (WPARAM)hWnd, (LPARAM)outatom );
			SendServiceEvent( collection->SourceID, responce, outdata, len );
			// has been sent, can be released.
			LineRelease( cmd );
		}
	}
#endif
}

//-----------------------------------------------------------------------

int OpenSQL( DBG_VOIDPASS )
{
	static uint32_t _bOpening;
	int bPrimaryComingUp = FALSE;
	int bBackupComingUp = FALSE;
	while( LockedExchange( &_bOpening, 1 ) )
	{
		lprintf( WIDE("Stacked waits here will be very very bad...") );
		lprintf( WIDE("Already attepting to open... hold on... we're trying ...") );
		IdleFor( 250 );
		//return -1; // better to continue (cause test is if !OpenSQL()) than return false false
	}
	if( !g.flags.bPrimaryUp && !g.flags.bBackupUp )
	{
		// had some sort of logging here...
	}
	if( !g.flags.bPrimaryUp )
	{
		//if(  g.PrimaryLastConnect < timeGetTime()
		//	&& ( g.PrimaryLastConnect = timeGetTime() + 1000 ) )
		{
#ifdef LOG_ACTUAL_CONNECTION
			lprintf( "Begin connection gPrimary=%d", g.Primary.flags.bConnected );
#endif
			if( OpenSQLConnectionEx( &g.Primary DBG_RELAY ) )
				if( !g.flags.bPrimaryUp )
					bPrimaryComingUp = TRUE;
#ifdef LOG_ACTUAL_CONNECTION
			lprintf( "after connection gPrimary=%d", g.Primary.flags.bConnected );
#endif
		}
	}
#ifdef LOG_ACTUAL_CONNECTION
	else
		lprintf( "primary up..." );
#endif
	if( !g.flags.bNoBackup
		&&(  !g.flags.bBackupUp ) )
	{
		//if ( g.BackupLastConnect < timeGetTime()
		//	&& ( g.BackupLastConnect = timeGetTime() + 1000 ) )
		{
#ifdef LOG_ACTUAL_CONNECTION
			lprintf( "Begin connection &gBackup=%p", g.Backup );
#endif
			if( OpenSQLConnectionEx( &g.Backup DBG_RELAY ) )
			{
				if( !g.flags.bBackupUp )
					bBackupComingUp = TRUE;
			}
			else
			{
				if( g.feedback_handler )
					g.feedback_handler( WIDE("Disabling backup - stop retrying.") );
				else
					lprintf( WIDE("Disabling backup - stop retrying.") );
				g.flags.bNoBackup = 1;
			}
#ifdef LOG_ACTUAL_CONNECTION
			lprintf( "Begin connection &gBackup=%p", g.Backup );
#endif

		}
	}
#ifdef LOG_ACTUAL_CONNECTION
	else
		lprintf( "%s %s."
				 , g.flags.bNoBackup?"No Backup...":"backup present and..."
				 , g.flags.bBackupUp?"Backup is up.":"Backup Down" );
#endif

	if( !g.odbc ) // no connection at all....
	{
		if( bPrimaryComingUp )
		{
			g.odbc = &g.Primary;
			g.flags.bPrimaryUp = 1;
		}
		else if( bBackupComingUp )
		{
			g.odbc= &g.Backup;
			g.flags.bBackupUp = 1;
		}
		else
		{
			// else nothing's coming up - can't set anything as up...
			g.flags.bBadODBC = 1; // better let the program know soonest...
		}
	}
	else // have a connection already..
	{
		// connection is to the backup....
		if( g.odbc == &g.Backup )
		{
			if( bPrimaryComingUp )
			{
				//PUPDATE_TASK task = g.UpdateTasks;
#ifdef LOG_ACTUAL_CONNECTION
				Log( WIDE("Primary is coming up - invoke primary recovered") );
#endif
				g.odbc = &g.Primary;
				g.flags.bPrimaryUp = 1;
			}
		}
		else // connection is already primary....
		{
			// but now we have a new backup?
			if( bBackupComingUp )
			{
				// hmm have a primary, and now have a backup
				// invoke BackupRestore ?  to synchronize tables with
				// primary?
				g.flags.bBackupUp = 1;
			}
		}
	}
#ifdef SQL_PROXY_SERVER
	if( g.flags.bPrimaryUp )
	{
		if( g.flags.bNoBackup )
			ChangeIcon( PROXY_FULL );
		else
			if( g.flags.bBackupUp )
				ChangeIcon( PROXY_FULL );
			else
				ChangeIcon( PROXY_PRIMARY );
	}
	else
	{
		if( g.flags.bNoBackup )
			ChangeIcon( PROXY_DOWN );
		else
			if( g.flags.bBackupUp )
				ChangeIcon( WIDE("bck.bmp") );
			else
				ChangeIcon( PROXY_DOWN );
	}
#endif
	// result TRUE if anything is up...
	if( g.flags.bPrimaryUp || g.flags.bBackupUp )
	{
		// dispatch any outstanding requests....
		// dispatch pending requests for primary/backup default connections
		if( bPrimaryComingUp || bBackupComingUp )
		{
			if( g.feedback_handler ) g.feedback_handler( WIDE("SQL Connected OK") );
		}
		_bOpening = FALSE;
		/* oh - I guess we can end up attempting to open here.. */
		DispatchPriorRequests( NULL );
		return TRUE;
	}

	// generate error
	_bOpening = FALSE;
	return FALSE;
}

//----------------------------------------------------------------------

void FailConnection( PODBC odbc )
{
	int is_default = 0;
	// g.odbc failed!
	if( odbc )
	{
		PTEXT text;
		PVARTEXT pvt = VarTextCreate();
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		if( odbc->flags.bSQLite_native )
			DumpInfo2( pvt, SQL_HANDLE_DBC, odbc, odbc->flags.bNoLogging );
#endif
#ifdef USE_ODBC
		if( odbc->flags.bODBC )
		{
			lprintf( WIDE("logging HDBC... (and closing)") );
			DumpInfo( odbc, pvt, SQL_HANDLE_DBC, &odbc->hdbc, odbc->flags.bNoLogging );
			if( odbc->flags.bFailEnvOnDbcFail )
				DumpInfo( odbc, pvt, SQL_HANDLE_ENV, &odbc->env, odbc->flags.bNoLogging );

		}
#endif
		text = VarTextPeek( pvt );
		Log1( WIDE("Status of DBC==%s"), text?GetText( text ):WIDE("NO ERROR RESULT") );
		VarTextDestroy( &pvt );
		odbc->flags.bConnected = 0;
	}
	if( odbc == &g.Primary )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT text;
		is_default = 1;
		Log( WIDE("Failed the primary connection!") );
		// DumpInfo results in the hdbc being closed...
		text = VarTextPeek( pvt );
		Log1( WIDE("Status of DBC==%s"), text?GetText( text ):WIDE("NO ERROR RESULT") );
		VarTextDestroy( &pvt );
		g.Primary.flags.bConnected = 0;
		g.flags.bPrimaryUp = FALSE;
		g.odbc = NULL;
	}
	else if( odbc == &g.Backup )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT text;
		is_default = 1;
		Log( WIDE("Failed the BACKUP connection! this is VERY bad") );
		// DumpInfo results in the hdbc being closed...
		text = VarTextPeek( pvt );
		Log1( WIDE("Status of DBC==%s"), text?GetText( text ):WIDE("NO ERROR RESULT") );
		VarTextDestroy( &pvt );
		g.flags.bBackupUp = FALSE;
		g.odbc = NULL;
	}
#ifdef SQL_PROXY_SERVER
	if( is_default )
	{
		if( g.flags.bPrimaryUp )
		{
			if( g.flags.bNoBackup )
				ChangeIcon( PROXY_FULL );
			else
				if( g.flags.bBackupUp )
					ChangeIcon( PROXY_FULL );
				else
					ChangeIcon( PROXY_PRIMARY );
		}
		else
		{
			if( g.flags.bNoBackup )
				ChangeIcon( PROXY_DOWN );
			else
				if( g.flags.bBackupUp )
					ChangeIcon( PROXY_BACKUP );
				else
					ChangeIcon( PROXY_DOWN );
		}
	}
#endif
	//Log( WIDE("Attempting to recover a connection") );
	if( is_default )
	{
		//lprintf( "Default connectionf ailed... open general (backup/primary)" );
		OpenSQL( DBG_VOIDSRC );
	}
	else
	{
		//lprintf( "re-open self?" );
		OpenSQLConnectionEx( odbc DBG_SRC );
		//lprintf( "... " );
	}
}

//----------------------------------------------------------------------

void ReleaseCollectionResults( PCOLLECT pCollect, int bEntire )
{
	{
		int idx;
		if( bEntire && pCollect->fields )
		{
			for( idx = 0; idx < pCollect->columns; idx++ )
			{
				Release( (POINTER)pCollect->fields[idx] );
			}
			Release( (POINTER)pCollect->fields );
			pCollect->fields = NULL;
			Release( pCollect->result_len );
			pCollect->result_len = NULL;
		}
		if( pCollect->results )
		{
			for( idx = 0; idx < pCollect->columns; idx++ )
			{
				if( pCollect->results[idx] )
					Release( (POINTER)pCollect->results[idx] );
			}
			Release( (POINTER)pCollect->results );
			pCollect->results = NULL;
		}
		if( bEntire )
		{
			if( pCollect->colsizes )
			{
				Release( pCollect->colsizes );
				pCollect->colsizes = NULL;
			}
			if( pCollect->coltypes )
			{
				//lprintf( "new column types - columns is 0... all is NULL" );
				Release( pCollect->coltypes );
				pCollect->coltypes = NULL;
			}
			pCollect->columns = 0;
		}
	}
	// and if pResult (single string... might clean that up...
	VarTextEmpty( pCollect->pvt_result );
	VarTextEmpty( pCollect->pvt_errorinfo );
}

//----------------------------------------------------------------------

void DestroyCollectorEx( PCOLLECT pCollect DBG_PASS )
#define DestroyCollector(c) DestroyCollectorEx(c DBG_SRC )
{
#ifdef LOG_COLLECTOR_STATES
	PODBC odbc = pCollect?pCollect->odbc:NULL;
	// added to supprot 'collectors'
	_lprintf(DBG_RELAY)( "Destroying a state, restoring prior if any... %s"
							 ,odbc?odbc->collection?odbc->collection->flags.bPushed?"pushed":"":"":"No ODBC"
			 );
	DumpODBCInfo( odbc );
#endif
// cannot destroy this local thing.
	if( !pCollect )
		return;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( pCollect->odbc->flags.bSQLite_native && pCollect->stmt )
	{
		sqlite3_finalize( pCollect->stmt );
		pCollect->stmt = NULL;
	}
#endif
#ifdef USE_ODBC
	if( pCollect->hstmt )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, pCollect->hstmt );
		pCollect->hstmt = 0;
	}
#endif

	if( !pCollect->flags.bDynamic ) // must keep at least one.
	{
		return;
	}
	ReleaseCollectionResults( pCollect, TRUE );
	VarTextDestroy( &pCollect->pvt_out );
	VarTextDestroy( &pCollect->pvt_result );
	VarTextDestroy( &pCollect->pvt_errorinfo );
#ifdef LOG_COLLECTOR_STATES
	lprintf( WIDE("pcollect %p is %p and %p"), pCollect, pCollect->next, pCollect->me );
#endif
	UnlinkThing( pCollect );
	Release( pCollect );
#ifdef LOG_COLLECTOR_STATES
	{
		collectors--;
		lprintf( "Collectors: %d", collectors );
		{
			PCOLLECT c;
			for( c = g.collections; c; c = c->next )
			{
				lprintf( "State is %s", GetText( VarTextPeek( c->pvt_out ) ) );
			}
			for( c = odbc->collection; c; c = c->next )
			{
				lprintf( "State is %s", GetText( VarTextPeek( c->pvt_out ) ) );
			}
		}
	}
#endif
}

//----------------------------------------------------------------------

PODBC ConnectToDatabaseLogin( CTEXTSTR DSN, CTEXTSTR user, CTEXTSTR pass, LOGICAL bRequireConnection DBG_PASS )
{
	PODBC pODBC;
	SqlStubInitLibrary();
	pODBC = New( ODBC );
	AddLink( &g.pOpenODBC, pODBC );
	MemSet( pODBC, 0, sizeof( ODBC ) );
	if( user )
		StrCpy( pODBC->info.pID, user );
	if( pass )
		StrCpy( pODBC->info.pPASSWORD, pass );
	pODBC->info.pDSN = StrDup( DSN );
	pODBC->flags.bAutoCheckpoint = g.flags.bAutoCheckpoint;
	pODBC->flags.bForceConnection = bRequireConnection;
	// source ID is not known...
	// is probably static link to library, rather than proxy operation
	//CreateCollector( 0, pODBC, FALSE );
	OpenSQLConnectionEx( pODBC DBG_RELAY );
	return pODBC;
}

PODBC ConnectToDatabaseExx( CTEXTSTR DSN, LOGICAL bRequireConnection DBG_PASS )
{
	return ConnectToDatabaseLogin( DSN, NULL, NULL, bRequireConnection DBG_RELAY );
}

#undef ConnectToDatabaseEx
PODBC ConnectToDatabaseEx( CTEXTSTR DSN, LOGICAL bRequireConnection )
{
	return ConnectToDatabaseLogin( DSN, NULL, NULL, bRequireConnection DBG_SRC );
}

//----------------------------------------------------------------------

#undef ConnectToDatabase
PODBC ConnectToDatabase( CTEXTSTR DSN )
{
	return ConnectToDatabaseLogin( DSN, NULL, NULL, g.flags.bRequireConnection DBG_SRC );
}

//----------------------------------------------------------------------

// result is 0, don't retry
			// result is 1, do retry...

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
int DumpInfo2( PVARTEXT pvt, SQLSMALLINT type, PODBC odbc, LOGICAL bNoLog )
{
	const TEXTCHAR *tmp;
	//DebugBreak();
	VarTextEmpty( pvt );
#ifdef _UNICODE
	tmp = DupCStr( sqlite3_errmsg(odbc->db) );
#else
	tmp = sqlite3_errmsg(odbc->db);
#endif
	if( StrCaseCmpEx( tmp, WIDE( "no such table" ), 13 ) == 0 )
		vtprintf( pvt, WIDE( "(S0002)" ) );
	vtprintf( pvt, WIDE( "%s" ), tmp );
	//lprintf( WIDE( "Result of prepare failed? %s at [%s]" ), tmp, tail );
	if( !bNoLog && EnsureLogOpen( odbc ) )
	{
		sack_fprintf( g.pSQLLog, WIDE("#SQLITE ERROR:%s\n"), tmp );
		sack_fflush( g.pSQLLog );
	}
	//vtprintf( pvt, "%s", sqlite3_errmsg(odbc->db) );
	return 0;
}
#endif

#ifdef USE_ODBC
int DumpInfoEx( PODBC odbc, PVARTEXT pvt, SQLSMALLINT type, SQLHANDLE *handle, LOGICAL bNoLog DBG_PASS )
{
	int retry = 0; // result of dumpinfo...
	RETCODE rc;
	TEXTCHAR statecode[6];
	TEXTCHAR message[256];
	/* SQLINTEGER is the same sort of thing as int32_t - it's a constant 32 bit integer value. */
	SQLINTEGER  native;
	short  msglen;
	SQLSMALLINT  n;
	n = 1;
#ifdef LOG_EVERYTHING
	_lprintf( DBG_RELAY )( WIDE( "Dumping Connection error Info..." ) );
#endif
	VarTextEmpty( pvt );
	do
	{
		rc = SQLGetDiagRec( type
								, *handle
								, n++,
#ifdef _UNICODE
                         (SQLWCHAR*)statecode
#else
								 (SQLCHAR*)statecode
#endif
                        ,
								&native
                         ,
#ifdef _UNICODE
								 (SQLWCHAR*)message
                         , sizeof( message )
#else
								 (SQLCHAR*)message
                         , sizeof( message )
#endif
								, &msglen );
		if( rc == SQL_INVALID_HANDLE )
		{
			vtprintf( pvt, WIDE("Invalid handle") );
			if( !bNoLog && EnsureLogOpen( odbc ) )
				sack_fprintf( g.pSQLLog, WIDE("#%s\n"), WIDE("Invalid Handle") );
			break;
		}
		else if( rc != SQL_NO_DATA )
		{
#ifdef LOG_EVERYTHING
			lprintf( WIDE( "Quick info is rc:%d  %d [%s]{%s}" ), rc, native, statecode, message );
#endif
			//DebugBreak();
			if( ( strcmp( statecode, WIDE( "IM002" ) ) == 0 ) && ( handle == &(g.Backup.hdbc) ) )
			{
				/* quiet error... */
				if( g.feedback_handler )
					g.feedback_handler( WIDE("Secondary DSN....") );
				else
					lprintf( WIDE( "Secondary DSN...." ) );
				vtprintf( pvt, WIDE("(%5s)[%") _32f WIDE("]:%s"), statecode, native, message );
				break;
				//
				//return 0;
			}

			// native 2003 == could not connect... (do not retry)
			// native 2013 == lost connection during query.
			if( ( ( strcmp( statecode, WIDE( "S1T00" ) ) == 0 ) ||
				 ( strcmp( statecode, WIDE( "08S01" ) ) == 0 ) )
				&& ( native == 2013 ) )
			{
				if( g.feedback_handler ) g.feedback_handler( WIDE("SQL Connection Lost...\nWaiting for reconnect...") );
				_lprintf(DBG_RELAY)( WIDE( "Connection was lost, closing, and attempting to reopen.  Resulting with a Retry." ) );
				vtprintf( pvt, WIDE("(%5s)[%") _32f WIDE("]:%s"), statecode, native, message );
				if( !bNoLog && EnsureLogOpen( odbc ) )
				{
					sack_fprintf( g.pSQLLog, WIDE("#(%5s)[%") _32f WIDE("]:%s\n"), statecode, native, message );
				}
				if( !bOpening )
				{
					lprintf( WIDE( "not opening, fail connection..." ) );
					FailConnection( odbc );
					if( IsSQLOpen( odbc ) )
					{
						lprintf( WIDE( "Connection closed, and re-open worked." ) );
						retry = 1;
					}
				}
				else
				{
					retry = 1;
					lprintf( WIDE( "Alsready opening?!" ) );
				}
			}
			else if( native == 2006 || native == 2003 )
			{
				if( odbc->flags.bConnected )
					if( g.feedback_handler ) g.feedback_handler( WIDE("SQL Connection Lost...\nWaiting for reconnect...") );
				_lprintf(DBG_RELAY)( WIDE( "[%s] This is 'connection lost' (not tempoary)" ), odbc->info.pDSN );

				if( !bOpening )
				{
					if( type == SQL_HANDLE_STMT )
					{
						RETCODE rc2 = SQLFreeHandle( type, *handle );
						(*handle) = 0;
						lprintf( WIDE( "Result of free (Stmt) : %d" ), rc2 );
					}
					FailConnection( odbc );
					while( !IsSQLOpen( odbc ) );
					lprintf( WIDE( "Connection closed, and re-open worked." ) );
					retry = 1;
				}
				else
				{
				}
			}
			else
			{
				lprintf( WIDE( "This is some other error (%5s)[%d]:%s" ), statecode, native, message );
				if( StrCmp( statecode, WIDE( "IM002" ) ) == 0 )
					vtprintf( pvt, WIDE("(%5s)[%") _32f WIDE("]:%s<%s>"), statecode, native, message, odbc->info.pDSN?odbc->info.pDSN:WIDE("") );
				else
					vtprintf( pvt, WIDE("(%5s)[%") _32f WIDE("]:%s"), statecode, native, message );
				if( !bNoLog && EnsureLogOpen( odbc ) )
					sack_fprintf( g.pSQLLog, WIDE("#%s\n"), GetText( VarTextPeek( pvt ) ) );
			}
		}
		else
		{
			//vtprintf( pvt, WIDE("(E0000)No Data on error!") );
			//if( g.pSQLLog )
			//	 fprintf( g.pSQLLog, WIDE("#No data for error!  This may be caused by the sql server disappearing." ) );
		}
	} while( ( rc != SQL_NO_DATA ) && (*handle) );
	if( !bNoLog && EnsureLogOpen( odbc ) )
		sack_fflush( g.pSQLLog );

#ifdef LOG_EVERYTHING
	lprintf( WIDE("Drop handle %p"), (*handle) );
#endif
	SQLFreeHandle( type, *handle );
	(*handle) = 0;
	return retry;
}
#endif

PCOLLECT FindCollection( PODBC odbc, PSERVICE_ROUTE SourceID )
{
	PCOLLECT pCollect = g.collections;
	// collectors are non-odbc connection specific.
	// they are more like a queue of pending requests against
	// either their own, specified ODBC or the primary/backup path.
#ifdef LOG_COLLECTOR_STATES
	lprintf( WIDE("Find collection for %") _32f , SourceID );
#endif
	if( odbc )
		for( pCollect = odbc->collection;
			pCollect;
			pCollect = NextThing( pCollect) )
			if( pCollect->SourceID == SourceID )
				return pCollect;
	for( pCollect = g.collections;
		pCollect;
		pCollect = NextThing( pCollect) )
		if( pCollect->SourceID == SourceID )
			return pCollect;
	// didn't find one, create one, and return it.
#ifdef LOG_COLLECTOR_STATES
	lprintf( WIDE("Creating new collector on an odbc connection") );
#endif
	return CreateCollector( SourceID, odbc, FALSE );;
}

//-----------------------------------------------------------------------
// also ifndef sql thing...

static PCOLLECT Collect( PCOLLECT collection, uint32_t *params, size_t paramlen )
{
	CTEXTSTR buffer = (CTEXTSTR)params;
	// make sure we have enough room.
	VarTextExpand( collection->pvt_out, paramlen );
	VarTextAddData( collection->pvt_out, buffer, paramlen );
	//lprintf( WIDE("Collected: %s"), buf );
	return collection;
}

void ReleaseODBC( PODBC odbc )
{
	if( odbc )
	{
		PCOLLECT pc = NULL;
		while( odbc->collection && ( pc != odbc->collection ) )
		{
			pc = odbc->collection;
			DestroyCollector( odbc->collection );
		}
	}
}

void CloseDatabaseEx( PODBC odbc, LOGICAL ReleaseConnection )
{
	uint32_t tick = GetTickCount();
	ReleaseODBC( odbc );
	odbc->flags.bClosed = 1;
	odbc->flags.bAutoCheckpoint = 0;
	odbc->last_command_tick = 0;
	while( ( (GetTickCount()-tick) < 100 ) && odbc->auto_checkpoint_thread ) {
		WakeThread( odbc->auto_checkpoint_thread );
		Relinquish();
	}
	while( odbc->auto_commit_thread )
	{
		SQLCommit( odbc );
		WakeThread( odbc->auto_commit_thread );
	}
				
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		int err = sqlite3_close( odbc->db );
		if( err )
		{
			lprintf( WIDE("sqlite3 returned %d on close..."), err );
		}
	}
	else
#endif
#ifdef USE_ODBC
	if( odbc->hdbc )
	{
		SQLDisconnect( odbc->hdbc );
		SQLFreeHandle( SQL_HANDLE_ENV, odbc->env );
		odbc->env = NULL;
		SQLFreeHandle( SQL_HANDLE_DBC, odbc->hdbc );
		odbc->hdbc = NULL;
	}
#endif
	DeleteLink( &g.pOpenODBC, odbc );
	if( ReleaseConnection )
		Release( odbc );
	else
		odbc->flags.bConnected = FALSE;
}

void CloseDatabase( PODBC odbc )
{
	CloseDatabaseEx( odbc, TRUE );
}

//-----------------------------------------------------------------------


int __DoSQLCommandEx( PODBC odbc, PCOLLECT collection DBG_PASS )
{
	int retry = 0;
#ifdef USE_ODBC
	RETCODE rc;
#endif
	PTEXT cmd;
	if( !odbc )
	{
		Log( WIDE("Delayed COMMAND") );
		collection->lastop = LAST_COMMAND;
		//collection->hLastWnd = hWnd;
		return FALSE;
	}

	if( odbc->flags.bThreadProtect )
	{
		EnterCriticalSec( &odbc->cs );
		odbc->nProtect++;
	}
corruptRetry:
	if( !OpenSQLConnectionEx( odbc DBG_SRC ) )
	{
		lprintf( WIDE("Fail connect odbc... should already be open?!") );
		GenerateResponce( collection, WM_SQL_RESULT_ERROR );
		if( odbc->flags.bThreadProtect )
		{
			odbc->nProtect--;
			LeaveCriticalSec( &odbc->cs );
		}
		return FALSE;
	}
	cmd = VarTextPeek( collection->pvt_out );
	if( EnsureLogOpen(odbc ) )
	{
		sack_fprintf( g.pSQLLog, WIDE("%s[%p]:%s\n"), odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" ), odbc, GetText( cmd ) );
		sack_fflush( g.pSQLLog );
	}
	VarTextEmpty( collection->pvt_result );
	VarTextEmpty( collection->pvt_errorinfo );

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		if( collection->stmt )
		{
			sqlite3_finalize( collection->stmt );
			collection->stmt = NULL;
		}
	}
#endif
#ifdef USE_ODBC
	if( odbc->flags.bODBC )
	{
		if( collection->hstmt )
		{
			SQLFreeHandle( SQL_HANDLE_STMT, collection->hstmt );
			collection->hstmt = 0;
		}
	}
#endif

	if( !(g.flags.bNoLog) )
	{
		if( odbc->flags.bNoLogging )
			//odbc->hidden_messages++
			;
		else
			_lprintf(DBG_RELAY)( WIDE( "Do Command[%p:%s]: %s" ), odbc, odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" ), GetText( cmd ) );
	}

#ifdef LOG_EVERYTHING
	lprintf( WIDE( "sql command on %p [%s]" ), collection->hstmt, GetText( cmd ) );
#endif

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		int result_code = WM_SQL_RESULT_SUCCESS;
		int rc3;
		const TEXTCHAR *tail;
retry:
		odbc->last_command_tick_ = timeGetTime();
		if( odbc->last_command_tick )
			odbc->last_command_tick = odbc->last_command_tick_;
		// can get back what was not used when parsing...
#ifdef UNICODE
		rc3 = sqlite3_prepare16_v2( odbc->db, (void*)GetText( cmd ), (int)(GetTextSize( cmd )) * sizeof( TEXTCHAR ), &collection->stmt, (const void**)&tail );
#else
		rc3 = sqlite3_prepare_v2( odbc->db, GetText( cmd ), (int)(GetTextSize( cmd )), &collection->stmt, &tail );
#endif
		if( rc3 )
		{
			vtprintf( collection->pvt_errorinfo, "Result of prepare failed? %s at char %" _size_f "[%" _string_f "] in [%" _string_f "]"
			       , sqlite3_errmsg(odbc->db), tail - GetText(cmd), tail, GetText(cmd) );
			if( EnsureLogOpen(odbc ) )
			{
				sack_fprintf( g.pSQLLog, WIDE("#SQLITE ERROR:%") _string_f WIDE("\n"), GetText( VarTextPeek( collection->pvt_errorinfo ) ) );
				sack_fflush( g.pSQLLog );
			}
 			GenerateResponce( collection, WM_SQL_RESULT_ERROR );
			if( odbc->flags.bThreadProtect )
			{
				// had to close a prior connection...
				odbc->nProtect--;
				LeaveCriticalSec( &odbc->cs );
			}
			return FALSE;
		}
		else
		{
			if( odbc->flags.bAutoCheckpoint && (!sqlite3_stmt_readonly( collection->stmt )) )
				startAutoCheckpoint( odbc );
			rc3 = sqlite3_step( collection->stmt );
			switch( rc3 )
			{
			case SQLITE_CORRUPT:
				if( odbc->pCorruptionHandler ) {
					odbc->pCorruptionHandler( odbc->psvCorruptionHandler, odbc );
					goto corruptRetry;
				}
				break;
			case SQLITE_OK:
			case SQLITE_DONE:
			case SQLITE_ROW:
				//if( !sqlite3_get_autocommit(odbc->db) )
				{
					// this is a noisy message when we start using start trans and endtrans
					//lprintf( "Database has fallen out of auto commit mode!" );
					//DebugBreak();
				}
				break;
			case SQLITE_BUSY:
				// going to retry the statement as a whole anyhow.
				sqlite3_finalize( collection->stmt );
				if( !odbc->flags.bNoLogging )
				{
					_lprintf(DBG_RELAY)( WIDE( "Database Busy, waiting on[%p:%s]: %s" ), odbc, odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" ), GetText( cmd ) );
					//DumpAllODBCInfo();
				}
				WakeableSleep( 25 );
				goto retry;
			default:
				//  SQLITE_CONSTRAINT - statement like an insert with a key that already exists.
				if( !odbc->flags.bNoLogging )
					_lprintf(DBG_RELAY)( WIDE( "[%s] Unknown, unhandled SQLITE error: %s" ), odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" ), sqlite3_errmsg(odbc->db ) );
				else 
					vtprintf( collection->pvt_errorinfo, "[%s] Unknown, unhandled SQLITE error: %s", odbc->info.pDSN ? odbc->info.pDSN : WIDE( "NoDSN?" ), sqlite3_errmsg( odbc->db ) );
				//DebugBreak();
				result_code = WM_SQL_RESULT_ERROR;
				break;
			}
			// this should be SQLITE_OK || SQLITE_DONE
		}
		if( odbc->flags.bSQLite_native && collection->stmt )
		{
			sqlite3_finalize( collection->stmt );
			collection->stmt = NULL;
		}
		GenerateResponce( collection, result_code );
#ifdef LOG_COLLECTOR_STATES
		DumpODBCInfo( odbc );
#endif
	}
#endif
#ifdef USE_ODBC
	if( odbc->flags.bODBC )
	{
		if( !collection->hstmt )
		{
			rc = SQLAllocHandle( SQL_HANDLE_STMT
									 , odbc->hdbc
									 , &collection->hstmt );
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
			{
				lprintf( WIDE("Failed to open ODBC statement handle....") );
				GenerateResponce( collection, WM_SQL_RESULT_ERROR );
				if( odbc->flags.bThreadProtect )
				{
					odbc->nProtect--;
					LeaveCriticalSec( &odbc->cs );
				}
				return FALSE;
			}
		}
		{
			rc = SQLExecDirect( collection->hstmt,
#ifdef _UNICODE
									 (SQLWCHAR*)GetText( cmd )
#else
									 (SQLCHAR*)GetText( cmd )
#endif
									, SQL_NTS );
		}
			if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
			{
				// DumpInfo will do the logging... shouldn't need to re-log it.
				retry = DumpInfo( odbc, collection->pvt_errorinfo, SQL_HANDLE_STMT, &collection->hstmt, odbc->flags.bNoLogging );
				//cmd = VarTextPeek( collection->pvt_errorinfo );
				//DebugBreak();
				//lprintf( WIDE("ODBC Command excecution failed(1)....%s"), cmd?GetText( cmd ):WIDE("NO ERROR RESULT") );
				if( EnsureLogOpen( odbc ) )
				{
					sack_fprintf( g.pSQLLog, WIDE("#%s\n"), GetText( cmd ) );
					sack_fflush( g.pSQLLog );
				}
				//lprintf( WIDE("result err...") );
				GenerateResponce( collection, WM_SQL_RESULT_ERROR );
			}
			else
			{
				if( collection->hstmt )
				{
					SQLFreeHandle( SQL_HANDLE_STMT, collection->hstmt );
					collection->hstmt = 0;
				}
#ifdef LOG_EVERYTHING
				lprintf( WIDE("Resulting okay.") );
#endif
				GenerateResponce( collection, WM_SQL_RESULT_SUCCESS );
			}
	}
#endif

	if( odbc->flags.bThreadProtect )
	{
		odbc->nProtect--;
		LeaveCriticalSec( &odbc->cs );
	}
	//  actually we keep collections around while there's a client...
	//lprintf( WIDE("Command destroy collection...") );
	//DestroyCollector( collection );
	return retry;
}

//-----------------------------------------------------------------------
SQLPROXY_PROC( int, SQLCommandEx )( PODBC odbc, CTEXTSTR command DBG_PASS )
{
	PODBC use_odbc;
	if( odbc->flags.bClosed )
		return 0;
	if( !IsSQLOpenEx( odbc DBG_RELAY ) )
		return 0;
	if( !( use_odbc = odbc ) )
	{
		use_odbc = g.odbc;
	}
	if( use_odbc )
	{
		PCOLLECT pCollector;
		BeginTransactEx( use_odbc, 0 );
		do
		{
#ifdef LOG_COLLECTOR_STATES
			lprintf( "creating collector...cmd: %s", command );
#endif
			Collect( pCollector = CreateCollector( 0, use_odbc, TRUE ), (uint32_t*)command, (uint32_t)strlen( command ) );
			//SimpleMessageBox( NULL, "Please shut down the database...", "Waiting.." );
		} while( __DoSQLCommandEx( use_odbc, pCollector DBG_RELAY ) );
		if( use_odbc->collection )
			return use_odbc->collection->responce == WM_SQL_RESULT_SUCCESS?TRUE:0;
		return WM_SQL_RESULT_ERROR;
	}
	else
		_xlprintf(1 DBG_RELAY )( WIDE("ODBC connection has not been opened") );
	return FALSE;
}

//-----------------------------------------------------------------------

SQLPROXY_PROC( int, SQLCommandExx )(PODBC odbc, CTEXTSTR command, size_t commandLen DBG_PASS)
{
	PODBC use_odbc;
	if( odbc->flags.bClosed )
		return 0;
	if( !IsSQLOpenEx( odbc DBG_RELAY ) )
		return 0;
	if( !(use_odbc = odbc) )
	{
		use_odbc = g.odbc;
	}
	if( use_odbc )
	{
		PCOLLECT pCollector;
		BeginTransactEx( use_odbc, 0 );
		do
		{
#ifdef LOG_COLLECTOR_STATES
			lprintf( "creating collector...cmd: %s", command );
#endif
			Collect( pCollector = CreateCollector( 0, use_odbc, TRUE ), (uint32_t*)command, commandLen?commandLen:strlen( command ) );
			//SimpleMessageBox( NULL, "Please shut down the database...", "Waiting.." );
		} while( __DoSQLCommandEx( use_odbc, pCollector DBG_RELAY ) );
		if( use_odbc->collection )
			return use_odbc->collection->responce == WM_SQL_RESULT_SUCCESS ? TRUE : 0;
		return WM_SQL_RESULT_ERROR;
	}
	else
		_xlprintf( 1 DBG_RELAY )(WIDE( "ODBC connection has not been opened" ));
	return FALSE;
}

//-----------------------------------------------------------------------

int DoSQLCommandEx( CTEXTSTR command DBG_PASS )
{
	return SQLCommandEx( NULL, command DBG_RELAY );
}

//-----------------------------------------------------------------------

void __GetSQLTypes( PODBC odbc, PCOLLECT collection )
{
	VarTextEmpty( collection->pvt_result );
	VarTextEmpty( collection->pvt_errorinfo );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		lprintf( WIDE("Someone's getting types - haven't figured this one out yet...") );
		/*
		if( collection->stmt )
		{
			sqlite3_finalize( collection->stmt );
			collection->stmt = NULL;
		}
		*/
		GenerateResponce( collection, WM_SQL_RESULT_NO_DATA );
		return;
	}
#endif
#ifdef USE_ODBC
	{
		RETCODE rc;
		if( collection->hstmt )
		{
			SQLFreeHandle( SQL_HANDLE_STMT, collection->hstmt );
			collection->hstmt = 0;
		}
		rc = SQLAllocHandle( SQL_HANDLE_STMT
								 , odbc->hdbc
								 , &collection->hstmt );
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
		{
			lprintf( WIDE("Failed to open ODBC statement handle....") );
			GenerateResponce( collection, WM_SQL_RESULT_ERROR );
			return;
		}
		rc = SQLGetTypeInfo( collection->hstmt, SQL_ALL_TYPES );
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
		{
			lprintf( WIDE("Failed to get SQL types...") );
			GenerateResponce( collection, WM_SQL_RESULT_ERROR );
			return;
		}
		GenerateResponce( collection, WM_SQL_RESULT_SUCCESS );
	}
#endif

}

//-----------------------------------------------------------------------

int GetSQLTypes( void )
{
	if( !OpenSQL( DBG_VOIDSRC ) )
	{
		return FALSE;
	}
	__GetSQLTypes( g.odbc, g.odbc->collection );
	return g.odbc->collection->responce == WM_SQL_RESULT_SUCCESS?TRUE:0;
}

//-----------------------------------------------------------------------

SQLPROXY_PROC( int, FetchSQLTypes )( PODBC odbc )
{
	if( odbc )
	{
		__GetSQLTypes( odbc, odbc->collection );
		return odbc->collection->responce == WM_SQL_RESULT_SUCCESS?TRUE:0;
	}
	return 0;
}

//-----------------------------------------------------------------------
#define SET_RESULT_STRING( result, string ) {  \
		static CTEXTSTR default_msg;                 \
		if( !default_msg )                        \
			default_msg = StrDup(string);\
		if( default_msg[0] != string[0] )  \
		{                            \
			xlprintf(LOG_ALWAYS)( WIDE("Someone deallocated our message!") ); \
	      exit(0);                                                     \
    	}\
		(*(result)) = default_msg; \
	}

SQLPROXY_PROC( int, FetchSQLError )( PODBC odbc, CTEXTSTR *result )
{
	if( !result )
		return 0;
	if( !odbc )
	{
		odbc = g.odbc;
		if( !odbc )
		{
			SET_RESULT_STRING( result, WIDE("Application has failed to inialize it's ODBC connection") );
			return 1;
		}
	}
	if( odbc->collection )
	{
		// result in the text hanging out ont he collection thing.
		odbc->collection->result_text = VarTextPeek( odbc->collection->pvt_errorinfo );
		if( odbc->collection->result_text )
		{
			*result = GetText( odbc->collection->result_text );
		}
		else
		{
		  SET_RESULT_STRING( result, WIDE("No data on error...") );
		}
	}
	else
		SET_RESULT_STRING( result, WIDE("Collection disappeared...") );
	return 1;
}
//-----------------------------------------------------------------------

SQLPROXY_PROC( int, GetSQLError )( CTEXTSTR *result )
{
	if( !g.odbc )
		if( !OpenSQL( DBG_VOIDSRC ) )
		{
			if( result ) (*result) = NULL;
			return 0;
		}
	return FetchSQLError( g.odbc, result );
}

//-----------------------------------------------------------------------

void __GetSQLError( PODBC odbc, PCOLLECT collection )
{
	GenerateResponce( collection, WM_SQL_RESULT_DATA );
}

//-----------------------------------------------------------------------

CTEXTSTR Deblobify( TEXTCHAR *data, int len )
{
	static TEXTCHAR table[256];
	TEXTCHAR *result;
	TEXTCHAR *output = NewArray( TEXTCHAR, len + 1 );

	if( !table['1'] )
	{
		int n;
		for( n = 0; n <= 9; n++ )
			table['0'+n] = (char)n;
		for( n = 0; n < 6; n++ )
		{
			table['a'+n] = (char)(10 + n);
			table['A'+n] = (char)(10 + n);
		}
	}
	result = output;
	while( data[0] && data[1] && len )
	{
		output[0] = (char)(( table[(int)data[0]] << 4 ) + table[(int)data[1]]);
		data += 2;
		output++;
		len--;
	}
	// add a nul terminator...
	output[0] = 0;
	return result;
}

//-----------------------------------------------------------------------

int __GetSQLResult( PODBC odbc, PCOLLECT collection, int bMore )
{
	int retry = 0;
	int result_cmd = WM_SQL_RESULT_DATA;
	if( !odbc )
	{
		collection->lastop = LAST_RESULT;
		//collection->hLastWnd = hWnd;
		return 0;
	}
#if !defined( SQLPROXY_LIBRARY_SOURCE ) && !defined( SQLPROXY_SOURCE )
	// really this shouldn't be done in get result
	// if the connection failed between query and result there wouldn't be a result anyway?
	// so I guess this is just to protect getresult if the connection did fail.?
	if( !OpenSQLConnectionEx( odbc DBG_SRC ) )
	{
		GenerateResponce( collection, WM_SQL_RESULT_ERROR );
		if( odbc->flags.bThreadProtect )
		{
			odbc->nProtect--;
			LeaveCriticalSec( &odbc->cs );
		}
		return 0;
	}
#endif
	if( bMore )
		result_cmd = WM_SQL_RESULT_MORE;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) || defined( USE_ODBC )
	if(
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		( odbc->flags.bSQLite_native && collection->stmt )
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
		||
#endif
#ifdef USE_ODBC
		( collection->hstmt )
#endif
	  )
	{
#ifdef USE_ODBC
		RETCODE rc;
#endif
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		int rc3;
#endif
		//INDEX idx;
		int lines = 0;
#ifdef USE_ODBC
		TEXTCHAR byResultStatic[256];
		TEXTCHAR *byResult;
#endif
		PVARTEXT pvtData;
		TEXTCHAR *tmpResult = NULL;
#ifdef USE_ODBC
      // used as a buffer length to get odbc result
		uintptr_t byTmpResultLen = 0;
#endif
		// SQLPrepare
		// includes the table to list... therefore list the fields in the table.
		//lprintf( "am I logging: %d %d %d", (!odbc->flags.bNoLogging) , (!g.flags.bNoLog) , g.flags.bLogData )
		if( (!odbc->flags.bNoLogging) && (!g.flags.bNoLog) && g.flags.bLogData )
			pvtData = VarTextCreate();
		else
			pvtData = NULL;

		//lprintf( WIDE("Attempting to fetch data...") );
		//VarTextEmpty( collection->pvt_result );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		if( odbc->flags.bSQLite_native )
		{
		retry:
			rc3 = sqlite3_step( collection->stmt ); // need to do this early to get columns
			switch( rc3 )
			{
			case SQLITE_BUSY:
				lprintf( WIDE("Database busy, waiting...") );
				WakeableSleep( 25 );
				goto retry;
			case SQLITE_LOCKED:
				lprintf( WIDE("Database locked, waiting...") );
				WakeableSleep( 100 );
				goto retry;
				break;
			case SQLITE_ROW:
			case SQLITE_DONE:
				break;
			case SQLITE_CORRUPT:
				if( odbc->pCorruptionHandler ) {
					odbc->pCorruptionHandler( odbc->psvCorruptionHandler, odbc );
				}
				vtprintf( collection->pvt_errorinfo, WIDE("Database is corrupt (should retry): %s\n"), sqlite3_errmsg(odbc->db ) );
				result_cmd = WM_SQL_RESULT_ERROR;
				break;
			default:
				lprintf( WIDE("Step status %d:%s %08x"), rc3, sqlite3_errmsg(odbc->db ), sqlite3_extended_errcode(odbc->db) );
				result_cmd = WM_SQL_RESULT_ERROR;
				break;
			}
		}
#endif
		if( ( result_cmd != WM_SQL_RESULT_DATA ) && ( result_cmd != WM_SQL_RESULT_MORE ) ) {
			GenerateResponce( collection, result_cmd );
			if( odbc->flags.bThreadProtect )
			{
				odbc->nProtect--;
				LeaveCriticalSec( &odbc->cs );
			}
			return 0;
		}
		ReleaseCollectionResults( collection, FALSE );
		if( collection->flags.bBuildResultArray )
		{
			int len;

			// after initial, and clear, these sizes will be NULL
			// otherwise, it's for getting a result against the current query.
			// the next query will have cleared these to NULL;
			if( !collection->coltypes )
			{
					//lprintf( "making types for %d columns", collection->columns + 1 );
				collection->coltypes = NewArray( SQLSMALLINT, ( collection->columns + 1 ) );
			}
			if( !collection->colsizes )
			{
				collection->colsizes = NewArray( SQLULEN, ( collection->columns + 1 ) );
			}
			if( !collection->results )
			{
				len = ( sizeof( CTEXTSTR ) * (collection->columns + 1) );
				//lprintf( "Allocating for %d columns...", collection->columns );
				collection->results = NewArray( TEXTSTR, collection->columns + 1 );
				MemSet( collection->results, 0, len );
			}
			if( !collection->fields )
			{
				len = ( sizeof( CTEXTSTR ) * (collection->columns + 1) );
				collection->fields = NewArray( TEXTSTR, collection->columns + 1 );
				MemSet( collection->fields, 0, len );
				len = (sizeof( size_t ) * (collection->columns + 1));
				collection->result_len = NewArray( size_t, collection->columns + 1 );
				MemSet( collection->result_len, 0, len );
				len = (sizeof( int ) * (collection->columns + 1));
				collection->column_types = NewArray( int, collection->columns + 1 );
				MemSet( collection->column_types, 0, len );
				// okay and now - pull column info from magic place...
				{
#ifdef USE_ODBC
					TEXTCHAR colname[256];
					short namelen;
#endif
					// cpg29dec2006 c:\work\sack\src\sqllib\sqlstub.c(1222) : warning C4018: '<=' : signed/unsigned mismatch
					// cpg29dec2006 INDEX idx;
					int idx;
					for( idx = 1; idx <= collection->columns; idx++ )
					{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
						//const unsigned char *sqlite3_column_text(sqlite3_stmt*, int iCol);
						if( odbc->flags.bSQLite_native )
						{
							collection->fields[idx-1] =
								DupCStr( sqlite3_column_name(collection->stmt
																	 , idx - 1 ) );
							collection->column_types[idx-1] = sqlite3_column_type( collection->stmt, idx - 1 );
						}
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
						else
#endif
#ifdef USE_ODBC
						{
							rc = SQLDescribeCol( collection->hstmt
													 , idx
													 ,
#ifdef _UNICODE
													  (SQLWCHAR*)colname
													  , sizeof(colname)
#else
													  (SQLCHAR*)colname
													  , sizeof(colname)
#endif
													 , (SQLSMALLINT*)&namelen
													 , collection->coltypes + idx - 1 // data type short int
													 , collection->colsizes + idx - 1 // columnsize int
													 , NULL // decimal digits short int
													 , NULL // nullable ptr ?
													 );
							//lprintf( WIDE("column %s is type %d(%d)"), colname, coltypes[idx-1], colsizes[idx-1] );
							if( rc != SQL_SUCCESS_WITH_INFO &&
								rc != SQL_SUCCESS )
							{
								retry = DumpInfo( odbc, collection->pvt_errorinfo, SQL_HANDLE_STMT, &collection->hstmt, odbc->flags.bNoLogging );
								lprintf( WIDE("GetData failed...") );
								result_cmd = WM_SQL_RESULT_ERROR;
								break;
							}
							//lprintf( "col %s is %d %d", colname, collection->colsizes[idx-1], collection->coltypes[idx-1] );
							colname[namelen] = 0; // always nul terminate this.
							collection->fields[idx-1] = StrDup( colname );
						}
#endif
					} // for
					// and stuff a NULL at the end too..
					collection->fields[idx-1] = NULL;
				}
			}
		}
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		if( odbc->flags.bSQLite_native && ( rc3 == SQLITE_DONE ) )
		{
			if( pvtData )
			{
				lprintf( WIDE("no data") );
				VarTextDestroy( &pvtData );
			}

			result_cmd = WM_SQL_RESULT_NO_DATA;
			collection->flags.bTemporary = 1;
			collection->flags.bEndOfFile = 1;
			GenerateResponce( collection, result_cmd );
			//DestroyCollection( collection );
			if( odbc->flags.bThreadProtect )
			{
				odbc->nProtect--;
				LeaveCriticalSec( &odbc->cs );
			}
			return 0;
		}
#endif
		if(
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
			( odbc->flags.bSQLite_native && ( rc3 == SQLITE_ROW ) )
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
			||
#endif
#ifdef USE_ODBC
			(
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
			 (!odbc->flags.bSQLite_native )
			 &&
#endif
			 ( ( rc = SQLFetch( collection->hstmt ) ) == SQL_SUCCESS )
			  )
#endif
			)
		{
			int idx;
			int first = 1;
#ifdef USE_ODBC
			SQLLEN ResultLen;
#endif
			idx = 1;
			lines++;
			//lprintf( WIDE("Yes, so lets' get the data to result with..") );
			do
			{
				//SQLULEN colsize;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
				if( odbc->flags.bSQLite_native )
				{
					char *text = (char*)sqlite3_column_text( collection->stmt, idx - 1 );
					TEXTSTR real_text;
					int colsize;
					colsize = sqlite3_column_bytes( collection->stmt, idx - 1 );
					real_text = DupCStrLen( text, colsize );
					if( collection->flags.bBuildResultArray )
					{
						collection->result_len[idx - 1] = colsize;
						if( collection->results[idx-1] )
							Release( (char*)collection->results[idx-1] );
						switch( collection->column_types[idx-1] )
						{
						case SQLITE_BLOB:
							//lprintf( "Got a blob..." );
							if( pvtData )vtprintf( pvtData, WIDE( "%s<binary>" ), idx>1?WIDE( "," ):WIDE( "" ) );
							collection->results[idx-1] = NewArray( TEXTCHAR, collection->result_len[idx - 1] );
							MemCpy( collection->results[idx-1], text, collection->result_len[idx - 1] );
							break;
						default:
							if( pvtData )vtprintf( pvtData, WIDE( "%s%s" ), idx>1?WIDE( "," ):WIDE( "" ), real_text );
							collection->results[idx-1] = real_text;
							break;
						}
					}
					else
					{
						if( pvtData )vtprintf( pvtData, WIDE( "%s%s" ), idx>1?WIDE( "," ):WIDE( "" ), real_text?real_text:WIDE( "<NULL>" ) );
						//lprintf( WIDE( "... %p" ), text );
						vtprintf( collection->pvt_result, WIDE("%s%s"), first?WIDE( "" ):WIDE( "," ), real_text?real_text:WIDE( "" ) );
						first=0;
						Release( real_text );
					}
				}
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
				else
#endif
#ifdef USE_ODBC
				{
					short coltype;
					SQLULEN colsize;
					rc = SQLDescribeCol( collection->hstmt
											 , (SQLUSMALLINT)idx
											 , NULL, 0 // colname, bufsize
											 , NULL // result colname size
											 , &coltype // data type short int
											 , &colsize // columnsize int
											 , NULL // decimal digits short int
											 , NULL // nullable ptr ?
											 );
					colsize = (colsize * 2) + 1 + 1024 ;
					if( colsize >= sizeof( byResultStatic ) )
					{
						if( colsize > byTmpResultLen )
						{
							if( tmpResult )
								Release( tmpResult );
							tmpResult = NewArray( TEXTCHAR, colsize );
							byTmpResultLen = colsize;
						}

						byResult = tmpResult;
					}
					else
					{
						byResult = byResultStatic;
					}
					if( collection->coltypes && ( ( collection->coltypes[idx-1] == SQL_VARBINARY ) || ( collection->coltypes[idx-1] == SQL_LONGVARBINARY ) ) )
					{
						rc = SQLGetData( collection->hstmt
											, (short)(idx)
											, SQL_C_BINARY
											, byResult
											, colsize
											, &ResultLen );
						if( SUS_GT( ResultLen,SQLINTEGER,collection->colsizes[idx-1],SQLUINTEGER) )
						{
							lprintf( WIDE( "SQL Result returned more data than the column described! (returned %d expected %d)" ), (int)ResultLen, (int)(collection->colsizes[idx-1]) );
						}
					}
					else
					{
						rc = SQLGetData( collection->hstmt
											, (short)(idx)
#ifdef _UNICODE
											, SQL_C_WCHAR
#else
											, SQL_C_CHAR
#endif
											, byResult
											, colsize
											, &ResultLen );

						// hvaing this cast as a UINTEGER for colsize comparison
						// breaks -1 being less than colsize... so test negative special and
						// do the same thing as < colsize
						if( ( ResultLen & 0x8000000 )
									|| ( (SQLUINTEGER)ResultLen < colsize ) )
						{
							if( (int)ResultLen < 0 )
								byResult[0] = 0;
							else
								byResult[ResultLen] = 0;
						}
						else
						{
							lprintf( WIDE( "SQL overflow (no room for nul character) %d of %d" ), (int)ResultLen, (int)colsize );
						}
					}
					collection->result_len[idx - 1] = ResultLen;
					//lprintf( WIDE( "Column %s colsize %d coltype %d coltype %d idx %d" ), collection->fields[idx-1], colsize, coltype, collection->coltypes[idx-1], idx );
					if( collection->coltypes && coltype != collection->coltypes[idx-1] )
					{
						lprintf( WIDE( "Col type mismatch?" ) );
						DebugBreak();
					}
					if( rc == SQL_SUCCESS ||
						rc == SQL_SUCCESS_WITH_INFO )
					{
						if( ResultLen == SQL_NO_TOTAL ||  // -4
							ResultLen == SQL_NULL_DATA )  // -1
						{
							//lprintf( WIDE("result data failed...") );
						}

						if( ResultLen > 0 )
						{
							if( collection->flags.bBuildResultArray )
							{
								if( collection->coltypes[idx-1] == SQL_LONGVARBINARY )
								{
									// I won't modify this anyhow, and it results
									// to users as a CTEXSTR, preventing them from changing it also...
									//lprintf( "Got a blob..." );
									//lprintf( WIDE( "size is %d" ), collection->colsizes[idx-1] );
									if( pvtData )
									{
										SQLUINTEGER n;
										vtprintf( pvtData, WIDE( "%s<" ), idx>1?WIDE( "," ):WIDE( "" ) );
										for( n = 0; n < collection->colsizes[idx-1]; n++ )
											vtprintf( pvtData, WIDE( "%02x " ), byResult[n] );
										vtprintf( pvtData, WIDE( ">" ) );
									}
									collection->results[idx-1] = NewArray( TEXTCHAR, collection->colsizes[idx-1] );
									//lprintf( WIDE( "dest is %p and src is %p" ), collection->results[idx-1], byResult );
									MemCpy( collection->results[idx-1], byResult, collection->colsizes[idx-1] );
									//lprintf( WIDE( "Column %s colsize %d coltype %d coltype %d idx %d" ), collection->fields[idx-1], colsize, coltype, coltypes[idx-1], idx );
									//collection->results[idx-1] = (TEXTSTR)Deblobify( byResult, colsizes[idx-1] );
								}
								else
								{
									if( collection->results[idx-1] )
										Release( (char*)collection->results[idx-1] );
									if( pvtData )vtprintf( pvtData, WIDE( "%s%s" ), idx>1?WIDE( "," ):WIDE( "" ), byResult );
									collection->results[idx-1] = StrDup( byResult );
								}
							}
							else
							{
								//lprintf( WIDE("Got a result: \'%s\'"), byResult );

								/*
								* if this is auto processed for the application, there is no
								* result indicator indicating how long it is, therefore the application
								* must in turn call Deblobify or some other custom routine to handle
								* this SQL database's binary format...
								*/
								/*
								if( coltypes[idx-1] == SQL_LONGVARBINARY )
								{
								POINTER tmp;
								vtprintf( collection->pvt_result, WIDE("%s%s"), first?WIDE( "" ):WIDE( "," ), tmp = Deblobify( byResult, collection->colsizes[idx-1] ) );
								Release( tmp );
								}
								else
								*/
								vtprintf( collection->pvt_result, WIDE("%s%s"), first?WIDE( "" ):WIDE( "," ), byResult );
								if( pvtData )vtprintf( pvtData, WIDE( "%s%s" ), idx>1?WIDE( "," ):WIDE( "" ), byResult );
								first = 0;
							}
						}
						else
						{
							if( !collection->flags.bBuildResultArray )
							{
								//lprintf( WIDE("Didn't get a result... null field?") );
								vtprintf( collection->pvt_result, WIDE("%s"), first?WIDE( "" ):WIDE( "," ) );
								if( pvtData )vtprintf( pvtData, WIDE( "%s%s" ), idx>1?WIDE( "," ):WIDE( "" ), byResult );
								first=0;
							}
							else
							{
								if( pvtData )vtprintf( pvtData, WIDE( "%s<NULL>" ), idx>1?WIDE( "," ):WIDE( "" ) );
							}
							// otherwise the entry will be NULL
						}
					}
					else
					{
						retry = DumpInfo( odbc, collection->pvt_errorinfo, SQL_HANDLE_STMT, &collection->hstmt, odbc->flags.bNoLogging );
						lprintf( WIDE("GetData failed...") );
						result_cmd = WM_SQL_RESULT_ERROR;
					}
				}
#endif

				idx++;
			} while( idx <= collection->columns
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
#else
					  && rc == SQL_SUCCESS
#endif
					 );
			//lprintf( WIDE("done...") );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
#else
			rc = SQL_SUCCESS;
#endif
		}
		else
		{
			// turns out the results will be released at the next command/query that uses this
			// or connection closes and destroyes the collection.
			collection->flags.bTemporary = 1;
			collection->flags.bEndOfFile = 1;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
			sqlite3_finalize( collection->stmt );
			collection->stmt = NULL;
#endif

			//lprintf( WIDE("What about the remainging results?") );
			//ReleaseCollectionResults( collection );
		}
		if( pvtData )
		{
			PTEXT data = VarTextGet( pvtData );
			lprintf( "%s", GetText( data ) );
			LineRelease( data );
			//lprintf( WIDE( "%s" ), GetText( VarTextPeek( pvtData ) ) );
			VarTextDestroy( &pvtData );
		}

		/* these were temporary for collectiong other meta information */
		if( tmpResult )
		{
			Release( tmpResult );
			tmpResult = NULL;
		}
		if( !lines )
		{
			// might want a different result here...
			// is am empty row result.
			//lprintf( WIDE("Result with no lines...") );
			result_cmd = WM_SQL_RESULT_NO_DATA;
			GenerateResponce( collection, result_cmd );
			//DestroyCollection( collection );
			if( odbc->flags.bThreadProtect )
			{
				odbc->nProtect--;
				LeaveCriticalSec( &odbc->cs );
			}
			return 0;
		}
	}
	else
	{
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		if( odbc->flags.bSQLite_native ){
		}
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
		else
#endif
#ifdef USE_ODBC
			retry = DumpInfo( odbc, collection->pvt_errorinfo, SQL_HANDLE_STMT, &collection->hstmt, odbc->flags.bNoLogging );
#endif
		lprintf( WIDE("result error") );
		result_cmd = WM_SQL_RESULT_ERROR;
	}
#endif
	GenerateResponce( collection, result_cmd );
	if( odbc->flags.bThreadProtect )
	{
		odbc->nProtect--;
		LeaveCriticalSec( &odbc->cs );
	}
	return 0;
}

//-----------------------------------------------------------------------
int FetchSQLRecord( PODBC odbc, CTEXTSTR **result )
{
	if( result )
	{
		if( *result )
			*result = NULL;
		if( !odbc )
			odbc = g.odbc;
		if( odbc )
		{
			if( odbc->flags.bThreadProtect )
			{
				EnterCriticalSec( &odbc->cs );
				odbc->nProtect++;
			}
			while( odbc->collection && odbc->collection->flags.bTemporary )
			{
				DestroyCollector( odbc->collection );
			}
			if( !odbc->collection )
			{
				lprintf( WIDE("Lost ODBC result collection...") );
				return 0;
			}
			odbc->collection->flags.bBuildResultArray = 1;
 			__GetSQLResult( odbc, odbc->collection, FALSE );
			if( odbc->collection->responce == WM_SQL_RESULT_DATA )
			{
				*result = (CTEXTSTR*)odbc->collection->results;
				// claim that we grabbed that...
				//odbc->collection->results = NULL;
			}
			if( odbc->flags.bThreadProtect )
			{
				LeaveCriticalSec( &odbc->cs );
				odbc->nProtect--;
			}
			return odbc->collection->responce == WM_SQL_RESULT_DATA?TRUE:0;
		}
	}
	return FALSE;
}
//-----------------------------------------------------------------------
SQLPROXY_PROC( int, FetchSQLResult )( PODBC odbc, CTEXTSTR *result )
{
	if( result )
		(*result) = NULL;
	if( !odbc )
		odbc = g.odbc;
	if( odbc )
	{
		if( odbc->flags.bThreadProtect )
		{
			EnterCriticalSec( &odbc->cs );
			odbc->nProtect++;
		}
		odbc->collection->flags.bBuildResultArray = 0;
		__GetSQLResult( odbc, odbc->collection, FALSE );
		if( odbc->collection->responce == WM_SQL_RESULT_DATA )
		{
			odbc->collection->result_text = VarTextPeek( odbc->collection->pvt_result );
			if( odbc->collection->result_text )
			{
				(*result) = GetText( odbc->collection->result_text );
			}
			else
			{
				if( odbc->flags.bThreadProtect )
				{
					LeaveCriticalSec( &odbc->cs );
					odbc->nProtect--;
				}
				return 0;
			}
		}
		if( odbc->flags.bThreadProtect )
		{
			LeaveCriticalSec( &odbc->cs );
			odbc->nProtect--;
		}
		return odbc->collection->responce == WM_SQL_RESULT_DATA?TRUE:0;
	}
	return FALSE;
}

//-----------------------------------------------------------------------
int GetSQLRecord( CTEXTSTR **result )
{
	if( result )
	{
		if( !OpenSQL( DBG_VOIDSRC ) )
		{
			return FALSE;
		}
		return FetchSQLRecord( g.odbc, result );
	}
	return FALSE;
}
//-----------------------------------------------------------------------

int GetSQLResult( CTEXTSTR *result )
{
	if( result )
	{
		if( !OpenSQL( DBG_VOIDSRC ) )
		{
			return FALSE;
		}
		return FetchSQLResult( g.odbc, result );
	}
	return FALSE;
}

//-----------------------------------------------------------------------

#if defined USE_ODBC && 0
static void __DoODBCBinding( HSTMT hstmt, PDATALIST pdlItems ) {
	INDEX idx;
	struct json_value_container *val;
	DATA_FORALL( pdlItems, idx, struct json_value_container *, val ) {
		int useIndex = idx + 1;
		int rc;
		//if( val->name ) {
		//	useIndex = sqlite3_bind_parameter_index( db, val->name );
		//}
		switch( val->value_type ) {
		default:
			lprintf( "Failed to handline binding for type: %d", val->value_type );
			DebugBreak();
			break;
		case VALUE_NUMBER:
			if( val->float_result ) {
				rc = SQLBindParamter( hstmt
										  , useIndex  // parameter number
										  , SQL_PARAM_INPUT // inputoutputtype
										  , SQL_C_DOUBLE  // value type
										  , SQL_DOUBLE    // parameter type
										  , 100 // precision (colsize)
										  , 0 // decimal digits
										  , &val->result_d  // pointer value
										  , sizeof( val->result_d ) // bufferlength
										  , SQL_NULL_DATA
										  );
			} else {
				rc = SQLBindParamter( hstmt
										  , useIndex  // parameter number
										  , SQL_PARAM_INPUT // inputoutputtype
										  , SQL_C_UBIGINT  // value type
										  , SQL_BIGINT    // parameter type
										  , 100 // precision (colsize)
										  , 0 // decimal digits
										  , &val->result_n  // pointer value
										  , sizeof( val->result_n ) // bufferlength
										  , SQL_NULL_DATA
										  );
			}
			break;
		case VALUE_TYPED_ARRAY:
			rc = sqlite3_bind_blob( db, useIndex, val->string, val->stringLen, NULL );
			break;
		case VALUE_STRING:
			rc = sqlite3_bind_text( db, useIndex, val->string, val->stringLen, NULL );
				rc = SQLBindParamter( hstmt
										  , useIndex  // parameter number
										  , SQL_PARAM_INPUT // inputoutputtype
										  , SQL_C_CHAR  // value type
										  , SQL_CHAR    // parameter type
										  , 100 // precision (colsize)
										  , 0 // decimal digits
										  , val->string  // pointer value
										  , val->stringLen // bufferlength
										  , SQL_NULL_DATA
										  );
			break;
		}
		if( rc )
			lprintf( "Error binding:%d %d", useIndex, rc );
	}

}
#endif

#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
static void __DoSQLiteBinding( sqlite3_stmt *db, PDATALIST pdlItems ) {
	INDEX idx;
	struct json_value_container *val;
	DATA_FORALL( pdlItems, idx, struct json_value_container *, val ) {
		int useIndex = idx + 1;
		int rc;
		if( val->name ) {
			useIndex = sqlite3_bind_parameter_index( db, val->name );
		}
		switch( val->value_type ) {
		default:
			lprintf( "Failed to handline binding for type: %d", val->value_type );
			DebugBreak();
			break;
		case VALUE_NUMBER:
			if( val->float_result ) {
				rc = sqlite3_bind_double( db, useIndex, val->result_d );
			} else {
				rc = sqlite3_bind_int64( db, useIndex, val->result_n );
			}
			break;
		case VALUE_TYPED_ARRAY:
			rc = sqlite3_bind_blob( db, useIndex, val->string, val->stringLen, NULL );
			break;
		case VALUE_STRING:
			rc = sqlite3_bind_text( db, useIndex, val->string, val->stringLen, NULL );
			break;
		}
		if( rc )
			lprintf( "Error binding:%d %d", useIndex, rc );
	}
}
#endif

//-----------------------------------------------------------------------

int __DoSQLQueryExx( PODBC odbc, PCOLLECT collection, CTEXTSTR query, size_t queryLength, PDATALIST pdlParams DBG_PASS )
{
	size_t queryLen;
	PTEXT tmp = NULL;
	int retry = 0;
#ifdef USE_ODBC
	RETCODE rc;
#endif
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	int rc3;
#endif
	int in_error = 0;
#ifdef LOG_EVERYTHING
	_lprintf(DBG_RELAY)( WIDE( ".... begin query.... " ));
#endif
	if( !query )
	{
		xlprintf(LOG_ALWAYS)( WIDE("REally should pass a query if you expect a query (was passed NULL).") );
		return FALSE;
	}
	if( !odbc )
	{
		Log( WIDE("Delayed QUERY") );
		if( collection->lastop != LAST_QUERY )
		{
			collection->lastop = LAST_QUERY;
			vtprintf( collection->pvt_out, WIDE("%s"), query );
		}
		else
		{
			lprintf( WIDE("Should not be a new query...") );
		}
		//collection->hLastWnd = hWnd;
		return FALSE;
	}
	if( !odbc )
	{
		DebugBreak();
	}
	if( !IsSQLOpen( odbc ) )
	{
		return FALSE;
	}
	odbc->last_command_tick_ = timeGetTime();
	if( odbc->last_command_tick )
		odbc->last_command_tick = odbc->last_command_tick_;
	if( odbc->flags.bThreadProtect )
	{
		EnterCriticalSec( &odbc->cs );
		odbc->nProtect++;
	}

	//lprintf( WIDE("Query: %s"), GetText( cmd ) );
	ReleaseCollectionResults( collection, TRUE );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		if( collection->stmt )
		{
			sqlite3_finalize( collection->stmt );
			collection->stmt = NULL;
		}
	}
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
	else
#endif
#ifdef USE_ODBC
	{
		if( collection->hstmt )
		{
#ifdef LOG_EVERYTHING
			lprintf( WIDE( "Releasing old handle... %p" ), collection->hstmt );
#endif
			SQLFreeHandle( SQL_HANDLE_STMT, collection->hstmt );
			collection->hstmt = 0;
		}
	}
#endif

	// try and get query from collector if NULL
	if( query )
	{
		VarTextEmpty( collection->pvt_out );
		VarTextAddData( collection->pvt_out, query, queryLength );
		//vtprintf( collection->pvt_out, WIDE( "%s" ), query );
	}

	{
		tmp = VarTextPeek( collection->pvt_out );
		query = GetText( tmp );
		queryLen = GetTextSize( tmp );
	}
	if( EnsureLogOpen(odbc ) )
	{
		sack_fprintf( g.pSQLLog, WIDE("%s[%p]:%s\n"), odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" ), odbc, query );
		sack_fflush( g.pSQLLog );
	}
	if( !g.flags.bNoLog )
	{
		if( odbc->flags.bNoLogging )
			//odbc->hidden_messages++
			;
		else
			_lprintf(DBG_RELAY)( WIDE( "Do Command[%p:%s]: %s" ), odbc
					, odbc->info.pDSN?odbc->info.pDSN:WIDE( "NoDSN?" )
					, query );
	}

	//lprintf( DBG_FILELINEFMT WIDE( "Query: %s" ) DBG_RELAY, GetText( query ) );
#ifdef LOG_EVERYTHING
	lprintf( WIDE( "sql command on %p [%s]" ), collection, query );
#endif
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
	{
		const TEXTCHAR *tail;
		// can get back what was not used when parsing...
		retry:
#ifdef UNICODE
		rc3 = sqlite3_prepare16_v2( odbc->db, (void*)query, (int)(querylen) * sizeof( TEXTCHAR ), &collection->stmt, (const void**)&tail );
#else
		rc3 = sqlite3_prepare_v2( odbc->db, query, (int)(queryLen), &collection->stmt, &tail );
#endif
		if( rc3 )
		{
			const char *tmp;
			if( rc3 == SQLITE_BUSY )
			{
				lprintf( WIDE("wait for lock...") );
				DumpAllODBCInfo();
				WakeableSleep( 200 );
				goto retry;
			}
			//DebugBreak();
			tmp = sqlite3_errmsg(odbc->db);
         // this will have to have a Char based version
			if( strnicmp( tmp, "no such table", 13 ) == 0 )
				vtprintf( collection->pvt_errorinfo, WIDE( "(S0002)" ) );
			vtprintf( collection->pvt_errorinfo, WIDE( "Result of prepare failed? %s at-or near char %")_size_f WIDE("[%") _cstring_f WIDE("] in [%") _string_f WIDE("]" ), tmp, tail - query, tail, query );
			if( EnsureLogOpen(odbc ) )
			{
				sack_fprintf( g.pSQLLog, WIDE( "#SQLITE ERROR:%s\n" ), GetText( VarTextPeek( collection->pvt_errorinfo ) ) );
				sack_fflush( g.pSQLLog );
			}
			in_error = 1;
		} else {
         if( pdlParams )
				__DoSQLiteBinding( collection->stmt, pdlParams );
			if( odbc->flags.bAutoCheckpoint && !sqlite3_stmt_readonly( collection->stmt ) )				
				startAutoCheckpoint( odbc );
		}
		// here don't step, wait for the GetResult to step. (fetch row)
	}
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
	else
#endif
#ifdef USE_ODBC
	{
#ifdef LOG_EVERYTHING
		lprintf( WIDE( "getting a new handle....against %p on %p" ), odbc->hdbc, odbc );
#endif
		rc = SQLAllocHandle( SQL_HANDLE_STMT
								 , odbc->hdbc
								 , &collection->hstmt );
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
		{
			xlprintf(LOG_ALWAYS)( WIDE("Failed to open ODBC statement handle....") );
			if( odbc->flags.bThreadProtect )
			{
				odbc->nProtect--;
				LeaveCriticalSec( &odbc->cs );
			}
			return FALSE;
		}
#ifdef LOG_EVERYTHING
		lprintf( WIDE( "new handle... %p" ), collection->hstmt );
#endif
		if( StrCmp( query, WIDE("show tables") ) == 0 )
		{
			rc = SQLTables( collection->hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0 );
		}
		else
		{
#if 0
         if( pdlParams )
				__DoODBCBinding( collection->hstmt, pdlParams );
#endif
			rc = SQLExecDirect( collection->hstmt
									,
#ifdef _UNICODE
									 (SQLWCHAR*)query
#else
									 (SQLCHAR*)query
#endif
									, SQL_NTS );
		}
		if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO )
		{
#ifdef LOG_EVERYTHING
			lprintf( WIDE( "rc is %d [not success]" ),rc );
#endif
			in_error = 1;
		}
	}
#endif
	if( in_error )
	{
		//PTEXT tmp;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		if( odbc->flags.bSQLite_native )
		{
		}
#endif
#if defined( USE_ODBC ) && defined( USE_SQLITE )
		else
#endif
#ifdef USE_ODBC
		{
			retry = DumpInfo( odbc, collection->pvt_errorinfo, SQL_HANDLE_STMT, &collection->hstmt, odbc->flags.bNoLogging );
		}
#endif
		GenerateResponce( collection, WM_SQL_RESULT_ERROR );
		if( odbc->flags.bThreadProtect )
		{
			odbc->nProtect--;
			LeaveCriticalSec( &odbc->cs );
		}
		return retry;
	}
	/*
	*/
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
		collection->columns = (SQLSMALLINT)sqlite3_column_count(collection->stmt);
#endif
#if ( defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE ) ) && defined( USE_ODBC )
	  else
#endif
#ifdef USE_ODBC
	  {
		  if( SQLNumResultCols(collection->hstmt, &collection->columns ) != SQL_SUCCESS )
		  {
			  lprintf( WIDE("Failed to get result cols...") );
			  retry = DumpInfo( odbc, collection->pvt_errorinfo
									, SQL_HANDLE_STMT
									, &collection->hstmt
									, odbc->flags.bNoLogging);
			  GenerateResponce( collection, WM_SQL_RESULT_ERROR );
			  //DestroyCollector( collection );
			  if( odbc->flags.bThreadProtect )
			  {
				  odbc->nProtect--;
				  LeaveCriticalSec( &odbc->cs );
			  }
			  return retry ;
		}
	}
#endif

	//lprintf( WIDE("Get result ...") );
	retry = __GetSQLResult( odbc, collection, FALSE );
	if( odbc->flags.bThreadProtect )
	{
		odbc->nProtect--;
		LeaveCriticalSec( &odbc->cs );
	}
	return retry;
}

//------------------------------------------------------------------

int __DoSQLQueryEx( PODBC odbc, PCOLLECT collection, CTEXTSTR query DBG_PASS ) {
	return __DoSQLQueryExx( odbc, collection, query, strlen( query ), NULL DBG_RELAY );
}

//------------------------------------------------------------------

void PopODBCExx( PODBC odbc, LOGICAL bAllowNonPush DBG_PASS )
{
	//lprintf( WIDE( "pop sql %p" ), odbc );
	if( !odbc )
		odbc = g.odbc;
	if( odbc )
	{
		// pop odbc should pop util a push?
		while( odbc->collection && odbc->collection->flags.bTemporary
				// if it is end of file, it is probable, that it was also the
				// result of a push...
				&& !odbc->collection->flags.bPushed
			  )
			DestroyCollector( odbc->collection );
		//lprintf( WIDE( "Pop ODBC %p %d %p %d" ), odbc, bAllowNonPush, odbc->collection, odbc->collection?odbc->collection->flags.bPushed:-1 );
		if( odbc->collection && !odbc->collection->flags.bPushed && !bAllowNonPush )
		{
			//DebugBreak();
			_lprintf(DBG_RELAY)( WIDE( "Warning! Popping a state which was not pushed! (should be a breakpoint here)" ) );
		}
		DestroyCollector( odbc->collection );
	}
}


//------------------------------------------------------------------

void SQLEndQuery( PODBC odbc )
{
	if( !odbc )
		odbc = g.odbc;
	if( odbc )
	{

		// pop odbc should pop util a push?
		while( odbc->collection && odbc->collection->flags.bTemporary
				// if it is end of file, it is probable, that it was also the
				// result of a push...
				&& !odbc->collection->flags.bPushed
			  )
		{
			if( odbc->collection->flags.bEndOfFile )
			{
#ifdef LOG_COLLECTOR_STATES
				lprintf( WIDE( "Okay, top temporary found, but it was also a query at end of file... set end of file OK. return." ) );
#endif
				//return;
			}
			DestroyCollector( odbc->collection );
		}
		if( odbc->collection )//&& odbc->collection->flags.bPushed )
		{
			//lprintf( WIDE( "Should have ended with a pop not a EndQuery DebugBreak()" ) );
			//DebugBreak();
#ifdef LOG_COLLECTOR_STATES
			lprintf( WIDE( "End Query forces temporary and end of recordset. (more like a seek(end)" ) );
#endif
			odbc->collection->flags.bTemporary = 1;
			odbc->collection->flags.bEndOfFile = 1;
		}
	}
//	PopODBCExx( odbc, TRUE );
}

//-----------------------------------------------------------------------

int SQLRecordQuery_v4( PODBC odbc
                     , CTEXTSTR query
                     , size_t queryLen
                     , int *nResults
                     , CTEXTSTR **result
                     , size_t **resultLengths
                     , CTEXTSTR **fields 
                     , PDATALIST pdlParams
                     DBG_PASS )
{
	PODBC use_odbc;
	int once = 0;
	// clean up what we think of as our result set data (reset to nothing)
	if( result )
		(*result) = NULL;
	if( nResults )
		*nResults = 0;
	do
	{
		if( !IsSQLOpenEx( odbc DBG_RELAY ) )
			return FALSE;
		if( !( use_odbc = odbc ) )
		{
			// setup error as invalid databse handle... well.. try the default one also
			// but mostly fail.
			use_odbc = g.odbc;
		}
		// if not a [sS]elect then begin a transaction.... some code uses query record for everything.
		if( !once && query[0] != 's' && query[0] != 'S' ) {
			once = 1;
			BeginTransactEx( use_odbc, 0 );
		}
		// collection is very important to have - even if we will have to be opened,
		// we ill need one, so make at least one.
		if( !use_odbc->collection || !use_odbc->collection->flags.bTemporary )
		{
			if( use_odbc->collection && use_odbc->collection->flags.bTemporary )
			{
#ifdef LOG_COLLECTOR_STATES
				lprintf( WIDE( "using existing collector..." ) );
#endif
				use_odbc->collection->flags.bTemporary = 0;
			}
			else
			{
#ifdef LOG_COLLECTOR_STATES
				lprintf( WIDE( "creating collector..." ) );
#endif
				use_odbc->collection = CreateCollector( 0, use_odbc, FALSE );
			}
		}
		// if it was temporary, it shouldn't be anymore
		use_odbc->collection->flags.bTemporary = 0;
		// ask the collector to build the type of result set we want...
		use_odbc->collection->flags.bBuildResultArray = 1;
		// this will do an open, and delay queue processing and all sorts
		// of good fun stuff...
	}
	while( __DoSQLQueryExx( use_odbc, use_odbc->collection, query, queryLen, pdlParams DBG_RELAY) );

	if( use_odbc->collection->responce == WM_SQL_RESULT_DATA )
	{
		//lprintf( WIDE("Result with data...") );
		if( nResults )
			(*nResults) = use_odbc->collection->columns;
		if( result )
			(*result) = (CTEXTSTR*)use_odbc->collection->results;
		if( resultLengths )
			(*resultLengths) = use_odbc->collection->result_len;
		// claim that we grabbed that...
		if( fields )
			(*fields) = (CTEXTSTR*)use_odbc->collection->fields;
		//use_odbc->collection->results = NULL;
		return TRUE;
	}
	else if( use_odbc->collection->responce == WM_SQL_RESULT_NO_DATA )
	{
		if( nResults )
			(*nResults) = use_odbc->collection->columns;
		if( resultLengths )
			(*resultLengths) = NULL;
		if( fields )
			(*fields) = (CTEXTSTR*)use_odbc->collection->fields;
		return TRUE;
	}
	return FALSE;
}

//--------------------------------------------------------------------

int SQLRecordQueryEx( PODBC odbc
	, CTEXTSTR query
	, int *nResults
	, CTEXTSTR **result, CTEXTSTR **fields DBG_PASS )
{
	return SQLRecordQuery_v4( odbc, query, strlen( query ), nResults, result, NULL, fields, NULL DBG_RELAY );
}

//--------------------------------------------------------------------

int SQLQueryEx( PODBC odbc, CTEXTSTR query, CTEXTSTR *result DBG_PASS )
{
	PODBC use_odbc;
	LOGICAL once = 0;
	// clean up our result data....
	if( *result )
		(*result) = NULL;
	// if not a [sS]elect then begin a transaction.... some code might use query for everything.
	do
	{
		if( !IsSQLOpen( odbc ) )
			return FALSE;

		if( !( use_odbc = odbc ) )
		{
			// setup error as invalid databse handle... well.. try the default one also
			// but mostly fail.
			use_odbc = g.odbc;
		}

		if( !once && query[0] != 's' && query[0] != 'S' ) {
			BeginTransactEx( use_odbc, 0 );
			once = 1;
		}
		// this would be hard to come by...
		// there's a collector stil around from the open command.
		if( !use_odbc->collection || !use_odbc->collection->flags.bTemporary )
		{
			if( use_odbc->collection && use_odbc->collection->flags.bTemporary )
			{
#ifdef LOG_COLLECTOR_STATES
				lprintf( WIDE( "using existing collector..." ) );
#endif
				use_odbc->collection->flags.bTemporary = 0;
			}
			else
			{
#ifdef LOG_COLLECTOR_STATES
				lprintf( WIDE( "creating collector...: %s" ), query );
#endif
				use_odbc->collection = CreateCollector( 0, use_odbc, FALSE );
			}
		}
		use_odbc->collection->flags.bTemporary = 0;

		// ask the collector GetSQLResult to build our sort of data...
		use_odbc->collection->flags.bBuildResultArray = 0;

		// go ahead, open connection, issue command, get some data
		// or have some sort of failure along that path.
	}
	while( __DoSQLQueryEx( use_odbc, use_odbc->collection, query DBG_RELAY) );
	if( use_odbc->collection->responce == WM_SQL_RESULT_DATA )
	{
		use_odbc->collection->result_text= VarTextPeek( use_odbc->collection->pvt_result );
		if( use_odbc->collection->result_text )
		{
			//lprintf( WIDE("Result with data...") );
			(*result) = GetText( use_odbc->collection->result_text );
		}
		else
		{
			// ahh here's the key - need to result in an empty string...
			SET_RESULT_STRING( result, WIDE("") );
		}
		//return use_odbc->collection->responce == WM_SQL_RESULT_DATA?TRUE:0;
		return TRUE;
	}
	else if( use_odbc->collection->responce == WM_SQL_RESULT_NO_DATA )
	{
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------

int DoSQLQueryEx( CTEXTSTR query, CTEXTSTR *result DBG_PASS )
{
	(*result) = NULL;
	if( result )
	{
		if( !OpenSQL( DBG_VOIDSRC ) )
		{
			return FALSE;
		}
		return SQLQueryEx( NULL, query, result DBG_RELAY );
	}
	return FALSE;
}

//-----------------------------------------------------------------------
int IsSQLOpenEx( PODBC odbc DBG_PASS )
{
	if( !odbc )
	{
		//lprintf( "open default..." );
		OpenSQL( DBG_VOIDRELAY );
		odbc = g.odbc;
	}
	else
		OpenSQLConnectionEx( odbc DBG_RELAY );
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc && odbc->flags.bSQLite_native )
		if( odbc && odbc->db )
			return TRUE;
#endif
#ifdef USE_ODBC
	if( odbc && odbc->hdbc && odbc->flags.bConnected )
	{
		return TRUE;
	}
#endif
	// attempt open?
	// odbc library tends to be sluggish when the database fails connection...
	// I dunno - this is intended to be used in an Idle() loop which will get the timer to fire eventually.
	return FALSE;
}

#undef IsSQLOpen
int IsSQLOpen( PODBC odbc )
{
	return IsSQLOpenEx( odbc DBG_SRC );
}

//-----------------------------------------------------------------------
int IsSQLReady( void )
{
	return IsSQLOpen( g.odbc );
}

//-----------------------------------------------------------------------

int PushSQLQueryExEx( PODBC odbc DBG_PASS )
//#define PushSQLQeuryEx(odbc) PushSQLQueryExEx(odbc DBG_SRC )
{
	//_lprintf(DBG_RELAY)( "push sql %p", odbc );
	if( !odbc )
		odbc = g.odbc;
	if( odbc && odbc->collection )
	{
#ifdef LOG_COLLECTOR_STATES
		lprintf( WIDE( "creating collector..." ) );
#endif
		CreateCollectorEx( 0, odbc, FALSE DBG_RELAY );
		odbc->collection->flags.bPushed = 1;
		odbc->collection->flags.bTemporary = 1;
#ifdef LOG_COLLECTOR_STATES
		_lprintf(DBG_RELAY)( WIDE("pushing the query onto stack... creating new state.") );
#endif
		//lprintf( WIDE("Adding %p to %p at %p"), collection, odbc, &odbc->collection );
		//LinkThing( odbc->collection, collection );
		return 1;
	}
	else
	{
		if( odbc )
			odbc->flags.bPushed = 1;
	}
	return 0;
}

//-----------------------------------------------------------------------

int PushSQLQuery( void )
{
	return PushSQLQueryEx( g.odbc );
}

#undef PushSQLQueryEx
int PushSQLQueryEx( PODBC odbc )
#define PushSQLQueryEx(odbc) PushSQLQueryExEx(odbc DBG_SRC )
{
	return PushSQLQueryExEx( odbc DBG_SRC );
}


//-----------------------------------------------------------------------
#ifdef SQLPROXY_LIBRARY_SOURCE
// insert commands of these varieties are not used by
// the proxy, so we reall need not host them...

// parameters to this are pairs of "name", bQuote, WIDE("value")
// the last pair's name is NULL, and value does not matter.
// insert values into said table.

static struct {
	struct {
		BIT_FIELD  batch  : 1;
	} flags;
	PVARTEXT pvt_insert;
	PVARTEXT pvt_columns;
	PVARTEXT pvt_values;
	PLIST values;
} sql_insert;

 int  SQLInsertFlush ( PODBC odbc )
{
	int status;
	PVARTEXT pvt = VarTextCreate();
	PTEXT tmp, tmp2;
	if( !odbc )
	{
		if( !OpenSQL( DBG_VOIDSRC ) )
			return FALSE;
		odbc = g.odbc;
	}
	tmp = VarTextGet( sql_insert.pvt_insert );
	tmp2 = VarTextGet( sql_insert.pvt_columns );
	vtprintf( pvt, WIDE( "%s (%s) values " ), GetText( tmp ), GetText(  tmp2 ) );
	{
		int first = 1;
		PTEXT value;
		INDEX idx;
		LIST_FORALL( sql_insert.values, idx, PTEXT, value )
		{
			vtprintf( pvt, WIDE( "%s(%s)" ), first?WIDE( "" ):WIDE( "," ), GetText( value ) );
			first = 0;
			LineRelease( value );
			SetLink( &sql_insert.values, idx, NULL );
		}
	}
	LineRelease( tmp );
	LineRelease( tmp2 );
	tmp = VarTextGet( pvt );
	status = SQLCommand( odbc, GetText( tmp ) );
	LineRelease( tmp );
	VarTextDestroy( &pvt );
	sql_insert.flags.batch = 0;
	return status;
}

 int  SQLInsertBegin ( PODBC odbc )
{
	if( sql_insert.flags.batch )
		SQLInsertFlush( odbc );
	sql_insert.flags.batch = 1;
	return 1;
}



 int  vSQLInsert ( PODBC odbc, CTEXTSTR table, va_list args )
{
	// need to do this so we know if the default odbc is perhaps
	// a MSAccess database...
	if( !odbc )
	{
		if( !OpenSQL( DBG_VOIDSRC ) )
			return FALSE;
		odbc = g.odbc;
	}
	{
		int quote;
		int first;
		char *varname;
		char *varval;
		int making_columns;
		const TEXTCHAR *open_quote_string, *close_quote_string;
		first = 1;
		// once I thought values for access were [ ] enclosed...
		// so now this is a silly variable...
		open_quote_string = WIDE( "\'" );
		close_quote_string = WIDE( "\'" );

		//Log2( WIDE("Command = %p (%d)"), command, 12 + strlen( table ) + nVarLen + 7 + nValLen );
		if( !sql_insert.pvt_insert )
		{
			sql_insert.pvt_insert = VarTextCreate();
			sql_insert.pvt_columns = VarTextCreate();
			sql_insert.pvt_values = VarTextCreate();
		}

		if( !sql_insert.flags.batch || !VarTextPeek( sql_insert.pvt_insert ) )
		{
			if( odbc && odbc->flags.bAccess )
				vtprintf( sql_insert.pvt_insert, WIDE("Insert into [%s]"), table );
			else
				vtprintf( sql_insert.pvt_insert, WIDE("Insert into `%s`"), table );
		}
		first = 1;
		making_columns = 0;
		for( varname = va_arg( args, char * );
			varname;
			varname = va_arg( args, char * ) )
		{
			if( making_columns || !sql_insert.flags.batch || !VarTextPeek( sql_insert.pvt_columns ) )
			{
				making_columns = 1;
				vtprintf( sql_insert.pvt_columns, WIDE( "%s%s" ), first?WIDE( "" ):WIDE( "," ), varname );
			}

			quote = va_arg( args, int );
			varval = va_arg( args, char *);
			if( quote == 2)
			{
				vtprintf( sql_insert.pvt_values, WIDE("%s%d")
					, first?WIDE( "" ):WIDE( "," )
					, (int)(uintptr_t)varval // this generates an error - typecast to different size.  This is probably okay... but that means we needed to have it push a 8 byte value here ...
					);
			}
			else
				vtprintf( sql_insert.pvt_values, WIDE("%s%s%s%s")
				, first?WIDE( "" ):WIDE( "," )
				, quote?open_quote_string:WIDE( "" )
				, varval
				, quote?close_quote_string:WIDE( "" )
				);
			first = 0;
		}
		if( !sql_insert.flags.batch )
		{
			AddLink( &sql_insert.values, VarTextGet( sql_insert.pvt_values ) );
			return SQLInsertFlush( odbc );
		}
		else
			AddLink( &sql_insert.values, VarTextGet( sql_insert.pvt_values ) );
	}
	return FALSE;
}


//------------------------------------------------------------------

 int  DoSQLInsert ( CTEXTSTR table, ... )
{
	va_list args;
	if( !OpenSQL( DBG_VOIDSRC ) )
	{
		return FALSE;
	}
	va_start( args, table );
	return vSQLInsert( g.odbc, table, args );
}

//------------------------------------------------------------------
// parameters to this are pairs of "name", bQuote, WIDE("value")
// the last pair's name is NULL, and value does not matter.
// insert values into said table.
 int  SQLInsert ( PODBC odbc, CTEXTSTR table, ... )
{
	va_list args;
	va_start( args, table );
	return vSQLInsert( odbc, table, args );
}

#endif

//-----------------------------------------------------------------------

#define natoi(n, p,len) for( n = 0, cnt = 0; p[cnt] && cnt < len; cnt++ ) (n) = ((n)*10) + (p)[cnt] - '0'; if( p[0] ) p += len;

void ConvertSQLDateEx( CTEXTSTR date
                     , int *year, int *month, int *day
                     , int *hour, int *minute, int *second
                     , int *msec, int32_t *nsec
                     , int *zone_ofs_hr, int *zone_ofs_mn
                     )
{
	CTEXTSTR p;
	int n, cnt;
	p = date;
	if( msec )
		*msec = 0;
	if( nsec )
		*nsec = 0;
	natoi( n, p, 4 ); if( year ) *year = n;
	if( p[0]=='-' || p[0] == '/' ) p++;
	natoi( n, p, 2 ); if( month ) *month = n;
	if( p[0]=='-' || p[0] == '/' ) p++;
	natoi( n, p, 2 ); if( day ) *day = n;
	if( p[0] )
	{
		if( p[0]== 'T' || p[0] == ' ' || p[0] == '-'  ) p++;
		natoi( n, p, 2 ); if( hour ) *hour = n;
		if( p[0]==':' ) p++;
		natoi( n, p, 2 ); if( minute ) *minute = n;
		if( p[0]==':' ) p++;
		natoi( n, p, 2 ); if( second ) *second = n;
		if( p[0]=='.' ) {
			p++;
			natoi( n, p, 3 ); if( msec ) *msec = n;
		}
	}
	else
	{
		if( hour ) *hour = 0;
		if( minute ) *minute = 0;
		if( second ) *second = 0;
	}
	if( p[0] )
	{
		int add_zone = 1;
		if( p[0] == ' ' )
			p++;
		if( p[0] == 'Z' )
		{
			p++;
			if( zone_ofs_hr ) (*zone_ofs_hr) = 0;
			if( zone_ofs_mn ) (*zone_ofs_mn) = 0;
		}
		else if( p[0] == '+' )
		{
			p++;
			add_zone = 1;
		}
		else if( p[0] == '-' )
		{
			p++;
			add_zone = 0;
		}
		natoi( n, p, 2 ); if( zone_ofs_hr ) (*zone_ofs_hr) = add_zone?n:-n;
		if( p[0] == ':' ) p++;
		natoi( n, p, 2 ); if( zone_ofs_mn ) (*zone_ofs_mn) = n;
	}
	else
	{
		if( zone_ofs_hr ) (*zone_ofs_hr) = 0;
		if( zone_ofs_mn ) (*zone_ofs_mn) = 0;
	}
}

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------

//#ifdef SQL_PROXY_SERVER
//-----------------------------------------------------------------------

void CPROC Timer( uintptr_t psv )
{
	// this attempts to re-open backup and/or primary
	// and within this is dispatched the backup/restore/init
	// tasks.
	//Log( WIDE("Tick...") );
	OpenSQL( DBG_VOIDSRC );
	if( g.odbc )
	{
		//VarTextEmpty( g.TimerCollect.pvt_out );
		g.flags.bNoLog = 1;
		// this is a command... this is only a command...
		__DoSQLQuery( g.odbc, &g.TimerCollect, WIDE("select 0") );

		if( g.TimerCollect.responce != WM_SQL_RESULT_DATA )
		{
			FailConnection( g.odbc );
			Log( WIDE("Connection FAILED!") );
		}
		else
		{
			// setup for next tick
		}
		g.flags.bNoLog = 0;
	}
}

//-----------------------------------------------------------------------

static void LoadTasks( void )
{
	int n;
	for( n = 1; ;n++ )
	{
		TEXTCHAR taskid[32];
		TEXTCHAR updatetask[256];

		tnprintf( taskid, sizeof( taskid ), WIDE("Task%d"), n );
		updatetask[0] = 0;
		//OptGetPrivateProfileString( WIDE("Recovery Update"), taskid, WIDE(""), updatetask, sizeof( updatetask ), SQL_INI );
		if( updatetask[0] )
		{
			PUPDATE_TASK task = (PUPDATE_TASK)Allocate( sizeof( UPDATE_TASK ) );
			lprintf( WIDE("Task: \'%s\'"), updatetask );
			StrCpyEx( task->name, updatetask, sizeof( task->name ) );
			task->PrimaryRecovered = (void(CPROC *)(PODBC,PODBC))LoadFunction( task->name, WIDE("_PrimaryRecovered") );
			// it better never be  a post _ which implies register convention
			//if( !task->PrimaryRecovered )
			// task->PrimaryRecovered = (void(CPROC *)(PODBC,PODBC))LoadFunction( task->name, WIDE("PrimaryRecovered_") );
			if( !task->PrimaryRecovered )
				task->PrimaryRecovered = (void(CPROC *)(PODBC,PODBC))LoadFunction( task->name, WIDE("PrimaryRecovered") );
			if( !task->PrimaryRecovered )
			{
				lprintf( WIDE("Failure to get task from plugin: %s"), updatetask );
				//MessageBox( NULL, WIDE("Failure to load plugin!"), WIDE("Update Task Error"), MB_OK );
				//Release( task );
			}
			task->CheckTables = (void(CPROC *)(PODBC))LoadFunction( task->name, WIDE("_CheckTables") );
			// it better never be  a post _ which implies register convention
			//if( !task->CheckTables )
			// task->CheckTables = (void(CPROC *)(PODBC))LoadFunction( task->name, WIDE("CheckTables_") );
			if( !task->CheckTables )
				task->CheckTables = (void(CPROC *)(PODBC))LoadFunction( task->name, WIDE("CheckTables") );
			if( !task->CheckTables )
			{
				lprintf( WIDE("Failure to get task from plugin: %s"), updatetask );
				//MessageBox( NULL, WIDE("Failure to load plugin!"), WIDE("Update Task Error"), MB_OK );
				//Release( task );
			}

			LinkThing( g.UpdateTasks, task );
		}
		else
		{
			lprintf( WIDE("Task: %s"), updatetask );
			break;
		}
	}
}

//--------------------------------------------------------------------------

int CPROC SQLServiceHandler( PSERVICE_ROUTE SourceRouteID
									, uint32_t MsgID
									, uint32_t *params, size_t param_length
									, uint32_t *result, size_t *result_length )
{
	//uint32_t result_buffer_length = (*result_length);
	PCOLLECT pCollector = FindCollection( NULL, SourceRouteID );
	// extended messages may provide a way to specify alternate ODBC sources of collectors
	//pCollector->result_buffer = result;
	//pCollector->result_len = result_length;

	if( MsgID != WM_SQL_GET_ERROR
		&& MsgID!= MSG_ServiceLoad
		&& MsgID!= MSG_ServiceUnload )
	{
		if( !IsSQLReady() )
		{
			// result timeout to client. forget this message.
			//(*result_length) = INVALID_INDEX; // by default
			lprintf( WIDE("SQL is not ready...") );
			if( pCollector && pCollector->lastop )
			{
				PTEXT prior;
				// return unexpected ERROR results!
				lprintf( WIDE("SQL command pending already! FAILURE!") );
				prior = VarTextPeek( pCollector->pvt_out );
				lprintf( WIDE("prior command: %s"), GetText( prior ) );
				return FALSE;
			}
			//return TRUE;
		}
	}
	// respond to command after processing
	// success/failure information only applies.
	// no data ever results from this?
	if( result_length )
		(*result_length) = (uint32_t)INVALID_INDEX; // by default
	{
		// do stuff here
		switch( MsgID )
		{
		case MSG_ServiceLoad:
			lprintf( WIDE("Service load request... respond with correct startup!") );
			result[0] = WM_SQL_NUM_MESSAGES;
			result[1] = WM_SQL_NUM_MESSAGES;
			(*result_length) = (uint32_t)((result+2) - result);
			return TRUE;
		case MSG_ServiceUnload:
			//do
			{
				PCOLLECT pCollect = g.collections;
										// collectors are non-odbc connection specific.
										// they are more like a queue of pending requests against
										// either their own, specified ODBC or the primary/backup path.
				for( pCollect = g.collections;
					  pCollect;
					  pCollect = NextThing( pCollect) )
				{
					if( pCollect->SourceID == SourceRouteID )
					{
						lprintf( WIDE("Destrying collector for client which is gone.") );
						DestroyCollector( pCollector );
					}
				}

					// also destroy all odbc connectors created
						// by this colient!
			} //while( ( pCollector = FindCollection( NULL, SourceRouteID ) ) );
			return TRUE;
		case WM_SQL_COMMAND:
			lprintf( WIDE("Do SQL Command...") );
			// application will get a timeout on this message.
			//
			// who I am and who they are is implied with this message stream...
			__DoSQLCommand( g.odbc, Collect( pCollector, params, param_length  )/*, g.message_id */ );
			return TRUE;
		case WM_SQL_QUERY:
			lprintf( WIDE("Do SQL Query...") );
			// query will be already on the collector...
			pCollector->flags.bBuildResultArray = 0;
			__DoSQLQuery( g.odbc, Collect( pCollector, params, param_length ), NULL );
			return TRUE;
		case WM_SQL_QUERY_RECORD:
			lprintf( WIDE("Do SQL Query...") );
			// query will be already on the collector...
			pCollector->flags.bBuildResultArray = 1;
			__DoSQLQuery( g.odbc, Collect( pCollector, params, param_length ), NULL );
			return TRUE;
		case WM_SQL_GET_ERROR:
			__GetSQLError( g.odbc, pCollector );
			return TRUE;
		case WM_SQL_MORE:
			lprintf( WIDE("Do SQL More...") );
			__GetSQLResult( g.odbc, pCollector, TRUE );
			return TRUE;
		case WM_SQL_RESULT_SUCCESS:
			lprintf( WIDE("Unexpected result success to SQL Proxy server!") );
			return TRUE;
		case WM_SQL_RESULT_ERROR:
			lprintf( WIDE("Unexpected result error to SQL Proxy server!") );
			return TRUE;
		case WM_SQL_RESULT_DATA:
			lprintf( WIDE("Unexpected result data to SQL Proxy server!") );
			return TRUE;
		case WM_SQL_DATA_START:
			// should validate that we're not already
			// collecting...
			{
				lprintf( WIDE("Start collect data") );
				if( VarTextLength( pCollector->pvt_out ) )
				{
					lprintf( WIDE("Was already having data collected at start... dropping it.") );
					LineRelease( VarTextGet( pCollector->pvt_out ) );
				}
				Collect( pCollector, params, param_length );
			}
			return TRUE;
		case WM_SQL_DATA_MORE:
			lprintf( WIDE("Add more data...") );
			Collect( pCollector, params, param_length );
			return TRUE;
		}
	}
	return FALSE; // respond failure, if there is a responce...
}


//--------------------------------------------------------------------------
#if 0
void SQLBeginService( void )
//int main( void )
{
	SqlStubInitLibrary();
	// provide task interface
#ifndef __NO_MSGSVR__
	RegisterServiceHandler( WIDE("SQL"), SQLServiceHandler );
#endif
	// load local schedulable tasks...
	LoadTasks();
	// should probably add some options about what exactly
	// we would wish to log... but for now, and for testing
	// purposes, we log both query and command strings.
#ifdef __HAVE_ICON___
	RegisterIcon( PROXY_DOWN );
#endif
	while( 1 )
	{
		// this COULD be a timerproc...
		// but the main app isn't doing anything so
		// I guess it can do the SQL-alive probing.
		Timer(0);
#ifdef _DEBUG
		// one second wake mode...
		WakeableSleep( 1000 );
#else
		WakeableSleep( 30000 );
		// try every 30 seconds in the real world...
#endif
	}
#ifdef __HAVE_ICON___
	UnregisterIcon();
#endif
	//return 0;
}
#endif
//-------------------------------------------------------------------------

void SQLSetUserData( PODBC odbc, uintptr_t psvUser )
{
	odbc->psvUser = psvUser;
}
//-------------------------------------------------------------------------

uintptr_t SQLGetUserData( PODBC odbc )
{
	return odbc->psvUser;
}

struct day_type_offset
{
	CTEXTSTR type;
	CTEXTSTR result;
};

CTEXTSTR GetSQLOffsetDate( PODBC odbc, CTEXTSTR BeginOfDayType, int default_begin )
{
	TEXTCHAR result[80];
	TEXTCHAR offset[25];
	TEXTCHAR default_val[12];
	int hours, minutes;
	INDEX idx;
	struct day_type_offset *dto;
	LIST_FORALL( g.date_offsets, idx, struct day_type_offset *, dto )
	{
		if( StrCaseCmp( dto->type, BeginOfDayType ) == 0 )
			return dto->result;
	}
	{
		dto = New( struct day_type_offset );
		dto->type = StrDup( BeginOfDayType );
		if( default_begin > 100 )
			tnprintf( default_val, sizeof( default_val ), WIDE( "%d:%02d" ), default_begin / 100, default_begin % 100 );
		else
			tnprintf( default_val, sizeof( default_val ), WIDE( "%d" ), default_begin );
#ifndef __NO_OPTIONS__
		SACK_GetProfileString( WIDE( "SACK/SQL/Day Offset" ), BeginOfDayType, default_val, offset, sizeof( offset ) );
#else
		StrCpyEx( offset, default_val, sizeof( offset ) );
#endif
		if( StrChr( offset, ':' ) )
		{
			tscanf( offset, WIDE( "%d:%d" ), &hours, &minutes );
		}
		else
		{
			minutes = 0;
			tscanf( offset, WIDE( "%d" ), &hours );
		}

		tnprintf( result, sizeof( result ), WIDE( "cast(date_add(now(),interval -%d minute) as date)" ), hours*60+minutes );
		dto->result = StrDup( result );
		AddLink( &g.date_offsets, dto );
	}
	return dto->result;
}

#undef PopODBCEx
void PopODBCEx( PODBC odbc )
{
	PopODBCExx( odbc, FALSE DBG_SRC );
}

#undef PopODBC
void PopODBC( void )
{
	PopODBCEx( NULL );
}

PODBC SQLGetODBCEx( CTEXTSTR dsn, CTEXTSTR user, CTEXTSTR pass )
{
	INDEX idx;
	struct odbc_queue *queue;
retry:
	LIST_FORALL( g.odbc_queues, idx, struct odbc_queue*, queue )
	{
		if( StrCaseCmp( dsn, queue->name ) == 0 )
		{

			PODBC odbc = (PODBC)DequeLink( &queue->connections );
			if( odbc )
				odbc->queue = queue;

			if( !odbc )
				odbc = ConnectToDatabaseLogin( dsn, user, pass, FALSE DBG_SRC );
			if( odbc )
				odbc->queue = queue;
			return odbc;
		}
	}
	{
		queue = New( struct odbc_queue );
		queue->name = StrDup( dsn );
		queue->connections = CreateLinkQueue();
		AddLink( &g.odbc_queues, queue );
		goto retry;
	}
}



PODBC SQLGetODBC( CTEXTSTR dsn )
{
	return SQLGetODBCEx( dsn, NULL, NULL );
}

void SQLDropODBC( PODBC odbc )
{
	EnqueLink( &odbc->queue->connections, odbc );
}

POINTER GetODBCHandle( PODBC odbc ) {
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	if( odbc->flags.bSQLite_native )
		return (POINTER)odbc->db;
	else
#endif
#ifdef USE_ODBC
		return (POINTER)odbc->hdbc;
#else
		;
#endif
	return NULL;
}


void SQLDropAndCloseODBC( CTEXTSTR dsn )
{
	INDEX idx;
	struct odbc_queue *queue;
	LIST_FORALL( g.odbc_queues, idx, struct odbc_queue*, queue )
	{
		if( StrCaseCmp( dsn, queue->name ) == 0 )
		{
			PODBC odbc;
			while( ( odbc = (PODBC)DequeLink( &queue->connections ) ) )
				CloseDatabase( odbc );
			break;
		}
	}
}

void SetSQLCorruptionHandler( PODBC odbc, void (CPROC*f)(uintptr_t psv,PODBC odbc), uintptr_t psv ) {
	if( odbc ) {
		odbc->pCorruptionHandler = f;
		odbc->psvCorruptionHandler = psv;
	}
}

SQL_NAMESPACE_END

