#define DEFINE_MESSAGE_SERVER_GLOBAL
#include "global.h"

MSGCLIENT_NAMESPACE

//--------------------------------------------------------------------

PRIORITY_PRELOAD( LoadMsgClientGlobal, MESSAGE_CLIENT_PRELOAD_PRIORITY )
{
#ifndef __STATIC_GLOBALS__
	SimpleRegisterAndCreateGlobal( global_msgclient );
#else
	static struct global_message_service_tag global_msgclient__;
	global_msgclient = &global_msgclient__;
#endif
	InitializeCriticalSec( &g.csMsgTransact );
	InitializeCriticalSec( &g.csLoading );
	//InitializeCriticalSec( &g._handler.csMsgTransact );
}

//--------------------------------------------------------------------

static void EndClient( PSERVICE_CLIENT client )
{
	// call the service termination function.
	PCLIENT_SERVICE service;
	INDEX idx;
	// for all services inform it that the client is defunct.
	Log( "Ending client (from death)" );
	if( client->flags.is_service )
	{
		lprintf( "Service is gone! please tell client..." );
		if( client->handler->Handler )
			client->handler->Handler( MSG_MateEnded, NULL, 0 ); //-V595
		if( client->handler->HandlerEx )
			client->handler->HandlerEx( &client->handler->RouteID, MSG_MateEnded, NULL, 0 );
		if( client->handler->HandlerExx )
			client->handler->HandlerExx( client->handler->psv, &client->handler->RouteID, MSG_MateEnded, NULL, 0 );
		UnlinkThing( client->handler );
		Release( client->handler );
	}
	else
	{
		LIST_FORALL( client->services, idx, PCLIENT_SERVICE, service )
		{
			//lprintf( "Client had service... %p", service );
			if( service->handler )
			{
				service->handler( &client->route, MSG_ServiceUnload
									 , NULL, 0
									 , NULL, NULL );
			}
			else if( service->handler_ex )
			{
				service->handler_ex( service->psv
										 , &client->route, MSG_ServiceUnload
										 , NULL, 0
										 , NULL, NULL );
			}
			else if( service->functions && service->functions[MSG_ServiceUnload].function )
			{
				uint32_t resultbuf[1];
				size_t resultlen = 4;
				service->functions[MSG_ServiceUnload].function( &client->route, NULL, 0
																			 , resultbuf, &resultlen );
			}
			//Log( "Ending service on client..." );
			// Use ungloadservice to signal server services of client loss...
			//if( !UnloadService( service->first_message_index, client->pid ) )
			//	Log( "Somehow unloading a known service failed..." );
		}
		DeleteList( &client->services );
		{
			static uint32_t msg[2048];
			int32_t len;
			//lprintf( "vvv" );
			while( (len=msgrcv( g.msgq_out, MSGTYPE msg, 8192, client->route.source.process_id, IPC_NOWAIT )) >= 0 )
				//	errno != ENOMSG )
				lprintf( "Flushed a message to dead client(%" _MsgID_f ",%" _32f ") from output (%08" _32fx ":%" _32fs " bytes)"
						 , client->route.source.process_id
						 , msg[0]
						 , msg[1]
						 , len );
			//lprintf( "^^^" );
			//lprintf( "vvv" );
			while( msgrcv( g.msgq_event, MSGTYPE msg, 8192, client->route.source.process_id, IPC_NOWAIT ) >= 0 ||
					GetLastError() != ENOMSG )
				Log( "Flushed a message to dead client from event" );
			//lprintf( "^^^" );
		}
	}
	// delete the client.
	Log( "Finally unlinking the client..." );
	UnlinkThing( client );
	Release( client );
}

//--------------------------------------------------------------------

