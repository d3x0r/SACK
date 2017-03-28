#include <stdhdrs.h>
#include <deadstart.h>

#ifdef WIN32
#include <tlhelp32.h>
#include <psapi.h>
#endif

#ifdef __LINUX__
#include <elf.h>
#endif

#include "self_compare.h"

#define Seek(a,b) (((uintptr_t)a)+(b))

#ifdef WIN32
# ifndef IMAGE_REL_BASED_ABSOLUTE
#    define  IMAGE_REL_BASED_ABSOLUTE 0
#  endif
#  ifndef IMAGE_REL_BASED_DIR64
#    define IMAGE_REL_BASED_DIR64 10
#  endif

uintptr_t ConvertVirtualToPhysical( PIMAGE_SECTION_HEADER sections, int nSections, uintptr_t base )
{
	int n;
	for( n = 0; n < nSections; n++ )
	{
		if( base >= sections[n].VirtualAddress && base < sections[n].VirtualAddress + sections[n].SizeOfRawData )
			return base - sections[n].VirtualAddress + sections[n].PointerToRawData;
	}
	return 0;
}


/*
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
*/

/* This returns the entry point to the library 
maybe it returns the library base... */
POINTER ScanLoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, LOGICAL (CPROC*Callback)(CTEXTSTR library) )
{
	static int generation;
	uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;
	static int level;
	//if( level == 0 )
	//	generation++;
	level++;
#ifdef DEBUG_LIBRARY_LOADING
	lprintf( "Load %s (%d:%d)\n", name, generation, level );
#endif
	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
			//lprintf( "Basic signature check failed; not a library" );
			return NULL;

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE )
			//lprintf( "Basic NT signature check failed; not a library" );
			return NULL;

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				//lprintf( "Optional header signature is incorrect... %x = %x",source_nt_header->OptionalHeader.Magic, IMAGE_NT_OPTIONAL_HDR_MAGIC );
				return NULL;
			}

#ifdef DEBUG_LIBRARY_LOADING
			lprintf( "Base is %p  %d"
						, source_nt_header->OptionalHeader.ImageBase
						, source_nt_header->OptionalHeader.FileAlignment
						);

			lprintf( WIDE("Program version was %d.%d (is now %d.%d)")
						, source_nt_header->OptionalHeader.MajorOperatingSystemVersion
						, source_nt_header->OptionalHeader.MinorOperatingSystemVersion
						, 4, 0
						);
#endif
		}
      // had to have this externally anyway.
		//if( StrCaseCmp( name, "MSVCR110.dll" ) == 0 )
      //   return 0;
		{
			int n;
			PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section = NULL;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			// compute size of total of sections
			// mark a few known sections for later processing
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".text"), sizeof( source_section[n].Name ) ) == 0 )
				{
					source_text_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".idata"), sizeof( source_section[n].Name ) ) == 0 )
				{
					source_import_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".rdata"), sizeof( source_section[n].Name ) ) == 0 )
				{
				}
			}


			// ------------- Go through the sections and move to expected virtual offsets
			//if( source_import_section )
			//	real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_import_section->PointerToRawData );
			//else
			{
				uintptr_t source_address = ConvertVirtualToPhysical( source_section, source_nt_header->FileHeader.NumberOfSections, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
				real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_address );
			}

			if( Callback && real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int n;
				// as long as charaacteristics is non zero it's not the end of the list.
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name;
#ifdef DEBUG_LIBRARY_LOADING
					lprintf( "dll anme from %p %p %p", real_import_base, real_import_base[n].Name, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
#endif
					dll_name = (const char*) Seek( real_import_base, real_import_base[n].Name - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress/*source_import_section->VirtualAddress*/ );
#ifdef DEBUG_LIBRARY_LOADING
					lprintf( "is %s loaded?", dll_name );
#endif
					if( !IsMappedLibrary( dll_name ) )
					{
#ifdef DEBUG_LIBRARY_LOADING
						lprintf( "need to preload %s", dll_name );
#endif
						Callback( dll_name );
					}
				}
			}

		}
	}
	level--;
	return 0;
}


