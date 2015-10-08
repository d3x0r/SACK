
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
				char buf[256];
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
