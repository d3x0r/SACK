
#define NO_UNICODE_C
#include <stdhdrs.h>
#ifdef __WATCOMC__
// definition of SH_DENYNO
#  include <share.h>
#endif
#include <procreg.h>
#include <filesys.h>
#include <deadstart.h>
#include <sack_vfs.h>
#define SQLLIB_SOURCE
#define BUILDS_INTERFACE
#define DEFINES_SQLITE_INTERFACE
#include "../../SQLlib/sqlstruc.h"
#include "sqlite3.h"
//#define LOG_OPERATIONS
#ifdef USE_SQLITE_INTERFACE

SQL_NAMESPACE

static void InitVFS( CTEXTSTR name, struct file_system_mounted_interface *fsi );

struct sqlite_interface my_sqlite_interface = {
	sqlite3_result_text
	 , sqlite3_user_data
	 , sqlite3_last_insert_rowid
	 , sqlite3_create_function
	 , sqlite3_get_autocommit
	 , sqlite3_open
	 , sqlite3_open_v2
	 , sqlite3_errmsg
	 , sqlite3_finalize
	 , sqlite3_close
#if ( SQLITE_VERSION_NUMBER > 3007013 )
         , sqlite3_close_v2
#endif
         , sqlite3_prepare_v2
#ifdef _UNICODE
         , sqlite3_prepare16_v2
#else
         , NULL
#endif
                                           , sqlite3_step
                                           , sqlite3_column_name
                                           , sqlite3_column_text
                                           , sqlite3_column_bytes
                                           , sqlite3_column_type
                                           , sqlite3_column_count
                                           , sqlite3_config
															 , sqlite3_db_config
															 , InitVFS
															 , sqlite3_backup_init
															 , sqlite3_backup_step
															 , sqlite3_backup_remaining
															 , sqlite3_backup_finish
															 , sqlite3_extended_errcode
                                           , sqlite3_stmt_readonly
                                           , sqlite3_column_table_name
                                           , sqlite3_column_table_alias
};

struct lockInfo {
	volatile struct my_sqlite3_vfs_file_data* fd;
	volatile THREAD_ID id;
};
struct my_sqlite3_vfs_file_lock {
	volatile DATALIST *shares;
	volatile struct lockInfo reserve;
	volatile struct lockInfo pending;
	volatile struct lockInfo exclusive;
	CRITICALSECTION csLock;
};

struct my_sqlite3_vfs_file_data
{
	struct sqlite3_io_methods *pMethods; // must be first member to be file subclass...
	FILE *file;
	//CRITICALSECTION cs;
	TEXTSTR filename;
	struct file_system_mounted_interface *mount;
	LOGICAL temp;
	PLIST otherHandles;
	int locktype;
	struct my_sqlite3_vfs_file_lock locks_;
	struct my_sqlite3_vfs_file_lock* locks;
};

struct my_sqlite3_vfs
{
	sqlite3_vfs vfs;
	struct file_system_mounted_interface *mount;
};

#ifdef l
#  undef l
#endif

#define l (*local_sqlite_interface)

struct local_data {
	PLIST registered_vfs;
	PLIST openFiles;
}
#ifdef __STATIC_GLOBALS__
    local_sqlite_interface__
#endif
;

struct local_data *local_sqlite_interface
#ifdef __STATIC_GLOBALS__
	= &local_sqlite_interface__
#endif
;

//typedef struct sqlite3_io_methods sqlite3_io_methods;


int xClose(sqlite3_file*file)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	if( my_file->file )
	{
		sack_fclose( my_file->file );
#ifdef LOG_OPERATIONS
		lprintf( "Close %p %s", my_file->file, my_file->filename );
#endif
		my_file->file = NULL;
	}
	if( my_file->temp )
	{
		sack_unlinkEx( 0, my_file->filename, my_file->mount );
#ifdef LOG_OPERATIONS
		lprintf( "unlink temp file : %s", my_file->filename );
#endif
	}
	EnterCriticalSec( &my_file->locks->csLock );
	{
		INDEX idx;
		struct my_sqlite3_vfs_file_data* check;
		LIST_FORALL( my_file->otherHandles, idx, struct my_sqlite3_vfs_file_data*, check ) {
			if( my_file->locks == &my_file->locks_ ) {
				// C++ Error: use of deleted function converting to '=' operator
				///check->locks_ = my_file->locks_;

				// they're the same structure type and everything, but C++ can't just assign them
				// but C++ throws some warning about assignment with no trivial copy-assigment.
				// (see above deleted operator)
				memcpy( &check->locks_ , & my_file->locks_, sizeof( check->locks_ ) );

				my_file->locks = &check->locks_;
			}
			check->locks = my_file->locks;
			DeleteLink( &check->otherHandles, my_file );
		}
		if( my_file->locks == &my_file->locks_ ) {
			DeleteDataList( (PDATALIST*)&my_file->locks_.shares );
		}
		DeleteList( &my_file->otherHandles );
	}
	LeaveCriticalSec( &my_file->locks->csLock );
	DeleteLink( &local_sqlite_interface->openFiles, file );
	return SQLITE_OK;
}

