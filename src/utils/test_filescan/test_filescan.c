#include <stdhdrs.h>
#include <filesys.h>


static void CPROC DoFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	printf( WIDE("%s%s:%s\n"), (flags&SFF_DRIVE)?WIDE("[drive]"):WIDE(""), (flags&SFF_DIRECTORY)?WIDE("path"):WIDE("file"), name );
	fflush( stdout );
}

int main( int argc, char ** argv )
{
	POINTER info = NULL;

#if 0
   printf( WIDE("1\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, 0, 0 ) )
      printf( WIDE("... application between files...\n") );

   printf( WIDE("5\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_NAMEONLY, 0 ) )
      printf( WIDE("... application between files...\n") );

#endif

   printf( WIDE("2\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_SUBCURSE, 0 ) )
		printf( WIDE("... application between files...\n") );

   printf( WIDE("3\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_DIRECTORIES, 0 ) )
		printf( WIDE("... application between files...\n") );

   printf( WIDE("4\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_SUBCURSE|SFF_DIRECTORIES, 0 ) )
		printf( WIDE("... application between files...\n") );

   printf( WIDE("6\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_NAMEONLY|SFF_SUBCURSE, 0 ) )
		printf( WIDE("... application between files...\n") );

   printf( WIDE("7\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_NAMEONLY|SFF_DIRECTORIES, 0 ) )
		printf( WIDE("... application between files...\n") );

   printf( WIDE("8\n") );
	while( ScanFiles( WIDE("."), WIDE("*"), &info, DoFile, SFF_NAMEONLY|SFF_SUBCURSE|SFF_DIRECTORIES, 0 ) )
		printf( WIDE("... application between files...\n") );
	return 0;
}
