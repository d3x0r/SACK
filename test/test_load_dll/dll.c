#include <stdio.h>

__declspec( dllimport) void g( void );
__declspec(dllexport) void f(void ) {
    printf( "F function\n" );
    g();
}
