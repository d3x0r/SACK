#include <stdhdrs.h>
#include <sack_vfs.h>

static struct vfs_command_local
{
	struct file_system_interface *fsi;
	struct volume *current_vol;
} l;

void StoreFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopen( 0, filename, "rb" );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, asfile, "wb", l.fsi );
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

void _StoreFile( PTRSZVAL psv,  CTEXTSTR filename, int flags )
{
	FILE *in = sack_fopen( 0, filename, "rb" );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, filename, "wb", l.fsi );
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

void StoreFile( CTEXTSTR filemask )
{
	void *info = NULL;
	while( ScanFiles( NULL, filemask, &info, _StoreFile, SFF_NAMEONLY, 0 ) );
}

void ExtractFile( CTEXTSTR filename )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.fsi );
	if( in )
	{
		FILE *out = sack_fopen( 0, filename, "wb" );
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

void ExtractFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.fsi );
	if( in )
	{
		FILE *out = sack_fopen( 0, asfile, "wb" );
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
	printf( "%s\n", file );
}

void GetDirectory( void )
{
	POINTER info = NULL;
	while( ScanFilesExx( NULL, "*", &info, ShowFile, SFF_SUBCURSE|SFF_SUBPATHONLY, 0, FALSE, l.fsi ) );

	//l.fsi->
}


SaneWinMain( argc, argv )
{
	int arg;

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

	}
	return 0;
}
EndSaneWinMain()
