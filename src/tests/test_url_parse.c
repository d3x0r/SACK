#include <stdhdrs.h>
#include <http.h>


int main( int argc, char **argv )
{
	struct url_data *data = SACK_URLParse( DupCharToText( argv[1] ) );
	CTEXTSTR url = SACK_BuildURL( data );
	printf( WIDE("recode: %s\n"), url );

	printf( WIDE("protocol            =%s\n"),data->protocol );
	printf( WIDE("user                =%s\n"),data->user );
	printf( WIDE("password            =%s\n"),data->password );
	printf( WIDE("host                =%s\n"),data->host );
	printf( WIDE("default port        =%d\n"),data->default_port );
	printf( WIDE("port                =%d\n"),data->port );
	printf( WIDE("resource_path       =%s\n"),data->resource_path );
	printf( WIDE("resource_file       =%s\n"),data->resource_file );
	printf( WIDE("resource_extension  =%s\n"),data->resource_extension );
	printf( WIDE("resource_anchor     =%s\n"),data->resource_anchor );

	if( !data->cgi_parameters )
		printf( WIDE("No CGI Parameters.\n") );
	else
	{
		INDEX idx;
		struct url_cgi_data *cgi_data;
		LIST_FORALL( data->cgi_parameters, idx, struct url_cgi_data *, cgi_data )
		{
			printf( WIDE("CGI PARAM %d: %s=%s\n"), (int)idx, cgi_data->name, cgi_data->value );
		}
	}
	return 0;
}



