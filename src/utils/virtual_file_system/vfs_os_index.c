


/*
 *  this is a disk reference to the next node storing the depth of that side with the reference.
 *  references should be by entry index, unless variable length in which case it is an absolute offset
 */

typedef union indexBlockType {
	// 0 is invalid; indexes must subtract 1 to get
	// real index index.
	uint64_t raw;
	struct indexBlockReference {
		uint64_t depth : 6; // 64 depth.
		uint64_t index : 58;
	} ref;
} INDEX_BLOCK_TYPE;


/*
 *   this is a struct storageIndexEntry with related temporary information for tracking
 */
struct memoryIndexEntry {
	// if dirent_fpi == 0; it's free.
	FPI this_fpi;     // for where to update this record in the file.
	uint64_t index;   // this index
	// if the block is free, greater is used as pointer to next free block
	// delete an object can leave free index nodes in the middle of the physical chain.

	//FPI dirent_fpi;   // directory FPI on disk that this record is
	//INDEX_BLOCK_TYPE lesser;         // FPI/32 within index chain
	//INDEX_BLOCK_TYPE greater;        // FPI/32 within index chain + (child depth in this direction AVL)
	struct memoryIndexEntry* parent;// reloading the index will update this
	struct storageIndexEntry* data;  /* pointer to the real entry('s data?)*/
};
typedef struct memoryIndexEntry MEMORY_INDEX_ENTRY, * PMEMORY_INDEX_ENTRY;
#define MAXMEMORY_INDEX_ENTRYSPERSET 256
DeclareSet( MEMORY_INDEX_ENTRY );


/*
 * This is the header of the entry in the index file.
 * IF  (tom baker or davidson?  IF... Index File... IF I had the index file I could find the index file)
 *
 */
struct storageIndexEntry {
	FPI dirent_fpi; // if 0, this entry is free.

	union {
		struct {
			INDEX_BLOCK_TYPE lesser;
			INDEX_BLOCK_TYPE greater;
		} used;
		struct {
			INDEX_BLOCK_TYPE prior;
			INDEX_BLOCK_TYPE next;
		} free;
	} edge;
	union {
		uint64_t tick;
		uint64_t i64;
		uint32_t i32;
		uint16_t i16;
		uint8_t i8;
		double f64;
		float  f32;
		struct utf8Value {
			uint8_t code;  // 0 - prevent signed comparison underflow
			unsigned char utf8[1];
		} utf8;
	} data;
};


enum index_header_type_flags {
	iltf_double = 1,
	iltf_intsize = (0x7 << 1),
	iltf_fixed = 1,
};

#  ifdef _MSC_VER
#    pragma pack (push, 1)
#  endif

PREFIX_PACKED struct index_header {
	INDEX_BLOCK_TYPE first_free_entry; // blank entry list, if any
	INDEX_BLOCK_TYPE rootNode;
	INDEX_BLOCK_TYPE next_free_entry; // phsyical last entry (size of index)

	struct index_header_type_info {
		enum jsox_value_types keyType;
		uint8_t flags; // 1 = float(double) value
		uint16_t keyLength; // 1 = variable (offsets are byte position)
	} indexType;

	uint16_t indexNameLength; // 1 = variable (offsets are byte position)
	uint8_t indexName[1];  // align to quadword
	// 8 dwords? 
	// uint64_t unused[4];  // leader padding/extra info preallocate?

	struct storageIndexEntry firstEntry;

} PACKED;

#  ifdef _MSC_VER
#    pragma pack (pop)
#  endif

struct memoryStorageIndex {
	// name/identifer
	// data type
	//struct sack_vfs_os_volume* vol;
	char* name;
	PMEMORY_INDEX_ENTRYSET* entries;
	struct sack_vfs_os_file* file;
	struct index_header* diskData;
	struct storageIndexEntry* firstEntry;
	//size_t storageIndexEntry_length; // diskData->indexType.keyLength
	FLAGSETTYPE* diskDataLoadedSectors;
};


static int hangIndexNode( struct memoryStorageIndex* index
	, struct memoryIndexEntry* timelineNode
);



static void storageIndexEntry_update() {

}

