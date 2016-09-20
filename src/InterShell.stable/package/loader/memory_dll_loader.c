#include <stdhdrs.h>
#include <deadstart.h>

#ifdef WIN32
#include <tlhelp32.h>
#include <Psapi.h>
#endif


#include "self_compare.h"

//#define DEBUG_LIBRARY_LOADING

#define Seek(a,b) (((uintptr_t)a)+(b))

#ifdef WIN32
#ifndef IMAGE_REL_BASED_ABSOLUTE
#define  IMAGE_REL_BASED_ABSOLUTE 0
#endif
#ifndef IMAGE_REL_BASED_DIR64
#define IMAGE_REL_BASED_DIR64 10
#endif

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


POINTER GetExtraData( POINTER block )
{
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

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
				//lprintf( "Optional header signature is incorrect..." );
				return NULL;
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
			//printf( "size is %d (%08x)\n", dwSize, dwSize );
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
}


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
	//printf( "Load %s (%d:%d)\n", name, generation, level );
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
					source_text_section = source_section + n;
				}
				if( StrCmpEx( (char*)source_section[n].Name, WIDE(".idata"), sizeof( source_section[n].Name ) ) == 0 )
				{
					//source_import_section = source_section + n;
				}
				//if( StrCmpEx( (char*)source_section[n].Name, WIDE(".rdata"), sizeof( source_section[n].Name ) ) == 0 )
				//{
				//}
			}


			// ------------- Go through the sections and move to expected virtual offsets
			if( source_import_section )
				real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_import_section->VirtualAddress );
			else
			{
				uintptr_t source_address = ConvertVirtualToPhysical( source_section, source_nt_header->FileHeader.NumberOfSections, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
				real_import_base = (PIMAGE_IMPORT_DESCRIPTOR)Seek( source_memory, source_address );
			}

			if( Callback && real_import_base &&  dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size )
			{
				int entries = dir[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof( real_import_base[0] );
				int n;
				// as long as charaacteristics is non zero it's not the end of the list.
				for( n = 0; real_import_base[n].Characteristics; n++ )
				{
					const char * dll_name;
					//lprintf( "dll anme from %p %p %p", real_import_base, real_import_base[n].Name, dir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress );
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


#endif


#ifdef COMPILE_TEST
PRIORITY_PRELOAD( NetworkLoaderTester, NAMESPACE_PRELOAD_PRIORITY + 1 )
{
	//CompareSelf();
}
#endif

#ifndef NO_EXPORTS
#  if __WATCOMC__ < 1200
PUBLIC( void, StupidUselessWatcomExport )( void )
{
}
#  endif
#endif

