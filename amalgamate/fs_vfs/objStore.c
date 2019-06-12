
#include "common.h"

//----------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------

//-s EXPORTED_FUNCTIONS="['_initJSOX']" 
void initFS( void )  EMSCRIPTEN_KEEPALIVE;
void initFS( void ) 
{
	EM_ASM( (
		
		var r = FS.mount(IDBFS, {}, '/home/web_user');
		  
        //persist changes
		FS.syncfs(false,function (err) {
                          assert(!err);
        });

		//console.log( "Log:", r );

		if( !Module.this_ ) {
			Module.this_ = {};
		}

		Module.this_.callbacks = Module.this_.callbacks || [];
		Module.this_.objects = Module.this_.objects || [undefined,false,true,null,-Infinity,Infinity,NaN];

		function TranslateText(s){ return s; }
		
		function Volume(mountName, fileName, version ) {
			var key1, key2;
			if( arguments.length == 0 ) {
				fileName = null;
				mountName = null;
				version = 0;
				key1 = null;
				key2 = null;
			} else if( arguments.length == 1 ) {
				fileName = mountName;
				mountName = NULL;
				version = 0;
				key1 = null;
				key2 = null;
			}
			else if( arguments.length == 2 ) {
				version = 0;
				key1 = null;
				key2 = null;
			}else {
				key1 = arguments[3];
				key2 = arguments[4];
			}
			if( !(this instanceof Volume) ) return new Volume( mountName,fileName,version,key1,key2);

			var s; // u8array
			var mni = mountName?allocate(s = intArrayFromString(mountName), 'i8', ALLOC_NORMAL):0;
			var fni = fileName?allocate(s = intArrayFromString(fileName), 'i8', ALLOC_NORMAL):0;
			var k1i = key1?allocate(s = intArrayFromString(key1), 'i8', ALLOC_NORMAL):0;
			var k2i = key2?allocate(s = intArrayFromString(key2), 'i8', ALLOC_NORMAL):0;
			if( typeof version !== "number" )
				version = 0;
			this.vol = Module._Volume(mni,fni,version,k1i,k2i);
			var result = Module.this_.objects[this.vol];
			if( arguments.length == 1 ) {
				fileName = mountName;
				mountName = null;
			}
		};

		Volume.mapFile =function(name) {
			console.log( "map File is not supported in emscripten" );
		};
		Volume.mkdir =function(name) {
			Module._sack_mkdir( name )
		};
		Volume.readAsString =function(name) {
			var intName = allocate(s = intArrayFromString(mountName), 'i8', ALLOC_NORMAL);
			var intstr = Module._readAsString( intName );
			Module._Release( intName );
			const string = UTF8ToString( intstr );
			Module._Release( intstr );
			return string;
		};


		const volumeMethods = {
			File(filename) {
				return new File( this.vol, filename );
			},
			dir(){
			},
			exists(fileOrPathName){
			},
			read(fileName){
			},
			readJSON(fileName, callback){
			},
			readJSON6(fileName, callback){
			},
			readJSOX(fileName, callback){
			},
			write(fileName,buffer){
			},
			Sqlite(database){
				return Module._openSqlite( this.vol, database );
			},
			rm(file){
			},
			mv(file,new_filename){
			},
			mkdir(path){
			},
			rekey(version,a,b){
			},
		};
			
		volumeMethods.delete = volumeMethods.rm;
		volumeMethods.unlink = volumeMethods.rm;
		volumeMethods.setOption = volumeMethods.so;
		volumeMethods.rename = volumeMethods.mv;
	        
		Object.defineProperties( Volume.prototype, Object.getOwnPropertyDescriptors( volumeMethods ));

		//-------------------------------------------------------------------


		function File(vol, name) {
			if( !(this instanceof File))return new File(vol,name);
			if( name ) {
				this.vol = vol;
				var s;
				var ni = allocate(s = intArrayFromString(name), 'i8', ALLOC_NORMAL);
				this.file = Module._File( vol, ni );
				Module._free( ni );
			}
		};

		File.SeekSet = 0;
		File.SeekCurrent = 1;
		File.SeekEnd = 2;

		const fileMethods = {
			read(size, position){
				if( position ) {

				}
			},
			readLine( position ){
			},
			write(value){
			},
			close(){
				Module._closeFile( this.file );
				FS.syncfs(false,function (err) {
					assert(!err);
		        });
			},
			writeLine( line, position ){

			},
			seek(pos,whence){
				if( typeof pos == "number" ){
					if( typeof whence == "number"){
						Module._seekFile(this.file, pos, whence );
					}else {
						Module._seekFile( this_file, pos, SEEK_SET );
					}
				}
			},
			trunc(){
				Module._truncateFile( this.file );
			},
            pos(){
				return Module._tellFile( this.file );
			},
		};
		Object.defineProperties( File.prototype, Object.getOwnPropertyDescriptors( fileMethods ));

		//-------------------------------------------------------------------

		function ObjectStore( fileName, version ) {
			
		};


		const objectMethods = {
			read( buf ) {
					},
					write() {
					},
					createIndex() {
					},
					put() {
					},
					get() {
					},
					delete() {
					}
		};

		Object.defineProperties( Object.getPrototypeOf( ObjectStore ), Object.getOwnPropertyDescriptors( objectMethods ));


		Module.defineFunction = function(cb) {
			return Module.this_.callbacks.push(cb);
		};

		Module.SACK = {
			Volume : Volume,
			Sqlite : null,  // filled in InitSQL();
			ObjectStorage : ObjectStore,
			JSON : null,
			JSON6 : null,
			JSOX : null,
			VESL : null,
			SaltyRNG : null, // filled in InitSRG();
		};
	) );
	InitSQL();
	InitSRG();
}


