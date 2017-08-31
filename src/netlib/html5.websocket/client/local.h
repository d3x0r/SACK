
#include <http.h>
#include <html5.websocket.client.h>

typedef struct web_socket_client *WebSocketClient;


struct web_socket_client_local
{
	uint32_t timer;
	PLIST clients;
	CRITICALSECTION cs_opening;
	struct web_socket_client *opening_client;
	struct random_context *rng;
} wsc_local;