void CPROC MonitorClientActive( uintptr_t psv )
{
	// for all clients connected send an alive probe to see
	// if we can free their resources.
	PSERVICE_CLIENT client, next = g.clients;
	//const char *pFile = __FILE__;
	//int nLine = __LINE__;
#ifdef DEBUG_RU_ALIVE_CHECK
	Log( "Checking client alive" );
#endif
	// if am handling a message don't check alivenmess..
	if( !g.flags.handling_client_message )
	{
		while( ( client = next ) )
		{
			next = client->next;
#ifdef DEBUG_RU_ALIVE_CHECK
			lprintf( "Client %d(%p) last received %d ms ago", client->pid, client, timeGetTime() - client->last_time_received );
#endif
			if( ( client->last_time_received + CLIENT_TIMEOUT ) < timeGetTime() )
			{
				Log( "Client has been silent +" STRSYM(CLIENT_TIMEOUT) "ms - he's dead. (maybe he unloaded and we forgot to forget him?!)" );
				EndClient( client );
			}
			else if( ( client->last_time_received + (CLIENT_TIMEOUT/2) ) <  timeGetTime() )
			{
				if( !client->flags.status_queried )
				{
					QMSG msg;
					//uint32_t msg[3];
#ifdef DEBUG_DATA_XFER
					DBG_VARSRC;
#endif

					//Log1( "Asking if client %d is alive", client->pid );
					msg.dest.process_id            = client->route.dest.process_id;
					msg.dest.service_id            = client->route.dest.service_id;
					msg.hdr.source.process_id = client->route.source.process_id;
					msg.hdr.source.service_id = client->route.source.service_id;
					msg.hdr.msgid             = RU_ALIVE;
					client->flags.status_queried = 1;
#ifdef DEBUG_RU_ALIVE_CHECK
					lprintf( "Ask RU_ALIVE..." );
#endif
					msgsnd( g.msgq_in, MSGTYPE &msg, sizeof( msg ) - sizeof( MSGIDTYPE ), 0 );
				}
				else
				{
#ifdef DEBUG_RU_ALIVE_CHECK
					lprintf( "client has been queried for alivness..." );
#endif
				}
			}
			else
			{
				// hmm maybe a shorter timeout can happen...
			}
		}
	}
#if 0
	{
		PTRANSACTIONHANDLER pHandler, pNextHandler = g.pTransactions;
		while( pHandler = pNextHandler )
		{
			lprintf( "Check event handler (Service) %p", pHandler );
			pNextHandler = pHandler->next;
			{
				uint32_t tick;
				if( ( pHandler->last_check_tick + (CLIENT_TIMEOUT) ) < ( tick = timeGetTime() ) )
				{
					pHandler->last_check_tick = tick;
					if( !ProbeClientAlive( pHandler->ServiceID ) )
					{
						lprintf( "Service is gone! please tell client..." );
						if( pHandler->Handler )
							pHandler->Handler( MSG_MateEnded, NULL, 0 );
						if( pHandler->HandlerEx )
							pHandler->HandlerEx( pHandler->ServiceID, MSG_MateEnded, NULL, 0 );
						if( pHandler->HandlerExx )
							pHandler->HandlerExx( pHandler->psv, pHandler->ServiceID, MSG_MateEnded, NULL, 0 );
						UnlinkThing( pHandler );
						Release( pHandler );
						// generate disconnect message to client(myself)
						continue;
					}
					else
						lprintf( "Service is okay..." );
				}
			}
		}
	}
#endif

}

//--------------------------------------------------------------------

void ResumeThreads( void )
{
#ifndef _WIN32
	uint32_t tick;
	if( g.pThread )
	{
#ifdef DEBUG_SERVICE_INPUT
		lprintf( "Resume Service" );
#endif
		tick = timeGetTime();
		g.pending = 1;
		pthread_kill( ( GetThreadHandle( g.pThread ) ), SIGUSR2 );
		while( ((tick+10)>timeGetTime()) && g.pending ) Relinquish();
	}
	if( g.pMessageThread )
	{
#ifdef DEBUG_SERVICE_INPUT
		lprintf( "Resume Responce" );
#endif
		tick = timeGetTime();
		g.pending = 1;
		pthread_kill( ( GetThreadHandle( g.pMessageThread ) ), SIGUSR2 );
		while( ((tick+10)>timeGetTime()) && g.pending ) Relinquish();
	}

	if( g.pEventThread )
	{
#ifdef DEBUG_SERVICE_INPUT
		lprintf( "Resume event" );
#endif
		tick = timeGetTime();
		g.pending = 1;
		pthread_kill( ( GetThreadHandle( g.pEventThread ) ), SIGUSR2 );
		while( ((tick+10)>timeGetTime()) && g.pending ) Relinquish();
	}
	if( g.pLocalEventThread )
	{
#ifdef DEBUG_SERVICE_INPUT
		lprintf( "Resume local event" );
#endif
		tick = timeGetTime();
		g.pending = 1;
		pthread_kill( ( GetThreadHandle( g.pLocalEventThread ) ), SIGUSR2 );
		while( ((tick+10)>timeGetTime()) && g.pending ) Relinquish();
	}
#else
	if( g.pThread )
		WakeThread( g.pThread );
	if( g.pMessageThread )
		WakeThread( g.pMessageThread );
	if( g.pEventThread )
		WakeThread( g.pEventThread );
	if( g.pLocalEventThread )
		WakeThread( g.pLocalEventThread );
#endif
}

