
#include <stdhdrs.h>
#include <http.h>


void ProcessRequest( uintptr_t psv
						 , struct HttpState *pHttpState
						 , CTEXTSTR site
						 , CTEXTSTR resource
						  // data may be CGI, but it might be binary like for a file post
						 , POINTER data, uint32_t length )
{

}

int main( void )
{
	POINTER buffer;
	uintptr_t size;
	TEXTCHAR url[256];
	struct HttpRequestState *http;

	{
		struct HttpServer *server;
		server = CreateHttpServer( NULL, NULL, ProcessRequest, 0 );
	}

	http = CreateHttpRequestState();

	if( HttpRequest( http, url, &buffer, &size ) )
	{

	}
   return 0;
}
