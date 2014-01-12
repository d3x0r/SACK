#include <stdhdrs.h>
#include <filesys.h>

int main( int argc, char **argv )
{
	int length = 0;
	if( argc > 1 )
	{
		length = atoi( DupCharToText( argv[1] ) );
	}
	{
#ifdef __LINUX__
#define O_BINARY 0
#endif
		int tmp = open( WIDE("blah"), O_RDWR|O_BINARY );
		lseek( tmp, length, SEEK_SET );
		set_eof( tmp );
		close( tmp );
	}
	return 0;
}