//--------------------------------------------------------------------

void RegisterWithMasterService( void )
{
	MSGIDTYPE ServiceID;

	if( g.flags.bMasterServer )
	{
		// I'm already done here....
		// I am what I am, and that's it.
		g.flags.connected = 1;
		g.flags.found_server = 1;
	}

	if( !g.flags.connected )
	{
		MSGIDTYPE Result;
		// result is a message ID that will be my SourceID
		size_t msglen = sizeof( ServiceID );

		/*
		{
			// create service 0 if it doesn't exist.
			PCLIENT_SERVICE service;
			for( service = g.services; service; service = service->next )
			{
				// process_id is already matched at this point, or we wouln't have the message
				// just have to give it to the local service.
				if( service->ServiceID == 0 )
				{
					//lprintf( "Found the service...%s", service->name );
					break;
				}
			}
			if( !service )
			{
				service = New( CLIENT_SERVICE );
				service->entries = 0;
				service->name = "Core Service";
				//service->
			}
		}
		*/
		//lprintf( "Connecting first time to service server...%"_MsgID_f ",%" _MsgID_f, g.master_service_route.dest.process_id, g.master_service_route.dest.service_id );
		if( !TransactServerMultiMessageExEx(DBG_VOIDSRC)( &g.master_service_route, CLIENT_CONNECT, 0
													, &Result, &ServiceID, &msglen
													, 100
													)
		  || Result != (CLIENT_CONNECT|SERVER_SUCCESS) )
		{
			Log( "Failed CLIENT_CONNECT" );
			//g.flags.failed = TRUE;
			// I see no purpose for this other than troubleshooting
			// RegisterWithMaster is called well after this should
			// have been set...
			//g.flags.message_responce_handler_ready = TRUE;
			return;
		}
		else
		{
			// result and pid in message received from server are trashed...
			//g.master_server_pid = msg[0];
			// modify my_message_id - this is now the
			// ID of messages which will be sent
			// and where repsonces will be returned
		// this therefore means that
#ifdef DEBUG_SERVICE_INPUT
			lprintf( "Initial service contact success" );
#endif
			g.flags.found_server = 1;
			g.flags.connected = 1;
			//g.my_message_id = msg[0];

			//lprintf( "Have changed my_message_id and now we need to wake all receivers..." );
			// this causes them to re-queue their requests with the new
			// flag... although the windows implementation passes the address
			// of this variable, so next message will wake this thread, however,
			// if the message was already posted, still will have to wake them
			// so they can scan for new-ready.
			ResumeThreads();
		}
	}
}

//--------------------------------------------------------------------

