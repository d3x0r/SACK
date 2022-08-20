#if !defined( SACK_AMALGAMATE ) || defined( __cplusplus )

/*

	FILE Data has extra fields stored with the data.

	   File Data - Directory entry filesize
	   references - a reference to a blockchain that contains the references to this object.
	        In the reference data block is FPI which is the directory entry ( converted directories? )

	   Sealant - length stored in NAME_OFFSET field of directory entry
	   patches - a sealed object has the ability to be modified with other signed and sealed patches.
			 A reference to the patch FileData is stored for each patch object.
			 (The patch object has a unique object identifier?  Or does it only exist for this object?)


*/



/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block, FFFFFFFE end of BAT

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#define SACK_VFS_OS_SOURCE
#define SKIP_LIGHT_ENCRYPTION(n)
#define VFS_OS_PARANOID_TRUNCATE

// this is a badly named debug symbol;
// it is the LAST debugging of delete logging/checking...
//#define DEBUG_DELETE_LAST


//#define USE_STDIO
#if 1
#  include <stdhdrs.h>
#  include <deadstart.h>
#  include <ctype.h> // tolower on linux
#ifndef USE_STDIO
#  include <filesys.h>
#endif
#  include <sack_vfs.h>
#  include <procreg.h>
#  include <salty_generator.h>
#  include <sqlgetoption.h>
#  include <jsox_parser.h>
#else
#  include <sack.h>
#  include <ctype.h> // tolower on linux
//#include <filesys.h>
//#include <procreg.h>
//#include <salty_generator.h>
//#include <sack_vfs.h>
//#include <sqlgetoption.h>
#endif



#ifdef _MSC_VER
// integer partial expresions summed into 64 bit.
#  pragma warning( disable: 26451 )
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
		// filesyslib/pathops.c
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

// have to enable TRACE_LOG for most of the symbols to actually log
//#define DEBUG_TRACE_LOG
//#define DEBUG_ROLLBACK_JOURNAL
//#define DEBUG_FILE_OPS
// this is large binary blocks.
//#define DEBUG_DISK_IO
//#define DEBUG_DISK_DATA
//#define DEBUG_DIRECTORIES
//#define DEBUG_BLOCK_INIT
//#define DEBUG_TIMELINE_AVL
//#define DEBUG_TIMELINE_DIR_TRACKING
//#define DEBUG_FILE_SCAN
//#define DEBUG_FILE_OPEN

//#define DEBUG_VALIDATE_TREE
//#define DEBUG_BLOCK_COMPUTE
//#define DEBUG_SECTOR_DIRT
//#define DEBUG_CACHE_FAULTS
//#define DEBUG_CACHE_FLUSH
//#define DEBUG_SET_SECTOR_SIZE

//#define DEBUG_FILE_TRUNCATE

#define FILE_BASED_VFS
#define VIRTUAL_OBJECT_STORE
#include "vfs_internal.h"
#define vfs_SEEK vfs_os_SEEK
#define vfs_BSEEK vfs_os_BSEEK

#define MAX_FILENAME_LEN 256

struct memoryTimelineNode;

#ifdef __cplusplus
namespace objStore {
#endif
//#define PARANOID_INIT


#undef LoG
#undef LoG_
#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#define LoG_( a,... ) _lprintf(DBG_RELAY)( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#define LoG_( a,... )
#endif
//#if !defined __GNUC__ and defined( __CPLUSPLUS )
#define sane_offsetof(type,member) ((size_t)&(((type*)0)->member))
//#endif


#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)
#define EODMARK   (1)
#define DIR_ALLOCATING_MARK (~0)
//#define DIR_DELETED_MARK    (1)
//#define DIR_CREATED_MARK    (2)

#undef GFB_INIT_NONE
#undef GFB_INIT_DIRENT
#undef GFB_INIT_NAMES

enum getFreeBlockInit {
	GFB_INIT_NONE       ,
	GFB_INIT_DIRENT     ,
	GFB_INIT_NAMES      ,
	GFB_INIT_PATCHBLOCK ,
	GFB_INIT_TIMELINE   ,
	GFB_INIT_TIMELINE_MORE,
	GFB_INIT_ROLLBACK   ,
};
// End Of Text Block
#define UTF8_EOTB 0xFF
// End Of Text
#define UTF8_EOT 0xFE

#define FIRST_DIR_BLOCK      0
//#define FIRST_NAMES_BLOCK    1
#define FIRST_TIMELINE_BLOCK 2
#define FIRST_ROLLBACK_BLOCK 3


// use this byte in hash as parent directory (block & char)
// utf8 names never use 0xFF as a codeunit.
#define DIRNAME_CHAR_PARENT 0xFF

struct dirent_cache {
	BLOCKINDEX entry_fpi;
	struct directory_entry entry;  // has file size within
	struct directory_entry entry_key;  // has file size within

	struct dirent_cache *patches;
	int usedPatches;
	int availPatches;
} dirCache;


struct hashnode {
	char leadin[MAX_FILENAME_LEN];
	int leadinDepth;
	BLOCKINDEX this_dir_block;
	size_t thisent;
};

struct sack_vfs_os_find_info {
	char filename[FILE_NAME_MAXLEN];
	struct sack_vfs_os_volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	size_t filesize;
	CTEXTSTR mask;
#ifdef VIRTUAL_OBJECT_STORE
	char leadin[MAX_FILENAME_LEN];
	int leadinDepth;
	PDATASTACK pds_directories;
	uint64_t ctime;
	uint64_t wtime;
	struct memoryTimelineNode *time;
#else
	BLOCKINDEX this_dir_block;
	size_t thisent;
#endif
};


static void sack_vfs_os_flush_block( struct sack_vfs_os_volume* vol, enum block_cache_entries entry );
static void vfs_os_smudge_cache( struct sack_vfs_os_volume* vol, enum block_cache_entries n );
static BLOCKINDEX _os_GetFreeBlock_( struct sack_vfs_os_volume *vol, enum block_cache_entries* cache, enum getFreeBlockInit init, int blocksize, LOGICAL flush_BAT_caches  DBG_PASS );

#define _os_GetFreeBlock(v,c,i,s) _os_GetFreeBlock_(v,c,i,s,FALSE DBG_SRC )

#define IS_OWNED(file)  ( (file->entry->name_offset) & DIRENT_NAME_OFFSET_FLAG_OWNED )

LOGICAL _os_ScanDirectory_( struct sack_vfs_os_volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_os_file *file
	, int path_match
	, char *leadin
	, int *leadinDepth
);
#define _os_ScanDirectory(v,f,db,nb,file,pm) ((l.leadinDepth = 0), _os_ScanDirectory_(v,f,db,nb,file,pm, l.leadin, &l.leadinDepth ))

// This getNextBlock is optional allocate new one; it uses _os_getFreeBlock_
static BLOCKINDEX vfs_os_GetNextBlock_v2( struct sack_vfs_os_volume* vol, BLOCKINDEX block, enum block_cache_entries* blockCache, enum getFreeBlockInit init, LOGICAL expand, int blockSize, int* realBlockSize, LOGICAL flush_BAT_caches );
static BLOCKINDEX vfs_os_GetNextBlock( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum block_cache_entries *cache, enum getFreeBlockInit init, LOGICAL expand, int blockSize, int *realBlockSize );

static LOGICAL _os_ExpandVolume( struct sack_vfs_os_volume *vol, BLOCKINDEX fromBlock, int size );
//static void reloadTimeEntry( struct memoryTimelineNode *time, struct sack_vfs_os_volume *vol, uint64_t timeEntry DBG_PASS );

#define vfs_os_BSEEK(v,b,s,c) vfs_os_BSEEK_(v,b,s,c DBG_SRC )
uintptr_t vfs_os_BSEEK_( struct sack_vfs_os_volume *vol, BLOCKINDEX block, int blockSize, enum block_cache_entries *cache_index DBG_PASS );
uint8_t* vfs_os_DSEEK_( struct sack_vfs_os_volume* vol, FPI dataFPI, int blockSize, enum block_cache_entries* cache_index DBG_PASS );
#define vfs_os_DSEEK(v,b,s,c) vfs_os_DSEEK_(v,b,s,c DBG_SRC )

uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume* vol
	, struct sack_vfs_os_file* file
	, BLOCKINDEX firstblock
	, FPI offset
	, enum block_cache_entries* cache_index
	, int blockSize
	DBG_PASS
);

static size_t CPROC sack_vfs_os_seek_internal( struct sack_vfs_os_file* file, size_t pos, int whence );
static size_t CPROC sack_vfs_os_write_internal( struct sack_vfs_os_file* file, const void* data_, size_t length
	, POINTER writeState );
static size_t CPROC sack_vfs_os_read_internal( struct sack_vfs_os_file* file, uint64_t version, void* data_, size_t length );



#  ifdef _MSC_VER
#    pragma pack (push, 1)
#  endif

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
		FPI raw;
	}entries[(DIR_BLOCK_SIZE-sizeof(BLOCKINDEX))/sizeof(uint32_t)];
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
	} entries[(DIR_BLOCK_SIZE)/sizeof( struct directory_patch_ref_entry )] PACKED;
} PACKED;

#  ifdef _MSC_VER
#    pragma pack (pop)
#  endif

enum sack_vfs_os_seal_states {
	SACK_VFS_OS_SEAL_NONE = 0,
	SACK_VFS_OS_SEAL_LOAD,
	SACK_VFS_OS_SEAL_VALID,
	SACK_VFS_OS_SEAL_STORE,
	SACK_VFS_OS_SEAL_INVALID,  // validate failed (read whole file check)
	SACK_VFS_OS_SEAL_CLEARED,  // stored patch is writeable
	SACK_VFS_OS_SEAL_STORE_PATCH,  // stored patch new sealant (after read valid, new write)
};

struct file_block_definition {
	uint32_t avail;
	uint32_t used;
};
struct file_block_small_definition {
	uint16_t avail;
	uint16_t used;
};
struct file_block_large_definition {
	uint64_t avail;
	uint64_t used;
};

struct file_header {
	struct file_block_small_definition sealant;
	struct file_block_definition references;
	struct file_block_large_definition fileData;
	struct file_block_small_definition indexes;
	struct file_block_definition referencedBy;
};

static void flushFileSuffix( struct sack_vfs_os_file* file );
static void WriteIntoBlock( struct sack_vfs_os_file* file, int blockType, FPI pos, CPOINTER data, FPI length );


#include "vfs_os_timeline_unsorted.c"

//#include "vfs_os_timeline.c"
//#define priorData prior.ref.index

struct blockInfo {
	BLOCKINDEX block;
	FPI start;
	int size;
};

struct sack_vfs_os_file
{
	struct sack_vfs_os_volume* vol; // which volume this is in
	FPI fpi;
	BLOCKINDEX _first_block;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
	LOGICAL delete_on_close;  // someone already deleted this...
	struct blockInfo* blockChain;
	unsigned int blockChainAvail;
	unsigned int blockChainLength;

#  ifdef FILE_BASED_VFS
	FPI entry_fpi;  // where to write the directory entry update to
	BLOCKINDEX dir_block; // delete also needs the block number
#    ifdef XX_VIRTUAL_OBJECT_STORE
	/* extended internal file information that just makes it harder to recover in a crash.*/
	int blockSize;
	struct file_header diskHeader;
	struct file_header header;  // in-memory size, so we can just do generic move op

	//struct memoryTimelineNode timeline;
	uint8_t* seal;
	uint8_t* sealant;
	uint8_t* readKey;
	uint16_t readKeyLen;
	//uint8_t sealantLen;
	uint8_t sealed; // boolean, on read, validates seal.  Defaults to FALSE.
	//char* filename;
	LOGICAL fileName;
#    endif
	struct sack_vfs_os_file_flags {
		BIT_FIELD versioned : 1;
	}flags;
	struct directory_entry  entry_;  // has file size within
	struct directory_entry* entry;  // has file size within
	VFS_DISK_DATATYPE filesize_;  // how big the file is (live - reflects size for files opened by version)

	enum block_cache_entries cache; // files without names use this as thier preferred cache target
#  else
	struct directory_entry* entry;  // has file size within
#  endif

};

typedef struct sack_vfs_os_file VFS_OS_FILE;
#define MAXVFS_OS_FILESPERSET 256
DeclareSet( VFS_OS_FILE );

#define l vfs_os_local
static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[KEY_SIZE];
	uint16_t index[256][256];
	char leadin[MAX_FILENAME_LEN];
	int leadinDepth;
	PLINKQUEUE plqCrypters;
	PLIST volumes;
	LOGICAL exited;
	PVFS_OS_FILESET files;

#ifdef DEBUG_CONVERT_DIRECTORY // symbol not defined
	int fileCount;
	int fileCount_old;
#endif
} l;


//static void _os_UpdateFileBlocks( struct sack_vfs_os_file* file );
static struct sack_vfs_os_file* _os_createFile( struct sack_vfs_os_volume* vol, BLOCKINDEX first_block, int blockSize );
static int sack_vfs_os_close_internal( struct sack_vfs_os_file* file, int unlock );
static enum block_cache_entries _os_UpdateSegmentKey_( struct sack_vfs_os_volume* vol, enum block_cache_entries* cache_idx, BLOCKINDEX segment DBG_PASS );

static uint32_t _os_AddSmallBlockUsage( struct file_block_small_definition* block, uint32_t more );

#ifdef DEBUG_DIRECTORIES
static int _os_dumpDirectories( struct sack_vfs_os_volume *vol, BLOCKINDEX start, LOGICAL init );
#endif
//#include "vfs_os_index.c"

static void _os_SetSmallBlockUsage( struct file_block_small_definition* block, int more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail += 128;
}

 uint32_t _os_AddSmallBlockUsage( struct file_block_small_definition* block, uint32_t more ) {
	uint32_t oldval = block->used;
	_os_SetSmallBlockUsage( block, block->used + more );
	return oldval;
}

static void _os_SetFileBlockUsage( struct file_block_small_definition* block, uint32_t more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail += 256;
}

ATEXIT( flushVolumes ){
	INDEX idx;
	struct sack_vfs_os_volume* vol;
	l.exited = 1;
	LIST_FORALL( l.volumes, idx, struct sack_vfs_os_volume*, vol ) {

		if( vol->file )
		sack_vfs_os_flush_volume( vol, TRUE );
#ifdef DEBUG_DIRECTORIES
		_os_dumpDirectories( vol, 0, 1 );
#endif
	}

}


#if 0

#define FILE_BLOCK_SEALANT 0
#define FILE_BLOCK_REFERENCES 1
#define FILE_BLOCK_DATA 2
#define FILE_BLOCK_INDEXES 3
#define FILE_BLOCK_REFERENCED_BY 4

static FPI GetBlockStart( struct sack_vfs_os_file* file, int blockType ) {
	FPI blockStart = sizeof( struct file_header );

	switch( blockType ) {
		//case 5:
		//	blockStart += file->header.fileData.avail; // end of file.
	case FILE_BLOCK_REFERENCED_BY:
		blockStart += file->header.indexes.avail;
	case FILE_BLOCK_INDEXES:
		blockStart += (FPI)file->header.fileData.avail;
	case FILE_BLOCK_DATA:
		blockStart += file->header.references.avail;
	case FILE_BLOCK_REFERENCES:
		blockStart += file->header.sealant.avail;
	case FILE_BLOCK_SEALANT:
		// starts at position 0.
		break;
	}
	return blockStart;
}
void WriteIntoBlock( struct sack_vfs_os_file* file, int blockType, FPI pos, CPOINTER data, FPI length ) {
	FPI blockStart = GetBlockStart( file, blockType );

	sack_vfs_os_seek_internal( file, (size_t)blockStart, SEEK_SET );
	sack_vfs_os_write_internal( file, data, (size_t)length, NULL );
}

static void _os_SetLargeBlockUsage( struct file_block_large_definition* block, uint64_t more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail = ( block->used + BLOCK_SIZE ) & BLOCK_MASK;
}
#endif

static void _os_ExtendBlockChain( struct sack_vfs_os_file* file ) {
	int newSize = ( file->blockChainAvail ) * 2 + 1;
	file->blockChain = ( struct blockInfo*)Reallocate( file->blockChain, newSize * sizeof( struct blockInfo ) );
#ifdef _DEBUG
	// debug
	memset( file->blockChain + file->blockChainAvail, 0, ( newSize - file->blockChainAvail ) * sizeof( struct blockInfo ) );
#endif
	 file->blockChainAvail = newSize;
}

static unsigned int getBlockChainBlock( struct sack_vfs_os_file* file, FPI fpi ) {
	unsigned int fileBlock = file->blockChainLength;
	int block;
	int minLen = 0;
	int maxLen = file->blockChainLength - 1;
	while( minLen <= maxLen ) {
		block = ( minLen + maxLen ) / 2;
		if( fpi >= ( file->blockChain[block].start + file->blockChain[block].size ) )
			minLen = block + 1;
		else if( fpi < file->blockChain[block].start )
			maxLen = block - 1;
		else {
			fileBlock = block;
			break;
		}
	}
	return fileBlock;
}

static void _os_SetBlockChain( struct sack_vfs_os_file* file, FPI fpi, BLOCKINDEX newBlock, int size ) {
	FPI fileBlock = file->blockChainLength;
	//lprintf( "Set chain %p %d %d %d", file, (int)fpi, (int)newBlock, (int)size );
	if( file->blockChainLength ) {
		if( fpi < ( file->blockChain[file->blockChainLength - 1].start + file->blockChain[file->blockChainLength - 1].size ) ) {
			// when seek happens and initial position is past the end,
			// seek has to step through the file, for each block to adjust size properly...
			//lprintf( "Re-setting an internal block?" );
			fileBlock = getBlockChainBlock( file, fpi );
		}
		else if( fpi == ( file->blockChain[file->blockChainLength - 1].start + file->blockChain[file->blockChainLength - 1].size ) ) {
		}
	}
#ifdef _DEBUG
	if( !newBlock ) DebugBreak();
#endif
	while( (fileBlock) >= file->blockChainAvail ) {
		_os_ExtendBlockChain( file );
	}
	if( fileBlock >= file->blockChainLength )
		file->blockChainLength = (unsigned int)(fileBlock + 1);
	//_lprintf(DBG_SRC)( "setting file at %d  to  %d to %d", (int)file->_first_block, (int)fileBlock, (int)newBlock );
	if( file->blockChain[fileBlock].block ) {
		if( file->blockChain[fileBlock].block == newBlock ) {
			return;
		}
		else {
			lprintf( "Re-setting chain to a new block... %d was %d and wants to be %d", (int)fpi, (int)file->blockChain[fileBlock].block, (int)newBlock );
			DebugBreak();
		}
	}
	file->blockChain[fileBlock].block = newBlock;
	file->blockChain[fileBlock].size = size;
	file->blockChain[fileBlock].start = fpi;
}



// seek by byte position from a starting block; as file; result with an offset into a block.
uintptr_t vfs_os_FSEEK_v2( struct sack_vfs_os_volume *vol
	, struct sack_vfs_os_file *file  // if no file, first block must be manually specified
	, BLOCKINDEX firstblock  // if file, firstblock comes from the file
	, FPI offset    // offset in the block-chain from the specified start
	, enum block_cache_entries *cache_index  // this is the cache entry that the data was loaded into
	, int blockSize   // if a new block is needed, use this size to allocate.
	, LOGICAL flush_BAT_caches // where the new block ( if any) is updated;
	DBG_PASS
)
{
#ifdef DEBUG_FILE_SEEK
	LoG_( "File Seek: %p %d %d", file, (int)offset, cache_index[0] );
#endif
	enum block_cache_entries cacheRoot = cache_index[0];
	size_t priorSize;
	uint8_t *data;
	FPI pos = 0;
	if( file ) {
		unsigned chainBlock = getBlockChainBlock( file, offset );
		if( chainBlock < file->blockChainLength ) {
			firstblock = file->blockChain[chainBlock].block;
			pos = file->blockChain[chainBlock].start;
			offset -= pos;
			priorSize = file->blockChain[chainBlock].size;
		} else {
			if( file->blockChainLength ) {
				if( offset >= file->blockChain[file->blockChainLength - 1].start ) {
					firstblock = file->blockChain[file->blockChainLength - 1].block;
					offset -= ( pos = file->blockChain[file->blockChainLength - 1].start );
					priorSize = file->blockChain[file->blockChainLength - 1].size;
				}
				else {
					lprintf( "Should have found a block for this offset before here." );
					DebugBreak();
				}
			}
			else {
				firstblock = file->_first_block;
				priorSize = vol->sector_size[cache_index[0]];
				cache_index[0] = cacheRoot;
			}
		}
	} else priorSize = blockSize;
	while( firstblock != EOFBLOCK && offset >= priorSize ) {
		int size;
		enum block_cache_entries cache =
				file ?
#ifdef XX_VIRTUAL_OBJECT_STORE
			file->fileName ? BC( FILE ) :
#endif
			file->cache: cacheRoot;
#ifdef DEBUG_FILE_SEEK
		LoG_( "Getting next block after %p %d %d", file, firstblock, blockSize );
#endif
		firstblock = vfs_os_GetNextBlock_v2( vol, firstblock
			, &cache
			, file?
#ifdef XX_VIRTUAL_OBJECT_STORE
			file->fileName?GFB_INIT_NONE:
#endif
			GFB_INIT_TIMELINE_MORE:GFB_INIT_NAMES, 1, blockSize, &size, flush_BAT_caches );
		if( size != blockSize ) {
			lprintf( "Tried to allocate %d got %d at %d (from %d)", blockSize, size, *cache_index, cacheRoot );
			DebugBreak();
		}

		offset -= priorSize;
		pos += priorSize;
		priorSize = size;
		if( file ) {
			_os_SetBlockChain( file, pos, firstblock, size );
		}
	}
	data = (uint8_t*)vfs_os_BSEEK_( vol, firstblock, blockSize, cache_index DBG_RELAY );
#if DEBUG_SET_SECTOR_SIZE
	LoG_( "Sector size: %p (ss)%d (blk)%d (ofs)%d (cache)%d",data, priorSize, (int)firstblock, (int)offset , cache_index[0] );
#endif
	return (uintptr_t)(data + (offset));
}

uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume* vol
	, struct sack_vfs_os_file* file  // if no file, first block must be manually specified
	, BLOCKINDEX firstblock  // if file, firstblock comes from the file
	, FPI offset    // offset in the block-chain from the specified start
	, enum block_cache_entries* cache_index  // this is the cache entry that the data was loaded into
	, int blockSize   // if a new block is needed, use this size to allocate.
	DBG_PASS
) {
	return vfs_os_FSEEK_v2( vol, file, firstblock, offset, cache_index, blockSize, FALSE DBG_RELAY );
}

