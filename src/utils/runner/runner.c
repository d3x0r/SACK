#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "run.h"



	struct {
		char section1[32];
		uint32_t first_section;
      CTEXTSTR startup_library;
	} decode = { "32 byte unique string goes here"
				  , 0
	};

	void DecodeSelf( void )
	{
		CTEXTSTR progname = GetProgramName();
		FILE *self;

		if( decode.first_section )
		{
			self = fopen( progname, "rb" );
			if( self )
			{
            POINTER out;
				uint32_t section_length;
            struct section section;
				char *filename;
				fseek( self, decode.first_section, SEEK_SET );
				while( fread( &section_length, 1, sizeof( section_length ), self ) )
				{
					fread( &section, 1, section_length, self );
					filename = (char*)Allocate( section.length );
					fread( filename, 1, section.length, self );
					out = OpenSpace( filename, NULL, 0 );
					fread( out, 1, section.library_length, self );
					if( section.flags & SECTION_FLAG_STARTUP )
					{
                  decode.startup_library = filename;
                  filename = NULL;
					}
					Release( out );
					if( filename )
						Release( filename );
				}
            fclose( self );
			}
		}
	}


#if (MODE==0) && !defined( __LINUX__ )
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
#else
int main( void )
#endif
	{
#ifdef WIN32
		//HANDLE hModule;
#else
	//int hModule;
#endif
	StartFunction Begin;
	DecodeSelf();
	if( decode.startup_library )
	{
      Begin = (StartFunction)LoadFunction( decode.startup_library, "Start" );
		if( Begin )
			Begin( );
	}
	else
	{
      //LoadFunction( GetCommandLine(),
	}
	return 0;
}
// $Log: runwin.c,v $
// Revision 1.6  2003/08/01 00:58:34  panther
// Fix loader for other alternate entry proc
//
// Revision 1.5  2003/03/25 08:59:03  panther
// Added CVS logging
//
