

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
	uint64_t unused2[8];
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


struct storageTimelineNode* getRawTimeEntry( struct sack_vfs_os_volume* vol, uint64_t timeEntry, enum block_cache_entries *cache )
{
	int locks;
	cache[0] = BC( TIMELINE );
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, cache, TIME_BLOCK_SIZE );
	SETMASK_( vol->seglock, seglock, cache[0], locks = GETMASK_( vol->seglock, seglock, cache[0] ) + 1 );

	return node;
}

TIMELINE_BLOCK_TYPE* getRawTimePointer( struct sack_vfs_os_volume* vol, uint64_t fpi, enum block_cache_entries *cache ) {
	cache[0] = BC( TIMELINE );
	return (TIMELINE_BLOCK_TYPE*)vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, fpi, cache, TIME_BLOCK_SIZE );
}

void dropRawTimeEntry( struct sack_vfs_os_volume* vol, enum block_cache_entries cache ) {
	int locks;
	SETMASK_( vol->seglock, seglock, cache, locks = GETMASK_( vol->seglock, seglock, cache ) - 1 );
}

void reloadTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, uint64_t timeEntry )
{
	enum block_cache_entries cache = BC( TIMELINE );
	//uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index ) {
	//if( timeEntry > 62 )DebugBreak();
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = (struct storageTimelineNode*)vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, &cache, TIME_BLOCK_SIZE );
	time->index = timeEntry;

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

	time->this_fpi = pos;
	//LoG( "Set this FPI: %d  %d", (int)timeEntry, (int)time->this_fpi );
}