int xRead(sqlite3_file*file, void*buffer, int iAmt, sqlite3_int64 iOfst)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	size_t actual;
#ifdef LOG_OPERATIONS
	lprintf( "read %p %s %d  %d", my_file->file, my_file->filename, iAmt, iOfst );
#endif
	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );
	if( ( actual = sack_fread( buffer, 1, iAmt, my_file->file ) ) == (size_t)iAmt )
	{
#ifdef LOG_OPERATIONS
		//LogBinary( buffer, iAmt );
#endif
		return SQLITE_OK;
	}
#ifdef LOG_OPERATIONS
	lprintf( "Errno : %d", errno );
#endif
	MemSet( ((char*)buffer)+actual, 0, iAmt - actual );
	return SQLITE_IOERR_SHORT_READ;

}

int xWrite(sqlite3_file*file, const void*buffer, int iAmt, sqlite3_int64 iOfst)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	size_t actual;
#ifdef LOG_OPERATIONS
	lprintf( "Write %p %s %d at %d", my_file->file, my_file->filename, iAmt, iOfst );
	//LogBinary( buffer, iAmt );
#endif
	{
		size_t filesize = sack_fsize( my_file->file );
		if( USS_LT( filesize, size_t, iOfst, sqlite3_int64 ) )
		{
			static unsigned char *filler;
			if( !filler )
			{
				filler = NewArray( unsigned char, 512 );
				MemSet( filler, 0, 512 );
			}
			sack_fseek( my_file->file, 0, SEEK_END );
			while( USS_LT( filesize, size_t, iOfst, sqlite3_int64 ) )
			{
				if( ( iOfst - filesize ) >= 512 )
				{
					sack_fwrite( filler, 1, 512, my_file->file );
					filesize += 512;
				}
				else
				{
					sack_fwrite( filler, 1, (int)( iOfst - filesize ), my_file->file );
					filesize += ( (size_t)iOfst - filesize );
				}
			}
		}
	}
	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );

	if( (size_t)iAmt == ( actual = sack_fwrite( buffer, 1, iAmt, my_file->file ) ) )
	{
#ifdef LOG_OPERATIONS
		lprintf( "file  %s is now %d", my_file->filename, sack_fsize( my_file->file ) );
#endif
		return SQLITE_OK;
	}
	return SQLITE_IOERR_WRITE;
}

int xTruncate(sqlite3_file*file, sqlite3_int64 size)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	sack_fseek( my_file->file, (size_t)size, SEEK_SET );
	sack_ftruncate( my_file->file ); // works through file system interface...
	//SetFileLength( my_file->filename, (size_t)size );
	return SQLITE_OK;
}

int xSync(sqlite3_file*file, int flags)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	if( !my_file->file )
		return SQLITE_OK;
#ifdef LOG_OPERATIONS
	lprintf( "Sync on %s", my_file->filename );
#endif
	sack_fflush( my_file->file );
	/* noop */
	return SQLITE_OK;
}

int xFileSize(sqlite3_file*file, sqlite3_int64 *pSize)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	if( my_file->file )
		(*pSize) = sack_fsize( my_file->file );
	else {
		(*pSize) = 0;
		return SQLITE_OK;
		//return SQLITE_IOERR_FSTAT;
	}
#ifdef LOG_OPERATIONS
	lprintf( "Get File size result of %s %d", my_file->filename, (*pSize) );
#endif
	return SQLITE_OK;
}

#ifdef DEBUG_FILE_LOCKING
struct lock_history_entry {
	int lockUnlock;
	int locktype;
	struct my_sqlite3_vfs_file_data* file;
	int shares;
	struct my_sqlite3_vfs_file_data* res;
	struct my_sqlite3_vfs_file_data* pend;
	struct my_sqlite3_vfs_file_data* excl;
};

int nHistory = 0;
struct lock_history_entry history[10000];

