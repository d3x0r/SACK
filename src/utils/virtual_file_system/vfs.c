/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#include <stdhdrs.h>
#include <ctype.h> // tolower on linux
#include <filesys.h>
#include <procreg.h>
#include <salty_generator.h>
#include <sack_vfs.h>
#include <sqlgetoption.h>

SACK_VFS_NAMESPACE
//#define PARANOID_INIT

//#define DEBUG_TRACE_LOG

#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#endif

#include "vfs_internal.h"

static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
} l;
#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)

#define GFB_INIT_NONE   0
#define GFB_INIT_DIRENT 1
#define GFB_INIT_NAMES  2
static BLOCKINDEX GetFreeBlock( struct volume *vol, int init );

static char mytolower( int c ) {	if( c == '\\' ) return '/'; return tolower( c ); }

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int MaskStrCmp( struct volume *vol, const char * filename, FPI name_offset ) {
	if( vol->key ) {
		int c;
		while(  ( c = ( ((uint8_t*)vol->disk)[name_offset] ^ vol->usekey[BLOCK_CACHE_NAMES][name_offset&BLOCK_MASK] ) )
			  && filename[0] ) {
			int del = mytolower(filename[0]) - mytolower(c);
			if( del ) return del;
			filename++;
			name_offset++;
		}
		// c will be 0 or filename will be 0... 
		return filename[0] - c;
	} else {
		//LoG( "doesn't volume always have a key?" );
		return StrCaseCmp( filename, (const char *)(((uint8_t*)vol->disk) + name_offset) );
	}
}

#ifdef DEBUG_TRACE_LOG
static void MaskStrCpy( char *output, size_t outlen, struct volume *vol, FPI name_offset ) {
	if( vol->key ) {
		int c;
		FPI name_start = name_offset;
		while(  ( c = ( ((uint8_t*)vol->disk)[name_offset] ^ vol->usekey[BLOCK_CACHE_NAMES][name_offset&BLOCK_MASK] ) ) ) {
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

static void UpdateSegmentKey( struct volume *vol, enum block_cache_entries cache_idx )
{
	SRG_ResetEntropy( vol->entropy );
	vol->curseg = cache_idx;  // so we know which 'segment[idx]' to use.
	SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->segkey, SHORTKEY_LENGTH * 8 );
	{
		int n;
		uint8_t* usekey = vol->usekey[cache_idx];
		for( n = 0; n < BLOCK_SIZE; n++ )
			(*usekey++) = vol->key[n] ^ vol->segkey[n&(SHORTKEY_LENGTH-1)];
	}
}

static LOGICAL ValidateBAT( struct volume *vol ) {
	BLOCKINDEX first_slab = 0;
	BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
	BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
	BLOCKINDEX n;
	if( vol->key ) {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) { 
			size_t m;
			BLOCKINDEX *BAT = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			//while( LockedExchange( &vol->key_lock[BLOCK_CACHE_BAT], 1 ) ) Relinquish();
			UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
			for( m = 0; ((BAT[m] & ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m]) != EOBBLOCK ) && m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[m] ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) break;
				if( block >= last_block ) return FALSE;
			}
			//vol->key_lock[BLOCK_CACHE_BAT] = 0;
		}
	} else {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			for( m = 0; ((BAT[m] & ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m]) != EOBBLOCK ) && m < BLOCKS_PER_BAT; m++ ) {
				BLOCKINDEX block = BAT[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) break;
				if( block >= last_block ) return FALSE;
			}
		}
	}
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
	size_t oldsize = vol->dwSize;
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
	if( new_disk != vol->disk ) {
		INDEX idx;
		struct sack_vfs_file *file;
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
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			if( ( n % (BLOCKS_PER_SECTOR) ) == 0 )	 UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
#ifdef PARANOID_INIT
			else SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->usekey[BLOCK_CACHE_BAT], BLOCK_SIZE * 8 );
#else
			else continue;
