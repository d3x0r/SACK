# Virtual File System(s)

| source | description |
|----|----|
| vfs.c | Memory mapped virtual file system  |
| vfs_fs.c | The same system as above, but based on C file IO sort of IO; might be mounted in above VFS |
| vfs_os.c | Object storage file system.  This has an optmized hash/name lookup mechanism for directory |

## Where/What this builds

This is part of the SACK core CMakeLists.txt.  It builds

|What| What is it|
|----|----|
| sack_vfs.module | dynamic link library that can be loaded dynamically to extend the file system interfaces of SACK. Dynamic link library/Shared object. |
| sack_vfs_pp.module | same as above, but is linked to bag++ instead of bag.  Dynamic link library/Shared object.  |
| \*.portable | same as the non-portable version of the program, but builds against static sources instead of SACK's bag\* dynamic libraries.  Will also favor a static C runtime. |
| \*.64/32 | specifies the size of the BLOCKINDEX used in the virtual file systems.  Default was mashine size; this allows 64/32 bit programs to work conversely on 32/64 bit filesystems. |
| sack_vfs_command | A command line utility interface for virtual file systems.  Links dynamically to SACK |
| sack_vfs_extract | It takes files out of a filesystem.  A specifal '.app.config' file can be included in the VFS to specify install/uninstall scripts to run and a default location to install to.  Used to attach a VFS to, and distribute as an installer. |
| sack_vfs_runner | Similar to extract, but instead of taking the files out, runs an application within a specified or attached VFS. |

These are built into `<build>/<build_type>_out/core/bin`  or `<install path>bin` if just the sack core project is built.

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

## Encryption

These all support a light encryption that is essentially a computed one-time-pad overlay.  A block-sized block
is created an dfilled with random data, each block in the file creates a 64 byte random sequence which is 
xor'ed with the original block, giving the encryption on the file as a whole non-repetitive random mask.

### Weakness

If the encryption generating keys are leaked; of course the disk is readable; however, failing that, if
one is able to get muliple copies of the VFS file at various times, the files can be xor-diffed to find
data that has changed; it may or may not reveal the whole value of a changed byte (depending on what the
previous value stored there was).

Statistical attacks; given enough '0' filled blocks, which betrays the original key, it may be possible
to identify sub-patterns within the blocks; because of the repeated merge of sector key over the disk block
key.  Given a sufficiently large amount of data like this, it may be possible to discover the common 4096
byte disk block key by discarding the 'noise' caused by the overlayers.  However, this is only possible
given known data in a file stored behind the encryption.  The attack will reveal a key for a single disk
image, but not help with any other disk.

vfs_os uses [XSWS](https://github.com/d3x0r/SACK/blob/master/src/salty_random_generator/SRG_XSWS_Encryption.md) which
does not have the above mentioned weaknesses.  Any single bit change in the data should result in 50% of the bits
of the whole data block changing.

## VFS Data structures

each 'sector' is preceeded by a single block that tracks the allocated blocks in that 'sector'.  All together
there are N blocks tracked together, using N+1 blocks.

| header  |  data |
|--|--|
|BAT| ................................................ BLOCKS TRACKED BY BAT ......................................... |
|BAT| ................................................ BLOCKS TRACKED BY BAT ......................................... |
|BAT| ................................................ BLOCKS TRACKED BY BAT ......................................... |

There are two types used in VFS; BLOCKINDEX - a 32/64 index by block number (typically), and FPI - a 32/64 index that
gives a literal byte offset.  The size of blocks are controlled with a few defines.


| Symbol | default | meaning |
|---|----|----|
| BLOCK_SIZE_BITS | 12  | size is computed from 1 << BLOCK_SIZE_BITS |
| BLOCK_SIZE | 1 << BLOCK_SIZE_BITS | the byte size of a block |
| BLOCK_SHIFT | BLOCK_SIZE_BITS - (2 or 3 ) | shift by number of BLOCKINDEX in a block;  2 or 3 bits is relative to size of BLOCKINDEX |
| BLOCKS_PER_BAT | (1<<BLOCK_SHIFT) |  How many blocks a single BAT block tracks |
| BLOCKS_PER_SECTOR | (1 + BLOCKS_PER_BAT) | How many blocks totoal in a sector |



The above sector definition might look like

```
struct disk
{
	BLOCKINDEX BAT[BLOCKS_PER_BAT];
	uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];
};
```


```
struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	VFS_DISK_DATATYPE filesize;  // how big the file is
#ifdef VIRTUAL_OBJECT_STORE
	uint64_t timelineEntry;  // when the file was created/last written
#endif
};
```

`vfs_os.c` uses several more structures for tracking it's data... this is just meant to be a high level overview of the
basic principles.

