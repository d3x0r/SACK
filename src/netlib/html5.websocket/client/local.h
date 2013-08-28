
#include <http.h>

struct web_socket_client
{
	struct web_socket_client_flags
	{
		BIT_FIELD want_close : 1; // schedule to close
	} flags;
	PCLIENT pc;
	POINTER buffer;
	CTEXTSTR host;
	CTEXTSTR address_url;
	struct url_data *url;

	web_socket_opened on_open;
	web_socket_event on_event;
	web_socket_closed on_close;
	web_socket_error on_error;
   PTRSZVAL psv_on;

};


struct web_socket_client_local
{
   _32 timer;
   PLIST clients;
} wsc_local;
