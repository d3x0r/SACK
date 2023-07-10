/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   created to provide standard memory allocation features.
 *   Release( Allocate(size) )
 *   Hold( pointer ); // must release a second time.
 *   if DEBUG, memory bounds checking enabled and enableable.
 *   if RELEASE standard memory includes no excessive operations
 *
 *  standardized to never use int. (was a clean port, however,
 *  inaccurate, knowing the conversions on calculations of pointers
 *  are handled by cast to int! )
 *
 * see also - include/sharemem.h
 *
 */

#include <stdhdrs.h>
#include <ctype.h>
#include <sharemem.h>

SACK_MEMORY_NAMESPACE

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if defined( __GNUC__ )
#  pragma GCC push_options
#  pragma GCC optimize ("O0")
#endif

void  MemSet ( POINTER p, uintptr_t n, size_t sz )
{
#if defined( _MSC_VER ) && !defined( __NO_WIN32API__ ) && !defined( UNDER_CE )
#  if defined( _WIN64 )
	//__asm cld;
	__stosq( (uint64_t*)p, n, sz/8 );
	if( sz & 4 )
		(*(uint32_t*)( ((uintptr_t)p) + sz - (sz&7) ) ) = (uint32_t)n;
	if( sz & 2 )
		(*(uint16_t*)( ((uintptr_t)p) + sz - (sz&3) ) ) = (uint16_t)n;
	if( sz & 1 )
		(*(uint8_t*)( ((uintptr_t)p) + sz - (sz&1) ) ) = (uint8_t)n;
#  else
#    ifdef __64__
	__stosq( (uint64_t*)p, n, sz / 4 );
	if( sz & 4 )
		(*(uint32_t*)( ((uintptr_t)p) + sz - (sz&7) ) ) = (uint32_t)n;
	if( sz & 2 )
		(*(uint16_t*)( ((uintptr_t)p) + sz - (sz&3) ) ) = (uint16_t)n;
	if( sz & 1 )
		(*(uint8_t*)( ((uintptr_t)p) + sz - (sz&1) ) ) = (uint8_t)n;
#    else
	__stosd( (DWORD*)p, n, sz / 4 );
	if( sz & 2 )
		(*(uint16_t*)( ((uintptr_t)p) + sz - (sz&3) ) ) = (uint16_t)n;
	if( sz & 1 )
		(*(uint8_t*)( ((uintptr_t)p) + sz - (sz&1) ) ) = (uint8_t)n;
#    endif
#  endif
#elif defined( __GNUC__ )
	{
      uintptr_t tmp = (uintptr_t)p;
#  ifdef __64__
		{
			int m; int len = sz/8;
			for( m = 0; m < len; m++ ) {
				((uint64_t*)tmp)[0] = n;
				tmp += 8;
			}
		}
		if( sz & 4 )
			(*(uint32_t*)( ((uintptr_t)p) + sz - (sz&7) ) ) = (uint32_t)n;
		if( sz & 2 )
			(*(uint16_t*)( ((uintptr_t)p) + sz - (sz&3) ) ) = (uint16_t)n;
		if( sz & 1 )
			(*(uint8_t*)( ((uintptr_t)p) + sz - 1 ) ) = (uint8_t)n;
#  else
		{
			int m; int len = sz/4;
			for( m = 0; m < len; m++ ) {
				((uint32_t*)tmp)[0] = n;
				tmp += 4;
			}
		}
		if( sz & 2 )
			(*(uint16_t*)( ((uintptr_t)p) + sz - (sz&3) ) ) = (uint16_t)n;
		if( sz & 1 )
			(*(uint8_t*)( ((uintptr_t)p) + sz - 1 ) ) = (uint8_t)n;
#  endif
	}
#else
   memset( p, n, sz );
#endif
}

#if defined( __GNUC__ )
#  pragma GCC pop_options
#endif


