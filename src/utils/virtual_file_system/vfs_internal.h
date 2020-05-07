#ifndef _MSC_VER
#pragma multiinclude
#endif
/**************
  VFS_VERSION
     used to track migration of keys and keying methods.  
  0x100 = version 1; SHORTKEY_LENGTH = 16
 **************/
#define VFS_VERSION     0x100

// 12 bits = 1 << 12 = 4096
#define BLOCK_SIZE_BITS 12
// BLOCKINDEX is either 4 or 8 bytes... sizeof( size_t )... 
// all constants though should compile out to a single value... and just for grins went to 16 bit size_t and 0 shift... or 1 byte
#define BLOCK_BAT_SHIFT (BLOCK_SIZE_BITS-(sizeof(BLOCKINDEX)==16?4:sizeof(BLOCKINDEX)==8?3:sizeof(BLOCKINDEX)==4?2:sizeof(BLOCKINDEX)==2?1:0) )
#define BLOCK_INDEX_SHIFT ((sizeof(BLOCKINDEX)==16?4:sizeof(BLOCKINDEX)==8?3:sizeof(BLOCKINDEX)==4?2:sizeof(BLOCKINDEX)==2?1:0) )
#define BLOCK_BYTE_SHIFT (BLOCK_SIZE_BITS)
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)


#define BLOCK_SMALL_SIZE     256


#define DIR_BLOCK_SIZE      4096
#define BAT_BLOCK_SIZE      4096
#define NAME_BLOCK_SIZE     4096
#define KEY_SIZE            1024 
#define TIME_BLOCK_SIZE     4096
#define ROLLBACK_BLOCK_SIZE 4096
#define FILE_NAME_MAXLEN    4096

#define BLOCK_MASK (BLOCK_SIZE-1) 
#ifdef VIRTUAL_OBJECT_STORE
// if the block index & BAT_BLOCK_MASK, is a data block
// all BATs are 4096
#  undef BLOCKS_PER_BAT
#  undef BLOCK_SECTOR_MASK
#  define BLOCKS_PER_BAT ((BAT_BLOCK_SIZE >> BLOCK_INDEX_SHIFT)-1)
#  define BLOCK_SECTOR_MASK BLOCKS_PER_BAT
#else
#  undef BLOCKS_PER_BAT
#  undef BLOCK_SECTOR_MASK
#  define BLOCKS_PER_BAT ((BAT_BLOCK_SIZE >> BLOCK_INDEX_SHIFT))
#  define BLOCK_SECTOR_MASK (BLOCKS_PER_BAT-1)
#endif

#define BAT_BLOCK_MASK ( ( BAT_BLOCK_SIZE >> BLOCK_INDEX_SHIFT ) - 1)
#define BLOCKS_PER_SECTOR (1+BLOCKS_PER_BAT)

// per-sector perumation; needs to be a power of 2 (in bytes)
#define SHORTKEY_LENGTH 16

#ifndef VFS_DISK_DATATYPE
#  define VFS_DISK_DATATYPE size_t
#endif

typedef VFS_DISK_DATATYPE BLOCKINDEX; // BLOCK_SIZE blocks...
typedef VFS_DISK_DATATYPE FPI; // file position type

/* BEFORE DEF */

