#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>

PRELOAD( loginfo )
{
	FILE *input[4];
	TEXTCHAR buffer[4][256];
	TEXTSTR file_name;

	file_name = ExpandPath( WIDE("@/info/") WIDE("generator.txt") );
	input[0] = sack_fopen( 0, file_name, WIDE("rt") );
	Release( file_name );

	file_name = ExpandPath( WIDE("@/info/") WIDE("type.txt") );
	input[1] = sack_fopen( 0, file_name, WIDE("rt") );
	Release( file_name );

	file_name = ExpandPath( WIDE("@/info/") WIDE("ver.txt") );
	input[2] = sack_fopen( 0, file_name, WIDE("rt") );
	Release( file_name );

	file_name = ExpandPath( WIDE("@/info/") WIDE("project.txt") );
	input[3] = sack_fopen( 0, file_name, WIDE("rt") );
	Release( file_name );

	if( input[0] && input[1] && input[2] )
	{
		int j = 1;
		while( fgets( buffer[0], sizeof( buffer[0] ), input[0] )
				&& fgets( buffer[1], sizeof( buffer[1] ), input[1] )
				&& fgets( buffer[2], sizeof( buffer[2] ), input[2] )
			  )
		{
			int tmp_len;
			if( (tmp_len = strlen( buffer[0] ) ), buffer[0][tmp_len-1] == '\n' )
            buffer[0][tmp_len-1] = 0;
			if( (tmp_len = strlen( buffer[1] ) ), buffer[1][tmp_len-1] == '\n' )
            buffer[1][tmp_len-1] = 0;
			if( (tmp_len = strlen( buffer[2] ) ), buffer[2][tmp_len-1] == '\n' )
            buffer[2][tmp_len-1] = 0;
			if( input[3] && fgets( buffer[3], sizeof( buffer[3] ), input[3] ) )
			{
				if( (tmp_len = strlen( buffer[3] ) ), buffer[3][tmp_len-1] == '\n' )
					buffer[3][tmp_len-1] = 0;
				lprintf( WIDE("Version %s: %s[%s]%s"), buffer[3], buffer[0], buffer[1], buffer[2] );
			}
			else
				lprintf( WIDE("Version %d: %s[%s]%s"), j, buffer[0], buffer[1], buffer[2] );
			j++;
		}
	}
	if( input[0] )
		fclose( input[0] );
	if( input[1] )
		fclose( input[1] );
	if( input[2] )
		fclose( input[2] );
	if( input[3] )
		fclose( input[3] );
}

#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 ) || defined( __WATCOMC__ )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
