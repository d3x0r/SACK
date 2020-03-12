#define DEBUG_TEST_LOCKS
//#define  DEBUG_LOG_LOCKS

//#define INVERSE_TEST
//#define DEBUG_DELETE_BALANCE

#define DEBUG_AVL_DETAIL

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


struct storageTimelineNode* getRawTimeEntry( struct sack_vfs_os_volume* vol, uint64_t timeEntry, enum block_cache_entries *cache DBG_PASS )
{
	int locks;
	cache[0] = BC( TIMELINE );
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, cache, TIME_BLOCK_SIZE );

	locks = GETMASK_( vol->seglock, seglock, cache[0] );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
	_lprintf(DBG_RELAY)( "Lock %d %d", cache[0], locks );
#endif
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
	return (TIMELINE_BLOCK_TYPE*)vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, fpi, cache, TIME_BLOCK_SIZE );
}

void dropRawTimeEntry( struct sack_vfs_os_volume* vol, enum block_cache_entries cache DBG_PASS ) {
	int locks;
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
	_lprintf(DBG_RELAY)( "UnLock %d %d", cache, locks );
#endif
	if( !locks ) {
		lprintf( "Lock UNDERFLOW" );
		DebugBreak();
	}
#endif
	locks--;
	SETMASK_( vol->seglock, seglock, cache, locks );
}

void reloadTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, uint64_t timeEntry DBG_PASS )
{
	enum block_cache_entries cache = BC( TIMELINE );
	//uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index ) {
	//if( timeEntry > 62 )DebugBreak();
	int locks;
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, &cache, TIME_BLOCK_SIZE );
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
	_lprintf(DBG_RELAY)( "Lock %d %d", cache, locks );
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

	/*
	time->dirent_fpi = (FPI)(node->dirent_fpi);

	time->prior = node->prior;
	time->time = node->time;
	time->priorData = node->priorData;
	time->bits.priorDataPad = node->priorDataPad ;
	time->bits.timeTz = node->timeTz;

	//time->time = node->time;
	time->slesser.raw = node->slesser.raw;
	time->sgreater.raw = node->sgreater.raw;
	time->me_fpi = node->me_fpi;

	*/
	//LoG( "Set this FPI: %d  %d", (int)timeEntry, (int)time->this_fpi );
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
		reloadTimeEntry( &curNode, vol, parent->disk->slesser.ref.index DBG_RELAY );
#if 0
		lprintf( "(lesser) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache DBG_RELAY );
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
		reloadTimeEntry( &curNode, vol, parent->disk->sgreater.ref.index DBG_RELAY );
#if 0
		lprintf( "(greater) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused DBG_RELAY );
		dropRawTimeEntry( vol, curNode.diskCache DBG_RELAY );
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
			, timeline->header.srootNode.ref.index  DBG_RELAY );
	}
	DumpTimelineTreeWork( vol, 0, &curNode, unused DBG_RELAY );
	dropRawTimeEntry( vol, curNode.diskCache DBG_RELAY );
}




//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL drop DBG_PASS ) {
#if 0
	FPI timeEntry = time->this_fpi;

	enum block_cache_entries cache = BC( TIMELINE );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0, time->this_fpi, &cache, TIME_BLOCK_SIZE );

	node->dirent_fpi = time->dirent_fpi;

	node->prior = time->prior;
	node->priorData = time->priorData;
	node->priorDataPad = time->bits.priorDataPad;
	node->timeTz = time->bits.timeTz;
	node->time = time->time;
	node->slesser.raw = time->slesser.raw;
	node->sgreater.raw = time->sgreater.raw;
	node->me_fpi = time->me_fpi;
#endif
	SETFLAG( vol->dirty, time->diskCache );
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
		struct storageTimelineNode* prior = getRawTimeEntry( vol, time->disk->prior.raw, &cache DBG_SRC );
		while( prior->prior.raw ) {
			dropRawTimeEntry( vol, cache DBG_RELAY );
			prior = getRawTimeEntry( vol, prior->prior.raw, &cache DBG_RELAY );
		}
		decoded_dirent->ctime = prior->time;
		dropRawTimeEntry( vol, cache DBG_RELAY );
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
		const char* dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache, NAME_BLOCK_SIZE );
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
				dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset + partial, &cache, NAME_BLOCK_SIZE );
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
		SETFLAG( vol->dirty, meCache );

		node->slesser.raw = left_->sgreater.raw;

		{
			struct storageTimelineNode* x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, node->slesser.ref.index, &cache DBG_RELAY );
			x->me_fpi = sane_offsetof( struct storageTimeline, entries[nodeIdx - 1].slesser );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "x fpi = nodeIdx" );
