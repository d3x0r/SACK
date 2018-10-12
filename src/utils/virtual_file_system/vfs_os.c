/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#define SACK_VFS_OS_SOURCE
//#define USE_STDIO
#if 1
#  include <stdhdrs.h>
#  include <ctype.h> // tolower on linux
#ifndef USE_STDIO
#  include <filesys.h>
#endif
#  include <procreg.h>
#  include <salty_generator.h>
#  include <sack_vfs.h>
#  include <sqlgetoption.h>
#else
#  include <sack.h>
#  include <ctype.h> // tolower on linux
//#include <filesys.h>
//#include <procreg.h>
//#include <salty_generator.h>
//#include <sack_vfs.h>
//#include <sqlgetoption.h>
#endif

#ifdef USE_STDIO
#define sack_fopen(a,b,c)     fopen(b,c)
#define sack_fseek(a,b,c)     fseek(a,b,c)
#define sack_fclose(a)        fclose(a)
#define sack_fread(a,b,c,d)   fread(a,b,c,d)
#define sack_fwrite(a,b,c,d)  fwrite(a,b,c,d)
#define sack_ftell(a)         ftell(a)

#ifdef __cplusplus
namespace sack {
	namespace filesys {
#endif
		// pathops.c
		extern LOGICAL  CPROC  IsPath( CTEXTSTR path );
		extern  int CPROC  MakePath( CTEXTSTR path );
		extern CTEXTSTR CPROC pathrchr( CTEXTSTR path );
		extern CTEXTSTR CPROC pathchr( CTEXTSTR path );
#ifdef __cplusplus
	}
}
using namespace sack::filesys;
#endif

#endif


SACK_VFS_NAMESPACE
//#define PARANOID_INIT

//#define DEBUG_TRACE_LOG

#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#endif

#define FILE_BASED_VFS
#define VIRTUAL_OBJECT_STORE
#include "vfs_internal.h"
#define vfs_SEEK vfs_os_SEEK
#define vfs_BSEEK vfs_os_BSEEK

static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
	uint16_t index[256][256];
} l;
#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)
#define EODMARK   (1)
#define GFB_INIT_NONE   0
#define GFB_INIT_DIRENT 1
#define GFB_INIT_NAMES  2
static BLOCKINDEX _os_GetFreeBlock( struct volume *vol, int init );
static LOGICAL _os_ScanDirectory( struct volume *vol, const char * filename
	, BLOCKINDEX *nameBlockStart
	, FPI *dirFPI, struct directory_entry *dirent
	, struct directory_entry *dirkey, int path_match );
static BLOCKINDEX vfs_os_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand );

static char _os_mytolower( int c ) {	if( c == '\\' ) return '/'; return tolower( c ); }

// seek by byte position from a starting block; as file; result with an offset into a block.
uintptr_t vfs_os_FSEEK( struct volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index ) {
	uint8_t *data;
	while( firstblock != EOBBLOCK && offset > BLOCK_SIZE ) {
		firstblock = vfs_os_GetNextBlock( vol, firstblock, 0, 0 );
		offset -= BLOCK_SIZE;
	}
	if( firstblock == EOBBLOCK ) return 0;
	data = BTSEEK( uint8_t*, vol, firstblock, cache_index[0] );
	return (uintptr_t)(data + (offset));
}

static int  _os_PathCaseCmpEx ( CTEXTSTR s1, CTEXTSTR s2, size_t maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && ( (s1[0]=='/'&&s2[0]=='\\')||(s1[0]=='\\'&&s2[0]=='/')||
									 (((s1[0] >='a' && s1[0] <='z' )?s1[0]-('a'-'A'):s1[0])
									 == ((s2[0] >='a' && s2[0] <='z' )?s2[0]-('a'-'A'):s2[0])) ) && maxlen;
		  s1++, s2++, maxlen-- );
	if( maxlen )
		return tolower(s1[0]) - tolower(s2[0]);
	return 0;
}

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int _os_MaskStrCmp( struct volume *vol, const char * filename, BLOCKINDEX nameBlock, FPI name_offset, int path_match ) {
	enum block_cache_entries cache = BLOCK_CACHE_NAMES;
	const char *dirname = (const char*)vfs_os_FSEEK( vol, nameBlock, name_offset, &cache );
	const char *dirkey;
	if( !dirname ) return 1;
	dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK );
	if( vol->key ) {
		int c;
		while(  ( c = (dirname[0] ^ dirkey[0] ) )
			  && filename[0] ) {
			int del = _os_mytolower(filename[0]) - _os_mytolower(c);
			if( del ) return del;
			filename++;
			dirname++;
			dirkey++;
			if( path_match && !filename[0] ) {
				c = (dirname[0] ^ dirkey[0] );
				if( c == '/' || c == '\\' ) return 0;
			}
		}
		// c will be 0 or filename will be 0...
		if( path_match ) return 1;
		return filename[0] - c;
	} else {
		//LoG( "doesn't volume always have a key?" );
		if( path_match ) {
			size_t l;
			int r = _os_PathCaseCmpEx( filename, dirname, l = strlen( filename ) );
			if( !r )
				if( (dirname)[l] == '/' || (dirname)[l] == '\\' )
					return 0;
				else
					return 1;
			return r;
		}
		else
			return _os_PathCaseCmpEx( filename, dirname, strlen(filename) );
	}
}

#ifdef DEBUG_TRACE_LOG
static void MaskStrCpy( char *output, size_t outlen, struct volume *vol, FPI name_offset ) {
	if( vol->key ) {
		int c;
		FPI name_start = name_offset;
		while(  ( c = ( vol->usekey_buffer[BLOCK_CACHE_NAMES][name_offset&BLOCK_MASK] ^ vol->usekey[BLOCK_CACHE_NAMES][name_offset&BLOCK_MASK] ) ) ) {
			if( ( name_offset - name_start ) < outlen )
				output[name_offset-name_start] = c;
			name_offset++;
		}
		if( ( name_offset - name_start ) < outlen )
			output[name_offset-name_start] = 0;
		else
			output[outlen-1] = 0;
	} else {
		//LoG( "doesn't volume always have a key?" );
		StrCpyEx( output, (const char *)(vol->usekey[BLOCK_CACHE_NAMES] + (name_offset & BLOCK_MASK )), outlen );
	}
}
#endif


static void _os_updateCacheAge( struct volume *vol, enum block_cache_entries *cache_idx, BLOCKINDEX segment, uint8_t *age, int ageLength ) {
	int n, m;
	int nLeast;
	BLOCKINDEX *test_segment = vol->segment + cache_idx[0];
	for( n = 0; n < (ageLength); n++,test_segment++ ) {
		if( test_segment[0] == segment ) {
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) {
				if( !age[m] ) break;
				if( age[m] > age[n] )
					age[m]--;
			}
			age[n] = m;
			break;
		}
		if( !age[n] ) { // end of list, empty entry.
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) { // age entries up to this one.
				if( !age[m] ) break;
				if( age[m] > ( n + 1 ) )
					age[m]--;
			}
			age[n] = n + 1; // make this one newest
			break;
		}
		if( age[n] == 1 ) nLeast = n; // this one will be oldest candidate
	}
	if( n == (ageLength) ) {
		for( n = 0; n < (ageLength); n++ ) {  // age evernthing.
			age[n]--;
		}
		age[nLeast] = (ageLength); // make this one the newest, and return it.

		if( TESTFLAG( vol->dirty, cache_idx[0]+ nLeast ) ) {
			sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx[0]], SEEK_SET );
			sack_fwrite( vol->usekey_buffer[cache_idx[0]], 1, BLOCK_SIZE, vol->file );
			RESETFLAG( vol->dirty, cache_idx[0] + nLeast );
		}

		// read new buffer for new segment
		sack_fseek( vol->file, (size_t)((segment-1)*BLOCK_SIZE), SEEK_SET );
		if( !sack_fread( vol->usekey_buffer[cache_idx[0] + nLeast], 1, BLOCK_SIZE, vol->file ) )
			memset( vol->usekey_buffer[cache_idx[0] + nLeast], 0, BLOCK_SIZE );

		cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + nLeast);
	}
}

