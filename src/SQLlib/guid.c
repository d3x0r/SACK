
#include <stdhdrs.h>
#include <pssql.h>
#include <procreg.h>

#if !defined( __LINUX__ ) && !defined( __INTERNAL_UUID__ )
#include <rpc.h>

SQL_NAMESPACE

PRELOAD( InitCo )
{
   CoInitializeEx( NULL, COINIT_MULTITHREADED );
}

#if ( defined( __WATCOMC__) && ( __WATCOMC__ < 1280 ) ) || defined( MINGW_SUX )
#ifdef __cplusplus
	extern "C" {
#endif
	RPC_STATUS RPC_ENTRY UuidCreateSequential( UUID __RPC_FAR *Uuid );
	RPC_STATUS RPC_ENTRY UuidCreate(
											  UUID __RPC_FAR *Uuid
											 );
#ifndef MINGW_SUX
	RPC_STATUS RPC_ENTRY UuidToString(
												 UUID __RPC_FAR *Uuid ,
												 unsigned char __RPC_FAR * __RPC_FAR *StringUuid
												);
	RPC_STATUS RPC_ENTRY UuidFromString(
													unsigned char __RPC_FAR *StringUuid,
													UUID __RPC_FAR *Uuid
												  );
#endif
#ifdef __cplusplus
	}
#endif
#endif

CTEXTSTR GetGUID( void )
{
	GUID guid;
	TEXTSTR text_guid = NewArray( TEXTCHAR, 37 );
	CTEXTSTR result;
	//TEXTSTR ext;
	int n;
	int x = 1;
	//CoCreateGuid( &guid );
	//UuidCreate( &guid );
	UuidCreate( &guid );
	snprintf( text_guid, 37, WIDE("%08lx-%04x-%04x-")
			  , guid.Data1
			  , guid.Data2
			  , guid.Data3
			  );
	for( n = 0; n < 8; n++ )
	{
		if( n == 2 )
		{
			// plus 2 more for being AFTER the digits.
			text_guid[18 + (n*2) + x] = '-';
			x++;
		}
		snprintf( text_guid + 18 + (n*2) + x, 3, WIDE("%02x"), guid.Data4[n] );
	}
	//lprintf( "Created GUid {%s}", text_guid );
	//lprintf( "My Conversion: %s", text_guid );
	//UuidToString( &guid, &ext );
	//lprintf( "   Conversion : %s", ext );
	result = SaveText( text_guid );
	Release( text_guid );
	return result;
}

CTEXTSTR GetSeqGUID( void )
{
	GUID guid;
	TEXTSTR text_guid = NewArray( TEXTCHAR, 37 );
	//TEXTSTR ext;
	int n;
	int x = 1;

	//CoCreateGuid( &guid );
	//UuidCreate( &guid );
	UuidCreateSequential( &guid );
	snprintf( text_guid, 37, WIDE("%08lx-%04x-%04x-")
			  , guid.Data1
			  , guid.Data2
			  , guid.Data3
			  );
	for( n = 0; n < 8; n++ )
	{
		if( n == 2 )
		{
			// plus 2 more for being AFTER the digits.
			text_guid[18 + (n*2) + x] = '-';
			x++;
		}
		snprintf( text_guid + 18 + (n*2) + x, 3, WIDE("%02x"), guid.Data4[n] );
	}
	//lprintf( "Created GUid {%s}", text_guid );
	//lprintf( "My Conversion: %s", text_guid );
	//UuidToString( &guid, &ext );
	//lprintf( "   Conversion : %s", ext );
	return SaveText( text_guid );
}
#else
#ifdef __INTERNAL_UUID__
#include "../uuid-1.6.2/uuid.h"
#else
#include <uuid/uuid.h>
#endif

SQL_NAMESPACE

CTEXTSTR GetGUID( void )
{
	uuid_t *tmp;
   char *str = NULL;
	TEXTCHAR *out_guid;
	CTEXTSTR out_guid2;
	uuid_create( &tmp );
	uuid_make( tmp, UUID_MAKE_V1 );
   uuid_export( tmp, UUID_FMT_STR, &str, NULL );
	out_guid = DupCharToText( str );
	out_guid2 = SaveText( out_guid );
	Release( out_guid );
	return out_guid2;
}
CTEXTSTR GetSeqGUID( void )
{
	uuid_t *tmp;
   char *str = NULL;
	TEXTCHAR *out_guid;
	CTEXTSTR out_guid2;
	uuid_create( &tmp );
	uuid_make( tmp, UUID_MAKE_V1 );
   uuid_export( tmp, UUID_FMT_STR, &str, NULL );
	out_guid = DupCharToText( str );
	out_guid2 = SaveText( out_guid );
	Release( out_guid );
	return out_guid2;
}
#endif

CTEXTSTR GuidZero( void )
{
	return WIDE("00000000-0000-0000-0000-000000000000");
}

SQL_NAMESPACE_END