#endif
			SETFLAG( vol->dirty, cache );
			dropRawTimeEntry( vol, cache DBG_RELAY );
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
		SETFLAG( vol->dirty, meCache );

		node->sgreater.raw = right_->slesser.raw;
		{
			struct storageTimelineNode *x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, node->sgreater.ref.index, &cache DBG_RELAY );
			x->me_fpi = sane_offsetof( struct storageTimeline, entries[nodeIdx - 1].sgreater );
			dropRawTimeEntry( vol, cache DBG_RELAY );
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

	_z = getRawTimeEntry( vol, curIndex, &cache_z DBG_RELAY );

	{
		while( _z ) {
			int doBalance;
			rightDepth = (int)_z->sgreater.ref.depth;
			leftDepth = (int)_z->slesser.ref.depth;
			if( _z->me_fpi & ~0x3f ) {
				tmp = getRawTimeEntry( vol, ( ( _z->me_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) ) + 1, & cache_tmp DBG_RELAY );
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
							dropRawTimeEntry( vol, cache_tmp DBG_RELAY );
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
								dropRawTimeEntry( vol, cache_tmp DBG_RELAY );
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
							dropRawTimeEntry( vol, cache_tmp DBG_RELAY );
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
								dropRawTimeEntry( vol, cache_tmp DBG_RELAY );
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
				SETFLAG( vol->dirty, cache_tmp );
				dropRawTimeEntry( vol, cache_tmp DBG_RELAY );
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
							SETFLAG( vol->dirty, cache_y );
							SETFLAG( vol->dirty, cache_z );

#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R1 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
						}
						else {
							//left/rightDepth
							_os_AVL_RotateToRight( vol, _y, idx_y, _x, idx_x DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R2 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							_os_AVL_RotateToLeft( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L1 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SETFLAG( vol->dirty, cache_z );
							SETFLAG( vol->dirty, cache_y );
							SETFLAG( vol->dirty, cache_x );
						}
					}
					else {
						if( idx_y == _z->slesser.ref.index ) {
							_os_AVL_RotateToLeft( vol, _y, idx_y, _x, idx_x DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L2 ---------------- " );
#endif
							_os_AVL_RotateToRight( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE R3 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SETFLAG( vol->dirty, cache_z );
							SETFLAG( vol->dirty, cache_y );
							SETFLAG( vol->dirty, cache_x );
							// rightDepth.left
						}
						else {
							//rightDepth/rightDepth
							_os_AVL_RotateToLeft( vol, _z, curIndex, _y, idx_y DBG_RELAY );
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- AFTER AVL BALANCER PHASE L3 ---------------- " );
							DumpTimelineTree( vol, TRUE DBG_RELAY );
#endif
							SETFLAG( vol->dirty, cache_y );
							SETFLAG( vol->dirty, cache_z );
						}
					}
#ifdef DEBUG_TIMELINE_AVL
					lprintf( "WR Balanced, should redo this one... %d %d", (int)_z->slesser.ref.index, _z->sgreater.ref.index );
#endif
				}
				else {
					//lprintf( "Not deep enough for balancing." );
				}
			}
			if( _x )
				dropRawTimeEntry( vol, cache_x DBG_RELAY );
			cache_x = cache_y;
			idx_x = idx_y;
			_x = _y;
			cache_y = cache_z;
			idx_y = curIndex;
			_y = _z;
			if( _z->me_fpi >= sizeof( struct timelineHeader ) ) {
				curIndex = ( ( _z->me_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) ) + 1;
				_z = getRawTimeEntry( vol, curIndex, &cache_z DBG_RELAY );
			} else {
				_z = NULL;
			}
			//_z = (struct memoryTimelineNode*)PopData( pdsStack );
		}

		if( _x )
			dropRawTimeEntry( vol, cache_x DBG_RELAY );
		if( _y )
			dropRawTimeEntry( vol, cache_y DBG_RELAY );
		if( _z )
			dropRawTimeEntry( vol, cache_z DBG_RELAY );
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
	//struct memoryTimelineNode curNode;
	//struct memoryTimelineNode* curNode_;
	//struct memoryTimelineNode* root;
	//struct memoryTimelineNode* parent;
	uint64_t curindex;
	enum block_cache_entries cacheCur;
	/*
	while( 1 ) {
		root = (struct memoryTimelineNode*)PeekData( pdsStack );
		if( !root )
			break;
		parent = (struct memoryTimelineNode*)PeekDataEx( pdsStack, 1 );
		if( !parent )
			break;

		if( parent->sgreater.ref.index == root->index ) {
			if( timelineNode->time < parent->time ) {
				// this belongs to the lesser side of parent...
				PopData( pdsStack );
				continue;
			}
		}
		if( parent->slesser.ref.index == root->index ) {
			if( timelineNode->time > parent->time ) {
				// this belongs to the greater isde of parent...
				PopData( pdsStack );
				continue;
			}
		}
		break;
	}
	*/

	//if( !root )
		{
			if( !timeline->header.srootNode.ref.index ) {
				timeline->header.srootNode.ref.index = index.ref.index ;
				timeline->header.srootNode.ref.depth = 0 ;
				timelineNode->me_fpi = offsetof( struct timelineHeader, srootNode );
				return 1;
			}

			pCurNode = getRawTimeEntry( vol, curindex = timeline->header.srootNode.ref.index, &cacheCur DBG_SRC );
		/*
			reloadTimeEntry( &curNode, vol
				, curindex = timeline->header.srootNode.ref.index  DBG_SRC );
			PushData( pdsStack, &curNode );
		*/
		}
	//else
		// the top of the stack is already setup to peek at.

	//check = root->tree;
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
				dropRawTimeEntry( vol, cacheCur DBG_SRC );
				pCurNode = getRawTimeEntry( vol, curindex = nextIndex, &cacheCur DBG_SRC );
			}
			else {

				pCurNode->slesser.ref.index = index.ref.index;
				pCurNode->slesser.ref.depth = 0;

				timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex - 1].slesser );
				SETFLAG( vol->dirty, cacheCur );
				//updateTimeEntry( curNode_, vol DBG_SRC );
				dropRawTimeEntry( vol, cacheCur DBG_SRC );
				break;
			}
		}
		else if( dir > 0 )
			if( nextIndex = pCurNode->sgreater.ref.index ) {
				dropRawTimeEntry( vol, cacheCur DBG_SRC );
				pCurNode = getRawTimeEntry( vol, curindex = nextIndex, &cacheCur DBG_SRC );
			}
			else {
				pCurNode->sgreater.ref.index = index.ref.index;
				pCurNode->sgreater.ref.depth = 0;

				timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex-1].sgreater );
				SETFLAG( vol->dirty, cacheCur );
				dropRawTimeEntry( vol, cacheCur DBG_SRC );
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
					dropRawTimeEntry( vol, cacheCur DBG_SRC );
					pCurNode = getRawTimeEntry( vol, curindex = nextLesserIndex, &cacheCur DBG_SRC );
				}
				else {
					{
						pCurNode->slesser.ref.index = index.ref.index;
						pCurNode->slesser.ref.depth = 0;
					}
					timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex - 1].slesser );
					SETFLAG( vol->dirty, cacheCur );
					dropRawTimeEntry( vol, cacheCur DBG_SRC );
					break;
				}
			}
			else {
				if( nextGreaterIndex ) {
					dropRawTimeEntry( vol, cacheCur DBG_SRC );
					pCurNode = getRawTimeEntry( vol, curindex = nextGreaterIndex, &cacheCur DBG_SRC );
				}
				else {
					{
						pCurNode->sgreater.ref.index = index.ref.index;
						pCurNode->sgreater.ref.depth = 0;
					}
					timelineNode->me_fpi = sane_offsetof( struct storageTimeline, entries[curindex-1].sgreater );
					SETFLAG( vol->dirty, cacheCur );
					dropRawTimeEntry( vol, cacheCur DBG_SRC );
					break;
				}
			}
		}
	}
