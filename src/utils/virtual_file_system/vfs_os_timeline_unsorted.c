//#define DEBUG_TEST_LOCKS

//#define DEBUG_VALIDATE_TREE_ADD
//#define DEBUG_LOG_LOCKS

//#define INVERSE_TEST
//#define DEBUG_DELETE_BALANCE

//#define DEBUG_TIMELINE_REORDER_LOGGING

//#define DEBUG_AVL_DETAIL

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
		uint64_t index;
	} ref;
} TIMELINE_BLOCK_TYPE;

#  ifdef _MSC_VER
#    pragma pack (push, 1)
#  endif
PREFIX_PACKED struct timelineHeader {
	TIMELINE_BLOCK_TYPE first_free_entry;
	TIMELINE_BLOCK_TYPE crootNode_deleted;
	TIMELINE_BLOCK_TYPE srootNode;  // this index is 0 when initialized, and has a +1 to the entry number.
	TIMELINE_BLOCK_TYPE last_added_entry;
	uint64_t unused[4];
	//uint64_t unused2[8];
} PACKED;

// current size is 64 bytes.
// me_fpi is the physical FPI in the timeline file of the TIMELINE_BLOCK_TYPE that references 'this' block.
// structure defines little endian structure for storage.

PREFIX_PACKED struct storageTimelineNode0 {
	// if dirent_fpi == 0; it's free; and priorData will point at another free node
	uint64_t dirent_fpi;

	uint32_t priorTime;
	uint16_t priorDataPad;
	uint8_t  filler8_1; // how much of the last block in the file is not used

	uint8_t  timeTz; // lesser least significant byte of time... sometimes can read time including timezone offset with time - 1 byte

	uint64_t time;

	uint64_t priorData; // if not 0, references a start block version of data.
} PACKED;

PREFIX_PACKED struct storageTimelineNode {
	// if dirent_fpi == 0; it's free; and priorData will point at another free node
	uint64_t dirent_fpi;

	uint32_t priorTime;
	uint16_t priorDataPad;
	uint8_t  filler8_1; // how much of the last block in the file is not used

	uint8_t  timeTz; // lesser least significant byte of time... sometimes can read time including timezone offset with time - 1 byte

	uint64_t time;

	uint64_t priorData; // if not 0, references a start block version of data.

	uint64_t nextWrite; // if not 0, references a start block version of data.
	uint64_t priorWrite; // if not 0, references a start block version of data.
	uint64_t priorDataSize; // This is the actual size of the data starting at block priorData
	uint64_t filler64_2; // if not 0, references a start block version of data.
} PACKED;

#  ifdef _MSC_VER
#    pragma pack (pop)
#  endif

struct memoryTimelineNode {
	// if dirent_fpi == 0; it's free.
	FPI this_fpi;
	uint64_t index;
	// the end of this is the same as storage timeline.

	struct storageTimelineNode* disk;
	enum block_cache_entries diskCache;
};


struct storageTimelineCursor {
	PDATASTACK parentNodes;  // save stack of parents in cursor
	struct storageTimelineCache dirents; // temp; needs work.
};

struct sack_vfs_os_time_cursor {
	struct sack_vfs_os_volume* vol;
	uint64_t at;
};

#  ifdef _MSC_VER
#    pragma pack (push, 1)
#  endif

#define NUM_ROOT_TIMELINE_NODES (TIME_BLOCK_SIZE - sizeof( struct timelineHeader )) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimeline {
	struct timelineHeader header;
	struct storageTimelineNode entries[NUM_ROOT_TIMELINE_NODES];
} PACKED;

/*
#define NUM_TIMELINE_NODES (TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimelineBlock {
	struct storageTimelineNode entries[(TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )];
} PACKED;
*/

#  ifdef _MSC_VER
#    pragma pack (pop)
#  endif

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