static void DumpTimelineTreeWork( struct sack_vfs_os_volume* vol, int level, struct memoryTimelineNode* parent, LOGICAL unused ) {
	struct memoryTimelineNode curNode;
	static char indent[256];
	int i;
#if 0
	lprintf( "input node %d %d %d", parent->index
		, parent->slesser.ref.index
		, parent->sgreater.ref.index
	);
#endif
	if( parent->slesser.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->slesser.ref.index );
#if 0
		lprintf( "(lesser) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused );
	}
	for( i = 0; i < level * 3; i++ )
		indent[i] = ' ';
	indent[i] = 0;
	lprintf( "CurNode: (%s -> %5d  %d <-%d %s has children %d %d  with depths of %d %d"
		, indent
		, (int)parent->dirent_fpi
		, (int)parent->index
		, parent->me_fpi >> 6
		, ( ( parent->me_fpi & 0x3f ) == 0x20 ) ? "L"
		: ( ( parent->me_fpi & 0x3f ) == 0x10 ) ? "R"
		: "G"
		, (int)(parent->slesser.ref.index)
		, (int)(parent->sgreater.ref.index)
		, (int)(parent->slesser.ref.depth)
		, (int)(parent->sgreater.ref.depth)
	);
	if( parent->sgreater.ref.index ) {
		reloadTimeEntry( &curNode, vol, parent->sgreater.ref.index );
#if 0
		lprintf( "(greater) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, unused );
	}
}

//---------------------------------------------------------------------------

static void DumpTimelineTree( struct sack_vfs_os_volume* vol, LOGICAL unused ) {
	enum block_cache_entries cache = BC( TIMELINE );

	struct storageTimeline* timeline = vol->timeline;// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	SETFLAG( vol->seglock, cache );

	struct memoryTimelineNode curNode;

	{
		if( !timeline->header.srootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, timeline->header.srootNode.ref.index  );
	}
	DumpTimelineTreeWork( vol, 0, &curNode, unused );
}




//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol ) {
	FPI timeEntry = time->this_fpi;

	enum block_cache_entries cache = BC( TIMELINE );
	struct storageTimelineNode* node = (struct storageTimelineNode*)vfs_os_FSEEK( vol, vol->timeline_file, 0, time->this_fpi, &cache, TIME_BLOCK_SIZE );

	node->dirent_fpi = time->dirent_fpi;

	node->prior = time->prior;
	node->priorData = time->priorData;
	node->priorDataPad = time->bits.priorDataPad;
	node->timeTz = time->bits.timeTz;
	node->time = time->time;
	node->slesser.raw = time->slesser.raw;
	node->sgreater.raw = time->sgreater.raw;
	node->me_fpi = time->me_fpi;

	SETFLAG( vol->dirty, cache );
	{
		int n = 0;
		struct memoryTimelineNode* stackNode;
		while( stackNode = (struct memoryTimelineNode*)PeekDataEx( &vol->pdsCTimeStack, n++ ) ) {
			if( (stackNode->index == time->index) && (time != stackNode) ) {
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "CR Found an existing entry that is a copy...." );
#endif
				stackNode[0] = time[0];
				break;
			}
		}
		n = 0;
		while( stackNode = (struct memoryTimelineNode*)PeekDataEx( &vol->pdsWTimeStack, n++ ) ) {
			if( (stackNode->index == time->index) && (time != stackNode) ) {
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "WR Found an existing entry that is a copy...." );
#endif
				stackNode[0] = time[0];
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------

void reloadDirectoryEntry( struct sack_vfs_os_volume* vol, struct memoryTimelineNode* time, struct sack_vfs_os_find_info* decoded_dirent ) {
	enum block_cache_entries cache = BC( DIRECTORY );
	struct directory_entry* dirent;// , * entkey;
	struct directory_hash_lookup_block* dirblock;
	//struct directory_hash_lookup_block* dirblockkey;
	PDATASTACK pdsChars = CreateDataStack( 1 );
	BLOCKINDEX this_dir_block = (time->dirent_fpi >> BLOCK_BYTE_SHIFT);
	BLOCKINDEX next_block;
	dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
	//dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
	dirent = (struct directory_entry*)(((uintptr_t)dirblock) + (time->dirent_fpi & BLOCK_SIZE));
	//entkey = (struct directory_entry*)(((uintptr_t)dirblockkey) + (time->dirent_fpi & BLOCK_SIZE));

	decoded_dirent->vol = vol;

	// all of this regards the current state of a find cursor...
	decoded_dirent->base = NULL;
	decoded_dirent->base_len = 0;
	decoded_dirent->mask = NULL;
	decoded_dirent->pds_directories = NULL;

	decoded_dirent->filesize = (size_t)( dirent->filesize );
	if( time->prior.raw ) {
		enum block_cache_entries cache;
		struct storageTimelineNode* prior = getRawTimeEntry( vol, time->prior.raw, &cache );
		while( prior->prior.raw ) {
			dropRawTimeEntry( vol, cache );
			prior = getRawTimeEntry( vol, prior->prior.raw, &cache );
		}
		decoded_dirent->ctime = prior->time;
		dropRawTimeEntry( vol, cache );
	}
	else
		decoded_dirent->ctime = time->time;
	decoded_dirent->wtime = time->time;

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
	LOGICAL unused,
	PDATASTACK * pdsStack,
	struct memoryTimelineNode* node,
	struct memoryTimelineNode* left_
)
{
	//node->lesser.ref.index *
	struct memoryTimelineNode* parent;
	parent = (struct memoryTimelineNode*)PeekData( pdsStack ); // the next one up the tree

	{
#if 0
		/* Perform rotation*/
		if( parent ) {
			if( parent->slesser.ref.index == node->index ) {
				parent->slesser.ref.index = node->slesser.ref.index;
				{
					struct storageTimelineNode* x;
					enum block_cache_entries cache;
					x = getRawTimeEntry( vol, node->slesser.ref.index, &cache );
					x->me_fpi = node->me_fpi;
					SETFLAG( vol->dirty, cache );
					dropRawTimeEntry( vol, cache );
				}
			} else {
#ifdef _DEBUG
				if( parent->sgreater.ref.index == node->index ) {
#endif
					parent->sgreater.ref.index = node->slesser.ref.index;
					{
						struct storageTimelineNode* x;
						enum block_cache_entries cache;
						x = getRawTimeEntry( vol, node->slesser.ref.index, &cache );
						x->me_fpi = node->me_fpi;
						SETFLAG( vol->dirty, cache );
						dropRawTimeEntry( vol, cache );
					}
#ifdef _DEBUG
				} else
					DebugBreak();
#endif
			}
		}
		else {
			vol->timeline->header.srootNode.raw = node->slesser.raw ;
			{
				struct storageTimelineNode* x;
				enum block_cache_entries cache;
				x = getRawTimeEntry( vol, node->slesser.ref.index, &cache );
				x->me_fpi = node->me_fpi;
				SETFLAG( vol->dirty, cache );
				dropRawTimeEntry( vol, cache );
			}
		}
#else
		{
			enum block_cache_entries meCache;
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, node->me_fpi, &meCache );
			ptr[0] = node->slesser;
			if( parent )
				if( ( node->me_fpi & 0x3f ) == 0x20 )
					parent->slesser = node->slesser;
				else
					parent->sgreater = node->slesser;
			SETFLAG( vol->dirty, meCache );
		}
#endif

		node->slesser.raw = left_->sgreater.raw;
		{
			struct storageTimelineNode* x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, node->slesser.ref.index, &cache );
			x->me_fpi = node->this_fpi + sane_offsetof( struct storageTimelineNode, slesser );
			SETFLAG( vol->dirty, cache );
			dropRawTimeEntry( vol, cache );
		}
		left_->me_fpi = node->me_fpi;

		left_->sgreater.ref.index = node->index;
		node->me_fpi = left_->this_fpi + sane_offsetof( struct storageTimelineNode, sgreater );


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

			if( parent ) {
				if( leftDepth > rightDepth ) {
					if( parent->sgreater.ref.index == left_->index )
						parent->sgreater.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->slesser.ref.index == left_->index )
#endif
							parent->slesser.ref.depth = leftDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
				else {
					if( parent->sgreater.ref.index == left_->index )
						parent->sgreater.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->slesser.ref.index == left_->index )
#endif
							parent->slesser.ref.depth = rightDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
			}
			else {
				if( leftDepth > rightDepth ) {
					vol->timeline->header.srootNode.ref.depth = leftDepth + 1;
				}
				else {
					vol->timeline->header.srootNode.ref.depth = rightDepth + 1;
				}
			}
		}
	}
	if( parent )
		updateTimeEntry( parent, vol );
	updateTimeEntry( node, vol );
	updateTimeEntry( left_, vol );
}

