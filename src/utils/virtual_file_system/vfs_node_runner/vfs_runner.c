#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>
#include <sack_vfs.h>


#define REPLACE_ME_1  "test.cvfs"
#define REPLACE_ME_2a  0/*"key1"*/
#define REPLACE_ME_2b  NULL/*"key1"*/
#define REPLACE_ME_3   NULL/*"key2"*/

static struct vfs_runner_local
{
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	struct file_system_mounted_interface *ram;

}l;


static TEXTSTR CPROC LoadLibraryDependant( CTEXTSTR name )
{
	if( sack_existsEx( name, l.rom ) )
	{
		FILE *file;
#ifdef DEBUG_LIBRARY_LOADING

		lprintf( "%s exists...", name );
#endif
		file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			CTEXTSTR path = ExpandPath( "*/tmp" );
			TEXTCHAR* tmpnam = NewArray( TEXTCHAR, 256 );
			size_t sz = sack_fsize( file );
			FILE *tmp;
#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "library is %d %s", sz, name );
#endif
			MakePath( path );
			snprintf( tmpnam, 256, "%s/%s", path, name );
			tmp = sack_fopenEx( 0, tmpnam, "wb", sack_get_default_mount() );
#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "Loading %s(%p)", tmpnam, tmp );
#endif
			if( sz && tmp )
			{
				size_t written, read ;
				POINTER data = NewArray( uint8_t, sz );
				read = sack_fread( data, sz, 1, file );
				written = sack_fwrite( data, sz, 1, tmp );
				sack_fclose( tmp );
				Release( data );
			}
			sack_fclose( file );
			return tmpnam;
		}
	}
	return NULL;
}

PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
#if _WIN32 
	TEXTSTR cmd = GetCommandLine();
	int argc;
	char **argv;

	ParseIntoArgs( cmd, &argc, &argv );

	{
		size_t sz = 0;
		POINTER memory = OpenSpace( NULL, argv[0], &sz );
		POINTER vfs_memory;
		struct volume *vol;
		struct volume *vol2;
		SetSystemLog( SYSLOG_FILE, stderr ); 
		vfs_memory = memory;
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((uintptr_t)vfs_memory-(uintptr_t)memory), REPLACE_ME_2a, REPLACE_ME_2b, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, FALSE );
		vol2 = sack_vfs_load_crypt_volume( "external.vfs", REPLACE_ME_2a, REPLACE_ME_2b, REPLACE_ME_3 );
		l.ram = sack_mount_filesystem( "extra", l.fsi, 110, (uintptr_t)vol, TRUE );

		if( vol )
		{
			
			TEXTSTR tmpPathNode = LoadLibraryDependant( "node.exe" );
			TEXTSTR tmpPathVfs = LoadLibraryDependant( "sack_vfs.node" );
			static TEXTCHAR cmd[4096];
			TEXTCHAR tmp[256];
			int ofs;
	if( !pathchr( argv[0] ) ) {
		snprintf( tmp, 256, "./%s", argv[0] );
		argv[0] = ExpandPath( tmp );
	}	
			for( ofs = 0; tmpPathNode[ofs]; ofs++ ) if( tmpPathNode[ofs] == '/' ) tmpPathNode[ofs] = '\\';
			for( ofs = 0; tmpPathVfs[ofs]; ofs++ ) if( tmpPathVfs[ofs] == '/' ) tmpPathVfs[ofs] = '\\';
			//ofs = snprintf( cmd, 4096, "\"%s\" -r \"%s\"", tmpPathNode, tmpPathVfs );
			ofs = snprintf( cmd, 4096, "\"%s\" -r \"%s\" -e \"var vfs= modules.find( name=>name.endsWith( '.node' ) ).exports;vfs.InitFS(\'%s\');", tmpPathNode, tmpPathVfs, argv[0] );
			for( int n = 1; n < argc; n++ ) {
				ofs += snprintf( cmd+ofs, 4096-ofs, "require(\'%s\');", argv[n] );
			}
			ofs += snprintf( cmd+ofs, 4096-ofs, "\"" );
			ofs += snprintf( cmd+ofs, 4096-ofs, " \"%s\"", argv[0] );
			System( cmd, NULL, 0 );
			//lprintf( "And the command completed?  Relaunch?" );
		}
	}
#endif
   //return 0;
}

#if __LINUX__
int main( int argc, char **argv, char **envp )
{
	return 0;
}
#endif
#if _WIN32
SaneWinMain(argc,argv)
{
	return 0;
}
EndSaneWinMain()
#endif

