#include <stdhdrs.h>
#include <construct.h>
#include <msgclient.h>

int main( void )
{
	PLIST list = NULL;
	INDEX idx;
	char *service;
   LoadComplete();
	GetServiceList( &list );
	printf( WIDE("Available services\n")
           WIDE("---------------------------\n") );
	LIST_FORALL( list, idx, char *, service )
	{
      printf( WIDE("%s\n"), service );
	}
   return 0;
}