static enum block_cache_entries _os_UpdateSegmentKey( struct volume *vol, enum block_cache_entries cache_idx, BLOCKINDEX segment )
{
	if( cache_idx == BLOCK_CACHE_FILE ) {
		_os_updateCacheAge( vol, &cache_idx, segment, vol->fileCacheAge, (BLOCK_CACHE_FILE_LAST - BLOCK_CACHE_FILE) );
	}
	else if( cache_idx == BLOCK_CACHE_NAMES ) {
		_os_updateCacheAge( vol, &cache_idx, segment, vol->nameCacheAge, (BLOCK_CACHE_NAMES_LAST - BLOCK_CACHE_NAMES) );
	}
#ifdef VIRTUAL_OBJECT_STORE
	else if( cache_idx == BLOCK_CACHE_DIRECTORY ) {
		_os_updateCacheAge( vol, &cache_idx, segment, vol->dirHashCacheAge, (BLOCK_CACHE_DIRECTORY_LAST - BLOCK_CACHE_DIRECTORY) );
	}
#endif
	else {
		if( vol->segment[cache_idx] != segment ) {
			if( TESTFLAG( vol->dirty, cache_idx ) ) {
				sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx], SEEK_SET );
				sack_fwrite( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file );
				RESETFLAG( vol->dirty, cache_idx );
			}

			// read new buffer for new segment
			sack_fseek( vol->file, (size_t)((segment - 1)*BLOCK_SIZE), SEEK_SET);
			if( !sack_fread( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file ) )
				memset( vol->usekey_buffer[cache_idx], 0, BLOCK_SIZE );
		}
		vol->segment[cache_idx] = segment;
	}

	if( !vol->key ) {
		return cache_idx;
	}
	while( ((segment)*BLOCK_SIZE) > vol->dwSize ) {
		LoG( "looping to get a size %d", ((vol->segment[cache] - 1)*BLOCK_SIZE) );
		if( !_os_ExpandVolume( vol ) ) return 0;
	}
	//vol->segment[cache_idx] = segment;
	if( vol->segment[cache_idx] == vol->_segment[cache_idx] )
		return cache_idx;
	SRG_ResetEntropy( vol->entropy );
	vol->_segment[cache_idx] = vol->segment[cache_idx];
	vol->curseg = cache_idx;  // so we know which 'segment[idx]' to use.
	SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->segkey, SHORTKEY_LENGTH * 8 );
	{
		int n;
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[cache_idx];
		uint64_t* volkey = (uint64_t*)vol->key;
		uint64_t* segkey = (uint64_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey += 2;
			volkey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[cache_idx];
		uint32_t* volkey = (uint32_t*)vol->key;
		uint32_t* segkey = (uint32_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey[2] = volkey[2] ^ (segkey[2]);
			usekey[3] = volkey[3] ^ (segkey[3]);
			usekey += 4;
			volkey += 4;
		}
#endif
	}
	return cache_idx;
}

static LOGICAL _os_ValidateBAT( struct volume *vol ) {
	BLOCKINDEX first_slab = 0;
	BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
	BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
	BLOCKINDEX n;
	enum block_cache_entries cache = BLOCK_CACHE_BAT;
	if( vol->key ) {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT; 
			BLOCKINDEX *blockKey;
			BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			//sack_fseek( vol->file, (size_t)n * BLOCK_SIZE, SEEK_SET );
			//if( !sack_fread( vol->usekey_buffer[BLOCK_CACHE_BAT], 1, BLOCK_SIZE, vol->file ) ) return FALSE;
			//BAT = (BLOCKINDEX*)vol->usekey_buffer[BLOCK_CACHE_BAT];
			blockKey = ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT]);
			_os_UpdateSegmentKey( vol, BLOCK_CACHE_BAT, n + 1 );

			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[0] ^ blockKey[0];
				BAT++; blockKey++;
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) break;
				if( block >= last_block ) return FALSE;
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	} else {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			BAT = (BLOCKINDEX*)vol->usekey_buffer[BLOCK_CACHE_BAT];
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				BLOCKINDEX block = BAT[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) break;
				if( block >= last_block ) return FALSE;
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	if( !_os_ScanDirectory( vol, NULL, NULL, NULL, NULL, NULL, 0 ) ) return FALSE;
	return TRUE;
}

//-------------------------------------------------------
// function to process a currently loaded program to get the
// data offset at the end of the executable.


static POINTER _os_GetExtraData( POINTER block )
{
#ifdef WIN32
#  define Seek(a,b) (((uintptr_t)a)+(b))
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
			LoG( "Basic signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE ) {
			LoG( "Basic NT signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				LoG( "Optional header signature is incorrect..." );
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
			dwSize += (BLOCK_SIZE*2)-1; // pad 1 full block, plus all but 1 byte of a full block(round up)
			dwSize &= ~(BLOCK_SIZE-1); // mask off the low bits; floor result to block boundary
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
#  undef Seek
#else
	// need to get elf size...
	return 0;
#endif
}

static void _os_AddSalt2( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct datatype { void* start; size_t length; } *data = (struct datatype*)psv;
	
	(*salt_size) = data->length;
	(*salt) = (POINTER)data->start;
	// only need to make one pass of it....
	data->length = 0;
	data->start = NULL;
}
const uint8_t *sack_vfs_os_get_signature2( POINTER disk, POINTER diskReal ) {
	if( disk != diskReal ) {
		static uint8_t usekey[BLOCK_SIZE];
		static struct random_context *entropy;
		static struct datatype { void* start; size_t length; } data;
		data.start = diskReal;
		data.length = ((uintptr_t)disk - (uintptr_t)diskReal) - BLOCK_SIZE;
		if( !entropy ) entropy = SRG_CreateEntropy2( _os_AddSalt2, (uintptr_t)&data );
		SRG_ResetEntropy( entropy );
		SRG_GetEntropyBuffer( entropy, (uint32_t*)usekey, BLOCK_SIZE*CHAR_BIT );
		return usekey;
	}
	return NULL;
}


// add some space to the volume....
static LOGICAL _os_ExpandVolume( struct volume *vol ) {
	LOGICAL created = FALSE;
	LOGICAL path_checked = FALSE;
	size_t oldsize = vol->dwSize;
	if( vol->file && vol->read_only ) return TRUE;
	if( !vol->file ) {
		{
			char *tmp = StrDup( vol->volname );
			char *dir = (char*)pathrchr( tmp );
			if( dir ) {
				dir[0] = 0;
				if( !IsPath( tmp ) ) MakePath( tmp );
			}
			Deallocate( char*, tmp );
		}

		vol->file = sack_fopen( 0, vol->volname, "rb+" );
		if( !vol->file ) {
			created = TRUE;
			vol->file = sack_fopen( 0, vol->volname, "wb+" );
		}
		sack_fseek( vol->file, 0, SEEK_END );
		vol->dwSize = sack_ftell( vol->file );
		sack_fseek( vol->file, 0, SEEK_SET );
	}

	//vol->dwSize += ((uintptr_t)vol->disk - (uintptr_t)vol->diskReal);
	// a BAT plus the sectors it references... ( BLOCKS_PER_BAT + 1 ) * BLOCK_SIZE
	vol->dwSize += BLOCKS_PER_SECTOR*BLOCK_SIZE;
	LoG( "created expanded volume: %p from %p size:%" _size_f, vol->file, BLOCKS_PER_SECTOR*BLOCK_SIZE, vol->dwSize );

	// can't recover dirents and nameents dynamically; so just assume
	// use the _os_GetFreeBlock because it will update encypted
	//vol->disk->BAT[0] = EOFBLOCK;  // allocate 1 directory entry block
	//vol->disk->BAT[1] = EOFBLOCK;  // allocate 1 name block
	if( created ) {
		_os_UpdateSegmentKey( vol, BLOCK_CACHE_BAT, 1 );
		((BLOCKINDEX*)vol->usekey_buffer[BLOCK_CACHE_BAT])[0]
			= EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[0];
		SETFLAG( vol->dirty, BLOCK_CACHE_BAT );
		sack_fseek( vol->file, 0, SEEK_SET );
		sack_fwrite( vol->usekey_buffer[BLOCK_CACHE_BAT], 1, BLOCK_SIZE, vol->file );
		{
			BLOCKINDEX dirblock = _os_GetFreeBlock( vol, GFB_INIT_DIRENT );
			enum block_cache_entries cache = BLOCK_CACHE_DIRECTORY;
			struct directory_hash_lookup_block *dir = BTSEEK( struct directory_hash_lookup_block *, vol, dirblock, cache );
			SETFLAG( vol->dirty, cache );
		}
	}

	return TRUE;
}


// shared with fuse module
// seek by byte position; result with an offset into a block.
uintptr_t vfs_os_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) {
	while( offset >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = (offset / BLOCK_SIZE) + 1;

		cache_index[0] = _os_UpdateSegmentKey( vol, cache_index[0], seg );

		sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_index[0]] = offset & ~BLOCK_MASK), SEEK_SET );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + (offset&BLOCK_MASK);
	}
}

// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block after first BAT.
uintptr_t vfs_os_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) {
	BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + ( block & (BLOCKS_PER_BAT-1) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		cache_index[0] = _os_UpdateSegmentKey( vol, cache_index[0], seg );
		sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_index[0]] = b & ~BLOCK_MASK), SEEK_SET );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + (b&BLOCK_MASK);
	}
}

static BLOCKINDEX _os_GetFreeBlock( struct volume *vol, int init )
{
	size_t n;
	int b = 0;
	enum block_cache_entries cache = BLOCK_CACHE_BAT;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	FPI start_POS = sack_ftell( vol->file );
	BLOCKINDEX *start_BAT = current_BAT;
	if( !current_BAT ) return 0;
	do
	{
		BLOCKINDEX check_val;
		BLOCKINDEX *blockKey; 
		blockKey = ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT]);
		for( n = 0; n < BLOCKS_PER_BAT; n++ )
		{
			check_val = current_BAT[0] ^ blockKey[0];
			if( !check_val || (check_val == 1) )
			{
				// mark it as claimed; will be enf of file marker...
				// adn thsi result will overwrite previous EOF.
				current_BAT[0] = ( EOFBLOCK ) ^ blockKey[0];
				if( init )
				{
					enum block_cache_entries cache;
					if( init == GFB_INIT_DIRENT ) {
						struct directory_hash_lookup_block *dir;
						struct directory_hash_lookup_block *dirkey;
						cache = _os_UpdateSegmentKey( vol, BLOCK_CACHE_DIRECTORY, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 );
						memset( vol->usekey_buffer[cache], 0, BLOCK_SIZE );

						dir = (struct directory_hash_lookup_block *)vol->usekey_buffer[cache];
						dirkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
						dir->names_first_block = _os_GetFreeBlock( vol, GFB_INIT_NAMES ) ^ dirkey->names_first_block;
						dir->used_names = 0 ^ dirkey->used_names;
						//((struct directory_hash_lookup_block*)(vol->usekey_buffer[cache]))->entries[0].first_block = EODMARK ^ ((struct directory_hash_lookup_block*)vol->usekey[cache])->entries[0].first_block;
					}
					else if( init == GFB_INIT_NAMES ) {
						cache = _os_UpdateSegmentKey( vol, BLOCK_CACHE_NAMES, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 );
						memset( vol->usekey_buffer[cache], 0, BLOCK_SIZE );
						((char*)(vol->usekey_buffer[cache]))[0] = ((char*)vol->usekey[cache])[0];
					}
					else {
						cache = _os_UpdateSegmentKey( vol, BLOCK_CACHE_FILE, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 );
					}
					//else
					//	memcpy( ((uint8_t*)vol->disk) + (vol->segment[cache]-1) * BLOCK_SIZE, vol->usekey[cache], BLOCK_SIZE );
				}
				SETFLAG( vol->dirty, cache );
				if( (check_val == EOBBLOCK) )
					if( n < (BLOCKS_PER_BAT - 1) ) {
						current_BAT[1] = EOBBLOCK ^ blockKey[1];
					}
					else {
						// have to write what is there now, seek will read new block in...
						current_BAT = TSEEK( BLOCKINDEX*, vol, (b + 1) * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
						blockKey = ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT]);
						current_BAT[0] = EOBBLOCK ^ blockKey[0];
						SETFLAG( vol->dirty, cache );
					}

				return b * BLOCKS_PER_BAT + n;
			}
			current_BAT++;
			blockKey++;
		}
		b++;
		current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
		start_POS = sack_ftell( vol->file );
	}while( 1 );
}

