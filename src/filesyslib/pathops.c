
#include <stdhdrs.h>
#include <sack_types.h>
#include <string.h>
#include <filesys.h>
#include <logging.h>
#ifdef __LINUX__
#include <time.h>
#include <sys/stat.h>
#endif
//-----------------------------------------------------------------------

FILESYS_NAMESPACE

extern TEXTSTR ExpandPath( CTEXTSTR path );

 CTEXTSTR  pathrchr ( CTEXTSTR path )
{
	CTEXTSTR end1, end2;
	end1 = StrRChr( path, '\\' );
	end2 = StrRChr( path, '/' );
	if( end1 > end2 )
		return end1;
   return end2;
}

#ifdef __cplusplus
 TEXTSTR  pathrchr ( TEXTSTR path )
{
	TEXTSTR end1, end2;
	end1 = StrRChr( path, '\\' );
	end2 = StrRChr( path, '/' );
	if( end1 > end2 )
		return end1;
   return end2;
}
#endif

//-----------------------------------------------------------------------

 CTEXTSTR  pathchr ( CTEXTSTR path )
{
	CTEXTSTR end1, end2;
	end1 = StrChr( path, (int)'\\' );
	end2 = StrChr( path, (int)'/' );
	if( end1 && end2 )
	{
		if( end1 < end2 )
			return end1;
	  return end2;
	}
	else if( end1 )
		return end1;
	else if( end2 )
	  return end2;
   return NULL;
}

//-----------------------------------------------------------------------

TEXTSTR GetCurrentPath( TEXTSTR path, int len )
{
	if( !path )
		return 0;
#ifndef UNDER_CE
#  ifdef _WIN32
	GetCurrentDirectory( len, path );
#  else
#	  ifdef UNICODE
	{
		char _path[256];
		TEXTCHAR *tmppath;
		//getcwd( _path, 256 );
		//tmppath = DupCStr( _path );
		//StrCpyEx( path, tmppath, len );
		path[0] = '.';
      path[1] = 0;
	}
#	  else
	getcwd( path, len );
#	  endif
#  endif
#endif
	return path;
}

#ifndef _WIN32
static void convert( P_64 outtime, time_t *time )
{
#warning convert time function is incomplete.
	*outtime = *time;
}
#endif

//-----------------------------------------------------------------------

_64 GetTimeAsFileTime ( void )
{
#if defined( __LINUX__ )
	struct timeval tmp;
	struct timezone tz;
	FILETIME result;
	gettimeofday( &tmp, &tz );
	result = ( tmp.tv_usec * 10LL ) + ( tmp.tv_sec * 1000LL * 1000LL * 10LL );
	return result;
#else
	SYSTEMTIME st;
	FILETIME result;
	GetLocalTime( &st );
	SystemTimeToFileTime( &st, &result );
	return *(_64*)&result;
#endif
}

 _64  GetFileWriteTime( CTEXTSTR name ) // last modification time.
{
	TEXTSTR tmppath = ExpandPath( name );
#ifdef _WIN32
	HANDLE hFile = CreateFile( tmppath
								  , 0 // device access?
								  , FILE_SHARE_READ|FILE_SHARE_WRITE
								  , NULL
								  , OPEN_EXISTING
								  , 0
								  , NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		FILETIME filetime;
		//_64 realtime;
		GetFileTime( hFile, NULL, NULL, &filetime );
		CloseHandle( hFile );
		//realtime = *(_64*)&filetime;
		//realtime *= 100; // nano seconds?
		return *(_64*)&filetime;
	}
	return 0;
#else
	struct stat statbuf;
	 _64 realtime;
#ifdef UNICODE
	{
		char *tmpname = CStrDup( tmppath );
		stat( tmpname, &statbuf );
		Release( tmpname );
	}
#else
	stat( name, &statbuf );
#endif
	convert( &realtime, (time_t*)&statbuf.st_mtime );
	return realtime;
#endif	
	return 0;
}

//-----------------------------------------------------------------------

 LOGICAL  SetFileWriteTime( CTEXTSTR name, _64 filetime ) // last modification time.
{
#ifdef _WIN32
	HANDLE hFile = CreateFile( name
								  , GENERIC_WRITE // device access?
								  , FILE_SHARE_READ|FILE_SHARE_WRITE
								  , NULL
								  , OPEN_EXISTING
								  , 0
								  , NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		//_64 realtime;
		SetFileTime( hFile, NULL, NULL, (CONST FILETIME*)&filetime );
		CloseHandle( hFile );
		//realtime = *(_64*)&filetime;
	   //realtime *= 100; // nano seconds?
	  return TRUE;
	}
	return FALSE;
#else
	struct stat statbuf;
	_64 realtime;
#ifdef UNICODE
	 {
		 int status;
	   char *tmpname = CStrDup( name );
		 stat( tmpname, &statbuf );
		 Release( tmpname );
	 }
#else
	 stat( name, &statbuf );
#endif
	convert( &realtime, (time_t*)&statbuf.st_mtime );
	return realtime;
#endif	
	return 0;
}

