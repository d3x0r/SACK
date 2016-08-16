#define SACKCOMMLIST_SOURCE
#include "listports.h"
#include <sharemem.h>

static LOGICAL Win9xListPorts( ListPortsCallback lpCallback, uintptr_t psv );
static LOGICAL WinNT40ListPorts( ListPortsCallback lpCallback, uintptr_t psv );
static LOGICAL Win2000ListPorts( ListPortsCallback lpCallback, uintptr_t psv ); 
static LOGICAL WinCEListPorts( ListPortsCallback lpCallback, uintptr_t psv);
static LOGICAL ScanEnumTree( CTEXTSTR lpEnumPath, ListPortsCallback lpCallback, uintptr_t psv );
static uint32_t OpenSubKeyByIndex( HKEY hKey, uint32_t dwIndex, REGSAM samDesired, PHKEY phkResult, TEXTSTR* lppSubKeyName ); 
static uint32_t QueryStringValue( HKEY hKey, CTEXTSTR lpValueName, TEXTSTR* lppStringValue );

LOGICAL ListPorts( ListPortsCallback lpCallback, uintptr_t psv )
{

#ifdef WIN32

	OSVERSIONINFO osvinfo;

	/* check parameters */

	if( !lpCallback )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	/* determine which platform we're running on and forward
	* to the appropriate routine
	*/

	ZeroMemory( &osvinfo, sizeof( osvinfo ) );
	osvinfo.dwOSVersionInfoSize = sizeof( osvinfo );

	GetVersionEx( &osvinfo );

	lprintf( WIDE(" Platform ID: %d, Major Version: %d"), osvinfo.dwPlatformId, osvinfo.dwMajorVersion );

	switch( osvinfo.dwPlatformId )
	{

		case VER_PLATFORM_WIN32_WINDOWS:
			
			return Win9xListPorts( lpCallback, psv );
			break;

		case VER_PLATFORM_WIN32_NT:

			if( osvinfo.dwMajorVersion < 4 )
			{
				SetLastError( ERROR_OLD_WIN_VERSION );
				return FALSE;
			}

			else if( osvinfo.dwMajorVersion == 4 )
			{
				return WinNT40ListPorts( lpCallback, psv );
			}

			else
			{
				return Win2000ListPorts( lpCallback, psv ); /* hopefully it'll also work for XP */
			}

			break;
#ifdef _WIN32_WCE

		case VER_PLATFORM_WIN32_CE:
			return WinCEListPorts( lpCallback, lpCallbackValue );

#endif

		default:

			SetLastError( ERROR_OLD_WIN_VERSION );
			return FALSE;
			break;
	}

#else

#endif
}

static LOGICAL Win9xListPorts( ListPortsCallback lpCallback, uintptr_t psv )
{
	return ScanEnumTree( WIDE( "ENUM" ), lpCallback, psv );
}

