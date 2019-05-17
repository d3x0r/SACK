

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
	TIMELINE_BLOCK_TYPE crootNode;
	TIMELINE_BLOCK_TYPE srootNode;
	uint64_t unused[5];
} PACKED;

PREFIX_PACKED struct storageTimelineNode {
	// if dirent_fpi == 0; it's free.
	uint64_t dirent_fpi;
	union {
		//FPI dirent_fpi;   // FPI on disk
		struct padding {
			uint64_t b;
		}unused;
	} data;

	// if the block is free, cgreater is used as pointer to next free block
	// delete an object can leave free timeline nodes in the middle of the physical chain.

	union {
		uint64_t raw;
		TIMELINE_TIME_TYPE parts;        // file time tick/ created stamp, sealing stamp
	}ctime;
	TIMELINE_BLOCK_TYPE clesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE cgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
	union {
		uint64_t raw;
		TIMELINE_TIME_TYPE parts;        // time file was stored
	}stime;
	TIMELINE_BLOCK_TYPE slesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE sgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
} PACKED;

struct memoryTimelineNode {
	// if dirent_fpi == 0; it's free.
	FPI dirent_fpi;   // FPI on disk
	FPI this_fpi;
	uint64_t index;
	// if the block is free, cgreater is used as pointer to next free block
	// delete an object can leave free timeline nodes in the middle of the physical chain.

	union {
		uint64_t raw;
		TIMELINE_TIME_TYPE parts;        // file time tick/ created stamp, sealing stamp
	}ctime;
	TIMELINE_BLOCK_TYPE clesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE cgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
	union {
		uint64_t raw;
		TIMELINE_TIME_TYPE parts;        // time file was stored
	}stime;
	TIMELINE_BLOCK_TYPE slesser;         // FPI/32 within timeline chain
	TIMELINE_BLOCK_TYPE sgreater;        // FPI/32 within timeline chain + (child depth in this direction AVL)
};


struct storageTimelineCursor {
	PDATASTACK parentNodes;  // save stack of parents in cursor
	struct storageTimelineCache dirents; // temp; needs work.
};

#define NUM_ROOT_TIMELINE_NODES (BLOCK_SIZE - sizeof( struct timelineHeader )) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimeline {
	struct timelineHeader header;
	struct storageTimelineNode entries[NUM_ROOT_TIMELINE_NODES];
} PACKED;

#define NUM_TIMELINE_NODES (BLOCK_SIZE) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimelineBlock {
	struct storageTimelineNode entries[(BLOCK_SIZE) / sizeof( struct storageTimelineNode )];
} PACKED;



void reloadTimeEntry( struct memoryTimelineNode* time, struct volume* vol, uint64_t timeEntry ) {
	enum block_cache_entries cache = BC( TIMELINE );
	//uintptr_t vfs_os_FSEEK( struct volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index ) {
	//if( timeEntry > 62 )DebugBreak();
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = (struct storageTimelineNode*)vfs_os_FSEEK( vol, vol->timeline_file, FIRST_TIMELINE_BLOCK, pos, &cache );
	struct storageTimelineNode* nodeKey = (struct storageTimelineNode*)(vol->usekey[cache] + (pos & BLOCK_MASK));
	time->index = timeEntry;

	time->dirent_fpi = (FPI)(node->dirent_fpi ^ nodeKey->dirent_fpi);

	time->ctime.raw = node->ctime.raw ^ nodeKey->ctime.raw;
	time->clesser.raw = node->clesser.raw ^ nodeKey->clesser.raw;
	time->cgreater.raw = node->cgreater.raw ^ nodeKey->cgreater.raw;

	time->stime.raw = node->stime.raw ^ nodeKey->stime.raw;
	time->slesser.raw = node->slesser.raw ^ nodeKey->slesser.raw;
	time->sgreater.raw = node->sgreater.raw ^ nodeKey->sgreater.raw;

	time->this_fpi = vol->bufferFPI[cache] + (pos & BLOCK_MASK);
	//LoG( "Set this FPI: %d  %d", (int)timeEntry, (int)time->this_fpi );
}



