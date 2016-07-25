#include <stdhdrs.h>
#include <deadstart.h>
#include <sack_vfs.h>
#include <sack_system.h>
#include <configscript.h>

#include "memory_dll_loader.h"

#define REPLACE_ME_1  "test.cvfs"
#define REPLACE_ME_2  (argc>2)?argv[2]:NULL
#define REPLACE_ME_3  (argc>3)?argv[3]:NULL

static struct vfs_runner_local
{
	char *first_file;
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	char *target_path;
	char *prior_output_path;  // prevent remaking directories for EVERY file...
	PCONFIG_HANDLER pch;
}l;

static PTRSZVAL CPROC SetDefaultPath( PTRSZVAL psv, arg_list args ) {
	PARAM( args, CTEXTSTR, path );
	l.target_path = ExpandPath( path );
	return psv;
}

static void InitConfigHandler( void ) {
	l.pch = CreateConfigurationHandler();
	AddConfigurationMethod( l.pch, "default path=%m", SetDefaultPath );
}


static LOGICAL CPROC LoadLibraryDependant( CTEXTSTR name )
{
	if( StrCmp( name, ".app.config" ) == 0 ) {
		FILE *file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( _8, sz );
				sack_fread( data, 1, sz, file );
				ProcessConfigurationInput( l.pch, data, sz, 0 );
			}
		}
	} else {
		FILE *file;
		file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( _8, sz );
				sack_fread( data, 1, sz, file );

				/* this is where the file should be output */
				{
					char target[256];
					char *tmp;
					snprintf( target, 256, "%s/%s", l.target_path, name );
					tmp = (char*)pathrchr( target );
					if( tmp ) {
						tmp[0] = 0;
						if( !l.prior_output_path 
							|| StrCmp( l.prior_output_path, target ) ) {
							l.prior_output_path = strdup( target );
							MakePath( target );
						}
						tmp[0] = '/';
					}
					{
						FILE *out;
						out = sack_fopenEx( 0, target, "wb", sack_get_default_mount() );
 						if( out ) {
							if( !l.first_file )
								l.first_file = strdup( target );
							sack_fwrite( data, 1, sz, out );
							sack_fclose( out );
						}
					}
				}

				//LoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
				Release( data );
			}
			sack_fclose( file );
			return TRUE;
		}
	}

	return FALSE;
}

static void CPROC ShowFile( PTRSZVAL psv, CTEXTSTR file, int flags )
{
	LoadLibraryDependant( file );
}


PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
	struct volume *vol;
#if _WIN32 
	wchar_t * wcmd = GetCommandLineW();
	char *cmd = WcharConvert( wcmd );
	int argc;
	char **argv;
	ParseIntoArgs( cmd, &argc, &argv );
#else //if defined( __LINUX__ )
	static char buf[4096], *pb;
	int n, m;
	n = readlink( "/proc/self/cmdline", buf, 4096 );
	argc = 0;
	for( m = 0; m < n; m++ )
		if( !buf[m] )
			argc++;
	argv = NewArray( char*, argc + 1 );
	argv[0] = buf;
	for( m = 0; m < n; m++ ) {
		if( !buf[m] ) {
			argc++;
			argv[argc] = buf + m + 1;
		}
	}
	argv[argc] = NULL;
#endif
	InitConfigHandler();


#ifdef STANDALONE_HEADER
	{
		size_t sz = 0;
		POINTER memory = OpenSpace( NULL, argv[0], &sz );
		POINTER vfs_memory;
		if( argc > 1 ) {
			SetCurrentPath( argv[1] );
		}
		SetSystemLog( SYSLOG_FILE, stderr ); 
		vfs_memory = GetExtraData( memory );
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((PTRSZVAL)vfs_memory-(PTRSZVAL)memory), REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (PTRSZVAL)vol, FALSE );
	}
#else
	{
		SetSystemLog( SYSLOG_FILE, stderr );
#ifdef ALT_VFS_NAME
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
#else
		l.fsi = sack_get_filesystem_interface( "sack_shmem" );
#endif
		sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		vol = sack_vfs_load_crypt_volume( argc> 1? argv[1]:"package.vfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (PTRSZVAL)vol, TRUE );
	}
#endif
	l.target_path = ".";
	if( vol )
	{
		POINTER info = NULL;
		while( ScanFilesEx( NULL, "*", &info, ShowFile, SFF_SUBCURSE | SFF_SUBPATHONLY
			, (PTRSZVAL)0, FALSE, l.rom ) );
	}
	//return 0;
}

#if __LINUX__
int main( int argc, char **argv, char **envp )
{
	if( l.first_file )
		System( l.first_file );
	return l.linux_entry_point( argc, argv, envp );
}
#endif
#if _WIN32
SaneWinMain(argc,argv)
{
	if( l.first_file )
		System( l.first_file, NULL, NULL );
	return 0; //l.entry_point( 0, 0, GetCommandLine(), 1 );
}
EndSaneWinMain()
#endif