static BLOCKINDEX vfs_os_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand ) {
	BLOCKINDEX sector = block >> BLOCK_SHIFT;
	enum block_cache_entries cache = BLOCK_CACHE_BAT;
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
	BLOCKINDEX check_val = (this_BAT[block & (BLOCKS_PER_BAT-1)]);
	if( !this_BAT ) return 0; // if this passes, later ones will also.

	check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[block & (BLOCKS_PER_BAT-1)];
	if( check_val == EOBBLOCK ) {
		(this_BAT[block & (BLOCKS_PER_BAT-1)]) = EOFBLOCK^((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[block & (BLOCKS_PER_BAT-1)];
		(this_BAT[1+block & (BLOCKS_PER_BAT-1)]) = EOBBLOCK^((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[1+block & (BLOCKS_PER_BAT-1)];
	}
	if( check_val == EOFBLOCK || check_val == EOBBLOCK ) {
		if( expand ) {
			BLOCKINDEX key = vol->key?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[block & (BLOCKS_PER_BAT-1)]:0;
			check_val = _os_GetFreeBlock( vol, init );
			// free block might have expanded...
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), cache );
			if( !this_BAT ) return 0;
			// segment could already be set from the _os_GetFreeBlock...
			this_BAT[block & (BLOCKS_PER_BAT-1)] = check_val ^ key;
			fwrite( this_BAT, 1, BLOCK_SIZE, vol->file );
		}
	}
	return check_val;
}




static void _os_AddSalt( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct volume *vol = (struct volume *)psv;
	if( vol->sigsalt ) {
		(*salt_size) = vol->sigkeyLength;
		(*salt) = (POINTER)vol->sigsalt;
		vol->sigsalt = NULL;
	}
	else if( vol->datakey ) {
		(*salt_size) = BLOCK_SIZE;
		(*salt) = (POINTER)vol->datakey;
		vol->datakey = NULL;
	}
	else if( vol->userkey ) {
		(*salt_size) = StrLen( vol->userkey );
		(*salt) = (POINTER)vol->userkey;
		vol->userkey = NULL;
	}
	else if( vol->devkey ) {
		(*salt_size) = StrLen( vol->devkey );
		(*salt) = (POINTER)vol->devkey;
		vol->devkey = NULL;
	}
	else if( vol->segment[vol->curseg] ) {
		BLOCKINDEX sector = vol->segment[vol->curseg];
		switch( vol->clusterKeyVersion ) {
		case 0:
			( *salt_size ) = sizeof( vol->segment[vol->curseg] );
			( *salt ) = &vol->segment[vol->curseg];
			break;
		case 1:
			memcpy( vol->tmpSalt, vol->key, 16 );
			vol->tmpSalt[sector & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[0];
			vol->tmpSalt[( sector >> 4 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[1];
			vol->tmpSalt[( sector >> 8 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[2];
			vol->tmpSalt[( sector >> 12 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[3];

			( (BLOCKINDEX*)vol->tmpSalt )[0] ^= sector;
			( (BLOCKINDEX*)vol->tmpSalt )[1] ^= sector;
			( *salt_size ) = 12;// sizeof( vol->segment[vol->curseg] );
			( *salt ) = vol->tmpSalt;
			break;
		}
	}
	else
		(*salt_size) = 0;
}

static void _os_AssignKey( struct volume *vol, const char *key1, const char *key2 )
{
	uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * BLOCK_CACHE_COUNT + BLOCK_SIZE + SHORTKEY_LENGTH;
	if( !vol->key_buffer ) {
		int n;
		vol->key_buffer = HeapAllocateAligned( NULL, size, 4096 );// NewArray( uint8_t, size );
		memset( vol->key_buffer, 0, size );
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ ) {
			vol->usekey_buffer[n] = vol->key_buffer + (n + 1) * BLOCK_SIZE;
		}
	}

	vol->userkey = key1;
	vol->devkey = key2;
	if( key1 || key2 )
	{
		int n;
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		else
			SRG_ResetEntropy( vol->entropy );

		vol->key = HeapAllocateAligned( NULL, size, 4096 ); //NewArray( uint8_t, size );

		for( n = 0; n < BLOCK_CACHE_COUNT; n++ ) {
			vol->usekey[n] = vol->key + (n + 1) * BLOCK_SIZE;
			vol->segment[n] = 0;
		}
		vol->segkey = vol->key + BLOCK_SIZE * (BLOCK_CACHE_COUNT + 1);
		vol->sigkey = vol->key + BLOCK_SIZE * (BLOCK_CACHE_COUNT + 1) + SHORTKEY_LENGTH;
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[BLOCK_CACHE_DIRECTORY] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, BLOCK_SIZE * 8 );
	}
	else {
		int n;
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ )
			vol->usekey[n] = l.zerokey;
		vol->segkey = l.zerokey;
		vol->sigkey = l.zerokey;
		vol->key = NULL;
	}
}

static uintptr_t volume_flusher( PTHREAD thread ) {
	struct volume *vol = (struct volume *)GetThreadParam( thread );
	while( 1 ) {
		WakeableSleep( 250 );
		while( LockedExchange( &vol->lock, 1 ) ) Relinquish();

		{
			INDEX idx;
			for( idx = 0; idx < BLOCK_CACHE_COUNT; idx++ )
				if( TESTFLAG( vol->dirty, idx ) ) {
					sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
					sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
					RESETFLAG( vol->dirty, idx );
				}
		}

		vol->lock = 0;
		// for all dirty
		WakeableSleep( SLEEP_FOREVER );

	}
}

struct volume *sack_vfs_os_load_volume( const char * filepath )
{
	struct volume *vol = New( struct volume );
	memset( vol, 0, sizeof( struct volume ) );
	vol->volname = strdup( filepath );
	_os_AssignKey( vol, NULL, NULL );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { Deallocate( struct volume*, vol ); return NULL; }
	return vol;
}

void sack_vfs_os_unload_volume( struct volume * vol );

struct volume *sack_vfs_os_load_crypt_volume( const char * filepath, uintptr_t version, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	if( !version ) version = 2;
	vol->clusterKeyVersion = version - 1;
	vol->volname = strdup( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	_os_AssignKey( vol, userkey, devkey );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { sack_vfs_os_unload_volume( vol ); return NULL; }
	return vol;
}
#if 0
struct volume *sack_vfs_os_use_crypt_volume( POINTER memory, size_t sz, uintptr_t version, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	vol->read_only = 1;
	_os_AssignKey( vol, userkey, devkey );
	if( !version ) version = 2;
	vol->clusterKeyVersion = version - 1;
	vol->external_memory = TRUE;
	vol->diskReal = (struct disk*)memory;
	vol->dwSize = sz;
#ifdef WIN32
	// elf has a different signature to check for .so extended data...
	struct disk *actual_disk;
	if( ((char*)memory)[0] == 'M' && ((char*)memory)[1] == 'Z' ) {
		actual_disk = (struct disk*)GetExtraData( memory );
		if( actual_disk ) {
			if( ( ( (uintptr_t)actual_disk - (uintptr_t)memory ) < vol->dwSize ) ) {
				const uint8_t *sig = sack_vfs_os_get_signature2( (POINTER)((uintptr_t)actual_disk-BLOCK_SIZE), memory );
				if( memcmp( sig, (POINTER)(((uintptr_t)actual_disk)-BLOCK_SIZE), BLOCK_SIZE ) ) {
					lprintf( "Signature failed comparison; the core has changed since it was attached" );
					vol->diskReal = NULL;
					vol->dwSize = 0;
					sack_vfs_os_unload_volume( vol );
					return FALSE;
				}
				vol->dwSize -= ((uintptr_t)actual_disk - (uintptr_t)memory);
				memory = (POINTER)actual_disk;
			} else {
				lprintf( "Signature failed comparison; the core is not attached to anything." );
				vol->diskReal = NULL;
				vol->disk = NULL;
				vol->dwSize = 0;
				sack_vfs_os_unload_volume( vol );
				return NULL;
			}
		}
	}
#endif
	vol->disk = (struct disk*)memory;

	if( !_os_ValidateBAT( vol ) ) { sack_vfs_os_unload_volume( vol );  return NULL; }
	return vol;
}
#endif
void sack_vfs_os_unload_volume( struct volume * vol ) {
	INDEX idx;
	struct sack_vfs_file *file;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		break;
	if( file ) {
		vol->closed = TRUE;
		return;
	}
	{
		for( idx = 0; idx < BLOCK_CACHE_COUNT; idx++ )
		if( TESTFLAG( vol->dirty, idx ) ) {
			sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
			sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
			RESETFLAG( vol->dirty, idx );
		}

	}
	free( (char*)vol->volname );
	DeleteListEx( &vol->files DBG_SRC );
	sack_fclose( vol->file );
	//if( !vol->external_memory )	CloseSpace( vol->diskReal );
	if( vol->key ) {
		Deallocate( uint8_t*, vol->key );
		SRG_DestroyEntropy( &vol->entropy );
	}
	Deallocate( uint8_t*, vol->key_buffer );
	Deallocate( struct volume*, vol );
}

void sack_vfs_os_shrink_volume( struct volume * vol ) {
	size_t n;
	int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	int last_bat = 0;
	enum block_cache_entries cache = BLOCK_CACHE_BAT;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		BLOCKINDEX *blockKey;
		blockKey = (BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT];
		for( n = 0; n < BLOCKS_PER_BAT; n++ ) {
			check_val = *(current_BAT++);
			if( vol->key )	check_val ^= *(blockKey++);
			if( check_val ) {
				last_bat = b;
				last_block = n;
			}
		}
		b++;
		if( b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) < vol->dwSize ) {
			current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
		} else
			break;
	}while( 1 );
	sack_fclose( vol->file );
	vol->file = NULL;
	/*
	SetFileLength( vol->volname,
			last_bat * BLOCKS_PER_SECTOR * BLOCK_SIZE + ( last_block + 1 + 1 )* BLOCK_SIZE );
	*/
	// setting 0 size will cause expand to do an initial open instead of expanding
	vol->dwSize = 0;
}

static void _os_mask_block( struct volume *vol, size_t n ) {
	BLOCKINDEX b = ( 1 + (n >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR) + (n & (BLOCKS_PER_BAT - 1)));
	_os_UpdateSegmentKey( vol, BLOCK_CACHE_DATAKEY, b + 1 );
	{
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[BLOCK_CACHE_DATAKEY];
		uint64_t* block = (uint64_t*)(((uintptr_t)vol->disk) + b * BLOCK_SIZE );
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block += 2; usekey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[BLOCK_CACHE_DATAKEY];
		uint32_t* block = (uint32_t*)vol->usekey_buffer[BLOCK_CACHE_DATAKEY];
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block[2] = block[2] ^ usekey[2];
			block[3] = block[3] ^ usekey[3];
			block += 4; usekey += 4;
		}
#endif
	}
}

LOGICAL sack_vfs_os_decrypt_volume( struct volume *vol )
{
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( !vol->key ) { vol->lock = 0; return FALSE; } // volume is already decrypted, cannot remove key
	{
		enum block_cache_entries cache = BLOCK_CACHE_BAT;
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey; 
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT]);
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				block[0] ^= blockKey[0];
				if( block[0] == EOBBLOCK ) break;
				else if( block[0] ) _os_mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block++;
				blockKey++;
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	_os_AssignKey( vol, NULL, NULL );
	vol->lock = 0;
	return TRUE;
}

LOGICAL sack_vfs_os_encrypt_volume( struct volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 ) {
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( vol->key ) { vol->lock = 0; return FALSE; } // volume already has a key, cannot apply new key
	if( !version ) version = 2;
	vol->clusterKeyVersion = version-1;
	_os_AssignKey( vol, key1, key2 );
	{
		int done;
		size_t n;
		enum block_cache_entries cache = BLOCK_CACHE_BAT;
		BLOCKINDEX slab = (vol->dwSize + (BLOCKS_PER_SECTOR*BLOCK_SIZE-1)) / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		done = 0;
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey;
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT]);

			//vol->segment[BLOCK_CACHE_BAT] = n + 1;
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				if( block[0] == EOBBLOCK ) done = TRUE;
				else if( block[0] ) _os_mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block[0] ^= blockKey[0];
				if( done ) break;
				block++;
				blockKey++;
			}
			if( done ) break;
		}
	}
	vol->lock = 0;
	return TRUE;
}