#define convertMeToParentFPI(n) ((n)&~0x3f)
#define convertMeToParentIndex(n) (((n)>sizeof(struct timelineHeader))?( ( convertMeToParentFPI((n)&~0x3f)- sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) + 1 ):0)


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
	//_lprintf(DBG_RELAY)( "Load Entry %d", (int)timeEntry );

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
	//lprintf( "Read Entry %d", (int)timeEntry );
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

#ifdef DEBUG_TIMELINE_REORDER_LOGGING

// didn't actually have to use this.
static void dumpTimeline( struct sack_vfs_os_volume* vol ) {
	lprintf( "--- Timeline ----" );
	lprintf( "root %lld last %lld free %lld", vol->timeline->header.srootNode.raw, vol->timeline->header.last_added_entry.raw, vol->timeline->header.first_free_entry.raw );
	int entry;
	enum block_cache_entries_os cache = BC( TIMELINE );
	struct storageTimelineNode* block;
	for( entry = 1; entry != vol->timeline->header.first_free_entry.raw; entry++ ) {
		block = getRawTimeEntry( vol, entry, &cache GRTENoLog DBG_SRC );
		lprintf( "Entry %d  de:%lld prev:%lld next:%lld time:%lld tz:%d", entry, block->dirent_fpi, block->priorWrite, block->nextWrite, block->time, block->timeTz );
		dropRawTimeEntry( vol, cache GRTENoLog DBG_SRC );
	}
}
#endif


//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------

