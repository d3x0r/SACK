//#include <stdhdrs.h>
#include <sack_types.h>
#include <deadstart.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <run.h>

// or maybe don't changeme...
#define CHANGEME argv[1]

int main( int argc, char **argv )
{
	int arg_offset = 1;
    void* hModule = NULL;
    char pMyPath[256];
    char pLoadName[280];
    char pLoadName2[280];
    MainFunction Main;
    BeginFunction Begin;
	 StartFunction Start;
    char *libname = CHANGEME;
    //putenv( "LD_LIBRARY_PATH=./" );
    //printf( "Added: %s\n", getenv( "LD_LIBRARY_PATH"));
    {
        char buf[256], *pb;
        int n;
        n = readlink("/proc/self/exe",buf,256);
        if( n >= 0 )
        {
            buf[n]=0; //linux
            if( !n )
            {
                strcpy( buf, "." );
                buf[ n = readlink( "/proc/curproc/",buf,256)]=0; // fbsd
            }
        }
        else
            strcpy( buf, ".") ;
        pb = strrchr(buf,'/');
        if( pb )
            pb[0]=0;
        fprintf( stderr, "My execution: %s\n", buf);
        strcpy( pMyPath, buf );
	 }
	 if( libname )
		 hModule = dlopen( libname, RTLD_NOW|RTLD_GLOBAL );
	 if( !hModule )
	 {
		 if( libname )
		 {
			 sprintf( pLoadName, "%s/%s", pMyPath, libname );
			 hModule = dlopen( pLoadName, RTLD_NOW|RTLD_GLOBAL );
		 }
		  if( !hModule )
		  {
#ifdef LOAD_LIBNAME
				hModule = dlopen( LOAD_LIBNAME, RTLD_NOW|RTLD_GLOBAL );
				if( !hModule )
				{
					sprintf( pLoadName2, "%s/%s", pMyPath, LOAD_LIBNAME );
					hModule = dlopen( pLoadName2, RTLD_NOW|RTLD_GLOBAL );
					if( !hModule )
					{
						fprintf( stderr, "Failed to load...%s(%s)\n", libname, dlerror() );
						fprintf( stderr, "Failed to load....%s(%s)\n", pLoadName, dlerror() );
						fprintf( stderr, "Failed to load...%s(%s)\n", LOAD_LIBNAME, dlerror() );
						fprintf( stderr, "Failed to load....%s(%s)\n", pLoadName, dlerror() );
						return 0;
					}
					else
                  arg_offset = 0;
				}
				else
					arg_offset = 0;
#else
				fprintf( stderr, "Failed to load...%s(%s)\n", libname, dlerror() );
            fprintf( stderr, "Failed to load....%s(%s)\n", pLoadName, dlerror() );
				return 0;
#endif
		  }
	 }
	 if( !hModule && !libname )
	 {
		 fprintf( stderr, "No library core specified to load.\n" );
       return 0;
	 }
	 if( hModule )
	 {
		 Main = (void(*)(int, char**,int))dlsym( hModule, "_Main" );
		 if( !Main )
			 Main = (void(*)(int, char**,int))dlsym( hModule, "Main" );
		 if( Main )
		 {
			 InvokeDeadstart();
			 Main( argc - arg_offset, argv + arg_offset, 1 ); // pass console true
		 }
		 else
			 fprintf( stderr, "Failed to load main routine.\n");

		 Begin = (void(*)(char*,int))dlsym( hModule, "_Begin" );
		 if( !Begin )
			 Begin = (void(*)(char*,int))dlsym( hModule, "Begin" );
		 if( Begin )
		 {
			 int xlen, ofs, arg;
			 char *x;
			 for( arg = arg_offset, xlen = 0; arg < argc; arg++, xlen += snprintf( NULL, 0, "%s%s", arg?" ":"", argv[arg] ) );
			 x = (char*)malloc( ++xlen );
			 for( arg = arg_offset, ofs = 0; arg < argc; arg++, ofs += snprintf( x + ofs, xlen - ofs, "%s%s", arg?" ":"", argv[arg] ) );
			 InvokeDeadstart();
			 Begin( x, 1 ); // pass console true
			 free( x );
		 }
		 else
			 fprintf( stderr, "Failed to load begin routine.\n");

		 Start = (void(*)())dlsym( hModule, "_Start" );
		 if( !Start )
			 Start = (void(*)())dlsym( hModule, "Start" );
		 if( Start )
		 {
			 InvokeDeadstart();
			 Start( ); // pass console true
		 }
		 else
			 fprintf( stderr, "Failed to load start routine.\n");
	 }
	 return 0;
}
// $Log: runlnx.c,v $
// Revision 1.9  2005/06/15 04:23:37  d3x0r
// Command line includes command executino path, not just arguments... preferably with the whole path, so that related plugins and libraries may be loaded from this point.
//
// Revision 1.8  2005/06/15 04:21:06  d3x0r
// Fix linux command line... build a single string from the args - loss of quotage imminant.
//
// Revision 1.7  2003/03/25 08:59:03  panther
// Added CVS logging
//
