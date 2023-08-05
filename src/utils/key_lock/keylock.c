
#include <stdhdrs.h>
#include <systray.h>


LOGICAL blocking = FALSE;
//SYSTRAY_PROC void AddSystrayMenuFunction( CTEXTSTR text, void (CPROC*function)(void) );
//SYSTRAY_PROC void AddSystrayMenuFunction_v2( CTEXTSTR text, void (CPROC* function)(uintptr_t), uintptr_t );


void enable( void ) {
    blocking = !blocking;
}


int main( ){
    RegisterIcon( "test.png" );
    SetIconDoubleClick( enable );

    while( 1 ) WakeableSleep( 10000 );
    return;
}