#ifdef DEBUG_DELETE_BALANCE
	lprintf( " ------------- BEFORE AVL BALANCER ---------------- " );
	DumpTimelineTree( vol, TRUE  DBG_RELAY );
#endif
	_os_AVLbalancer( vol, index.ref.index DBG_RELAY );
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

		//time = getRawTimeEntry( vol, index, &cache );
#ifdef DEBUG_DELETE_BALANCE
		lprintf( "delete index %d", index );
#endif
		if( !time->slesser.ref.index ) {
			if( time->sgreater.ref.index ) {
				enum block_cache_entries cache;
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				struct storageTimelineNode *greater = getRawTimeEntry( vol, time->sgreater.ref.index, &cache DBG_DELETE_ );
				ptr[0] = time->sgreater;
				greater->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );

#ifdef DEBUG_DELETE_BALANCE
				lprintf( "had only greater" );
#endif
				bottom_me_fpi = greater->me_fpi;
				dropRawTimeEntry( vol, cache DBG_DELETE_ );
				//bottom = greater;
			} else {
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				ptr[0].raw = timelineBlockIndexNull;
				SETFLAG( vol->dirty, meCache );
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Leaf node; no left or right" );
#endif
				// no greater or lesser...
				bottom_me_fpi = time->me_fpi;
				//bottom = time;
				//bottomCache = BC(ZERO);
			}
		} else if( !time->sgreater.ref.index ) {
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
			enum block_cache_entries cache;
			struct storageTimelineNode* lesser = getRawTimeEntry( vol, time->slesser.ref.index, &cache DBG_DELETE_ );
			ptr[0].raw = time->slesser.raw;
			lesser->me_fpi = time->me_fpi;
			SETFLAG( vol->dirty, meCache );
			bottom_me_fpi = lesser->me_fpi;
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "had only lesser" );
#endif
			dropRawTimeEntry( vol, cache DBG_DELETE_ );
		} else {

#ifdef DEBUG_DELETE_BALANCE
			lprintf( "has greater and lesser %d %d", time->slesser.ref.depth, time->sgreater.ref.depth );
#endif
			if( time->slesser.ref.depth > time->sgreater.ref.depth ) {
				enum block_cache_entries leastCache;
				bottom_me_fpi = time->me_fpi;
				//bottom = time;
				//bottomCache = BC( ZERO );
				
				least = getRawTimeEntry( vol, leastIndex = time->slesser.ref.index, &leastCache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Stepped to least %d", leastIndex );
#endif
				while( least->sgreater.raw ) {
					bottom_me_fpi = least->me_fpi;
					dropRawTimeEntry( vol, leastCache DBG_DELETE_ );
					least = getRawTimeEntry( vol, leastIndex = least->sgreater.ref.index, &leastCache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Stepped to least1 %d", leastIndex );
#endif
				}

				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->slesser.raw ) {
					enum block_cache_entries cache;
					struct storageTimelineNode* leastLesser = getRawTimeEntry( vol, least->slesser.ref.index, &cache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Least has a lesser node (off of greatest on least) %d", least->slesser.ref.index );
#endif
					leastLesser->me_fpi = least->me_fpi;
					SETFLAG( vol->dirty, cache );
					ptr[0].raw = least->slesser.raw;
					least->slesser.raw = 0; // now no longer points to a thing.
					bottom_me_fpi = leastLesser->me_fpi;
					dropRawTimeEntry( vol, cache DBG_DELETE_ );
	}
				else {
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Fill parent of least with null" );
#endif
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, leastCache DBG_DELETE_ );
				// bottom contains this reference, and will release later.
				//dropRawTimeEntry( vol, leastCache DBG_DELETE_ );
			}else {
				enum block_cache_entries cache = BC( ZERO );

				least = getRawTimeEntry( vol, leastIndex = time->slesser.ref.index, &cache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
				lprintf( "Stepped to least2 %d", leastIndex );
#endif
				while( least->sgreater.raw ) {
					dropRawTimeEntry( vol, cache DBG_DELETE_ );
					least = getRawTimeEntry( vol, leastIndex = least->slesser.ref.index, &cache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Stepped to least3 %d", leastIndex );
#endif
				}
				bottom_me_fpi = least->me_fpi;
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->sgreater.raw ) {
					enum block_cache_entries cache;
					ptr[0].raw = least->sgreater.raw;
					struct storageTimelineNode* leastGreater = getRawTimeEntry( vol, least->sgreater.ref.index, &cache DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "least greater : %d", least->sgreater.ref.index );
#endif
					leastGreater->me_fpi = least->me_fpi;
					least->sgreater.raw = 0; // now no longer points to a thing.
					SETFLAG( vol->dirty, cache );
					bottom_me_fpi = leastGreater->me_fpi;
					dropRawTimeEntry( vol, cache DBG_DELETE_ );
				} else {
#ifdef DEBUG_DELETE_BALANCE
					lprintf( "Fill parent of least with null2" );
#endif
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, cache DBG_DELETE_ );

			}
		}
		if( least ) {
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
#ifdef DEBUG_DELETE_BALANCE
			lprintf( "Moving least node into in-place of deleted node. %d %d", time->me_fpi, leastIndex );
#endif
			ptr[0].ref.index = leastIndex;

			//least->me_fpi = time->me_fpi;

			if( least->slesser.raw = time->slesser.raw ) {
				enum block_cache_entries cache;
				struct storageTimelineNode* tmp;
				tmp = getRawTimeEntry( vol, time->slesser.ref.index, &cache DBG_DELETE_ );
				tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].slesser );
				dropRawTimeEntry( vol, cache DBG_DELETE_ );
			}
			if( least->sgreater.raw = time->sgreater.raw ) {
				enum block_cache_entries cache;
				struct storageTimelineNode* tmp;
				tmp = getRawTimeEntry( vol, time->sgreater.ref.index, &cache DBG_DELETE_ );
				tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].sgreater );
				dropRawTimeEntry( vol, cache DBG_DELETE_ );
			}