//---------------------------------------------------------------------------

static void _os_AVL_RotateToLeft(
	struct sack_vfs_os_volume* vol,
	LOGICAL unused,
	PDATASTACK * pdsStack,
	struct memoryTimelineNode* node,
	struct memoryTimelineNode* right_
)
//#define _os_AVL_RotateToLeft(node)
{
	struct memoryTimelineNode* parent;
	parent = (struct memoryTimelineNode*)PeekData( pdsStack );

	{
#if 0
		// old way to check for link to set.
		if( parent ) {
			
			if( parent->slesser.ref.index == node->index ) {
				parent->slesser.raw = node->sgreater.raw;
				{
					struct memoryTimelineNode x;
					reloadTimeEntry( &x, vol, node->sgreater.ref.index );
					x.me_fpi = node->me_fpi; // I know by the offset of 
					updateTimeEntry( &x, vol );
				}
			} else {
#ifdef _DEBUG
				if( parent->sgreater.ref.index == node->index ) {
#endif
					parent->sgreater.raw = node->sgreater.raw;
					{
						struct memoryTimelineNode x;
						reloadTimeEntry( &x, vol, node->sgreater.ref.index );
						x.me_fpi = node->me_fpi;
						updateTimeEntry( &x, vol );
					}
#ifdef _DEBUG
				} else
					DebugBreak();
#endif
			}
		else {
			vol->timeline->header.srootNode.raw = node->sgreater.raw;
			{
				struct memoryTimelineNode x;
				reloadTimeEntry( &x, vol, node->sgreater.ref.index );
				x.me_fpi = node->me_fpi;
				updateTimeEntry( &x, vol );
			}
		}
#else
		{
			enum block_cache_entries meCache;
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, node->me_fpi, &meCache );
			ptr[0] = node->sgreater;
			if( parent )
			if( ( node->me_fpi & 0x3f ) == 0x20 )
				parent->slesser = node->sgreater;
			else
				parent->sgreater = node->sgreater;
			SETFLAG( vol->dirty, meCache );
		}
#endif

		node->sgreater.raw = right_->slesser.raw;
		{
			struct storageTimelineNode *x;
			enum block_cache_entries cache;
			x = getRawTimeEntry( vol, right_->slesser.ref.index, &cache );
			x->me_fpi = node->this_fpi + sane_offsetof( struct storageTimelineNode, sgreater );
			dropRawTimeEntry( vol, cache );
		}
		right_->me_fpi = node->me_fpi;

		right_->slesser.ref.index = node->index;
		node->me_fpi = right_->this_fpi + sane_offsetof( struct storageTimelineNode, slesser );;



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

			//struct memoryTimelineNode *parent;
			//parent = (struct memoryTimelineNode *)PeekData( pdsStack );
			if( parent ) {
				if( leftDepth > rightDepth ) {
					if( parent->slesser.ref.index == right_->index )
						parent->slesser.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->sgreater.ref.index == right_->index )
#endif
							parent->sgreater.ref.depth = leftDepth + 1;
				}
				else {
					if( parent->slesser.ref.index == right_->index )
						parent->slesser.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->sgreater.ref.index == right_->index )
#endif
							parent->sgreater.ref.depth = rightDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
			}
			else
				if( leftDepth > rightDepth ) {
					vol->timeline->header.srootNode.ref.depth = leftDepth + 1;
				}
				else {
					vol->timeline->header.srootNode.ref.depth = rightDepth + 1;
				}
		}
	}

	if( parent )
		updateTimeEntry( parent, vol );
	updateTimeEntry( node, vol );
	updateTimeEntry( right_, vol );
}

