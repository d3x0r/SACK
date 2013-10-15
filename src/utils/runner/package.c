#include <stdhdrs.h>
#include <sharemem.h>
#include "run.h"

void CreateProgram( char *outname, PLIST parts )
{
	FILE *output = fopen( outname, "wb" );
	if( output )
	{
		//POINTER input;
		INDEX idx;
		struct section *section;
		LIST_FORALL( parts, idx, struct section *, section )
		{
		}
      fclose( output );
	}
}

void ProcessLine( PLIST *sections, char buf[256] )
{

}

int main( int argc, char **argv )
{
   char *script;
	int arg;
   PLIST sections = NULL;
	for( arg = 1; arg < argc; arg++ )
	{
      script = argv[arg];
		{
			struct section *section = (struct section *)Allocate( sizeof( *section ) );
			FILE *file;
			file = fopen( "runner.exe", "rb" );
			fseek( file, 0, SEEK_END );
			section->library_length = ftell( file );
			section->length = sizeof( "runner.exe" );
			section->flags = SECTION_FLAG_ORIGIN;
         AddLink( &sections, section );
		}
		{
			FILE *in = fopen( script, "rb" );
			if( in )
			{
            char buf[256];
				while( fgets( buf, sizeof( buf ), in ) )
				{
               ProcessLine( &sections, buf );
				}
			}
		}
	}
   return 0;
}