#ifdef DEBUG_DELETE_BALANCE
			lprintf( " ------------- DELETE TIME ENTRY move least node to node? ---------------- " );
			DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
			least = NULL;
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
				tmp = getRawTimeEntry( vol, node_idx, &cacheTmp DBG_DELETE_ );
				
				if( updating ) {
					if( tmp->slesser.raw ) {
						if( tmp->sgreater.raw ) {
							int tmp1, tmp2;
							if( ( tmp1 = tmp->slesser.ref.depth ) > ( tmp2 = tmp->sgreater.ref.depth ) ) {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
								if( ptr[0].ref.depth != ( tmp1 + 1 ) ) {
									ptr[0].ref.depth = tmp1 + 1;
									SETFLAG( vol->dirty, meCache );
								}
								else
									updating = 0;

								//dropRawTimeEntry( vol, meCache DBG_DELETE_ );
								//backtrack->depth = tmp1 + 1;
								{
									if( ( tmp1 - tmp2 ) > 1 ) {
										int tmp3, tmp4;
										enum block_cache_entries cache;
										struct storageTimelineNode* lesser = getRawTimeEntry( vol, tmp->slesser.ref.index, &cache DBG_DELETE_ );
										tmp3 = lesser->slesser.ref.depth;
										tmp4 = lesser->sgreater.ref.depth;
										struct storageTimelineNode *_y;
										_y = lesser;
#ifdef DEBUG_DELETE_BALANCE
										lprintf( "y node is %d", tmp->slesser.ref.index );
#endif
										if( tmp3 > tmp4 ) {
											_os_AVL_RotateToRight( vol, tmp, node_idx, _y, tmp->slesser.ref.index DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to right ---------------- " );
											DumpTimelineTree( vol, TRUE DBG_DELETE_ );
#endif
										}
										else {
											struct storageTimelineNode *_x;
											enum block_cache_entries cache_x;
											BLOCKINDEX lessGreater = lesser->sgreater.ref.index;
											_x = getRawTimeEntry( vol, lesser->sgreater.ref.index, &cache_x DBG_DELETE_ );
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
											dropRawTimeEntry( vol, cache_x DBG_DELETE_ );
										}
										dropRawTimeEntry( vol, cache DBG_DELETE_ );
									}
								}
							} else {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
								if( ptr[0].ref.depth != (tmp2 + 1) ) {
									ptr[0].ref.depth = tmp2 + 1;
									SETFLAG( vol->dirty, meCache );
								}
								else
									updating = 0;
								//dropRawTimeEntry( vol, meCache DBG_DELETE_ );
								//backtrack->depth = tmp2 + 1;
								{
									if( ( tmp2 - tmp1 ) > 1 ) {
										int tmp3, tmp4;
										enum block_cache_entries cache;
										struct storageTimelineNode* lesser = getRawTimeEntry( vol, tmp->sgreater.ref.index, &cache DBG_DELETE_ );
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
										}
										else {
											struct storageTimelineNode *_x;
											enum block_cache_entries cache_x;
											BLOCKINDEX greatLesser = lesser->slesser.ref.index;
											_x = getRawTimeEntry( vol, lesser->slesser.ref.index, &cache_x DBG_DELETE_ );
											_os_AVL_RotateToRight( vol, _y, tmp->slesser.ref.index, _x, greatLesser DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY after rotate to right ---------------- " );
											DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
											_os_AVL_RotateToLeft( vol, tmp, node_idx, _x, greatLesser DBG_DELETE_ );
#ifdef DEBUG_DELETE_BALANCE
											lprintf( " ------------- DELETE TIME ENTRY  after rotte to left---------------- " );
											DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
											dropRawTimeEntry( vol, cache_x DBG_DELETE_ );
										}
										dropRawTimeEntry( vol, cache DBG_DELETE_ );
									}
								}
							}
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp->slesser.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp->slesser.ref.depth + 1;
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY lesser ony ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
							//dropRawTimeEntry( vol, meCache DBG_DELETE_ );
							//backtrack->depth = backtrack->lesser->depth + 1;
						}
					} else {
						if( tmp->sgreater.raw ) {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp->sgreater.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp->sgreater.ref.depth + 1;
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY greater no lesser? ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
							//dropRawTimeEntry( vol, meCache DBG_DELETE_ );
							//backtrack->depth = backtrack->greater->depth + 1;
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp->me_fpi, &meCache );
							if( ptr[0].ref.depth ) {
								ptr[0].ref.depth = 1; // my parent has me as the node.
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
#ifdef DEBUG_DELETE_BALANCE
							lprintf( " ------------- DELETE TIME ENTRY no greater no lesser? ---------------- " );
							DumpTimelineTree( vol, TRUE  DBG_DELETE_ );
#endif
							//dropRawTimeEntry( vol, meCache DBG_DELETE_ );
						}
					}
				}
				dropRawTimeEntry( vol, cacheTmp DBG_DELETE_ );
				node_fpi = tmp->me_fpi & ~0x3f;
			}

			{
				struct storageTimeline* timeline = vol->timeline;
				time->sgreater.ref.index = timeline->header.first_free_entry.ref.index;
				timeline->header.first_free_entry.ref.index = index;
				SETFLAG( vol->dirty, vol->timelineCache );
				SETFLAG( vol->dirty, cache );
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

		lprintf( "Delete start..." );
		time = getRawTimeEntry( vol, index, &cache DBG_SRC );
		next = time->prior.raw;
		deleteTimelineIndexWork( vol, index, time, cache DBG_SRC );
		dropRawTimeEntry( vol, cache DBG_SRC );
		lprintf( "Delete done..." );
	} while( index = next );
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
	//if( keepDirent ) orig_dirent = time->disk->dirent_fpi;
	reloadTimeEntry( time, vol, index = freeIndex.ref.index DBG_RELAY );
	//if( keepDirent ) time->disk->dirent_fpi = orig_dirent;

	if( time->disk->sgreater.ref.index ) {
		timeline->header.first_free_entry.ref.index = time->disk->sgreater.ref.index;
		SETFLAG( vol->dirty, cache );
	}
	else {
		timeline->header.first_free_entry.ref.index = timeline->header.first_free_entry.ref.index + 1;
		SETFLAG( vol->dirty, cache );
	}

	// make sure the new entry is emptied.
	//time->clesser.ref.index = 0;
	//time->clesser.ref.depth = 0;
	//time->sgreater.ref.index = 0;
	//time->sgreater.ref.depth = 0;

	time->disk->slesser.raw = timelineBlockIndexNull;
	time->disk->sgreater.raw = timelineBlockIndexNull;

	//time->stime.raw = time->ctime.raw =
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

	hangTimelineNode( vol
		, freeIndex
		, 0
		, timeline
		, time->disk
		DBG_RELAY );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol, FALSE DBG_RELAY );

#ifdef DEBUG_DELETE_BALANCE
	lprintf( " ------------- NEW TIME ENTRY ---------------- " );
	DumpTimelineTree( vol, TRUE  DBG_RELAY );
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
			dropRawTimeEntry( vol, inputCache DBG_RELAY );
			return newIndex;
		}
		else {
			struct memoryTimelineNode time_;
			struct storageTimelineNode* timeold;
			uint64_t inputIndex = index;
			enum block_cache_entries inputCache;
			FPI dirent_fpi;
			timeold = getRawTimeEntry( vol, index, &inputCache DBG_RELAY );
			//reloadTimeEntry( &time_, vol, index DBG_RELAY );
			//inputCache = time_.diskCache;
			dirent_fpi = timeold->dirent_fpi;
			dropRawTimeEntry( vol, inputCache DBG_RELAY );

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
		reloadTimeEntry( time, vol, index DBG_RELAY );
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
