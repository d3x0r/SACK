/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 _8  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#include <stdhdrs.h>
#include <filesys.h>
#include <procreg.h>
#include <salty_generator.h>
#include <sack_vfs.h>

SACK_VFS_NAMESPACE
#define PARANOID_INIT

#define BLOCK_SIZE 4096
#define BLOCK_MASK (BLOCK_SIZE-1) 
#define BLOCKS_PER_BAT (BLOCK_SIZE/sizeof(BLOCKINDEX))
#define BLOCKS_PER_SECTOR (1 + (BLOCK_SIZE/sizeof(BLOCKINDEX)))
// per-sector perumation; needs to be a power of 2
#define SHORTKEY_LENGTH 16

typedef size_t BLOCKINDEX; // BLOCK_SIZE blocks...
typedef size_t FPI; // file position type

enum block_cache_entries
{
	BLOCK_CACHE_DIRECTORY
	, BLOCK_CACHE_NAMES
	, BLOCK_CACHE_FILE
	, BLOCK_CACHE_BAT
	, BLOCK_CACHE_COUNT
};

PREFIX_PACKED struct volume {
	CTEXTSTR volname;
	struct disk *disk;
	//_32 dirents;  // constant 0
	//_32 nameents; // constant 1
	size_t dwSize;
	CTEXTSTR userkey;
	CTEXTSTR devkey;
	enum block_cache_entries curseg;
	BLOCKINDEX segment[BLOCK_CACHE_COUNT];// associated with usekey[n]
	struct random_context *entropy;
	P_8 key;  // allow byte encrypting...
	P_8 segkey;  // allow byte encrypting... key based on sector volume file index
	P_8 usekey[BLOCK_CACHE_COUNT]; // composite key
	PLIST files; // when reopened file structures need to be updated also...
   LOGICAL read_only;
} PACKED;

PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	size_t filesize;  // how big the file is
	//_32 filler;  // extra data(unused)
} PACKED;
#define ENTRIES ( BLOCK_SIZE/sizeof( struct directory_entry) )

struct disk
{
	// BAT is at 0 of every BLOCK_SIZE blocks (4097 total)
	// &BAT[0] == itself....
	// BAT[0] == first directory entry (actually next entry; first is always here)
	// BAT[1] == first name entry (actually next name block; first is known as here)
	// bat[BLOCK_SIZE] == NEXT_BAT[0]; NEXT_BAT = BAT + BLOCK_SIZE + 1024*BLOCK_SIZE;
	// bat[8192] == ... ( 0 + ( BLOCK_SIZE + BLOCKS_PER_BAT*BLOCK_SIZE ) * N >> 12 )
	BLOCKINDEX BAT[BLOCKS_PER_BAT];
	//struct directory_entry directory[BLOCK_SIZE/sizeof( struct directory_entry)]; // 256
	//TEXTCHAR names[BLOCK_SIZE/sizeof(TEXTCHAR)];
	_8  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];
};

struct sack_vfs_file
{
	struct directory_entry *entry;  // has file size within
	struct directory_entry dirent_key;
	struct volume *vol; // which volume this is in
	FPI fpi;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
}  ;

static struct {
	struct volume *default_volume;
	struct directory_entry zero_entkey;
} l;

static BLOCKINDEX GetFreeBlock( struct volume *vol, LOGICAL init );
static void DumpDirectory( struct volume *vol );

static int mytolower( int c ) {	if( c == '\\' ) return '/'; return tolower( c ); }

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int MaskStrCmp( struct volume *vol, CTEXTSTR filename, FPI name_offset )
{
	if( vol->key )
	{
		int c;
		while(  ( c = ( ((P_8)vol->disk)[name_offset] ^ vol->usekey[BLOCK_CACHE_NAMES][name_offset&BLOCK_MASK] ) )
			  && filename[0] )
		{
			int del = mytolower(filename[0]) - mytolower(c);
			if( del ) return del;
			filename++;
			name_offset++;
		}
		// c will be 0 or filename will be 0... 
		return filename[0] - c;
	}
	else
	{
		return StrCaseCmp( filename, (CTEXTSTR)(((P_8)vol->disk) + name_offset) );
	}
}

