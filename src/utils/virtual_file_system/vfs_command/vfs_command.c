#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#include <sack_vfs.h>

static struct vfs_command_local
{
	struct file_system_interface *fsi;
	struct volume *current_vol;
	struct file_system_mounted_interface *current_mount;
} l;

static void StoreFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", sack_get_default_mount() );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, asfile, "wb", l.current_mount );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( _8, size );
		sack_fread( data, 1, size, in );
		sack_fwrite( data, 1, size, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

static void _StoreFile( PTRSZVAL psv,  CTEXTSTR filename, int flags )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", sack_get_default_mount() );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, filename, "wb", l.current_mount );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( _8, size );
		sack_fread( data, 1, size, in );
		sack_fwrite( data, 1, size, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

static void StoreFile( CTEXTSTR filemask )
{
	void *info = NULL;
	while( ScanFilesEx( NULL, filemask, &info, _StoreFile, SFF_SUBPATHONLY, 0, FALSE, sack_get_default_mount() ) );
}

static void ExtractFile( CTEXTSTR filename )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.current_mount );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, filename, "wb", sack_get_default_mount() );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( _8, size );
		sack_fread( data, 1, size, in );
		sack_fwrite( data, 1, size, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

static void AppendFilesAs( CTEXTSTR filename1, CTEXTSTR filename2, CTEXTSTR outputname )
{
	FILE *file1;
	size_t file1_size;
	FILE *file2;
	size_t file2_size;
	FILE *file_out;
	size_t file_out_size;

	POINTER buffer;

	file1 = sack_fopenEx( 0, filename1, "rb", sack_get_default_mount() );
	if( !file1 ) { printf( "Failed to read file to append: %s", filename1 ); return; }
	file1_size = sack_fsize( file1 );
	file2 = sack_fopenEx( 0, filename2, "rb", sack_get_default_mount() );
	if( !file2 ) { printf( "Failed to read file to append: %s", filename2 ); return; }
	file2_size = sack_fsize( file2 );
	file_out = sack_fopenEx( 0, outputname, "wb", sack_get_default_mount() );
	if( !file_out ) { printf( "Failed to read file to append to: %s", outputname ); return; }
	file_out_size = sack_fsize( file_out );

	buffer = NewArray( _8, file1_size );
	sack_fread( buffer, 1, file1_size, file1 );
	sack_fwrite( buffer, 1, file1_size, file_out );
	{
		int fill = 4096 - ( file1_size & 0xFFF );
		int n;
		if( fill < 4096 )
			for( n = 0; n < fill; n++ ) sack_fwrite( "", 1, 1, file_out );
	}
	Release( buffer );

	buffer = NewArray( _8, file2_size );
	sack_fread( buffer, 1, file2_size, file2 );
	sack_fwrite( buffer, 1, file2_size, file_out );


	sack_fclose( file1 );
	sack_fclose( file2 );
	sack_fclose( file_out );
}

static void ExtractFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.current_mount );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, asfile, "wb", sack_get_default_mount() );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( _8, size );
		sack_fread( data, 1, size, in );
		sack_fwrite( data, 1, size, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

static void CPROC ShowFile( PTRSZVAL psv, CTEXTSTR file, int flags )
{
	void *f = l.fsi->open( (PTRSZVAL)psv, file + 2 );
	printf( "%9d %s\n", l.fsi->size( f ), file );
	l.fsi->close( f );
}

static void GetDirectory( void )
{
	POINTER info = NULL;
	while( ScanFilesEx( NULL, "*", &info, ShowFile, SFF_SUBCURSE|SFF_SUBPATHONLY
	                  , (PTRSZVAL)l.current_vol, FALSE, l.current_mount ) );
	//l.fsi->
}

static void usage( void )
{
	printf( "arguments are processed in order... commands may be appended on the same line...\n" );
	printf( "   vfs <filename>                 : specify a unencrypted VFS file to use\n" );
	printf( "   cvfs <filename> <key1> <key2>  : specify an encrypted VFS file to use; and keys to use\n" );
	printf( "   dir                            : show current directory\n" );
	printf( "   rm <filename>                  : delete file within VFS\n" );
	printf( "   delete <filename>              : delete file within VFS\n" );
	printf( "   store <filemask>               : store files that match the name in the VFS from local filesystem\n" );
	printf( "   extract <filemask>             : extract files that match the name in the VFS to local filesystem\n" );
	printf( "   storeas <filename> <as file>   : store file from <filename> into VFS as <as file>\n" );
	printf( "   extractas <filename> <as file> : extract file <filename> from VFS as <as file>\n" );
}

SaneWinMain( argc, argv )
{
	int arg;
	if( argc < 2 ) { usage(); return 0; }

	l.fsi = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME );
	if( !l.fsi ) 
	{
		printf( "Failed to load file system interface.\n" );
		return 0;
	}
	for( arg = 1; arg < argc; arg++ )
	{
		if( StrCaseCmp( argv[arg], "dir" ) == 0 )
			GetDirectory();
		else if( StrCaseCmp( argv[arg], "cvfs" ) == 0 )
		{
			if( l.current_vol )
				sack_vfs_unload_volume( l.current_vol );
			l.current_vol = sack_vfs_load_crypt_volume( argv[arg+1], argv[arg+2], argv[arg+3] );
			if( !l.current_vol )
			{
				printf( "Failed to load vfs: %s", argv[arg+1] );
				return 2;
			}
			l.current_mount = sack_mount_filesystem( l.fsi, 10, (PTRSZVAL)l.current_vol, 1 );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "vfs" ) == 0 )
		{
			if( l.current_vol )
				sack_vfs_unload_volume( l.current_vol );
			l.current_vol = sack_vfs_load_volume( argv[arg+1] );
			if( !l.current_vol )
			{
				printf( "Failed to load vfs: %s", argv[arg+1] );
				return 2;
			}
			l.current_mount = sack_mount_filesystem( l.fsi, 10, (PTRSZVAL)l.current_vol, 1 );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "rm" ) == 0
			|| StrCaseCmp( argv[arg], "delete" ) == 0 )
		{
			l.fsi->unlink( (PTRSZVAL)l.current_vol, argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "store" ) == 0 )
		{
			StoreFile( argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "extract" ) == 0 )
		{
			ExtractFile( argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "storeas" ) == 0 )
		{
			StoreFileAs( argv[arg+1], argv[arg+2] );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "extractas" ) == 0 )
		{
			ExtractFileAs( argv[arg+1], argv[arg+2] );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "append" ) == 0 )
		{
			AppendFilesAs( argv[arg+1], argv[arg+2], argv[arg+3] );
			arg += 3;
		}

	}
	return 0;
}
EndSaneWinMain()
