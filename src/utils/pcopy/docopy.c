#define NO_UNICODE_C

#include <stdhdrs.h>
#include <stdio.h>
#include <sack_types.h>
#include <filesys.h>
#include <sharemem.h>

#include "pcopy.h"

extern GLOBAL g;
static PFILESOURCE copytree;

PFILESOURCE CreateFileSource( CTEXTSTR name )
{
	PFILESOURCE pfs = (PFILESOURCE)Allocate( sizeof( FILESOURCE ) );
	MemSet( pfs, 0, sizeof( FILESOURCE ) );
	pfs->name = StrDup( name );
	return pfs;
}

PFILESOURCE FindFileSource( PFILESOURCE pfs, CTEXTSTR name )
{
	PFILESOURCE pFound = NULL;
	while( pfs )
	{
		if( StrCaseCmp( name, pfs->name ) == 0 )
			pFound = pfs;
		if( !pFound ) pFound = FindFileSource( pfs->children, name );
		if( pFound ) break;
		pfs = pfs->next;
	}
	return pFound;
}

PFILESOURCE AddDependCopy( PFILESOURCE pfs, CTEXTSTR name )
{
	PFILESOURCE pfsNew;
	if( g.flags.bVerbose )
	{
		printf( "Adding %s as dependant of %s\n", name, pfs->name );
	}
	if( !(pfsNew = FindFileSource( copytree, name ) ) )
	{
		pfsNew = CreateFileSource( name );
		pfsNew->next = pfs->children;
		pfs->children = pfsNew;
	}
	return pfsNew;
}

void CPROC DoScanFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	PFILESOURCE pfs = CreateFileSource( name );
	pfs->next = copytree;
	copytree = pfs;
	ScanFile( copytree );
}

void AddFileCopy( CTEXTSTR name )
{
	void *p = NULL;
	if( pathrchr( name ) )
	{
		TEXTSTR tmp = StrDup( name );
		TEXTSTR x = (TEXTSTR)pathrchr( tmp );
		//lprintf( "..." );
		x[0] = 0;
		//printf( "Old path: %s\n%s\n", OSALOT_GetEnvironmentVariable( "PATH" ), tmp );
		if( !StrStr( OSALOT_GetEnvironmentVariable( WIDE("PATH") ), tmp ) )
		{
			if( !( pathrchr( tmp ) == tmp || tmp[1] == ':' ) )
			{
				TEXTSTR path = (TEXTSTR)OSALOT_GetEnvironmentVariable( WIDE("MY_WORK_PATH") );
				size_t len;
#ifdef WIN32
#define PATHCHAR WIDE("\\")
#else
#define PATHCHAR WIDE("/")
#endif
				TEXTSTR real_tmp = NewArray( TEXTCHAR, len = StrLen(path) + StrLen( tmp ) + 2 );
#ifdef _MSC_VER
#define snwprintf _snwprintf
#endif
#ifdef _UNICODE
				snwprintf( real_tmp, len, WIDE("%s") PATHCHAR WIDE("%s"), path, tmp );
#else
				snprintf( real_tmp, len, WIDE("%s") PATHCHAR WIDE("%s"), path, tmp );
#endif
				Release( tmp );
				while( path = StrStr( real_tmp, WIDE("..") ) )
				{
					TEXTSTR prior;
					path[-1] = 0;
					prior = (TEXTSTR)pathrchr( real_tmp );
					if( !prior )
					{
						path[-1] = PATHCHAR[0];
						break;
					}
					StrCpyEx( prior, path+2, StrLen( path ) - 1 );
				}
				tmp = real_tmp;
				if( StrStr( OSALOT_GetEnvironmentVariable( WIDE("PATH") ), tmp ) )
				{
					goto skip_path;
				}
			}
#ifdef WIN32
			x[0] = ';';
#else
			x[0] = ':';
#endif
			x[1] = 0;
			OSALOT_PrependEnvironmentVariable( WIDE("PATH"), tmp );
			Release( tmp );
			//printf( "New path: %s\n", OSALOT_GetEnvironmentVariable( "PATH" ) );
			fflush( stdout );
		}
	}
skip_path:
   while( ScanFiles( NULL, name, &p, DoScanFile, 0, 0 ) );
   //return pfs;
}


void DoScanFileCopyTree( PFILESOURCE pfs )
{
	while( pfs )
	{
		if( !pfs->flags.bScanned && !pfs->flags.bInvalid )
		{
			ScanFile( pfs );
			if( pfs->flags.bSystem )
            ; //fprintf( stderr, WIDE("Skipped system library %s\n"), pfs->name );
			else if( pfs->flags.bExternal )
            ; //fprintf( stderr, WIDE("Scanned extern library %s\n"), pfs->name );

		}
		DoScanFileCopyTree( pfs->children );
		pfs = pfs->next;
	}
}

void ScanFileCopyTree( void )
{
	DoScanFileCopyTree( copytree );
}

void copy( TEXTCHAR *src, TEXTCHAR *dst )
{
	if( g.flags.bDoNotCopy )
	{
		fprintf( stdout, "%s\n", DupTextToChar( src ) );
	}
	else
	{
		static _8 buffer[4096];
		FILE *in, *out;
		_64 filetime;
		_64 filetime_dest;

		filetime = GetFileWriteTime( src );
		filetime_dest = GetFileWriteTime( dst );

		if( filetime <= filetime_dest )
			return;
		in = sack_fopen( 0, src, WIDE("rb") );
		if( in )
			out = sack_fopen( 0, dst, WIDE("wb") );
		else
			out = NULL;
		if( in && out )
		{
			size_t len;
			while( len = fread( buffer, 1, sizeof( buffer ), in ) )
				fwrite( buffer, 1, len, out );
		}
		if( in )
			fclose( in );
		if( out )
			fclose( out );
		SetFileWriteTime( dst, filetime );
	}
	g.copied++;
}

void DoCopyFileCopyTree( PFILESOURCE pfs, CTEXTSTR dest )
{
	TEXTCHAR fname[256];
	while( pfs )
	{
		const TEXTCHAR *name;
		name = pathrchr( pfs->name );
		if( !name )
			name = pfs->name;
		if( dest )
		{
#ifdef _UNICODE
			snwprintf( fname, sizeof( fname ), WIDE("%s/%s"), dest, name );
#else
			snprintf( fname, sizeof( fname ), WIDE("%s/%s"), dest, name );
#endif
		}
		//if( !IsFile( fname ) )
		{
         // only copy if the file is new...
			if( pfs->flags.bScanned && !pfs->flags.bSystem )
			{
				copy( pfs->name, dest?fname:NULL );
			}
		}
		DoCopyFileCopyTree( pfs->children, dest );
      pfs = pfs->next;
	}
}

void CopyFileCopyTree( CTEXTSTR dest )
{
   DoScanFileCopyTree( copytree );
   DoCopyFileCopyTree( copytree, dest );

}

