#if !defined( SACK_AMALGAMATE ) || defined( __cplusplus )

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
#  include <sack_vfs.h>
#  include <procreg.h>
#  include <salty_generator.h>
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
#define sack_fseek(a,b,c)     fseek(a,(long)b,c)
#define sack_fclose(a)        fclose(a)
#define sack_fread(a,b,c,d)   fread(a,b,c,d)
#define sack_fwrite(a,b,c,d)  fwrite(a,b,c,d)
#define sack_ftell(a)         ftell(a)
#undef StrDup
#define StrDup(a)             strdup(a)
#define free(a)               Deallocate( POINTER, a )

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

#else
#define free(a)               Deallocate( POINTER, a )

#endif

SACK_VFS_NAMESPACE

#ifdef __cplusplus
namespace objStore {
#endif
//#define PARANOID_INIT

//#define DEBUG_TRACE_LOG
//#define DEBUG_FILE_OPS
//#define DEBUG_DISK_IO
//#define DEBUG_DIRECTORIES

#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#define LoG_( a,... ) _lprintf(DBG_RELAY)( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#define LoG_( a,... )
#endif

#define FILE_BASED_VFS
#define VIRTUAL_OBJECT_STORE
#include "vfs_internal.h"
#define vfs_SEEK vfs_os_SEEK
#define vfs_BSEEK vfs_os_BSEEK

#define l vfs_os_local
static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
	uint16_t index[256][256];
	char leadin[256];
	int leadinDepth;
	PLINKQUEUE plqCrypters;
} l;
#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)
#define EODMARK   (1)
#define GFB_INIT_NONE       0
#define GFB_INIT_DIRENT     1
#define GFB_INIT_NAMES      2
#define GFB_INIT_PATCHBLOCK 3
// End Of Text Block
#define UTF8_EOTB 0xFF
// End Of Text
#define UTF8_EOT 0xFE

// use this character in hash as parent directory (block & char)
#define DIRNAME_CHAR_PARENT 0xFF



PREFIX_PACKED struct directory_hash_lookup_block
{
	BLOCKINDEX next_block[256];
	struct directory_entry entries[VFS_DIRECTORY_ENTRIES];
	BLOCKINDEX names_first_block;
	uint8_t used_names;
} PACKED;

PREFIX_PACKED struct directory_patch_block
{
	union direction_patch_block_entry_union {
		struct direction_patch_block_entry {
			BIT_FIELD index : 8;
			BIT_FIELD hash_block : 24;
		} dirIndex;
		uint32_t raw;
	}entries[(BLOCK_SIZE-sizeof(BLOCKINDEX))/sizeof(uint32_t)];
	uint8_t usedEntries;
	BLOCKINDEX morePatches;
} PACKED;


PREFIX_PACKED struct directory_patch_ref_block
{
	PREFIX_PACKED struct directory_patch_ref_entry {
		BLOCKINDEX patchBlockStart;
		BLOCKINDEX dirBlock; // first patch block 
		uint16_t patchNum;
		uint8_t dirEntry; // which directory entry this patches
	} entries[(BLOCK_SIZE)/sizeof( struct directory_patch_ref_entry )] PACKED;
} PACKED;


enum sack_vfs_os_seal_states {
	SACK_VFS_OS_SEAL_NONE = 0,
	SACK_VFS_OS_SEAL_LOAD,
	SACK_VFS_OS_SEAL_VALID,
	SACK_VFS_OS_SEAL_STORE,
	SACK_VFS_OS_SEAL_INVALID,  // validate failed (read whole file check)
	SACK_VFS_OS_SEAL_CLEARED,  // stored patch is writeable
	SACK_VFS_OS_SEAL_STORE_PATCH,  // stored patch new sealant (after read valid, new write)
};


PREFIX_PACKED struct sealant_suffix {
	BLOCKINDEX firstPatchBlock;
	BLOCKINDEX patchRefBlock; // first patch block 
	uint16_t patchRefIndex; // patch entry that this is a patch to.
}PACKED;


static BLOCKINDEX _os_GetFreeBlock_( struct volume *vol, int init DBG_PASS );
#define _os_GetFreeBlock(v,i) _os_GetFreeBlock_(v,i DBG_SRC )

LOGICAL _os_ScanDirectory_( struct volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_file *file
	, int path_match
	, char *leadin
	, int *leadinDepth
);
#define _os_ScanDirectory(v,f,db,nb,file,pm) ((l.leadinDepth = 0), _os_ScanDirectory_(v,f,db,nb,file,pm, l.leadin, &l.leadinDepth ))

static BLOCKINDEX vfs_os_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand );
static LOGICAL _os_ExpandVolume( struct volume *vol );

#define vfs_os_BSEEK(v,b,c) vfs_os_BSEEK_(v,b,c DBG_SRC )
uintptr_t vfs_os_BSEEK_( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index DBG_PASS );

// seek by byte position from a starting block; as file; result with an offset into a block.
uintptr_t vfs_os_FSEEK( struct volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index ) {
	uint8_t *data;
	while( firstblock != EOFBLOCK && offset >= BLOCK_SIZE ) {
		//LoG( "Skipping a whole block of 'file' %d %d", firstblock, offset );
		firstblock = vfs_os_GetNextBlock( vol, firstblock, 0, 0 );
		offset -= BLOCK_SIZE;
	}
	data = (uint8_t*)vfs_os_BSEEK_( vol, firstblock, cache_index DBG_NULL );
	return (uintptr_t)(data + (offset));
}

#define tolower_(c) (c)

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
	for( ;s1[0] && ((unsigned char)s2[0] != UTF8_EOT) && (s1[0] == s2[0]) && maxlen;
		  s1++, s2++, maxlen-- );
	if( maxlen )
		return tolower_(s1[0]) - (((unsigned char)s2[0] == UTF8_EOT)?0:tolower_(s2[0]));
	return 0;
}

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int _os_MaskStrCmp( struct volume *vol, const char * filename, BLOCKINDEX nameBlock, FPI name_offset, int path_match ) {
	enum block_cache_entries cache = BC(NAMES);
	const char *dirname = (const char*)vfs_os_FSEEK( vol, nameBlock, name_offset, &cache );
	const char *dirkey;
	if( !dirname ) return 1;
	dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK );
	if( vol->key ) {
		int c;
		while( ((unsigned char)( c = (dirname[0] ^ dirkey[0] )) != UTF8_EOT )
			  && filename[0] ) {
			int del = tolower_(filename[0]) - tolower_(c);
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
		while( UTF8_EOT != (unsigned char)( c = ( vol->usekey_buffer[BC(NAMES)][name_offset&BLOCK_MASK] ^ vol->usekey[BC(NAMES)][name_offset&BLOCK_MASK] ) ) ) {
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
		StrCpyEx( output, (const char *)(vol->usekey[BC(NAMES)] + (name_offset & BLOCK_MASK )), outlen );
	}
}
#endif

#ifdef DEBUG_DIRECTORIES
static int _os_dumpDirectories( struct volume *vol, BLOCKINDEX start, LOGICAL init ) {
	struct directory_hash_lookup_block *dirBlock;
	struct directory_hash_lookup_block *dirBlockKey;
	struct directory_entry *next_entries;
	static char leadin[256];
	static int leadinDepth;
	char outfilename[256];
	int outfilenamelen;
	size_t n;
	if( init )
		leadinDepth = 0;
	{
		enum block_cache_entries cache = BC( DIRECTORY );
		enum block_cache_entries name_cache = BC( NAMES );

		dirBlock = BTSEEK( struct directory_hash_lookup_block *, vol, start, cache );
		dirBlockKey = (struct directory_hash_lookup_block *)vol->usekey[cache];

		lprintf( "leadin : %*.*s %d %d", leadinDepth, leadinDepth, leadin, leadinDepth, dirBlock->used_names ^ dirBlockKey->used_names );
		next_entries = dirBlock->entries;

		for( n = 0; n < dirBlock->used_names ^ dirBlockKey->used_names; n++ ) {
			struct directory_entry *entkey = (vol->key) ? ((struct directory_hash_lookup_block *)vol->usekey[cache])->entries + n : &l.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			const char *filename;
			int l;

			// if file is deleted; don't check it's name.
			if( (name_ofs) > vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( vol, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
			if( !filename ) return;
			outfilenamelen = 0;
			for( l = 0; l < leadinDepth; l++ ) outfilename[outfilenamelen++] = leadin[l];

			if( vol->key ) {
				int c;
				while( (c = (((uint8_t*)filename)[0] ^ vol->usekey[name_cache][name_ofs&BLOCK_MASK])) ) {
					outfilename[outfilenamelen++] = c;
					filename++;
					name_ofs++;
				}
				outfilename[outfilenamelen] = c;
			}
			else {
				StrCpy( outfilename + outfilenamelen, filename );
			}
			//if( strlen( outfilename ) < 40 ) DebugBreak();
			lprintf( "%3d filename: %5d %s", n, name_ofs, outfilename );
		}

		for( n = 0; n < 256; n++ ) {
			BLOCKINDEX block = dirBlock->next_block[n] ^ dirBlockKey->next_block[n];
			if( block ) {
				lprintf( "Found directory with char '%c'", n );
				leadin[leadinDepth] = n;
				leadinDepth = leadinDepth + 1;
				_os_dumpDirectories( vol, block, 0 );
				leadinDepth = leadinDepth - 1;
			}
		}
	};
	return 0;
}
#endif


#define _os_updateCacheAge(v,c,s,a,l) _os_updateCacheAge_(v,c,s,a,l DBG_SRC )
static void _os_updateCacheAge_( struct volume *vol, enum block_cache_entries *cache_idx, BLOCKINDEX segment, uint8_t *age, int ageLength DBG_PASS ) {
	int n, m;
	int least;
	int nLeast;
	enum block_cache_entries cacheRoot = cache_idx[0];
	BLOCKINDEX *test_segment = vol->segment + cacheRoot;
	least = ageLength + 1;
#ifdef DEBUG_CACHE_AGING
	lprintf( "age start:" );
	LogBinary( age, ageLength );
	{
		int z;
		char buf[256];
		int ofs = 0;
		for( z = 0; z < ageLength; z++ ) ofs += snprintf( buf + ofs, 256 - ofs, "%x ", vol->bufferFPI[z+ cacheRoot] );
		lprintf( "%s", buf );
	}
#endif
	for( n = 0; n < (ageLength); n++,test_segment++ ) {
		if( test_segment[0] == segment ) {
			//if( pFile ) LoG_( "Cache found existing segment already. %d at %d(%d)", (int)segment, (cache_idx[0]+n), (int)n );
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) {
				if( !age[m] ) break;
				if( age[m] > age[n] )
					age[m]--;
			}
			age[n] = m;
#ifdef DEBUG_CACHE_AGING
			lprintf( "age end:" );
			LogBinary( age, ageLength );
#endif
			return;
			//break;
		}
		if( !age[n] ) { // end of list, empty entry.
			//LoG_( "Cache found unused segment already. %d at %d(%d)", (int)segment, (cache_idx[0] + n), (int)n );
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) { // age entries up to this one.
				if( !age[m] ) break;
				if( age[m] > ( n + 1 ) )
					age[m]--;
			}
			vol->segment[cache_idx[0]] = segment;
			age[n] = n + 1; // make this one newest
			break;
		}
		if( (age[n] < least) && !TESTFLAG( vol->seglock, cache_idx[0] + n) ) {
			least = age[n]; 
			nLeast = n; // this one will be oldest, unlocked candidate
		}
	}
	if( n == (ageLength) ) {
		int useCache = cacheRoot + nLeast;
#ifdef _DEBUG
		if( least > ageLength ) DebugBreak();
#endif
		for( n = 0; n < (ageLength); n++ ) {  // age evernthing.
			if( age[n] > least )
				age[n]--;
		}
		cache_idx[0] = (enum block_cache_entries)useCache;
		age[nLeast] = (ageLength); // make this one the newest, and return it.
		vol->segment[useCache] = segment;

		if( TESTFLAG( vol->dirty, useCache ) || TESTFLAG( vol->_dirty, useCache ) ) {
#ifdef DEBUG_DISK_IO
			LoG_( "MUST CLAIM SEGMENT Flush dirty segment: %d %x %d", nLeast, vol->bufferFPI[useCache], vol->segment[useCache] );
#endif
			sack_fseek( vol->file, (size_t)vol->bufferFPI[useCache], SEEK_SET );
			sack_fwrite( vol->usekey_buffer[useCache], 1, BLOCK_SIZE, vol->file );
			RESETFLAG( vol->dirty, useCache );
			RESETFLAG( vol->_dirty, useCache );
		}
	}
	{
		// read new buffer for new segment
		sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_idx[0]] = (size_t)((segment-1)*BLOCK_SIZE)), SEEK_SET );