static LOGICAL WinNT40ListPorts( ListPortsCallback lpCallback, uintptr_t psv )
/* Unlike Win9x, information on serial ports registry storing in NT 4.0 is
 * scarce. HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM is reliably
 * documented to hold the names of all installed serial ports, but I haven't
 * confirmed this for infrared ports and other nonstandard drivers.
 * Descriptions stored under SERIALCOMM lack the "FRIENDLYNAME" found
 * in Win9x, there's only the bare names of the ports.
 */
{
	uint32_t     dwError = 0;
	HKEY    hKey = NULL;
	uint32_t     dwIndex;
	TEXTSTR lpValueName = NULL;
	TEXTSTR lpPortName = NULL;

	if( dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WIDE( "HARDWARE\\DEVICEMAP\\SERIALCOMM" ), 0, KEY_READ,&hKey ) )
	{
		/* it is really strange that this key does not exist, but could happen in theory */
		if( dwError == ERROR_FILE_NOT_FOUND ) dwError = 0;
		goto end;
	}

	for(dwIndex=0;;++dwIndex)
	{
		uint32_t              cbValueName = 32 * sizeof( TEXTCHAR );
		uint32_t              cbPortName = 32 * sizeof( TEXTCHAR );
		LISTPORTS_PORTINFO portinfo;

		/* loop asking for the value data til we allocated enough memory */

		for(;;)
		{
			free( lpValueName );
			if( !( lpValueName = (TEXTSTR)malloc( cbValueName ) ) )
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;
				goto end;
			}

			free( lpPortName );

			if( !( lpPortName = (TEXTSTR)malloc( cbPortName ) ) )
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;
				goto end;
			}

			if( !( dwError = RegEnumValue( hKey, dwIndex, lpValueName, (LPDWORD)&cbValueName, NULL, NULL, (LPBYTE)lpPortName, (LPDWORD)&cbPortName) ) )
			{
				break; /* we did it */
			}

			else if( dwError == ERROR_MORE_DATA )
			{	
				/* not enough space */
				dwError = 0;
				/* no indication of space required, we try doubling */
				cbValueName =( cbValueName + sizeof( TEXTCHAR ) ) * 2;
			}

			else goto end;
		}

		portinfo.lpPortName = lpPortName;
		portinfo.lpFriendlyName = lpPortName; /* no friendly name in NT 4.0 */
		portinfo.lpTechnology = WIDE(""); /* this information is not available */

		if( !lpCallback( psv,&portinfo ) )
		{
			goto end; /* listing aborted by callback */
		}
	} 

end:

	free( lpValueName );
	free( lpPortName );

	if( hKey != NULL ) 
		RegCloseKey( hKey );

	if( dwError!=0 )
	{
		SetLastError(dwError);
		return FALSE;
	}

	else return TRUE;
}

static LOGICAL Win2000ListPorts( ListPortsCallback lpCallback, uintptr_t psv )
/* Information on serial ports is stored in the Win2000 registry in a manner very
 * similar to that of Win9x, with three differences:
 *   � The ENUM tree is not located under HKLM, but under HKLM\SYSTEM\CURRENTCONTROLSET.
 *   � The parameter "PORTNAME" is not a LEVEL3 value like "CLASS" and "FRIENDLYNAME";
 *     instead, it is located under an additional subkey named "DEVICE PARAMETERS"
 *   � "CLASS" seems to be deprecated in favor of "CLASSGUID" wich contains a GUID
 *     identifying each type of device. I've seen Win200 installations with and
 *     without "CLASS" values. Moreover, I've seen Win9x installations containing
 *     "CLASSGUID" values, so, to be as robust as possible, ScanEnumTree() accept either
 *     type of value.
 *
 * An example follows:
 *
 * HKLM\SYSTEM\CURRENTCONTROLSET
 *   |-BIOS
 *     |-*PNP0501
 *       |-0D (or any other value, this is not important for us)
 *         � CLASSGUID=    "{4D36E978-E325-11CE-BFC1-08002BE10318}"
 *         � FRIENDLYNAME= "Communications Port (COM1)"
 *         |-DEVICE PARAMETERS
 *           � PORTNAME=   "COM1"
 */
{
  return ScanEnumTree( WIDE( "SYSTEM\\CURRENTCONTROLSET\\ENUM" ), lpCallback, psv );
}