#undef BC
#ifdef VIRTUAL_OBJECT_STORE
/* THIS DEFINES SACK_VFS_OS_VOLUME */
#  define BC(n) BLOCK_CACHE_VOS_##n
#    ifdef block_cache_entries
#      undef block_cache_entries
#      undef directory_entry
#      undef sack_vfs_disk
#      undef sack_vfs_diskSection
#      undef directory_hash_lookup_block
#      undef sack_vfs_volume
#      undef sack_vfs_file
#    endif
#    define block_cache_entries block_cache_entries_os
#    define directory_entry directory_entry_os
#    define sack_vfs_disk sack_vfs_disk_os
#    define sack_vfs_diskSection sack_vfs_diskSection_os
#    define directory_hash_lookup_block directory_hash_lookup_block_os
#    define sack_vfs_volume sack_vfs_os_volume
#    define sack_vfs_file sack_vfs_os_file
#   ifdef __cplusplus
namespace objStore {
#   endif

#elif defined FILE_BASED_VFS
#  define BC(n) BLOCK_CACHE_FS_##n
#    ifdef block_cache_entries
#      undef block_cache_entries
#      undef directory_entry
#      undef sack_vfs_disk
#      undef sack_vfs_diskSection
#      undef directory_hash_lookup_block
#      undef sack_vfs_volume
#      undef sack_vfs_file
#    endif
#    define block_cache_entries block_cache_entries_fs
#    define directory_entry directory_entry_fs
#    define sack_vfs_disk sack_vfs_disk_fs
#    define sack_vfs_diskSection sack_vfs_diskSection_fs
#    define directory_hash_lookup_block directory_hash_lookup_block_fs
/* THIS DEFINES SACK_VS_VOLUME */
#    define sack_vfs_volume sack_vfs_fs_volume
#    define sack_vfs_file sack_vfs_fs_file

#   ifdef __cplusplus
namespace fs {
#   endif

#else
#  define BC(n) BLOCK_CACHE_##n
#endif

/* AFTER DEF */

int sack_vfs_volume;

enum block_cache_entries
{
	BC( ZERO )
	, BC( DIRECTORY ) = 0
#ifdef VIRTUAL_OBJECT_STORE
	, BC( DIRECTORY_LAST ) = BC( DIRECTORY ) + 64
#endif
	, BC( NAMES )
	, BC( NAMES_LAST ) = BC( NAMES ) + 16
	, BC( BAT )
#ifdef VIRTUAL_OBJECT_STORE
	// keep a few tables for cache (file system too?)
	, BC( BAT_LAST ) = BC( BAT ) + 4
#endif
	, BC(DATAKEY)
	, BC(FILE)
	, BC(FILE_LAST) = BC(FILE) + 32
#ifdef VIRTUAL_OBJECT_STORE
	, BC( TIMELINE )
	, BC( TIMELINE_LAST ) = BC( TIMELINE ) + 48
#endif
#if defined( VIRTUAL_OBJECT_STORE )
	// really shouldn't need more than one of these...
	// record
	// 1 - header
	// 0/1 - entry (might be with header)
	// 1 - small/big block journal entry

	// replay
	// 1 - header
	// 0/1 - entry (might be with header)
	// 1 - small/big block journal entry
	// 1 - target disk sector

