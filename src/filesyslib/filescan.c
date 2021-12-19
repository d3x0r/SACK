#define NO_UNICODE_C
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include <stdhdrs.h>
#include <procreg.h>
#include <stdio.h>
#include <string.h>
#if defined( _WIN32 ) && !defined( __TURBOC__ )
#include <stdhdrs.h> // windows lean_and_mean
#ifndef UNDER_CE
#include <io.h>  // findfirst,findnext, fileinfo
#endif
#else
#include <dirent.h> // opendir etc..
#include <sys/stat.h>
#endif
#include <filesys.h>

FILESYS_NAMESPACE

#include "filesys_local.h"

#ifndef MAX_PATH_NAME
#  define MAX_PATH_NAME 512
#endif
// DEBUG_COMPARE 1 == full debug
// DEBUG_COMPARE 2 == quieter debug
#ifdef _DEBUG
#define DEBUG_COMPARE 4
#else
#define DEBUG_COMPARE 999
#endif
//--------------------------------------------------------------------------

 int  CompareMask ( CTEXTSTR mask, CTEXTSTR name, int keepcase )
{
	int m = 0, n = 0;
	int anymatch;
	int wasanymatch, wasmaskmatch;
	int matchone;
	TEXTCHAR namech, maskch;
	if( !mask )
		return 1;
	if( !name )
		return 0;
try_mask:
	anymatch = 0;
	wasanymatch = 0;
	wasmaskmatch = 0;
	matchone = 0;
#if ( DEBUG_COMPARE < 3 )
	lprintf( "Check %s vs %s", mask + m, name );
#endif
	do
	{
		if( mask[m] == '\t' || mask[m] == '|' )
		{
			//lprintf( "Found mask seperator - skipping to next mask :%s", mask + m + 1 );
			n = 0;
			m++;
			continue;
		}
		while( mask[m] == '*' )
		{
			anymatch = 1;
			m++;
		}
		while( mask[m] == '?' )
		{
#if ( DEBUG_COMPARE < 2 )
         //Log( "Match any one character" );
#endif
			matchone++;
			m++;
		}
		if( !keepcase && name[n]>= 'a' && name[n] <= 'z' )
			namech = name[n] - ('a' - 'A');
		else if( name[n] == '/' )
			namech = '\\';
		else
			namech = name[n];

		if( !keepcase && mask[m]>= 'a' && mask[m] <= 'z' )
			maskch = mask[m] - ('a' - 'A');
		else if( mask[m] == '/' )
			maskch = '\\';
		else
			maskch = mask[m];


		if( matchone )
		{
			matchone--;
			n++;
		}
		else if(
#if ( DEBUG_COMPARE < 2 )
				  lprintf( "Check %c == %c?", maskch, namech ),
#endif
				  maskch == namech )
		{
#if ( DEBUG_COMPARE < 2 )
			lprintf( " yes." );
#endif
		 	if( anymatch )
		 	{
			 	wasanymatch = n+1;
			 	wasmaskmatch = m+1;
			 	anymatch = 0;
			}
		 	n++;
	 		m++;
		}
		else if(
#if ( DEBUG_COMPARE < 2 )
				  lprintf( " no. Any match?" ),
#endif
				  anymatch )
		{
#if ( DEBUG_COMPARE < 2 )
			lprintf( " yes");
#endif
			n++;
		}
		else if(
#if ( DEBUG_COMPARE < 2 )
				  lprintf( " No. wasanymatch?" ),
#endif
				  wasanymatch )
		{
#if ( DEBUG_COMPARE < 2 )
			lprintf( " yes. reset to anymatch." );
#endif
			n = wasanymatch - 1;
			m = wasmaskmatch - 1;
			anymatch = 1;
			n++;
		}
		else
		{
#if ( DEBUG_COMPARE < 2 )
			lprintf( " No. match failed." );
#endif
			break;
		}
	}while( name[n] );
	// 0 or more match a *
	// so auto match remaining *
	while( mask[m] && mask[m] == '*' )
		m++;
#if ( DEBUG_COMPARE < 3 )
	lprintf( "Skipping to next mask" );
#endif
	if( mask[m] &&
		 ( mask[m] != '\t' &&
			mask[m] != '|' ) )
	{
		int mask_m = m;
		while( mask[m] )
		{
			if( mask[m] == '\t' || mask[m] == '|' )
			{
				n = 0;
				m++;
				break;
			}
			m++;
		}
		if( mask[m] )
			goto try_mask;
		m = mask_m;
	}
	//lprintf( "Result: %d %c %d", matchone, mask[m], name[n] );
	// match ???? will not match abc
	// a??? abc not match
	if( !matchone && (!mask[m] || mask[m] == '\t' || mask[m] == '|' ) && !name[n] )
		return 1;
	return  0;
}