static void markHistory( int lock, struct my_sqlite3_vfs_file_data* file, int locktype ) {
	history[nHistory].lockUnlock = lock;
	history[nHistory].locktype = locktype;
	history[nHistory].file = file;
	history[nHistory].shares = (int)file->locks->shares->Cnt;
	history[nHistory].res = (struct my_sqlite3_vfs_file_data*)file->locks->reserve.fd;
	history[nHistory].pend = (struct my_sqlite3_vfs_file_data*) file->locks->pending.fd;
	history[nHistory].excl = (struct my_sqlite3_vfs_file_data*) file->locks->exclusive.fd;
	nHistory++;
	if( nHistory >= 10000 ) nHistory = 0;
}
#endif

static void removeOldLock( struct my_sqlite3_vfs_file_data* my_file ) {
	switch( my_file->locktype ) {
	case SQLITE_LOCK_NONE:
		//lprintf( "Had no lock, why is this still unlocking?" );
		break;
	case SQLITE_LOCK_SHARED:
		{
			INDEX idx;
			struct lockInfo* lock;
			DATA_FORALL( my_file->locks->shares, idx, struct lockInfo*, lock ) {
				if( lock->fd == my_file )
					break;
			}
			if( lock )
				DeleteDataItem( (PDATALIST*)&my_file->locks->shares, idx );
		}
		break;
	case SQLITE_LOCK_RESERVED:
		if( my_file->locks->reserve.fd == my_file )
			my_file->locks->reserve.fd = NULL;
		else
			lprintf( "Reserved lock type, but the lock wasn't owned by this thread" );
		break;
	case SQLITE_LOCK_PENDING:
		if( my_file->locks->pending.fd == my_file )
			my_file->locks->pending.fd = NULL;
		else
			lprintf( "Pending lock type, but the lock wasn't owned by this thread" );
		break;
	case SQLITE_LOCK_EXCLUSIVE:
		if( my_file->locks->exclusive.fd == my_file )
			my_file->locks->exclusive.fd = NULL;
		else
			lprintf( "Exclusive lock type, but the lock wasn't owned by this thread" );
		break;
	}
	my_file->locktype = SQLITE_LOCK_NONE;
}
#define LockReturn(n) { LeaveCriticalSec( &my_file->locks->csLock ); return n; }
int xLock(sqlite3_file*file, int locktype)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	THREAD_ID tid = GetThisThreadID();
	//markHistory( TRUE, my_file, locktype );
	EnterCriticalSec( &my_file->locks->csLock );
	//lprintf( "lock %p %d %d %p %p %p ", my_file, locktype, my_file->locktype, my_file->locks->reserve.fd, my_file->locks->pending.fd, my_file->locks->exclusive.fd );
	switch( locktype )
	{
	case SQLITE_LOCK_NONE:
		removeOldLock( my_file );
		my_file->locktype = locktype;
		break;
	case SQLITE_LOCK_SHARED:
		while( my_file->locks->exclusive.fd || my_file->locks->pending.fd ) {
			{
				INDEX idx;
				struct lockInfo* lock;
				DATA_FORALL( my_file->locks->shares, idx, struct lockInfo*, lock ) {
					if( lock->fd == my_file )
						break;
				}
				if( lock )
					DeleteDataItem( (PDATALIST*)&my_file->locks->shares, idx );
			}
			//if( FindLink( (PLIST*)&my_file->locks->shares, my_file ) != INVALID_INDEX )
			//	LockReturn( SQLITE_OK );
			LockReturn( SQLITE_BUSY );// Relinquish();
		}
		removeOldLock( my_file );
		{
			struct lockInfo li;
			li.fd = my_file;
			li.id = GetThisThreadID();
			AddDataItem( (PDATALIST*)&my_file->locks->shares, &li );
		}
		//AddLink( (PLIST*)&my_file->locks->shares, my_file );
		my_file->locktype = locktype;
		break;
	case SQLITE_LOCK_RESERVED:
		while( my_file->locks->reserve.fd || my_file->locks->exclusive.fd || my_file->locks->pending.fd )
			LockReturn( SQLITE_BUSY );//Relinquish();
		removeOldLock( my_file );
		my_file->locks->reserve.fd = my_file;
		my_file->locks->reserve.id = tid;
		my_file->locktype = locktype;
		break;
	case SQLITE_LOCK_PENDING:
		while( my_file->locks->exclusive.fd || my_file->locks->pending.fd )
			LockReturn( SQLITE_BUSY );//Relinquish();
		removeOldLock( my_file );
		my_file->locks->pending.fd = my_file;
		my_file->locks->pending.id = tid;
		my_file->locktype = locktype;
		break;
	case SQLITE_LOCK_EXCLUSIVE:
		//while( my_file->locks->shares || my_file->locks->exclusive || my_file->locks->pending || my_file->locks->reserve ) Relinquish();
		while( my_file->locks->exclusive.fd || my_file->locks->pending.fd ) {
			/*
			if( my_file->locks->reserve == my_file )
			{
				my_file->locks->pending = my_file;
				my_file->locks->reserve = NULL;
			}
			*/
			LockReturn( SQLITE_BUSY );//Relinquish();
		}
		{
			struct lockInfo* li;
			INDEX idx;
			DATA_FORALL( my_file->locks->shares, idx, struct lockInfo*, li ) {
				if( li->fd && li->id != tid )
					LockReturn( SQLITE_BUSY );
			}
		}
		removeOldLock( my_file );
		my_file->locks->exclusive.fd = my_file;
		my_file->locks->exclusive.id = tid;
		my_file->locktype = locktype;
		break;
	}
	LockReturn( SQLITE_OK );
}