	, BC( ROLLBACK )
	, BC( ROLLBACK_LAST ) = BC( ROLLBACK ) + 6
#endif
#if defined( VIRTUAL_OBJECT_STORE ) && defined( DEBUG_VALIDATE_TREE )
	, BC( TIMELINE_RO )
	, BC( TIMELINE_RO_LAST ) = BC( TIMELINE_RO ) + 48
#endif
	, BC(COUNT)
};

// could effecitvely be fewer than this
// 82 dirents * 512 byte names = 40000
#define DIRENT_NAME_OFFSET_OFFSET             0x0001FFFF
// (sealant length / 4)  (mulitply by 4 to get real length)
#define DIRENT_NAME_OFFSET_FLAG_SEALANT       0x003E0000
#define DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT 17
#define DIRENT_NAME_OFFSET_FLAG_OWNED         0x00400000
#define DIRENT_NAME_OFFSET_FLAG_READ_KEYED    0x00800000
#define DIRENT_NAME_OFFSET_VERSIONED          0x01000000
#define DIRENT_NAME_OFFSET_VERSION_SHIFT      25
#define DIRENT_NAME_OFFSET_VERSIONS           0x1E000000

#define DIRENT_NAME_OFFSET_UNUSED             0xFE000000



PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	VFS_DISK_DATATYPE filesize;  // how big the file is
#ifdef VIRTUAL_OBJECT_STORE
	uint64_t timelineEntry;  // when the file was created/last written
#endif
} PACKED;

#undef VFS_DIRECTORY_ENTRIES
#ifdef VIRTUAL_OBJECT_STORE
// subtract name has index
// subtrace name index 
#  define VFS_DIRECTORY_ENTRIES ( ( BLOCK_SIZE - ( 2*sizeof(BLOCKINDEX) + 256*sizeof(BLOCKINDEX)) ) /sizeof( struct directory_entry) )
#  define VFS_PATCH_ENTRIES ( ( BLOCK_SIZE ) /sizeof( struct directory_entry) )
#else
#  define VFS_DIRECTORY_ENTRIES ( ( BLOCK_SIZE ) /sizeof( struct directory_entry) )
#  define VFS_PATCH_ENTRIES ( ( BLOCK_SIZE ) /sizeof( struct directory_entry) )
#endif

/*
struct sack_vfs_diskSection
{
	// BAT is at 0 of every BLOCK_SIZE blocks (4097 total)
	// &BAT[0] == itself....
	// BAT[0] == first directory entry (actually next entry; first is always here)
	// BAT[1] == first name entry (actually next name block; first is known as here)
	// bat[BLOCK_SIZE] == NEXT_BAT[0]; NEXT_BAT = BAT + BLOCK_SIZE + 1024*BLOCK_SIZE;
	// bat[8192] == ... ( 0 + ( BLOCK_SIZE + BLOCKS_PER_BAT*BLOCK_SIZE ) * N >> 12 )
	BLOCKINDEX BAT[BLOCKS_PER_BAT];
	//struct directory_entry directory[BLOCK_SIZE/sizeof( struct directory_entry)]; // 256
	//char  names[BLOCK_SIZE/sizeof(char)];
	uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];
};

struct sack_vfs_disk {
	struct sack_vfs_diskSection firstBlock;
	struct sack_vfs_diskSection blocks[];
};
*/

struct sack_vfs_os_BAT_info {
	FPI sectorStart;
	FPI sectorEnd;
	BLOCKINDEX blockStart;
	int size;
};


static int const seglock_mask_size = 4;

#ifdef DEBUG_SECTOR_DIRT
#define SMUDGECACHE(vol,n) { \
	if( !TESTFLAG( vol->dirty, n ) ) {\
		SETFLAG( vol->dirty, n ); \
		lprintf( "set dirty on %d", n); \
	} else {  \
		/*lprintf( "Already dirty on %d", n );*/ \
	}         \
}  
#define CLEANCACHE(vol,n) { \
	lprintf( "reset dirty on %d", n); \
	RESETFLAG( vol->dirty, n ); \
} 
#else
#define SMUDGECACHE(vol,n) { \
   vfs_os_smudge_cache(vol,n);   \
}

#define CLEANCACHE(vol,n) { \
	RESETFLAG( vol->dirty, n ); \
} 
#endif

struct vfs_os_rollback_journal {
	struct sack_vfs_os_file* rollback_file;
	struct sack_vfs_os_file* rollback_journal_file;
	struct sack_vfs_os_file* rollback_small_journal_file;
	PDATALIST pdlPendingRecord;
	BLOCKINDEX nextBlock;
	BLOCKINDEX nextSmallBlock;
	PDATALIST pdlJournaled;
};

#ifdef small
#  undef small
#endif

PREFIX_PACKED struct vfs_os_rollback_entry {
	BLOCKINDEX fileBlock;
	struct {
		uint64_t small : 1;
		uint64_t zero : 1;  // block was full of 0's
	} flags;
	// block size is retrievable when the block is reloadeded to write
} PACKED entries[1];

PREFIX_PACKED struct vfs_os_rollback_header {
	struct {
		uint64_t dirty : 1;
		uint64_t processing : 1;
	} flags;
	BLOCKINDEX journal;  // where the blocks are tracked.
	BLOCKINDEX small_journal; // where small blocks are tracked
	BLOCKINDEX rollbackLength;
	BLOCKINDEX nextBlock;
	BLOCKINDEX nextSmallBlock;
	BLOCKINDEX nextEntry;
	// where this is tracked.
	struct vfs_os_rollback_entry  entries[1];
}PACKED ;