#ifdef DEBUG_DISK_IO
		LoG_( "Read into block: fpi:%x cache:%d n:%d  seg:%d", (int)vol->bufferFPI[cache_idx[0]], (int)cache_idx[0] , (int)n, (int)segment );
#endif
		if( !sack_fread( vol->usekey_buffer[cache_idx[0]], 1, BLOCK_SIZE, vol->file ) )
			memset( vol->usekey_buffer[cache_idx[0]], 0, BLOCK_SIZE );
	}
#ifdef DEBUG_CACHE_AGING
	lprintf( "age end2:" );
	LogBinary( age, ageLength );
#endif
}

#define _os_UpdateSegmentKey(v,c,s) _os_UpdateSegmentKey_(v,c,s DBG_SRC )

static enum block_cache_entries _os_UpdateSegmentKey_( struct volume *vol, enum block_cache_entries cache_idx, BLOCKINDEX segment DBG_PASS )
{
	//lprintf( "UPDATE OS SEGKEY %d %d", cache_idx, segment );
	if( cache_idx == BC(FILE) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->fileCacheAge, (BC(FILE_LAST) - BC(FILE)) DBG_RELAY );
	}
	else if( cache_idx == BC(NAMES) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->nameCacheAge, (BC(NAMES_LAST) - BC(NAMES)) DBG_RELAY );
	}
#ifdef VIRTUAL_OBJECT_STORE
	else if( cache_idx == BC(DIRECTORY) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->dirHashCacheAge, (BC(DIRECTORY_LAST) - BC(DIRECTORY)) DBG_RELAY );
	}
	else if( cache_idx == BC( BAT ) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->batHashCacheAge, (BC(BAT_LAST) - BC(BAT)) DBG_RELAY );
	}
#endif
	else {
		if( vol->segment[cache_idx] != segment ) {
			if( TESTFLAG( vol->dirty, cache_idx ) || TESTFLAG( vol->_dirty, cache_idx ) ) {
#ifdef DEBUG_DISK_IO
				LoG_( "MUST CLAIM SEGEMNT Flush dirty segment: %x %d", vol->bufferFPI[cache_idx], vol->segment[cache_idx] );
#endif
				sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx], SEEK_SET );
				sack_fwrite( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file );
				RESETFLAG( vol->dirty, cache_idx );
				RESETFLAG( vol->_dirty, cache_idx );
#ifdef DEBUG_DISK_IO
				LoG( "Flush dirty sector: %d", cache_idx, vol->bufferFPI[cache_idx]/BLOCK_SIZE );
#endif
			}

			// read new buffer for new segment
			sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_idx]=(segment - 1)*BLOCK_SIZE), SEEK_SET);
#ifdef DEBUG_DISK_IO
			LoG( "OS VFS read old sector: fpi:%d %d %d", (int)vol->bufferFPI[cache_idx], cache_idx, segment );
#endif
			if( !sack_fread( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file ) ) {
				//lprintf( "Cleared BLock on failed read." );
				memset( vol->usekey_buffer[cache_idx], 0, BLOCK_SIZE );
			}
		}
		vol->segment[cache_idx] = segment;
	}

	if( !vol->key ) {
		//lprintf( "Resulting stored segment in %d", cache_idx );
		return cache_idx;
	}
	while( ((segment)*BLOCK_SIZE) > vol->dwSize ) {
		if( !_os_ExpandVolume( vol ) ) {
			DebugBreak();
		}
	}
	//vol->segment[cache_idx] = segment;
	if( vol->segment[cache_idx] == vol->_segment[cache_idx] ) {
		//lprintf( "Resulting stored segment in %d", cache_idx );
		return cache_idx;
	}
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
	//LoG( "Resulting stored segment in %d", cache_idx );
	return cache_idx;
}

