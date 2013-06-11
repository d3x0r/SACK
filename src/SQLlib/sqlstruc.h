#include <pssql.h>

#ifndef SQL_STRUCT_DEFINED
#define SQL_STRUCT_DEFINED
# if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
#     include <sqlite3.h>
#  ifndef USE_ODBC
// if not using odbc, need these 
// otherwise they will be defined in sql.h
typedef int RETCODE; // sqllite uses a generic int type for result codes
typedef int SQLSMALLINT;
typedef unsigned int SQLULEN;
typedef int SQLINTEGER;
enum {
	SQL_HANDLE_DBC
      , SQL_HANDLE_STMT
};
#  endif
# endif
#ifdef USE_ODBC
#include <sql.h>
#include <sqlext.h>
#endif
#include <msgclient.h>
#include <msgprotocol.h>
#include <timers.h> // critical section

SQL_NAMESPACE

enum {
	WM_SQL_COMMAND = MSG_UserServiceMessages
	  , WM_SQL_QUERY
	  , WM_SQL_DATA_START
	  , WM_SQL_DATA_MORE
	  , WM_SQL_RESULT_ERROR
	  , WM_SQL_RESULT_SUCCESS
     , WM_SQL_RESULT_DATA
     , WM_SQL_RESULT_MORE
	  , WM_SQL_MORE
	  , WM_SQL_GET_ERROR
	  , WM_SQL_RESULT_NO_DATA // no error, but no lines result
	  // which is not the same as no error, but an empty line resulted...
	  , WM_SQL_QUERY_RECORD
     , WM_SQL_RESULT_RECORD
     , WM_SQL_NUM_MESSAGES
};


typedef struct data_collection_tag
{
	struct {
		BIT_FIELD  bTemporary  : 1; // sql commands during queries are given temporary status.
		BIT_FIELD  bDynamic : 1; // not a static member
		BIT_FIELD  bBuildResultArray  : 1;
		BIT_FIELD bEndOfFile : 1;
		BIT_FIELD bPushed : 1; // pop should work up to this point.
	} flags;
	PVARTEXT pvt_out; // the last SQL command for this...
	PVARTEXT pvt_result; // the last result for this...
	PVARTEXT pvt_errorinfo; // the last error info for this...
	PSERVICE_ROUTE SourceID;
	struct odbc_handle_tag *odbc;
	_32      responce;
	_32      lastop;
	_32 *result;
	_32 *result_len;
	TEXTSTR *results;
	//_32 nResults; // this is columns
	TEXTSTR *fields;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	sqlite3_stmt *stmt;
#endif
#ifdef USE_ODBC
	SQLHSTMT    hstmt;
#endif
	SQLSMALLINT columns;
	PTEXT result_text;
	SQLULEN  *colsizes;
	SQLSMALLINT *coltypes;
	DeclareLink( struct data_collection_tag );
#ifdef WINDOWS_PROXY_EXTENSION
	_32 MyID;
	HWND     hLastWnd;
	HWND     hWnd;
#endif
} COLLECT, *PCOLLECT;

typedef struct database_info_tag
{
	struct {
		BIT_FIELD  bAutoUser  : 1;
	} flags;
	CTEXTSTR pDSN;
	TEXTCHAR pID[64];
	TEXTCHAR pPASSWORD[64];
} DB_INFO, *PDB_INFO;