static MSGQ_TYPE OpenQueueEx( CTEXTSTR name, int key, int flags DBG_PASS )
#ifdef _WIN32
#define OpenQueue(n,k,f) OpenQueueEx(n,0,f DBG_SRC)
#else
#define OpenQueue(n,k,f)  OpenQueueEx( n,k,f DBG_SRC )
#endif
{
	static TEXTCHAR errbuf[256];
	MSGQ_TYPE queue;
	if( g.flags.bMasterServer )
	{
		queue = msgget( name, key, IPC_CREAT|IPC_EXCL|0666 );
		if( queue == MSGFAIL )
		{
			//strerror_s(errbuf, sizeof( errbuf ), errno);
         errbuf[0] = 0;
			lprintf( "Failed to create message Q for \"%s\":%s for" DBG_FILELINEFMT, name
				, errbuf
				DBG_RELAY );
			queue = msgget( name, key, 0 );
			if( queue == MSGFAIL )
			{

				//perror( "Failed to open message Q" );
			}
			else
			{
				lprintf( "Removing message queue id for %s", name );
				msgctl( queue, IPC_RMID, NULL );
				queue = msgget( name, key, IPC_CREAT|IPC_EXCL|0666 );
				if( queue == MSGFAIL )
				{
					//strerror_s(errbuf, sizeof( errbuf ), errno);
					errbuf[0] = 0;
					lprintf( "Failed to open message Q for \"%s\":%s", name
						, errbuf
						);
				}
			}
		}
	}
	else
	{
		queue = msgget( name, key, flags|0666 );
		if( queue == MSGFAIL )
		{
#ifdef DEBUG_MSGQ_OPEN
			//strerror_s(errbuf, sizeof( errbuf ), errno);
         errbuf[0] = 0;
			lprintf( "Failed to create message Q for \"%s\":%s for " DBG_FILELINEFMT, name
				, errbuf
				DBG_RELAY );
#endif
			//lprintf( "Failed to open message Q for \"%s\":%s", name, strerror(errno) );
		}
	}
	return queue;
}

//--------------------------------------------------------------------

#ifndef WIN32
static void ResumeSignal( int signal )
{
	//lprintf( "Got a resume signal.... resuming uhmm some thread." );
	//lprintf( "Uhmm and then pending should be 0?" );
	g.pending = 0;
}
#endif


