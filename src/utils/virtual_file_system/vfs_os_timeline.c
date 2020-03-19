//#define DEBUG_TEST_LOCKS

//#define DEBUG_VALIDATE_TREE_ADD
//#define DEBUG_LOG_LOCKS

//#define INVERSE_TEST
//#define DEBUG_DELETE_BALANCE

#define DEBUG_AVL_DETAIL

int nodes;

struct storageTimelineCache {
	BLOCKINDEX timelineSector;
	FPI dirEntry[BLOCK_SIZE / sizeof( FPI )];
	struct dirent_cache caches[BLOCK_SIZE / sizeof( FPI )];
	//	struct dirent_cache caches[BLOCK_SIZE / sizeof( FPI )];
};

#define timelineBlockIndexNull 0


typedef union timelineBlockType {
	// 0 is invalid; indexes must subtract 1 to get
	// real timeline index.
	uint64_t raw;
	struct timelineBlockReference {
		uint64_t depth : 6;
		uint64_t index : 58;
	} ref;
} TIMELINE_BLOCK_TYPE;

// this is milliseconds since 1970 (unix epoc) * 256 + timezoneOffset /15 in the low byte
typedef struct timelineTimeType {
	int64_t tzOfs : 8;
	uint64_t tick : 56;
} TIMELINE_TIME_TYPE;

PREFIX_PACKED struct timelineHeader {
	TIMELINE_BLOCK_TYPE first_free_entry;
	TIMELINE_BLOCK_TYPE crootNode_deleted;
	TIMELINE_BLOCK_TYPE srootNode;
	uint64_t unused[5];
	//uint64_t unused2[8];
} PACKED;

// current size is 64 bytes.
// me_fpi is the physical FPI in the timeline file of the TIMELINE_BLOCK_TYPE that references 'this' block.
// structure defines little endian structure for storage.

PREFIX_PACKED struct storageTimelineNode {
	// if dirent_fpi == 0; it's free.
	uint64_t dirent_fpi;
	TIMELINE_BLOCK_TYPE prior;
	// if the block is free, sgreater is used as pointer to next free block
	// delete an object can leave free timeline nodes in the middle of the physical chain.

	//uint64_t padding[4];
/*
	uint64_t ctime;                      // uses timeGetTime64() tick resolution
	//union {
	//	uint64_t raw;
	//	TIMELINE_TIME_TYPE parts;         // file time tick/ created stamp, sealing stamp
	//}ctime;
	TIMELINE_BLOCK_TYPE clesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE sgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
	TIMELINE_BLOCK_TYPE cparent;
*/
	uint32_t filler32_1;
	uint16_t priorDataPad;
	uint8_t  filler8_1; // how much of the last block in the file is not used

	uint8_t  timeTz; // lesser least significant byte of time... sometimes can read time including timezone offset with time - 1 byte

	uint64_t time;
	//union {
	//	uint64_t raw;
	//	TIMELINE_TIME_TYPE parts;        // time file was stored
	//}stime;
	TIMELINE_BLOCK_TYPE slesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE sgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
	uint64_t me_fpi; // it is know by  ( me_fpi & 0x3f ) == 32 or == 36 whether this is slesser or sgreater, (me_fpi & ~3f) = parent_fpi
	uint64_t priorData; // if not 0, references a start block version of data.
} PACKED;

struct memoryTimelineNode {
	// if dirent_fpi == 0; it's free.
	FPI this_fpi;
	uint64_t index;
	// the end of this is the same as storage timeline.

	struct storageTimelineNode* disk;
	enum block_cache_entries diskCache;
#if 0
	FPI dirent_fpi;   // FPI on disk
	// if the block is free, sgreater is used as pointer to next free block
	// delete an object can leave free timeline nodes in the middle of the physical chain.
	TIMELINE_BLOCK_TYPE prior;

	PREFIX_PACKED struct {
		uint32_t filler_32;
		uint16_t priorDataPad;
		uint8_t filler_8;
		uint8_t timeTz;
	} PACKED bits;

	uint64_t time;
	TIMELINE_BLOCK_TYPE slesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE sgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
	uint64_t me_fpi; // it is know by  ( me_fpi & 0x3f ) == 32 or == 36 whether this is slesser or sgreater, (me_fpi & ~3f) = parent_fpi
	uint64_t priorData; // if not 0, references a start block version of data.
#endif
};


struct storageTimelineCursor {
	PDATASTACK parentNodes;  // save stack of parents in cursor
	struct storageTimelineCache dirents; // temp; needs work.
};

#define NUM_ROOT_TIMELINE_NODES (TIME_BLOCK_SIZE - sizeof( struct timelineHeader )) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimeline {
	struct timelineHeader header;
	struct storageTimelineNode entries[NUM_ROOT_TIMELINE_NODES];
} PACKED;

#define NUM_TIMELINE_NODES (TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimelineBlock {
	struct storageTimelineNode entries[(TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )];
} PACKED;

#ifdef DEBUG_VALIDATE_TREE
#define VTReadOnly  , TRUE
#define VTReadWrite  , FALSE

#else
#define VTReadOnly
#define VTReadWrite
#endif


#ifdef _DEBUG
#define GRTENoLog ,0
#define GRTELog ,1
#else
#define GRTENoLog 
#define GRTELog 
#endif

struct storageTimelineNode* getRawTimeEntry( struct sack_vfs_os_volume* vol, uint64_t timeEntry, enum block_cache_entries *cache
#if _DEBUG
	, int log
#endif
	 DBG_PASS )
{
	int locks;
	cache[0] = BC( TIMELINE );
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, cache, TIME_BLOCK_SIZE DBG_SRC );

	locks = GETMASK_( vol->seglock, seglock, cache[0] );
#ifdef DEBUG_TEST_LOCKS
#  ifdef DEBUG_LOG_LOCKS
#    ifdef _DEBUG
	if( log )
#    endif
		_lprintf(DBG_RELAY)( "Lock %d %d %d", (int)timeEntry, cache[0], locks );
#  endif
	if( locks > 9 ) {
		lprintf( "Lock OVERFLOW" );
		DebugBreak();
	}
#endif
	locks++;
	
	SETMASK_( vol->seglock, seglock, cache[0], locks );

	return node;
}

TIMELINE_BLOCK_TYPE* getRawTimePointer( struct sack_vfs_os_volume* vol, uint64_t fpi, enum block_cache_entries *cache ) {
	cache[0] = BC( TIMELINE );
	return (TIMELINE_BLOCK_TYPE*)vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, fpi, cache, TIME_BLOCK_SIZE DBG_SRC );
}

void dropRawTimeEntry( struct sack_vfs_os_volume* vol, enum block_cache_entries cache
#if _DEBUG
	, int log
#endif
	 DBG_PASS ) {
	int locks;
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#  ifdef DEBUG_LOG_LOCKS
#    ifdef _DEBUG
	if( log )
#    endif
	_lprintf(DBG_RELAY)( "UnLock %d %d", cache, locks );
#  endif
	if( !locks ) {
		lprintf( "Lock UNDERFLOW" );
		DebugBreak();
	}
#endif
	locks--;
	SETMASK_( vol->seglock, seglock, cache, locks );
}

void reloadTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, uint64_t timeEntry
#ifdef DEBUG_VALIDATE_TREE
	, LOGICAL readOnly
#endif
#if _DEBUG
	, int log
#endif
	 DBG_PASS )
{
	enum block_cache_entries cache =
#ifdef DEBUG_VALIDATE_TREE
		readOnly ?BC(TIMELINE_RO):
#endif
		BC( TIMELINE );
	//uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index DBG_SRC ) {
	//if( timeEntry > 62 )DebugBreak();
	int locks;
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, &cache, TIME_BLOCK_SIZE DBG_RELAY );
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
#ifdef _DEBUG
	if( log )
#endif
		_lprintf(DBG_RELAY)( "Lock %d %d %d", (int)timeEntry, cache, locks );
#endif
	if( locks > 12 ) {
		lprintf( "Lock OVERFLOW" );
		DebugBreak();
	}
