/*
 512 // proprietary header/configuration
 512 // root directory entries
 2048 // skip
 4096 // first fat block - 32 bit entries; next offset from 'here' in each spot 'here'

 ... data...
 */

#include <stdhdrs.h>

#define SEEK(v,o) (((PTRSZVAL)(vol->disk)) + o)

PREFIX_PACKED struct volume {
   CTEXTSTR volname;
	struct disk *disk;
   DWORD dwSize;
} PACKED;

PREFIX_PACKED struct directory_entry
{
	_32 name_offset;
   _32 first_block;
	_32 filesize;
   _32 filler;
} PACKED;
#define ENTRIES ( 4096/sizeof( struct directory_entry) )

struct disk
{
	struct directory_entry directory[4096/sizeof( struct directory_entry)]; // 256
   _32 name_blocks[1024];
   _32 BAT[4096];
};

struct sack_vfs_file
{
	struct directory_entry *entry;
	_32 FPI;

}  ;

static _32 ExtendBAT( struct volume *vol )
{
	int n;
	_32 *current_BAT = vol->BAT;
	do
	{
		if( current_BAT[0] )
         current_BAT = SEEK( vol->disk, current_BAT[0] * 4096 );
		for( n = 0; n < 4096; n++ )
		{
         if( current_BAT[0]
		}
   SEEK( vol->disk, vol->disk
}

static _32 GetBlock( struct volume *vol )
{
	int n;
	_32 *current_BAT = vol->BAT;
	do
	{
		for( n = 1; n < 4096; n++ )
		{
			if( !current_BAT[n] )
			{
            current_BAT[n] = 0xFFFFFFFF;
				return ( ( (PTRSZVAL)current_BAT - ((PTRSZVAL)vol->disk) ) / 4096 << 12 ) + n;
			}
		}
		if( current_BAT[0] )
			current_BAT = SEEK( vol->disk, current_BAT[0] * 4096 );
	}
   return 0;
}

static _32 GetNextBlock( struct volume *vol, _32 block )
{
	_32 sector = block & 0xFFFFF000;
	_32 *this_BAT = SEEK( vol->disk, sector );
	if( this_BAT[block & 0xFFF] == 0xFFFFFFFF )
	{
      this_BAT[block & 0xFFF] = GetBlock( vol );
	}
   return this_BAT[block & 0xFFF];
}

static void ExpandVolume( struct volume *vol )
{
	LOGICAL created;
	_32 oldsize = vol->dwSize;
	if( oldsize )
	{
		CloseSpace( vol->disk );
		vol->dwSize += 4096*4096;
	}
	else
		vol->dwSize = 4096 + 4096 + ( 4096 * 16 ) + ( 4096 * 16 );
	vol->disk = (struct disk*)OpenSpace( NULL, vol->volname, 0, &vol->dwSize, &created );
	if( created )
	{
      MemSet( vol, 0, vol->dwSize );
	}
	else
	{
      MemSet( ((P_8)vol) + oldsize, 0, vol->dwSize - oldsize );
	}
}


struct volume *LoadVolume( CTEXTSTR filepath )
{
	struct volume *vol = New( struct volume );
	vol->dwSize = 0;
	vol->disk = 0;
	vol->volname = SaveName( filepath );
   return vol;
}

static struct directory_entry * ScanDirectory( struct volume *vol, CTEXTSTR filename )
{
	int n;
	struct directory_entry *next_entries = vol->disk->directory;
	do
	{
		for( n = 0; n < ENTRIES-1; n++ )
		{
			struct directory_entry *ent = next_entries + n;
			CTEXTSTR testname = SEEK( vol->disk->names, ent->name_offset - 1 );
			if( MaskCompare( filename, testname, FALSE ) )
				return ent;
		}
      if( ((struct directory_entry *)vol->disk->directory[n]).filesize )
			next_entries = SEEK( vol->disk, ((struct directory_entry *)vol->disk->directory[n]).filesize );
		else
         next_entries = NULL;
	}
	while( next_entries );
   return NULL;
}

static int SaveFileName( struct volume *vol, CTEXTSTR filename )
{
	int n;
	for( n = 0; n < 1024; n++ )
	{
		if( vol->name_blocks[n] )
		{
			CTEXTSTR names = SEEK( vol->disk, vol->name_blocks[n] * 4096 );
			CTEXTSTR name = names;
			if( n == 0 )
            name++;
			while( name < ( names + 65536 ) )
			{
				if( !name[0] )
				{
               size_t namelen;
					if( ( namelen = StrLen( filename ) ) > ( ( names + 65536 ) - name ) )
					{
						MemCpy( name, filename, ( namelen + 1 ) * sizeof( TEXTCHAR )  );
						return name - names + ( vol_name_blocks[n] * 4096 );
					}
				}
				else
					if( StrCaseCmp( name, filename ) == 0 )
						return name - names + ( vol_name_blocks[n] * 4096 );
			}
		}
		else
		{
         // need a new block of namespace.....
		}
	}
}


static struct directory_entry * GetNewDirectory( struct volume *vol, CTEXTSTR filename )
{
	int n;
	struct directory_entry *next_entries = vol->disk->directory;
	do
	{
		for( n = 0; n < ENTRIES-1; n++ )
		{
			struct directory_entry *ent = next_entries + n;
			if( ent->name_offset )
				continue;
         ent->name_offset = SaveFileName( vol, filename );
  			return ent;
		}
      if( ((struct directory_entry *)vol->disk->directory[n]).filesize )
			next_entries = SEEK( vol->disk, ((struct directory_entry *)vol->disk->directory[n]).filesize );
		else
         next_entries = NULL;
	}
	while( next_entries );

}

struct sack_vfs_file *OpenFile( struct volume *vol, CTEXTSTR filename )
{
   struct sack_vfs_file *file = New( struct sack_vfs_file );
	file->entry = ScanDirectory( vol, filename );
	if( !file->entry )
		file->entry = GetNewDirectory( vol, filename );
	file->FPI = 0;
   return file;
}

_32 sack_vfs_tell( struct sack_vfs_file *file )
{
   return file->FPI;
}

_32 sack_vfs_seek( struct sack_vfs_file *file, _32 pos )
{
   file->FPI = pos;
}

_32 sack_vfs_write( struct sack_vfs_file *file, POINTER data, _32 length )
{
}

_32 sack_vfs_read( struct sack_vfs_file *file, POINTER data, _32 length )
{
}


