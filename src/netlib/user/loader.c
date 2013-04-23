#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>

int main( int argc, char **argv )
{
    void* hModule;
    int (*nextmain)(int argc, char **argv );
    int loadcnt;
    sprintf( dl, WIDE("puser.so"), argv[2] );
    hModule = dlopen( dl, RTLD_NOW );
    if( !hModule )
        fprintf( stderr, WIDE("Failed to load program \'%s\'.(%s)\n"), argv[2], dlerror() );
    else
    {
        nextmain = dlsym( hModule, WIDE("main") );
        return nextmain( argc-2, argv+2 );
    }
    return 0;
}
// $Log: $