#endif
	locks++;
	SETMASK_( vol->seglock, seglock, cache, locks );

	time->disk = node;
	time->diskCache = cache;

	time->index = timeEntry;
	time->this_fpi = pos;

}



static void DumpTimelineTreeWork( struct sack_vfs_os_volume* vol, int level, struct memoryTimelineNode* parent, LOGICAL unused DBG_PASS ) {
	struct memoryTimelineNode curNode;
	static char indent[256];
	int i;
#if 1
	lprintf( "input node %d %d %d", parent->index
		, parent->disk->slesser.ref.index
		, parent->disk->sgreater.ref.index
	);
#endif
	if( parent->disk->slesser.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->disk->slesser.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		lprintf( "(lesser) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
	}
	for( i = 0; i < level * 3; i++ )
		indent[i] = ' ';
	indent[i] = 0;
	lprintf( "CurNode: (%s -> %5d  %d <-%d %s has children %d %d  with depths of %d %d"
		, indent
		, (int)parent->disk->dirent_fpi
		, (int)parent->index
		, parent->disk->me_fpi >> 6
		, ( ( parent->disk->me_fpi & 0x3f ) == 0x20 ) ? "L"
		: ( ( parent->disk->me_fpi & 0x3f ) == 0x10 ) ? "R"
		: "G"
		, (int)(parent->disk->slesser.ref.index)
		, (int)(parent->disk->sgreater.ref.index)
		, (int)(parent->disk->slesser.ref.depth)
		, (int)(parent->disk->sgreater.ref.depth)
	);
	if( parent->disk->sgreater.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->disk->sgreater.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		lprintf( "(greater) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
	}
}

//---------------------------------------------------------------------------

static void DumpTimelineTree( struct sack_vfs_os_volume* vol, LOGICAL unused DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE );
	struct storageTimeline* timeline = vol->timeline;// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	SETFLAG( vol->seglock, cache );
	struct memoryTimelineNode curNode;
	{
		if( !timeline->header.srootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, timeline->header.srootNode.ref.index  VTReadOnly GRTENoLog DBG_RELAY );
	}
	DumpTimelineTreeWork( vol, 0, &curNode, unused DBG_RELAY );
	dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
}

//---------------------------------------------------------------------------

static void LogTimelineTreeWork( PVARTEXT pvt, struct sack_vfs_os_volume* vol, int level, struct memoryTimelineNode* parent DBG_PASS ) {
	struct memoryTimelineNode curNode;
	static char indent[256];
	int i;
#if 0
	vtprintf( pvt,"input node %d %d %d\n", parent->index
		, parent->disk->slesser.ref.index
		, parent->disk->sgreater.ref.index
	);
#endif
	if( parent->disk->slesser.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->disk->slesser.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		vtprintf( pvt,"(lesser) go to node %d", curNode.index );
#endif
		LogTimelineTreeWork( pvt, vol, level + 1, &curNode DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
	}
	for( i = 0; i < level * 3; i++ )
		indent[i] = ' ';
	indent[i] = 0;
	vtprintf( pvt,"CurNode: (%s -> %5d  %d <-%d %s has children %d %d  with depths of %d %d\m"
		, indent
		, (int)parent->disk->dirent_fpi
		, (int)parent->index
		, parent->disk->me_fpi >> 6
		, ( ( parent->disk->me_fpi & 0x3f ) == 0x20 ) ? "L"
		: ( ( parent->disk->me_fpi & 0x3f ) == 0x10 ) ? "R"
		: "G"
		, (int)( parent->disk->slesser.ref.index )
		, (int)( parent->disk->sgreater.ref.index )
		, (int)( parent->disk->slesser.ref.depth )
		, (int)( parent->disk->sgreater.ref.depth )
	);
	if( parent->disk->sgreater.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->disk->sgreater.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		vtprintf( pvt,"(greater) go to node %d", curNode.index );
#endif
		LogTimelineTreeWork( pvt, vol, level + 1, &curNode DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
	}
}

//---------------------------------------------------------------------------

static void LogTimelineTree( PVARTEXT pvt, struct sack_vfs_os_volume* vol, BLOCKINDEX fromRoot DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE );
	struct storageTimeline* timeline = vol->timeline;// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	SETFLAG( vol->seglock, cache );
	struct memoryTimelineNode curNode;
	{
		if( !timeline->header.srootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, fromRoot?fromRoot:timeline->header.srootNode.ref.index  VTReadOnly GRTENoLog DBG_RELAY );
	}
	LogTimelineTreeWork( pvt, vol, 0, &curNode DBG_RELAY );
	dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
}

//---------------------------------------------------------------------------

static void checkRoot( struct sack_vfs_os_volume* vol ) {
	enum block_cache_entries cache = BC( TIMELINE_RO );
	// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	struct storageTimeline* timeline = vol->timeline;
	struct memoryTimelineNode curNode;
	if( !timeline->header.srootNode.ref.index ) {
		return;
	}
	reloadTimeEntry( &curNode, vol
		, timeline->header.srootNode.ref.index VTReadOnly GRTENoLog  DBG_SRC );
	if( curNode.disk->me_fpi != 0x10 ) {
		lprintf( "Root of tree does not point to itself." );
		DebugBreak();
	}
	dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_SRC );
}
//---------------------------------------------------------------------------

#ifdef DEBUG_VALIDATE_TREE
static int ValidateTimelineTreeWork( struct sack_vfs_os_volume* vol, int level
	, struct memoryTimelineNode* parent
	, struct memoryTimelineNode* left
	, struct memoryTimelineNode* right
	, BLOCKINDEX index DBG_PASS ) {
	struct memoryTimelineNode curNodeLeft;
	struct memoryTimelineNode curNodeRight;
	static char indent[256];
	int i;
	int n = 0;
#if 0
	lprintf( "input node %d %d %d", parent->index
		, parent->disk->slesser.ref.index
		, parent->disk->sgreater.ref.index
	);
#endif
	if( parent->disk->slesser.ref.index ) {
		reloadTimeEntry( left, vol, parent->disk->slesser.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		lprintf( "(lesser) go to node %d", curNode.index );
#endif
		if( ( parent->this_fpi + sane_offsetof( struct storageTimelineNode, slesser ) ) != left->disk->me_fpi ) {
			DumpTimelineTree( vol, 0 DBG_SRC );
			DebugBreak();
		}
		if( !parent->disk->slesser.ref.index && parent->disk->slesser.ref.depth )
			DebugBreak();
		if( !parent->disk->sgreater.ref.index && parent->disk->sgreater.ref.depth )
			DebugBreak();
		n += ValidateTimelineTreeWork( vol, level + 1, left, &curNodeLeft, &curNodeRight, parent->disk->slesser.ref.index DBG_RELAY );
		if( curNodeLeft.index && curNodeRight.index )
			if( curNodeLeft.disk->time > curNodeRight.disk->time ) {
				lprintf( "tree is out of order children.  %d", left->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
		if( curNodeLeft.index ) {
			if( curNodeLeft.disk->time > left->disk->time ) {
				lprintf( "tree is out of order to left.  %d", left->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
			dropRawTimeEntry( vol, curNodeLeft.diskCache GRTENoLog DBG_RELAY );
		}
		if( curNodeRight.index ) {
			if( curNodeRight.disk->time < left->disk->time ) {
				lprintf( "tree is out of order to right.  %d", left->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
			dropRawTimeEntry( vol, curNodeRight.diskCache GRTENoLog DBG_RELAY );
		}
	}
	else
		left->index = 0;

	if( parent->disk->sgreater.ref.index ) {
		reloadTimeEntry( right, vol, parent->disk->sgreater.ref.index VTReadOnly GRTENoLog DBG_RELAY );
#if 0
		lprintf( "(greater) go to node %d", right->index );
#endif
		if( ( parent->this_fpi + sane_offsetof( struct storageTimelineNode, sgreater ) ) != right->disk->me_fpi ) {
			DumpTimelineTree( vol, 0 DBG_SRC );
			DebugBreak();
		}

		n += ValidateTimelineTreeWork( vol, level + 1, right, &curNodeLeft, &curNodeRight, parent->disk->sgreater.ref.index DBG_RELAY );

		if( curNodeLeft.index && curNodeRight.index )
			if( curNodeLeft.disk->time > curNodeRight.disk->time ) {
				lprintf( "tree is out of order.  %d", right->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
		if( curNodeLeft.index ) {
			if( curNodeLeft.disk->time > right->disk->time ) {
				lprintf( "tree is out of order to left.  %d", right->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
			dropRawTimeEntry( vol, curNodeLeft.diskCache GRTENoLog DBG_RELAY );
		}
		if( curNodeRight.index ) {
			if( curNodeRight.disk->time < right->disk->time ) {
				lprintf( "tree is out of order to right.  %d", right->index );
				DumpTimelineTree( vol, 0 DBG_SRC );
				DebugBreak();
			}
			dropRawTimeEntry( vol, curNodeRight.diskCache GRTENoLog DBG_RELAY );
		}
	}
	else
		right->index = 0;
	return n + 1;
}

//---------------------------------------------------------------------------

static void ValidateTimelineTree( struct sack_vfs_os_volume* vol DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE_RO );
	// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	struct storageTimeline* timeline = vol->timeline;
	SETFLAG( vol->seglock, cache );
	struct memoryTimelineNode curNode;
	struct memoryTimelineNode curNodeLeft;
	struct memoryTimelineNode curNodeRight;
	int nodeCount;
	checkRoot( vol );
	{
		if( !timeline->header.srootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, timeline->header.srootNode.ref.index VTReadOnly GRTENoLog  DBG_RELAY );
	}
	nodeCount = ValidateTimelineTreeWork( vol, 0, &curNode, &curNodeLeft, &curNodeRight, timeline->header.srootNode.ref.index DBG_RELAY );
	if( nodeCount != nodes ) {
		lprintf( "The count of nodes in the tree and those that are free is differnt." );
		DebugBreak();
	}
	dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
	if( curNodeLeft.index )
		dropRawTimeEntry( vol, curNodeLeft.diskCache GRTENoLog DBG_RELAY );
	if( curNodeRight.index )
		dropRawTimeEntry( vol, curNodeRight.diskCache GRTENoLog DBG_RELAY );

		
	{
		BLOCKINDEX freeblock;
		int count = 0;
		if( freeblock = timeline->header.first_free_entry.ref.index ) {
			while( freeblock ) {
				dropRawTimeEntry( vol, curNode.diskCache GRTENoLog DBG_RELAY );
				reloadTimeEntry( &curNode, vol
					, freeblock VTReadOnly GRTENoLog  DBG_RELAY );
				freeblock = curNode.disk->sgreater.ref.index;
				count++;
			}
		}
	}


}

#endif
//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL drop DBG_PASS ) {
	SMUDGECACHE( vol, time->diskCache );
	if( drop ) {
		int locks;
		locks = GETMASK_( vol->seglock, seglock, time->diskCache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
		lprintf( "Unlock %d %d", time->diskCache, locks );
#endif
		if( !locks ) {
			lprintf( "Lock UNDERFLOW" );
			DebugBreak();
		}
#endif
		locks--;
		SETMASK_( vol->seglock, seglock, time->diskCache, locks );
	}
}

//---------------------------------------------------------------------------

void reloadDirectoryEntry( struct sack_vfs_os_volume* vol, struct memoryTimelineNode* time, struct sack_vfs_os_find_info* decoded_dirent DBG_PASS ) {
	enum block_cache_entries cache = BC( DIRECTORY );
	struct directory_entry* dirent;// , * entkey;
	struct directory_hash_lookup_block* dirblock;
	//struct directory_hash_lookup_block* dirblockkey;
	PDATASTACK pdsChars = CreateDataStack( 1 );
	BLOCKINDEX this_dir_block = (time->disk->dirent_fpi >> BLOCK_BYTE_SHIFT);
	BLOCKINDEX next_block;
	dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
	//dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
	dirent = (struct directory_entry*)(((uintptr_t)dirblock) + (time->disk->dirent_fpi & BLOCK_SIZE));
	//entkey = (struct directory_entry*)(((uintptr_t)dirblockkey) + (time->dirent_fpi & BLOCK_SIZE));

	decoded_dirent->vol = vol;

	// all of this regards the current state of a find cursor...
	decoded_dirent->base = NULL;
	decoded_dirent->base_len = 0;
	decoded_dirent->mask = NULL;
	decoded_dirent->pds_directories = NULL;

	decoded_dirent->filesize = (size_t)( dirent->filesize );
	if( time->disk->prior.raw ) {
		enum block_cache_entries cache;
		struct storageTimelineNode* prior = getRawTimeEntry( vol, time->disk->prior.raw, &cache GRTENoLog DBG_SRC );
		while( prior->prior.raw ) {
			dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
			prior = getRawTimeEntry( vol, prior->prior.raw, &cache GRTENoLog DBG_RELAY );
		}
		decoded_dirent->ctime = prior->time;
		dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
	}
	else
		decoded_dirent->ctime = time->disk->time;
	decoded_dirent->wtime = time->disk->time;

	while( (next_block = dirblock->next_block[DIRNAME_CHAR_PARENT]) ) {
		enum block_cache_entries back_cache = BC( DIRECTORY );
		struct directory_hash_lookup_block* back_dirblock;
		back_dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, next_block, DIR_BLOCK_SIZE, back_cache );
		//back_dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[back_cache];
		int i;
		for( i = 0; i < DIRNAME_CHAR_PARENT; i++ ) {
			if( (back_dirblock->next_block[i]) == this_dir_block ) {
				PushData( &pdsChars, &i );
				break;
			}
		}
		if( i == DIRNAME_CHAR_PARENT ) {
			// directory didn't have a forward link to it?
			DebugBreak();
		}
		this_dir_block = next_block;
		dirblock = back_dirblock;
	}

	char* c;
	int n = 0;
	// could fill leadin....
	decoded_dirent->leadin[0] = 0;
	decoded_dirent->leadinDepth = 0;
	while( c = (char*)PopData( &pdsChars ) )
		decoded_dirent->filename[n++] = c[0];
	DeleteDataStack( &pdsChars );

	{
		BLOCKINDEX nameBlock;
		nameBlock = dirblock->names_first_block;
		FPI name_offset = (dirent[n].name_offset ) & DIRENT_NAME_OFFSET_OFFSET;

		enum block_cache_entries cache = BC( NAMES );
		const char* dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache, NAME_BLOCK_SIZE DBG_SRC );
		const char* dirname_ = dirname;
		//const char* dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK);
		const char* prior_dirname = dirname;
		int c;
		do {
			while( (((unsigned char)(c = (dirname[0] )) != UTF8_EOT))
				&& ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) == (((uintptr_t)dirname) & ~BLOCK_MASK))
				) {
				decoded_dirent->filename[n++] = c;
				dirname++;
				//dirkey++;
			}
			if( ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) != (((uintptr_t)dirname) & ~BLOCK_MASK)) ) {
				int partial = (int)(dirname - dirname_);
				cache = BC( NAMES );
				dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset + partial, &cache, NAME_BLOCK_SIZE DBG_SRC );
				//dirkey = (const char*)(vol->usekey[cache]) + ((name_offset + partial) & BLOCK_MASK);
				dirname_ = dirname - partial;
				prior_dirname = dirname;
				continue;
			}
			// didn't stop because it exceeded a sector boundary
			break;
		} while( 1 );
	}
	decoded_dirent->filename[n] = 0;
	decoded_dirent->filenamelen = n;
	//time->dirent_fpi

}

//---------------------------------------------------------------------------

static void _os_AVL_RotateToRight(
	struct sack_vfs_os_volume* vol,
	struct storageTimelineNode* node,
	BLOCKINDEX nodeIdx,
	struct storageTimelineNode* left_,
	BLOCKINDEX leftIdx
	DBG_PASS
)
{
	{
		enum block_cache_entries meCache;
		TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, node->me_fpi, &meCache );
		ptr[0] = node->slesser;

		if( node->slesser.raw = left_->sgreater.raw ) // this could be NULL
		{
			struct storageTimelineNode* x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, node->slesser.ref.index, &cache GRTELog DBG_RELAY );
			x->me_fpi = sane_offsetof( struct storageTimeline, entries[nodeIdx - 1].slesser );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "x fpi = nodeIdx" );
#endif
			SMUDGECACHE( vol, cache );
			dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
		}
#ifdef DEBUG_DELETE_BALANCE
		lprintf( "left fpi = node fpi  %d  %d", nodeIdx, node->me_fpi );
#endif
		left_->me_fpi = node->me_fpi;

		left_->sgreater.ref.index = nodeIdx;
#ifdef DEBUG_DELETE_BALANCE
		lprintf( "node-> fpi = nodeIdx %d %d", nodeIdx, leftIdx );
#endif
		node->me_fpi = sane_offsetof( struct storageTimeline, entries[leftIdx - 1].sgreater );


		/* Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->slesser.ref.depth;
			rightDepth = (int)node->sgreater.ref.depth;
			if( leftDepth > rightDepth )
				left_->sgreater.ref.depth = leftDepth + 1;
			else
				left_->sgreater.ref.depth = rightDepth + 1;

			leftDepth = (int)left_->slesser.ref.depth;
			rightDepth = (int)left_->sgreater.ref.depth;

			if( leftDepth > rightDepth ) {
				ptr[0].ref.depth = leftDepth + 1;
			}
			else {
				ptr[0].ref.depth = rightDepth + 1;
			}
		}
		SMUDGECACHE( vol, meCache );
	}
	//updateTimeEntry( node, vol DBG_RELAY );
	//updateTimeEntry( left_, vol DBG_RELAY );
}

//---------------------------------------------------------------------------

static void _os_AVL_RotateToLeft(
	struct sack_vfs_os_volume* vol,
	struct storageTimelineNode* node,
	BLOCKINDEX nodeIdx,
	struct storageTimelineNode* right_,
	BLOCKINDEX leftIdx
	DBG_PASS
)
{
	{
		enum block_cache_entries meCache;
		TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, node->me_fpi, &meCache );
		ptr[0] = node->sgreater;
		SMUDGECACHE( vol, meCache );

		if( node->sgreater.raw = right_->slesser.raw )
		{
			struct storageTimelineNode *x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, node->sgreater.ref.index, &cache GRTELog DBG_RELAY );
			x->me_fpi = sane_offsetof( struct storageTimeline, entries[nodeIdx - 1].sgreater );
			SMUDGECACHE( vol, cache );
			dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
		}
		right_->me_fpi = node->me_fpi;

		right_->slesser.ref.index = nodeIdx;
		node->me_fpi = sane_offsetof( struct storageTimeline, entries[leftIdx - 1].slesser );

		/*  Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->slesser.ref.depth;
			rightDepth = (int)node->sgreater.ref.depth;
			if( leftDepth > rightDepth )
				right_->slesser.ref.depth = leftDepth + 1;
			else
				right_->slesser.ref.depth = rightDepth + 1;

			leftDepth = (int)right_->slesser.ref.depth;
			rightDepth = (int)right_->sgreater.ref.depth;

			if( leftDepth > rightDepth ) {
				ptr[0].ref.depth = leftDepth + 1;
			}
			else {
				ptr[0].ref.depth = rightDepth + 1;
			}
		}
		SMUDGECACHE( vol, meCache );
	}

	//updateTimeEntry( node, vol );
	//updateTimeEntry( right_, vol );
}

//---------------------------------------------------------------------------


static void _os_AVLbalancer( struct sack_vfs_os_volume* vol, BLOCKINDEX index DBG_PASS ) {
	struct storageTimelineNode* _x = NULL;
	BLOCKINDEX idx_x;
	struct storageTimelineNode* _y = NULL;
	BLOCKINDEX idx_y = 0;
	struct storageTimelineNode* _z = NULL;
	struct storageTimelineNode* tmp;
	enum block_cache_entries cache_z, cache_x, cache_y = BC(ZERO);
	enum block_cache_entries cache_tmp;
	BLOCKINDEX curIndex = index;
	int leftDepth;
	int rightDepth;
	LOGICAL balanced = FALSE;

	_z = getRawTimeEntry( vol, curIndex, &cache_z GRTELog DBG_RELAY );

	{
		while( _z ) {
			int doBalance;
			rightDepth = (int)_z->sgreater.ref.depth;
			leftDepth = (int)_z->slesser.ref.depth;
			if( _z->me_fpi & ~0x3f ) {
				tmp = getRawTimeEntry( vol, ( ( _z->me_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) ) + 1, & cache_tmp GRTELog DBG_RELAY );
			}
			else tmp = NULL;
			if( tmp ) {
#ifdef DEBUG_TIMELINE_AVL
		//		lprintf( "WR (P)left/right depths: %d  %d   %d    %d  %d", (int)tmp->index, (int)leftDepth, (int)rightDepth, (int)tmp->sgreater.ref.index, (int)tmp->slesser.ref.index );
				lprintf( "WR left/right depths: %d   %d   %d    %d  %d", (int)curIndex, (int)leftDepth, (int)rightDepth, (int)_z->sgreater.ref.index, (int)_z->slesser.ref.index );
#endif
				if( leftDepth > rightDepth ) {
					if( tmp->sgreater.ref.index == curIndex ) {
						if( (1 + leftDepth) == tmp->sgreater.ref.depth ) {
							//if( zz )
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							dropRawTimeEntry( vol, cache_tmp GRTELog DBG_RELAY );
							break;
						}
						tmp->sgreater.ref.depth = 1 + leftDepth;
					}
					else
#ifdef _DEBUG
						if( tmp->slesser.ref.index == curIndex )
#endif
						{
							if( (1 + leftDepth) == tmp->slesser.ref.depth ) {
								//if( zz )
								//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
								dropRawTimeEntry( vol, cache_tmp GRTELog DBG_RELAY );
								break;
							}
							tmp->slesser.ref.depth = 1 + leftDepth;
						}
#ifdef _DEBUG
						else
							DebugBreak();// Should be one or the other... 
#endif
				}
				else {
					if( tmp->sgreater.ref.index == curIndex ) {
						if( (1 + rightDepth) == tmp->sgreater.ref.depth ) {
							//if(zz)
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							dropRawTimeEntry( vol, cache_tmp GRTELog DBG_RELAY );
							break;
						}
						tmp->sgreater.ref.depth = 1 + rightDepth;
					}
					else
#ifdef _DEBUG
						if( tmp->slesser.ref.index == curIndex )
#endif
						{
							if( (1 + rightDepth) == tmp->slesser.ref.depth ) {
								//if(zz)
								//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
								dropRawTimeEntry( vol, cache_tmp GRTELog DBG_RELAY );
								break;
							}
							tmp->slesser.ref.depth = 1 + rightDepth;
						}
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "WR updated left/right depths: %d      %d  %d", (int)idx_y, (int)tmp->sgreater.ref.depth, (int)tmp->slesser.ref.depth );
#endif
				SMUDGECACHE( vol, cache_tmp );
				dropRawTimeEntry( vol, cache_tmp GRTELog DBG_RELAY );
			}
			if( leftDepth > rightDepth )
				doBalance = ((leftDepth - rightDepth) > 1);
			else
				doBalance = ((rightDepth - leftDepth) > 1);


			if( doBalance ) {
				if( _x ) {
					if( idx_x == _y->slesser.ref.index ) {
						if( idx_y == _z->slesser.ref.index ) {
							// left/left
							_os_AVL_RotateToRight( vol, _z, curIndex, _y, idx_y DBG_RELAY );
							SMUDGECACHE( vol, cache_y );
							SMUDGECACHE( vol, cache_z );

#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R1 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
#ifdef DEBUG_VALIDATE_TREE_ADD
							//ValidateTimelineTree( vol DBG_SRC );
#endif
						}
						else {
							//left/rightDepth
							_os_AVL_RotateToRight( vol, _y, idx_y, _x, idx_x DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R2 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
#ifdef DEBUG_VALIDATE_TREE_ADD
							//ValidateTimelineTree( vol DBG_SRC );
#endif
							_os_AVL_RotateToLeft( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L1 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SMUDGECACHE( vol, cache_z );
							SMUDGECACHE( vol, cache_y );
							SMUDGECACHE( vol, cache_x );
#ifdef DEBUG_VALIDATE_TREE_ADD
							ValidateTimelineTree( vol DBG_SRC );
#endif
						}
					}
					else {
						if( idx_y == _z->slesser.ref.index ) {
							_os_AVL_RotateToLeft( vol, _y, idx_y, _x, idx_x DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L2 ---------------- " );
#endif
							SMUDGECACHE( vol, cache_y );
							SMUDGECACHE( vol, cache_x );
#ifdef DEBUG_VALIDATE_TREE_ADD
							//ValidateTimelineTree( vol DBG_SRC );
#endif
							_os_AVL_RotateToRight( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R3 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SMUDGECACHE( vol, cache_z );
							SMUDGECACHE( vol, cache_y );
#ifdef DEBUG_VALIDATE_TREE_ADD
							//ValidateTimelineTree( vol DBG_SRC );
#endif
							// rightDepth.left
						}
						else {
							//rightDepth/rightDepth
							_os_AVL_RotateToLeft( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L3 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SMUDGECACHE( vol, cache_y );
							SMUDGECACHE( vol, cache_z );
#ifdef DEBUG_VALIDATE_TREE_ADD
							ValidateTimelineTree( vol DBG_SRC );
#endif
						}
					}
#ifdef DEBUG_TIMELINE_AVL
					lprintf( "WR Balanced, should redo this one... %d %d", (int)_z->slesser.ref.index, _z->sgreater.ref.index );
#endif
					// reset to bottom of tree so we get a proper tail going further up
					// x and y and z can reverse order in the above.
					dropRawTimeEntry( vol, cache_z GRTELog DBG_RELAY );
					dropRawTimeEntry( vol, cache_y GRTELog DBG_RELAY );
					_z = _x;
					cache_z = cache_x;
					_y = _x = NULL;
				}
				else {
					//lprintf( "Not deep enough for balancing." );
				}
			}
			if( _x )
				dropRawTimeEntry( vol, cache_x GRTELog DBG_RELAY );
			cache_x = cache_y;
			idx_x = idx_y;
			_x = _y;
			cache_y = cache_z;
			idx_y = curIndex;
			_y = _z;
			if( _z->me_fpi >= sizeof( struct timelineHeader ) ) {
				curIndex = ( ( _z->me_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) ) + 1;
				_z = getRawTimeEntry( vol, curIndex, &cache_z GRTELog DBG_RELAY );
			} else {
				_z = NULL;
			}
			//_z = (struct memoryTimelineNode*)PopData( pdsStack );
		}

		if( _x )
			dropRawTimeEntry( vol, cache_x GRTELog DBG_RELAY );
		if( _y )
			dropRawTimeEntry( vol, cache_y GRTELog DBG_RELAY );
		if( _z )
			dropRawTimeEntry( vol, cache_z GRTELog DBG_RELAY );
	}
}

//---------------------------------------------------------------------------


static int hangTimelineNode( struct sack_vfs_os_volume* vol
	, TIMELINE_BLOCK_TYPE index
	, LOGICAL unused
	, struct storageTimeline* timeline
	, struct storageTimelineNode* timelineNode
	DBG_PASS
)
{
	PDATASTACK* pdsStack = &vol->pdsWTimeStack;
	struct storageTimelineNode *pCurNode;
	uint64_t curindex;
	enum block_cache_entries cacheCur;


		{
			if( !timeline->header.srootNode.ref.index ) {
				timeline->header.srootNode.ref.index = index.ref.index ;
				timeline->header.srootNode.ref.depth = 0 ;
				timelineNode->me_fpi = offsetof( struct timelineHeader, srootNode );
				SMUDGECACHE( vol, vol->timelineCache );
				return 1;
			}

			pCurNode = getRawTimeEntry( vol, curindex = timeline->header.srootNode.ref.index, &cacheCur GRTELog DBG_SRC );
		}

	while( 1 ) {
		int dir;// = root->Compare( node->key, check->key );
		//curNode_ = (struct memoryTimelineNode*)PeekData( pdsStack );
		{
			if( pCurNode->time > timelineNode->time )
				dir = -1;
			else if( pCurNode->time < timelineNode->time )
				dir = 1;
			else
				dir = 0;

		}
#ifdef INVERSE_TEST
		dir = -dir;
#endif
		uint64_t nextIndex;
		//dir = -dir; // test opposite rotation.
		if( dir < 0 ) {
			if( nextIndex = pCurNode->slesser.ref.index ) {
				dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
				pCurNode = getRawTimeEntry( vol, curindex = nextIndex, &cacheCur GRTELog DBG_SRC );
			}
			else {

				pCurNode->slesser.ref.index = index.ref.index;
				pCurNode->slesser.ref.depth = 0;

				timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex - 1].slesser );
				SMUDGECACHE( vol, cacheCur );
				//updateTimeEntry( curNode_, vol DBG_SRC );
				dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
				break;
			}
		}
		else if( dir > 0 )
			if( nextIndex = pCurNode->sgreater.ref.index ) {
				dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
				pCurNode = getRawTimeEntry( vol, curindex = nextIndex, &cacheCur GRTELog DBG_SRC );
			}
			else {
				pCurNode->sgreater.ref.index = index.ref.index;
				pCurNode->sgreater.ref.depth = 0;

				timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex-1].sgreater );
				SMUDGECACHE( vol, cacheCur );
				dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
				break;
			}
		else {
			// allow duplicates; but link in as a near node, either left
			// or right... depending on the depth.
			int leftdepth = 0, rightdepth = 0;
			uint64_t nextLesserIndex, nextGreaterIndex;
			if( nextLesserIndex = pCurNode->slesser.ref.index )
				leftdepth = (int)( pCurNode->slesser.ref.depth);
			if( nextGreaterIndex = pCurNode->sgreater.ref.index )
				rightdepth = (int)( pCurNode->sgreater.ref.depth);
			if( leftdepth < rightdepth ) {
				if( nextLesserIndex ) {
					dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
					pCurNode = getRawTimeEntry( vol, curindex = nextLesserIndex, &cacheCur GRTELog DBG_SRC );
				}
				else {
					{
						pCurNode->slesser.ref.index = index.ref.index;
						pCurNode->slesser.ref.depth = 0;
					}
					timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex - 1].slesser );
					SMUDGECACHE( vol, cacheCur );
					dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
					break;
				}
			}
			else {
				if( nextGreaterIndex ) {
					dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
					pCurNode = getRawTimeEntry( vol, curindex = nextGreaterIndex, &cacheCur GRTELog DBG_SRC );
				}
				else {
					{
						pCurNode->sgreater.ref.index = index.ref.index;
						pCurNode->sgreater.ref.depth = 0;
					}
					timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex-1].sgreater );
					SMUDGECACHE( vol, cacheCur );
					dropRawTimeEntry( vol, cacheCur GRTELog DBG_SRC );
					break;
				}
			}
		}
	}
#ifdef DEBUG_DELETE_BALANCE
	lprintf( " ------------- BEFORE AVL BALANCER ---------------- " );
	DumpTimelineTree( vol, TRUE  DBG_RELAY );
#endif
#ifdef DEBUG_VALIDATE_TREE_ADD
	ValidateTimelineTree( vol DBG_SRC );
#endif
	_os_AVLbalancer( vol, index.ref.index DBG_RELAY );
#ifdef DEBUG_VALIDATE_TREE_ADD
	ValidateTimelineTree( vol DBG_SRC );
#endif
	return 1;
}

static void deleteTimelineIndexWork( struct sack_vfs_os_volume* vol, BLOCKINDEX index, struct storageTimelineNode* time, enum block_cache_entries cache DBG_PASS ) {
#define DBG_DELETE_ DBG_SRC
	enum block_cache_entries cache_left = BC( TIMELINE );
	enum block_cache_entries cache_right = BC( TIMELINE );
	enum block_cache_entries cache_parent = BC( TIMELINE );
	enum block_cache_entries meCache;
	enum block_cache_entries cacheTmp = BC(ZERO);

	{
		//enum block_cache_entries bottomCache = BC( ZERO );
		//struct storageTimelineNode* bottom;
		FPI bottom_me_fpi;
		struct storageTimelineNode* least = NULL;
		BLOCKINDEX leastIndex;
		struct storageTimelineNode *tmp;
		PDATALIST pdlVisited = CreateDataList( sizeof( BLOCKINDEX ) );
		VarTextEmpty( vol->pvtDeleteBuffer );
		vtprintf( vol->pvtDeleteBuffer, "Delete %d\n", index );
		checkRoot( vol );
		//time = getRawTimeEntry( vol, index, &cache );
#ifdef DEBUG_DELETE_BALANCE
		lprintf( "delete index %d", index );
#endif
		if( !time->slesser.ref.index ) {
			if( time->sgreater.ref.index ) {
				enum block_cache_entries cache;
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				struct storageTimelineNode *greater = getRawTimeEntry( vol, time->sgreater.ref.index, &cache GRTELog DBG_DELETE_ );
				ptr[0] = time->sgreater; // my depth to my greater is correct to copy anyway.
				greater->me_fpi = time->me_fpi;
				SMUDGECACHE( vol, meCache );
				SMUDGECACHE( vol, cache );

				vtprintf( vol->pvtDeleteBuffer, "Only Right %d\n", time->sgreater.ref.index );

#ifdef DEBUG_DELETE_BALANCE
				lprintf( "had only greater" );
#endif
				bottom_me_fpi = greater->me_fpi;
				dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
			} else {
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				vtprintf( vol->pvtDeleteBuffer, "No Children\n" );
				ptr[0].raw = timelineBlockIndexNull;
				SMUDGECACHE( vol, meCache );
				// no greater or lesser...
				bottom_me_fpi = time->me_fpi;
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Leaf node; no left or right" );
#endif
			}
		} else if( !time->sgreater.ref.index ) {
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
			enum block_cache_entries cache;
			struct storageTimelineNode* lesser = getRawTimeEntry( vol, time->slesser.ref.index, &cache  GRTELog DBG_DELETE_ );
			ptr[0].raw = time->slesser.raw;
			lesser->me_fpi = time->me_fpi;
			SMUDGECACHE( vol, meCache );
			bottom_me_fpi = lesser->me_fpi;
			SMUDGECACHE( vol, cache );
			dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
			vtprintf( vol->pvtDeleteBuffer, "Only Left %d\n", time->slesser.ref.index );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "had only lesser" );
#endif
		} else {

#ifdef DEBUG_DELETE_BALANCE
			lprintf( "has greater and lesser %d %d", time->slesser.ref.depth, time->sgreater.ref.depth );
#endif
			vtprintf( vol->pvtDeleteBuffer, "both left and right %d  %d\n", time->slesser.ref.index, time->sgreater.ref.index );
			if( time->slesser.ref.depth > time->sgreater.ref.depth ) {
				PDATALIST pdlVisitedLesser = CreateDataList( sizeof( BLOCKINDEX ) );
				enum block_cache_entries leastCache;
				bottom_me_fpi = time->me_fpi;
				//bottom = time;
				//bottomCache = BC( ZERO );
					
				vtprintf( vol->pvtDeleteBuffer, "left is deeper %d %d\n", time->slesser.ref.depth, time->sgreater.ref.depth );
				least = getRawTimeEntry( vol, leastIndex = time->slesser.ref.index, &leastCache GRTELog DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Stepped to least %d", leastIndex );
#endif
				while( least->sgreater.raw ) {
					bottom_me_fpi = least->me_fpi;
					{
						INDEX idx;
						uint64_t* c;
						DATA_FORALL( pdlVisited, idx, uint64_t*, c ) {
							if( c[0] == bottom_me_fpi ) {
								lprintf( "going up the tree is broken now." );
								DebugBreak();
							}
						}
					}
					AddDataItem( &pdlVisitedLesser, &bottom_me_fpi );
					dropRawTimeEntry( vol, leastCache GRTELog DBG_DELETE_ );
					least = getRawTimeEntry( vol, leastIndex = least->sgreater.ref.index, &leastCache GRTELog DBG_DELETE_ );
					vtprintf( vol->pvtDeleteBuffer, "move to lesser's greater %d\n", leastIndex );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Stepped to least1 %d", leastIndex );
#endif
				}
				DeleteDataList( &pdlVisitedLesser );

				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->slesser.raw ) {
					enum block_cache_entries cache;
					struct storageTimelineNode* leastLesser = getRawTimeEntry( vol, least->slesser.ref.index, &cache GRTELog DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Least has a lesser node (off of greatest on least) %d", least->slesser.ref.index );
#endif
					leastLesser->me_fpi = least->me_fpi;
					ptr[0].raw = least->slesser.raw;
					least->slesser.raw = 0; // now no longer points to a thing.
					SMUDGECACHE( vol, cache );
					vtprintf( vol->pvtDeleteBuffer, "lesser's greatest has a lesser %d\n", least->slesser.ref.index );

					bottom_me_fpi = leastLesser->me_fpi;
					dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
				} else {
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Fill parent of least with null" );
#endif
					vtprintf( vol->pvtDeleteBuffer, "lesser's greatest is the bottom\n", least->slesser.ref.index );
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SMUDGECACHE( vol, meCache );
				SMUDGECACHE( vol, leastCache );
				dropRawTimeEntry( vol, leastCache GRTELog DBG_DELETE_ );
				// bottom contains this reference, and will release later.
				//dropRawTimeEntry( vol, leastCache GRTELog DBG_DELETE_ );
			}else {
				PDATALIST pdlVisitedLesser = CreateDataList( sizeof( BLOCKINDEX ) );
				enum block_cache_entries cache = BC( ZERO );

				bottom_me_fpi = time->me_fpi;
				least = getRawTimeEntry( vol, leastIndex = time->sgreater.ref.index, &cache GRTELog DBG_DELETE_ );
				vtprintf( vol->pvtDeleteBuffer, "right is deeper %d %d\n", time->slesser.ref.depth, time->sgreater.ref.depth );
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Stepped to least2 %d", leastIndex );
#endif
				while( least->slesser.raw ) {
					{
						INDEX idx;
						uint64_t* c;
						DATA_FORALL( pdlVisited, idx, uint64_t*, c ) {
							if( c[0] == bottom_me_fpi ) {
								lprintf( "going up the tree is broken now." );
								DebugBreak();
							}
						}
					}
					bottom_me_fpi = least->me_fpi;
					AddDataItem( &pdlVisitedLesser, &bottom_me_fpi );
					dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
					least = getRawTimeEntry( vol, leastIndex = least->slesser.ref.index, &cache GRTELog DBG_DELETE_ );
					vtprintf( vol->pvtDeleteBuffer, "move to greater's lesser %d\n", leastIndex );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Stepped to least3 %d", leastIndex );
#endif
				}
				DeleteDataList( &pdlVisitedLesser );
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->sgreater.raw ) {
					enum block_cache_entries cache;
					ptr[0].raw = least->sgreater.raw;
					struct storageTimelineNode* leastGreater = getRawTimeEntry( vol, least->sgreater.ref.index, &cache GRTELog DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "least greater : %d", least->sgreater.ref.index );
#endif
					leastGreater->me_fpi = least->me_fpi;
					vtprintf( vol->pvtDeleteBuffer, "greater's least has a greater %d\n", least->sgreater.ref.index );
					least->sgreater.raw = 0; // now no longer points to a thing.
					SMUDGECACHE( vol, cache );
					bottom_me_fpi = leastGreater->me_fpi;
					dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
				} else {
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Fill parent of least with null2" );
#endif
					vtprintf( vol->pvtDeleteBuffer, "greater's least is the bottom\n", least->slesser.ref.index );
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SMUDGECACHE( vol, meCache );
				SMUDGECACHE( vol, cache );
				dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );

			}
		}

		if( least ) {
			int depth = 0;
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "Moving least node into in-place of deleted node. %d %d", time->me_fpi, leastIndex );
#endif
			ptr[0].ref.index = leastIndex;

			//least->me_fpi = time->me_fpi;

			// the old node that pointed to 'the lesser' is being deleted, no need to update him.
			if( least->slesser.raw = time->slesser.raw ) {
				enum block_cache_entries cache;
				struct storageTimelineNode* tmp;
				tmp = getRawTimeEntry( vol, time->slesser.ref.index, &cache GRTELog DBG_DELETE_ );
				tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].slesser );
				if( tmp->sgreater.ref.depth > tmp->slesser.ref.depth )
					depth = tmp->sgreater.ref.depth;
				else
					depth = tmp->slesser.ref.depth;
				//if( least->slesser.ref.depth != depth+1 )
				//	DebugBreak();
				vtprintf( vol->pvtDeleteBuffer, "least's new lesser is %d  %d %d\n", least->slesser.ref.index, least->slesser.ref.depth, depth + 1 );
				SMUDGECACHE( vol, cache );
				dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
			}
			// the old node that pointed to 'the greater' is being deleted, no need to update him.
			if( least->sgreater.raw = time->sgreater.raw ) {
				enum block_cache_entries cache;
				struct storageTimelineNode* tmp;
				tmp = getRawTimeEntry( vol, time->sgreater.ref.index, &cache GRTELog DBG_DELETE_ );
				tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].sgreater );
				if( tmp->sgreater.ref.depth > tmp->slesser.ref.depth )
					depth = tmp->sgreater.ref.depth;
				else
					depth = tmp->slesser.ref.depth;
				//if( least->sgreater.ref.depth != depth + 1 )
				//	DebugBreak();
				vtprintf( vol->pvtDeleteBuffer, "least's new greater is %d  %d %d\n", least->sgreater.ref.index, least->sgreater.ref.depth, depth + 1 );
				SMUDGECACHE( vol, cache );
				dropRawTimeEntry( vol, cache GRTELog DBG_DELETE_ );
			}

			if( least->slesser.ref.depth > least->sgreater.ref.depth )
				ptr[0].ref.depth = least->slesser.ref.depth + 1;
			else
				ptr[0].ref.depth = least->sgreater.ref.depth + 1;

			SMUDGECACHE( vol, cache ); // mark that the least node was updated.
			SMUDGECACHE( vol, meCache );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( " ------------- DELETE TIME ENTRY move least node to node? ---------------- " );
			DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
			//least = NULL;
		}

		// this is the incoming thing...
		// I think this was already dropped really(?)
		{
			uint64_t node_fpi;
			int updating = 1;

			// bottom->parent
			node_fpi = bottom_me_fpi & ~0x3f;

			while( node_fpi > 0x3f ) {
				uint64_t node_idx = ( node_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) + 1;
				{
					INDEX idx;
					uint64_t* c;
					DATA_FORALL( pdlVisited, idx, uint64_t*, c ) {
						if( c[0] == node_fpi ) {
							lprintf( "going up the tree is broken now." );
							DebugBreak();
						}
					}
				}
				AddDataItem( &pdlVisited, &node_fpi );
				tmp = getRawTimeEntry( vol, node_idx, &cacheTmp GRTELog DBG_DELETE_ );
				node_fpi = tmp->me_fpi & ~0x3f;
				if( node_fpi == ( bottom_me_fpi & ~0x3f ) ) DebugBreak();
				vtprintf( vol->pvtDeleteBuffer, "going up the tree %d\n", node_idx );
				LogTimelineTree( vol->pvtDeleteBuffer, vol, node_idx DBG_SRC );

				if( updating ) {
					if( tmp->slesser.raw ) {
						if( tmp->sgreater.raw ) {
							int tmp1, tmp2;
							if( ( tmp1 = tmp->slesser.ref.depth ) > ( tmp2 = tmp->sgreater.ref.depth ) ) {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
								if( ptr[0].ref.depth != ( tmp1 + 1 ) ) {
									ptr[0].ref.depth = tmp1 + 1;
									SMUDGECACHE( vol, meCache );
								}
								else
									updating = 0;

								//dropRawTimeEntry( vol, meCache GRTELog DBG_DELETE_ );
								//backtrack->depth = tmp1 + 1;
								{
									if( ( tmp1 - tmp2 ) > 1 ) {
										int tmp3, tmp4;
										enum block_cache_entries cache;
										struct storageTimelineNode* lesser = getRawTimeEntry( vol, tmp->slesser.ref.index, &cache GRTELog DBG_DELETE_ );
										tmp3 = lesser->slesser.ref.depth;
										tmp4 = lesser->sgreater.ref.depth;
										struct storageTimelineNode* _y;
										_y = lesser;
#ifdef DEBUG_DELETE_BALANCE
										lprintf( "y node is %d", tmp->slesser.ref.index );
#endif
										if( tmp3 >= tmp4 ) {
											_os_AVL_RotateToRight( vol, tmp, node_idx, _y, tmp->slesser.ref.index DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to right ---------------- " );
											DumpTimelineTree( vol, TRUE DBG_DELETE_ );
#endif
											SMUDGECACHE( vol, cacheTmp );
											SMUDGECACHE( vol, cache );
#ifdef DEBUG_VALIDATE_TREE
											ValidateTimelineTree( vol DBG_SRC );
#endif
										}
										else {
											struct storageTimelineNode* _x;
											enum block_cache_entries cache_x;
											BLOCKINDEX lessGreater = lesser->sgreater.ref.index;
											_x = getRawTimeEntry( vol, lesser->sgreater.ref.index, &cache_x GRTELog DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- to TIME ENTRY after rotate to left ---------------- %d %d ", node_idx, lessGreater );
#endif
											_os_AVL_RotateToLeft( vol, _y, tmp->slesser.ref.index, _x, lessGreater DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to left ---------------- %d %d ", node_idx, lesser->sgreater.ref.index );
											DumpTimelineTree( vol, TRUE DBG_DELETE_ );
#endif
											_os_AVL_RotateToRight( vol, tmp, node_idx, _x, lessGreater DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to right ---------------- %d %d", node_idx, lessGreater );
											DumpTimelineTree( vol, TRUE DBG_DELETE_ );
#endif
											SMUDGECACHE( vol, cacheTmp );
											SMUDGECACHE( vol, cache );
											SMUDGECACHE( vol, cache_x );
											dropRawTimeEntry( vol, cache_x GRTENoLog DBG_DELETE_ );
#ifdef DEBUG_VALIDATE_TREE
											ValidateTimelineTree( vol DBG_SRC );
#endif
										}
										dropRawTimeEntry( vol, cache GRTENoLog DBG_DELETE_ );
									}
								}
							} else {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
								if( ptr[0].ref.depth != (tmp2 + 1) ) {
									ptr[0].ref.depth = tmp2 + 1;
									SMUDGECACHE( vol, meCache );
								}
								else
									updating = 0;
								//dropRawTimeEntry( vol, meCache GRTENoLog DBG_DELETE_ );
								//backtrack->depth = tmp2 + 1;
								{
									if( ( tmp2 - tmp1 ) > 1 ) {
										int tmp3, tmp4;
										enum block_cache_entries cache;
										struct storageTimelineNode* lesser = getRawTimeEntry( vol, tmp->sgreater.ref.index, &cache GRTELog DBG_DELETE_ );
										tmp3 = lesser->slesser.ref.depth;
										tmp4 = lesser->sgreater.ref.depth;
										struct storageTimelineNode* _y;
										_y = lesser;

										if( tmp4 >= tmp3 ) {
											_os_AVL_RotateToLeft( vol, tmp, node_idx, _y, tmp->sgreater.ref.index DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to left ---------------- " );
											DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
											SMUDGECACHE( vol, cacheTmp );
											SMUDGECACHE( vol, cache );
#ifdef DEBUG_VALIDATE_TREE
											ValidateTimelineTree( vol DBG_SRC );
#endif
										}
										else {
											struct storageTimelineNode *_x;
											enum block_cache_entries cache_x;
											BLOCKINDEX greatLesser = lesser->slesser.ref.index;
											_x = getRawTimeEntry( vol, lesser->slesser.ref.index, &cache_x GRTELog DBG_DELETE_ );
											_os_AVL_RotateToRight( vol, _y, tmp->sgreater.ref.index, _x, greatLesser DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to right ---------------- " );
											DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
											_os_AVL_RotateToLeft( vol, tmp, node_idx, _x, greatLesser DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY  after rotte to left---------------- " );
											DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
											SMUDGECACHE( vol, cacheTmp );
											SMUDGECACHE( vol, cache );
											SMUDGECACHE( vol, cache_x );
											dropRawTimeEntry( vol, cache_x GRTENoLog DBG_DELETE_ );
#ifdef DEBUG_VALIDATE_TREE
											ValidateTimelineTree( vol DBG_SRC );
#endif
										}
										dropRawTimeEntry( vol, cache GRTENoLog DBG_DELETE_ );
									}
								}
							}
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp->slesser.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp->slesser.ref.depth + 1;
								SMUDGECACHE( vol, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY lesser ony ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
						}
					} else {
						if( tmp->sgreater.raw ) {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp->sgreater.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp->sgreater.ref.depth + 1;
								SMUDGECACHE( vol, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY greater no lesser? ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth ) {
								ptr[0].ref.depth = 1; // my parent has me as the node.
								SMUDGECACHE( vol, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY no greater no lesser? ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
						}
					}
				}
				dropRawTimeEntry( vol, cacheTmp GRTENoLog DBG_DELETE_ );
#ifdef DEBUG_VALIDATE_TREE
				ValidateTimelineTree( vol DBG_SRC );
#endif
			}

			{
				struct storageTimeline* timeline = vol->timeline;
				time->sgreater.ref.index = timeline->header.first_free_entry.ref.index;
				timeline->header.first_free_entry.ref.index = index;
				SMUDGECACHE( vol, vol->timelineCache );
				SMUDGECACHE( vol, cache );
			}

		}


	}
#ifdef DEBUG_DELETE_BALANCE
	lprintf( " ------------- DELETE TIME ENTRY ---------------- " );
	DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
	//node->
}

static void deleteTimelineIndex( struct sack_vfs_os_volume* vol, BLOCKINDEX index ) {
	BLOCKINDEX next;
	do {
		struct storageTimelineNode* time;
		enum block_cache_entries cache = BC( TIMELINE );

		lprintf( "Delete start... %d", index );
		time = getRawTimeEntry( vol, index, &cache GRTELog DBG_SRC );
		next = time->prior.raw;
		nodes--;
		checkRoot( vol );
		if( !nodes ) {
			lprintf( "CurNode: (%s -> %5d  %d <-%d %s has children %d %d  with depths of %d %d"
				, "*"
				, (int)time->dirent_fpi
				, (int)index
				, time->me_fpi >> 6
				, ( ( time->me_fpi & 0x3f ) == 0x20 ) ? "L"
				: ( ( time->me_fpi & 0x3f ) == 0x10 ) ? "R"
				: "G"
				, (int)( time->slesser.ref.index )
				, (int)( time->sgreater.ref.index )
				, (int)( time->slesser.ref.depth )
				, (int)( time->sgreater.ref.depth )
			);
		}
		deleteTimelineIndexWork( vol, index, time, cache DBG_SRC );
		dropRawTimeEntry( vol, cache GRTELog DBG_SRC );
#ifdef DEBUG_VALIDATE_TREE
		//ValidateTimelineTree( vol DBG_SRC );
#endif
		//lprintf( "Delete done... %d", index );
	} while( index = next );
	checkRoot( vol );
	if( !nodes && vol->timeline->header.srootNode.ref.index ) {
		lprintf( "No more nodes, but the root points at something." );
		DebugBreak();
	}
	//lprintf( "Root is now %d %d", nodes, vol->timeline->header.srootNode.ref.index );
}

BLOCKINDEX getTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL keepDirent, void(*init)(uintptr_t, struct memoryTimelineNode*), uintptr_t psv DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE );
	enum block_cache_entries cache_last = BC( TIMELINE );
	enum block_cache_entries cache_free = BC( TIMELINE );
	enum block_cache_entries cache_new = BC( TIMELINE );
	FPI orig_dirent;
	struct storageTimeline* timeline = vol->timeline;
	TIMELINE_BLOCK_TYPE freeIndex;
	BLOCKINDEX index;
	BLOCKINDEX priorIndex = time->index;

	freeIndex.ref.index = timeline->header.first_free_entry.ref.index;
	freeIndex.ref.depth = 0;

	// update next free.
	reloadTimeEntry( time, vol, index = freeIndex.ref.index VTReadWrite GRTELog DBG_RELAY );

	if( time->disk->sgreater.ref.index ) {
		timeline->header.first_free_entry.ref.index = time->disk->sgreater.ref.index;
	}
	else {
		timeline->header.first_free_entry.ref.index = timeline->header.first_free_entry.ref.index + 1;
	}
	SMUDGECACHE( vol, vol->timelineCache );

	// make sure the new entry is emptied.
	time->disk->slesser.raw = timelineBlockIndexNull;
	time->disk->sgreater.raw = timelineBlockIndexNull;

	time->disk->time = timeGetTime64ns();

	time->disk->prior.raw = priorIndex;

	{
		int tz = GetTimeZone();
		if( tz < 0 )
			tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
		else
			tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
		time->disk->timeTz = tz;
	}

	if( init ) init( psv, time );
	nodes++;
	//lprintf( "Add start... %d", freeIndex.ref.index );
	hangTimelineNode( vol
		, freeIndex
		, 0
		, timeline
		, time->disk
		DBG_RELAY );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol, FALSE DBG_RELAY ); // don't drop; returning this one.

#ifdef DEBUG_DELETE_BALANCE
	lprintf( " ------------- NEW TIME ENTRY ---------------- " );
	DumpTimelineTree( vol, TRUE  DBG_RELAY );
#endif
#ifdef DEBUG_VALIDATE_TREE
	ValidateTimelineTree( vol DBG_SRC );
#endif
	//DumpTimelineTree( vol, FALSE );
	return index;
}

BLOCKINDEX updateTimeEntryTime( struct memoryTimelineNode* time
			, struct sack_vfs_os_volume *vol, uint64_t index
			, LOGICAL allocateNew
			, void( *init )( uintptr_t, struct memoryTimelineNode* ), uintptr_t psv DBG_PASS ) {
	if( allocateNew ) {
		if( time ) {
			uint64_t inputIndex = time ? time->index : index;
			// gets a new timestamp.
			enum block_cache_entries inputCache = time ? time->diskCache : BC( ZERO );
			BLOCKINDEX newIndex = getTimeEntry( time, vol, TRUE, init, psv DBG_RELAY );
			time->disk->prior.raw = inputIndex;
			updateTimeEntry( time, vol, FALSE DBG_RELAY );
			dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );
			return newIndex;
		}
		else {
			struct memoryTimelineNode time_;
			struct storageTimelineNode* timeold;
			uint64_t inputIndex = index;
			enum block_cache_entries inputCache;
			FPI dirent_fpi;
			timeold = getRawTimeEntry( vol, index, &inputCache GRTELog DBG_RELAY );
			dirent_fpi = timeold->dirent_fpi;
			dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );

			// gets a new timestamp.
			time_.index = index;
			BLOCKINDEX newIndex = getTimeEntry( &time_, vol, TRUE, init, psv DBG_RELAY );
			time_.disk->prior.raw = inputIndex;
			time_.disk->dirent_fpi = dirent_fpi;
			updateTimeEntry( &time_, vol, TRUE DBG_RELAY );
			return newIndex;
		}
	}
	else {
		reloadTimeEntry( time, vol, index VTReadWrite GRTENoLog DBG_RELAY );
		{
			int tz = GetTimeZone();
			if( tz < 0 )
				tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
			else
				tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
			time->disk->timeTz = tz;
		}
		time->disk->time = timeGetTime64ns();
		updateTimeEntry( time, vol, FALSE DBG_RELAY );
		return index;
	}
}
