#ifndef NO_FILEOP_ALIAS
#  define NO_FILEOP_ALIAS
#endif
#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>
#include <sack_vfs.h>

struct file_system_interface *fsi;
struct volume *volume;

void test1( void )
{
	void*file = fsi->open( (uintptr_t)volume, "apple", "unused" );
	if( file )
	{
		lprintf( "file size is now: %d", fsi->size( file ) );
		fsi->seek( file, fsi->size( file ), 0 );
		fsi->_write( file, "1", 1 );
		fsi->_close( file );
	}
}


void test2( void )
{
	char buf[256];
	int n;
	void* file;
	for( n = 0; n < 10000; n++ )
	{
		snprintf( buf, 256, "file.%d", n );
		file = fsi->open( (uintptr_t)volume, buf, "unused" );
		fsi->_close( file );
	}
}

PRIORITY_PRELOAD( Sack_VFS_Register, SQL_PRELOAD_PRIORITY )
{
	volume = sack_vfs_load_crypt_volume( ExpandPath( "*/sack.vault" ), 0, "alkj109ad908a0a8asdf908na90na80a98d098ahkljwerklja", "0000000-0000-0000-000000-000000" );
	//sack_vfs_load_volume( ExpandPath( "*/sack.vault" ) );

}

SaneWinMain( argc, argv )
{
	SetSystemLog( SYSLOG_FILE, stdout );
	{
		fsi = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME );
		sack_mount_filesystem( "test", fsi, 800, (uintptr_t)volume, 1 );
		if( fsi )
		{
			//sack_vfs_load_crypt_volume( ExpandPath( "*/sack.vault" ), "alkj109ad908a0a8asdf908na90na80a98d098ahkljwerklja", "0000000-0000-0000-000000-000000" );

			test1();
			test2();

		}
		else
		{
			lprintf( "failed to load virtual fils sytem" );
		}
	}
	return 0;
}
EndSaneWinMain()