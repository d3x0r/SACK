#include <stdhdrs.h>
#include <logging.h>
#include <filesys.h>
#include <sharemem.h>
#include <deadstart.h>
#ifdef BUILD_SERVICE
#include <service_hook.h>
#endif
#include "run.h"

#ifndef LOAD_LIBNAME

#  ifdef INTERSHELL_PROGRAM
#    define MODE 0
#    define LOAD_LIBNAME "InterShell.core"
#  endif
#endif

static MainFunction Main;
static BeginFunction Begin;
static TEXTCHAR *x;  // argument for Begin function

static StartFunction Start;
static TEXTCHAR **my_argv;
static int my_argc;
static int arg_offset = 1;

static CTEXTSTR libname;
static generic_function hModule = NULL;



#ifdef BUILD_SERVICE

PRIORITY_PRELOAD( ImpersonateStart, NAMESPACE_PRELOAD_PRIORITY + 1 )
{
//	ImpersonateInteractiveUser();
}

static void CPROC DoStart( void )
{
   lprintf( "Impersonate interactive user now?  Is it already too late?" );
	if( Main )
		Main( my_argc-arg_offset, my_argv+arg_offset, MODE );
	else if( Begin )
		Begin( x, MODE );
	else if( Start )
      Start();
}

#  ifdef BUILD_SERVICE_THREAD
static uintptr_t CPROC DoStart2( PTHREAD thread )
#  else
static void CPROC DoStart2( void )
#  endif
{
#ifdef WIN32
   //lprintf( "Begin impersonation..." );
	ImpersonateInteractiveUser();
#endif
	//lprintf( "Begin impersonation2..." );
#  ifndef LOAD_LIBNAME
	if( my_argc > 1 )
	{
		hModule = LoadFunction( libname = my_argv[1], NULL );
		if( hModule )
         arg_offset++;
	}
#  endif
	if( !hModule )
	{
#  ifdef LOAD_LIBNAME
      SetCurrentPath( GetProgramPath() );
		hModule = LoadFunction( libname = LOAD_LIBNAME, NULL );
		if( !hModule )
		{
#    ifndef UNDER_CE
			lprintf( "error: (%"_32fs ")%s"
					 , GetLastError()
					 , strerror(GetLastError()) );
#    endif
#    ifdef BUILD_SERVICE_THREAD
			return 0;
#    else
			return;
#    endif
		}
		else
			arg_offset = 0;
#  else
		lprintf( "strerror(This is NOT right... what's the GetStrError?): (%ld)%s"
				 , GetLastError()
				 , strerror(GetLastError()) );
#    ifdef BUILD_SERVICE_THREAD
		return 0;
#    else
		return;
#    endif
#  endif
	}
	{

		Main = (MainFunction)LoadFunction( libname, "_Main" );
		if( !Main )
			Main = (MainFunction)LoadFunction( libname, "Main" );
		if( !Main )
			Main = (MainFunction)LoadFunction( libname, "Main_" );
		if( Main )
		{
			// main should be considered windowd.... (no console for output either)
			Main( my_argc-arg_offset, my_argv+arg_offset, FALSE );
		}
		else
		{
			Begin = (BeginFunction)LoadFunction( libname, "_Begin" );
			if( !Begin )
				Begin = (BeginFunction)LoadFunction( libname, "Begin" );
			if( !Begin )
				Begin = (BeginFunction)LoadFunction( libname, "Begin_" );
			if( Begin )
			{
				int xlen, ofs, arg;
				for( arg = arg_offset, xlen = 0; arg < my_argc; arg++, xlen += snprintf( NULL, 0, "%s%s", arg?" ":"", my_argv[arg] ) );
				x = (TEXTCHAR*)malloc( ++xlen );
				for( arg = arg_offset, ofs = 0; arg < my_argc; arg++, ofs += snprintf( x + ofs, xlen - ofs, "%s%s", arg?" ":"", my_argv[arg] ) );
				Begin( x, MODE ); // pass console defined in Makefile
				free( x );
			}
			else
			{
				Start = (StartFunction)LoadFunction( libname, "_Start" );
				if( !Start )
					Start = (StartFunction)LoadFunction( libname, "Start" );
				if( !Start )
					Start = (StartFunction)LoadFunction( libname, "Start_" );
				if( Start )
				{
					Start( );
				}
			}
		}
	}
#  ifdef BUILD_SERVICE_THREAD
	return 0;
#  endif
}