struct odbc_handle_tag{
	DB_INFO info;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	sqlite3 *db;
#endif
#ifdef USE_ODBC
	SQLHENV    env;  // odbc database access handle...
	SQLHDBC    hdbc; // handle to database connection
#endif
	struct {
		BIT_FIELD  bConnected  : 1;
		BIT_FIELD  bAccess  : 1; // operate as if talking to an access MDB
		BIT_FIELD  bSQLite  : 1; // sqllite via sqlite odbc driver...
		BIT_FIELD  bMySQL : 1;  // for selecting how transactions are done.
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
		BIT_FIELD  bSQLite_native  : 1;
#endif
#ifdef USE_ODBC
		BIT_FIELD  bODBC  : 1; // odbc is actually an odbc
#endif
		BIT_FIELD bSkipODBC  : 1; // set to skip ODBC connection (if dsn has a '.' in it)
		BIT_FIELD bNoLogging  : 1; // do NOT log to SQL.log
		BIT_FIELD bPushed : 1; // pop should work up to this point. just did a push... next collector should be marked.
		BIT_FIELD bForceConnection : 1; // same sort of thing as the global flag... but that should just apply to default connection?
		BIT_FIELD bFailEnvOnDbcFail : 1;
		// generate begintransaction and commit automatically.
		BIT_FIELD bAutoTransact : 1;
		BIT_FIELD bThreadProtect : 1; // use enter/leave critical section on this connector (auto transact protector)
		BIT_FIELD bAutoClose : 1; // don't leave the connection open 100%; open when required and close when idle
	} flags;
	_32 last_command_tick;
	_32 commit_timer;
	PCOLLECT collection;
	int native; // saved for resulting with native error code...
	PTRSZVAL psvUser; // allow user to associate some data with this.
	CRITICALSECTION cs;
	int nProtect; // critical section is currently owned
	PTHREAD auto_commit_thread;
	PTHREAD auto_close_thread;
   struct odbc_queue *queue;
};

struct odbc_queue
{
	CTEXTSTR name;
   PLINKQUEUE connections;
};


#ifdef SQLLIB_SOURCE
typedef struct global_tag
{
   CRITICALSECTION Init;
   //POPTION_INTERFACE pOptionInterface;
   _32 PrimaryLastConnect, BackupLastConnect;
   ODBC Primary, Backup;
	PODBC odbc; // current connection
#ifdef __cplusplus
//	sack::containers::list::
#endif
	PLIST pOpenODBC; // list of PODBC objects which are open.
	PCOLLECT collections; // collections which were created for g.odbc while it was NULL
   COLLECT LocalCollect; // this is used for fatal errors when neither primary or backup are set in g.odbc
   COLLECT TimerCollect; // used when checking status... as a static element it's more reliable.
   struct {
      BIT_FIELD  bInited  : 1;
      // when this happens, something
      // needs to be done to update information from the
      // backup to the primary
      // and from the primary to the backup
      // if BOTH have failed - we're how shall I say... fucked.
      BIT_FIELD  bPrimaryRestored  : 1;
      BIT_FIELD  bNoBackup  : 1;
      BIT_FIELD  bPrimaryUp  : 1;
      BIT_FIELD  bBackupUp  : 1;
      BIT_FIELD  bNoLog  : 1;
      BIT_FIELD  bOpening  : 1;
      BIT_FIELD  bMySQL  : 1;
		BIT_FIELD  bPostgresql  : 1;
		BIT_FIELD  bBadODBC  : 1;
		BIT_FIELD  bLogging  : 1; // do log to SQL.log
		BIT_FIELD  bFallback : 1;
		BIT_FIELD  bRequireConnection : 1;
		BIT_FIELD bLogData : 1;
		BIT_FIELD  bLogOptionConnection : 1;
		BIT_FIELD bCriticalSectionInited : 1;
		BIT_FIELD bDeadstartCompleted : 1;
   } flags;
	struct update_task_def *UpdateTasks;
   PSERVICE_ROUTE SQLMsgBase;
	FILE *pSQLLog;
	void (CPROC*feedback_handler)(CTEXTSTR message);
	ODBC OptionDb; // a third, well-known DSN used for option library by default.  May be SQLite.
	PLIST date_offsets;
   PLIST odbc_queues;
} GLOBAL;
#endif

INDEX GetIndexOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name);
PTREEROOT GetTableCache( PODBC odbc, CTEXTSTR tablename );
CTEXTSTR GetKeyOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name);
int OpenSQL( DBG_VOIDPASS );