static void reorderEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, int toEnd DBG_PASS ) {
	if( time ) {
		// time changed...(maybe?)
		{
			uint64_t myself = time->index;
			struct storageTimelineNode* prev;
			enum block_cache_entries_os cache, _cache = BC(ZERO);
			if( time->disk->priorWrite ) {
				prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
			} else { prev = NULL; cache = BC(ZERO); }

			enum block_cache_entries_os cache2, _cache2 = BC(ZERO);
			struct storageTimelineNode* next;
			if( time->disk->nextWrite ) {
				next = getRawTimeEntry( vol, time->disk->nextWrite, &cache2 GRTENoLog DBG_RELAY );
			} else { next = NULL; cache2 = BC(ZERO); }

			if( toEnd ) {
				enum block_cache_entries_os cache3;
				struct storageTimelineNode* last;
				last = getRawTimeEntry( vol, vol->timeline->header.last_added_entry.ref.index, &cache3 GRTENoLog DBG_SRC );
				if( last && last->time <= time->disk->time ) {
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
					dumpTimeline( vol );
#endif
					last->nextWrite = myself;
					if( !time->disk->priorWrite ) {
						if( next ) next->priorWrite = 0;
						vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
					} else if(next ) next->priorWrite = time->disk->priorWrite;
					if( prev ) prev->nextWrite = time->disk->nextWrite;
					time->disk->priorWrite = vol->timeline->header.last_added_entry.ref.index;
					time->disk->nextWrite = 0;
					// if this is the new end of the list, update the last entry....
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
					lprintf( "new last block:%lld after %lld", myself, vol->timeline->header.last_added_entry.ref.index );
#endif
					vol->timeline->header.last_added_entry.ref.index = myself;
					SMUDGECACHE( vol, vol->timelineCache );
					if( prev ) dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
					if( next ) dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
					if( last ) dropRawTimeEntry( vol, cache3 GRTELog DBG_RELAY );
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
					dumpTimeline( vol );
#endif
					return;
				} else {
					if( last ) dropRawTimeEntry( vol, cache3 GRTENoLog DBG_RELAY );
				}
			}

			if( next && ( next->time < time->disk->time ) ) {
				//myself = next->priorWrite;
				if( prev )
					prev->nextWrite = time->disk->nextWrite;
				else {
					vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
					SMUDGECACHE( vol, vol->timelineCache );
				}
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
				lprintf( "Searching forward...." );
#endif
				next->priorWrite = time->disk->priorWrite;
				while( ( prev = next ), (_cache? dropRawTimeEntry(vol,_cache GRTENoLog DBG_RELAY ):(void)0), ( _cache=cache ), (cache=BC(TIMELINE)),
					( next = getRawTimeEntry( vol, prev->nextWrite, &cache GRTENoLog DBG_SRC ) )
					) {
					if( !next->nextWrite ) {
						if( next->time < time->disk->time ) {

							next->nextWrite = myself;
							if( !time->disk->priorWrite ) {
								struct storageTimelineNode* next;
								enum block_cache_entries_os cache = BC( TIMELINE );
								next = getRawTimeEntry( vol, time->disk->nextWrite, &cache GRTENoLog DBG_RELAY );
								if( next ) next->priorWrite = 0;
								vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
								dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
							}
							time->disk->priorWrite = prev->nextWrite;
							time->disk->nextWrite = 0;

							// if this is the new end of the list, update the last entry....
							vol->timeline->header.last_added_entry.ref.index = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							break; // done. (at end anyway)
						}
					}

					if( next->time > time->disk->time ) {
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
						lprintf( "found insertion point %lld  %lld %lld", myself, prev->nextWrite, next->priorWrite );
#endif
						if( !time->disk->priorWrite ) {
							struct storageTimelineNode* next;
							enum block_cache_entries_os cache = BC( TIMELINE );
							next = getRawTimeEntry( vol, time->disk->nextWrite, &cache GRTENoLog DBG_RELAY );
							if( next ) next->priorWrite = 0;
							vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
							dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
						}
						if( !time->disk->nextWrite ) {
							struct storageTimelineNode* prev;
							enum block_cache_entries_os cache = BC( TIMELINE );
							prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
							if( prev ) prev->nextWrite = 0;
							vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
							dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
						}
						time->disk->nextWrite = prev->nextWrite;
						time->disk->priorWrite = next->priorWrite;
						prev->nextWrite = myself;
						next->priorWrite = myself;
						dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
						if( cache2 ) dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
						break;
					} else {
						if( !next->nextWrite ) {
							next->nextWrite = myself;
							if( !time->disk->priorWrite ) {
								struct storageTimelineNode* next;
								enum block_cache_entries_os cache = BC( TIMELINE );
								next = getRawTimeEntry( vol, time->disk->nextWrite, &cache GRTENoLog DBG_RELAY );
								if( next ) next->priorWrite = 0;
								vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
								dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
							}
							time->disk->priorWrite = prev->nextWrite;
							time->disk->nextWrite = 0;
							// if this is the new end of the list, update the last entry....
							vol->timeline->header.last_added_entry.ref.index = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
							lprintf( "ran into end of chain anyway %lld", myself );
							if( cache2 ) dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
							break;
						}
					}
				}
			} else if( prev && ( prev->time > time->disk->time ) ) {
				//myself = prev->nextWrite;
				if( !( prev->nextWrite = time->disk->nextWrite ) ) {
					vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
					SMUDGECACHE( vol, vol->timelineCache );
				}
				if( next )
					next->priorWrite = time->disk->priorWrite;
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
				lprintf( "Searching backward" );
				dumpTimeline( vol );
#endif
				while( (next = prev ), ( _cache2 ? dropRawTimeEntry( vol, _cache2 GRTENoLog DBG_RELAY ) : (void)0 ), ( _cache2 = cache2 ), ( cache2 = BC( TIMELINE ) ) ) {
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
					lprintf( "checking next record %lld", next->priorWrite );
#endif
					if( !next->priorWrite ) {
						next->priorWrite = myself;
						if( !time->disk->nextWrite ) {
							struct storageTimelineNode* prev;
							enum block_cache_entries_os cache = BC( TIMELINE );
							prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
							if( prev ) prev->nextWrite = 0;
							vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
							dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
						}
						time->disk->nextWrite = vol->timeline->header.srootNode.ref.index;
						time->disk->priorWrite = 0;
						vol->timeline->header.srootNode.ref.index = myself;
						SMUDGECACHE( vol, vol->timelineCache );
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
						lprintf( "Saving as first..." );
#endif
						if( cache ) dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
						dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
						break;
					} else {
						( prev = getRawTimeEntry( vol, next->priorWrite, &cache2 GRTENoLog DBG_SRC ) );
					}
					if( !prev->priorWrite ) {
						if( prev->time > time->disk->time ) {
							// new root node...
							vol->timeline->header.srootNode.ref.index = prev->priorWrite = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							if( !time->disk->priorWrite ) {
								struct storageTimelineNode* next;
								enum block_cache_entries_os cache = BC( TIMELINE );
								next = getRawTimeEntry( vol, time->disk->nextWrite, &cache GRTENoLog DBG_RELAY );
								if( next ) next->priorWrite = 0;
								vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
								dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
							}
							if( !time->disk->nextWrite ) {
								struct storageTimelineNode* prev;
								enum block_cache_entries_os cache = BC( TIMELINE );
								prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
								if( prev ) prev->nextWrite = 0;
								vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
								dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
							}
							time->disk->priorWrite = 0;
							time->disk->nextWrite = next->priorWrite;
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
							lprintf( "Saving as first(2)..." );
#endif
							if( cache ) dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
							dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
							break; // done. (at end anyway)
						}
					}

					if( prev->time < time->disk->time ) {
						if( !time->disk->nextWrite ) {
							struct storageTimelineNode* prev;
							enum block_cache_entries_os cache = BC( TIMELINE );
							prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
							if( prev ) prev->nextWrite = 0;
							vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
							dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
						}
						time->disk->nextWrite = prev->nextWrite;
						time->disk->priorWrite = next->priorWrite;
						prev->nextWrite = myself;
						next->priorWrite = myself;
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
						lprintf( "Saving in middle..." );
#endif
						if( cache ) dropRawTimeEntry( vol, cache GRTELog DBG_RELAY );
						dropRawTimeEntry( vol, cache2 GRTELog DBG_RELAY );
						break;
					}
				}
			} else {
				// didn't have to move anything... maybe it's time is still the same relative to everything?
			}

		}
	}
#ifdef DEBUG_TIMELINE_REORDER_LOGGING
	dumpTimeline( vol );
#endif
}
//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL drop DBG_PASS ) {
	if( time ) {
		SMUDGECACHE( vol, time->diskCache );
		// time changed...(maybe?)
#if 0
		{
			uint64_t myself;
			struct storageTimelineNode* prev;
			enum block_cache_entries_os cache;
			if( time->disk->priorWrite ) {
				prev = getRawTimeEntry( vol, time->disk->priorWrite, &cache GRTENoLog DBG_RELAY );
			} else prev = NULL;
			struct storageTimelineNode* next;
			if( time->disk->nextWrite ) {
				next = getRawTimeEntry( vol, time->disk->nextWrite, &cache GRTENoLog DBG_RELAY );
			} else next = NULL;

			if( next && ( next->time < time->disk->time ) ) {
				myself = next->priorWrite;
				if( prev )
					prev->nextWrite = time->disk->nextWrite;
				else {
					vol->timeline->header.srootNode.ref.index = time->disk->nextWrite;
					SMUDGECACHE( vol, vol->timelineCache );
				}

				next->priorWrite = time->disk->priorWrite;
				while( ( prev = next ),
					( next = getRawTimeEntry( vol, prev->nextWrite, &cache GRTENoLog DBG_SRC ) )
					) {
					if( !next->nextWrite ) {
						if( next->time < time->disk->time ) {
							next->nextWrite = myself;
							time->disk->priorWrite = prev->nextWrite;
							time->disk->nextWrite = 0;

							// if this is the new end of the list, update the last entry....
							vol->timeline->header.last_added_entry.ref.index = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							break; // done. (at end anyway)
						}
					}

					if( next->time > time->disk->time ) {
						time->disk->nextWrite = prev->nextWrite;
						time->disk->priorWrite = next->priorWrite;
						prev->nextWrite = myself;
						next->priorWrite = myself;
						break;
					} else {
						if( !next->nextWrite ) {
							next->nextWrite = myself;
							time->disk->priorWrite = prev->nextWrite;
							time->disk->nextWrite = 0;
							// if this is the new end of the list, update the last entry....
							vol->timeline->header.last_added_entry.ref.index = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							break;
						}
					}
				}
			} else if( prev && ( prev->time > time->disk->time ) ) {
				myself = prev->nextWrite;
				if( !(prev->nextWrite = time->disk->nextWrite) ) 	{
					vol->timeline->header.last_added_entry.ref.index = time->disk->priorWrite;
					SMUDGECACHE( vol, vol->timelineCache );
				}
				if( next )
					next->priorWrite = time->disk->priorWrite;

				while( next = prev ) {
					if( !next->priorWrite ) {
						next->priorWrite = myself;
						time->disk->nextWrite = vol->timeline->header.srootNode.ref.index;
						time->disk->priorWrite = 0;
						vol->timeline->header.srootNode.ref.index = myself;
						SMUDGECACHE( vol, vol->timelineCache );
						break;
					} else
						( prev = getRawTimeEntry( vol, next->priorWrite, &cache GRTENoLog DBG_SRC ) );
					if( !prev->priorWrite ) {
						if( prev->time > time->disk->time ) {
							// new root node...
							vol->timeline->header.srootNode.ref.index = prev->priorWrite = myself;
							SMUDGECACHE( vol, vol->timelineCache );
							time->disk->priorWrite = 0;
							time->disk->nextWrite = next->priorWrite;
							break; // done. (at end anyway)
						}
					}

					if( prev->time < time->disk->time ) {
						time->disk->nextWrite = prev->nextWrite;
						time->disk->priorWrite = next->priorWrite;
						prev->nextWrite = myself;
						next->priorWrite = myself;
						break;
					}
				}
			} else {
				// didn't have to move anything... maybe it's time is still the same relative to everything?
			}
		}
#endif
	}

	if( drop ) {
		int locks;
		int bit = time->diskCache;
		locks = GETMASK_( vol->seglock, seglock, bit );
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
		SETMASK_( vol->seglock, seglock, bit, locks );
	}
}

