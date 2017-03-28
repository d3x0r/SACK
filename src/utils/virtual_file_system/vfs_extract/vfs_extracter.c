#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>
#include <sack_vfs.h>
#include <sack_system.h>
#include <configscript.h>

#ifdef STANDALONE_HEADER
#  define REPLACE_ME_2  (argc>2)?argv[2]:NULL
#  define REPLACE_ME_3  (argc>3)?argv[3]:NULL
#else
#  define REPLACE_ME_2  (argc>3)?argv[3]:NULL
#  define REPLACE_ME_3  (argc>4)?argv[4]:NULL
#endif

struct command {
	TEXTSTR exists;
	TEXTSTR cmd;
	TEXTSTR args;
};

static struct vfs_runner_local
{
	char *first_file;
	PLIST commands;
	PLIST preCommands;
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	char *target_path;
	char *prior_output_path;  // prevent remaking directories for EVERY file...
	PCONFIG_HANDLER pch;
}l;

//---------------------------------------------------------------------------

static uintptr_t CPROC SetDefaultPath( uintptr_t psv, arg_list args ) {
	PARAM( args, CTEXTSTR, path );
	if( !l.target_path ) { // otherwise it was already set on the commandline
		l.target_path = ExpandPath( path );
	}
	return psv;
}

static uintptr_t CPROC AddPostInstallCommand( uintptr_t psv, arg_list args ) {
	PARAM( args, CTEXTSTR, cmd );
	PARAM( args, CTEXTSTR, cmd_args );
	struct command *command = New( struct command );
	command->cmd = StrDup( cmd );
	command->args = StrDup( cmd_args );
	{ char *c = command->cmd; while( c[0] ){ if( c[0]=='/' ) c[0]='\\'; c++; } }
	AddLink( &l.commands, command );
	return psv;
}

static uintptr_t CPROC AddPreInstallCommand( uintptr_t psv, arg_list args ) {
	PARAM( args, CTEXTSTR, exists );
	PARAM( args, CTEXTSTR, cmd );
	PARAM( args, CTEXTSTR, cmd_args );
	struct command *command = New( struct command );
	command->exists = StrDup( exists );
	command->cmd = StrDup( cmd );
	command->args = StrDup( cmd_args );
	{ char *c = command->cmd; while( c[0] ){ if( c[0]=='/' ) c[0]='\\'; c++; } }
	AddLink( &l.preCommands, command );
	return psv;
}

static void InitConfigHandler( void ) {
	l.pch = CreateConfigurationHandler();
	AddConfigurationMethod( l.pch, "defaultPath \"%m\"", SetDefaultPath );
	AddConfigurationMethod( l.pch, "pre-run \"%m\" \"%m\" \"%m\"", AddPreInstallCommand );
	AddConfigurationMethod( l.pch, "run \"%m\" \"%m\"", AddPostInstallCommand );
}

//---------------------------------------------------------------------------

static LOGICAL CPROC ExtractFile( CTEXTSTR name )
{
	FILE *file;
	size_t sz;
	POINTER data;

	file = sack_fopenEx( 0, name, "rb", l.rom );
	if( file )
	{
		sz = sack_fsize( file );
		if( sz )
		{
			data = NewArray( uint8_t, sz );
			sack_fread( data, sz, 1, file );

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
						sack_fwrite( data, sz, 1, out );
						sack_fclose( out );
					}
				}
			}
			Release( data );
		}
		sack_fclose( file );
		return TRUE;
	}

	return FALSE;
}

static void CPROC ShowFile( uintptr_t psv, CTEXTSTR file, int flags )
{
	ExtractFile( file );
}


PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
	struct volume *vol;
	int argc;
	char **argv;
#if _WIN32 
	wchar_t * wcmd = GetCommandLineW();
	char *cmd = WcharConvert( wcmd );
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
		if( argc > 1 ) {
			l.target_path = ExpandPath( argv[1] );
		}
		SetSystemLog( SYSLOG_FILE, stderr ); 
		if( !memory ) {
			lprintf( "Please launch with full application name/path" );
			return;
		}
		// raw EXE images can be passed to VFS module now and it internally figures an offset
		// includes verification of the EXE signature.
		//lprintf( "Memory: %p %d", memory, sz );
		
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		//lprintf( "use crypt.." );
		vol = sack_vfs_use_crypt_volume( memory, sz, REPLACE_ME_2, REPLACE_ME_3 );
		if( !vol ) {
			lprintf( "Failed to load attached vault." );
			return;
		}
		//lprintf( "mount... %p", vol );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, FALSE );
	}
#else
	{
		SetSystemLog( SYSLOG_FILE, stderr );
#  ifdef ALT_VFS_NAME
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
#  else
		l.fsi = sack_get_filesystem_interface( "sack_shmem" );
#  endif
		if( argc > 2 ) {
			l.target_path = ExpandPath( argv[2] );
		} else {
			l.target_path = ExpandPath( "." );
		}
		sack_set_default_filesystem_interface( l.fsi );
		vol = sack_vfs_load_crypt_volume( argc> 1? argv[1]:"package.vfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, TRUE );
	}
#endif
	//l.target_path = ".";
	if( vol )
	{
		POINTER info = NULL;
		FILE *file = sack_fopenEx( 0, ".app.config", "rb", l.rom );
		//lprintf( "open aoppconfig = %p", file );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( uint8_t, sz );
				sack_fread( data, sz, 1, file );
				ProcessConfigurationInput( l.pch, data, sz, 0 );
				if( !l.target_path )
					l.target_path = ".";
				Deallocate( POINTER, data );
			}
			sack_fclose( file );
		}
		else 
			if( !l.target_path ) l.target_path = (char*)GetProgramName();
		{
			INDEX idx;
			struct command *command;
			static TEXTCHAR buf[4096];
			LIST_FORALL( l.preCommands, idx, struct command *, command ) {
				snprintf( buf, 4096, "%s/%s"
				        , l.target_path
				        , command->exists );
				if( sack_exists( buf ) ) {
					snprintf( buf, 4096, "\"%s\\%s\"%s%s"
					        , l.target_path
					        , command->cmd
					        , command->args ? " ":""
					        , command->args?command->args:"" );
					lprintf( "pre-run:%s", buf );
					System( buf, NULL, 0 );
				}
			}
		}

		while( ScanFilesEx( NULL, "*", &info, ShowFile, SFF_SUBCURSE | SFF_SUBPATHONLY
			, (uintptr_t)0, FALSE, l.rom ) );
	}
	//return 0;
}

SaneWinMain(argc,argv)
{
	INDEX idx;
	struct command *command;
	static TEXTCHAR buf[4096];
	LIST_FORALL( l.commands, idx, struct command *, command ) {
		snprintf( buf, 4096, "\"%s\\%s\"%s%s"
		        , l.target_path
		        , command->cmd
		        , command->args ? " ":""
		        , command->args?command->args:"" );
		lprintf( "run:%s", buf );
		System( buf, NULL, 0 );
	}
	return 0;
}
EndSaneWinMain()