int xUnlock(sqlite3_file*file, int locktype)
{
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	EnterCriticalSec( &my_file->locks->csLock );
	//markHistory( FALSE, my_file, locktype );
	//lprintf( "unlock %p %d %d", my_file, locktype, my_file->locktype );
	//removeOldLock( my_file );
	switch( locktype )
	{
	case SQLITE_LOCK_NONE:
	case SQLITE_LOCK_SHARED:
		switch( my_file->locktype ) {
		case SQLITE_LOCK_NONE:
			//lprintf( "Had no lock, why is this still unlocking?" );
			break;
		case SQLITE_LOCK_SHARED:
			if( locktype == SQLITE_LOCK_SHARED )
				lprintf( "THIS SHOULD BE ILLEGAL, Lock is shared, and going to shared with unlock." );
			{
				INDEX idx;
				struct lockInfo* lock;
				DATA_FORALL( my_file->locks->shares, idx, struct lockInfo*, lock ) {
					if( lock->fd == my_file )
						break;
				}
				if( lock )
					DeleteDataItem( (PDATALIST*)&my_file->locks->shares, idx );
				else
					lprintf( "Shared lock was not found in list of share locks" );

			}
			//if( !DeleteLink( (PLIST*)&my_file->locks->shares, my_file ) )
			//	lprintf( "Shared lock was not found in list of share locks" );
			break;
		case SQLITE_LOCK_RESERVED:
			if( my_file->locks->reserve.fd == my_file )
				my_file->locks->reserve.fd = NULL;
			else
				lprintf( "Reserved lock type, but the lock wasn't owned by this thread" );
			break;
		case SQLITE_LOCK_PENDING:
			if( my_file->locks->pending.fd == my_file )
				my_file->locks->pending.fd = NULL;
			else
				lprintf( "Pending lock type, but the lock wasn't owned by this thread" );
			break;
		case SQLITE_LOCK_EXCLUSIVE:
			if( my_file->locks->exclusive.fd == my_file )
				my_file->locks->exclusive.fd = NULL;
			else
				lprintf( "Exclusive lock type, but the lock wasn't owned by this thread" );
			break;
		}
		if( locktype == SQLITE_LOCK_SHARED ) {
			my_file->locktype = SQLITE_LOCK_SHARED;
			{
				struct lockInfo li;
				li.fd = my_file;
				li.id = GetThisThreadID();
				AddDataItem( (PDATALIST*)&my_file->locks->shares, &li );
			}
			//AddLink( (PLIST*)&my_file->locks->shares, my_file );
		}
		my_file->locktype = locktype;
		//memset( my_file->locks, 0, sizeof( *my_file->locks ) );
		break;
	case SQLITE_LOCK_RESERVED:
		lprintf( "never unlocks into reserved state?" );
		DebugBreak();
		//my_file->locks->reserve.fd = 0;
		break;
	case SQLITE_LOCK_PENDING:
		lprintf( "never unlocks into pending state?" );
		DebugBreak();
		my_file->locks->pending.fd = NULL;
		break;
	case SQLITE_LOCK_EXCLUSIVE:
		lprintf( "never unlocks into exclusive state?" );
		DebugBreak();
		my_file->locks->exclusive.fd = NULL;
		break;
	}
	LockReturn( SQLITE_OK );
}


int xCheckReservedLock(sqlite3_file*file, int *pResOut)
{
	struct my_sqlite3_vfs_file_data* my_file = (struct my_sqlite3_vfs_file_data*)file;
	if( my_file->locks->reserve.fd || my_file->locks->pending.fd || my_file->locks->exclusive.fd )
		*pResOut = TRUE;
	else
		*pResOut = FALSE;
	//lprintf( "Called checkReserved...%d", pResOut[0] );
	return SQLITE_OK;
}