static void DumpTimelineTreeWork( struct volume* vol, int level, struct memoryTimelineNode* parent, LOGICAL bSortCreation ) {
	struct memoryTimelineNode curNode;
	static char indent[256];
	int i;
#if 0
	lprintf( "input node %d %d %d", parent->index
		, bSortCreation ? parent->clesser.ref.index : parent->slesser.ref.index
		, bSortCreation ? parent->cgreater.ref.index : parent->sgreater.ref.index
	);
#endif
	if( bSortCreation ? parent->clesser.ref.index : parent->slesser.ref.index ) {
		reloadTimeEntry( &curNode, vol, bSortCreation ? parent->clesser.ref.index : parent->slesser.ref.index );
#if 0
		lprintf( "(lesser) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, bSortCreation );
	}
	for( i = 0; i < level * 3; i++ )
		indent[i] = ' ';
	indent[i] = 0;
	lprintf( "CurNode: (%s ->  %d  has children %d %d  with depths of %d %d"
		, indent
		, (int)parent->index
		, (int)(bSortCreation ? parent->clesser.ref.index : parent->slesser.ref.index)
		, (int)(bSortCreation ? parent->cgreater.ref.index : parent->sgreater.ref.index)
		, (int)(bSortCreation ? parent->clesser.ref.depth : parent->slesser.ref.depth)
		, (int)(bSortCreation ? parent->cgreater.ref.depth : parent->sgreater.ref.depth)
	);
	if( bSortCreation ? parent->cgreater.ref.index : parent->sgreater.ref.index ) {
		reloadTimeEntry( &curNode, vol, bSortCreation ? parent->cgreater.ref.index : parent->sgreater.ref.index );
#if 0
		lprintf( "(greater) go to node %d", curNode.index );
#endif
		DumpTimelineTreeWork( vol, level + 1, &curNode, bSortCreation );
	}
}

//---------------------------------------------------------------------------

static void DumpTimelineTree( struct volume* vol, LOGICAL bSortCreation ) {
	enum block_cache_entries cache = BC( TIMELINE );

	struct storageTimeline* timeline = vol->timeline;// (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
	SETFLAG( vol->seglock, cache );
	struct storageTimeline* timelineKey = vol->timelineKey; // (struct storageTimeline *)(vol->usekey[cache]);

	struct memoryTimelineNode curNode;

	if( bSortCreation ) {
		if( !timeline->header.crootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, (timeline->header.crootNode.ref.index ^ timelineKey->header.crootNode.ref.index) );
	}
	else {
		if( !timeline->header.srootNode.ref.index ) {
			return;
		}
		reloadTimeEntry( &curNode, vol
			, timeline->header.srootNode.ref.index ^ timelineKey->header.srootNode.ref.index );
	}
	DumpTimelineTreeWork( vol, 0, &curNode, bSortCreation );
}







//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct volume* vol ) {
	FPI timeEntry = time->this_fpi;

	enum block_cache_entries cache = BC( TIMELINE );
	struct storageTimelineNode* nodeKey;
	struct storageTimelineNode* node = (struct storageTimelineNode*)vfs_os_DSEEK( vol, time->this_fpi, &cache, (POINTER*)& nodeKey );

	node->dirent_fpi = time->dirent_fpi ^ nodeKey->dirent_fpi;

	node->ctime.raw = time->ctime.raw ^ nodeKey->ctime.raw;
	node->clesser.raw = time->clesser.raw ^ nodeKey->clesser.raw;
	node->cgreater.raw = time->cgreater.raw ^ nodeKey->cgreater.raw;

	node->stime.raw = time->stime.raw ^ nodeKey->stime.raw;
	node->slesser.raw = time->slesser.raw ^ nodeKey->slesser.raw;
	node->sgreater.raw = time->sgreater.raw ^ nodeKey->sgreater.raw;

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

void reloadDirectoryEntry( struct volume* vol, struct memoryTimelineNode* time, struct _os_find_info* decoded_dirent ) {
	enum block_cache_entries cache = BC( DIRECTORY );
	struct directory_entry* dirent, * entkey;
	struct directory_hash_lookup_block* dirblock;
	struct directory_hash_lookup_block* dirblockkey;
	PDATASTACK pdsChars = CreateDataStack( 1 );
	BLOCKINDEX this_dir_block = (time->dirent_fpi >> BLOCK_BYTE_SHIFT);
	BLOCKINDEX next_block;
	dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, this_dir_block, cache );
	dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
	dirent = (struct directory_entry*)(((uintptr_t)dirblock) + (time->dirent_fpi & BLOCK_SIZE));
	entkey = (struct directory_entry*)(((uintptr_t)dirblockkey) + (time->dirent_fpi & BLOCK_SIZE));

	decoded_dirent->vol = vol;

	// all of this regards the current state of a find cursor...
	decoded_dirent->base = NULL;
	decoded_dirent->base_len = 0;
	decoded_dirent->mask = NULL;
	decoded_dirent->pds_directories = NULL;

	decoded_dirent->filesize = dirent->filesize ^ entkey->filesize;
	decoded_dirent->ctime = time->ctime.raw;
	decoded_dirent->wtime = time->stime.raw;

	while( (next_block = dirblock->next_block[DIRNAME_CHAR_PARENT] ^ dirblockkey->next_block[DIRNAME_CHAR_PARENT]) ) {
		enum block_cache_entries back_cache = BC( DIRECTORY );
		struct directory_hash_lookup_block* back_dirblock;
		struct directory_hash_lookup_block* back_dirblockkey;
		back_dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, next_block, back_cache );
		back_dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[back_cache];
		int i;
		for( i = 0; i < DIRNAME_CHAR_PARENT; i++ ) {
			if( (back_dirblock->next_block[i] ^ back_dirblockkey->next_block[i]) == this_dir_block ) {
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
		dirblockkey = back_dirblockkey;
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
		FPI name_offset = (dirent[n].name_offset ^ entkey->name_offset) & DIRENT_NAME_OFFSET_OFFSET;

		enum block_cache_entries cache = BC( NAMES );
		const char* dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache );
		const char* dirname_ = dirname;
		const char* dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK);
		const char* prior_dirname = dirname;
		int c;
		do {
			while( (((unsigned char)(c = (dirname[0] ^ dirkey[0])) != UTF8_EOT))
				&& ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) == (((uintptr_t)dirname) & ~BLOCK_MASK))
				) {
				decoded_dirent->filename[n++] = c;
				dirname++;
				dirkey++;
			}
			if( ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) != (((uintptr_t)dirname) & ~BLOCK_MASK)) ) {
				int partial = (int)(dirname - dirname_);
				cache = BC( NAMES );
				dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset + partial, &cache );
				dirkey = (const char*)(vol->usekey[cache]) + ((name_offset + partial) & BLOCK_MASK);
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
	struct volume* vol,
	LOGICAL bSortCreation,
	PDATASTACK * pdsStack,
	struct memoryTimelineNode* node,
	struct memoryTimelineNode* left_
)
{
	//node->lesser.ref.index *
	struct memoryTimelineNode* parent;
	parent = (struct memoryTimelineNode*)PeekData( pdsStack ); // the next one up the tree

	if( bSortCreation ) {
		/* Perform rotation*/
		if( parent ) {
			if( parent->clesser.ref.index == node->index )
				parent->clesser.raw = node->clesser.raw;
			else
#ifdef _DEBUG
				if( parent->cgreater.ref.index == node->index )
#endif
					parent->cgreater.raw = node->clesser.raw;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		}
		else {
			vol->timeline->header.crootNode.raw = node->clesser.raw ^ vol->timelineKey->header.crootNode.raw;
		}

		// read into stack entry
		//reloadTimeEntry( left_, vol, node->clesser.ref.index );

		node->clesser.raw = left_->cgreater.raw;
		left_->cgreater.ref.index = node->index;
		/* Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->clesser.ref.depth;
			rightDepth = (int)node->cgreater.ref.depth;
			if( leftDepth > rightDepth )
				left_->cgreater.ref.depth = leftDepth + 1;
			else
				left_->cgreater.ref.depth = rightDepth + 1;

			leftDepth = (int)left_->clesser.ref.depth;
			rightDepth = (int)left_->cgreater.ref.depth;

			if( parent ) {
				if( leftDepth > rightDepth )
					if( parent->cgreater.ref.index == left_->index )
						parent->cgreater.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->clesser.ref.index == left_->index )
#endif
							parent->clesser.ref.depth = leftDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				else
					if( parent->cgreater.ref.index == left_->index )
						parent->cgreater.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->clesser.ref.index == left_->index )
#endif
							parent->clesser.ref.depth = rightDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
			}
			else {
				if( leftDepth > rightDepth ) {
					vol->timeline->header.crootNode.ref.depth = leftDepth + 1;
				}
				else {
					vol->timeline->header.crootNode.ref.depth = rightDepth + 1;
				}
			}
		}
	}
	else {
		/* Perform rotation*/
		if( parent ) {
			if( parent->slesser.ref.index == node->index )
				parent->slesser.ref.index = node->slesser.ref.index;
			else
#ifdef _DEBUG
				if( parent->sgreater.ref.index == node->index )
#endif
					parent->sgreater.ref.index = node->slesser.ref.index;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		}
		else {
			vol->timeline->header.srootNode.raw = node->slesser.raw ^ vol->timelineKey->header.srootNode.raw;
		}

		node->slesser.raw = left_->sgreater.raw;
		left_->sgreater.ref.index = node->index;

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
	else
		SETFLAG( vol->dirty, FIRST_TIMELINE_BLOCK );
	updateTimeEntry( node, vol );
	updateTimeEntry( left_, vol );
}

//---------------------------------------------------------------------------

static void _os_AVL_RotateToLeft(
	struct volume* vol,
	LOGICAL bSortCreation,
	PDATASTACK * pdsStack,
	struct memoryTimelineNode* node,
	struct memoryTimelineNode* right_
)
//#define _os_AVL_RotateToLeft(node)
{
	struct memoryTimelineNode* parent;
	parent = (struct memoryTimelineNode*)PeekData( pdsStack );

	if( bSortCreation ) {
		if( parent ) {
			if( parent->clesser.ref.index == node->index )
				parent->clesser.raw = node->cgreater.raw;
			else
#ifdef _DEBUG
				if( parent->cgreater.ref.index == node->index )
#endif
					parent->cgreater.raw = node->cgreater.raw;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		}
		else
			vol->timeline->header.crootNode.raw = node->cgreater.raw ^ vol->timelineKey->header.crootNode.raw;

		node->cgreater.raw = right_->clesser.raw;
		right_->clesser.ref.index = node->index;

		/*  Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->clesser.ref.depth;
			rightDepth = (int)node->cgreater.ref.depth;
			if( leftDepth > rightDepth )
				right_->clesser.ref.depth = leftDepth + 1;
			else
				right_->clesser.ref.depth = rightDepth + 1;

			leftDepth = (int)right_->clesser.ref.depth;
			rightDepth = (int)right_->cgreater.ref.depth;

			//struct memoryTimelineNode *parent;
			//parent = (struct memoryTimelineNode *)PeekData( pdsStack );
			if( parent ) {
				if( leftDepth > rightDepth ) {
					if( parent->clesser.ref.index == right_->index )
						parent->clesser.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->cgreater.ref.index == right_->index )
#endif
							parent->cgreater.ref.depth = leftDepth + 1;
				}
				else {
					if( parent->clesser.ref.index == right_->index )
						parent->clesser.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->cgreater.ref.index == right_->index )
#endif
							parent->cgreater.ref.depth = rightDepth + 1;
				}
			}
			else
				if( leftDepth > rightDepth ) {
					vol->timeline->header.crootNode.ref.depth = leftDepth + 1;
				}
				else {
					vol->timeline->header.crootNode.ref.depth = rightDepth + 1;
				}
		}
	}
	else {
		if( parent )
			if( parent->slesser.ref.index == node->index )
				parent->slesser.raw = node->sgreater.raw;
			else
#ifdef _DEBUG
				if( parent->sgreater.ref.index == node->index )
#endif
					parent->sgreater.raw = node->sgreater.raw;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		else
			vol->timeline->header.srootNode.raw = node->sgreater.raw ^ vol->timelineKey->header.srootNode.raw;

		node->sgreater.raw = right_->slesser.raw;
		right_->slesser.ref.index = node->index;

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
	else
		SETFLAG( vol->dirty, FIRST_TIMELINE_BLOCK );
	updateTimeEntry( node, vol );
	updateTimeEntry( right_, vol );
}

//---------------------------------------------------------------------------


static void _os_AVLbalancer( struct volume* vol, LOGICAL bSortCreation, PDATASTACK * pdsStack
	, struct memoryTimelineNode* node ) {
	struct memoryTimelineNode* _x = NULL;
	struct memoryTimelineNode* _y = NULL;
	struct memoryTimelineNode* _z = NULL;
	struct memoryTimelineNode* tmp;
	int leftDepth;
	int rightDepth;
	LOGICAL balanced = FALSE;
	_z = node;

	if( bSortCreation ) {
		while( _z ) {
			int doBalance;
			doBalance = FALSE;
			rightDepth = (int)_z->cgreater.ref.depth;
			leftDepth = (int)_z->clesser.ref.depth;
			if( tmp = (struct memoryTimelineNode*)PeekData( pdsStack ) ) {
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "CR (P)left/right depths: %d  %d   %d    %d  %d", (int)tmp->index, (int)leftDepth, (int)rightDepth, (int)tmp->cgreater.ref.index, (int)tmp->clesser.ref.index );
				lprintf( "CR left/right depths: %d  %d   %d    %d  %d", (int)_z->index, (int)leftDepth, (int)rightDepth, (int)_z->cgreater.ref.index, (int)_z->clesser.ref.index );
#endif
				if( leftDepth > rightDepth ) {
					if( tmp->cgreater.ref.index == _z->index ) {
						if( (1 + leftDepth) == tmp->cgreater.ref.depth ) {
							break;
						}
						tmp->cgreater.ref.depth = 1 + leftDepth;
					}
					else {
#ifdef _DEBUG
						if( tmp->clesser.ref.index == _z->index )

#endif
						{
							if( (1 + leftDepth) == tmp->clesser.ref.depth ) {
								break;
							}
							tmp->clesser.ref.depth = 1 + leftDepth;
						}
#ifdef _DEBUG
						else
							DebugBreak();// Should be one or the other... 
#endif
					}
				}
				else {
					if( tmp->cgreater.ref.index == _z->index ) {
						if( (1 + rightDepth) == tmp->cgreater.ref.depth ) {
							break;
						}
						tmp->cgreater.ref.depth = 1 + rightDepth;
					}
					else {
#ifdef _DEBUG
						if( tmp->clesser.ref.index == _z->index )
#endif
						{
							if( (1 + rightDepth) == tmp->clesser.ref.depth ) {
								break;
							}
							tmp->clesser.ref.depth = 1 + rightDepth;
						}
#ifdef _DEBUG

						else
							DebugBreak();
#endif
					}
				}
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "CR updated left/right depths: %d      %d  %d", (int)tmp->index, (int)tmp->cgreater.ref.depth, (int)tmp->clesser.ref.depth );
#endif
				updateTimeEntry( tmp, vol );
			}
			if( leftDepth > rightDepth )
				doBalance = ((leftDepth - rightDepth) > 1);
			else
				doBalance = ((rightDepth - leftDepth) > 1);

			if( doBalance && _x ) {
				if( _x->index == _y->clesser.ref.index ) {
					if( _y->index == _z->clesser.ref.index ) {
						// left/left
						_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _z, _y );
					}
					else {
						//left/rightDepth
						_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _y, _x );
						_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _z, _y );
					}
				}
				else {
					if( _y->index == _z->clesser.ref.index ) {
						_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _y, _x );
						_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _z, _y );
						// rightDepth.left
					}
					else {
						//rightDepth/rightDepth
						_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _z, _y );
					}
				}
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "CR Balanced, should redo this one... %d %d", (int)_z->clesser.ref.index, _z->cgreater.ref.index );
#endif
			}
			_x = _y;
			_y = _z;
			_z = (struct memoryTimelineNode*)PopData( pdsStack );
		}
	}
	else {
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
							_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _z, _y );
						}
						else {
							//left/rightDepth
							_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _y, _x );
							_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _z, _y );
						}
					}
					else {
						if( _y->index == _z->slesser.ref.index ) {
							_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _y, _x );
							_os_AVL_RotateToRight( vol, bSortCreation, pdsStack, _z, _y );
							// rightDepth.left
						}
						else {
							//rightDepth/rightDepth
							_os_AVL_RotateToLeft( vol, bSortCreation, pdsStack, _z, _y );
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


static int hangTimelineNode( struct volume* vol
	, TIMELINE_BLOCK_TYPE index
	, LOGICAL bSortCreation
	, struct storageTimeline* timeline
	, struct storageTimeline* timelineKey
	, struct memoryTimelineNode* timelineNode
)
{
	PDATASTACK* pdsStack = bSortCreation ? &vol->pdsCTimeStack : &vol->pdsWTimeStack;
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

		if( bSortCreation ) {
			if( parent->cgreater.ref.index == root->index ) {
				if( timelineNode->ctime.raw < parent->ctime.raw ) {
					// this belongs to the lesser side of parent...
					PopData( pdsStack );
					continue;
				}
			}
			if( parent->clesser.ref.index == root->index ) {
				if( timelineNode->ctime.raw > parent->ctime.raw ) {
					// this belongs to the greater isde of parent...
					PopData( pdsStack );
					continue;
				}
			}
		}
		else {
			if( parent->sgreater.ref.index == root->index ) {
				if( timelineNode->stime.raw < parent->stime.raw ) {
					// this belongs to the lesser side of parent...
					PopData( pdsStack );
					continue;
				}
			}
			if( parent->slesser.ref.index == root->index ) {
				if( timelineNode->stime.raw > parent->stime.raw ) {
					// this belongs to the greater isde of parent...
					PopData( pdsStack );
					continue;
				}
			}
		}
		break;
	}

	if( !root )
		if( bSortCreation ) {
			if( !timeline->header.crootNode.ref.index ) {
				timeline->header.crootNode.ref.index = index.ref.index ^ timeline->header.crootNode.ref.index;
				timeline->header.crootNode.ref.depth = 0 ^ timeline->header.crootNode.ref.depth;
				return 1;
			}

			reloadTimeEntry( &curNode, vol
				, curindex = (timeline->header.crootNode.ref.index ^ timelineKey->header.crootNode.ref.index) );
			PushData( pdsStack, &curNode );

		}
		else {
			if( !timeline->header.srootNode.ref.index ) {
				timeline->header.srootNode.ref.index = index.ref.index ^ timeline->header.srootNode.ref.index;
				timeline->header.srootNode.ref.depth = 0 ^ timeline->header.srootNode.ref.depth;
				return 1;
			}
			reloadTimeEntry( &curNode, vol
				, curindex = timeline->header.srootNode.ref.index ^ timelineKey->header.srootNode.ref.index );
			PushData( pdsStack, &curNode );
		}
	//else
		// the top of the stack is already setup to peek at.

	//check = root->tree;
	while( 1 ) {
		int dir;// = root->Compare( node->key, check->key );
		curNode_ = (struct memoryTimelineNode*)PeekData( pdsStack );
		if( bSortCreation ) {
#ifdef DEBUG_TIMELINE_AVL
			lprintf( "Compare node %lld  %lld %lld", curNode_->index, curNode_->ctime.raw, timelineNode->ctime.raw );;
#endif
			if( curNode_->ctime.raw > timelineNode->ctime.raw )
				dir = -1;
			else if( curNode_->ctime.raw < timelineNode->ctime.raw )
				dir = 1;
			else
				dir = 0;
		}
		else {
			if( curNode_->stime.raw > timelineNode->stime.raw )
				dir = -1;
			else if( curNode_->stime.raw < timelineNode->stime.raw )
				dir = 1;
			else
				dir = 0;

		}

		uint64_t nextIndex;
		//dir = -dir; // test opposite rotation.
		if( dir < 0 ) {
			if( nextIndex = bSortCreation ? curNode_->clesser.ref.index : curNode_->slesser.ref.index ) {
				reloadTimeEntry( &curNode, vol
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
				//check = check->lesser;
			}
			else {
				if( bSortCreation ) {
					curNode_->clesser.ref.index = index.ref.index;
					curNode_->clesser.ref.depth = 0;
				}
				else {
					curNode_->slesser.ref.index = index.ref.index;
					curNode_->slesser.ref.depth = 0;
				}
				updateTimeEntry( curNode_, vol );
				break;
			}
		}
		else if( dir > 0 )
			if( nextIndex = bSortCreation ? curNode_->cgreater.ref.index : curNode_->sgreater.ref.index ) {
				reloadTimeEntry( &curNode, vol
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
			}
			else {
				if( bSortCreation ) {
					curNode_->cgreater.ref.index = index.ref.index;
					curNode_->cgreater.ref.depth = 0;
				}
				else {
					curNode_->sgreater.ref.index = index.ref.index;
					curNode_->sgreater.ref.depth = 0;
				}
				updateTimeEntry( curNode_, vol );
				break;
			}
		else {
			// allow duplicates; but link in as a near node, either left
			// or right... depending on the depth.
			int leftdepth = 0, rightdepth = 0;
			uint64_t nextLesserIndex, nextGreaterIndex;
			if( nextLesserIndex = bSortCreation ? curNode_->clesser.ref.index : curNode_->slesser.ref.index )
				leftdepth = (int)(bSortCreation ? curNode_->clesser.ref.depth : curNode_->slesser.ref.depth);
			if( nextGreaterIndex = bSortCreation ? curNode_->cgreater.ref.index : curNode_->sgreater.ref.index )
				rightdepth = (int)(bSortCreation ? curNode_->cgreater.ref.depth : curNode_->sgreater.ref.depth);
			if( leftdepth < rightdepth ) {
				if( nextLesserIndex ) {
					reloadTimeEntry( &curNode, vol
						, curindex = nextLesserIndex );
					PushData( pdsStack, &curNode );
				}
				else {
					if( bSortCreation ) {
						curNode_->clesser.ref.index = index.ref.index;
						curNode_->clesser.ref.depth = 0;
					}
					else {
						curNode_->slesser.ref.index = index.ref.index;
						curNode_->slesser.ref.depth = 0;
					}
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
					if( bSortCreation ) {
						curNode_->cgreater.ref.index = index.ref.index;
						curNode_->cgreater.ref.depth = 0;
					}
					else {
						curNode_->sgreater.ref.index = index.ref.index;
						curNode_->sgreater.ref.depth = 0;
					}
					updateTimeEntry( curNode_, vol );
					break;
				}
			}
		}
	}
	//PushData( &pdsStack, timelineNode );
	_os_AVLbalancer( vol, bSortCreation, pdsStack, timelineNode );
	return 1;
}


void getTimeEntry( struct memoryTimelineNode* time, struct volume* vol, LOGICAL keepDirent, void(*init)(uintptr_t, struct memoryTimelineNode*), uintptr_t psv ) {
	enum block_cache_entries cache = BC( TIMELINE );
	enum block_cache_entries cache_last = BC( TIMELINE );
	enum block_cache_entries cache_free = BC( TIMELINE );
	enum block_cache_entries cache_new = BC( TIMELINE );
	FPI orig_dirent;
	struct storageTimeline* timeline = vol->timeline;
	struct storageTimeline* timelineKey = vol->timelineKey;
	TIMELINE_BLOCK_TYPE freeIndex;

	freeIndex.ref.index = (timeline->header.first_free_entry.ref.index
		^ timelineKey->header.first_free_entry.ref.index);
	freeIndex.ref.depth = 0;

	// update next free.
	if( keepDirent ) orig_dirent = time->dirent_fpi;
	reloadTimeEntry( time, vol, freeIndex.ref.index );
	if( keepDirent ) time->dirent_fpi = orig_dirent;

	if( time->cgreater.ref.index ) {
		timeline->header.first_free_entry.ref.index = time->cgreater.ref.index ^ timelineKey->header.first_free_entry.ref.index;
		SETFLAG( vol->dirty, cache );
	}
	else {
		timeline->header.first_free_entry.ref.index = ((timeline->header.first_free_entry.ref.index
			^ timelineKey->header.first_free_entry.ref.index) + 1)
			^ timelineKey->header.first_free_entry.ref.index;
		SETFLAG( vol->dirty, cache );
	}

	// make sure the new entry is emptied.
	time->clesser.ref.index = 0;
	time->clesser.ref.depth = 0;
	time->cgreater.ref.index = 0;
	time->cgreater.ref.depth = 0;

	time->slesser.ref.index = 0;
	time->slesser.ref.depth = 0;
	time->sgreater.ref.index = 0;
	time->sgreater.ref.depth = 0;

	time->stime.raw = time->ctime.raw = GetTimeOfDay();

	if( init ) init( psv, time );

	hangTimelineNode( vol
		, freeIndex
		, 0
		, timeline, timelineKey
		, time );
	hangTimelineNode( vol
		, freeIndex
		, 1
		, timeline, timelineKey
		, time );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol );
	//DumpTimelineTree( vol, TRUE );
	//DumpTimelineTree( vol, FALSE );
}


