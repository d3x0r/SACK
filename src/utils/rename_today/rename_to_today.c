#include <windows.h>
#include <stdio.h>
//#include <sack_types.h> // wIDE()


int main( int argc, char **argv )
{
	char newname[256];
	char *src;
	SYSTEMTIME st;
	GetLocalTime( &st );
	if( argc < 2 )
	{
		printf( "Usage: %s (prefix) (postfix) [filename]\n", argv[0] );
		printf( " prefix and postfix are optional... AND you cannot have\n" );
		printf( " a postfix without a prefix\n" );
		printf( " The filename to rename MUST be the last parameter\n" );
		return 0; 
	}

	if( argc < 3 )
	{
		src = argv[1];
	   sprintf( newname, "%d%02d%02d", st.wYear, st.wMonth, st.wDay );
	}
	else if( argc < 4 )
	{
		src = argv[2];
	   sprintf( newname, "%s%d%02d%02d", argv[1], st.wYear, st.wMonth, st.wDay );
	}
	else if( argc < 5 )
	{
		src = argv[3];
	   sprintf( newname, "%s%d%02d%02d%s", argv[1], st.wYear, st.wMonth, st.wDay, argv[2] );
	}
	rename( src, newname );

	return 0;
}