#endif
			//memcpy( ((uint8_t*)vol->disk) + n * BLOCK_SIZE, vol->usekey[BLOCK_CACHE_BAT], BLOCK_SIZE );
			((BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE))[0] = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[0];
			memset( ((BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE))+1, 0, BLOCK_SIZE - sizeof( BLOCKINDEX ) );
		}
	}
	else if( !oldsize )  {
		memset( vol->disk, 0, vol->dwSize );
		((BLOCKINDEX*)vol->disk)[0] = EOBBLOCK;
	} else if( oldsize )  {
		memset( ((uint8_t*)vol->disk) + oldsize, 0, vol->dwSize - oldsize );
		((BLOCKINDEX*)(((uint8_t*)vol->disk) + oldsize))[0] = EOBBLOCK;
	}

	if( !oldsize ) {
		// can't recover dirents and nameents dynamically; so just assume
		// use the GetFreeBlock because it will update encypted
		//vol->disk->BAT[0] = EOFBLOCK;  // allocate 1 directory entry block
		//vol->disk->BAT[1] = EOFBLOCK;  // allocate 1 name block
		/* vol->dirents = */GetFreeBlock( vol, GFB_INIT_DIRENT );
		/* vol->nameents = */GetFreeBlock( vol, GFB_INIT_NAMES );
	}
	return TRUE;
}

// shared with fuse module
uintptr_t vfs_SEEK( struct volume *vol, FPI offset, enum block_cache_entries cache_index ) {
	while( offset >= vol->dwSize ) if( !ExpandVolume( vol ) ) return 0;
	if( vol->key ) {
		BLOCKINDEX seg = ( offset / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index] ) {
			vol->segment[cache_index] = seg;
			UpdateSegmentKey( vol, cache_index );
		}
	}
	return ((uintptr_t)vol->disk) + offset;
}

// shared with fuse module
uintptr_t vfs_BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries cache_index ) {
	BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + ( block & (BLOCKS_PER_BAT-1) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !ExpandVolume( vol ) ) return 0;
	if( vol->key ) {
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index] ) {
			vol->segment[cache_index] = seg;
			UpdateSegmentKey( vol, cache_index );
		}
	}
	return ((uintptr_t)vol->disk) + b;
}

static BLOCKINDEX GetFreeBlock( struct volume *vol, int init )
{
	size_t n;
	int b = 0;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, BLOCK_CACHE_BAT );
	if( !current_BAT ) return 0;
	do
	{
		BLOCKINDEX check_val;
		for( n = 0; n < BLOCKS_PER_BAT; n++ )
		{
			check_val = current_BAT[n];
			if( vol->key )
				check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[n];
			if( !check_val || (check_val == 1) )
			{
				// mark it as claimed; will be enf of file marker...
				// adn thsi result will overwrite previous EOF.
				if( vol->key )
				{
					current_BAT[n] = EOFBLOCK ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[n];
					if( init )
					{
						vol->segment[BLOCK_CACHE_FILE] = b * (BLOCKS_PER_SECTOR) + n + 1 + 1;  
						UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
						while( ((vol->segment[BLOCK_CACHE_FILE]-1)*BLOCK_SIZE) > vol->dwSize ){
							LoG( "looping to get a size %d", ((vol->segment[BLOCK_CACHE_FILE]-1)*BLOCK_SIZE) );
							if( !ExpandVolume( vol ) ) return 0;
						}
						if( init == GFB_INIT_DIRENT )
							((struct directory_entry*)(((uint8_t*)vol->disk) + (vol->segment[BLOCK_CACHE_FILE]-1) * BLOCK_SIZE))[0].first_block = 1^((struct directory_entry*)vol->usekey[BLOCK_CACHE_FILE])->first_block;
						else if( init == GFB_INIT_NAMES )
							((char*)(((uint8_t*)vol->disk) + (vol->segment[BLOCK_CACHE_FILE]-1) * BLOCK_SIZE))[0] = ((char*)vol->usekey[BLOCK_CACHE_FILE])[0];
						else
							memcpy( ((uint8_t*)vol->disk) + (vol->segment[BLOCK_CACHE_FILE]-1) * BLOCK_SIZE, vol->usekey[BLOCK_CACHE_FILE], BLOCK_SIZE );
					}
				} else {
					current_BAT[n] = EOFBLOCK;
				}
				if( (check_val == EOBBLOCK) && (n < (BLOCKS_PER_BAT-1)) )
					current_BAT[n+1] = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[n+1];
				return b * BLOCKS_PER_BAT + n;
			}
		}
		b++;
		current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), BLOCK_CACHE_BAT );
	}while( 1 );
}