static LOGICAL _os_ValidateBAT( struct volume *vol ) {
	BLOCKINDEX first_slab = 0;
	BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
	BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
	BLOCKINDEX n;
	enum block_cache_entries cache = BC(BAT);
	if( vol->key ) {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT; 
			BLOCKINDEX *blockKey;
			BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			//sack_fseek( vol->file, (size_t)n * BLOCK_SIZE, SEEK_SET );
			//if( !sack_fread( vol->usekey_buffer[BC(BAT)], 1, BLOCK_SIZE, vol->file ) ) return FALSE;
			//BAT = (BLOCKINDEX*)vol->usekey_buffer[BC(BAT)];
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			_os_UpdateSegmentKey( vol, cache, n + 1 );

			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[0] ^ blockKey[0];
				BAT++; blockKey++;
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n + m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	} else {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			BAT = (BLOCKINDEX*)vol->usekey_buffer[cache];
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				BLOCKINDEX block = BAT[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n + m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	if( !_os_ScanDirectory( vol, NULL, 0, NULL, NULL, 0 ) ) return FALSE;
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
LOGICAL _os_ExpandVolume( struct volume *vol ) {
	LOGICAL created = FALSE;
	LOGICAL path_checked = FALSE;
	size_t oldsize = vol->dwSize;
	if( vol->file && vol->read_only ) return TRUE;
	if( !vol->file ) {
		char *fname;
		char *iface;
		char *tmp;
		{
			char *tmp = StrDup( vol->volname );
			char *dir = (char*)pathrchr( tmp );
			if( dir ) {
				dir[0] = 0;
				if( !IsPath( tmp ) ) MakePath( tmp );
			}
			free( tmp );
			//Deallocate( char*, tmp );
		}
		if( tmp =(char*)StrChr( vol->volname, '@' ) ) {
			tmp[0] = 0;
			iface = (char*)vol->volname;
			fname = tmp + 1;
			struct file_system_mounted_interface *mount = sack_get_mounted_filesystem( iface );
			//struct file_system_interface *iface = sack_get_filesystem_interface( iface );
			if( !sack_exists( fname ) ) {
				vol->file = sack_fopenEx( 0, fname, "rb+", mount );
				if( !vol->file )
					vol->file = sack_fopenEx( 0, fname, "wb+", mount );
				created = TRUE;
			}
			else
				vol->file = sack_fopenEx( 0, fname, "wb+", mount );
			tmp[0] = '@';
		}
		else {
			vol->file = sack_fopen( 0, vol->volname, "rb+" );
			if( !vol->file ) {
				created = TRUE;
				vol->file = sack_fopen( 0, vol->volname, "wb+" );
			}
		}
		sack_fseek( vol->file, 0, SEEK_END );
		vol->dwSize = sack_ftell( vol->file );
		if( vol->dwSize == 0 )
			created = TRUE;
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
		_os_UpdateSegmentKey( vol, BC(BAT), 1 );
		((BLOCKINDEX*)vol->usekey_buffer[BC(BAT)])[0]
			= EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC(BAT)])[0];
		SETFLAG( vol->dirty, BC(BAT) );
		vol->bufferFPI[BC( BAT )] = 0;
		{
			BLOCKINDEX dirblock = _os_GetFreeBlock( vol, GFB_INIT_DIRENT );
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
		//LoG( "RETURNING SEEK CACHED %p %d  0x%x   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)offset, (int)seg );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + (offset&BLOCK_MASK);
	}
}

// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block after first BAT.
uintptr_t vfs_os_BSEEK_( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index DBG_PASS ) {
	BLOCKINDEX b = ( 1 + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR) + ( block & (BLOCKS_PER_BAT-1) ) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		cache_index[0] = _os_UpdateSegmentKey_( vol, cache_index[0], seg DBG_RELAY );
		//LoG( "RETURNING BSEEK CACHED %p  %d %d %d  0x%x  %d   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)(block>>BLOCK_SHIFT), (int)(BLOCKS_PER_BAT-1), (int)b, (int)block, (int)seg );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + (b&BLOCK_MASK);
	}
}


static BLOCKINDEX _os_GetFreeBlock_( struct volume *vol, int init DBG_PASS )
{
	size_t n;
	unsigned int b = 0;
	enum block_cache_entries cache = BC( BAT );
	BLOCKINDEX *current_BAT;// = TSEEK( BLOCKINDEX*, vol, 0, cache );
	BLOCKINDEX *blockKey;
	BLOCKINDEX check_val;
	if( vol->pdlFreeBlocks->Cnt ) {
		BLOCKINDEX newblock = ((BLOCKINDEX*)GetDataItem( &vol->pdlFreeBlocks, vol->pdlFreeBlocks->Cnt - 1 ))[0];
		check_val = 0;
		b = (unsigned int)(newblock / BLOCKS_PER_SECTOR);
		n = newblock % BLOCKS_PER_SECTOR;
		vol->pdlFreeBlocks->Cnt--;
	}
	else {
		check_val = EOBBLOCK;
		b = (unsigned int)(vol->lastBatBlock / BLOCKS_PER_SECTOR);
		n = vol->lastBatBlock % BLOCKS_PER_SECTOR;
	}
	//lprintf( "check, start, b, n %d %d %d %d", (int)check_val, (int) vol->lastBatBlock, (int)b, (int)n );
	current_BAT = TSEEK( BLOCKINDEX*, vol, b*BLOCKS_PER_SECTOR*BLOCK_SIZE, cache ) + n;
	blockKey = ((BLOCKINDEX*)vol->usekey[cache]) + n;

	if( !current_BAT ) return 0;

	current_BAT[0] = EOFBLOCK ^ blockKey[0];

	if( (check_val == EOBBLOCK) ) {
		if( n < (BLOCKS_PER_BAT - 1) ) {
			current_BAT[1] = EOBBLOCK ^ blockKey[1];
			vol->lastBatBlock++;
		}
		else {
			cache = BC( BAT );
			current_BAT = TSEEK( BLOCKINDEX*, vol, (b + 1) * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			current_BAT[0] = EOBBLOCK ^ blockKey[0];
			vol->lastBatBlock = (b + 1) * BLOCKS_PER_SECTOR;
			//lprintf( "Set last block....%d", (int)vol->lastBatBlock );
		}
		SETFLAG( vol->dirty, cache );
	}

	if( init ) {
		enum block_cache_entries newcache;
		if( init == GFB_INIT_DIRENT ) {
			struct directory_hash_lookup_block *dir;
			struct directory_hash_lookup_block *dirkey;
			LoG( "Create new directory: result %d", (int)(b * BLOCKS_PER_BAT + n) );
			newcache = _os_UpdateSegmentKey_( vol, BC( DIRECTORY ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );

			dir = (struct directory_hash_lookup_block *)vol->usekey_buffer[newcache];
			dirkey = (struct directory_hash_lookup_block *)vol->usekey[newcache];
			dir->names_first_block = _os_GetFreeBlock( vol, GFB_INIT_NAMES ) ^ dirkey->names_first_block;
			dir->used_names = 0 ^ dirkey->used_names;
			//((struct directory_hash_lookup_block*)(vol->usekey_buffer[newcache]))->entries[0].first_block = EODMARK ^ ((struct directory_hash_lookup_block*)vol->usekey[cache])->entries[0].first_block;
		}
		else if( init == GFB_INIT_NAMES ) {
			newcache = _os_UpdateSegmentKey_( vol, BC( NAMES ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );
			((char*)(vol->usekey_buffer[newcache]))[0] = (char)UTF8_EOTB ^ ((char*)vol->usekey[newcache])[0];
			//LoG( "New Name Buffer: %x %p", vol->segment[newcache], vol->usekey_buffer[newcache] );
		}
		else {
			//	memcpy( ((uint8_t*)vol->disk) + (vol->segment[newcache]-1) * BLOCK_SIZE, vol->usekey[newcache], BLOCK_SIZE );
			newcache = _os_UpdateSegmentKey_( vol, BC( FILE ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
		}
		SETFLAG( vol->dirty, newcache );
	}
	LoG( "Return block:%d   %d  %d", (int)(b*BLOCKS_PER_BAT + n), (int)b, (int)n );
	return b * BLOCKS_PER_BAT + n;
}

static BLOCKINDEX vfs_os_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand ) {
	BLOCKINDEX sector = block / BLOCKS_PER_BAT;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
	BLOCKINDEX check_val;
	if( !this_BAT ) return 0; // if this passes, later ones will also.
#ifdef _DEBUG
	if( !block ) DebugBreak();
#endif

	check_val = (this_BAT[block & (BLOCKS_PER_BAT - 1)]) ^ ((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)];
#ifdef _DEBUG
	if( !check_val ) {
		lprintf( "STOP: %p  %d  %d  %d", this_BAT, (int)check_val, (int)(block), (int)sector );
		DebugBreak();
	}
#endif
	if( check_val == EOBBLOCK ) {
		(this_BAT[block & (BLOCKS_PER_BAT-1)]) = EOFBLOCK^((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)];
		if( block < (BLOCKS_PER_BAT - 1) )
			(this_BAT[1 + block & (BLOCKS_PER_BAT - 1)]) = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC( BAT )])[1 + block & (BLOCKS_PER_BAT - 1)];
		//else
		//	lprintf( "THIS NEEDS A NEW BAT BLOCK TO MOVE THE MARKER" );//
	}
	if( check_val == EOFBLOCK || check_val == EOBBLOCK ) {
		if( expand ) {
			BLOCKINDEX key = vol->key?((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)]:0;
			check_val = _os_GetFreeBlock( vol, init );
#ifdef _DEBUG
			if( !check_val )DebugBreak();
#endif
			// free block might have expanded...
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), cache );
			if( !this_BAT ) return 0;
			// segment could already be set from the _os_GetFreeBlock...
			this_BAT[block & (BLOCKS_PER_BAT-1)] = check_val ^ key;
			SETFLAG( vol->dirty, cache );
		}
	}
#ifdef _DEBUG
	if( !check_val )DebugBreak();
#endif
	LoG( "return next block:%d %d", (int)block, (int)check_val );
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
	uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * BC(COUNT) + BLOCK_SIZE + SHORTKEY_LENGTH;
	if( !vol->key_buffer ) {
		int n;
		vol->key_buffer = (uint8_t*)HeapAllocateAligned( NULL, size, 4096 );// NewArray( uint8_t, size );
		memset( vol->key_buffer, 0, size );
		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey_buffer[n] = vol->key_buffer + (n + 1) * BLOCK_SIZE;
		}
		for( n = 0; n < BC( COUNT ); n++ ) {
			vol->segment[n] = ~0;
			vol->_segment[n] = ~0;
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

		vol->key = (uint8_t*)HeapAllocateAligned( NULL, size, 4096 ); //NewArray( uint8_t, size );

		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey[n] = vol->key + (n + 1) * BLOCK_SIZE;
		}
		vol->segkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1);
		vol->sigkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1) + SHORTKEY_LENGTH;
		vol->curseg = BC(DIRECTORY);
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, BLOCK_SIZE * 8 );
	}
	else {
		int n;
		for( n = 0; n < BC(COUNT); n++ )
			vol->usekey[n] = l.zerokey;
		vol->segkey = l.zerokey;
		vol->sigkey = l.zerokey;
		vol->key = NULL;
	}
}

