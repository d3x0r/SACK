#define NO_UNICODE_C
#include <stdhdrs.h>
#ifdef __WATCOMC__
// definition of SH_DENYNO
#include <share.h>
#endif
#include <procreg.h>
#include <deadstart.h>
#ifndef USE_SQLITE_INTERFACE
#define USE_SQLITE_INTERFACE
#endif
#define BUILDS_INTERFACE
#include "../../SQLlib/sqlstruc.h"
#include "sqlite3.h"

#ifndef UNICODE  // fix strings
//#define LOG_OPERATIONS
#endif

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
                                           , sqlite3_prepare16_v2
                                           , sqlite3_step
                                           , sqlite3_column_name
                                           , sqlite3_column_text
                                           , sqlite3_column_bytes
                                           , sqlite3_column_type
														 , sqlite3_column_count
                                           , sqlite3_config
															 , sqlite3_db_config
															 , InitVFS
};



struct my_file_data
{
	struct sqlite3_io_methods *pMethods; // must be first member to be file subclass...
	FILE *file;
	CRITICALSECTION cs;
	TEXTSTR filename;
	int locktype;
	struct file_system_mounted_interface *mount;
	LOGICAL temp;
};

struct my_sqlite3_vfs
{
	sqlite3_vfs vfs;
	struct file_system_mounted_interface *mount;
};

#define l local_sqlite_interface

struct local_data {
	int volume;
	struct karaway_interface* kwe;
	PLIST registered_vfs;
} local_sqlite_interface;

//typedef struct sqlite3_io_methods sqlite3_io_methods;


int xClose(sqlite3_file*file)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	if( my_file->file )
	{
		sack_fclose( my_file->file );
#ifdef LOG_OPERATIONS
		lprintf( "Close %s", my_file->filename );
#endif
	}
	if( my_file->temp )
	{
		sack_unlinkEx( 0, my_file->filename, my_file->mount );
#ifdef LOG_OPERATIONS
		lprintf( "unlink temp file : %s", my_file->filename );
#endif
	}
	return SQLITE_OK;
}

int xRead(sqlite3_file*file, void*buffer, int iAmt, sqlite3_int64 iOfst)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	size_t actual;
#ifdef LOG_OPERATIONS
	lprintf( "read %s %d  %d", my_file->filename, iAmt, iOfst );
#endif
	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );
	if( ( actual = sack_fread( buffer, 1, iAmt, my_file->file ) ) == iAmt )
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
	struct my_file_data *my_file = (struct my_file_data*)file;
	size_t actual;
#ifdef LOG_OPERATIONS
	lprintf( "Write %s %d at %d", my_file->filename, iAmt, iOfst );
	//LogBinary( buffer, iAmt );
#endif
	{
		size_t filesize = sack_fsize( my_file->file );
		if( filesize < iOfst )
		{
			static unsigned char *filler;
			if( !filler )
			{
				filler = NewArray( unsigned char, 512 );
				MemSet( filler, 0, 512 );
			}
			sack_fseek( my_file->file, 0, SEEK_END );
			while( filesize < iOfst )
			{
				if( ( iOfst - filesize ) >= 512 )
				{
					sack_fwrite( filler, 1, 512, my_file->file );
					filesize += 512;
				}
				else
				{
					sack_fwrite( filler, 1, ( iOfst - filesize ), my_file->file );
					filesize += ( iOfst - filesize );
				}
			}
		}
	}
	sack_fseek( my_file->file, (size_t)iOfst, SEEK_SET );

	if( iAmt == ( actual = sack_fwrite( buffer, 1, iAmt, my_file->file ) ) )
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
	struct my_file_data *my_file = (struct my_file_data*)file;
	sack_fseek( my_file->file, size, SEEK_SET );
	sack_ftruncate( my_file->file ); // works through file system interface...
	//SetFileLength( my_file->filename, (size_t)size );
	return SQLITE_OK;
}

int xSync(sqlite3_file*file, int flags)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
#ifdef LOG_OPERATIONS
	lprintf( "Sync on %s", my_file->filename );
#endif
	sack_fflush( my_file->file );
	/* noop */
	return SQLITE_OK;
}