const char *sack_vfs_os_get_signature( struct volume *vol ) {
	static char signature[257];
	static const char *output = "0123456789ABCDEF";
	if( !vol )
		return NULL;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	{
		static BLOCKINDEX datakey[BLOCKS_PER_BAT];
		uint8_t* usekey = vol->key?vol->usekey[BLOCK_CACHE_DATAKEY]:l.zerokey;
		signature[256] = 0;
		memset( datakey, 0, sizeof( datakey ) );
		{
			{
				size_t n;
				BLOCKINDEX this_dir_block = 0;
				BLOCKINDEX next_dir_block;
				BLOCKINDEX *next_entries;
				do {
					enum block_cache_entries cache = BLOCK_CACHE_DATAKEY;
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, cache );
					for( n = 0; n < BLOCKS_PER_BAT; n++ )
						datakey[n] ^= next_entries[n] ^ ((BLOCKINDEX*)(((uint8_t*)usekey)))[n];
					
					next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, FALSE );
					if( this_dir_block == next_dir_block )
						DebugBreak();
					if( next_dir_block == 0 )
						DebugBreak();
					this_dir_block = next_dir_block;
				}
				while( next_dir_block != EOFBLOCK );
			}
		}
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		SRG_ResetEntropy( vol->entropy );
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[vol->curseg] = 0;
		vol->datakey = (const char *)datakey;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)usekey, 128 * 8 );
		{
			int n;
			for( n = 0; n < 128; n++ ) {
				signature[n*2] = output[( usekey[n] >> 4 ) & 0xF];
				signature[n*2+1] = output[usekey[n] & 0xF];
			}
		}
	}
	vol->lock = 0;
	return signature;
}

LOGICAL _os_ScanDirectory( struct volume *vol, const char * filename
	, BLOCKINDEX *nameBlockStart
	, FPI *dirFPI, struct directory_entry *dirent
	, struct directory_entry *dirkey, int path_match ) {
	size_t n;
	int ofs = 0;
	BLOCKINDEX this_dir_block = 0;
	BLOCKINDEX next_dir_block;
	int usedNames;
	int minName;
	int curName;
	static int leadin[256];
	static int leadinDepth = 0;
	struct directory_hash_lookup_block *dirblock;
	struct directory_hash_lookup_block *dirblockkey;
	struct directory_entry *next_entries;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	leadinDepth = 0;
	do {
		enum block_cache_entries cache = BLOCK_CACHE_DIRECTORY;
		BLOCKINDEX nameBlock;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
		nameBlock = dirblock->names_first_block;
		if( filename )
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[ofs]] ^ dirblockkey->next_block[filename[ofs]];
			if( nextblock ) {
				leadin[leadinDepth++] = filename[ofs];
				ofs += 1;
				this_dir_block = nextblock;
				continue;
			}
		}
		usedNames = dirblock->used_names;
		minName = 0;
		curName = (usedNames) >> 1;
		{
			next_entries = dirblock->entries;
			while( minName < usedNames )
			//for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) 
			{
				BLOCKINDEX bi;
				enum block_cache_entries name_cache = BLOCK_CACHE_NAMES;
				struct directory_entry *entkey = dirblockkey->entries + (n=curName);
				struct directory_entry *entry = dirblock->entries + n;
				//const char * testname;
				FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
		        
				if( filename && !name_ofs )	return FALSE; // done.
				LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
				   , n, name_ofs
				   , next_entries[n].name_offset ^ entkey->name_offset
				   , next_entries[n].first_block ^ entkey->first_block
				   , filename );
				bi = next_entries[n].first_block ^ entkey->first_block;
				// if file is deleted; don't check it's name.
				if( !bi ) continue;
				// if file is end of directory, done sanning.
				if( bi == EODMARK ) return filename?FALSE:(2); // done.
				if( name_ofs > vol->dwSize ) { return FALSE; }
				//testname =
				if( filename ) {
					int d;
					LoG( "this name: %s", names );
					if( ( d = _os_MaskStrCmp( vol, filename, nameBlock, name_ofs, path_match ) ) == 0 ) {
						if( dirkey ) {
							dirkey[0] = (*entkey);
							if( dirent ) {
								if( dirFPI )
									dirFPI[0] = vol->segment[BLOCK_CACHE_DIRECTORY] * BLOCK_SIZE 
									          + ( n * sizeof( struct directory_entry ) );
								dirent->first_block = entry->first_block;
								dirent->name_offset = entry->name_offset;
								dirent->filesize = entry->filesize;
							}
						}
						LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %s", next_entries + n, name_ofs, next_entries[n].first_block ^ dirkey->first_block, filename );
						if( nameBlockStart ) nameBlockStart[0] = dirblock->names_first_block ^ dirblockkey->names_first_block;
						return TRUE;
					}
					if( d > 0 ) {
						minName = curName + 1;
					} else {
						usedNames = curName;
					}
					curName = (minName + usedNames) >> 1;
				}
			}
			return filename ? FALSE : (2); // done.;
		}
		next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, FALSE, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
#endif
		if( next_dir_block == 0 ) { DebugBreak(); return FALSE; }  // should have a last-entry before no more blocks....
		this_dir_block = next_dir_block;
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI _os_SaveFileName( struct volume *vol, BLOCKINDEX firstNameBlock, const char * filename ) {
	size_t n;
	BLOCKINDEX this_name_block = firstNameBlock;
	while( 1 ) {
		enum block_cache_entries cache = BLOCK_CACHE_NAMES;
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block, cache );
		unsigned char *name = (unsigned char*)names;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) ) {
			int c = name[0];
			if( vol->key ) c = c ^ vol->usekey[cache][(uintptr_t)name-(uintptr_t)names];
			if( !c ) {
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) ) {
					LoG( "using unused entry for new file...%" _size_f "  %" _size_f " %s", this_name_block, (uintptr_t)name - (uintptr_t)names, filename );
					if( vol->key ) {						
						for( n = 0; n < namelen + 1; n++ )
							name[n] = filename[n] ^ vol->usekey[cache][n + (name-(unsigned char*)names)];
						if( (namelen + 1) < (size_t)(((unsigned char*)names + BLOCK_SIZE) - name) )
							name[n] = vol->usekey[cache][n + (name - (unsigned char*)names)];
					} else
						memcpy( name, filename, ( namelen + 1 ) );

					sack_fwrite( vol->usekey_buffer[cache], 1, BLOCK_SIZE, vol->file );
					return ((uintptr_t)name) - ((uintptr_t)names);
				}
			}
			else
				if( _os_MaskStrCmp( vol, filename, firstNameBlock, (uintptr_t)name - (uintptr_t)names, 0 ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					return ((uintptr_t)name) - ((uintptr_t)names);
				}
			if( vol->key ) {
				while( ( name[0] ^ vol->usekey[cache][name-(unsigned char*)names] ) ) name++;
				name++;
			} else
				name = name + StrLen( (const char*)name ) + 1;
			//LoG( "new position is %" _size_f "  %" _size_f, this_name_block, (uintptr_t)name - (uintptr_t)names );
		}
		this_name_block = vfs_os_GetNextBlock( vol, this_name_block, GFB_INIT_DIRENT, TRUE );
		LoG( "Need a new directory block....", this_name_block );
	}
}

