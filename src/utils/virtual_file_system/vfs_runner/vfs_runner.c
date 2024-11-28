#include <stdhdrs.h>
#include <filesys.h>
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
#if _WIN32
	int(WINAPI*entry_point)(HINSTANCE,HINSTANCE,LPSTR,int);
#endif
#if __LINUX__
	int(*linux_entry_point)( int argc, char **argv, char **env );
#endif
}l;

#define ADVANCED_LOADER 1

#if ADVANCED_LOADER
static LOGICAL CPROC LoadLibraryDependant( CTEXTSTR name )
{
	static PLIST loading;
	CTEXTSTR del_string;
#ifdef DEBUG_LIBRARY_LOADING
	lprintf( "LoadLIb %s", name );
#endif
	{
		CTEXTSTR test;
		INDEX idx;
		LIST_FORALL( loading, idx, CTEXTSTR, test )
		{
#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "Compare %s vs %s", name, test );
#endif
			if( StrStr( name, test ) || StrStr( test, name ) )
			{
				//lprintf( "already loading %s(%s)", name, test );
				return TRUE;
			}
		}
	}
	if( IsMappedLibrary( name ) )
		return TRUE;
	AddLink( &loading, del_string = StrDup( name ) );
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
			TEXTCHAR tmpnam[256];
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
#ifdef DEBUG_LIBRARY_LOADING
				size_t written, read ;
#endif
				POINTER data = NewArray( uint8_t, sz );
#ifdef DEBUG_LIBRARY_LOADING
				read =
#endif
					sack_fread( data, sz, 1, file );
#ifdef DEBUG_LIBRARY_LOADING
				written =
#endif
					sack_fwrite( data, sz, 1, tmp );
				sack_fclose( tmp );
#ifdef DEBUG_LIBRARY_LOADING
				lprintf( "written file... closed file...now scanning and then load %d %d", read, written );
#endif
#ifdef WIN32
				ScanLoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
#endif
				//if( !LoadFunction( name, NULL ) )
				{
					LoadFunction( tmpnam, NULL );
				}
#if 0
				{
					int n;
					for( n = 0; fixup_entries[n].libname; n++ )
					{
						if( fixup_entries[n].fixed )
							continue;
						if( StrStr( tmpnam, fixup_entries[n].libname ) != 0 )
						{
							POINTER p;
							//lprintf( "need fix abort.." );
							//fixup_entries[n].fixed = TRUE;
							p = LoadLibrary( tmpnam );
							FixAbortLink( p, fixup_entries + n, TRUE );
						}
					}
				}
#endif
				//sack_unlinkEx( 0, tmpnam, sack_get_default_mount() );
				Release( data );
			}
			DeleteLink( &loading, del_string );
			sack_fclose( file );
			return TRUE;
		}
	}
	else
		LoadFunction( name, NULL );
    DeleteLink( &loading, del_string );

	return FALSE;
}
#else

