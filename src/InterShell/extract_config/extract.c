#include <stdio.h>
#include <sqlgetoption.h>

int main( int argc, char **argv )
{
	POINTER buffer;
	_32 buflen;
   const char *filename;
	SACK_GetProfileBlob( WIDE("intershell/configuration"), filename = (argc<2?"issue_pos.config":argv[1] ), &buffer, &buflen );
	{
		FILE *out = fopen( filename, "wb" );
		fwrite( buffer, 1, buflen, out );
		fclose( out );
	}
   return 0;
}