static void ConvertDirectory( struct volume *vol, const char *leadin, int leadinLength, BLOCKINDEX this_dir_block, struct directory_hash_lookup_block *orig_dirblock ) {
	size_t n;
	int ofs = 0;
	do {
		enum block_cache_entries cache = BLOCK_CACHE_DIRECTORY;
		FPI dirblockFPI;
		FPI nameoffset = 0;
		BLOCKINDEX new_dir_block;
		struct directory_hash_lookup_block *dirblock;
		struct directory_hash_lookup_block *dirblockkey;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
		{
			static int counters[256];
			static uint8_t namebuffer[18*4096]; 
			uint8_t *nameblock;
			uint8_t *namekey;
			int maxc = 0;
			int imax = 0;
			int f;
			enum block_cache_entries name_cache = BLOCK_CACHE_NAMES;
			BLOCKINDEX name_block = dirblock->names_first_block;

			do {
				uint8_t *out = namebuffer + nameoffset;
				nameblock = BTSEEK( uint8_t *, vol, name_block, name_cache );
				namekey = (uint8_t*)vol->usekey_buffer[cache];
				if( vol->key )
					for( n = 0; n < 4096; n++ )
						(*out++) = (*nameblock++) ^ (*namekey++);
				else
					for( n = 0; n < 4096; n++ )
						(*out++) = (*nameblock++);
				name_block = vfs_os_GetNextBlock( vol, name_block, 0, 0 );
				nameoffset += 4096;
			} while( name_block != EOFBLOCK );

			memset( counters, 0, sizeof( counters ) );
			// 257/85 
			for( f = 0; f < VFS_DIRECTORY_ENTRIES; f++ ) {
				BLOCKINDEX first = dirblock->entries[f].first_block ^ dirblockkey->entries[f].first_block;
				FPI name;
				int count;
				if( first == EODMARK ) break;
				name = dirblock->entries[f].name_offset ^ dirblockkey->entries[f].name_offset;
				count = (++counters[namebuffer[name]]);
				if( count > maxc )
					imax = namebuffer[name];
			}

			dirblock->next_block[imax]
				= ( new_dir_block 
				  = _os_GetFreeBlock( vol, GFB_INIT_DIRENT ) ) ^ dirblockkey->next_block[imax];
			{
				struct directory_hash_lookup_block *newDirblock;
				struct directory_hash_lookup_block *newDirblockkey;
				BLOCKINDEX newFirstNameBlock;
				int usedNames = dirblock->used_names ^ dirblockkey->used_names;
				int nf = 0;
				cache = BLOCK_CACHE_DIRECTORY;
				newDirblock = BTSEEK( struct directory_hash_lookup_block *, vol, new_dir_block, cache );
				newDirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
				newFirstNameBlock = newDirblock->names_first_block ^ newDirblockkey->names_first_block;
				
				for( f = 0; f < usedNames; f++ ) {
					BLOCKINDEX first = dirblock->entries[f].first_block ^ dirblockkey->entries[f].first_block;
					struct directory_entry *entry;
					struct directory_entry *entkey;
					struct directory_entry *newEntry;
					struct directory_entry *newEntkey;
					FPI name;
					int count;
					FPI name_ofs;
					if( first == EODMARK ) break;
					entry = dirblock->entries + (f);
					entkey = dirblockkey->entries + (f);
					name = entry->name_offset ^ entkey->name_offset;
					if( namebuffer[name] == imax ) {
						newEntry = newDirblock->entries + (nf);
						newEntkey = newDirblockkey->entries + (nf);

						name_ofs = _os_SaveFileName( vol, newFirstNameBlock, namebuffer + name + 1 ) ^ newEntkey->name_offset;
						{
							INDEX idx;
							struct sack_vfs_file  * file;
							LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
								file->entry_fpi = 0; // new entry_fpi.
							}
						}


						newEntry->filesize = (entry->filesize ^ entkey->filesize) ^ newEntkey->filesize;
						newEntry->name_offset = name_ofs;
						newEntry->first_block = (entry->first_block ^ entkey->first_block) ^ newEntkey->first_block;
						SETFLAG( vol->dirty, cache );
						nf++;

						{
							int m;
							for( m = f; m < usedNames; m++ ) {
								dirblock->entries[m].first_block = (dirblock->entries[m + 1].first_block
									^ dirblockkey->entries[m + 1].first_block)
									^ dirblockkey->entries[m].first_block;
								dirblock->entries[m].name_offset = (dirblock->entries[m + 1].name_offset
									^ dirblockkey->entries[m + 1].name_offset)
									^ dirblockkey->entries[m].name_offset;
								dirblock->entries[m].filesize = (dirblock->entries[m + 1].filesize
									^ dirblockkey->entries[m + 1].filesize)
									^ dirblockkey->entries[m].filesize;
							}
							usedNames--;
							f--;
						}
					}
				}
			}
			break;  // a set of names has been moved out of this block.
			// has block.
		}
	} while( 0 );

	// unlink here
	// unlink dirblock->names_first_block

}

static struct directory_entry * _os_GetNewDirectory( struct volume *vol, const char * filename, FPI *entFPI, struct directory_entry *dirent, struct directory_entry *_entkey ) {
	size_t n;
	const char *_filename = filename;
	static int leadin[256];
	static int leadinDepth = 0;
	BLOCKINDEX this_dir_block = 0;
	struct directory_entry *next_entries;
	LOGICAL moveMark = FALSE;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	leadinDepth = 0;
	do {
		enum block_cache_entries cache = BLOCK_CACHE_DIRECTORY;
		FPI dirblockFPI;
		int usedNames;
		int minName;
		struct directory_hash_lookup_block *dirblock;
		struct directory_hash_lookup_block *dirblockkey;
		BLOCKINDEX firstNameBlock;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
		firstNameBlock = dirblock->names_first_block^dirblockkey->names_first_block;
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[0]] ^ dirblockkey->next_block[filename[0]];
			if( nextblock ) {
				leadin[leadinDepth++] = filename[0];
				filename++;
				this_dir_block = nextblock;
				continue; // retry;
			}
		}
		usedNames = dirblock->used_names ^ dirblockkey->used_names;
		if( usedNames == VFS_DIRECTORY_ENTRIES ) {
			ConvertDirectory( vol, leadin, leadinDepth, this_dir_block, dirblock, &cache );
			continue; // retry;
		}
		{
			struct directory_entry *entkey;
			struct directory_entry *ent;
			FPI name_ofs;
			BLOCKINDEX first_blk;

			next_entries = dirblock->entries;
			dirblockFPI = sack_ftell( vol->file );
			entkey = dirblockkey->entries;
			ent = dirblock->entries;
			for( n = 0; n < usedNames; n++ ) {
				ent = dirblock->entries + (n);
				entkey = dirblockkey->entries + (n);
				name_ofs = ent->name_offset ^ entkey->name_offset;
				first_blk = ent->first_block ^ entkey->first_block;
				// not name_offset (end of list) or not first_block(free entry) use this entry
				//if( name_ofs && (first_blk > 1) )  continue;

				if( _os_MaskStrCmp( vol, filename, firstNameBlock, name_ofs, 0 ) < 0 ) {
					int m;
					for( m = dirblock->used_names; m > n; m-- ) {
						dirblock->entries[m].filesize = dirblock->entries[m - 1].filesize ^ dirblockkey->entries[m - 1].filesize;
						dirblock->entries[m].first_block = dirblock->entries[m - 1].first_block ^ dirblockkey->entries[m - 1].first_block;
						dirblock->entries[m].name_offset = dirblock->entries[m - 1].name_offset ^ dirblockkey->entries[m - 1].name_offset;
					}
					dirblock->used_names++;
				}
			}
			if( n == usedNames ) {
				dirblock->used_names = (n + 1) ^ dirblockkey->used_names;
			}
			//if( first_blk == EODMARK ) moveMark = TRUE;

			name_ofs = _os_SaveFileName( vol, firstNameBlock, filename ) ^ entkey->name_offset;
			// have to allocate a block for the file, otherwise it would be deleted.
			first_blk = _os_GetFreeBlock( vol, FALSE ) ^ entkey->first_block;
		        
			//ent = next_entries + n;
			_entkey[0] = entkey[0];
			entFPI[0] = dirblockFPI + n * sizeof( struct directory_entry );
			dirent->filesize = ent->filesize = entkey->filesize;
			dirent->name_offset = ent->name_offset = name_ofs;
			dirent->first_block = ent->first_block = first_blk;
			SETFLAG( vol->dirty, cache );
			return dirent;

			lprintf( "FATAL, SHOULD NEVER HAVE MORE than 1 CHAIN" );
			this_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, TRUE );
		}
	}
	while( 1 );

}

struct sack_vfs_file * CPROC sack_vfs_os_openfile( struct volume *vol, const char * filename ) {
	struct sack_vfs_file *file = New( struct sack_vfs_file );
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	file->entry = &file->_entry;
	if( filename[0] == '.' && filename[1] == '/' ) filename += 2;
	LoG( "sack_vfs open %s = %p on %s", filename, file, vol->volname );
	if( !_os_ScanDirectory( vol, filename, NULL, &file->entry_fpi, &file->_entry, &file->dirent_key, 0 ) ) {
		if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; Deallocate( struct sack_vfs_file *, file ); return NULL; }
		else _os_GetNewDirectory( vol, filename, &file->entry_fpi, &file->_entry, &file->dirent_key );
	}
	file->vol = vol;
	file->fpi = 0;
	file->delete_on_close = 0;
	file->_first_block = file->block = file->entry->first_block ^ file->dirent_key.first_block;
	AddLink( &vol->files, file );
	vol->lock = 0;
	return file;
}

static struct sack_vfs_file * CPROC sack_vfs_os_open( uintptr_t psvInstance, const char * filename, const char *opts ) {
	return sack_vfs_os_openfile( (struct volume*)psvInstance, filename );
}

