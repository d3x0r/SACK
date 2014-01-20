
#include <stdhdrs.h>
#include <pssql.h>


#define DEFINE_GLOBAL
#include "global.h"

typedef struct project_root_tag PROJECT_ROOT, *PPROJECT_ROOT;
struct project_root_tag
{
	CTEXTSTR root_path;
	CTEXTSTR id;

	INDEX build_id;
	INDEX project_id;
};

struct local_data_tag
{
	PODBC odbc;
} l;


int main( int argc, char **argv )
{
	int n;
	g.dsn = "version.db";
	g.build_id = INVALID_INDEX;
	g.flags.use_common_build = 1;  // force this flag...
	for( n = 1; n < argc; n++ )
	{
		if( argv[n][0] == '-' )
		{
			switch( argv[n][1] )
			{
			case 'n':
			case 'N':
				g.flags.skip_push = 1;
				break;
			case 'b':
			case 'B':
				if( argv[n][2] )
					g.build_id = atoi( argv[n] + 2);
				else if( n < (argc-1) )
					g.build_id = atoi( argv[++n] );

            break;
			case 'u':
			case 'U':
				if( !l.odbc )
					l.odbc = ConnectToDatabase( g.dsn );
				IncrementBuild( l.odbc );
				break;
			case 'd':
			case 'D':
				if( argv[n][2] )
					g.dsn = argv[n] + 2;
				else if( n < (argc-1) )
					g.dsn = argv[++n];

				if( l.odbc )
					CloseDatabase( l.odbc );
				l.odbc = ConnectToDatabase( g.dsn );
				lprintf( "new db %p", l.odbc );
				break;

			case 'v':
			case 'V':
				if( !l.odbc )
					l.odbc = ConnectToDatabase( g.dsn );

				if( argv[n][2] )
					SetCurrentVersion( l.odbc, argv[n]+2 );
				else if( n < (argc-1) )
					SetCurrentVersion( l.odbc, argv[++n] );
				break;
			}
		}
		else
		{
			PPROJECT_ROOT project_root = New( PROJECT_ROOT );
			if( !l.odbc )
				l.odbc = ConnectToDatabase( g.dsn );

			project_root->root_path = argv[n];
			project_root->project_id = CheckProjectTable( l.odbc, argv[n] );
			SetCurrentDirectory( project_root->root_path );
			// GetBuildVersion also does the hg tag....
			project_root->build_id = GetBuildVersion( l.odbc, project_root->project_id );
			if( !g.flags.skip_push )
				System( "hg push", NULL, 0 );
		}
	}
	if( n == 1 )
	{
		printf( "Usage: %s [-b build_id] [-u] [-v version] [-d dsn] [-n] [project]\n", argv[0] );
		printf( "  option order matters; such that - option must preceed the project otherwise does not matter\n" );
		printf( " -b sets the current build number\n" );
		printf( "  build_id is an integer; it specifies the least significant build digits\n" );
		printf( " -v sets the major version part\n" );
		printf( "  version is a string; it specifies the major part of the version\n" );
		printf( " -d specifies the database; if not specified will be version.db in the current working directory.\n" );
		printf( "  dsn is a string; it is the data source name (.db extension will trigger sqlite)\n" );
		printf( "  project is a string; it specifies the application name for the versioning\n" );
		printf( " -u increments the current build number.\n" );
		printf( " -n skips running 'hg push' after 'hg tag'\n" );
		printf( " using -v will set the current default for the next update\n" );
		printf( " using -b sets the current so the tool can be called without -u to rebuild.\n" );
	}


   return 0;
}
