#include <stdhdrs.h>
#include <http.h>


int main( int argc, char **argv )
{
	struct url_data *data = SACK_URLParse( argv[1] );
	CTEXTSTR url = SACK_BuildURL( data );
	printf( "recode: %s\n", url );

	printf( "protocol            =%s\n",data->protocol );
	printf( "user                =%s\n",data->user );
	printf( "password            =%s\n",data->password );
	printf( "host                =%s\n",data->host );
	printf( "default port        =%d\n",data->default_port );
	printf( "port                =%d\n",data->port );
	printf( "resource_path       =%s\n",data->resource_path );
	printf( "resource_file       =%s\n",data->resource_file );
	printf( "resource_extension  =%s\n",data->resource_extension );
	printf( "resource_anchor     =%s\n",data->resource_anchor );

	if( !data->cgi_parameters )
		printf( "No CGI Parameters.\n" );
   else
	{
		INDEX idx;
		struct url_cgi_data *cgi_data;
		LIST_FORALL( data->cgi_parameters, idx, struct url_cgi_data *, cgi_data )
		{
			printf( "CGI PARAM %d: %s=%s\n", idx, cgi_data->name, cgi_data->value );
		}
	}
   return 0;
}