struct os_Vol {
	struct sack_vfs_fs_volume *vol;
	char *mountName;
	char *fileName;
	struct file_system_interface *fsInt;
	struct file_system_mounted_interface* fsMount;

	LOGICAL volNative;
};

struct os_File {
	struct os_Vol *vol;
	struct sack_vfs_fs_file *file;
	FILE *cfile;
	//Local<Object> sack_vfs_volume;
	PLIST buffers; // because these are passed as physical data buffers, don't release them willy-nilly
	char* buf;
	size_t size;
};

uintptr_t Volume( char *mount, char *filename, int version, char *key1, char *key2 ) EMSCRIPTEN_KEEPALIVE;
uintptr_t Volume( char *mount, char *filename, int version, char *key1, char *key2 ) 
{
	struct os_Vol *vol = New( struct os_Vol );

	if( !mount && !filename ) {
		vol->volNative = FALSE;
		vol->fsInt = sack_get_filesystem_interface( "native" );	
		vol->fsMount = sack_get_default_mount();
	}else if( mount && !filename ) {
		vol->volNative = FALSE;
		vol->fsMount = sack_get_mounted_filesystem( mount );
		vol->fsInt = sack_get_mounted_filesystem_interface( vol->fsMount );
		vol->vol = (struct sack_vfs_fs_volume*)sack_get_mounted_filesystem_instance( vol->fsMount );
		//lprintf( "open native mount" );
	} else {
		//lprintf( "sack_vfs_volume: %s %p %p", filename, key1, key2 );
		vol->fileName = StrDup( filename );
		vol->volNative = TRUE;
		vol->vol = sack_vfs_fs_load_crypt_volume( filename, version, key1, key2 );
		//lprintf( "VOL: %p for %s %d %p %p", vol, filename, version, key1, key2 );
		if( vol->vol )
			vol->fsMount = sack_mount_filesystem( mount, vol->fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-fs" )
					, 2000, (uintptr_t)vol->vol, TRUE );
		else
			vol->fsMount = NULL;
	}
	return (uintptr_t)vol;
}

static void resetVolume( struct os_Vol *v ) {
	/* volumes that got garbage collection might still have files in them... */
	(void)v;

}


uintptr_t readAsString( char *filename )   EMSCRIPTEN_KEEPALIVE;
uintptr_t readAsString( char *filename ) {
	FILE *file = sack_fopen( 0, filename, "rb" );
	{
		size_t len = sack_fsize( file );
		char *string = Allocate( len );
		sack_fread( string, 1, len, file );
		sack_fclose( file );
		return (uintptr_t)string;
	}
	return 0;
}

uintptr_t File( uintptr_t psvVol, char *name )  EMSCRIPTEN_KEEPALIVE;
uintptr_t File( uintptr_t psvVol, char *name ) {
	struct os_File *file = New( struct os_File);
	file->vol = (struct os_Vol*)psvVol;
	file->buf = NULL;
	
	if( file->vol->volNative ){
		if( !file->vol->vol ) return 0;
		file->file = sack_vfs_fs_openfile( file->vol->vol, name );
	}else {
		file->cfile = sack_fopenEx( 0, name, "rb+", file->vol->fsMount );
		if( !file->cfile )
			file->cfile = sack_fopenEx( 0, name, "wb", file->vol->fsMount );
	}
	return (uintptr_t)file;
}

void closeFile( uintptr_t psvFile )  EMSCRIPTEN_KEEPALIVE;
void closeFile( uintptr_t psvFile ){
	struct os_File *file = (struct os_File *)psvFile;
	lprintf( "Closing file..." );
	if( file->buf )
		Release( file->buf );
	lprintf( "calling real function %p %p", file, file->file );
	if( file->vol->volNative )
		sack_vfs_fs_close( file->file );
	else
		sack_fclose( file->cfile );
	lprintf( "calling reset..." );
	resetVolume( file->vol );
	lprintf( "Release file" );
	Release( file );
	lprintf( "and done?");

}

size_t tellFile( uintptr_t psvFile )  EMSCRIPTEN_KEEPALIVE;
size_t tellFile( uintptr_t psvFile ) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		return sack_vfs_fs_tell( file->file );
	else
		return sack_ftell( file->cfile );
}

void truncateFile(uintptr_t psvFile)  EMSCRIPTEN_KEEPALIVE;
void truncateFile(uintptr_t psvFile) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		sack_vfs_fs_truncate( file->file ); // sets end of file mark to current position.
	else
		sack_ftruncate( file->cfile );
}

void seekFile( uintptr_t psvFile, int pos, int whence )  EMSCRIPTEN_KEEPALIVE;
void seekFile( uintptr_t psvFile, int pos, int whence ) {
	struct os_File *file = (struct os_File *)psvFile;
	if( file->vol->volNative )
		sack_vfs_fs_seek( file->file, pos, whence );
	else
		sack_fseek( file->cfile, pos, whence );
}

			
uintptr_t openVolDb( struct os_Vol *vol, char *filename ) EMSCRIPTEN_KEEPALIVE;
uintptr_t openVolDb( struct os_Vol *vol, char *filename ) {
			char dbName[256];
 			snprintf( dbName, 256, "$sack@%s$%s", vol->mountName, filename );
			uintptr_t o = (uintptr_t)createSqlObject( dbName );
			return o;
}