static void UpdateSegmentKey( struct volume *vol, enum block_cache_entries cache_idx )
{
	SRG_ResetEntropy( vol->entropy );
	vol->curseg = cache_idx;  // so we know which 'segment[idx]' to use.
	SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->segkey, SHORTKEY_LENGTH * 8 );
	{
		int n;
		P_8 usekey = vol->usekey[cache_idx];
		for( n = 0; n < BLOCK_SIZE; n++ )
			(*usekey++) = vol->key[n] ^ vol->segkey[n&(SHORTKEY_LENGTH-1)];
	}
}

static LOGICAL ValidateBAT( struct volume *vol )
{
	if( vol->key )
	{
		BLOCKINDEX first_slab = 0;
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
		BLOCKINDEX n;
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  )
		{
			int m;
			BLOCKINDEX *BAT = (BLOCKINDEX*)(((P_8)vol->disk) + n * BLOCK_SIZE);
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[m] ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_BAT])[m];
				if( block == ~0 ) continue;
				if( block >= last_block ) return FALSE;
			}
		}
	}
	else 
	{
		BLOCKINDEX first_slab = 0;
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
		BLOCKINDEX n;
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  )
		{
			int m;
			BLOCKINDEX *BAT = (BLOCKINDEX*)(((P_8)vol->disk) + n * BLOCK_SIZE);
			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[m];
				if( block == ~0 ) continue;
				if( block >= last_block ) return FALSE;
			}
		}
	}
	return TRUE;
}

// add some space to the volume....
static void ExpandVolume( struct volume *vol )
{
	LOGICAL created;
	struct disk* new_disk;
	size_t oldsize = vol->dwSize;
	if( vol->read_only ) return;
	if( !vol->dwSize )
	{
		new_disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
		if( new_disk && vol->dwSize )
		{
			vol->disk = new_disk;
			return;
		}
		else
			created = 1;
	}
	
	if( oldsize ) CloseSpace( vol->disk );

	// a BAT plus the sectors it references... ( BLOCKS_PER_BAT + 1 ) * BLOCK_SIZE
	vol->dwSize += BLOCKS_PER_SECTOR*BLOCK_SIZE;

	new_disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
	if( new_disk != vol->disk )
	{
		INDEX idx;
		struct sack_vfs_file *file;
		LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		{
			file->entry = (struct directory_entry*)((PTRSZVAL)file->entry - (PTRSZVAL)vol->disk + (PTRSZVAL)new_disk );
		}
		vol->disk = new_disk;
	}
	if( vol->key )
	{
		BLOCKINDEX first_slab = oldsize / ( BLOCK_SIZE );
		BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
		BLOCKINDEX n;
		for( n = first_slab; n < slab; n++  )
		{
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			if( ( n % (BLOCKS_PER_SECTOR) ) == 0 )	 UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
#ifdef PARANOID_INIT
			else 	                 SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->usekey[BLOCK_CACHE_BAT], BLOCK_SIZE * 8 );
#else
			else continue;
#endif
			memcpy( ((P_8)vol->disk) + n * BLOCK_SIZE, vol->usekey[BLOCK_CACHE_BAT], BLOCK_SIZE );
		}
	}
	else if( !oldsize ) memset( vol->disk, 0, vol->dwSize );
	else if( oldsize ) memset( ((P_8)vol->disk) + oldsize, 0, vol->dwSize - oldsize );

	if( !oldsize )
	{
		// can't recover dirents and nameents dynamically; so just assume
		// use the GetFreeBlock because it will update encypted
		//vol->disk->BAT[0] = ~0;  // allocate 1 directory entry block
		//vol->disk->BAT[1] = ~0;  // allocate 1 name block
		/* vol->dirents = */GetFreeBlock( vol, TRUE );
		/* vol->nameents = */GetFreeBlock( vol, TRUE );
	}

}

static PTRSZVAL SEEK( struct volume *vol, FPI offset, enum block_cache_entries cache_index )
{
	while( offset >= vol->dwSize ) ExpandVolume( vol );
	if( vol->key )
	{
		BLOCKINDEX seg = ( offset / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index] )
		{
			vol->segment[cache_index] = seg;
			UpdateSegmentKey( vol, cache_index );
		}
	}
	return ((PTRSZVAL)vol->disk) + offset;
}