void sack_vfs_os_flush_volume( struct volume * vol ) {
	{
		INDEX idx;
		for( idx = 0; idx < BC( COUNT ); idx++ )
			if( TESTFLAG( vol->dirty, idx ) || TESTFLAG( vol->_dirty, idx ) ) {
				LoG( "Flush dirty segment: %zx %d", vol->bufferFPI[idx], vol->segment[idx] );
				sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
				sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
				RESETFLAG( vol->dirty, idx );
				RESETFLAG( vol->_dirty, idx );
			}
	}
}

static uintptr_t volume_flusher( PTHREAD thread ) {
	struct volume *vol = (struct volume *)GetThreadParam( thread );
	while( 1 ) {

		while( 1 ) {
			int updated;
			INDEX idx;
			updated = 0;
			if( !LockedExchange( &vol->lock, 1 ) ) {
				// this could be 'faster' testing the whole
				// flag type size data.
				for( idx = 0; idx < BC( COUNT ); idx++ )
					if( TESTFLAG( vol->dirty, idx ) ) {
						updated = 1;
						SETFLAG( vol->_dirty, idx );
						RESETFLAG( vol->dirty, idx );
					}

				if( updated ) {
					vol->lock = 0; // data changed, don't flush.
					WakeableSleep( 256 );
				}
				else 
					break; // have lock, break; flush dirty sectors(if any)
			}
			else // didn't get lock, wait.
				Relinquish();
		}

		{
			INDEX idx;
			for( idx = 0; idx < BC(COUNT); idx++ )
				if( TESTFLAG( vol->_dirty, idx ) ) {
					sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
					sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
					RESETFLAG( vol->_dirty, idx );
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
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->volname = StrDup( filepath );
	_os_AssignKey( vol, NULL, NULL );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { Deallocate( struct volume*, vol ); return NULL; }
#ifdef DEBUG_DIRECTORIES
	_os_dumpDirectories( vol, 0, 1 );
#endif
	return vol;
}

void sack_vfs_os_unload_volume( struct volume * vol );

struct volume *sack_vfs_os_load_crypt_volume( const char * filepath, uintptr_t version, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	if( !version ) version = 2;
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->clusterKeyVersion = version - 1;
	vol->volname = StrDup( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	_os_AssignKey( vol, userkey, devkey );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { sack_vfs_os_unload_volume( vol ); return NULL; }
	return vol;
}

void sack_vfs_os_unload_volume( struct volume * vol ) {
	INDEX idx;
	struct sack_vfs_file *file;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		break;
	if( file ) {
		vol->closed = TRUE;
		return;
	}
	sack_vfs_os_flush_volume( vol );
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
	unsigned int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	unsigned int last_bat = 0;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		BLOCKINDEX *blockKey;
		blockKey = (BLOCKINDEX*)vol->usekey[cache];
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
	_os_UpdateSegmentKey( vol, BC(DATAKEY), b + 1 );
	{
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[BC(DATAKEY)];
		uint64_t* block = (uint64_t*)vol->usekey_buffer[BC( DATAKEY )];
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block += 2; usekey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[BC(DATAKEY)];
		uint32_t* block = (uint32_t*)vol->usekey_buffer[BC(DATAKEY)];
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
		enum block_cache_entries cache = BC(BAT);
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey; 
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
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
		enum block_cache_entries cache = BC(BAT);
		BLOCKINDEX slab = (vol->dwSize + (BLOCKS_PER_SECTOR*BLOCK_SIZE-1)) / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		done = 0;
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey;
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);

			//vol->segment[BC(BAT)] = n + 1;
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
		uint8_t* usekey = vol->key?vol->usekey[BC(DATAKEY)]:l.zerokey;
		signature[256] = 0;
		memset( datakey, 0, sizeof( datakey ) );
		{
			{
				size_t n;
				BLOCKINDEX this_dir_block = 0;
				BLOCKINDEX next_dir_block;
				BLOCKINDEX *next_entries;
				do {
					enum block_cache_entries cache = BC(DATAKEY);
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, cache );
					for( n = 0; n < BLOCKS_PER_BAT; n++ )
						datakey[n] ^= next_entries[n] ^ ((BLOCKINDEX*)(((uint8_t*)usekey)))[n];
					
					next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, FALSE );
#ifdef _DEBUG
					if( this_dir_block == next_dir_block )
						DebugBreak();
					if( next_dir_block == 0 )
						DebugBreak();
#endif
					this_dir_block = next_dir_block;
				}
				while( next_dir_block != EOFBLOCK );
			}
		}
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		SRG_ResetEntropy( vol->entropy );
		vol->curseg = BC(DIRECTORY);
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


LOGICAL _os_ScanDirectory_( struct volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_file *file
	, int path_match 
	, char *leadin
	, int *leadinDepth
) {
	size_t n;
	int ofs = 0;
	BLOCKINDEX this_dir_block = dirBlockSeg;
	BLOCKINDEX next_dir_block;
	int usedNames;
	int minName;
	int curName;
	struct directory_hash_lookup_block *dirblock;
	struct directory_hash_lookup_block *dirblockkey;
	struct directory_entry *next_entries;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		BLOCKINDEX nameBlock;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
		nameBlock = dirblock->names_first_block;
		if( filename )
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[ofs]] ^ dirblockkey->next_block[filename[ofs]];
			if( nextblock ) {
				leadin[(*leadinDepth)++] = filename[ofs];
				ofs += 1;
				this_dir_block = nextblock;
				continue;
			}
		}
		else {
			for( n = 0; n < 256; n++ ) {
				BLOCKINDEX nextblock = dirblock->next_block[n] ^ dirblockkey->next_block[n];
				if( nextblock ) {
					LOGICAL r;
					leadin[(*leadinDepth)++] = (char)n;
					r = _os_ScanDirectory_( vol, NULL, nextblock, nameBlockStart, file, path_match, leadin, leadinDepth );
					(*leadinDepth)--;
					if( r )
						return r;
				}
			}
		}
		usedNames = dirblock->used_names;
		minName = 0;
		curName = (usedNames) >> 1;
		{
			next_entries = dirblock->entries;
			while( minName <= usedNames && ( curName < usedNames ) && ( curName > 0 ) )
			//for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) 
			{
				BLOCKINDEX bi;
				enum block_cache_entries name_cache = BC(NAMES);
				struct directory_entry *entkey = dirblockkey->entries + (n=curName);
				struct directory_entry *entry = dirblock->entries + n;
				//const char * testname;
				FPI name_ofs = ( next_entries[n].name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
		        
				//if( filename && !name_ofs )	return FALSE; // done.
				if(0)
				LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
				   , n, name_ofs
				   , next_entries[n].name_offset ^ entkey->name_offset
				   , next_entries[n].first_block ^ entkey->first_block
				   , filename+ofs );
				if( USS_LT( n, size_t, usedNames, int ) ) {
					bi = next_entries[n].first_block ^ entkey->first_block;
					// if file is deleted; don't check it's name.
					if( !bi ) continue;
					// if file is end of directory, done sanning.
					if( bi == EODMARK ) return filename ? FALSE : (2); // done.
					if( name_ofs > vol->dwSize ) { return FALSE; }
				}
				//testname =
				if( filename ) {
					int d;
					//LoG( "this name: %s", names );
					if( ( d = _os_MaskStrCmp( vol, filename+ofs, nameBlock, name_ofs, path_match ) ) == 0 ) {
						if( file )
						{
							file->dirent_key = (*entkey);
							file->cache = cache;
							file->entry_fpi = vol->segment[BC(DIRECTORY)] * BLOCK_SIZE + ((uintptr_t)(((struct directory_hash_lookup_block *)0)->entries + n));
							file->entry = entry;
						}
						LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %*.*s%s"
							, next_entries + n, name_ofs, next_entries[n].first_block ^ entkey->first_block
							, *leadinDepth, *leadinDepth, leadin
							, filename+ofs );
						if( nameBlockStart ) nameBlockStart[0] = dirblock->names_first_block ^ dirblockkey->names_first_block;
						return TRUE;
					}
					if( d > 0 ) {
						minName = curName + 1;
					} else {
						usedNames = curName - 1;
					}
					curName = (minName + usedNames) >> 1;
				}
				else 
					minName++;
			}
			return filename ? FALSE : (2); // done.;
		}
		next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, FALSE, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) { DebugBreak(); return FALSE; }  // should have a last-entry before no more blocks....
