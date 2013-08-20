
#include "global.h"

MSGCLIENT_NAMESPACE

//--------------------------------------------------------------------

PSERVICE_CLIENT FindClient( PSERVICE_ROUTE pid )
{
	PSERVICE_CLIENT client = g.clients;
	while( client )
	{
		if( ( client->route.dest.process_id == pid->dest.process_id )
         && ( client->route.dest.service_id == pid->dest.service_id ) )
		{
			break;
		}
		client = client->next;
	}
	return client;
}

//--------------------------------------------------------------------

static PSERVICE_CLIENT AddClient( PSERVICE_ROUTE pid )
{
	{
		PSERVICE_CLIENT client = FindClient( pid );
		if( client )
		{
			//Log( WIDE("Client has reconnected?!?!?!") );
			// reconnect is done when requesting a service from
			// a server that supplies one or more services itself...
			// suppose we can just let him continue...
			return client;
		}
	}

	{
		PSERVICE_CLIENT client = New( SERVICE_CLIENT );
		MemSet( client, 0, sizeof( SERVICE_CLIENT ) );
		client->route = pid[0];
		client->last_time_received = timeGetTime();
		client->flags.valid = 1;
		LinkThing( g.clients, client );
		g.clients = client;
		//Log( WIDE("Added client...") );
		return client;
	}
}


//--------------------------------------------------------------------