static LOGICAL CPROC LoadLibraryDependant( CTEXTSTR name )
{
	if( sack_existsEx( name, l.rom ) )
	{
		FILE *file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			size_t sz = sack_fsize( file );
			if( sz )
			{
				POINTER data = NewArray( uint8_t, sz );
				sack_fread( data, 1, sz, file );
				LoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
				Release( data );
			}
			sack_fclose( file );
			return TRUE;
		}
	}

	// some DLLs have to be loaded with resource support from system... 
	if( 0 )
	{
		CTEXTSTR altpaths[2] = { ".", "c:/windows/syswow64" };
		TEXTCHAR othername[256];
		int n;
		for( n = 0; n < 2; n++ )
		{
			snprintf( othername, 256, "%s/%s", altpaths[n], name );
			if( sack_exists( othername ) )
			{
				FILE *file = sack_fopenEx( 0, othername, "rb", NULL );
				if( file )
				{
					size_t sz = sack_fsize( file );
					if( sz )
					{
						POINTER data = NewArray( uint8_t, sz );
						sack_fread( data, 1, sz, file );
						LoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
						Release( data );
					}
					sack_fclose( file );
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

#endif

#ifdef STANDALONE_HEADER
static char * FindProgram( const char *name ) {
	if( sack_existsEx( name, l.rom ) )
	{
		FILE *file;
#ifdef DEBUG_LIBRARY_LOADING

		lprintf( "%s exists...", name );
#endif
		file = sack_fopenEx( 0, name, "rb", l.rom );
		if( file )
		{
			TEXTSTR path = ExpandPath( "*/tmp" );
			TEXTCHAR tmpnam[256];
			size_t sz = sack_fsize( file );
			FILE *tmp;
			if( sz ) {
#ifdef DEBUG_LIBRARY_LOADING
				lprintf( "library is %d %s", sz, name );
#endif
				MakePath( path );
				snprintf( tmpnam, 256, "%s/%s", path, name );
				tmp = sack_fopenEx( 0, tmpnam, "wb", sack_get_default_mount() );
#ifdef DEBUG_LIBRARY_LOADING
				lprintf( "Loading %s(%p)", tmpnam, tmp );
#endif
				if( tmp )
				{
					//size_t written, read ;
					POINTER data = NewArray( uint8_t, sz );
					//read = 
						sack_fread( data, sz, 1, file );
					//written = 
						sack_fwrite( data, sz, 1, tmp );
					sack_fclose( tmp );
#ifdef DEBUG_LIBRARY_LOADING
					lprintf( "written file... closed file...now scanning and then load %d %d", read, written );
#endif
				}
			}
			sack_fclose( file );
         return path;
		}
	}
	return NULL;
}
#endif

void FixupMyTLS( void )
{
	/* This didn't actually work... manually loading a DLL into memory that uses TLS
	 * and then trying to fixup the TLS directory didn't work.
	 * TLS directory is not actually used by the loader? but is
	 * used by the program itself to initialize TLS data?
	 */
#if _WIN32
	#define Seek(a,b) (((uintptr_t)a)+(b))

	HMODULE p = GetModuleHandle( NULL );
	PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)p;
	PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( p, source_dos_header->e_lfanew );
	PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
	PIMAGE_TLS_DIRECTORY tls = (PIMAGE_TLS_DIRECTORY)Seek( p, dir[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress );
	int n;
	if( dir[IMAGE_DIRECTORY_ENTRY_TLS].Size )
	{
		PLIST new_list = CreateList();
		int count = dir[IMAGE_DIRECTORY_ENTRY_TLS].Size / sizeof( IMAGE_TLS_DIRECTORY );
		if( count > 1 )
			DebugBreak();
		AddLink( &new_list, 0 );
		for( n = 0; n < count; n++ )
		{
			//POINTER data;
#if defined( _MSC_VER )
			//DWORD dwInit;
#endif
			//size_t size_init = ( tls->EndAddressOfRawData - tls->StartAddressOfRawData );
			//size_t size = size_init + tls->SizeOfZeroFill;
			/*
			printf( "%p %p %p(%d) %p\n"
						, tls->AddressOfCallBacks
						, tls->StartAddressOfRawData, tls->EndAddressOfRawData
						, ( tls->EndAddressOfRawData - tls->StartAddressOfRawData ) + tls->SizeOfZeroFill
						, tls->AddressOfIndex );
				*/
#if defined( _MSC_VER )
			//dwInit = (*((DWORD*)tls->AddressOfIndex));
			{
#  ifdef __64__
#  else
				POINTER data;
				{
					_asm mov ecx, fs:[2ch];
					_asm mov eax, dwInit;
					//_asm mov edx, data;
					_asm mov edx, [ecx+eax*4];
					_asm mov data, edx;
				}
				new_list->pNode[0] = data;
				(*((DWORD*)tls->AddressOfIndex)) = dwInit + 1;
				{
					_asm mov ecx, new_list;
					_asm mov fs:[2ch], ecx;
				}
#  endif
			}
#endif
		}
	}
#endif
}

PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
#if _WIN32 
	TEXTSTR cmd = GetCommandLine();
	int argc;
	char **argv;
#if 0
	{
		DWORD dwInit;
		POINTER tls_list;
		POINTER data;
								{
									//fs:[18h]; == fs:[0] in ds...
							_asm mov ecx, fs:[2ch];
							_asm mov tls_list, ecx;
						}
						dwInit = 0;
						//while( (*tls_list) ) { tls_list++; dwInit++; }
						//(*tls_list) = data;
						DebugBreak();
						LoadLibrary( "bag.dll" );
						LoadLibrary( "bag.video.dll" );
						LoadLibrary( "avutil-54.dll" );
						{
							_asm mov ecx, fs:[2ch];
							_asm mov tls_list, ecx;
						}

	}
#endif
	//FixupMyTLS();
	ParseIntoArgs( cmd, &argc, &argv );
#ifdef STANDALONE_HEADER
	{
		size_t sz = 0;
		POINTER memory = OpenSpace( NULL, argv[0], &sz );
		POINTER vfs_memory;
		struct sack_vfs_volume *vol;
		//struct sack_vfs_volume *vol2;
		SetSystemLog( SYSLOG_FILE, stderr ); 
		vfs_memory = memory;
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
		sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		SetExternalFindProgram( FindProgram );
		SetProgramName( "program" );
		vol = sack_vfs_use_crypt_volume( vfs_memory, sz-((uintptr_t)vfs_memory-(uintptr_t)memory), 0, REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, FALSE );
		//vol2 = 
			sack_vfs_load_crypt_volume( "external.vfs", 0, REPLACE_ME_2, REPLACE_ME_3 );
		l.ram = sack_mount_filesystem( "extra", l.fsi, 110, (uintptr_t)vol, TRUE );
		if( vol )
		{
			FILE *file = sack_fopenEx( 0, "0", "rb", l.rom );
			size_t sz = sack_fsize( file );
			POINTER data = NewArray( uint8_t, sz );
			sack_fread( data, sz, 1, file );
			sack_fclose( file );
			l.entry_point = (int(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))
				   LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
			Release( data );
		}
	}
#else
	{
		struct sack_vfs_volume *vol;
		//struct sack_vfs_volume *vol2;
		TEXTSTR cmd = GetCommandLine();
		int argc;
		char **argv;
		ParseIntoArgs( cmd, &argc, &argv );
		SetSystemLog( SYSLOG_FILE, stderr ); 
#ifdef ALT_VFS_NAME
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
#else
		l.fsi = sack_get_filesystem_interface( "sack_shmem" );
#endif
		sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		SetProgramName( "program" );
		vol = sack_vfs_load_crypt_volume( "test.scvfs", 0, REPLACE_ME_2, REPLACE_ME_3 );
		l.rom = sack_mount_filesystem( "self", l.fsi, 100, (uintptr_t)vol, TRUE );
		//vol2 = 
			sack_vfs_load_crypt_volume( "external.vfs", 0, REPLACE_ME_2, REPLACE_ME_3 );
		l.ram = sack_mount_filesystem( "extra", l.fsi, 110, (uintptr_t)vol, TRUE );
		if( vol )
		{
			FILE *file = sack_fopenEx( 0, "0", "rb", l.rom );
			size_t sz = sack_fsize( file );
			POINTER data = NewArray( uint8_t, sz );

			sack_fread( data, sz, 1, file );
			sack_fclose( file );
			l.entry_point = (int(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))
				   LoadLibraryFromMemory( "program.exe", data, sz, FALSE, LoadLibraryDependant );
			Release( data );
		}
	}
#endif
#endif
   //return 0;
}

#if __LINUX__
int main( int argc, char **argv, char **envp )
{
	return l.linux_entry_point( argc, argv, envp );
}
#endif
#if _WIN32
SaneWinMain(argc,argv)
{
	return l.entry_point( 0, 0, GetCommandLine(), 1 );
}
EndSaneWinMain()
#endif

