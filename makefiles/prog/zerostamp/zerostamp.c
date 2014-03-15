#define BYTE unsigned char
#define WORD unsigned short
#define LONG signed long
#define DWORD unsigned long

#include <stdio.h>

#ifdef UNICODE
#error WIDE NOT DEFINED
#else
#define WIDE(s) s
#endif

#define IMAGE_DOS_SIGNATURE 0x5A4D

typedef struct _IMAGE_DOS_HEADER {
	WORD e_magic;
	WORD e_cblp;
	WORD e_cp;
	WORD e_crlc;
	WORD e_cparhdr;
	WORD e_minalloc;
	WORD e_maxalloc;
	WORD e_ss;
	WORD e_sp;
	WORD e_csum;
	WORD e_ip;
	WORD e_cs;
	WORD e_lfarlc;
	WORD e_ovno;
	WORD e_res[4];
	WORD e_oemid;
	WORD e_oeminfo;
	WORD e_res2[10];
	LONG e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;

#define IMAGE_NT_SIGNATURE 0x00004550

typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;
typedef struct _IMAGE_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Reserved1;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
	BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	} Misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD NumberOfRelocations;
	WORD NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	union {
		struct {
			DWORD NameOffset:31;
			DWORD NameIsString:1;
		};
		DWORD Name;
		WORD Id;
	};
	union {
		DWORD OffsetToData;
		struct {
			DWORD OffsetToDirectory:31;
			DWORD DataIsDirectory:1;
		};
	};
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	WORD NumberOfNamedEntries;
	WORD NumberOfIdEntries;
   IMAGE_RESOURCE_DIRECTORY_ENTRY entries[]; 
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;

typedef struct _IMAGE_NT_HEADERS {
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;

void FixResourceDirEntry( char *resources, PIMAGE_RESOURCE_DIRECTORY pird )
{
	int x;
	pird->TimeDateStamp = 0;
	for( x = 0; x < pird->NumberOfNamedEntries + 
	                pird->NumberOfIdEntries; x++ )
	{
		//printf( WIDE("DirectoryEntry: %08x %08x\n"), pird->entries[x].Name, pird->entries[x].OffsetToData );
		if( pird->entries[x].DataIsDirectory )
		{
			FixResourceDirEntry( resources
									, (PIMAGE_RESOURCE_DIRECTORY)(resources 
												+ pird->entries[x].OffsetToDirectory) );
		}
	}
}


int main( int argc, char **argv )
{
	FILE *file;
	int n;
	IMAGE_DOS_HEADER dos_header;
	IMAGE_NT_HEADERS nt_header;
	for( n = 1; n < argc; n++ )
	{
		file = fopen( argv[n], WIDE("rb+") );
		if( file )
		{
			unsigned long optional_header_pos;
			fread( &dos_header, 1, sizeof( dos_header ), file );
			if( dos_header.e_magic != IMAGE_DOS_SIGNATURE )
			{
				fprintf( stderr, WIDE("File is not a program.\n") );
				fclose( file );
				return 1;
				continue;
			}
			fseek( file, dos_header.e_lfanew, SEEK_SET );
         // sizeof(nt_header)
			fread( &nt_header, sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER ), 1, file );
			optional_header_pos = ftell( file );
			if( nt_header.Signature != IMAGE_NT_SIGNATURE )
			{
				fprintf( stderr, WIDE("File is not a 32 bit windows program.\n") );
				fclose( file );
				return 1;
				continue;
			}
			if( nt_header.FileHeader.TimeDateStamp != 0 )
			{
				nt_header.FileHeader.TimeDateStamp = 0;
				fseek( file, dos_header.e_lfanew, SEEK_SET );
				fwrite( &nt_header, sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER ), 1, file );
			}
			else
				fprintf( stderr, WIDE("Time stamp already reset.\n") );

			//printf( WIDE("Optional Header : %d\n"), nt_header.FileHeader.SizeOfOptionalHeader );
         if( nt_header.FileHeader.SizeOfOptionalHeader )
			{
            fseek( file, optional_header_pos, SEEK_SET );
				fread( &nt_header.OptionalHeader, nt_header.FileHeader.SizeOfOptionalHeader, 1, file );
				printf( WIDE("Program version was %d.%d (is now %d.%d)\n")
						, nt_header.OptionalHeader.MajorOperatingSystemVersion
						, nt_header.OptionalHeader.MinorOperatingSystemVersion
						, 4, 0
						);
				nt_header.OptionalHeader.MajorOperatingSystemVersion = 4;
				nt_header.OptionalHeader.MinorOperatingSystemVersion = 0;
				fseek( file, optional_header_pos, SEEK_SET );
            fwrite( &nt_header.OptionalHeader, nt_header.FileHeader.SizeOfOptionalHeader, 1, file );
			}
			// track down and kill resources.
			{
				int n;
				long FPISections = dos_header.e_lfanew 
				                 + sizeof( nt_header ) 
				                 + nt_header.FileHeader.SizeOfOptionalHeader;
				IMAGE_SECTION_HEADER section;
				fseek( file, FPISections, SEEK_SET );
				for( n = 0; n < nt_header.FileHeader.NumberOfSections; n++ )
				{
					fread( &section, 1, sizeof( section ), file );
					if( strcmp( section.Name, WIDE(".rsrc") ) == 0 )
					{
						IMAGE_RESOURCE_DIRECTORY *ird;
						// Resources begin here....
						char *data;
						data = malloc( section.SizeOfRawData );
						fseek( file, section.PointerToRawData, SEEK_SET );
						fread( data, 1, section.SizeOfRawData, file );
						FixResourceDirEntry( data, (IMAGE_RESOURCE_DIRECTORY *)data );
						fseek( file, section.PointerToRawData, SEEK_SET );
						fwrite( data, 1, section.SizeOfRawData, file );
						free( data );
					}
					/*
					printf( WIDE("%*.*s\n")
								, IMAGE_SIZEOF_SHORT_NAME
								, IMAGE_SIZEOF_SHORT_NAME
								, section.Name );
					*/
				}
			}


			fclose( file );
		}
		else
		{
			fprintf( stderr, WIDE("Failed to open %s"), argv[n] );
			return 1;
		}	
	}
	return 0;
}
