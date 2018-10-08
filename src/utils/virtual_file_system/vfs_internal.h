
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
#define BLOCKS_PER_BAT (BLOCK_SIZE/sizeof(BLOCKINDEX))
#define BLOCKS_PER_SECTOR (1 + (BLOCK_SIZE/sizeof(BLOCKINDEX)))
// per-sector perumation; needs to be a power of 2 (in bytes)
#define SHORTKEY_LENGTH 16

#ifndef VFS_DISK_DATATYPE
#  define VFS_DISK_DATATYPE size_t
#endif

typedef VFS_DISK_DATATYPE BLOCKINDEX; // BLOCK_SIZE blocks...
typedef VFS_DISK_DATATYPE FPI; // file position type

enum block_cache_entries
{
	BLOCK_CACHE_DIRECTORY
#ifdef VIRTUAL_OBJECT_STORE
	, BLOCK_CACHE_HASH_DIRECTORY
	, BLOCK_CACHE_HASH_DIRECTORY_LAST = BLOCK_CACHE_HASH_DIRECTORY + 16
#endif
	, BLOCK_CACHE_NAMES
	, BLOCK_CACHE_BAT
	, BLOCK_CACHE_DATAKEY
	, BLOCK_CACHE_FILE
	, BLOCK_CACHE_FILE_LAST = BLOCK_CACHE_FILE + 10
	, BLOCK_CACHE_COUNT
};



PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	VFS_DISK_DATATYPE filesize;  // how big the file is
	//uint32_t filler;  // extra data(unused)
} PACKED;
#define VFS_DIRECTORY_ENTRIES ( BLOCK_SIZE/sizeof( struct directory_entry) )

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
	BLOCKINDEX _segment[BLOCK_CACHE_COUNT];// cached segment with usekey[n]
	BLOCKINDEX segment[BLOCK_CACHE_COUNT];// associated with usekey[n]
	uint8_t fileCacheAge[BLOCK_CACHE_FILE_LAST - BLOCK_CACHE_FILE];
	uint8_t fileNextAge;
	struct random_context *entropy;
	uint8_t* key;  // root of all cached key buffers
	uint8_t* segkey;  // allow byte encrypting... key based on sector volume file index
	uint8_t* sigkey;  // signature of executable attached as header
	uint8_t* sigsalt;  // signature of executable attached as header
	size_t sigkeyLength;
	uint8_t* usekey[BLOCK_CACHE_COUNT]; // composite key

#ifdef FILE_BASED_VFS
	uint8_t* key_buffer;  // root buffer space of all cache blocks
	uint8_t* usekey_buffer[BLOCK_CACHE_COUNT]; // data cache blocks
	FLAGSET( dirty, BLOCK_CACHE_COUNT );
	FPI bufferFPI[BLOCK_CACHE_COUNT];
#endif

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
};

#define TSEEK(type,v,o,c) ((type)vfs_SEEK(v,o,&c))
#define BTSEEK(type,v,o,c) ((type)vfs_BSEEK(v,o,&c))
#endif

#ifdef __GNUC__
#define HIDDEN __attribute__ ((visibility ("hidden")))
#else
#define HIDDEN
#endif
uintptr_t vfs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) HIDDEN;
uintptr_t vfs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) HIDDEN;
//BLOCKINDEX vfs_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand );

uintptr_t vfs_fs_SEEK( struct fs_volume *vol, FPI offset, enum block_cache_entries *cache_index ) HIDDEN;
uintptr_t vfs_fs_BSEEK( struct fs_volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) HIDDEN;