static BLOCKINDEX vfs_GetNextBlock( struct volume *vol, BLOCKINDEX block, int init, LOGICAL expand ) {
	BLOCKINDEX sector = block >> BLOCK_SHIFT;
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), BLOCK_CACHE_FILE );
	BLOCKINDEX seg;
	BLOCKINDEX check_val = (this_BAT[block & (BLOCKS_PER_BAT-1)]);
	if( !this_BAT ) return 0; // if this passes, later ones will also.

	if( vol->key ) {
		seg = ( ((uintptr_t)this_BAT - (uintptr_t)vol->disk) / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[BLOCK_CACHE_FILE] ) {
			vol->segment[BLOCK_CACHE_FILE] = seg;
			UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
		}
		check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[block & (BLOCKS_PER_BAT-1)];
	}
	if( check_val == EOFBLOCK ) {
		if( expand ) {
			BLOCKINDEX key = vol->key?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[block & (BLOCKS_PER_BAT-1)]:0;
			check_val = GetFreeBlock( vol, init );
			// free block might have expanded...
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), BLOCK_CACHE_FILE );
			if( !this_BAT ) return 0;
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
		(*salt_size) = sizeof( vol->segment[vol->curseg] );
		(*salt) = &vol->segment[vol->curseg];
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
		uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * BLOCK_CACHE_COUNT + BLOCK_SIZE + SHORTKEY_LENGTH;
		int n;
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( AddSalt, (uintptr_t)vol );
		else
			SRG_ResetEntropy( vol->entropy );
		vol->key = (uint8_t*)OpenSpace( NULL, NULL, &size );
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ )
			vol->usekey[n] = vol->key + (n + 1) * BLOCK_SIZE;
		vol->segkey = vol->key + BLOCK_SIZE * (BLOCK_CACHE_COUNT + 1);
		vol->sigkey = vol->key + BLOCK_SIZE * (BLOCK_CACHE_COUNT + 1) + SHORTKEY_LENGTH;
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[BLOCK_CACHE_DIRECTORY] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, BLOCK_SIZE * 8 );
	}
	else {
		int n;
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ )
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
	vol->volname = SaveText( filepath );
	AssignKey( vol, NULL, NULL );
	if( !ExpandVolume( vol ) || !ValidateBAT( vol ) ) { Deallocate( struct volume*, vol ); return NULL; }
	return vol;
}

struct volume *sack_vfs_load_crypt_volume( const char * filepath, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	vol->volname = SaveText( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	AssignKey( vol, userkey, devkey );
	if( !ExpandVolume( vol ) || !ValidateBAT( vol ) ) { sack_vfs_unload_volume( vol ); return NULL; }
	return vol;
}

struct volume *sack_vfs_use_crypt_volume( POINTER memory, size_t sz, const char * userkey, const char * devkey ) {
	struct volume *vol = New( struct volume );
	MemSet( vol, 0, sizeof( struct volume ) );
	vol->read_only = 1;
	AssignKey( vol, userkey, devkey );
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
	if( !vol->external_memory )	Deallocate( struct disk *, vol->diskReal );
	if( vol->key ) {
		Deallocate( uint8_t*, vol->key );
		SRG_DestroyEntropy( &vol->entropy );
	}
	Deallocate( struct volume*, vol );
}

void sack_vfs_shrink_volume( struct volume * vol ) {
	size_t n;
	int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	int last_bat = 0;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, BLOCK_CACHE_FILE );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		for( n = 0; n < BLOCKS_PER_BAT; n++ ) {
			check_val = current_BAT[n];
			if( vol->key )	check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[n];
			if( check_val ) {
				last_bat = b;
				last_block = n;
			}
			//if( !check_val ) found_free = TRUE;
		}
		b++;
		if( b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) < vol->dwSize ) {
			current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), BLOCK_CACHE_FILE );
		} else
			break;
	}while( 1 );

	Deallocate( struct disk *, vol->diskReal );
	SetFileLength( vol->volname, 
			((uintptr_t)vol->disk - (uintptr_t)vol->diskReal) +
			last_bat * BLOCKS_PER_SECTOR * BLOCK_SIZE + ( last_block + 1 + 1 )* BLOCK_SIZE );
	// setting 0 size will cause expand to do an initial open instead of expanding
	vol->dwSize = 0;
}