static int storageIndexEntry_length( struct index_header* leader ) {
	switch( leader->indexType.keyType ) {
	default:
		lprintf( "unsuppoorted index type; WTF?" );
		break;
	case JSOX_VALUE_NUMBER:
		if( leader->indexType.flags & iltf_double ) {
			switch( (leader->indexType.flags & iltf_intsize) >> 1 ) {
			case 0: // invalid.
			case 1:
				return sizeof( leader->firstEntry.dirent_fpi ) + 4;
				break;
			case 2:
				return sizeof( leader->firstEntry.dirent_fpi ) + 8;
				break;
			}
		}
		else {
			switch( (leader->indexType.flags & iltf_intsize) >> 1 ) {
			case 0: // invalid.
			case 1:
				return sizeof( leader->firstEntry.dirent_fpi ) + 1;
				break;
			case 2:
				return sizeof( leader->firstEntry.dirent_fpi ) + 2;
				break;
			case 3:
				return sizeof( leader->firstEntry.dirent_fpi ) + 4;
				break;
			case 4:
				return sizeof( leader->firstEntry.dirent_fpi ) + 8;
				break;
			}
		}
		break;
	case JSOX_VALUE_STRING:
		if( leader->indexType.flags & iltf_fixed ) {
			return sizeof( leader->firstEntry.dirent_fpi ) + leader->indexType.keyLength;
		}
		else {
			lprintf( "Variable length values not yet supported" );
			return 1;
		}
		break;
	}
	lprintf( "unhandled type; return variable length" );
	return 1;
}

struct memoryStorageIndex* openIndexFile( struct sack_vfs_os_volume* vol, BLOCKINDEX startBlock ) {
	struct memoryStorageIndex* file = New( struct memoryStorageIndex );
	file->file = _os_createFile( vol, startBlock );


	file->diskDataLoadedSectors = NewArray( FLAGSETTYPE, (file->file->entry->filesize >> BLOCK_SIZE_BITS) / FLAGTYPEBITS( FLAGSETTYPE ) );
	file->diskData = (struct index_header*)NewArray( uint8_t, file->file->entry->filesize );

	{
		SETFLAG( file->diskDataLoadedSectors, 0 );
		sack_vfs_os_read_internal( file->file, file->diskData, BLOCK_SIZE );
	}
	return file;
}


struct memoryStorageIndex* createIndexFile( struct sack_vfs_os_volume* vol, const char *name, size_t nameLen  ) {
	struct memoryStorageIndex* file = New( struct memoryStorageIndex );
	BLOCKINDEX startBlock = _os_GetFreeBlock( vol, GFB_INIT_NONE );
	file->file = _os_createFile( vol, startBlock );
	file->file->entry->filesize = BLOCK_SIZE;

	file->diskDataLoadedSectors = NewArray( FLAGSETTYPE, (file->file->entry->filesize >> BLOCK_SIZE_BITS) / FLAGTYPEBITS( FLAGSETTYPE ) );
	file->diskData = (struct index_header*)NewArray( uint8_t, file->file->entry->filesize );
	file->diskData->indexNameLength = ( ( nameLen + 1 ) + 7 ) & ~7;
	file->firstEntry = (struct storageIndexEntry*)( ((uintptr_t)file->diskData) + sane_offsetof( struct index_header, indexName[nameLen] ) ) ;

	memcpy( file->diskData->indexName, name, nameLen );

	file->diskData->indexType.keyType = JSOX_VALUE_UNSET;
	file->diskData->indexType.flags = 0;
	file->diskData->indexType.keyLength = storageIndexEntry_length( file->diskData );

	file->diskData->first_free_entry.raw = 0;
	file->diskData->rootNode.raw = 0;

	SETFLAG( file->diskDataLoadedSectors, 0 );
	sack_vfs_os_write_internal( file->file, file->diskData, BLOCK_SIZE, NULL );

	//file->storageIndexEntry_length = file->diskData->indexType.keyLength;

	return file;

}

struct memoryStorageIndex* allocateIndex( struct sack_vfs_os_file* file
                  , const char* filename, size_t filenameLen 
) {
	uint32_t indexOffset = _os_AddSmallBlockUsage( &file->header.indexes, (uint32_t)(filenameLen + sizeof( BLOCKINDEX )) );
	struct memoryStorageIndex* newIndex = createIndexFile( file->vol, filename, filenameLen );
	//newIndex->name = DupCStrLen( filename, filenameLen );
	WriteIntoBlock( file, 3, indexOffset, &newIndex, sizeof( BLOCKINDEX ) );
	WriteIntoBlock( file, 3, indexOffset + sizeof( BLOCKINDEX ), filename, filenameLen );
	return newIndex;
}