void DumpSystemMemory( POINTER p_match )
{
	{
		DWORD nHeaps;
		HANDLE pHeaps[1024];
		HANDLE hHeap = GetProcessHeap();
		PROCESS_HEAP_ENTRY entry;
		size_t total = 0;
		DWORD nHeap;
		DWORD n = 0;
		nHeaps = GetProcessHeaps( 1024, pHeaps );
		{
			size_t n = 256;
			HMODULE *modules = NewArray( HMODULE, 256 );
			DWORD needed = 0;
			//l.EnumProcessModules( GetCurrentProcess(), modules, sizeof( HMODULE ) * 256, &needed );
			if( needed / sizeof( HMODULE ) == n )
				lprintf( "loaded module overflow" );
			needed /= sizeof( HMODULE );
			for( n = 0; n < needed; n++ )
			{
				POINTER real_memory = modules[n];
				PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)real_memory;
				PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( real_memory, source_dos_header->e_lfanew );
				PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
				PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );
				char *dll_name = (char*) Seek( real_memory, exp_dir->Name );
				MEMORY_BASIC_INFORMATION info;
				PIMAGE_IMPORT_DESCRIPTOR imp_des = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
				if( dir[IMAGE_DIRECTORY_ENTRY_TLS].Size )
					lprintf( "has TLS" );
				VirtualQueryEx( GetCurrentProcess(), real_memory, &info, sizeof( info ) );
				lprintf( "virtual block at %p %d %s", real_memory, info.RegionSize, dll_name );
				LogBinary( real_memory, 16 );
			}
		}
		for( nHeap = 0; nHeap < nHeaps; nHeap++ )
		{
			entry.lpData = NULL;
			lprintf( "Begin New Heap walk... %d(%d) %p", nHeap, nHeaps, pHeaps[nHeap] );
			while( HeapWalk( pHeaps[nHeap], &entry ) )
			{
				total += entry.cbData;
				if( !p_match || p_match == entry.lpData )
				{
					if( entry.lpData && !(entry.wFlags & 2 ) && !IsBadReadPtr( entry.lpData, 16 ) )
						LogBinary( entry.lpData, 16 );
					if( entry.lpData )
						lprintf( "heap chunk %d %d %p %d %d %08x", n++, total
									, entry.lpData
									, entry.cbData
									, entry.cbOverhead
									, entry.wFlags );
					else
					{
						lprintf( "end of data?" );
						break;
					}
				}
			}
		}
	}
}