struct sack_vfs_volume {
	const char * volname;
#ifdef FILE_BASED_VFS
	FILE *file;
	struct file_system_mounted_interface *mount;
#else
	struct sack_vfs_disk *disk;
	struct sack_vfs_disk *diskReal; // disk might be offset from diskReal because it's a .exe attached.
#endif
	//uint32_t dirents;  // constant 0
	//uint32_t nameents; // constant 1
	uintptr_t dwSize;
	const char * datakey;  // used for directory signatures
	const char * userkey;
	const char * devkey;
	enum block_cache_entries curseg;
	BLOCKINDEX _segment[BC(COUNT)];// cached segment with usekey[n]
	BLOCKINDEX segment[BC(COUNT)];// associated with usekey[n]
#ifdef VIRTUAL_OBJECT_STORE
	struct vfs_os_rollback_journal journal;
	BLOCKINDEX lastBlock;
	PDATALIST pdl_BAT_information;
	//PDATASTACK pdsCTimeStack;// = CreateDataStack( sizeof( struct memoryTimelineNode ) );
	//PDATASTACK pdsWTimeStack;// = CreateDataStack( sizeof( struct memoryTimelineNode ) );

	struct storageTimeline *timeline; // timeline root
	enum block_cache_entries timelineCache;

	struct storageTimeline *timelineKey; // timeline root key
	struct sack_vfs_os_file *timeline_file;
	struct sack_vfs_os_file* timeline_index_file;
	//struct storageTimelineCursor *timeline_cache;
	MASKSET_( seglock, BC( COUNT ), 4 );  // segment is locked into cache.
	unsigned int sector_size[BC( COUNT )];
#endif

	uint8_t fileCacheAge[BC(FILE_LAST) - BC(FILE)];
#ifdef VIRTUAL_OBJECT_STORE
	uint8_t dirHashCacheAge[BC(DIRECTORY_LAST) - BC(DIRECTORY)];
	uint8_t batHashCacheAge[BC(BAT_LAST) - BC(BAT)];
	uint8_t timelineCacheAge[BC( TIMELINE_LAST ) - BC( TIMELINE )];
	uint8_t rollbackCacheAge[BC( ROLLBACK_LAST ) - BC( ROLLBACK )];
#endif
	uint8_t nameCacheAge[BC(NAMES_LAST) - BC(NAMES)];

	struct random_context *entropy;
	uint8_t* key;  // root of all cached key buffers
#ifdef FILE_BASED_VFS
	uint8_t* oldkey;  // root of all cached key buffers
#endif
#ifndef VIRTUAL_OBJECT_STORE
	uint8_t* segkey;  // allow byte encrypting... key based on sector volume file index
	uint8_t* usekey[BC( COUNT )]; // composite key
#endif
	uint8_t* sigkey;  // signature of executable attached as header
	uint8_t* sigsalt;  // signature of executable attached as header
	size_t sigkeyLength;

#  ifdef FILE_BASED_VFS
	uint8_t* key_buffer;  // root buffer space of all cache blocks
	uint8_t* usekey_buffer[BC(COUNT)]; // data cache blocks
	uint8_t* usekey_buffer_clean[BC(COUNT)]; // duplicate copy of original sector data
	PTHREAD flusher;
	volatile LOGICAL flushing;
	PVARTEXT pvtDeleteBuffer;
#ifdef DEBUG_CACHE_FAULTS
	int cacheRequests[10];
	int cacheFaults[10];
#endif
	FLAGSET( dirty, BC(COUNT) );
	FLAGSET( _dirty, BC( COUNT ) );
	FPI bufferFPI[BC(COUNT)];
#  endif
	BLOCKINDEX lastBatBlock;
	PDATALIST pdlFreeBlocks;
#ifdef VIRTUAL_OBJECT_STORE
	BLOCKINDEX lastBatSmallBlock;
	PDATALIST pdlFreeSmallBlocks;
#endif
	PLIST files; // when reopened file structures need to be updated also...
	LOGICAL read_only;
	LOGICAL external_memory;
	LOGICAL closed;
	volatile uint32_t lock;
#ifdef VFS_IMPLEMENT_FILE_LOCKING
	THREAD_ID locked_thread;
#endif
	uint8_t tmpSalt[16];
	uintptr_t clusterKeyVersion;
};


