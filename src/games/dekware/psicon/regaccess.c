#include "stdhdrs.h"
//#include "RComLib.h"  // make sure we have the same typecasts...

/*
//==============================================================================
HKEY OpenReg(HKEY root, TEXTCHAR *Name, DWORD Access)
{
   HKEY temp;
   DWORD dwStatus;
   dwStatus = RegOpenKeyEx(root, Name, 0, Access, &temp);
   switch(dwStatus)
   {
      case ERROR_SUCCESS:
         break;
      default:
         temp=NULL;
         break;
   }
   return(temp);
}

//==============================================================================
LONG CloseReg(HKEY root)
{
   return(RegCloseKey(root));
}

//==============================================================================
HKEY GetHive(LPSTR *lpszPath)
{
   BYTE lpTemp[32];
   LPSTR lpPath   =  *lpszPath;
   WORD wOffset   =  0;
   while( (*lpPath != '\\') && (wOffset < 31) )
   {
      lpTemp[wOffset++] = *lpPath++;
   }
   lpTemp[wOffset++] = 0;
   if( stricmp( lpTemp, "HKEY_CURRENT_USER" ) == 0 )
   {
      *lpszPath += wOffset;
      return(HKEY_CURRENT_USER);
   }
   if( stricmp( lpTemp, "HKEY_CLASSES_ROOT" ) == 0 )
   {
      *lpszPath += wOffset;
      return(HKEY_CLASSES_ROOT);
   }
   if( stricmp( lpTemp, "HKEY_LOCAL_MACHINE") == 0 )
   {
      *lpszPath += wOffset;
      return(HKEY_LOCAL_MACHINE);
   }
   if( stricmp( lpTemp, "HKEY_USERS") == 0 )
   {
      *lpszPath += wOffset;
      return(HKEY_USERS);
   }  
   return(NULL);
}

//==============================================================================
#define GCS_MAX_STRING_LEN 2048     // Max len. of lpszPath concat with lpszSection
DWORD GetConfigString(LPCTSTR lpszSection, LPSTR lpszKey,
                      LPCTSTR lpszDefault, LPSTR lpszReturnBuffer,
                      DWORD cchReturnBuffer, LPSTR lpszPath)
{
   HKEY hTemp;
   HKEY hRoot;
   TEXTCHAR  szString[GCS_MAX_STRING_LEN];
   DWORD dwStatus,dwType;
   DWORD cBufSize = cchReturnBuffer;
   
   // Force pointer to the default string to not be NULL
   if( !lpszDefault )
      lpszDefault = "";
   
   // Set initial default (in case something's wrong)
   strncpy( lpszReturnBuffer, (LPSTR)lpszDefault, cchReturnBuffer-1 );

   // Parse out begin of lpszPath...
   hRoot = GetHive(&lpszPath);
   if( hRoot )
   {
      // Validate the string is not too long
      if( (strlen(lpszPath) + strlen(lpszSection) + 2) <= GCS_MAX_STRING_LEN )
      {
         wsprintf( szString, "%s\\%s", lpszPath, lpszSection);
         dwStatus = RegOpenKeyEx( hRoot, szString, 0, KEY_READ, &hTemp);
         if( (dwStatus == ERROR_SUCCESS) && hTemp )
         {
            dwStatus = RegQueryValueEx(hTemp, lpszKey, 0
                                , &dwType
                                , lpszReturnBuffer
                                , &cBufSize );
            if( dwType != REG_SZ &&
                dwType != REG_EXPAND_SZ )
            {
               // Restore the default over strange data type value.
               strccpy( lpszReturnBuffer, (LPSTR)lpszDefault, cchReturnBuffer-1 );
            }
            else if( (dwStatus == ERROR_SUCCESS) &&
                     (cBufSize < cchReturnBuffer) )
            {  // Cover for an odd/even boundary problem with the registry...
               lpszReturnBuffer[cBufSize] = '\0';
            }
            CloseReg( hTemp );
         }
      }
      CloseReg( hRoot );
   }
   return( strlen(lpszReturnBuffer) );
}

//==============================================================================
DWORD SetConfigString(LPCTSTR lpszSection,LPSTR lpszKey,
                      LPCTSTR lpszDefault,LPSTR lpszReturnBuffer,
                      DWORD cchReturnBuffer,LPTSTR lpszPath)
{
   HKEY hTemp;
   HKEY hRoot  = HKEY_CURRENT_USER;
   TEXTCHAR szString[GCS_MAX_STRING_LEN];
   DWORD dwStatus, dwType, dwDisposition;
   
   // Parse out begin of lpszPath...

   hRoot = GetHive(&lpszPath);
   if (!hRoot)
      return(ERROR_PATH_NOT_FOUND);

   // Validate the string is not too long
   if( (strlen(lpszPath) + strlen(lpszSection) + 2) > GCS_MAX_STRING_LEN )
   {
      return(ERROR_PATH_NOT_FOUND);
   }

   wsprintf( szString, "%s\\%s", lpszPath, lpszSection);
   dwStatus = RegOpenKeyEx( hRoot, szString, 0, KEY_WRITE, &hTemp );
   switch( dwStatus )
   {
   case ERROR_SUCCESS:
      break;
   case ERROR_FILE_NOT_FOUND:
      dwStatus = RegCreateKeyEx(hRoot, szString, 0
                             , ""
                             , REG_OPTION_NON_VOLATILE
                             , KEY_WRITE
                             , NULL
                             , &hTemp
                             , &dwDisposition);
      switch(dwStatus)
      {
      case ERROR_SUCCESS:
         break;
      default:
         return(dwStatus);
      }
      break;
   default:
      return(dwStatus);
      break;
   }
   if (hTemp)
   {
      dwType = REG_SZ;
      dwStatus = RegSetValueEx(hTemp, lpszKey, 0
                          , dwType
                          , lpszReturnBuffer
                          , cchReturnBuffer);
      CloseReg(hTemp);
   }
   return(dwStatus);
}

BOOL DelConfigString( LPCSTR lpszSection, LPSTR lpszKey, LPTSTR lpszPath )
{
   HKEY hTemp;
   HKEY hRoot  = HKEY_CURRENT_USER;
   TEXTCHAR szString[GCS_MAX_STRING_LEN];
   DWORD dwStatus, dwType;

   hRoot = GetHive(&lpszPath);  // open the root key.

   if (!hRoot)
      return(ERROR_PATH_NOT_FOUND);

   // Validate the string is not too long
   if( (strlen(lpszPath) + strlen(lpszSection) + 2) > GCS_MAX_STRING_LEN )
   {
      return(ERROR_PATH_NOT_FOUND);
   }

   wsprintf( szString, "%s\\%s", lpszPath, lpszSection); // open the key.
   dwStatus = RegOpenKeyEx( hRoot, szString, 0, KEY_WRITE, &hTemp );
   switch( dwStatus )
   {
   case ERROR_SUCCESS:
      break;
   case ERROR_FILE_NOT_FOUND:
      return ERROR_SUCCESS;  // if the key didn't exist, it's gone.
      break;
   default:
      return(dwStatus);
      break;
   }
   if (hTemp)
   {
      dwType = REG_SZ;
      dwStatus = RegDeleteValue( hTemp, lpszKey );
      CloseReg(hTemp);
   }
   return(dwStatus);
      
}
*/

