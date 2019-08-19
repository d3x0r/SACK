
#include "common.h"

//----------------------------------------------------------------------
// INIT
//-----------------------------------------------------------------------

//-s EXPORTED_FUNCTIONS="['_initJSOX']" 
void initFS( void )  EMSCRIPTEN_KEEPALIVE;
void initFS( void ) 
{
	//printf( "INVOKE DEADSTART\n" );
	InvokeDeadstart();
	InitCommon();
	EM_ASM( (

	 //if( typeof indexedDB !== "undefined" ) {
         //var r = FS.mount(IDBFS, {}, '/home/web_user');
		  
        //persist changes

         //FS.syncfs(false,function (err) {
         //                 assert(!err);
         // });
		//}
		//console.log( "Log:", r );

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
				var tmp = Module._SRG_ID_Generator();
				mountName = UTF8ToString( tmp );
				Module._HeapReleaseEx( tmp, 0, 0 );
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
			this.mountName = mountName;
			this.fileName = fileName;
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
			dir(path,mask){
				var pi;
				var mi;
				if( path ){
					pi = allocate(intArrayFromString(path), 'i8', ALLOC_NORMAL);
				}else pi = 0;
				if( mask ){
					mi = allocate(intArrayFromString(mask), 'i8', ALLOC_NORMAL);
				}else mi = 0;
				var v;
				var r = Module.this_.objects[ v= Module._getDirectory( this.vol, pi, mi )];
				Module.this_.objects[v] = null;
				return r;
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
			Sqlite: function(db){
				if( ( this instanceof Volume ) ){
					if( this.mountName ) {
						db = "$sack@"+this.mountName+"$"+db;
					}
				}
				if( !(this instanceof Module.SACK.Volume.prototype.Sqlite) ) {
					console.log( "Retriggering as New(to get aswsociated methods )" ); 
					return new Module.SACK.Volume.prototype.Sqlite(db);
				}
				var si = db?allocate(intArrayFromString(db), 'i8', ALLOC_NORMAL):0;
				this.sql = Module._createSqlObject( si );
				Module._free( si );
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
	  Module.volMethods = volumeMethods;
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

		Object.defineProperties( ObjectStore.prototype, Object.getOwnPropertyDescriptors( objectMethods ));


		Module.defineFunction = function(cb) {
			return Module.this_.callbacks.push(cb);
		};

      Object.assign( SACK,{
			Volume : Volume,
			Sqlite : null,  // filled in InitSQL();
			ObjectStorage : ObjectStore,
			JSON : null,
			JSON6 : null,
			JSOX : null,  // filled with InitJSXO
			VESL : null,
			SaltyRNG : null, // filled in InitSRG();
		} );
		Module.SACK = SACK;
	) );
	InitSQL();
	InitSRG();
	InitJSON6();
	InitJSOX();
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
		if( vol->vol ) {
			//lprintf( "Sucess with volume, mount it as %s", mount);
			vol->fsMount = sack_mount_filesystem( mount, vol->fsInt = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-fs" )
					, 2000, (uintptr_t)vol->vol, TRUE );
		} else
			vol->fsMount = NULL;
	}
	return (uintptr_t)vol;
}

static EMSCRIPTEN_KEEPALIVE uintptr_t openSqlite( struct os_Vol*vol, char *filename );
uintptr_t openSqlite( struct os_Vol*vol, char *filename ) {
	char dbName[256];
 	snprintf( dbName, 256, "$sack@%s$%s", vol->mountName, filename );
	return (uintptr_t)createSqlObject( dbName );
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



int getDirectory( struct os_Vol *vol, char const*path, char const *mask ) EMSCRIPTEN_KEEPALIVE;
int getDirectory( struct os_Vol *vol, char const*path, char const *mask ) {
	struct find_cursor *fi;
	if( path )
		if( mask ) 
			fi = vol->fsInt->find_create_cursor( (uintptr_t)vol->vol, path, mask );
		else
			fi = vol->fsInt->find_create_cursor( (uintptr_t)vol->vol, path, "*" );
	else
		fi = vol->fsInt->find_create_cursor( (uintptr_t)vol->vol, ".", "*" );

	int pool = getLocal();
	int result = makeLocalArray(pool);
	int markObjects;
	int found;
	int n = 0;
	for( found = vol->fsInt->find_first( fi ); found; found = vol->fsInt->find_next( fi ) ) {
		char *name = vol->fsInt->find_get_name( fi );
		size_t length = vol->fsInt->find_get_size( fi );
		int entry = makeLocalObject(pool);
		//lprintf( "Get DirectoryEntry:%s", name );

		LSET( pool, entry, "name", makeLocalString( pool, name, strlen(name ) ) );
		if( length == ((size_t)-1) )
			LSETG( pool, entry, "folder", JS_VALUE_TRUE );
		else {
			LSETG( pool, entry, "folder", JS_VALUE_FALSE );
			LSET( pool, entry, "length", makeLocalNumber( pool, length ) );
		}
		LSETN( pool, result, n++, entry );
	} 
	vol->fsInt->find_close( fi );
	dropLocalAndSave( pool, result );
	return pool;
}

