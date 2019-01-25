/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#if 1
#  include <stdhdrs.h>
#  include <ctype.h> // tolower on linux
#  include <filesys.h>
#  include <procreg.h>
#  include <salty_generator.h>
#  include <sack_vfs.h>
#  include <sqlgetoption.h>
#else
#  include <sack.h>
#  include <ctype.h> // tolower on linux
//#include <filesys.h>
//#include <procreg.h>
//#include <salty_generator.h>
//#include <sack_vfs.h>
//#include <sqlgetoption.h>
#endif

SACK_VFS_NAMESPACE
//#define PARANOID_INIT

//#define DEBUG_TRACE_LOG

#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#endif

#define MMAP_BASED_VFS
#include "vfs_internal.h"

static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
} l;
#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)
#define EODMARK   (1)
#define GFB_INIT_NONE   0
#define GFB_INIT_DIRENT 1
#define GFB_INIT_NAMES  2
static BLOCKINDEX GetFreeBlock( struct volume *vol, int init );
static struct directory_entry * ScanDirectory( struct volume *vol, const char * filename, struct directory_entry *dirkey, int path_match );

static char mytolower( int c ) {	if( c == '\\' ) return '/'; return tolower( c ); }


static int  PathCaseCmpEx ( CTEXTSTR s1, CTEXTSTR s2, size_t maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && ( (s1[0]=='/'&&s2[0]=='\\')||(s1[0]=='\\'&&s2[0]=='/')||
									 (((s1[0] >='a' && s1[0] <='z' )?s1[0]-('a'-'A'):s1[0])
									 == ((s2[0] >='a' && s2[0] <='z' )?s2[0]-('a'-'A'):s2[0])) ) && maxlen;
		  s1++, s2++, maxlen-- );
	if( maxlen )
		return tolower(s1[0]) - tolower(s2[0]);
	return 0;
}

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int MaskStrCmp( struct volume *vol, const char * filename, FPI name_offset, int path_match ) {
	if( vol->key ) {
		int c;
		while(  ( c = ( ((uint8_t*)vol->disk)[name_offset] ^ vol->usekey[BC(NAMES)][name_offset&BLOCK_MASK] ) )
			  && filename[0] ) {
			int del = mytolower(filename[0]) - mytolower(c);
			if( ( filename[0] == '/' && c == '\\' )
			    || ( filename[0] == '\\' && c == '/' ) )
				del = 0;
			if( del ) return del;
			filename++;
			name_offset++;
			if( path_match && !filename[0] ) {
				c = ( ((uint8_t*)vol->disk)[name_offset] ^ vol->usekey[BC(NAMES)][name_offset&BLOCK_MASK] );
				if( c == '/' || c == '\\' ) return 0;
			}
		}
		// c will be 0 or filename will be 0...
		if( path_match ) return 1;
		return filename[0] - c;
	} else {
		//LoG( "doesn't volume always have a key?" );
		if( path_match ) {
			size_t l;
			int r = PathCaseCmpEx( filename, (const char *)(((uint8_t*)vol->disk) + name_offset), l = strlen( filename ) );
			if( !r )
				if( ((const char *)(((uint8_t*)vol->disk) + name_offset))[l] == '/' || ((const char *)(((uint8_t*)vol->disk) + name_offset))[l] == '\\' )
					return 0;
				else
					return 1;
			return r;
		}
		else
			return PathCaseCmpEx( filename, (const char *)(((uint8_t*)vol->disk) + name_offset), strlen(filename) );
	}
}

#ifdef DEBUG_TRACE_LOG
static void MaskStrCpy( char *output, size_t outlen, struct volume *vol, FPI name_offset ) {
	if( vol->key ) {
		int c;
		FPI name_start = name_offset;
		while(  ( c = ( ((uint8_t*)vol->disk)[name_offset] ^ vol->usekey[BC(NAMES)][name_offset&BLOCK_MASK] ) ) ) {
			if( ( name_offset - name_start ) < outlen )
				output[name_offset-name_start] = c;
			name_offset++;
		}
		if( ( name_offset - name_start ) < outlen )
			output[name_offset-name_start] = 0;
		else
			output[outlen-1] = 0;
	} else {
		//LoG( "doesn't volume always have a key?" );
		StrCpyEx( output, (const char *)(((uint8_t*)vol->disk) + name_offset), outlen );
	}
}
#endif

static void ExtendBlockChain( struct sack_vfs_file *file ) {
	int newSize = ( file->blockChainAvail ) * 2 + 1;
	file->blockChain = (BLOCKINDEX*)Reallocate( file->blockChain, newSize * sizeof( BLOCKINDEX ) );
#ifdef _DEBUG
	// debug
	memset( file->blockChain + file->blockChainAvail, 0, (newSize - file->blockChainAvail ) * sizeof(BLOCKINDEX) );
#endif
	file->blockChainAvail = newSize;
	
}

static void SetBlockChain( struct sack_vfs_file *file, FPI fpi, BLOCKINDEX newBlock ) {
	FPI fileBlock = fpi >> BLOCK_SIZE_BITS;
#ifdef _DEBUG
	if( !newBlock ) DebugBreak();
#endif
	while( (fileBlock) >= file->blockChainAvail ) {
		ExtendBlockChain( file );
	}
	if( fileBlock >= file->blockChainLength )
		file->blockChainLength = (unsigned int)(fileBlock + 1);
	//_lprintf(DBG_RELAY)( "setting %d to %d", (int)fileBlock, (int)newBlock );
	if( file->blockChain[fileBlock] ) {
		if( file->blockChain[fileBlock] == newBlock ) {
			return;
		}
	}
	file->blockChain[fileBlock] = newBlock;
}


static enum block_cache_entries UpdateSegmentKey( struct volume *vol, enum block_cache_entries cache_idx, BLOCKINDEX segment )
{
	if( !vol->key ) {
		vol->segment[cache_idx] = segment;
		return cache_idx;
	}
	if( cache_idx == BC(FILE) ) {
		int n, m;
		int nLeast;
		uint8_t next = 0;
		for( n = 0; n < (BC(FILE_LAST) - BC(FILE)); n++ ) {
			if( vol->segment[cache_idx + n] == segment ) {
				cache_idx = (enum block_cache_entries)((cache_idx)+n);
				for( m = 0; m < (BC(FILE_LAST) - BC(FILE)); m++ ) {
					if( !vol->fileCacheAge[m] ) break;
					if( vol->fileCacheAge[m] > vol->fileCacheAge[n] )
						vol->fileCacheAge[m]--;
				}
				vol->fileCacheAge[n] = m;
				break;
			}
			if( !vol->fileCacheAge[n] ) {
				cache_idx = (enum block_cache_entries)((cache_idx)+n);
				for( m = 0; m < (BC(FILE_LAST) - BC(FILE)); m++ ) {
					if( !vol->fileCacheAge[m] ) break;
					if( vol->fileCacheAge[m] >( n + 1 ) )
						vol->fileCacheAge[m]--;
				}
				vol->fileCacheAge[n] = n + 1;
				break;
			}
			if( vol->fileCacheAge[n] == 1 ) nLeast = n;
		}
		if( n == (BC(FILE_LAST) - BC(FILE)) ) {

			for( n = 0; n < (BC(FILE_LAST) - BC(FILE)); n++ ) {
				vol->fileCacheAge[n]--;
			}
			vol->fileCacheAge[nLeast] = (BC(FILE_LAST) - BC(FILE));
			cache_idx = (enum block_cache_entries)(BC(FILE) + nLeast);
		}
	}
	vol->segment[cache_idx] = segment;
	if( vol->segment[cache_idx] == vol->_segment[cache_idx] )
		return cache_idx;
	SRG_ResetEntropy( vol->entropy );
	vol->_segment[cache_idx] = vol->segment[cache_idx];
	vol->curseg = cache_idx;  // so we know which 'segment[idx]' to use.
	SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->segkey, SHORTKEY_LENGTH * 8 );
	{
		int n;
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[cache_idx];
		uint64_t* volkey = (uint64_t*)vol->key;
		uint64_t* segkey = (uint64_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey += 2;
			volkey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[cache_idx];
		uint32_t* volkey = (uint32_t*)vol->key;
		uint32_t* segkey = (uint32_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey[2] = volkey[2] ^ (segkey[2]);
			usekey[3] = volkey[3] ^ (segkey[3]);
			usekey += 4;
			volkey += 4;
		}
#endif
	}
	return cache_idx;
}

