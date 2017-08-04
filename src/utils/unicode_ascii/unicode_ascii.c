#include <stdhdrs.h>
#include <filesys.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>


struct rune_buffer {
	TEXTRUNE runes[16384];
	int used;
	struct rune_buffer *next;
};

static struct local_info
{
	struct rune_buffer *first, *last;

	char *char_buffer;
	wchar_t *wchar_buffer;
	size_t char_buffer_len;
	size_t wchar_buffer_len;
} l;

void pushRune( TEXTRUNE rune ) {
	if( !l.first ) {
		l.first = l.last = New( struct rune_buffer );
		l.first->next = NULL;
		l.first->used = 0;
	} else if( l.first->used == 16384 ) {
		l.last = ( l.last->next = New( struct rune_buffer ) );
		l.last->next = NULL;
		l.last->used = 0;
	}
	l.last->runes[l.last->used++] = rune;
}


void read_ascii( FILE *input )
{
	sack_fseek( input, 0, SEEK_END );
	l.char_buffer_len = sack_ftell( input );
	l.char_buffer = (char*)malloc( l.char_buffer_len );
	sack_fseek( input, 0, SEEK_SET );
	sack_fread( l.char_buffer, l.char_buffer_len, 1, input );
	
	const char *pos = l.char_buffer;
	TEXTRUNE rune;

	while( ( (size_t)( pos - l.char_buffer ) < l.char_buffer_len ) && ( rune = GetUtfChar( &pos ) ) )
		pushRune( rune );

}

void read_unicode( FILE *input )
{
	sack_fseek( input, 0, SEEK_END );
	l.wchar_buffer_len = sack_ftell( input );
	l.wchar_buffer = (wchar_t*)malloc( l.wchar_buffer_len );
	sack_fseek( input, 0, SEEK_SET );
	sack_fread( l.wchar_buffer, l.wchar_buffer_len, 1, input );

	const wchar_t *pos = l.wchar_buffer;
	l.wchar_buffer_len >>= 1;
	TEXTRUNE rune;
	if( pos[0] == 0xFEFF ) pos++;
	else if( pos[0] == 0xFEFF ) {
		printf( "big endian unicode not supported." );
		exit( 0 );
	}
	while( ( (size_t)( pos - l.wchar_buffer ) < l.wchar_buffer_len ) && ( rune = GetUtfCharW( &pos ) ) )
		pushRune( rune );
}

void write_ascii( FILE *input, LOGICAL overlong )
{
	sack_fseek( input, 0, SEEK_SET );
	{
		struct rune_buffer *outbuf = l.first;
		int c;
		while( outbuf ) {
			for( c = 0; c < outbuf->used; c++ ) {
				char buf[7];
				int chars = ConvertToUTF8Ex( buf, outbuf->runes[c], overlong );
				sack_fwrite( buf, 1, chars, input );
			}
			outbuf = outbuf->next;
		}
	}
}

void write_unicode( FILE *input )
{
	sack_fseek( input, 0, SEEK_SET );
	sack_fwrite( "\xFF\xFE", 1, 2, input );
	{
		struct rune_buffer *outbuf = l.first;
		int c;
		while( outbuf ) {
			for( c = 0; c < outbuf->used; c++ ) {
				wchar_t buf[7];
				int chars = ConvertToUTF16( buf, outbuf->runes[c] );
				sack_fwrite( buf, 2, chars, input );
			}
			outbuf = outbuf->next;
		}
	}
}

SaneWinMain( argc, argv )
{
	FILE *input;
	FILE *output;
	if( argc > 2 )
	{
		input = sack_fopen( 0, argv[2], WIDE("rb+") );
		if( argc > 3 )
			output = sack_fopen( 0, argv[3], "wb" );
		else
			output = input;
		if( input )
		{
			if( argv[1][0] == 'u' || argv[1][0] == 'U' )
			{
				read_ascii( input );
				//ascii_to_unicode();
				write_unicode( output );
			}
			else if( argv[1][0] == 'o' )
			{
				read_ascii( input );
				//ascii_to_overlong();
				write_ascii( output, TRUE );
			}
			else if( argv[1][0] == 'O' )
			{
				read_unicode( input );
				//unicode_to_overlong();
				write_ascii( output, TRUE );
			}
			else
			{
				if( argv[1][0] == 'a' )
					read_unicode( input );
				else if( argv[1][0] == 'A' )
					read_ascii( input );
				//unicode_to_ascii();
				write_ascii( output, FALSE );
			}
			sack_fclose( input );
			if( output != input )
				sack_fclose( output );
		}
		else
			printf( WIDE( "Failed to open %s" ), argv[2] );
	}
	else
	{
		printf( WIDE("Usage: %s [u/a] [filename]\n"), argv[0] );
		printf( WIDE( "  u or U read utf16 write utf8\n" ) );
		printf( WIDE( "  a read utf8 write utf16\n" ) );
		printf( WIDE( "  A read utf8 write utf8\n" ) );
		printf( WIDE("  o read ascii, output overlong utf8\n") );
		printf( WIDE("  O read unicode, output overlong utf8\n") );
		printf( WIDE("  ascii translates from unicode to ascii\n") );
		printf( WIDE("  file will be written back in-place\n") );
	}
	return 0;
}
EndSaneWinMain()

