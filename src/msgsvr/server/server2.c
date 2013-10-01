#define SERVERMSG_SOURCE
#include <stdhdrs.h>
#include <deadstart.h>
#include <sharemem.h>
#include <msgclient.h>

MSGCLIENT_NAMESPACE
	extern void CPROC HandleServerMessage( PMSGHDR msg );
	extern void CPROC HandleServerMessageEx( PMSGHDR msg DBG_PASS );
MSGCLIENT_NAMESPACE_END

typedef struct service_tag
{
	struct service_tag *next, **me;
	struct {
		_32 bRemote : 1;
	} flags;
	TEXTCHAR *name;

	SERVICE_ENDPOINT client_id;
} SERVICE, *PSERVICE;

typedef struct client_tag
{
   // unique routing ID of this client... services, clients, etc all ahve one.
	SERVICE_ROUTE route_id; // process only? no thread?
	struct {
		_32 valid : 1;
		_32 error : 1; // uhmm - something like got a message from this but it wasn't known
		_32 status_queried : 1;
	} flags;
	struct client_tag *next, **me;
} CLIENT, *PCLIENT;

typedef struct global_tag
{
	PSERVICE services;
	PCLIENT clients;
	int nClient;
	int nService; // for now, these are globally unique
} GLOBAL;

static GLOBAL g;

//-----------------------------------------------------------------------
//--------------------------------------------------------------------
// first client ID will be 200.
// subsequent clients use this ID for message service...
// using PID sucks cause under 2.6 kernels PTHREAD_SELF results
// with what looks like a stack memory address - and I'd hate to
// truncate the top bit....

static void DeleteAService( PSERVICE service )
{
	if( ( (*(service->me)) = (service->next) ) )
		service->next->me = service->me;
	Release( service->name );
	Release( service );
}

static PCLIENT AddClient( PSERVICE_ROUTE client_id )
{
	PCLIENT client = NULL;
	for( client = g.clients; client; client = client->next )
	{
		// need to always add a client...
		// this is the whole purpose of handling CLINET_CONNECT
		// to give each person a unique ID to communicate with
		// cause nothing is reliable anymore under linux!
		if( client->route_id.dest.process_id == client_id->source.process_id )
		{
			Log( WIDE("Client has reconnected?!?!?!") );
			// suppose we can just let him continue...
			return client;
		}
	}

	{
		PCLIENT client = (PCLIENT)Allocate( sizeof( CLIENT ) );
		MemSet( client, 0, sizeof( CLIENT ) );

		client->route_id.dest.process_id = client_id->dest.process_id;
		client->route_id.dest.service_id = g.nClient++;
		client->route_id.source.process_id = 1;
		client->route_id.source.service_id = 0;

		client->flags.valid = 1;
		//lprintf( WIDE("New client..."));
		if( ( client->next = g.clients ) )
			g.clients->me = &client->next;
		client->me = &g.clients;
		g.clients = client;
		Log( WIDE("Added client...") );
		return client;
	}
}

// provide name of the service (dll)
// and result is the base index of the functions in the function table.
static PSERVICE FindService( TEXTCHAR *name )
{
	PSERVICE service;
	// check to see if the service is already loaded
	service = g.services;
	while( service )
	{
#undef strcmp
#undef strdup
#undef strlen
		if( !StrCmp( service->name, name ) )
			break;
		service = service->next;
	}
	if( service )
	{
		//Log( WIDE("Already loaded the service, resulting base ID") );
		if( !service->flags.bRemote )
		{
			// if the service is remote, it is not our job today to keep track of _____
			//service->references++;
		}
		return service;
	}
	return NULL;
}


static int CPROC MY_CLIENT_CONNECT( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
	PCLIENT client = AddClient( route );
	if( client )
	{
		// this is the ID that the client should use
		// to receive on.
		lprintf( WIDE("Client ID will be %d"), client->route_id.dest.service_id );
		((MSGIDTYPE*)result)[0] = client->route_id.dest.service_id;
		(*result_length) = sizeof( MSGIDTYPE );
		return TRUE;
	}
	(*result_length) = 0;
	return FALSE;
}

