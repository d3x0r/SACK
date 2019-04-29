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
	printf( "Available services\n"
	        "---------------------------\n" );
	LIST_FORALL( list, idx, char *, service )
	{
		printf( "%s\n", service );
	}
	return 0;
}

