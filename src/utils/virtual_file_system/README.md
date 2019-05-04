# Virtual File System(s)

| source | description |
|----|----|
| vfs.c | Memory mapped virtual file system  |
| vfs_fs.c | The same system as above, but based on C file IO sort of IO; might be mounted in above VFS |
| vfs_os.c | Object storage file system.  This has an optmized hash/name lookup mechanism for directory |


## VFS General

Sectors are generally 4k blocks (intel page-sized blocks).  Blocks that are linked together form a chain;
and you'll forgive the use of 'block chain'.

## vfs.c (simplest)

It relies purely on a memory mapped interface to the disk data.

This filesystem uses 4096 byte blocks.  Internal block references can be either 32 or 64 bit.   This 
will change the number of blocks per sector.

A 'Sector' is a block followed by either (4096/8) or (4096/4) blocks (depending on the size of the 
block reference.  THe first block tracks the allocation of the blocks following it.  If the entry in 
the BAT (block allocation table) is 0, the block is free, if it's -1 (maxuint), that's the current id of a 
block chain; if it is -2 (maxuint-1), it is the end of allocated blocks.

THe first allocated block is the directory;  It alwasys starts are block 0.

The next allocated block are the names of entries in the directory.

All other blocks get allocated for files.

The directory entry has an offset of the name in the name block chain, the size of the file, and the first
block of that file.  There are no date/times stored with the file information.

## vfs_fs.c - Same as the old

vfs_fs.c is the same disk layout as vfs.c, but instead uses file system methods to read/write the data into
the file (open/read/seek/write/close).  Ths filesystem could be mounted in the above file syste, but 
not the other way around.

## vfs_os.c - Revised Object Storage

This looks at files more like arbitrary blobs of data.  This tracks data blobs that have unique identifiers
using a combination hash-table/short binary search to find the file information.  

This tracks creation and write time of the file in addition to name, size and starting data.  THe creation
and update times of files are indexed, so you can find files after a certain time.

It also has the ability to perform a light Proof-of-Work algorithm storing the nonce with the file data and
resulting with a hash of the file that fits the PoW result criteria.


