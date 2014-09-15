#include <stdhdrs.h>
#include <procreg.h>
#include <deadstart.h>
#ifndef USE_SQLITE_INTERFACE
#define USE_SQLITE_INTERFACE
#endif
#define BUILDS_INTERFACE
#include "../SQLlib/sqlstruc.h"
#include "3.7.16.2/sqlite3.h"

static void set_open_filesystem_interface( struct file_system_interface *fsi );

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
														 , sqlite3_close_v2
                                           , sqlite3_prepare_v2
                                           , sqlite3_step
                                           , sqlite3_column_name
                                           , sqlite3_column_text
                                           , sqlite3_column_bytes
                                           , sqlite3_column_type
														 , sqlite3_column_count
                                           , sqlite3_config
															 , sqlite3_db_config
															 , set_open_filesystem_interface
};



struct my_file_data
{
	struct sqlite3_io_methods *pMethods; // must be first member to be file subclass...
	FILE *file;
	CRITICALSECTION cs;
	TEXTSTR filename;
	int locktype;
};

#define l local_sqlite_interface

struct local_data {
	int volume;
	struct karaway_interface* kwe;
	struct file_system_interface *next_fsi;
} local_sqlite_interface;

//typedef struct sqlite3_io_methods sqlite3_io_methods;

void set_open_filesystem_interface( struct file_system_interface *fsi )
{
	l.next_fsi = fsi;
}


int xClose(sqlite3_file*file)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	if( my_file->file )
		sack_fclose( my_file->file );
	//lprintf( "Close %s", my_file->filename );
	return SQLITE_OK;
}

int xRead(sqlite3_file*file, void*buffer, int iAmt, sqlite3_int64 iOfst)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	size_t actual;
	//lprintf( "read %s %d  %d", my_file->filename, iAmt, iOfst );
	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );
	if( ( actual = sack_fread( buffer, 1, iAmt, my_file->file ) ) == iAmt )
		return SQLITE_OK;
	//lprintf( "Errono : %d", errno );
	MemSet( ((char*)buffer)+actual, 0, iAmt - actual );
	return SQLITE_IOERR_SHORT_READ;

}

int xWrite(sqlite3_file*file, const void*buffer, int iAmt, sqlite3_int64 iOfst)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	size_t actual;
	//lprintf( "Write %s %d  %d", my_file->filename, iAmt, iOfst );

	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );
	if( iAmt == ( actual = sack_fwrite( buffer, 1, iAmt, my_file->file ) ) )
		return SQLITE_OK;
	return SQLITE_IOERR_WRITE;
}

int xTruncate(sqlite3_file*file, sqlite3_int64 size)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	SetFileLength( my_file->filename, (size_t)size );
	return SQLITE_OK;
}

int xSync(sqlite3_file*file, int flags)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	//lprintf( "Sync on %s", my_file->filename );
	sack_fflush( my_file->file );
	/* noop */
	return SQLITE_OK;
}

int xFileSize(sqlite3_file*file, sqlite3_int64 *pSize)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	size_t here = sack_ftell( my_file->file );
	size_t length;
	/* !!! Missing Method!!! */
	sack_fseek( my_file->file, 0, SEEK_END );
	length = sack_ftell( my_file->file );
	sack_fseek( my_file->file, here, SEEK_SET );
	(*pSize) = length;
	return SQLITE_OK;

}

int xLock(sqlite3_file*file, int locktype)
{
	return SQLITE_OK;
#if 0
	struct my_file_data *my_file = (struct my_file_data*)file;
	switch( locktype )
	{
	case SQLITE_LOCK_NONE:
		//return SQLITE_OK;
	case SQLITE_LOCK_SHARED:
	case SQLITE_LOCK_RESERVED:
	case SQLITE_LOCK_PENDING:
	case SQLITE_LOCK_EXCLUSIVE:
		EnterCriticalSec( &my_file->cs );
		my_file->locktype = locktype;
		return SQLITE_OK;
		break;
	}
#endif
}

int xUnlock(sqlite3_file*file, int locktype)
{
	return SQLITE_OK;
#if 0
	struct my_file_data *my_file = (struct my_file_data*)file;
	switch( locktype )
	{
	case SQLITE_LOCK_NONE:
		//break;
	case SQLITE_LOCK_SHARED:
	case SQLITE_LOCK_RESERVED:
	case SQLITE_LOCK_PENDING:
	case SQLITE_LOCK_EXCLUSIVE:
		//my_file->error = SQLITE_LOCK_EXCLUSIVE;
		LeaveCriticalSec( &my_file->cs );
		my_file->locktype = SQLITE_LOCK_NONE;
		break;
	}
	return SQLITE_OK;
#endif
}


int xCheckReservedLock(sqlite3_file*file, int *pResOut)
{
	
	*pResOut = 0;
	return SQLITE_OK;
#if 0
	struct my_file_data *my_file = (struct my_file_data*)file;
	if( EnterCriticalSecNoWait( &my_file->cs, NULL ) )
	{
		LeaveCriticalSec( &my_file->cs );
		return SQLITE_LOCK_NONE;
	}
	return my_file->locktype;
#endif
}

int xFileControl(sqlite3_file*file, int op, void *pArg)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	//lprintf( WIDE("file control op: %d %p"), op, pArg );
	switch( op )
	{
	case SQLITE_FCNTL_LOCKSTATE:
	case SQLITE_GET_LOCKPROXYFILE:
	case SQLITE_SET_LOCKPROXYFILE:
	case SQLITE_LAST_ERRNO:
	case SQLITE_FCNTL_SIZE_HINT:
	case SQLITE_FCNTL_CHUNK_SIZE:
	case SQLITE_FCNTL_FILE_POINTER:
	case SQLITE_FCNTL_SYNC_OMITTED:
	case SQLITE_FCNTL_WIN32_AV_RETRY:
	case SQLITE_FCNTL_PERSIST_WAL:
	case SQLITE_FCNTL_OVERWRITE:
	case SQLITE_FCNTL_VFSNAME:
	case SQLITE_FCNTL_POWERSAFE_OVERWRITE:
	case SQLITE_FCNTL_PRAGMA:
	case SQLITE_FCNTL_BUSYHANDLER:
	case SQLITE_FCNTL_TEMPFILENAME:
		break;
	}
	return SQLITE_OK;
}

