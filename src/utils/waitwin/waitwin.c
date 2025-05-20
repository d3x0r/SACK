
#include <stdio.h>
#include <windows.h>
#include <stdint.h>

char * WcharConvert_v2 ( const wchar_t *wch, size_t len, size_t *outlen );
wchar_t * CharWConvertExx ( const char *wch, size_t len  );


BOOL CALLBACK enumFunc(
  _In_ HWND   hwnd,
  _In_ LPARAM lParam
							 ){
   printf( "Window %p   ", hwnd );
   if( !hwnd ) return 0;

   	wchar_t buf[256];
	int l = GetWindowTextW( hwnd, buf, 256 );
   size_t ol;
   char *buf2 = WcharConvert_v2( buf, l, &ol );
	printf( "Name:%.*s\n",ol, buf2 );

//   GetWindowTextA( hwnd, buf, 256 );
//	printf( "Name:%s\n", buf );

	return 1;
}

int main( int argc, char **argv )
{
	LPWSTR cmd = GetCommandLineW();

	int n;
		size_t ol;
	for( n = 0; cmd[n]; n++ ) {
		if( cmd[n] == ' ' ) { n++; break; }
	}
   char *buf2;
		if( cmd[n] == ' ' ) { n++; }
	if( cmd[n] == '\"' ) {
		n++;
		int i;
		for( i = n+1; cmd[i]; i++ ) {
			if( cmd[i] =='"' && cmd[i+1]!='"' ) { cmd[i] = 0; break; }
			printf( "%c", cmd[i] );
		}
		buf2 = WcharConvert_v2( cmd + n, i-n, &ol );
	} else {
		int i;
		for( i = n; cmd[i]; i++ ) {
			if( cmd[i] == 's' && cmd[i+1] == 't' && cmd[i+2] == 'a' &&cmd[i+3] == 'r' &&cmd[i+4] == 't'&&cmd[i+5] == 'e'&&cmd[i+6] == 'd' ) {
            if( cmd[i-1] == ' ' )
					i--;
            break;
			}
		}
		buf2 = WcharConvert_v2( cmd + n, i-n, &ol );
	}



	if( cmd[n] )
	{
      int wait_exit = 1;
		wchar_t * check = CharWConvertExx( buf2, ol );
		printf( "waiting for [%s]\n", buf2 );

		fflush( stdout );
		if( argc > 2 && argv[2] )
			if( stricmp( argv[2], "started" ) == 0 )
			{
				wait_exit = 0;
				while( !FindWindowW( NULL, check ) )
					Sleep( 250 );
			}
      if( wait_exit )
			while( FindWindowW( NULL, check ) )
				Sleep( 250 );
	}
	else
	{
		printf( "%s <window title> <started>\n"
				 " - while a window with the title exists, this waits.\n"
				 " - if 'started' is specified as a second argument, "
				 "   then this waits for the window to exist instead of waiting for it to close\n"
				, argv[0] );
  		EnumWindows(enumFunc, 0 );
	}
   return 0;
}



