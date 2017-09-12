#define NO_UNICODE_C
//#define DEBUG_DISCOVERY
//#define BYTE unsigned char
//#define WORD unsigned short
//#define LONG signed long
//#define DWORD unsigned long
#include <stdhdrs.h>
#include <stdio.h>
#include <sharemem.h>
#include <filesys.h>
#include "pcopy.h"

GLOBAL g;

#define MY_IMAGE_DOS_SIGNATURE 0x5A4D

typedef struct MY_IMAGE_DOS_HEADER {
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
} MY_IMAGE_DOS_HEADER,*PMY_IMAGE_DOS_HEADER;

#define MY_IMAGE_NT_SIGNATURE 0x00004550

typedef struct MY_IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} MY_IMAGE_FILE_HEADER, *PMY_IMAGE_FILE_HEADER;

#define MY_IMAGE_SIZEOF_SHORT_NAME 8

typedef struct MY_IMAGE_SECTION_HEADER {
	BYTE Name[MY_IMAGE_SIZEOF_SHORT_NAME];
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
} MY_IMAGE_SECTION_HEADER,*PMY_IMAGE_SECTION_HEADER;

typedef struct MY_IMAGE_IMPORT_BY_NAME {
	WORD Hint;
	BYTE Name[1];
} MY_IMAGE_IMPORT_BY_NAME,*PMY_IMAGE_IMPORT_BY_NAME;

#define DUMMYUNIONNAME
#ifndef _ANONYMOUS_UNION
#define _ANONYMOUS_UNION
#endif
typedef struct MY_IMAGE_IMPORT_DESCRIPTOR {
	_ANONYMOUS_UNION union {
		DWORD Characteristics;
		DWORD OriginalFirstThunk;
	} DUMMYUNIONNAME;
	DWORD TimeDateStamp;
	DWORD ForwarderChain;
	DWORD Name;
	DWORD FirstThunk;
} MY_IMAGE_IMPORT_DESCRIPTOR,*PMY_IMAGE_IMPORT_DESCRIPTOR;

typedef struct MY_IMAGE_IMPORT_LOOKUP_TABLE {
	union {
		struct {
			DWORD NameOffset:31;
			DWORD NameIsString:1;
		};
		DWORD Name;
		WORD Id;
	};
} MY_IMAGE_IMPORT_LOOKUP_TABLE;

typedef struct MY_IMAGE_RESOURCE_DIRECTORY_ENTRY {
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
} MY_IMAGE_RESOURCE_DIRECTORY_ENTRY,*PMY_IMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct MY_IMAGE_RESOURCE_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	WORD NumberOfNamedEntries;
	WORD NumberOfIdEntries;
   MY_IMAGE_RESOURCE_DIRECTORY_ENTRY entries[]; 
} MY_IMAGE_RESOURCE_DIRECTORY,*PMY_IMAGE_RESOURCE_DIRECTORY;


typedef struct MY_IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} MY_IMAGE_DATA_DIRECTORY,*PMY_IMAGE_DATA_DIRECTORY;

#define MY_IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