#define KEY_PREFIX WIDE("Software\\Freedom Collective\\")

int GetRegistryItem( HKEY hRoot, TEXTCHAR *pPrefix, 
                     TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                     DWORD dwType,  
                     TEXTCHAR *nResult, int nSize )
{
   TEXTCHAR szString[512];
   TEXTCHAR pValue[512];
   TEXTCHAR *pszString = szString;
   DWORD dwStatus, dwRetType, dwBufSize;
   HKEY hTemp;

   if( pProduct )
      snprintf( pszString, sizeof( szString ), WIDE("%s%s"), pPrefix, pProduct );
   else
      snprintf( pszString, sizeof( szString ), WIDE("%s"), pPrefix );

   dwStatus = RegOpenKeyEx( hRoot, 
                            pszString, 0, 
                            KEY_READ, &hTemp );
   if( (dwStatus == ERROR_SUCCESS) && hTemp )
   {
      dwBufSize = sizeof( pValue );
      dwStatus = RegQueryValueEx(hTemp, pKey, 0
                          , &dwRetType
                          , (PBYTE)&pValue
                          , &dwBufSize );
      RegCloseKey( hTemp );
      if( dwStatus == ERROR_SUCCESS &&
          dwRetType == dwType )
      {
         memcpy( nResult, pValue, nSize );
         return TRUE;
      }
   }
   return FALSE;
}