int CPROC sack_vfs_os_exists( struct volume *vol, const char * file ) {
	LOGICAL result;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	result = _os_ScanDirectory( vol, file, NULL, NULL, NULL, NULL, 0 );
	vol->lock = 0;
	return result;
}

size_t CPROC sack_vfs_os_tell( struct sack_vfs_file *file ) { return (size_t)file->fpi; }

size_t CPROC sack_vfs_os_size( struct sack_vfs_file *file ) {	return (size_t)(file->entry->filesize ^ file->dirent_key.filesize); }

size_t CPROC sack_vfs_os_seek( struct sack_vfs_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	{
		if( ( file->fpi & ( ~BLOCK_MASK ) ) >= ( old_fpi & ( ~BLOCK_MASK ) ) ) {
			do {
				if( ( file->fpi & ( ~BLOCK_MASK ) ) == ( old_fpi & ( ~BLOCK_MASK ) ) ) {
					file->vol->lock = 0;
					return (size_t)file->fpi;
				}
				file->block = vfs_os_GetNextBlock( file->vol, file->block, FALSE, TRUE );
				old_fpi += BLOCK_SIZE;
			} while( 1 );
		}
	}
	{
		size_t n = 0;
		BLOCKINDEX b = file->_first_block;
		while( n * BLOCK_SIZE < ( pos & ~BLOCK_MASK ) ) {
			b = vfs_os_GetNextBlock( file->vol, b, FALSE, TRUE );
			n++;
		}
		file->block = b;
	}
	file->vol->lock = 0;
	return (size_t)file->fpi;
}

static void _os_MaskBlock( struct volume *vol, uint8_t* usekey, uint8_t* block, BLOCKINDEX block_ofs, size_t ofs, const char *data, size_t length ) {
	size_t n;
	block += block_ofs;
	usekey += ofs;
	if( vol->key )
		for( n = 0; n < length; n++ ) (*block++) = (*data++) ^ (*usekey++);
	else
		memcpy( block, data, length );
}

size_t CPROC sack_vfs_os_write( struct sack_vfs_file *file, const char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	LOGICAL updated = FALSE;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
	if( ofs ) {
		enum block_cache_entries cache = BLOCK_CACHE_FILE;
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			sack_fwrite( block + ofs, 1, BLOCK_SIZE - ofs, file->vol->file );
			data += BLOCK_SIZE - ofs;
			written += BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
				updated = TRUE;
			}

			file->block = vfs_os_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE - ofs;
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, length );
			sack_fwrite( file->vol->usekey_buffer[cache] + ofs, 1, BLOCK_SIZE - ofs, file->vol->file );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
				updated = TRUE;
			}
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		enum block_cache_entries cache = BLOCK_CACHE_FILE;
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( file->block < 2 ) DebugBreak();
		if( length >= BLOCK_SIZE ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, BLOCK_SIZE );
			sack_fwrite( block, 1, BLOCK_SIZE, file->vol->file );
			data += BLOCK_SIZE;
			written += BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				updated = TRUE;
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			}
			file->block = vfs_os_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE;
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, length );
			sack_fwrite( block, 1, BLOCK_SIZE, file->vol->file );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				updated = TRUE;
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			}
			length = 0;
		}
	}
	if( updated ) {
		sack_fseek( file->vol->file, (size_t)file->entry_fpi, SEEK_SET );
		sack_fwrite( &file->entry, 1, sizeof( file->entry ), file->vol->file );
	}
	file->vol->lock = 0;
	return written;
}

size_t CPROC sack_vfs_os_read( struct sack_vfs_file *file, char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) ) {
		if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < file->fpi )
			length = 0;
		else
			length = (size_t)(( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi);
	}
	if( !length ) {  file->vol->lock = 0; return 0; }

	if( ofs ) {
		enum block_cache_entries cache = BLOCK_CACHE_FILE;
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), BLOCK_SIZE - ofs );
			written += BLOCK_SIZE - ofs;
			data += BLOCK_SIZE - ofs;
			length -= BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			file->block = vfs_os_GetNextBlock( file->vol, file->block, FALSE, TRUE );
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BLOCK_CACHE_FILE;
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= BLOCK_SIZE ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, BLOCK_SIZE - ofs );
			written += BLOCK_SIZE;
			data += BLOCK_SIZE;
			length -= BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = vfs_os_GetNextBlock( file->vol, file->block, FALSE, TRUE );
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	file->vol->lock = 0;
	return written;
}

static void sack_vfs_os_unlink_file_entry( struct volume *vol, FPI entFPI, struct directory_entry *entry, struct directory_entry *entkey, BLOCKINDEX first_block, LOGICAL deleted ) {
	BLOCKINDEX block, _block;
	struct sack_vfs_file *file_found = NULL;
	struct sack_vfs_file *file;
	INDEX idx;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file ) {
		if( file->_first_block == first_block ) {
			file_found = file;
			file->delete_on_close = TRUE;
		}
	}
	if( !deleted ) {
		// delete the file entry now; this disk entry may be reused immediately.
		entry->first_block = entkey->first_block;

		sack_fseek( vol->file, (size_t)entFPI, SEEK_SET );
		sack_fwrite( entry, 1, sizeof( *entry ), vol->file );
	}

	if( !file_found ) {
		_block = block = first_block;// entry->first_block ^ entkey->first_block;
		LoG( "(marking physical deleted (again?)) entry starts at %d", block );
		// wipe out file chain BAT
		do {
			enum block_cache_entries cache = BLOCK_CACHE_BAT;
			enum block_cache_entries fileCache = BLOCK_CACHE_DATAKEY;
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
			BLOCKINDEX _thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
			//BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + (block & (BLOCKS_PER_BAT - 1)) * BLOCK_SIZE;
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( vol, block, &fileCache );
			//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
			memset( blockData, 0, BLOCK_SIZE );
			// after seek, block was read, and file position updated.
			sack_fwrite( blockData, 1, BLOCK_SIZE, vol->file );

			block = vfs_os_GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;


			_block = block;
		} while( block != EOFBLOCK );
	}
}

static void _os_shrinkBAT( struct sack_vfs_file *file ) {
	struct volume *vol = file->vol;
	BLOCKINDEX block, _block;
	size_t bsize = 0;

	_block = block = file->entry->first_block ^ file->dirent_key.first_block;
	do {
		enum block_cache_entries cache = BLOCK_CACHE_BAT;
		enum block_cache_entries data_cache = BLOCK_CACHE_DATAKEY;
		BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
		BLOCKINDEX _thiskey;
		_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
		block = vfs_os_GetNextBlock( vol, block, FALSE, FALSE );
		if( bsize > (file->entry->filesize ^ file->dirent_key.filesize) ) {
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, &data_cache );
			//LoG( "clearing a datablock after a file..." );
			memset( blockData, 0, BLOCK_SIZE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
		} else {
			bsize++;
			if( bsize > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, &data_cache );
				//LoG( "clearing a partial datablock after a file..., %d, %d", BLOCK_SIZE-(file->entry->filesize & (BLOCK_SIZE-1)), ( file->entry->filesize & (BLOCK_SIZE-1)) );
				memset( blockData + (file->entry->filesize & (BLOCK_SIZE-1)), 0, BLOCK_SIZE-(file->entry->filesize & (BLOCK_SIZE-1)) );
				this_BAT[_block & (BLOCKS_PER_BAT-1)] = ~_thiskey;
			}
		}
		_block = block;
	} while( block != EOFBLOCK );	
}

size_t CPROC sack_vfs_os_truncate( struct sack_vfs_file *file ) { file->entry->filesize = file->fpi ^ file->dirent_key.filesize; _os_shrinkBAT( file ); return (size_t)file->fpi; }

int CPROC sack_vfs_os_close( struct sack_vfs_file *file ) {
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
#ifdef DEBUG_TRACE_LOG
	{
		enum block_cache_entries cache = BLOCK_CACHE_NAMES;
		static char fname[256];
		FPI name_ofs = file->entry->name_offset ^ file->dirent_key.name_offset;
		TSEEK( const char *, file->vol, name_ofs, cache ); // have to do the seek to the name block otherwise it might not be loaded.
		MaskStrCpy( fname, sizeof( fname ), file->vol, name_ofs );
		LoG( "close file:%s(%p)", fname, file );
	}
#endif
	DeleteLink( &file->vol->files, file );
	if( file->delete_on_close ) sack_vfs_os_unlink_file_entry( file->vol, file->entry_fpi, &file->_entry, &file->dirent_key, file->_first_block, TRUE );
	file->vol->lock = 0;
	if( file->vol->closed ) sack_vfs_os_unload_volume( file->vol );
	Deallocate( struct sack_vfs_file *, file );
	return 0;
}

