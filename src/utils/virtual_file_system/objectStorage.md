

# Object Storage Hash Principles

## Palm Branch Model

Consider a root stick, at the end of the stick, there are a bunch of petals, each of these
is a value to consider in the graph, when the number of values at the end of a branch exceeds
a certain amount, those that all have a similar value create a new stick, and move those leaves
that apply to that stick to the new location; This operation is only the idea, not required, the
existing values could remain where they are, just creating a branch, on insertion, when there
is no room in this directory tables; and putting only new values in the new location(s).

The number of hard branches is `HASH_MODULOUS_BITS` (HMB)  from which `HASH_MODULOUS` (HM) is derived, as
`1<<HASH_MODULOUS_BITS`, which is the number of ways that it will fork
to new hash blocks.  Values stored in the new leaves (could) have the low bits that went into 
indexing the hash removed, or a sliding window-mask based on the level in the tree could be used
to non-destructively read the value for the next hash block.

First search all values at the current node, and on failing
follow the branch that is the group of values that are similar to this value... something like `N`
is the value, and `N & ( (HASH_MODULOUS-1) << (TREE_LEVEL*HASH_MODULOUS_BITS)` and ` >> (TREE_LEVEL*HASH_MODULOUS_BITS)`
at the next level for the next index... (each level before already used the previous HMB)

The Value nodes on the leaf of each branch.  A subset of all values stored in the leaves of 
a hash node 


// this could be like O(log(log(N)))


## Structures and Givens

For a given page size... , these are "BLOCKS" a BLOCKINDEX refers to a 4096 block of memory.
It doesn't have a specific type, just an alignment.  The goal is to pack everything into 
pages of memory.


```
#define BLOCK_SIZE 4096

// BLOCKINDEX  This is some pointer type to another hash node.

typedef uintptr_t BLOCKINDEX;
// uint64_t on x64, uint32 on x86
// can just store a simple integer number.

```

A certain dimension on the branching should be chosen... this is how wide the sub-indexing should be
this could, in theory, be 2, and be a binary tree, with pools of values along the path.

```

#define HASH_MODULUS_BITS  5
#define HASH_MODULUS       ( 1 << HASH_MODULUS_BITS )
// 5 bits is 32 values.
// 1 bits is 2 values; this paged key lookup could just add data to nodes in a binary tree.

```




Hash nodes look like this... 

This is the number of leaf entries per block.... Application 
specific information may be of use here; 

```



// this measures the directory entries.
// the block size, minus ...
//   2 block indexes at the tail, for a pointer to the actual name/data block, and a count of used names.
//   HASH_MODULOUS block indexes - for the loctions of the next hash blocks...
//   (and 3 or 7 bytes padding)
#  define VFS_DIRECTORY_ENTRIES ( \
                ( BLOCK_SIZE - ( 2*sizeof(BLOCKINDEX) + HASH_MODULOUS*sizeof(BLOCKINDEX)) )  \
                / sizeof( struct directory_entry) \
          )


PREFIX_PACKED struct directory_hash_lookup_block
{
	BLOCKINDEX next_block[256];
	struct directory_entry entries[VFS_DIRECTORY_ENTRIES];
	BLOCKINDEX names_first_block;
	uint8_t used_names;
} PACKED;

```








# Find and key's value restoration

```

struct sack_vfs_os_find_info {
	char filename[FILE_NAME_MAXLEN];
	struct sack_vfs_os_volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	size_t filesize;
	CTEXTSTR mask;
#ifdef VIRTUAL_OBJECT_STORE
	char leadin[256];
	int leadinDepth;
	PDATASTACK pds_directories;
	uint64_t ctime;
	uint64_t wtime;
	struct memoryTimelineNode *time;
#else
	BLOCKINDEX this_dir_block;
	size_t thisent;
#endif
};





## Object Storage Cache Regions

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