#ifdef USE_SQLITE_INTERFACE
struct sqlite_interface
{
void(*sqlite3_result_text)(sqlite3_context*, const char*, int, void(*)(void*));
void*(*sqlite3_user_data)(sqlite3_context*);
sqlite3_int64 (*sqlite3_last_insert_rowid)(sqlite3*);
int (*sqlite3_create_function)(  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
  void (*xFinal)(sqlite3_context*)
);
int (*sqlite3_get_autocommit)(sqlite3*);
int (*sqlite3_open)(  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb          /* OUT: SQLite db handle */
);
const char* (*sqlite3_errmsg)(sqlite3*);
int (*sqlite3_finalize)(sqlite3_stmt *);
int (*sqlite3_close)(sqlite3*);
int (*sqlite3_prepare_v2)(
  sqlite3 *db,            
  const char *zSql,       
  int nByte,              
  sqlite3_stmt **ppStmt,  
  const char **pzTail     );
int (*sqlite3_step)(sqlite3_stmt *);
const char* (*sqlite3_column_name)(sqlite3_stmt *pStmt, int col);
const unsigned char* (*sqlite3_column_text)(sqlite3_stmt *pStmt, int col);
int (*sqlite3_column_bytes)(sqlite3_stmt *pStmt, int col);
int (*sqlite3_column_type )(sqlite3_stmt *pStmt, int col);
int (*sqlite3_column_count)(sqlite3_stmt *pStmt);
int (*sqlite3_config)(int,...);
int (*sqlite3_db_config)(sqlite3*, int op, ...);

};

#ifdef USES_SQLITE_INTERFACE
#  ifndef DEFINES_SQLITE_INTERFACE
extern
#  endif
        struct sqlite_interface *sqlite_iface;
#  ifdef DEFINES_SQLITE_INTERFACE
PRIORITY_PRELOAD( LoadSQLiteInterface, SQL_PRELOAD_PRIORITY-1 )
{
   sqlite_iface = (struct sqlite_interface*)GetInterface( WIDE("sqlite3") );
}
#  endif
#endif

#ifndef BUILDS_INTERFACE
#define sqlite3_result_text          sqlite_iface->sqlite3_result_text
#define sqlite3_user_data            sqlite_iface->sqlite3_user_data
#define sqlite3_last_insert_rowid    sqlite_iface->sqlite3_last_insert_rowid
#define sqlite3_create_function      sqlite_iface->sqlite3_create_function
#define sqlite3_get_autocommit       sqlite_iface->sqlite3_get_autocommit
#define sqlite3_open(a,b)                 (sqlite_iface)?(sqlite_iface)->sqlite3_open(a,b):SQLITE_ERROR
#define sqlite3_errmsg(db)               (sqlite_iface)?(sqlite_iface)->sqlite3_errmsg(db):"No Sqlite3 Interface"
#define sqlite3_finalize             sqlite_iface->sqlite3_finalize
#define sqlite3_close                sqlite_iface->sqlite3_close
#define sqlite3_prepare_v2           sqlite_iface->sqlite3_prepare_v2
#define sqlite3_step                 sqlite_iface->sqlite3_step
#define sqlite3_column_name          sqlite_iface->sqlite3_column_name
#define sqlite3_column_text          sqlite_iface->sqlite3_column_text
#define sqlite3_column_bytes         sqlite_iface->sqlite3_column_bytes
#define sqlite3_column_type          sqlite_iface->sqlite3_column_type
#define sqlite3_column_count         sqlite_iface->sqlite3_column_count
#define sqlite3_config         sqlite_iface->sqlite3_config
#define sqlite3_db_config         sqlite_iface->sqlite3_db_config
#endif

#endif

//-----------------------------------------------
// Private Exports for plugins to use...

//#define SQL

SQL_NAMESPACE_END

#endif
