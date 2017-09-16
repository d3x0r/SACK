/* Generalized HTTP Processing. All POST, GET, RESPONSE packets
   all fit within this structure.
                                                                */

#ifndef HTTP_PROCESSING_INCLUDED
/* Multiple inclusion protection symbol */
#define HTTP_PROCESSING_INCLUDED

#include <sack_types.h>
#include <procreg.h>
#include <sack_system.h>
#include <network.h>

#ifdef HTTP_SOURCE
#define HTTP_EXPORT EXPORT_METHOD
#else
/* Defines how external functions are referenced
   (dllimport/export/extern)                     */
#define HTTP_EXPORT IMPORT_METHOD
#endif
/* The API type of HTTP functions - default to CPROC. */
#define HTTPAPI CPROC

#ifdef __cplusplus
/* A symbol to define the sub-namespace of HTTP_NAMESPACE  */
#define _HTTP_NAMESPACE namespace http {
/* A macro to end just the HTTP sub namespace. */
#define _HTTP_NAMESPACE_END }
#else
#define _HTTP_NAMESPACE
#define _HTTP_NAMESPACE_END
#endif
/* HTTP full namespace  */
#define HTTP_NAMESPACE TEXT_NAMESPACE _HTTP_NAMESPACE
/* Macro to use to define where http utility namespace ends. */
#define HTTP_NAMESPACE_END _HTTP_NAMESPACE_END TEXT_NAMESPACE_END

SACK_CONTAINER_NAMESPACE
/* Text library functions. PTEXT is kept as a linked list of
   segments of text. Each text segment has a size and the data,
   and additional format flags. PTEXT may also be indirect
   segments (that is this segment points at another list of
   segments that are the actualy content for this place.
                                                                */
_TEXT_NAMESPACE
	/* Simple HTTP Packet processing state. Its only intelligence is
	   that there are fields of http header, and that one of those
	   fields might be content-length; so it can seperate individual
	   fields name-value pairs and the packet content.               */
	_HTTP_NAMESPACE

struct HttpField {
	PTEXT name;
	PTEXT value;
};

typedef struct HttpState *HTTPState;
enum ProcessHttpResult{
	HTTP_STATE_RESULT_NOTHING = 0,
	HTTP_STATE_RESULT_CONTENT = 200,
    HTTP_STATE_RESULT_CONTINUE = 100,
	HTTP_STATE_INTERNAL_SERVER_ERROR=500,
	HTTP_STATE_RESOURCE_NOT_FOUND=404,
   HTTP_STATE_BAD_REQUEST=400,
};

HTTP_EXPORT /* Creates an empty http state, the next operation should be
   AddHttpData.                                              */
HTTPState  HTTPAPI CreateHttpState( void );
HTTP_EXPORT /* Destroys a http state, releasing all resources associated
   with it.                                                  */
void HTTPAPI DestroyHttpState( HTTPState pHttpState );
HTTP_EXPORT /* Add another bit of data to the block. After adding data,
   ProcessHttp should be called to see if the data has completed
   a packet.
   Parameters
   pHttpState :  state to add data to
   buffer :      pointer to some data bytes
   size :        length of data bytes                           
   Returns: TRUE if content is added... if collecting chunked encoding may return FALSE.
   */
LOGICAL HTTPAPI AddHttpData( HTTPState pHttpState, POINTER buffer, size_t size );
/* \returns TRUE if completed until content-length if
   content-length is not specified, data is still collected, but
   the status never results TRUE.
   
   
	Parameters
	pc : Occasionally the http processor needs to send data on the
	     socket without application being aware it did.
   pHttpState :  Http State to process (after having added data to
                 it)
   
   Return Value List
   TRUE :   A completed HTTP packet has been gathered \- according
            to 'content\-length' meta tag.
   FALSE :  Still collecting full packet                           */
//HTTP_EXPORT int HTTPAPI ProcessHttp( HTTPState pHttpState );
HTTP_EXPORT int HTTPAPI ProcessHttp( PCLIENT pc, HTTPState pHttpState );

