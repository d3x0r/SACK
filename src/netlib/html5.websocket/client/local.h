

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
};


struct web_socket_client_local
{

   PLIST clients;
} wsc_local;
