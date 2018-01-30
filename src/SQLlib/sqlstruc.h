#include <pssql.h>
#include <filesys.h>
#ifndef SQL_STRUCT_DEFINED
#define SQL_STRUCT_DEFINED


# if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
#     include <sqlite3.h>
# endif

#if defined( __NO_ODBC__ )
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
#else
#  define USE_ODBC
#  include <sql.h>
#  include <sqlext.h>
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
	uint32_t      responce;
	uint32_t      lastop;
   int    *column_types;
	size_t *result_len;
	TEXTSTR *results;
	//uint32_t nResults; // this is columns
	TEXTSTR *fields;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	sqlite3_stmt *stmt;
#endif
#if !defined( __NO_ODBC__ )
	SQLHSTMT    hstmt;
#endif
	SQLSMALLINT columns;
	PTEXT result_text;
	SQLULEN  *colsizes;
	SQLSMALLINT *coltypes;
	DeclareLink( struct data_collection_tag );
#ifdef WINDOWS_PROXY_EXTENSION
	uint32_t MyID;
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
	TEXTCHAR pConnString[256];
} DB_INFO, *PDB_INFO;

struct odbc_handle_tag{
	DB_INFO info;
#if defined( USE_SQLITE ) || defined( USE_SQLITE_INTERFACE )
	sqlite3 *db;
#endif
#if !defined( __NO_ODBC__ )
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
#if USE_ODBC
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
		BIT_FIELD bAutoCheckpoint : 1; // sqlite; alternative to closing; generate wal_checkpoints automatically on idle.
		BIT_FIELD bVFS : 1;
		BIT_FIELD bClosed : 1;
	} flags;
	uint32_t last_command_tick; // this one tracks auto commit state; it is cleared when a commit happens
	uint32_t last_command_tick_; // this one tracks truly the last operation on a connection
	uint32_t commit_timer;
	PCOLLECT collection;
	int native; // saved for resulting with native error code...
	uintptr_t psvUser; // allow user to associate some data with this.
	CRITICALSECTION cs;
	int nProtect; // critical section is currently owned
	PTHREAD auto_commit_thread;
	PTHREAD auto_close_thread;
	PTHREAD auto_checkpoint_thread;
	struct odbc_queue *queue;
	void (CPROC*auto_commit_callback)(uintptr_t,PODBC);
	uintptr_t auto_commit_callback_psv;
};

struct odbc_queue
{
	CTEXTSTR name;
   PLINKQUEUE connections;
};


#ifdef SQLLIB_SOURCE
struct pssql_global
{
	CRITICALSECTION Init;
	//POPTION_INTERFACE pOptionInterface;
	uint32_t PrimaryLastConnect, BackupLastConnect;
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
		BIT_FIELD bAutoCheckpoint : 1;
		BIT_FIELD bAutoCheckpointRecover : 1;
	} flags;
	struct update_task_def *UpdateTasks;
	PSERVICE_ROUTE SQLMsgBase;
	FILE *pSQLLog;
	void (CPROC*feedback_handler)(CTEXTSTR message);
	ODBC OptionDb; // a third, well-known DSN used for option library by default.  May be SQLite.
	PLIST date_offsets;
	PLIST odbc_queues;
	PLIST option_database_init;
	PLIST database_init;
};
#endif

INDEX GetIndexOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name);
PTREEROOT GetTableCache( PODBC odbc, CTEXTSTR tablename );
CTEXTSTR GetKeyOfName(PODBC odbc, CTEXTSTR table,CTEXTSTR name);
int OpenSQL( DBG_VOIDPASS );