#endif
		this_dir_block = next_dir_block;
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI _os_SaveFileName( struct volume *vol, BLOCKINDEX firstNameBlock, const char * filename, size_t namelen ) {
	size_t n;
	int blocks = 0;
	BLOCKINDEX this_name_block = firstNameBlock;
#ifdef _DEBUG
	if( !firstNameBlock ) DebugBreak();
#endif
	while( 1 ) {
		enum block_cache_entries cache = BC(NAMES);
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block, cache );
		unsigned char *name = (unsigned char*)names;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) ) {
			int c = name[0];
			if( vol->key ) c = c ^ vol->usekey[cache][(uintptr_t)name-(uintptr_t)names];
			if( (unsigned char)c == UTF8_EOTB ) {
				if( namelen < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) ) {
					//LoG( "using unused entry for new file...%" _size_f " %d(%d)  %" _size_f " %s", this_name_block, cache, cache - BC(NAMES), (uintptr_t)name - (uintptr_t)names, filename );
					if( vol->key ) {						
						for( n = 0; n < namelen; n++ )
							name[n] = filename[n] ^ vol->usekey[cache][n + (name-(unsigned char*)names)];
					} else
						memcpy( name, filename, namelen );

					name[namelen+0] = UTF8_EOT ^ vol->usekey[cache][(uintptr_t)name - (uintptr_t)names + namelen+0];
					name[namelen+1] = UTF8_EOTB ^ vol->usekey[cache][(uintptr_t)name - (uintptr_t)names + namelen+1];
					SETFLAG( vol->dirty, cache );
					//lprintf( "OFFSET:%d %d", ((uintptr_t)name) - ((uintptr_t)names), +blocks * BLOCK_SIZE );
					return ((uintptr_t)name) - ((uintptr_t)names) + blocks * BLOCK_SIZE;
				}
			}
			else
				if( _os_MaskStrCmp( vol, filename, firstNameBlock, ((uintptr_t)name - (uintptr_t)names)+blocks*BLOCK_SIZE, 0 ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					return ((uintptr_t)name) - ((uintptr_t)names) + blocks * BLOCK_SIZE;
				}
			while( UTF8_EOT != ( name[0] ^ vol->usekey[cache][name-(unsigned char*)names] ) ) name++;
			name++;
			//LoG( "new position is %" _size_f "  %" _size_f, this_name_block, (uintptr_t)name - (uintptr_t)names );
		}
		this_name_block = vfs_os_GetNextBlock( vol, this_name_block, GFB_INIT_NAMES, TRUE );
		blocks++;
		//LoG( "Need a new name block.... %d", this_name_block );
	}
}

static void ConvertDirectory( struct volume *vol, const char *leadin, int leadinLength, BLOCKINDEX this_dir_block, struct directory_hash_lookup_block *orig_dirblock, enum block_cache_entries *newCache ) {
	size_t n;
	int ofs = 0;
	LoG( "------------ BEGIN CONVERT DIRECTORY ----------------------" );
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
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
			enum block_cache_entries name_cache;
			BLOCKINDEX name_block = dirblock->names_first_block ^ dirblockkey->names_first_block;
			// read name block chain into a single array
			do {
				uint8_t *out = namebuffer + nameoffset;
				name_cache = BC( NAMES );
				nameblock = BTSEEK( uint8_t *, vol, name_block, name_cache );
				namekey = (uint8_t*)vol->usekey[name_cache];
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
				if( count > maxc ) {
					imax = namebuffer[name];
					maxc = count;
				}
			}

			dirblock->next_block[imax]
				= ( new_dir_block 
				  = _os_GetFreeBlock( vol, GFB_INIT_DIRENT ) ) ^ dirblockkey->next_block[imax];


			SETFLAG( vol->dirty, cache );
			{
				struct directory_hash_lookup_block *newDirblock;
				struct directory_hash_lookup_block *newDirblockkey;
				BLOCKINDEX newFirstNameBlock;
				int usedNames = dirblock->used_names ^ dirblockkey->used_names;
				int _usedNames = usedNames;
				int nf = 0;
				int firstNameOffset = -1;
				int finalNameOffset = 0;;
				cache = BC(DIRECTORY);
				newDirblock = BTSEEK( struct directory_hash_lookup_block *, vol, new_dir_block, cache );
				LoG( "new dir block cache is  %d   %d", cache, (int)new_dir_block );
				newDirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
				newFirstNameBlock = newDirblock->names_first_block ^ newDirblockkey->names_first_block;
#ifdef _DEBUG
				if( !newDirblock->names_first_block )
					DebugBreak();
#endif
				newDirblock->next_block[DIRNAME_CHAR_PARENT] = (this_dir_block << 8) | imax;

				// SETFLAG( vol->dirty, cache ); // this will be dirty because it was init above.

				for( f = 0; f < usedNames; f++ ) {
					BLOCKINDEX first = dirblock->entries[f].first_block ^ dirblockkey->entries[f].first_block;
					struct directory_entry *entry;
					struct directory_entry *entkey;
					struct directory_entry *newEntry;
					struct directory_entry *newEntkey;
					FPI name;
					FPI name_ofs;
					entry = dirblock->entries + (f);
					entkey = dirblockkey->entries + (f);
					name = ( entry->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
					if( namebuffer[name] == imax ) {
						int namelen;
						newEntry = newDirblock->entries + (nf);
						newEntkey = newDirblockkey->entries + (nf);
						//LoG( "Saving existing name %d %s", name, namebuffer + name );
						//LogBinary( namebuffer, 32 );
						namelen = 0;
						while( namebuffer[name + namelen] != UTF8_EOT )namelen++;
						name_ofs = _os_SaveFileName( vol, newFirstNameBlock, (char*)(namebuffer + name + 1), namelen -1 ) ^ newEntkey->name_offset;
						{
							INDEX idx;
							struct sack_vfs_file  * file;
							LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
								if( file->entry == entry ) {
									file->entry_fpi = 0; // new entry_fpi.

								}
							}
						}
						dirblock->used_names = ((dirblock->used_names ^ dirblockkey->used_names) - 1) ^ dirblockkey->used_names;
						newEntry->filesize = (entry->filesize ^ entkey->filesize) ^ newEntkey->filesize;
						newEntry->tick     = (entry->tick     ^ entkey->tick)     ^ newEntkey->tick;
						newEntry->name_offset = name_ofs;
						newEntry->first_block = (entry->first_block ^ entkey->first_block) ^ newEntkey->first_block;
						SETFLAG( vol->dirty, cache );
						nf++;

						newDirblock->used_names = ((newDirblock->used_names ^ newDirblockkey->used_names) + 1) ^ newDirblockkey->used_names;

						// move all others down 1.
						{
							int m;
							for( m = f; m < usedNames; m++ ) {
								if( m == (VFS_DIRECTORY_ENTRIES - 1) ) {
									dirblock->entries[m].first_block = (0)
										^ dirblockkey->entries[m].first_block;
									dirblock->entries[m].name_offset = (0)
										^ dirblockkey->entries[m].name_offset;
									dirblock->entries[m].filesize = (0)
										^ dirblockkey->entries[m].filesize;
									dirblock->entries[m].tick = (0)
										^ dirblockkey->entries[m].tick;
#ifdef _DEBUG
									if( !dirblock->names_first_block ) DebugBreak();
#endif
								}
								else {
									dirblock->entries[m].first_block = (dirblock->entries[m + 1].first_block
										^ dirblockkey->entries[m + 1].first_block)
										^ dirblockkey->entries[m].first_block;
									dirblock->entries[m].name_offset = (dirblock->entries[m + 1].name_offset
										^ dirblockkey->entries[m + 1].name_offset)
										^ dirblockkey->entries[m].name_offset;
									dirblock->entries[m].filesize = (dirblock->entries[m + 1].filesize
										^ dirblockkey->entries[m + 1].filesize)
										^ dirblockkey->entries[m].filesize;
									dirblock->entries[m].tick = (dirblock->entries[m + 1].tick
										^ dirblockkey->entries[m + 1].tick)
										^ dirblockkey->entries[m].tick;
#ifdef _DEBUG
									if( !dirblock->names_first_block ) DebugBreak();
#endif
								}
							}
							usedNames--;
							f--;
						}
					}
				}
				if( usedNames ) {
					static uint8_t newnamebuffer[18 * 4096];
					int newout = 0;
					int min_name = BLOCK_SIZE + 1;
					int _min_name = -1; // min found has to be after this one.
					//lprintf( "%d names remained.", usedNames );
					for( f = 0; f < usedNames; f++ ) {
						struct directory_entry *entry;
						struct directory_entry *entkey;
						FPI name;
						entry = dirblock->entries + (f);
						entkey = dirblockkey->entries + (f);
						name = ( entry->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
						entry->name_offset = ( newout ^ entkey->name_offset ) 
							| ( (entry->name_offset ^ entkey->name_offset) 
								& ~DIRENT_NAME_OFFSET_OFFSET );
						while( namebuffer[name] != UTF8_EOT )
							newnamebuffer[newout++] = namebuffer[name++];
						newnamebuffer[newout++] = namebuffer[name++];
					}
					newnamebuffer[newout++] = UTF8_EOTB;
					memcpy( namebuffer, newnamebuffer, newout );
					memset( newnamebuffer, 0, newout );
				}
				else {
					namebuffer[0] = UTF8_EOTB;
				}
				{
					name_block = dirblock->names_first_block ^ dirblockkey->names_first_block;
					nameoffset = 0;
					do {
						uint8_t *out;
						nameblock = namebuffer + nameoffset;
						name_cache = BC( NAMES );
						out = BTSEEK( uint8_t *, vol, name_block, name_cache );
						namekey = (uint8_t*)vol->usekey[name_cache];
						if( vol->key )
							for( n = 0; n < 4096; n++ )
								(*out++) = (*nameblock++) ^ (*namekey++);
						else
							for( n = 0; n < 4096; n++ )
								(*out++) = (*nameblock++);
						SETFLAG( vol->dirty, cache );
						name_block = vfs_os_GetNextBlock( vol, name_block, 0, 0 );
						nameoffset += 4096;
					} while( name_block != EOFBLOCK );
				}
			}
			break;  // a set of names has been moved out of this block.
			// has block.
		}
	} while( 0 );

	// unlink here
	// unlink dirblock->names_first_block
}

