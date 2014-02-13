#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifndef WIDE
#ifdef UNICODE
#error WIDE NOT DEFINED
#else
#define WIDE(s) s
#endif
#endif

// Okay this was originally intended to merely
// add tabs to commands written for target commands to perform...
// also otpionally execute that before returning...
// it must now also act like linux sh/bash and consume
// escape cahracters \( \) specifically.

char *strip( char *p )
{
	static char tmp[256];
	char *out = tmp;
	char escape = 0;
	printf( WIDE("Strip: %s\n"), p );
	while( p && p[0] )
	{
		if( escape )
		{
			switch( p[0] )
			{
			case '(':
			case ')':
				out[0] = p[0];
            break;
			default:
				out[0] = escape;
				out[1] = p[0];
				out+=2;
            break;
			}
			p++;
         continue;
		}
		if( p[0] == '\\' )
		{
			p++;
         escape = '\\';
         continue;
		}
		out[0] = p[0];
      p++;
	}
   if( p )
		out[0] = p[0];
   return tmp;
}

char *nextarg( char **arg )
{
   char *start = *arg;
	char *p = *arg;
	if( !p[0] )
      return NULL;
	while( p[0] && p[0] != '\t' && p[0] != ' ' ) p++;
   if( p[0] )
		*(p++) = 0;
	while( p[0] && ( p[0] == '\t' || p[0] == ' ' ) ) p++;
	start = *arg;
   *arg = p;
   //printf( WIDE("nextarg=%s\n"), start );
   return start;
}

int main( int argc, char **argv )
{
   int execute = 0, iscmd = 0;
	char cmd[8192];
   char *myargv;
#ifdef _WIN32
	char *cmdline = strdup( GetCommandLine() );
	//fprintf( stderr, WIDE("cmdline=%s\n"), cmdline );
	while( cmdline[0] && cmdline[0] != ' ' ) cmdline++;
   if( cmdline[0] )
		*(cmdline++) = 0;
	while( cmdline[0] && cmdline[0] == ' ' ) cmdline++;
	//fprintf( stderr, WIDE("cmdline=%s\n"), cmdline );
#else
   int n = 1;
#define nextarg(unused) ((n)<(argc)?(argv)[(n)++]:NULL)
#endif
	if( argc < 2 )
	{
		printf( WIDE("usage: echoto [-x] <output> <command line.....>\n") );
		printf( WIDE("usage:    x : execute the command line also...\n") );
		return 1;
	}

	while( (
#ifdef _WIN32
			  myargv = nextarg( &cmdline )
#else
			  myargv = (argv++)[1]
#endif
			 )
						&& myargv[0] == '-' )
	{
		int c = 1;
		while( myargv[c] ) switch( myargv[c] )
		{
		case 'c':
		case 'C':
			iscmd = 1;
         c++;
         break;
		case 'X':
		case 'x':
			execute = 1;
         iscmd = 1;
         c++;
			break;
		default:
			printf( WIDE("Unknown option: %c\n"), myargv[c] );
         c++;
         break;
		}
	}
	{
		int ofs = 0;
		FILE *out = fopen( myargv, WIDE("at+") );
		if( !out )
			out =fopen( myargv, WIDE("wt") );
		if( !out )
		{
			printf( WIDE("echoto: Failed to open %s for output\n"), myargv );
			return 1;
		}
		if( iscmd )
         fprintf( out, WIDE("\t") );
		while( myargv = nextarg(&cmdline) )
		{
			if( execute )
				ofs += sprintf( cmd + ofs, WIDE("%s "), strip(myargv) );
			//printf( WIDE("Word=%s\n"),myargv );
			fprintf( out, WIDE("%s "), myargv );
		}
      fprintf( out, WIDE("\n") );
		fclose( out );
		if( execute )
		{
			return system( cmd );
		}
	}
   return 0;
}