LOGICAL sack_vfs_decrypt_volume( struct volume *vol )
{
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( !vol->key ) return FALSE; // volume is already decrypted, cannot remove key
	{
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *block = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
			for( m = 0; m < BLOCKS_PER_BAT; m++ )
				block[m] ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m];
		}
	}
	AssignKey( vol, NULL, NULL );
	vol->lock = 0;
	return TRUE;
}

LOGICAL sack_vfs_encrypt_volume( struct volume *vol, CTEXTSTR key1, CTEXTSTR key2 ) {
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( vol->key ) return FALSE; // volume already has a key, cannot apply new key
	AssignKey( vol, key1, key2 );
	{
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *block = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * BLOCK_SIZE);
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
			for( m = 0; m < BLOCKS_PER_BAT; m++ )
				block[m] ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m];
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
		uint8_t* usekey = vol->key?vol->usekey[BLOCK_CACHE_DATAKEY]:l.zerokey;
		signature[256] = 0;
		memset( datakey, 0, sizeof( datakey ) );
		{
			{
				size_t n;
				BLOCKINDEX this_dir_block = 0;
				BLOCKINDEX next_dir_block;
				BLOCKINDEX *next_entries;
				do {
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, BLOCK_CACHE_DATAKEY );
					for( n = 0; n < BLOCKS_PER_BAT; n++ )
						datakey[n] ^= next_entries[n] ^ ((BLOCKINDEX*)(((uint8_t*)usekey)))[n];
					
					next_dir_block = vfs_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, FALSE );
					if( this_dir_block == next_dir_block )
						DebugBreak();
					if( next_dir_block == 0 )
						DebugBreak();
					this_dir_block = next_dir_block;
				}
				while( next_dir_block != EOFBLOCK );
			}
		}
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( AddSalt, (uintptr_t)vol );
		SRG_ResetEntropy( vol->entropy );
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[vol->curseg] = 0;
		vol->datakey = (const char *)datakey;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)usekey, 128 );
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