static struct directory_entry * _os_GetNewDirectory( struct volume *vol, const char * filename
		, struct sack_vfs_file *file ) {
	size_t n;
	const char *_filename = filename;
	static char leadin[256];
	static int leadinDepth = 0;
	BLOCKINDEX this_dir_block = 0;
	struct directory_entry *next_entries;
	LOGICAL moveMark = FALSE;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	leadinDepth = 0;
	do {
		enum block_cache_entries cache;
		FPI dirblockFPI;
		int usedNames;
		struct directory_hash_lookup_block *dirblock;
		struct directory_hash_lookup_block *dirblockkey;
		BLOCKINDEX firstNameBlock;
		cache = BC( DIRECTORY );
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
#ifdef _DEBUG
		if( !dirblock->names_first_block ) DebugBreak();
#endif
		dirblockFPI = sack_ftell( vol->file );
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
		//lprintf( " --------------- THIS DIR BLOCK ---------------" );
		//_os_dumpDirectories( vol, this_dir_block, 1 );
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
			entkey = dirblockkey->entries;
			ent = dirblock->entries;
			for( n = 0; USS_LT( n, size_t, usedNames, int ); n++ ) {
				ent = dirblock->entries + (n);
				entkey = dirblockkey->entries + (n);
				name_ofs = ( ent->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
				first_blk = ent->first_block ^ entkey->first_block;
				// not name_offset (end of list) or not first_block(free entry) use this entry
				//if( name_ofs && (first_blk > 1) )  continue;

				if( _os_MaskStrCmp( vol, filename, firstNameBlock, name_ofs, 0 ) < 0 ) {
					int m;
					LoG( "Insert new directory" );
					for( m = dirblock->used_names; SUS_GT( m, int, n, size_t ); m-- ) {
						dirblock->entries[m].filesize    = dirblock->entries[m - 1].filesize    ^ dirblockkey->entries[m - 1].filesize;
						dirblock->entries[m].first_block = dirblock->entries[m - 1].first_block ^ dirblockkey->entries[m - 1].first_block;
						dirblock->entries[m].name_offset = dirblock->entries[m - 1].name_offset ^ dirblockkey->entries[m - 1].name_offset;
						dirblock->entries[m].tick        = dirblock->entries[m - 1].tick        ^ dirblockkey->entries[m - 1].tick;
					}
					dirblock->used_names++;
					break;
				}
			}
			ent = dirblock->entries + (n);
			if( n == usedNames ) {
				dirblock->used_names = (uint8_t)((n + 1) ^ dirblockkey->used_names);
			}
			//LoG( "Get New Directory save naem:%s", filename );
			name_ofs = _os_SaveFileName( vol, firstNameBlock, filename, StrLen( filename ) ) ^ entkey->name_offset;
			// have to allocate a block for the file, otherwise it would be deleted.
			first_blk = _os_GetFreeBlock( vol, FALSE ) ^ entkey->first_block;
		        
			ent->filesize = entkey->filesize;
			ent->name_offset = name_ofs;
			ent->first_block = first_blk;
			ent->tick = GetTimeOfDay() ^ entkey->tick;
			if( file ) {
				SETFLAG( vol->seglock, cache );
				file->entry_fpi = dirblockFPI + ((uintptr_t)(((struct directory_hash_lookup_block *)0)->entries + n));
				file->entry = ent;
				file->dirent_key = entkey[n];
				file->cache = cache;
			}
			SETFLAG( vol->dirty, cache );
			return ent;
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
	if( !_os_ScanDirectory( vol, filename, 0, NULL, file, 0 ) ) {
		if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; Deallocate( struct sack_vfs_file *, file ); return NULL; }
		else _os_GetNewDirectory( vol, filename, file );
	}
	{
		BLOCKINDEX offset = (file->entry->name_offset ^ file->dirent_key.name_offset);
		uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
		if( sealLen ) {
			file->seal = NewArray( uint8_t, sealLen );
			file->sealantLen = sealLen;
			file->sealed = SACK_VFS_OS_SEAL_LOAD;
		}
		else {
			file->seal = NULL;
			file->sealantLen = 0;
			file->sealed = SACK_VFS_OS_SEAL_NONE;
		}
		file->filename = StrDup( filename );
	}
	file->readKey = NULL;
	file->readKeyLen = 0;
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



static char * getFilename( const char *objBuf, size_t objBufLen, char *sealBuf, size_t sealBufLen, LOGICAL owner, char *idBuf, size_t idBufLen ) {

	if( sealBuf ) {
		struct random_context *signEntropy = (struct random_context *)DequeLink( &l.plqCrypters );
		char *fileKey;
		size_t keyLen;
		uint8_t outbuf[32];

		if( !signEntropy )
			signEntropy = SRG_CreateEntropy3( NULL, (uintptr_t)0 );


		if( owner ) {
			char *metakey = SRG_ID_Generator3();
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)metakey, 44 );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)sealBuf, sealBufLen );
			SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

		}
		else {
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)sealBuf, sealBufLen );
			SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
		}

		SRG_ResetEntropy( signEntropy );
		SRG_FeedEntropy( signEntropy, (const uint8_t*)objBuf, objBufLen );
		SRG_FeedEntropy( signEntropy, (const uint8_t*)outbuf, 32 );
		fileKey = EncodeBase64Ex( outbuf, 32, &keyLen, (const char*)1 );

		SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

		SRG_DestroyEntropy( &signEntropy );
		{
			size_t len;
			char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
			//rid[43] = '=';
			StrCpyEx( idBuf, rid, idBufLen );
			Deallocate( char*, rid );
		}
		EnqueLink( &l.plqCrypters, signEntropy );
		return fileKey;
	}
	else {
		char *objid = SRG_ID_Generator3();
		objid[43] = 0;
		StrCpyEx( idBuf, objid, idBufLen );
		Deallocate( char*, objid );
		if( idBuf )
			idBuf[0] = 0;
		return NULL;
	}
}


int CPROC sack_vfs_os_exists( struct volume *vol, const char * file ) {
	LOGICAL result;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	result = _os_ScanDirectory( vol, file, 0, NULL, NULL, 0 );
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

#define IS_OWNED(file)  ( (file->entry->name_offset^file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_OWNED )

size_t CPROC sack_vfs_os_write( struct sack_vfs_file *file, const char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	LOGICAL updated = FALSE;
	uint8_t *cdata;
	size_t cdataLen;
	if( file->readKey ) {
		SRG_XSWS_encryptData( (uint8_t*)data, length, file->entry->tick^ file->dirent_key.tick
			, file->readKey, file->readKeyLen
			, &cdata, &cdataLen );
		data = (const char *)cdata;
		length = cdataLen;
	}
	else {
		cdata = NULL;
	}
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( (file->entry->name_offset^file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_SEALANT ) {
		char filename[64];
		// read-only data block.
		lprintf( "INCOMPLETE - TODO WRITE PATCH" );
		char *sealer = getFilename( data, length, (char*)file->sealant, file->sealantLen, IS_OWNED(file), filename, 64 );

		struct sack_vfs_file *pFile = sack_vfs_os_openfile( file->vol, filename );
		pFile->sealant = (uint8_t*)sealer;
		pFile->sealantLen = 32;
		return sack_vfs_os_write( pFile, data, length );
	}
#ifdef DEBUG_FILE_OPS
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
#endif
	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			SETFLAG( file->vol->dirty, cache );
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
			SETFLAG( file->vol->dirty, cache );
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
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
#ifdef _DEBUG
		if( file->block < 2 ) DebugBreak();
#endif
		if( length >= BLOCK_SIZE ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, BLOCK_SIZE );
			SETFLAG( file->vol->dirty, cache );
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
			SETFLAG( file->vol->dirty, cache );
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
	if( file->sealant && (void*)file->sealant != (void*)data ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		sack_vfs_os_write( file, (char*)file->sealant, file->sealantLen );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
	}
	if( updated ) {
		SETFLAG( file->vol->dirty, file->cache ); // directory cache block (locked)
	}
	file->vol->lock = 0;
	return written;
}

static enum sack_vfs_os_seal_states ValidateSeal( struct sack_vfs_file *file, char *data, size_t length ) {
	BLOCKINDEX offset = (file->entry->name_offset ^ file->dirent_key.name_offset);
	uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
	struct random_context *signEntropy;// = (struct random_context *)DequeLink( &signingEntropies );
	uint8_t outbuf[32];

	signEntropy = SRG_CreateEntropy3( NULL, (uintptr_t)0 );

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->sealantLen );
	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
	if( (file->sealantLen != 32) || MemCmp( outbuf, file->sealant, 32 ) )
		return SACK_VFS_OS_SEAL_INVALID;

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)data, length );

	// DO NOT DOUBLE_PROCESS THIS DATA
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->sealantLen );

	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

	SRG_DestroyEntropy( &signEntropy );
	{
		enum sack_vfs_os_seal_states success = SACK_VFS_OS_SEAL_INVALID;
		size_t len;
		char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
		if( StrCmp( file->filename, rid ) == 0 )
			success = SACK_VFS_OS_SEAL_VALID;
		Deallocate( char *, rid );
		return success;
	}
}

