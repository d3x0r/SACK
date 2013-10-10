#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

static struct local_info
{
	char *char_buffer;
   wchar_t *wchar_buffer;
	int char_buffer_len;
   int wchar_buffer_len;
} l;



int read_ascii( FILE *input )
{
	fseek( input, 0, SEEK_END );
	l.char_buffer_len = ftell( input );
	l.char_buffer = (char*)malloc( l.wchar_buffer_len );
	fseek( input, 0, SEEK_SET );
	fread( l.char_buffer, 1, l.char_buffer_len, input );
	return 0;
}

int read_unicode( FILE *input )
{
	fseek( input, 0, SEEK_END );
	l.wchar_buffer_len = ftell( input );
	l.wchar_buffer = (wchar_t*)malloc( l.wchar_buffer_len );
	fseek( input, 0, SEEK_SET );
	fread( l.wchar_buffer, 1, l.wchar_buffer_len, input );
	return 0;
}

int write_ascii( FILE *input )
{
	fseek( input, 0, SEEK_SET );
	fwrite( l.char_buffer, 1, l.char_buffer_len, input );
	return 0;
}

int write_unicode( FILE *input )
{
	fseek( input, 0, SEEK_SET );
   fwrite( "\xFE\xFF", 1, 2, input );
   fwrite( l.wchar_buffer, 1, l.wchar_buffer_len, input );
   return 0;
}

void unicode_to_ascii ( void  )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	int len;
	size_t convertedChars = 0;
	size_t  sizeInBytes;
#if defined( _MSC_VER)
	errno_t err;
#else
	int err;
#endif
	len = l.wchar_buffer_len;

	sizeInBytes = ((len + 1) * sizeof( char ));
   l.char_buffer_len = sizeInBytes;
	err = 0;
	l.char_buffer = (char*)malloc( sizeInBytes);
#if defined( _MSC_VER )
	err = wcstombs_s(&convertedChars, 
                    l.char_buffer, sizeInBytes,
						  l.wchar_buffer, sizeInBytes);
#else
	convertedChars = wcstombs( l.char_buffer, l.wchar_buffer, l.char_buffer_len);
   err = ( convertedChars == -1 );
#endif
	if (err != 0)
		lprintf( WIDE("wcstombs_s  failed!") );

}

void ascii_to_unicode( void )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	int len;
	size_t convertedChars = 0;
#if defined( _MSC_VER)
	errno_t err;
#else
	int err;
#endif
    len = l.char_buffer_len;
	l.wchar_buffer_len = (len * sizeof( wchar_t ) );
	err = 0;
	l.wchar_buffer = (wchar_t*)malloc( l.wchar_buffer_len + 4);
#if defined( _MSC_VER )
	err = mbstowcs_s(&convertedChars, 
                    l.wchar_buffer, (l.wchar_buffer_len+4)/sizeof(wchar_t),
						  l.char_buffer, l.char_buffer_len );
#else
	convertedChars = mbstowcs( l.wchar_buffer, l.char_buffer, l.wchar_buffer_len);
    err = ( convertedChars == -1 );
#endif
	if (err != 0)
		lprintf( WIDE( "wcstombs_s  failed!" ));

}

SaneWinMain( argc, argv )
{
	FILE *input;
	if( argc > 2 )
	{
		input = sack_fopen( 0, argv[2], WIDE("rb+") );
		if( input )
		{
			if( StrCaseCmpEx( argv[1], WIDE("u"), 1 ) == 0 )
			{
				read_ascii( input );
				ascii_to_unicode();
				write_unicode( input );
			}
         else
			{
				read_unicode( input );
				unicode_to_ascii();
				write_ascii( input );
			}
			fclose( input );
		}
	}
	else
	{
		printf( WIDE("Usage: %s [u/a] [filename]\n"), argv[0] );
		printf( WIDE("  u or a is unicode or ascii mode; unicode translates from ascii to unicode\n") );
		printf( WIDE("  ascii translates from unicode to ascii\n") );
		printf( WIDE("  file will be written back in-place\n") );
	}
	return 0;
}
EndSaneWinMain()

