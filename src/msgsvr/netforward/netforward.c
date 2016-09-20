
#include <network.h>
#include <netservice.h>
#include <sharemem.h>
#include <timers.h>
#include <msgclient.h>
#include <configscript.h>






typedef struct {
	SOCKADDR *sockaddr;
	PCLIENT pc;
} SERVER;

typedef SERVER *PSERVER;

struct service_served {
	INDEX service_idx;
   uint32_t client_registered_id;
};

struct wait_struct {
	struct {
		BIT_FIELD bWaitComplete : 1;
	} flags;
	PCLIENT pc;
	PTHREAD thread; // message_receiption thread...
	uint32_t SourceRouteID;
   int status;
	uint32_t *result;
   uint32_t *result_length;
};

static struct {
	PLIST servers;
	PLIST services;
   PLIST waiting_service;
} l;



enum {
	NL_BUFFER
	  , NL_LENGTH
     , NL_SERVICE_ID
};

enum {
   MSG_VERSION
	  , MSG_SERVICE
	  , MSG_SERVICE_REGISTERED
	  , MSG_SERVICE_MESSAGE
	  , MSG_SERVICE_RESPONCE
     , MSG_SERVICE_LOAD
     , MSG_SERVICE_LOADED
     , MSG_SERVICE_UNLOAD
	  , MSG_SERVICE_EVENT
	  , MSG_SEND_IN
	  , MSG_SEND_OUT
	  , MSG_RESPOND_IN
	  , MSG_RESPOND_OUT
};


#pragma pack(0)
PREFIX_PACKED struct struct_msg {
	uint32_t length;
	uint32_t MsgId;
} PACKED;
PREFIX_PACKED struct struct_msg_r {
	// length member already consumed on receive side.
	uint32_t MsgId;
	// void _user structure here...
} PACKED;

/* anti-visual studio maneuver */
#define DeclareMsg( name, ... )   \
	PREFIX_PACKED struct struct_##name##_msg { \
	struct struct_msg;                            \
	__VA_ARGS__                           \
	} PACKED;                                 \
	PREFIX_PACKED struct struct_##name##_msg_r {  \
	struct struct_msg_r;                             \
            __VA_ARGS__                           \
	} PACKED;

DeclareMsg( version
			 , uint32_t version;
			 );
DeclareMsg( service
			 , INDEX idx;
				TEXTCHAR name[];
			 );
DeclareMsg( service_registered
			 , INDEX idx;
				uint32_t dwMsgBase;
			 );
DeclareMsg( service_message
			 , uint32_t MsgID;
				uint32_t responce;
				uint32_t result;
				uint32_t result_length;
				uint32_t actual_result_length;
            uint32_t param_length;
				uint32_t params[];
			 );

DeclareMsg( service_responce
			 , uint32_t status;
				uint32_t responce;
				uint32_t result_length;
			 );

DeclareMsg( service_load
			 , INDEX service; //service index
            // followed nul teriminated client_name
			 );

DeclareMsg( service_unload
			 , INDEX service; //service index
			   uint32_t client_id;
			 );

DeclareMsg( service_event
			 , uint32_t MsgID;
			 );

DeclareMsg( service_loaded
			 , uint32_t client_id;
			 );


#define EasySendTCP( sock, thing ) SendTCP( pc, &thing, sizeof( thing ) )

#pragma pack()