//--------------------------------------------------------------------------
//---------------------------------------------------------------------------

typedef struct result_buffer
{
	TEXTSTR buffer;
	int len;
	int result_len;
} RESULT_BUFFER, *PRESULT_BUFFER;

static void CPROC MatchFile( uintptr_t psvUser, CTEXTSTR name, enum ScanFileProcessFlags flags )
{
	PRESULT_BUFFER buffer = (PRESULT_BUFFER)psvUser;
	buffer->result_len = tnprintf( buffer->buffer, buffer->len*sizeof(TEXTCHAR), "%s", name );
}

int  GetMatchingFileName ( CTEXTSTR filemask, enum ScanFileFlags  flags, TEXTSTR pResult, int nResult )
{
	void *info = NULL;
	RESULT_BUFFER result_buf;
	result_buf.buffer = pResult;
	result_buf.len = nResult;
	result_buf.result_len = 0;
	// may need a while loop here...
	// but I'm just going to result the first matching anyhow.
	while( ScanFiles( NULL, filemask, &info, MatchFile, flags, (uintptr_t)&result_buf ) );
	return result_buf.result_len;
}

//---------------------------------------------------------------------------

#if defined( _WIN32 ) && !defined( __TURBOC__ )

#ifdef UNDER_CE
#define finddata_t WIN32_FIND_DATA
#define findfirst FindFirstFile
#define findnext  FindNextFile
#define findclose FindClose
#else
#  ifdef UNICODE
#define finddata_t _wfinddata_t
#define findfirst _wfindfirst
#define findnext  _wfindnext
#define findclose _findclose
#  else
#define finddata_t _finddata_t
#define findfirst _findfirst
#define findnext  _findnext
#define findclose _findclose
#  endif
#endif

#else


#endif

typedef struct myfinddata {
#ifdef WIN32
#  ifdef _MSC_VER
#define HANDLECAST intptr_t
	intptr_t
#  else
#define HANDLECAST int
	int
#  endif
#else
#  define HANDLECAST DIR*
	DIR*
#endif
		handle;

#  ifdef WIN32
#    ifdef UNDER_CE
	WIN32_FIND_DATA fd;
#    else
	struct _wfinddata_t fd;
#  endif
#endif
	struct find_cursor *cursor;
	INDEX scanning_interface_index;
	LOGICAL new_mount;
	LOGICAL single_mount;
	struct file_system_mounted_interface *scanning_mount;
	TEXTCHAR buffer[MAX_PATH_NAME];
	TEXTCHAR file_buffer[MAX_PATH_NAME];
	TEXTCHAR basename[MAX_PATH_NAME];
	TEXTCHAR findmask[MAX_PATH_NAME];
	wchar_t *findmaskw;
	struct myfinddata *current;
	struct myfinddata *prior;
	struct myfinddata **root_info;
} MFD, *PMFD;