static LOGICAL ValidateBAT( struct volume *vol ) {
	BLOCKINDEX first_slab = 0;
	BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
	BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
	BLOCKINDEX n;
	if( vol->key ) {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT; 
			BLOCKINDEX *blockKey; 
			BAT = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			blockKey = ((BLOCKINDEX*)vol->usekey[BC(BAT)]);
			UpdateSegmentKey( vol, BC(BAT), n + 1 );

			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[0] ^ blockKey[0];
				BAT++; blockKey++;
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n+m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	} else {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				BLOCKINDEX block = BAT[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n + m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	if( !ScanDirectory( vol, NULL, NULL, 0 ) ) return FALSE;
	return TRUE;
}

//-------------------------------------------------------
// function to process a currently loaded program to get the
// data offset at the end of the executable.


static POINTER GetExtraData( POINTER block )
{
#ifdef WIN32
#  define Seek(a,b) (((uintptr_t)a)+(b))
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
			LoG( "Basic signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE ) {
			LoG( "Basic NT signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				LoG( "Optional header signature is incorrect..." );
				return NULL;
			}
		}
		{
			int n;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].PointerToRawData) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			dwSize += (BLOCK_SIZE*2)-1; // pad 1 full block, plus all but 1 byte of a full block(round up)
			dwSize &= ~(BLOCK_SIZE-1); // mask off the low bits; floor result to block boundary
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
#  undef Seek
#else
	// need to get elf size...
	return 0;
#endif
}

static void AddSalt2( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct datatype { void* start; size_t length; } *data = (struct datatype*)psv;
	
	(*salt_size) = data->length;
	(*salt) = (POINTER)data->start;
	// only need to make one pass of it....
	data->length = 0;
	data->start = NULL;
}
const uint8_t *sack_vfs_get_signature2( POINTER disk, POINTER diskReal ) {
	if( disk != diskReal ) {
		static uint8_t usekey[BLOCK_SIZE];
		static struct random_context *entropy;
		static struct datatype { void* start; size_t length; } data;
		data.start = diskReal;
		data.length = ((uintptr_t)disk - (uintptr_t)diskReal) - BLOCK_SIZE;
		if( !entropy ) entropy = SRG_CreateEntropy2( AddSalt2, (uintptr_t)&data );
		SRG_ResetEntropy( entropy );
		SRG_GetEntropyBuffer( entropy, (uint32_t*)usekey, BLOCK_SIZE*CHAR_BIT );
		return usekey;
	}
	return NULL;
}


// add some space to the volume....
static LOGICAL ExpandVolume( struct volume *vol ) {
	LOGICAL created;
	LOGICAL path_checked = FALSE;
	struct disk* new_disk;
	BLOCKINDEX oldsize = (BLOCKINDEX)vol->dwSize;
	if( vol->read_only ) return TRUE;
	if( !vol->dwSize ) {
		{
			char *tmp = StrDup( vol->volname );
			char *dir = (char*)pathrchr( tmp );
			if( dir ) {
				dir[0] = 0;
				if( !IsPath( tmp ) ) MakePath( tmp );
			}
			Deallocate( char*, tmp );
		}
		new_disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
		if( new_disk && vol->dwSize ) {
			CloseSpace( vol->diskReal );
			vol->diskReal = new_disk;
#ifdef WIN32
			// elf has a different signature to check for .so extended data...
			struct disk *actual_disk;
			if( ((char*)new_disk)[0] == 'M' && ((char*)new_disk)[1] == 'Z' ) {
				actual_disk = (struct disk*)GetExtraData( new_disk );
				if( actual_disk ) {
					if( ( ( (uintptr_t)actual_disk - (uintptr_t)new_disk ) < vol->dwSize ) ) {
						const uint8_t *sig = sack_vfs_get_signature2( (POINTER)((uintptr_t)actual_disk-BLOCK_SIZE), new_disk );
						if( memcmp( sig, (POINTER)(((uintptr_t)actual_disk)-BLOCK_SIZE), BLOCK_SIZE ) ) {
							lprintf( "Signature failed comparison; the core has changed since it was attached" );
							CloseSpace( vol->diskReal );
							vol->diskReal = NULL;
							vol->dwSize = 0;
							return FALSE;
						}
						vol->dwSize -= ((uintptr_t)actual_disk - (uintptr_t)new_disk);
						new_disk = actual_disk;
					} else {
						lprintf( "Signature failed comparison; the core is not attached to anything." );
						CloseSpace( vol->diskReal );
						vol->diskReal = NULL;
						vol->dwSize = 0;
						return FALSE;
					}
				}
			}
#endif
			vol->disk = new_disk;
			if( created && vol->disk == vol->diskReal ) {
				enum block_cache_entries cache = BC(DIRECTORY);
				struct directory_entry *next_entries = BTSEEK( struct directory_entry *, vol, 0, cache );
				struct directory_entry *entkey = (vol->key) ? ((struct directory_entry *)vol->usekey[BC(DIRECTORY)]) : &l.zero_entkey;
				// initialize directory list.
				((struct directory_entry*)(((uintptr_t)vol->disk) + BLOCK_SIZE))->first_block = EODMARK ^ entkey->first_block;

				// initialize first BAT block.
				cache = BC(BAT);
				TSEEK( BLOCKINDEX*, vol, 0, cache );
				((BLOCKINDEX*)(((uintptr_t)vol->disk) + 0))[0] = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[cache])[0];
			}
			return TRUE;
		}
		else {
			// really this is bad anyway.
			if( new_disk )
				created = 1; // zero size result?, but with memory
		}
	}

	if( oldsize ) CloseSpace( vol->diskReal );

	vol->dwSize += ((uintptr_t)vol->disk - (uintptr_t)vol->diskReal);
	// a BAT plus the sectors it references... ( BLOCKS_PER_BAT + 1 ) * BLOCK_SIZE
	vol->dwSize += BLOCKS_PER_SECTOR*BLOCK_SIZE;
	new_disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
	LoG( "created expanded volume: %p from %p size:%" _size_f, new_disk, vol->disk, vol->dwSize );
	if( new_disk && new_disk != vol->disk ) {
		INDEX idx;
		struct sack_vfs_file *file;
		CloseSpace( vol->diskReal );
		vol->diskReal = new_disk;
#ifdef WIN32
		// elf has a different signature to check for .so extended data...
		{
			struct disk *actual_disk;
			if( ((char*)new_disk)[0] == 'M' && ((char*)new_disk)[1] == 'Z' ) {
				actual_disk = (struct disk*)GetExtraData( new_disk );
				if( actual_disk ) {
					const uint8_t *sig = sack_vfs_get_signature2( (POINTER)((uintptr_t)actual_disk-BLOCK_SIZE), new_disk );
					if( memcmp( sig, (POINTER)(((uintptr_t)actual_disk)-BLOCK_SIZE), BLOCK_SIZE ) ) {
						lprintf( "Signature failed comparison; the core has changed since it was attached" );
						CloseSpace( vol->diskReal );
						vol->diskReal = NULL;
						vol->dwSize = 0;
						return FALSE;
					}
					vol->dwSize -= ((uintptr_t)actual_disk - (uintptr_t)new_disk);
					new_disk = actual_disk;
				}
			}
		}
#endif
		LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file ) {
			file->entry = (struct directory_entry*)((uintptr_t)file->entry - (uintptr_t)vol->disk + (uintptr_t)new_disk);
		}
		vol->disk = new_disk;
	}
	if( vol->key ) {
		BLOCKINDEX first_slab = oldsize / ( BLOCK_SIZE );
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		BLOCKINDEX n;
		for( n = first_slab; n < slab; n++  ) {
			//vol->segment[BC(BAT)] = n + 1;
			if( ( n % (BLOCKS_PER_SECTOR) ) == 0 )	 UpdateSegmentKey( vol, BC(BAT), n + 1 );
#ifdef PARANOID_INIT
			else SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->usekey[BC(BAT)], BLOCK_SIZE * 8 );
#else
			else continue;
#endif
			//memcpy( ((uint8_t*)vol->disk) + n * BLOCK_SIZE, vol->usekey[BC(BAT)], BLOCK_SIZE );
			((BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE))[0] = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC(BAT)])[0];
			memset( ((BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE))+1, 0, BLOCK_SIZE - sizeof( BLOCKINDEX ) );
		}
	}
	else if( !oldsize )  {
		memset( vol->disk, 0, vol->dwSize );
	} else if( oldsize )  {
		memset( ((uint8_t*)vol->disk) + oldsize, 0, vol->dwSize - oldsize );
	}

	if( !oldsize ) {
		// can't recover dirents and nameents dynamically; so just assume
		// use the GetFreeBlock because it will update encypted
		//vol->disk->BAT[0] = EOFBLOCK;  // allocate 1 directory entry block
		//vol->disk->BAT[1] = EOFBLOCK;  // allocate 1 name block
		if( created && vol->disk == vol->diskReal ) {
			UpdateSegmentKey( vol, BC(BAT), 1 );
			((BLOCKINDEX*)(((uintptr_t)vol->disk) + 0))[0] = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC(BAT)])[0];
		}
		/* vol->dirents = */GetFreeBlock( vol, GFB_INIT_DIRENT );
		/* vol->nameents = */GetFreeBlock( vol, GFB_INIT_NAMES );
	}
	return TRUE;
}

