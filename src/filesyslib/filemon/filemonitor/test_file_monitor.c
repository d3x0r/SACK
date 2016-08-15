
#include <stdhdrs.h>
#include <filemon.h>


static int CPROC ShowChange( uintptr_t psv, CTEXTSTR filepath, int deleted )
{
	printf( WIDE("%9ld %s changed%s\n"), timeGetTime(), filepath, deleted?WIDE("(deleted)"):WIDE("") );
	lprintf( WIDE("%s changed%s\n"), filepath, deleted?WIDE("(deleted)"):WIDE("") );
   return 1;
}


int main( int argc, char **argv )
{
	CTEXTSTR root = WIDE(".");
   POINTER data = NULL;
	if( argc >= 2 )
		root = DupCharToText( argv[1] );

	// the real part of the test (mode 1)
   // this monitors every subdirectory for changes from a point
	{
		PMONITOR monitor = MonitorFilesEx( root, 500, SFF_NAMEONLY|SFF_SUBCURSE );
		AddFileChangeCallback( monitor, WIDE("*"), ShowChange, 0 );
	}

   // stupid wait loop.  hit (EOF) to end program
	{
      TEXTCHAR buf[10];
		while( fgets( buf, 1, stdin ) );
	}
   return 1;
}