static void vfs_os_empty_rollback( struct sack_vfs_os_volume* vol ) {
	enum block_cache_entries rollbackCache = BC( ROLLBACK );
	struct vfs_os_rollback_header* rollback = ( struct vfs_os_rollback_header* )vfs_os_FSEEK( vol, vol->journal.rollback_file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
#ifdef DEBUG_ROLLBACK_JOURNAL
	LoG( "------- ROLLBACK FLUSH---------------- %d", rollback->flags.dirty );
#endif
	if( rollback->flags.dirty ) {
		vol->journal.pdlJournaled->Cnt = 0;
		vol->journal.journalLength = 0;
		rollback->flags.dirty = 0;
		rollback->nextBlock = 0;
		rollback->nextSmallBlock = 0;
		rollback->nextEntry = 0;
		SMUDGECACHE( vol, rollbackCache );
		sack_vfs_os_flush_block( vol, rollbackCache );
	}
}

//---------------------------------------------------------------------------

static void ExpandJournalIndex(struct vfs_os_rollback_journal *journal ) {
	BLOCKINDEX *pNewJournaled; // sectors that are in rollback already
	//int journalLength; // how long pJournaled is used
	//int journalAvail; // max length of pJournaled
	journal->journalAvail = journal->journalAvail*2+1;

	pNewJournaled = NewArray( BLOCKINDEX, 	journal->journalAvail );
	MemCpy( pNewJournaled, journal->pJournaled, journal->journalLength * sizeof( *pNewJournaled ) );
	Release( journal->pJournaled );
	journal->pJournaled = pNewJournaled;
}

//---------------------------------------------------------------------------


static void vfs_os_record_rollback( struct sack_vfs_os_volume* vol, enum block_cache_entries entry ) {
	INDEX nextRecord = 0;
	INDEX curIdx = 0;
	BLOCKINDEX segment = vol->segment[entry];
	if( entry >= BC( ROLLBACK ) ) {
		return;
	}
	// not setup to journal yet (initial loading/configuration)
	if( !vol->journal.rollback_file ) {
		//lprintf( "Journal is not a file... bad open? Unjournaled storage?");
		//AddLink( &vol->pending_rollback, (uintptr_t)entry );
		sack_vfs_os_flush_block( vol, entry );
		return;
	}
	if( vol->journal.journalLength )
	{
		int imin = 0;
		int imax = vol->journal.journalLength-1;

		if( segment < vol->journal.pJournaled[imax] )
			
		{
			curIdx = imax >> 1;
			while( imin <= imax && ( curIdx <= imax ) && ( imax >= 0 ) ) {
				int d;
				//LoG( "this name: %s", names );
				if( ( d = (int)(segment - vol->journal.pJournaled[curIdx] ) ) == 0  ) {
					return;
				}
				if( d > 0 ) {
					imin = (int)curIdx + 1;
				} else {
					if( !curIdx ) break;
					imax = (int)curIdx - 1;
				}
				curIdx = (imin + imax) >> 1;
			}
		}
		else if( segment == vol->journal.pJournaled[imax] )
			return; // is already saved as the last.
		else
			curIdx = imax;	
		if( 0 )
		{
			INDEX idx;
			BLOCKINDEX* check;
			DATA_FORALL( vol->journal.pdlJournaled, idx, BLOCKINDEX*, check ) {
				if( check[0] == segment ) {
#ifdef DEBUG_ROLLBACK_JOURNAL
					LoG( "Journal recording already has this sector marked %d %d", idx, segment );
#endif
					return;
				}
			}
		}
	}
#ifdef DEBUG_ROLLBACK_JOURNAL
	LoG( "Recording rollback for %d %d", (int)entry, (int)vol->segment[entry] );
#endif
	if( vol->journal.pdlPendingRecord->Cnt ) {
		// is already in-progress, record that this should be done later.
		AddDataItem( &vol->journal.pdlPendingRecord, &entry );
		return;
	}
	// mark in-progress.
	AddDataItem( &vol->journal.pdlPendingRecord, &entry );

	{
		if( (vol->journal.journalLength+1) >= vol->journal.journalAvail ) ExpandJournalIndex( &vol->journal );
		if( curIdx >= vol->journal.journalLength ) {
			if( curIdx && ( segment < vol->journal.pJournaled[curIdx-1] ) )  {
				vol->journal.pJournaled[curIdx] = vol->journal.pJournaled[curIdx-1];
				vol->journal.pJournaled[curIdx-1] = segment;
				vol->journal.journalLength++;
			}else {				
				vol->journal.pJournaled[curIdx] = segment;
				vol->journal.journalLength++;
			}
		} else {
			if( segment < vol->journal.pJournaled[curIdx] ){
				if( curIdx ) {
					if( segment > vol->journal.pJournaled[curIdx-1] ){
						for( int n = vol->journal.journalLength; n > curIdx; n-- ) {
							vol->journal.pJournaled[n] = vol->journal.pJournaled[n-1];
						}	
					}
					else lprintf( "segment is bad" );
				}else {
					for( int n = vol->journal.journalLength; n > curIdx; n-- ) {
						vol->journal.pJournaled[n] = vol->journal.pJournaled[n-1];
					}	
				}
			}
			else if( segment > vol->journal.pJournaled[curIdx] ){
				if( curIdx + 1 >= vol->journal.journalLength ) {
					curIdx = curIdx+1;
				} else {
					while( segment > vol->journal.pJournaled[curIdx] ) curIdx++;
					for( int n = vol->journal.journalLength; n > (curIdx); n-- ) {
						vol->journal.pJournaled[n] = vol->journal.pJournaled[n-1];
					}
				}
			}
			vol->journal.pJournaled[curIdx] = segment;
			vol->journal.journalLength++;
		}
	}
	//AddDataItem( &vol->journal.pdlJournaled, &segment );

	enum block_cache_entries rollbackCache = BC( ROLLBACK );
	struct vfs_os_rollback_header* rollback = ( struct vfs_os_rollback_header* )vfs_os_FSEEK( vol, vol->journal.rollback_file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
	if( rollback->flags.processing ) return; // don't journal recovery.
	rollback->flags.dirty = 1;

	do {
		enum block_cache_entries rollbackCacheJournal;
		enum block_cache_entries rollbackEntryCache ;
		struct vfs_os_rollback_entry* rollbackEntry;// = vfs_os_FSEEK( vol, vol->journal.rollback_file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
		POINTER journal;
		rollbackEntryCache = BC( ROLLBACK );
#ifdef DEBUG_ROLLBACK_JOURNAL
		LoG( "Record to journal: e: %d fpi:%x seg:%d  ne: %d  at %d", entry, (int)vol->bufferFPI[entry], (int)vol->segment[entry], rollback->nextEntry, sane_offsetof( struct vfs_os_rollback_header, entries[rollback->nextEntry]) );
#endif
		rollbackEntry = (struct vfs_os_rollback_entry*)vfs_os_FSEEK_v2( vol, vol->journal.rollback_file, 0
			, sane_offsetof( struct vfs_os_rollback_header, entries[rollback->nextEntry++] )
			, &rollbackEntryCache, ROLLBACK_BLOCK_SIZE, FALSE DBG_SRC );

		rollbackEntry->fileBlock = vol->segment[entry];

		rollbackCacheJournal = BC( ROLLBACK );
		if( vol->sector_size[entry] == BLOCK_SIZE ) {
			int n;
			uint64_t* p = (uint64_t*)vol->usekey_buffer_clean[entry];
			rollbackEntry->flags.zero = 0;
			rollbackEntry->flags.small = 0;
			for( n = 0; !p[0] && (n < (BLOCK_SIZE / sizeof( uint64_t ))); n++, p++ ) ;
			if( n < BLOCK_SIZE / sizeof( uint64_t ) ) {
#ifdef DEBUG_ROLLBACK_JOURNAL
				LoG( "Found non 0 at: %d  %d", n, p[0] );
				LoG( "Saving large, clean block" );
#endif
				journal = (POINTER)vfs_os_FSEEK_v2( vol, vol->journal.rollback_journal_file, 0, vol->sector_size[entry] * rollback->nextBlock++, &rollbackCacheJournal, vol->sector_size[entry], TRUE DBG_SRC );
			}  else {
#ifdef DEBUG_ROLLBACK_JOURNAL
				LoG( "Save as large zero" );
#endif
				rollbackEntry->flags.zero = 1;
			}
		} else {
			int n;
			uint64_t* p = (uint64_t*)vol->usekey_buffer_clean[entry];
			rollbackEntry->flags.zero = 0;
			rollbackEntry->flags.small = 1;
			for( n = 0; !p[0] && n < BLOCK_SMALL_SIZE / sizeof( uint64_t ); n++, p++ ) ;
			if( n < BLOCK_SMALL_SIZE / sizeof( uint64_t ) ) {
#ifdef DEBUG_ROLLBACK_JOURNAL
				LoG( "Found non 0 at: %d  %d", n, p[0] );
				LoG( "Saving small, clean block" );
#endif
				journal = (POINTER)vfs_os_FSEEK_v2( vol, vol->journal.rollback_small_journal_file, 0, vol->sector_size[entry] * rollback->nextSmallBlock++, &rollbackCacheJournal, vol->sector_size[entry], TRUE DBG_SRC );
			} else {
#ifdef DEBUG_ROLLBACK_JOURNAL
				LoG( "Save as small zero" );
#endif
				rollbackEntry->flags.zero = 1;
			}
		}
		if( !rollbackEntry->flags.zero ) {
			memcpy( journal, vol->usekey_buffer_clean[entry], vol->sector_size[entry] );

			SMUDGECACHE( vol, rollbackCacheJournal );
			sack_vfs_os_flush_block( vol, rollbackCacheJournal );
		}
		if( rollbackEntryCache != rollbackCache ) {
			SMUDGECACHE( vol, rollbackEntryCache );
			sack_vfs_os_flush_block( vol, rollbackEntryCache );
		}
		if( ++nextRecord < vol->journal.pdlPendingRecord->Cnt )
			entry = ( ( enum block_cache_entries* )GetDataItem( &vol->journal.pdlPendingRecord, nextRecord ) )[0];
		else {
			vol->journal.pdlPendingRecord->Cnt = 0; // empty the list.
			//vol->journal.pdlJournaled->Cnt = 0; // empty the list.
		}
	} while( vol->journal.pdlPendingRecord->Cnt );
	{
		int n;
		BLOCKINDEX prior = vol->journal.pJournaled[0];
		for( n = 1; n < vol->journal.journalLength; n++ ) {
			if( vol->journal.pJournaled[n] < prior ) DebugBreak();
			prior = vol->journal.pJournaled[n];
		}
	}

	SMUDGECACHE( vol, rollbackCache );
	sack_vfs_os_flush_block( vol, rollbackCache );
}

// reads the rollback journal and reverts anything.
static void vfs_os_process_rollback( struct sack_vfs_os_volume* vol ) {
	enum block_cache_entries rollbackCache = BC( ROLLBACK );
	enum block_cache_entries rollbackCacheJournal;
	enum block_cache_entries rollbackEntryCache;
	enum block_cache_entries rollbackEntryCache_ = BC( ZERO );
	enum block_cache_entries entry;
	struct vfs_os_rollback_header* rollback = ( struct vfs_os_rollback_header* )vfs_os_FSEEK( vol, vol->journal.rollback_file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
	struct vfs_os_rollback_entry* rollbackEntry;// = vfs_os_FSEEK( vol, vol->journal.rollback_file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
	POINTER journal;
	SETMASK_( vol->seglock, seglock, rollbackCache, GETMASK_( vol->seglock, seglock, rollbackCache )+1 );
	BLOCKINDEX e;
#ifdef DEBUG_ROLLBACK_JOURNAL
	LoG( "---- READ ROLLBACK ---- %d", rollback->flags.dirty );
#endif
	if( rollback->flags.dirty ) {
		BLOCKINDEX bigSector = 0;
		BLOCKINDEX smallSector = 0;
		struct BATInfo {
			struct vfs_os_rollback_entry entry;
			BLOCKINDEX bigSector;
		};
		PDATALIST pdlBATs = CreateDataList( sizeof( struct BATInfo ) );
		rollback->flags.processing = 1;
		for( e = 0; e < rollback->nextEntry; e++ ) {
			rollbackEntryCache = BC( ROLLBACK );
			rollbackEntry = ( struct vfs_os_rollback_entry* )vfs_os_FSEEK( vol, vol->journal.rollback_file, 0
				, sane_offsetof( struct vfs_os_rollback_header, entries[e] )
				, &rollbackEntryCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
			if( rollbackEntryCache != rollbackEntryCache_ ) {
				if( rollbackEntryCache_ ) {
					// unlock the previous (if there was one)
					SETMASK_( vol->seglock, seglock, rollbackEntryCache_, GETMASK_( vol->seglock, seglock, rollbackEntryCache_ ) - 1 );
				}
				// lock the new one
				SETMASK_( vol->seglock, seglock, rollbackEntryCache, GETMASK_( vol->seglock, seglock, rollbackEntryCache ) + 1 );
				rollbackEntryCache_ = rollbackEntryCache;
			}
			if( ( ( rollbackEntry->fileBlock - 1 ) % BLOCKS_PER_SECTOR ) == 0 ) {
				// defer restoring BAT blocks until end;
				// the journal itself may exist in the dirty blocks already in the image.
				struct BATInfo info;
				info.entry = rollbackEntry[0];
				info.bigSector = bigSector++;
				AddDataItem( &pdlBATs, &info );
				continue;
			}
#ifdef DEBUG_ROLLBACK_JOURNAL
			LoG( "Reading rollback fileblock:%d %s", rollbackEntry->fileBlock, rollbackEntry->flags.zero ? "Empty" : "" );
#endif
			entry = BC( ROLLBACK );
			_os_UpdateSegmentKey_( vol, &entry, rollbackEntry->fileBlock DBG_SRC );
			vol->sector_size[entry] = rollbackEntry->flags.small ? BLOCK_SMALL_SIZE : BLOCK_SIZE;
			if( rollbackEntry->flags.zero ) {
				memset( vol->usekey_buffer[entry], 0, rollbackEntry->flags.small ? BLOCK_SMALL_SIZE : BLOCK_SIZE );
			}
			else {
				int useSize;
				rollbackCacheJournal = BC( ROLLBACK );
				if( !rollbackEntry->flags.small ) {
					journal = (POINTER)vfs_os_FSEEK( vol, vol->journal.rollback_journal_file, 0
						, BLOCK_SIZE * bigSector++, &rollbackCacheJournal, BLOCK_SIZE DBG_SRC );
#ifdef DEBUG_ROLLBACK_JOURNAL
					if( memcmp( vol->usekey_buffer[entry], journal, BLOCK_SIZE ) == 1 ) {
						LoG( "Journal is actually updating data..." );
					}
#endif
					memcpy( vol->usekey_buffer[entry], journal, useSize = BLOCK_SIZE );
				}
				else {
					journal = (POINTER)vfs_os_FSEEK( vol, vol->journal.rollback_small_journal_file, 0
						, BLOCK_SMALL_SIZE * smallSector++, &rollbackCacheJournal, BLOCK_SMALL_SIZE DBG_SRC );
#ifdef DEBUG_ROLLBACK_JOURNAL
					if( memcmp( vol->usekey_buffer[entry], journal, BLOCK_SMALL_SIZE ) == 1 ) {
						LoG( "Journal is actually updating data..." );
					}
#endif
					memcpy( vol->usekey_buffer[entry], journal, useSize = BLOCK_SMALL_SIZE );
				}
				{
					int seg;
					for( seg = 0; seg < BC( ROLLBACK ); seg++ ) {
						if( vol->segment[seg] == rollbackEntry->fileBlock ) {
							//lprintf( "Duplicate! %d", vol->segment[seg] );
							memcpy( vol->usekey_buffer[seg], vol->usekey_buffer[entry], useSize );
							memcpy( vol->usekey_buffer_clean[seg], vol->usekey_buffer[entry], useSize );
						}
					}
				}
			}


			SMUDGECACHE( vol, entry );
		}
		// finally, clear the BAT entries with any existing bat sectors
		{
			struct BATInfo *info;
			DATA_FORALL( pdlBATs, e, struct BATInfo*, info ) {
				entry = BC( ROLLBACK );
				_os_UpdateSegmentKey_( vol, &entry, info->entry.fileBlock DBG_SRC );
				// BATs are always not-small blocks.
				vol->sector_size[entry] = BLOCK_SIZE;
				if( info->entry.flags.zero ) {
					// might happen later; usually these are non-zero filled
					memset( vol->usekey_buffer[entry], 0, BLOCK_SIZE );
				}
				else {
					rollbackCacheJournal = BC( ROLLBACK );
					journal = (POINTER)vfs_os_FSEEK( vol, vol->journal.rollback_journal_file, 0
						, BLOCK_SIZE * info->bigSector++, &rollbackCacheJournal, BLOCK_SIZE DBG_SRC );

					memcpy( vol->usekey_buffer[entry], journal, BLOCK_SIZE );
					memcpy( vol->usekey_buffer_clean[entry], journal, BLOCK_SIZE );
				}
				{
					int seg;
					for( seg = 0; seg < BC( ROLLBACK ); seg++ ) {
						if( vol->segment[seg] == info->entry.fileBlock ) {
							//lprintf( "Duplicate! %d", vol->segment[seg] );
							memcpy( vol->usekey_buffer[seg], vol->usekey_buffer[entry], BLOCK_SIZE );
							memcpy( vol->usekey_buffer_clean[seg], vol->usekey_buffer[entry], BLOCK_SIZE );
						}
					}
				}
			}
			DeleteDataList( &pdlBATs );
		}

		rollback->flags.dirty = 0;
		rollback->nextEntry = 0;
		rollback->nextBlock = 0;
		rollback->nextSmallBlock = 0;

		SETMASK_( vol->seglock, seglock, rollbackEntryCache_, GETMASK_( vol->seglock, seglock, rollbackEntryCache_ ) - 1 );
		//SETMASK_( vol->seglock, seglock, rollbackCacheJournal, GETMASK_( vol->seglock, seglock, rollbackCacheJournal ) - 1 );
		SETMASK_( vol->seglock, seglock, rollbackCache, GETMASK_( vol->seglock, seglock, rollbackCache ) - 1 );
		SMUDGECACHE( vol, rollbackCache );
		rollback->flags.processing = 0;
		// after processing, the disk should be back to 0; so don't rollback NEXT reload again.
		sack_vfs_os_flush_block( vol, rollbackCache );

	}
}

void vfs_os_smudge_cache( struct sack_vfs_os_volume* vol, enum block_cache_entries n ) {
	if( !TESTFLAG( vol->dirty, n ) ) {
#ifdef DEBUG_SECTOR_DIRT
		lprintf( "set dirty on %d", n);
#endif
		SETFLAG( vol->dirty, n );
		if( !TESTFLAG( vol->_dirty, n ) )
			vfs_os_record_rollback( vol, n );
	}
}

// pass absolute, 0 based, block number that is the index of the block in the filesystem.
static FPI vfs_os_compute_block( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum block_cache_entries cache ) {
	struct sack_vfs_os_BAT_info *info;
	INDEX idx = block / BLOCKS_PER_SECTOR;
	info = (struct sack_vfs_os_BAT_info*)GetDataItem( &vol->pdl_BAT_information, idx );
	if( !info ) {
		if( !block ) return 0;
		else {
			info = (struct sack_vfs_os_BAT_info*)GetDataItem( &vol->pdl_BAT_information, idx-1 );
			if( info ) 
				return info->sectorEnd;
		}
		return ~0;
	}
	{
		if( block % BLOCKS_PER_SECTOR ) { // not reading a BAT, add the fixed offset, 0 based data offset
			//if( cache < BC(COUNT) )
#if defined( DEBUG_SET_SECTOR_SIZE )
			LoG( "Setting sector size according to BAT information:%d %d %d %d", info->size, (int)block, (cache<BC(COUNT))?(int)vol->segment[cache]:0, (int)cache );
#endif
			if( cache >= BC(COUNT) ) ;//lprintf( "CACHE OVERFLOW not setting info %d", info->size );
			else vol->sector_size[cache] = info->size;
			// the first 'block' is always bat size, so add that, and then the remaining
			// smaller blocks...

			return info->sectorStart + BAT_BLOCK_SIZE + ( ( block % BLOCKS_PER_SECTOR ) - 1 ) * info->size;
		} else {
			if( cache >= BC(COUNT) ) ;//lprintf( "CACHE OVERFLOW skipping size(BAT)" );
			else vol->sector_size[cache] = BAT_BLOCK_SIZE;
			return info->sectorStart;
		}
	}
	return 0;
}

// pass absolute, 0 based, block number that is the index of the block in the filesystem.
static FPI vfs_os_compute_data_block( struct sack_vfs_os_volume* vol, BLOCKINDEX block, enum block_cache_entries cache ) {
	struct sack_vfs_os_BAT_info* info;
	INDEX idx = block / BLOCKS_PER_BAT;
	info = ( struct sack_vfs_os_BAT_info* )GetDataItem( &vol->pdl_BAT_information, idx );
	if( !info ) return 0;

	// not reading a BAT, add the fixed offset, 0 based data offset
	if( cache < BC( COUNT ) )
		vol->sector_size[cache] = info->size;

	// the first 'block' is always bat size, so add that, and then the remaining
	// smaller blocks...
	return info->sectorStart + BAT_BLOCK_SIZE + ( ( block - 1 ) % BLOCKS_PER_BAT ) * info->size;
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
		return s1[0] - (((unsigned char)s2[0] == UTF8_EOT)?0:s2[0]);
	return 0;
}

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int _os_MaskStrCmp( struct sack_vfs_os_volume *vol, const char * filename, BLOCKINDEX nameBlock, FPI name_offset, int path_match ) {
	enum block_cache_entries cache = BC(NAMES);
	const char *dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache, NAME_BLOCK_SIZE DBG_SRC );
	const char *prior_dirname = dirname;
	if( !dirname ) return 1;
	{
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
static void _os_MaskStrCpy( char *output, size_t outlen, struct sack_vfs_os_volume *vol, enum block_cache_entries cache, FPI name_offset ) {
	{
		int c;
		FPI name_start = name_offset;
		while( UTF8_EOT != (unsigned char)( c = vol->usekey_buffer[cache][name_offset&BLOCK_MASK] ) ) {
			if( ( name_offset - name_start ) < outlen )
				output[name_offset-name_start] = c;
			name_offset++;
		}
		if( ( name_offset - name_start ) < outlen )
			output[name_offset-name_start] = 0;
		else
			output[outlen-1] = 0;
	}
}
#endif

#ifdef DEBUG_DIRECTORIES
int _os_dumpDirectories( struct sack_vfs_os_volume *vol, BLOCKINDEX start, LOGICAL init ) {
	struct directory_hash_lookup_block *dirBlock;
	struct directory_entry *next_entries;
	static char leadin[MAX_FILENAME_LEN];
	static int leadinDepth;
	char outfilename[MAX_FILENAME_LEN];
	int outfilenamelen;
	size_t n;
	if( init )
		leadinDepth = 0;
	LoG( "Starting directory dump: %d %d", start, init );
	{
		enum block_cache_entries cache = BC( DIRECTORY );
		enum block_cache_entries name_cache = BC( NAMES );
		struct directory_hash_lookup_block _dirBlock;
		
		dirBlock = BTSEEK( struct directory_hash_lookup_block *, vol, start, DIR_BLOCK_SIZE, cache );
		_dirBlock = dirBlock[0];
		dirBlock = &_dirBlock;
		lprintf( "leadin : %*.*s %d names:%d start:%d", leadinDepth, leadinDepth, leadin, leadinDepth, dirBlock->used_names, (int)start );
		next_entries = dirBlock->entries;

		for( n = 0; n < dirBlock->used_names; n++ ) {
			FPI name_ofs = next_entries[n].name_offset & DIRENT_NAME_OFFSET_OFFSET;
			const char *filename;
			int l;

			// if file is deleted; don't check it's name.
			if( (name_ofs) > vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
			if( !filename ) return 0;
			outfilenamelen = 0;
			for( l = 0; l < leadinDepth; l++ ) outfilename[outfilenamelen++] = leadin[l];

			if( vol->key ) {
				int c;
				while( (c = (((uint8_t*)filename)[0] )) != UTF8_EOT ) {
					if( c == UTF8_EOTB ) break;
					outfilename[outfilenamelen++] = c;
					//filename++;
					name_ofs++;
					name_cache = BC( NAMES );
					filename = (const char *)vfs_os_FSEEK( vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
				}
			}
			else {
				int c;
				while( (c = (((uint8_t*)filename)[0] )) != UTF8_EOT ) {
					if( c == UTF8_EOTB ) break;
					outfilename[outfilenamelen++] = c;
					//filename++;
					name_ofs++;
					name_cache = BC( NAMES );
					filename = (const char *)vfs_os_FSEEK( vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
				}
			}
			lprintf( "%3d filename: %5d %.*s", n, next_entries[n].name_offset & DIRENT_NAME_OFFSET_OFFSET, outfilenamelen, outfilename );
			if( outfilenamelen > 44 ) DebugBreak();
		}

		for( n = 0; n < 255; n++ ) {
			BLOCKINDEX block = dirBlock->next_block[n];
			if( block ) {
				lprintf( "Found directory with char '%c' %d", n, block );
				leadin[leadinDepth] = (char)n;
				leadinDepth = leadinDepth + 1;
#ifdef DEBUG_DIRECTORIES
				_os_dumpDirectories( vol, block, 0 );
#endif
				leadinDepth = leadinDepth - 1;
			}
		}
	};
	return 0;
}
#endif

#ifdef _MSC_VER
// this is about nLeast not being initialized.
// it will be set if it's used, if it's not
// initialized, it won't be used.
#pragma warning( disable: 6001 )
#endif

#define _os_updateCacheAge(v,c,s,a,l) _os_updateCacheAge_(v,c,s,a,l DBG_SRC )
//
// THis is assigning segment into a cache entry, and then reading that data into memory.
// Large block filebuffers (? like 64 megs of empty space?)
// where the block is located determines the size of that block; this updates
// the size
static void _os_updateCacheAge_( struct sack_vfs_os_volume *vol, enum block_cache_entries *cache_idx, BLOCKINDEX segment, uint8_t *age, int ageLength DBG_PASS ) {
	int n, m;
	int least;
	int nLeast = -1;
	enum block_cache_entries cacheRoot = cache_idx[0];
	BLOCKINDEX *test_segment = vol->segment + cacheRoot;
	least = ageLength + 1;
#ifdef DEBUG_CACHE_FAULTS
	switch( cacheRoot ) {
	case BC(TIMELINE):
		vol->cacheRequests[0]++;
		break;
	case BC( DIRECTORY ):
		vol->cacheRequests[1]++;
		break;
	}
#endif
	for( n = 0; n < (ageLength); n++,test_segment++ ) {
		if( test_segment[0] == segment ) {
			//if( pFile ) LoG_( "Cache found existing segment already. %d at %d(%d)", (int)segment, (cache_idx[0]+n), (int)n );
#ifdef DEBUG_VALIDATE_TREE
			//if( cache_idx[0] < BC( TIMELINE_RO ) )
			//	_lprintf( DBG_RELAY )( "FOUND segment in cache: %d   %d  %d   %d", segment, n, age[n], cache_idx[0] );
#endif
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) {
				if( !age[m] ) break;
				if( age[m] > age[n] ) {
					age[m]--;
				}
			}
			age[n] = m;
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
		if( (age[n] < least) && !GETMASK_( vol->seglock, seglock, cache_idx[0] + n ) ) {
			least = age[n];
			nLeast = n; // this one will be oldest, unlocked candidate
		}
	}
	if( ( n == ( ageLength ) ) && ( nLeast < 0 ) ) {
		lprintf( "All cache blocks are locked, unable to find a free, old block." );
		DebugBreak();
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
		//_lprintf(DBG_RELAY)( "reclaim %d for seg %d", useCache, segment );
		if( TESTFLAG( vol->dirty, useCache ) || TESTFLAG( vol->_dirty, useCache ) ) {
#ifdef DEBUG_DISK_IO
			LoG_( "MUST CLAIM SEGMENT Flush dirty segment: %d fpi:%x %d cache:%d", nLeast, vol->bufferFPI[useCache], vol->segment[useCache], useCache );
#  ifdef DEBUG_DISK_DATA
			LogBinary( vol->usekey_buffer[useCache], vol->sector_size[useCache] );
#  endif
#endif
			sack_fseek( vol->file, (size_t)vol->bufferFPI[useCache], SEEK_SET );
			if( vol->key ) {
				uint8_t* crypt;
				size_t cryptlen;
				SRG_XSWS_encryptData( vol->usekey_buffer[useCache], vol->sector_size[useCache]
					, vol->segment[useCache], (const uint8_t*)vol->key, 1024 /* some amount of the key to use */
					, &crypt, &cryptlen );
				if( !vol->flags.halted )
					sack_fwrite( crypt, 1, vol->sector_size[useCache], vol->file );
				Deallocate( uint8_t*, crypt );
			}else {
				if( !vol->flags.halted )
					sack_fwrite( vol->usekey_buffer[useCache], 1, vol->sector_size[useCache], vol->file );
			}

#ifdef DEBUG_CACHE_FLUSH
			// if not dirty, then clean and buffer have to match; and this is clearing the dirty flag
			memcpy( vol->usekey_buffer_clean[useCache], vol->usekey_buffer[useCache], BLOCK_SIZE );
#  ifdef DEBUG_VALIDATE_TREE
			if( useCache < BC( TIMELINE_RO ) ) // timeline cache is noisy for readonly
#  endif
				_lprintf(DBG_RELAY)( "(usedto)Updated clean buffer %d", useCache );
#endif
			CLEANCACHE( vol, useCache );
			RESETFLAG( vol->_dirty, useCache );
		}
#ifdef DEBUG_VALIDATE_TREE
		else {
#ifdef DEBUG_CACHE_FLUSH
#  ifdef DEBUG_VALIDATE_TREE
			if( cache_idx[0] < BC(TIMELINE_RO) )
#  endif
				if( memcmp( vol->usekey_buffer_clean[cache_idx[0]], vol->usekey_buffer[cache_idx[0]], BLOCK_SIZE ) ) {
					lprintf( "Block was written to, but was not flagged as dirty, changes will be lost." );
					DebugBreak();
				}
#endif
		}
#endif
		vol->segment[useCache] = segment;
	}
#ifdef DEBUG_VALIDATE_TREE
	//if( cache_idx[0] < BC(TIMELINE_RO) )
	//	_lprintf(DBG_RELAY)( "Get segment into cache: %d   %d", segment, cache_idx[0] );
#endif
#ifdef DEBUG_CACHE_FAULTS
	switch( cacheRoot ) {
	case BC( TIMELINE ):
		vol->cacheFaults[0]++;
		break;
	case BC( DIRECTORY ):
		vol->cacheFaults[1]++;
		break;
	}
#endif
	{
		size_t short_Read;
		do {
			vol->bufferFPI[cache_idx[0]] = vfs_os_compute_block( vol, segment - 1, cache_idx[0] );
			if( vol->bufferFPI[cache_idx[0]] == ~0 ) 
				_os_ExpandVolume( vol, vol->lastBlock, vol->sector_size[cache_idx[0]] );
		} while( vol->bufferFPI[cache_idx[0]] == ~0 );
		// read new buffer for new segment
		sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx[0]], SEEK_SET );
#ifdef DEBUG_DISK_IO
		LoG_( "Read into block: fpi:%x cache:%d n:%d  seg:%d buf:%p size:%d", (int)vol->bufferFPI[cache_idx[0]], (int)cache_idx[0] , (int)n, (int)segment, vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]]  );
#endif
		if( !( short_Read = sack_fread( vol->usekey_buffer[cache_idx[0]], 1, vol->sector_size[cache_idx[0]], vol->file ) ) ) {
			memset( vol->usekey_buffer[cache_idx[0]], 0, vol->sector_size[cache_idx[0]] );
			// only need to clear this if it's checked for write data without setting dirty flag.
			memset( vol->usekey_buffer_clean[cache_idx[0]], 0, vol->sector_size[cache_idx[0]] );
		} else {
			if( short_Read != vol->sector_size[cache_idx[0]] ) {
				lprintf( "Short read on file:%" _size_f, short_Read );
			}
			if( vol->key )
				SRG_XSWS_decryptData( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]]
					, vol->segment[cache_idx[0]], (const uint8_t*)vol->oldkey?vol->oldkey:vol->key, 1024 /* some amount of the key to use */
					, NULL, NULL );
			// after reading what was on the disk (in the sector), save it as the 'clean' state for journaling
			// modifications happen to usekey_buffer before SMUDGE is called.
			memcpy( vol->usekey_buffer_clean[cache_idx[0]], vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]] );
#ifdef DEBUG_CACHE_FLUSH
#  ifdef DEBUG_VALIDATE_TREE
			if( cache_idx[0] < BC(TIMELINE_RO) )
#  endif
				_lprintf(DBG_RELAY)( "Updated clean buffer %d", cache_idx[0] );
#endif
		}
#ifdef DEBUG_DISK_IO
#  ifdef DEBUG_DISK_DATA
		LogBinary( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]] );
