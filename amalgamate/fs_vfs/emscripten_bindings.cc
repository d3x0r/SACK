#include <stdint.h>
#include <cmath> 
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;
#include "vfs_fs.h"

struct a {
};


//#define MODE NewStringType::kInternalized

static struct fs_volume * vfs_load( const char *file ) {
	struct fs_volume *vol = sack_vfs_fs_load_volume( file );
	return vol;
}

static FILE* vfs_open( struct fs_volume *vol, const char *filename, const char *mode ) {
	FILE* file = (FILE*)sack_vfs_fs_openfile( vol, filename );
	return file;
}

static const char * vfs_read( FILE*file, int length ) {
	char *buf= (char*)malloc(length);
	sack_vfs_fs_read( (struct sack_vfs_fs_file*)file, buf, length );
	return buf;
}

static int vfs_write( FILE*file, const char *data, int length ) {
	int wrote = sack_vfs_fs_write( (struct sack_vfs_fs_file*)file, data, length );	
	return wrote;
}

EMSCRIPTEN_BINDINGS(my_module) {
    function("load", vfs_load, allow_raw_pointers() );
}

