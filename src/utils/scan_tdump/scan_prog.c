
#include <sack_types.h>
#include <sack_system.h>
#include <filesys.h>
	

FILE *out;
int done;

void CPROC HandleTaskOutput( PTRSZVAL psv, PTASK_INFO pti, CTEXTSTR buffer, _32 size )
{
   lprintf( "Got %s", buffer );
   fprintf( out, "%s", buffer );
}

void CPROC HandleTaskDone( PTRSZVAL psv, PTASK_INFO pti )
{
   done = 1;
}

void CPROC ProcessAFile( CTEXTSTR progname, PTRSZVAL psv, CTEXTSTR name, int flags )
{
	char cmd[256];
	CTEXTSTR args[3];
	CTEXTSTR p = pathrchr( name );
	if( p )
		p++;
	else
		p = name;
	sprintf( cmd, WIDE("cmd /c%s"), progname );
	args[0] = cmd;
	args[1] = p;
	args[2] = NULL;
	done = 0;
   lprintf( "spawn!" );
	LaunchPeerProgram( cmd, NULL, args, HandleTaskOutput, HandleTaskDone, 0 );
	while( !done )
	{
		Relinquish();
	}
}

int main( int argc, char **argv )
{
	void *info = NULL;

	out = fopen( WIDE("xx"), WIDE("wb") );
   ProcessAFile( argv[1], 0, argv[2], 0 );
	//while( ScanFiles( WIDE("."), WIDE("*.dll\t*.exe"), &info, ProcessAFile, 0, 0 ) );
   fclose( out );
	//system( WIDE("scan_tdump <xx >yy") );
	//system( WIDE("edit yy") );
}