// shared with fuse module
uintptr_t vfs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries *cache_index ) {
	while( offset >= vol->dwSize ) if( !ExpandVolume( vol ) ) return 0;
	if( vol->key ) {
		BLOCKINDEX seg = ( offset / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index[0]] ) {
			//vol->segment[cache_index] = seg;
			cache_index[0] = UpdateSegmentKey( vol, cache_index[0], seg );
		}
	}
	return ((uintptr_t)vol->disk) + (uintptr_t)offset;
}

// shared with fuse module
uintptr_t vfs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index ) {
	BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + ( block & (BLOCKS_PER_BAT-1) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !ExpandVolume( vol ) ) return 0;
	if( vol->key ) {
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index[0]] ) {
			//vol->segment[cache_index] = seg;
			if( (cache_index[0] == BC(FILE))
				&& (seg < 3) ) {
				lprintf( "CRITICAL FAILURE, SEEK OUT OF DISK %d", seg );
				(*(int*)0) = 0;
			}
			cache_index[0] = UpdateSegmentKey( vol, cache_index[0], seg );
		}
	}
	return ((uintptr_t)vol->disk) + (uintptr_t)b;
}

static BLOCKINDEX GetFreeBlock( struct volume *vol, int init )
{
	size_t n;
	unsigned int b = 0;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	BLOCKINDEX *blockKey;
	BLOCKINDEX check_val;
	if( vol->pdlFreeBlocks->Cnt ) {
		BLOCKINDEX newblock = ((BLOCKINDEX*)GetDataItem( &vol->pdlFreeBlocks, vol->pdlFreeBlocks->Cnt - 1 ))[0];
		check_val = 0;
		b = (unsigned int)(newblock / BLOCKS_PER_BAT);
		n = newblock % BLOCKS_PER_BAT;
		vol->pdlFreeBlocks->Cnt--;
	}
	else {
		check_val = EOBBLOCK;
		b = (unsigned int)(vol->lastBatBlock / BLOCKS_PER_BAT);
		n = vol->lastBatBlock % BLOCKS_PER_BAT;
	}
	//lprintf( "check, start, b, n %d %d %d %d", (int)check_val, (int) vol->lastBatBlock, (int)b, (int)n );
	current_BAT = TSEEK( BLOCKINDEX*, vol, b*BLOCKS_PER_SECTOR*BLOCK_SIZE, cache ) + n;
	blockKey = ((BLOCKINDEX*)vol->usekey[cache]) + n;

	if( !current_BAT ) return 0;

	current_BAT[0] = EOFBLOCK ^ blockKey[0];

	if( (check_val == EOBBLOCK) ) {
		if( n < (BLOCKS_PER_BAT - 1) ) {
			current_BAT[1] = EOBBLOCK ^ blockKey[1];
			vol->lastBatBlock++;
		}
		else {
			cache = BC( BAT );
			current_BAT = TSEEK( BLOCKINDEX*, vol, (b + 1) * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			current_BAT[0] = EOBBLOCK ^ blockKey[0];
			vol->lastBatBlock = (b + 1) * BLOCKS_PER_BAT;
			//lprintf( "Set last block....%d", (int)vol->lastBatBlock );
		}
	}

	if( init ) {
		enum block_cache_entries cache;
		cache = UpdateSegmentKey( vol, BC( FILE ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 );
		while( ((vol->segment[cache] - 1)*BLOCK_SIZE) > vol->dwSize ) {
			LoG( "looping to get a size %d", ((vol->segment[cache] - 1)*BLOCK_SIZE) );
			if( !ExpandVolume( vol ) ) return 0;
		}
		if( init == GFB_INIT_DIRENT )
			((struct directory_entry*)(((uint8_t*)vol->disk) + (vol->segment[cache] - 1) * BLOCK_SIZE))[0].first_block = EODMARK ^ ((struct directory_entry*)vol->usekey[cache])->first_block;
		else if( init == GFB_INIT_NAMES )
			((char*)(((uint8_t*)vol->disk) + (vol->segment[cache] - 1) * BLOCK_SIZE))[0] = ((char*)vol->usekey[cache])[0];
		//else
		//	memcpy( ((uint8_t*)vol->disk) + (vol->segment[cache]-1) * BLOCK_SIZE, vol->usekey[cache], BLOCK_SIZE );
	}
	//lprintf( "Return block:%d   %d  %d", (int)(b*BLOCKS_PER_BAT + n), (int)b, (int)n );
	return b * BLOCKS_PER_BAT + n;

}

static BLOCKINDEX vfs_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand ) {
	BLOCKINDEX sector = block / BLOCKS_PER_BAT;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
	BLOCKINDEX seg;
	BLOCKINDEX check_val = (this_BAT[block & (BLOCKS_PER_BAT-1)]);
	if( !this_BAT ) return 0; // if this passes, later ones will also.

	seg = ( ((uintptr_t)this_BAT - (uintptr_t)vol->disk) / BLOCK_SIZE ) + 1;
	if( seg != vol->segment[cache] ) {
		//vol->segment[BC(BAT)] = seg;
		UpdateSegmentKey( vol, cache, seg );
	}
	check_val ^= ((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)];
	if( check_val == EOBBLOCK ) {
		lprintf( "the file itself should never get a EOBBLOCK in it. %d  %d", (int)block, (int)sector );
		(*(int*)0) = 0;
		// the file itself should never get a EOBBLOCK in it.
		//(this_BAT[block & (BLOCKS_PER_BAT-1)]) = EOFBLOCK^((BLOCKINDEX*)vol->usekey[BC(BAT)])[block & (BLOCKS_PER_BAT-1)];
		//(this_BAT[1+block & (BLOCKS_PER_BAT-1)]) = EOBBLOCK^((BLOCKINDEX*)vol->usekey[BC(BAT)])[1+block & (BLOCKS_PER_BAT-1)];
	}
	if( check_val == EOFBLOCK ) {
		if( expand ) {
			BLOCKINDEX key;
			check_val = GetFreeBlock( vol, init );
			// free block might have expanded...
			cache = BC( BAT );
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), cache );
			key = vol->key ? ((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT - 1)] : 0;
			if( !this_BAT ) return 0;
#ifdef _DEBUG
			if( !block ) DebugBreak();
#endif
			// segment could already be set from the GetFreeBlock...
			this_BAT[block & (BLOCKS_PER_BAT-1)] = check_val ^ key;
		}
	}
	return check_val;
}

