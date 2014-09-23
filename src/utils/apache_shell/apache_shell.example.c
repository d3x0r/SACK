// sources for this module... http://httpd.apache.org/docs/2.4/developer/modguide.html

// don't define exit() apache has a definition for it.
#define NO_SACK_EXIT_OVERRIDE

#include <stdhdrs.h>

#include <httpd.h>  // apache header files.

/*
 http://ci.apache.org/projects/httpd/trunk/doxygen/structrequest__rec.html.
 or - request_rec structure can be found in the httpd.h header

Some key elements of the request_rec structure are:

r->handler (char*): Contains the name of the handler the server is currently asking to do the handling of this request
r->method (char*): Contains the HTTP method being used, f.x. GET or POST
r->filename (char*): Contains the translated filename the client is requesting
r->args (char*): Contains the query string of the request, if any
r->headers_in (apr_table_t*): Contains all the headers sent by the client
r->connection (conn_rec*): A record containing information about the current connection
r->user (char*): If the URI requires authentication, this is set to the username provided
r->useragent_ip (char*): The IP address of the client connecting to us
r->pool (apr_pool_t*): The memory pool of this request. We'll discuss this in the "Memory management" chapter.


General response codes:

DECLINED: We are not handling this request
OK: We handled this request and it went well
DONE: We handled this request and the server should just close this thread without further processing

HTTP specific return codes (excerpt):

HTTP_OK (200): Request was okay
HTTP_MOVED_PERMANENTLY (301): The resource has moved to a new URL
HTTP_UNAUTHORIZED (401): Client is not authorized to visit this page
HTTP_FORBIDDEN (403): Permission denied
HTTP_NOT_FOUND (404): File not found
HTTP_INTERNAL_SERVER_ERROR (500): Internal server error (self explanatory)


Some useful functions you should know
ap_rputs(const char *string, request_rec *r): 
Sends a string of text to the client. This is a shorthand version of ap_rwrite.
ap_rputs("Hello, world!", r);
ap_rprintf: 
This function works just like printf, except it sends the result to the client.
ap_rprintf(r, "Hello, %s!", r->useragent_ip);
ap_set_content_type(request_rec *r, const char *type): 
Sets the content type of the output you are sending.
ap_set_content_type(r, "text/plain"); // force a raw text output


Memory management
void* apr_palloc( apr_pool_t *p, apr_size_t size): Allocates size number of bytes in the pool for you
void* apr_pcalloc( apr_pool_t *p, apr_size_t size): Allocates size number of bytes in the pool for you and sets all bytes to 0
char* apr_pstrdup( apr_pool_t *p, const char *s): Creates a duplicate of the string s. This is useful for copying constant values so you can edit them
char* apr_psprintf( apr_pool_t *p, const char *fmt, ...): Similar to sprintf, except the server supplies you with an appropriately allocated target variable
Let's put these functions into an example handler:


{
   apr_table_t*GET;
   apr_array_header_t*POST;

   ap_args_to_table(r, &GET);
	ap_parse_form_data(r, NULL, &POST, -1, 8192);
}


*/


static int example_handler(request_rec *r)
{
    /* First off, we need to check if this is a call for the "example-handler" handler.
     * If it is, we accept it and do our things, if not, we simply return DECLINED,
     * and the server will try somewhere else.
     */
    if (!r->handler || strcmp(r->handler, "example-handler")) return (DECLINED);
    
    /* Now that we are handling this request, we'll write out "Hello, world!" to the client.
     * To do so, we must first set the appropriate content type, followed by our output.
     */
    ap_set_content_type(r, "text/html");
    ap_rprintf(r, "Hello, world!");
    
    /* Lastly, we must tell the server that we took care of this request and everything went fine.
     * We do so by simply returning the value OK to the server.
     */
    return OK;
}

static void register_hooks(apr_pool_t *pool)
{
    /* Create a hook in the request handler, so we get called when a request arrives */
    ap_hook_handler(example_handler, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA foo_module =
{
    STANDARD20_MODULE_STUFF,
    NULL, //create_dir_conf, /* Per-directory configuration handler */
    NULL, //merge_dir_conf,  /* Merge handler for per-directory configurations */
    NULL, //create_svr_conf, /* Per-server configuration handler */
    NULL, //merge_svr_conf,  /* Merge handler for per-server configurations */
    NULL, //directives,      /* Any directives we may have for httpd */
    register_hooks   /* Our hook registering function */
};