#endif

SaneWinMain( argc, argv )
{
	{
#ifndef BUILD_SERVICE
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
			hModule = LoadFunction( libname = ExpandPath(LOAD_LIBNAME), NULL );
			if( !hModule )
			{
#ifndef UNDER_CE
				lprintf( "error: (%"_32fs ")%s"
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
#endif
		my_argc = argc;
		my_argv = argv;

#ifdef BUILD_SERVICE
		{
			// look through command line, and while -L options exist, use thsoe to load more libraries
			// then pass the final remainer to the proc (if used)
#ifdef _WIN32
			for( ; arg_offset < argc; arg_offset++ )
			{
				if( StrCaseCmp( argv[arg_offset], "install" ) == 0 )
				{
					ServiceInstall( GetProgramName() );
					return 0;
				}
				if( StrCaseCmp( argv[arg_offset], "uninstall" ) == 0 )
				{
					ServiceUninstall( GetProgramName() );
					return 0;
				}
			}
			// need to do this before windows are created?
#ifdef BUILD_SERVICE_THREAD
			xlprintf(2400)( "Go To Service. %s", GetProgramName() );
			SetupServiceThread( (TEXTSTR)GetProgramName(), DoStart2, 0 );
#else
			xlprintf(2400)( "Go To Service. %s", GetProgramName() );
			SetupService( (TEXTSTR)GetProgramName(), DoStart2 );
#endif
#endif
		}
#endif
#ifndef BUILD_SERVICE
		{

			Main = (MainFunction)LoadFunction( libname, "_Main" );
			if( !Main )
				Main = (MainFunction)LoadFunction( libname, "Main" );
			if( !Main )
				Main = (MainFunction)LoadFunction( libname, "Main_" );
			if( Main )
			{
				Main( argc-arg_offset, argv+arg_offset, MODE );
			}
			else
			{
				Begin = (BeginFunction)LoadFunction( libname, "_Begin" );
				if( !Begin )
					Begin = (BeginFunction)LoadFunction( libname, "Begin" );
				if( !Begin )
					Begin = (BeginFunction)LoadFunction( libname, "Begin_" );
				if( Begin )
				{
					int xlen, ofs, arg;
					for( arg = arg_offset, xlen = 0; arg < argc; arg++, xlen += snprintf( NULL, 0, "%s%s", arg?" ":"", argv[arg] ) );
					x = (TEXTCHAR*)malloc( ++xlen );
					for( arg = arg_offset, ofs = 0; arg < argc; arg++, ofs += snprintf( (TEXTCHAR*)x + ofs, xlen - ofs, "%s%s", arg?" ":"", argv[arg] ) );
					Begin( x, MODE ); // pass console defined in Makefile
					free( x );
				}
				else
				{
					Start = (StartFunction)LoadFunction( libname, "_Start" );
					if( !Start )
						Start = (StartFunction)LoadFunction( libname, "Start" );
					if( !Start )
						Start = (StartFunction)LoadFunction( libname, "Start_" );
					if( Start )
					{
						Start( );
					}
				}
			}
		}
#endif
	}
	return 0;
}
EndSaneWinMain( )

// $Log: runwin.c,v $
// Revision 1.6  2003/08/01 00:58:34  panther
// Fix loader for other alternate entry proc
//
// Revision 1.5  2003/03/25 08:59:03  panther
// Added CVS logging
//