int xFileSize(sqlite3_file*file, sqlite3_int64 *pSize)
{
	struct my_file_data *my_file = (struct my_file_data*)file;
	(*pSize) = sack_fsize( my_file->file );
#ifdef LOG_OPERATIONS
	lprintf( "Get File size result of %s %d", my_file->filename, (*pSize) );
#endif
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
#ifdef LOG_OPERATIONS
	lprintf( WIDE("file %s control op: %d %p"), my_file->filename, op, pArg );
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
			char *name = files[3];
			
			files[0] = sqlite3_mprintf( "%s", files[2] );
			//xOpen( my_file->
		}
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
	  return 0;
  }
  int xShmLock(sqlite3_file*file, int offset, int n, int flags)
  {
	  return 0;
  }

  void xShmBarrier(sqlite3_file*file)
  {
  }

  int xShmUnmap(sqlite3_file*file, int deleteFlag)
  {
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
	struct my_sqlite3_vfs *my_vfs = (struct my_sqlite3_vfs *)vfs;
	struct my_file_data *my_file = (struct my_file_data*)file;
	static int temp_id;
	char buf[32];
	file->pMethods = &my_methods;
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
	lprintf( "Open file: %s (vfs:%s)", zName, vfs->zName );
#endif
	/* also open the file... */
	{
		//int hResult = KWloadVolume( "core.volume" );
		//if( hResult < 0 )
		//	hResult = KWcreateVolume( "core.volume", 10*1024*1024 );
		//if( hResult == 0 )
		{
			//lprintf( WIDE("is it ok?") );
			my_file->filename = DupCStr( zName );
#if defined( __GNUC__ )
			//__ANDROID__
#define sack_fsopen(a,b,c,d) sack_fopen(a,b,c)
#define sack_fsopenEx(a,b,c,d,fsi) sack_fopenEx(a,b,c, fsi)
#endif
			if( my_vfs->mount )
			{
            //lprintf( "try on mount..%s .%p", my_file->filename, my_vfs->mount );
				my_file->file = sack_fsopenEx( 0, my_file->filename, WIDE("rb+"), _SH_DENYNO, my_vfs->mount );//KWfopen( zName );
				if( my_file->file )
				{
					InitializeCriticalSec( &my_file->cs );
					return SQLITE_OK;
				}
            //lprintf( "failed..." );
			}
			else
			{
				my_file->file = sack_fsopen( 0, my_file->filename, WIDE("rb+"), _SH_DENYNO );//KWfopen( zName );
				if( !my_file->file )
					my_file->file = sack_fsopen( 0, my_file->filename, WIDE("wb+"), _SH_DENYNO );//KWfopen( zName );
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
	int eAccess = F_OK;             /* Second argument to access() */
	/*
	assert( flags==SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) 
       || flags==SQLITE_ACCESS_READ         /* access(zPath, R_OK) 
       || flags==SQLITE_ACCESS_READWRITE    /* access(zPath, R_OK|W_OK) 
  );
  */
#ifdef LOG_OPERATIONS
	//lprintf( "Open file: %s (vfs:%s)", zName, vfs->zName );
	//lprintf( "Access on %s %s", zPath, pVfs->zName );
#endif
	if( flags==SQLITE_ACCESS_READWRITE ) eAccess = R_OK|W_OK;
	if( flags==SQLITE_ACCESS_READ )			eAccess = R_OK;
	//if( flags & SQLITE_ACCESS_EXISTS )
	{
		FILE *tmp;
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
	//lprintf( "Register sqlite vfs called %s", name );
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
	//lprintf( WIDE("getting interface...") );
	{
		sqlite3_vfs *default_vfs = sqlite3_vfs_find(NULL);
		struct my_sqlite3_vfs *new_vfs;
		new_vfs = New( struct my_sqlite3_vfs );
		MemCpy( &new_vfs->vfs, default_vfs, sizeof( sqlite3_vfs ) );
		new_vfs->vfs.pAppData = 0;
		new_vfs->vfs.szOsFile = sizeof( struct my_file_data );
		new_vfs->vfs.zName = CStrDup( name );
		new_vfs->vfs.xOpen = xOpen;
		new_vfs->vfs.xDelete = xDelete;
		new_vfs->vfs.xAccess = xAccess;
		new_vfs->vfs.xFullPathname = xFullPathname;
		new_vfs->mount = mount;
		if( sqlite3_vfs_register( &new_vfs->vfs, 0 ) )
		{

			//lprintf( WIDE("error registering my interface") );
		}
		AddLink( &l.registered_vfs, new_vfs );
		//int sqlite3_vfs_unregister(sqlite3_vfs*);
	}
}

static void DoInitVFS( void )
{
	InitVFS( WIDE("sack"), NULL );	
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
SQL_NAMESPACE_END