static void AddSalt( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct volume *vol = (struct volume *)psv;
	if( vol->sigsalt ) {
		(*salt_size) = vol->sigkeyLength;
		(*salt) = (POINTER)vol->sigsalt;
		vol->sigsalt = NULL;
	}
	else if( vol->datakey ) {
		(*salt_size) = BLOCK_SIZE;
		(*salt) = (POINTER)vol->datakey;
		vol->datakey = NULL;
	}
	else if( vol->userkey ) {
		(*salt_size) = StrLen( vol->userkey );
		(*salt) = (POINTER)vol->userkey;
		vol->userkey = NULL;
	}
	else if( vol->devkey ) {
		(*salt_size) = StrLen( vol->devkey );
		(*salt) = (POINTER)vol->devkey;
		vol->devkey = NULL;
	}
	else if( vol->segment[vol->curseg] ) {
		BLOCKINDEX sector = vol->segment[vol->curseg];
		switch( vol->clusterKeyVersion ) {
		case 0:
			( *salt_size ) = sizeof( vol->segment[vol->curseg] );
			( *salt ) = &vol->segment[vol->curseg];
			break;
		case 1:
			memcpy( vol->tmpSalt, vol->key, 16 );
			vol->tmpSalt[sector & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[0];
			vol->tmpSalt[( sector >> 4 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[1];
			vol->tmpSalt[( sector >> 8 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[2];
			vol->tmpSalt[( sector >> 12 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[3];

			( (BLOCKINDEX*)vol->tmpSalt )[0] ^= sector;
			( (BLOCKINDEX*)vol->tmpSalt )[1] ^= sector;
			( *salt_size ) = 12;// sizeof( vol->segment[vol->curseg] );
			( *salt ) = vol->tmpSalt;
			break;
		}
	}
	else
		(*salt_size) = 0;
}

static void AssignKey( struct volume *vol, const char *key1, const char *key2 )
{
	vol->userkey = key1;
	vol->devkey = key2;
	if( key1 || key2 )
	{
		uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * BC(COUNT) + BLOCK_SIZE + SHORTKEY_LENGTH;
		int n;
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( AddSalt, (uintptr_t)vol );
		else
			SRG_ResetEntropy( vol->entropy );
		vol->key = (uint8_t*)OpenSpace( NULL, NULL, &size );
		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey[n] = vol->key + (n + 1) * BLOCK_SIZE;
			vol->segment[n] = 0;
		}
		vol->segkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1);
		vol->sigkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1) + SHORTKEY_LENGTH;
		vol->curseg = BC(DIRECTORY);
		vol->segment[BC(DIRECTORY)] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, BLOCK_SIZE * 8 );
	}
	else {
		int n;
		for( n = 0; n < BC(COUNT); n++ )
			vol->usekey[n] = l.zerokey;
		vol->segkey = l.zerokey;
		vol->sigkey = l.zerokey;
		vol->key = NULL;
	}
}

struct volume *sack_vfs_load_volume( const char * filepath )
{
	struct volume *vol = New( struct volume );
	memset( vol, 0, sizeof( struct volume ) );
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->volname = SaveText( filepath );
	AssignKey( vol, NULL, NULL );
	if( !ExpandVolume( vol ) || !ValidateBAT( vol ) ) { Deallocate( struct volume*, vol ); return NULL; }
	return vol;
}

struct volume *sack_vfs_load_crypt_volume( const char * filepath, uintptr_t version, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	if( !version ) version = 2;
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->clusterKeyVersion = version - 1;
	vol->volname = SaveText( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	AssignKey( vol, userkey, devkey );
	if( !ExpandVolume( vol ) || !ValidateBAT( vol ) ) { sack_vfs_unload_volume( vol ); return NULL; }
	return vol;
}

struct volume *sack_vfs_use_crypt_volume( POINTER memory, size_t sz, uintptr_t version, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	vol->read_only = 1;
	AssignKey( vol, userkey, devkey );
	if( !version ) version = 2;
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->clusterKeyVersion = version - 1;
	vol->external_memory = TRUE;
	vol->diskReal = (struct disk*)memory;
	vol->dwSize = sz;
#ifdef WIN32
	// elf has a different signature to check for .so extended data...
	struct disk *actual_disk;
	if( ((char*)memory)[0] == 'M' && ((char*)memory)[1] == 'Z' ) {
		actual_disk = (struct disk*)GetExtraData( memory );
		if( actual_disk ) {
			if( ( ( (uintptr_t)actual_disk - (uintptr_t)memory ) < vol->dwSize ) ) {
				const uint8_t *sig = sack_vfs_get_signature2( (POINTER)((uintptr_t)actual_disk-BLOCK_SIZE), memory );
				if( memcmp( sig, (POINTER)(((uintptr_t)actual_disk)-BLOCK_SIZE), BLOCK_SIZE ) ) {
					lprintf( "Signature failed comparison; the core has changed since it was attached" );
					vol->diskReal = NULL;
					vol->dwSize = 0;
					sack_vfs_unload_volume( vol );
					return FALSE;
				}
				vol->dwSize -= ((uintptr_t)actual_disk - (uintptr_t)memory);
				memory = (POINTER)actual_disk;
			} else {
				lprintf( "Signature failed comparison; the core is not attached to anything." );
				vol->diskReal = NULL;
				vol->disk = NULL;
				vol->dwSize = 0;
				sack_vfs_unload_volume( vol );
				return NULL;
			}
		}
	}
#endif
	vol->disk = (struct disk*)memory;

	if( !ValidateBAT( vol ) ) { sack_vfs_unload_volume( vol );  return NULL; }
	return vol;
}

void sack_vfs_unload_volume( struct volume * vol ) {
	INDEX idx;
	struct sack_vfs_file *file;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		break;
	if( file ) {
		vol->closed = TRUE;
		return;
	}
	DeleteListEx( &vol->files DBG_SRC );
	DeleteDataList( &vol->pdlFreeBlocks );
	if( !vol->external_memory )	CloseSpace( vol->diskReal );
	if( vol->key ) {
		Deallocate( uint8_t*, vol->key );
		SRG_DestroyEntropy( &vol->entropy );
	}
	Deallocate( struct volume*, vol );
}

void sack_vfs_shrink_volume( struct volume * vol ) {
	size_t n;
	unsigned int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	unsigned int last_bat = 0;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		BLOCKINDEX *blockKey;
		blockKey = (BLOCKINDEX*)vol->usekey[cache];
		for( n = 0; n < BLOCKS_PER_BAT; n++ ) {
			check_val = *(current_BAT++);
			if( vol->key )	check_val ^= *(blockKey++);
			if( check_val ) {
				last_bat = b;
				last_block = n;
			}
		}
		b++;
		if( b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) < vol->dwSize ) {
			current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
		} else
			break;
	}while( 1 );

	Deallocate( struct disk *, vol->diskReal );
	SetFileLength( vol->volname,
			((uintptr_t)vol->disk - (uintptr_t)vol->diskReal) +
			(size_t)(last_bat * BLOCKS_PER_SECTOR * BLOCK_SIZE + ( last_block + 1 + 1 )* BLOCK_SIZE) );
	// setting 0 size will cause expand to do an initial open instead of expanding
	vol->dwSize = 0;
}

static void mask_block( struct volume *vol, size_t n ) {
	BLOCKINDEX b = ( 1 + (n >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR) + (n & (BLOCKS_PER_BAT - 1)));
	UpdateSegmentKey( vol, BC(DATAKEY), b + 1 );
	{
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[BC(DATAKEY)];
		uint64_t* block = (uint64_t*)(((uintptr_t)vol->disk) + b * BLOCK_SIZE );
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block += 2; usekey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[BC(DATAKEY)];
		uint32_t* block = (uint32_t*)(((uintptr_t)vol->disk) + b * BLOCK_SIZE );
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block[2] = block[2] ^ usekey[2];
			block[3] = block[3] ^ usekey[3];
			block += 4; usekey += 4;
		}
#endif
	}
}

LOGICAL sack_vfs_decrypt_volume( struct volume *vol )
{
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( !vol->key ) { vol->lock = 0; return FALSE; } // volume is already decrypted, cannot remove key
	{
		enum block_cache_entries cache = BC(BAT);
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey; 
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				block[0] ^= blockKey[0];
				if( block[0] == EOBBLOCK ) break;
				else if( block[0] ) mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block++;
				blockKey++;
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	AssignKey( vol, NULL, NULL );
	vol->lock = 0;
	return TRUE;
}

LOGICAL sack_vfs_encrypt_volume( struct volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 ) {
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( vol->key ) { vol->lock = 0; return FALSE; } // volume already has a key, cannot apply new key
	if( !version ) version = 2;
	vol->clusterKeyVersion = version-1;
	AssignKey( vol, key1, key2 );
	{
		int done;
		size_t n;
		enum block_cache_entries cache = BC(BAT);
		BLOCKINDEX slab = (vol->dwSize + (BLOCKS_PER_SECTOR*BLOCK_SIZE-1)) / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		done = 0;
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey;
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);

			//vol->segment[BC(BAT)] = n + 1;
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				if( block[0] == EOBBLOCK ) done = TRUE;
				else if( block[0] ) mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block[0] ^= blockKey[0];
				if( done ) break;
				block++;
				blockKey++;
			}
			if( done ) break;
		}
	}
	vol->lock = 0;
	return TRUE;
}