//---------------------------------------------------------------------------

void reloadDirectoryEntry( struct sack_vfs_os_volume* vol, struct memoryTimelineNode* time, struct sack_vfs_os_find_info* decoded_dirent DBG_PASS ) {
	enum block_cache_entries cache = BC( DIRECTORY );
	struct directory_entry* dirent;// , * entkey;
	struct directory_hash_lookup_block* dirblock;
	//struct directory_hash_lookup_block* dirblockkey;
	PDATASTACK pdsChars = CreateDataStack( 1 );
	BLOCKINDEX this_dir_block = (time->disk->dirent_fpi >> DIR_BLOCK_SIZE_BITS )-1;
	BLOCKINDEX next_block;
	dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
	//dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
	dirent = (struct directory_entry*)( ( (uintptr_t)dirblock ) + ( time->disk->dirent_fpi & ( DIR_BLOCK_SIZE - 1 ) ) );
	//entkey = (struct directory_entry*)(((uintptr_t)dirblockkey) + (time->dirent_fpi & BLOCK_SIZE));

	decoded_dirent->vol = vol;

	// all of this regards the current state of a find cursor...
	decoded_dirent->base = NULL;
	decoded_dirent->base_len = 0;
	decoded_dirent->mask = NULL;
	decoded_dirent->pds_directories = NULL;

	decoded_dirent->filesize = (size_t)( dirent->filesize );
	if( time->disk->priorTime ) {
		enum block_cache_entries cache;
		struct storageTimelineNode* prior = getRawTimeEntry( vol, time->disk->priorTime, &cache GRTENoLog DBG_SRC );
		while( prior->priorTime ) {
			dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
			prior = getRawTimeEntry( vol, prior->priorTime, &cache GRTENoLog DBG_RELAY );
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

static void deleteTimelineIndex( struct sack_vfs_os_volume* vol, BLOCKINDEX index ) {
	BLOCKINDEX next;
	do {
		struct storageTimelineNode* time;
		enum block_cache_entries cache = BC( TIMELINE );

		//lprintf( "Delete start... %d", index );
		time = getRawTimeEntry( vol, index, &cache GRTELog DBG_SRC );
		next = (BLOCKINDEX)time->priorTime; // this type is larger than index in some configurations
		nodes--;

		{
			struct storageTimeline* timeline = vol->timeline;
			time->priorTime = (uint32_t)timeline->header.first_free_entry.ref.index;
			timeline->header.first_free_entry.ref.index = index;
			SMUDGECACHE( vol, vol->timelineCache );
			SMUDGECACHE( vol, cache );
		}

		dropRawTimeEntry( vol, cache GRTELog DBG_SRC );
#ifdef DEBUG_VALIDATE_TREE
		//ValidateTimelineTree( vol DBG_SRC );
#endif
		//lprintf( "Delete done... %d", index );
	} while( index = next );
#ifdef DEBUG_DELETE_LAST
	checkRoot( vol );
#endif
	if( !nodes && vol->timeline->header.srootNode.ref.index ) {
		lprintf( "No more nodes, but the root points at something." );
		DebugBreak();
	}
	//lprintf( "Root is now %d %d", nodes, vol->timeline->header.srootNode.ref.index );
}

BLOCKINDEX getTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL unused, void(*init)(uintptr_t, struct memoryTimelineNode*), uintptr_t psv DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE );
	enum block_cache_entries cache_last = BC( TIMELINE );
	enum block_cache_entries cache_free = BC( TIMELINE );
	enum block_cache_entries cache_new = BC( TIMELINE );
	struct storageTimeline* timeline = vol->timeline;
	TIMELINE_BLOCK_TYPE freeIndex;
	BLOCKINDEX index;
	BLOCKINDEX priorIndex = (BLOCKINDEX)time->index; // ref.index type is larger than index in some configurations; but won't exceed those bounds
	BLOCKINDEX lastIndex = timeline->header.last_added_entry.ref.index;

	freeIndex.ref.index = timeline->header.first_free_entry.ref.index;

	// update next free.
	reloadTimeEntry( time, vol, index = (BLOCKINDEX)freeIndex.ref.index VTReadWrite GRTELog DBG_RELAY ); // ref.index type is larger than index in some configurations; but won't exceed those bounds

	if( !timeline->header.srootNode.ref.index )
		timeline->header.srootNode.ref.index = 1;

	timeline->header.first_free_entry.ref.index = timeline->header.first_free_entry.ref.index + 1;

	// make sure the new entry is emptied.
	//time->disk->me_fpi = 0;
	time->disk->dirent_fpi = 0;
	time->disk->priorTime = 0;
	time->disk->priorData = 0;
	time->disk->priorDataSize = 0;
	if( lastIndex )
	{
		enum block_cache_entries cache_near = BC( TIMELINE );
		struct storageTimelineNode* last = getRawTimeEntry( vol, lastIndex, &cache_near GRTENoLog DBG_RELAY );
		if( !last->nextWrite ) {
			last->nextWrite = index;
			// updated a value here...
			SMUDGECACHE( vol, cache_near );
		} else {
			lprintf( "Shouldn't have to find what the last node in the chain is...." );
			/*
			dropRawTimeEntry( vol, cache_near GRTENoLog DBG_RELAY ); 
			while( last = getRawTimeEntry( vol, last->nextWrite, &cache_near GRTENoLog DBG_RELAY ) ) {
				dropRawTimeEntry( vol, cache_near );
				if( !last->nextWrite ) {
					last->nextWrite = index;
					break;
				}
			}
			*/
		}
		dropRawTimeEntry( vol, cache_near GRTENoLog DBG_RELAY );
	}
	time->disk->priorWrite = lastIndex;
	time->disk->nextWrite = 0;
	time->disk->time = timeGetTime64ns(); // there really shouldn't be any times after this one....
	timeline->header.last_added_entry.ref.index = index;

	SMUDGECACHE( vol, vol->timelineCache );

	{
		int tz = GetTimeZone();
		if( tz < 0 )
			tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
		else
			tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
		//time->disk->time += (int64_t)tz * 900 * (int64_t)1000000000;
		time->disk->timeTz = tz;
	}

	if( init ) init( psv, time );
	nodes++;
	//lprintf( "Add start... %d", freeIndex.ref.index );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol, FALSE DBG_RELAY ); // don't drop; returning this one.

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
			//enum block_cache_entries inputCache = time ? time->diskCache : BC( ZERO );
			BLOCKINDEX newIndex = getTimeEntry( time, vol, TRUE, init, psv DBG_RELAY );
			time->disk->priorTime = (uint32_t)inputIndex;

			updateTimeEntry( time, vol, FALSE DBG_RELAY );
			//dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );
			return newIndex;
		}
		else {
			struct memoryTimelineNode time_;
			struct storageTimelineNode* timeold;
			uint64_t inputIndex = index;
			enum block_cache_entries inputCache;
			FPI dirent_fpi;
			timeold = getRawTimeEntry( vol, index, &inputCache GRTELog DBG_RELAY );
			dirent_fpi = (FPI)timeold->dirent_fpi; // ref.index type is larger than index in some configurations; but won't exceed those bounds
			dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );

			// gets a new timestamp.
			time_.index = index;
			BLOCKINDEX newIndex = getTimeEntry( &time_, vol, TRUE, init, psv DBG_RELAY );
			time_.disk->priorTime = (uint32_t)inputIndex;
			time_.disk->dirent_fpi = dirent_fpi;
			updateTimeEntry( &time_, vol, TRUE DBG_RELAY );
			return newIndex;
		}
	}
	else {
		struct memoryTimelineNode time_;
		LOGICAL existing = ( time ) ? 1 : 0;
		if( !time ) time = &time_;
		reloadTimeEntry( time, vol, index VTReadWrite GRTENoLog DBG_RELAY );
		time->disk->time = timeGetTime64ns();
		{
			int tz = GetTimeZone();
			if( tz < 0 )
				tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
			else
				tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
			//time->disk->time += (int64_t)tz * 900 * (int64_t)1000000000;
			time->disk->timeTz = tz;
		}
		reorderEntry( time, vol, 1 DBG_RELAY );
		updateTimeEntry( time, vol, TRUE DBG_RELAY );
		return (BLOCKINDEX)index; // index type is larger than index in some configurations; but won't exceed those bounds
	}
}