//-----------------------------------------------------------


int GetRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                    int *nResult )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_DWORD,
                           (TEXTCHAR*)nResult, sizeof(DWORD) );
}

//-----------------------------------------------------------


int GetLocalRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                         int *nResult )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_DWORD,
                           (TEXTCHAR*)nResult, sizeof(DWORD) );
}

//-----------------------------------------------------------


int GetRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                       TEXTCHAR *pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_SZ,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------


int GetRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pValue,
                       POINTER pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue,
                           REG_BINARY,
                           (TEXTCHAR*)pResult, nMaxLen );
}

//-----------------------------------------------------------

int GetLocalRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                            TEXTCHAR *pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_SZ,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int GetLocalRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pValue,
                            TEXTCHAR *pResult, int nMaxLen )
{
   return GetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue,
                           REG_BINARY,
                           pResult, nMaxLen );
}

//-----------------------------------------------------------

int SetRegistryItem( HKEY hRoot, TEXTCHAR *pPrefix,
                     TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                     DWORD dwType,
                     CTEXTSTR pValue, int nSize )
{
   TEXTCHAR szString[512];
   TEXTCHAR *pszString = szString;
   DWORD dwStatus;
   HKEY hTemp;

   if( pProduct )
      wsprintf( pszString, WIDE("%s%s"), pPrefix, pProduct );
   else
      wsprintf( pszString, WIDE("%s"), pPrefix );

   dwStatus = RegOpenKeyEx( hRoot, 
                            pszString, 0, 
                            KEY_WRITE, &hTemp );
   if( dwStatus == ERROR_FILE_NOT_FOUND )
   {
      DWORD dwDisposition;
      dwStatus = RegCreateKeyEx( hRoot, 
                                 pszString, 0
                             , WIDE("")
                             , REG_OPTION_NON_VOLATILE
                             , KEY_WRITE
                             , NULL
                             , &hTemp
                             , &dwDisposition);
      if( dwDisposition == REG_OPENED_EXISTING_KEY )
         OutputDebugString( WIDE("Failed to open, then could open???") );
      if( dwStatus )   // ERROR_SUCCESS == 0 
         return FALSE; 
   }
   if( (dwStatus == ERROR_SUCCESS) && hTemp )
   {
      dwStatus = RegSetValueEx(hTemp, pKey, 0
                                , dwType
                                , (P_8)pValue, nSize );
      RegCloseKey( hTemp );
      if( dwStatus == ERROR_SUCCESS )
      {
         return TRUE;
      }
   }
   return FALSE;
}

//-----------------------------------------------------------

int SetRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                    size_t nValue )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pValue, 
                           REG_DWORD,
                           (TEXTCHAR*)&nValue, sizeof(int) );
}

//-----------------------------------------------------------

int SetLocalRegistryInt( TEXTCHAR *pProduct, TEXTCHAR *pValue, 
                         int nValue )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pValue, 
                           REG_DWORD,
                           (TEXTCHAR*)&nValue, sizeof(int) );
}

//-----------------------------------------------------------

int SetRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                       TEXTCHAR *pValue )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_SZ,
                           pValue, strlen( pValue ) );
}

//-----------------------------------------------------------

int SetRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                       POINTER pValue, int nLen )
{
   return SetRegistryItem( HKEY_LOCAL_MACHINE, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_BINARY,
                           (TEXTCHAR*)pValue, nLen );
}


//-----------------------------------------------------------

int SetLocalRegistryString( TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                            TEXTCHAR *pValue )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_SZ,
                           pValue, strlen( pValue ) );
}
//-----------------------------------------------------------

int SetLocalRegistryBinary( TEXTCHAR *pProduct, TEXTCHAR *pKey, 
                            TEXTCHAR *pValue, int nLen )
{
   return SetRegistryItem( HKEY_CURRENT_USER, KEY_PREFIX,
                           pProduct, pKey, 
                           REG_BINARY,
                           pValue, nLen );
}
// $Log: regaccess.c,v $
// Revision 1.5  2004/03/08 09:25:43  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.4  2004/01/20 08:21:34  d3x0r
// Common updates for merging more commonality
//
// Revision 1.2  2003/03/25 08:59:04  panther
// Added CVS logging
//