static PTRSZVAL BSEEK( struct volume *vol, BLOCKINDEX block, enum block_cache_entries cache_index )
{
	BLOCKINDEX b = BLOCK_SIZE + (block >> 10) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + ( block & (BLOCKS_PER_BAT-1) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) ExpandVolume( vol );
	if( vol->key )
	{
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[cache_index] )
		{
			vol->segment[cache_index] = seg;
			UpdateSegmentKey( vol, cache_index );
		}
	}
	return ((PTRSZVAL)vol->disk) + b;
}

#define TSEEK(type,v,o,c) ((type)SEEK(v,o,c))
#define BTSEEK(type,v,o,c) ((type)BSEEK(v,o,c))

static BLOCKINDEX GetFreeBlock( struct volume *vol, LOGICAL init )
{
	int n;
	int b = 0;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, BLOCK_CACHE_FILE );
	do
	{
		BLOCKINDEX check_val;
		for( n = 0; n < BLOCKS_PER_BAT; n++ )
		{
			check_val = current_BAT[n];
			if( vol->key )
				check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[n];
			if( !check_val )
			{
				// mark it as claimed; will be enf of file marker...
				// adn thsi result will overwrite previous EOF.
				if( vol->key )
				{
					current_BAT[n] = ~0 ^ ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[n];
					if( init )
					{
						vol->segment[BLOCK_CACHE_FILE] = b * (BLOCKS_PER_SECTOR) + n + 1 + 1;  
						UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
						while( ((vol->segment[BLOCK_CACHE_FILE]-1)*BLOCK_SIZE) > vol->dwSize )
							ExpandVolume( vol );
						memcpy( ((P_8)vol->disk) + (vol->segment[BLOCK_CACHE_FILE]-1) * BLOCK_SIZE, vol->usekey[BLOCK_CACHE_FILE], BLOCK_SIZE );
					}
				}
				else
				{
					current_BAT[n] = ~0;
				}
				return b * BLOCKS_PER_BAT + n;
			}
		}
		b++;
		current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), BLOCK_CACHE_FILE );
	}while( 1 );
}

static BLOCKINDEX GetNextBlock( struct volume *vol, BLOCKINDEX block, LOGICAL init, LOGICAL expand )
{
	BLOCKINDEX sector = block >> 10;
	BLOCKINDEX new_block = 0;
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), BLOCK_CACHE_FILE );
	BLOCKINDEX seg;

	BLOCKINDEX check_val = (this_BAT[block & (BLOCKS_PER_BAT-1)]);
	if( vol->key )
	{
		seg = ( ((PTRSZVAL)this_BAT - (PTRSZVAL)vol->disk) / BLOCK_SIZE ) + 1;
		if( seg != vol->segment[BLOCK_CACHE_FILE] )
		{
			vol->segment[BLOCK_CACHE_FILE] = seg;
			UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
		}
		check_val ^= ((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[block & (BLOCKS_PER_BAT-1)];
	}
	if( check_val == ~0 )
	{
		if( expand )
		{
			BLOCKINDEX key = vol->key?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[block & (BLOCKS_PER_BAT-1)]:0;
			check_val = GetFreeBlock( vol, init );
			// free block might have expanded...
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), BLOCK_CACHE_FILE );
			// segment could already be set from the GetFreeBlock...
			this_BAT[block & (BLOCKS_PER_BAT-1)] = check_val ^ key;
		}
	}
	return check_val;
}

static void DumpDirectory( struct volume *vol )
{
	int n;
	int this_dir_block = 0;
	int next_dir_block;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < ENTRIES; n++ )
		{
			struct directory_entry *entkey = ( vol->key )?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs ) return;  // end of directory			
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) continue;// if file is deleted; don't check it's name.
			{
				TEXTCHAR buf[256];
				P_8 name = TSEEK( P_8, vol, name_ofs, BLOCK_CACHE_NAMES );
				int ch;
				_8 c;
				if( name_ofs < 8192 )
				{
					name_ofs -= BLOCK_SIZE;
					name_ofs += 8192;
					next_entries[n].name_offset = name_ofs ^ entkey->name_offset;
					next_entries[n].first_block = entkey->first_block;
					name = TSEEK( P_8, vol, name_ofs, BLOCK_CACHE_NAMES );
				}
				for( ch = 0; c = (name[ch] ^ vol->usekey[BLOCK_CACHE_NAMES][( name_ofs & BLOCK_MASK ) +ch]); ch++ )
					buf[ch] = c;
				buf[ch] = c;
				lprintf( "Directory entry: %s  start %d length %d", buf
					, next_entries[n].first_block ^ entkey->first_block
					, next_entries[n].filesize ^ entkey->filesize
					);
			}
		}
		next_dir_block = GetNextBlock( vol, this_dir_block, TRUE, TRUE );
		if( this_dir_block == next_dir_block )
			DebugBreak();
		if( next_dir_block == 0 )
			DebugBreak();
		this_dir_block = next_dir_block;

	}
	while( 1 );
}

