
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

typedef size_t BLOCKINDEX; // BLOCK_SIZE blocks...
typedef size_t FPI; // file position type

enum block_cache_entries
{
	BLOCK_CACHE_DIRECTORY
	, BLOCK_CACHE_NAMES
	, BLOCK_CACHE_FILE
	, BLOCK_CACHE_BAT
	, BLOCK_CACHE_DATAKEY
	, BLOCK_CACHE_COUNT
};

PREFIX_PACKED struct volume {
	const char * volname;
	struct disk *disk;
	//_32 dirents;  // constant 0
	//_32 nameents; // constant 1
	PTRSZVAL dwSize;
	const char * datakey;  // used for directory signatures
	const char * userkey;
	const char * devkey;
	enum block_cache_entries curseg;
	BLOCKINDEX segment[BLOCK_CACHE_COUNT];// associated with usekey[n]
	struct random_context *entropy;
	P_8 key;  // allow byte encrypting...
	P_8 segkey;  // allow byte encrypting... key based on sector volume file index
	P_8 usekey[BLOCK_CACHE_COUNT]; // composite key
	PLIST files; // when reopened file structures need to be updated also...
	LOGICAL read_only;
	_32 lock;
} PACKED;

PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	size_t filesize;  // how big the file is
	//_32 filler;  // extra data(unused)
} PACKED;
#define ENTRIES ( BLOCK_SIZE/sizeof( struct directory_entry) )

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
	_8  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];
};

struct sack_vfs_file
{
	struct directory_entry *entry;  // has file size within
	struct directory_entry dirent_key;
	struct volume *vol; // which volume this is in
	FPI fpi;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
	LOGICAL delete_on_close;  // someone already deleted this...
};

#define TSEEK(type,v,o,c) ((type)vfs_SEEK(v,o,c))
#define BTSEEK(type,v,o,c) ((type)vfs_BSEEK(v,o,c))

#ifdef __GNUC__
#define HIDDEN
//__attribute__ ((visibility ("hidden")))
#else
#define HIDDEN
#endif
PTRSZVAL vfs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries cache_index ) HIDDEN;
PTRSZVAL vfs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries cache_index ) HIDDEN;
BLOCKINDEX vfs_GetNextBlock( struct volume *vol, BLOCKINDEX block, LOGICAL init, LOGICAL expand );