const char *sack_vfs_get_signature( struct volume *vol ) {
	static char signature[257];
	static const char *output = "0123456789ABCDEF";
	if( !vol )
		return NULL;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	{
		static BLOCKINDEX datakey[BLOCKS_PER_BAT];
		uint8_t* usekey = vol->key?vol->usekey[BC(DATAKEY)]:l.zerokey;
		signature[256] = 0;
		memset( datakey, 0, sizeof( datakey ) );
		{
			{
				size_t n;
				BLOCKINDEX this_dir_block = 0;
				BLOCKINDEX next_dir_block;
				BLOCKINDEX *next_entries;
				do {
					enum block_cache_entries cache = BC(DATAKEY);
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, cache );
					for( n = 0; n < BLOCKS_PER_BAT; n++ )
						datakey[n] ^= next_entries[n] ^ ((BLOCKINDEX*)(((uint8_t*)usekey)))[n];
					
					next_dir_block = vfs_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, FALSE );
#ifdef _DEBUG
					if( this_dir_block == next_dir_block )
						DebugBreak();
					if( next_dir_block == 0 )
						DebugBreak();
#endif
					this_dir_block = next_dir_block;
				}
				while( next_dir_block != EOFBLOCK );
			}
		}
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( AddSalt, (uintptr_t)vol );
		SRG_ResetEntropy( vol->entropy );
		vol->curseg = BC(DIRECTORY);
		vol->segment[vol->curseg] = 0;
		vol->datakey = (const char *)datakey;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)usekey, 128 * 8 );
		{
			int n;
			for( n = 0; n < 128; n++ ) {
				signature[n*2] = output[( usekey[n] >> 4 ) & 0xF];
				signature[n*2+1] = output[usekey[n] & 0xF];
			}
		}
	}
	vol->lock = 0;
	return signature;
}

struct directory_entry * ScanDirectory( struct volume *vol, const char * filename, struct directory_entry *dirkey, int path_match ) {
	size_t n;
	BLOCKINDEX this_dir_block = 0;
	BLOCKINDEX next_dir_block;
	struct directory_entry *next_entries;
	if( filename && filename[0] == '.' && filename[1] == '/' ) filename += 2;
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, cache );
		for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			BLOCKINDEX bi;
			enum block_cache_entries name_cache = BC(NAMES);
			struct directory_entry *entkey = ( vol->key)?((struct directory_entry *)vol->usekey[BC(DIRECTORY)])+n:&l.zero_entkey;
			//const char * testname;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;

			if( filename && !name_ofs )	return NULL; // done.
			//LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
			//   , n, name_ofs
			//   , next_entries[n].name_offset ^ entkey->name_offset
			//   , next_entries[n].first_block ^ entkey->first_block
			//   , filename );
			bi = next_entries[n].first_block ^ entkey->first_block;
			// if file is deleted; don't check it's name.
			if( !bi ) continue;
			// if file is end of directory, done sanning.
			if( bi == EODMARK ) return filename?NULL:((struct directory_entry*)1); // done.
			if( name_ofs > vol->dwSize ) { return NULL; }
			//testname =
			if( filename ) {
				TSEEK( const char *, vol, name_ofs, name_cache ); // have to do the seek to the name block otherwise it might not be loaded.
				if( MaskStrCmp( vol, filename, name_ofs, path_match ) == 0 ) {
					if( dirkey ) dirkey[0] = (*entkey);
					LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %s", next_entries + n, name_ofs, next_entries[n].first_block ^ dirkey->first_block, filename );
					return next_entries + n;
				}
			}
		}
		next_dir_block = vfs_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) { DebugBreak(); return NULL; }  // should have a last-entry before no more blocks....