static int CPROC MY_CLIENT_LOAD_SERVICE( PSERVICE_ROUTE route, _32 *params, size_t param_length
								  , _32 *result, size_t *result_length )
{
	// okay now we're to the heart of the matter.
	// shoudl I have a table of services ? or rely on the client
	// knowing what the name of the service literally is?
	// I think that the aliasing can be good - then I can
	// have demo and test things
	// a client will have had to connect first to me....
	//PCLIENT client = FindClient( params[-1] );
	{
		PSERVICE service = FindService( (TEXTCHAR*)(params) );
		if( service )
		{
			if( service->flags.bRemote )
			{
				// need to send this message to the real person... and have them
				// result with this message....
				lprintf( WIDE("Actually this is a remote process... and we're going to forward to it's handler...") );
				if( ProbeClientAlive( &service->client_id ) )
				{
					PQMSG msg = (PQMSG)( ((P_8)params) - (sizeof( SERVICE_ENDPOINT )*2 + sizeof( _32 ) ));
					PSERVICE_ROUTE msg_route = (PSERVICE_ROUTE)msg;
					msg_route->dest = service->client_id;
					msg_route->source.service_id = g.nService++;
					// change source ID to reflect originator of Load
					//route->dest = service->client_id;
					// the real service will result to the client...
					// and further conversation will not involve me.

					if(SendOutMessage( msg
										, param_length + (sizeof( SERVICE_ENDPOINT )*2 + sizeof( _32 ) - sizeof( MSGIDTYPE ) ) ) >= 0 )
					{
						// this a GOOD result - we do NOT want to respond.
						// the server at the other end of this will be responsible for responding.
						(*result_length) = INVALID_INDEX;
						return FALSE;
					}
				}
				else
				{
					lprintf( WIDE("Oops, turns out that remote process is no longer active.") );
					// and remove from the lsit...
					DeleteAService( service );
				}
			}
			else
			{
				//lprintf( WIDE("This 'remote' service is really local...") );
				(*result_length) = INVALID_INDEX;
				//HandleServerMessageEx( (PMSGHDR)(params - 2) DBG_SRC );
				return TRUE;

			}
		}

	}
	// do send back a responce now - it's all bad.
	(*result_length) = 0;
	return FALSE;
}

static int CPROC MY_CLIENT_REGISTER_SERVICE( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
// add this to the list of known services... there's much data
// within this packet....  no... just a name I guess
	(*result_length) = 0;
	{
	// message[1] is the PID of the process registering...
	// this should be the responce ID to people loading this service.
	// message[2] is the real PID of the service process
	// message + 3 is a text string identifier for this service...
	// int  value register actually takes a PTRSZVAL which should be
	// at least _32 to match the process identifier....
		PSERVICE service;
 		for( service = g.services; service; service = service->next )
		{
			if( StrCmp( service->name, (TEXTCHAR*)params ) == 0 )
				break;
		}

		if( service )
		{
			//_32 Responce;
			// service has already been registered...
			// maybe we could inquire of the previous registered
			// person and see if they still exist?
			// for now - respond failure .
			if( ProbeClientAlive( &service->client_id ) )
			{

				return FALSE;
			}
			// otherwise the prior service is dead, and that's OK.
			DeleteAService( service );
			// release, and then recreate the service.
			service = NULL;
		}
		if( !service )
		{
			service = (PSERVICE)Allocate( sizeof( SERVICE ) );
			MemSet( service, 0, sizeof( service ) );
			if( route->dest.process_id == 1 )
			{
				service->flags.bRemote = 0;
			}
			else
			{
				service->flags.bRemote = 1;
			}
			service->client_id.service_id = 
				((MSGIDTYPE*)result)[0] = g.nService++;
			(*result_length) = sizeof( MSGIDTYPE );
			// route is already reversed for return-to-sender.
			service->client_id.process_id = route->dest.process_id;
			service->name = StrDup( (TEXTCHAR*)(params) );
			// link into the global list of services available;
			LinkThing( g.services, service );
			return TRUE;
		}
	}
	return FALSE;
}
static int CPROC MY_IM_ALIVE( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
	(*result_length) = INVALID_INDEX;
	return FALSE;
}
static int CPROC MY_RU_ALIVE( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
	(*result_length) = 0;
	return TRUE;
}
static int CPROC MY_CLIENT_DISCONNECT( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
	(*result_length) = INVALID_INDEX;
	return FALSE;
}
static int CPROC MY_CLIENT_UNLOAD_SERVICE( PSERVICE_ROUTE route, _32 *params, size_t param_length
	 , _32 *result, size_t *result_length )
{
	(*result_length) = INVALID_INDEX;
	return FALSE;
}