#  endif
#endif
	}
#ifdef DEBUG_CACHE_AGING
	LoG( "age end2:" );
	LogBinary( age, ageLength );
#endif
}

#define _os_UpdateSegmentKey(v,c,s) _os_UpdateSegmentKey_(v,c,s DBG_SRC )

enum block_cache_entries _os_UpdateSegmentKey_( struct sack_vfs_os_volume *vol, enum block_cache_entries* cache_idx, BLOCKINDEX segment DBG_PASS )
{
	//BLOCKINDEX oldSegs[BC(COUNT)];
	//memcpy(oldSegs, vol->segment, sizeof(oldSegs));
	if( cache_idx[0] == BC(FILE) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->fileCacheAge, (BC(FILE_LAST) - BC(FILE)) DBG_RELAY );
	}
	else if( cache_idx[0] == BC(NAMES) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->nameCacheAge, (BC(NAMES_LAST) - BC(NAMES)) DBG_RELAY );
	}
#ifdef VIRTUAL_OBJECT_STORE
	else if( cache_idx[0] == BC(DIRECTORY) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->dirHashCacheAge, (BC(DIRECTORY_LAST) - BC(DIRECTORY)) DBG_RELAY );
	}
	else if( cache_idx[0] == BC( TIMELINE ) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->timelineCacheAge, (BC( TIMELINE_LAST ) - BC( TIMELINE )) DBG_RELAY );
	}
	else if( cache_idx[0] == BC( ROLLBACK ) ) {
		//lprintf( "Cache age rollback: %d", (int)segment );
		_os_updateCacheAge_( vol, cache_idx, segment, vol->rollbackCacheAge, ( BC( ROLLBACK_LAST ) - BC( ROLLBACK ) ) DBG_RELAY );
	}
#ifdef DEBUG_VALIDATE_TREE
	else if( cache_idx[0] == BC( TIMELINE_RO ) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->timelineCacheAge, ( BC( TIMELINE_RO_LAST ) - BC( TIMELINE_RO ) ) DBG_RELAY );
		{
			int n;
			for( n = BC( TIMELINE ); n < BC( TIMELINE_LAST ); n++ ) {
				if( vol->segment[n] == segment ) {
					if( TESTFLAG( vol->dirty, n ) || TESTFLAG( vol->_dirty, n ) ) {
						// use the cached value instead of the disk value.
						memcpy( vol->usekey_buffer[cache_idx[0]], vol->usekey_buffer[n], BLOCK_SIZE );
						memcpy( vol->usekey_buffer_clean[cache_idx[0]], vol->usekey_buffer[n], BLOCK_SIZE );
						//lprintf( "Updaed clean buffer %d", n );
					}
					break;
				}
			}
		}
	}
#endif
	else if( cache_idx[0] == BC( BAT ) ) {
		_os_updateCacheAge_( vol, cache_idx, segment, vol->batHashCacheAge, (BC(BAT_LAST) - BC(BAT)) DBG_RELAY );
	}
#endif
	else {
		if( vol->segment[cache_idx[0]] != segment ) {
			if( TESTFLAG( vol->dirty, cache_idx[0] ) || TESTFLAG( vol->_dirty, cache_idx[0] ) ) {
#ifdef DEBUG_DISK_IO
				LoG_( "MUST CLAIM SEGMENT Flush dirty segment: fpi:%x %d", vol->bufferFPI[cache_idx[0]], vol->segment[cache_idx[0]] );
#  ifdef DEBUG_DISK_DATA
				LogBinary( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]] );
#  endif
#endif
				sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx[0]], SEEK_SET );
				if( vol->key ) {
					uint8_t* crypt;
					size_t cryptlen;
					SRG_XSWS_encryptData( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]]
						, vol->segment[cache_idx[0]], (const uint8_t*)vol->key, 1024 /* some amount of the key to use */
						, &crypt, &cryptlen );
					if( !vol->flags.halted )
						sack_fwrite( crypt, 1, vol->sector_size[cache_idx[0]], vol->file );
					Deallocate( uint8_t*, crypt );
				}
				else {
					if( !vol->flags.halted )
						sack_fwrite( vol->usekey_buffer[cache_idx[0]], 1, vol->sector_size[cache_idx[0]], vol->file );
				}
				CLEANCACHE( vol, cache_idx[0] );
				RESETFLAG( vol->_dirty, cache_idx[0] );
#ifdef DEBUG_DISK_IO
				LoG( "Flush dirty sector: %d", cache_idx[0], vol->bufferFPI[cache_idx[0]] );
#endif
			}

			// read new buffer for new segment
			vol->bufferFPI[cache_idx[0]] = vfs_os_compute_block( vol, segment - 1, cache_idx[0] );
			if( vol->bufferFPI[cache_idx[0]] >= vol->dwSize ) _os_ExpandVolume( vol, vol->lastBlock, vol->sector_size[cache_idx[0]] );

			sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_idx[0]]), SEEK_SET);
#ifdef DEBUG_DISK_IO
			LoG( "OS VFS read old sector: fpi:%d %d %d", (int)vol->bufferFPI[cache_idx[0]], cache_idx[0], segment );
#endif
			if( !sack_fread( vol->usekey_buffer[cache_idx[0]], 1, vol->sector_size[cache_idx[0]], vol->file ) ) {
				//lprintf( "Cleared BLock on failed read." );
				memset( vol->usekey_buffer[cache_idx[0]], 0, vol->sector_size[cache_idx[0]] );
				// only need to clear this if it's checked for write data without setting dirty flag.
				memset( vol->usekey_buffer_clean[cache_idx[0]], 0, vol->sector_size[cache_idx[0]] );
			}
			else {
				if( vol->key )
					SRG_XSWS_decryptData( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]]
						, vol->segment[cache_idx[0]], (const uint8_t*)vol->oldkey?vol->oldkey:vol->key, 1024 /* some amount of the key to use */
						, NULL, NULL );
				// after reading what was on the disk (in the sector), save it as the 'clean' state for journaling
				// modifications happen to usekey_buffer before SMUDGE is called.
				memcpy( vol->usekey_buffer_clean[cache_idx[0]], vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]] );
			}
#ifdef DEBUG_DISK_IO
#  ifdef DEBUG_DISK_DATA
		LogBinary( vol->usekey_buffer[cache_idx[0]], vol->sector_size[cache_idx[0]] );
#  endif
#endif
		}
		vol->segment[cache_idx[0]] = segment;
	}
//#ifdef DEBUG_TRACE_LOG
//	if (segment != oldSegs[cache_idx])
//		_lprintf(DBG_RELAY)("UPDATE OS SEGKEY %d %d", cache_idx, segment);
//#endif
	//LoG( "Resulting stored segment in %d", cache_idx );
	//lprintf( "Got Block %d into cache %d", (int)segment, cache_idx );
	return cache_idx[0];
}

#ifdef _MSC_VER
#pragma warning( default: 6001 )
#endif
//---------------------------------------------------------------------------


struct sack_vfs_os_file * _os_createFile( struct sack_vfs_os_volume *vol, BLOCKINDEX first_block, int blockSize )
{
	struct sack_vfs_os_file * file = GetFromSet( VFS_OS_FILE, &l.files );//New( struct sack_vfs_os_file );
	// breaks in C++
	//file[0] = ( struct sack_vfs_os_file ){ 0 };
	MemSet( file, 0, sizeof( struct sack_vfs_os_file ) );
	_os_SetBlockChain( file, 0, first_block, blockSize );
	//lprintf( "Create simple file: %d", first_block );
	file->_first_block = first_block;
	file->block = first_block;
	file->vol = vol;
	file->cache = BC( FILE );
	return file;
}

//---------------------------------------------------------------------------

uintptr_t vfs_os_block_index_SEEK( struct sack_vfs_os_volume* vol, BLOCKINDEX block, int blockSize, enum block_cache_entries* cache_index ) {
	FPI offset;

	while( ( offset = vfs_os_compute_block( vol, block, cache_index[0] ) ) >= vol->dwSize )
		if( !_os_ExpandVolume( vol, vol->lastBlock, blockSize ) ) return 0;
	{
		_os_UpdateSegmentKey( vol, cache_index, block + 1 );
		//LoG( "RETURNING SEEK CACHED %p %d  0x%x   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)offset, (int)seg );
		return ( (uintptr_t)vol->usekey_buffer[cache_index[0]] ) + ( offset % vol->sector_size[cache_index[0]] );
	}
}



// shared with fuse module
// seek by byte position; result with an offset into a block.
// this is used to access BAT information; and should be otherwise avoided.
uintptr_t vfs_os_SEEKX( struct sack_vfs_os_volume* vol, FPI offset, int blockSize, enum block_cache_entries* cache_index ) {
	//lprintf( "This is more complicated with variable size data blocks" );
	// unused function mostly.
	while( offset >= vol->dwSize ) if( !_os_ExpandVolume( vol, vol->lastBlock, blockSize ) ) return 0;

	// this assumes that all blocks are the same size; which is untrue.  This should be removed.
	
	{
		BLOCKINDEX seg = ( offset / BLOCK_SIZE ) + 1;
		_os_UpdateSegmentKey( vol, cache_index, seg );
		//LoG( "RETURNING SEEK CACHED %p %d  0x%x   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)offset, (int)seg );
		return ( (uintptr_t)vol->usekey_buffer[cache_index[0]] ) + ( offset & BLOCK_MASK );
	}
}


// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block after first BAT.
uintptr_t vfs_os_BSEEK_( struct sack_vfs_os_volume* vol, BLOCKINDEX block, int blockSize, enum block_cache_entries* cache_index DBG_PASS ) {
	// b is the absolute block number
	if( block != DIR_ALLOCATING_MARK ) {
		BLOCKINDEX b = ( 1 /* for first BAT */ + ( block / BLOCKS_PER_BAT ) * (BLOCKS_PER_SECTOR)+( block % BLOCKS_PER_BAT ) );
#define _os_segment_to_block( s ) ( ( (s) - 1 ) / BLOCKS_PER_SECTOR ) * BLOCKS_PER_BAT + ( ( (s) - ( 1 + (s) / BLOCKS_PER_SECTOR ) ) % BLOCKS_PER_BAT );
#if defined( DEBUG_BLOCK_COMPUTE )
		{
			//int block_ = ( ( b - 1 ) / BLOCKS_PER_SECTOR ) * BLOCKS_PER_BAT + ( ( b - ( 1 + b / BLOCKS_PER_SECTOR ) ) % BLOCKS_PER_BAT );
			int block_ = _os_segment_to_block( b );
			if( block_ != block ) {
				lprintf( "FAILED" );
				DebugBreak();
			}
		}
#endif
		// first call can skip setting information
		while( vfs_os_compute_block( vol, b, BC( COUNT ) ) >= vol->dwSize ) if( !_os_ExpandVolume( vol, vol->lastBlock, blockSize ) ) return 0;
		{
			_os_UpdateSegmentKey_( vol, cache_index, b + 1 DBG_RELAY );
			//LoG( "RETURNING BSEEK CACHED %p  %d %d %d  0x%x  %d   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)(block/ BLOCKS_PER_BAT), (int)(BLOCKS_PER_BAT-1), (int)b, (int)block, (int)seg );
			return ( (uintptr_t)vol->usekey_buffer[cache_index[0]] )/* + (b&BLOCK_MASK) always 0 */;
		}
	} else {
		BLOCKINDEX b = _os_GetFreeBlock( vol, cache_index, GFB_INIT_NONE, blockSize );
		b = ( 1 /* for first BAT */ + ( b / BLOCKS_PER_BAT ) * (BLOCKS_PER_SECTOR)+( b % BLOCKS_PER_BAT ) );
		_os_UpdateSegmentKey_( vol, cache_index, b + 1 DBG_RELAY );
		// the returned block is set in the ((segment[cache_index[0]]-1)-1)
		return ( (uintptr_t)vol->usekey_buffer[cache_index[0]] );
	}
}

// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block of disk.
uint8_t * vfs_os_DSEEK_( struct sack_vfs_os_volume* vol, FPI dataFPI, int blockSize, enum block_cache_entries* cache_index DBG_PASS ) {
	BLOCKINDEX block;
	struct sack_vfs_os_BAT_info* info;
	INDEX idx;
	INDEX minIndex = 0;
	INDEX maxIndex = vol->pdl_BAT_information->Cnt;
	//DATA_FORALL( vol->pdl_BAT_information, idx, struct sack_vfs_os_BAT_info *, info )
	while( minIndex <= maxIndex )
	{
		idx = ( minIndex + maxIndex ) / 2;
		info = ((struct sack_vfs_os_BAT_info*)vol->pdl_BAT_information->data) + idx;
		if( dataFPI < info->sectorStart ) { maxIndex = idx - 1; continue; }
		if( dataFPI > info->sectorEnd ) { minIndex = idx + 1; continue; }

		{
			dataFPI -= info->sectorStart;
			if( dataFPI < BAT_BLOCK_SIZE )
				block = 0;
			else
				block = 1 + ( dataFPI - BAT_BLOCK_SIZE ) / info->size;
			_os_UpdateSegmentKey( vol, cache_index, info->blockStart + block + 1 );

			return vol->usekey_buffer[cache_index[0]] + ( dataFPI & (vol->sector_size[cache_index[0]] - 1) );
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

static LOGICAL _os_ValidateBAT( struct sack_vfs_os_volume *vol ) {
	//BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );

	BLOCKINDEX n;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX sector = 0;
	{
		struct sack_vfs_os_BAT_info info;
		struct sack_vfs_os_BAT_info *priorInfo;
		FPI thisPos;
		//struct sack_vfs_os_BAT_info* oldinfo;

		vol->pdl_BAT_information->Cnt = 0;
		info.sectorEnd = BAT_BLOCK_SIZE + ( BLOCKS_PER_BAT * 4096 );
		info.sectorStart = 0;
		info.blockStart = n = 0;
		info.size = 4096;

		AddDataItem( &vol->pdl_BAT_information, &info );
		priorInfo = (struct sack_vfs_os_BAT_info*)GetDataItem( &vol->pdl_BAT_information, vol->pdl_BAT_information->Cnt-1 );

		for( n = 0; ( thisPos = info.sectorStart ) < vol->dwSize; n += BLOCKS_PER_SECTOR ) {
			BLOCKINDEX dataBlock = ( n / BLOCKS_PER_SECTOR ) * BLOCKS_PER_BAT;
			size_t m;
			BLOCKINDEX *BAT;
			cache = BC(BAT); // reset cache, so we get a new bat cache block

			// seek loads and updates the segment key...
			BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, n, 0, &cache );

			info.size = priorInfo->size = BAT[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
			{
				priorInfo->sectorEnd = thisPos + BAT_BLOCK_SIZE + ( ( ( BLOCKS_PER_BAT * info.size ) + 4095 ) & ~4095 );

				info.sectorStart = priorInfo->sectorEnd;
				info.blockStart = n + BLOCKS_PER_SECTOR;

				vol->lastBlock = n + BLOCKS_PER_SECTOR;
				AddDataItem( &vol->pdl_BAT_information, &info );
				priorInfo = (struct sack_vfs_os_BAT_info*)GetDataItem( &vol->pdl_BAT_information, vol->pdl_BAT_information->Cnt-1 );
			}
			sector++;

			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[0];
				BAT++;
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					if( info.size == 4096 )
						vol->lastBatBlock = dataBlock + m;
					else
						vol->lastBatSmallBlock = dataBlock + m;
					break;
				}
				//if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					if( info.size == 4096 ) {
						vol->lastBatBlock = dataBlock + m; // use as a temp variable....
						LoG( "Marking free (lg)blank sector %d", vol->lastBatBlock );
						AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
					}
					else {
						vol->lastBatSmallBlock = dataBlock + m; // use as a temp variable....
						LoG( "Marking free (sm)blank sector %d", vol->lastBatBlock );
						AddDataItem( &vol->pdlFreeSmallBlocks, &vol->lastBatSmallBlock );
					}
				}
			}
			//if( m < BLOCKS_PER_BAT ) break;
		}

		// this ends up pusing 1 more so that compute can actually work on reload
		vol->pdl_BAT_information->Cnt--;
		priorInfo = (struct sack_vfs_os_BAT_info*)GetDataItem( &vol->pdl_BAT_information, vol->pdl_BAT_information->Cnt-1 );

		if( priorInfo->sectorEnd > vol->dwSize )
			vol->dwSize = priorInfo->sectorEnd;
	}

	// need to handle rollback before any timeline/directory loading
	// otherwise they will cache sectors that are duplicated here.
	if( !vol->journal.rollback_file ) {
		struct sack_vfs_os_file* file;
		file = _os_createFile( vol, FIRST_ROLLBACK_BLOCK, ROLLBACK_BLOCK_SIZE );
		file->cache = BC( ROLLBACK );

		enum block_cache_entries rollbackCache = BC( ROLLBACK );
		struct vfs_os_rollback_header* rollback = (struct vfs_os_rollback_header*)vfs_os_FSEEK( vol, file, 0, 0, &rollbackCache, ROLLBACK_BLOCK_SIZE DBG_SRC );
		if( !rollback->journal ) {
			enum block_cache_entries firstBlockCache = BC( ROLLBACK );
			rollback->journal = _os_GetFreeBlock( vol, &firstBlockCache, GFB_INIT_NONE, 4096 );
			firstBlockCache = BC( ROLLBACK );
			rollback->small_journal = _os_GetFreeBlock( vol, &firstBlockCache, GFB_INIT_NONE, 256 );
			LoG( "Initial allocate of journal blocks: %d %d", rollback->journal, rollback->small_journal );
		} else {
			LoG( "Reload allocate of journal blocks: %d %d", rollback->journal, rollback->small_journal );
		}
		vol->journal.rollback_journal_file = _os_createFile( vol, rollback->journal, BLOCK_SIZE );
		vol->journal.rollback_journal_file->cache = BC( ROLLBACK );
		vol->journal.rollback_small_journal_file = _os_createFile( vol, rollback->small_journal, BLOCK_SMALL_SIZE );
		vol->journal.rollback_small_journal_file->cache = BC( ROLLBACK );
		vol->journal.rollback_file = file;

		vol->journal.pdlPendingRecord = CreateDataList( sizeof( enum block_cache_entries ) );
		vol->journal.pdlJournaled = CreateDataList( sizeof( BLOCKINDEX ) );

		if( !vol->flags.skipRollbackProcessing )
			vfs_os_process_rollback( vol );
		{
			INDEX idx;
			enum block_cache_entries block;
			LIST_FORALL( vol->pending_rollback, idx, enum block_cache_entries, block ) {
				vfs_os_record_rollback( vol, block );
			}
			DeleteList( &vol->pending_rollback );
			sack_vfs_os_polish_volume( vol );
		}
	}


	vol->timeline_file = _os_createFile( vol, FIRST_TIMELINE_BLOCK, TIME_BLOCK_SIZE );
	vol->timeline_file->cache = BC( TIMELINE );

	{
		int locks;
		vol->timelineCache = BC( TIMELINE );
		vol->timeline = (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, TIME_BLOCK_SIZE, &vol->timelineCache );

		SETMASK_( vol->seglock, seglock, vol->timelineCache, locks = GETMASK_( vol->seglock, seglock, vol->timelineCache )+1 );
		if( locks > 5 ) {
			lprintf( "Lock is in danger of overflow" );
		}
	}

	if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, NULL, NULL, 0 ) ) return FALSE;
	//lprintf( "total files:%d", l.fileCount );

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
LOGICAL _os_ExpandVolume( struct sack_vfs_os_volume *vol, BLOCKINDEX fromBlock, int size ) {
	LOGICAL created = FALSE;
	LOGICAL path_checked = FALSE;
	int n;
	LoG( "Expand Volume: %d %d", fromBlock, size ); 
	size_t oldsize = vol->dwSize;
	if( vol->file && vol->read_only ) return TRUE;
	if( !size ) return FALSE;
	if( !vol->file ) {
		char *fname;
		char *iface;
		char *tmp;
#ifndef USE_STDIO

		if( tmp =(char*)StrChr( vol->volname, '@' ) ) {
			if( tmp[1] == '@' ) {
				strcpy( tmp, tmp + 1 );
				goto defaultOpen;
			} else {
				tmp[0] = 0;
				iface = (char*)vol->volname;
				fname = tmp + 1;
				struct file_system_mounted_interface* mount = sack_get_mounted_filesystem( iface );
				//struct file_system_interface *iface = sack_get_filesystem_interface( iface );
				if( !sack_exists( fname ) ) {
					vol->file = sack_fopenEx( 0, fname, "rb+", mount );
					if( !vol->file )
						vol->file = sack_fopenEx( 0, fname, "wb+", mount );
					created = TRUE;
				} else
					vol->file = sack_fopenEx( 0, fname, "rb+", mount );
				tmp[0] = '@';
			}
		}
		else {
defaultOpen:
			{
				char* tmp = StrDup( vol->volname );
				char* dir = (char*)pathrchr( tmp );
				if( dir ) {
					dir[0] = 0;
					if( !IsPath( tmp ) ) MakePath( tmp );
				}
				free( tmp );
				//Deallocate( char*, tmp );
			}
			vol->file = sack_fopenEx( 0, vol->volname, "rb+", vol->mount );
			if( !vol->file ) {
				created = TRUE;
				vol->file = sack_fopenEx( 0, vol->volname, "wb+", vol->mount );
			}
		}
#else
		vol->file = fopen( 0, vol->volname, "rb+" );
		if( !vol->file ) {
			created = TRUE;
			vol->file = fopen( 0, vol->volname, "wb+" );
		}
#endif
		if( !vol->file ) {
			//lprintf( "Failed to open volume" );
			return FALSE;
		}
		sack_fseek( vol->file, 0, SEEK_END );
		vol->dwSize = sack_ftell( vol->file );
		if( vol->dwSize == 0 )
			created = TRUE;
		sack_fseek( vol->file, 0, SEEK_SET );
	}

	{
		struct sack_vfs_os_BAT_info info;
		struct sack_vfs_os_BAT_info* pinfo;
		if( vol->pdl_BAT_information->Cnt )
			pinfo = ( struct sack_vfs_os_BAT_info* )GetDataItem( &vol->pdl_BAT_information, vol->pdl_BAT_information->Cnt - 1 );
		else
			pinfo = NULL;

		info.sectorEnd = ( pinfo ? pinfo->sectorEnd : 0 ) + ( BAT_BLOCK_SIZE + ( ( ( BLOCKS_PER_BAT * size ) + 4095 ) & ( ~4095 ) ) );
		info.sectorStart = ( pinfo ? pinfo->sectorEnd : 0 );
		info.blockStart = ( pinfo ? ( pinfo->blockStart + BLOCKS_PER_SECTOR ) : 0 );
		info.size = size;
		AddDataItem( &vol->pdl_BAT_information, &info );
		/*
		{
			INDEX idx;
			struct sack_vfs_os_BAT_info *info;
			// dump reloaded bat information (or bat so far...)
			DATA_FORALL( vol->pdl_BAT_information, idx, struct sack_vfs_os_BAT_info *, info ) {
				lprintf( "BAT Updated expanded: %d %d %d %d", info->blockStart, info->sectorStart, info->sectorEnd, info->size );
			}
		}
		*/
		// a BAT plus the sectors it references... ( BLOCKS_PER_BAT + 1 ) * BLOCK_SIZE
		if( info.sectorEnd > vol->dwSize )
			vol->dwSize = info.sectorEnd;
	}


	LoG( "created expanded volume: %p from %p size:%" _size_f, vol->file, BLOCKS_PER_SECTOR*size, vol->dwSize );
	vol->lastBlock += BLOCKS_PER_SECTOR;

	// can't recover dirents and nameents dynamically; so just assume
	// use the _os_GetFreeBlock because it will update encypted
	//vol->disk->BAT[0] = EOFBLOCK;  // allocate 1 directory entry block
	//vol->disk->BAT[1] = EOFBLOCK;  // allocate 1 name block
	n = 0;

	if( created || ( (n=1),size == BLOCK_SMALL_SIZE && oldsize == ( BLOCK_SIZE * BLOCKS_PER_SECTOR ) ) ) {

		enum block_cache_entries cache = BC(BAT);
		_os_UpdateSegmentKey( vol, &cache, n*BLOCKS_PER_SECTOR + 1 );
		((BLOCKINDEX*)vol->usekey_buffer[cache])[0] = EOBBLOCK;
		((BLOCKINDEX*)vol->usekey_buffer[cache])[BLOCKS_PER_BAT] = (size== BLOCK_SMALL_SIZE )?1:(size==4096)?0:2;

		if( created ) {


			enum block_cache_entries dirCache = BC( DIRECTORY );
			enum block_cache_entries timeCache = BC( TIMELINE );
			enum block_cache_entries rollbackCache = BC( ROLLBACK );

			BLOCKINDEX dirblock = _os_GetFreeBlock( vol, &dirCache, GFB_INIT_DIRENT, DIR_BLOCK_SIZE );
			BLOCKINDEX timeblock = _os_GetFreeBlock( vol, &timeCache, GFB_INIT_TIMELINE, TIME_BLOCK_SIZE );
			BLOCKINDEX rollbackblock = _os_GetFreeBlock( vol, &rollbackCache, GFB_INIT_ROLLBACK, ROLLBACK_BLOCK_SIZE );

			vol->lastBatBlock = 0;
		}
		else {
			//vol->lastBatSmallBlock = BLOCKS_PER_BAT;
		}

		SMUDGECACHE( vol, cache );
		sack_vfs_os_flush_block( vol, cache );
		vol->bufferFPI[cache] = oldsize;

	}

	return TRUE;
}