size_t CPROC sack_vfs_os_read( struct sack_vfs_file *file, char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	if( (file->entry->name_offset ^ file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_READ_KEYED ) {
		if( !file->readKey ) return 0;
	}
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) ) {
		if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < file->fpi )
			length = 0;
		else
			length = (size_t)(( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi);
	}
	if( !length ) {  file->vol->lock = 0; return 0; }

	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
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
		enum block_cache_entries cache = BC(FILE);
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

	if( file->readKey
	   && ( file->fpi == ( file->entry->filesize ^ file->dirent_key.filesize ) )
	   && ( (file->entry->name_offset ^ file->dirent_key.name_offset) 
	      & DIRENT_NAME_OFFSET_FLAG_READ_KEYED) )
	{
		uint8_t *outbuf;
		size_t outlen;
		SRG_XSWS_decryptData( (uint8_t*)data, written, (file->entry->tick ^ file->dirent_key.tick )
		                    , file->readKey, file->readKeyLen
		                    , &outbuf, &outlen );
		memcpy( data, outbuf, outlen );
		Release( outbuf );
		written = outlen;
	}

	if( file->sealant
		&& (void*)file->sealant != (void*)data
		&& length == ( file->entry->filesize ^ file->dirent_key.filesize ) ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		file->entry->filesize = ((file->entry->filesize
			^ file->dirent_key.filesize) + file->sealantLen + sizeof( BLOCKINDEX ))
			^ file->dirent_key.filesize;
		sack_vfs_os_read( file, (char*)file->sealant, file->sealantLen );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = ValidateSeal( file, data, length );
	}
	file->vol->lock = 0;
	return written;
}

static BLOCKINDEX sack_vfs_os_read_patches( struct sack_vfs_file *file ) {
	size_t written = 0;
	BLOCKINDEX saveFpi = file->fpi;
	size_t length;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	length = (size_t)(file->entry->filesize  ^ file->dirent_key.filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek( file, length, SEEK_SET );

	if( file->sealant ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX patches;
		file->entry->filesize = ((file->entry->filesize
			^ file->dirent_key.filesize) + file->sealantLen + sizeof( BLOCKINDEX ))
			^ file->dirent_key.filesize;
		sack_vfs_os_read( file, (char*)file->sealant, file->sealantLen );
		sack_vfs_os_read( file, (char*)&patches, sizeof( BLOCKINDEX ) );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = SACK_VFS_OS_SEAL_LOAD;
		return patches;
	}
	file->vol->lock = 0;
	return written;
}

static size_t sack_vfs_os_set_patch_block( struct sack_vfs_file *file, BLOCKINDEX patchBlock ) {
	size_t written = 0;
	size_t length;
	BLOCKINDEX saveFpi = file->fpi;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	length = (size_t)(file->entry->filesize  ^ file->dirent_key.filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek( file, length, SEEK_SET );

	if( file->sealant ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		file->entry->filesize = ((file->entry->filesize
			^ file->dirent_key.filesize) + file->sealantLen + sizeof( BLOCKINDEX ))
			^ file->dirent_key.filesize;
		sack_vfs_os_seek( file, file->sealantLen, SEEK_CUR );
		sack_vfs_os_write( file, (char*)&patchBlock, sizeof( BLOCKINDEX ) );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
	}
	file->vol->lock = 0;
	return written;
}

static void sack_vfs_os_unlink_file_entry( struct volume *vol, struct sack_vfs_file *dirinfo, BLOCKINDEX first_block, LOGICAL deleted ) {
	//FPI entFPI, struct directory_entry *entry, struct directory_entry *entkey
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
		dirinfo->entry->first_block = dirinfo->dirent_key.first_block;
		SETFLAG( vol->dirty, dirinfo->cache );
	}

	if( !file_found ) {
		_block = block = first_block;// entry->first_block ^ entkey->first_block;
		LoG( "(marking physical deleted (again?)) entry starts at %d", block );
		// wipe out file chain BAT
		do {
			enum block_cache_entries cache = BC(BAT);
			enum block_cache_entries fileCache = BC(DATAKEY);
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
			BLOCKINDEX _thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
			//BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + (block & (BLOCKS_PER_BAT - 1)) * BLOCK_SIZE;
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( vol, block, &fileCache );
			//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
			memset( blockData, 0, BLOCK_SIZE );
			// after seek, block was read, and file position updated.
			SETFLAG( vol->dirty, cache );

			block = vfs_os_GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
			AddDataItem( &vol->pdlFreeBlocks, &_block );

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
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries data_cache = BC(DATAKEY);
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
		enum block_cache_entries cache = BC(NAMES);
		static char fname[256];
		FPI name_ofs = file->entry->name_offset ^ file->dirent_key.name_offset;
		TSEEK( const char *, file->vol, name_ofs, cache ); // have to do the seek to the name block otherwise it might not be loaded.
		MaskStrCpy( fname, sizeof( fname ), file->vol, name_ofs );
#ifdef DEBUG_FILE_OPS
		LoG( "close file:%s(%p)", fname, file );
#endif
	}
#endif
	DeleteLink( &file->vol->files, file );
	if( file->delete_on_close ) sack_vfs_os_unlink_file_entry( file->vol, file, file->_first_block, TRUE );
	{
		INDEX idx;
		struct sack_vfs_file * testFile;
		LIST_FORALL( file->vol->files, idx, struct sack_vfs_file *, testFile ) {
			if( testFile->cache == file->cache )
				break;
		}
		if( !testFile )
			RESETFLAG( file->vol->seglock, file->cache );
	}
	Deallocate( char *, file->filename );
	if( file->sealant )
		Deallocate( uint8_t*, file->sealant );
	file->vol->lock = 0;
	if( file->vol->closed ) sack_vfs_os_unload_volume( file->vol );
	Deallocate( struct sack_vfs_file *, file );
	return 0;
}

int CPROC sack_vfs_os_unlink_file( struct volume *vol, const char * filename ) {
	int result = 0;
	struct sack_vfs_file tmp_dirinfo;
	if( !vol ) return 0;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	LoG( "unlink file:%s", filename );
	if( _os_ScanDirectory( vol, filename, 0, NULL, &tmp_dirinfo, 0 ) ) {
		sack_vfs_os_unlink_file_entry( vol, &tmp_dirinfo, tmp_dirinfo.entry->first_block ^ tmp_dirinfo.dirent_key.first_block, FALSE );
		result = 1;
	}
	vol->lock = 0;
	return result;
}

int CPROC sack_vfs_os_flush( struct sack_vfs_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_os_need_copy_write( void ) {	return FALSE; }

struct hashnode {
	char leadin[256];
	int leadinDepth;
	BLOCKINDEX this_dir_block;
	size_t thisent;
};

struct _os_find_info {
	char filename[BLOCK_SIZE];
	struct volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	size_t filesize;
	CTEXTSTR mask;
#ifdef VIRTUAL_OBJECT_STORE
	char leadin[256];
	int leadinDepth;
	PDATASTACK pds_directories;
#else 
	BLOCKINDEX this_dir_block;
	size_t thisent;
#endif
};

struct find_info * CPROC sack_vfs_os_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask )
{
	struct _os_find_info *info = New( struct _os_find_info );
	info->pds_directories = CreateDataStack( sizeof( struct hashnode ) );
	info->base = base;
	info->base_len = StrLen( base );
	info->mask = mask;
	info->vol = (struct volume *)psvInst;
	info->leadinDepth = 0;
	return (struct find_info*)info;
}


static int _os_iterate_find( struct find_info *_info ) {
	struct _os_find_info *info = (struct _os_find_info *)_info;
	struct directory_hash_lookup_block *dirBlock;
	struct directory_hash_lookup_block *dirBlockKey;
	struct directory_entry *next_entries;
	int n;
	do 
	{
		enum block_cache_entries cache = BC(DIRECTORY);
		enum block_cache_entries name_cache = BC(NAMES);
		struct hashnode node = ((struct hashnode *)PopData( &info->pds_directories ))[0];

		dirBlock = BTSEEK( struct directory_hash_lookup_block *, info->vol, node.this_dir_block, cache );
		dirBlockKey = (struct directory_hash_lookup_block *)info->vol->usekey[cache];

		if( !node.thisent ) {
			struct hashnode subnode;
			subnode.thisent = 0;
			for( n = 255; n >= 0; n-- ) {
				BLOCKINDEX block = dirBlock->next_block[n] ^ dirBlockKey->next_block[n];
				if( block ) {
					memcpy( subnode.leadin, node.leadin, node.leadinDepth );
					subnode.leadin[node.leadinDepth] = (char)n;
					subnode.leadinDepth = node.leadinDepth + 1;
					subnode.leadin[subnode.leadinDepth] = 0;
					subnode.this_dir_block = block;
					PushData( &info->pds_directories, &subnode );
				}
			}
		}
		//lprintf( "%p ledin : %*.*s %d", node, node.leadinDepth, node.leadinDepth, node.leadin, node.leadinDepth );
		next_entries = dirBlock->entries;
		for( n = (int)node.thisent; n < (dirBlock->used_names ^ dirBlockKey->used_names); n++ ) {
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_hash_lookup_block *)info->vol->usekey[cache])->entries+n:&l.zero_entkey;
			FPI name_ofs = ( next_entries[n].name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
			const char *filename;
			int l;

			// if file is deleted; don't check it's name.
			info->filesize = (size_t)(next_entries[n].filesize ^ entkey->filesize);
			if( (name_ofs) > info->vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( info->vol, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
			info->filenamelen = 0;
			for( l = 0; l < node.leadinDepth; l++ ) info->filename[info->filenamelen++] = node.leadin[l];

			if( info->vol->key ) {
				int c;
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
				StrCpy( info->filename + info->filenamelen, filename );
				LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			}
			node.thisent = n + 1;
			PushData( &info->pds_directories, &node );
			return 1;
		}
	}
	while( info->pds_directories->Top );
	return 0;
}

int CPROC sack_vfs_os_find_first( struct find_info *_info ) {
	struct _os_find_info *info = (struct _os_find_info *)_info;
	struct hashnode root;
	root.this_dir_block = 0;
	root.leadinDepth = 0;
	root.thisent = 0;
	PushData( &info->pds_directories, &root );
	//info->thisent = 0;
	return _os_iterate_find( _info );
}

int CPROC sack_vfs_os_find_close( struct find_info *_info ) {
	struct _os_find_info *info = (struct _os_find_info *)_info;
	Deallocate( struct _os_find_info*, info ); return 0; }
int CPROC sack_vfs_os_find_next( struct find_info *_info ) { return _os_iterate_find( _info ); }
char * CPROC sack_vfs_os_find_get_name( struct find_info *_info ) {
	struct _os_find_info *info = (struct _os_find_info *)_info;
	return info->filename; }
size_t CPROC sack_vfs_os_find_get_size( struct find_info *_info ) {
	struct _os_find_info *info = (struct _os_find_info *)_info;
	return info->filesize; }
LOGICAL CPROC sack_vfs_os_find_is_directory( struct find_cursor *cursor ) { return FALSE; }
LOGICAL CPROC sack_vfs_os_is_directory( uintptr_t psvInstance, const char *path ) {
	if( path[0] == '.' && path[1] == 0 ) return TRUE;
	{
		struct volume *vol = (struct volume *)psvInstance;
		if( _os_ScanDirectory( vol, path, 0, NULL, NULL, 1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}

LOGICAL CPROC sack_vfs_os_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct volume *vol = (struct volume *)psvInstance;
	lprintf( "RENAME IS NOT SUPPORTED IN OBJECT STORAGE(OR NEEDS TO BE FIXED)" );
	// fail if the names are the same.
	return TRUE;
}


uintptr_t CPROC sack_vfs_file_ioctl( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;
		break;
	case SOSFSFIO_TAMPERED:
	{
		struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		int *result = va_arg( args, int* );

		if( file->sealant ) {
			switch( file->sealed ) {
			case SACK_VFS_OS_SEAL_STORE:
			case SACK_VFS_OS_SEAL_VALID:
				(*result) = 1;
			default:
				(*result) = 0;
			}
		}
		else
			(*result) = 1;
	}
	break;
	case SOSFSFIO_PROVIDE_SEALANT:
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->sealant )
				Release( file->sealant );
			file->sealant = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			file->sealantLen = (uint8_t)len;
			if( file->sealed == SACK_VFS_OS_SEAL_NONE )
				file->sealed = SACK_VFS_OS_SEAL_STORE;
			else if( file->sealed == SACK_VFS_OS_SEAL_VALID || file->sealed == SACK_VFS_OS_SEAL_LOAD )
				file->sealed = SACK_VFS_OS_SEAL_STORE_PATCH;
			else
				lprintf( "Unhandled SEAL state." );
			//file->sealant = sealant;
			//file->sealantLen = sealantLen;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset ^ file->dirent_key.name_offset)
				| ((len >> 2) << 17)) ^ file->dirent_key.name_offset);
		}
	}
	break;
	case SOSFSFIO_PROVIDE_READKEY:
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->readKey )
				Release( file->readKey );
			file->readKey = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			file->readKeyLen = (uint16_t)len;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset ^ file->dirent_key.name_offset)
				| DIRENT_NAME_OFFSET_FLAG_READ_KEYED) ^ file->dirent_key.name_offset);
		}
	}
	break;
	}
	return TRUE;
}