static struct directory_entry * ScanDirectory( struct volume *vol, const char * filename, struct directory_entry *dirkey ) {
	size_t n;
	BLOCKINDEX this_dir_block = 0;
	BLOCKINDEX next_dir_block;
	struct directory_entry *next_entries;
	do {
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			BLOCKINDEX bi;
			struct directory_entry *entkey = ( vol->key)?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			//const char * testname;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs )	return NULL;
			//LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
			//   , n, name_ofs
			//   , next_entries[n].name_offset ^ entkey->name_offset
			//   , next_entries[n].first_block ^ entkey->first_block
			//   , filename );
			bi = next_entries[n].first_block ^ entkey->first_block;
			// if file is deleted; don't check it's name.
			if( !bi ) continue;
			// if file is end of directory, done sanning.
			if( bi == 1 ) return NULL; // done.
			//testname =
			TSEEK( const char *, vol, name_ofs, BLOCK_CACHE_NAMES ); // have to do the seek to the name block otherwise it might not be loaded.
			if( MaskStrCmp( vol, filename, name_ofs ) == 0 ) {
				dirkey[0] = (*entkey);
				LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %s", next_entries+n, name_ofs, next_entries[n].first_block ^ dirkey->first_block, filename );
				return next_entries + n;
			}
		}
		next_dir_block = vfs_GetNextBlock( vol, this_dir_block, FALSE, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) DebugBreak();
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
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block, BLOCK_CACHE_NAMES );
		unsigned char *name = (unsigned char*)names;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) ) {
			int c = name[0];
			if( vol->key ) c = c ^ vol->usekey[BLOCK_CACHE_NAMES][name-(unsigned char*)names];
			if( !c ) {
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) ) {
					LoG( "using unused entry for new file...%" _size_f "  %" _size_f " %s", this_name_block, (uintptr_t)name - (uintptr_t)names, filename );
					if( vol->key ) {						
						for( n = 0; n < namelen + 1; n++ )
							name[n] = filename[n] ^ vol->usekey[BLOCK_CACHE_NAMES][n + (name-(unsigned char*)names)];
						if( (namelen + 1) < (size_t)(((unsigned char*)names + BLOCK_SIZE) - name) )
							name[n] = vol->usekey[BLOCK_CACHE_NAMES][n + (name - (unsigned char*)names)];
					} else
						memcpy( name, filename, ( namelen + 1 ) );
					return ((uintptr_t)name) - ((uintptr_t)vol->disk);
				}
			}
			else
				if( MaskStrCmp( vol, filename, name - (unsigned char*)vol->disk ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					return ((uintptr_t)name) - ((uintptr_t)vol->disk);
				}
			if( vol->key ) {
				while( ( name[0] ^ vol->usekey[BLOCK_CACHE_NAMES][name-(unsigned char*)names] ) ) name++;
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
	do {
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			struct directory_entry *entkey = ( vol->key )?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			struct directory_entry *ent = next_entries + n;
			FPI name_ofs = ent->name_offset ^ entkey->name_offset;
			BLOCKINDEX first_blk = ent->first_block ^ entkey->first_block;
			// not name_offset (end of list) or not first_block(free entry) use this entry
			if( name_ofs && (first_blk > 1) )  continue;
			name_ofs = SaveFileName( vol, filename ) ^ entkey->name_offset;
			first_blk = GetFreeBlock( vol, FALSE ) ^ entkey->first_block;
			// get free block might have expanded and moved the disk; reseek and get ent address
			next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
			ent = next_entries + n;
			ent->filesize = entkey->filesize;
			ent->name_offset = name_ofs;
			ent->first_block = first_blk;
			if( n < (VFS_DIRECTORY_ENTRIES - 1) ) {
				struct directory_entry *enttmp = next_entries + (n+1);
				enttmp->first_block = 1 ^ entkey[1].first_block;
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
	file->entry = ScanDirectory( vol, filename, &file->dirent_key );
	if( !file->entry ) {
		if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; Deallocate( struct sack_vfs_file *, file ); return NULL; }
		else file->entry = GetNewDirectory( vol, filename );
	}
	if( vol->key )
		memcpy( &file->dirent_key, vol->usekey[BLOCK_CACHE_DIRECTORY] + ( (uintptr_t)file->entry & BLOCK_MASK ), sizeof( struct directory_entry ) );
	else
		memset( &file->dirent_key, 0, sizeof( struct directory_entry ) );
	file->vol = vol;
	file->fpi = 0;
	file->delete_on_close = 0;
	file->block = file->entry->first_block ^ file->dirent_key.first_block;
	AddLink( &vol->files, file );
	vol->lock = 0;
	return file;
}

struct sack_vfs_file * CPROC sack_vfs_open( uintptr_t psvInstance, const char * filename ) { return sack_vfs_openfile( (struct volume*)psvInstance, filename ); }

int CPROC _sack_vfs_exists( struct volume *vol, const char * file ) {
	struct directory_entry entkey;
	struct directory_entry *ent;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	ent = ScanDirectory( vol, file, &entkey );
	//lprintf( "sack_vfs exists %s %s", ent?"ya":"no", file );
	vol->lock = 0;
	if( ent ) return TRUE;
	return FALSE;
}

int CPROC sack_vfs_exists( uintptr_t psvInstance, const char * file ) { return _sack_vfs_exists( (struct volume*)psvInstance, file ); }

size_t CPROC sack_vfs_tell( struct sack_vfs_file *file ) { return file->fpi; }

size_t CPROC sack_vfs_size( struct sack_vfs_file *file ) {	return file->entry->filesize ^ file->dirent_key.filesize; }

size_t CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	{
		if( ( file->fpi & ( ~BLOCK_MASK ) ) >= ( old_fpi & ( ~BLOCK_MASK ) ) ) {
			do {
				if( ( file->fpi & ( ~BLOCK_MASK ) ) == ( old_fpi & ( ~BLOCK_MASK ) ) ) {
					file->vol->lock = 0;
					return file->fpi;
				}
				file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
				old_fpi += BLOCK_SIZE;
			} while( 1 );
		}
	}
	{
		size_t n = 0;
		BLOCKINDEX b = file->entry->first_block ^ file->dirent_key.first_block;
		while( n * BLOCK_SIZE < ( pos & ~BLOCK_MASK ) ) {
			b = vfs_GetNextBlock( file->vol, b, FALSE, TRUE );
			n++;
		}
		file->block = b;
	}
	file->vol->lock = 0;
	return file->fpi;
}

static void MaskBlock( struct volume *vol, uint8_t* usekey, uint8_t* block, BLOCKINDEX block_ofs, size_t ofs, const char *data, size_t length ) {
	size_t n;
	block += block_ofs;
	usekey += ofs;
	if( vol->key )
		for( n = 0; n < length; n++ ) (*block++) = (*data++) ^ (*usekey++);
	else
		memcpy( block, data, length );
}

size_t CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length ) {
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
	if( ofs ) {
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE - ofs;
			written += BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE - ofs;
		} else {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, ofs, ofs, data, length );
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
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= BLOCK_SIZE ) {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, 0, 0, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE;
			written += BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE;
		} else {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, 0, 0, data, length );
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
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) )
		length = ( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi;
	if( !length ) {  file->vol->lock = 0; return 0; }

	if( ofs ) {
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), BLOCK_SIZE - ofs );
			written += BLOCK_SIZE - ofs;
			data += BLOCK_SIZE - ofs;
			length -= BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
		} else {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		uint8_t* block = (uint8_t*)vfs_BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= BLOCK_SIZE ) {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (uint8_t*)data, 0, 0, (const char*)block, BLOCK_SIZE - ofs );
			written += BLOCK_SIZE;
			data += BLOCK_SIZE;
			length -= BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = vfs_GetNextBlock( file->vol, file->block, FALSE, TRUE );
		} else {
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (uint8_t*)data, 0, 0, (const char*)block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	file->vol->lock = 0;
	return written;
}