int _InitMessageService( int local )
{
#ifdef __LINUX__
	key_t key, key2, key3, key4;
	signal( SIGUSR2, ResumeSignal ); // ignore this ...
#endif

	// key and key2 are reversed from the server - so my out is his in
	// and his inis my out.
	// we do funny things here since we switch in/out vs server.
#ifdef __LINUX__
	key = *(long*)MSGQ_ID_BASE; // server input, client output
	key2 = key + 1;  // server output, client input
	key3 = key + 2;  // pid-addressed events (all ways)
	key4 = key + 3;  // pid-addressed events (all ways)
#endif
	// until connected, our message handler ID
	// is my pid.  Then, once connected, we listen
	// for messages with the ID which was granted by the message
	// service.
	// maybe this could be declared to be '2'
	// and then before the client-connect is done, attempt
	// to send to process 2... if a responce is given, someone
	// else is currently registering with the message server
	// and we need to wait.

	// not failed... attempting to re-connect
	g.flags.failed = 0;
	if( !local )
	{
		if( g.flags.disconnected )
		{
			lprintf( "Previously we had closed all communication... allowing re-open." );
			g.flags.disconnected = 0;
			g.my_message_id = 0; // reset this... so we re-request for new path...
		}
	}

	if( !g.flags.bAliveThreadStarted )
	{
		// this timer monitors ALL clients for inactivity
		// it will probe them with RU_ALIVE messages
		// to which they must respond otherwise be termintated.
		g.flags.bAliveThreadStarted = 1;
		AddTimer( CLIENT_TIMEOUT/4, MonitorClientActive, 0 );
		// each service gets 1 thread to handle their own
		// messages... services do not have 'events' generated
		// to them.
	}

	if( !g.my_message_id )
	{
#ifdef __LINUX__
		g.my_message_id = getpid(); //pService->thread->ThreadID & 0x7FFFFFFFUL; /*(uint32_t)getpid()*/;
#else
		g.my_message_id = GetCurrentProcessId();
#endif
	}

	g.master_service_route.dest.process_id = 1;
	g.master_service_route.dest.service_id = 0;
	g.master_service_route.source.process_id = g.my_message_id;
	g.master_service_route.source.service_id = 0;

	if( !local && !g.msgq_in && !g.flags.message_responce_handler_ready )
	{
#ifdef DEBUG_MSGQ_OPEN
		lprintf( "opening message queue? %d %d %d"
				 , local, g.msgq_in, g.flags.message_responce_handler_ready );
#endif
		g.msgq_in = OpenQueue( MSGQ_ID_BASE "1", key2, 0 );
#ifdef DEBUG_MSGQ_OPEN
		lprintf( "Result msgq_in = %ld", g.msgq_in );
#endif
		if( g.msgq_in == MSGFAIL )
		{
			g.msgq_in = 0;
			return FALSE;
		}
#ifdef DEBUG_THREADS
		lprintf( "Creating thread to handle responces..." );
#endif
		AddIdleProc( ProcessClientMessages, 0 );
		ThreadTo( HandleMessages, g.my_message_id );
		while( !g.flags.message_responce_handler_ready )
			Relinquish();
	}
	if( !local && !g.msgq_out && !g.flags.message_handler_ready )
	{
		g.msgq_out = OpenQueue( MSGQ_ID_BASE "0", key, 0 );
		if( g.msgq_out == MSGFAIL )
		{
			g.msgq_out = 0;
			return FALSE;
		}
		// just allow this thread to be created later...
		// need to open the Queue... but that's about it.
#ifdef DEBUG_THREADS
		//lprintf( "Creating thread to handle messages..." );
		// thread is now created in RegisterService portion
#endif
	}
	if( !local && !g.msgq_event && !g.flags.events_ready )
	{
		g.msgq_event = OpenQueue( MSGQ_ID_BASE "2", key3, 0 );
		if( g.msgq_event == MSGFAIL )
		{
			g.msgq_event = 0;
			return FALSE;
		}
#ifdef DEBUG_THREADS
		lprintf( "Creating thread to handle events..." );
#endif
		ThreadTo( HandleEventMessages, 0 );
		while( !g.flags.events_ready )
			Relinquish();
	}
	if( local && !g.msgq_local && !g.flags.local_events_ready )
	{
	// huh - how can I open this shm under linux?
		g.msgq_local = OpenQueue( NULL, key4, IPC_CREAT );
		if( g.msgq_local == MSGFAIL )
		{
			g.msgq_local = 0;
			return FALSE;
		}
#ifdef DEBUG_THREADS
		lprintf( "Creating thread to handle local events..." );
#endif
		AddIdleProc( ProcessClientMessages, 0 );
		ThreadTo( HandleLocalEventMessages, 0 );
		while( !g.flags.local_events_ready )
			Relinquish();
	}

	// right now our PID is the message ID
	// and after this we will have our correct message ID...
	g._handler.RouteID.dest.process_id = 1;
	g._handler.RouteID.dest.service_id = 0;

	if( g.flags.failed )
	{
		//g.flags.initialized = FALSE;
		return -1;
	}


	return 1;
}

//--------------------------------------------------------------------

void CloseMessageQueues( void )
{
	g.flags.disconnected = TRUE;
	if( g.flags.bMasterServer )
	{
		// master server closing, removes all id's
		// removing the message queue should be enough
		// to wake each of these threads...
		if( g.msgq_in != MSGFAIL )
		{
			msgctl( g.msgq_in, IPC_RMID, NULL );
			g.msgq_in = 0;
		}
		if( g.msgq_out != MSGFAIL )
		{
			msgctl( g.msgq_out, IPC_RMID, NULL );
			g.msgq_out = 0;
		}
		if( g.msgq_event != MSGFAIL )
		{
			msgctl( g.msgq_event, IPC_RMID, NULL );
			g.msgq_event = 0;
		}
		if( g.msgq_local != MSGFAIL )
		{
			msgctl( g.msgq_local, IPC_RMID, NULL );
			g.msgq_local = 0;
		}
	}
	else
	{
		// we have to wake up everyone, so they can realize we're disconnected
		// and leave...
		// then we wait some short time for everyone to exit...
		ResumeThreads();
	}
	{
		uint32_t attempts = 0;
		uint32_t time;
		g.msgq_in = 0;
		g.msgq_out = 0;
		g.msgq_event = 0;
		g.msgq_local = 0;
		g.my_message_id = 0;
		do
		{
			time = timeGetTime();
			while( ((time+100)>timeGetTime()) && g.pThread ) Relinquish();
			time = timeGetTime();
			while( ((time+100)>timeGetTime()) && g.pMessageThread ) Relinquish();
			time = timeGetTime();
			while( ((time+100)>timeGetTime()) && g.pEventThread ) Relinquish();
			time = timeGetTime();
			while( ((time+100)>timeGetTime()) && g.pLocalEventThread ) Relinquish();
			if( g.pThread || g.pMessageThread || g.pEventThread || g.pLocalEventThread )
			{
				attempts++;
				lprintf( "Threads are not exiting... %" _32f " times", attempts );
				if( attempts < 10 )
					continue; // skips
			}
			break;
		} while( 1 );
	}
	// re-establish our communication ID if we
	// end up with more work to do...
	g.flags.connected = 0;
}

