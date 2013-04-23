
#include <sack_types.h>
#include <system.h>
#include <filesys.h>
	

FILE *out;
int done;

void CPROC HandleTaskOutput( PTRSZVAL psv, PTASK_INFO pti, CTEXTSTR buffer, _32 size )
{
	fprintf( out, "%s", buffer );
   fflush( out );
}

void CPROC HandleTaskDone( PTRSZVAL psv, PTASK_INFO pti )
{
   done = 1;
}

void CPROC ProcessAFile( PTRSZVAL psv, CTEXTSTR name, int flags )
{
	char cmd[256];
	CTEXTSTR args[3];
	CTEXTSTR p = pathrchr( name );
	if( p )
		p++;
	else
		p = name;
	args[0] = cmd;
	args[1] = p;
	args[2] = NULL;
	done = 0;
   printf( "Scanning %s\n", name );
	LaunchPeerProgram( WIDE("cmd /ctdump"), NULL, args, HandleTaskOutput, HandleTaskDone, 0 );
	while( !done )
	{
		Relinquish();
	}
}

int main( void )
{
	void *info = NULL;

   out = fopen( WIDE("xx"), WIDE("wb") );
	while( ScanFiles( WIDE("."), WIDE("*.dll\t*.exe"), &info, ProcessAFile, 0, 0 ) );
	fclose( out );
#ifdef __cplusplus
	::
#endif
	system( WIDE("scan_tdump <xx >yy") );
#ifdef __cplusplus
	::
#endif
	system( WIDE("edit yy") );
}