#ifdef USE_SQLITE_INTERFACE
#  if defined( __WATCOMC__ ) && !defined( BUILDS_INTERFACE ) && ( __WATCOMC__ < 1300 )
#    define FIXREF
#    define FIXDEREF
#    define FIXREF2 *
#    define FIXDEREF2 *
#  else
#    define FIXREF
#    define FIXDEREF
#    define FIXREF2
#    define FIXDEREF2
#  endif
struct sqlite_interface
{
	void(FIXREF2 *sqlite3_result_text)(sqlite3_context*, const char*, int, void(*)(void*));
	void*(FIXREF*sqlite3_user_data)(sqlite3_context*);
	sqlite3_int64 (FIXREF2*sqlite3_last_insert_rowid)(sqlite3*);
	int (FIXREF*sqlite3_create_function)(  sqlite3 *db,
	  const char *zFunctionName,
	  int nArg,
	  int eTextRep,
	  void *pApp,
	  void (*xFunc)(sqlite3_context*,int,sqlite3_value**),
	  void (*xStep)(sqlite3_context*,int,sqlite3_value**),
	  void (*xFinal)(sqlite3_context*)
	);
	int (FIXREF2*sqlite3_get_autocommit)(sqlite3*);
	int (FIXREF2*sqlite3_open)(  const char *filename,   /* Database filename (UTF-8) */
	  sqlite3 **ppDb          /* OUT: SQLite db handle */
	);
	int (FIXREF2*sqlite3_open_v2)(
	  const char *filename,   /* Database filename (UTF-8) */
	  sqlite3 **ppDb,         /* OUT: SQLite db handle */
	  int flags,              /* Flags */
	  const char *zVfs        /* Name of VFS module to use */
	);
	const char* (FIXREF2*sqlite3_errmsg)(sqlite3*);
	int (FIXREF2*sqlite3_finalize)(sqlite3_stmt *);
	int (FIXREF2*sqlite3_close)(sqlite3*);
#  if ( SQLITE_VERSION_NUMBER > 3007013 )
	int (FIXREF2*sqlite3_close_v2)(sqlite3*);
#  endif
	int (FIXREF2*sqlite3_prepare_v2)(
	  sqlite3 *db,            
	  const char *zSql,       
	  int nByte,              
	  sqlite3_stmt **ppStmt,  
	  const char **pzTail     );
	int (FIXREF2*sqlite3_prepare16_v2)(
		  sqlite3 *db,            /* Database handle */
		  const void *zSql,       /* SQL statement, UTF-16 encoded */
		  int nByte,              /* Maximum length of zSql in bytes. */
		  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
		  const void **pzTail     /* OUT: Pointer to unused portion of zSql */
		);
	int (FIXREF2*sqlite3_step)(sqlite3_stmt *);
	const char* (FIXREF2*sqlite3_column_name)(sqlite3_stmt *pStmt, int col);
	const unsigned char* (FIXREF*sqlite3_column_text)(sqlite3_stmt *pStmt, int col);
	int (FIXREF2*sqlite3_column_bytes)(sqlite3_stmt *pStmt, int col);
	int (FIXREF*sqlite3_column_type )(sqlite3_stmt *pStmt, int col);
	int (FIXREF2*sqlite3_column_count)(sqlite3_stmt *pStmt);
	int (FIXREF2*sqlite3_config)(int,...);
	int (FIXREF2*sqlite3_db_config)(sqlite3*, int op, ...);
	// allow full definition of a VFS including the FS interface
	void (*InitVFS)( CTEXTSTR name, struct file_system_mounted_interface *fsi );
	sqlite3_backup *( FIXREF2*sqlite3_backup_init)(
			  sqlite3 *pDest,                        /* Destination database handle */
			  const char *zDestName,                 /* Destination database name */
			  sqlite3 *pSource,                      /* Source database handle */
			  const char *zSourceName                /* Source database name */
				);
	int ( FIXREF2*sqlite3_backup_step)(sqlite3_backup *p, int nPage);
	int ( FIXREF2*sqlite3_backup_remaining)(sqlite3_backup *p);
	//int ( FIXREF2*sqlite3_backup_pagecount)(sqlite3_backup *p);
	int ( FIXREF2*sqlite3_backup_finish)(sqlite3_backup *p);
	int ( FIXREF2*sqlite3_extended_errcode)(sqlite3 *db);
	int ( FIXREF2*sqlite3_stmt_readonly)(sqlite3_stmt *pStmt);
	const char *( FIXREF2*sqlite3_column_table_name )( sqlite3_stmt *odbc, int col );
	const char *( FIXREF2*sqlite3_column_table_alias )( sqlite3_stmt *odbc, int col );
};

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

