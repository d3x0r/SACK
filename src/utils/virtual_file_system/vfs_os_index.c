

typedef union indexBlockType {
	// 0 is invalid; indexes must subtract 1 to get
	// real index index.
	uint64_t raw;
	struct indexBlockReference {
		uint64_t depth : 6; // 64 depth.
		uint64_t index : 58;
	} ref;
} INDEX_BLOCK_TYPE;


PREFIX_PACKED struct indexHeader {
	INDEX_BLOCK_TYPE first_free_entry;
	INDEX_BLOCK_TYPE crootNode;
	uint64_t unused[6];
} PACKED;

PREFIX_PACKED struct storageIndexNode {
	PREFIX_PACKED union {
		//FPI dirent_fpi;   // FPI on disk
		PREFIX_PACKED struct dataTypeInfo {
			uint32_t type : 1; // text/binary 16 types... 
			uint32_t size : 6; // up to 32 bytes
			uint32_t continued : 1; // next entry also contains data.
			uint32_t unused : 24; // remaining bits for data description.
		}type PACKED;
		uint8_t bigEndianData[4];
	} data PACKED;
	// if dirent_fpi == 0; it's free.
	uint64_t dirent_fpi;

	// if the block is free, cgreater is used as pointer to next free block
	// delete an object can leave free index nodes in the middle of the physical chain.
	INDEX_BLOCK_TYPE lesser;         // FPI/32 within index chain
	INDEX_BLOCK_TYPE greater;        // FPI/32 within index chain + (child depth in this direction AVL)
	
	uint8_t moreBigEndianData[32];
	// 64... 40+24 ... (36+4) + (24) ...
	// 4096/64 = 64.
} PACKED;

struct memoryIndexNode {
	// if dirent_fpi == 0; it's free.
	FPI dirent_fpi;   // FPI on disk
	FPI this_fpi;
	uint64_t index;
	// if the block is free, cgreater is used as pointer to next free block
	// delete an object can leave free index nodes in the middle of the physical chain.
	INDEX_BLOCK_TYPE slesser;         // FPI/32 within index chain
	INDEX_BLOCK_TYPE sgreater;        // FPI/32 within index chain + (child depth in this direction AVL)

	uint8_t data[1]; // C++ requires non 0, non blank.
};


struct storageIndex {
	// name/identifer
	// data type
	struct sack_vfs_file *file;
};

struct vfs_os_storageIndex * CreateStorageIndex( char *hash ) {
	return NULL;
}

struct vfs_os_storageIndex * LoadStorageIndex( char *hash ) {
	return NULL;

}