POINTER LoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, void (*Callback)(CTEXTSTR library) )
{
/* This returns the entry point to the library 
maybe it returns the library base... */
	static int generation;
	uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;
	POINTER real_memory;
	static int level;
	//if( level == 0 )
	//	generation++;
	level++;
	//lprintf( "Load %s (%d:%d)\n", name, generation, level );
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
				lprintf( "Optional header signature is incorrect... %x = %x",source_nt_header->OptionalHeader.Magic, IMAGE_NT_OPTIONAL_HDR_MAGIC );
			}
			/*
			lprintf( "Base is %p  %d"
						, source_nt_header->OptionalHeader.ImageBase
						, source_nt_header->OptionalHeader.FileAlignment
						);

			lprintf( WIDE("Program version was %d.%d (is now %d.%d)")
						, source_nt_header->OptionalHeader.MajorOperatingSystemVersion
						, source_nt_header->OptionalHeader.MinorOperatingSystemVersion
						, 4, 0
						);
			*/
		}
		{
			int n;
			PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section = NULL;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			// compute size of total of sections
			// mark a few known sections for later processing
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".text"), sizeof( source_section[n].Name ) ) == 0 )
				{
					//image_base = (POINTER)Seek( real_memory, source_section[n].VirtualAddress );
					source_text_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".idata"), sizeof( source_section[n].Name ) ) == 0 )
				{
					//import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_section[n].PointerToRawData );
					source_import_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".rdata"), sizeof( source_section[n].Name ) ) == 0 )
				{
					//source_rdata_base = (POINTER)Seek( source_memory, source_section[n].PointerToRawData );
					//real_rdata_base = (POINTER)Seek( real_memory, source_section[n].VirtualAddress );
					//source_rdata_section = source_section;
				}
			}
			// ------------- Go through the sections and move to expected virtual offsets
			real_memory = VirtualAlloc( NULL, dwSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE );
			if( source_import_section )
				real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, source_import_section->VirtualAddress );
			else
			{
				real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
			}

			MemCpy( (POINTER)( real_memory )
				, (POINTER)( source_memory )
				, source_section[0].PointerToRawData );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				//source_section[n].VirtualAddress = Seek( real_memory, source_section[n].VirtualAddress );
				MemCpy( (POINTER)Seek( real_memory, source_section[n].VirtualAddress )
					, (POINTER)Seek( source_memory, source_section[n].PointerToRawData )
					, source_section[n].SizeOfRawData );
			}
			{
				PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );
				char *dll_name = (char*) Seek( real_memory, exp_dir->Name );
				AddMappedLibrary( dll_name, real_memory );
			}
			{
				PIMAGE_DATA_DIRECTORY dir;
				PIMAGE_IMPORT_DESCRIPTOR imp_des;
				dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
				imp_des = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
				if( imp_des != real_import_base )
				{
					lprintf( "2 import tables." );
				}
			}

			if( Callback && real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int n;
				// as long as charaacteristics is non zero it's not the end of the list.
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name;
					dll_name = (const char*) Seek( real_import_base, real_import_base[n].Name - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress/*source_import_section->VirtualAddress*/ );
					if( !IsMappedLibrary( dll_name ) )
					{
						//printf( "need to load %s\n", dll_name );
						Callback( dll_name );
					}
				}
			}

			if( real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int n;
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name = name;
					int f;
					uintptr_t *dwFunc;
					uintptr_t *dwTargetFunc;
					PIMAGE_IMPORT_BY_NAME import_desc;
					if( real_import_base[n].Name )
						dll_name = (const char*) Seek( real_import_base, real_import_base[n].Name - dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress/*source_import_section->VirtualAddress*/ );
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
							//printf( "Oridinal is %d\n", dwFunc[f] & 0xFFFF );
							dwTargetFunc[f] = (uintptr_t)LoadFunction( dll_name, (CTEXTSTR)(dwFunc[f] & 0xFFFF) );
						}
						else
						{
							import_desc = (PIMAGE_IMPORT_BY_NAME)Seek( real_memory, dwFunc[f] );
							//printf( " sub thing %s\n", import_desc->Name );
							dwTargetFunc[f] = (uintptr_t)LoadFunction( dll_name, import_desc->Name );
						}
					}
				}
			}

			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections + n * sizeof( IMAGE_SECTION_HEADER ) );
				if( StrCmpEx( (char*)source_section[0].Name, WIDE(".reloc"), sizeof( source_section[0].Name ) ) == 0 )
				{
					int num_reloc;
					DWORD section_offset = 0;
					IMAGE_BASE_RELOCATION *ibr = NULL;
					IMAGE_BASE_RELOCATION *real_ibr = NULL;
					while( section_offset < source_section[0].SizeOfRawData )
					{
						int reloc_entry;
						WORD *pb;
						ibr = (IMAGE_BASE_RELOCATION *)Seek( source_memory, source_section[0].PointerToRawData + section_offset );
						real_ibr = (IMAGE_BASE_RELOCATION *)Seek( real_memory, source_section[0].VirtualAddress + section_offset );
						
						if( !ibr->SizeOfBlock )
							break;
						num_reloc = (ibr->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
						pb = (WORD *)Seek( ibr, sizeof( IMAGE_BASE_RELOCATION ) );
						//lprintf( "Handle relocate block (%d<%d)  %d syms", section_offset, source_section[n].SizeOfRawData
						//	, num_reloc );
						//uintptr_t *pa = (uintptr_t *)Seek( real_memory, ibr->VirtualAddress ) ;
						for( reloc_entry = 0; reloc_entry < num_reloc; reloc_entry++ )
						{
							int mode = ( pb[reloc_entry] >> 12 );
							// Need to do things with real_offset
							uintptr_t *real_offset = (uintptr_t *)Seek( real_memory, ibr->VirtualAddress + ( pb[reloc_entry] & 0xFFF ) );
							uintptr_t source_address = ConvertVirtualToPhysical( (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections ), source_nt_header->FileHeader.NumberOfSections, ibr->VirtualAddress );
							uintptr_t *source_offset = (uintptr_t *)Seek( source_memory, source_address + ( pb[reloc_entry] & 0xFFF ) );
							switch( mode )
							{
							case IMAGE_REL_BASED_ABSOLUTE:
								/* unused; except for padding */
								//*((uint32_t*)real_offset) = (uint32_t)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
								//lprintf( "unused?");
								break;
							case IMAGE_REL_BASED_HIGH :
								/* unused */
								lprintf( "unused?");
								break;
							case IMAGE_REL_BASED_LOW :
								/* unused */
								lprintf( "unused?");
								break;
							case IMAGE_REL_BASED_HIGHLOW:
								*((uint32_t*)real_offset) = (uint32_t)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
								//lprintf( "update %p to %08x", real_offset, *(uint32_t*)real_offset );
								break;
							case IMAGE_REL_BASED_DIR64:
								*(real_offset) = (uintptr_t)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
								break;
							default:
								lprintf( "Failed to determine type for relocation." );
							}
						}
						section_offset += ibr->SizeOfBlock;
						//lprintf( "uhmm %08x %08x", source_section[n].VirtualAddress, source_section[n].PointerToRelocations );
					}
				}
			}

			{
				// thread local storage fixup
				PIMAGE_TLS_DIRECTORY tls = (PIMAGE_TLS_DIRECTORY)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress );
				int n;
				if( dir[IMAGE_DIRECTORY_ENTRY_TLS].Size )
				{
					lprintf( "processing TLS..." );
					for( n = 0; n < dir[IMAGE_DIRECTORY_ENTRY_TLS].Size / sizeof( IMAGE_TLS_DIRECTORY ); n++ )
					{
						POINTER data;
						DWORD dwInit;
						POINTER *tls_list = (POINTER*)&tls_list;
#ifdef __WATCOMC__
                  extern void **fn1( void );
#pragma aux fn1 = "mov eax, fs:[2ch]";
#endif
						size_t size_init = ( tls->EndAddressOfRawData - tls->StartAddressOfRawData );
						size_t size = size_init + tls->SizeOfZeroFill;
						/*
						printf( "%p %p %p(%d) %p\n"
									, tls->AddressOfCallBacks
									, tls->StartAddressOfRawData, tls->EndAddressOfRawData
									, ( tls->EndAddressOfRawData - tls->StartAddressOfRawData ) + tls->SizeOfZeroFill
									, tls->AddressOfIndex );
							*/
						data = NewArray( uint8_t, size );
#ifdef __WATCOMC__
                  tls_list = fn1();
#elif defined( _MSC_VER )
						{
#  ifdef __64__
#  else
							_asm mov ecx, fs:[2ch];
							_asm mov tls_list, ecx;
#  endif
						}
#else
//#error need assembly to get this...
#endif
						dwInit = 0;
						{
							memcpy( data, (*tls_list), size );
						DumpSystemMemory( *tls_list );
						//while( (*tls_list) ) { tls_list++; dwInit++; }
						//(*tls_list) = data;
						//DebugBreak();
						LoadLibrary( "avutil-54.dll" );
						{
							int n;
							for( n = 0; n < size; n++ )
							{
								// byte 120 differed...
								if( ((char*)data)[n] != ((char*)(*tls_list))[n] )
								{
									lprintf( "differed..." ); 
								}
							}
						}
						}
#ifdef __WATCOMC__
                  tls_list = fn1();
#elif defined( _MSC_VER )
						{
#  ifdef __64__
#  else
							_asm mov ecx, fs:[2ch];
							_asm mov tls_list, ecx;
#  endif
						}
#else
//#error need assembly to get this...
#endif
						(*((DWORD*)tls->AddressOfIndex)) = dwInit;
						//printf( "%p is %d\n", tls->AddressOfIndex, dwInit );
						//data = LocalAlloc( 0, size );
						//	035FA434  mov         ecx,dword ptr fs:[2Ch]  
						//	035FA43B  mov         edx,dword ptr [ecx+eax*4]  

						//TlsSetValue( dwInit, data );
						//printf( "%d is %p\n", dwInit, data );
						//printf( "and it was also %p\n", TlsGetValue( dwInit ));
						memcpy( data, (POINTER)tls->StartAddressOfRawData, size_init );
						memset( ((uint8_t*)data) + size_init, 0, tls->SizeOfZeroFill );
					}
				}
			}

			if( source_text_section )
			{
				DWORD old_protect;
				if( !VirtualAlloc( (POINTER)Seek( real_memory, source_text_section->VirtualAddress )
					, source_text_section->SizeOfRawData
					, MEM_COMMIT
					, PAGE_EXECUTE_READ ) )
				{
					lprintf( "Couldn't commit written pages : %d", GetLastError() );
				}
					  

				if( !VirtualProtect( (POINTER)Seek( real_memory, source_text_section->VirtualAddress )
					, source_text_section->SizeOfRawData
					, PAGE_EXECUTE_READ
					, &old_protect ) )
				{
					lprintf( "bad protect: %d", GetLastError() );
				}
			}
		}
		// run entry point.
		if( library )
		{
			void(WINAPI*entry_point)(void*, DWORD, void*) = (void(WINAPI*)(void*,DWORD,void*))Seek( real_memory, source_nt_header->OptionalHeader.AddressOfEntryPoint );
			if( entry_point )
			{
				printf( "Library Entry:%s\n", name );
				entry_point(real_memory, DLL_PROCESS_ATTACH, NULL );
				printf( "Library Entry Done:%s\n", name );
			}
		}
		else
		{
			void(WINAPI*entry_point)(HINSTANCE,HINSTANCE,LPSTR,int) = (void(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))Seek( real_memory, source_nt_header->OptionalHeader.AddressOfEntryPoint );
			return entry_point;
		}
		//AddMappedLibrary( name, real_memory );
	}
	level--;
	return 0;
}