static BLOCKINDEX _os_GetFreeBlock_( struct sack_vfs_os_volume *vol, enum block_cache_entries *blockCache, enum getFreeBlockInit init, int blockSize, LOGICAL flush_BAT_caches DBG_PASS )
{
	size_t n;
	unsigned int b = 0;
	enum block_cache_entries cache = BC( BAT );
	BLOCKINDEX *current_BAT;
	BLOCKINDEX check_val;

	if( blockSize == 4096 ) {
		if( vol->pdlFreeBlocks->Cnt ) {
			BLOCKINDEX newblock = ((BLOCKINDEX*)GetDataItem( &vol->pdlFreeBlocks, vol->pdlFreeBlocks->Cnt - 1 ))[0];
			check_val = 0;
			b = (unsigned int)(newblock / BLOCKS_PER_BAT);
			n = newblock % BLOCKS_PER_BAT;
			vol->pdlFreeBlocks->Cnt--;
		}
		else {
			check_val = EOBBLOCK;
			b = (unsigned int)(vol->lastBatBlock / BLOCKS_PER_BAT);
			n = vol->lastBatBlock % BLOCKS_PER_BAT;
		}
	}
	else {
		if( vol->pdlFreeSmallBlocks->Cnt ) {
			BLOCKINDEX newblock = ( (BLOCKINDEX*)GetDataItem( &vol->pdlFreeSmallBlocks, vol->pdlFreeSmallBlocks->Cnt - 1 ) )[0];
			check_val = 0;
			n = newblock % BLOCKS_PER_BAT;
			b = (unsigned int)( newblock / BLOCKS_PER_BAT );
			vol->pdlFreeSmallBlocks->Cnt--;
		}
		else {
			check_val = EOBBLOCK;
			n = vol->lastBatSmallBlock % BLOCKS_PER_BAT;
			b = (unsigned int)( vol->lastBatSmallBlock / BLOCKS_PER_BAT );
		}
	}
#ifdef DEBUG_SET_SECTOR_SIZE
	LoG_( "GetFreeBlock is using... check, start, b, n %d %d %d %d", (int)check_val, (int) vol->lastBatBlock, (int)b, (int)n );
#endif
//	vfs_os_compute_block( vol, b * BLOCKS_PER_SECTOR, cache );
	current_BAT =  (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, b*BLOCKS_PER_SECTOR, blockSize, &cache ) + n;
	if( !current_BAT ) return 0;

	current_BAT[0] = EOFBLOCK;
	if( (check_val == EOBBLOCK) ) {
		if( n < (BLOCKS_PER_BAT - 1) ) {
			current_BAT[1] = EOBBLOCK;
			if( blockSize == 4096 )
				vol->lastBatBlock++;
			else
				vol->lastBatSmallBlock++;
		}
		else {
			BLOCKINDEX lastB = ( ( vol->lastBatSmallBlock > vol->lastBatBlock ) ? vol->lastBatSmallBlock : vol->lastBatBlock ) / BLOCKS_PER_BAT;
			enum block_cache_entries cache = BC( BAT );

			//_os_ExpandVolume( vol, lastB, blockSize );
			current_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, ( lastB + 1 ) * ( BLOCKS_PER_SECTOR ), blockSize, &cache );
			current_BAT[BLOCKS_PER_BAT] = ( blockSize == BLOCK_SMALL_SIZE ) ? 1 : ( blockSize == 4096 ) ? 0 : 2;
			//lprintf( "Initialized bat at block %d to %d", lastB + 1, current_BAT[BLOCKS_PER_BAT] );
			current_BAT[0] = EOBBLOCK;
			// update the clean buffer, so journal writes initialized data.
			//memcpy( vol->usekey_buffer_clean[cache], vol->usekey_buffer[cache], DIR_BLOCK_SIZE );

			if( blockSize == 4096 )
				vol->lastBatBlock = ( lastB + 1) * BLOCKS_PER_BAT;
			else
				vol->lastBatSmallBlock = ( lastB + 1 ) * BLOCKS_PER_BAT;

			if( flush_BAT_caches ) sack_vfs_os_flush_block( vol, cache );
			else SMUDGECACHE( vol, cache ); // make sure this gets saved... 
			//lprintf( "Set last block....%d", (int)vol->lastBatBlock );
		}
	}
	if( flush_BAT_caches ) sack_vfs_os_flush_block( vol, cache );
	else SMUDGECACHE( vol, cache );

	switch( init ) {
	case GFB_INIT_DIRENT: {
			struct directory_hash_lookup_block *dir;
#ifdef DEBUG_BLOCK_INIT
			LoG( "Create new directory: result %d", (int)(b * BLOCKS_PER_BAT + n) );
#endif
			blockCache[0] = BC( DIRECTORY );
			_os_UpdateSegmentKey_( vol, blockCache, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			memset( vol->usekey_buffer[blockCache[0]], 0, DIR_BLOCK_SIZE );

			dir = (struct directory_hash_lookup_block *)vol->usekey_buffer[blockCache[0]];
			enum block_cache_entries newcache2 = BC( NAMES );
			dir->names_first_block = _os_GetFreeBlock( vol, &newcache2, GFB_INIT_NAMES, NAME_BLOCK_SIZE );
			dir->used_names = 0;
			// update the clean buffer, so journal writes initialized data.
			//memcpy( vol->usekey_buffer_clean[newcache], vol->usekey_buffer[newcache2], DIR_BLOCK_SIZE );
			break;
		}
	case GFB_INIT_TIMELINE: {
			struct storageTimeline *tl;
#ifdef DEBUG_BLOCK_INIT
			LoG( "new block, init as root timeline" );
#endif
			_os_UpdateSegmentKey_( vol, blockCache, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			tl = (struct storageTimeline *)vol->usekey_buffer[blockCache[0]];
			//tl->header.timeline_length  = 0;
			//tl->header.crootNode.raw = 0;
			tl->header.srootNode.raw = 0;
			tl->header.first_free_entry.ref.index = 1;
			//tl->header.first_free_entry.ref.depth = 0;
			// update the clean buffer, so journal writes initialized data.
			//memcpy( vol->usekey_buffer_clean[blockCache[0]], vol->usekey_buffer[blockCache[0]], TIME_BLOCK_SIZE );
			break;
		}
	case GFB_INIT_TIMELINE_MORE:
#ifdef DEBUG_BLOCK_INIT
		LoG( "new block, init timeline more " );
#endif
		_os_UpdateSegmentKey_( vol, blockCache, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
		memset( vol->usekey_buffer[blockCache[0]], 0, vol->sector_size[blockCache[0]] );
		// update the clean buffer, so journal writes initialized data.
		//memcpy( vol->usekey_buffer_clean[blockCache[0]],  vol->usekey_buffer[blockCache[0]], TIME_BLOCK_SIZE );
		break;
	case GFB_INIT_NAMES:
#ifdef DEBUG_BLOCK_INIT
		LoG( "new block, init names" );
#endif
		_os_UpdateSegmentKey_( vol, blockCache, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
		memset( vol->usekey_buffer[blockCache[0]], 0, vol->sector_size[blockCache[0]] );
		((char*)(vol->usekey_buffer[blockCache[0]]))[0] = (char)UTF8_EOTB;
		// update the clean buffer, so journal writes initialized data.
		//memcpy( vol->usekey_buffer_clean[blockCache[0]], vol->usekey_buffer[blockCache[0]], DIR_BLOCK_SIZE );
		//LoG( "New Name Buffer: %x %p", vol->segment[blockCache[0]], vol->usekey_buffer[blockCache[0]] );
		break;
		
	default:
#ifdef DEBUG_BLOCK_INIT
		LoG( "Default or NO init..." );
#endif
		_os_UpdateSegmentKey_( vol, blockCache, b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
		break;
	}
	
	SMUDGECACHE( vol, blockCache[0] );
#ifdef DEBUG_ROLLBACK_JOURNAL
	LoG( "(post smudge)Return Free block:%d   %d  %d", (int)(b*BLOCKS_PER_BAT + n), (int)b, (int)n );
#endif
	//lprintf( "return block allocated: %d %d %d", (int)(b* BLOCKS_PER_BAT + n), (int)b, n );
	return b * BLOCKS_PER_BAT + n;
}

static BLOCKINDEX vfs_os_GetNextBlock_v2( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum block_cache_entries* blockCache, enum getFreeBlockInit init, LOGICAL expand, int blockSize, int *realBlockSize, LOGICAL flush_BAT_caches ) {
	BLOCKINDEX sector = block / BLOCKS_PER_BAT;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *this_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, sector * (BLOCKS_PER_SECTOR), blockSize, &cache );
	int thisSize = this_BAT[BLOCKS_PER_BAT]? BLOCK_SMALL_SIZE :4096;
	BLOCKINDEX check_val;
	if( !this_BAT ) return 0; // if this passes, later ones will also.
#ifdef _DEBUG
	if( !block ) DebugBreak();
#endif

	check_val = (this_BAT[block % BLOCKS_PER_BAT]);
#ifdef _DEBUG
	if( !check_val ) {
		lprintf( "STOP: %p  %d  %d  %d", this_BAT, (int)check_val, (int)(block), (int)sector );
		DebugBreak();
	}
#endif
	if( check_val == EOBBLOCK ) {
		(this_BAT[block % (BLOCKS_PER_BAT)]) = EOFBLOCK;
		if( block < (BLOCKS_PER_BAT - 1) )
			(this_BAT[(1 + block) % BLOCKS_PER_BAT]) = EOBBLOCK;
		//else
		//	lprintf( "THIS NEEDS A NEW BAT BLOCK TO MOVE THE MARKER" );//
	}
	if( check_val == EOFBLOCK || check_val == EOBBLOCK ) {
		if( expand ) {
			SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )+1 );

			check_val = _os_GetFreeBlock_( vol, blockCache, init, blockSize, flush_BAT_caches DBG_SRC );
#ifdef _DEBUG
			if( !check_val )DebugBreak();
#endif
			{
				// free block might have expanded... get the size that was actually allocated
				BLOCKINDEX nextSector = check_val / BLOCKS_PER_BAT;
				if( sector != nextSector ) {
					enum block_cache_entries cache = BC(BAT);
					BLOCKINDEX* this_BAT2 = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, nextSector * ( BLOCKS_PER_SECTOR ), BAT_BLOCK_SIZE, &cache );
					if( !this_BAT2 ) {
						lprintf( "failed to load next bat to get size" );
						return 0;
					}
					thisSize = this_BAT2[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
				} else
					thisSize = this_BAT[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
			}
			// segment could already be set from the _os_GetFreeBlock...
			//lprintf( "set block %d %d %d to %d", (int)block, (int)( block % BLOCKS_PER_BAT ), (int)sector, (int)check_val );
			this_BAT[block % BLOCKS_PER_BAT] = check_val;
			//lprintf( "Set %d  %d %d to %d", block, sector, block % BLOCKS_PER_BAT, check_val );
			if( flush_BAT_caches ) sack_vfs_os_flush_block( vol, cache );
			else SMUDGECACHE( vol, cache );

			SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache ) - 1 );
		}
	} else {
		enum block_cache_entries cache = BC(BAT);
		BLOCKINDEX nextSector = check_val / BLOCKS_PER_BAT;
		if( nextSector != sector ) {
			BLOCKINDEX* this_BAT2 = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, nextSector * ( BLOCKS_PER_SECTOR ), BAT_BLOCK_SIZE, &cache );
			if( !this_BAT2 ) {
				lprintf( "failed to load next bat to get size" );
				return 0;
			}
			thisSize = this_BAT2[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
		} else
			thisSize = this_BAT[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
	}
#ifdef _DEBUG
	if( !check_val )DebugBreak();
#endif
	if( realBlockSize ) realBlockSize[0] = thisSize;
	//LoG( "return next block:%d %d", (int)block, (int)check_val );
	return check_val;
}

static BLOCKINDEX vfs_os_GetNextBlock( struct sack_vfs_os_volume* vol, BLOCKINDEX block, enum block_cache_entries* blockCache, enum getFreeBlockInit init, LOGICAL expand, int blockSize, int* realBlockSize ) {
	return vfs_os_GetNextBlock_v2( vol, block, blockCache, init, expand, blockSize, realBlockSize, FALSE );
}


static void _os_AddSalt( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psv;
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
	else if( vol->curseg < BC(COUNT) && vol->segment[vol->curseg] != ~0 ) {
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
			( *salt_size ) = 12;
			( *salt ) = vol->tmpSalt;
			break;
		}
	}
	else
		(*salt_size) = 0;
}

static void _os_AssignKey( struct sack_vfs_os_volume *vol, const char *key1, const char *key2 )
{
	// *2 is to duplicate all buffers so there's a backing clean-copy for rollback

	uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * ( BC(COUNT) * 2 )
		+ BLOCK_SIZE + SHORTKEY_LENGTH;
	if( !vol->key_buffer ) {
		int n;
		// internal buffers to read and decode into
		vol->key_buffer = (uint8_t*)HeapAllocateAligned( NULL, size, 4096 );// NewArray( uint8_t, size );
		memset( vol->key_buffer, 0, size );
		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey_buffer[n] = vol->key_buffer + (n + 1) * BLOCK_SIZE;
			vol->usekey_buffer_clean[n] = vol->key_buffer + ( n + 1 + BC(COUNT) ) * BLOCK_SIZE ;
		}
		for( n = 0; n < BC( COUNT ); n++ ) {
			vol->segment[n] = ~0; // if not dirty, ~0 wont' be written but ages don't have to change.
			CLEANCACHE( vol, n );
			RESETFLAG( vol->_dirty, n );
		}
	}

	vol->userkey = key1;
	vol->devkey = key2;
	if( key1 || key2 )
	{
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		else
			SRG_ResetEntropy( vol->entropy );

		if( vol->oldkey ) Release( vol->oldkey );
		vol->oldkey = vol->key;
		vol->key = (uint8_t*)HeapAllocateAligned( NULL, 1024, 4096 ); //NewArray( uint8_t, size );

		vol->curseg = BC(COUNT);
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, 1024 * 8 );
	}
	else {
		if( vol->oldkey ) Release( vol->oldkey );
		vol->oldkey = vol->key;
		vol->key = NULL;
	}
}

static void sack_vfs_os_flush_block( struct sack_vfs_os_volume* vol, enum block_cache_entries entry ) {
	INDEX idx = entry;
#ifdef DEBUG_DISK_IO
	LoG( "Flush dirty segment: %d  fpi:%zx %d", (int)idx, vol->bufferFPI[idx], vol->segment[idx] );
#  ifdef DEBUG_DISK_DATA
	LogBinary( vol->usekey_buffer[idx], vol->sector_size[idx] );
#  endif
#endif
	sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
	if( vol->key ) {
		uint8_t* crypt;
		size_t cryptlen;
		SRG_XSWS_encryptData( vol->usekey_buffer[idx], vol->sector_size[idx]
			, vol->segment[idx], (const uint8_t*)vol->key, 1024
			, &crypt, &cryptlen );
		if( !vol->flags.halted )
			sack_fwrite( crypt, 1, vol->sector_size[idx], vol->file );
		Deallocate( uint8_t*, crypt );
	} else {
		if( !vol->flags.halted )
			sack_fwrite( vol->usekey_buffer[idx], vol->sector_size[idx], 1, vol->file );
	}
	/*
	if( !GETMASK_( vol->seglock, seglock, idx ) )
		// don't HAVE To release that this segment is in this cache block...
		// it's just claimable, and not dirty.
		// vol->segment[idx] = ~0;
		;
	*/
	memcpy( vol->usekey_buffer_clean[idx], vol->usekey_buffer[idx], vol->sector_size[idx] );
	//lprintf( "Updated clean buffer %d", idx );
	CLEANCACHE( vol, idx );
	RESETFLAG( vol->_dirty, idx );
	//RESETFLAG( vol->dirty, idx );  // CLEANCACHE is this op...
}


void sack_vfs_os_flush_volume( struct sack_vfs_os_volume * vol, LOGICAL unload ) {
	{
		INDEX idx;
		while( LockedExchange( &vol->lock, 1 ) ) Relinquish();

		for( idx = 0; idx < BC( ROLLBACK ); idx++ ) {
			if( unload ) {
				SETMASK_( vol->seglock, seglock, idx, 0 );  // reset any locks. (will fail any open files)
				//RESETFLAG( vol->seglock, idx );
			}
			if( TESTFLAG( vol->dirty, idx ) || TESTFLAG( vol->_dirty, idx ) ) {
				sack_vfs_os_flush_block( vol, ( enum block_cache_entries )idx );
			} else {
#ifdef DEBUG_CACHE_FLUSH
				if( memcmp( vol->usekey_buffer_clean[idx], vol->usekey_buffer[idx], BLOCK_SIZE ) ) {
					lprintf( "Block was written to, but was not flagged as dirty, changes will be lost." );
					DebugBreak();
				}
#endif
			}
		}
		vfs_os_empty_rollback( vol );

	}
	vol->lock = 0;
}

static uintptr_t volume_flusher( PTHREAD thread ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)GetThreadParam( thread );
	while( 1 ) {

		vol->flushing = 1;
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
						CLEANCACHE( vol, idx );
					}

				if( updated ) {
					vol->lock = 0; // data changed, don't flush.
					WakeableSleep( 256 );
					if( l.exited )break;
				}
				else
					break; // have lock, break; no new changes; flush dirty sectors(if any)
			}
			else { // didn't get lock, wait.
				WakeableSleep( 10 );
				if( l.exited )break;
				continue;
			}
		}
		if( l.exited )break;

		{
			INDEX idx;
			for( idx = 0; idx < BC( ROLLBACK ); idx++ )
				if( TESTFLAG( vol->_dirty, idx )  // last pass marked this dirty
					&& !TESTFLAG( vol->dirty, idx ) ) { // hasn't been re-marked as dirty, so it's been idle...
					sack_vfs_os_flush_block( vol, (enum block_cache_entries)idx );
				}
				else {
#ifdef DEBUG_CACHE_FLUSH
					if( !TESTFLAG( vol->_dirty, idx )
						// hasn't been re-marked as dirty, so it's been idle...
						&& !TESTFLAG( vol->dirty, idx ) )
						if( memcmp( vol->usekey_buffer_clean[idx], vol->usekey_buffer[idx], BLOCK_SIZE ) ) {
							lprintf( "Block was written to, but was not flagged as dirty, changes will be lost." );
							//DebugBreak();
						}
#endif
				}
#ifdef DEBUG_DIRECTORIES
			_os_dumpDirectories( vol, 0, 1 );
#endif
			vfs_os_empty_rollback(vol);
		}
		vol->flushing = 0;
		vol->lock = 0;
		// for all dirty
		WakeableSleep( SLEEP_FOREVER );
		if( l.exited )break;

	}
	return 0;
}

