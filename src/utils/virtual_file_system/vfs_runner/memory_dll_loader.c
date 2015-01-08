#include <stdhdrs.h>
#include <deadstart.h>

#include "self_compare.h"

#define Seek(a,b) (((PTRSZVAL)a)+(b))

#ifdef WIN32
#ifndef IMAGE_REL_BASED_ABSOLUTE
#define  IMAGE_REL_BASED_ABSOLUTE 0
#endif
#ifndef IMAGE_REL_BASED_DIR64
#define IMAGE_REL_BASED_DIR64 10
#endif

PTRSZVAL ConvertVirtualToPhysical( PIMAGE_SECTION_HEADER sections, int nSections, PTRSZVAL base )
{
	int n;
	for( n = 0; n < nSections; n++ )
	{
		if( base >= sections[n].VirtualAddress && base < sections[n].VirtualAddress + sections[n].SizeOfRawData )
			return base - sections[n].VirtualAddress + sections[n].PointerToRawData;
	}
	return 0;
}


POINTER GetExtraData( POINTER block )
{
	//PTRSZVAL source_memory_length = block_len;
	POINTER source_memory = block;
	POINTER real_memory;

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
			PIMAGE_IMPORT_DESCRIPTOR import_base;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_import_section;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			PTRSZVAL dwSize = 0;
			PTRSZVAL newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
}

/* This returns the entry point to the library 
maybe it returns the library base... */
POINTER LoadLibraryFromMemory( CTEXTSTR name, POINTER block, size_t block_len, int library, void (*Callback)(CTEXTSTR library) )
{
	PTRSZVAL source_memory_length = block_len;
	POINTER source_memory = block;
	POINTER real_memory;
	static int level;
	level++;
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
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section;
			PIMAGE_IMPORT_DESCRIPTOR import_base;
			PIMAGE_IMPORT_DESCRIPTOR real_import_base;
			PIMAGE_SECTION_HEADER source_export_section;
			PIMAGE_SECTION_HEADER source_import_section;
			PIMAGE_SECTION_HEADER source_text_section = NULL;
			PTRSZVAL dwSize = 0;
			PTRSZVAL newSize;
			PIMAGE_DATA_DIRECTORY dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
			PIMAGE_EXPORT_DIRECTORY exp_dir = (PIMAGE_EXPORT_DIRECTORY)dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			// compute size of total of sections
			// mark a few known sections for later processing
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].VirtualAddress) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
				if( (DWORD)exp_dir >= source_section[n].VirtualAddress 
					&& (DWORD)exp_dir < ( source_section[n].VirtualAddress + source_section[n].SizeOfRawData ) )
				{
					source_export_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".text"), sizeof( source_section[n].Name ) ) == 0 )
				{
					//image_base = (POINTER)Seek( real_memory, source_section[n].VirtualAddress );
					source_text_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".idata"), sizeof( source_section[n].Name ) ) == 0 )
				{
					import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_section[n].PointerToRawData );
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
			real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, source_import_section->VirtualAddress );

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
#if 0
			//------------- Done copying sections into system-block bounded things
				{
					PIMAGE_DATA_DIRECTORY dir;
					PIMAGE_EXPORT_DIRECTORY exp_dir;
					int n;
					int ord;
					dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
					exp_dir = (PIMAGE_EXPORT_DIRECTORY)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress );
					//if( ( ord = ((PTRSZVAL)funcname & 0xFFFF ) ) == (PTRSZVAL)funcname )
					{
						char *dll_name = (char*) Seek( real_memory, exp_dir->Name );
						void (**f)(void) = (void (**)(void))Seek( real_memory, exp_dir->AddressOfFunctions );
						char **names = (char**)Seek( real_memory, exp_dir->AddressOfNames );
						_16 *ords = (_16*)Seek( real_memory, exp_dir->AddressOfNameOrdinals );
						int n;
						for( n = 0; n < exp_dir->NumberOfFunctions; n++ )
						{
							//char *name = (char*)Seek( real_memory, names[n] );
							//f[n] = ((PTRSZVAL)f[n] + source_export_section->VirtualAddress
						}
					}
					//else
						for( n = 0; n < exp_dir->NumberOfFunctions; n++ )
						{
						}
				}