//---------------------------------------------------------------------------


static void _os_AVLbalancer( struct sack_vfs_os_volume* vol, LOGICAL unused, PDATASTACK * pdsStack
	, struct memoryTimelineNode* node ) {
	struct memoryTimelineNode* _x = NULL;
	struct memoryTimelineNode* _y = NULL;
	struct memoryTimelineNode* _z = NULL;
	struct memoryTimelineNode* tmp;
	int leftDepth;
	int rightDepth;
	LOGICAL balanced = FALSE;
	_z = node;

	{
		while( _z ) {
			int doBalance;
			rightDepth = (int)_z->sgreater.ref.depth;
			leftDepth = (int)_z->slesser.ref.depth;
			if( tmp = (struct memoryTimelineNode*)PeekData( pdsStack ) ) {
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "WR (P)left/right depths: %d  %d   %d    %d  %d", (int)tmp->index, (int)leftDepth, (int)rightDepth, (int)tmp->sgreater.ref.index, (int)tmp->slesser.ref.index );
				lprintf( "WR left/right depths: %d   %d   %d    %d  %d", (int)_z->index, (int)leftDepth, (int)rightDepth, (int)_z->sgreater.ref.index, (int)_z->slesser.ref.index );
#endif
				if( leftDepth > rightDepth ) {
					if( tmp->sgreater.ref.index == _z->index ) {
						if( (1 + leftDepth) == tmp->sgreater.ref.depth ) {
							//if( zz )
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							break;
						}
						tmp->sgreater.ref.depth = 1 + leftDepth;
					}
					else
#ifdef _DEBUG
						if( tmp->slesser.ref.index == _z->index )
#endif
						{
							if( (1 + leftDepth) == tmp->slesser.ref.depth ) {
								//if( zz )
								//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
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
					if( tmp->sgreater.ref.index == _z->index ) {
						if( (1 + rightDepth) == tmp->sgreater.ref.depth ) {
							//if(zz)
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							break;
						}
						tmp->sgreater.ref.depth = 1 + rightDepth;
					}
					else
#ifdef _DEBUG
						if( tmp->slesser.ref.index == _z->index )
#endif
						{
							if( (1 + rightDepth) == tmp->slesser.ref.depth ) {
								//if(zz)
								//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
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
				lprintf( "WR updated left/right depths: %d      %d  %d", (int)tmp->index, (int)tmp->sgreater.ref.depth, (int)tmp->slesser.ref.depth );
#endif
				updateTimeEntry( tmp, vol );
			}
			if( leftDepth > rightDepth )
				doBalance = ((leftDepth - rightDepth) > 1);
			else
				doBalance = ((rightDepth - leftDepth) > 1);


			if( doBalance ) {
				if( _x ) {
					if( _x->index == _y->slesser.ref.index ) {
						if( _y->index == _z->slesser.ref.index ) {
							// left/left
							_os_AVL_RotateToRight( vol, unused, pdsStack, _z, _y );
						}
						else {
							//left/rightDepth
							_os_AVL_RotateToRight( vol, unused, pdsStack, _y, _x );
							_os_AVL_RotateToLeft( vol, unused, pdsStack, _z, _y );
						}
					}
					else {
						if( _y->index == _z->slesser.ref.index ) {
							_os_AVL_RotateToLeft( vol, unused, pdsStack, _y, _x );
							_os_AVL_RotateToRight( vol, unused, pdsStack, _z, _y );
							// rightDepth.left
						}
						else {
							//rightDepth/rightDepth
							_os_AVL_RotateToLeft( vol, unused, pdsStack, _z, _y );
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
			_x = _y;
			_y = _z;
			_z = (struct memoryTimelineNode*)PopData( pdsStack );
		}
	}
}

//---------------------------------------------------------------------------


static int hangTimelineNode( struct sack_vfs_os_volume* vol
	, TIMELINE_BLOCK_TYPE index
	, LOGICAL unused
	, struct storageTimeline* timeline
	, struct memoryTimelineNode* timelineNode
)
{
	PDATASTACK* pdsStack = &vol->pdsWTimeStack;
	struct memoryTimelineNode curNode;
	struct memoryTimelineNode* curNode_;
	struct memoryTimelineNode* root;
	struct memoryTimelineNode* parent;
	uint64_t curindex;

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

	if( !root )
		{
			if( !timeline->header.srootNode.ref.index ) {
				timeline->header.srootNode.ref.index = index.ref.index ;
				timeline->header.srootNode.ref.depth = 0 ;
				timelineNode->me_fpi = offsetof( struct timelineHeader, srootNode );
				return 1;
			}
			reloadTimeEntry( &curNode, vol
				, curindex = timeline->header.srootNode.ref.index  );
			PushData( pdsStack, &curNode );
		}
	//else
		// the top of the stack is already setup to peek at.

	//check = root->tree;
	while( 1 ) {
		int dir;// = root->Compare( node->key, check->key );
		curNode_ = (struct memoryTimelineNode*)PeekData( pdsStack );
		{
			if( curNode_->time > timelineNode->time )
				dir = -1;
			else if( curNode_->time < timelineNode->time )
				dir = 1;
			else
				dir = 0;

		}

		uint64_t nextIndex;
		//dir = -dir; // test opposite rotation.
		if( dir < 0 ) {
			if( nextIndex = curNode_->slesser.ref.index ) {
				reloadTimeEntry( &curNode, vol
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
				//check = check->lesser;
			}
			else {

				curNode_->slesser.ref.index = index.ref.index;
				curNode_->slesser.ref.depth = 0;

				timelineNode->me_fpi = curNode_->this_fpi + sane_offsetof( struct storageTimelineNode, slesser );
				updateTimeEntry( curNode_, vol );
				break;
			}
		}
		else if( dir > 0 )
			if( nextIndex = curNode_->sgreater.ref.index ) {
				reloadTimeEntry( &curNode, vol
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
			}
			else {
				/*
				enum block_cache_entries cache;
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, curNode_->me_fpi, cache );
				ptr[0].ref.index = index.ref.index;
				SETFLAG( vol->dirty, cache );
				*/
				curNode_->sgreater.ref.index = index.ref.index;
				curNode_->sgreater.ref.depth = 0;
				timelineNode->me_fpi = curNode_->this_fpi + sane_offsetof( struct storageTimelineNode, sgreater );
				updateTimeEntry( timelineNode, vol );
				updateTimeEntry( curNode_, vol );
				break;
			}
		else {
			// allow duplicates; but link in as a near node, either left
			// or right... depending on the depth.
			int leftdepth = 0, rightdepth = 0;
			uint64_t nextLesserIndex, nextGreaterIndex;
			if( nextLesserIndex = curNode_->slesser.ref.index )
				leftdepth = (int)(curNode_->slesser.ref.depth);
			if( nextGreaterIndex = curNode_->sgreater.ref.index )
				rightdepth = (int)(curNode_->sgreater.ref.depth);
			if( leftdepth < rightdepth ) {
				if( nextLesserIndex ) {
					reloadTimeEntry( &curNode, vol
						, curindex = nextLesserIndex );
					PushData( pdsStack, &curNode );
				}
				else {
					{
						curNode_->slesser.ref.index = index.ref.index;
						curNode_->slesser.ref.depth = 0;
					}
					timelineNode->me_fpi = curNode_->this_fpi + sane_offsetof( struct storageTimelineNode, slesser );
					updateTimeEntry( timelineNode, vol );
					updateTimeEntry( curNode_, vol );
					break;
				}
			}
			else {
				if( nextGreaterIndex ) {
					reloadTimeEntry( &curNode, vol
						, curindex = nextGreaterIndex );
					PushData( pdsStack, &curNode );
				}
				else {
					{
						curNode_->sgreater.ref.index = index.ref.index;
						curNode_->sgreater.ref.depth = 0;
					}
					timelineNode->me_fpi = curNode_->this_fpi + sane_offsetof( struct storageTimelineNode, sgreater );
					updateTimeEntry( timelineNode, vol );
					updateTimeEntry( curNode_, vol );
					break;
				}
			}
		}
	}
	//PushData( &pdsStack, timelineNode );
	lprintf( " ------------- BEFORE AVL BALANCER ---------------- " );
	DumpTimelineTree( vol, TRUE );
	_os_AVLbalancer( vol, 0, pdsStack, timelineNode );
	return 1;
}

static void deleteTimelineIndexWork( struct sack_vfs_os_volume* vol, BLOCKINDEX index, struct storageTimelineNode* time, enum block_cache_entries cache ) {
	//PDATASTACK* pdsStack = &vol->pdsWTimeStack;
	enum block_cache_entries cache_left = BC( TIMELINE );
	enum block_cache_entries cache_right = BC( TIMELINE );
	enum block_cache_entries cache_parent = BC( TIMELINE );
	enum block_cache_entries meCache;

	{
		

		enum block_cache_entries bottomCache = BC( ZERO );
		struct storageTimelineNode* bottom;
		struct storageTimelineNode* least = NULL;
		BLOCKINDEX leastIndex;
		struct memoryTimelineNode tmp;

		//time = getRawTimeEntry( vol, index, &cache );

		if( !time->slesser.ref.index ) {
			if( time->sgreater.ref.index ) {
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				struct storageTimelineNode *greater = getRawTimeEntry( vol, time->sgreater.ref.index, &bottomCache );
				ptr[0] = time->sgreater;
				greater->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, meCache );

				bottom = greater;
			} else {
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
				ptr[0].raw = timelineBlockIndexNull;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, meCache );

				// no greater or lesser...
				bottom = time;
			}
		} else if( !time->sgreater.ref.index ) {
			TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, time->me_fpi, &meCache );
			struct storageTimelineNode* lesser = getRawTimeEntry( vol, time->slesser.ref.index, &bottomCache );
			ptr[0].raw = time->slesser.raw;
			lesser->me_fpi = time->me_fpi;
			SETFLAG( vol->dirty, meCache );
			dropRawTimeEntry( vol, meCache );
			bottom = lesser;
		} else {

			if( time->slesser.ref.depth > time->sgreater.ref.depth ) {
				enum block_cache_entries cache;
				bottom = time;
				least = getRawTimeEntry( vol, leastIndex = time->slesser.ref.index, &cache );
				while( least->sgreater.raw ) {
					bottom = least;
					least = getRawTimeEntry( vol, leastIndex = least->sgreater.ref.index, &cache );
				}

				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->slesser.raw ) {
					enum block_cache_entries cache;
					struct storageTimelineNode* leastLesser = getRawTimeEntry( vol, least->slesser.ref.index, &cache );
					leastLesser->me_fpi = least->me_fpi;
					SETFLAG( vol->dirty, cache );
					ptr[0].raw = least->slesser.raw;
					least->slesser.raw = 0; // now no longer points to a thing.
					bottom = leastLesser;
				}
				else {
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, meCache );
			}else {
				enum block_cache_entries cache;
				enum block_cache_entries cache_ = BC( ZERO );
				bottom = time;
				least = getRawTimeEntry( vol, leastIndex = time->slesser.ref.index, &cache );
				while( least->sgreater.raw ) {
					if( cache_ ) 
						dropRawTimeEntry( vol, cache_ );
					cache_ = cache;
					bottom = least;
					least = getRawTimeEntry( vol, leastIndex = least->slesser.ref.index, &cache );
				}

				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, least->me_fpi, &meCache );
				if( least->sgreater.raw ) {
					ptr[0].raw = least->sgreater.raw;
					if( cache_ )
						dropRawTimeEntry( vol, cache_ );
					struct storageTimelineNode* leastGreater = getRawTimeEntry( vol, least->sgreater.ref.index, &cache_ );
					leastGreater->me_fpi = least->me_fpi;
					SETFLAG( vol->dirty, cache );
					bottom = leastGreater;
				} else {
					ptr[0].raw = timelineBlockIndexNull;
				}
				least->me_fpi = time->me_fpi;
				SETFLAG( vol->dirty, meCache );
				dropRawTimeEntry( vol, meCache );
			}
		}
		dropRawTimeEntry( vol, cache );

		{
			uint64_t node_fpi;
			int updating = 1;
			PDATASTACK pdlStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );
			PDATASTACK pdlStack2 = CreateDataStack( sizeof( struct memoryTimelineNode ) );

			PushData( &pdlStack, bottom );

			// bottom->parent
			node_fpi = bottom->me_fpi & ~0x3f;

			while( node_fpi > 0x3f ) {
				uint64_t node_idx = ( node_fpi - sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) + 1;
				reloadTimeEntry( &tmp, vol, node_idx );

				if( tmp.me_fpi < sizeof( struct timelineHeader ) ) break;
				if( updating ) {
					if( tmp.slesser.raw ) {
						if( tmp.sgreater.raw ) {
							int tmp1, tmp2;
							if( ( tmp1 = tmp.slesser.ref.depth ) > ( tmp2 = tmp.sgreater.ref.depth ) ) {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp.me_fpi, &meCache );
								if( ptr[0].ref.depth != ( tmp1 + 1 ) ) {
									ptr[0].ref.depth = tmp1 + 1;
									SETFLAG( vol->dirty, meCache );
								}
								else
									updating = 0;

								dropRawTimeEntry( vol, meCache );
								//backtrack->depth = tmp1 + 1;
							} else {
								TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp.me_fpi, &meCache );
								if( ptr[0].ref.depth != (tmp2 + 1) ) {
									ptr[0].ref.depth = tmp2 + 1;
									SETFLAG( vol->dirty, meCache );
								}
								else
									updating = 0;
								dropRawTimeEntry( vol, meCache );
								//backtrack->depth = tmp2 + 1;
							}
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp.me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp.slesser.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp.slesser.ref.depth + 1;
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
							dropRawTimeEntry( vol, meCache );
							//backtrack->depth = backtrack->lesser->depth + 1;
						}
					} else {
						if( tmp.sgreater.raw ) {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp.me_fpi, &meCache );
							if( ptr[0].ref.depth != ( tmp.sgreater.ref.depth + 1 ) ) {
								ptr[0].ref.depth = tmp.sgreater.ref.depth + 1;
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
							dropRawTimeEntry( vol, meCache );
							//backtrack->depth = backtrack->greater->depth + 1;
						} else {
							TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, tmp.me_fpi, &meCache );
							if( ptr[0].ref.depth ) {
								ptr[0].ref.depth = 0;
								SETFLAG( vol->dirty, meCache );
							}
							else
								updating = 0;
							dropRawTimeEntry( vol, meCache );
						}
					}
				}
				PushData( &pdlStack, &tmp );
				node_fpi = tmp.me_fpi & ~0x3f;
			}
			if( least ) {
				TIMELINE_BLOCK_TYPE* ptr = getRawTimePointer( vol, bottom->me_fpi, &meCache );
				ptr[0].ref.index = leastIndex;
				least->me_fpi = time->me_fpi;
				if( least->slesser.raw = time->slesser.raw )
				{
					enum block_cache_entries cache;
					struct storageTimelineNode* tmp;
					tmp = getRawTimeEntry( vol, time->slesser.ref.index, &cache );
					tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].slesser );
					dropRawTimeEntry( vol, cache );
				}
				if( least->sgreater.raw = time->sgreater.raw )
				{
					enum block_cache_entries cache;
					struct storageTimelineNode* tmp;
					tmp = getRawTimeEntry( vol, time->sgreater.ref.index, &cache );
					tmp->me_fpi = sane_offsetof( struct storageTimeline, entries[leastIndex - 1].sgreater );
					dropRawTimeEntry( vol, cache );
				}
				least = NULL;
			}
			{
				struct storageTimeline* timeline = vol->timeline;
				TIMELINE_BLOCK_TYPE freeIndex;
				time->sgreater.ref.index = timeline->header.first_free_entry.ref.index;
				timeline->header.first_free_entry.ref.index = index;
				SETFLAG( vol->dirty, vol->timelineCache );
				SETFLAG( vol->dirty, cache );

			}
			{
				struct memoryTimelineNode* p, *_p = NULL;
				// reverse the stack before the balancer
				for( p = ( struct memoryTimelineNode*)PopData( &pdlStack );
					p;
					(_p?PushData( &pdlStack2, _p ):0)
					, (_p = p)
					, (p = (struct memoryTimelineNode*)PopData( &pdlStack ))
					);
				if( _p )
					_os_AVLbalancer( vol, 0, &pdlStack2, p );
			}
			DeleteDataStack( &pdlStack );
			DeleteDataStack( &pdlStack2 );

		}


	}

	lprintf( " ------------- DELETE TIME ENTRY ---------------- " );
	DumpTimelineTree( vol, TRUE );
	//node->
}