int xFileControl(sqlite3_file*file, int op, void *pArg)
{
	struct my_sqlite3_vfs_file_data* my_file = (struct my_sqlite3_vfs_file_data*)file;
#ifdef LOG_OPERATIONS
	lprintf( "file %s control op: %d %p", my_file->filename, op, pArg );
#endif
	switch( op )
	{
#if ( SQLITE_VERSION_NUMBER > 3007013 )
	case SQLITE_FCNTL_BUSYHANDLER:
	case SQLITE_FCNTL_TEMPFILENAME:
		break;
	case SQLITE_FCNTL_HAS_MOVED:
		{
			int *val = (int*)pArg;
			(*val)= 0;
		}
		break;
#endif
	case SQLITE_FCNTL_LOCKSTATE:
		lprintf( "Return file's lock state" );
		if( my_file->locks->exclusive.fd )
			((int*)pArg)[0] = SQLITE_LOCK_EXCLUSIVE;
		else if( my_file->locks->pending.fd )
			((int*)pArg)[0] = SQLITE_LOCK_PENDING;
		else if( my_file->locks->reserve.fd )
			((int*)pArg)[0] = SQLITE_LOCK_RESERVED;
		else if( my_file->locks->shares->Cnt )
			((int*)pArg)[0] = SQLITE_LOCK_SHARED;
		else
			((int*)pArg)[0] = SQLITE_LOCK_NONE;
		break;
	case SQLITE_GET_LOCKPROXYFILE:
	case SQLITE_SET_LOCKPROXYFILE:
	case SQLITE_LAST_ERRNO:
		break;
	case SQLITE_FCNTL_SIZE_HINT:
		//lprintf( "hint is %d", *(int*)pArg );
		// might preallocate the file here...
		break;
	case SQLITE_FCNTL_CHUNK_SIZE:
	case SQLITE_FCNTL_FILE_POINTER:
	case SQLITE_FCNTL_SYNC_OMITTED:
	case SQLITE_FCNTL_WIN32_AV_RETRY:
	case SQLITE_FCNTL_PERSIST_WAL:
	case SQLITE_FCNTL_OVERWRITE:
	case SQLITE_FCNTL_VFSNAME:
	case SQLITE_FCNTL_POWERSAFE_OVERWRITE:
		break;
	case SQLITE_FCNTL_PRAGMA:
		{
			char **files = (char**)pArg;
			//char *name = files[3];
			//lprintf( "pragma... (%s)", files[1] );
			//files[0] = sqlite3_mprintf( "%s", files[2] );
			return SQLITE_NOTFOUND;
			//xOpen( my_file->
		}
		break;
	}
	return SQLITE_OK;
}

int xSectorSize(sqlite3_file*file)
{
	//struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	return 4096;
}

int xDeviceCharacteristics(sqlite3_file*file)
{
	//struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	return SQLITE_IOCAP_ATOMIC4K|SQLITE_IOCAP_SAFE_APPEND|SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN|SQLITE_IOCAP_POWERSAFE_OVERWRITE;
}


int xShmMap(sqlite3_file*file, int iPg, int pgsz, int a, void volatile**b)
{
	(void)file;
	(void)iPg;
	(void)pgsz;
	(void)a;
	(void)b;
	return 0;
}
int xShmLock(sqlite3_file*file, int offset, int n, int flags)
{
	(void)file;
	(void)offset;
	(void)n;
	(void)flags;
	return 0;
}

void xShmBarrier(sqlite3_file*file)
{
	(void)file;
}

int xShmUnmap(sqlite3_file*file, int deleteFlag)
{
	(void)file;
	(void)deleteFlag;
	return 0;
}


/* Methods above are valid for version 1 */
//int xShmMap(sqlite3_file*file, int iPg, int pgsz, int, void volatile**);
//int xShmLock(sqlite3_file*file, int offset, int n, int flags);
//void xShmBarrier(sqlite3_file*file);
//int xShmUnmap(sqlite3_file*file, int deleteFlag);
//{
// }
/* Methods above are valid for version 2 */
  /* Additional methods may be added in future releases */

struct sqlite3_io_methods my_methods = { 1
													/*, sizeof( struct my_sqlite3_vfs_file_data )*/
													, xClose
													, xRead
													, xWrite
													, xTruncate
													, xSync
													, xFileSize
													, xLock
													, xUnlock
													, xCheckReservedLock
													, xFileControl
													, xSectorSize
													, xDeviceCharacteristics
                                       , NULL //, xShmMap
                                       , NULL //, xShmLock
                                       , NULL //, xShmBarrier
                                       , NULL //, xShmUnmap
                                       , NULL  // xfetch
                                       , NULL // xunfetch
};

int xOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file*file,
			 int flags, int *pOutFlags)
{
	struct my_sqlite3_vfs *my_vfs = (struct my_sqlite3_vfs *)vfs;
	struct my_sqlite3_vfs_file_data *my_file = (struct my_sqlite3_vfs_file_data*)file;
	static int temp_id;
	char buf[32];
	AddLink( &local_sqlite_interface->openFiles, file );
	file->pMethods = &my_methods;
	my_file->otherHandles = NULL;
	my_file->mount = my_vfs->mount;
	if( zName == NULL )
	{
		snprintf( buf, 32, "sql-%d.tmp", temp_id++ );
		my_file->temp = TRUE;
		zName = buf;
	}
	else
		my_file->temp = FALSE;
#ifdef LOG_OPERATIONS
	lprintf( "Open file: %s (vfs:%s) (already)%p", zName, vfs->zName, my_file->file );
#endif

	my_file->filename = DupCStr( zName );

	{
		INDEX idx;
		struct my_sqlite3_vfs_file_data* check;
		LIST_FORALL( local_sqlite_interface->openFiles, idx, struct my_sqlite3_vfs_file_data*, check ) {
			if( check != my_file && StrCmp( check->filename, my_file->filename ) == 0 ) {
				if( check->mount == my_vfs->mount ) {
					my_file->locks = check->locks;
					AddLink( &my_file->otherHandles, check );
					AddLink( &check->otherHandles, my_file );
				}
			}
		}
		if( !my_file->locks ) {
			my_file->locks_.shares = CreateDataList( sizeof( struct lockInfo ) );
			my_file->locks = &my_file->locks_;
			InitializeCriticalSec( &my_file->locks_.csLock );
		}
	}
#if defined( __GNUC__ )
	//__ANDROID__
#define sack_fsopen(a,b,c,d) sack_fopen(a,b,c)
#define sack_fsopenEx(a,b,c,d,fsi) sack_fopenEx(a,b,c, fsi)
#endif
	if( my_vfs->mount )
	{
		//lprintf( "try on mount..%s .%p", my_file->filename, my_vfs->mount );
		if( (flags & (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) == (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) ) {
			if( !(my_file->file = sack_fsopenEx( 0, my_file->filename, "rb+", _SH_DENYNO, my_vfs->mount ) ) )
				my_file->file = sack_fsopenEx( 0, my_file->filename, "wb+", _SH_DENYNO, my_vfs->mount );
		} else if( flags & (SQLITE_OPEN_READWRITE) )
			my_file->file = sack_fsopenEx( 0, my_file->filename, "rb+", _SH_DENYNO, my_vfs->mount );
		else if( flags & SQLITE_OPEN_CREATE )
			my_file->file = sack_fsopenEx( 0, my_file->filename, "wb", _SH_DENYNO, my_vfs->mount );
		if( my_file->file )
		{
			sack_ioctl( my_file->file, SOSFSFIO_SET_BLOCKSIZE, 4096 );
#ifdef LOG_OPERATIONS
			lprintf( "Opened file: %s (vfs:%s) %p", zName, vfs->zName, my_file->file );
#endif
			//InitializeCriticalSec( &my_file->cs );
			return SQLITE_OK;
		}
		//lprintf( "failed..." );
	}
	else
	{
		my_file->file = sack_fsopen( 0, my_file->filename, "rb+", _SH_DENYNO );
		if( !my_file->file )
			my_file->file = sack_fsopen( 0, my_file->filename, "wb+", _SH_DENYNO );
		if( my_file->file )
		{
#ifdef LOG_OPERATIONS
			lprintf( "Opened file: %s (vfs:%s) %p", zName, vfs->zName, my_file->file );
#endif
			//InitializeCriticalSec( &my_file->cs );
			return SQLITE_OK;
		}
	}
#if defined( __GNUC__ )
	//__ANDROID__
#undef sack_fsopen
#undef sack_fsopenEx
#endif
	return SQLITE_CANTOPEN;
}

int xDelete(sqlite3_vfs*vfs, const char *zName, int syncDir)
{
	struct my_sqlite3_vfs *my_vfs = (struct my_sqlite3_vfs *)vfs;
#ifdef LOG_OPERATIONS
	lprintf( "delete on %s (%s:%p)", zName, vfs->zName, my_vfs->mount );
#endif
#ifdef UNICODE
		sack_unlinkEx( 0, (TEXTSTR)zName, my_vfs->mount );
#else
		sack_unlinkEx( 0, zName, my_vfs->mount );
#endif
	return SQLITE_OK;
}


#ifndef F_OK
# define F_OK 0
#endif
#ifndef R_OK
# define R_OK 4
#endif
#ifndef W_OK
# define W_OK 2
#endif
/*
** Query the file-system to see if the named file exists, is readable or
** is both readable and writable.
*/
static int xAccess(
  sqlite3_vfs *pVfs,
  const char *zPath,
  int flags,
  int *pResOut
){
	struct my_sqlite3_vfs *my_vfs = (struct my_sqlite3_vfs *)pVfs;
	int rc = 0;                         /* access() return code */
	//int eAccess = F_OK;             /* Second argument to access() */
#if 0
	assert( flags==SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK)*/
       || flags==SQLITE_ACCESS_READ         /* access(zPath, R_OK)*/
       || flags==SQLITE_ACCESS_READWRITE    /* access(zPath, R_OK|W_OK)*/
  );
#endif
#ifdef LOG_OPERATIONS
	//lprintf( "Open file: %s (vfs:%s)", zName, vfs->zName );
	//lprintf( "Access on %s %s", zPath, pVfs->zName );
#endif
	//if( flags==SQLITE_ACCESS_READWRITE ) eAccess = R_OK|W_OK;
	//if( flags==SQLITE_ACCESS_READ )			eAccess = R_OK;
	//if( flags & SQLITE_ACCESS_EXISTS )
	{
#ifdef UNICODE
		CTEXTSTR _zPath = DupCStr(zPath);
#       define zPath _zPath
#endif
		if( sack_existsEx( zPath, my_vfs->mount ) )
		{
#ifdef LOG_OPERATIONS
			//lprintf( "Open file: %s (vfs:%s)", zName, vfs->zName );
			lprintf( "Access on %s %s = file exists path", zPath, pVfs->zName );
#endif
			rc = 0;
		}
		else
		{
#ifdef LOG_OPERATIONS
			//lprintf( "Open file: %s (vfs:%s)", zName, vfs->zName );
			lprintf( "Access on %s %s = no path", zPath, pVfs->zName );
#endif
			rc = -1;
		}
	}

	//rc = sack_access(zPath, eAccess);
	*pResOut = (rc==0);
	return SQLITE_OK;
}

