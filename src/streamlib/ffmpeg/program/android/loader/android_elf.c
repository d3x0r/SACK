#include <string.h>
#ifdef __LINUX__
#include <elf.h>
#endif

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "org.d3x0r.sack.android_elf", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "org.d3x0r.sack.android_elf", __VA_ARGS__))
#define Seek(a,b) (((size_t)a)+(b))

#ifdef lprintf
#undef lprintf
#endif
#define lprintf LOGI


#ifndef WIN32

#include <sys/mman.h>

struct elf_loader_lib
{
	void* base;
	char *libname;
	size_t size;
	// further information?  so we can use this to find symbols?
   struct elf_loader_lib *next, **me;
};

static struct elf_loader_local
{
   struct elf_loader_lib *loaded_libs;
} l;

/* the other way to get address of the loaded library is to check
 /proc/self/maps
 */
#define LIBRARY_ADDRESS_BY_HANDLE(dlhandle) ((NULL == dlhandle) ? NULL : (void*)*(size_t const*)(dlhandle))

/* alternative scan
 http://stackoverflow.com/questions/6625252/finding-the-load-address-of-a-shared-library-in-linux
 #include <link.h>

off_t load_offset;
for (Elf64_Dyn *dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn) {
    if (dyn->d_tag == DT_DEBUG) {
        struct r_debug *r_debug = (struct r_debug *) dyn->d_un.d_ptr;
        struct link_map *link_map = r_debug->r_map;
        while (link_map) {
            if (strcmp(link_map->l_name, libname) == 0) {
                load_offset = (off_t)link_map->l_addr;
                break;
            }
            link_map = link_map->l_next;
        }
        break;
    }
}

*/

#define LogBinary
#define New(a) (a*)malloc(sizeof(a))
struct elf_loader_lib*LoadMemoryLibrary( char *libname, char * real_memory, size_t source_memory_length )
{
	struct elf_loader_lib *lib;

	if( !real_memory )
	{
		//lprintf( "Failed to load library:%s", name );
		return;
	}

	lib = New( struct elf_loader_lib );
	lib->base = real_memory;
	lib->size = source_memory_length;
	lib->libname = strdup( libname );


