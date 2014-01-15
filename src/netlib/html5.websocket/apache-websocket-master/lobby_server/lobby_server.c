#include <stdhdrs.h>
#include "websocket_plugin.h"


// return NULL to decline
static void * CALLBACK OnConnect(const WebSocketServer *server)
{
	request_rec *r = server->request( server );
	lprintf( "uri is %s", r->unparsed_uri );
	lprintf( "uri is %s", r->uri );
	// find registered interface for this request.


}

static size_t CALLBACK echo_on_message(void *plugin_private, const WebSocketServer *server,
    const int type, unsigned char *buffer, const size_t buffer_size)
{
  return server->send(server, type, buffer, buffer_size);
}

/*
 * Since we are dealing with a simple echo WebSocket plugin, we don't need to
 * concern ourselves with any connection state. Also, since we are returning a
 * pointer to static memory, there is no need for a "destroy" function.
 */

static WebSocketPlugin s_plugin = {
  sizeof(WebSocketPlugin),
  WEBSOCKET_PLUGIN_VERSION_0,
  NULL, /* destroy */
  NULL, /* on_connect */
  echo_on_message,
  NULL /* on_disconnect */
};

extern EXPORT WebSocketPlugin * CALLBACK echo_init()
{
  return &s_plugin;
}