struct storageIndexEntry* indexFileFind( struct memoryStorageIndex* index, void* data, size_t dataLen ) {
//	struct memoryStorageIndex* file;
	struct storageIndexEntry* entry = NULL;

	return entry;
}

struct memoryIndexEntry* loadIndexEntry( struct memoryStorageIndex* index, struct memoryIndexEntry* parent, uint64_t timeEntry ) {
	FPI pos = (FPI)( (sizeof( struct index_header ) - sizeof( struct storageIndexEntry ))
	               + index->diskData->indexType.keyLength * timeEntry);
	FPI ofs;
	struct memoryIndexEntry* entry;
	if( (ofs = index->diskData->indexType.keyLength) == 1 ) {
		ofs = BLOCK_SIZE;
		entry = GetFromSet( MEMORY_INDEX_ENTRY, &index->entries );
	}
	else {
		entry = GetUsedSetMember( MEMORY_INDEX_ENTRY, &index->entries, (INDEX)timeEntry );
		if( !entry ) {
			entry = GetSetMember( MEMORY_INDEX_ENTRY, &index->entries, (INDEX)timeEntry );

		}
	}
	entry->parent = parent;

	if( !TESTFLAG( index->diskDataLoadedSectors, pos >> BLOCK_SHIFT ) ) {
		sack_vfs_os_seek_internal( index->file, pos & BLOCK_MASK, SEEK_SET );
		sack_vfs_os_read_internal( index->file, index->diskData + (pos & BLOCK_MASK), BLOCK_SIZE );
		SETFLAG( index->diskDataLoadedSectors, pos >> BLOCK_SHIFT );
	}

	// load next sector too, if it spans a boundary

	if( !TESTFLAG( index->diskDataLoadedSectors, (pos + ofs) >> BLOCK_SHIFT ) ) {
		sack_vfs_os_seek_internal( index->file, (pos + ofs) & BLOCK_MASK, SEEK_SET );
		sack_vfs_os_read_internal( index->file, index->diskData + ((pos + ofs) & BLOCK_MASK), BLOCK_SIZE );
		SETFLAG( index->diskDataLoadedSectors, (pos + ofs) >> BLOCK_SHIFT );
	}

	entry->data = (struct storageIndexEntry*)(((uintptr_t)index->diskData) + pos);
	entry->index = timeEntry;
	entry->this_fpi = pos;

	//LoG( "Set this FPI: %d  %d", (int)timeEntry, (int)entry->this_fpi );
	return entry;
}


void reloadIndexEntry( struct memoryStorageIndex* index, struct memoryIndexEntry* entry, uint64_t timeEntry ) {
	FPI pos = (FPI)((sizeof( struct index_header ) - sizeof( struct storageIndexEntry ))
	               + index->diskData->indexType.keyLength * timeEntry);
	if( !TESTFLAG( index->diskDataLoadedSectors, pos >> BLOCK_SHIFT ) ) {
		sack_vfs_os_seek_internal( index->file, pos & BLOCK_MASK, SEEK_SET );
		sack_vfs_os_read_internal( index->file, index->diskData + (pos & BLOCK_MASK), BLOCK_SIZE );
		SETFLAG( index->diskDataLoadedSectors, pos >> BLOCK_SHIFT );
	}

	entry->data = (struct storageIndexEntry*)(((uintptr_t)index->diskData) + pos);
	entry->index = timeEntry;
	entry->this_fpi = pos;

	//LoG( "Set this FPI: %d  %d", (int)timeEntry, (int)entry->this_fpi );
}

void updateIndexEntry( struct memoryIndexEntry* entry, struct memoryStorageIndex* index ) {
	struct storageIndexEntry* node = (struct storageIndexEntry*)(((uintptr_t)index->diskData) + entry->this_fpi);

	//node->dirent_fpi = entry->dirent_fpi;

	//node->edge.used.lesser.raw = entry->lesser.raw;
	//node->edge.used.greater.raw = entry->lesser.raw;

	sack_vfs_os_seek_internal( index->file, (size_t)entry->this_fpi, SEEK_SET );
	sack_vfs_os_write_internal( index->file, node, sane_offsetof( struct storageIndexEntry, data ), NULL );

}