static void sack_vfs_unlink_file_entry( struct volume *vol, struct directory_entry *entry, struct directory_entry *entkey ) {
	BLOCKINDEX block, _block;
	struct sack_vfs_file *file_found = NULL;
	struct sack_vfs_file *file;
	INDEX idx;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file  ) {
		if( file->entry == entry ) {
			file_found = file;
			file->delete_on_close = TRUE;
		}
	}
	if( !file_found ) {
		_block = block = entry->first_block ^ entkey->first_block;
		LoG( "entry starts at %d", entry->first_block ^ entkey->first_block );
		entry->first_block = entkey->first_block; // zero the block... keep the name.
		// wipe out file chain BAT
		do {
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), BLOCK_CACHE_FILE );
			BLOCKINDEX _thiskey;
			_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[_block & (BLOCKS_PER_BAT-1)]:0;
			uint8_t* blockData = (uint8_t*)vfs_BSEEK( vol, block, BLOCK_CACHE_DATAKEY );
			//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
			memset( blockData, 0, BLOCK_SIZE );
			block = vfs_GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
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
		BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), BLOCK_CACHE_FILE );
		BLOCKINDEX _thiskey;
		_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[_block & (BLOCKS_PER_BAT-1)]:0;
		block = vfs_GetNextBlock( vol, block, FALSE, FALSE );
		if( bsize > (entry->filesize ^ entkey->filesize) ) {
			uint8_t* blockData = (uint8_t*)vfs_BSEEK( file->vol, _block, BLOCK_CACHE_DATAKEY );
			//LoG( "clearing a datablock after a file..." );
			memset( blockData, 0, BLOCK_SIZE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
		} else {
			bsize++;
			if( bsize > (entry->filesize ^ entkey->filesize) ) {
				uint8_t* blockData = (uint8_t*)vfs_BSEEK( file->vol, _block, BLOCK_CACHE_DATAKEY );
				//LoG( "clearing a partial datablock after a file..., %d, %d", BLOCK_SIZE-(entry->filesize & (BLOCK_SIZE-1)), ( entry->filesize & (BLOCK_SIZE-1)) );
				memset( blockData + ( entry->filesize & (BLOCK_SIZE-1)), 0, BLOCK_SIZE-(entry->filesize & (BLOCK_SIZE-1)) );
				this_BAT[_block & (BLOCKS_PER_BAT-1)] = ~_thiskey;
			}
		}
		_block = block;
	} while( block != EOFBLOCK );	
}

size_t CPROC sack_vfs_truncate( struct sack_vfs_file *file ) { file->entry->filesize = file->fpi ^ file->dirent_key.filesize; shrinkBAT( file ); return file->fpi; }

