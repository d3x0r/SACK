#include <stdhdrs.h>
#include <logging.h>
#include <sharemem.h>
#include <deadstart.h>
#include "run.h"

#ifndef LOAD_LIBNAME
#ifdef MILK_PROGRAM
#define MODE 0
#define LOAD_LIBNAME "milk.core"
#endif
#ifdef INTERSHELL_PROGRAM
#define MODE 0
#define LOAD_LIBNAME "InterShell.core.dll"
#endif
#endif


#if (MODE==0)
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev
						  , LPTSTR lpCmdLine
						  , int nCmdShow )
//int main( int argc, char **argv )
{

	TEXTCHAR **argv;
	int argc;
	ParseIntoArgs( GetCommandLine(), &argc, &argv );

#else
int main( int argc, char **argv )
{
#endif
	{
	int arg_offset = 1;
	generic_function hModule = NULL;
	MainFunction Main;
	BeginFunction Begin;
	StartFunction Start;
	CTEXTSTR libname;
#ifndef LOAD_LIBNAME
	if( argc > 1 )
	{
		hModule = LoadFunction( libname = argv[1], NULL );
		if( hModule )
         arg_offset++;
	}
#endif

	if( !hModule )
	{
#ifdef LOAD_LIBNAME
		hModule = LoadFunction( libname = _WIDE(LOAD_LIBNAME), NULL );
		if( !hModule )
		{
#ifndef UNDER_CE
			lprintf( "error: (%ld)%s"
					 , GetLastError()
					 , strerror(GetLastError()) );
#endif
			return 0;
		}
		else
			arg_offset = 0;
#else
		lprintf( "strerror(This is NOT right... what's the GetStrError?): (%ld)%s"
				 , GetLastError()
				 , strerror(GetLastError()) );
		return 0;
#endif
	}
	{
		// look through command line, and while -L options exist, use thsoe to load more libraries
      // then pass the final remainer to the proc (if used)
	}
	{

		Main = (MainFunction)LoadFunction( libname, WIDE( "_Main" ) );
		if( !Main )
			Main = (MainFunction)LoadFunction( libname, WIDE( "Main" ) );
		if( !Main )
			Main = (MainFunction)LoadFunction( libname, WIDE( "Main_" ) );
		if( Main )
			Main( argc-arg_offset, argv+arg_offset, MODE );
		else
		{
			Begin = (BeginFunction)LoadFunction( libname, WIDE( "_Begin" ) );
			if( !Begin )
				Begin = (BeginFunction)LoadFunction( libname, WIDE( "Begin" ) );
			if( !Begin )
				Begin = (BeginFunction)LoadFunction( libname, WIDE( "Begin_" ) );
			if( Begin )
			{
				int xlen, ofs, arg;
				char *x;
				for( arg = arg_offset, xlen = 0; arg < argc; arg++, xlen += snprintf( NULL, 0, WIDE( "%s%s" ), arg?WIDE( " " ):WIDE( "" ), argv[arg] ) );
				x = (char*)malloc( ++xlen );
				for( arg = arg_offset, ofs = 0; arg < argc; arg++, ofs += snprintf( x + ofs, xlen - ofs, WIDE( "%s%s" ), arg?WIDE( " " ):WIDE( "" ), argv[arg] ) );
				Begin( x, MODE ); // pass console defined in Makefile
				free( x );
			}
			else
			{
				Start = (StartFunction)LoadFunction( libname, WIDE( "_Start" ) );
				if( !Start )
					Start = (StartFunction)LoadFunction( libname, WIDE( "Start" ) );
				if( !Start )
					Start = (StartFunction)LoadFunction( libname, WIDE( "Start_" ) );
				if( Start )
					Start( );
			}
		}
	}
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