#ifdef _MSC_VER
#pragma pack(push, 4)
#endif
typedef PREFIX_PACKED struct MY_IMAGE_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	//------ preceed is 'standard fields'
	union {
		struct {
			DWORD BaseOfData; /* not used in PE32+ */
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
			MY_IMAGE_DATA_DIRECTORY DataDirectory[MY_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
		} PE32;
		struct {
			uint64_t ImageBase;
			DWORD SectionAlignment;
			DWORD FileAlignment;
			WORD MajorOperatingSystemVersion;
			WORD MinorOperatingSystemVersion;
			WORD MajorImageVersion;
			WORD MinorImageVersion;
			WORD MajorSubsystemVersion;
			WORD MinorSubsystemVersion;
			DWORD Win32VersionValue;
			DWORD SizeOfImage;
			DWORD SizeOfHeaders;
			DWORD CheckSum;
			// -- windows specific fieds preceed...
			WORD Subsystem;
			WORD DllCharacteristics;
			uint64_t SizeOfStackReserve;
			uint64_t SizeOfStackCommit;
			uint64_t SizeOfHeapReserve;
			uint64_t SizeOfHeapCommit;
			DWORD LoaderFlags;  // obsolete
			DWORD NumberOfRvaAndSizes;
			MY_IMAGE_DATA_DIRECTORY DataDirectory[MY_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
		} PE32_plus;
		struct {
			uint64_t ImageBase;
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
			// -- windows specific fieds preceed...
			WORD Subsystem;
			WORD DllCharacteristics;
			uint64_t SizeOfStackReserve;
			uint64_t SizeOfStackCommit;
			uint64_t SizeOfHeapReserve;
			uint64_t SizeOfHeapCommit;
			DWORD LoaderFlags;  // obsolete
			DWORD NumberOfRvaAndSizes;
			MY_IMAGE_DATA_DIRECTORY DataDirectory[MY_IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
		} PE64;
	};
}  MY_IMAGE_OPTIONAL_HEADER,*PMY_IMAGE_OPTIONAL_HEADER;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct MY_IMAGE_NT_HEADERS {
	DWORD Signature;
	MY_IMAGE_FILE_HEADER FileHeader;
	MY_IMAGE_OPTIONAL_HEADER OptionalHeader;
} MY_IMAGE_NT_HEADERS,*PMY_IMAGE_NT_HEADERS;

#define num64Machines (sizeof(_64bitMachines)/sizeof(_64bitMachines[0]))
WORD _64bitMachines[] = {
	0x8664,   /*IMAGE_FILE_MACHINE_AMD64*/
	0x0200,   /*IMAGE_FILE_MACHINE_IA64*/
	0x0284,   /*IMAGE_FILE_MACHINE_ALPHA64*/
};


void FixResourceDirEntry( char *resources, PMY_IMAGE_RESOURCE_DIRECTORY pird )
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
									, (PMY_IMAGE_RESOURCE_DIRECTORY)(resources 
												+ pird->entries[x].OffsetToDirectory) );
		}
	}
}