LOGICAL addIndexEntry( struct memoryStorageIndex* index // entry to fill
	, struct sack_vfs_os_file* refereceTo
	, struct jsox_value_container *value
	) {
	if( index->diskData->indexType.keyType == JSOX_VALUE_UNSET ) {
		index->diskData->indexType.keyType = value->value_type;
		// update data... 
	}
	else if( index->diskData->indexType.keyType != value->value_type ) {
		lprintf( "Data stored is not the proper type..." );
		return FALSE;
	}
	switch( value->value_type ) {
	case JSOX_VALUE_STRING: //= 4 string

		return TRUE;
		break;
	case JSOX_VALUE_NUMBER: //= 5 string + result_d | result_n
		// this will end up having to adjust to float or int based on type.
		// unless pre-specified.
		return TRUE;
		break;
		// up to here is supported in JSON
	case JSOX_VALUE_DATE:  // = 12 comes in as a number, string is data.
		return TRUE;
		break;
	case JSOX_VALUE_BIGINT: // = 13 string data, needs bigint library to process...
		lprintf( "Probably need a bigint library for this." );
		break;
	case JSOX_VALUE_TYPED_ARRAY:  // = 15 string is base64 encoding of bytes.
		lprintf( "Blob indexint?" );
		break;
		//, JSOX_VALUE_TYPED_ARRAY_MAX = JSOX_VALUE_TYPED_ARRAY + 12  // = 14 string is base64 encoding of bytes.
	default:
		lprintf( "Unsupported data type for index. %d", value->value_type );
		break;
	}
	return FALSE;
}

void getIndexEntry( struct memoryIndexEntry* entry // entry to fill
	, struct memoryStorageIndex* index
	, LOGICAL keepDirent
	, void(*init)(uintptr_t, struct memoryIndexEntry*), uintptr_t psv ) {

	FPI orig_dirent;
	struct index_header* indexHeader = index->diskData;
	TIMELINE_BLOCK_TYPE freeIndex;

	freeIndex.ref.index = indexHeader->first_free_entry.ref.index;
	freeIndex.ref.depth = 0;

	// update next free.
	if( keepDirent ) orig_dirent = entry->data->dirent_fpi;
	reloadIndexEntry( index, entry, freeIndex.ref.index );
	if( keepDirent ) entry->data->dirent_fpi = orig_dirent;

	if( entry->data->edge.used.greater.ref.index ) {
		indexHeader->first_free_entry.ref.index = entry->data->edge.used.greater.ref.index;
	}
	else {
		indexHeader->first_free_entry.ref.index = ((indexHeader->first_free_entry.ref.index) + 1);
	}
	lprintf( "need to flush index header update" );

	// make sure the new entry is emptied.
	entry->data->edge.used.lesser.ref.index = 0;
	entry->data->edge.used.lesser.ref.depth = 0;
	entry->data->edge.used.greater.ref.index = 0;
	entry->data->edge.used.greater.ref.depth = 0;

	//entry->data->
	//entry->stime.raw = entry->ctime.raw = GetTimeOfDay();

	if( init ) init( psv, entry );

	hangIndexNode( index, entry );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return entry entry:%d", entry->index );
#endif
	// flush to disk.
	updateIndexEntry( entry, index );
	//DumpTimelineTree( vol, TRUE );
	//DumpTimelineTree( vol, FALSE );
}







//---------------------------------------------------------------------------