int  MemChk ( POINTER p, uintptr_t val, size_t sz )
{
#if defined( _MSC_VER ) && !defined( __NO_WIN32API__ ) && !defined( UNDER_CE )
	size_t n;
	uintptr_t *data = (uintptr_t*)p;
	for( n = 0; n < sz/sizeof(uintptr_t); n++, data++ )
		if( data[0] != val )
			return 0;
   return 1;
#else
	//   memset( p, n, sz );
   return 1;
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 void  MemCpy ( POINTER pTo, CPOINTER pFrom, size_t sz )
{
#if defined( _MSC_VER ) && !defined( __NO_WIN32API__ )&& !defined( UNDER_CE )

#  ifdef _WIN64
	__movsq( (uint64_t*)pTo, (uint64_t*)pFrom, sz/8 );
	if( sz & 4 )
		(*(uint32_t*)( ((uintptr_t)pTo) + sz - (sz&7) ) ) = (*(uint32_t*)( ((uintptr_t)pFrom) + sz - (sz&7) ) );
#  else
	__movsd( (DWORD*)pTo, (DWORD*)pFrom, sz/4 );
#  endif
	if( sz & 2 )
		(*(uint16_t*)( ((uintptr_t)pTo) + sz - (sz&3) ) ) = (*(uint16_t*)( ((uintptr_t)pFrom) + sz - (sz&3) ) );
	if( sz & 1 )
		(*(uint8_t*)( ((uintptr_t)pTo) + sz - (sz&1) ) ) = (*(uint8_t*)( ((uintptr_t)pFrom) + sz - (sz&1) ) );
#else
	memcpy( pTo, pFrom, sz );
#endif
}

int  MemCmp ( CPOINTER pOne, CPOINTER pTwo, size_t sz )
{
   // zero byte comparison, always same.
	if( !sz )
      return 0;
	if( !pOne && !pTwo )
		return 0;
	if( !pOne )
		return -1; // NULL < "anything"
	if( !pTwo )
		return 1;  // "anything" > NULL
	return memcmp( pOne, pTwo, sz );
}


TEXTSTR StrCpyEx( TEXTSTR s1, CTEXTSTR s2, size_t n )
{
	size_t x;
	if( !s1 ) return s1;
	if( !s2 ) { if( s1 ) { s1[0] = 0; return s1; } }
	for( x = 0; x < n && (s1[x]=s2[x]); x++ );
	if( n )
		s1[n-1] = 0;
	return s1;
}

#undef StrCpy
TEXTSTR StrCpy( TEXTSTR s1, CTEXTSTR s2 )
{
	int x;
	for( x = 0; (s1[x]=s2[x]); x++ );
	return s1;
}


CTEXTSTR StrChr( CTEXTSTR s1, TEXTCHAR c )
{
	CTEXTSTR p1 = s1;
	if( !p1 ) return NULL;
	while( p1[0] && p1[0] != c ) p1++;
	if( p1[0] )
		return p1;
	return NULL;
}

CTEXTSTR StrRChr( CTEXTSTR s1, TEXTRUNE c )
{
	TEXTRUNE c1;
	CTEXTSTR  p1 = s1;
	if( !p1 ) return NULL;
	while( GetUtfChar( &p1 ) ); p1--; // go back to \0
	while( p1 != s1 && ( c1 = GetPriorUtfChar( s1, &p1 ) ) != c ); 
	if( c1 == c )
		return p1;
	return NULL;
}

const wchar_t* StrRChrW( const wchar_t* s1, TEXTRUNE c ) {
	TEXTRUNE c1;
	const wchar_t* p1 = s1;
	if( !p1 ) return NULL;
	while( GetUtfCharW( &p1 ) ); p1--; // go back to \0
	while( p1 != s1 && ( c1 = GetPriorUtfCharW( s1, &p1 ) ) != c );
	if( c1 == c )
		return p1;
	return NULL;
}

#ifdef __cplusplus
TEXTSTR StrChr( TEXTSTR s1, TEXTCHAR c )
{
	TEXTSTR p1 = s1;
	if( !p1 ) return NULL;
	while( p1[0] && p1[0] != c ) p1++;
	if( p1[0] )
		return p1;
	return NULL;
}

TEXTSTR StrRChr( TEXTSTR s1, TEXTCHAR c )
{
	TEXTSTR  p1 = s1;
	if( !p1 ) return NULL;
	while( p1[0] ) p1++;
	while( p1 != s1 && p1[0] != c ) p1--;
	if( p1[0] == c )
		return p1;
	return NULL;
}

wchar_t* StrRChrW( wchar_t* s1, TEXTRUNE c ) {
	TEXTRUNE c1;
	wchar_t* p1 = s1;
	if( !p1 ) return NULL;
	while( GetUtfCharW( (const wchar_t**)&p1 ) ); p1--; // go back to \0
	while( p1 != s1 && ( c1 = GetPriorUtfCharW( s1, (const wchar_t**) & p1) ) != c );
	if( c1 == c )
		return p1;
	return NULL;
}

#endif

CTEXTSTR StrCaseStr( CTEXTSTR s1, CTEXTSTR s2 )
{
	int match = 0;
	CTEXTSTR p1, p2, began_at;
	if( !s1 )
		return NULL;
	if( !s2 )
		return s1;
	began_at = NULL;
	p1 = s1;
	p2 = s2;
	while( p1[0] && p2[0] )
	{
		if( p1[0] == p2[0] ||
         (p1[0] == '/' && p2[0] == '\\' ) ||
         (p1[0] == '\\' && p2[0] == '/' ) ||
			(((p1[0] >='a' && p1[0] <='z' )?p1[0]-('a'-'A'):p1[0])
			 == ((p2[0] >='a' && p2[0] <='z' )?p2[0]-('a'-'A'):p2[0]) ))
		{
			if( !match )
			{
				match++;
				began_at = p1;
			}
			//p2++;
		}	
		else
		{
			if( began_at )
			{
				p1 = began_at;  // start at the beginning again..
				p2 = s2; // reset searching string.
            began_at = NULL;
			}
			match = 0;
		}
		p1++;
      if( match )
			p2++;
	}
	if( !p2[0] )
		return began_at;
	return NULL;
}

CTEXTSTR StrStr( CTEXTSTR s1, CTEXTSTR s2 )
{
	int match = 0;
	CTEXTSTR p1, p2, began_at;
	if( !s1 )
		return NULL;
	if( !s2 )
		return s1;
	began_at = NULL;
	p1 = s1;
	p2 = s2;
	while( p1[0] && p2[0] )
	{
		if( p1[0] == p2[0] )
		{
			if( !match )
			{
				match++;
				began_at = p1;
			}
			//p2++;
		}	
		else
		{
			if( began_at )
			{
				p1 = began_at;  // start at the beginning again..
				p2 = s2; // reset searching string.
            began_at = NULL;
			}
			match = 0;
		}
		p1++;
      if( match )
			p2++;
	}
	if( !p2[0] )
		return began_at;
	return NULL;
}

#ifdef __cplusplus
TEXTSTR StrStr( TEXTSTR s1, CTEXTSTR s2 )
{
	int match = 0;
	TEXTSTR p1, began_at;
	CTEXTSTR p2;
	if( !s1 )
		return NULL;
	if( !s2 )
		return s1;
	began_at = p1 = s1; // set began_at here too..
	p2 = s2;
	while( p1[0] && p2[0] )
	{
		if( p1[0] == p2[0] )
		{
			if( !match )
			{
				match++;
				began_at = p1;
			}
		}
		else
		{
			if( began_at )
			{
				p1 = began_at;  // start at the beginning again..
				p2 = s2; // reset searching string.
            began_at = NULL;
			}
			match = 0;
		}
		p1++;
      if( match )
			p2++;
	}
	if( !p2[0] )
		return began_at;
	return NULL;
}
#endif


//------------------------------------------------------------------------------------------------------
// result in 0(equal), 1 above, or -1 below
// *r contains the position of difference
 int  CmpMem8 ( void *s1, void *s2, unsigned long n, unsigned long *r )
{
	int t1, t2;
	uint32_t pos;
	{
		pos = 0;
		while( pos < n )
		{
			t1 = *(unsigned char*)s1;
			t2 = *(unsigned char*)s2;
			if( ( t1 ) == ( t2 ) ) {
				(pos)++;
				s1 = (void*)(((uintptr_t)s1) + 1);
				s2 = (void*)(((uintptr_t)s2) + 1);
			} else if( t1 > t2 ) {
				if( r )
					*r = pos;
				return 1;
			} else {
				if( r )
					*r = pos;
				return -1;
			}
		}
	}
	if( r )
		*r = pos;
	return 0;
}

//------------------------------------------------------------------------------------------------------

 TEXTSTR  StrDupEx ( CTEXTSTR original DBG_PASS )
{
	if( original )
	{
		uintptr_t len = (uintptr_t)StrLen( original ) + 1;
		TEXTCHAR *result;
		result = (TEXTCHAR*)AllocateEx( sizeof(TEXTCHAR)*len  DBG_RELAY );
		MemCpy( result, original, sizeof(TEXTCHAR)*len );
		return result;
	}
	return NULL;
}

size_t StrLen( CTEXTSTR s )
{
	size_t l;
	if( !s )
		return 0;
	for( l = 0; s[0];l++, s++);
	return l;	
}

size_t StrBytesW( wchar_t const* s ) {
	size_t l;
	if( !s )
		return 0;
	for( l = 0; s[0]; s++ ) {
		l += 2;
	}
	return l+2;
}

size_t StrBytesWu8( wchar_t const* s ) {
	size_t l;
	char ch[4];
	TEXTRUNE r;
	if( !s )
		return 0;
	for( l = 0; r = GetUtfCharW( &s ); s++ ) {
		l += ConvertToUTF8( ch, r );
	}
	return l + 1;
}

size_t CStrLen( char const* s )
{
	size_t l;
	if( !s )
		return 0;
	for( l = 0; s[0];l++,s++);
	return l;	
}

#ifdef _UNICODE
char *  CStrDupEx ( CTEXTSTR original DBG_PASS )
{
	return WcharConvertEx( original DBG_RELAY );
}

TEXTSTR  DupCStrEx ( const char * original DBG_PASS )
{
	if( original )
		return CharWConvertEx( original DBG_RELAY );
	return NULL;
}

TEXTSTR  DupCStrLenEx( const char * original, size_t chars DBG_PASS )
{
	if( original )
		return CharWConvertExx( original, chars DBG_RELAY );
	return NULL;
}
#else


char *  CStrDupEx ( CTEXTSTR original DBG_PASS )
{
	INDEX len;
	char *result;
	if( !original )
		return NULL;
	for( len = 0; original[len]; len++ ); 
	result = (char*)AllocateEx( (len+1) * sizeof( result[0] ) DBG_RELAY );
	len = 0;
	while( ( result[len] = original[len] ) != 0 ) len++;
	return result;
}

TEXTSTR  DupCStrLenEx( const char * original, size_t chars DBG_PASS )
{
	size_t len = 0;
	TEXTSTR result, _result;
	if( !original )
		return NULL;
	_result = result = NewArray( TEXTCHAR, chars + 1 );// (TEXTSTR)AllocateEx( (len + 1) * sizeof( result[0] )  DBG_RELAY );
	len = 0;
	while( len < chars ) ((*result++) = (*original++)), len++;
	result[0] = 0;
	return _result;
}

TEXTSTR  DupCStrEx( const char * original DBG_PASS )
{
	size_t len = 0;
	const char *_original;
	if( !original )
		return NULL;
	_original = original;
	while( (*original++) ) len++;
	return DupCStrLenEx( _original, len DBG_RELAY );
}

#endif

wchar_t *   DupTextToWideEx( CTEXTSTR original DBG_PASS )
{
#ifdef _UNICODE
   return StrDupEx( original DBG_RELAY );
#else
   return CharWConvertEx( original DBG_RELAY );
#endif
}

char *     DupTextToCharEx( CTEXTSTR original DBG_PASS )
{
#ifdef _UNICODE
   return WcharConvertEx( original DBG_RELAY );
#else
   return StrDupEx( original DBG_RELAY );
#endif
}

TEXTSTR     DupWideToTextEx( const wchar_t * original DBG_PASS )
{
#ifdef _UNICODE
   return StrDupEx( original DBG_RELAY );
#else
   return WcharConvertEx( original DBG_RELAY );
#endif
}

TEXTSTR     DupCharToTextEx( const char * original DBG_PASS )
{
#ifdef _UNICODE
   return CharWConvertEx( original DBG_RELAY );
#else
   return StrDupEx( original DBG_RELAY );
#endif
}


 int  StrCmp ( CTEXTSTR s1, CTEXTSTR s2 )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
      return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && ( s1[0] == s2[0] ); s1++, s2++ );
	return s1[0] - s2[0];
}

 int  StrCaseCmp ( CTEXTSTR s1, CTEXTSTR s2 )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && (((s1[0] >='a' && s1[0] <='z' )?s1[0]-('a'-'A'):s1[0])
									 == ((s2[0] >='a' && s2[0] <='z' )?s2[0]-('a'-'A'):s2[0]) );
		  s1++, s2++ );
	return tolower(s1[0]) - tolower(s2[0]);
}