static void deleteTimelineIndex( struct sack_vfs_os_volume* vol, BLOCKINDEX index ) {
	BLOCKINDEX next;
	do {
		struct storageTimelineNode* time;
		enum block_cache_entries cache = BC( TIMELINE );

		time = getRawTimeEntry( vol, index, &cache );
		next = time->prior.raw;
		deleteTimelineIndexWork( vol, index, time, cache );
	} while( index = next );
}

void getTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL keepDirent, void(*init)(uintptr_t, struct memoryTimelineNode*), uintptr_t psv ) {
	enum block_cache_entries cache = BC( TIMELINE );
	enum block_cache_entries cache_last = BC( TIMELINE );
	enum block_cache_entries cache_free = BC( TIMELINE );
	enum block_cache_entries cache_new = BC( TIMELINE );
	FPI orig_dirent;
	struct storageTimeline* timeline = vol->timeline;
	TIMELINE_BLOCK_TYPE freeIndex;

	freeIndex.ref.index = timeline->header.first_free_entry.ref.index;
	freeIndex.ref.depth = 0;

	// update next free.
	if( keepDirent ) orig_dirent = time->dirent_fpi;
	reloadTimeEntry( time, vol, freeIndex.ref.index );
	if( keepDirent ) time->dirent_fpi = orig_dirent;

	if( time->sgreater.ref.index ) {
		timeline->header.first_free_entry.ref.index = time->sgreater.ref.index;
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

	time->slesser.raw = timelineBlockIndexNull;
	time->sgreater.raw = timelineBlockIndexNull;

	//time->stime.raw = time->ctime.raw =
	time->time = timeGetTime64ns();
	{
		int tz = GetTimeZone();
		if( tz < 0 )
			tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
		else
			tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
		time->bits.timeTz = tz;
	}

	if( init ) init( psv, time );

	hangTimelineNode( vol
		, freeIndex
		, 0
		, timeline
		, time );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol );
	lprintf( " ------------- NEW TIME ENTRY ---------------- " );
	DumpTimelineTree( vol, TRUE );
	//DumpTimelineTree( vol, FALSE );
}

void updateTimeEntryTime( struct memoryTimelineNode* time
			, struct sack_vfs_os_volume *vol, uint64_t index
			, LOGICAL allocateNew
			, void( *init )( uintptr_t, struct memoryTimelineNode* ), uintptr_t psv ) {
	if( allocateNew ) {
		uint64_t inputIndex = time->index;
		// gets a new timestamp.
		getTimeEntry( time, vol, TRUE, init, psv );
		time->prior.raw = inputIndex;
		updateTimeEntry( time, vol );
	}
	else {
		reloadTimeEntry( time, vol, index );
		{
			int tz = GetTimeZone();
			if( tz < 0 )
				tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
			else
				tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
			time->bits.timeTz = tz;
		}
		time->time = timeGetTime64ns();
		updateTimeEntry( time, vol );
	}
}