static void _os_index_AVL_RotateToRight(
	struct memoryStorageIndex* index,
	PDATASTACK * pdsStack,
	struct memoryIndexEntry* node,
	struct memoryIndexEntry* left_
)
{
	//node->lesser.ref.index *
	struct memoryIndexEntry* parent;
	parent = (struct memoryIndexEntry*)PeekData( pdsStack ); // the next one up the tree

	{
		/* Perform rotation*/
		if( parent ) {
			if( parent->data->edge.used.lesser.ref.index == node->index )
				parent->data->edge.used.lesser.ref.index = node->data->edge.used.lesser.ref.index;
			else
#ifdef _DEBUG
				if( parent->data->edge.used.greater.ref.index == node->index )
#endif
					parent->data->edge.used.greater.ref.index = node->data->edge.used.lesser.ref.index;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		}
		else {
			index->diskData->rootNode.raw = node->data->edge.used.lesser.raw;
		}

		node->data->edge.used.lesser.raw = left_->data->edge.used.greater.raw;
		left_->data->edge.used.greater.ref.index = node->index;

		/* Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->data->edge.used.lesser.ref.depth;
			rightDepth = (int)node->data->edge.used.greater.ref.depth;
			if( leftDepth > rightDepth )
				left_->data->edge.used.greater.ref.depth = leftDepth + 1;
			else
				left_->data->edge.used.greater.ref.depth = rightDepth + 1;

			leftDepth = (int)left_->data->edge.used.lesser.ref.depth;
			rightDepth = (int)left_->data->edge.used.greater.ref.depth;

			if( parent ) {
				if( leftDepth > rightDepth ) {
					if( parent->data->edge.used.greater.ref.index == left_->index )
						parent->data->edge.used.greater.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->data->edge.used.lesser.ref.index == left_->index )
#endif
							parent->data->edge.used.lesser.ref.depth = leftDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
				else {
					if( parent->data->edge.used.greater.ref.index == left_->index )
						parent->data->edge.used.greater.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->data->edge.used.lesser.ref.index == left_->index )
#endif
							parent->data->edge.used.lesser.ref.depth = rightDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
			}
			else {
				if( leftDepth > rightDepth ) {
					index->diskData->rootNode.ref.depth = leftDepth + 1;
				}
				else {
					index->diskData->rootNode.ref.depth = rightDepth + 1;
				}
			}
		}
	}
	if( parent )
		updateIndexEntry( parent, index );
	else
		SETFLAG( index->file->vol->dirty, FIRST_TIMELINE_BLOCK );
	updateIndexEntry( node, index );
	updateIndexEntry( left_, index );
}

//---------------------------------------------------------------------------
// index tree hanger
//---------------------------------------------------------------------------

static void _os_index_AVL_RotateToLeft(
	struct memoryStorageIndex* index,
	PDATASTACK * pdsStack,
	struct memoryIndexEntry* node,
	struct memoryIndexEntry* right_
)
//#define _os_index_AVL_RotateToLeft(node)
{
	struct memoryIndexEntry* parent;
	parent = (struct memoryIndexEntry*)PeekData( pdsStack );

	{
		if( parent )
			if( parent->data->edge.used.lesser.ref.index == node->index )
				parent->data->edge.used.lesser.raw = node->data->edge.used.greater.raw;
			else
#ifdef _DEBUG
				if( parent->data->edge.used.greater.ref.index == node->index )
#endif
					parent->data->edge.used.greater.raw = node->data->edge.used.greater.raw;
#ifdef _DEBUG
				else
					DebugBreak();
#endif
		else
			index->diskData->rootNode.raw = node->data->edge.used.greater.raw;

		node->data->edge.used.greater.raw = right_->data->edge.used.lesser.raw;
		right_->data->edge.used.lesser.ref.index = node->index;

		/*  Update heights */
		{
			int leftDepth, rightDepth;
			leftDepth = (int)node->data->edge.used.lesser.ref.depth;
			rightDepth = (int)node->data->edge.used.greater.ref.depth;
			if( leftDepth > rightDepth )
				right_->data->edge.used.lesser.ref.depth = leftDepth + 1;
			else
				right_->data->edge.used.lesser.ref.depth = rightDepth + 1;

			leftDepth = (int)right_->data->edge.used.lesser.ref.depth;
			rightDepth = (int)right_->data->edge.used.greater.ref.depth;

			//struct memoryIndexEntry *parent;
			//parent = (struct memoryIndexEntry *)PeekData( pdsStack );
			if( parent ) {
				if( leftDepth > rightDepth ) {
					if( parent->data->edge.used.lesser.ref.index == right_->index )
						parent->data->edge.used.lesser.ref.depth = leftDepth + 1;
					else
#ifdef _DEBUG
						if( parent->data->edge.used.greater.ref.index == right_->index )
#endif
							parent->data->edge.used.greater.ref.depth = leftDepth + 1;
				}
				else {
					if( parent->data->edge.used.lesser.ref.index == right_->index )
						parent->data->edge.used.lesser.ref.depth = rightDepth + 1;
					else
#ifdef _DEBUG
						if( parent->data->edge.used.greater.ref.index == right_->index )
#endif
							parent->data->edge.used.greater.ref.depth = rightDepth + 1;
#ifdef _DEBUG
						else
							DebugBreak();
#endif
				}
			}
			else
				if( leftDepth > rightDepth ) {
					index->diskData->rootNode.ref.depth = leftDepth + 1;
				}
				else {
					index->diskData->rootNode.ref.depth = rightDepth + 1;
				}
		}
	}

	if( parent )
		updateIndexEntry( parent, index );
	else
		SETFLAG( index->file->vol->dirty, FIRST_TIMELINE_BLOCK );
	updateIndexEntry( node, index );
	updateIndexEntry( right_, index );
}

