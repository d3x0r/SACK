
#ifndef SACK_VFS_DEFINED
/* Header multiple inclusion protection symbol. */
#define SACK_VFS_DEFINED

#ifdef SACK_VFS_SOURCE
#  define SACK_VFS_PROC EXPORT_METHOD
#else
#  define SACK_VFS_PROC IMPORT_METHOD
#endif

#ifdef __cplusplus
/* defined the file system partial namespace (under
   SACK_NAMESPACE probably)                         */
#define _SACK_VFS_NAMESPACE  namespace SACK_VFS {
/* Define the ending symbol for file system namespace. */
#define _SACK_VFS_NAMESPACE_END }
#else
#define _SACK_VFS_NAMESPACE 
#define _SACK_VFS_NAMESPACE_END
#endif
/* define the file system namespace end. */
#define SACK_VFS_NAMESPACE_END _SACK_VFS_NAMESPACE_END SACK_NAMESPACE_END 
/* define the file system namespace. */
#define SACK_VFS_NAMESPACE SACK_NAMESPACE _SACK_VFS_NAMESPACE 

SACK_VFS_NAMESPACE

#define SACK_VFS_FILESYSTEM_NAME WIDE("sack_shmem")

SACK_VFS_PROC struct volume * CPROC sack_vfs_load_volume( CTEXTSTR filepath );
SACK_VFS_PROC struct volume * CPROC sack_vfs_load_crypt_volume( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey );
SACK_VFS_PROC struct volume * CPROC sack_vfs_use_crypt_volume( POINTER filemem, size_t size, CTEXTSTR userkey, CTEXTSTR devkey );
SACK_VFS_PROC void            CPROC sack_vfs_unload_volume( struct volume * vol );
SACK_VFS_PROC void            CPROC sack_vfs_shrink_volume( struct volume * vol );
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_decrypt_volume( struct volume *vol );
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_encrypt_volume( struct volume *vol, CTEXTSTR key1, CTEXTSTR key2 );
SACK_VFS_PROC const char *    CPROC sack_vfs_get_signature( struct volume *vol );


SACK_VFS_PROC struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, CTEXTSTR filename );
SACK_VFS_PROC int CPROC sack_vfs_exists( PTRSZVAL psvInstance, const char * file );
SACK_VFS_PROC int CPROC sack_vfs_close( struct sack_vfs_file *file );
SACK_VFS_PROC size_t CPROC sack_vfs_tell( struct sack_vfs_file *file );
SACK_VFS_PROC size_t CPROC sack_vfs_size( struct sack_vfs_file *file );
SACK_VFS_PROC size_t CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence );
SACK_VFS_PROC size_t CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length );
SACK_VFS_PROC size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length );
SACK_VFS_PROC size_t CPROC sack_vfs_truncate( struct sack_vfs_file *file );
// psv should be struct volume *vol;
SACK_VFS_PROC void CPROC sack_vfs_unlink_file( PTRSZVAL psv, const char * filename );


SACK_VFS_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::SACK_VFS;
#endif
#endif
