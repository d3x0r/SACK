
#include <sack_types.h>
#include <sack_system.h>
#include <filesys.h>
	

FILE *out;
int done;

void CPROC HandleTaskOutput( uintptr_t psv, PTASK_INFO pti, CTEXTSTR buffer, uint32_t size )
{
	fprintf( out, "%s", buffer );
   fflush( out );
}

void CPROC HandleTaskDone( uintptr_t psv, PTASK_INFO pti )
{
   done = 1;
}

void CPROC ProcessAFile( uintptr_t psv, CTEXTSTR name, int flags )
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


