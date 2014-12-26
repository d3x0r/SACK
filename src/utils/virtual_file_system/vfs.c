/*
 _32 BAT[1024] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 _8  block_data[1024][4096];

 // 1025 * 4096 total...
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

typedef _32 BLOCKINDEX; // 4096 blocks... 
typedef _32 FPI; // file position type

enum block_cache_entries
{
	BLOCK_CACHE_DIRECTORY
	, BLOCK_CACHE_NAMES
	, BLOCK_CACHE_FILE
	, BLOCK_CACHE_BAT
};

PREFIX_PACKED struct volume {
	CTEXTSTR volname;
	struct disk *disk;
	//_32 dirents;  // constant 0
	//_32 nameents; // constant 1
	DWORD dwSize;
	CTEXTSTR userkey;
	CTEXTSTR devkey;
	int curseg;
	int segment[4];// associated with usekey[n]
	struct random_context *entropy;
	P_8 key;  // allow byte encrypting...
	P_8 segkey;  // allow byte encrypting... key based on sector volume file index
	P_8 usekey[4]; // composite key
	PLIST files; // when reopened file structures need to be updated also...
} PACKED;

PREFIX_PACKED struct directory_entry
{
	FPI name_offset;  // name offset from beginning of disk
	BLOCKINDEX first_block;  // first block of data of the file
	_32 filesize;  // how big the file is
	_32 filler;  // extra data(unused)
} PACKED;
#define ENTRIES ( 4096/sizeof( struct directory_entry) )

struct disk
{
	// BAT is at 0 of every 4096 blocks (4097 total)
	// BAT[0] == itself....
	// BAT[0] == first directory entry (actually next entry; first is always here)
	// BAT[1] == first name entry (actually next name block; first is known as here)
	// bat[4096] == NEXT_BAT[0]; NEXT_BAT = BAT + 4096 + 1024*4096;
	// bat[8192] == ... ( 0 + ( 4096 + 1024*4096 ) * N >> 12 )
	BLOCKINDEX BAT[4096 / sizeof( _32 )];
	struct directory_entry directory[4096/sizeof( struct directory_entry)]; // 256
	TEXTCHAR names[4096/sizeof(TEXTCHAR)];
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

static _32 GetFreeBlock( struct volume *vol, LOGICAL init );
static void DumpDirectory( struct volume *vol );


// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int MaskStrCmp( struct volume *vol, CTEXTSTR filename, _32 name_offset )
{
	if( vol->key )
	{
		int c;
		while(  ( c = ( ((P_8)vol->disk)[name_offset] ^ vol->usekey[BLOCK_CACHE_NAMES][name_offset&0x3FF] ) )
			  && filename[0] )
		{
			if( ! (((filename[0] >='a' && filename[0] <='z' )?filename[0]-('a'-'A'):filename[0])
									 == ((c >='a' && c <='z' )?c-('a'-'A'):c) ) )
  				return 1;  // not 0
			filename++;
			name_offset++;
		}
		if( !filename[0] && !c )
			return 0;
		return 1;
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
	SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->segkey, 16 * 8 );
	{
		int n;
		P_8 usekey = vol->usekey[cache_idx];
		for( n = 0; n < 4096; n++ )
		{
			(*usekey++) = vol->key[n] ^ vol->segkey[n&0xF];
		}
	}
}

// add some space to the volume....
static void ExpandVolume( struct volume *vol )
{
	LOGICAL created;
	struct disk* new_disk;
	_32 oldsize = vol->dwSize;
	if( !vol->dwSize )
	{
		new_disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
		if( new_disk && vol->dwSize )
		{
			vol->disk = new_disk;
			//DumpDirectory( vol );
			return;
		}
		else
			created = 1;
	}
	
	if( oldsize )
	{
		CloseSpace( vol->disk );
	}
	// a BAT plus the sectors it references... ( 1024 + 1 ) * 4096
	vol->dwSize += 1025*4096;

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
	if( !oldsize )
	{
		MemSet( vol->disk, 0, vol->dwSize );
	}
	else
	{
		if( oldsize )
			MemSet( ((P_8)vol->disk) + oldsize, 0, vol->dwSize - oldsize );
	}
	if( vol->key )
	{
		_32 ofs = oldsize;
		_32 first_slab = ofs / ( 4096 );
		_32 slab = vol->dwSize / ( 4096 );
		_32 n;
		for( n = ((first_slab /1025)*1025);  n < slab; n += 1025  )
		{
			if( ( n * 4096 ) < oldsize )
				continue;
			vol->segment[BLOCK_CACHE_BAT] = n + 1;
			UpdateSegmentKey( vol, BLOCK_CACHE_BAT );
			MemCpy( ((P_8)vol->disk) + n * 4096, vol->usekey[BLOCK_CACHE_BAT], 4096 );
		}

	}
	if( !oldsize )
	{
		// can't recover dirents and nameents dynamically; so just assume
		// use the GetFreeBlock because it will update encypted
		//vol->disk->BAT[0] = 0xFFFFFFFF;  // allocate 1 directory entry block
		//vol->disk->BAT[1] = 0xFFFFFFFF;  // allocate 1 name block
		/* vol->dirents = */GetFreeBlock( vol, TRUE );
		/* vol->nameents = */GetFreeBlock( vol, TRUE );
	}

}

