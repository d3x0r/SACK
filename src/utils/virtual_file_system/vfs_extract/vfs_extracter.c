#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>
#include <sack_vfs.h>
#include <sack_system.h>
#include <configscript.h>

#define REPLACE_ME_2  (argc>2)?argv[2]:NULL
#define REPLACE_ME_3  (argc>3)?argv[3]:NULL


struct command {
	TEXTSTR cmd;
	TEXTSTR args;
};

static struct vfs_runner_local
{
	char *first_file;
	PLIST commands;
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	char *target_path;
	char *prior_output_path;  // prevent remaking directories for EVERY file...
	PCONFIG_HANDLER pch;
}l;

//-------------------------------------------------------
// function to process a currently loaded program to get the
// data offset at the end of the executable.

#define Seek(a,b) (((uintptr_t)a)+(b))

POINTER GetExtraData( POINTER block )
{
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
			lprintf( "Basic signature check failed; not a library" );


		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE )
			lprintf( "Basic NT signature check failed; not a library" );

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				lprintf( "Optional header signature is incorrect..." );
			}
		}
		{
			int n;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].PointerToRawData) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			dwSize += 0xFFF;
			dwSize &= ~0xFFF;
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
}

//---------------------------------------------------------------------------


static uintptr_t CPROC SetDefaultPath( uintptr_t psv, arg_list args ) {
	PARAM( args, CTEXTSTR, path );
	if( !l.target_path ) { // otherwise it was already set on the commandline
		l.target_path = ExpandPath( path );
 		SetCurrentPath( l.target_path );
	}
	return psv;
}

static uintptr_t CPROC AddPostInstallCommand( uintptr_t psv, arg_list args ) {
	PARAM( args, CTEXTSTR, cmd );
	PARAM( args, CTEXTSTR, cmd_args );
	struct command *command = New( struct command );
	command->cmd = StrDup( cmd );
	command->args = StrDup( cmd_args );
	AddLink( &l.commands, command );
	return psv;
}

static void InitConfigHandler( void ) {
	l.pch = CreateConfigurationHandler();
	AddConfigurationMethod( l.pch, "defaultPath \"%m\"", SetDefaultPath );
	AddConfigurationMethod( l.pch, "run \"%m\" \"%m\"", AddPostInstallCommand );
}

//---------------------------------------------------------------------------

static LOGICAL CPROC ExtractFile( CTEXTSTR name )
{
	if( StrCmp( name, ".app.config" ) == 0 ) {
		FILE *file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( uint8_t, sz );
				sack_fread( data, 1, sz, file );
				ProcessConfigurationInput( l.pch, data, sz, 0 );
				if( !l.target_path )
               l.target_path = ".";
				Release( data );
			}
         sack_fclose( file );
		}
	} else {
		FILE *file;
		file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( uint8_t, sz );
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
				Release( data );
			}
			sack_fclose( file );
			return TRUE;
		}
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
         l.target_path = ExpandPath( argv[1] );
			SetCurrentPath( l.target_path );
		}
		SetSystemLog( SYSLOG_FILE, stderr ); 
		vfs_memory = GetExtraData( memory );
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((uintptr_t)vfs_memory-(uintptr_t)memory), REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, FALSE );
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
		vol = sack_vfs_load_crypt_volume( argc> 1? argv[1]:"package.vfs", REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, TRUE );
	}
#endif
	//l.target_path = ".";
	if( vol )
	{
		POINTER info = NULL;
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
		snprintf( buf, 4096, "\"%s/%s\"%s%s"
		        , l.target_path
		        , command->cmd
		        , command->args ? " ":""
		        , command->args?command->args:"" );
		System( buf, NULL, 0 );
	}
	return 0;
}
EndSaneWinMain()