static int parse64( FILE *file, PFILESOURCE pfs 
	, MY_IMAGE_DOS_HEADER *dos_header
	, MY_IMAGE_NT_HEADERS *nt_header
	) {
	MY_IMAGE_OPTIONAL_HEADER nt_optional_header;
			
	sack_fread( &nt_optional_header, nt_header->FileHeader.SizeOfOptionalHeader, 1, file );


	// track down and kill resources.
	{
		int n;
		long FPISections = dos_header->e_lfanew
			+ sizeof( nt_header->FileHeader ) + sizeof( nt_header->Signature )
			+ nt_header->FileHeader.SizeOfOptionalHeader;
		PLIST sections = NULL;
		PMY_IMAGE_SECTION_HEADER section;
		PMY_IMAGE_DATA_DIRECTORY dir = (PMY_IMAGE_DATA_DIRECTORY)nt_header->OptionalHeader.PE64.DataDirectory;
		//sack_fseek( file, FPISections, SEEK_SET );
		for( n = 0; n < nt_header->FileHeader.NumberOfSections; n++ )
		{
			section = New( MY_IMAGE_SECTION_HEADER );
			sack_fseek( file, FPISections + n * sizeof( *section ), SEEK_SET );
			sack_fread( section, 1, sizeof( *section ), file );
			AddLink( &sections, section );
			//lprintf( "Read section '%s' %d of %d", section.Name, n, nt_header->FileHeader.NumberOfSections );
			//if( ( strcmp( section.Name, ".idata" ) == 0 ) )
		}
				//cpg27dec 2006 c:\work\sack\src\utils\pcopy\pcopy.c(295): Warning! W202: Symbol 'iilt' has been defined, but not referenced
				//data = (char*)malloc( section.SizeOfRawData );
				//sack_fseek( file, section.PointerToRawData, SEEK_SET );
				//sack_fread( data, 1, section.SizeOfRawData, file );
		{
				//FixResourceDirEntry( data, (MY_IMAGE_RESOURCE_DIRECTORY *)data );
				//sack_fseek( file, section.PointerToRawData, SEEK_SET );
				//sack_fwrite( data, 1, section.SizeOfRawData, file );
			{
				char *data;
				MY_IMAGE_IMPORT_DESCRIPTOR *iid;
				long v = dir[1].VirtualAddress;
				MY_IMAGE_IMPORT_DESCRIPTOR *directory;
				int m;
				INDEX i;
				LIST_FORALL( sections, i, PMY_IMAGE_SECTION_HEADER, section ) {
					if( section->VirtualAddress < dir[1].VirtualAddress &&
						(section->VirtualAddress + section->SizeOfRawData) > dir[1].VirtualAddress )
						break;
				}
				v = section->PointerToRawData;
				{

					data = Allocate( section->SizeOfRawData/*dir[1].Size*/ );
					sack_fseek( file, v, SEEK_SET );
					sack_fread( data, 1, section->SizeOfRawData/*dir[1].Size*/, file );
				}
				directory = (MY_IMAGE_IMPORT_DESCRIPTOR*)(data + dir[1].VirtualAddress - section->VirtualAddress);
				//printf( "Import data at %08x\n", section.VirtualAddress );

				for( m = 0; (iid = (MY_IMAGE_IMPORT_DESCRIPTOR*)(directory+m))// (data + directory + sizeof( *iid ) * m))
						, ( iid->Characteristics || iid->TimeDateStamp || iid->Name ||
									iid->FirstThunk || iid->ForwarderChain )
						; m++ )
				{
					//IMAGE_IMPORT_BY_NAME **iibn = (IMAGE_IMPORT_BY_NAME**)(data + ( iid->Characteristics - section->VirtualAddress ));
					TEXTSTR tmpname = DupCharToText( data+( iid->Name - section->VirtualAddress ) );
					AddDependCopy( pfs, tmpname );
#ifdef DEBUG_DISCOVERY
					printf( "%s %08x %08x %08x\n"
							, data +( iid->Name - section.VirtualAddress )
								, ( iid->Name - section.VirtualAddress )
							, iid->Characteristics
							, iid->FirstThunk );
#endif
				}

#if 0
				{
					MY_IMAGE_IMPORT_LOOKUP_TABLE iilt;
					do
					{
						printf( "Reading %d\n", sizeof( iilt ) );
						fread( &iilt, sizeof( iilt ), 1, file );
						if( !iilt.NameIsString )
						{
							printf( "Name at %08x\n", iilt.Name - section.VirtualAddress + section.PointerToRawData );
						}
						else
							printf( "Oridinal %d\n", iilt.Id );
					} while( iilt.Name );

					do
					{
						printf( "Reading %d\n", sizeof( iilt ) );
						fread( &iilt, sizeof( iilt ), 1, file );
						if( !iilt.NameIsString )
						{
							printf( "Name at %08x\n", iilt.Name - section.VirtualAddress + section.PointerToRawData );
						}
						else
							printf( "Oridinal %d\n", iilt.Id );
					} while( iilt.Name );
				}
#endif
				//free( data );

			}
        //else
			//	printf( "%*.*s\n"
			//			, MY_IMAGE_SIZEOF_SHORT_NAME
			//			, MY_IMAGE_SIZEOF_SHORT_NAME
			//			, section.Name );
					
		}
	}


	sack_fclose( file );
	return 0;
}