#endif
		this_dir_block = next_dir_block;
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI SaveFileName( struct volume *vol, const char * filename ) {
	size_t n;
	BLOCKINDEX this_name_block = 1;
	while( 1 ) {
		enum block_cache_entries cache = BC(NAMES);
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block, cache );
		unsigned char *name = (unsigned char*)names;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) ) {
			int c = name[0];
			if( vol->key ) c = c ^ vol->usekey[BC(NAMES)][name-(unsigned char*)names];
			if( !c ) {
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) ) {
					LoG( "using unused entry for new file...%" _size_f "  %" _size_f " %s", this_name_block, (uintptr_t)name - (uintptr_t)names, filename );
					if( vol->key ) {						
						for( n = 0; n < namelen + 1; n++ )
							name[n] = filename[n] ^ vol->usekey[BC(NAMES)][n + (name-(unsigned char*)names)];
						if( (namelen + 1) < (size_t)(((unsigned char*)names + BLOCK_SIZE) - name) )
							name[n] = vol->usekey[BC(NAMES)][n + (name - (unsigned char*)names)];
					} else
						memcpy( name, filename, ( namelen + 1 ) );
					return ((uintptr_t)name) - ((uintptr_t)vol->disk);
				}
			}
			else
				if( MaskStrCmp( vol, filename, name - (unsigned char*)vol->disk, 0 ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					return ((uintptr_t)name) - ((uintptr_t)vol->disk);
				}
			if( vol->key ) {
				while( ( name[0] ^ vol->usekey[BC(NAMES)][name-(unsigned char*)names] ) ) name++;
				name++;
			} else
				name = name + StrLen( (const char*)name ) + 1;
			LoG( "new position is %" _size_f "  %" _size_f, this_name_block, (uintptr_t)name - (uintptr_t)names );
		}
		this_name_block = vfs_GetNextBlock( vol, this_name_block, GFB_INIT_DIRENT, TRUE );
		LoG( "Need a new directory block....", this_name_block );
	}
}


static struct directory_entry * GetNewDirectory( struct volume *vol, const char * filename ) {
	size_t n;
	BLOCKINDEX this_dir_block = 0;
	struct directory_entry *next_entries;
	LOGICAL moveMark = FALSE;
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, cache );
		for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			struct directory_entry *entkey = ( vol->key )?((struct directory_entry *)vol->usekey[cache])+n:&l.zero_entkey;
			struct directory_entry *ent = next_entries + n;
			FPI name_ofs = ent->name_offset ^ entkey->name_offset;
			BLOCKINDEX first_blk = ent->first_block ^ entkey->first_block;
			// not name_offset (end of list) or not first_block(free entry) use this entry
			if( name_ofs && (first_blk > 1) )  continue;
			if( first_blk == EODMARK ) moveMark = TRUE;
			name_ofs = SaveFileName( vol, filename ) ^ entkey->name_offset;
			first_blk = GetFreeBlock( vol, FALSE ) ^ entkey->first_block;
			// get free block might have expanded and moved the disk; reseek and get ent address
			next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, cache );
			ent = next_entries + n;
			ent->filesize = entkey->filesize;
			ent->name_offset = name_ofs;
			ent->first_block = first_blk;
			if( n < (VFS_DIRECTORY_ENTRIES - 1) ) {
				if( moveMark ) {
					struct directory_entry *enttmp = next_entries + (n + 1);
					enttmp->first_block = EODMARK ^ entkey[1].first_block;
				}
			} else {
				// otherwise pre-init the next directory sector
				this_dir_block = vfs_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, TRUE );
			}
			return ent;
		}
		this_dir_block = vfs_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, TRUE );
	}
	while( 1 );

}

struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, const char * filename ) {
	struct sack_vfs_file *file = New( struct sack_vfs_file );
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( filename[0] == '.' && filename[1] == '/' ) filename += 2;
	LoG( "sack_vfs open %s = %p on %s", filename, file, vol->volname );
	file->entry = ScanDirectory( vol, filename, &file->dirent_key, 0 );
	if( !file->entry ) {
		if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; Deallocate( struct sack_vfs_file *, file ); return NULL; }
		else file->entry = GetNewDirectory( vol, filename );
	}
	if( vol->key )
		memcpy( &file->dirent_key, vol->usekey[BC(DIRECTORY)] + ( (uintptr_t)file->entry & BLOCK_MASK ), sizeof( struct directory_entry ) );
	else
		memset( &file->dirent_key, 0, sizeof( struct directory_entry ) );
	file->vol = vol;
	file->fpi = 0;
	file->delete_on_close = 0;
	file->_first_block = file->block = file->entry->first_block ^ file->dirent_key.first_block;
	LoG( "file block is %d", (int)file->block );
	file->blockChain = NULL;
	file->blockChainAvail = 0;
	file->blockChainLength = 0;
	SetBlockChain( file, 0, file->block );
	AddLink( &vol->files, file );
	vol->lock = 0;
	return file;
}

static struct sack_vfs_file * CPROC sack_vfs_open( uintptr_t psvInstance, const char * filename, const char *opts ) {
	return sack_vfs_openfile( (struct volume*)psvInstance, filename );
}

int CPROC sack_vfs_exists( struct volume *vol, const char * file ) {
	struct directory_entry entkey;
	struct directory_entry *ent;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	ent = ScanDirectory( vol, file, &entkey, 0 );
	//lprintf( "sack_vfs exists %s %s", ent?"ya":"no", file );
	vol->lock = 0;
	if( ent ) return TRUE;
	return FALSE;
}

size_t CPROC sack_vfs_tell( struct sack_vfs_file *file ) { return (size_t)file->fpi; }

size_t CPROC sack_vfs_size( struct sack_vfs_file *file ) { return (size_t)(file->entry->filesize ^ file->dirent_key.filesize); }

