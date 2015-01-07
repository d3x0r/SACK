
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
SACK_VFS_PROC void            CPROC sack_vfs_unload_volume( struct volume * vol );

SACK_VFS_PROC struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, CTEXTSTR filename );

SACK_VFS_NAMESPACE_END

#endif