void sack_vfs_os_polish_volume( struct sack_vfs_os_volume* vol ) {
	static PTHREAD flusher;
	if( !vol->flusher )
		vol->flusher = ThreadTo( volume_flusher, (uintptr_t)vol );
	else if( !vol->flushing )
		WakeThread( vol->flusher );
}

void sack_vfs_os_unload_volume( struct sack_vfs_os_volume* vol );

struct sack_vfs_os_volume *sack_vfs_os_load_volume( const char * filepath, struct file_system_mounted_interface*mount )
{
	struct sack_vfs_os_volume *vol = New( struct sack_vfs_os_volume );
	memset( vol, 0, sizeof( struct sack_vfs_os_volume ) );
	if( !mount )
		mount = sack_get_default_mount();
	vol->mount = mount;
	// since time is morely forward going; keeping the stack for the avl
	// balancer can reduce forward-scanning insertion time
	// vol->pdsCTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );
	// vol->pdsWTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );

	vol->pdl_BAT_information = CreateDataList( sizeof( struct sack_vfs_os_BAT_info ) );
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->pdlFreeSmallBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->volname = StrDup( filepath );
#ifdef DEBUG_DELETE_LAST
	vol->pvtDeleteBuffer = VarTextCreate();
#endif
	_os_AssignKey( vol, NULL, NULL );
	if( !_os_ExpandVolume( vol, 0, 4096 )
	  || !_os_ExpandVolume(vol, BLOCKS_PER_SECTOR, BLOCK_SMALL_SIZE )
	  || !_os_ValidateBAT( vol ) )
	{
		sack_vfs_os_unload_volume( vol );
		return NULL;
	}
#ifdef DEBUG_DIRECTORIES
	_os_dumpDirectories( vol, 0, 1 );
#endif
	AddLink( &l.volumes, vol );
	return vol;
}

struct sack_vfs_os_volume* sack_vfs_os_load_volume_v2( int flags, CTEXTSTR filepath, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey, struct file_system_mounted_interface* mount ) 	{
	struct sack_vfs_os_volume *vol = New( struct sack_vfs_os_volume );
	MemSet( vol, 0, sizeof( struct sack_vfs_os_volume ) );
	if( !mount )
		mount = sack_get_default_mount();
	vol->mount = mount;
	if( !version ) version = 2;
	if( flags & 1 ) vol->flags.skipRollbackProcessing = TRUE;
	if( flags & 2 ) vol->flags.halted = TRUE;
	vol->pdl_BAT_information = CreateDataList( sizeof( struct sack_vfs_os_BAT_info ) );
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->pdlFreeSmallBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->clusterKeyVersion = version - 1;
	vol->volname = StrDup( filepath );
	vol->pvtDeleteBuffer = VarTextCreate();
	_os_AssignKey( vol, userkey, devkey );
	if( !_os_ExpandVolume( vol, 0, 4096 ) || !_os_ExpandVolume( vol, BLOCKS_PER_SECTOR, BLOCK_SMALL_SIZE ) || !_os_ValidateBAT( vol ) ) { sack_vfs_os_unload_volume( vol ); return NULL; }
#ifdef DEBUG_DIRECTORIES
	_os_dumpDirectories( vol, 0, 1 );
#endif
	AddLink( &l.volumes, vol );
	return vol;
}

struct sack_vfs_os_volume* sack_vfs_os_load_crypt_volume( const char* filepath, uintptr_t version, const char* userkey, const char* devkey, struct file_system_mounted_interface* mount ) {
	return sack_vfs_os_load_volume_v2( 0, filepath, version, userkey, devkey, mount );
}

void sack_vfs_os_unload_volume( struct sack_vfs_os_volume * vol ) {
	INDEX idx;
	struct sack_vfs_file *file;
#ifdef DEBUG_DIRECTORIES
	_os_dumpDirectories( vol, 0, 1 );
#endif
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		break;
	if( file ) {
		vol->closed = TRUE;
		return;
	}
	DeleteLink( &l.volumes, vol );
	if( vol->file )
		sack_vfs_os_flush_volume( vol, TRUE );
	free( (char*)vol->volname );
	DeleteListEx( &vol->files DBG_SRC );
	sack_fclose( vol->file );
	DeleteDataList( &vol->pdl_BAT_information );
	DeleteDataList( &vol->pdlFreeBlocks );
	DeleteDataList( &vol->pdlFreeSmallBlocks );
	//if( !vol->external_memory )	CloseSpace( vol->diskReal );
	if( vol->key ) {
		Deallocate( uint8_t*, vol->key );
		SRG_DestroyEntropy( &vol->entropy );
	}
	Deallocate( uint8_t*, vol->key_buffer );
	Deallocate( struct sack_vfs_os_volume*, vol );
}

void sack_vfs_os_shrink_volume( struct sack_vfs_os_volume * vol ) {
	size_t n;
	unsigned int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	unsigned int last_bat = 0;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *current_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, 0, 0, &cache );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		for( n = 0; n < BLOCKS_PER_BAT; n++ ) {
			check_val = *(current_BAT++);
			if( check_val ) {
				last_bat = b;
				last_block = n;
			}
		}
		b++;
		if( vfs_os_compute_block( vol, b * BLOCKS_PER_SECTOR, BC(COUNT) ) < vol->dwSize ) {
			current_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK(  vol, b * ( BLOCKS_PER_SECTOR), 0, &cache );
		} else
			break;
	}while( 1 );
	sack_fclose( vol->file );
	vol->file = NULL;

	// setting 0 size will cause expand to do an initial open instead of expanding
	vol->dwSize = 0;
}

LOGICAL sack_vfs_os_decrypt_volume( struct sack_vfs_os_volume *vol )
{
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( !vol->key ) { vol->lock = 0; return FALSE; } // volume is already decrypted, cannot remove key
	_os_AssignKey( vol, NULL, NULL );
	{
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries cache2 = BC(BAT);
		size_t n;
		for( n = 0; vfs_os_compute_block( vol, n, BC(COUNT) ) < vol->dwSize; n++ ) {
			size_t m;
			BLOCKINDEX *block = (BLOCKINDEX*)vfs_os_block_index_SEEK(  vol, n, 0, &cache );

			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				if( block[0] == EOBBLOCK ) break;
				else if( block[0] ) vfs_os_block_index_SEEK( vol, n + m, 0, &cache2 ); // load the block using oldkey, flush will use new key
				block++;
			}
			if( m < BLOCKS_PER_BAT ) break;
			n += m;
		}
	}
	return TRUE;
}

LOGICAL sack_vfs_os_encrypt_volume( struct sack_vfs_os_volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 ) {
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	//if( vol->key ) { vol->lock = 0; return FALSE; } // volume already has a key, cannot apply new key
	if( !version ) version = 2;
	vol->clusterKeyVersion = version-1; // how key gets computed.
	_os_AssignKey( vol, key1, key2 );
	{
		size_t n;
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries cache2 = BC(BAT);
		for( n = 0; vfs_os_compute_block( vol, n, BC(COUNT) ) < vol->dwSize; n++ ) {
			size_t m;
			BLOCKINDEX *block = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, n, 0, &cache );

			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				if( block[0] == EOBBLOCK ) break;
				else if( block[0] ) vfs_os_block_index_SEEK( vol, n + m, 0, &cache2 ); // load the block using oldkey, flush will use new key
				block++;
			}
			if( m < BLOCKS_PER_BAT ) break;
			n += m;
		}
	}
	vol->lock = 0;
	return TRUE;
}

const char *sack_vfs_os_get_signature( struct sack_vfs_os_volume *vol ) {
	static char signature[257];
	static const char *output = "0123456789ABCDEF";
	if( !vol )
		return NULL;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	{
		static BLOCKINDEX datakey[BLOCKS_PER_BAT];
		uint8_t usekey[128];
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
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
					for( n = 0; n < ( DIR_BLOCK_SIZE / BLOCKS_PER_BAT ); n++ )
						datakey[n] ^= next_entries[n];

					next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, &cache, GFB_INIT_DIRENT, FALSE, 4096, NULL );
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
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)usekey, sizeof(usekey) * 8 );
		{
			int n;
			for( n = 0; n < sizeof(usekey); n++ ) {
				signature[n*2] = output[( usekey[n] >> 4 ) & 0xF];
				signature[n*2+1] = output[usekey[n] & 0xF];
			}
		}
	}
	vol->lock = 0;
	return signature;
}

LOGICAL checkFileLength( struct sack_vfs_os_volume *vol
	, BLOCKINDEX firstBlock
	, FPI expectedLength ) {
	BLOCKINDEX curBlock = firstBlock;
	size_t len;
	enum block_cache_entries cache;
	BLOCKINDEX sector = firstBlock / BLOCKS_PER_BAT;
	enum block_cache_entries batcache = BC(BAT);
	BLOCKINDEX *this_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, sector * (BLOCKS_PER_SECTOR), BLOCK_SIZE, &batcache );
	len = this_BAT[BLOCKS_PER_BAT] ? BLOCK_SMALL_SIZE : 4096;
	while( curBlock != EOFBLOCK ) {
		int blockSize;
		cache = BC(FILE);

		curBlock = vfs_os_GetNextBlock( vol, curBlock, &cache, GFB_INIT_NONE, FALSE, 0, &blockSize );
		if( curBlock != EOFBLOCK )
			len += blockSize;
	}
	if( len < expectedLength ) {
		lprintf( "Short file chain: %d %d", (int)len, (int)expectedLength );
		return FALSE;
	}
	//else lprintf( "Success: %d %d %d",  (int) firstBlock, (int)len, (int)expectedLength );
	return TRUE;
	
	
}

//-----------------------------------------------------------------------------------
// Director Support Functions
//-----------------------------------------------------------------------------------

LOGICAL _os_ScanDirectory_( struct sack_vfs_os_volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_os_file *file
	, int path_match
	, char *leadin
	, int *leadinDepth
) {
	int ofs = 0;
	BLOCKINDEX this_dir_block = dirBlockSeg;
	int usedNames;
	int minName;
	int curName;
	struct directory_hash_lookup_block *dirblock;
	struct directory_entry *next_entries;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	if( !file && !filename && nameBlockStart )
		lprintf( "Begin a scan dir:%d", (int)dirBlockSeg );
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		BLOCKINDEX nameBlock;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, DIR_BLOCK_SIZE, cache );

		SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )+1 );

		if( !dirblock->next_block[255] ) {
#ifdef DEBUG_CONVERT_DIRECTORY // symbol not defined
			l.fileCount = 0;
#endif
		}
		else{
			if( !file && !filename && nameBlockStart )
				 lprintf( "parent block(may be truncated): %d(%d) %d p:%d c:%d c:%c", (int)this_dir_block, cache, (int)dirblock->used_names
							, (int)(dirblock->next_block[255] >> 8)
							, (int)(dirblock->next_block[255] & 0xFF), (char)(dirblock->next_block[255] & 0xFF) );
		}
		nameBlock = dirblock->names_first_block;
		if( filename )
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[ofs]];
			if( nextblock ) {
				leadin[(*leadinDepth)++] = filename[ofs];
				ofs += 1;
				this_dir_block = nextblock;
				// just stepping to new block - unlock old, will lock new.
				SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )-1 );
				//lprintf( "Follow subdirectory %d", (int)nextblock );
				continue;
			}
		}
		else {
			uint8_t charIndex;
			for( charIndex = 0; charIndex < 255; charIndex++ ) {
				BLOCKINDEX nextblock = dirblock->next_block[charIndex];
				if( nextblock ) {
					LOGICAL r;
					leadin[(*leadinDepth)++] = (char)charIndex;
#ifdef _DEBUG
					if( !file && !filename && nameBlockStart )
						lprintf( "Check subdirectory %d %d %c", (int)nextblock, (int)(dirblock->next_block[255] >> 8), charIndex );
#endif
					r = _os_ScanDirectory_( vol, NULL, nextblock, nameBlockStart, file, path_match, leadin, leadinDepth );
					(*leadinDepth)--;
					if( r && r != 2 )   {
						lprintf( "scan is returning early.. %d",r );
						SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )-1 );
						return r;
					}
				}
			}
		}
		usedNames = dirblock->used_names - 1;
		if( SUS_GT( usedNames, int, ( sizeof( dirblock->entries ) / sizeof( dirblock->entries[0] ) ), size_t ) ) {
			lprintf( "Directory block name count is corrupt." );
			DebugBreak();
		}
		minName = 0;
		curName = (usedNames) >> 1;
		{
			next_entries = dirblock->entries;
			//lprintf( "name block %d %d %d", (int)dirBlockSeg, (int)usedNames, (int)cache );
			while( minName <= usedNames && ( curName <= usedNames ) && ( curName >= 0 ) )
			{
				BLOCKINDEX bi;
				enum block_cache_entries name_cache = BC(NAMES);
				struct directory_entry *entry = dirblock->entries + curName;
				//const char * testname;
				FPI name_ofs = ( entry->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
#ifdef DEBUG_CONVERT_DIRECTORY // symbol not defined

				l.fileCount++;
#endif

#ifdef DEBUG_TIMELINE_DIR_TRACKING
				if( entry->timelineEntry ) // else we have a different issue.
				{
					// make sure timeline and file entries reference each other.
					struct memoryTimelineNode time;
					reloadTimeEntry( &time, vol, entry->timelineEntry VTReadOnly GRTELog DBG_SRC );
					FPI entry_fpi = vol->bufferFPI[cache] + sane_offsetof( struct directory_hash_lookup_block, entries[curName] );
					if( entry_fpi != time.disk->dirent_fpi ) {
						lprintf( "!!!! directory entry doesn't match: %d %d", entry_fpi, time.disk->dirent_fpi );
						DebugBreak();
					}
					dropRawTimeEntry( vol, time.diskCache GRTELog DBG_SRC );
				}
#endif
				//if( filename && !name_ofs )	return FALSE; // done.
				if( 0 ) {
					LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
						, curName, name_ofs
						, entry->name_offset
						, entry->first_block
						, filename + ofs );
				}
				if( curName < usedNames ) {
					bi = entry->first_block ;
					// if file is deleted; don't check it's name.
					if( !bi ) {
						lprintf( "File is already deleted... (these should be removed)" );
						continue;
					}
					// if file is end of directory, done sanning.
					if( bi == EODMARK ) { lprintf( "Found end of directory mark." ); return filename ? FALSE : (2); }// done.
					if( name_ofs > vol->dwSize ) { lprintf( "name offset is bigger than volume size!"); return FALSE; }
				}
				//testname =
				if( filename ) {
					int d;
					//LoG( "this name: %s", names );
					if( ( d = _os_MaskStrCmp( vol, filename+ofs, nameBlock, name_ofs, path_match ) ) == 0 ) {
						if( file )
						{
							/* can just keep the existing lock...
							int locks = GETMASK_( vol->seglock, seglock, cache ) + 1;
							if( locks > 12 ) {
								lprintf( "Too many locks open... " );
								DebugBreak();
							}
							SETMASK_( vol->seglock, seglock, cache, locks );
							*/
							file->cache = cache;
							file->entry_fpi = vol->bufferFPI[BC(DIRECTORY)] + ((uintptr_t)(((struct directory_hash_lookup_block *)0)->entries + curName));
							file->dir_block = this_dir_block;
							file->entry_.name_offset = ( entry->name_offset & DIRENT_NAME_OFFSET_OFFSET ) + vfs_os_compute_data_block( vol, dirblock->names_first_block, BC( COUNT ) );
							file->entry = entry;
						}
#ifdef DEBUG_DIRECTORIES
						if(0)
						LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %*.*s%s"
							, entry, name_ofs, entry->first_block
							, *leadinDepth, *leadinDepth, leadin
							, filename+ofs );
#endif
						if( nameBlockStart ) nameBlockStart[0] = dirblock->names_first_block ;
						// keep the lock because of the file cache reference.
						if( !file )
							SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )-1 );
						return TRUE;
					}
					if( d > 0 ) {
						minName = curName + 1;
					} else {
						usedNames = curName - 1;
					}
					curName = (minName + usedNames) >> 1;
				}
				else {
					if( minName < usedNames && !file ) {
						if( !checkFileLength( vol, entry->first_block, entry->filesize ) ) {
							lprintf( "directory scan found a short file chain" );
							return FALSE;
						}
					}
					curName++;
				}
			}
			// no file created, found end, unlock this directory
			SETMASK_( vol->seglock, seglock, cache, GETMASK_( vol->seglock, seglock, cache )-1 );

			return filename ? FALSE : (2); // done.;
		}
		// unreachable, and broken code.
#if 0
		BLOCKINDEX next_dir_block;
		next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_TIMELINE_MORE, TRUE, DIR_BLOCK_SIZE, NULL );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) { DebugBreak(); return FALSE; }  // should have a last-entry before no more blocks....
#endif
		this_dir_block = next_dir_block;
#endif
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI _os_SaveFileName( struct sack_vfs_os_volume *vol, BLOCKINDEX firstNameBlock, const char * filename, size_t namelen ) {
	int blocks = 0;
	LOGICAL scanToEnd = FALSE;
	BLOCKINDEX this_name_block = firstNameBlock;
#ifdef _DEBUG
	if( !firstNameBlock ) DebugBreak();
#endif
	//lprintf( "Save filename:%s", filename );

	while( 1 ) {
		enum block_cache_entries cache = BC(NAMES);
		unsigned char *names = BTSEEK( unsigned char *, vol, this_name_block, NAME_BLOCK_SIZE, cache );
		unsigned char *name = (unsigned char*)names;
		if( scanToEnd ) {
			while(
				(UTF8_EOT != (name[0] ))
				&& (name - names < NAME_BLOCK_SIZE)
				) name++;
			if( ( name - names ) >= NAME_BLOCK_SIZE ) {
				// wow, that is a really LONG name.
				cache = BC( NAMES );
				this_name_block = vfs_os_GetNextBlock( vol, this_name_block, &cache, GFB_INIT_NAMES, TRUE, NAME_BLOCK_SIZE, NULL );
				blocks++;
				continue;
			}
		}
		scanToEnd = FALSE;
		while( name < ( (unsigned char*)names + NAME_BLOCK_SIZE ) ) {
			int c = name[0];
			if( (unsigned char)c == UTF8_EOTB ) {
				if( namelen < (size_t)( ( (unsigned char*)names + NAME_BLOCK_SIZE ) - name ) ) {
					//LoG( "using unused entry for new file...%" _size_f " %d(%d)  %" _size_f " %s", this_name_block, cache, cache - BC(NAMES), name - names, filename );
					memcpy( name, filename, namelen );
					name[namelen+0] = UTF8_EOT;
					name[namelen+1] = UTF8_EOTB;
					SMUDGECACHE( vol, cache );
					//LoG( "save name OFFSET:%d %d", (name) - (names), +blocks * NAME_BLOCK_SIZE );
					return (name) - (names) + blocks * NAME_BLOCK_SIZE;
				}
			}
			else
				if( _os_MaskStrCmp( vol, filename, firstNameBlock, (name - names)+blocks*NAME_BLOCK_SIZE, 0 ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					lprintf( "name already in cache %d", cache );
					return (name) - (names) + blocks * NAME_BLOCK_SIZE;
				}
			while(
				( UTF8_EOT != ( name[0] ) )
				&& ( name-names < NAME_BLOCK_SIZE )
			) name++;
			if( name - names <= NAME_BLOCK_SIZE ) {
				if( name - names < NAME_BLOCK_SIZE ) {
					name++;
				}
				else {
					// next seek will be on right character...
				}
			}
			else
				scanToEnd = TRUE; // still looking for EOT
			//LoG( "new position is %" _size_f "  %" _size_f, this_name_block, name - names );
		}
		this_name_block = vfs_os_GetNextBlock( vol, this_name_block, &cache, GFB_INIT_NAMES, TRUE, NAME_BLOCK_SIZE, NULL );
		blocks++;
		//LoG( "Need a new name block.... %d", this_name_block );
	}
	lprintf( "didn't actually save the name?" );
}

static void deleteDirectoryEntryName( struct sack_vfs_os_volume* vol, struct sack_vfs_os_file* file, int nameOffset, enum block_cache_entries nameCache, BLOCKINDEX dir_block_index ) {
	size_t n;
	FPI nameoffset_temp = 0;

	static uint8_t namebuffer[3 * 4096];
	uint8_t* nameblock = NULL;
	uint8_t* nameblock_;
	int f;
	int e = -1;
	enum block_cache_entries name_cache = BC(ZERO), name_cache_;
	int endNameOffset = 0; // this will be a smallish int
	int findNameOffset = nameOffset;
	struct directory_hash_lookup_block* dirblock = (struct directory_hash_lookup_block*)vol->usekey_buffer[nameCache];
	BLOCKINDEX name_block = dirblock->names_first_block;

#ifdef DEBUG_FILE_OPEN
	LoG( "------------ BEGIN DELETE DIRECTORY ENTRY NAME ----------------------" );
#endif
	// read name block chain into a single array
	do {
		uint8_t* out;
		uint8_t* in;
		name_cache_ = name_cache;
		name_cache = BC( NAMES );
		nameblock_ = nameblock;
		nameblock = BTSEEK( uint8_t*, vol, name_block, NAME_BLOCK_SIZE, name_cache );
		if( findNameOffset < NAME_BLOCK_SIZE ) {
			// first read block (nameoffset_temp = 0)
			// not already found end of text mark ( endNameOffset = 0 )
			SETMASK_( vol->seglock, seglock, name_cache, GETMASK_( vol->seglock, seglock, name_cache ) + 1 );
			if( !endNameOffset ) {

				for( n = findNameOffset; n < NAME_BLOCK_SIZE; n++ ) {
					if( nameblock[n] == UTF8_EOT ) {
						endNameOffset = (int)(nameoffset_temp + n + 1);
						break;
					}
				}
				// if the difference was found, move the rest of this block.
				if( endNameOffset ) {
					in = nameblock + endNameOffset;
					out = nameblock + findNameOffset;
					for( n = endNameOffset; n < NAME_BLOCK_SIZE; n++ ) {
						( *out++ ) = ( *in++ );
					}
					SMUDGECACHE( vol, name_cache );  // re-wrote block;
				}
			}
			else {
				// already have a known end of name, and the offset
				// move the data in this block forward.
				unsigned int namelength = ( endNameOffset - findNameOffset );
				unsigned int length = NAME_BLOCK_SIZE - ( namelength );
				in = nameblock;
				out = nameblock_ + length;
				for( n = 0; n < namelength; n++ ) {
					( *out++ ) = ( *in++ );
				}
				// switch blocks.
				out = nameblock;
				for( n = 0; n < length; n++ ) {
					( *out++ ) = ( *in++ );
				}
				SMUDGECACHE( vol, name_cache );  // re-wrote block;
				// unlock the previous block (if there was one);
				if( name_cache_ )
					SETMASK_( vol->seglock, seglock, name_cache_, GETMASK_( vol->seglock, seglock, name_cache_ ) - 1 );
			}
			nameoffset_temp += NAME_BLOCK_SIZE;
		}
		else {
			findNameOffset -= NAME_BLOCK_SIZE;
		}
		enum block_cache_entries gnbCache = BC( NAMES );
		name_block = vfs_os_GetNextBlock( vol, name_block, &gnbCache, GFB_INIT_NONE, FALSE, NAME_BLOCK_SIZE, NULL );
	} while( name_block != EOFBLOCK );
	// unlock the last block
	SETMASK_( vol->seglock, seglock, name_cache, GETMASK_( vol->seglock, seglock, name_cache ) - 1 );
	{
		uint8_t* out;
		out = nameblock + ( n = NAME_BLOCK_SIZE - ( endNameOffset - findNameOffset ) );
		// fill tail of block with 0.
		for( ; n < NAME_BLOCK_SIZE; n++ ) {
			( *out++ ) = 0;
		}
	}
	// move the directory entries down.
	//file->entry->name_offset = 0;
	for( f = 0; f < dirblock->used_names; f++ ) {
		if( USS_GT( ( dirblock->entries[f].name_offset & DIRENT_NAME_OFFSET_OFFSET ), FPI, nameOffset, int ) )
			dirblock->entries[f].name_offset -= endNameOffset - findNameOffset;
		if( ( dirblock->entries + f ) == file->entry ) {
			e = f;
		}
		else if( e >= 0 ) {
			if( dirblock->entries[f].timelineEntry ) {
				struct memoryTimelineNode time;
				enum block_cache_entries  timeCache = BC( TIMELINE );
				reloadTimeEntry( &time, vol, ( dirblock->entries[f].timelineEntry ) VTReadWrite GRTENoLog DBG_SRC );
				time.disk->dirent_fpi = vol->bufferFPI[nameCache] + sane_offsetof( struct directory_hash_lookup_block, entries[f - 1] );
				{
					uint64_t index = time.disk->priorTime;
					while( index ) {
						struct memoryTimelineNode time2;
						reloadTimeEntry( &time2, vol, index GRTENoLog VTReadWrite DBG_SRC );
						time2.disk->dirent_fpi = time.disk->dirent_fpi;
						index = time2.disk->priorTime;
						updateTimeEntry( &time2, vol, TRUE DBG_SRC );
					}
				}
#ifdef DEBUG_TIMELINE_DIR_TRACKING
				lprintf( "Set timeline %d to %d", (int)time.index, (int)time.disk->dirent_fpi );
#endif
				updateTimeEntry( &time, vol, TRUE DBG_SRC );
			}
			dirblock->entries[f - 1] = dirblock->entries[f];
		}
	}
	dirblock->used_names--;
	SMUDGECACHE( vol, name_cache );

}

static void ConvertDirectory( struct sack_vfs_os_volume *vol, const char *leadin, int leadinLength, BLOCKINDEX this_dir_block, struct directory_hash_lookup_block *orig_dirblock, enum block_cache_entries *newCache ) {
	size_t n;
#ifdef DEBUG_CONVERT_DIRECTORY
	LoG( "------------ BEGIN CONVERT DIRECTORY ---------------------- %d", this_dir_block );
#endif
#ifdef DEBUG_CONVERT_DIRECTORY // symbol not defined
	{
		BLOCKINDEX a;
		if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, NULL, NULL, 0 ) ) {
		 	lprintf( "Directory scan in close failed" );
			DebugBreak();
			//return ;
		}
		if( l.fileCount_old > l.fileCount ) {
			BLOCKINDEX x;
			lprintf( "operation befoe the convert lost files. %d %d", l.fileCount_old, l.fileCount );
			//if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, &x, NULL, 0 ) ) {
		 	//lprintf( "Directory scan in close failed" );
			DebugBreak();
			//return ;
		}
	}
	lprintf( "total files:%d", l.fileCount );
	l.fileCount_old = l.fileCount;