static int parse32( FILE *file, PFILESOURCE pfs 
	, MY_IMAGE_DOS_HEADER *dos_header
	, MY_IMAGE_NT_HEADERS *nt_header
	) {

	MY_IMAGE_OPTIONAL_HEADER nt_optional_header;
			
	sack_fread( &nt_optional_header, nt_header->FileHeader.SizeOfOptionalHeader, 1, file );

	if( nt_optional_header.Magic == 0x10b /*MAGE_NT_OPTIONAL_HDR32_MAGIC*/ )
	{
	}
	if( nt_optional_header.Magic == 0x20b /*MAGE_NT_OPTIONAL_HDR64_MAGIC*/ )
	{
	}


	// track down and kill resources.
	{
		int n;
		long FPISections = dos_header->e_lfanew
			+ sizeof( nt_header->FileHeader ) + sizeof( nt_header->Signature )
			+ nt_header->FileHeader.SizeOfOptionalHeader;
		PLIST sections = NULL;
		PMY_IMAGE_SECTION_HEADER section;
		PMY_IMAGE_DATA_DIRECTORY dir = (PMY_IMAGE_DATA_DIRECTORY)nt_header->OptionalHeader.PE32.DataDirectory;
		//sack_fseek( file, FPISections, SEEK_SET );
		for( n = 0; n < nt_header->FileHeader.NumberOfSections; n++ )
		{
			section = New( MY_IMAGE_SECTION_HEADER );
			sack_fseek( file, FPISections + n * sizeof( *section ), SEEK_SET );
			sack_fread( section, 1, sizeof( *section ), file );
			AddLink( &sections, section );
			//lprintf( "Read section '%s' %d of %d", section.Name, n, nt_header->FileHeader.NumberOfSections );
			//if( ( strcmp( section.Name, ".idata" ) == 0 ) )
		}
				//cpg27dec 2006 c:\work\sack\src\utils\pcopy\pcopy.c(295): Warning! W202: Symbol 'iilt' has been defined, but not referenced
				//data = (char*)malloc( section.SizeOfRawData );
				//sack_fseek( file, section.PointerToRawData, SEEK_SET );
				//sack_fread( data, 1, section.SizeOfRawData, file );
		{
				//FixResourceDirEntry( data, (MY_IMAGE_RESOURCE_DIRECTORY *)data );
				//sack_fseek( file, section.PointerToRawData, SEEK_SET );
				//sack_fwrite( data, 1, section.SizeOfRawData, file );
			{
				char *data;
				MY_IMAGE_IMPORT_DESCRIPTOR *iid;
				long v = dir[1].VirtualAddress;
				MY_IMAGE_IMPORT_DESCRIPTOR *directory;
				int m;
				INDEX i;
				LIST_FORALL( sections, i, PMY_IMAGE_SECTION_HEADER, section ) {
					if( section->VirtualAddress < dir[1].VirtualAddress &&
						(section->VirtualAddress + section->SizeOfRawData) > dir[1].VirtualAddress )
						break;
				}
				v = section->PointerToRawData;
				{

					data = Allocate( section->SizeOfRawData/*dir[1].Size*/ );
					sack_fseek( file, v, SEEK_SET );
					sack_fread( data, 1, section->SizeOfRawData/*dir[1].Size*/, file );
				}
				directory = (MY_IMAGE_IMPORT_DESCRIPTOR*)(data + dir[1].VirtualAddress - section->VirtualAddress);
				//printf( "Import data at %08x\n", section.VirtualAddress );

				for( m = 0; (iid = (MY_IMAGE_IMPORT_DESCRIPTOR*)(directory+m))// (data + directory + sizeof( *iid ) * m))
					 , ( iid->Characteristics || iid->TimeDateStamp || iid->Name ||
								 iid->FirstThunk || iid->ForwarderChain )
					 ; m++ )
				{
					//IMAGE_IMPORT_BY_NAME **iibn = (IMAGE_IMPORT_BY_NAME**)(data + ( iid->Characteristics - section->VirtualAddress ));
					TEXTSTR tmpname = DupCharToText( data+( iid->Name - section->VirtualAddress ) );
					AddDependCopy( pfs, tmpname );
#ifdef DEBUG_DISCOVERY
					printf( "%s %08x %08x %08x\n"
							, data +( iid->Name - section.VirtualAddress )
							 , ( iid->Name - section.VirtualAddress )
							, iid->Characteristics
							, iid->FirstThunk );
#endif
				}

#if 0
				{
					MY_IMAGE_IMPORT_LOOKUP_TABLE iilt;
					do
					{
						printf( "Reading %d\n", sizeof( iilt ) );
						fread( &iilt, sizeof( iilt ), 1, file );
						if( !iilt.NameIsString )
						{
							printf( "Name at %08x\n", iilt.Name - section.VirtualAddress + section.PointerToRawData );
						}
						else
							printf( "Oridinal %d\n", iilt.Id );
					} while( iilt.Name );

					do
					{
						printf( "Reading %d\n", sizeof( iilt ) );
						fread( &iilt, sizeof( iilt ), 1, file );
						if( !iilt.NameIsString )
						{
							printf( "Name at %08x\n", iilt.Name - section.VirtualAddress + section.PointerToRawData );
						}
						else
							printf( "Oridinal %d\n", iilt.Id );
					} while( iilt.Name );
				}
#endif
				//free( data );

			}
			//else
			//	printf( "%*.*s\n"
			//			, MY_IMAGE_SIZEOF_SHORT_NAME
			//			, MY_IMAGE_SIZEOF_SHORT_NAME
			//			, section.Name );
			
		}
	}
	sack_fclose( file );
	return 0;
}