//--------------------------------------------------------------------

static void DisconnectClient(void)
{
	static int bDone;
	PEVENTHANDLER pHandler;
	if( !global_msgclient || bDone )
		return;
	bDone = 1;

	//lprintf( "Disconnect all clients... %Lx", GetMyThreadID() );
	while( ( pHandler = g.pHandlers ) )
	{
		//lprintf( "Unloading a service..." );
		UnloadService( pHandler->servicename );
	}
	//lprintf( "Okay all registered services are gone." );
	// no real purpose in this....
	// well perhaps... but eh...
	// if( !master server )
	//SendServerMessage( CLIENT_DISCONNECT, NULL, 0 );
	CloseMessageQueues();

}

PRIORITY_ATEXIT( _DisconnectClient, ATEXIT_PRIORITY_MSGCLIENT )
{
	DisconnectClient();
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( int, ProbeClientAlive )( PSERVICE_ENDPOINT RouteID )
{
	MSGIDTYPE Responce;
	SERVICE_ROUTE ping_route;
	if( RouteID->process_id == g.my_message_id )
	{
		lprintf( "Yes, I, myself, am alive..." );
		return TRUE;
	}
	//lprintf( "Hmm is client %p" " alive?", RouteID );
	{
		PEVENTHANDLER handler;
		for( handler = g.pHandlers; handler; handler = handler->next )
			if( ( handler->RouteID.dest.process_id == RouteID->process_id )
			  && ( handler->RouteID.dest.service_id == RouteID->service_id ) )
				break;
		if( !handler )
		{
			// has a nul name.
			handler = New( EVENTHANDLER );
			MemSet( handler, 0, sizeof( EVENTHANDLER ) );
			//InitializeCriticalSec( &handler->csMsgTransact );
			handler->RouteID.dest = RouteID[0];
			LinkThing( g.pHandlers, handler );
			//lprintf( "Created a HANDLER to coordinate probe alive request.." );
		}
	}
	ping_route.dest.process_id = RouteID->process_id;
	ping_route.dest.service_id = 0; // PING goes to service 0 (internals)
	ping_route.source.process_id = g.my_message_id;
	ping_route.source.service_id = 0;

	if( TransactRoutedServerMultiMessageEx( &ping_route, RU_ALIVE, 0
													  , &Responce, NULL, NULL
#ifdef DEBUG_DATA_XFER
														// if logging data xfer - we need more time
													  , 250
#else
													  , 250 // 10 millisecond timeout... should be more than generous.
#endif
													  , NULL, NULL ) &&
		Responce == ( IM_ALIVE ) )
	{
		//lprintf( "Ping Success." );
		return TRUE;
	}
	//lprintf( "Ping Failure." );
	return FALSE;
}



//-------------------------------------------------------------

CLIENTMSG_PROC( int, SendOutMessageEx )( PQMSG buffer, size_t len DBG_PASS )
{
#ifdef DEBUG_MESSAGE_BASE_ID
	//DBG_VARSRC;
#endif
	int stat;
	if( ( stat = msgsnd( g.msgq_out, MSGTYPE (buffer), len, 0 ) ) < 0 )
	{
		TEXTCHAR msg[256];
		//strerror_s(errbuf, sizeof( errbuf ), errno);
		msg[0] = 0;
		lprintf( "Error sending message: %s"
			, msg );
	}
	return stat;
}
CLIENTMSG_PROC( int, SendOutMessage )( PQMSG buffer, size_t len )
{
	return SendOutMessageEx(buffer,len DBG_SRC);
}

//-------------------------------------------------------------

CLIENTMSG_PROC( void, SetMasterServer )( void )
{
	g.flags.bMasterServer = 1;
}

//-------------------------------------------------------------

CLIENTMSG_PROC( void, DumpServiceList )(void )
{
	PLIST list = NULL;
	int bDone = 0;
	PREFIX_PACKED struct {
		PLIST *ppList;
		int *pbDone;
		PTHREAD me;
	} PACKED mydata;
	mydata.ppList = &list;
	mydata.pbDone = &bDone;
	if( !_InitMessageService( FALSE ) )
	{
		lprintf( "Initization of public message participation failed, cannot query service master" );
		return;
	}
	RegisterWithMasterService();
	mydata.me = MakeThread();
	lprintf( "Sending message to server ... list services..." );
	LogBinary( (uint8_t*)&mydata, sizeof(mydata) );
	SendServerMessage( &g.master_service_route, CLIENT_LIST_SERVICES, &mydata, sizeof(mydata) );
	// wait for end of list...
	while( !bDone )
	{
		WakeableSleep( 5000 );
		if( !bDone )
		{
			lprintf( "Treading water, but I think I'm stuck here forever..." );
		}
	}
	{
		INDEX idx;
		char *service;
		LIST_FORALL( list, idx, char *, service )
		{
			// ID of service MAY be available... but is not yet through
			// thsi interface...
			lprintf( "Available Service: %s", service );
			Release( service );
		}
		lprintf( "End of service list." );
		DeleteList( &list );
	}
}

//-------------------------------------------------------------

CLIENTMSG_PROC( void, GetServiceList )( PLIST *list )
{
	int bDone = 0;
	PREFIX_PACKED struct {
		PLIST *ppList;
		int *pbDone;
		PTHREAD me;
	} PACKED mydata;
	mydata.ppList = list;
	mydata.pbDone = &bDone;
	if( !_InitMessageService( FALSE ) )
	{
		lprintf( "Initization of public message participation failed, cannot query service master" );
		return;
	}
	RegisterWithMasterService();
	mydata.me = MakeThread();
	lprintf( "Sending message to server ... list services..." );
	SendServerMessage( &g.master_service_route, CLIENT_LIST_SERVICES, &mydata, sizeof(mydata) );
	// wait for end of list...
	while( !bDone )
	{
		WakeableSleep( 5000 );
		if( !bDone )
		{
			lprintf( "Treading water, but I think I'm stuck here forever..." );
		}
	}
	/*
	{
		INDEX idx;
		char *service;
		LIST_FORALL( list, idx, char *, service )
		{
			// ID of service MAY be available... but is not yet through
			// thsi interface...
			lprintf( "Available Service: %s", service );
		}
		lprintf( "End of service list." );
		}
		*/
}

CLIENTMSG_PROC( LOGICAL, IsSameMsgEndPoint )( PSERVICE_ENDPOINT a, PSERVICE_ENDPOINT b )
{
   return ( a->process_id == b->process_id ) && ( a->service_id == b->service_id );
}

CLIENTMSG_PROC( LOGICAL, IsSameMsgSource )( PSERVICE_ROUTE a, PSERVICE_ROUTE b )
{
	return IsSameMsgEndPoint( &a->source, &b->source );
}

CLIENTMSG_PROC( LOGICAL, IsSameMsgDest )( PSERVICE_ROUTE a, PSERVICE_ROUTE b )
{
	return IsSameMsgEndPoint( &a->dest, &b->dest );
}

CLIENTMSG_PROC( LOGICAL, IsMsgSourceSameAsMsgDest )( PSERVICE_ROUTE a, PSERVICE_ROUTE b )
{
	return IsSameMsgEndPoint( &a->source, &b->dest );
}

void DropMessageBuffer( PQMSG msg )
{
	EnqueLink( &g.Messages, msg );
}

PQMSG GetMessageBuffer( void )
{
	PQMSG msg = (PQMSG)DequeLink( &g.Messages );
	if( !msg )
	{
		msg = NewPlus( QMSG, 8192 );
		return msg;
	}
	return msg;
}


MSGCLIENT_NAMESPACE_END

//-------------------------------------------------------------