int CPROC sack_vfs_close( struct sack_vfs_file *file ) { 
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
#ifdef DEBUG_TRACE_LOG
	{
		static char fname[256];
		FPI name_ofs = file->entry->name_offset ^ file->dirent_key.name_offset;
		TSEEK( const char *, file->vol, name_ofs, BLOCK_CACHE_NAMES ); // have to do the seek to the name block otherwise it might not be loaded.
		MaskStrCpy( fname, sizeof( fname ), file->vol, name_ofs );
		LoG( "close file:%s(%p)", fname, file );
	}
#endif
	DeleteLink( &file->vol->files, file ); 
	if( file->delete_on_close ) sack_vfs_unlink_file_entry( file->vol, file->entry, &file->dirent_key );  
	file->vol->lock = 0;
	if( file->vol->closed ) sack_vfs_unload_volume( file->vol );
	Deallocate( struct sack_vfs_file *, file );
	return 0; 
}

void CPROC sack_vfs_unlink_file( uintptr_t psv, const char * filename ) {
	struct volume *vol = (struct volume *)psv;
	struct directory_entry entkey;
	struct directory_entry *entry;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	LoG( "unlink file:%s", filename );
	if( ( entry  = ScanDirectory( vol, filename, &entkey ) ) )
		sack_vfs_unlink_file_entry( vol, entry, &entkey );
	vol->lock = 0;
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
	size_t filesize;
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
		next_entries = BTSEEK( struct directory_entry *, info->vol, info->this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = info->thisent; n < VFS_DIRECTORY_ENTRIES; n++ ) {
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_entry *)info->vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			//const char * testname;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs )	
				return 0;
			// if file is deleted; don't check it's name.
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) 
				continue;
			if( (next_entries[n].first_block ^ entkey->first_block ) == 1 ) 
				return 0; // end of directory.
			info->filesize = next_entries[n].filesize ^ entkey->filesize;
			//testname =
				TSEEK( const char *, info->vol, name_ofs, BLOCK_CACHE_NAMES );
			if( info->vol->key ) {
				int c;
				info->filenamelen = 0;
				while( ( c = ( ((uint8_t*)info->vol->disk)[name_ofs] ^ info->vol->usekey[BLOCK_CACHE_NAMES][name_ofs&BLOCK_MASK] ) ) ) {
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
size_t CPROC sack_vfs_find_get_size( struct find_info *info ) { return info->filesize; }

static LOGICAL CPROC sack_vfs_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct volume *vol = (struct volume *)psvInstance;
	if( vol ) {
		struct directory_entry entkey;
		struct directory_entry *entry;
		while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
		if( ( entry  = ScanDirectory( vol, original, &entkey ) ) ) {
			struct directory_entry new_entkey;
			struct directory_entry *new_entry;
			if( ( new_entry = ScanDirectory( vol, newname, &new_entkey ) ) ) return FALSE;
			entry->name_offset = SaveFileName( vol, newname ) ^ entkey.name_offset;
			vol->lock = 0;
			return TRUE;
		}
		vol->lock = 0;
	}
	return FALSE;
}

static LOGICAL CPROC sack_vfs_is_directory( const char *name ) { return FALSE; }



//#ifndef NO_SACK_INTERFACE

static struct file_system_interface sack_vfs_fsi = { 
                                                     (void*(CPROC*)(uintptr_t,const char *))sack_vfs_open
                                                   , (int(CPROC*)(void*))sack_vfs_close
                                                   , (size_t(CPROC*)(void*,char*,size_t))sack_vfs_read
                                                   , (size_t(CPROC*)(void*,const char*,size_t))sack_vfs_write
                                                   , (size_t(CPROC*)(void*,size_t,int))sack_vfs_seek
                                                   , (void(CPROC*)(void*))sack_vfs_truncate
                                                   , sack_vfs_unlink_file
                                                   , (size_t(CPROC*)(void*))sack_vfs_size
                                                   , (size_t(CPROC*)(void*))sack_vfs_tell
                                                   , (int(CPROC*)(void*))sack_vfs_flush
                                                   , sack_vfs_exists
                                                   , sack_vfs_need_copy_write
                                                   , (struct find_cursor*(CPROC*)(uintptr_t,const char *,const char *))             sack_vfs_find_create_cursor
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_first
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_close
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_find_next
                                                   , (char*(CPROC*)(struct find_cursor*))           sack_vfs_find_get_name
                                                   , (size_t(CPROC*)(struct find_cursor*))          sack_vfs_find_get_size
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

//#endif

SACK_VFS_NAMESPACE_END