#endif



	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		FPI nameoffset_temp = 0;
		BLOCKINDEX new_dir_block;
		struct directory_hash_lookup_block *dirblock;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
		{
			static int counters[256];
			static uint8_t namebuffer[18*4096];
			uint8_t *nameblock;
			int maxc = 0;
			int imax = 0;
			int f;
			enum block_cache_entries name_cache;
			BLOCKINDEX name_block = dirblock->names_first_block;
			// read name block chain into a single array
			do {
				uint8_t *out = namebuffer + nameoffset_temp;
				name_cache = BC( NAMES );
				nameblock = BTSEEK( uint8_t *, vol, name_block, NAME_BLOCK_SIZE, name_cache );
				for( n = 0; n < 4096; n++ )
					(*out++) = (*nameblock++);
				enum block_cache_entries gnbCache = BC( NAMES );
				name_block = vfs_os_GetNextBlock( vol, name_block, &gnbCache, GFB_INIT_NONE, 0, NAME_BLOCK_SIZE, NULL );
				nameoffset_temp += NAME_BLOCK_SIZE;
			} while( name_block != EOFBLOCK );


			memset( counters, 0, sizeof( counters ) );
			// 257/85
			for( f = 0; f < VFS_DIRECTORY_ENTRIES; f++ ) {
				BLOCKINDEX first = dirblock->entries[f].first_block;
				FPI name;
				int count;
				if( first == EODMARK ) break;
				name = dirblock->entries[f].name_offset & DIRENT_NAME_OFFSET_OFFSET;
				count = (++counters[namebuffer[name]]);
				if( count > maxc ) {
					imax = namebuffer[name];
					maxc = count;
				}
			}
			// after finding most used first byte; get a new block, and point
			// hash entry to that.
			enum block_cache_entries newBlockCache = BC( DIRECTORY );
			dirblock->next_block[imax]
				= ( new_dir_block
				  = _os_GetFreeBlock( vol, &newBlockCache, GFB_INIT_DIRENT, 4096 ) );

			//if( new_dir_block == 48 || new_dir_block == 36 )
			//	DebugBreak();

			SMUDGECACHE( vol, cache );
			{
				struct directory_hash_lookup_block *newDirblock;
				enum block_cache_entries newdir_cache;
				BLOCKINDEX newFirstNameBlock;
				int usedNames = dirblock->used_names;
				int _usedNames = usedNames;
				int nf = 0;
				int firstNameOffset = -1;
				int finalNameOffset = 0;;
				int movedEntry = 0;
				int offset;
				newdir_cache = BC(DIRECTORY);
				newDirblock = BTSEEK( struct directory_hash_lookup_block *, vol, new_dir_block, DIR_BLOCK_SIZE, newdir_cache );

#ifdef DEBUG_CONVERT_DIRECTORY
				LoG( "new dir block cache is  %d   %d   %d  %d", newdir_cache, (int)new_dir_block, (int)this_dir_block, imax );
#endif
				newFirstNameBlock = newDirblock->names_first_block;
#ifdef _DEBUG
				if( !newDirblock->names_first_block )
					DebugBreak();
#endif
				newDirblock->next_block[DIRNAME_CHAR_PARENT] = (this_dir_block << 8) | imax;

				//SMUDGECACHE( vol, newdir_cache ); // this will be dirty because it was init above.

				for( f = 0; f < usedNames; f++ ) {
					BLOCKINDEX first = dirblock->entries[f].first_block;
					struct directory_entry *entry;
					struct directory_entry *newEntry;
					FPI name;
					FPI name_ofs;
					entry = dirblock->entries + (f);
					name = ( entry->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
					if( namebuffer[name] == imax ) {
						int namelen;
						if( !movedEntry ) movedEntry = f+1;
						newEntry = newDirblock->entries + (nf);
						//LoG( "Saving existing name %d %s", name, namebuffer + name );
						//LogBinary( namebuffer, 32 );
						namelen = 0;
						while( namebuffer[name + namelen] != UTF8_EOT )namelen++;
						name_ofs = _os_SaveFileName( vol, newFirstNameBlock, (char*)(namebuffer + name + 1), namelen -1 );
						{
							INDEX idx;
							struct sack_vfs_os_file  * file;
							LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
								if( file->entry == entry ) {
									file->entry_fpi = 0; // new entry_fpi.
								}
							}
						}
						dirblock->used_names = ((dirblock->used_names) - 1);
						if( dirblock->used_names > ( sizeof( dirblock->entries ) / sizeof( dirblock->entries[0] ) ) ) {
							lprintf( "Directory block name count is corrupt." );
							DebugBreak();
						}
						newEntry->filesize = entry->filesize;
						{
							struct memoryTimelineNode time;
							FPI oldFPI;
							enum block_cache_entries  timeCache = BC( TIMELINE );
							reloadTimeEntry( &time, vol, (entry->timelineEntry     ) VTReadWrite GRTENoLog DBG_SRC );
							oldFPI = (FPI)time.disk->dirent_fpi; // dirent_fpi type is larger than index in some configurations; but won't exceed those bounds
							// new entry is still the same timeline entry as the old entry.
							newEntry->timelineEntry = (entry->timelineEntry     )     ;
							// timeline points at new entry.
							time.disk->dirent_fpi = vol->bufferFPI[newdir_cache] + sane_offsetof(struct directory_hash_lookup_block, entries[nf]);
							{
								uint64_t index = time.disk->priorTime;
								while( index ) {
									struct memoryTimelineNode time2;
									reloadTimeEntry( &time2, vol, index VTReadWrite GRTENoLog DBG_SRC );
									time2.disk->dirent_fpi = time.disk->dirent_fpi;
									updateTimeEntry( &time2, vol, TRUE DBG_SRC );
									index = time2.disk->priorTime;
								}
							}
#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "Set timeline %d to %d", (int)time.index, (int)time.disk->dirent_fpi );
#endif
							updateTimeEntry( &time, vol, TRUE DBG_SRC );

#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "direntry at %d  %d is time %d", (int)new_dir_block, (int)nf, (int)newEntry->timelineEntry );
#endif

							{
								INDEX idx;
								struct sack_vfs_file  * file;
								LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
									if( file->entry_fpi == oldFPI ) {
										// unlock old directory
										int locks = GETMASK_( vol->seglock, seglock, file->cache ) - 1;
										if( locks < 0 ) {
											lprintf( "File lock in convert underflow... " );
											DebugBreak();
										}
										SETMASK_( vol->seglock, seglock, cache, locks );

										// new entry_fpi.
										file->entry_fpi = (FPI)time.disk->dirent_fpi; // dirent_fpi type is larger than index in some configurations; but won't exceed those bounds

										//file->dir_block = time.disk->dir

										lprintf( "File cache might have been wrong... (AND USED OLD ENTRY)" );
										file->entry = newEntry;
										file->cache = newdir_cache;

										// lock new cache entry
										locks = GETMASK_( vol->seglock, seglock, file->cache ) + 1;
										if( locks < 0 ) {
											lprintf( "File lock in convert underflow... " );
											DebugBreak();
										}
										SETMASK_( vol->seglock, seglock, cache, locks );

									}
								}
							}
						}

						newEntry->name_offset = name_ofs;
						newEntry->first_block = (entry->first_block ) ;
						//lprintf( "Convert File new block %d", entry->first_block );
						SMUDGECACHE( vol, cache );
						nf++;

						newDirblock->used_names = ((newDirblock->used_names) + 1);
						if( newDirblock->used_names > ( sizeof( newDirblock->entries ) / sizeof( newDirblock->entries[0] ) ) ) {
							lprintf( "Directory block name count is corrupt." );
							DebugBreak();
						}
						//usedNames--;
					}
					else {
						if( movedEntry ) {
							break;
						}
					}
				}
				LoG( "blocks: %d %d %d old names new names %d %d = %d"
						, this_dir_block, new_dir_block
						, imax
						, dirblock->used_names, newDirblock->used_names, dirblock->used_names+ newDirblock->used_names );
				// move all others down 1.
				movedEntry = movedEntry - 1;
				offset = (f - movedEntry);
				usedNames -= (f-movedEntry);
				//for( ; f < usedNames; f++ )
				{
					int m;
					for( m = movedEntry; m < usedNames; m++ ) {
						dirblock->entries[m].first_block = dirblock->entries[m + offset].first_block;
						dirblock->entries[m].name_offset = dirblock->entries[m + offset].name_offset;
						dirblock->entries[m].filesize = dirblock->entries[m + offset].filesize;
						dirblock->entries[m].timelineEntry = dirblock->entries[m + offset].timelineEntry;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						lprintf( "direntry at %d  %d is time %d", (int)this_dir_block, (int)m, (int)dirblock->entries[m].timelineEntry );
#endif
						{
							struct memoryTimelineNode time;
							enum block_cache_entries  timeCache = BC( TIMELINE );
							reloadTimeEntry( &time, vol, (dirblock->entries[m + offset].timelineEntry) VTReadWrite GRTENoLog DBG_SRC );
							time.disk->dirent_fpi = this_dir_block * BLOCK_SIZE /*vol->bufferFPI[cache]*/ + sane_offsetof( struct directory_hash_lookup_block, entries[m] );
							{
								uint64_t index = time.disk->priorTime;
								while( index ) {
									struct memoryTimelineNode time2;
									reloadTimeEntry( &time2, vol, index VTReadWrite GRTENoLog DBG_SRC );
									time2.disk->dirent_fpi = time.disk->dirent_fpi;
									updateTimeEntry( &time2, vol, TRUE DBG_SRC );
									index = time2.disk->priorTime;
								}
							}
#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "Set timeline %d to %d", (int)time.index, (int)time.disk->dirent_fpi );
#endif
							updateTimeEntry( &time, vol, TRUE DBG_SRC );
						}
#ifdef _DEBUG
						if( !dirblock->names_first_block ) DebugBreak();
#endif
					}

					for( m = usedNames; m < VFS_DIRECTORY_ENTRIES; m++ ) {
						dirblock->entries[m].first_block = (0);
						dirblock->entries[m].name_offset = (0);
						dirblock->entries[m].filesize = (0);
						dirblock->entries[m].timelineEntry = (0);
#ifdef _DEBUG
						if( !dirblock->names_first_block ) DebugBreak();
#endif
					}
				}


				if( usedNames ) {
					static uint8_t newnamebuffer[18 * 4096];
					int newout = 0;
					int min_name = NAME_BLOCK_SIZE + 1;
					int _min_name = -1; // min found has to be after this one.
					//lprintf( "%d names remained.", usedNames );
					for( f = 0; f < usedNames; f++ ) {
						struct directory_entry *entry;
						FPI name;
						entry = dirblock->entries + (f);
						name = ( entry->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
						entry->name_offset = ( newout )
							| ( (entry->name_offset)
								& ~DIRENT_NAME_OFFSET_OFFSET );
						while( namebuffer[name] != UTF8_EOT )
							newnamebuffer[newout++] = namebuffer[name++];
						newnamebuffer[newout++] = namebuffer[name++];
					}
					newnamebuffer[newout++] = UTF8_EOTB;
					memcpy( namebuffer, newnamebuffer, newout );
					memset( namebuffer + newout, 0, NAME_BLOCK_SIZE - newout ); // tidy up the end of the old buffer.
					memset( newnamebuffer, 0, newout );
				}
				else {
					namebuffer[0] = UTF8_EOTB;
				}
				{
					name_block = dirblock->names_first_block;
					nameoffset_temp = 0;
					do {
						uint8_t *out;
						nameblock = namebuffer + nameoffset_temp;
						name_cache = BC( NAMES );
						out = BTSEEK( uint8_t *, vol, name_block, NAME_BLOCK_SIZE, name_cache );
						for( n = 0; n < 4096; n++ )
							(*out++) = (*nameblock++);
						SMUDGECACHE( vol, name_cache );
						enum block_cache_entries gnbCache = BC(NAMES);

						name_block = vfs_os_GetNextBlock( vol, name_block, &gnbCache, GFB_INIT_NONE, 0, NAME_BLOCK_SIZE, NULL );
						nameoffset_temp += 4096;
					} while( name_block != EOFBLOCK );
				}
			}
#ifdef DEBUG_CONVERT_DIRECTORY // symbol not defined

			if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, NULL, NULL, 0 ) ) {
			 	lprintf( "Directory scan in close failed" );
				DebugBreak();
				//return ;
			}
			if( l.fileCount_old > l.fileCount ) {
				BLOCKINDEX x;
				lprintf( "This convert operation lost files. %d %d", l.fileCount_old, l.fileCount );
				if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, &x, NULL, 0 ) ) {
				 	lprintf( "Directory scan in close failed" );
					DebugBreak();
					//return ;
				}
				DebugBreak();
			}
			lprintf( "total files:%d", l.fileCount );
			l.fileCount_old = l.fileCount;
#endif


			break;  // a set of names has been moved out of this block.
			// has block.
		}
	} while( 0 );

	// unlink here
	// unlink dirblock->names_first_block
}

static struct directory_entry * _os_GetNewDirectory( struct sack_vfs_os_volume *vol,
#if defined( _MSC_VER )
	_In_
#endif
	const char * filename
		, struct sack_vfs_os_file *file ) {
	size_t n;
	const char *_filename = filename;
	static char leadin[256];
	static int leadinDepth = 0;
	BLOCKINDEX this_dir_block = FIRST_DIR_BLOCK;
	struct directory_entry *next_entries;
	LOGICAL moveMark = FALSE;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	leadinDepth = 0;
	do {
		enum block_cache_entries cache;
		FPI dirblockFPI;
		int usedNames;
		struct directory_hash_lookup_block *dirblock;
		BLOCKINDEX firstNameBlock;
		cache = BC( DIRECTORY );
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
#ifdef _DEBUG
		if( !dirblock->names_first_block ) {
			if( dirblock->used_names )
				DebugBreak();
			{
				enum block_cache_entries newcache = BC( NAMES );
				dirblock->names_first_block = _os_GetFreeBlock( vol, &newcache, GFB_INIT_NAMES, NAME_BLOCK_SIZE );
			}
			dirblock->used_names = 0;
		}
#endif

		dirblockFPI = vol->bufferFPI[cache];
		firstNameBlock = dirblock->names_first_block;
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[0]];
			if( nextblock ) {
				leadin[leadinDepth++] = filename[0];
				filename++;
				this_dir_block = nextblock;
				// retry;
				continue;
			}
		}
		usedNames = dirblock->used_names;
		//lprintf( " --------------- THIS DIR BLOCK ---------------" );
		if( usedNames == VFS_DIRECTORY_ENTRIES ) {
			ConvertDirectory( vol, leadin, leadinDepth, this_dir_block, dirblock, &cache );
			/* retry */
#ifdef DEBUG_DIRECTORIES
			lprintf( "----- CONVERTED-------" );
			_os_dumpDirectories( vol, this_dir_block, 1 );
			lprintf( "----- CONVERTED (ALL)-------" );
			_os_dumpDirectories( vol, 0, 1 );
#endif
			continue;
		}
		{
			struct directory_entry *ent;
			FPI name_ofs;
			BLOCKINDEX first_blk;

			next_entries = dirblock->entries;
			ent = dirblock->entries;
			for( n = 0; USS_LT( n, size_t, usedNames, int ); n++ ) {
				ent = dirblock->entries + (n);
				name_ofs = ( ent->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
				first_blk = ent->first_block;
				// not name_offset (end of list) or not first_block(free entry) use this entry
				//if( name_ofs && (first_blk > 1) )  continue;

				if( _os_MaskStrCmp( vol, filename, firstNameBlock, name_ofs, 0 ) < 0 ) {
					int m;
#ifdef DEBUG_FILE_OPEN
					LoG( "Insert new directory" );
#endif
					for( m = dirblock->used_names; SUS_GT( m, int, n, size_t ); m-- ) {
						struct memoryTimelineNode node;
						dirblock->entries[m].filesize      = dirblock->entries[m - 1].filesize      ;
						dirblock->entries[m].first_block   = dirblock->entries[m - 1].first_block   ;
						dirblock->entries[m].name_offset   = dirblock->entries[m - 1].name_offset   ;
						reloadTimeEntry( &node, vol, dirblock->entries[m - 1].timelineEntry VTReadWrite GRTENoLog DBG_SRC );
						dirblock->entries[m].timelineEntry = dirblock->entries[m - 1].timelineEntry;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						LoG( "direntry at %d  %d is time %d", (int)this_dir_block, (int)m, (int)dirblock->entries[m].timelineEntry );
#endif
						node.disk->dirent_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[m] );
						{
							uint64_t index = node.disk->priorTime;
							while( index ) {
								struct memoryTimelineNode time2;
								reloadTimeEntry( &time2, vol, index VTReadWrite GRTENoLog DBG_SRC );
								time2.disk->dirent_fpi = node.disk->dirent_fpi;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
								LoG( "(Move)Set timeline %d to %d", (int)time2.index, (int)time2.disk->dirent_fpi );
#endif
								updateTimeEntry( &time2, vol, TRUE DBG_SRC );
								index = time2.disk->priorTime;
							}
						}
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						lprintf( "Set timeline %d to %d", (int)node.index, (int)node.disk->dirent_fpi );
#endif
						updateTimeEntry( &node, vol, TRUE DBG_SRC );
					}
					dirblock->used_names++;
					break;
				}
			}
			ent = dirblock->entries + (n);
			if( n == usedNames ) {
				dirblock->used_names = (uint8_t)((n + 1));
			}
			if( dirblock->used_names > ( sizeof( dirblock->entries ) / sizeof( dirblock->entries[0] ) ) ) {
				lprintf( "Directory block name count is corrupt." );
				DebugBreak();
			}
			//LoG( "Get New Directory save naem:%s", filename );
			name_ofs = _os_SaveFileName( vol, firstNameBlock, filename, StrLen( filename ) );
			ent->filesize = 0;
			ent->name_offset = name_ofs;
			// have to allocate a block for the file, otherwise it would be deleted.
			ent->first_block = DIR_ALLOCATING_MARK;// first_blk;
			{
				struct memoryTimelineNode time_;
				struct memoryTimelineNode *time = &time_;
				time_.index = 0;
				ent->timelineEntry = getTimeEntry( time, vol, 0, NULL, 0 DBG_SRC );
				// reset dirent_fpi afterward.
				time->disk->dirent_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[n] );;
				// associate a time entry with this directory entry, and vice-versa.
#ifdef DEBUG_TIMELINE_DIR_TRACKING
				lprintf( "Set timeline %d to %d", (int)time->index, (int)time->disk->dirent_fpi );
#endif
#ifdef DEBUG_TIMELINE_DIR_TRACKING
				lprintf( "direntry at %d  %d is time %d", (int)this_dir_block, (int)n, (int)dirblock->entries[n].timelineEntry );
#endif
				// update drop the new entry.
				updateTimeEntry( time, vol, TRUE DBG_SRC );
			}
			if( file ) {
				int locks;
				locks = GETMASK_( vol->seglock, seglock, cache ) + 1;
				if( locks > 12 ) {
					lprintf( "Too many locks open... " );
					DebugBreak();
				}
				SETMASK_( vol->seglock, seglock, cache, locks );
				file->entry_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[n] );;
				file->dir_block = this_dir_block;
				file->entry_.name_offset = ( file->entry->name_offset & DIRENT_NAME_OFFSET_OFFSET ) + vfs_os_compute_data_block( vol, dirblock->names_first_block, BC( COUNT ) );
				// the modern version only uses entry and cache; entry_fpi, dir_block above are unused; this dirent is locked into the caching subsystem.
				file->entry = ent;
				file->cache = cache;
			}
			SMUDGECACHE( vol, cache );
			return ent;
		}
	}
	while( 1 );

}

static struct sack_vfs_os_file * CPROC sack_vfs_os_openfile_internal( struct sack_vfs_os_volume *vol, const char * filename, uint64_t version, LOGICAL create ) {
	struct sack_vfs_os_file *file = GetFromSet( VFS_OS_FILE, &l.files );//New( struct sack_vfs_os_file );
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	MemSet( file, 0, sizeof( struct sack_vfs_os_file ) );
	BLOCKINDEX offset;
	file->vol = vol;
	file->entry = &file->entry_; // default to internal buffer; might never have a real directory
	//file->sealant = NULL;

	if( filename[0] == '.' && ( filename[1] == '\\' || filename[1] == '/' ) ) filename += 2;

#ifdef DEBUG_FILE_OPEN
	LoG( "sack_vfs open %s = %p on %s", filename, file, vol->volname );
#endif
	if( filename )
		if( !_os_ScanDirectory( vol, filename, FIRST_DIR_BLOCK, NULL, file, 0 ) ) {
			if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; 
				DeleteFromSet( VFS_OS_FILE, &l.files, file ); //Deallocate( struct sack_vfs_os_file*, file ); 
				return NULL; 
			}
			else _os_GetNewDirectory( vol, filename, file );
		}

	if( ( file->entry->first_block != DIR_ALLOCATING_MARK ) && create ) {
		// if there is already data
		file->flags.versioned = 1;
	} else 
		file->flags.versioned = 0;
	
	file->filesize_ = file->entry->filesize; // saved for versioning really
	// update to the file's first block (allocating, data, whatever)
	file->_first_block = file->block = file->entry->first_block;

	if( create ) {  // sort of a opened for write
		// this updates the timestamp of the file, and allocates a new one
		PDATALIST pdlTimes = CreateDataList( sizeof( uint64_t ) );
		struct sack_vfs_os_volume* vol = file->vol;
	        
		struct memoryTimelineNode time;
		enum block_cache_entries  timeCache = BC( TIMELINE );
	        
		BLOCKINDEX priorData = file->entry->first_block;
		reloadTimeEntry( &time, vol, file->entry->timelineEntry VTReadWrite GRTENoLog  DBG_SRC );
#ifdef _DEBUG
		if( !time.disk->time ) DebugBreak();
#endif
		// open oldest by default, with no prior time set...
		if( time.disk->priorTime ) {
			BLOCKINDEX priorTime = time.disk->priorTime;
			while( priorTime ) {
				enum block_cache_entries cache;
				struct storageTimelineNode* prior = getRawTimeEntry( vol, priorTime, &cache GRTENoLog DBG_SRC );
				//prior->
				priorTime = prior->priorTime;
				priorData = prior->priorData;
				file->filesize_ = prior->priorDataSize;
				if( prior->time <= version ) break;
				dropRawTimeEntry( file->vol, cache GRTENoLog DBG_SRC );
			}
		}
		dropRawTimeEntry( vol, time.diskCache GRTENoLog DBG_SRC );
	}
	offset = file->entry_.name_offset; // file->entry->name_offset;
	//file->filename = StrDup( filename );
	//file->fileName = !!filename;