static PTRSZVAL SEEK( struct volume *vol, _32 offset, enum block_cache_entries cache_index )
{
	while( offset >= vol->dwSize )
		ExpandVolume( vol );
	if( vol->key )
	{
		_32 seg = ( offset / 4096 ) + 1;
		if( seg != vol->segment[cache_index] )
		{
			vol->segment[cache_index] = seg;
			UpdateSegmentKey( vol, cache_index );
		}
	}
	return ((PTRSZVAL)vol->disk) + offset;
}

static PTRSZVAL BSEEK( struct volume *vol, _32 block, enum block_cache_entries cache_index )
{
	_32 b = 4096 + (block >> 10) * (4096*1025) + ( block & 0x3FF ) * 4096;
	while( b >= vol->dwSize )
		ExpandVolume( vol );
	if( vol->key )
	{
		_32 seg = ( b / 4096 ) + 1;
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

static _32 GetFreeBlock( struct volume *vol, LOGICAL init )
{
	int n;
	int b = 0;
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, BLOCK_CACHE_FILE );
	do
	{
		_32 check_val;
		for( n = 0; n < 1024; n++ )
		{
			check_val = current_BAT[n];
			if( vol->key )
				check_val ^= ((P_32)vol->usekey[BLOCK_CACHE_FILE])[n];
			if( !check_val )
			{
				// mark it as claimed; will be enf of file marker...
				// adn thsi result will overwrite previous EOF.
				if( vol->key )
				{
					current_BAT[n] = 0xFFFFFFFF ^ ((P_32)vol->usekey[BLOCK_CACHE_FILE])[n];
					if( init )
					{
						vol->segment[BLOCK_CACHE_FILE] = b * 1025 + n + 1 + 1;  
						UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
						while( ((vol->segment[BLOCK_CACHE_FILE]-1)*4096U) > vol->dwSize )
							ExpandVolume( vol );
						MemCpy( ((P_8)vol->disk) + (vol->segment[BLOCK_CACHE_FILE]-1) * 4096, vol->usekey[BLOCK_CACHE_FILE], 4096 );
					}
				}
				else
				{
					current_BAT[n] = 0xFFFFFFFF;
					/* MemSet(  ((P_8)vol->disk) + ( b * 1024 + n ) * 4096, 0, 4096 ); */
				}
				return b * 1024 + n;
			}
		}
		b++;
		current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( 4096 * ( 1 + 1024 ) ), BLOCK_CACHE_FILE );
	}while( 1 );
}