struct volume *sack_vfs_load_volume( CTEXTSTR filepath )
{
	struct volume *vol = New( struct volume );
	vol->read_only = 0;
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveText( filepath );
	vol->key = NULL;
	vol->files = NULL;
	ExpandVolume( vol );
	if( !ValidateBAT( vol ) )  return NULL;
	if( !l.default_volume )	l.default_volume = vol;
	return vol;
}

static void AddSalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
	struct volume *vol = (struct volume *)psv;
	if( vol->userkey )
	{
		(*salt_size) = StrLen( vol->userkey );
		(*salt) = (POINTER)vol->userkey;
		vol->userkey = NULL;
	}
	else if( vol->devkey )
	{
		(*salt_size) = StrLen( vol->devkey );
		(*salt) = (POINTER)vol->devkey;
		vol->devkey = NULL;
	}
	else if( vol->segment[vol->curseg] )
	{
		(*salt_size) = sizeof( vol->segment[vol->curseg] );
		(*salt) = &vol->segment[vol->curseg];
	}
	else 
		(*salt_size) = 0;
}

struct volume *sack_vfs_load_crypt_volume( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
{
	struct volume *vol = New( struct volume );
	vol->read_only = 0;
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveText( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	{
		FPI size = BLOCK_SIZE + BLOCK_SIZE * BLOCK_CACHE_COUNT + SHORTKEY_LENGTH;
		int n;
		vol->entropy = SRG_CreateEntropy2( AddSalt, (PTRSZVAL)vol );
		vol->key = (P_8)OpenSpace( NULL, NULL, &size );
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ )
			vol->usekey[n] = vol->key + (n+1) * BLOCK_SIZE;
		vol->segkey = vol->key + BLOCK_SIZE * (n+1);
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[BLOCK_CACHE_DIRECTORY] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->key, BLOCK_SIZE * 8 );
	}
	vol->files = NULL;
	ExpandVolume( vol );
	if( !ValidateBAT( vol ) ) return NULL;
	if( !l.default_volume )  l.default_volume = vol;
	return vol;
}

struct volume *sack_vfs_use_crypt_volume( POINTER memory, size_t sz, CTEXTSTR userkey, CTEXTSTR devkey )
{
	struct volume *vol = New( struct volume );
	vol->read_only = 1;
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = NULL;
	vol->userkey = userkey;
	vol->devkey = devkey;
	{
		FPI size = BLOCK_SIZE + BLOCK_SIZE * BLOCK_CACHE_COUNT + SHORTKEY_LENGTH;
		int n;
		vol->entropy = SRG_CreateEntropy2( AddSalt, (PTRSZVAL)vol );
		vol->key = (P_8)OpenSpace( NULL, NULL, &size );
		for( n = 0; n < BLOCK_CACHE_COUNT; n++ )
			vol->usekey[n] = vol->key + (n+1) * BLOCK_SIZE;
		vol->segkey = vol->key + BLOCK_SIZE * (n+1);
		vol->curseg = BLOCK_CACHE_DIRECTORY;
		vol->segment[BLOCK_CACHE_DIRECTORY] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->key, BLOCK_SIZE * 8 );
	}
	vol->files = NULL;
	vol->disk = (struct disk*)memory;
	vol->dwSize = sz;
	if( !ValidateBAT( vol ) ) { Release( vol ); return NULL; }
	if( !l.default_volume )  l.default_volume = vol;
	return vol;
}

