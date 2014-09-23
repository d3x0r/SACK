
#include <stdio.h>
int main( int argc, char **argv )
{
	FILE *file =fopen( argv[1], "rt" );
	FILE *fileout = fopen( argv[2], "wt" );
	char buffer[4095];
	while( fgets( buffer, 4094, file ) )
	{
		char *blanks = buffer;
		while( blanks[0] == ' ' )
         blanks++;
		if( strlen( blanks ) > 1 )
			fputs( buffer, fileout );
	}
	fclose( file );
   fclose( fileout );
   return 0;

}