#define findhandle(pInfo) ( ((PMFD)(*pInfo))->handle)
#define finddata(pInfo) ( &((PMFD)(*pInfo))->fd)
#define findbuffer(pInfo) ( ((PMFD)(*pInfo))->buffer)
#define findbasename(pInfo) ( ((PMFD)(*pInfo))->basename)
#define findmask(pInfo)     ( ((PMFD)(*pInfo))->findmask)
#define findmaskw(pInfo)     ( ((PMFD)(*pInfo))->findmaskw)
#define findinfo(pInfo)     (((PMFD)(*pInfo)))
#define findcursor(pInfo)     ( ((PMFD)(*pInfo))->cursor)

struct find_cursor *GetScanFileCursor( void *pInfo ) {
	PMFD mfd = (PMFD)pInfo;
	return mfd->cursor;
}

 int  ScanFilesEx ( CTEXTSTR base
           , CTEXTSTR mask
           , void **pInfo
           , void CPROC Process( uintptr_t psvUser, CTEXTSTR name, enum ScanFileProcessFlags flags )
           , enum ScanFileFlags flags
           , uintptr_t psvUser
		   , LOGICAL begin_sub_path
		   , struct file_system_mounted_interface *mount
		   )
{
	PMFD pDataCurrent = (PMFD)(pInfo);
	PMFD pData = (PMFD)(*pInfo);
	TEXTSTR tmp_base = NULL;
	enum ScanFileProcessFlags sendflags;
	int processed = 0;
#ifndef WIN32
	struct dirent *de;
#endif
	if( begin_sub_path )
	{
		pInfo = (void**)&(pDataCurrent->current);
	}
	else
		pDataCurrent = NULL;

	//lprintf( "Search in %s for %s   %d %d", base?base:"(NULL)", mask?mask:"(*)", (*pInfo)?((PMFD)*pInfo)->scanning_mount:0, (*pInfo)?((PMFD)*pInfo)->single_mount:0 );
	if( !*pInfo || begin_sub_path || ((PMFD)*pInfo)->new_mount )
	{
		TEXTCHAR findmask[4096+32];
		wchar_t findmaskw[256];
		pData = (PMFD)(*pInfo);
		if( !pData )
		{
			*pInfo = Allocate( sizeof( MFD ) );
			pData = (PMFD)(*pInfo);
			if( !( pData->scanning_mount = mount ) )
			{
				if( !winfile_local )
					SimpleRegisterAndCreateGlobal( winfile_local );

				//lprintf( "... %p", winfile_local );
				pData->single_mount = FALSE;
				pData->scanning_mount = FileSysThreadInfo.mounted_file_systems;
			}
			else
				pData->single_mount = TRUE;

			if( !pData->scanning_mount )
			{
				Deallocate( PMFD, pData );
				if( tmp_base )
					Release( tmp_base );
				return 0;
			}
			if( pData->scanning_mount->fsi && pData->scanning_mount->fsi->find_create_cursor )
			{
				char *tmp1, *tmp2;
				//lprintf( "create cursor" );
				tmp_base = ExpandPathEx( base, pData->scanning_mount->fsi );
				pData->cursor = pData->scanning_mount->fsi->find_create_cursor( pData->scanning_mount->psvInstance, tmp2 = CStrDup( tmp_base ), tmp1 = CStrDup( mask ) );
				Deallocate( char*, tmp1 );
				Deallocate( char*, tmp2 );
			}
			else
			{
				//lprintf( "no cursor" );
				pData->cursor = NULL;
			}
		}
		else
		{
			if( pData->new_mount )
			{
				if( pData->scanning_mount->fsi && pData->scanning_mount->fsi->find_create_cursor )
				{
					//lprintf( "create cursor (new mount)" );
					tmp_base = ExpandPathEx( base, pData->scanning_mount->fsi );
					pData->cursor = pData->scanning_mount->fsi->find_create_cursor( pData->scanning_mount->psvInstance, CStrDup( tmp_base ), CStrDup( mask ) );
				}
				else
					pData->cursor = NULL;
			}
		}
		pData->new_mount = FALSE;
		pData->current = NULL;
		pData->prior = pDataCurrent;

		if( pDataCurrent )
		{
			pData->root_info = pDataCurrent->root_info;
			pInfo = (void**)pData->root_info;
		}
		else
		{
			pData->root_info = (struct myfinddata**)pInfo;
		}

		(*pData->root_info) = pData;
		if( base )
		{
			TEXTSTR tmp;
			StrCpyEx( findbasename(pInfo), tmp = ExpandPathEx( base, pData->scanning_mount?pData->scanning_mount->fsi:NULL ), MAX_PATH_NAME );
			Release( tmp );
			StrCpyEx( findmask(pInfo), mask, MAX_PATH_NAME );
		}
		else
		{
			CTEXTSTR p = pathrchr( mask );
			if( p )
			{
				StrCpyEx( findbasename(pInfo), mask, p - mask + 1 );
				StrCpyEx( findmask(pInfo), p + 1, MAX_PATH_NAME );
				//mask = p + 1;
			}
			else
			{
				StrCpyEx( findbasename(pInfo), "", 2 );
				StrCpyEx( findmask(pInfo), mask, MAX_PATH_NAME );
			}
		}
		findmaskw(pInfo) = CharWConvertLen( findmask( pInfo ), strlen( findmask( pInfo ) ) );

		if( findbasename(pInfo)[0] )
			tnprintf( findmask, sizeof(findmask), "%s" PATHCHAR "*", findbasename(pInfo) );
		else {
			tnprintf( findmask, sizeof( findmask ), "*" );
		}
		{
			wchar_t* out = findmaskw;
			const char* tmp = findmask;
			while( tmp[0] ) {
				out += ConvertToUTF16( out, GetUtfChar( &tmp ) );
			}
			out[0] = 0;
		}

		if( pData->scanning_mount?pData->scanning_mount->fsi:NULL ) {
#ifndef _WIN32
			de = NULL;
#endif
			if( pData->scanning_mount->fsi->find_first( findcursor(pInfo) ) )
				findhandle(pInfo) = 0;
			else
				findhandle(pInfo) = (HANDLECAST)-1;
		} else {
#ifdef WIN32
			findhandle(pInfo) = _wfindfirst( findmaskw, finddata(pInfo) );
#else
			lprintf( "opendir [%s]", findbasename(pInfo) );
			if( !findbasename(pInfo)[0] ) {
				TEXTSTR tmp;
				tmp = ExpandPathEx( ".", pData->scanning_mount?pData->scanning_mount->fsi:NULL );
				findhandle( pInfo ) = opendir( tmp );
				Deallocate( TEXTSTR, tmp );
			} else
				findhandle( pInfo ) = opendir( findbasename(pInfo) );
			if( !findhandle(pInfo ) )
				findhandle(pInfo) = (HANDLECAST)-1;
			else
				de = readdir( (DIR*)findhandle( pInfo ) );
#endif
		}
		if( findhandle(pInfo) == (HANDLECAST)-1 )
		{
			PMFD prior = pData->prior;
			//lprintf( "first use of cursor or first open of directoy failed..." );
			if( pData->scanning_mount && pData->scanning_mount->fsi )
				pData->scanning_mount->fsi->find_close( (struct find_cursor*)findcursor(pInfo) );
			else
			{
#ifdef WIN32
				findclose( findhandle(pInfo) );
#else
				// but it failed... so ... don't close
				//closedir( findhandle( pInfo ) );
#endif
			}
			pData->scanning_mount =  pData->scanning_mount ->nextLayer;
			if( !pData->scanning_mount || pData->single_mount )
			{
				(*pData->root_info) = pData->prior;
				if( !begin_sub_path ) {
					Release( pData ); pInfo[0] = NULL;
				}
				//lprintf( "%p %d", prior, processed );
				if( tmp_base )
					Release( tmp_base );
				return prior?processed:0;
			}
			pData->new_mount = TRUE;
				if( tmp_base )
					Release( tmp_base );
			return 1;
		}
	}
	else
	{
		int r;
getnext:
		//lprintf( "returning customer..." );
		if( pData->scanning_mount?pData->scanning_mount->fsi:NULL )
			r = !pData->scanning_mount->fsi->find_next( findcursor( pInfo ) );
		else
		{
#ifdef _WIN32
			r = _wfindnext( findhandle(pInfo), finddata( pInfo ) );
#else
			de = readdir( (DIR*)findhandle( pInfo ) );
			//lprintf( "using %p got %p", findhandle( pInfo ), de );
			r = (de == NULL);
#endif
		}
		if( r )
		{
			PMFD prior = pData->prior;
			//lprintf( "nothing left to find..." );
			if( pData->scanning_mount && pData->scanning_mount->fsi )
				pData->scanning_mount->fsi->find_close( findcursor(pInfo) );
			else
			{
#ifdef WIN32
				findclose( findhandle(pInfo) );
#else
				closedir( (DIR*)findhandle(pInfo));
#endif
			}
			pData->scanning_mount =  pData->scanning_mount ->nextLayer;
			//lprintf( "Step mount... %p %d", pData->scanning_mount, pData->single_mount );
			if( !pData->scanning_mount || pData->single_mount )
			{
				//lprintf( "done with mounts?" );
				(*pData->root_info) = pData->prior;
				Release( pData );
				if( prior )
					prior->current = NULL;
				if( !processed && !begin_sub_path )
				{
					//pInfo = (void**)&(prior->prior->current);
					pData = prior;
					if( pData )
						goto getnext;
				}
				if( tmp_base )
					Release( tmp_base );
				return (*pInfo)?processed:0;
			}
			pData->new_mount = TRUE;
			if( tmp_base )
				Release( tmp_base );
			return 1;
		}
	}
	if( pData->scanning_mount?pData->scanning_mount->fsi:NULL )
	{
		char * path = pData->scanning_mount->fsi->find_get_name( findcursor(pInfo) );
		//lprintf( "... %s", path );
		if( !strcmp( ".", path ) ||
		    !strcmp( "..", path ) )
		goto getnext;
	}
	else
	{
#ifdef WIN32
		{
			const wchar_t* tmp = finddata( pInfo )->name;
			char* out = findbuffer( pInfo );
			while( tmp[0] ) {
				out += ConvertToUTF8( out, GetUtfCharW( &tmp ) );
			}
			out[0] = 0;
		}
		//lprintf( "... %s", finddata(pInfo)->name );
		if( !StrCmp( ".", findbuffer(pInfo) ) ||
		    !StrCmp( "..", findbuffer(pInfo) ) )
#else
		if( !StrCmp( ".", de->d_name ) ||
		    !StrCmp( "..", de->d_name ) )
#endif
			goto getnext;
	}
	if( !(flags & SFF_NAMEONLY) ) // if nameonly - have to rebuild the correct name.
	{
		if( pData->scanning_mount?pData->scanning_mount->fsi:NULL )
		{
			tnprintf( pData->file_buffer, MAX_PATH_NAME, "%s", pData->scanning_mount->fsi->find_get_name( findcursor(pInfo) ) );
			if( findbasename( pInfo )[0] )
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s" PATHCHAR "%s", findbasename(pInfo), pData->file_buffer );
			else
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s", pData->file_buffer );
		}
		else
		{
#ifdef WIN32
#  ifdef UNDER_CE
			tnprintf( pData->file_buffer, MAX_PATH_NAME, "%s", finddata( pInfo )->cFileName );
			tnprintf( pData->buffer, MAX_PATH_NAME, "%s" PATHCHAR "%s", findbasename(pInfo), finddata(pInfo)->cFileName );
#  else
			tnprintf( pData->file_buffer, MAX_PATH_NAME, "%ls", finddata(pInfo)->name );
			tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%s", findbasename(pInfo), findbasename( pInfo )[0]?PATHCHAR:"", pData->file_buffer );
#  endif
#else
			tnprintf( pData->file_buffer, MAX_PATH_NAME, "%s", de->d_name );
			tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%s", findbasename(pInfo), findbasename( pInfo )[0]?PATHCHAR:"", de->d_name );
#endif
		}
	}
	else
	{
		if( flags & SFF_SUBCURSE )
		{
			if( pData->scanning_mount?pData->scanning_mount->fsi:NULL )
			{
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%s"
					  , pData->prior?pData->prior->buffer:""
					  , pData->prior?PATHCHAR:""
					, pData->scanning_mount->fsi->find_get_name( findcursor(pInfo) )
					);
			}
			else
			{
#ifdef WIN32
#  ifdef UNDER_CE
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%s"
						  , pData->prior?pData->prior->buffer:""
						  , pData->prior?PATHCHAR:""
						  , finddata(pInfo)->cFileName );
#  else
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%ls"
						  , pData->prior?pData->prior->buffer:""
						  , pData->prior?PATHCHAR:""
						  , finddata(pInfo)->name );
#  endif
#else
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s%s%s"
					  , pData->prior?pData->prior->buffer:""
					  , pData->prior?PATHCHAR:""
					  , de->d_name );
					  lprintf( "resulting is %s", pData->buffer );