static int CPROC MY_CLIENT_LIST_SERVICES( PSERVICE_ROUTE route, _32 *params, size_t param_length
													 , _32 *result, size_t *result_length )
{
	PREFIX_PACKED struct msg {
		PLIST *list;
		int *bDone;
		POINTER thread;
	} PACKED *msg_data = (struct msg*)params;
	PSERVICE service;
	lprintf( WIDE("Received list service message...") );
	for( service = g.services; service; service = service->next )
	{
		SendMultiServiceEvent( route, MSG_SERVICE_DATA
									, 3
									, &msg_data->list, sizeof( msg_data->list )
									, &service->client_id, sizeof( service->client_id )
									, service->name, StrLen( service->name) + 1
									);
	}
	SendMultiServiceEvent( route, MSG_SERVICE_NOMORE, 1
								, &msg_data->bDone
								, sizeof( msg_data->bDone )
								+ sizeof( msg_data->thread )
								);
	(*result_length) = INVALID_INDEX;
	return FALSE;
}

#define NUM_FUNCTIONS ( sizeof( MasterServiceTable ) / sizeof( SERVER_FUNCTION ) )
static SERVER_FUNCTION MasterServiceTable[] =
{
	ServerFunctionEntry( NULL ) //	INVALID_MESSAGE
, ServerFunctionEntry( MY_CLIENT_LOAD_SERVICE )
, ServerFunctionEntry( MY_CLIENT_UNLOAD_SERVICE ) // client no longer needs a service (unload msgbase)
, ServerFunctionEntry( MY_CLIENT_CONNECT )		 // new client wants to connect
, ServerFunctionEntry( MY_CLIENT_DISCONNECT )	 // client disconnects (no responce)
, ServerFunctionEntry( MY_RU_ALIVE )				 // server/client message to other requesting status
, ServerFunctionEntry( MY_IM_ALIVE )				 // server/client message to other responding status
, ServerFunctionEntry( MY_CLIENT_REGISTER_SERVICE ) // client register service (name, serivces, callback table.)
// this comes back on the event channel...
, ServerFunctionEntry( MY_CLIENT_LIST_SERVICES ) // client register service (name, serivces, callback table.)
,
};

PRIORITY_PRELOAD( RegisterMasterService, MESSAGE_SERVICE_PRELOAD_PRIORITY )
{
	// register service returns 0 on success, and non zero base message id otherwise.
	// register with name of NULL causes setmasterserver...
	// this is invoked via LoadFunction( msg.core.service )
	RegisterService( NULL, MasterServiceTable, NUM_FUNCTIONS );
	g.nClient = 100;
   g.nService = 200;
}

PUBLIC( void, ExportASymboleSoExportLibraryGetsBuilt )( void )
{
	lprintf( WIDE("Don't call me, I don't do anything.") );
}

//---------------------------------------------------------------------
// $Log: server2.c,v $
// Revision 1.3  2005/06/30 13:12:08  d3x0r
// Changes to support making the master message server a library.  The msgsvr then becomes a stupid program that loads the message server library.  This library can be loaded bu summoner instead.
//
// Revision 1.2  2005/06/05 05:27:38  d3x0r
// Add default icon handler for windows to give us an exit method.  Also build default mode windows
//
// Revision 1.1  2005/05/25 17:05:12  d3x0r
// Initial commit