char * WcharConvert_v2 ( const wchar_t *wch, size_t len, size_t *outlen )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the
	// conversion functions such as:
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t  sizeInBytes;
	char  tmp[2];
	char	 *ch;
	char	 *_ch;
	const wchar_t *_wch = wch;
	sizeInBytes = 1; // start with 1 for the ending nul
	_ch = ch = tmp;
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//printf( "wch = %04x\n", wch[0] );
			if( !( wch[0] & 0xFF80 ) )
			{
			  // printf( "1 byte encode...\n" );
				sizeInBytes++;
			}
			else if( !( wch[0] & 0xF800 ) )
			{
			  // printf( "2 byte encode...\n" );
				sizeInBytes += 2;
			}
			else if( (  ( ( wch[0] & 0xFC00 ) >= 0xD800 )
					   && ( ( wch[0] & 0xFC00 ) < 0xDC00 ) )
					 && ( ( ( wch[1] & 0xFC00 ) >= 0xDC00 )
					   && ( ( wch[1] & 0xFC00 ) < 0xE000 ) )
					 )
			{
				int longer_value = 0x10000 + ( ( ( wch[0] & 0x3ff ) << 10 ) | ( ( wch[1] & 0x3ff ) ) );
			  // printf( "3 or 4 byte encode...\n" );
				if( !(longer_value & 0xFFFF0000 ) )
					sizeInBytes += 3;
				else if( ( longer_value >= 0xF0000 ) && ( longer_value < 0xF0800 ) ) // hack a way to encode D800-DFFF
					sizeInBytes += 2;
				else
					sizeInBytes += 4;
				wch++;
			}
			else
			{
				// just encode the 16 bits as it is.
			//	printf( " 3 byte encode?\n" );
				sizeInBytes+= 3;
			}
			wch++;
		}
	}
	wch = _wch;
	_ch = ch = malloc(sizeInBytes);
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			{
				if( !( wch[0] & 0xFF80 ) )
				{
					(*ch++) = ((unsigned char*)wch)[0];
				}
				else if( !( wch[0] & 0xFF00 ) )
				{
					//(*ch++) = ((unsigned char*)wch)[0];
					(*ch++) = 0xC0 | ( ( ((unsigned char*)wch)[1] & 0x7 ) << 2 ) | ( ( ((unsigned char*)wch)[0] ) >> 6 );
					(*ch++) = 0x80 | ( ((unsigned char*)wch)[0] & 0x3F );
				}
				else if( !( wch[0] & 0xF800 ) )
				{
					(*ch++) = 0xC0 | ( ( ((unsigned char*)wch)[1] & 0x7 ) << 2 ) | ( ( ((unsigned char*)wch)[0] ) >> 6 );
					(*ch++) = 0x80 | ( ((unsigned char*)wch)[0] & 0x3F );
				}
				else if( (  ( ( wch[0] & 0xFC00 ) >= 0xD800 )
							 && ( ( wch[0] & 0xFC00 ) < 0xDC00 ) )
						  && ( ( ( wch[1] & 0xFC00 ) >= 0xDC00 )
								&& ( ( wch[1] & 0xFC00 ) < 0xE000 ) )
					 )
				{
					uint32_t longer_value;
					longer_value = 0x10000 + ( ( ( wch[0] & 0x3ff ) << 10 ) | ( ( wch[1] & 0x3ff ) ) );
					if( ( longer_value >= 0xF0000 ) && ( longer_value < 0xF0800 ) ) // hack a way to encode D800-DFFF
					{
						longer_value = ( longer_value - 0xF0000 ) + 0xD800;
						sizeInBytes += 2;
					}
					wch++;
					if( !(longer_value & 0xFFFF ) )
					{
						// 16 bit encoding (shouldn't be hit
						(*ch++) = 0xE0 | (char)( ( longer_value >> 12 ) & 0x0F );
						(*ch++) = 0x80 | (char)( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 0 ) & 0x3f );
					}
					else if( !( longer_value & 0xFFE00000 ) )
					{
						// 21 bit encoding ...
						(*ch++) = 0xF0 | (char)( ( longer_value >> 18 ) & 0x07 );
						(*ch++) = 0x80 | (char)( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 0 ) & 0x3f );
					}
					/*  ** functionally removed from spec ..... surrogates cannot be this long.
					else if( !( longer_value & 0xFC000000 ) )
					{
						(*ch++) = 0xF8 | ( longer_value >> 24 );
						(*ch++) = 0x80 | ( ( longer_value >> 18 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 0 ) & 0x3f );
					}
					else  if( !( longer_value & 0x80000000 ) )
					{
						// 31 bit encode
						(*ch++) = 0xFC | ( longer_value >> 30 );
						(*ch++) = 0x80 | ( ( longer_value >> 24 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 18 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 0 ) & 0x3f );
					}
					*/
					else
					{
						// too long to encode.
					}
				}
				else
				{
					   //lprintf( " 3 byte encode?  16 bits" );
						(*ch++) = 0xE0 | ( ( wch[0] >> 12 ) & 0x0F ); // mask just in case of stupid compiles that tread wchar as signed?
						(*ch++) = 0x80 | ( ( wch[0] >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( wch[0] >> 0 ) & 0x3f );
				}
			}
			wch++;
		}
	}
	(*ch) = 0;
	if( outlen ) outlen[0] = ch - _ch;
	ch = _ch;
	return ch;
}


wchar_t * CharWConvertExx ( const char *wch, size_t len )
{
	// Conversion to wchar_t* :
	// Can just convert wchar_t* to char* using one of the
	// conversion functions such as:
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t  sizeInChars;
	const char *_wch;
	wchar_t	*ch;
	wchar_t   *_ch;
	if( !wch ) return NULL;
	sizeInChars = 0;
	_wch = wch;
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//lprintf( "first char is %d (%08x)", wch[0] );
			if( (wch[0] & 0xE0) == 0xC0 )
				wch += 2;
			else if( (wch[0] & 0xF0) == 0xE0 )
				wch += 3;
			else if( (wch[0] & 0xF0) == 0xF0 )
			{
				sizeInChars++;
				wch += 4;
			}
			else
				wch++;
			sizeInChars++;
		}
	}
	wch = _wch;
	_ch = ch = malloc( sizeInChars + 1 );
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//lprintf( "first char is %d (%08x)", wch[0] );
			if( ( wch[0] & 0xE0 ) == 0xC0 )
			{
				ch[0] = ( ( (wchar_t)wch[0] & 0x1F ) << 6 ) | ( (wchar_t)wch[1] & 0x3f );
				wch += 2;
			}
			else if( ( wch[0] & 0xF0 ) == 0xE0 )
			{
				ch[0] = ( ( (wchar_t)wch[0] & 0xF ) << 12 )
					| ( ( (wchar_t)wch[1] & 0x3F ) << 6 )
					| ( (wchar_t)wch[2] & 0x3f );
				wch += 3;
			}
			else if( ( wch[0] & 0xF0 ) == 0xF0 )
			{
				uint32_t literal_char =  ( ( (wchar_t)wch[0] & 0x7 ) << 18 )
				                 | ( ( (wchar_t)wch[1] & 0x3F ) << 12 )
				                 | ( (wchar_t)wch[2] & 0x3f ) << 6
				                 | ( (wchar_t)wch[3] & 0x3f );
				//lprintf( "literal char is %d (%08x", literal_char, literal_char );
				ch[0] = 0xD800 + ( ( ( literal_char - 0x10000 ) & 0xFFC00 ) >> 10 );// ((wchar_t*)&literal_char)[0];
				ch[1] = 0xDC00 + ( ( literal_char - 0x10000 ) & 0x3ff );// ((wchar_t*)&literal_char)[1];
				ch++;
				wch += 4;
			}
			else
			{
				ch[0] = wch[0] & 0x7f;
				wch++;
			}
			ch++;
		}
		ch[0] = 0;
	}
	return _ch;
}