void sack_vfs_unload_volume( struct volume * vol )
{
	Release( vol->key );
	SRG_DestroyEntropy( &vol->entropy );
	Release( vol );
	if( l.default_volume == vol ) l.default_volume = NULL;
}

static struct directory_entry * ScanDirectory( struct volume *vol, CTEXTSTR filename, struct directory_entry *dirkey )
{
	int n;
	int this_dir_block = 0;
	int next_dir_block;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < ENTRIES; n++ )
		{
			struct directory_entry *entkey = ( vol->key)?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			CTEXTSTR testname;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs )	return NULL;
			// if file is deleted; don't check it's name.
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) continue;
			testname = TSEEK( CTEXTSTR, vol, name_ofs, BLOCK_CACHE_NAMES );
			if( MaskStrCmp( vol, filename, name_ofs ) == 0 )
			{
				dirkey[0] = (*entkey);
				return next_entries + n;
			}
		}
		next_dir_block = GetNextBlock( vol, this_dir_block, TRUE, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) DebugBreak();
#endif
		this_dir_block = next_dir_block;
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI SaveFileName( struct volume *vol, CTEXTSTR filename )
{
	size_t n;
	int this_name_block = 1;
	while( 1 )
	{
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block, BLOCK_CACHE_NAMES );
		unsigned char *name = (unsigned char*)names;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) )
		{
			int c = name[0];
			if( vol->key )	c = c ^ vol->usekey[BLOCK_CACHE_NAMES][name-(unsigned char*)names];
			if( !c )
			{
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) )
				{
					if( vol->key )
					{						
						for( n = 0; n < namelen + 1; n++ )
							name[n] = filename[n] ^ vol->usekey[BLOCK_CACHE_NAMES][n + (name-(unsigned char*)names)];
					}
					else
						memcpy( name, filename, ( namelen + 1 ) * sizeof( TEXTCHAR )  );
					return ((PTRSZVAL)name) - ((PTRSZVAL)vol->disk);
				}
			}
			else
				if( MaskStrCmp( vol, filename, name - (unsigned char*)vol->disk ) == 0 )
					return ((PTRSZVAL)name) - ((PTRSZVAL)vol->disk);
			if( vol->key )
			{
				while( ( name[0] ^ vol->usekey[BLOCK_CACHE_NAMES][name-(unsigned char*)names] ) ) name++;
				name++;
			}
			else
				name = name + StrLen( (TEXTCHAR*)name ) + 1;
		}
		this_name_block = GetNextBlock( vol, this_name_block, TRUE, TRUE );
	}
}


static struct directory_entry * GetNewDirectory( struct volume *vol, CTEXTSTR filename )
{
	int n;
	int this_dir_block = 0;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = 0; n < ENTRIES; n++ )
		{
			struct directory_entry *entkey = ( vol->key )?((struct directory_entry *)vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			struct directory_entry *ent = next_entries + n;
			FPI name_ofs = ent->name_offset ^ entkey->name_offset;
			BLOCKINDEX first_blk = ent->first_block ^ entkey->first_block;
			// not name_offset (end of list) or not first_block(free entry) use this entry
			if( name_ofs && first_blk )	continue;
			name_ofs = SaveFileName( vol, filename ) ^ entkey->name_offset;
			first_blk = GetFreeBlock( vol, FALSE ) ^ entkey->first_block;
			// get free block might have expanded and moved the disk; reseek and get ent address
			next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block, BLOCK_CACHE_DIRECTORY );
			ent = next_entries + n;
			ent->name_offset = name_ofs;
			ent->first_block = first_blk;
  			return ent;
		}
		this_dir_block = GetNextBlock( vol, this_dir_block, TRUE, TRUE );
	}
	while( 1 );

}

struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, CTEXTSTR filename )
{
	struct sack_vfs_file *file = New( struct sack_vfs_file );
	if( filename[0] == '.' && filename[1] == '/' ) filename += 2;
	lprintf( "sack_vfs open %s", filename );
	file->entry = ScanDirectory( vol, filename, &file->dirent_key );
	if( !file->entry )
		if( vol->read_only ) { Release( file ); return NULL; }
		else file->entry = GetNewDirectory( vol, filename );
	if( vol->key )
	{
		//FPI offset = ( (PTRSZVAL)file->entry - (PTRSZVAL)vol->disk->directory );
		memcpy( &file->dirent_key, vol->usekey[BLOCK_CACHE_DIRECTORY] + ( (PTRSZVAL)file->entry & BLOCK_MASK ), sizeof( struct directory_entry ) );
	}
	else
		memset( &file->dirent_key, 0, sizeof( struct directory_entry ) );
	file->vol = vol;
	file->fpi = 0;
	file->block = file->entry->first_block ^ file->dirent_key.first_block;
	AddLink( &vol->files, file );
	return file;
}