int ScanFile( PFILESOURCE pfs )
{
	FILE *file;
	LOGICAL pe64 = FALSE;
	MY_IMAGE_DOS_HEADER dos_header;
	MY_IMAGE_NT_HEADERS nt_header;
	{
		if( !g.flags.bIncludeSystem )
		{
			if( StrCaseCmpEx( pfs->name, g.SystemRoot, StrLen( g.SystemRoot ) ) == 0 )
			{
				pfs->flags.bScanned = 1;
				pfs->flags.bSystem = 1;
				return 0;
			}
		}
		file = sack_fopen( 0, pfs->name, WIDE("rb") );
		if( file )
		{
			pfs->flags.bScanned = 1;
			sack_fread( &dos_header, 1, sizeof( dos_header ), file );
			if( dos_header.e_magic != MY_IMAGE_DOS_SIGNATURE )
			{
				//fprintf( stderr, WIDE("warning: \'%s\' is not a program.\n"), pfs->name );
				// copy it anyway.
				sack_fclose( file );
				return 0;
			}
			sack_fseek( file, dos_header.e_lfanew, SEEK_SET );
			sack_fread( &nt_header, sizeof( nt_header ), 1, file );
			if( nt_header.Signature != MY_IMAGE_NT_SIGNATURE )
			{
				fprintf( stderr, "warning: \'%s\' is not a 32 bit windows program.\n", pfs->name );
				sack_fclose( file );
				return 1;
			}

			if( 0 )
			{
				int n;
				for( n = 0; n < num64Machines; n++ )
					if( nt_header.FileHeader.Machine == _64bitMachines[n] ) {
						parse64( file, pfs, &dos_header, &nt_header );
						break;
					}
			}

			pfs->flags.bInvalid = 1;
			if( nt_header.FileHeader.SizeOfOptionalHeader == 240 )
			{
				pe64 = TRUE;
				g.flags.bWas64 = 1;
				if( g.flags.bWas32 )
				{
					if( g.flags.bIncludeSystem )
						fprintf( stderr, "ERROR: Mixing x86 and x64 sources to same target, and including system targets.\n" );
					else
						fprintf( stderr, "WARNING: Mixing x86 and x64 sources to same target.\n" );
				}
				pfs->flags.bInvalid = parse64( file, pfs, &dos_header, &nt_header );
			}
			else
			{
				g.flags.bWas32 = 1;
				if( g.flags.bWas64 )
				{
					if( g.flags.bIncludeSystem )
						fprintf( stderr, "ERROR: Mixing x86 and x64 sources to same target, and including system targets.\n" );
					else
						fprintf( stderr, "WARNING: Mixing x86 and x64 sources to same target.\n" );
				}
				pfs->flags.bInvalid = parse32( file, pfs, &dos_header, &nt_header );
			}
			
			if( pfs->flags.bInvalid )
				fprintf( stderr, "Failed to open %s\n", pfs->name );
			return 1;
		}
		else
		{
#ifdef WIN32
			HMODULE hModule;
			static TEXTCHAR name[256];

			if( g.flags.bVerbose )
				printf( "Locating %s\n", pfs->name );
			hModule = LoadLibrary( pfs->name );
			if( hModule )
			{
				GetModuleFileName( hModule, name, sizeof( name ) );
				if( g.flags.bVerbose )
					printf( "%s's real name is %s\n", pfs->name, name );
				//fflush( stdout );
				AddDependCopy( pfs, name )->flags.bExternal = 1;
				FreeLibrary( hModule );
				return 0;
			}
			else
			{
				TEXTSTR path = (TEXTSTR)OSALOT_GetEnvironmentVariable( WIDE("PATH") );
				TEXTSTR tmp;
				static TEXTCHAR tmpfile[256 + 64];
				while( tmp = (TEXTSTR)StrChr( path, ';' ) )
				{
					tmp[0] = 0;
#ifdef _MSC_VER
#define snwprintf _snwprintf
#endif
#ifdef _UNICODE
					snwprintf( tmpfile, sizeof( tmpfile ), WIDE("%s/%s"), path, pfs->name );
#else
					snprintf( tmpfile, sizeof( tmpfile ), "%s/%s", path, pfs->name );
#endif
					//printf( "attmpt %s\n", tmpfile );

					hModule = LoadLibrary( tmpfile );
					if( hModule )
					{
						GetModuleFileName( hModule, name, sizeof( name ) );
						//printf( "*Real name is %s\n", name );
						//fflush( stdout );
						AddDependCopy( pfs, name )->flags.bExternal = 1;
						FreeLibrary( hModule );
						return 0;
					}
					else
					{
						FILE *file = sack_fopen( 0, tmpfile, WIDE("rb") );
						if( file )
						{
							AddDependCopy( pfs, tmpfile )->flags.bExternal = 1;
							sack_fclose( file );
							return 0;
						}
					}
					path = tmp+1;
				}

			}
#endif
		}
	}
	return 0;
}