//---------------------------------------------------------------------------


static void _os_index_AVLbalancer( struct memoryStorageIndex* index, PDATASTACK * pdsStack
	, struct memoryIndexEntry* entry ) {
	struct memoryIndexEntry* _x = NULL;
	struct memoryIndexEntry* _y = NULL;
	struct memoryIndexEntry* _z = NULL;
	struct memoryIndexEntry* tmp;
	int leftDepth;
	int rightDepth;
	LOGICAL balanced = FALSE;
	_z = entry;

	while( _z ) {
		int doBalance;
		rightDepth = (int)_z->data->edge.used.greater.ref.depth;
		leftDepth = (int)_z->data->edge.used.lesser.ref.depth;
		if( tmp = (struct memoryIndexEntry*)PeekData( pdsStack ) ) {
#ifdef DEBUG_TIMELINE_AVL
			lprintf( "WR (P)left/right depths: %d  %d   %d    %d  %d", (int)tmp->index, (int)leftDepth, (int)rightDepth, (int)tmp->data->edge.used.greater.ref.index, (int)tmp->data->edge.used.lesser.ref.index );
			lprintf( "WR left/right depths: %d   %d   %d    %d  %d", (int)_z->index, (int)leftDepth, (int)rightDepth, (int)_z->data->edge.used.greater.ref.index, (int)_z->data->edge.used.lesser.ref.index );
#endif
			if( leftDepth > rightDepth ) {
				if( tmp->data->edge.used.greater.ref.index == _z->index ) {
					if( (1 + leftDepth) == tmp->data->edge.used.greater.ref.depth ) {
						//if( zz )
						//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
						break;
					}
					tmp->data->edge.used.greater.ref.depth = 1 + leftDepth;
				}
				else
#ifdef _DEBUG
					if( tmp->data->edge.used.lesser.ref.index == _z->index )
#endif
					{
						if( (1 + leftDepth) == tmp->data->edge.used.lesser.ref.depth ) {
							//if( zz )
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							break;
						}
						tmp->data->edge.used.lesser.ref.depth = 1 + leftDepth;
					}
#ifdef _DEBUG
					else
						DebugBreak();// Should be one or the other... 
#endif
			}
			else {
				if( tmp->data->edge.used.greater.ref.index == _z->index ) {
					if( (1 + rightDepth) == tmp->data->edge.used.greater.ref.depth ) {
						//if(zz)
						//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
						break;
					}
					tmp->data->edge.used.greater.ref.depth = 1 + rightDepth;
				}
				else
#ifdef _DEBUG
					if( tmp->data->edge.used.lesser.ref.index == _z->index )
#endif
					{
						if( (1 + rightDepth) == tmp->data->edge.used.lesser.ref.depth ) {
							//if(zz)
							//	lprintf( "Stopped checking: %d %d %d", height, leftDepth, rightDepth );
							break;
						}
						tmp->data->edge.used.lesser.ref.depth = 1 + rightDepth;
					}
#ifdef _DEBUG
					else
						DebugBreak();
#endif
			}
#ifdef DEBUG_TIMELINE_AVL
			lprintf( "WR updated left/right depths: %d      %d  %d", (int)tmp->index, (int)tmp->data->edge.used.greater.ref.depth, (int)tmp->data->edge.used.lesser.ref.depth );
#endif
			updateIndexEntry( tmp, index );
		}
		if( leftDepth > rightDepth )
			doBalance = ((leftDepth - rightDepth) > 1);
		else
			doBalance = ((rightDepth - leftDepth) > 1);


		if( doBalance ) {
			if( _x ) {
				if( _x->index == _y->data->edge.used.lesser.ref.index ) {
					if( _y->index == _z->data->edge.used.lesser.ref.index ) {
						// left/left
						_os_index_AVL_RotateToRight( index, pdsStack, _z, _y );
					}
					else {
						//left/rightDepth
						_os_index_AVL_RotateToRight( index, pdsStack, _y, _x );
						_os_index_AVL_RotateToLeft( index, pdsStack, _z, _y );
					}
				}
				else {
					if( _y->index == _z->data->edge.used.lesser.ref.index ) {
						_os_index_AVL_RotateToLeft( index, pdsStack, _y, _x );
						_os_index_AVL_RotateToRight( index, pdsStack, _z, _y );
						// rightDepth.left
					}
					else {
						//rightDepth/rightDepth
						_os_index_AVL_RotateToLeft( index, pdsStack, _z, _y );
					}
				}
#ifdef DEBUG_TIMELINE_AVL
				lprintf( "WR Balanced, should redo this one... %d %d", (int)_z->data->edge.used.lesser.ref.index, _z->data->edge.used.greater.ref.index );
#endif
			}
			else {
				//lprintf( "Not deep enough for balancing." );
			}
		}
		_x = _y;
		_y = _z;
		_z = (struct memoryIndexEntry*)PopData( pdsStack );
	}
}


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

