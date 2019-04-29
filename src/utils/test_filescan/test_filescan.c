#include <stdhdrs.h>
#include <filesys.h>


static void CPROC DoFile( uintptr_t psv, CTEXTSTR name, int flags )
{
	printf( "%s%s:%s\n", (flags&SFF_DRIVE)?"[drive]":"", (flags&SFF_DIRECTORY)?"path":"file", name );
	fflush( stdout );
}

int main( int argc, char ** argv )
{
	POINTER info = NULL;

#if 0
   printf( "1\n" );
	while( ScanFiles( ".", "*", &info, DoFile, 0, 0 ) )
      printf( "... application between files...\n" );

   printf( "5\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_NAMEONLY, 0 ) )
      printf( "... application between files...\n" );

#endif

   printf( "2\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_SUBCURSE, 0 ) )
		printf( "... application between files...\n" );

   printf( "3\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_DIRECTORIES, 0 ) )
		printf( "... application between files...\n" );

   printf( "4\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_SUBCURSE|SFF_DIRECTORIES, 0 ) )
		printf( "... application between files...\n" );

   printf( "6\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_NAMEONLY|SFF_SUBCURSE, 0 ) )
		printf( "... application between files...\n" );

   printf( "7\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_NAMEONLY|SFF_DIRECTORIES, 0 ) )
		printf( "... application between files...\n" );

   printf( "8\n" );
	while( ScanFiles( ".", "*", &info, DoFile, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, 0 ) )
		printf( "... application between files...\n" );
	return 0;
}