size_t CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	BLOCKINDEX b;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();


	if( (file->fpi >> BLOCK_SIZE_BITS) < file->blockChainLength ) {
		enum block_cache_entries cache = BC( FILE );
		file->block = file->blockChain[file->fpi >> BLOCK_SIZE_BITS];
#ifdef _DEBUG
		if( !file->block )DebugBreak();
#endif
		//LoG( "file block is %d", (int)file->block );
		vfs_BSEEK( file->vol, file->block, &cache );
		file->vol->lock = 0;
		return (size_t)file->fpi;
	}
	else {
		LoG( "NEed more blocks after end of file...." );
		file->block = b = file->blockChain[file->blockChainLength - 1];
		old_fpi = ( file->blockChainLength - 1 ) * BLOCK_SIZE;
	}

	{
		if( ( file->fpi & ( ~BLOCK_MASK ) ) >= ( old_fpi & ( ~BLOCK_MASK ) ) ) {
			do {
				if( ( file->fpi & ( ~BLOCK_MASK ) ) == ( old_fpi & ( ~BLOCK_MASK ) ) ) {
					file->block = b;
					file->vol->lock = 0;
					LoG( "-- file block is finally %d", (int)b );
					return (size_t)file->fpi;
				}
				b = vfs_GetNextBlock( file->vol, b, FALSE, TRUE );
				LoG( "-- file block will be %d   %d  %d", (int)b, (int)file->fpi, (int)(old_fpi) );
				old_fpi += BLOCK_SIZE;// the actual old FPI already had a block (input file->block), new FPI gets this block.
				SetBlockChain( file, old_fpi, b );
				//SetBlockChain( file, old_fpi, file->block );
			} while( 1 );
		}
	}
	{
		size_t n = file->blockChainLength - 1;
#ifdef _DEBUG
		if( n & 0x80000000 ) DebugBreak();
#endif
		while( n * BLOCK_SIZE < ( pos & ~BLOCK_MASK ) ) {
			b = vfs_GetNextBlock( file->vol, b, FALSE, TRUE );
			n++;
		}
		file->block = b;
		LoG( "++ file block is %d", (int)file->block );
		SetBlockChain( file, file->fpi, file->block );
	}
	file->vol->lock = 0;
	return (size_t)file->fpi;
}

static void MaskBlock( struct volume *vol, uint8_t* usekey, uint8_t* block, BLOCKINDEX block_ofs, size_t ofs, const char *data, size_t length ) {
	size_t n;
	block += block_ofs;
	usekey += ofs;
	if( vol->key )
		for( n = 0; n < length; n++ ) (*block++) = (*data++) ^ (*usekey++);
	else {
		for( n = 0; n < length; n++, block++, data++ ) block[0] = data[0];
		//memcpy( block, data, length );
	}
}

size_t CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE - ofs;
			written += BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache ); // in case the block needs to be allocated/expanded.
			LoG( "file block is %d", (int)file->block );
			SetBlockChain( file, file->fpi, file->block );
			length -= BLOCK_SIZE - ofs;
		} else {
			MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, length );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache );
		if( length >= BLOCK_SIZE ) {
			MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE;
			written += BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache ); // in case the block needs to be allocated/expanded.
#ifdef _DEBUG
			if( !file->block ) DebugBreak();
#endif
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) ) {
				SetBlockChain( file, file->fpi, file->block );
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			}
			LoG( "file block is %d", (int)file->block );
			length -= BLOCK_SIZE;
		} else {
			MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, length );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			length = 0;
		}
	}
	file->vol->lock = 0;
	return written;
}

size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) ) {
		if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < file->fpi )
			length = 0;
		else
			length = (size_t)( file->entry->filesize  ^ file->dirent_key.filesize ) - (size_t)file->fpi;
	}
	if( !length ) {  file->vol->lock = 0; return 0; }

	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), BLOCK_SIZE - ofs );
			written += BLOCK_SIZE - ofs;
			data += BLOCK_SIZE - ofs;
			length -= BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache ); // in case the block needs to be allocated/expanded.
			LoG( "file block is %d", (int)file->block );
			SetBlockChain( file, file->fpi, file->block );
		} else {
			MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache );
		if( length >= BLOCK_SIZE ) {
			MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, BLOCK_SIZE - ofs );
			written += BLOCK_SIZE;
			data += BLOCK_SIZE;
			length -= BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			block = (uint8_t*)vfs_BSEEK( file->vol, file->block, &cache ); // in case the block needs to be allocated/expanded.
			LoG( "file block is %d", (int)file->block );
			SetBlockChain( file, file->fpi, file->block );
		} else {
			MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	file->vol->lock = 0;
	return written;
}

static void sack_vfs_unlink_file_entry( struct volume *vol, struct directory_entry *entry, struct directory_entry *entkey, BLOCKINDEX first_block, LOGICAL deleted ) {
	BLOCKINDEX block, _block;
	struct sack_vfs_file *file_found = NULL;
	struct sack_vfs_file *file;
	INDEX idx;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file ) {
		if( file->_first_block == entry->first_block ) {
			file_found = file;
			file->delete_on_close = TRUE;
		}
	}
	if( !deleted ) {
		entry->first_block = entkey->first_block; // zero the block... keep the name.
	}

	if( !file_found ) {
		_block = block = first_block;// entry->first_block ^ entkey->first_block;
		LoG( "(marking physical deleted (again?)) entry starts at %d", block );
		// wipe out file chain BAT
		do {
			enum block_cache_entries cache = BC(BAT);
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
			BLOCKINDEX _thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
			uint8_t* blockData = (uint8_t*)vfs_BSEEK( vol, block, &cache );
			//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
			memset( blockData, 0, BLOCK_SIZE );
			block = vfs_GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
			AddDataItem( &vol->pdlFreeBlocks, &_block );
			_block = block;
		} while( block != EOFBLOCK );
	}
}