void _os_index_deleteEntry( struct memoryStorageIndex* index, struct memoryIndexEntry* entry ) {
	// deletion can be done by swapping the least greater node or the greatest least
	// node dependong on which direction is tallest.
	// the operation will either reduce the height or not modify the height
	//PDATASTACK pdsStack
	struct memoryIndexEntry* replacement, * nextCandidate;

	if( entry->data->edge.used.lesser.ref.depth > entry->data->edge.used.greater.ref.depth ) {
		nextCandidate = loadIndexEntry( index, entry, entry->data->edge.used.lesser.ref.index );
		for( replacement = nextCandidate; replacement; replacement = nextCandidate ) {
			if( !replacement->data->edge.used.greater.ref.index ) {

				if( replacement->data->edge.used.lesser.ref.index ) {
					// this node has to replace the replacement node, and then the replacment node 
					// can be swapped in.
					//entry->data.

					if( entry->parent->data->edge.used.greater.ref.index == entry->index )
						entry->parent->data->edge.used.greater.ref.index = replacement->index;
					else
						entry->parent->data->edge.used.lesser.ref.index = replacement->index;
					entry->data = replacement->data;

				}
				else {
					//replacement->data.edge = entry->data.edge;
					//entry->
					//entry->data.edge
					//	entry->data = replacment->data;

				}

			}
			nextCandidate = loadIndexEntry( index, replacement, replacement->data->edge.used.greater.ref.index );
		}

	}
	else {
		//for( replacement = entry; replacement; replacement = nextCandidate ) {
			//nextCandidate =
		//}
	}
}

//---------------------------------------------------------------------------


static int _os_index_comparison( size_t len, LOGICAL isSigned, POINTER left, POINTER right ) {
	if( isSigned ) {
		int lsign = ((*((uint8_t*)left)) & 0x80);
		int rsign = ((*((uint8_t*)right)) & 0x80);
		if( lsign && rsign )
			return -memcmp( left, right, len );
		if( lsign && !rsign )
			return -1;
		if( !lsign && rsign )
			return 1;
	}
	// generation zero; lets go with... raw byte compare.
	return memcmp( &left, &right, len );
}

//---------------------------------------------------------------------------


