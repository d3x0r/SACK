#define NO_UNICODE_C
#include <sack_types.h>
#define SUFFER_WITH_NO_SNPRINTF
#include <final_types.h>
#ifdef __WATCOMC__
#undef snprintf
#endif
#include <stdio.h>
#include <string.h>

#if defined( __LINUX64__ )
#define SHARED_LIBPATH "lib64"
#define LINK_LIBPATH "lib64"
#define SHARED_BINPATH "bin"
#elif defined( __LINUX__ )
#define SHARED_LIBPATH "lib"
#define LINK_LIBPATH "lib"
#define SHARED_BINPATH "bin"
#else
#define SHARED_LIBPATH "bin"
#define LINK_LIBPATH "lib"
#define SHARED_BINPATH "bin"
#endif


#ifdef WIN32
#include <windows.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

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
   optional_wide = szString;
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
		tmp = pKey;
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
		FILE *file = fopen(  "tmp.reg", "wt" ); \
		fprintf( file, "Windows Registry Editor Version 5.00\r\n\r\n[HKEY_LOCAL_MACHINE\\" MyKey "]\r\n\"Install_Dir\"=\"%s\"", path ); \
		fclose( file ); \
		system( "start /wait tmp.reg" ); \
		unlink( "tmp.reg" ); \
	} \
	else \
		SetRegistryItem( HKEY_LOCAL_MACHINE, "SOFTWARE", "\\Freedom Collective\\" program, "Install_Dir", REG_SZ, (BYTE*)path, (int)strlen(path));


#endif

