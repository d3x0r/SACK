#include <stdhdrs.h>
#include <deadstart.h>
#include <sack_vfs.h>

#include "memory_dll_loader.h"

#define REPLACE_ME_1  "test.cvfs"
#define REPLACE_ME_2  "key1"
#define REPLACE_ME_3  "key2"

static struct vfs_runner_local
{
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	struct file_system_mounted_interface *ram;
	int(WINAPI*entry_point)(HINSTANCE,HINSTANCE,LPSTR,int);
}l;

static LOGICAL CPROC LoadLibraryDependant( CTEXTSTR name )
{
	if( sack_exists( name ) )
	{
		FILE *file = sack_fopenEx( 0, name, "rb", l.rom );
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


PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
	TEXTSTR cmd = GetCommandLine();
	int argc;
	char **argv;
	ParseIntoArgs( cmd, &argc, &argv );
#ifdef STANDALONE_HEADER
	{
		size_t sz = 0;
		POINTER memory = OpenSpace( NULL, argv[0], &sz );
		POINTER vfs_memory;
		struct volume *vol;
		struct volume *vol2;
		SetSystemLog( SYSLOG_FILE, stderr ); 
		vfs_memory = GetExtraData( memory );
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		SetProgramName( "program" );
		vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((PTRSZVAL)vfs_memory-(PTRSZVAL)memory), REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( l.fsi, 100, (PTRSZVAL)vol, FALSE );
		vol2 = sack_vfs_load_crypt_volume( "external.vfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.ram = sack_mount_filesystem( l.fsi, 110, (PTRSZVAL)vol, TRUE );
		if( vol )
		{
			FILE *file = sack_fopenEx( 0, "0", "rb", l.rom );
			size_t sz = sack_fsize( file );
			POINTER data = NewArray( _8, sz );
			sack_fread( data, 1, sz, file );
			sack_fclose( file );
			l.entry_point = (int(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))
				   LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
			Release( data );
		}
	}
#else
	{
		struct volume *vol;
		struct volume *vol2;
		TEXTSTR cmd = GetCommandLine();
		int argc;
		char **argv;
		ParseIntoArgs( cmd, &argc, &argv );
		SetSystemLog( SYSLOG_FILE, stderr ); 
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		SetProgramName( "program" );
		vol = sack_vfs_load_crypt_volume( "test.scvfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( l.fsi, 100, (PTRSZVAL)vol, TRUE );
		vol2 = sack_vfs_load_crypt_volume( "external.vfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.ram = sack_mount_filesystem( l.fsi, 110, (PTRSZVAL)vol, TRUE );
		if( vol )
		{
			FILE *file = sack_fopenEx( 0, "0", "rb", l.rom );
			size_t sz = sack_fsize( file );
			POINTER data = NewArray( _8, sz );

			sack_fread( data, 1, sz, file );
			sack_fclose( file );
			l.entry_point = (int(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))
				   LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
			Release( data );
		}
	}
#endif
   //return 0;
}


SaneWinMain(argc,argv)
{
	return l.entry_point( 0, 0, GetCommandLine(), 1 );
}
EndSaneWinMain()