#ifdef XX_VIRTUAL_OBJECT_STORE
	if( ( file->entry->name_offset ) & DIRENT_NAME_OFFSET_FLAG_SEALANT ) {
		sack_vfs_os_read_internal( file, 0, &file->diskHeader, sizeof( file->diskHeader ) );
		file->header = file->diskHeader;
		file->fpi = file->header.sealant.avail + file->header.references.avail;
		{
			uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
			if( sealLen ) {
				file->seal = NewArray( uint8_t, sealLen );
				//file->sealantLen = sealLen;
				file->sealed = SACK_VFS_OS_SEAL_LOAD;
			}
			else {
				file->seal = NULL;
				//file->sealantLen = 0;
				file->sealed = SACK_VFS_OS_SEAL_NONE;
			}
		}
	}
#endif
	AddLink( &vol->files, file );
	vol->lock = 0;
	return file;
}

struct sack_vfs_os_file * CPROC sack_vfs_os_openfile( struct sack_vfs_os_volume *vol, const char * filename ) {
	return sack_vfs_os_openfile_internal( vol, filename, 0, FALSE );
}

static struct sack_vfs_os_file * CPROC sack_vfs_os_open( uintptr_t psvInstance, const char * filename, const char *opts ) {
	return sack_vfs_os_openfile( (struct sack_vfs_os_volume*)psvInstance, filename );
}



static char * getFilename( const char *objBuf, size_t objBufLen
	, char *sealBuf, size_t sealBufLen, LOGICAL owner
	, char **idBuf, size_t *idBufLen ) {

	if( sealBuf ) {
		struct random_context *signEntropy = (struct random_context *)DequeLink( &l.plqCrypters );
		char *fileKey;
		size_t keyLen;
		uint8_t outbuf[32];

		if( !signEntropy )
			signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );

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

		idBuf[0] = EncodeBase64Ex( outbuf, 256 / 8, idBufLen, (const char *)1 );

		EnqueLink( &l.plqCrypters, signEntropy );
		return fileKey;
	}
	else {
		idBuf[0] = SRG_ID_Generator3();
		idBufLen[0] = 42;
		return idBuf[0];
	}
}


int CPROC sack_vfs_os_exists( struct sack_vfs_os_volume *vol, const char * file ) {
	LOGICAL result;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	result = _os_ScanDirectory( vol, file, FIRST_DIR_BLOCK, NULL, NULL, 0 );
	vol->lock = 0;
	return result;
}

size_t CPROC sack_vfs_os_tell( struct sack_vfs_os_file *file ) { return (size_t)file->fpi; }

size_t CPROC sack_vfs_os_size( struct sack_vfs_os_file *file ) {	return (size_t)(file->filesize_); }

size_t CPROC sack_vfs_os_seek_internal( struct sack_vfs_os_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->filesize_  ) + pos;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	{
		if( file->fpi >= old_fpi ) {
			FPI dist = file->fpi - old_fpi;
			if( dist > 4096 ) {
				do {
					int blockSize;
					if( file->fpi <= old_fpi ) {
						file->vol->lock = 0;
						return (size_t)file->fpi; // block in file can accept data now...
					}
					enum block_cache_entries gnbCache = BC( FILE );
					file->block = vfs_os_GetNextBlock( file->vol, file->block, &gnbCache, GFB_INIT_NONE
						, TRUE
						, dist>4096?4096:dist<2048? BLOCK_SMALL_SIZE :4096
						, &blockSize );
					old_fpi += blockSize;
				} while( 1 );
			}
		}
	}
	{
		size_t n = 0;
		BLOCKINDEX b = file->_first_block;///aa
		while( n < ( pos ) ) {
			enum block_cache_entries cache;
			int bs;
			size_t dist = pos-n;
			cache = BC( FILE );
			if( b == DIR_ALLOCATING_MARK )
				file->entry->first_block
					= file->_first_block
					= b
					= _os_GetFreeBlock( file->vol, &cache, GFB_INIT_NONE, bs=(dist > 4096 ? 4096 : dist < 2048 ? BLOCK_SMALL_SIZE : 4096) );
			else
				b = vfs_os_GetNextBlock( file->vol, b, &cache, GFB_INIT_NONE, TRUE, dist>4096?4096:dist<2048? BLOCK_SMALL_SIZE :4096, &bs );
			n += bs;
		}
		file->block = b;
	}
	file->vol->lock = 0;
	return (size_t)file->fpi;
}

size_t CPROC sack_vfs_os_seek( struct sack_vfs_os_file* file, size_t pos, int whence ) {
	return sack_vfs_os_seek_internal( (struct sack_vfs_os_file*) file, pos, whence );
}


size_t CPROC sack_vfs_os_write_internal( struct sack_vfs_os_file* file, const void* data_, size_t length
		, POINTER writeState ) {
	const char* data = (const char*)data_;
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	LOGICAL updated = FALSE;

#ifdef DEBUG_DISK_DATA
	lprintf( "Write to %p %d at %d", data_, length, file->fpi );
	LogBinary( data, file->blockSize );
#endif

#ifdef XX_VIRTUAL_OBJECT_STORE
	uint8_t* cdata;
	size_t cdataLen;
	if( file->readKey && !file->fpi ) {
		enum block_cache_entries cache;
		struct storageTimelineNode* time = getRawTimeEntry( file->vol, file->entry->timelineEntry, &cache GRTENoLog DBG_SRC );
		SRG_XSWS_encryptData( (uint8_t*)data, length, time->time
			, (const uint8_t*)file->readKey, file->readKeyLen
			, &cdata, &cdataLen );
		dropRawTimeEntry( file->vol, cache GRTENoLog DBG_SRC );
		data = (const char*)cdata;
		length = cdataLen;
	}
	else {
		cdata = NULL;
	}
#endif
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	if( file->entry->first_block != DIR_ALLOCATING_MARK )
		if( file->flags.versioned )
		{
			// if versioned, but no limit, just do this.
			if( file->entry->name_offset & DIRENT_NAME_OFFSET_VERSIONS ) {
				// if there's a limit to the number of versions

			}

			{
				int last = file->blockChainLength - 1;
				enum block_cache_entries cache;
				struct storageTimelineNode* timeline = getRawTimeEntry( file->vol, file->entry->timelineEntry, &cache GRTENoLog DBG_SRC );
				timeline->priorDataPad = (uint16_t)( file->blockChain[last].size - ( file->entry->filesize & ( file->blockChain[last].size - 1 ) ) );
				//timeline->priorData = file->entry->first_block;
				timeline->priorData = file->entry->first_block;
				timeline->priorDataSize = file->entry->filesize;
				file->entry->first_block = DIR_ALLOCATING_MARK;
				file->entry->filesize = 0;
				file->filesize_ = 0;
				file->blockChainLength = 0;
				dropRawTimeEntry( file->vol, cache GRTENoLog DBG_SRC );
			}
			//lprintf( "this needs to result with the new timestamp" );
			//file->entry->timelineEntry = file->entry->timelineEntry;
			updated = TRUE;
		} else {
			// no versioning - so just keep 1 block so we get last write and first create
			if( !( file->entry->name_offset & DIRENT_NAME_OFFSET_VERSIONS ) ) {
				// don't have a new time block for write time; so create one
				file->entry->timelineEntry = updateTimeEntryTime( NULL, file->vol, file->entry->timelineEntry, TRUE, NULL, 0 DBG_SRC );
				file->entry->name_offset |= 1 << DIRENT_NAME_OFFSET_VERSION_SHIFT;
			}
			else {
				// update the current time.
				file->entry->timelineEntry = updateTimeEntryTime( NULL, file->vol, file->entry->timelineEntry, FALSE, NULL, 0 DBG_SRC );
			}
			//file->entry->timelineEntry = file->timeline.index;
			updated = TRUE;
		}

#ifdef XX_VIRTUAL_OBJECT_STORE

	if( (file->entry->name_offset) & DIRENT_NAME_OFFSET_FLAG_SEALANT ) {
		char* filename;
		size_t filenameLen = 64;
		// read-only data block.
		lprintf( "INCOMPLETE - TODO WRITE PATCH" );
		char* sealer = getFilename( data, length, (char*)file->sealant, file->header.sealant.used, IS_OWNED( file ), &filename, &filenameLen );

		struct sack_vfs_os_file* pFile = (struct sack_vfs_os_file*)sack_vfs_os_openfile( file->vol, filename );
		pFile->sealant = (uint8_t*)sealer;
		if( cdata ) Release( cdata );
		return sack_vfs_os_write_internal( pFile, data, length, (POINTER)1 );
	}
#endif
#ifdef DEBUG_FILE_OPS
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
#endif
	if( ofs ) {
		enum block_cache_entries cache = BC( FILE );
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, length > 4096 ? 4096 : length < 2048 ? BLOCK_SMALL_SIZE : 4096, &cache );
		int blockSize = file->vol->sector_size[cache];

		if( length >= (blockSize - (ofs)) ) {
			memcpy( block + ofs, data, blockSize-ofs );
			SMUDGECACHE( file->vol, cache );
			data += blockSize - ofs;
			written += blockSize - ofs;
			file->fpi += blockSize - ofs;
			if( file->fpi > (file->entry->filesize) ) {
				file->filesize_ = file->entry->filesize = file->fpi;
				updated = TRUE;
			}

			length -= blockSize - ofs;
			cache = BC( FILE );
			// the following code is never run anyway, it's always a get next block...
			if( file->block == DIR_ALLOCATING_MARK )
				file->entry->first_block
				= file->_first_block
				= file->block
				= _os_GetFreeBlock( file->vol, &cache, GFB_INIT_NONE, length > 4096 ? 4096 : length < 2048 ? BLOCK_SMALL_SIZE : 4096 );
			else
				file->block = vfs_os_GetNextBlock( file->vol, file->block, &cache, GFB_INIT_NONE, TRUE
					,
#ifdef XX_VIRTUAL_OBJECT_STORE
					file->blockSize
						? file->blockSize
						:
#endif
					(length>4096)?4096:length<2048? BLOCK_SMALL_SIZE :4096, (int*)&blockSize );
		}
		else {
			memcpy( block+ofs, data, length );
			SMUDGECACHE( file->vol, cache );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize) ) {
				file->filesize_ = file->entry->filesize = file->fpi;
				updated = TRUE;
			}
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BC( FILE );
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block,
			/*
#ifdef XX_VIRTUAL_OBJECT_STORE
			file->blockSize
			? file->blockSize
			:
#endif
			*/
			length > 4096 ? 4096 : length < 2048 ? BLOCK_SMALL_SIZE : 4096, &cache );
		unsigned int blockSize = file->vol->sector_size[cache];
		if( file->block == DIR_ALLOCATING_MARK ) {
			updated = TRUE;  // directy now has a real block.
			// this are data blocks...
			file->block = file->entry->first_block = (
				( file->vol->segment[cache] - 2 ) / BLOCKS_PER_SECTOR ) * BLOCKS_PER_BAT
				+ ( ( file->vol->segment[cache] - 1 - ( 1+ ( file->vol->segment[cache] - 1 ) / BLOCKS_PER_SECTOR ) ) % BLOCKS_PER_BAT );
				//- 1 /* minus first BAT */;
			//lprintf( "computed new file block %d from %d", (int)file->block, (int)file->vol->segment[cache] );
		}

#ifdef _DEBUG
		if( file->block < 2 ) DebugBreak();
#endif
		if( length >= blockSize ) {
			memcpy( block, data, blockSize );
			SMUDGECACHE( file->vol, cache );
			data += blockSize;
			written += blockSize;
			file->fpi += blockSize;
			if( file->fpi > ( file->entry->filesize  ) ) {
				updated = TRUE;
				file->filesize_ = file->entry->filesize = file->fpi ;
			}
			length -= blockSize;
			cache = BC( FILE );
			file->block = vfs_os_GetNextBlock( file->vol, file->block, &cache, GFB_INIT_NONE, TRUE
				,
#ifdef XX_VIRTUAL_OBJECT_STORE
				file->blockSize
				? file->blockSize
				:
#endif
				(length>4096)?4096:length<2048?BLOCK_SMALL_SIZE:4096, (int*)&blockSize );
		}
		else {
			memcpy( block, data, length );
			SMUDGECACHE( file->vol, cache );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize ) ) {
				updated = TRUE;
				file->filesize_ = file->entry->filesize = file->fpi ;
			}
			length = 0;
		}
	}

#if 0
	if( !writeState && file->sealant && (void*)file->sealant != (void*)data ) {
		flushFileSuffix( file );

		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		sack_vfs_os_write_internal( file, (char*)file->sealant, file->header.sealant.used, (POINTER)1 );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
	}
#endif
	if( updated ) {
		SMUDGECACHE( file->vol, file->cache );
	}
	//if( cdata ) Release( cdata );
	//if( !writeState )
	file->vol->lock = 0;
	return written;
}

size_t CPROC sack_vfs_os_write( struct sack_vfs_os_file *file, const void * data_, size_t length ) {
	return sack_vfs_os_write_internal( (struct sack_vfs_os_file* )file, data_, length, NULL );
}

#ifdef XX_VIRTUAL_OBJECT_STORE
static enum sack_vfs_os_seal_states ValidateSeal( struct sack_vfs_os_file *file, char *data, size_t length ) {
	BLOCKINDEX offset = (file->entry->name_offset );
	uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
	struct random_context *signEntropy;// = (struct random_context *)DequeLink( &signingEntropies );
	uint8_t outbuf[32];

	signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->header.sealant.used );
	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
	if( (file->header.sealant.used != 32) || MemCmp( outbuf, file->sealant, 32 ) )
		return SACK_VFS_OS_SEAL_INVALID;

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)data, length );

	// DO NOT DOUBLE_PROCESS THIS DATA
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->header.sealant.used );

	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

	SRG_DestroyEntropy( &signEntropy );
	{
		enum sack_vfs_os_seal_states success = SACK_VFS_OS_SEAL_INVALID;
		size_t len;
		char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
		//if( StrCmp( file->filename, rid ) == 0 )
		//	success = SACK_VFS_OS_SEAL_VALID;
		Deallocate( char *, rid );
		return success;
	}
}
#endif

size_t CPROC sack_vfs_os_read_internal( struct sack_vfs_os_file *file, uint64_t version, void * data_, size_t length ) {
	char* data = (char*)data_;
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
#ifdef XX_VIRTUAL_OBJECT_STORE
	if( (file->entry->name_offset ) & DIRENT_NAME_OFFSET_FLAG_READ_KEYED ) {
		if( !file->readKey ) return 0;
	}
#endif
	if( ( file->filesize_ ) < ( file->fpi + length ) ) {
		if( ( file->filesize_ ) < file->fpi )
			length = 0;
		else
			length = (size_t)(( file->filesize_ ) - file->fpi);
	}
	if( !length || file->block == EOFBLOCK ) {  return 0; }

	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, 0, &cache );
		int blockSize = file->vol->sector_size[cache];
		if( length >= ( blockSize - ( ofs ) ) ) {
			memcpy( data, block+ofs, blockSize-ofs );
			written += blockSize - ofs;
			data += blockSize - ofs;
			length -= blockSize - ofs;
			file->fpi += blockSize - ofs;
			cache = BC( FILE );
			file->block = vfs_os_GetNextBlock( file->vol, file->block, &cache, GFB_INIT_NONE, FALSE, 0, &blockSize );
#ifdef _DEBUG
			if( file->block == EOFBLOCK ) {
				lprintf( "bad read - file data too short" );
				DebugBreak();
			}
#endif
		} else {
			memcpy( data, block + ofs, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, 0, &cache );
		unsigned int blockSize = file->vol->sector_size[cache];
		if( length >= blockSize ) {
			memcpy( data, block, blockSize - ofs );
			written += blockSize;
			data += blockSize;
			length -= blockSize;
			file->fpi += blockSize;
			cache = BC( FILE );
			file->block = vfs_os_GetNextBlock( file->vol, file->block, &cache, GFB_INIT_NONE, FALSE, 0, (int*)&blockSize );
#ifdef _DEBUG
			if( file->block == EOFBLOCK ) {
				lprintf( "bad read - file data too short" );
				DebugBreak();
			}
#endif
		} else {
			memcpy( data, block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
#ifdef XX_VIRTUAL_OBJECT_STORE
	if( file->readKey
	   && ( file->fpi == ( file->filesize_ ) ) //entry->filesize ) )
	   && ( (file->entry->name_offset)
	      & DIRENT_NAME_OFFSET_FLAG_READ_KEYED) )
	{
		uint8_t *outbuf;
		size_t outlen;
		enum block_cache_entries cache;
		struct storageTimelineNode* time = getRawTimeEntry( file->vol, file->entry->timelineEntry, &cache GRTENoLog DBG_SRC );
		SRG_XSWS_decryptData( (uint8_t*)data, written, time->time
		                    , (const uint8_t*)file->readKey, file->readKeyLen
		                    , &outbuf, &outlen );
		dropRawTimeEntry( file->vol, cache GRTENoLog DBG_SRC );
		memcpy( data, outbuf, outlen );
		Release( outbuf );
		written = outlen;
	}

	if( file->sealant
		&& (void*)file->sealant != (void*)data
		&& length == ( file->filesize_ ) ) { //file->entry->filesize ) ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		file->entry->filesize = ((file->entry->filesize
			) + file->header.sealant.used + sizeof( BLOCKINDEX ))
			;
		sack_vfs_os_read_internal( file, 0, (char*)file->sealant, file->header.sealant.used );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = ValidateSeal( file, data, length );
	}
#endif
	return written;
}

size_t CPROC sack_vfs_os_read( struct sack_vfs_os_file* file, void* data_, size_t length ) {
	size_t result;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	result = sack_vfs_os_read_internal( (struct sack_vfs_os_file*)file, 0, data_, length );
	file->vol->lock = 0;
	return result;
}

#ifdef XX_VIRTUAL_OBJECT_STORE

static BLOCKINDEX sack_vfs_os_read_patches( struct sack_vfs_os_file *file ) {
	size_t written = 0;
	BLOCKINDEX saveFpi = file->fpi;
	size_t length;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	length = (size_t)(file->filesize_ );//entry->filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

#if 0
	if( file->sealant ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX patches;

		//WriteIntoBlock( file, 0, 0, file->sealant, file->header.sealant.used );

		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = SACK_VFS_OS_SEAL_LOAD;
		return patches;
	}
#endif
	file->vol->lock = 0;
	return written;
}

