#include <windows.h>
#include <stdio.h>
#include <filesys.h>

int parents;
int failed;

void MakeDirectory( char *name )
{
	char *path;
	printf( WIDE("Checking %s\n"), name );
	path = pathrchr( name );
	if( path )
	{
		path[0] = 0;
		MakeDirectory( name );
		path[0] = '/';
	}
	if( !CreateDirectory( name, NULL ) )
	{
		if( GetLastError() != ERROR_ALREADY_EXISTS ) // 183
		{
			fprintf( stderr, WIDE("Create Directory failed on :%s %d\n"), name, GetLastError() );
			failed = 1;
		}
	}
}


int main( int argc, char **argv )
{
	int n;
	for( n = 1; n < argc; n++ )
	{
		if( argv[n][0] == '-' )
		{
			int c;
			for( c = 1; argv[n][c]; c++ )
			{
				switch( argv[n][c] )
				{
				case 'p':
					parents = 1;
				}
			}
		}
		else
		{
			char tmp[1024];
			strcpy( tmp, argv[n] );
			if( !parents )
			{
				if( !CreateDirectory( argv[n], NULL ) )
					if( GetLastError() != ERROR_ALREADY_EXISTS ) // 183
					{
						fprintf( stderr, WIDE("Create Directory failed on :%s %d\n"), tmp, GetLastError() );
						failed = 1;
					}
			}
			else
			{
				MakeDirectory( tmp );	
			}
		}
	}
	return failed;
}