#pragma multiinclude

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
#define BLOCK_SHIFT (BLOCK_SIZE_BITS-(sizeof(BLOCKINDEX)==16?4:sizeof(BLOCKINDEX)==8?3:sizeof(BLOCKINDEX)==4?2:sizeof(BLOCKINDEX)==2?1:0) )
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)
#define BLOCK_MASK (BLOCK_SIZE-1) 
#define BLOCKS_PER_BAT (1<<BLOCK_SHIFT)
#define BLOCKS_PER_SECTOR (1 + BLOCKS_PER_BAT)
// per-sector perumation; needs to be a power of 2 (in bytes)
#define SHORTKEY_LENGTH 16

#ifndef VFS_DISK_DATATYPE
#  define VFS_DISK_DATATYPE size_t
#endif

typedef VFS_DISK_DATATYPE BLOCKINDEX; // BLOCK_SIZE blocks...
typedef VFS_DISK_DATATYPE FPI; // file position type

#undef BC
#ifdef VIRTUAL_OBJECT_STORE
#  define BC(n) BLOCK_CACHE_VOS_##n
#  ifndef __cplusplus
#    ifdef block_cache_entries
#      undef block_cache_entires
#      undef directory_entry
#      undef disk
#      undef directory_hash_lookup_block
#      undef volume
#      undef sack_vfs_file
#    endif
#    define block_cache_entries block_cache_entries_os
#    define directory_entry directory_entry_os
#    define disk disk_os
#    define directory_hash_lookup_block directory_hash_lookup_block_os
//#    define volume volume_os
//#    define sack_vfs_file sack_vfs_file_os
#  endif
#elif defined FILE_BASED_VFS
#  define BC(n) BLOCK_CACHE_FS_##n
#  ifndef __cplusplus
#    ifdef block_cache_entries
#      undef block_cache_entires
#      undef directory_entry
#      undef disk
#      undef directory_hash_lookup_block
#      undef volume
#      undef sack_vfs_file
#    endif
#    define block_cache_entries block_cache_entries_fs
#    define directory_entry directory_entry_fs
#    define disk disk_fs
#    define directory_hash_lookup_block directory_hash_lookup_block_fs
//#    define volume volume_fs
//#    define sack_vfs_file sack_vfs_file_fs
#  endif
#else
#  define BC(n) BLOCK_CACHE_##n
#endif


enum block_cache_entries
{
	BC( DIRECTORY )
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
	, BC(FILE_LAST) = BC(FILE) + 10
	, BC(COUNT)
};


PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	VFS_DISK_DATATYPE filesize;  // how big the file is
	//uint32_t filler;  // extra data(unused)
} PACKED;

#undef VFS_DIRECTORY_ENTRIES
#ifdef VIRTUAL_OBJECT_STORE
// subtract name has index
// subtrace name index 
#define VFS_DIRECTORY_ENTRIES ( ( BLOCK_SIZE - ( 2*sizeof(BLOCKINDEX) + 256*sizeof(BLOCKINDEX)) ) /sizeof( struct directory_entry) )
#else
#define VFS_DIRECTORY_ENTRIES ( ( BLOCK_SIZE ) /sizeof( struct directory_entry) )
#endif


PREFIX_PACKED struct directory_hash_lookup_block
{
	BLOCKINDEX next_block[256];
	struct directory_entry entries[VFS_DIRECTORY_ENTRIES];
	BLOCKINDEX names_first_block;
	uint8_t used_names;
} PACKED;



struct disk
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



#ifdef SACK_VFS_FS_SOURCE


#define TSEEK(type,v,o,c) ((type)vfs_fs_SEEK(v,o,&c))
#define BTSEEK(type,v,o,c) ((type)vfs_fs_BSEEK(v,o,&c))


#else