HTTP_EXPORT /* Gets the specific result code at the header of the packet -
   http 2.0 OK sort of thing.                                  */
PTEXT HTTPAPI GetHttpResponce( HTTPState pHttpState );

/* Get the method of the request in ht e http state.
*/
HTTP_EXPORT PTEXT HTTPAPI GetHttpMethod( struct HttpState *pHttpState );

/*Get the value of a HTTP header field, by name
   Parameters
	pHttpState: the state to get the header field from.
	name: name of the field to get (checked case insensitive)
*/
HTTP_EXPORT PTEXT HTTPAPI GetHTTPField( HTTPState pHttpState, CTEXTSTR name );


/* Gets the specific request code at the header of the packet -
   http 2.0 OK sort of thing.                                  */
HTTP_EXPORT PTEXT HTTPAPI GetHttpRequest( HTTPState pHttpState );
/* \Returns the body of the HTTP packet (the part of data
   specified by content-length or by termination of the
   connection(? think I didn't implement that right)      */
HTTP_EXPORT PTEXT HTTPAPI GetHttpContent( HTTPState pHttpState );

/* \Returns the resource path/name of the HTTP packet (the part of data
   specified by content-length or by termination of the
   connection(? think I didn't implement that right)      */
HTTP_EXPORT PTEXT HTTPAPI GetHttpResource( HTTPState pHttpState );

/* Returns a list of fields that were included in a request header.
   members of the list are of type struct HttpField.
   see also: ProcessHttpFields and ProcessCGIFields
*/
HTTP_EXPORT PLIST HTTPAPI GetHttpHeaderFields( HTTPState pHttpState );

HTTP_EXPORT int HTTPAPI GetHttpVersion( HTTPState pHttpState );

HTTP_EXPORT /* Enumerates the various http header fields by passing them
   each sequentially to the specified callback.
   Parameters
   pHttpState :  _nt_
   _nt_ :        _nt_
   psv :         _nt_                                        */
void HTTPAPI ProcessCGIFields( HTTPState pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv );
HTTP_EXPORT /* Enumerates the various http header fields by passing them
   each sequentially to the specified callback.
   Parameters
   pHttpState :  _nt_
   _nt_ :        _nt_
   psv :         _nt_                                        */
void HTTPAPI ProcessHttpFields( HTTPState pHttpState, void (CPROC*f)( uintptr_t psv, PTEXT name, PTEXT value ), uintptr_t psv );
HTTP_EXPORT /* Resets a processing state, so it can start collecting the
   next state. After a ProcessHttp results with true, this
   should be called after processing the packet content.
   Parameters
   pHttpState :  state to reset for next read...             */
void HTTPAPI EndHttp( HTTPState pHttpState );


HTTP_EXPORT
/* reply message - 200/OK with this body, sent as Content-Type that was requested */
void HTTPAPI SendHttpMessage( HTTPState pHttpState, PCLIENT pc, PTEXT body );

HTTP_EXPORT
/* generate response message, specifies the numeric (200), the text (OK), the content type field value, and the body to send */
void HTTPAPI SendHttpResponse ( HTTPState pHttpState, PCLIENT pc, int numeric, CTEXTSTR text, CTEXTSTR content_type, PTEXT body );

/* Callback type used when creating an http server.
 If there is no registered handler match, then this is called.
 This should return FALSE if there was no content, allowing a 404 status result.
 Additional ways of dispatching need to be implemented (like handlers for paths, wildcards...)
 */
typedef LOGICAL (CPROC *ProcessHttpRequest)( uintptr_t psv
												 , HTTPState pHttpState );

HTTP_EXPORT
/* Intended to create a generic http service, which you can
   attach URL handlers to. Incomplete                      
   Works mostly?  OnGet has been known to get called....
   */
struct HttpServer *CreateHttpServerEx( CTEXTSTR interface_address, CTEXTSTR TargetName, CTEXTSTR site, ProcessHttpRequest handle_request, uintptr_t psv );


HTTP_EXPORT
/* Intended to create a generic http service, which you can
   attach URL handlers to. Incomplete
   Works mostly?  OnGet has been known to get called....
   */