struct sack_vfs_file * CPROC sack_vfs_open( CTEXTSTR filename ) { return sack_vfs_openfile( l.default_volume, filename ); }

int CPROC _sack_vfs_exists( struct volume *vol, CTEXTSTR file )
{
	struct directory_entry entkey;
	struct directory_entry *ent;
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	ent = ScanDirectory( vol, file, &entkey );
	lprintf( "sack_vfs exists %s %s", ent?"ya":"no", file );
	if( ent ) return TRUE;
	return FALSE;
}

int CPROC sack_vfs_exists( CTEXTSTR file ) { return _sack_vfs_exists( l.default_volume, file ); }

size_t CPROC sack_vfs_tell( struct sack_vfs_file *file ) { return file->fpi; }

size_t CPROC sack_vfs_size( struct sack_vfs_file *file ) {	return file->entry->filesize ^ file->dirent_key.filesize; }

size_t CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;

	{
		if( ( file->fpi & ( ~BLOCK_MASK ) ) >= ( old_fpi & ( ~BLOCK_MASK ) ) )
		{
			do
			{
				if( ( file->fpi & ( ~BLOCK_MASK ) ) == ( old_fpi & ( ~BLOCK_MASK ) ) )
					return file->fpi;
				file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
				old_fpi += BLOCK_SIZE;
			} while( 1 );
		}
	}
	{
		size_t n = 0;
		int b = file->entry->first_block ^ file->dirent_key.first_block;
		while( n * BLOCK_SIZE < ( pos & ~BLOCK_MASK ) )
		{
			b = GetNextBlock( file->vol, b, FALSE, TRUE );
			n++;
		}
		file->block = b;
	}
	return file->fpi;
}

static void MaskBlock( struct volume *vol, P_8 usekey, P_8 block, BLOCKINDEX block_ofs, int ofs, const char *data, size_t length )
{
	size_t n;
	block += block_ofs;
	usekey += ofs;
	if( vol->key )
		for( n = 0; n < length; n++ ) (*block++) = (*data++) ^ (*usekey++);
	else
		memcpy( block + block_ofs, data, length );
}

size_t CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length )
{
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE - ofs;
			written += BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE - ofs;
		}
		else
		{
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
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= BLOCK_SIZE )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, 0, 0, data, BLOCK_SIZE - ofs );
			data += BLOCK_SIZE;
			written += BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= BLOCK_SIZE;
		}
		else
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, 0, 0, data, length );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			length = 0;
		}
	}
	return written;
}

size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length )
{
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) )
		length = ( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi;
	if( !length )
		return 0;

	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, ofs, (const char*)(block+ofs), BLOCK_SIZE - ofs );
			written += BLOCK_SIZE - ofs;
			data += BLOCK_SIZE - ofs;
			length -= BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
		}
		else
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length > BLOCK_SIZE )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, 0, (const char*)block, BLOCK_SIZE - ofs );
			written += BLOCK_SIZE;
			data += BLOCK_SIZE;
			length -= BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
		}
		else
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, 0, (const char*)block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	return written;
}

size_t sack_vfs_truncate( struct sack_vfs_file *file ) { file->entry->filesize = file->fpi ^ file->dirent_key.filesize; return file->fpi; }

int sack_vfs_close( struct sack_vfs_file *file ) { DeleteLink( &file->vol->files, file ); Release( file ); return 0; }