/*
** Argument zPath points to a nul-terminated string containing a file path.
** If zPath is an absolute path, then it is copied as is into the output
** buffer. Otherwise, if it is a relative path, then the equivalent full
** path is written to the output buffer.
**
** This function assumes that paths are UNIX style. Specifically, that:
**
**   1. Path components are separated by a '/'. and
**   2. Full paths begin with a '/' character.
*/
static int xFullPathname(
  sqlite3_vfs *pVfs,              /* VFS */
  const char *zPath,              /* Input path (possibly a relative path) */
  int nPathOut,                   /* Size of output buffer in bytes */
  char *zPathOut                  /* Pointer to output buffer */
){
#ifdef UNICODE
	TEXTSTR tmp = DupCStr( zPath );
	TEXTSTR path = ExpandPath( tmp );
	char *tmp2 = CStrDup( path );
	strncpy( zPathOut, tmp2, nPathOut );
	Release( tmp );
	Release( tmp2 );
#else
	TEXTSTR path = ExpandPath( zPath );
	StrCpyEx( zPathOut, path, nPathOut );
#endif
	//lprintf( "Expand %s = %s", zPath, path );
	Release( path );
	return SQLITE_OK;
}

void InitVFS( CTEXTSTR name, struct file_system_mounted_interface *mount )
{
	struct my_sqlite3_vfs *vfs;
	INDEX idx;
	LIST_FORALL( l.registered_vfs, idx, struct my_sqlite3_vfs *, vfs )
	{
#ifdef UNICODE
		char *tmp = CStrDup( name );
		if( stricmp( vfs->vfs.zName, tmp ) == 0 )
			return;
#else
		if( StrCaseCmp( vfs->vfs.zName, name ) == 0 )
			return;
#endif
	}
	//lprintf( "getting interface..." );
	{
		sqlite3_vfs *default_vfs = sqlite3_vfs_find(NULL);
		struct my_sqlite3_vfs *new_vfs;
		//lprintf( "Register sqlite vfs called %s", name );
		new_vfs = New( struct my_sqlite3_vfs );
		MemCpy( &new_vfs->vfs, default_vfs, sizeof( sqlite3_vfs ) );
		new_vfs->vfs.pAppData = 0;
		new_vfs->vfs.szOsFile = sizeof( struct my_sqlite3_vfs_file_data );
		new_vfs->vfs.zName = CStrDup( name );
		new_vfs->vfs.xOpen = xOpen;
		new_vfs->vfs.xDelete = xDelete;
		new_vfs->vfs.xAccess = xAccess;
		new_vfs->vfs.xFullPathname = xFullPathname;
		new_vfs->mount = mount;
		if( sqlite3_vfs_register( &new_vfs->vfs, 0 ) )
		{

			//lprintf( "error registering my interface" );
		}
		AddLink( &l.registered_vfs, new_vfs );
		//int sqlite3_vfs_unregister(sqlite3_vfs*);
	}
}