static LOGICAL WinCEListPorts( ListPortsCallback lpCallback, uintptr_t psv )
/* Available COM ports on Pocket PC/Windows CE devices are stored in registry under
 * key: HKLM\Drivers\BuiltIn. Note, that also other stuff like native Bluetooth ports
 * are written in this part of registry, so we must skip them using condition
 * (_tcsncicmp(lpPortName,WIDE("COM"),3)!=0) below.
 */
{
	uint32_t                 dwError = 0;
	HKEY                hKey = NULL;
	uint32_t                 dwIndex;
	TEXTSTR             lpPortName = NULL;
	HKEY                hkLevel1 = NULL;
	TEXTSTR             lpFriendlyName = NULL;
	LISTPORTS_PORTINFO  portinfo;
	uint32_t                 index;
	uint32_t                 wordSize = sizeof( uint32_t );

	portinfo.lpPortName = (TEXTSTR)malloc( 64 );

	if( dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WIDE( "Drivers\\BuiltIn" ), 0, KEY_READ, &hKey ) )
	{
		/* it is really strange that this key does not exist, but could happen in theory */
		if( dwError == ERROR_FILE_NOT_FOUND )
			dwError = 0;

		goto end;
	}

	for( dwIndex=0; ; ++dwIndex ) 
	{
		dwError = 0;

		if ( dwError = OpenSubKeyByIndex( hKey, dwIndex, KEY_ENUMERATE_SUB_KEYS, &hkLevel1, NULL ) ) 
		{
			if ( dwError == ERROR_NO_MORE_ITEMS ) dwError = 0;
			break;
		}

		if ( dwError = QueryStringValue( hkLevel1, WIDE( "PREFIX" ), &lpPortName ) ) 
		{
			if( dwError == ERROR_FILE_NOT_FOUND )
				continue; 
			
			else
				break;
		}

		if ( StrCaseCmpEx( lpPortName, WIDE( "COM" ), 3 ) != 0 ) 
			continue; // We want only COM serial ports

		if ( dwError = RegQueryValueEx( hkLevel1, WIDE( "INDEX" ), NULL, NULL, (LPBYTE)&index, (LPDWORD)&wordSize) )
		{
			if( dwError == ERROR_FILE_NOT_FOUND ) 
				continue; 
			
			else 
				break;
		}

		// Now "index" contains serial port number, we put it together with "COM"
		// to format like "COM<index>"
		snprintf( portinfo.lpPortName, sizeof( portinfo.lpPortName ), WIDE("COM%u"), index );		

		// Get friendly name
		dwError = QueryStringValue( hkLevel1, WIDE( "FRIENDLYNAME" ), &lpFriendlyName );
		portinfo.lpFriendlyName = dwError ? (TEXTSTR)WIDE( "" ) : lpFriendlyName;

		portinfo.lpTechnology = WIDE( "" ); /* this information is not available */

		if( !lpCallback( psv, &portinfo ) )
		{
			break;
		}
	} 

end:
	free( portinfo.lpPortName );
	free( lpFriendlyName );
	free( lpPortName );

	if( hKey != NULL ) 
		RegCloseKey( hKey );

	if( hkLevel1 != NULL )
		RegCloseKey( hkLevel1 );

	if( dwError != 0 )
	{
		SetLastError( dwError );
		return FALSE;
	}

	else 
		return TRUE;
}

