
#include <stdio.h>


int main( int argc, char **argv )
{
	FILE *file =fopen( argv[1], "rt" );
	FILE *fileout = fopen( argv[2], "wt" );
	char buffer[4095];
	int c;
	int slashes = 0;
	int block = 0;
   int end_slashes = 0;
	while( ( c = fgetc( file ) ) >= 0 )
	{
		if( block )
		{
			switch( end_slashes )
			{
			case 0:
				if( c == '*' )
					end_slashes++;
				break;
			case 1:
				if( c == '/' )
				{
					block = 0;
					slashes = 0;
					end_slashes = 0;
				}
				else if( c == '*' )
				{
				}
				else
				{
					end_slashes = 0;
				}
            break;
			}
		}
		else
		{
			switch( slashes )
			{
			case 0:
				if( c == '/' )
					slashes++;
				else
               fputc( c, fileout );
				break;
			case 1:
				if( c == '/' )
					slashes++;
				else if( c == '*' )
					block = 1;
				else
				{
               slashes = 0;
               fputc( '/', fileout );
					fputc( c, fileout );
				}
				break;
			case 2:
				if( c == '\n' )
				{
					slashes = 0;
					fputc( c, fileout );
				}
            break;
			}
		}
	}
	fclose( file );
   fclose( fileout );
   return 0;

}