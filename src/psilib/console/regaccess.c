
#include <stdhdrs.h>
#include <sqlgetoption.h>
#include "regaccess.h"
#define KEY_PREFIX WIDE( "Software\\Freedom Collective\\" )

int GetRegistryItem( HKEY hRoot, CTEXTSTR pPrefix, 
                     CTEXTSTR pProduct, CTEXTSTR pKey, 
                     DWORD dwType,  
                     TEXTSTR nResult, int nSize )
{
	TEXTCHAR szString[512];

	if( pProduct )
		snprintf( szString, 512, WIDE( "%s%s" ), pPrefix, pProduct );
	else
		snprintf( szString, 512, WIDE( "%s" ), pPrefix );

	switch( dwType ) {
	case REG_SZ:
		SACK_GetProfileString( szString, pKey, "", nResult, nSize );
		break;
	case REG_DWORD:
		((int*)nResult)[0] = SACK_GetProfileInt( szString, pKey, ((int*)nResult)[0] );
		break;
	case REG_BINARY:
		{
			TEXTCHAR *data;
			size_t dataLen;
			((int*)nResult)[0] = SACK_GetProfileBlob( szString, pKey, &data, &dataLen );
			memcpy( nResult, data, dataLen );
			Release( data );
			return dataLen;
		}
		break;
	}

	return TRUE;
}

//-----------------------------------------------------------


int GetRegistryInt( CTEXTSTR pProduct, CTEXTSTR pValue,
                    int *nResult )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_DWORD,
                           (TEXTCHAR*)nResult, sizeof(DWORD) );
}

//-----------------------------------------------------------


int GetLocalRegistryInt( CTEXTSTR pProduct, CTEXTSTR pValue, 
                         int *nResult )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_DWORD,
                           (TEXTCHAR*)nResult, sizeof(DWORD) );
}

//-----------------------------------------------------------

int GetRegistryString( CTEXTSTR pProduct, CTEXTSTR pValue, 
                       TEXTSTR pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_SZ,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int GetRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pValue,
                       TEXTSTR pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_BINARY,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int GetLocalRegistryString( CTEXTSTR pProduct, CTEXTSTR pValue, 
                            TEXTSTR pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_SZ,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int GetLocalRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pValue,
                            TEXTSTR pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_BINARY,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int SetRegistryItem( HKEY hRoot, CTEXTSTR pPrefix,
                     CTEXTSTR pProduct, CTEXTSTR pKey, 
                     DWORD dwType,
                     CTEXTSTR pValue, int nSize )
{
	TEXTCHAR szString[512];

	if( pProduct )
		snprintf( szString, 512, WIDE( "%s%s" ), pPrefix, pProduct );
	else
		snprintf( szString, 512, WIDE( "%s" ), pPrefix );
	switch( dwType ) {
	case REG_DWORD:
		SACK_WriteProfileInt( szString, pKey, ((int*)pValue)[0] );
		break;
	case REG_SZ:
		SACK_WriteProfileString( szString, pKey, pValue );
		break;
	case REG_BINARY:
		SACK_WriteProfileBlob( szString, pKey, (TEXTCHAR*)pValue, nSize );
		break;
	}
	return TRUE;
}

//-----------------------------------------------------------

int SetRegistryInt( CTEXTSTR pProduct, CTEXTSTR pValue, 
                    int nValue )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue, 
                           REG_DWORD,
                           (TEXTCHAR*)&nValue, sizeof(int) );
}

//-----------------------------------------------------------

int SetLocalRegistryInt( CTEXTSTR pProduct, CTEXTSTR pValue, 
                         int nValue )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue, 
                           REG_DWORD,
                           (TEXTCHAR*)&nValue, sizeof(int) );
}

//-----------------------------------------------------------

int SetRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, 
                       CTEXTSTR pValue )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_SZ,
                           pValue, StrLen( pValue ) );
}

//-----------------------------------------------------------

int SetRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, 
                       CTEXTSTR pValue, int nLen )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_BINARY,
                           pValue, nLen );
}


//-----------------------------------------------------------

int SetLocalRegistryString( CTEXTSTR pProduct, CTEXTSTR pKey, 
                            CTEXTSTR pValue )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_SZ,
                           pValue, StrLen( pValue ) );
}
//-----------------------------------------------------------

int SetLocalRegistryBinary( CTEXTSTR pProduct, CTEXTSTR pKey, 
                            CTEXTSTR pValue, int nLen )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_BINARY,
                           pValue, nLen );
}

