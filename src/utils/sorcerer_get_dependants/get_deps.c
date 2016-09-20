#include <stdhdrs.h>

	PTASK_INFO cat;
	PTASK_INFO awk;

void HandleOutput( uintptr_t psv, PTASK_INFO task, CTEXTSTR line, uint32_t linelen )
{
	if( awk && task == cat )
	{
		fprintf( stderr, "sending %s to awk\n", line );
		pprintf( awk, "%s", line );	
	}
	// task sent a line of output...
	fprintf( stderr, "test: %s\n", line );
}

void HandleTaskEnd( uintptr_t psv, PTASK_INFO task )
{
	fprintf( stderr, "uhmm" );
}

void GetDeps( char *pkg )
{

	PCTEXTSTR args = NULL;
	PVARTEXT pvt = VarTextCreate();
	vtprintf( pvt, 
		"cat /etc/sorcery/log/install/zlib"
		);
	ParseIntoArgs( 
		 GetText( VarTextPeek( pvt ) )
		, NULL
		, &args );
	cat = LaunchPeerProgram( "/bin/cat", NULL, args, HandleOutput, 
HandleTaskEnd, 0 );

	return;
	vtprintf( pvt, 
		"\"$1~/\/etc\/sorcery\/ {print $1}\"" );
	ParseIntoArgs( 
		 GetText( VarTextPeek( pvt ) )
		, NULL
		, &args );
	awk = LaunchPeerProgram( "/usr/bin/awk", NULL, args, 
HandleOutput, 
HandleTaskEnd, 0 );
	
/*
	FILE *in;
	in = fopen( "pkg", "rt" );
	if( !in )
	{
		char tmp[280];
		snprintf( tmp, "/etc/sorcery/log/install/%s", pkg );
		in = fopen( tmp, "rt" );
		if( !in )
		{
			fprintf( stderr, "Failed to open package %s\n", pkg );
			return;
		}
	}

	{
		static char buf[4096];
		while( fgets( buf, sizeof( buf ), in ) )
		{
			
		}
	}

	fclose( in );
*/
}


int main( int argc, char **argv )
{
	int arg;
	if( argc < 2 )
	{
		fprintf( stderr, "usage: %s <packages...>\n", argv[0] );
		exit(0);
	}

	for( arg = 1; arg < argc; arg++ )
	{
		GetDeps( argv[arg] );
		fprintf( stderr, "waiting..." );
		while( 1 ) WakeableSleep( 5000 );
		WakeableSleep( 5000 );
		WakeableSleep( 5000 );
		fprintf( stderr, "uhm..." );
	}

	return 0;
}