#endif
			}
		}
		else
		{
			if( pData->scanning_mount?pData->scanning_mount->fsi:NULL )
			{
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s", pData->scanning_mount->fsi->find_get_name( findcursor(pInfo) ) );
			}
			else
			{
#ifdef WIN32
#  ifdef UNDER_CE
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s", finddata(pInfo)->cFileName );
#  else
				tnprintf( pData->buffer, MAX_PATH_NAME, "%ls", finddata(pInfo)->name );
#  endif
#else
				tnprintf( pData->buffer, MAX_PATH_NAME, "%s", de->d_name );
#endif
			}
		}
	}
	pData->buffer[MAX_PATH_NAME-1] = 0; // force nul termination...
#ifdef UNICODE
	{
		char *pDataBuffer = CStrDup( pData->buffer );
#else
#  define pDataBuffer pData->buffer
#endif
	//lprintf( "Check if %s is a directory...", pData->buffer );
	if( ((flags & (SFF_DIRECTORIES | SFF_SUBCURSE))
		&& (pData->scanning_mount && pData->scanning_mount->fsi
			&& (pData->scanning_mount->fsi->is_directory
				&& pData->scanning_mount->fsi->is_directory( pData->scanning_mount->psvInstance, pDataBuffer ))))
		|| (!(pData->scanning_mount ? pData->scanning_mount->fsi : NULL)
#ifdef WIN32
#  ifdef UNDER_CE
			&& (finddata( pInfo )->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#  else
			&& (finddata( pInfo )->attrib & _A_SUBDIR)
#  endif
#else
			&& IsPath( pData->buffer )
#endif
			) )
	{
#ifdef UNICODE
		Deallocate( char *, pDataBuffer );
#else
#  undef pDataBuffer
#endif
		//lprintf( "... it is?" );
		if( flags & SFF_DIRECTORIES )
		{
			if( Process != NULL )
			{
				//lprintf( "Send %s", pData->buffer );
				Process( psvUser, pData->buffer, SFF_DIRECTORY );
				processed = 1;
			}
			//return 1;
		}
		if( flags & SFF_SUBCURSE )
		{
			//int ofs = 0;
			TEXTCHAR tmpbuf[MAX_PATH_NAME + 512];
			if( flags & SFF_NAMEONLY )
			{
				// even in name only - need to have this full buffer for subcurse.
				if( pData->scanning_mount && pData->scanning_mount->fsi )
				{
					/*ofs = */tnprintf( tmpbuf, sizeof( tmpbuf ), "%s" PATHCHAR "%s", findbasename( pInfo ), pData->scanning_mount->fsi->find_get_name( findcursor( pInfo ) ) );
				}
				else
				{
#ifdef WIN32
#  ifdef UNDER_CE
					/*ofs = */tnprintf( tmpbuf, sizeof( tmpbuf ), "%s" PATHCHAR "%s", findbasename( pInfo ), finddata( pInfo )->cFileName );
#  else
					/*ofs = */tnprintf( tmpbuf, sizeof( tmpbuf ), "%s" PATHCHAR "%ls", findbasename( pInfo ), finddata( pInfo )->name );
#  endif
#else
					/*ofs = */tnprintf( tmpbuf, sizeof( tmpbuf ), "%s" PATHCHAR "%s", findbasename( pInfo ), de->d_name );
#endif
				}
				//lprintf( "process sub... %s %s", tmpbuf, findmask(pInfo)  );
				processed |= ScanFilesEx( tmpbuf, findmask( pInfo ), (POINTER*)pData, Process, flags, psvUser, TRUE, pData->scanning_mount );
			}
			else
			{
				//lprintf( "process sub..." );
				processed |= ScanFilesEx( pData->buffer, findmask( pInfo ), (POINTER*)pData, Process, flags, psvUser, TRUE, pData->scanning_mount );
			}
		}
		if( !processed )
			goto getnext;
		if( tmp_base )
			Release( tmp_base );
		return (*pInfo) ? 1 : 0;
	}
#ifdef UNICODE
	Deallocate( char *, pDataBuffer );
	}
#else
#  undef pDataBuffer
#endif
	if( ( sendflags = SFF_DIRECTORY, ( ( flags & SFF_DIRECTORIES )
#ifdef WIN32
#  ifdef UNDER_CE
												 && ( finddata(pInfo)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
#  else
												 && ( finddata(pInfo)->attrib & _A_SUBDIR )
#  endif
#else
												 && ( IsPath( pData->buffer ) )

#endif
												) ) || ( (sendflags = (enum ScanFileProcessFlags)0), CompareMask( findmask( pInfo )
#ifdef WIN32
#  ifdef UNDER_CE
																							  , finddata(pInfo)->cFileName
#  else
																							  , pData->file_buffer
#  endif
#else
																							  , pData->scanning_mount?pData->scanning_mount->fsi->find_get_name( findcursor( pInfo ) ) : de->d_name
#endif
																								// yes this is silly - but it's correct...
																							  , (flags & SFF_IGNORECASE)?0:0 ) ) )
	{
		//lprintf( "Send %s", pData->buffer );
		if( Process != NULL )
			Process( psvUser, pData->buffer, sendflags );
		if( tmp_base )
			Release( tmp_base );
		return (*pInfo)?1:0;
	}
	if( tmp_base )
		Release( tmp_base );
	return (*pInfo)?1:0;
}


int  ScanFiles ( CTEXTSTR base
                , CTEXTSTR mask
                , void **pInfo
                , void CPROC Process( uintptr_t psvUser, CTEXTSTR name, enum ScanFileProcessFlags flags )
                , enum ScanFileFlags flags
                , uintptr_t psvUser )
 {
	 return ScanFilesEx( base, mask, pInfo, Process, flags, psvUser, FALSE, NULL );
 }


//---------------------------------------------------------------------------

 void  ScanDrives ( void (CPROC*Process)(uintptr_t user, CTEXTSTR letter, int flags)
									, uintptr_t user )
{
#ifdef WIN32
#  ifdef UNDER_CE
	Process( user, "", SFF_DRIVE );
#  else
	uint32_t drives;
	int i;
	drives = GetLogicalDrives();
	for( i = 0; i < 26; i++ )
	{
		TEXTCHAR name[2];
		name[1] = 0;
		if( drives & ( 1 << i ) )
		{
			name[0] = 'A' + i;
			if( Process )
				Process( user, name, SFF_DRIVE );
		}
	}
#  endif
#endif

}


#undef findhandle
#undef finddata
#undef findbuffer
#undef findbasename
#undef findmask
#undef findmaskw
#undef findinfo
#undef findcursor


FILESYS_NAMESPACE_END