static PSERVICE_ROUTE _LoadService( CTEXTSTR service
							  , EventHandlerFunction EventHandler
							  , EventHandlerFunctionEx EventHandlerEx
							  , EventHandlerFunctionExx EventHandlerExx
							  , server_message_handler handler
                       , server_message_handler_ex handler_ex
							  , PTRSZVAL psv
							  )
{
	_32 MsgID;
	MsgSrv_ReplyServiceLoad msg;
	size_t MsgLen = sizeof( msg ); // expect MsgBase = 0, EventMessgaeCount = 1
	PEVENTHANDLER pHandler;

	// can check now if some other part of this has loaded
	// this service.
	// reset this status...

	if( !_InitMessageService( service?FALSE:TRUE ) )
	{
#ifdef DEBUG_MSGQ_OPEN
		lprintf( WIDE("Load of %s message service failed."), service );
#endif
		return NULL;
	}
	if( service )
	{
		RegisterWithMasterService();
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
#if 0
		// always query for service, don't short cut... ?
		while( pHandler )
		{
			// only one connection to any given service name
			// may be maintained.  The service itself is resulted...
			if( !strcmp( pHandler->servicename, service ) )
				return &pHandler->RouteID;
			pHandler = pHandler->next;
		}
#endif
		EnterCriticalSec( &g.csLoading );
		pHandler = New( EVENTHANDLER );
		MemSet( pHandler, 0, sizeof( EVENTHANDLER ) );
		//InitializeCriticalSec( &pHandler->csMsgTransact );
		pHandler->servicename = StrDup( service );

		pHandler->RouteID.dest.process_id = 1;
		pHandler->RouteID.dest.service_id = 0;
		pHandler->RouteID.source.process_id = g.my_message_id;
		pHandler->RouteID.source.service_id = 0;

		//lprintf( WIDE("Allocating local structure which manages our connection to this service...") );
 
		// MsgInfo is used both on the send and receives the
		// responce from the service...
		// LoadService goes to the msgsvr and requests the
		// location of the service.
		if( !TransactRoutedServerMultiMessageEx( &pHandler->RouteID
															, MSG_ServiceLoad, 1
															, &MsgID, &msg, &MsgLen
															, 250 /* short timeout */
															, service, (StrLen( service ) + 1) *sizeof(TEXTCHAR) // include NUL
															) )
		{
			Log( WIDE("Transact message timeout.") );
			Release( pHandler );
			LeaveCriticalSec( &g.csLoading );
			return NULL;
		}
		if( MsgID != (MSG_ServiceLoad|SERVER_SUCCESS) )
		{
			lprintf( WIDE("Server reports it failed to load [%s] (%08") _32fx WIDE("!=%08x)")
					 , service
					 , MsgID
					 , MSG_ServiceLoad|SERVER_SUCCESS );
			Release( pHandler );
			LeaveCriticalSec( &g.csLoading );
			return NULL;
		}
		// uncorrectable anymore.
		//if( MsgLen == 16 )
		//{
		//	lprintf( WIDE("Old server load service responce... lacks the PID of the event handler.") );
		//}
		if( MsgLen != sizeof( msg ) )
		{
			lprintf( WIDE("Server responce was the wrong length!!! %") _32f WIDE(" expecting %d"), MsgLen, sizeof( msg ) );
			Release( pHandler );
			LeaveCriticalSec( &g.csLoading );
			return NULL;
		}
	}
	else
	{
		// loading special NULL service.
		// the NULL service looks like a queue available
      // for events only?  fakes a server response in msg.
		pHandler = New( EVENTHANDLER );
		MemSet( pHandler, 0, sizeof( EVENTHANDLER ) );
		//InitializeCriticalSec( &pHandler->csMsgTransact );
		pHandler->RouteID.dest.process_id = g.my_message_id;
		pHandler->RouteID.dest.service_id = 0;
		pHandler->servicename = StrDup( WIDE("local_events") );

		msg.ServiceID = 0; // this is a special event channel to myself.

		//lprintf( WIDE("opening local only service... we're making up numbers here.") );
		if( g.pLocalEventThread )
		{
			msg.thread = GetThreadID( g.pLocalEventThread );
		}
		else
		{
			lprintf( WIDE("Event message system has not started correctly...") );
			Release( pHandler );
			LeaveCriticalSec( &g.csLoading );
			return NULL;
		}
	}

	// EVENTHANDLER is the outbound structure to idenfity
	// the service information which messages go where...
	{
		//pHandler = Allocate( sizeof( EVENTHANDLER ) + strlen( service?service:"local_events" ) );
		//strcpy( pHandler->servicename, service?service:"local_events" );
		//lprintf( WIDE("Allocating local structure which manages our connection to this service...") );
		pHandler->flags.destroyed = 0;
		pHandler->flags.dispatched = 0;

		//pHandler->MsgCountEvents = msg.events;
		//pHandler->MsgCount = msg.functions;
		pHandler->Handler = EventHandler;
		pHandler->HandlerEx = EventHandlerEx;
		pHandler->HandlerExx = EventHandlerExx;
      pHandler->psv = psv;
		// thread ID to wake for events? or to probe?
		// thread ID unused.
		pHandler->EventID = msg.thread;
		if( service )
		{
			pHandler->flags.local_service = 0;
			//pHandler->RouteID.dest = msg.ServiceID; // magic place where source ID is..
			pHandler->msgq_events = g.msgq_event;
		}
		else
		{
			pHandler->flags.local_service = 1;
			pHandler->RouteID.dest.process_id = g.my_message_id;
			pHandler->RouteID.dest.service_id = 0;
			pHandler->msgq_events = g.msgq_local;
		}
		LinkThing( g.pHandlers, pHandler );
		if( service )
		{
			PSERVICE_CLIENT pClient = AddClient( &pHandler->RouteID ); // hang this on the list of services to check...
			pClient->flags.is_service = 1;
			pClient->handler = pHandler;
		}
		LeaveCriticalSec( &g.csLoading );
	}
	return &pHandler->RouteID;
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( PSERVICE_ROUTE, LoadService)( CTEXTSTR service, EventHandlerFunction EventHandler )
{
	return _LoadService( service, EventHandler, NULL, NULL, NULL, NULL, 0 );
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceEx)( CTEXTSTR service, EventHandlerFunctionEx EventHandlerEx )
{
	return _LoadService( service, NULL, EventHandlerEx, NULL, NULL, NULL, 0 );
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( PSERVICE_ROUTE, LoadServiceExx)( CTEXTSTR service, EventHandlerFunctionExx EventHandlerEx, PTRSZVAL psv )
{
	return _LoadService( service, NULL, NULL, EventHandlerEx, NULL, NULL, psv );
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( void, UnloadService )( CTEXTSTR name )
{
	PEVENTHANDLER pHandler;
	pHandler = g.pHandlers;
	while( pHandler )
	{
		if( StrCaseCmp( pHandler->servicename, name ) == 0 )
			break;
		pHandler = pHandler->next;
	}
	if( pHandler )
	{
		_32 Responce;
		//lprintf( WIDE("Unload service: %s"), pHandler->servicename );
		if( pHandler->flags.local_service )
		{
			//lprintf( WIDE("Local service... resulting quick success...") );
			Responce = (MSG_ServiceUnload)|SERVER_SUCCESS;
		}
		else
		{
			//lprintf( WIDE("Requesting message %d from %d "), MSG_ServiceUnload , pHandler->MsgBase );
			Responce = ((MSG_ServiceUnload)|SERVER_SUCCESS);
			if( !TransactServerMessage( &pHandler->RouteID
											  , MSG_ServiceUnload, NULL, 0
											  , &Responce/*NULL*/, NULL, 0 ) )
			{
				lprintf( WIDE("Transaction to ServiceUnload failed...") );
			}
			else if( Responce != ((MSG_ServiceUnload)|SERVER_SUCCESS) )
			{
				lprintf( WIDE("Server reports it failed to unload the service %08") _32fx WIDE(" %08") _32fx WIDE("")
						 , Responce, ((MSG_ServiceUnload)|SERVER_SUCCESS) );
			// no matter what the result, this must still release this
			// resource....
			//return;
			}
			while( pHandler->flags.dispatched )
			{
				Relinquish();
			}
		}

		UnlinkThing( pHandler );

		//lprintf( WIDE("Release? wow release hangs forever?") );
		//Release( pHandler );
		if( 0 && !g.pHandlers )
		{
			Log( WIDE("No more services loaded - killing threads, disconnecting") );
			if( g.pLocalEventThread )
			{
				EndThread( g.pLocalEventThread );
				// wake up the thread...
			}
			if( g.pEventThread )
				EndThread( g.pEventThread );
			if( g.pThread )
				EndThread( g.pThread );

			CloseMessageQueues();
			g.flags.events_ready = 0;
			g.flags.local_events_ready = 0;
			g.flags.failed = 0;
			g.flags.message_handler_ready = 0;
			g.flags.message_responce_handler_ready = 0;
		}
		//Log( WIDE("Done unloading services...") );
		return;
	}
	Log( WIDE("Service was already Unloaded!?!?!?!?!?") );
}


MSGCLIENT_NAMESPACE_END

//-------------------------------------------------------------