static int CPROC handle_any( uintptr_t psv
						  , uint32_t SourceRouteID, uint32_t MsgID
						  , uint32_t *params, uint32_t param_length
						  , uint32_t *result, uint32_t *result_length )
{
	PCLIENT pc = (PCLIENT)psv;
	switch( MsgID )
	{
	case MSG_ServiceUnload:
		{
			struct struct_service_unload_msg msg;
			msg.service = GetNetworkLong( pc, NL_SERVICE_ID );
         msg.MsgId = MSG_SERVICE_UNLOAD;
			msg.length = sizeof( msg );
			msg.client_id = params[0];
         msg.service = 0;
			EasySendTCP( pc, msg );
			if( result_length )
				(*result_length) = INVALID_INDEX;
         return TRUE;
		}
		break;
	case MSG_ServiceLoad:
		// new client... send the request to the real server...
		{
			struct struct_service_load_msg msg;
         CTEXTSTR service_name = (CTEXTSTR)params;
			msg.service = GetNetworkLong( pc, NL_SERVICE_ID );
         msg.MsgId = MSG_SERVICE_LOAD;
			msg.length = sizeof( msg ) + strlen( service_name ) + 1;
			EasySendTCP( pc, msg );
         SendTCP( pc, (POINTER)service_name, strlen( service_name ) + 1 );
		}
      break;
	default:
		{
			struct struct_service_message_msg msg;
			msg.length = sizeof( msg ) - sizeof( msg.length ) + param_length;
			msg.MsgId = MSG_SERVICE_MESSAGE;
			TellClientTardy( SourceRouteID, 3000 ); // gimme like 3 seconds, instead of quick timeout
			msg.MsgID = MsgID;

			if( result )
				msg.result = 1;
			else
				msg.result = 0;
			if( result_length )
				msg.result_length = 1;
			else
				msg.result_length = 0;
			msg.actual_result_length = result_length?sizeof( *result_length ):0;
			msg.param_length = param_length;
			EasySendTCP( pc, msg );
			SendTCP( pc, params, param_length );
		}
		break;
	}
	{
		struct wait_struct *wait;
		wait = New( struct wait_struct );
		wait->pc = pc;
		wait->thread = MakeThread();
		wait->SourceRouteID = SourceRouteID;
		wait->result = result;
		/* set result_length = INVALID_INDEX to send nothing. */
		wait->result_length = result_length;

		AddLink( &l.waiting_service, wait );
		wait->flags.bWaitComplete = 0;
		while( !wait->flags.bWaitComplete )
			WakeableSleep( SLEEP_FOREVER );
		{
			int status = wait->status;
			Release( wait );
			return status;
		}
	}
	return FALSE;
}

static void CPROC ClientClosed( PCLIENT pc )
{
	//UnregisterServiceHandlerEx( GetNetworkLong( pc, NL_SERVICE_ID ) );

}


static void CPROC ReceivedMessage( PCLIENT pc, POINTER buffer, int size )
{
   uintptr_t toread = 4;
	if( !buffer )
	{
      lprintf( "there is a maximal size in message service..." );
		buffer = NewArray( uint32_t, 1024 ); // there is a maximal size in message service...
		SetNetworkLong( pc, NL_BUFFER, (uintptr_t)buffer );
      SetNetworkLong( pc, NL_LENGTH, 0 );
	}
	else
	{

      toread = GetNetworkLong( pc, NL_LENGTH );
		if( !toread )
		{
			uint32_t length;
			// size == 4.
			length = ((uint32_t*)buffer)[0];
			toread = length;
         SetNetworkLong( pc, NL_LENGTH, toread = length );
		}
		else
		{
			//struct struct_msg msg;
			//struct struct_msg_r rmsg;
			/* what do we get sent? someone should send something... */
			switch( ((uint32_t*)buffer)[0] )
			{
			case MSG_SERVICE:
				{
					struct struct_service_msg_r *in = (struct struct_service_msg_r *)buffer;
               struct struct_service_registered_msg msg;
					msg.length = sizeof( msg ) - sizeof( msg.length );
					msg.dwMsgBase = RegisterServiceHandlerEx( in->name, handle_any, (uintptr_t)pc );
					msg.MsgId = MSG_SERVICE_REGISTERED;
               msg.idx = in->idx;
					SetTCPNoDelay( pc, TRUE );
					EasySendTCP( pc, msg );
				}
            break;
			case MSG_VERSION:
				// his version is ((uint32_t*)buffer)[1]
				{
               struct struct_version_msg msg;
					msg.length = sizeof( struct struct_version_msg_r );
					msg.MsgId = MSG_VERSION;
               msg.version = 0;
					SendTCP( pc, &msg, sizeof( msg ) );
				}
				break;
			case MSG_SERVICE_RESPONCE:
				// mark the service was started on the client side...
            // not so important to us yet...
				{
					struct struct_service_responce_msg_r *in =
						(struct struct_service_responce_msg_r *)buffer;
					INDEX idx;
					struct wait_struct *wait;
					LIST_FORALL( l.waiting_service, idx, struct wait_struct *, wait )
					{
						if( wait->pc == pc )
						{
							wait->status = in->status;
							if( wait->result )
								MemCpy( wait->result
										, in + 1, size - sizeof( *in ) );
							if( wait->result_length )
								(*wait->result_length) = size - sizeof( *in );
							wait->status = in->status;
                     wait->flags.bWaitComplete = 1;
							WakeThread( wait->thread );
							SetLink( &l.waiting_service, idx, NULL );
                     break;
						}
					}
				}
				break;
			case MSG_SERVICE_EVENT:
				{
					struct struct_service_event_msg_r *in = (struct struct_service_event_msg_r *)buffer;
               uint32_t client_id = 0; // need a valid clientID here...
               SendServiceEvent( client_id, in->MsgID, (uint32_t*)(in + 1), size - sizeof( *in ) );
				}
            break;
			}
		}
	}
   ReadTCPMsg( pc, buffer, (int)toread );
}