struct HttpServer *CreateHttpsServerEx( CTEXTSTR interface_address, CTEXTSTR TargetName, CTEXTSTR site, ProcessHttpRequest handle_request, uintptr_t psv );

/* results with just the content of the message; no access to other information avaialble */
HTTP_EXPORT PTEXT HTTPAPI PostHttp( PTEXT site, PTEXT resource, PTEXT content );
/* results with just the content of the message; no access to other information avaialble */
HTTP_EXPORT PTEXT HTTPAPI GetHttp( PTEXT site, PTEXT resource, LOGICAL secure );
/* results with just the content of the message; no access to other information avaialble */
HTTP_EXPORT PTEXT HTTPAPI GetHttps( PTEXT address, PTEXT url );

/* results with the http state of the message response; Allows getting other detailed information about the result */
HTTP_EXPORT HTTPState  HTTPAPI PostHttpQuery( PTEXT site, PTEXT resource, PTEXT content );
/* results with the http state of the message response; Allows getting other detailed information about the result */
HTTP_EXPORT HTTPState  HTTPAPI GetHttpQuery( PTEXT site, PTEXT resource );
/* results with the http state of the message response; Allows getting other detailed information about the result */
HTTP_EXPORT HTTPState HTTPAPI GetHttpsQuery( PTEXT site, PTEXT resource );

#define CreateHttpServer(interface_address,site,psv) CreateHttpServerEx( interface_address,NULL,site,NULL,psv )
#define CreateHttpServer2(interface_address,site,default_handler,psv) CreateHttpServerEx( interface_address,NULL,site,default_handler,psv )

// receives events for either GET if aspecific OnHttpRequest has not been defined for the specific resource
// Return TRUE if processed, otherwise will attempt to match other Get Handlers
#define OnHttpGet( site, resource ) \
	__DefineRegistryMethod(WIDE( "SACK/Http/Methods" ),OnHttpGet,site,resource,WIDE( "Get" ),LOGICAL,(uintptr_t,PCLIENT,struct HttpState *,PTEXT),__LINE__)

// receives events for either GET if aspecific OnHttpRequest has not been defined for the specific resource
// Return TRUE if processed, otherwise will attempt to match other Get Handlers
#define OnHttpPost( site, resource ) \
	__DefineRegistryMethod(WIDE( "SACK/Http/Methods" ),OnHttpPost,site,resource,WIDE( "Post" ),LOGICAL,(uintptr_t,PCLIENT,struct HttpState *,PTEXT),__LINE__)

// define a specific handler for a specific resource name on a host
#define OnHttpRequest( site, resource ) \
	__DefineRegistryMethod(WIDE( "SACK/Http/Methods" ),OnHttpRequest,WIDE( "something" ),site WIDE( "/" ) resource,WIDE( "Get" ),void,(uintptr_t,PCLIENT,struct HttpState *,PTEXT),__LINE__)


//--------------------------------------------------------------
//  URL.c  (url parsing utility)

struct url_cgi_data
{
	CTEXTSTR name;
	CTEXTSTR value;
};

struct url_data
{
	CTEXTSTR protocol;
	CTEXTSTR user;
	CTEXTSTR password;
	CTEXTSTR host;
	int default_port;
	int port;  // encoding RFC3986 http://tools.ietf.org/html/rfc3986  specifies port characters are in the set of digits.
	//CTEXTSTR port_data;  // during collection, the password may be in the place of 'port'
	CTEXTSTR resource_path;
	CTEXTSTR resource_file;
	CTEXTSTR resource_extension;
	CTEXTSTR resource_anchor;
   // list of struct url_cgi_data *
	PLIST cgi_parameters;
};

HTTP_EXPORT struct url_data * HTTPAPI SACK_URLParse( const char *url );
HTTP_EXPORT char *HTTPAPI SACK_BuildURL( struct url_data *data );
HTTP_EXPORT void HTTPAPI SACK_ReleaseURL( struct url_data *data );


	_HTTP_NAMESPACE_END
TEXT_NAMESPACE_END

#ifdef __cplusplus
using namespace sack::containers::text::http;
#endif

#endif