#  ifndef BUILDS_INTERFACE
#    define sqlite3_result_text          (FIXDEREF2 (sqlite_iface->sqlite3_result_text))
#    define sqlite3_user_data            (FIXDEREF (sqlite_iface->sqlite3_user_data))
#    define sqlite3_last_insert_rowid    (FIXDEREF2 (sqlite_iface->sqlite3_last_insert_rowid))
#    define sqlite3_create_function      (FIXDEREF (sqlite_iface->sqlite3_create_function))
#    define sqlite3_get_autocommit       (FIXDEREF2 (sqlite_iface->sqlite3_get_autocommit))
#    define sqlite3_open(a,b)            (sqlite_iface)?(FIXDEREF2((sqlite_iface)->sqlite3_open))(a,b):SQLITE_ERROR
#    define sqlite3_open_v2(a,b,c,d)     (sqlite_iface)?(FIXDEREF2((sqlite_iface)->sqlite3_open_v2))(a,b,c,d):SQLITE_ERROR
#    define sqlite3_errmsg(db)           (sqlite_iface)?(FIXDEREF2((sqlite_iface)->sqlite3_errmsg))(db):"No Sqlite3 Interface"
#    define sqlite3_finalize             (FIXDEREF2 (sqlite_iface->sqlite3_finalize))
#    define sqlite3_close                (FIXDEREF2 (sqlite_iface->sqlite3_close))
#    define sqlite3_close_v2             (FIXDEREF2 (sqlite_iface->sqlite3_close_v2))
#    define sqlite3_prepare_v2           (FIXDEREF2 (sqlite_iface->sqlite3_prepare_v2))
#    define sqlite3_prepare16_v2         (FIXDEREF2 (sqlite_iface->sqlite3_prepare16_v2))
#    define sqlite3_step                 (FIXDEREF2 (sqlite_iface->sqlite3_step))
#    define sqlite3_column_name          (FIXDEREF2 (sqlite_iface->sqlite3_column_name))
#    define sqlite3_column_text          (FIXDEREF (sqlite_iface->sqlite3_column_text))
#    define sqlite3_column_bytes         (FIXDEREF2 (sqlite_iface->sqlite3_column_bytes))
#    define sqlite3_column_type          (FIXDEREF (sqlite_iface->sqlite3_column_type))
#    define sqlite3_column_count         (FIXDEREF2 (sqlite_iface->sqlite3_column_count))
#    define sqlite3_config               (FIXDEREF2 (sqlite_iface->sqlite3_config))
#    define sqlite3_db_config            (FIXDEREF2 (sqlite_iface->sqlite3_db_config))
#    define sqlite3_backup_init          (FIXDEREF2 (sqlite_iface->sqlite3_backup_init))
#    define sqlite3_backup_step          (FIXDEREF2 (sqlite_iface->sqlite3_backup_step))
#    define sqlite3_backup_finish        (FIXDEREF2 (sqlite_iface->sqlite3_backup_finish))
#    define sqlite3_backup_remaining     (FIXDEREF2 (sqlite_iface->sqlite3_backup_remaining))
#    define sqlite3_extended_errcode     (FIXDEREF2 (sqlite_iface->sqlite3_extended_errcode))
#    define sqlite3_stmt_readonly        (FIXDEREF2 (sqlite_iface->sqlite3_stmt_readonly))
#    define sqlite3_column_table_name    (FIXDEREF2 (sqlite_iface->sqlite3_column_table_name))
#    define sqlite3_column_table_alias (FIXDEREF2 (sqlite_iface->sqlite3_column_table_alias))
#  endif
#endif

SQL_NAMESPACE_END

#endif
