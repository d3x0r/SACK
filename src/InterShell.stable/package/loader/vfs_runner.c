#include <stdhdrs.h>
#include <network.h>
#include <deadstart.h>
#include <sack_vfs.h>

#include "memory_dll_loader.h"
//#include "app-signature.h"

//#define DEBUG_LIBRARY_LOADING

static struct vfs_runner_local
{
	struct file_system_interface *fsi;
	struct file_system_mounted_interface *rom;
	struct file_system_mounted_interface *ram;
	int(CPROC*entry_point)(int argc, char**argv, int bConsole,struct volume* (CPROC *load)( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey ),void (CPROC*unload)(struct volume *));
	struct volume* resource_fs;
	struct volume* core_fs;
	struct volume* user_fs;
	struct volume* user_mirror_fs;
	struct volume* rom_fs;
	struct file_system_mounted_interface *resource_mount;
	struct file_system_mounted_interface *core_mount;
}l;

struct fixup_table_entry {
	CTEXTSTR libname;
	CTEXTSTR import_libname;
	CTEXTSTR import_name;
	POINTER pointer;
	LOGICAL fixed;
};

static void FakeAbort( void );


static struct fixup_table_entry fixup_entries[] = { { "libgcc_s_dw2-1.dll", "msvcrt.dll", "abort", (POINTER)FakeAbort } 
					, {NULL }
					};


static void FakeAbort( void )
{
	TerminateProcess( GetCurrentProcess(), 0 );
}

static void *MyAlloc( size_t size )
{
	return Allocate( size );
}

static void MyFree( void *p )
{
	Deallocate( void *, p );
}
static void * MyRealloc( void *p, size_t size )
{
	return Reallocate( p, size );
}

#define Seek(a,b) (((uintptr_t)a)+(b))

static void FixAbortLink( POINTER p, struct fixup_table_entry *entry, LOGICAL gcclib )
{
	POINTER source_memory = p;
	POINTER real_memory = p;
	//printf( "Load %s (%d:%d)\n", name, generation, level );
	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
		{
			lprintf( "Basic signature check failed; not a library" );
			return;
		}

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE )
		{
			lprintf( "Basic NT signature check failed; not a library" );
			return;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				//lprintf( "Optional header signature is incorrect... %x = %x",source_nt_header->OptionalHeader.Magic, IMAGE_NT_OPTIONAL_HDR_MAGIC );
			}
		}
		{
			//int n;
			PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section = NULL;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			uintptr_t dwSize = 0;
			//uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			// compute size of total of sections
			// mark a few known sections for later processing
			// ------------- Go through the sections and move to expected virtual offsets
			real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
			if( real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int n;
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name;
					int f;
					uintptr_t *dwFunc;
					uintptr_t *dwTargetFunc;
					PIMAGE_IMPORT_BY_NAME import_desc;
					if( real_import_base[n].Name )
						dll_name = (const char*) Seek( real_import_base, real_import_base[n].Name - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress/*source_import_section->VirtualAddress*/ );
					if( gcclib )
					{
						if( StrCmp( dll_name, entry->import_libname ) )
						{
							//lprintf( "skipping %s", dll_name );
							continue;
						}
					}
					//lprintf( "import %s", dll_name );
					//char * function_name = (char*) Seek( import_base, import_base[n]. - source_import_section->VirtualAddress );
					//printf( "thing %s\n", dll_name );
#if __WATCOMC__ && __WATCOMC__ < 1200
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OrdinalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#else
					dwFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].OriginalFirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#endif
					dwTargetFunc = (uintptr_t*)Seek( real_import_base, real_import_base[n].FirstThunk - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
					for( f = 0; dwFunc[f]; f++ )
					{
						if( dwFunc[f] & ( (uintptr_t)1 << ( ( sizeof( uintptr_t ) * 8 ) - 1 ) ) )
						{
							//lprintf( "Oridinal is %d", dwFunc[f] & 0xFFFF );
							//dwTargetFunc[f] = (uintptr_t)LoadFunction( dll_name, (CTEXTSTR)(dwFunc[f] & 0xFFFF) );
						}
						else
						{
							import_desc = (PIMAGE_IMPORT_BY_NAME)Seek( real_memory, dwFunc[f] );
							//lprintf( " sub thing %s", import_desc->Name );
							if( gcclib )
							{
								if( StrCmp( (CTEXTSTR)import_desc->Name, entry->import_name ) == 0 )
								{
									//lprintf( "Fixed abort...%p to %p", dwTargetFunc + f, entry->pointer );
									dwTargetFunc[f] = (uintptr_t)entry->pointer;
									return;
								}
							}
#if 0
							else
							{
			{
				DWORD old_protect;
				VirtualProtect( (POINTER)( dwTargetFunc + f )
						, 4
						, PAGE_EXECUTE_READWRITE
						, &old_protect );
								if( StrCmp( (CTEXTSTR)import_desc->Name, "malloc" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyAlloc;
									//return;
								}
								if( StrCmp( (CTEXTSTR)import_desc->Name, "free" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyFree;
									//return;
								}
								if( StrCmp( (CTEXTSTR)import_desc->Name, "realloc" ) == 0 )
								{
									dwTargetFunc[f] = (uintptr_t)MyRealloc;
									//return;
								}

				VirtualProtect( (POINTER)( dwTargetFunc + f )
						, 4
						, old_protect
						, &old_protect );
			}
							}
#endif
						}
					}
				}
			}
		}
	}
}

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
				int written, read ;
				POINTER data = NewArray( uint8_t, sz );
				read = sack_fread( data, 1, sz, file );
				written = sack_fwrite( data, 1, sz, tmp );
				sack_fclose( tmp );
