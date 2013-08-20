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


void  MemSet ( POINTER p, PTRSZVAL n, size_t sz )
{
#if defined( _MSC_VER ) && !defined( __NO_WIN32API__ ) && !defined( UNDER_CE )
#  if defined( _WIN64 )
	//__asm cld;
	__stosq( (_64*)p, n, sz/8 );
	if( sz & 4 )
		(*(_32*)( ((PTRSZVAL)p) + sz - (sz&7) ) ) = (_32)n;
	if( sz & 2 )
		(*(_16*)( ((PTRSZVAL)p) + sz - (sz&3) ) ) = (_16)n;
	if( sz & 1 )
		(*(_8*)( ((PTRSZVAL)p) + sz - (sz&1) ) ) = (_8)n;
#  else
#    ifdef __64__
	__stosq( (_64*)p, n, sz / 4 );
	if( sz & 4 )
		(*(_32*)( ((PTRSZVAL)p) + sz - (sz&7) ) ) = (_32)n;
	if( sz & 2 )
		(*(_16*)( ((PTRSZVAL)p) + sz - (sz&3) ) ) = (_16)n;
	if( sz & 1 )
		(*(_8*)( ((PTRSZVAL)p) + sz - (sz&1) ) ) = (_8)n;
#    else
	__stosd( (_32*)p, n, sz / 4 );
	if( sz & 2 )
		(*(_16*)( ((PTRSZVAL)p) + sz - (sz&3) ) ) = (_16)n;
	if( sz & 1 )
		(*(_8*)( ((PTRSZVAL)p) + sz - (sz&1) ) ) = (_8)n;
#    endif
#  endif
#else
   memset( p, n, sz );
#endif
}

int  MemChk ( POINTER p, PTRSZVAL val, size_t sz )
{
#if defined( _MSC_VER ) && !defined( __NO_WIN32API__ ) && !defined( UNDER_CE )
	int n;
	PTRSZVAL *data = (PTRSZVAL*)p;
	for( n = 0; n < sz/sizeof(PTRSZVAL); n++ )
		if( data[n] != val )
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
	__movsq( (_64*)pTo, (_64*)pFrom, sz/8 );
	if( sz & 4 )
		(*(_32*)( ((PTRSZVAL)pTo) + sz - (sz&7) ) ) = (*(_32*)( ((PTRSZVAL)pFrom) + sz - (sz&7) ) );
#  else
	__movsd( (_32*)pTo, (_32*)pFrom, sz/4 );
#  endif
	if( sz & 2 )
		(*(_16*)( ((PTRSZVAL)pTo) + sz - (sz&3) ) ) = (*(_16*)( ((PTRSZVAL)pFrom) + sz - (sz&3) ) );
	if( sz & 1 )
		(*(_8*)( ((PTRSZVAL)pTo) + sz - (sz&1) ) ) = (*(_8*)( ((PTRSZVAL)pFrom) + sz - (sz&1) ) );
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

CTEXTSTR StrRChr( CTEXTSTR s1, TEXTCHAR c )
{
	CTEXTSTR  p1 = s1;
	if( !p1 ) return NULL;
	while( p1[0] ) p1++;
	while( p1 != s1 && p1[0] != c ) p1--;
	if( p1[0] )
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
	if( p1[0] )
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
	register int t1, t2;
	_32 pos;
	{
		pos = 0;
		while( pos < n )
		{
			t1 = *(unsigned char*)s1;
			t2 = *(unsigned char*)s2;
			if( ( t1 ) == ( t2 ) )
			{
				(pos)++;
				s1 = (void*)(((PTRSZVAL)s1) + 1);
				s2 = (void*)(((PTRSZVAL)s2) + 1);
			}
			else if( t1 > t2 )
			{
				if( r )
					*r = pos;
				return 1;
			}
			else
				if( r )
					*r = pos;
				return -1;
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
		PTRSZVAL len = (PTRSZVAL)StrLen( original ) + 1;
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
	for( l = 0; s[l];l++);
	return l;	
}

size_t CStrLen( char *s )
{
	size_t l;
	if( !s )
		return 0;
	for( l = 0; s[l];l++);
	return l;	
}

#ifdef _UNICODE
char *  CStrDupEx ( CTEXTSTR original DBG_PASS )
{
   return WcharConvert( original );
}

TEXTSTR  DupCStrEx ( const char * original DBG_PASS )
{
   return CharWConvert( original );
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

TEXTSTR  DupCStrEx ( const char * original DBG_PASS )
{
	INDEX len = 0;
	TEXTSTR result;
	if( !original )
		return NULL;
	while( original[len] ) len++;
	result = (TEXTSTR)AllocateEx( (len+1) * sizeof( result[0] )  DBG_RELAY );
	len = 0;
	while( ( result[len] = original[len] ) != 0 ) len++;
	return result;
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