static LOGICAL ScanEnumTree( CTEXTSTR lpEnumPath, ListPortsCallback lpCallback, uintptr_t psv )
{
	static const TEXTCHAR lpstrPortsClass[] =    WIDE( "PORTS" );
	static const TEXTCHAR lpstrPortsClassGUID[] = WIDE( "{4D36E978-E325-11CE-BFC1-08002BE10318}" );
	int terd = 0;
	uint32_t    dwError = 0;
	HKEY   hkEnum = NULL;
	uint32_t    dwIndex1;
	HKEY   hkLevel1 = NULL;
	uint32_t    dwIndex2;
	HKEY   hkLevel2 = NULL;
	uint32_t    dwIndex3;
	HKEY   hkLevel3 = NULL;
	HKEY   hkDeviceParameters = NULL;
	TEXTCHAR  lpClass[sizeof( lpstrPortsClass ) / sizeof( lpstrPortsClass[0] )];
	uint32_t    cbClass;
	TEXTCHAR  lpClassGUID[sizeof( lpstrPortsClassGUID ) / sizeof( lpstrPortsClassGUID[0] )];
	uint32_t    cbClassGUID;
	TEXTSTR lpPortName = NULL;
	TEXTSTR lpFriendlyName = NULL;
	TEXTSTR lpTechnology = NULL;

	if( dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, lpEnumPath, 0, KEY_ENUMERATE_SUB_KEYS, &hkEnum ) )
	{
		goto end;
	}

	for( dwIndex1 = 0; ; ++dwIndex1 )
	{
		if( hkLevel1 != NULL )
		{
			RegCloseKey( hkLevel1 );
			hkLevel1 = NULL;
		}

		if( dwError = OpenSubKeyByIndex( hkEnum, dwIndex1, KEY_ENUMERATE_SUB_KEYS, &hkLevel1, &lpTechnology ) )
		{
			if( dwError == ERROR_NO_MORE_ITEMS )
			{
				dwError = 0;
				break;
			}

			else
				goto end;
		}

		for( dwIndex2 = 0; ; ++dwIndex2 )
		{
			if( hkLevel2 != NULL )
			{
				RegCloseKey( hkLevel2 );
				hkLevel2 = NULL;
			}

			if( dwError = OpenSubKeyByIndex( hkLevel1, dwIndex2, KEY_ENUMERATE_SUB_KEYS, &hkLevel2, NULL ) )
			{
				if( dwError == ERROR_NO_MORE_ITEMS )
				{
					dwError = 0;
					break;
				}

				else 
					goto end;
			}

			for( dwIndex3 = 0; ;++dwIndex3 )
			{
				LOGICAL bFriendlyNameNotFound = FALSE;
				LISTPORTS_PORTINFO portinfo;

				if( hkLevel3 != NULL )
				{
					RegCloseKey( hkLevel3 );
					hkLevel3 = NULL;
				}

				if( dwError = OpenSubKeyByIndex( hkLevel2, dwIndex3, KEY_READ, &hkLevel3, NULL ) )
				{
					if( dwError == ERROR_NO_MORE_ITEMS )
					{
						dwError = 0;
						break;
					}

					else
						goto end;
				}

				/* Look if the driver class is the one we're looking for.
					* We accept either "CLASS" or "CLASSGUID" as identifiers.
					* No need to dynamically arrange for space to retrieve the values,
					* they must have the same length as the strings they're compared to
					* if the comparison is to be succesful.
					*/

				cbClass = sizeof( lpClass );
				
				if( ( terd = RegQueryValueEx( hkLevel3, WIDE( "CLASS" ), NULL, NULL, (LPBYTE)lpClass, (LPDWORD)&cbClass ) ) == ERROR_SUCCESS && StrCaseCmp( lpClass, lpstrPortsClass ) == 0 )
				{						 
					/* ok */
				}

				else
				{
					cbClassGUID = sizeof( lpClassGUID );					
					if( RegQueryValueEx( hkLevel3, WIDE( "CLASSGUID" ), NULL, NULL, (LPBYTE)lpClassGUID, (LPDWORD)&cbClassGUID ) == ERROR_SUCCESS && StrCaseCmp( lpClassGUID, lpstrPortsClassGUID ) == 0 )
					{						
						/* ok */
					}

					else
						continue;
				}

				/* get "PORTNAME" */

				dwError = QueryStringValue( hkLevel3, WIDE( "PORTNAME" ), &lpPortName );
				if( dwError == ERROR_FILE_NOT_FOUND )
				{
					/* In Win200, "PORTNAME" is located under the subkey "DEVICE PARAMETERS".
					* Try and look there.
					*/

					if( hkDeviceParameters != NULL )
					{
						RegCloseKey( hkDeviceParameters );
						hkDeviceParameters = NULL;
					}
				
					if( RegOpenKeyEx( hkLevel3, WIDE( "DEVICE PARAMETERS" ), 0, KEY_READ, &hkDeviceParameters ) == ERROR_SUCCESS )
					{
						dwError = QueryStringValue( hkDeviceParameters, WIDE( "PORTNAME" ), &lpPortName );
					}
				}

				if( dwError )
				{
					if( dwError == ERROR_FILE_NOT_FOUND )
					{ 
						/* boy that was strange, we better skip this device */
						dwError = 0;
						continue;
					}
				
					else
						goto end;			
				}

				/* check if it is a serial port (instead of, say, a parallel port) */

				if( StrCaseCmpEx( lpPortName, WIDE( "COM" ), 3 ) != 0 )
					continue;

				/* now go for "FRIENDLYNAME" */

				if( dwError = QueryStringValue( hkLevel3, WIDE( "FRIENDLYNAME" ), &lpFriendlyName ) )
				{
					if( dwError == ERROR_FILE_NOT_FOUND )
					{
						bFriendlyNameNotFound = TRUE;
						dwError = 0;
					}
				
					else
						goto end;
				}

				/* Assemble the information and pass it on to the callback.
					* In the unlikely case there's no friendly name available,
					* use port name instead.
					*/
				portinfo.lpPortName = lpPortName;
				portinfo.lpFriendlyName = bFriendlyNameNotFound ? lpPortName : lpFriendlyName;
				portinfo.lpTechnology = lpTechnology;
				if( !lpCallback( psv, &portinfo ) )
				{
					goto end; /* listing aborted by callback */
				}
			}
		}
	}

