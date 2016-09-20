#include <windows.h>
#include <stdio.h>
#include <sack_types.h>
#include <filesys.h>
#include <sharemem.h>

typedef struct global_tag
{
	struct {
		uint32_t force : 1;
		uint32_t recurse : 1;
		uint32_t preserve : 1; // preserve rights - no meaning.
		uint32_t interactive : 1;
		uint32_t directories : 1;
		//uint32_t fast : 1;
	} flags;
	int nSrc;
	PLIST src;
	char original_destbuffer[256];
	char destbuffer[256];
	char *dest;
} GLOBAL;

GLOBAL g;

int MyCopyFile( char *src, char *dest, int unused )
{
	uint32_t src_size = 0;
	POINTER pSrc;
	POINTER pDest;
	pSrc  = OpenSpace( NULL, src, &src_size );
	if( !pSrc )
	 	fprintf( stderr, WIDE("Failed to  map %s\n"), src );
	pDest = OpenSpace( NULL, dest, &src_size );
	if( !pDest )
	 	fprintf( stderr, WIDE("Failed to  map %s(%d)\n"), dest, src_size );
	if( pSrc && pDest && src_size )
		MemCpy( pDest, pSrc, src_size );
	if( pSrc )
		CloseSpace( pSrc );
	if( pDest )
		CloseSpace( pDest );
	return !( (pSrc&&pDest) ||  
	          (!pSrc && !pDest) );
}

int MyCopyFile2( char *src, char *dest, int unused )
{
	FILE *in, *out;
	in = fopen( src, WIDE("rb") );
	out = fopen( dest, WIDE("wb") );
	if( !in )
		printf( WIDE("Failed in") );
	if( !out )
		printf( WIDE("Failed out") );
   return 1;
}

void CPROC DoCopy( uintptr_t skip, char *src, int flags )
{
	char tmp[256];
	sprintf( tmp, WIDE("%s/%s"), g.dest, src + skip );
	//printf( WIDE("Copy %s to %s"), src, tmp );
	if( flags & SFF_DIRECTORY )
	{
		char filepath1[256];
		char *file1;
		char filepath2[256];
		char *file2;
		GetFullPathName( src, 256, filepath1, &file1 );
		GetFullPathName( g.original_destbuffer, 256, filepath2, &file2 );
		Log1( WIDE("Filepath1: %s"), filepath1);
		Log1( WIDE("Filepath2: %s"), filepath2);
		if( !strcmp( filepath1, filepath2 ) )
		{
			fprintf( stderr, WIDE("Omitting destination %s from copy."), g.dest );
			return;
		}
	}
	if( flags & SFF_DIRECTORY )
	{
		//printf( WIDE("(path)\n") );
		MakePath( tmp );
	}
	else
	{
		//printf( WIDE("(file)\n") );
		//if( g.flags.fast )
		//	MyCopyFile( src, tmp, FALSE );
		//else
		// my copyfile doesn't copy the attributes/time..... so we go a little slower...
      //fprintf( stderr, WIDE("COpy %s to %s\n"), src, tmp );
		if( !MyCopyFile2( src, tmp, FALSE ) )
         fprintf( stderr, WIDE("copy %s to %s failed\n"), src, tmp );
	}
}

int HasWild( char *name )
{
	if( strchr( name, '?' )
	  || strchr( name, '*' ) )
		return 1;
   return 0;
}

int main( int argc, char **argv )
{
	int arg;
	//uint32_t start = GetTickCount();
	//SetSystemLog( SYSLOG_FILENAME, WIDE("cp.log") );
	for( arg = 1; arg < argc; arg++ )
	{
		if( argv[arg][0] == '-' )
		{
			int c;
			for( c = 1; c && argv[arg][c]; c++ )
			{
				switch( argv[arg][c] )
				{
				case 'f':
					g.flags.force = 1;
					break;
				case 'r':
					g.flags.recurse = 1;
					break;
				case 'R':
					g.flags.recurse = 1;
					g.flags.directories = 1;
					break;
				case 'i':
					g.flags.interactive = 1;
					break;
			   //case 'a':
			  // 	g.flags.fast = 1;
			   //	break;
				case '-':
					c = -1; // increments and becomes 0 which breaks the for.
					break;	
				}
			}
		}
		else
		{
			g.dest = argv[arg];
         g.nSrc++;
			AddLink( &g.src, argv[arg] );
		}
	}
	// move this to a buffer that is known to be able to be updated...
	strcpy( g.original_destbuffer, g.dest );
	strcpy( g.destbuffer, g.dest );
	DeleteLink( &g.src, g.dest );
	g.dest = g.destbuffer;
	g.nSrc--;
	{
		int len = strlen( g.dest );
		if( len && 
		    ( g.dest[len-1] == '\\' 
		    || g.dest[len-1] == '/' ) )
			g.dest[len-1] = 0;
	}
	if( HasWild( g.dest ) )
	{
		fprintf( stderr, WIDE("Wildcard in destination is not supported.\n") );
		return 1;
	}
	if( IsPath( g.dest ) )
	{
		INDEX idx;
		char *src;
		POINTER info = NULL;
		LIST_FORALL( g.src, idx, char*, src )
		{
			if( HasWild( src ) )
			{
				char *mask = (char*)pathrchr( src );
				if( mask )
				{
					mask[0] = 0;
					mask++;
					while( ScanFiles( src, mask, &info, DoCopy
										 , SFF_DIRECTORIES 
										 //| SFF_NAMEONLY
										 | (g.flags.recurse?SFF_SUBCURSE:0)
										 , strlen( src ) +1) );
				}
				else
					while( ScanFiles( WIDE("."), src, &info, DoCopy
										 , SFF_DIRECTORIES 
										 //| SFF_NAMEONLY
										 | (g.flags.recurse?SFF_SUBCURSE:0)
										 , 2 ) );
			}
			else
			{
				// direct copy - this is when 'directories' flag applies.
				if( IsPath( src ) )
				{
					char tmp[256];
					char *mask = (char*)pathrchr( src );
					if( mask )
					{
						mask++;
						sprintf( tmp, WIDE("%s/%s"), g.dest, mask );
					}
					else
					{
						sprintf( tmp, WIDE("%s/%s"), g.dest, src );
					}
					if( g.flags.directories )
					{
						MakePath( tmp );
						strcpy( g.dest, tmp );
						while( ScanFiles( src, WIDE("*"), &info, DoCopy 
						                , SFF_DIRECTORIES 
											 | SFF_SUBCURSE
											 , strlen( src ) +1) );
						strcpy( g.destbuffer, g.original_destbuffer );
					}
					else
					{
						if( g.flags.recurse )
							while( ScanFiles( src, WIDE("*"), &info, DoCopy 
							                , SFF_DIRECTORIES 
												 | SFF_SUBCURSE
												 , strlen( src ) +1) );
					}	
				} 
			}
		}
		//printf( WIDE("Took %dms\n"), GetTickCount() - start );
	}
	else
	{
		INDEX idx;
		char *src;
		src = GetLink( &g.src, 0 );
		if( ( g.nSrc > 1 )
		   || HasWild( src ) )
		{
			fprintf( stderr
			       , WIDE("%s is not a directory - and multiple source files listed.\n")
			       , g.dest );
			return 1;
		}
		
		return !CopyFile( src, g.dest, FALSE );
	}
	return 0;
}