int CPROC sack_vfs_os_unlink_file( struct volume *vol, const char * filename ) {
	int result = 0;
	struct directory_entry entkey;
	struct directory_entry entry;
	FPI entFPI;
	if( !vol ) return 0;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	LoG( "unlink file:%s", filename );
	if( _os_ScanDirectory( vol, filename, NULL, &entFPI, &entry, &entkey, 0 ) ) {
		sack_vfs_os_unlink_file_entry( vol, entFPI, &entry, &entkey, entry.first_block ^ entkey.first_block, FALSE );
		result = 1;
	}
	vol->lock = 0;
	return result;
}

int CPROC sack_vfs_os_flush( struct sack_vfs_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_os_need_copy_write( void ) {	return FALSE; }

struct find_info {
	BLOCKINDEX this_dir_block;
	char filename[BLOCK_SIZE];
	struct volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	size_t filesize;
	CTEXTSTR mask;
	size_t thisent;
};

struct find_info * CPROC sack_vfs_os_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask )
{
	struct find_info *info = New( struct find_info );
	info->base = base;
	info->base_len = StrLen( base );
	info->mask = mask;
	info->vol = (struct volume *)psvInst;
	return info;
}

static int _os_iterate_find( struct find_info *info ) {
	struct directory_entry *next_entries;
	size_t n;
	do {
		enum block_cache_entries cache = BLOCK_CACHE_DIRECTORY;
		enum block_cache_entries name_cache = BLOCK_CACHE_NAMES;
		next_entries = BTSEEK( struct directory_entry *, info->vol, info->this_dir_block, cache );
		for( n = info->thisent; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_entry *)info->vol->usekey[cache])+n:&l.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			const char *filename;
			if( !name_ofs )	
				return 0;
			// if file is deleted; don't check it's name.
			if( !(next_entries[n].first_block ^ entkey->first_block ) )
				continue;
			if( (next_entries[n].first_block ^ entkey->first_block ) == EODMARK )
				return 0; // end of directory.
			info->filesize = (size_t)(next_entries[n].filesize ^ entkey->filesize);
			if( (name_ofs) > info->vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}
			filename = TSEEK( const char *, info->vol, name_ofs, name_cache );
			if( info->vol->key ) {
				int c;
				info->filenamelen = 0;
				while( ( c = ( ((uint8_t*)filename)[0] ^ info->vol->usekey[name_cache][name_ofs&BLOCK_MASK] ) ) ) {
					info->filename[info->filenamelen++] = c;
					filename++;
					name_ofs++;
				}
				info->filename[info->filenamelen]	 = c;
				LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			} else {
				StrCpy( info->filename, filename );
				LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			}
			info->thisent = n + 1;
			return 1;
		}
		info->thisent = 0; // new block, set new starting index.
		info->this_dir_block = vfs_os_GetNextBlock( info->vol, info->this_dir_block, FALSE, FALSE );
	}
	while( info->this_dir_block != EOFBLOCK );
	return 0;
}

int CPROC sack_vfs_os_find_first( struct find_info *info ) {
	info->this_dir_block = 0;
	info->thisent = 0;
	return _os_iterate_find( info );
}

int CPROC sack_vfs_os_find_close( struct find_info *info ) { Deallocate( struct find_info*, info ); return 0; }
int CPROC sack_vfs_os_find_next( struct find_info *info ) { return _os_iterate_find( info ); }
char * CPROC sack_vfs_os_find_get_name( struct find_info *info ) { return info->filename; }
size_t CPROC sack_vfs_os_find_get_size( struct find_info *info ) { return info->filesize; }
LOGICAL CPROC sack_vfs_os_find_is_directory( struct find_cursor *cursor ) { return FALSE; }
LOGICAL CPROC sack_vfs_os_is_directory( uintptr_t psvInstance, const char *path ) {
	if( path[0] == '.' && path[1] == 0 ) return TRUE;
	{
		struct volume *vol = (struct volume *)psvInstance;
		if( _os_ScanDirectory( vol, path, NULL, NULL, NULL, NULL, 1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}

LOGICAL CPROC sack_vfs_os_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct volume *vol = (struct volume *)psvInstance;
	// fail if the names are the same.
	if( strcmp( original, newname ) == 0 )
		return FALSE;
	if( vol ) {
		struct directory_entry entkey;
		struct directory_entry entry;
		BLOCKINDEX namesBlock;
		FPI entFPI;
		while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
		if( ( _os_ScanDirectory( vol, original, &namesBlock, &entFPI, &entry, &entkey, 0 ) ) ) {
			struct directory_entry new_entkey;
			struct directory_entry new_entry;
			if( (_os_ScanDirectory( vol, newname, &namesBlock, &entFPI, &new_entry, &new_entkey, 0 )) ) {
				vol->lock = 0;
				sack_vfs_os_unlink_file( vol, newname );
				while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
			}
			entry.name_offset = _os_SaveFileName( vol, namesBlock, newname ) ^ entkey.name_offset;
			sack_fseek( vol->file, (size_t)entFPI, SEEK_SET );
			sack_fwrite( &entry, 1, sizeof( entry ), vol->file );
			vol->lock = 0;
			return TRUE;
		}
		vol->lock = 0;
	}
	return FALSE;
}

#ifndef USE_STDIO
static struct file_system_interface sack_vfs_os_fsi = {
                                                     (void*(CPROC*)(uintptr_t,const char *, const char*))sack_vfs_os_open
                                                   , (int(CPROC*)(void*))sack_vfs_os_close
                                                   , (size_t(CPROC*)(void*,char*,size_t))sack_vfs_os_read
                                                   , (size_t(CPROC*)(void*,const char*,size_t))sack_vfs_os_write
                                                   , (size_t(CPROC*)(void*,size_t,int))sack_vfs_os_seek
                                                   , (void(CPROC*)(void*))sack_vfs_os_truncate
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_os_unlink_file
                                                   , (size_t(CPROC*)(void*))sack_vfs_os_size
                                                   , (size_t(CPROC*)(void*))sack_vfs_os_tell
                                                   , (int(CPROC*)(void*))sack_vfs_os_flush
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_os_exists
                                                   , sack_vfs_os_need_copy_write
                                                   , (struct find_cursor*(CPROC*)(uintptr_t,const char *,const char *))             sack_vfs_os_find_create_cursor
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_first
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_close
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_next
                                                   , (char*(CPROC*)(struct find_cursor*))           sack_vfs_os_find_get_name
                                                   , (size_t(CPROC*)(struct find_cursor*))          sack_vfs_os_find_get_size
                                                   , sack_vfs_os_find_is_directory
                                                   , sack_vfs_os_is_directory
                                                   , sack_vfs_os_rename
                                                   };

PRIORITY_PRELOAD( Sack_VFS_OS_Register, CONFIG_SCRIPT_PRELOAD_PRIORITY - 2 )
{
#ifdef ALT_VFS_NAME
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME "-os.runner"
#else
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME "-os"
#endif
	sack_register_filesystem_interface( DEFAULT_VFS_NAME, &sack_vfs_os_fsi );
}

PRIORITY_PRELOAD( Sack_VFS_OS_RegisterDefaultFilesystem, SQL_PRELOAD_PRIORITY + 1 ) {
	if( SACK_GetProfileInt( GetProgramName(), "SACK/VFS/Mount FS VFS", 0 ) ) {
		struct volume *vol;
		TEXTCHAR volfile[256];
		TEXTSTR tmp;
		SACK_GetProfileString( GetProgramName(), "SACK/VFS/OS File", "*/../assets.os", volfile, 256 );
		tmp = ExpandPath( volfile );
		vol = sack_vfs_os_load_volume( tmp );
		Deallocate( TEXTSTR, tmp );
		sack_mount_filesystem( "sack_shmem-os", sack_get_filesystem_interface( DEFAULT_VFS_NAME )
		                     , 900, (uintptr_t)vol, TRUE );
	}
}

#endif

SACK_VFS_NAMESPACE_END
