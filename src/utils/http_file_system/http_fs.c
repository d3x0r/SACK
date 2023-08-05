#include <stdhdrs.h>

#include <filesys.h>

#include <emscirptem.h>
#include <emscripten/html.h>

struct http_file {
   char *name;
	PTEXT data;
   size_t fpi;
};


struct http_volume {
   char *weborigin;
	char *webroot;
	LOGICAL https;
   LOGICAL readonly;
};

static struct http_volume *createVolume( const char *origin, char char *root, LOGICAL https ) {
	struct http_volume *vol = New( struct http_volume );
	vol->weborigin = StrDup( origin );
	vol->webroot->StrDup( root );
	vol->https = https;
   vol->readonly = TRUE;
   return vol;
}


fetch

void* CPROC http_fs_open(uintptr_t psvInstance, const char *name, const char *mode){
	struct http_volume *volume = (struct http_volume *)psvInstance;

	char realname[256];
   HTTPState *result;
	snprintf( "%s://%s/%s/%s", volume->https?"HTTPS":"HTTP", volume->weborigin, volume->webroot, name );
	if( vol->https )
		result = GetHttpsQuery( NULL, SegCreateFromText( realname ), NULL );
	else
		result = GetHttpQuery( NULL, SegCreateFromText( realname ) );
	if( result ) {
		switch( GetHttpResponseCode( result ) ) {
		case 200:
			{
				PTEXT filedata = GetHttpContent( result );
				struct http_file *file = New( struct http_file * );
				file->name = name;
            Hold( filedata );
				file->data = filedata;
            file->fpi = 0;
				DestroyHttpState( result );
            return file;
			}
		defualt:
			lprintf( "Failed to get file from server:%s", name );
			DestroyHttpState( result );
			return NULL;
		}
	}
}                                                  //filename

int CPROC http_fs__close(void *pFile){
	struct file_data *file = (struct file_data *)pFile;
	LineRelease( file->data );
	Release( file->name );
   Release( file );
}                                                 //file *

size_t CPROC http_fs__read(void *file,void *buf, size_t len){
   struct file_data *file = (struct file_data *)pFile;
	if( file ) {

		return file->data->data.size;
	}
}                    //file *, buffer, length to read)

size_t CPROC http_fs__write(void*,const void *, size_t){
   struct file_data *file = (struct file_data *)pFile;
}                    //file *, buffer, length to write)

size_t CPROC http_fs_seek( void *, size_t, int whence){
   struct file_data *file = (struct file_data *)pFile;
}

void  CPROC http_fs_truncate( void *){
   struct file_data *file = (struct file_data *)pFile;
}

int CPROC http_fs__unlink( uintptr_t psvInstance, const char *){
   struct file_data *file = (struct file_data *)pFile;
}

size_t CPROC http_fs_size( void *){
	struct file_data *file = (struct file_data *)pFile;
   if( file )
		return file->data->data.size;
   return 0;
} // get file size

size_t CPROC http_fs_tell( void *){
   struct file_data *file = (struct file_data *)pFile;
} // get file current position

int CPROC http_fs_flush (void *kp){
   struct file_data *file = (struct file_data *)pFile;
}

int CPROC http_fs_exists( uintptr_t psvInstance, const char *file ){
   struct file_data *file = (struct file_data *)pFile;
}

LOGICAL CPROC*copy_write_buffer(void ){
   struct file_data *file = (struct file_data *)pFile;
}