static size_t sack_vfs_os_set_patch_block( struct sack_vfs_os_file *file, BLOCKINDEX patchBlock ) {
	size_t written = 0;
	size_t length;
	BLOCKINDEX saveFpi = file->fpi;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	length = (size_t)(file->entry->filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

	if( file->header.sealant.avail ) {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	} else {
		BLOCKINDEX saveSize = file->entry->filesize;
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	file->vol->lock = 0;
	return written;
}

static size_t sack_vfs_os_set_reference_block( struct sack_vfs_os_file *file, BLOCKINDEX patchBlock ) {
	size_t written = 0;
	size_t length;
	BLOCKINDEX saveFpi = file->fpi;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	length = (size_t)(file->entry->filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

	if( file->sealant ) {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	else {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	file->vol->lock = 0;
	return written;
}
#endif

static void sack_vfs_os_unlink_file_entry( struct sack_vfs_os_volume *vol, struct sack_vfs_os_file *dirinfo, BLOCKINDEX first_block, LOGICAL deleted ) {
	//FPI entFPI, struct directory_entry *entry, struct directory_entry *entkey
	BLOCKINDEX block, _block;
	struct sack_vfs_os_file *file_found = NULL;
	struct sack_vfs_os_file *file;
	INDEX idx;
	LIST_FORALL( vol->files, idx, struct sack_vfs_os_file *, file ) {
		if( file->_first_block == first_block ) {
			file_found = file;
			file->delete_on_close = TRUE;
		}
	}
	if( !deleted ) {
		// delete the file entry now; this disk entry may be reused immediately.
		dirinfo->entry_.first_block = dirinfo->_first_block;
		dirinfo->_first_block = dirinfo->entry->first_block = 0;
		SMUDGECACHE( vol, dirinfo->cache );
	}

	if( !file_found ) {
		_block = block = first_block;
#ifdef DEBUG_DIRECTORIES
		LoG( "(marking physical deleted (again?)) entry starts at %d", block );
#endif
		// wipe out file chain BAT
		if( first_block != DIR_ALLOCATING_MARK )
			do {
				enum block_cache_entries cache = BC(BAT);
				enum block_cache_entries fileCache = BC(DATAKEY);
				BLOCKINDEX *this_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, ( ( block / BLOCKS_PER_BAT ) * ( BLOCKS_PER_SECTOR ) ), 0, &cache );
				uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( vol, block, 0, &fileCache );
				//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
				memset( blockData, 0, vol->sector_size[fileCache] );
				// after seek, block was read, and file position updated.
				SMUDGECACHE( vol, fileCache );
				SMUDGECACHE( vol, cache );

				//lprintf( "clear block %d %d %d ", (int)block, (int)( block% BLOCKS_PER_BAT ), (int)(block/BLOCKS_PER_BAT) );
				enum block_cache_entries gnbCache = BC( ZERO );

				block = vfs_os_GetNextBlock( vol, block, &gnbCache, GFB_INIT_NONE, FALSE, vol->sector_size[fileCache], NULL );
				this_BAT[_block % BLOCKS_PER_BAT] = 0;
				if( vol->sector_size[fileCache] == BLOCK_SMALL_SIZE )
					AddDataItem( &vol->pdlFreeSmallBlocks, &_block );
				else
					AddDataItem( &vol->pdlFreeBlocks, &_block );
				LoG( "unlink storing free block:%d", _block );
				_block = block;
			} while( block != EOFBLOCK );
			// this deletes the allocated name
			// it also removes the directory entry from list of entries
			deleteTimelineIndex( vol, (BLOCKINDEX)dirinfo->entry->timelineEntry ); // timelineEntry type is larger than index in some configurations; but won't exceed those bounds
			deleteDirectoryEntryName( vol, dirinfo, dirinfo->entry->name_offset & DIRENT_NAME_OFFSET_OFFSET, dirinfo->cache, dirinfo->dir_block );

	}
}

static void _os_shrinkBAT( struct sack_vfs_os_file *file ) {
	struct sack_vfs_os_volume *vol = file->vol;
	BLOCKINDEX block, _block;
	size_t bsize = 0;
	int smallBlocks = 0;
	int nBlock = 0;
	if( file->entry->first_block == EOFBLOCK ) return;  // no data blocks already.
	_block = block = file->entry->first_block;
	do {
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries data_cache = BC( FILE );
		//lprintf( " block %d %d %d ", (int)block, (int)( block % BLOCKS_PER_BAT ), (int)( block / BLOCKS_PER_BAT ) );
		BLOCKINDEX *this_BAT = (BLOCKINDEX*)vfs_os_block_index_SEEK( vol, ( ( block / BLOCKS_PER_BAT ) * ( BLOCKS_PER_SECTOR) ), 0, &cache );
		if( !this_BAT[block % BLOCKS_PER_BAT] ) {
			lprintf( "This file is already deleted..." );
			return;
		}
		enum block_cache_entries gnbCache = BC( FILE );
		block = vfs_os_GetNextBlock( vol, block, &gnbCache, GFB_INIT_NONE, FALSE, BAT_BLOCK_SIZE, NULL );
		if( bsize >= (file->entry->filesize) ) {
#ifdef VFS_OS_PARANOID_TRUNCATE
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, 0, &data_cache );
			memset( blockData, 0, file->vol->sector_size[data_cache] );
			//LoG( "clearing a datablock after a file..." );
#endif
			//lprintf( "Should be able to unlink this... extra block of data %d", (int)_block );
			if( vol->sector_size[data_cache] == BLOCK_SMALL_SIZE )
				AddDataItem( &vol->pdlFreeSmallBlocks, &_block );
			else
				AddDataItem( &vol->pdlFreeBlocks, &_block );
#ifdef DEBUG_FILE_TRUNCATE
			LoG( "shrink storing free block:%d", _block );
#endif
			this_BAT[_block % BLOCKS_PER_BAT] = 0;
		} else {
			if( this_BAT[BLOCKS_PER_BAT] ) {
				smallBlocks++;
				bsize += BLOCK_SMALL_SIZE;
			} else
				bsize += 4096;
			if( bsize > (file->entry->filesize) ) {
				uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, 0, &data_cache );
				int blockSize = file->vol->sector_size[data_cache];
				//LoG( "clearing a partial datablock after a file..., %d, %d", blockSize-(file->entry->filesize & (blockSize-1)), ( file->entry->filesize & (blockSize-1)) );
#ifdef VFS_OS_PARANOID_TRUNCATE
				memset( blockData + (file->entry->filesize & (blockSize-1)), 0, blockSize-(file->entry->filesize & (blockSize-1)) );
#endif
				//this_BAT[_block % BLOCKS_PER_BAT] = 0;
			}
			else if( file->entry->filesize )
				nBlock++;
		}
		_block = block;
	} while( block != EOFBLOCK );

	if( !file->entry->filesize ) {
		file->_first_block = file->block = file->entry->first_block = EOFBLOCK;
#ifdef DEBUG_FILE_TRUNCATE
		LoG( "Truncated file block chain length is now:%d", nBlock );
#endif
		file->blockChainLength = nBlock;
	}
	if( smallBlocks > ( 4096 / BLOCK_SMALL_SIZE ) * 2 ) {
		lprintf( "File has lots of fragments, consider defragmenting its small blocks" );
	}
}

size_t CPROC sack_vfs_os_truncate_internal( struct sack_vfs_os_file *file ) {
	if( file->entry->filesize != file->fpi ) {
		file->filesize_ = file->entry->filesize = file->fpi;
		_os_shrinkBAT( file );
		SMUDGECACHE( file->vol, file->cache );
	}
	return (size_t)file->fpi;
}

size_t CPROC sack_vfs_os_truncate( struct sack_vfs_os_file* file ) {
	size_t result;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	result = sack_vfs_os_truncate_internal( (struct sack_vfs_os_file*)file );
	file->vol->lock = 0;
	return result;
}

int sack_vfs_os_close_internal( struct sack_vfs_os_file *file, int unlock ) {
#ifdef DEBUG_TRACE_LOG
#  ifdef DEBUG_FILE_OPS
	{
		enum block_cache_entries cache = BC(NAMES);
		static char fname[256];
		FPI name_ofs = file->entry_.name_offset;
		// this following line needs to be updated.
		//FPI base = (const char *)
		//char const *filename = (char const *)vfs_os_DSEEK( file->vol, name_ofs, 0, &cache ); // have to do the seek to the name block otherwise it might not be loaded.
		_os_MaskStrCpy( fname, sizeof( fname ), file->vol, cache, name_ofs );
		LoG( "close file:%s(%p)", fname, file );
	}
#  endif
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
		if( !testFile ) {
			int locks = GETMASK_( file->vol->seglock, seglock, file->cache );
			if( !locks ) {
				lprintf( "Underflow file locks... " );
				DebugBreak();
			}
			locks--;
			SETMASK_( file->vol->seglock, seglock, file->cache, locks );
		}
	}
	//Deallocate( char *, file->filename );
#ifdef XX_VIRTUAL_OBJECT_STORE
	if( file->sealant )
		Deallocate( uint8_t*, file->sealant );
#endif
	if( file->vol->closed ) sack_vfs_os_unload_volume( file->vol );
	if( unlock ) file->vol->lock = 0;
	DeleteFromSet( VFS_OS_FILE, &l.files, file );
	//Deallocate( struct sack_vfs_os_file *, file );
	return 0;
}
int CPROC sack_vfs_os_close( struct sack_vfs_os_file* file ) {
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	int status = sack_vfs_os_close_internal( (struct sack_vfs_os_file*) file, TRUE );
	return status;
}

int CPROC sack_vfs_os_unlink_file( struct sack_vfs_os_volume *vol, const char * filename ) {
	int result = 0;
	struct sack_vfs_os_file tmp_dirinfo;
	if( !vol ) return 0;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
#ifdef DEBUG_DIRECTORIES
	LoG( "unlink file:%s", filename );
#endif
	if( _os_ScanDirectory( vol, filename, FIRST_DIR_BLOCK, NULL, &tmp_dirinfo, 0 ) ) {
		sack_vfs_os_unlink_file_entry( vol, &tmp_dirinfo, tmp_dirinfo.entry->first_block, FALSE );
		{
			int locks = GETMASK_( vol->seglock, seglock, tmp_dirinfo.cache );
			if( !locks ) {
				lprintf( "File locks underflowed" );
				DebugBreak();
			}
			else locks--;
			SETMASK_( vol->seglock, seglock, tmp_dirinfo.cache, locks );
		}

		result = 1;
	}
	vol->lock = 0;
	return result;
}

int CPROC sack_vfs_os_flush( struct sack_vfs_os_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_os_need_copy_write( void ) {	return FALSE; }


struct sack_vfs_os_find_info * CPROC sack_vfs_os_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask )
{
	struct sack_vfs_os_find_info *info = New( struct sack_vfs_os_find_info );
	info->pds_directories = CreateDataStack( sizeof( struct hashnode ) );
	info->base = base;
	info->base_len = StrLen( base );
	info->mask = mask;
	info->vol = (struct sack_vfs_os_volume *)psvInst;
	info->leadinDepth = 0;
	return (struct sack_vfs_os_find_info*)info;
}


static int _os_iterate_find( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	struct directory_hash_lookup_block *dirBlock;
	struct directory_entry *next_entries;
	int n;
	do
	{
		enum block_cache_entries cache = BC(DIRECTORY);
		enum block_cache_entries name_cache = BC(NAMES);
		struct hashnode node = ((struct hashnode *)PopData( &info->pds_directories ))[0];
		dirBlock = BTSEEK( struct directory_hash_lookup_block *, info->vol, node.this_dir_block, 0, cache );

		if( !node.thisent ) {
			struct hashnode subnode;
			subnode.thisent = 0;
			for( n = 254; n >= 0; n-- ) {
				BLOCKINDEX block = dirBlock->next_block[n];
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
		for( n = (int)node.thisent; n < (dirBlock->used_names ); n++ ) {
			FPI name_ofs = ( next_entries[n].name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
			FPI name_ofs_ = name_ofs;
			const char *filename, *filename_;
			int l;

			struct memoryTimelineNode time;
			enum block_cache_entries  timeCache = BC( TIMELINE );
			reloadTimeEntry( &time, info->vol, (next_entries[n].timelineEntry) VTReadWrite GRTENoLog  DBG_SRC );
			if( !time.disk->time ) DebugBreak();
			if( time.disk->priorTime )
			{
				enum block_cache_entries cache;
				struct storageTimelineNode* prior = getRawTimeEntry( info->vol, time.disk->priorTime, &cache GRTENoLog DBG_SRC );
				while( prior->priorTime ) prior = getRawTimeEntry( info->vol, prior->priorTime, &cache GRTENoLog DBG_SRC );
				info->ctime = (prior->time/1000000)<<8 | prior->timeTz;
			}
			else
				info->ctime = (time.disk->time / 1000000) << 8 | time.disk->timeTz;
			info->wtime = (time.disk->time / 1000000) << 8 | time.disk->timeTz;
			dropRawTimeEntry( info->vol, time.diskCache GRTENoLog DBG_SRC );

			// if file is deleted; don't check it's name.
			info->filesize = (size_t)(next_entries[n].filesize);
			if( (name_ofs) > info->vol->dwSize ) {
				lprintf( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
			filename_ = filename;
			info->filenamelen = 0;
			for( l = 0; l < node.leadinDepth; l++ ) info->filename[info->filenamelen++] = node.leadin[l];

			if( info->vol->key ) {
				int c;
				do {
					while( ((name_ofs & ~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK))
						&& (((c = ((uint8_t*)filename)[0] )) != UTF8_EOT)
						) {
						info->filename[info->filenamelen++] = c;
						filename++;
						name_ofs++;
					}
					if( ((name_ofs & ~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK)) ) {
						name_cache = BC( NAMES );
						filename = (const char *)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
						name_ofs_ = name_ofs;
						continue;
					}
					break;
				} while( 1 );
				info->filename[info->filenamelen]	 = 0;
				//LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			} else {
				int c;
				do {
					while(
						((name_ofs&~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK))
						&& ((c = (((uint8_t*)filename)[0])) != UTF8_EOT)
						) {
						info->filename[info->filenamelen++] = c;
						filename_ = filename;
						filename++;
						name_ofs++;
					}
					if( ((((uintptr_t)filename)&~BLOCK_MASK) != (((uintptr_t)filename_)&~BLOCK_MASK)) ) {
						name_cache = BC( NAMES );
						filename = (const char*)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block, name_ofs, &name_cache, NAME_BLOCK_SIZE DBG_SRC );
						name_ofs_ = name_ofs;
						continue;
					}
					break;
				}
				while( 1 );
				info->filename[info->filenamelen] = 0;
#ifdef DEBUG_FILE_SCAN
				LoG( "Scan return filename: %s", info->filename );
#endif
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

int CPROC sack_vfs_os_find_first( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	struct hashnode root;
	root.this_dir_block = 0;
	root.leadinDepth = 0;
	root.thisent = 0;
	PushData( &info->pds_directories, &root );
	//info->thisent = 0;
	return _os_iterate_find( _info );
}

int CPROC sack_vfs_os_find_close( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	Deallocate( struct sack_vfs_os_find_info*, info ); return 0; }
int CPROC sack_vfs_os_find_next( struct sack_vfs_os_find_info *_info ) { return _os_iterate_find( _info ); }
char * CPROC sack_vfs_os_find_get_name( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	return info->filename; }
size_t CPROC sack_vfs_os_find_get_size( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	return info->filesize; }
LOGICAL CPROC sack_vfs_os_find_is_directory( struct sack_vfs_os_find_info *cursor ) { return FALSE; }
LOGICAL CPROC sack_vfs_os_is_directory( uintptr_t psvInstance, const char *path ) {
	if( path[0] == '.' && path[1] == 0 ) return TRUE;
	{
		struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psvInstance;
		if( _os_ScanDirectory( vol, path, FIRST_DIR_BLOCK, NULL, NULL, 1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}
uint64_t CPROC sack_vfs_os_find_get_ctime( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	if( info ) return info->ctime;
	return 0;
}
uint64_t CPROC sack_vfs_os_find_get_wtime( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	if( info ) return info->wtime;
	return 0;
}

LOGICAL CPROC sack_vfs_os_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psvInstance;
	lprintf( "RENAME IS NOT SUPPORTED IN OBJECT STORAGE(OR NEEDS TO BE FIXED)" );
	// fail if the names are the same.
	return TRUE;
}


uintptr_t CPROC sack_vfs_os_file_ioctl_internal( struct sack_vfs_os_file* file, uintptr_t opCode, va_list args ) {
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;
		break;
	case SOSFSFIO_DESTROY_INDEX:
	{
		const char* indexname = va_arg( args, const char* );

		break;
	}
	case SOSFSFIO_CREATE_INDEX:
	{
		const char* indexname = va_arg( args, const char* );
		size_t indexnameLen = va_arg( args, size_t );
		lprintf( "Indexes should be implemented higher..." );
		//enum jsox_value_types type = va_arg( args, enum jsox_value_types );
		//int typeExtra = va_arg( args, int );
		//struct memoryStorageIndex* index = allocateIndex( file, indexname, indexnameLen );
		//file->
		return (uintptr_t)0/*index*/;
		break;
	}
	case SOSFSFIO_ADD_INDEX_ITEM:
	{
		struct memoryStorageIndex* index = (struct memoryStorageIndex*)va_arg( args, uintptr_t );
		struct sack_vfs_os_file *reference = va_arg( args, struct sack_vfs_os_file* );
		struct jsox_value_container * value = va_arg( args, struct jsox_value_container* );

		//file->
		break;
	}
	case SOSFSFIO_REMOVE_INDEX_ITEM:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_ADD_REFERENCE:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_REMOVE_REFERENCE:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_ADD_REFERENCE_BY:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_REMOVE_REFERENCE_BY:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_TAMPERED:
	{
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		int *result = va_arg( args, int* );

#ifdef XX_VIRTUAL_OBJECT_STORE
		if( file->sealant ) {
			switch( file->sealed ) {
			case SACK_VFS_OS_SEAL_STORE:
			case SACK_VFS_OS_SEAL_VALID:
				(*result) = 1;
            break;
			default:
				(*result) = 0;
			}
		}
		else
			(*result) = 1;
#endif
	}
	break;
	case SOSFSFIO_PROVIDE_SEALANT:
#ifdef XX_VIRTUAL_OBJECT_STORE
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		lprintf( "This should be a higher level thing." );
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->sealant )
				Release( file->sealant );
			file->sealant = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)len );
			//_os_UpdateFileBlocks( file );
			if( file->sealed == SACK_VFS_OS_SEAL_NONE )
				file->sealed = SACK_VFS_OS_SEAL_STORE;
			else if( file->sealed == SACK_VFS_OS_SEAL_VALID || file->sealed == SACK_VFS_OS_SEAL_LOAD )
				file->sealed = SACK_VFS_OS_SEAL_STORE_PATCH;
			else
				lprintf( "Unhandled SEAL state." );
			//file->sealant = sealant;
			//file->sealantLen = sealantLen;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset)
				| ((len >> 2) << 17)) );
		}
	}
#endif
	break;
	case SOSFSFIO_PROVIDE_READKEY:
#ifdef XX_VIRTUAL_OBJECT_STORE
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->readKey )
				Release( file->readKey );
			file->readKey = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			file->readKeyLen = (uint16_t)len;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset )
				| DIRENT_NAME_OFFSET_FLAG_READ_KEYED) );
		}
	}
#endif
	break;
	case SOSFSFIO_SET_TIME:
	{
		uint64_t timestamp = va_arg( args, uint64_t );
		int8_t tz = (uint8_t)va_arg( args, int );
		return sack_vfs_os_set_time( file, timestamp, tz );
	}
	break;
	case SOSFSFIO_GET_TIME:
	{
		uint64_t** timeArray = va_arg( args, uint64_t** );
		int8_t** tzArray = va_arg( args, int8_t** );
		size_t* timeCount  = va_arg( args, size_t* );
		
		return file->entry->timelineEntry;
	}
	break;
	case SOSFSFIO_GET_TIMES:
	{
		uint64_t** timeArray = va_arg( args, uint64_t** );
		int8_t** tzArray = va_arg( args, int8_t** );
		size_t* timeCount  = va_arg( args, size_t* );
		
		return sack_vfs_os_get_times( file, timeArray, tzArray, timeCount );
	}
	break;
	case SOSFSFIO_SET_BLOCKSIZE: // automatic managment is good enough?
	{
		int size = va_arg( args, int );
		//file->blockSize = size;
	}
	break;
	}
	return TRUE;
}

uintptr_t CPROC sack_vfs_os_file_ioctl_interface( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	return sack_vfs_os_file_ioctl_internal( (struct sack_vfs_os_file*)psvInstance, opCode, args );
}
uintptr_t CPROC sack_vfs_os_file_ioctl( struct sack_vfs_os_file *psvInstance, uintptr_t opCode, ... ) {
	va_list args;
	va_start( args, opCode );
	return sack_vfs_os_file_ioctl_internal( (struct sack_vfs_os_file*)psvInstance, opCode, args );
}


uintptr_t CPROC sack_vfs_os_system_ioctl_internal( struct sack_vfs_os_volume *vol, uintptr_t opCode, va_list args ) {
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;

	case SOSFSSIO_OPEN_VERSION:
		{
			const char * name;name = va_arg( args, const char* );
			uint64_t version = va_arg( args, uint64_t );
			return (uintptr_t)sack_vfs_os_openfile_internal( vol, name, version, FALSE );
		}
		break;
	case SOSFSSIO_NEW_VERSION:
		{
			const char * name;name = va_arg( args, const char* );
			return (uintptr_t)sack_vfs_os_openfile_internal( vol, name, 0, TRUE );
		}
		break;
	case SOSFSSIO_OPEN_TIMELINE:
		{
			return (uintptr_t)sack_vfs_os_get_time_cursor( vol );
		}
		break;
	case SOSFSSIO_READ_TIMELINE:
		{
			struct sack_vfs_os_time_cursor* cursor;cursor = va_arg(args, struct sack_vfs_os_time_cursor* );
			int step = va_arg( args, int );
			uint64_t timestamp; timestamp = va_arg( args, uint64_t );
			uint64_t* result_entry; result_entry = va_arg( args, uint64_t* );
			const char ** filename;filename = va_arg( args, const char ** );
			uint64_t* timestamp_result;timestamp_result = va_arg( args, uint64_t* );
			int8_t* tz_result;tz_result = va_arg( args, int8_t* );
			const char** buffer;buffer = va_arg( args, const char** );
			size_t* size_result;size_result = va_arg( args, size_t* );
			sack_vfs_os_read_time_cursor( cursor, step, timestamp, result_entry, filename, timestamp_result, tz_result, buffer, size_result );
			return TRUE;
		}
		break;
	case SOSFSSIO_LOAD_OBJECT:
		return FALSE;

	case SOSFSSIO_PATCH_OBJECT:
		{
		LOGICAL owner;owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key

		char *objIdBuf;objIdBuf = va_arg( args, char * );
		/*
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
		*/
#ifdef XX_VIRTUAL_OBJECT_STORE

		if( sack_vfs_os_exists( vol, objIdBuf ) ) {
			struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, objIdBuf );
			BLOCKINDEX patchBlock = sack_vfs_os_read_patches( file );
			enum block_cache_entries cacheSomething = BC(FILE);
			if( !patchBlock ) {
				patchBlock = _os_GetFreeBlock( vol, &cacheSomething, GFB_INIT_PATCHBLOCK, 4096 );
			}
			{
				enum block_cache_entries cache;
				struct directory_patch_block *newPatchblock;

				cache = BC(FILE);
				newPatchblock = BTSEEK( struct directory_patch_block *, vol, patchBlock, DIR_BLOCK_SIZE, cache );

				while( 1 ) {
					//char objId[45];
					//size_t objIdLen;
					char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, FALSE, &idBuf, &idBufLen );

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
						struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, idBuf );

						//  file->entry_fpi
						newPatchblock->entries[newPatchblock->usedEntries].raw
							= file->entry_fpi;
						newPatchblock->usedEntries = (newPatchblock->usedEntries + 1);
						SMUDGECACHE( vol, cache );

						file->sealant = (uint8_t*)seal;
						_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)strlen( seal ) );
						//_os_UpdateFileBlocks( file );
						sack_vfs_os_seek_internal( file, (size_t)GetBlockStart( file, FILE_BLOCK_DATA ), SEEK_SET );
						sack_vfs_os_write_internal( file, objBuf, objBufLen, 0 );
						sack_vfs_os_close_internal( file, FALSE );
					}
					return TRUE;
				}
			}
		}
#endif
		return FALSE; // object to patch was not found.
	}
	break;
	case SOSFSSIO_STORE_OBJECT:
	#if 0
	{
		LOGICAL owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key
		char *objBuf = va_arg( args, char * );
		size_t objBufLen = va_arg( args, size_t );
		char *objIdBuf = va_arg( args, char * );  // provided for re-write; provided also for private named objects
		size_t objIdBufLen = va_arg( args, size_t );
		char *sealBuf = va_arg( args, char * );  // user provided sealant if any
		size_t sealBufLen = va_arg( args, size_t );
		char *keyBuf = va_arg( args, char * );  // encryption key
		size_t keyBufLen = va_arg( args, size_t );
		char **idBuf = va_arg( args, char ** );  // output buffer
		size_t *idBufLen = va_arg( args, size_t* );
		while( 1 ) {
			char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, owner, idBuf, idBufLen );
			if( sack_vfs_os_exists( vol, idBuf[0] ) ) {
				if( !sealBuf ) { // accidental key collision.
					continue; // try again.
				}
				else {
					// deliberate key collision; and record already exists.
					return TRUE;
				}
			}
			else {
				struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, idBuf[0] );
#ifdef XX_VIRTUAL_OBJECT_STORE
				if( sealBuf ) {
					file->sealant = (uint8_t*)seal;
					_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)strlen( seal ) );
					WriteIntoBlock( file, 0, 0, seal, strlen( seal ) );
					//file->sealantLen = (uint8_t)strlen( seal );
				} else {
					file->sealant = NULL;
					_os_SetSmallBlockUsage( &file->header.sealant, 0 );
					//file->sealantLen = 0;
				}
#endif
				sack_vfs_os_write_internal( file, objBuf, objBufLen, NULL );
				sack_vfs_os_close_internal( file, FALSE );
			}
			return TRUE;
		}

	}
#endif	
	break;
	}
	return 0;
}


uintptr_t CPROC sack_vfs_os_system_ioctl_interface( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	return sack_vfs_os_system_ioctl_internal( (struct sack_vfs_os_volume*)psvInstance, opCode, args );
}
uintptr_t CPROC sack_vfs_os_system_ioctl( struct sack_vfs_os_volume* vol, uintptr_t opCode, ... ) {
	va_list args;
	va_start( args, opCode );
	return sack_vfs_os_system_ioctl_internal( vol, opCode, args );
}


LOGICAL sack_vfs_os_get_times( struct sack_vfs_os_file* file, uint64_t** timeArray, int8_t** tzArray, size_t* timeCount ) {
	if( !timeArray ) return TRUE;
	struct scratchTime {
		uint64_t scratchTime;
		uint8_t scratchTz;
	} scratch;
	PDATALIST pdlTimes = CreateDataList( sizeof( scratch ) );
	struct sack_vfs_os_volume* vol = file->vol;

	struct memoryTimelineNode time;
	enum block_cache_entries  timeCache = BC( TIMELINE );

	reloadTimeEntry( &time, vol, file->entry->timelineEntry VTReadWrite GRTENoLog  DBG_SRC );
	if( !time.disk->time ) DebugBreak();
	scratch.scratchTime = time.disk->time;
	scratch.scratchTz = time.disk->timeTz;
	AddDataItem( &pdlTimes, &scratch );
	if( time.disk->priorTime ) {
		BLOCKINDEX priorTime = time.disk->priorTime;
		while( priorTime ) {
			enum block_cache_entries cache;
			struct storageTimelineNode* prior = getRawTimeEntry( vol, priorTime, &cache GRTENoLog DBG_SRC );
			scratch.scratchTime = prior->time;
			scratch.scratchTz = prior->timeTz;
			priorTime = prior->priorTime;
			dropRawTimeEntry( file->vol, cache GRTENoLog DBG_SRC );
			AddDataItem( &pdlTimes, &scratch );
		}
	}

	dropRawTimeEntry( vol, time.diskCache GRTENoLog DBG_SRC );

	timeArray[0] = NewArray( uint64_t, pdlTimes->Cnt );
	tzArray[0] = NewArray( int8_t, pdlTimes->Cnt );
	{
		struct scratchTime* st;
		INDEX idx;
		DATA_FORALL( pdlTimes, idx, struct scratchTime*, st ) {
			timeArray[0][idx] = st->scratchTime;
			tzArray[0][idx] = st->scratchTz;
		}
	}
	//MemCpy( timeArray[0], pdlTimes->data, pdlTimes->Cnt * sizeof( timeArray[0] ) );
	timeCount[0] = pdlTimes->Cnt;
	DeleteDataList( &pdlTimes );
	return TRUE;

}

LOGICAL sack_vfs_os_set_time( struct sack_vfs_os_file* file, uint64_t timeVal, int8_t tz ) {
	struct sack_vfs_os_volume* vol = file->vol;

	struct memoryTimelineNode time;
	enum block_cache_entries  timeCache = BC( TIMELINE );
	
	reloadTimeEntry( &time, vol, file->entry->timelineEntry VTReadWrite GRTENoLog  DBG_SRC );
	//int tz = timeVal & 0xFF;
	//timeVal = ( timeVal >> 8 ) * 1000000LL;
	return setTimeEntryTime( &time, vol, timeVal, tz );
}

LOGICAL sack_vfs_os_halt( struct sack_vfs_os_volume* volume ) {
	LOGICAL prior = volume->flags.halted;
	volume->flags.halted = TRUE;
	return prior;
}

#ifndef USE_STDIO
static struct file_system_interface sack_vfs_os_fsi = {
                                                     (void*(CPROC*)(uintptr_t,const char *, const char*))sack_vfs_os_open
                                                   , (int(CPROC*)(void*))sack_vfs_os_close
                                                   , (size_t(CPROC*)(void*,void*,size_t))sack_vfs_os_read
                                                   , (size_t(CPROC*)(void*,const void*,size_t))sack_vfs_os_write
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
                                                   , (LOGICAL(CPROC*)(struct find_cursor*))         sack_vfs_os_find_is_directory
                                                   , (LOGICAL(CPROC*)(uintptr_t, const char*))      sack_vfs_os_is_directory
                                                   , (LOGICAL(CPROC*)(uintptr_t, const char*, const char*))sack_vfs_os_rename
                                                   , (uintptr_t(CPROC*)(uintptr_t, uintptr_t, va_list))sack_vfs_os_file_ioctl_interface
												   , (uintptr_t(CPROC*)(uintptr_t, uintptr_t, va_list))sack_vfs_os_system_ioctl_interface
	, (uint64_t( CPROC*)(struct find_cursor* cursor)) sack_vfs_os_find_get_ctime
	, (uint64_t( CPROC* )(struct find_cursor* cursor)) sack_vfs_os_find_get_wtime
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
		struct sack_vfs_os_volume *vol;
		TEXTCHAR volfile[256];
		TEXTSTR tmp;
		SACK_GetProfileString( GetProgramName(), "SACK/VFS/OS File", "*/../assets.os", volfile, 256 );
		tmp = ExpandPath( volfile );
		vol = sack_vfs_os_load_volume( tmp, NULL );
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

#undef SACK_VFS_SOURCE
#undef SACK_VFS_OS_SOURCE
#ifdef _MSC_VER
// integer partial expresions summed into 64 bit.
#  pragma warning( default: 26451 )
#endif