#ifdef DEBUG_LIBRARY_LOADING
				lprintf( "written file... closed file...now scanning and then load %d %d", read, written );
#endif
				ScanLoadLibraryFromMemory( name, data, sz, TRUE, LoadLibraryDependant );
				//if( !LoadFunction( name, NULL ) )
				{
					LoadFunction( tmpnam, NULL );
				}
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

static void CPROC InstallFinished( uintptr_t psv, PTASK_INFO task )
{
	((LOGICAL*)psv)[0] = TRUE;

}

PRIORITY_PRELOAD( LowestInit, 1 )
{
		//SetProgramName( "program" );
		//sack_set_common_data_producer( "Karaway Entertainment" );
}

PRIORITY_PRELOAD( XSaneWinMain, DEFAULT_PRELOAD_PRIORITY + 20 )//( argc, argv )
{
	TEXTSTR cmd = GetCommandLine();
	int argc;
	char **argv;
	//FixupMyTLS();
	//MessageBox( NULL, "Pause", "Attach", MB_OK );
	ParseIntoArgs( cmd, &argc, &argv );
	//SetSystemLog( SYSLOG_FILE, stderr ); 
#ifdef ALT_VFS_NAME
		l.fsi = sack_get_filesystem_interface( "sack_shmem.runner" );
#else
		l.fsi = sack_get_filesystem_interface( "sack_shmem" );
#endif
		//sack_set_default_filesystem_interface( l.fsi );
		SetExternalLoadLibrary( LoadLibraryDependant );
		lprintf( "Open Core FS Log" );

#define _appload( a, b )  sack_vfs_load_crypt_volume( "application.dat", NULL /*#a "-" b*/, NULL/*app_signature*/ )
#define appload( a,b )  _appload( a,b )

#define _sfxappload( a, b )  sack_vfs_use_crypt_volume( vfs_memory, sz-((uintptr_t)vfs_memory-(uintptr_t)memory), NULL, NULL )
//#define _appload( a,b )  sack_vfs_load_crypt_volume( "application.dat", a, b )
#define sfxappload( a,b )  _sfxappload( a,b )

#define _resload( a,b )  sack_vfs_load_crypt_volume( "resources.kw", #a, "" b )
//#define _resload( a,b )  sack_vfs_load_crypt_volume( "resources.kw", a, b )
#define resload( a,b )  _resload( a,b )

	{
#ifdef STANDALONE_HEADER
		uintptr_t sz = 0;
		POINTER memory = OpenSpace( NULL, argv[0], &sz );
		POINTER vfs_memory;
		//printf( "memory is %p(%d)\n", memory, sz );
		if( memory == NULL )
		{
			MessageBox( NULL, "Please Launch with full path", "Startup Error", MB_OK );
			exit(0);
		}
		vfs_memory = GetExtraData( memory );
		//printf( "extra is %d(%08x)\n", vfs_memory, vfs_memory );
		l.rom_fs = sfxappload( _WIDE(CMAKE_BUILD_TYPE), CPACK_PACKAGE_VERSION_PATCH );
		if( !l.rom_fs )
		{
			MessageBox( NULL, "Failed to load ROM", "Startup Error", MB_OK );
			exit(0);
		}
		//if( StrCmp( sack_vfs_get_signature( l.rom_fs ), app_signature ) )
		//	return;
		//printf( "and... we get %p\n", l.rom_fs );
		if( !l.rom_fs )
			return;
#else
		l.rom_fs = appload( CMAKE_BUILD_TYPE, CPACK_PACKAGE_VERSION_PATCH );
#endif
	}

	l.rom = sack_mount_filesystem( "self", l.fsi, 900, (uintptr_t)l.rom_fs, FALSE );

	if( 0 )
	{
		if( !( l.core_fs = sack_vfs_load_volume( ExpandPath( "*/asset.svfs" ) ) ) )
		{
			MessageBox( NULL, "Failed to load application data.", "Failed Initialization", MB_OK );
			exit(0);
		}
		l.core_mount = sack_mount_filesystem( "sack_shmem", sack_get_filesystem_interface( "sack_shmem.runner" ), 900, (uintptr_t)l.core_fs, TRUE );
	}

#if 0
	if( !(l.resource_fs = resload( CMAKE_BUILD_TYPE, CPACK_PACKAGE_VERSION_PATCH ) ) )
	{
		//SimpleMessageBox( NULL, "Failed VFS", "Failed to load virtual file system" );
		exit(0);
	}
	l.resource_mount = sack_mount_filesystem( "resource_vfs"
														 , l.fsi
														 , 950
														 , (uintptr_t)l.resource_fs, 0 );
#endif

#ifndef _DEBUG
	//LoadFunction( "dl.dll", NULL );
	//LoadFunction( "libdl.dll", NULL );
#endif
#ifdef _WIN32
   if( 0 )
	{
		HMODULE hMod;
		if( !( hMod = LoadLibrary( "openAL32.dll" ) ) )
		{
			LOGICAL finished = FALSE;
			TEXTSTR path = ExpandPath( "*/oalinst.exe" );
			CTEXTSTR args[3];
			FILE *oalinst = sack_fopen( 0, "oalinst.exe", "rb" );
			if( oalinst )
			{
				FILE *output = sack_fopenEx( 0, path, "wb", sack_get_default_mount() );
				if( output )
				{
					size_t size = sack_fsize( oalinst );
					POINTER p = Allocate( size );
					sack_fread( p, 1, size, oalinst );
					sack_fwrite( p, 1, size, output );
					sack_fclose( output );
				}
				sack_fclose( oalinst );
			}
			args[0] = path;
			args[1] = "/silent";
			args[2] = NULL;
			LaunchProgramEx( args[0], NULL, args, InstallFinished, (uintptr_t)&finished );
			while( !finished )
				WakeableSleep( 1000 );
			if( !( hMod = LoadLibrary( "openAL32.dll" ) ) )
			{
				MessageBox( NULL, "Failed to load OpenAL", "Fatal Error", MB_OK );
				//printf( "Failed to load Open AL\n" );
				return;
			}
		}
		FreeLibrary( hMod );
	}
#else
#error need different DLL name
#endif
	if( l.rom_fs )
	{
#ifdef __GNUC__
#  ifdef WIN32
		l.entry_point = (int(CPROC*)(int argc, char**argv, int bConsole
											  ,struct volume* (CPROC *load)( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
											  ,void (CPROC*unload)(struct volume *)
											  ))LoadFunction( "intershell.core", "Main" );
#  else
		l.entry_point = (int(CPROC*)(int argc, char**argv, int bConsole
											  ,struct volume* (CPROC *load)( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
											  ,void (CPROC*unload)(struct volume *)
											  ))LoadFunction( "libintershell.core.so", "Main" );
#  endif
#else
		l.entry_point = (int(CPROC*)(int argc, char**argv, int bConsole
											  ,struct volume* (CPROC *load)( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
											  ,void (CPROC*unload)(struct volume *)
											  ))LoadFunction( "intershell.core", "Main" );
#endif
	}
   //return 0;
}


SaneWinMain(argc,argv)
{
	if( l.entry_point )
		return l.entry_point(argc, argv, FALSE,  sack_vfs_load_crypt_volume, sack_vfs_unload_volume );
	MessageBox( NULL, "Failed to get program entrypoint", "Critical Error", MB_OK );
	return 0;
}
EndSaneWinMain()