static int CPROC DiscoveredHandler( uintptr_t psv, SOCKADDR *responder ); // sorry forward declaration :(

static void CPROC NetworkClosed( PCLIENT pc )
{

	DiscoverService( TRUE, 5324, DiscoveredHandler, 0 );
}

static int CPROC DiscoveredHandler( uintptr_t psv, SOCKADDR *responder )
{
	PSERVER server;
	INDEX idx;
	LIST_FORALL( l.servers, idx, PSERVER, server )
	{
		if( CompareAddress( server->sockaddr, responder ) == 0 )
			break;
	}
	if( !server )
	{
		server = New( SERVER );

		server->pc = OpenTCPClientAddrEx( responder, ReceivedMessage, NetworkClosed, NULL );
		server->sockaddr = DuplicateAddress( responder ); // use network's allocer to dup.
		AddLink( &l.servers, server );
		// when can we do this?
		// cause again, what if we want to wait for a time until this closes?
		// collecting all available local broadcastable services... 
		EndDiscoverService( 5324 );
	}
	else
	{
		lprintf( "Client is already known, someone else asked? He's responding broadcasst? Or? I'm still asking... but maybe validate client?" );

	}

	return TRUE;
}

static void CPROC CloseClient( uintptr_t psv )
{
	RemoveClient( (PCLIENT)psv );
	//RemoveTimer( timer );
}

static int CPROC EventHandler( uintptr_t psv, uint32_t SourceID, uint32_t MsgID, uint32_t*params, uint32_t paramlen)
{
   PCLIENT pc = (PCLIENT)psv;
	//server has generated an event...
	struct struct_service_event_msg msg;
	msg.length = sizeof( struct struct_service_event_msg_r ) + paramlen;
	msg.MsgId = MSG_SERVICE_EVENT;
	msg.MsgID = MsgID;
	EasySendTCP( pc, msg );
	SendTCP( pc, params, paramlen );
   return 0; // don't need event_dispatch ever...
}