void errorLogCallback(void *pArg, int iErrCode, const char *zMsg){
	if( iErrCode == SQLITE_NOTICE_RECOVER_WAL ) {
#ifdef __STATIC_GLOBALS__
#  define sg (global_sqlstub_data)
		extern struct pssql_global global_sqlstub_data;
#else
#  define sg (*global_sqlstub_data)
		extern struct pssql_global *global_sqlstub_data;
#endif
		lprintf( "Sqlite3 Notice: wal recovered: generating checkpoint:%s", zMsg );
		// after open returns, generate an automatic wal_checkpoint.
		sg.flags.bAutoCheckpointRecover = 1;
	}
	else if( iErrCode == SQLITE_NOTICE_RECOVER_ROLLBACK ) {
		lprintf( "Sqlite3 Notice: journal rollback:%s", zMsg );
	}
	else if( iErrCode == SQLITE_ERROR )
		; // these will generally be logged by other error handling.
	else if( (iErrCode & SQLITE_CONSTRAINT) == SQLITE_CONSTRAINT )
		; // these will generally be logged by other error handling.
	else
		lprintf( "Sqlite3 Err: (%d) %s", iErrCode, zMsg);
}


static POINTER SimpleAllocate( int size )
{
	return HeapAllocateAligned( 0, size, 8 );
}
static POINTER SimpleReallocate( POINTER p, int size )
{
	return Reallocate( p, size );
}

static void SimpleFree( POINTER size )
{
	Release( size );
}

// this routine must return 'int' which is what sqlite expects for its interface
static int SimpleSize( POINTER p )
{
	return (int)SizeOfMemBlock( p );
}

static int SimpleRound( int size )
{
	return size;
}

static int SimpleInit( POINTER p )
{
	return TRUE;
}
static void SimpleShutdown( POINTER p )
{
}

static void DoInitVFS( void )
{
	if( l.registered_vfs )
		return;
	sqlite3_config( SQLITE_CONFIG_LOG, errorLogCallback, 0);

	{
#if SQLITE_VERSION_NUMBER >= 3006011
		static sqlite3_mem_methods mem_routines;
		if( mem_routines.pAppData == NULL )
		{
			sqlite3_config( SQLITE_CONFIG_GETMALLOC, &mem_routines );
			mem_routines.xMalloc = SimpleAllocate;
			mem_routines.xFree = SimpleFree;
			mem_routines.xRealloc = SimpleReallocate;
			mem_routines.xSize = SimpleSize;
			//mem_routines.xRoundUp = SimpleRound;
			//mem_routines.xInit = SimpleInit;
			//mem_routines.xShutdown = SimpleShutdown;
			mem_routines.pAppData = &mem_routines;
//#if SQLITE_VERSION_NUMBER >= 3006023
//			sqlite3_db_config( odbc->db, SQLITE_CONFIG_MALLOC, &mem_routines );
//#else
			sqlite3_config( SQLITE_CONFIG_MALLOC, &mem_routines );
//#endif
		}
#endif
	}


	InitVFS( "sack", NULL );
}


static POINTER CPROC GetSQLiteInterface( void )
{
	//sqlite3_enable_shared_cache( 1 );
	DoInitVFS();
	return &my_sqlite_interface;
}

static void CPROC DropSQLiteInterface( POINTER p )
{
}

PRIORITY_PRELOAD( RegisterSQLiteInterface, SQL_PRELOAD_PRIORITY-2 )
{
#ifndef __STATIC_GLOBALS__
	SimpleRegisterAndCreateGlobal( local_sqlite_interface );
#endif
	if( !GetInterface( "sqlite3" ) )
		RegisterInterface( "sqlite3", GetSQLiteInterface, DropSQLiteInterface );

}

#undef l

SQL_NAMESPACE_END
#endif
