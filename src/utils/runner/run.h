#include <sack_types.h>

#define SECTION_FLAG_ORIGIN  0x00001
#define SECTION_FLAG_STARTUP 0x00002

		struct section {
			_32 library_length; // length of the library data
			_32 length; // length of name data
			_32 flags; // flags
			//char name[];
		};

	struct section_block {
		_32 section_length; // sizeof this section - minus this member...
      struct section info;
	}; // followed by name, and then data.



typedef void (CPROC *StartFunction )( void );
typedef void (CPROC *BeginFunction )( TEXTCHAR *lpCmdLine, int bConsole );
typedef void (CPROC *MainFunction )( int argc, TEXTCHAR **argv, int bConsole );
// $Log: run.h,v $
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