static void shrinkBAT( struct sack_vfs_file *file ) {
	struct volume *vol = file->vol;
	struct directory_entry *entry = file->entry;
	struct directory_entry *entkey = &file->dirent_key;
	BLOCKINDEX block, _block;
	size_t bsize = 0;

	_block = block = entry->first_block ^ entkey->first_block;
	do {
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries data_cache = BC(DATAKEY);
		BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
		BLOCKINDEX _thiskey;
		_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
		block = vfs_GetNextBlock( vol, block, FALSE, FALSE );
		if( bsize > (entry->filesize ^ entkey->filesize) ) {
			uint8_t* blockData = (uint8_t*)vfs_BSEEK( file->vol, _block, &data_cache );
			//LoG( "clearing a datablock after a file..." );
			memset( blockData, 0, BLOCK_SIZE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
		} else {
			bsize++;
			if( bsize > (entry->filesize ^ entkey->filesize) ) {
				uint8_t* blockData = (uint8_t*)vfs_BSEEK( file->vol, _block, &data_cache );
				//LoG( "clearing a partial datablock after a file..., %d, %d", BLOCK_SIZE-(entry->filesize & (BLOCK_SIZE-1)), ( entry->filesize & (BLOCK_SIZE-1)) );
				memset( blockData + ( entry->filesize & (BLOCK_SIZE-1)), 0, BLOCK_SIZE-(entry->filesize & (BLOCK_SIZE-1)) );
				this_BAT[_block & (BLOCKS_PER_BAT-1)] = ~_thiskey;
			}
		}
		_block = block;
	} while( block != EOFBLOCK );	
}

size_t CPROC sack_vfs_truncate( struct sack_vfs_file *file ) { file->entry->filesize = file->fpi ^ file->dirent_key.filesize; shrinkBAT( file ); return (size_t)file->fpi; }

int CPROC sack_vfs_close( struct sack_vfs_file *file ) {
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
#ifdef DEBUG_TRACE_LOG
	{
		enum block_cache_entries cache = BC(NAMES);
		static char fname[256];
		FPI name_ofs = file->entry->name_offset ^ file->dirent_key.name_offset;
		TSEEK( const char *, file->vol, name_ofs, cache ); // have to do the seek to the name block otherwise it might not be loaded.
		MaskStrCpy( fname, sizeof( fname ), file->vol, name_ofs );
		LoG( "close file:%s(%p)", fname, file );
	}
#endif
	DeleteLink( &file->vol->files, file );
	if( file->delete_on_close ) sack_vfs_unlink_file_entry( file->vol, file->entry, &file->dirent_key, file->_first_block, TRUE );
	file->vol->lock = 0;
	if( file->vol->closed ) sack_vfs_unload_volume( file->vol );
	Deallocate( struct sack_vfs_file *, file );
	return 0;
}

int CPROC sack_vfs_unlink_file( struct volume *vol, const char * filename ) {
	int result = 0;
	struct directory_entry entkey;
	struct directory_entry *entry;
	if( !vol ) return 0;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	LoG( "unlink file:%s", filename );
	if( ( entry  = ScanDirectory( vol, filename, &entkey, 0 ) ) ) {
		sack_vfs_unlink_file_entry( vol, entry, &entkey, entry->first_block ^ entkey.first_block, FALSE );
		result = 1;
	}
	vol->lock = 0;
	return result;
}

int CPROC sack_vfs_flush( struct sack_vfs_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_need_copy_write( void ) {	return FALSE; }

struct find_info {
	BLOCKINDEX this_dir_block;
	char filename[BLOCK_SIZE];
	struct volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	VFS_DISK_DATATYPE filesize;
	CTEXTSTR mask;
	size_t thisent;
};

struct find_info * CPROC sack_vfs_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask )
{
	struct find_info *info = New( struct find_info );
	info->base = base;
	info->base_len = StrLen( base );
	info->mask = mask;
	info->vol = (struct volume *)psvInst;
	return info;
}

static int iterate_find( struct find_info *info ) {
	struct directory_entry *next_entries;
	size_t n;
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		enum block_cache_entries name_cache = BC(NAMES);
		next_entries = BTSEEK( struct directory_entry *, info->vol, info->this_dir_block, cache );
		for( n = info->thisent; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_entry *)info->vol->usekey[cache])+n:&l.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs )	
				return 0;
			// if file is deleted; don't check it's name.
			if( !(next_entries[n].first_block ^ entkey->first_block ) )
				continue;
			if( (next_entries[n].first_block ^ entkey->first_block ) == EODMARK )
				return 0; // end of directory.
			info->filesize = next_entries[n].filesize ^ entkey->filesize;
			if( (name_ofs) > info->vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}
			TSEEK( const char *, info->vol, name_ofs, name_cache );
			if( info->vol->key ) {
				int c;
				info->filenamelen = 0;
				while( ( c = ( ((uint8_t*)info->vol->disk)[name_ofs] ^ info->vol->usekey[name_cache][name_ofs&BLOCK_MASK] ) ) ) {
					info->filename[info->filenamelen++] = c;
					name_ofs++;
				}
				info->filename[info->filenamelen]	 = c;
				LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			} else {
				StrCpy( info->filename, (const char *)(((uint8_t*)info->vol->disk) + name_ofs) );
				LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			}
			info->thisent = n + 1;
			return 1;
		}
		info->thisent = 0; // new block, set new starting index.
		info->this_dir_block = vfs_GetNextBlock( info->vol, info->this_dir_block, FALSE, FALSE );
	}
	while( info->this_dir_block != EOFBLOCK );
	return 0;
}

int CPROC sack_vfs_find_first( struct find_info *info ) {
	info->this_dir_block = 0;
	info->thisent = 0;
	return iterate_find( info );
}

int CPROC sack_vfs_find_close( struct find_info *info ) { Deallocate( struct find_info*, info ); return 0; }
int CPROC sack_vfs_find_next( struct find_info *info ) { return iterate_find( info ); }
char * CPROC sack_vfs_find_get_name( struct find_info *info ) { return info->filename; }
size_t CPROC sack_vfs_find_get_size( struct find_info *info ) { return (size_t)info->filesize; }
LOGICAL CPROC sack_vfs_find_is_directory( struct find_cursor *cursor ) { return FALSE; }
LOGICAL CPROC sack_vfs_is_directory( uintptr_t psvInstance, const char *path ) {
	if( path[0] == '.' && path[1] == 0 ) return TRUE;
	{
		struct volume *vol = (struct volume *)psvInstance;
		if( ScanDirectory( vol, path, NULL, 1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}

LOGICAL CPROC sack_vfs_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct volume *vol = (struct volume *)psvInstance;
	// fail if the names are the same.
	if( strcmp( original, newname ) == 0 )
		return FALSE;
	if( vol ) {
		struct directory_entry entkey;
		struct directory_entry *entry;
		while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
		if( ( entry  = ScanDirectory( vol, original, &entkey, 0 ) ) ) {
			struct directory_entry new_entkey;
			struct directory_entry *new_entry;
			if( (new_entry = ScanDirectory( vol, newname, &new_entkey, 0 )) ) {
				vol->lock = 0;
				sack_vfs_unlink_file( vol, newname );
				while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
			}
			entry->name_offset = SaveFileName( vol, newname ) ^ entkey.name_offset;
			vol->lock = 0;
			return TRUE;
		}
		vol->lock = 0;
	}
	return FALSE;
}

static struct file_system_interface sack_vfs_fsi = {
                                                     (void*(CPROC*)(uintptr_t,const char *, const char*))sack_vfs_open
                                                   , (int(CPROC*)(void*))sack_vfs_close
                                                   , (size_t(CPROC*)(void*,char*,size_t))sack_vfs_read
                                                   , (size_t(CPROC*)(void*,const char*,size_t))sack_vfs_write
                                                   , (size_t(CPROC*)(void*,size_t,int))sack_vfs_seek
                                                   , (void(CPROC*)(void*))sack_vfs_truncate
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_unlink_file
                                                   , (size_t(CPROC*)(void*))sack_vfs_size
                                                   , (size_t(CPROC*)(void*))sack_vfs_tell
                                                   , (int(CPROC*)(void*))sack_vfs_flush
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_exists
                                                   , sack_vfs_need_copy_write
                                                   , (struct find_cursor*(CPROC*)(uintptr_t,const char *,const char *))             sack_vfs_find_create_cursor
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_first
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_close
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_next
                                                   , (char*(CPROC*)(struct find_cursor*))           sack_vfs_find_get_name
                                                   , (size_t(CPROC*)(struct find_cursor*))          sack_vfs_find_get_size
                                                   , sack_vfs_find_is_directory
                                                   , sack_vfs_is_directory
                                                   , sack_vfs_rename
                                                   };

PRIORITY_PRELOAD( Sack_VFS_Register, CONFIG_SCRIPT_PRELOAD_PRIORITY - 2 )
{
#ifdef ALT_VFS_NAME
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME ".runner"
#else
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME
#endif
	sack_register_filesystem_interface( DEFAULT_VFS_NAME, &sack_vfs_fsi );
}

PRIORITY_PRELOAD( Sack_VFS_RegisterDefaultFilesystem, SQL_PRELOAD_PRIORITY + 1 ) {
	if( SACK_GetProfileInt( GetProgramName(), "SACK/VFS/Mount VFS", 0 ) ) {
		struct volume *vol;
		TEXTCHAR volfile[256];
		TEXTSTR tmp;
		SACK_GetProfileString( GetProgramName(), "SACK/VFS/File", "*/../assets.svfs", volfile, 256 );
		tmp = ExpandPath( volfile );
		vol = sack_vfs_load_volume( tmp );
		Deallocate( TEXTSTR, tmp );
		sack_mount_filesystem( "sack_shmem", sack_get_filesystem_interface( DEFAULT_VFS_NAME )
		                     , 900, (uintptr_t)vol, TRUE );
	}
}

SACK_VFS_NAMESPACE_END
