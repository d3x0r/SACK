/*
 512 // proprietary header/configuration
 512 // root directory entries
 2048 // skip
 4096 // first fat block - 32 bit entries; next offset from 'here' in each spot 'here'

 ... data...
 */

#include <stdhdrs.h>
#include <filesys.h>
#include <procreg.h>
#include <sack_vfs.h>

SACK_VFS_NAMESPACE

typedef _32 BLOCKINDEX; //
typedef _32 FPI; // file position type

PREFIX_PACKED struct volume {
	CTEXTSTR volname;
	struct disk *disk;
	DWORD dwSize;
} PACKED;

PREFIX_PACKED struct directory_entry
{
	FPI name_offset;
	BLOCKINDEX first_block;
	_32 filesize;
	_32 filler;
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
	struct volume *vol; // which volume this is in
	FPI fpi;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
}  ;

static struct {
	struct volume *default_volume;
} l;

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

	vol->disk = (struct disk*)OpenSpaceExx( NULL, vol->volname, 0, &vol->dwSize, &created );
	if( !oldsize )
	{
		MemSet( vol->disk, 0, vol->dwSize );
		vol->disk->BAT[0] = 0xFFFFFFFF;  // allocate 1 directory entry block
		vol->disk->BAT[1] = 0xFFFFFFFF;  // allocate 1 name block
	}
	else
	{
		if( oldsize )
			MemSet( ((P_8)vol->disk) + oldsize, 0, vol->dwSize - oldsize );
	}
}



static PTRSZVAL SEEK( struct volume *vol, _32 offset )
{
	while( offset >= vol->dwSize )
		ExpandVolume( vol );
	return ((PTRSZVAL)vol->disk) + offset;
}

static PTRSZVAL BSEEK( struct volume *vol, _32 block )
{
	_32 b = 4096 + (block >> 10) * (4096*1025) + ( block & 0x3FF ) * 4096;
	while( b >= vol->dwSize )
		ExpandVolume( vol );
	return ((PTRSZVAL)vol->disk) + b;
}

#define TSEEK(type,v,o) ((type)SEEK(v,o))
#define BTSEEK(type,v,o) ((type)BSEEK(v,o))

static _32 GetFreeBlock( struct volume *vol )
{
	int n;
	int b = 0;
	BLOCKINDEX *current_BAT = vol->disk->BAT;
	do
	{
		for( n = 0; n < 1024; n++ )
		{
			if( !current_BAT[n] )
			{
				// mark it as claimed; will be enf of file marker...
				// adn thsi result will overwrite previous EOF.
				current_BAT[n] = 0xFFFFFFFF;
				return b * 1024 + n;
			}
		}
		b++;
		current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( 4096 * ( 1 + 1024 ) ) );
	}while( 1 );
}

static _32 GetNextBlock( struct volume *vol, _32 block, LOGICAL expand )
{
	_32 sector = block >> 10;
	_32 *this_BAT = TSEEK( _32 *, vol, sector * ( 4096 * 1025 ) );
	if( this_BAT[block & 0x3FF] == 0xFFFFFFFF )
	{
		if( expand )
			this_BAT[block & 0x3FF] = GetFreeBlock( vol );
	}
	return this_BAT[block & 0x3FF];
}


struct volume *sack_vfs_load_volume( CTEXTSTR filepath )
{
	struct volume *vol = New( struct volume );
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveText( filepath );
	ExpandVolume( vol );
	l.default_volume = vol;
	return vol;
}

static struct directory_entry * ScanDirectory( struct volume *vol, CTEXTSTR filename )
{
	int n;
	int this_dir_block = 0;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block );
		for( n = 0; n < ENTRIES; n++ )
		{
			struct directory_entry *ent = next_entries + n;
			CTEXTSTR testname;
			if( !ent->name_offset )
				return NULL;
			testname = TSEEK( CTEXTSTR, vol, ent->name_offset );
			if( CompareMask( filename, testname, FALSE ) )
				return ent;
		}
		this_dir_block = GetNextBlock( vol, this_dir_block, TRUE );
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI SaveFileName( struct volume *vol, CTEXTSTR filename )
{
	int n;
	int this_name_block = 1;
	while( 1 )
	{
		TEXTSTR names = BTSEEK( TEXTSTR, vol, this_name_block );
		TEXTSTR name = names;
		while( name < ( names + 4096 ) )
		{
			if( !name[0] )
			{
				size_t namelen;
				if( ( namelen = StrLen( filename ) ) < ( ( names + 4096 ) - name ) )
				{
					MemCpy( name, filename, ( namelen + 1 ) * sizeof( TEXTCHAR )  );
					return ((PTRSZVAL)name) - ((PTRSZVAL)vol->disk);
				}
			}
			else
				if( StrCaseCmp( name, filename ) == 0 )
					return name - names + ( this_name_block * 4096 );
			name = name + StrLen( name ) + 1;
		}
		this_name_block = GetNextBlock( vol, this_name_block, TRUE );
	}
}


static struct directory_entry * GetNewDirectory( struct volume *vol, CTEXTSTR filename )
{
	int n;
	int this_dir_block = 0;
	struct directory_entry *next_entries;
	do
	{
		next_entries = BTSEEK( struct directory_entry *, vol, this_dir_block );
		for( n = 0; n < ENTRIES; n++ )
		{
			struct directory_entry *ent = next_entries + n;
            // not name_offset (end of list) or not first_block(free entry) use this block
			if( ent->name_offset && ent->first_block )
				continue;
			ent->name_offset = SaveFileName( vol, filename );
			ent->first_block = GetFreeBlock( vol );
  			return ent;
		}
		this_dir_block = GetNextBlock( vol, this_dir_block, TRUE );
	}
	while( 1 );

}

struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, CTEXTSTR filename )
{
	struct sack_vfs_file *file = New( struct sack_vfs_file );
	file->entry = ScanDirectory( vol, filename );
	if( !file->entry )
		file->entry = GetNewDirectory( vol, filename );
	file->vol = vol;
	file->fpi = 0;
	file->block = file->entry->first_block;
	return file;
}

struct sack_vfs_file * CPROC sack_vfs_open( CTEXTSTR filename )
{
	return sack_vfs_openfile( l.default_volume, filename );
}


LOGICAL CPROC _sack_vfs_exists( struct volume *vol, CTEXTSTR file )
{
	struct directory_entry *ent = ScanDirectory( vol, file );
	if( ent )
		return TRUE;
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
	return file->entry->filesize;
}

_32 CPROC sack_vfs_seek( struct sack_vfs_file *file, S_32 pos, int whence )
{
	if( whence == SEEK_SET )
		file->fpi = pos;
	if( whence == SEEK_CUR )
		file->fpi += pos;
	if( whence == SEEK_END )
		file->fpi = file->entry->filesize + pos;

	{
		int n = 0;
		int b = file->entry->first_block;
		while( n * 4096 < ( pos & ~0x3FF ) )
		{
			b = GetNextBlock( file->vol, b, TRUE );
			n++;
		}
		file->block = b;
	}
}

_32 CPROC sack_vfs_write( struct sack_vfs_file *file, P_8 data, _32 length )
{
	_32 written = 0;
	_32 ofs = file->fpi & 0xFFF;
	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block );
		if( length > ( 4096 - ( ofs ) ) )
		{
			MemCpy( block + ofs, data, ( 4096 - ofs ) );
			data += 4096 - ofs;
			written += 4096 - ofs;
			file->fpi += 4096 - ofs;
			if( file->fpi > file->entry->filesize )
				file->entry->filesize = file->fpi;
			file->block = GetNextBlock( file->vol, file->block, TRUE );
			length -= 4096 - ofs;
		}
		else
		{
			MemCpy( block + ofs, data, length );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > file->entry->filesize )
				file->entry->filesize = file->fpi;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block );
		if( length > 4096 )
		{
			MemCpy( block, data, ( 4096 - ofs ) );
			data += 4096;
			written += 4096;
			file->fpi += 4096;
			if( file->fpi > file->entry->filesize )
				file->entry->filesize = file->fpi;
			file->block = GetNextBlock( file->vol, file->block, TRUE );
			length -= 4096;
		}
		else
		{
			MemCpy( block, data, length );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > file->entry->filesize )
				file->entry->filesize = file->fpi;
			length = 0;
		}
	}
	return written;
}

_32 sack_vfs_read( struct sack_vfs_file *file, P_8 data, _32 length )
{
	_32 written = 0;
	_32 ofs = file->fpi & 0xFFF;
	if( file->entry->filesize < ( file->fpi + length ) )
		length = file->entry->filesize - file->fpi;
	if( !length )
		return 0;

	if( ofs )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block );
		if( length > ( 4096 - ( ofs ) ) )
		{
			MemCpy( data, block + ofs, ( 4096 - ofs ) );
			written += 4096 - ofs;
			data += 4096 - ofs;
			length -= 4096 - ofs;
			file->fpi += 4096 - ofs;
			file->block = GetNextBlock( file->vol, file->block, TRUE );
		}
		else
		{
			MemCpy( data, block + ofs, length );
			written += length;
			length = 0;
			file->fpi += length;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length )
	{
		P_8 block = (P_8)BSEEK( file->vol, file->block );
		if( length > 4096 )
		{
			MemCpy( data, block, ( 4096 - ofs ) );
			written += 4096;
			data += 4096;
			length -= 4096;
			file->fpi += 4096;
			file->block = GetNextBlock( file->vol, file->block, TRUE );
		}
		else
		{
			MemCpy( data, block, length );
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
}

void sack_vfs_close( struct sack_vfs_file *file )
{
	Release( file );
}

void sack_vfs_unlink( CTEXTSTR filename )
{
	struct directory_entry *entry = ScanDirectory( l.default_volume, filename );
	if( entry )
	{
		int block, _block;
		entry->name_offset = 0;
		_block = block = entry->first_block;
		// wipe out file chain BAT
		do
		{
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, l.default_volume, ( ( block >> 10 ) * ( 1025 * 4096 ) ) );

			if( block != 0xFFFFFFFF )
			{
				block = GetNextBlock( l.default_volume, block, FALSE );
			}
			this_BAT[_block & 0x3ff] = 0;
		}while( block != 0xFFFFFFFF );
	}
}

int sack_vfs_flush( struct sack_vfs_file *file )
{
	/* noop */
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
};

PRIORITY_PRELOAD( Sack_VFS_Register, SQL_PRELOAD_PRIORITY )
{
	sack_register_filesystem_interface( SACK_VFS_FILESYSTEM_NAME, &sack_vfs_fsi );
}

SACK_VFS_NAMESPACE_END