LOGICAL setTimeEntryTime( struct memoryTimelineNode* time
			, struct sack_vfs_os_volume *vol
			, uint64_t tick 
			, int tz ) {
	if( !time ) {
//time = &time_;
		lprintf( "invalid time entry passed" );
		return FALSE;
	} else {
		//reloadTimeEntry( time, vol, index VTReadWrite GRTENoLog DBG_RELAY );
		time->disk->timeTz = tz;
		time->disk->time = tick;
		reorderEntry( time, vol, 0 DBG_SRC );
		updateTimeEntry( time, vol, FALSE DBG_SRC );
		return TRUE;
	}
}

struct sack_vfs_os_time_cursor* sack_vfs_os_get_time_cursor( struct sack_vfs_os_volume *vol ) {
	struct sack_vfs_os_time_cursor* cursor;
	cursor = New( struct sack_vfs_os_time_cursor );
	cursor->vol = vol;
	cursor->at = 0;
	return cursor;
}


//--------------------------------
//  read TIme Cursor reads/steps the cursor...
//    step==0 && time === 0 && at === 0 ; start at the start of timeline.
//    step==0 && time === N ; seek to time N, update at to the found record
//    step==1 && time === N ; seek to record N, update at to the time at the indexed record
//

LOGICAL sack_vfs_os_read_time_cursor( struct sack_vfs_os_time_cursor* cursor, int step, uint64_t time, uint64_t* result_entry, const char**filename
	, uint64_t *result_timestamp, int8_t *result_tz, const char**buffer, size_t *size ) {
	static char* dataBuffer;
	static size_t bufsize;
	//uint64_t time = (time_ >> 8) * 1000000;

	enum block_cache_entries_os cache; // last raw entry cache
	LOGICAL dropCache = FALSE;
	uint64_t entry = 0; // used as the record found indicator.

	if( step == 2 ) {
		entry = cursor->at;
	}
	else if( step == 1 ) {
		if( !time ) {
			cursor->at = entry = cursor->vol->timeline->header.srootNode.ref.index;
		}else
			cursor->at = entry = time;
	}

	else if( step == 0 ) {
		// if( !time_ )
		struct storageTimelineNode* timeNode = getRawTimeEntry( cursor->vol, cursor->vol->timeline->header.srootNode.ref.index, &cache GRTENoLog DBG_SRC );
		while( timeNode && timeNode->time < time ) {
			uint64_t next = timeNode->nextWrite;
			dropRawTimeEntry( cursor->vol, cache  GRTENoLog DBG_SRC );
			if( next ) timeNode = getRawTimeEntry( cursor->vol, next, &cache GRTENoLog DBG_SRC );
			else timeNode = NULL;
			entry = next;
		}
		if( !timeNode )
			return FALSE;
		dropCache = TRUE;
	}

	if( entry )
	{
		LOGICAL retVal = TRUE;
		{
		struct memoryTimelineNode memEntry;

		reloadTimeEntry( &memEntry, cursor->vol, entry GRTENoLog DBG_SRC );
		if( memEntry.disk->dirent_fpi ) {
			cursor->at = memEntry.disk->nextWrite;
			struct sack_vfs_os_find_info decoded_dirent;
			reloadDirectoryEntry( cursor->vol, &memEntry, &decoded_dirent DBG_SRC );
			if( result_entry ) {
				result_entry[0] = memEntry.index;
			}
			if( result_tz ) {
				result_tz[0] = memEntry.disk->timeTz;
			}
			if( result_timestamp ) {
				result_timestamp[0] = memEntry.disk->time;
			}

			if( filename ) {
				filename[0] = StrDup( decoded_dirent.filename );
			}
			if( size ) {
				size[0] = decoded_dirent.filesize;

				if( buffer ) {
					if( bufsize < size[0] ) {
						dataBuffer = (char*)Reallocate( dataBuffer, size[0] );
					}
					buffer[0] = dataBuffer;
					{
						// there might be a more optimal method of doing this; but this is easy to read.
						struct sack_vfs_file* file = sack_vfs_os_openfile( cursor->vol, decoded_dirent.filename );
						sack_vfs_os_read( file, dataBuffer, size[0] );
						sack_vfs_os_close( file );
					}
				}
			}
		} else
			retVal = FALSE;

		dropRawTimeEntry( cursor->vol, memEntry.diskCache GRTENoLog DBG_SRC );

		if( dropCache )
			dropRawTimeEntry( cursor->vol, cache  GRTENoLog DBG_SRC );
		}
		return retVal;
		//cursor->at = time;
	}


	return FALSE;
}