void TrimLastPathPart( char *path )
{
	{
		char *last1 = strrchr( path, '/' );
		char *last2 = strrchr( path, '\\' );
		char *last;
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
	}
}

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
	char *path = argv[0];
	FILE *out;
	char tmp[256];
	int no_registry = 0;
	if( argc > 1 )
	{
		char *tmp = argv[1];
		if( strcmp( tmp, "-nr" ) == 0 )
			no_registry = 1;
	}

	TrimLastPathPart( path ); // remove executable name from path
	//TrimLastPathPart( path ); // remove /${CMAKE_BUILD_TYPE}/
	TrimLastPathPart( path ); // remove /bin/

	snprintf( tmp, sizeof( tmp ), "%s/CMakePackage", path );
	out = fopen( tmp, "wt" );
	if( out )
	{
		if( !no_registry )
		{
			int c;
			for( c = 0; path[c]; c++ )
				if( path[c] == '\\' ) path[c] = '/';
#ifdef WIN32
			if( stricmp( path, GetKey() ) != 0 )
			{
				SetInstall( "${CMAKE_PROJECT_NAME}", path );
			}
			else
				printf( "Registry already set.\n" );
			if(0)
			{
				FILE *out2;
				snprintf( tmp, sizeof( tmp ), "%s/CMakePackage", path );
#ifdef _UNICODE
				{
               char tmp2[256];
					char *tmpname;
               snprintf( tmp2, sizeof( tmp2 ), "%s/MakeShortcut.vbs", path );
					tmpname = tmp2;
#  ifdef _MSC_VER
					fopen_s( &out2, tmpname, "wt" );
#  else
					out2 = fopen( tmpname, "wt" );
#  endif
#else
#  ifdef _MSC_VER
					fopen_s( &out2, "%s/MakeShortcut.vbs", "wt" );
#  else
					out2 = fopen( "%s/MakeShortcut.vbs", "wt" );
#  endif
#endif
					if( out2 )
					{
						fprintf( out2, "set WshShell = WScript.CreateObject(\"WScript.Shell\" )\n" );
						fprintf( out2, "strDesktop = WshShell.SpecialFolders(\"AllUsersDesktop\" )\n" );
						fprintf( out2, "set oShellLink = WshShell.CreateShortcut(strDesktop & \"\\shortcut name.lnk\" )\n" );
						fprintf( out2, "oShellLink.TargetPath = \"c:\\application folder\\application.exe\"\n" );
						fprintf( out2, "oShellLink.WindowStyle = 1\n" );
						fprintf( out2, "oShellLink.IconLocation = \"c:\\application folder\\application.ico\"\n" );
						fprintf( out2, "oShellLink.Description = \"Shortcut Script\"\n" );
						fprintf( out2, "oShellLink.WorkingDirectory = \"c:\\application folder\"\n" );
						fprintf( out2, "oShellLink.Save\n" );
						fclose( out2 );
#ifdef _UNICODE
						system( tmpname );
#else
						system( tmp );
#endif
					}
#ifdef _UNICODE
					//Deallocate( char *, tmpname );
				}
#endif
			}
#endif
		}

		fprintf( out, "#set was_monolithic_build to build mode\n" );
		fprintf( out, "set( WAS_MONOLITHIC ${BUILD_MONOLITHIC} )\n" );
		fprintf( out, "\n" );
#if MAKE_RCOORD_SINGLE
		fprintf( out, "add_definitions( -DMAKE_RCOORD_SINGLE )\n" );
#endif

#if __BULLET_ENABLED__
		fprintf( out, "set( BULLET_SOURCE ${BULLET_SOURCE} )\n" );
#ifdef BT_USE_DOUBLE_PRECISION
		fprintf( out, "add_definitions( -DBT_USE_DOUBLE_PRECISION )\n" );
#endif
      // (after MiniCL) BulletWorldImporter
		fprintf( out, "set( BULLET_LIBRARIES BulletMultiThreaded MiniCL BulletSoftBody BulletDynamics BulletCollision LinearMath ) \n" );
#endif
		fprintf( out, "\n" );
		fprintf( out, "enable_language(C)\n" );
		fprintf( out, "enable_language(CXX)\n" );
		fprintf( out, "\n" );
		fprintf( out, "if( MSVC )\n" );
		fprintf( out, "set( SUPPORTS_PARALLEL_BUILD_TYPE 1 )\n" );
		fprintf( out, "endif( MSVC )\n" );
		fprintf( out, "\n" );
		fprintf( out, "set( SACK_BASE %s )\n", SlashFix( path ) );
		fprintf( out, "set( SACK_INCLUDE_DIR $""{SACK_BASE}/include/SACK )\n" );
		fprintf( out, "set( SACK_BAG_PLUSPLUS @SACK_BAG_PLUSPLUS@ )\n" );
		fprintf( out, "if( WAS_MONOLITHIC )\n" );
		fprintf( out, "set( SACK_LIBRARIES sack_bag $""{SACK_BAG_PLUSPLUS} )\n" );
		fprintf( out, "else( WAS_MONOLITHIC )\n" );
		fprintf( out, "set( SACK_LIBRARIES ${BAG_PLUSPLUS} bag ${BAG_PSI_PLUSPLUS} bag.psi bag.externals )\n" );
		fprintf( out, "endif( WAS_MONOLITHIC )\n" );
		fprintf( out, "set( SACK_LIBRARY_DIR $""{SACK_BASE}/lib )\n" );
		fprintf( out, "\n" );
		fprintf( out, "set( USE_OPTIONS ${USE_OPTIONS} )\n" );
		fprintf( out, "if( NOT USE_OPTIONS )\n" );
		fprintf( out, "add_definitions( -D__NO_OPTIONS__ )\n" );
		fprintf( out, "endif( NOT USE_OPTIONS )\n" );
#ifdef MINGW_SUX
		fprintf( out, "add_definitions( -DMINGW_SUX )\n" );
#endif
		fprintf( out, "set( __NO_GUI__ ${__NO_GUI__} )\n" );
		fprintf( out, "if( __NO_GUI__ )\n" );
		fprintf( out, "add_definitions( -D__NO_GUI__ )\n" );
		fprintf( out, "endif( __NO_GUI__ )\n" );
		fprintf( out, "set( __LINUX__ ${__LINUX__} )\n" );
		fprintf( out, "set( __LINUX__ ${__LINUX__} )\n" );
		fprintf( out, "set( __LINUX64__ ${__LINUX64__} )\n" );
		#ifdef __LINUX__
		fprintf( out, "add_definitions( -D__LINUX__ )\n" );
		#endif
		#ifdef __LINUX64__
		fprintf( out, "add_definitions( -D__LINUX64__ )\n" );
		#endif
		#ifdef __64__
		fprintf( out, "add_definitions( -D__64__ )\n" );
		#endif
		#ifdef __WINDOWS__
		fprintf( out, "add_definitions( -D__WINDOWS__ )\n" );
		#endif
		fprintf( out, "set( WIN_SYS_LIBS ${WIN_SYS_LIBS} )\n" );
		fprintf( out, "set( SOCKET_LIBRARIES ${SOCKET_LIBRARIES} )\n" );
		fprintf( out, "\n" );
		fprintf( out, "\n" );
		fprintf( out, "set(  CMAKE_CXX_FLAGS_DEBUG \"$""{CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG\" )\n" );
		fprintf( out, "set(  CMAKE_CXX_FLAGS_RELWITHDEBINFO \"$""{CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_DEBUG\" )\n" );
		fprintf( out, "set(  CMAKE_C_FLAGS_DEBUG \"$""{CMAKE_C_FLAGS_DEBUG} -D_DEBUG\" )\n" );
		fprintf( out, "set(  CMAKE_C_FLAGS_RELWITHDEBINFO \"$""{CMAKE_C_FLAGS_RELWITHDEBINFO} -D_DEBUG\" )\n" );
		fprintf( out, "set(  SACK_REPO_REVISION \"${CURRENT_REPO_REVISION}\" )\n" );
		fprintf( out, "set(  SACK_BUILD_TYPE \"${CMAKE_BUILD_TYPE}\" )\n" );
		fprintf( out, "set(  SACK_GENERATOR \"${CMAKE_GENERATOR}\" )\n" );
		fprintf( out, "set(  SACK_PROJECT_NAME \"${CMAKE_PROJECT_NAME}\" )\n" );
		fprintf( out, "\n" );
		fprintf( out, "  if( $""{CMAKE_COMPILER_IS_GNUCC} )\n" );
		fprintf( out, "    if( UNIX )\n" );
		fprintf( out, "      SET( CMAKE_EXE_LINKER_FLAGS \"-Wl,--as-needed\" )\n" );
		fprintf( out, "      SET( CMAKE_SHARED_LINKER_FLAGS \"-Wl,--as-needed\" )\n" );
		fprintf( out, "      SET( CMAKE_MODULE_LINKER_FLAGS \"-Wl,--as-needed\" )\n" );
		fprintf( out, "    endif( UNIX )\n" );
		fprintf( out, "    SET( FIRST_GCC_LIBRARY_SOURCE $""{SACK_BASE}/src/sack/deadstart_list.c )\n" );
		fprintf( out, "    SET( FIRST_GCC_PROGRAM_SOURCE $""{SACK_BASE}/src/sack/deadstart_list.c )\n" );
		fprintf( out, "    SET( LAST_GCC_LIBRARY_SOURCE $""{SACK_BASE}/src/sack/deadstart_lib.c $""{SACK_BASE}/src/sack/deadstart_end.c )\n" );
		fprintf( out, "    SET( LAST_GCC_PROGRAM_SOURCE $""{SACK_BASE}/src/sack/deadstart_lib.c $""{SACK_BASE}/src/sack/deadstart_prog.c $""{SACK_BASE}/src/sack/deadstart_end.c )\n" );
		fprintf( out, "  endif()\n" );
		fprintf( out, "\n" );
		fprintf( out, "if( MSVC OR WATCOM )\n" );
		fprintf( out, "  SET( LAST_GCC_PROGRAM_SOURCE $""{SACK_BASE}/src/sack/deadstart_prog.c )\n" );
		fprintf( out, "endif( MSVC OR WATCOM )\n" );
		fprintf( out, "\n" );
		fprintf( out, "add_definitions( -D_WIN32_WINNT=${WIN32_VERSION} -DWINVER=${WIN32_VERSION})\n" );
      fprintf( out, "\n" );
		fprintf( out, "if( MSVC )\n" );
      // remove snprintf deprication and posix warning
		fprintf( out, "add_definitions( -D_CRT_SECURE_NO_WARNINGS -wd4995 -wd4996)\n" );
		fprintf( out, "    if( CMAKE_CL_64 )\n" );
		fprintf( out, "      add_definitions( -D_AMD64_ -D__64__ -D_WIN64 )\n" );
		fprintf( out, "    else( CMAKE_CL_64 )\n" );
		fprintf( out, "      add_definitions( -D_X86_ )\n" );
		fprintf( out, "    endif( CMAKE_CL_64 )\n" );
		fprintf( out, "endif( MSVC )\n" );
#if UNICODE
		fprintf( out, "add_definitions( -DUNICODE )\n" );
#endif
#if _UNICODE
		fprintf( out, "add_definitions( -D_UNICODE )\n" );
#endif

		fprintf( out, "SET( DATA_INSTALL_PREFIX resources )\n" );
      fprintf( out, "\n" );
		fprintf( out, "#### Carried Definition of how library was linked\n" );
		fprintf( out, "SET( FORCE_MSVCRT ${FORCE_MSVCRT})\n" );
		fprintf( out, "SET( SACK_PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES})\n" );
		fprintf( out, "SET( sack_extra_link_flags ${extra_link_flags})\n" );
      fprintf( out, "\n" );
		fprintf( out, "include( $""{SACK_BASE}/DefaultInstall.cmake )\n" );
		fprintf( out, "if( NOT WAS_MONOLITHIC )\n" );
		fprintf( out, "macro( INSTALL_SACK dest )\n" );
		fprintf( out, "\n" );
		fprintf( out, "if(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}service_list${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}sack.msgsvr.service${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.externals${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.video${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.psi${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PSI_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.video.puregl${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image.puregl${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image.puregl2${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}glew${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "if( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/Images/frame_border.png DESTINATION $""{dest}/Images )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/EditOptions${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/DumpFontCache${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "endif( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/SetOption${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/loginfo.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/application_delay.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/importini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/exportini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		
		// there is no interface.conf for linux (yet)
#ifndef __LINUX__
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/interface.conf DESTINATION $""{dest} )\n" );
#endif
		fprintf( out, "else(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/service_list${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.externals${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.video${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.psi${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}${BAG_PSI_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
#ifndef __LINUX__
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.video.puregl${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image.puregl${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}bag.image.puregl2${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}glew${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
#endif
#ifndef __NO_OPTIONS__
		fprintf( out, "if( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/Images/frame_border.png DESTINATION $""{dest}/Images )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/EditOptions${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/DumpFontCache${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "endif( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/SetOption${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/loginfo.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/application_delay.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/importini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/exportini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
#endif
		// there is no interface.conf for linux (yet)
#ifndef __LINUX__
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/interface.conf DESTINATION $""{dest} )\n" );
#endif
		fprintf( out, "endif(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "ENDMACRO( INSTALL_SACK )\n" );
		fprintf( out, "else( NOT WAS_MONOLITHIC )\n" );
		fprintf( out, "add_definitions( -DFORCE_NO_INTERFACE )\n" );
		fprintf( out, "macro( INSTALL_SACK dest )\n" );
		fprintf( out, "\n" );
		fprintf( out, "if(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}sack_bag${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "if( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/Images/frame_border.png DESTINATION $""{dest}/Images )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/EditOptions${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/DumpFontCache${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "endif( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/SetOption${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/loginfo.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/application_delay.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/importini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/exportini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		// there is no interface.conf for linux (yet)
#ifndef __LINUX__
		fprintf( out, "install( FILES $""{SACK_BASE}/bin/interface.conf DESTINATION $""{dest} )\n" );
#endif
		fprintf( out, "else(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}${SACK_BAG_PLUSPLUS}${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/${CMAKE_SHARED_LIBRARY_PREFIX}sack_bag${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "if( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/Images/frame_border.png DESTINATION $""{dest}/Images )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/EditOptions${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/DumpFontCache${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "endif( NOT __NO_GUI__ )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/SetOption${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/loginfo.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/application_delay.module DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_BINPATH "/msgsvr${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/sack.msgsvr.service.plugin DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/importini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/exportini${CMAKE_EXECUTABLE_SUFFIX} DESTINATION $""{dest} )\n" );
		
		// there is no interface.conf for linux (yet)
#ifndef __LINUX__
		fprintf( out, "install( FILES $""{SACK_BASE}/" SHARED_LIBPATH "/interface.conf DESTINATION $""{dest} )\n" );
#endif
		fprintf( out, "endif(SUPPORTS_PARALLEL_BUILD_TYPE)\n" );
		fprintf( out, "ENDMACRO( INSTALL_SACK )\n" );
		fprintf( out, "endif( NOT WAS_MONOLITHIC )\n" );
		fprintf( out, "\n" );

		//fprintf( out, "IF(CMAKE_BUILD_TPYE_INITIALIZED_TO_DEFAULT)\n" );

		fprintf( out, "set(CMAKE_BUILD_TYPE \"${CMAKE_BUILD_TYPE}\" CACHE STRING \"Set build type\")\n" );
		fprintf( out, "set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS $""{CMAKE_CONFIGURATION_TYPES} Debug Release RelWithDebInfo MinSizeRel )\n" );
                
		//fprintf( out, "ENDIF(CMAKE_BUILD_TPYE_INITIALIZED_TO_DEFAULT)\n" );

		fprintf( out, "\n" );

		fclose( out );
	}
	return 0;
}
