
#include <stdhdrs.h>
#include <http.h>

int main( int argc, char **argv )
{
	if( argv[1] )
	{
		struct url_data *url = SACK_URLParse( argv[1] );
		HTTPState httpResponse;		
		PVARTEXT pvtSite = VarTextCreate();
		PVARTEXT pvtResource = VarTextCreate();
		char* resource;
		NetworkStart();
		vtprintf( pvtSite, "%s:%d", url->host, url->port?url->port:url->default_port );
		if( url->resource_extension )
			vtprintf( pvtResource, "%s/%s.%s", url->resource_path, url->resource_file, url->resource_extension );
		else if( url->resource_file )
			vtprintf( pvtResource, "%s/%s", url->resource_path, url->resource_file, url->resource_extension );
		else if( url->resource_path )
			vtprintf( pvtResource, "%s", url->resource_path, url->resource_file, url->resource_extension );

		resource = GetText( VarTextPeek( pvtResource ) );
		if( !resource ) resource = "";

      printf( "waiting for [%s/%s]\n", GetText( VarTextPeek( pvtSite ) ), resource );

		do {
			if( url->protocol[4] == 's' ) {
				httpResponse = GetHttpsQuery( VarTextPeek( pvtSite ), VarTextPeek( pvtResource ), NULL );
			} else {
				httpResponse = GetHttpQuery( VarTextPeek( pvtSite ), VarTextPeek( pvtResource ) );
			}
			//printf( "Got: %p", httpResponse );
			if( httpResponse ) {
				int code = GetHttpResponseCode( httpResponse );
				if(  code == 200 || code == 301 )
					break;
				else 
					printf( "Request Failed: %s.\n", code?"reject":"timeout");
			} else
				printf( "Request Critically Failed.\n" );

		} while(1);

		SACK_ReleaseURL( url );
	}
	else
	{
		printf( "%s <http[s]://address[:port][/path]>\n"
				 " - wait until an HTTP 200 OK response is received from specified address port and path.\n"
				, argv[0] );
	}
   return 0;
}