void Usage( CTEXTSTR *argv )
{
	printf( "usage: %s <-xlvs> <file...> <destination>\n"
			 "  -l : list only, do not copy (doesn't require destination)\n"
			 "  -p : add additional path to check\n"
			 "  -s : include system DLLs\n"
			 "  -v : verbose output?\n"
			 "  -x <file mask> : exclude this file from copy\n"
			 "  file - .dll or .exe referenced, all referenced DLLs\n"
			 "         are also copied to the dstination\n"
			 "  dest - directory name to copy to, will fail otherwise.\n"
			, argv[0]
			);
}

int main( int argc, CTEXTSTR *argv )
{
	if( argc < 2 )
	{
		Usage( argv );
		return 1;
	}
	StrCpyEx( g.SystemRoot, OSALOT_GetEnvironmentVariable( WIDE("SystemRoot") ), sizeof( g.SystemRoot )/sizeof( TEXTCHAR ) );
	{
		int c;
		for( c = 1;
			 c < ( g.flags.bDoNotCopy?argc:(argc-1) );
			 c++ )
		{
			if( argv[c][0] == '-' )
			{
				int done = 0;
				int ch = 1;
				while( argv[c][ch] && !done )
				{
					switch(argv[c][ch])
					{
					case 'S':
					case 's':
						g.flags.bIncludeSystem = 1;
						break;
					case 'l':
					case 'L':
						g.flags.bDoNotCopy = 1;
						break;
					case 'v':
					case 'V':
						g.flags.bVerbose = 1;
						break;
					case 'x':
					case 'X':
						if( argv[c][ch+1] )
						{
							AddLink( &g.excludes, StrDup( argv[c] + ch + 1 ) );
							done = 1; // skip remaining characters in parameter
						}
						else
						{
							if( ( c + 1 ) < (argc-1) )
							{
								AddLink( &g.excludes, StrDup( argv[c+1] ) );
								c++; // skip one word...
								done = 1; // skip remining characters, go to next parameter (c++)
							}
							else
							{
								fprintf( stderr, "-x parameter specified without a name to exclude... invalid paramters..." );
								exit(1);
							}
						}
						break;
					case 'p':
					case 'P':
						if( argv[c][ch+1] )
						{
							AddLink( &g.additional_paths, StrDup( argv[c] + ch + 1 ) );
							done = 1; // skip remaining characters in parameter
						}
						else
						{
							if( ( c + 1 ) < (argc-1) )
							{
								AddLink( &g.additional_paths, StrDup( argv[c+1] ) );
								c++; // skip one word...
								done = 1; // skip remining characters, go to next parameter (c++)
							}
							else
							{
								fprintf( stderr, "-p parameter specified without a name for path... invalid paramters..." );
								exit(1);
							}
						}
						break;
					}
					ch++;
				}
			}
			else
				AddFileCopy( argv[c ]);
		}
	}
	if( !g.flags.bDoNotCopy )
	{
		if( !IsPath( argv[argc-1] ) )
		{
			printf( "EROR: Final argument is not a directory\n" );
         Usage( argv );
         return 1;
		}

		CopyFileCopyTree( argv[argc-1] );
		printf( "Copied %d file%s\n", g.copied, g.copied==1?"":"s" );
	}
	else
	{
		CopyFileCopyTree( NULL );
	}
	return 0;

}