int  StrCaseCmp_u8u16( const char* s1, const wchar_t* s2 ) {
	TEXTRUNE c1;
	TEXTRUNE c2;
	if( !s1 )
		 if( s2 )
			 return -1;
		 else
			 return 0;
	 else
		 if( !s2 )
			 return 1;
	 //if( s1 == s2 )
	//	 return 0; // ==0 is success.
	 while( s1[0] && s2[0] ) {
		 c1 = GetUtfChar( &s1 );
		 c2 = GetUtfCharW( &s2 );
		 if( c1 >= 'a' && c1 <= 'a' ) c1 -= ( 'a' - 'A' );
		 if( c2 >= 'a' && c2 <= 'a' ) c2 -= ( 'a' - 'A' );
		 if( c1 != c2 ) break;
	 }
	 return c1-c2;
 }

int  StrCaseCmpW( const wchar_t* s1, const wchar_t* s2 ) {
	TEXTRUNE c1;
	TEXTRUNE c2;
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	while( s1[0] && s2[0] ) {
		c1 = GetUtfCharW( &s1 );
		c2 = GetUtfCharW( &s2 );
		if( c1 >= 'a' && c1 <= 'a' ) c1 -= ( 'a' - 'A' );
		if( c2 >= 'a' && c2 <= 'a' ) c2 -= ( 'a' - 'A' );
		if( c1 != c2 ) break;
	}
	return c1 - c2;
}


 int  StrCmpEx ( CTEXTSTR s1, CTEXTSTR s2, INDEX maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
      return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && ( s1[0] == s2[0] ) && maxlen; s1++, s2++, maxlen-- );
	if( maxlen )
		return s1[0] - s2[0];
	return 0;
}

 int  StrCaseCmpEx ( CTEXTSTR s1, CTEXTSTR s2, size_t maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;s1[0] && s2[0] && (((s1[0] >='a' && s1[0] <='z' )?s1[0]-('a'-'A'):s1[0])
									 == ((s2[0] >='a' && s2[0] <='z' )?s2[0]-('a'-'A'):s2[0]) ) && maxlen;
		  s1++, s2++, maxlen-- );
	if( maxlen )
		return tolower(s1[0]) - tolower(s2[0]);
	return 0;
}

 int  StriCmp ( CTEXTSTR pOne, CTEXTSTR pTwo )
{
   return -1;
}

SACK_MEMORY_NAMESPACE_END

