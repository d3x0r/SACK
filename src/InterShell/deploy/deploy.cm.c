#define NO_UNICODE_C

#include <sack_types.h>
#define SUFFER_WITH_NO_SNPRINTF
#include <final_types.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

#include "deploy.package.h"

#define MySubKey "Freedom Collective\\${CMAKE_PROJECT_NAME}"
#define MyKey "SOFTWARE\\" MySubKey
#define NewArray(a,b)  (a*)malloc( sizeof(a)*b )
#define Deallocate(a,b)  free(b)

char * MyWcharConvert ( const wchar_t *wch )
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
	char    *ch;
	for( len = 0; wch[len]; len++ );
	sizeInBytes = ((len + 1) * sizeof( wchar_t ));
	err = 0;
	ch = NewArray( char, sizeInBytes);
#if defined( _MSC_VER )
	err = wcstombs_s(&convertedChars, 
                    ch, sizeInBytes,
						  wch, sizeInBytes);
#else
	convertedChars = wcstombs( ch, wch, sizeInBytes);
   err = ( convertedChars == -1 );
#endif
	if (err != 0)
		printf( "wcstombs_s  failed!\n" );

	return ch;
}

wchar_t * MyCharWConvert ( const char *wch )
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
	wchar_t   *ch;
	for( len = 0; wch[len]; len++ );
	sizeInBytes = ((len + 1) * sizeof( char ) );
	err = 0;
	ch = NewArray( wchar_t, sizeInBytes);
#if defined( _MSC_VER )
	err = mbstowcs_s(&convertedChars, 
                    ch, sizeInBytes,
						  wch, sizeInBytes * sizeof( wchar_t ) );
#else
	convertedChars = mbstowcs( ch, wch, sizeInBytes);
   err = ( convertedChars == -1 );
#endif
	if (err != 0)
		printf( "wcstombs_s  failed!\n" );

	return ch;
}



int SetRegistryItem( HKEY hRoot, char *pPrefix,
                     char *pProduct, char *pKey, 
                     DWORD dwType,
                     const BYTE *pValue, int nSize )
{
   char szString[512];
   char *pszString = szString;
   DWORD dwStatus;
	HKEY hTemp;
	TEXTSTR optional_wide;
	TEXTSTR tmp;

   if( pProduct )
      snprintf( szString, sizeof( szString ),"%s%s", pPrefix, pProduct );
   else
      snprintf( szString, sizeof( szString ),"%s", pPrefix );
#ifdef _UNICODE
	optional_wide = MyCharWConvert( szString );
#else
   optional_wide = strdup( szString );
#endif
   dwStatus = RegOpenKeyEx( hRoot,
                            optional_wide, 0,
									KEY_WRITE, &hTemp );
   if( dwStatus == ERROR_FILE_NOT_FOUND )
   {
      DWORD dwDisposition;
      dwStatus = RegCreateKeyEx( hRoot, 
                                 optional_wide, 0
                             , WIDE("")
                             , REG_OPTION_NON_VOLATILE
                             , KEY_WRITE
                             , NULL
                             , &hTemp
                             , &dwDisposition);
      if( dwDisposition == REG_OPENED_EXISTING_KEY )
         fprintf( stderr, "Failed to open, then could open???" );
		if( dwStatus )   // ERROR_SUCCESS == 0
		{
#ifdef _UNICODE
			Deallocate( TEXTSTR, optional_wide );
#endif
			return FALSE;
		}
   }
   if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
#ifdef _UNICODE
		tmp = MyCharWConvert( pKey );
#else
		tmp = strdup( pKey );
#endif
      dwStatus = RegSetValueEx(hTemp, tmp, 0
                                , dwType
										, pValue, nSize );
      Deallocate( TEXTSTR, tmp );
      RegCloseKey( hTemp );
      if( dwStatus == ERROR_SUCCESS )
      {
#ifdef _UNICODE
			Deallocate( TEXTSTR, optional_wide );
			Deallocate( TEXTSTR, tmp );
#endif
         return TRUE;
      }
   }
#ifdef _UNICODE
	Deallocate( TEXTSTR, optional_wide );
	Deallocate( TEXTSTR, tmp );