end:
	free( lpTechnology );
	free( lpFriendlyName );
	free( lpPortName );
	if( hkDeviceParameters != NULL ) RegCloseKey( hkDeviceParameters );
	if( hkLevel3!=NULL ) RegCloseKey( hkLevel3 );
	if( hkLevel2!=NULL ) RegCloseKey( hkLevel2 );
	if( hkLevel1!=NULL ) RegCloseKey( hkLevel1 );
	if( hkEnum != NULL )  RegCloseKey( hkEnum );

	if( dwError != 0 )
	{
		SetLastError( dwError );
		return FALSE;
	}

	else
		return TRUE;
}

static uint32_t OpenSubKeyByIndex( HKEY hKey, uint32_t dwIndex, REGSAM samDesired, PHKEY phkResult, TEXTSTR* lppSubKeyName )
{
	uint32_t              dwError = 0;
	LOGICAL          bLocalSubkeyName = FALSE;
	TEXTSTR          lpSubkeyName = NULL;
	uint32_t              cbSubkeyName = 128 * sizeof( TEXTCHAR ); /* an initial guess */
	FILETIME         filetime;

	if( lppSubKeyName == NULL )
	{
		bLocalSubkeyName = TRUE;
		lppSubKeyName = &lpSubkeyName;
	}
	/* loop asking for the subkey name til we allocated enough memory */

	for( ; ; )
	{
		free( *lppSubKeyName );
		
		if( !( *lppSubKeyName = (TEXTSTR)malloc( cbSubkeyName ) ) )
		{
			dwError = ERROR_NOT_ENOUGH_MEMORY;
			goto end;
		}

		if( !( dwError = RegEnumKeyEx( hKey, dwIndex, *lppSubKeyName, (LPDWORD)&cbSubkeyName, 0, NULL, NULL, &filetime ) ) )
		{
			break; /* we did it */
		}
		
		else if( dwError == ERROR_MORE_DATA )
		{ 
			/* not enough space */
			dwError=0;
			/* no indication of space required, we try doubling */
			cbSubkeyName = ( cbSubkeyName + sizeof( TEXTCHAR ) ) * 2;
		}
		
		else
			goto end;
	}

	dwError = RegOpenKeyEx( hKey, *lppSubKeyName, 0, samDesired, phkResult );

end:
	if( bLocalSubkeyName )
		free( *lppSubKeyName );

	return dwError;
}

static uint32_t QueryStringValue( HKEY hKey, CTEXTSTR lpValueName, TEXTSTR* lppStringValue )
{
	uint32_t cbStringValue = 128 * sizeof( TEXTCHAR ); /* an initial guess */

	for(;;)
	{
		uint32_t dwError;

		free( *lppStringValue );

		if( !( *lppStringValue = (TEXTSTR)malloc( cbStringValue ) ) )
		{
			return ERROR_NOT_ENOUGH_MEMORY;
		}
		
		if( !( dwError = RegQueryValueEx( hKey, lpValueName, NULL, NULL, (LPBYTE)*lppStringValue, (LPDWORD)&cbStringValue ) ) )
		{
			return ERROR_SUCCESS;
		}

		else if( dwError == ERROR_MORE_DATA )
		{
			/* not enough space, keep looping */
		}

		else
			return dwError;
	}
}