#endif
				{
					PIMAGE_DATA_DIRECTORY dir;
					PIMAGE_IMPORT_DESCRIPTOR imp_des;
					dir = (PIMAGE_DATA_DIRECTORY)source_nt_header->OptionalHeader.DataDirectory;
					imp_des = (PIMAGE_IMPORT_DESCRIPTOR)Seek( real_memory, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
					if( imp_des != real_import_base )
					{
						lprintf( "2 import tables." );
					}
					//if( ( ord = ((PTRSZVAL)funcname & 0xFFFF ) ) == (PTRSZVAL)funcname )
				}



			if( Callback && real_import_base )
			{
				int n;
				// as long as charaacteristics is non zero it's not the end of the list.
				SuspendDeadstart();
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					char * dll_name = (char*) Seek( import_base, import_base[n].Name - source_import_section->VirtualAddress );
					if( !IsMappedLibrary( dll_name ) )
						Callback( dll_name );
				}
				ResumeDeadstart();
			}

			if( import_base )
			{
				int n;
				for( n = 0; import_base[n].Characteristics; n++ )
				{
					char * dll_name = (char*) Seek( import_base, import_base[n].Name - source_import_section->VirtualAddress );
					int f;
					PTRSZVAL *dwFunc;
					PTRSZVAL *dwTargetFunc;
					PIMAGE_IMPORT_BY_NAME import_desc;
					//char * function_name = (char*) Seek( import_base, import_base[n]. - source_import_section->VirtualAddress );
					//lprintf( "thing %s", dll_name );
#ifdef __WATCOMC__
					dwFunc = (PTRSZVAL*)Seek( import_base, import_base[n].OrdinalFirstThunk - source_import_section->VirtualAddress );
#else
					dwFunc = (PTRSZVAL*)Seek( import_base, import_base[n].OriginalFirstThunk - source_import_section->VirtualAddress );
#endif
					dwTargetFunc = (PTRSZVAL*)Seek( real_import_base, import_base[n].FirstThunk - source_import_section->VirtualAddress );
					for( f = 0; dwFunc[f]; f++ )
					{
						if( dwFunc[f] & ( (PTRSZVAL)1 << ( ( sizeof( PTRSZVAL ) * 8 ) - 1 ) ) )
						{
							//lprintf( "Oridinal is %d", dwFunc[f] & 0xFFFF );
							dwTargetFunc[f] = (PTRSZVAL)LoadFunction( dll_name, (CTEXTSTR)(dwFunc[f] & 0xFFFF) );
						}
						else
						{
							import_desc = (PIMAGE_IMPORT_BY_NAME)Seek( import_base, dwFunc[f] - source_import_section->VirtualAddress );
							//lprintf( " sub thing %s", import_desc->Name );
							dwTargetFunc[f] = (PTRSZVAL)LoadFunction( dll_name, import_desc->Name );
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
						//PTRSZVAL *pa = (PTRSZVAL *)Seek( real_memory, ibr->VirtualAddress ) ;
						for( reloc_entry = 0; reloc_entry < num_reloc; reloc_entry++ )
						{
							int mode = ( pb[reloc_entry] >> 12 );
							// Need to do things with real_offset
							PTRSZVAL *real_offset = (PTRSZVAL *)Seek( real_memory, ibr->VirtualAddress + ( pb[reloc_entry] & 0xFFF ) );
							PTRSZVAL source_address = ConvertVirtualToPhysical( (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections ), source_nt_header->FileHeader.NumberOfSections, ibr->VirtualAddress );
							PTRSZVAL *source_offset = (PTRSZVAL *)Seek( source_memory, source_address + ( pb[reloc_entry] & 0xFFF ) );
							switch( mode )
							{
							case IMAGE_REL_BASED_ABSOLUTE:
								/* unused; except for padding */
								//*((_32*)real_offset) = (_32)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
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
								*((_32*)real_offset) = (_32)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
								//lprintf( "update %p to %08x", real_offset, *(_32*)real_offset );
								break;
							case IMAGE_REL_BASED_DIR64:
								*(real_offset) = (PTRSZVAL)Seek( real_memory, (*real_offset - source_nt_header->OptionalHeader.ImageBase) );
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
			if( source_text_section )
			{
				DWORD old_protect;
				if( !VirtualAlloc( (POINTER)Seek( real_memory, source_text_section->VirtualAddress )
					, source_text_section->SizeOfRawData
					, MEM_COMMIT
					,  PAGE_EXECUTE_READ ) )
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
				entry_point(real_memory, DLL_PROCESS_ATTACH, NULL );
			}
		}
		else
		{
			void(WINAPI*entry_point)(HINSTANCE,HINSTANCE,LPSTR,int) = (void(WINAPI*)(HINSTANCE,HINSTANCE,LPSTR,int))Seek( real_memory, source_nt_header->OptionalHeader.AddressOfEntryPoint );
			if( entry_point )
			{

				SuspendDeadstart();
				if( level == 1 )
					ResumeDeadstart();
				{
					void (*f)(void) = (void(*)(void))LoadFunction( "bag.dll", "InvokeDeadstart" );
					if( f )
						f();
				}
				entry_point(real_memory, real_memory, "program.exe", 0 );
			}
		}
		AddMappedLibrary( name, real_memory );
	}
	level--;
	return 0;
}

#ifdef COMPILE_TEST
static void CompareSelf( void )
{
	PTRSZVAL source_memory_length = 0;
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

PUBLIC( void, StupidUselessWatcomExport )( void )
{
}