int hangIndexNode( struct memoryStorageIndex* index
	, struct memoryIndexEntry* timelineNode
)
{
	PDATASTACK pdsStack_ = CreateDataStack( sizeof( struct memoryIndexEntry ) );
	PDATASTACK* pdsStack = &pdsStack_;
	struct memoryIndexEntry curNode;
	struct memoryIndexEntry* curNode_;
	struct memoryIndexEntry* root;
	struct memoryIndexEntry* parent;
	uint64_t curindex;

	while( 1 ) {
		root = (struct memoryIndexEntry*)PeekData( pdsStack );
		if( !root )
			break;
		parent = (struct memoryIndexEntry*)PeekDataEx( pdsStack, 1 );
		if( !parent )
			break;

		int d = _os_index_comparison( 1, 0, &timelineNode->data, &parent->data );
		if( parent->data->edge.used.greater.ref.index == root->index ) {
			if( d < 0 ) {
				// this belongs to the lesser side of parent...
				PopData( pdsStack );
				continue;
			}
		}
		if( parent->data->edge.used.lesser.ref.index == root->index ) {
			if( d > 0 ) {
				// this belongs to the greater isde of parent...
				PopData( pdsStack );
				continue;
			}
		}
		break;
	}

	if( !root )
		if( !index->diskData->rootNode.ref.index ) {
			index->diskData->rootNode.ref.index = timelineNode->index;
			index->diskData->rootNode.ref.depth = 0;
			return 1;
		}

	reloadIndexEntry( index, &curNode
		, curindex = (index->diskData->rootNode.ref.index) );
	PushData( pdsStack, &curNode );


	//check = root->tree;
	while( 1 ) {
		int dir;// = root->Compare( node->key, check->key );
		curNode_ = (struct memoryIndexEntry*)PeekData( pdsStack );
#ifdef DEBUG_TIMELINE_AVL
		lprintf( "Compare node %lld  %lld %lld", curNode_->index, curNode_->data, timelineNode->data );;
#endif
		dir = _os_index_comparison( 1, 0, &curNode_->data, &timelineNode->data );

		uint64_t nextIndex;
		//dir = -dir; // test opposite rotation.
		if( dir < 0 ) {
			if( nextIndex = curNode_->data->edge.used.lesser.ref.index ) {
				reloadIndexEntry( index, &curNode
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
				//check = check->lesser;
			}
			else {
				curNode_->data->edge.used.lesser.ref.index = timelineNode->index;
				curNode_->data->edge.used.lesser.ref.depth = 0;
				updateIndexEntry( curNode_, index );
				break;
			}
		}
		else if( dir > 0 )
			if( nextIndex = curNode_->data->edge.used.greater.ref.index ) {
				reloadIndexEntry( index, &curNode
					, curindex = nextIndex );
				PushData( pdsStack, &curNode );
			}
			else {
				curNode_->data->edge.used.greater.ref.index = timelineNode->index;
				curNode_->data->edge.used.greater.ref.depth = 0;
				updateIndexEntry( curNode_, index );
				break;
			}
		else {
			// allow duplicates; but link in as a near node, either left
			// or right... depending on the depth.
			int leftdepth = 0, rightdepth = 0;
			uint64_t nextLesserIndex, nextGreaterIndex;
			if( nextLesserIndex = curNode_->data->edge.used.lesser.ref.index )
				leftdepth = (int)(curNode_->data->edge.used.lesser.ref.depth);
			if( nextGreaterIndex = curNode_->data->edge.used.greater.ref.index )
				rightdepth = (int)(curNode_->data->edge.used.greater.ref.depth);
			if( leftdepth < rightdepth ) {
				if( nextLesserIndex ) {
					reloadIndexEntry( index, &curNode
						, curindex = nextLesserIndex );
					PushData( pdsStack, &curNode );
				}
				else {
					curNode_->data->edge.used.lesser.ref.index = timelineNode->index;
					curNode_->data->edge.used.lesser.ref.depth = 0;
					updateIndexEntry( curNode_, index );
					break;
				}
			}
			else {
				if( nextGreaterIndex ) {
					reloadIndexEntry( index, &curNode
						, curindex = nextGreaterIndex );
					PushData( pdsStack, &curNode );
				}
				else {
					curNode_->data->edge.used.greater.ref.index = timelineNode->index;
					curNode_->data->edge.used.greater.ref.depth = 0;
					updateIndexEntry( curNode_, index );
					break;
				}
			}
		}
	}
	//PushData( &pdsStack, timelineNode );
	_os_index_AVLbalancer( index, pdsStack, timelineNode );
	return 1;
}