struct find_cursor *(CPROC http_fs_find_create_cursor ( uintptr_t psvInstance, const char *root, const char *filemask ){
   struct file_data *file = (struct file_data *)pFile;
}

int CPROC http_fs_find_first( struct find_cursor *cursor ){
   struct file_data *file = (struct file_data *)pFile;
}

int CPROC http_fs_find_close( struct find_cursor *cursor ){
   struct file_data *file = (struct file_data *)pFile;
}

int CPROC http_fs_find_next( struct find_cursor *cursor ){
   struct file_data *file = (struct file_data *)pFile;
}

char * CPROC http_fs_find_get_name( struct find_cursor *cursor ){
   struct file_data *file = (struct file_data *)pFile;
}

size_t CPROC http_fs_find_get_size( struct find_cursor *cursor ){
}

LOGICAL CPROC http_fs_find_is_directory( struct find_cursor *cursor ){
}

uint64_t( CPROC http_fs_find_get_ctime (struct find_cursor *cursor){
}

uint64_t( CPROC http_fs_find_get_wtime (struct find_cursor *cursor){
}



LOGICAL CPROC http_fs_is_directory( uintptr_t psvInstance, const char *cursor ){
}

LOGICAL CPROC http_fs_rename ( uintptr_t psvInstance, const char *original_name, const char *new_name ){
}

uintptr_t CPROC http_fs_ioctl( uintptr_t psvInstance, uintptr_t opCode, va_list args ){
}

uintptr_t CPROC http_fs_fs_ioctl(uintptr_t psvInstance, uintptr_t opCode, va_list args){
}




/* Extended external file system interface to be able to use external file systems */
static struct file_system_interface http_fs_interface = {
	http_fs_open,
	int CPROC http_fs__close,
	size_t CPROC http_fs__read,
	size_t CPROC http_fs__write,
	size_t CPROC http_fs_seek,
	void  CPROC http_fs_truncate,
	int CPROC http_fs__unlink,
	size_t CPROC http_fs_size,
	size_t CPROC http_fs_tell,
	int CPROC http_fs_flush,
	int CPROC http_fs_exists,
	LOGICAL CPROC*copy_write_buffer,
	struct find_cursor * CPROC http_fs_find_create_cursor,
	int CPROC http_fs_find_first( struct find_cursor *cursor ),
	int CPROC http_fs_find_close( struct find_cursor *cursor ),
	int CPROC http_fs_find_next( struct find_cursor *cursor ),
	char * CPROC http_fs_find_get_name( struct find_cursor *cursor ),
	size_t CPROC http_fs_find_get_size( struct find_cursor *cursor ),
	LOGICAL CPROC http_fs_find_is_directory( struct find_cursor *cursor ),
	LOGICAL CPROC http_fs_is_directory( uintptr_t psvInstance, const char *cursor ),
	LOGICAL CPROC http_fs_rename ( uintptr_t psvInstance, const char *original_name, const char *new_name ),
	uintptr_t CPROC http_fs_ioctl( uintptr_t psvInstance, uintptr_t opCode, va_list args ),
	uintptr_t CPROC http_fs_fs_ioctl(uintptr_t psvInstance, uintptr_t opCode, va_list args),
	uint64_t( CPROC http_fs_find_get_ctime (struct find_cursor *cursor),
	uint64_t( CPROC http_fs_find_get_wtime (struct find_cursor *cursor)
};


PRIORITY_PRELOAD( Sack_HTTP_FS_Register, CONFIG_SCRIPT_PRELOAD_PRIORITY - 2 )
{
	sack_register_filesystem_interface( "http-fs", &sack_vfs_fs_fsi );
}

PRIORITY_PRELOAD( Sack_HTTP_FS_RegisterDefaultFilesystem, SQL_PRELOAD_PRIORITY + 1 ) {
	if( SACK_GetProfileInt( "SACK/HTTP FS", "Mount FS VFS", 0 ) ) {
		struct http_volume *vol;
		TEXTCHAR volOrigin[256];
		TEXTCHAR volPath[256];
		TEXTCHAR volfile[256];
      LOGICAL https;
		TEXTSTR tmp;
		SACK_GetProfileString( "SACK/HTTP FS", "Origin", "127.0.0.1:8087", volOrigin, 256 );
		SACK_GetProfileString( "SACK/HTTP FS", "Root", "/assets", volRoot, 256 );
		https = SACK_GetProfileInt( "SACK/HTTP FS", "HTTPS", 1 );
      vol = createVolume( volOrigin, volRoot, https );
		tmp = ExpandPath( volfile );
		Deallocate( TEXTSTR, tmp );
		sack_mount_filesystem( "http-fs", sack_get_filesystem_interface( DEFAULT_VFS_NAME )
		                     , 750, (uintptr_t)vol, TRUE );
	}
}