void sack_vfs_unlink_file( struct volume *vol, CTEXTSTR filename )
{
	struct directory_entry entkey;
	struct directory_entry *entry;
	while( entry  = ScanDirectory( vol, filename, &entkey ) )
	{
		int block, _block;
		_block = block = entry->first_block ^ entkey.first_block;
		entry->first_block = entkey.first_block; // zero the block... keep the name.
		// wipe out file chain BAT
		do
		{
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> 10 ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), BLOCK_CACHE_FILE );
			BLOCKINDEX _thiskey;
			_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[BLOCK_CACHE_FILE])[_block & (BLOCKS_PER_BAT-1)]:0;
			block = GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
			_block = block;
		}while( block != ~0 );
	}
}

void sack_vfs_unlink( CTEXTSTR filename ) {	sack_vfs_unlink_file( l.default_volume, filename ); }

int sack_vfs_flush( struct sack_vfs_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_need_copy_write( void ) {	return FALSE; }

struct find_info 
{
	BLOCKINDEX this_dir_block;
	char filename[BLOCK_SIZE];
	struct volume *vol;
	size_t filenamelen;
	int thisent;
};

static struct find_info * CPROC sack_vfs_find_create_cursor( void )
{
	struct find_info *info = New( struct find_info );
	return info;
}

static int iterate_find( struct find_info *info )
{
	struct directory_entry *next_entries;
	int n;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, info->vol, info->this_dir_block, BLOCK_CACHE_DIRECTORY );
		for( n = info->thisent; n < ENTRIES; n++ )
		{
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_entry *)info->vol->usekey[BLOCK_CACHE_DIRECTORY])+n:&l.zero_entkey;
			CTEXTSTR testname;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs )	
				return 1;
			// if file is deleted; don't check it's name.
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) 
				continue;
			testname = TSEEK( CTEXTSTR, info->vol, name_ofs, BLOCK_CACHE_NAMES );
			if( info->vol->key )
			{
				int c;
				info->filenamelen = 0;
				while( c = ( ((P_8)info->vol->disk)[name_ofs] ^ info->vol->usekey[BLOCK_CACHE_NAMES][name_ofs&BLOCK_MASK] ) )
				{
					info->filename[info->filenamelen++] = c;
					name_ofs++;
				}
				info->filename[info->filenamelen] = c;
			}
			else
			{
				StrCpy( info->filename, (CTEXTSTR)(((P_8)info->vol->disk) + name_ofs) );
			}
			info->thisent = n + 1;
			return 0;
		}
		info->this_dir_block = GetNextBlock( info->vol, info->this_dir_block, TRUE, TRUE );
	}
	while( 1 );
}

static int CPROC sack_vfs_find_first( char *filename, struct find_info *info )
{
	info->this_dir_block = 0;
	info->thisent = 0;
	info->vol = l.default_volume;
	return iterate_find( info );
}

static int CPROC sack_vfs_find_close( int fd ) { return 0; }
static int CPROC sack_vfs_find_next( int fd, struct find_info *info ) { return iterate_find( info ); }
static char * CPROC sack_vfs_find_get_name( struct find_info *info ) { return info->filename; }

static struct file_system_interface sack_vfs_fsi = { sack_vfs_open
                                                   , sack_vfs_close
                                                   , sack_vfs_read
                                                   , sack_vfs_write
                                                   , sack_vfs_seek
                                                   , sack_vfs_truncate
                                                   , sack_vfs_unlink
                                                   , sack_vfs_size
                                                   , sack_vfs_tell
                                                   , sack_vfs_flush
                                                   , sack_vfs_exists
                                                   , sack_vfs_need_copy_write
												   , (struct find_cursor*(CPROC*)(void))             sack_vfs_find_create_cursor
												   , (int(CPROC*)(const char *,struct find_cursor*)) sack_vfs_find_first
												   , (int(CPROC*)(int))                              sack_vfs_find_close
												   , (int(CPROC*)(int,struct find_cursor*))          sack_vfs_find_next
												   , (char*(CPROC*)(struct find_cursor*))            sack_vfs_find_get_name
                                                   };

PRIORITY_PRELOAD( Sack_VFS_Register, SQL_PRELOAD_PRIORITY )
{
#ifdef ALT_VFS_NAME
	sack_register_filesystem_interface( SACK_VFS_FILESYSTEM_NAME ".runner", &sack_vfs_fsi );
#else
	sack_register_filesystem_interface( SACK_VFS_FILESYSTEM_NAME, &sack_vfs_fsi );
#endif
}

SACK_VFS_NAMESPACE_END
