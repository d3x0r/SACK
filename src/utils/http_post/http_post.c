
#include <stdhdrs.h>
#include <http.h>


int main( void )
{
   PTEXT result = PostHttp( SegCreateFromText( "www.hyenaspots.com" ), SegCreateFromText( "code.php" ), SegCreateFromText( "c=12" ) );
	if( result )
	{
      lprintf( "%s", GetText( result ) );
	}
   LineRelease( result );
   return 0;
}