#if !defined( VIRTUAL_OBJECT_STORE )
struct sack_vfs_file
{
	struct sack_vfs_volume *vol; // which volume this is in
	struct directory_entry dirent_key;
	FPI fpi;
	BLOCKINDEX _first_block;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
	LOGICAL delete_on_close;  // someone already deleted this...
	BLOCKINDEX *blockChain;
	BLOCKINDEX blockChainAvail;
	BLOCKINDEX blockChainLength;

#  ifdef FILE_BASED_VFS
	FPI entry_fpi;  // where to write the directory entry update to
#    ifdef VIRTUAL_OBJECT_STORE
	enum block_cache_entries cache;
	struct memoryTimelineNode *timeline;
	uint8_t *seal;
	uint8_t *sealant;
	uint8_t *readKey;
	uint16_t readKeyLen;
	uint8_t sealantLen;
	uint8_t sealed; // boolean, on read, validates seal.  Defaults to FALSE.
	char *filename;
#    endif
	struct directory_entry _entry;  // has file size within
	struct directory_entry *entry;  // has file size within
#  else
	struct directory_entry *entry;  // has file size within
#  endif

};

#endif

#  undef TSEEK
#  undef BTSEEK
#  ifdef VIRTUAL_OBJECT_STORE
#    define TSEEK(type,v,o,s,c) ((type)vfs_os_SEEK(v,o,s,&c))
#    define BTSEEK(type,v,o,s,c) ((type)vfs_os_BSEEK(v,o,s,&c))
#  elif defined FILE_BASED_VFS
#    define TSEEK(type,v,o,c) ((type)vfs_fs_SEEK(v,o,&c))
#    define BTSEEK(type,v,o,c) ((type)vfs_fs_BSEEK(v,o,&c))
#  else
#    define TSEEK(type,v,o,c) ((type)vfs_SEEK(v,o,&c))
#    define BTSEEK(type,v,o,c) ((type)vfs_BSEEK(v,o,&c))
#  endif


#if defined( __GNUC__ ) && !defined( _WIN32 )
#define HIDDEN __attribute__ ((visibility ("hidden")))
#else
#define HIDDEN
#endif

#if !defined( VIRTUAL_OBJECT_STORE ) && !defined( FILE_BASED_VFS )
  uintptr_t vfs_SEEK( struct sack_vfs_volume* vol, FPI offset, enum block_cache_entries* cache_index ) HIDDEN;
  uintptr_t vfs_BSEEK( struct sack_vfs_volume* vol, BLOCKINDEX block, enum block_cache_entries* cache_index ) HIDDEN;
#elif defined( VIRTUAL_OBJECT_STORE ) 
  uintptr_t vfs_os_SEEK( struct sack_vfs_os_volume* vol, FPI offset, int size, enum block_cache_entries* cache_index ) HIDDEN;
  uintptr_t vfs_os_BSEEK( struct sack_vfs_os_volume* vol, BLOCKINDEX block, int size, enum block_cache_entries* cache_index ) HIDDEN;
#elif defined( FILE_BASED_VFS )
  uintptr_t vfs_fs_SEEK( struct sack_vfs_fs_volume* vol, FPI offset, enum block_cache_entries* cache_index ) HIDDEN;
  uintptr_t vfs_fs_BSEEK( struct sack_vfs_fs_volume* vol, BLOCKINDEX block, enum block_cache_entries* cache_index ) HIDDEN;
#endif

#if defined( VIRTUAL_OBJECT_STORE ) || defined( FILE_BASED_VFS )
#   ifdef __cplusplus
}
using namespace sack::SACK_VFS;
#   endif
#  endif