	lprintf( "dlopen is %p  %d", real_memory, source_memory_length );
	LogBinary( real_memory, 256 );
	mprotect( real_memory, source_memory_length, PROT_READ|PROT_EXEC );
	{
#ifdef __64__
#define Elf_Shdr Elf64_Shdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Ehdr Elf64_Ehdr
#else
#define Elf_Shdr Elf32_Shdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Ehdr Elf32_Ehdr
#endif
		Elf_Ehdr* real_dos_header = (Elf_Ehdr*)real_memory;
		if( real_dos_header->e_ident[0] != ELFMAG0
		  && real_dos_header->e_ident[1] != ELFMAG1
		  && real_dos_header->e_ident[2] != ELFMAG2
		  && real_dos_header->e_ident[3] != ELFMAG3
		  )
		{
			lprintf( "source Signature not OK" );
		}

		if( real_dos_header->e_ident[EI_DATA] != ELFDATA2LSB )
		{
         lprintf( "unsupported endian format..." );
		}

#ifdef __64__
		if( real_dos_header->e_ident[EI_CLASS] == ELFCLASS32 )
		{
			lprintf( "32 bit file on 64 bit system; ignoring" );
         return;
		}
#endif
#ifndef __64__
      if( real_dos_header->e_ident[EI_CLASS] == ELFCLASS64 )
		{
			lprintf( "64 bit file on 32 bit system; ignoreing." );
         return;
		}
#endif

		{
			Elf_Shdr *section = (Elf_Shdr*)Seek( real_memory, real_dos_header->e_shoff );
			// this section exists; but the strings don't, have to use the source file for section names
			Elf_Shdr *real_section = (Elf_Shdr*)Seek( real_memory, real_dos_header->e_shoff );
			Elf_Phdr *program_header = (Elf_Phdr*)Seek( real_memory, real_dos_header->e_phoff );

         size_t section_header_size = real_dos_header->e_shentsize;
			size_t program_header_size = real_dos_header->e_phentsize;

         size_t section_header_count = real_dos_header->e_shnum;
			size_t program_header_count = real_dos_header->e_phnum;

         size_t string_section_index = real_dos_header->e_shstrndx;


         lprintf( "section header %d %d %d", section_header_size, sizeof( Elf_Shdr), section_header_count );
         lprintf( "program header %d %d %d", program_header_size, sizeof( Elf_Phdr), program_header_count );

         lprintf( "covered %d bytes...  %d", sizeof( Elf_Ehdr ), real_dos_header->e_ehsize );
			//CompareMemoryBlock( real_memory, real_memory, 0, 0, real_dos_header->e_ehsize );
#define _size_fX  "zd"
			lprintf( "source point a : %"_size_fX, real_dos_header->e_entry );
			lprintf( "real point a : %"_size_fX, real_dos_header->e_entry );

			/* ELF format has lots of places that zero-test for failed or unused */

			{
				int n;
				char *strings = (char*)Seek( real_memory, section[string_section_index].sh_offset );
            //LogBinary( strings, 256 );
				for( n = 0; n < section_header_count; n++ )
				{
					if( !( section[n].sh_flags & SHF_ALLOC ) )
					{
						//lprintf( "must skip section; it's not in memory?  %d   %s %08x %p  %d", n, strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
                  continue;
					}
					//lprintf( "section %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
					switch( section[n].sh_type )
					{
					case SHT_NULL: // a non-section.
						continue;
					case SHT_PROGBITS:
						lprintf( "Program section %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
						break;
					case SHT_RELA:
						lprintf( "Relocation section %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
						break;
					case SHT_STRTAB:
                  lprintf( "How many string tables do we need? %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
						break;
					case SHT_SYMTAB:
                  lprintf( "Symbol table; should be constant... %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
						break;
					case SHT_DYNSYM:
					case SHT_DYNAMIC:
                  lprintf( "DYNAMIC symbols? %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
                  break;
					default:
						lprintf( "UHANDLED section %s %08x %p  %d", strings + section[n].sh_name, section[n].sh_type, section[n].sh_addr, section[n].sh_size );
                  break;
					}
//               if( StrCmpEx( strings + section[n].sh_name, "
				}
			}
			/*
          :section header 64 64 26
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(434):program header 56 56 5
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(436):covered 64 bytes...  64
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(438):source point a : DB0
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(439):real point a : DB0
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .hash 00000005 0x158  228
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(477):DYNAMIC symbols? .dynsym 0000000b 0x240  912
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(470):How many string tables do we need? .dynstr 00000003 0x5d0  557
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .gnu.version 6fffffff 0x7fe  76
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .gnu.version_r 6ffffffe 0x850  32
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(466):Relocation section .rela.dyn 00000004 0x870  312
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(466):Relocation section .rela.plt 00000004 0x9a8  600
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .init 00000001 0xc00  14
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .plt 00000001 0xc10  416
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .text 00000001 0xdb0  6328
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .fini 00000001 0x2668  9
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .rodata 00000001 0x2678  1225
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .eh_frame_hdr 00000001 0x2b44  140
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .eh_frame 00000001 0x2bd0  580
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .init_array 0000000e 0x203000  16
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .fini_array 0000000f 0x203010  8
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .jcr 00000001 0x203018  8
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(477):DYNAMIC symbols? .dynamic 00000006 0x203020  496
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .got 00000001 0x203210  40
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .got.plt 00000001 0x203238  224
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section .data 00000001 0x203318  8
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(461):Program section deadstart_list 00000001 0x203320  192
Mar 13 16:19:10 tower2 EditOptions@selfcompare.c(481):UHANDLED section .bss 00000008 0x2033e0  8
*/
			//CompareMemoryBlock( real_memory, real_memory, real_dos_header->e_shoff, real_dos_header->e_shoff, section_header_size );

		}

	}

	lprintf( "need to register newly loaded library for symbol location..." );

	if( lib->next = l.loaded_libs )
		lib->next->me = &lib->next;
   lib->me = &l.loaded_libs;
   l.loaded_libs = lib;

	//AddMappedLibrary( name, real_memory );
   return lib;
}


void *LoadMemoryLibraryProc( struct elf_loader_lib* real_memory, const char *funcname  )
{
}

#endif


//PRIORITY_PRELOAD( SelfCompare, NAMESPACE_PRELOAD_PRIORITY + 1 )
//{
//	CompareSelf();
//}