#ifdef COMPILE_TEST
static void CompareSelf( void )
{
	uintptr_t source_memory_length = 0;
	void (*f)(void);
	POINTER source_memory = OpenSpace( NULL, "self_secure.module", &source_memory_length );
	LoadLibraryFromMemory( "self_secure.module", source_memory, source_memory_length );
	f = (void(*)(void))LoadFunction( "self_secure.module", "ResolveThisSymbol" );
	if( f )
		f();
}
#endif
#endif


#ifdef COMPILE_TEST
PRIORITY_PRELOAD( NetworkLoaderTester, NAMESPACE_PRELOAD_PRIORITY + 1 )
{
	//CompareSelf();
}
#endif

#if __WATCOMC__ < 1200
PUBLIC( void, StupidUselessWatcomExport )( void )
{
}

#endif

#ifdef __LINUX__

#define PAGE_MASK 0xFFF

#ifdef __64__
#  define ElfN_Ehdr Elf64_Ehdr
#  define ElfN_Phdr Elf64_Phdr
#else
#  define ElfN_Ehdr Elf32_Ehdr
#  define ElfN_Phdr Elf32_Phdr
#endif


POINTER LoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, void (*Callback)(CTEXTSTR library) )
{
	ElfN_Ehdr *elf = (ElfN_Ehdr*)block;
	ElfN_Phdr *p;
   void *realMemory;
   int n;
   POINTER entry_point;
	if( ( elf->e_ident[EI_MAG0] != ELFMAG0 )
	  || ( elf->e_ident[EI_MAG1] != ELFMAG1 )
	  || ( elf->e_ident[EI_MAG2] != ELFMAG2 )
	  || ( elf->e_ident[EI_MAG3] != ELFMAG3 ) )
		return NULL;
#if defined( __64__ )
	if( elf->e_ident[EI_CLASS] != ELFCLASS64 )
		return NULL;
#else
	if( elf->e_ident[EI_CLASS] != ELFCLASS32 )
		return NULL;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	if( elf->e_ident[EI_DATA] != ELFDATA2LSB )
		return NULL;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	if( elf->e_ident[EI_DATA] != ELFDATA2MSB )
		return NULL;
#endif
	switch( elf->e_type )
	{
	case ET_REL:
		lprintf( "relocatable file" );
		break;
	case ET_EXEC:
		lprintf( "executable (different entry point arguments)" );
		break;
	case ET_DYN:
		lprintf( "shared object (different entry point arguments)" );
		break;
	case ET_CORE:
		lprintf( "unsupported: core file" );
		return NULL;
	default:
		lprintf( "unsupported: unknown object type" );
		return NULL;
	}
	entry_point = (POINTER)( (uintptr_t)elf->e_entry + (uintptr_t)block );
	p = (ElfN_Phdr*)( (uintptr_t)block + elf->e_phoff );
	// transfer code in memory-file to virtual address space.
	{
		size_t minVirt;
		size_t newSize;
		size_t maxVirt = 0;
      void *realMemory;
		for( n = 0; n < elf->e_phnum; n++ )
		{
			switch( p->p_type )
			{
			case PT_INTERP:
            // cannot load a program; already have a program.
            return NULL;
			case PT_LOAD:
				{
					if( n == 0 )
						minVirt = p[n].p_vaddr;
					else
						if( p[n].p_vaddr < minVirt )
                     minVirt = p[n].p_vaddr;
					newSize = p[n].p_vaddr + p[n].p_memsz;
					if( newSize > maxVirt )
                  maxVirt = newSize;
					break;
				}
			}
		}
#if 0
		realMemory = mmap( NULL
							  , ( maxVirt - minVirt ) + ( minVirt & PAGE_MASK )
							  , 
							  , MAP_PRIVATE | MAP_ANONYMOUS
							  , -1, 0 );

		for( n = 0; n < elf->e_phnum; n++ )
		{
			switch( p->p_type )
			{
			case PT_LOAD:
				{
					mmap( ( (uintptr_t)realMemory + ( p[n].p_vaddr - minVirt ) ) & ~PAGE_MASK
						 , p[n].p_memsz + ( ( (uintptr_t)realMemory + ( p[n].p_vaddr - minVirt ) ) & PAGE_MASK )
						 , p[n].p_flags
						 , MAP_PRIVATE | MAP_ANONYMOUS
						 , -1, 0 );
					memcpy( (void*)((uintptr_t)realMemory + ( p[n].p_vaddr - minVirt ) )
							, (void*)((uintptr_t)block + p[n].p_offset )
							, p[n].p_filesz );
					break;
				}
			}
		}
		munmap( realMemory, maxVirt - minVirt );
#endif
	}
	// do relocations as required
	{

	}

}
#endif