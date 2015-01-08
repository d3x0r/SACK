#include <stdhdrs.h>
#include <sack_vfs.h>

#include "memory_dll_loader.h"

#define REPLACE_ME_1  "test.cvfs"
#define REPLACE_ME_2  "key1"
#define REPLACE_ME_3  "key2"

static struct vfs_runner_local
{
	struct file_system_interface *fsi;
}l;

LOGICAL CPROC LoadLibraryDependant( CTEXTSTR name )
{
	if( l.fsi->exists( name ) )
	{
		FILE *file = sack_fopenEx( 0, name, "rb", l.fsi );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( _8, sz );
				sack_fread( data, 1, sz, file );
				LoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
				Release( data );
			}
			sack_fclose( file );
			return TRUE;
		}
	}
	return FALSE;
}


SaneWinMain( argc, argv )
{
#ifdef STANDALONE_HEADER
	size_t sz = 0;
	POINTER memory = OpenSpace( NULL, argv[0], &sz );
	POINTER vfs_memory;
	struct volume *vol;
	vfs_memory = GetExtraData( memory );
	l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
	sack_set_default_filesystem_interface( l.fsi );
	SetExternalLoadLibrary( LoadLibraryDependant );
	vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((PTRSZVAL)vfs_memory-(PTRSZVAL)memory), REPLACE_ME_2, REPLACE_ME_3 );
	if( vol )
	{
		FILE *file = sack_fopenEx( 0, "0", "rb", l.fsi );
		size_t sz = sack_fsize( file );
		POINTER data = NewArray( _8, sz );
		sack_fread( data, 1, sz, file );
		sack_fclose( file );
		LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
		Release( data );
	}
#else
	struct volume *vol ;
	l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
	sack_set_default_filesystem_interface( l.fsi );
	SetExternalLoadLibrary( LoadLibraryDependant );
	vol = sack_vfs_load_crypt_volume( "test.scvfs", REPLACE_ME_2, REPLACE_ME_3 );
	if( vol )
	{
		FILE *file = sack_fopenEx( 0, "0", "rb", l.fsi );
		size_t sz = sack_fsize( file );
		POINTER data = NewArray( _8, sz );
		sack_fread( data, 1, sz, file );
		sack_fclose( file );
		LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
		Release( data );
	}
#endif
   return 0;
}
EndSaneWinMain()


