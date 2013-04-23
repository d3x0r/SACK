
#define MSGQ_ID_BASE "Srvr"

enum server_failure_messages {
	CLIENT_UNKNOWN
, MESSAGE_UNKNOWN
, MESSAGE_INVALID // sending server(sourced) messages to server
, SERVICE_UNKNOWN // could not find a service for the message.
, UNABLE_TO_LOAD
};

enum service_messages {
	SERVER_FAILURE      // server responce to clients - failure
      // failure may result for the above reasons.
, SERVER_SUCCESS       // server responce to clients - success
, CLIENT_LOAD_SERVICE  // client requests a service
, CLIENT_CONNECT       // new client wants to connect
, CLIENT_DISCONNECT    // client disconnects (no responce)
, RU_ALIVE             // server/client message to other requesting status
, IM_ALIVE             // server/client message to other responding status
};

// $Log: $