static void CPROC ServerRead( PCLIENT pc, POINTER buffer, int length )
{
   uintptr_t toread = 4;
	if( !buffer )
	{
      lprintf( "there is a maximal size in message service..." );
		buffer = NewArray( uint32_t, 1024 ); // there is a maximal size in message service...
		SetNetworkLong( pc, NL_BUFFER, (uintptr_t)buffer );
		SetNetworkLong( pc, NL_LENGTH, 0 );
		{
			struct struct_version_msg msg;
         struct struct_service_msg msg2;
			CTEXTSTR service;
			msg.length = sizeof( struct struct_version_msg_r );
			msg.MsgId = MSG_VERSION;
         msg.version = 0;
			SendTCP( pc, &msg, sizeof( msg ) );

			msg2.MsgId = MSG_SERVICE;
			LIST_FORALL( l.services, msg2.idx, CTEXTSTR, service )
			{
				int sendlen = strlen( service ) + 1; // send nul
				msg2.length = sizeof( struct struct_service_msg_r );
            // send what services I know.
				EasySendTCP( pc, msg2 );
            // should be a CPOINTER declaration...
				SendTCP( pc, (POINTER)service, sendlen );
			}

		}
	}
	else
	{

      toread = GetNetworkLong( pc, NL_LENGTH );
		if( !toread )
		{
			uint32_t length;
			// size == 4.
			length = ((uint32_t*)buffer)[0];
			toread = length;
         SetNetworkLong( pc, NL_LENGTH, toread = length );
		}
		else
		{
			/* what do we get sent? someone should send something... */
			/* what do we get sent? someone should send something... */
			switch( ((uint32_t*)buffer)[0] )
			{
			case MSG_SERVICE_REGISTERED:
				SetTCPNoDelay( pc, TRUE );
            //((uint32_t*)buffer)[1];
				break;
			case MSG_VERSION:
				/* client responded with his version, he thinks he can work with me...*/
            // his version is ((uint32_t*)buffer)[1]
				break;
			case MSG_SERVICE_LOAD:
				{
					struct struct_service_load_msg_r *in = (struct struct_service_load_msg_r *)buffer;
               struct struct_service_loaded_msg msg;
					msg.client_id = LoadServiceExx( (TEXTSTR)in + 1, EventHandler, (uintptr_t)pc );
					msg.length = sizeof( struct struct_service_loaded_msg_r );
					msg.MsgId = MSG_SERVICE_LOADED;
               
				};
            break;
			case MSG_SERVICE_UNLOAD:
				{
					struct struct_service_unload_msg_r *in = (struct struct_service_unload_msg_r *)buffer;
               UnloadService( in->client_id );
				}
				break;
			case MSG_SERVICE_MESSAGE:
				{
				struct struct_service_message_msg_r *in = (struct struct_service_message_msg_r *)buffer;
#pragma pack(1)
				static PREFIX_PACKED struct
				{
               struct struct_service_responce_msg msg;
					uint32_t tmpbuf[1024]; // largest buffer size?
				}PACKED msg;
#pragma pack()
				if( msg.msg.status = TransactServerMessage( in->MsgId, in->params, in->param_length
												 , in->responce?&msg.msg.responce:NULL
												 , in->result?msg.tmpbuf:NULL
												 , in->result_length?&in->result_length:NULL ) )
				{
					//struct struct_msg msg;
					msg.msg.length = sizeof( struct struct_service_responce_msg )
						+ (in->result_length==INVALID_INDEX)?0:in->result_length;
					msg.msg.MsgId = MSG_SERVICE_RESPONCE;
					EasySendTCP( pc, msg.msg );
               if( in->result_length != INVALID_INDEX )
						SendTCP( pc, msg.tmpbuf, in->result_length );
				}
				}
				break;
			}

		}
	}
   ReadTCPMsg( pc, buffer, (int)toread );
}

void CPROC connected( PCLIENT PcServer, PCLIENT pcNew )
{
	lprintf( "Accepted a client..." );
	RemoveClient( (PCLIENT)pcNew );
   SetNetworkReadComplete( pcNew, ServerRead );
	//timer = AddTimer( 0, CloseClient, (uintptr_t)pcNew );
}


//int main( void )
int respond( void )
{
	NetworkStart();
	// look for a service, when a service is found, specify the method
	// which is to be discovered.
	DiscoverService( TRUE, 5235, DiscoveredHandler, 0 );
	//EndDiscoverService( 5235 );
	
	OpenTCPListenerEx( 5235, connected ); // expected response to me claiming presence is... a tcp socket to me.
	ServiceRespond( 5235 );
	
	while( 1 )
		WakeableSleep( 1000 );
   return 1;
}

void ReadConfig( void )
{
	PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
	//AddConfigurationMethod( pch, "server %m", AddKnownServer );
   ProcessConfigurationFile( pch, "bag.msgsrvr.net.forward.server.config", 0 );
}


int main( int argc, char **argv )
{
	PLIST list = NULL;
	LoadComplete();
	GetServiceList( &list );
	{
		int n;
		for( n = 1; n < argc; n++ )
		{
			INDEX idx;
			CTEXTSTR service;
			LIST_FORALL( list, idx, CTEXTSTR, service )
			{
				if( StrCaseCmp( service, argv[n] ) == 0 )
				{
					AddLink( &l.services, argv[n] );
               break;
				}
			}
			if( !service )
			{
				fprintf( stderr, "Service: %s is not loaded\n", argv[n] );
			}
		}
	}
	if( l.services )
		respond(); // waits until told not to respond?





   return 0;
}