static _32 GetNextBlock( struct volume *vol, _32 block, LOGICAL init, LOGICAL expand )
{
	_32 sector = block >> 10;
	_32 new_block = 0;
	_32 *this_BAT = TSEEK( _32 *, vol, sector * ( 4096 * 1025 ), BLOCK_CACHE_FILE );
	_32 seg;

	_32 check_val = (this_BAT[block & 0x3FF]);
	if( vol->key )
	{
		seg = ( ((PTRSZVAL)this_BAT - (PTRSZVAL)vol->disk) / 4096 ) + 1;
		if( seg != vol->segment[BLOCK_CACHE_FILE] )
		{
			vol->segment[BLOCK_CACHE_FILE] = seg;
			UpdateSegmentKey( vol, BLOCK_CACHE_FILE );
		}
		check_val ^= ((P_32)vol->usekey[BLOCK_CACHE_FILE])[block & 0x3FF];
	}
	if( check_val == 0 )
		DebugBreak();
	if( check_val == 0xFFFFFFFF )
	{
		if( expand )
		{
			_32 key = vol->key?((P_32)vol->usekey[BLOCK_CACHE_FILE])[block & 0x3ff]:0;
			check_val = GetFreeBlock( vol, init );
			// free block might have expanded...
			this_BAT = TSEEK( _32 *, vol, sector * ( 4096 * 1025 ), BLOCK_CACHE_FILE );
			// segment could already be set from the GetFreeBlock...
			this_BAT[block & 0x3FF] = check_val ^ key;
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
			_32 name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			if( !name_ofs ) return;  // end of directory			
			if( !(next_entries[n].first_block ^ entkey->first_block ) ) continue;// if file is deleted; don't check it's name.
			{
				TEXTCHAR buf[256];
				P_8 name = TSEEK( P_8, vol, name_ofs, BLOCK_CACHE_NAMES );
				int ch;
				_8 c;
				if( name_ofs < 8192 )
				{
					name_ofs -= 4096;
					name_ofs += 8192;
					next_entries[n].name_offset = name_ofs ^ entkey->name_offset;
					next_entries[n].first_block = entkey->first_block;
					name = TSEEK( P_8, vol, name_ofs, BLOCK_CACHE_NAMES );
				}
				for( ch = 0; c = (name[ch] ^ vol->usekey[BLOCK_CACHE_NAMES][( name_ofs & 0x7FF ) +ch]); ch++ )
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
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveText( filepath );
	vol->key = NULL;
	vol->files = NULL;
	ExpandVolume( vol );
	if( !l.default_volume )	l.default_volume = vol;
	return vol;
}

static void AddSalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
{
	struct volume *vol = (struct volume *)psv;
	if( vol->segment[vol->curseg] )
	{
		(*salt_size) = 4;
		(*salt) = &vol->segment[vol->curseg];
	}
	else if( vol->userkey )
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
	else
		(*salt_size) = 0;
}

struct volume *sack_vfs_load_crypt_volume( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
{
	struct volume *vol = New( struct volume );
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveText( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	{
		_32 size = 4096 + 4096 * 4 + 16;
		int n;
		vol->entropy = SRG_CreateEntropy2( AddSalt, (PTRSZVAL)vol );
		vol->key = (P_8)OpenSpace( NULL, NULL, &size );
		for( n = 0; n < 4; n++ )
			vol->usekey[n] = vol->key + (n+1) * 4096;
		vol->segkey = vol->key + 4096 * (n+1);
		vol->curseg = 0;
		vol->segment[0] = 0;
		SRG_GetEntropyBuffer( vol->entropy, (P_32)vol->key, 4096 * 8 );
	}
	vol->files = NULL;
	ExpandVolume( vol );
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
			_32 name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
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
		while( name < ( (unsigned char*)names + 4096 ) )
		{
			int c = name[0];
			if( vol->key )	c = c ^ vol->usekey[BLOCK_CACHE_NAMES][name-(unsigned char*)names];
			if( !c )
			{
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < (size_t)( ( (unsigned char*)names + 4096 ) - name ) )
				{
					if( vol->key )
					{						
						for( n = 0; n < namelen + 1; n++ )
							name[n] = filename[n] ^ vol->usekey[BLOCK_CACHE_NAMES][n + (name-(unsigned char*)names)];
					}
					else
						MemCpy( name, filename, ( namelen + 1 ) * sizeof( TEXTCHAR )  );
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
			_32 name_ofs = ent->name_offset ^ entkey->name_offset;
			_32 first_blk = ent->first_block ^ entkey->first_block;
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
	file->entry = ScanDirectory( vol, filename, &file->dirent_key );
	if( !file->entry )
		file->entry = GetNewDirectory( vol, filename );
	if( vol->key )
	{
		_32 offset = ( (PTRSZVAL)file->entry - (PTRSZVAL)vol->disk->directory );
		MemCpy( &file->dirent_key, vol->usekey[BLOCK_CACHE_DIRECTORY] + ( offset & 0xFFF ), sizeof( struct directory_entry ) );
	}
	else
		MemSet( &file->dirent_key, 0, sizeof( struct directory_entry ) );
	file->vol = vol;
	file->fpi = 0;
	file->block = file->entry->first_block ^ file->dirent_key.first_block;
	AddLink( &vol->files, file );
	return file;
}

struct sack_vfs_file * CPROC sack_vfs_open( CTEXTSTR filename )
{
	return sack_vfs_openfile( l.default_volume, filename );
}


LOGICAL CPROC _sack_vfs_exists( struct volume *vol, CTEXTSTR file )
{
	struct directory_entry entkey;
	struct directory_entry *ent = ScanDirectory( vol, file, &entkey );
	if( ent ) return TRUE;
	return FALSE;
}

LOGICAL CPROC sack_vfs_exists( CTEXTSTR file )
{
	return _sack_vfs_exists( l.default_volume, file );
}


_32 CPROC sack_vfs_tell( struct sack_vfs_file *file )
{
	return file->fpi;
}

_32 CPROC sack_vfs_size( struct sack_vfs_file *file )
{
	return file->entry->filesize ^ file->dirent_key.filesize;
}

_32 CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence )
{
	_32 old_fpi = file->fpi;
	if( whence == SEEK_SET )
		file->fpi = pos;
	if( whence == SEEK_CUR )
		file->fpi += pos;
	if( whence == SEEK_END )
		file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;
	{
		if( ( file->fpi & ( ~0xFFF ) ) >= ( old_fpi & ( ~0xFFF ) ) )
		{
			do
			{
				if( ( file->fpi & ( ~0xFFF ) ) == ( old_fpi & ( ~0xFFF ) ) )
					return file->fpi;
				file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
				old_fpi += 4096;
			} while( 1 );
		}
	}
	{
		size_t n = 0;
		int b = file->entry->first_block ^ file->dirent_key.first_block;
		while( n * 4096 < ( pos & ~0xFFF ) )
		{
			b = GetNextBlock( file->vol, b, FALSE, TRUE );
			n++;
		}
		file->block = b;
	}
	return file->fpi;
}

static void MaskBlock( struct volume *vol, P_8 usekey, P_8 block, _32 block_ofs, _32 ofs, const char *data, size_t length )
{
	_32 n;
	if( vol->key )
		for( n = 0; n < length; n++ )
		{
			block[block_ofs] = data[n] ^ usekey[ofs];
			block_ofs++;
			ofs++;
		}
	else
		MemCpy( block + block_ofs, data, length );
}

_32 CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length )
{
	_32 written = 0;
	_32 ofs = file->fpi & 0xFFF;
	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( 4096 - ( ofs ) ) )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, ofs, ofs, data, 4096 - ofs );
			data += 4096 - ofs;
			written += 4096 - ofs;
			file->fpi += 4096 - ofs;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= 4096 - ofs;
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
		if( length >= 4096 )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], block, 0, 0, data, 4096 - ofs );
			data += 4096;
			written += 4096;
			file->fpi += 4096;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) )
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
			length -= 4096;
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

