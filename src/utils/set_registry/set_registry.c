#include <stdhdrs.h>
#include <windows.h>
#include <stdio.h>


static int SetRegistryItem( HKEY hRoot, TCHAR *pPrefix,
                     TCHAR *pKey, 
                     DWORD dwType,
                     TCHAR *pValue, int nSize )
{
   DWORD dwStatus;
   HKEY hTemp;

   dwStatus = RegOpenKeyEx( hRoot, 
                            pPrefix, 0, 
                            KEY_WRITE, &hTemp );
   if( dwStatus == ERROR_FILE_NOT_FOUND )
   {
      DWORD dwDisposition;
      dwStatus = RegCreateKeyEx( hRoot, 
                                 pPrefix, 0
                             , TEXT("")
                             , REG_OPTION_NON_VOLATILE
                             , KEY_WRITE
                             , NULL
                             , &hTemp
                             , &dwDisposition);
      if( dwStatus )   // ERROR_SUCCESS == 0 
         return FALSE; 
      if( dwDisposition == REG_OPENED_EXISTING_KEY )
         lprintf( TEXT("Failed to open, then could open???") );
   }
   if( (dwStatus == ERROR_SUCCESS) && hTemp )
   {
      dwStatus = RegSetValueEx(hTemp, pKey, 0
                                , dwType
                                , (const BYTE *)pValue, nSize );
      RegCloseKey( hTemp );
      if( dwStatus == ERROR_SUCCESS )
      {
         return TRUE;
      }
   }
   return FALSE;
}


static void Usage( TCHAR **argv )
{
	printf( TEXT("%s [<STRING/DWORD> <registry root> <registry path> <entry name> <value>] ...\n")
			 TEXT(" registry root values : HKEY_LOCAL_MACHINE\n")
			, argv[0] );

}

SaneWinMain( argc, argv )
{
	int arg;
	int state = 0;

   DWORD dwType;
	TCHAR * reg_path;
   TCHAR * reg_value;
   TCHAR * reg_entry;
	HKEY hkey_root;
	if( argc < 2 )
	{
		Usage( argv );
		return 0;
	}
	for( arg = 1; arg < argc; arg++ )
	{
		switch( state )
		{
		case 0:
			if( StrCaseCmp( argv[arg], TEXT("STRING") ) == 0 )
			{
				dwType = REG_SZ;
				state++;
			}
			else if( StrCaseCmp( argv[arg], TEXT("DWORD") ) == 0 )
			{
				dwType = REG_DWORD;
				state++;
			}
			else
			{
				printf( TEXT("Bad value type [%s] at position %d\n"), argv[arg], arg );
				return 0;
			}
			break;
		case 1:
			if( StrCaseCmp( argv[arg], TEXT("HKEY_LOCAL_MACHINE") ) == 0 )
			{
				hkey_root = HKEY_LOCAL_MACHINE;
				state++;
			}
			else
			{
				printf( TEXT("Bad registry root value [%s] at position %d\n"), argv[arg], arg );
				return 0;
			}
			break;
		case 2:
			reg_path = argv[arg];
			state++;
			break;
		case 3:
			reg_entry = argv[arg];
			state++;
			break;
		case 4:
			switch( dwType )
			{
			case REG_SZ:
				if( !SetRegistryItem( hkey_root, reg_path, reg_entry, dwType, argv[arg], strlen( argv[arg] ) ) )
					printf( TEXT("Failed to set string item: %d"), GetLastError() );
				break;
			case REG_DWORD:
				{
					DWORD dwVal = atoi( argv[arg] );
					if( !SetRegistryItem( hkey_root, reg_path, reg_entry, dwType, (TCHAR*)&dwVal, 4 ) )
						printf( TEXT("Failed to set dword item: %d"), GetLastError() );

				}
				break;
			}
         state = 0;
         break;
		}
	}
   return 0;
}
EndSaneWinMain()