PREFIX_PACKED struct volume {
	const char * volname;
#ifdef FILE_BASED_VFS
	FILE *file;
	struct file_system_mounted_interface *mount;
#else
	struct disk *disk;
	struct disk *diskReal; // disk might be offset from diskReal because it's a .exe attached.
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
	FLAGSET( seglock, BC( COUNT ) );  // segment is locked into cache.
#endif

	uint8_t fileCacheAge[BC(FILE_LAST) - BC(FILE)];
#ifdef VIRTUAL_OBJECT_STORE
	uint8_t dirHashCacheAge[BC(DIRECTORY_LAST) - BC(DIRECTORY)];
	uint8_t batHashCacheAge[BC(BAT_LAST) - BC(BAT)];
#endif
	uint8_t nameCacheAge[BC(NAMES_LAST) - BC(NAMES)];

	struct random_context *entropy;
	uint8_t* key;  // root of all cached key buffers
	uint8_t* segkey;  // allow byte encrypting... key based on sector volume file index
	uint8_t* sigkey;  // signature of executable attached as header
	uint8_t* sigsalt;  // signature of executable attached as header
	size_t sigkeyLength;
	uint8_t* usekey[BC(COUNT)]; // composite key

#ifdef FILE_BASED_VFS
	uint8_t* key_buffer;  // root buffer space of all cache blocks
	uint8_t* usekey_buffer[BC(COUNT)]; // data cache blocks
	FLAGSET( dirty, BC(COUNT) );
	FLAGSET( _dirty, BC( COUNT ) );
	FPI bufferFPI[BC(COUNT)];
#endif
	BLOCKINDEX lastBatBlock;
	PDATALIST pdlFreeBlocks;
	PLIST files; // when reopened file structures need to be updated also...
	LOGICAL read_only;
	LOGICAL external_memory;
	LOGICAL closed;
	uint32_t lock;
	uint8_t tmpSalt[16];
	uintptr_t clusterKeyVersion;
} PACKED;


struct sack_vfs_file
{
#ifdef FILE_BASED_VFS
	FPI entry_fpi;  // where to write the directory entry update to
#ifdef VIRTUAL_OBJECT_STORE
	enum block_cache_entries cache;
#endif
	struct directory_entry _entry;  // has file size within
	struct directory_entry *entry;  // has file size within
#else
	struct directory_entry *entry;  // has file size within
#endif
	struct directory_entry dirent_key;
	struct volume *vol; // which volume this is in
	FPI fpi;
	BLOCKINDEX _first_block;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
	LOGICAL delete_on_close;  // someone already deleted this...
	BLOCKINDEX *blockChain;
	int blockChainAvail;
	int blockChainLength;
};

#undef TSEEK
#undef BTSEEK
#ifdef VIRTUAL_OBJECT_STORE
#define TSEEK(type,v,o,c) ((type)vfs_os_SEEK(v,o,&c))
#define BTSEEK(type,v,o,c) ((type)vfs_os_BSEEK(v,o,&c))
#elif defined FILE_BASED_VFS
#define TSEEK(type,v,o,c) ((type)vfs_fs_SEEK(v,o,&c))
#define BTSEEK(type,v,o,c) ((type)vfs_fs_BSEEK(v,o,&c))
#else
#define TSEEK(type,v,o,c) ((type)vfs_SEEK(v,o,&c))
#define BTSEEK(type,v,o,c) ((type)vfs_BSEEK(v,o,&c))
#endif

#endif

#ifdef __GNUC__
#define HIDDEN __attribute__ ((visibility ("hidden")))
#else
#define HIDDEN
#endif

#if !defined( VIRTUAL_OBJECT_STORE ) && !defined( FILE_BASED_VFS )
uintptr_t vfs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) HIDDEN;
uintptr_t vfs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) HIDDEN;
#endif
//BLOCKINDEX vfs_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand );

#if defined( FILE_BASED_VFS )
uintptr_t vfs_fs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) HIDDEN;
uintptr_t vfs_fs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) HIDDEN;
#endif

#if defined( VIRTUAL_OBJECT_STORE ) 
uintptr_t vfs_os_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) HIDDEN;
uintptr_t vfs_os_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) HIDDEN;
#endif