#endif
   return FALSE;
}

char *GetKey( void )
{
	static char old_path[256];
	DWORD dwType;
	DWORD data = 0;
	HKEY hTemp;
	DWORD dwStatus;
	DWORD data_size = sizeof( old_path );
	TEXTSTR optional_wide;
#ifdef _UNICODE
	optional_wide = MyCharWConvert( MyKey );
#else
	optional_wide = MyKey;
#endif
	dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									 optional_wide, 0,
                            KEY_READ, &hTemp );
	if( dwStatus == ERROR_SUCCESS )
	{
		dwStatus = RegQueryValueEx( hTemp,
											WIDE("Install_Dir"),
											NULL,
											&dwType,
											(LPBYTE)old_path,
											&data_size );
      //printf( "Result was %d\n", dwStatus );
	}
#ifdef _UNICODE
	Deallocate( TEXTSTR, optional_wide );
#endif
   //printf( "Result is %s[%s]\n", MyKey, old_path );
	return old_path;
}

int CheckUAC( void )
{
	DWORD dwType;
	DWORD data = 0;
	HKEY hTemp;
	DWORD dwStatus;
	DWORD data_size = sizeof( data );
	dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									WIDE("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0,
                            KEY_READ, &hTemp );
	if( dwStatus == ERROR_SUCCESS )
	{
		RegQueryValueEx( hTemp,
						WIDE("EnableLUA"),
						NULL,
						&dwType,
						(LPBYTE)&data,
						&data_size );
	}
	return data;
}

#define SetInstall(program,path) 		if( CheckUAC() )  \
	{ \
		FILE *file = fopen( "tmp.reg", "wt" ); \
		fprintf( file, "Windows Registry Editor Version 5.00\r\n\r\n[HKEY_LOCAL_MACHINE\\"MyKey"]\r\n\"Install_Dir\"=\"%s\"", path ); \
		fclose( file ); \
		system( "start /wait tmp.reg" ); \
		unlink( "tmp.reg" ); \
	} \
	else \
		SetRegistryItem( HKEY_LOCAL_MACHINE, "SOFTWARE", "\\Freedom Collective\\" program, "Install_Dir", REG_SZ, (BYTE*)path, strlen(path));

#endif

char * SlashFix( char * path )
{
   char * _path = path;
	while( path[0] )
	{
		if( path[0] == '\\' )
			path[0] = '/';
      path++;
	}
   return _path;
}

int main( int argc, char **argv )
{
	char *path = strdup( argv[0] );
	FILE *out;
	char tmp[256];
	char *last1 = strrchr( path, '/' );
	char *last2 = strrchr( path, '\\' );
	char *last;
   int no_registry = 0;
	if( argc > 1 )
	{
		if( strcmp( argv[1], "-nr" ) == 0 )
         no_registry = 1;
	}

	if( last1 )
		if( last2 )
			if( last1 > last2 )
				last = last1;
			else
				last = last2;
		else
			last = last1;
	else
		if( last2 )
			last = last2;
		else
			last = NULL;
	if( last )
		last[0] = 0;
	else
		path = ".";
	snprintf( tmp, sizeof( tmp ), "%s/CMakePackage", path );
#ifdef _MSC_VER
	fopen_s( &out, tmp, "wt" );
#else
	out = fopen( tmp, "wt" );
#endif
	{
		int c;
		for( c = 0; path[c]; c++ )
			if( path[c] == '\\' ) path[c] = '/';
	}
	if( out )
	{
		if( !no_registry )
		{
#ifdef WIN32
			if( stricmp( path, GetKey() ) != 0 )
			{
				SetInstall( "${CMAKE_PROJECT_NAME}", path );
			}
#endif
		}

      {
			const char *replace_start;
			const char *replace_end;
			replace_start = strstr( package, "@@@" );
			if( replace_start )
			{
				fprintf( out, "%*.*s", replace_start - package, replace_start - package, package );
				fprintf( out, "%s", SlashFix( path ) );
				fprintf( out, "%s", replace_start + 3 );
			}
			else
				fprintf( out, "%s", package );
		}


		fclose( out );
	}
	return 0;
}