#ifdef WIN32
_64 ConvertFileTimeToInt( const FILETIME *filetime )
{
	ULARGE_INTEGER tmp;
	tmp.u.LowPart = filetime->dwLowDateTime;
	tmp.u.HighPart = filetime->dwHighDateTime;
	return tmp.QuadPart;
}

void ConvertFileIntToFileTime( _64 int_filetime, FILETIME *filetime )
{
	ULARGE_INTEGER tmp;
	tmp.QuadPart = int_filetime;
	filetime->dwLowDateTime  = tmp.u.LowPart;
	filetime->dwHighDateTime = tmp.u.HighPart;
}
#endif

LOGICAL  SetFileTimes( CTEXTSTR name
							, _64 time_create  // last modification time.
							, _64 time_modify // last modification time.
							, _64 time_access  // last modification time.
							)
{
#ifdef _WIN32
	LOGICAL result = TRUE;
	HANDLE hFile = CreateFile( name
									 , GENERIC_ALL //GENERIC_WRITE|FILE_WRITE_ATTRIBUTES //GENERIC_ALL // device access?
									 , FILE_SHARE_READ|FILE_SHARE_WRITE
									 , NULL
									 , OPEN_EXISTING
									 , FILE_FLAG_BACKUP_SEMANTICS
									 , NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		FILETIME filetime_create;
		FILETIME filetime_modify;
		FILETIME filetime_access;
		ULARGE_INTEGER tmp;
		//_64 realtime;
		tmp.QuadPart = time_create;
		filetime_create.dwLowDateTime = tmp.u.LowPart;
		filetime_create.dwHighDateTime = tmp.u.HighPart;
		tmp.QuadPart = time_access;
		filetime_access.dwLowDateTime = tmp.u.LowPart;
		filetime_access.dwHighDateTime = tmp.u.HighPart;
		tmp.QuadPart = time_modify;
		filetime_modify.dwLowDateTime = tmp.u.LowPart;
		filetime_modify.dwHighDateTime = tmp.u.HighPart;
#if 0
		{
			TEXTCHAR buf[3][64];
			SYSTEMTIME st;
			FileTimeToSystemTime( &time_create, &st );
			snprintf( buf[0], 64, "%02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &time_access, &st );
			snprintf( buf[1], 64,"%02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );
			FileTimeToSystemTime( &time_modify, &st );
			snprintf( buf[2], 64, "%02d/%02d/%04d %02d:%02d:%02d.%03d", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds );

			lprintf( "File times on [%s] are to be : %s %s %s", name, buf[0], buf[1], buf[2] );
		}
#endif
		if( !SetFileTime( hFile, (CONST FILETIME*)&filetime_create, (CONST FILETIME*)&filetime_access, (CONST FILETIME*)&filetime_modify ) )
		{
			result = FALSE;
			lprintf( WIDE("Failed to set times:(%s)%d"), name, GetLastError() );
		}
		CloseHandle( hFile );
		//realtime = *(_64*)&filetime;
		//realtime *= 100; // nano seconds?
		return result;
	}
	else
	{
		lprintf( WIDE("Failed to open to set time on %s:%d"), name, GetLastError() );
	}
	return FALSE;
#else
	struct stat statbuf;
	 _64 realtime;
#ifdef UNICODE
	 {
	   char *tmpname = CStrDup( name );
		 stat( tmpname, &statbuf );
	   Release( tmpname );
	 }
#else
	 stat( name, &statbuf );
#endif
	convert( &realtime, (time_t*)&statbuf.st_mtime );
	return realtime;
#endif	
	return 0;
}

//-----------------------------------------------------------------------

LOGICAL  IsPath ( CTEXTSTR path )
{
	
	if( !path )
		return 0;
#ifdef _WIN32
	{
		DWORD dwResult;
		dwResult = GetFileAttributes( path );
		if( dwResult == 0xFFFFFFFF )
			return 0;
		if( dwResult & FILE_ATTRIBUTE_DIRECTORY ) 
			return 1;
		return 0;
	}
#else
	 {
		 struct stat statbuf;
#ifdef UNICODE
		 {
			 int status;
			 char *tmppath = CStrDup( path );
			 stat( tmppath, &statbuf );
		  Release( tmppath );
		 }
#else
		 stat( path, &statbuf );
#endif
		 return S_ISDIR( statbuf.st_mode );
	 }
#endif
}

//-----------------------------------------------------------------------

 int  MakePath ( CTEXTSTR path )
{
	int status;
	if( !path )
		return 0;
#ifdef _WIN32
	status = CreateDirectory( path, NULL );
	if( !status )
	{
		TEXTSTR tmppath = StrDup( path );
		TEXTSTR last = (TEXTSTR)pathrchr( tmppath );
		if( last )
		{
			last[0] = 0;
			MakePath( tmppath );
			status = CreateDirectory( path, NULL );
		}
	}
	return status;
#else
#ifdef UNICODE
	 {
		 int status;
	   char *tmppath = CStrDup( path );
		 status = mkdir( tmppath, -1 ); // make directory with full umask permissions
		 Release( tmppath );
		 return !status;
	 }
#else
	 return !mkdir( path, -1 ); // make directory with full umask permissions
#endif
#endif
}

//-----------------------------------------------------------------------

int  SetCurrentPath ( CTEXTSTR path )
{
	int status = 1;
   TEXTSTR tmp_path;
	if( !path )
		return 0;
   tmp_path = ExpandPath( path );
#ifndef UNDER_CE
#  ifdef _WIN32
	status = SetCurrentDirectory( tmp_path );
#  else
#	ifdef UNICODE
	 {
	   char *tmppath = CStrDup( path );
		 status = chdir( tmppath ); // make directory with full umask permissions
		 Release( tmppath );
	 }
#	else
	 status = !chdir( tmp_path );
#	endif
#  endif
   Release( tmp_path );
	if( status )
	{
		TEXTCHAR tmp[256];
		path = GetCurrentPath( tmp, sizeof( tmp ) );
		SetDefaultFilePath( path );
	}
	else
	{
		TEXTCHAR tmp[256];
		lprintf( WIDE( "Failed to change to [%s](%d) from %s" ), path, GetLastError(), GetCurrentPath( tmp, sizeof( tmp ) ) );
	}
#endif

	return status;
}

LOGICAL IsAbsolutePath( CTEXTSTR path )
{
	if(path)
	{
#ifdef WIN32
		if( ( path[0] && path[1] && path[2] ) &&
			  ( ( ( ( path[0] >= 'a' && path[0] <= 'z' )
				  || ( path[0] >= 'A' && path[0] <= 'Z' ) )
				  && ( path[1] == ':' )
				  && ( path[2] == '/' || path[2] == '\\' ) )
				|| ( path[0] == '/' && path[1] == '/' )
				|| ( path[0] == '\\' && path[1] == '\\' )
			  || ( path[0] == '/' || path[0] == '\\' ) )
		  )
			return TRUE;
#else
		if( path[0] == '/' || path[0] == '\\' )
			return TRUE;
#endif
	}
	return FALSE;
}

LOGICAL SetFileLength( CTEXTSTR path, size_t length )
{
#ifdef __LINUX__
	// files are by default binary in linux
#  ifndef O_BINARY
#	define O_BINARY 0
#  endif
#endif
	INDEX file;
	file = sack_iopen( 0, path, O_RDWR|O_BINARY );
	if( file == INVALID_INDEX )
		return FALSE;
	sack_ilseek( file, length, SEEK_SET );
	sack_iset_eof( file );
	sack_iclose( file );
	return TRUE;
}


FILESYS_NAMESPACE_END


//-----------------------------------------------------------------------
// $Log: pathops.c,v $
// Revision 1.14  2005/05/06 18:15:24  jim
// Add ability to set file write time.
//
// Revision 1.13  2004/05/27 21:37:39  d3x0r
// Syncpoint.
//
// Revision 1.12  2004/05/06 08:13:46  d3x0r
// Apply repsiective const changes to pathchr, pathrchr
//
// Revision 1.11  2004/01/12 08:42:16  panther
// Fix return type of pathchr
//
// Revision 1.10  2004/01/07 09:46:37  panther
// Fix pathops to handle const char * decl.  Disable logging
//
// Revision 1.9  2003/08/12 12:14:01  panther
// ...
//
// Revision 1.8  2003/04/21 20:02:31  panther
// Support option to return file name only
//
// Revision 1.7  2003/01/31 16:23:24  panther
// Cleaned for visual studio warnings
//
// Revision 1.6  2003/01/28 02:24:43  panther
// Fixes to network - common timer for network pause... minor updates which should have been commited already
//
// Revision 1.5  2002/08/16 21:41:11  panther
// Updated to compile under linux, also updated to new method of declaring
// public externals.
//
// Revision 1.4  2002/07/26 09:16:55  panther
// Added IsPath, CreatePath, SetCurrentPath, GetFileWriteTime
// Modified GetCurrentPath
//
//