int xSectorSize(sqlite3_file*file)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	return 512;
}

int xDeviceCharacteristics(sqlite3_file*file)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	return SQLITE_IOCAP_ATOMIC|SQLITE_IOCAP_SAFE_APPEND|SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN|SQLITE_IOCAP_POWERSAFE_OVERWRITE;
}


  int xShmMap(sqlite3_file*file, int iPg, int pgsz, int a, void volatile**b)
  {

  }
  int xShmLock(sqlite3_file*file, int offset, int n, int flags)
  {
  }

  void xShmBarrier(sqlite3_file*file)
  {
  }

  int xShmUnmap(sqlite3_file*file, int deleteFlag)
  {
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
													/*, sizeof( struct my_file_data )*/
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
													//, xShmMap
													//, xShmLock
													//, xShmBarrier
													//, xShmUnmap
};

int xOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file*file,
			 int flags, int *pOutFlags)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	file->pMethods = &my_methods;
	if( zName == NULL )
		zName = "sql.tmp";
	//lprintf( "OPen file: %s", zName );
	/* also open the file... */
	{
		//int hResult = KWloadVolume( "core.volume" );
		//if( hResult < 0 )
		//	hResult = KWcreateVolume( "core.volume", 10*1024*1024 );
		//if( hResult == 0 )
		{
			//lprintf( WIDE("is it ok?") );
			my_file->filename = StrDup( zName );
#if defined( __GNUC__ )
			//__ANDROID__
#define sack_fsopen(a,b,c,d) sack_fopen(a,b,c)
#define sack_fsopenEx(a,b,c,d,fsi) sack_fopenEx(a,b,c, fsi)
#endif
			if( l.next_fsi )
			{
				my_file->file = sack_fsopenEx( 0, zName, "rb+", _SH_DENYNO, l.next_fsi );//KWfopen( zName );
				l.next_fsi = NULL; // clear this, next open neeeds a new one.
				if( my_file->file )
				{
					InitializeCriticalSec( &my_file->cs );
					return SQLITE_OK;
				}
			}
			else
			{
				my_file->file = sack_fsopen( 0, zName, "rb+", _SH_DENYNO );//KWfopen( zName );
				if( !my_file->file )
					my_file->file = sack_fsopen( 0, zName, "wb+", _SH_DENYNO );//KWfopen( zName );
				if( my_file->file )
				{
					InitializeCriticalSec( &my_file->cs );
					return SQLITE_OK;
				}
			}
		}
		//else
		//  lprintf( WIDE("hResult = %08x"), hResult );
	}
	return SQLITE_ERROR;
}

int xDelete(sqlite3_vfs*vfs, const char *zName, int syncDir)
{
	//lprintf( "Never implemented delete on %s", zName );
	sack_unlink( 0, zName );
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
  int rc;                         /* access() return code */
  int eAccess = F_OK;             /* Second argument to access() */
  /*
  assert( flags==SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) 
       || flags==SQLITE_ACCESS_READ         /* access(zPath, R_OK) 
       || flags==SQLITE_ACCESS_READWRITE    /* access(zPath, R_OK|W_OK) 
  );
  */
  //lprintf( "Access on %s", zPath );
  if( flags==SQLITE_ACCESS_READWRITE ) eAccess = R_OK|W_OK;
  if( flags==SQLITE_ACCESS_READ )      eAccess = R_OK;

  rc = access(zPath, eAccess);
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
	TEXTSTR path = ExpandPath( zPath );
	//lprintf( "Expand %s = %s", zPath, path );
	StrCpyEx( zPathOut, path, nPathOut );
	Release( path );
	return SQLITE_OK;
}
static void DoInitVFS( void )
{
	sqlite3_vfs *default_vfs = sqlite3_vfs_find(NULL);
	//lprintf( WIDE("getting interface...") );
	{
		static sqlite3_vfs new_vfs;
		MemCpy( &new_vfs, default_vfs, sizeof( new_vfs ) );
		new_vfs.pAppData = 0;
		new_vfs.szOsFile = sizeof( struct my_file_data );
		new_vfs.zName = "sack";
		new_vfs.xOpen = xOpen;
		new_vfs.xDelete = xDelete;
		new_vfs.xAccess = xAccess;
		new_vfs.xFullPathname = xFullPathname;
		if( sqlite3_vfs_register( &new_vfs, 0 ) )
		{

			//lprintf( WIDE("error registering my interface") );
		}
		//int sqlite3_vfs_unregister(sqlite3_vfs*);
	}
}


static POINTER CPROC GetSQLiteInterface( void )
{
	//RealSQLiteInterface._global_font_data = GetGlobalFonts();
   //sqlite3_enable_shared_cache( 1 );
	DoInitVFS();
	return &my_sqlite_interface;
}

static void CPROC DropSQLiteInterface( POINTER p )
{
}

PRIORITY_PRELOAD( RegisterSQLiteInterface, SQL_PRELOAD_PRIORITY-2 )
{
	RegisterInterface( WIDE("sqlite3"), GetSQLiteInterface, DropSQLiteInterface );

}

#ifdef __WATCOMC__
// watcom requires at least one export
PUBLIC( void, AtLeastOneExport )( void )
{
}
#endif