_32 CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length )
{
	_32 written = 0;
	_32 ofs = file->fpi & 0xFFF;
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) )
		length = ( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi;
	if( !length )
		return 0;

	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length >= ( 4096 - ( ofs ) ) )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, ofs, (const char*)(block+ofs), 4096 - ofs );
			written += 4096 - ofs;
			data += 4096 - ofs;
			length -= 4096 - ofs;
			file->fpi += 4096 - ofs;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
		}
		else
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			length = 0;
			file->fpi += length;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block, BLOCK_CACHE_FILE );
		if( length > 4096 )
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, 0, (const char*)block, 4096 - ofs );
			written += 4096;
			data += 4096;
			length -= 4096;
			file->fpi += 4096;
			file->block = GetNextBlock( file->vol, file->block, FALSE, TRUE );
		}
		else
		{
			MaskBlock( file->vol, file->vol->usekey[BLOCK_CACHE_FILE], (P_8)data, 0, 0, (const char*)block, length );
			written += length;
			length = 0;
			file->fpi += length;
		}
	}
	return written;
}

_32 sack_vfs_truncate( struct sack_vfs_file *file )
{
	file->entry->filesize = file->fpi;
	return file->fpi;
}

int sack_vfs_close( struct sack_vfs_file *file )
{
	DeleteLink( &file->vol->files, file );
	Release( file );
	return 0;
}

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
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> 10 ) * ( 1025 * 4096 ) ), BLOCK_CACHE_FILE );
			_32 _thiskey;
			_thiskey = ( vol->key )?((P_32)vol->usekey[BLOCK_CACHE_FILE])[_block & 0x3FF]:0;
			block = GetNextBlock( vol, block, FALSE, FALSE );
			this_BAT[_block & 0x3ff] = _thiskey;
			_block = block;
		}while( block != 0xFFFFFFFF );
	}
}

void sack_vfs_unlink( CTEXTSTR filename )
{
	sack_vfs_unlink_file( l.default_volume, filename );
}

int sack_vfs_flush( struct sack_vfs_file *file )
{
	/* noop */
	return 0;
}

static LOGICAL CPROC sack_vfs_need_copy_write( void )
{
	return FALSE;
}

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
                                                   };

PRIORITY_PRELOAD( Sack_VFS_Register, SQL_PRELOAD_PRIORITY )
{
	sack_register_filesystem_interface( SACK_VFS_FILESYSTEM_NAME, &sack_vfs_fsi );
}

SACK_VFS_NAMESPACE_END
