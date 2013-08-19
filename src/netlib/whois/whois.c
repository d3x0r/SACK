#include <stdhdrs.h>
#include <network.h>

#include <sharemem.h>
PVARTEXT result;

int main( int argc, char **argv, char **env )
{
   SetAllocateLogging( TRUE );
    if( argc< 2 )
    {
        printf( WIDE("usage: %s <hostname>\n"), argv[0] );
        return -1;
	 }
    result = VarTextCreateEx( 10000, 1000 );
    DoWhois( argv[1], NULL, result );
    printf( WIDE("%s"), result );
    return 0;
}

// $Log: whois.c,v $
// Revision 1.2  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