uintptr_t CPROC sack_vfs_system_ioctl( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	struct volume *vol = (struct volume *)psvInstance;
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;

	case SOSFSSIO_LOAD_OBJECT:
		return FALSE;

	case SOSFSSIO_PATCH_OBJECT:
		{
		LOGICAL owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key

		char *objIdBuf = va_arg( args, char * );
		size_t objIdBufLen = va_arg( args, size_t );

		char *patchAuth = va_arg( args, char * );
		size_t patchAuthLen = va_arg( args, size_t );

		char *objBuf = va_arg( args, char * );
		size_t objBufLen = va_arg( args, size_t );

		char *sealBuf = va_arg( args, char * );
		size_t sealBufLen = va_arg( args, size_t );

		char *keyBuf = va_arg( args, char * );
		size_t keyBufLen = va_arg( args, size_t );

		char *idBuf = va_arg( args, char * );
		size_t idBufLen = va_arg( args, size_t );

		if( sack_vfs_os_exists( vol, objIdBuf ) ) {
			struct sack_vfs_file* file = sack_vfs_os_openfile( vol, objIdBuf );
			BLOCKINDEX patchBlock = sack_vfs_os_read_patches( file );
			if( !patchBlock ) {
				patchBlock = _os_GetFreeBlock( vol, GFB_INIT_PATCHBLOCK );
			}
			{

				enum block_cache_entries cache;
				struct directory_patch_block *newPatchblock;
				struct directory_patch_block *newPatchblockkey;

				cache = BC(FILE);
				newPatchblock = BTSEEK( struct directory_patch_block *, vol, patchBlock, cache );
				newPatchblockkey = (struct directory_patch_block *)vol->usekey[cache];

				while( 1 ) {
					//char objId[45];
					//size_t objIdLen;
					char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, FALSE, idBuf, idBufLen );

					if( sack_vfs_os_exists( vol, idBuf ) ) {
						if( !sealBuf ) { // accidental key collision. 
							continue; // try again.
						}
						else {
							// deliberate key collision; and record already exists.
							return TRUE;
						}
					}
					else {
						struct sack_vfs_file* file = sack_vfs_os_openfile( vol, idBuf );

						//  file->entry_fpi
						newPatchblock->entries[newPatchblock->usedEntries].raw
							= file->entry_fpi ^ newPatchblockkey->entries[newPatchblock->usedEntries].raw;
						newPatchblock->usedEntries = (newPatchblock->usedEntries + 1) ^ newPatchblockkey->usedEntries;
						SETFLAG( vol->dirty, cache );

						file->sealant = (uint8_t*)seal;
						file->sealantLen = (uint8_t)strlen( seal );
						sack_vfs_os_write( file, objBuf, objBufLen );
						sack_vfs_os_close( file );
					}
					return TRUE;
				}
			}
		}
		return FALSE; // object to patch was not found.
	}
	break;
	case SOSFSSIO_STORE_OBJECT:
	{
		LOGICAL owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key
		char *objBuf = va_arg( args, char * );
		size_t objBufLen = va_arg( args, size_t );
		char *sealBuf = va_arg( args, char * );
		size_t sealBufLen = va_arg( args, size_t );
		char *keyBuf = va_arg( args, char * );
		size_t keyBufLen = va_arg( args, size_t );
		char *idBuf = va_arg( args, char * );
		size_t idBufLen = va_arg( args, size_t );
		while( 1 ) {
			char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, owner, idBuf, idBufLen );
			if( sack_vfs_os_exists( vol, idBuf ) ) {
				if( !sealBuf ) { // accidental key collision. 
					continue; // try again.
				}
				else {
					// deliberate key collision; and record already exists.
					return TRUE;
				}
			}
			else {
				struct sack_vfs_file* file = sack_vfs_os_openfile( vol, idBuf );
				file->sealant = (uint8_t*)seal;
				file->sealantLen = (uint8_t)strlen( seal );
				sack_vfs_os_write( file, objBuf, objBufLen );
				sack_vfs_os_close( file );
			}
			return TRUE;
		}

	}
	break;
	}
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
                                                   , sack_vfs_file_ioctl
												   , sack_vfs_system_ioctl
};

PRIORITY_PRELOAD( Sack_VFS_OS_Register, CONFIG_SCRIPT_PRELOAD_PRIORITY - 2 )
{
#undef DEFAULT_VFS_NAME
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

#ifdef __cplusplus
}
#endif

#ifdef USE_STDIO
#  undef sack_fopen
#  undef sack_fseek
#  undef sack_fclose
#  undef sack_fread
#  undef sack_fwrite
#  undef sack_ftell
#  undef free
#  undef StrDup
#  define StrDup(o) StrDupEx( (o) DBG_SRC )
#endif

#undef free

SACK_VFS_NAMESPACE_END

#undef l
#endif