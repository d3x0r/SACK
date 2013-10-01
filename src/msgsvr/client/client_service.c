
#include "global.h"

MSGCLIENT_NAMESPACE

//--------------------------------------------------------------------

LOGICAL HandleCoreMessage( PQMSG msg, size_t msglen DBG_PASS )
{
	//Log2( WIDE("Read message to %d (%08x)"), g.pid_me, msg->hdr.msgid );

	if( msg->hdr.msgid == IM_TARDY )
	{
		PTRANSACTIONHANDLER handler;
		lprintf( WIDE("Server wants to extend timout to %") _32f WIDE(""), QMSGDATA( msg )[0] );
		for( handler = g.pTransactions; handler; handler = handler->next )
			if( ( handler->route->dest.process_id == msg->dest.process_id )
				&& ( handler->route->dest.service_id == msg->dest.service_id ) )
			{
			// result of IM_TARDY, add an amount of time to the message...
				handler->wait_for_responce = msg->hdr.msgid + timeGetTime();
				break;
			}
		if( !handler )
		{
			lprintf( WIDE("A service announced it was going to be tardy to someone who was not talking to it!") );
			DebugBreak();
		}
		// okay continue in a do_while will execute the while condition
		// also, much like while(){} will...
		msg->hdr.msgid = RU_ALIVE;
		return TRUE; // handled.
	}
	else if( msg->hdr.msgid == RU_ALIVE )
	{
		QMSG Msg;
#ifdef DEBUG_MESSAGE_BASE_ID
		DBG_VARSRC;
#endif
		//Log( WIDE("Got RU_ALIVE am responding  AM ALIVE!!") );
		Msg.dest.process_id              = msg->hdr.source.process_id;
		Msg.dest.service_id     = msg->hdr.source.service_id;
		Msg.hdr.source.process_id = msg->dest.process_id;
		Msg.hdr.source.service_id = msg->dest.service_id;
		Msg.hdr.msgid = IM_ALIVE;
		// by using the input message queue, it makes sure that
		// both sides are processing input messages... otherwise
		// previously queued messages would be received first.
		msgsnd( g.msgq_in, MSGTYPE &Msg, sizeof(QMSG) - sizeof( MSGIDTYPE ), 0 );
		return TRUE; // handled.
	}
	else if( msg->hdr.msgid == IM_ALIVE )
	{
		PSERVICE_CLIENT client = FindClient( (PSERVICE_ROUTE)&msg );
#ifdef DEBUG_RU_ALIVE_CHECK
		lprintf( WIDE("Got message IM_ALIVE from client... %") _32f WIDE(""), msg->hdr.source.process_id );
#endif
		if( client )
		{
			//lprintf( WIDE("Updating client %p with current time...allowing him to requery.."), client );
			if( client->flags.status_queried )
			{
				client->flags.status_queried = 0;
				// fake a get-next-message... if status_queried is NOT set
				// then, this is a request of ProbeClientAlive.
				msg->hdr.msgid = RU_ALIVE;
			}
			client->last_time_received = timeGetTime();
		}
		// go back to top and get another message...
		// application can care about this...
		// maybe should check...
		//msg->hdr.msgid = RU_ALIVE;
		return TRUE; // handled.
	}
	return FALSE;
}

PTRSZVAL CPROC HandleServiceMessages( PTHREAD thread )
//#define DoHandleServiceMessages(p) DoHandleServiceMessagesEx(p DBG_SRC)
{
	static _32 one = 1;
	LOGICAL master_service = (LOGICAL)GetThreadParam( thread );
	S_32 length;
	MSGIDTYPE msgid;
	size_t result_length = INVALID_INDEX; // default to ...
	// can't use getpid cause we really want the threadID
	// also this value may NOT be negative.

	PQMSG recv = (PQMSG)Allocate( MSG_DEFAULT_RESULT_BUFFER_MAX );
	PQMSG result = (PQMSG)Allocate( MSG_DEFAULT_RESULT_BUFFER_MAX );
	// consume any outstanding messages... so we don't get confused
	// someone may have requested things for this service...
	//lprintf( "vvv" );
	(msgid=master_service?one:g.my_message_id);
#ifdef DEBUG_DATA_XFER
	lprintf( "Recieving on %ld", msgid );
#endif
	while( msgrcv( g.msgq_out
					 , MSGTYPE recv
					 , 8192
					 , msgid
					 , IPC_NOWAIT ) > 0 )
	{
		//lprintf( WIDE("Dropping a message...") );
	}
			//lprintf( "^^^" );
	while( !g.flags.disconnected )
	{
		//lprintf( WIDE("service is waiting for messages to %08lx"), g.my_message_id );
		// receiving from the 'out' queue which is commands TO a service.
			//lprintf( "vvv" );
		g.flags.bWaitingInReceive = 1;

#ifdef DEBUG_DATA_XFER
		lprintf( "Recieving on %ld", msgid );
#endif
		length = msgrcv( g.msgq_out
							, MSGTYPE recv
							, 8192
							, msgid
							, 0 );

		g.flags.bWaitingInReceive = 0;
#ifdef DEBUG_DATA_XFER
		lprintf( "^^^ %d", length );
#endif
		length -= ( sizeof( QMSG ) - sizeof( MSGIDTYPE ) );
		if( length < 0 )
		{
			if( errno == EINTR ) // got a signal - ignore and try again.
				continue;
			if( errno == EIDRM )
			{
				Log( WIDE("Server ended.") );
				break;
			}
			if( errno == EINVAL )
			{
				Log( WIDE("Queues Closed?") );
				g.flags.disconnected = 1;
				break;
			}
#if defined( _WIN32 ) || defined( USE_SACK_MSGQ )
			if( errno == EABORT )
			{
				Log( WIDE( "Server Read Abort." ) );
				break;
			}
#endif
			Log1( WIDE("msgrecv error: %d"), errno );
			continue;
		}
#ifdef DEBUG_DATA_XFER
		else
		{
			lprintf( WIDE("Received Message.... g.msgq_out %d"), length );
			LogBinary( (P_8)recv, length + sizeof( QMSG ) );
		}
#endif

		// setup the result message to be a reply to the incoming message.
		result->dest.process_id            = recv->hdr.source.process_id;
		result->dest.service_id            = recv->hdr.source.service_id;
		result->hdr.source.process_id = recv->dest.process_id;
		result->hdr.source.service_id = recv->dest.service_id;
		result->hdr.msgid             = recv->hdr.msgid | SERVER_UNHANDLED;

		if( recv->hdr.msgid & 0xF0000000 )
		{
			// message is a responce from someone else...
		}
		else
		{
			if( recv->dest.service_id == 0 )
			{
				if( HandleCoreMessage( recv, length DBG_SRC ) )
					continue;
			}
			{
				int handled = FALSE;
				PCLIENT_SERVICE service;
				for( service = g.services; service; service = service->next )
				{
					// process_id is already matched at this point, or we wouln't have the message
					// just have to give it to the local service.
#ifdef DEBUG_DATA_XFER
					lprintf( "is %d == %d", service->ServiceID, recv->dest.service_id );
#endif
					if( service->ServiceID == recv->dest.service_id )
					{
						//lprintf( WIDE("Found the service...%s"), service->name );
						break;
					}
				}
				if( service )
				{
					_32 msgid = recv->hdr.msgid;
#ifdef DEBUG_MESSAGE_BASE_ID
					lprintf( WIDE("service base %ld(+%ld) and this is from %s")
							 , 0
                       , service->entries
							 , ( g.my_message_id == recv->hdr.source.process_id )?"myself":"someone else" );
#endif
					if( msgid < service->entries )
					{
						int result_okay = 0;
						g.flags.handling_client_message = 1;
						if( service->handler_ex )
						{
#if defined( LOG_HANDLED_MESSAGES )
							lprintf( WIDE("Got a service message to handler: %08lx length %ld")
									 , recv->hdr.source.process_id
									 , length + sizeof(QMSG) );
#endif
							result_length = MSG_DEFAULT_RESULT_BUFFER_MAX;
							handled = TRUE;
							result_okay = service->handler_ex( service->psv
																		, (PSERVICE_ROUTE)result
																		, msgid
																		, QMSGDATA(recv), length
																		, QMSGDATA(result), &result_length ) ;
						}
						if( !handled && service->handler )
						{
#if defined( LOG_HANDLED_MESSAGES )
							lprintf( WIDE("Got a service message to handler: %08lx length %ld")
									 , recv->hdr.source.process_id
									 , length + sizeof(QMSG) );
#endif
							result_length = MSG_DEFAULT_RESULT_BUFFER_MAX;
							handled = TRUE;
							result_okay = service->handler( (PSERVICE_ROUTE)result
																	, msgid
																	, QMSGDATA(recv), length
																			, QMSGDATA(result), &result_length ) ;
						}
						if( !handled )
						{
							if( service->functions
								&& service->functions[msgid].function )
							{
								//result_length = 4096; // maximum responce buffer...
#if defined( LOG_HANDLED_MESSAGES )
								lprintf( WIDE("Got a service : (%d)%s from %08lx length %ld")
										 , msgid
#ifdef _DEBUG
										 , service->functions[msgid].name
#else
										 , WIDE("noname")
#endif
										 , recv->hdr.source.process_id
										 , length + sizeof(QMSG) );
#endif
								result_length = 0; // safer default. although uninformative.
								handled = TRUE;
								result_okay = service->functions[msgid].function( (PSERVICE_ROUTE)result
																								, QMSGDATA( recv )
																								, length
																								, QMSGDATA(result)
																								, &result_length );
								switch( msgid )
								{
								case MSG_ServiceLoad:
									if( result_okay && ( result_length == 0 ) )
									{
#if defined( LOG_HANDLED_MESSAGES )
										lprintf( "Using default handler for service load" );
#endif
										((MsgSrv_ReplyServiceLoad*)QMSGDATA(result))->ServiceID = result->hdr.source.service_id;
										((MsgSrv_ReplyServiceLoad*)QMSGDATA(result))->thread = 0;
										result_length = sizeof( MsgSrv_ReplyServiceLoad );
									}
									break;
								}
							}
							else if( service->functions )
							{
								switch( msgid )
								{
								case MSG_ServiceUnload:
#if defined( LOG_HANDLED_MESSAGES )
									lprintf( "Using default handler for service unload" );
#endif
									result_okay = 1;
									break;
								case MSG_ServiceLoad:
#if defined( LOG_HANDLED_MESSAGES )
									lprintf( "Using default handler for service load" );
#endif
									((MsgSrv_ReplyServiceLoad*)QMSGDATA(result))->ServiceID = result->dest.service_id;
									((MsgSrv_ReplyServiceLoad*)QMSGDATA(result))->thread = 0;
									result_length = sizeof( MsgSrv_ReplyServiceLoad );
									result_okay = 1;
									break;
								default:
#if defined( LOG_HANDLED_MESSAGES )
									DebugBreak();
									lprintf( WIDE("didn't have a function for 0x%lx (%ld) or %s")
											 , msgid
											 , msgid
#ifdef _DEBUG
											 , service->functions[msgid].name
#else
											 , WIDE("noname")
#endif

										 );
#endif
									result_okay = 0;
									result_length = INVALID_INDEX;
									break;
								}
							}
							else
							{
								DebugBreak();
								result_okay = 0;
								result_length = 0;
							}
						}
						if( result_okay )
							result->hdr.msgid = recv->hdr.msgid | SERVER_SUCCESS;
						else
							result->hdr.msgid = recv->hdr.msgid | SERVER_FAILURE;

						// a key result value to indicate there is
						// no responce to be sent to the client.
						if( result_length != INVALID_INDEX )
						{
#ifdef DEBUG_DATA_XFER
							DBG_VARSRC;
#endif
							msgsnd( g.msgq_in, MSGTYPE result, result_length + (sizeof(QMSG) - sizeof( MSGIDTYPE )), 0 );
						}
						else
						{
							//Log( WIDE("No responce sent") );
						}
					}
				}
				else
				{
					lprintf( WIDE("Failed to find target service for message.") );
				}
			}
#if defined(_DEBUG) && defined( LOG_HANDLED_MESSAGES )
			Log( WIDE("Message complete...") );
#endif
			g.flags.handling_client_message = 0;
		}
	}
	Release( recv );
	Release( result );

	return 0;
}

//--------------------------------------------------------------------

void DoRegisterService( PCLIENT_SERVICE pService )
{
	_32 MsgID;
	// if I'm the master service, I don't very well have to
	// register with myself do I?

	if( !pService->flags.bFailed )
	{
		//lprintf( WIDE("Transacting a message....") );
		size_t result_len = sizeof( pService->ServiceID );
		//lprintf( WIDE("Transacting a message....") );
		if( !TransactServerMultiMessage( &g.master_service_route, CLIENT_REGISTER_SERVICE, 1
												 , &MsgID, &pService->ServiceID, &result_len
												 , pService->name, (StrLen( pService->name ) + 1) * sizeof(TEXTCHAR) // include NUL
												 ) )
		{
			pService->flags.bFailed = 1;
			lprintf( WIDE("registration failed.") );
		}
		else
		{
			//lprintf( WIDE("MsgID is %lx and should be %lx? "),MsgID , ( CLIENT_REGISTER_SERVICE | SERVER_SUCCESS ) );
			if( MsgID != ( CLIENT_REGISTER_SERVICE | SERVER_SUCCESS ) )
			{
				pService->flags.bFailed = 1;
				lprintf( WIDE("registration failed.") );
			}
		}
		pService->flags.bRegistered = 1;
	}
}

//--------------------------------------------------------------------

int ReceiveServerMessageEx( PTRANSACTIONHANDLER handler, PQMSG MessageIn, size_t MessageLen DBG_PASS )
{
	/*
	 first check... LoadService() response.
    LoadService() will have been called in another thread,
    */
	if( (MessageIn->hdr.msgid&0xFFFFFFF) == (MSG_ServiceLoad) )
	{
		lprintf( WIDE("Loading service responce... setup the service ID for future com") );

		handler->route->dest.process_id = MessageIn->hdr.source.process_id;
		handler->route->dest.service_id = MessageIn->hdr.source.service_id;
		handler->route->source.process_id = MessageIn->dest.process_id;
		handler->route->source.service_id = MessageIn->dest.service_id;

		if( handler->MessageID )
			(*handler->MessageID) = MessageIn->hdr.msgid;
	}

	if( MessageIn->hdr.source.process_id == 1
		&& MessageIn->hdr.source.service_id == 0
	  )
	{
		//if( handler != &g._handler )
		{
			// result received from some other handler (probably something like
			//lprintf( WIDE("message from core service ... wrong handler?") );
			if( MessageIn->hdr.msgid & SERVER_FAILURE )
			{
				// eat this message...
				//return 0;
			}
			//else
			//	return 1;
			//DebugBreak();
		}
		if( handler->MessageID )
			(*handler->MessageID) = MessageIn->hdr.msgid;
  //  	handler = &g._handler;
	}

	if( ( handler->route->dest.process_id != MessageIn->hdr.source.process_id )
		|| ( handler->route->dest.service_id != MessageIn->hdr.source.service_id ) )
	{
		//lprintf( WIDE("%ld and %ld "), handler->ServiceID, MessageIn->hdr.source.process_id );
		// this handler is not for this message responce...
		//DebugBreak();

		//lprintf( WIDE("this handler is not THE handler!") );
		return 1;
	}
	else
	{
		//lprintf( WIDE("All is well, check message ID %p"), handler );
		if( handler->MessageID )
			(*handler->MessageID) = MessageIn->hdr.msgid;

		if( handler->LastMsgID != ( (MessageIn->hdr.msgid)& 0xFFFFFFF ) )
		{
			LogBinary( (P_8)MessageIn, MessageLen );
			lprintf( WIDE("len was %") _32f, MessageLen );
			lprintf( WIDE("Message is for this guy - but isn't the right ID! %") _32f WIDE(" %") _32f WIDE(" %") _32f WIDE("")
					 , handler->LastMsgID, (MessageIn->hdr.msgid) & 0xFFFFFFF, 0 );
			//DebugBreak();
			return 1;
		}
	}

	if( handler->msg && handler->len )
	{
		MessageLen -= sizeof(QMSG) - sizeof( MSGIDTYPE );  // subtract message ID and message source from it.
		if( MessageLen > 0 )
		{
			if( (S_32)(*handler->len) < MessageLen )
			{
				_lprintf( DBG_RELAY )( WIDE("Cutting out possible data to the application - should provide a failure! %") _32f WIDE(" expected %") _32fs WIDE(" returned"), (*handler->len), MessageLen );
				MessageLen = (*handler->len);
			}
			MemCpy( handler->msg, QMSGDATA( MessageIn ), MessageLen );
		}
		(*handler->len) = MessageLen;
	}
	else
	{
		// maybe it was just interested in the header...
		// which will be the source ID and the message ID
		if( MessageLen - ( sizeof( QMSG ) - sizeof( MSGIDTYPE ) ) )
		{
			LogBinary( (P_8)MessageIn, MessageLen + sizeof( QMSG ) );
			SystemLogEx( WIDE("Server returned result data which the client did not get") DBG_RELAY );
		}
	}
	// we now have to wait for another response.
	// this one has been consumed.
	handler->flags.responce_received = 0;

	return 0;
}

//--------------------------------------------------------------------

#undef RegisterServiceEx
CLIENTMSG_PROC( LOGICAL, RegisterServiceEx )( CTEXTSTR name
														  , server_function_table functions
														  , int entries
														  , server_message_handler handler
														)
{
   return RegisterServiceExx( name, functions, entries, handler, NULL, 0 );
}


// don't really get a route from this...
// service_routes will become available as clients connect.
CLIENTMSG_PROC( LOGICAL, RegisterServiceExx )( CTEXTSTR name
															, server_function_table functions
															, int entries
															, server_message_handler handler
															, server_message_handler_ex handler_ex
															, PTRSZVAL psv
															)
{
	int status;
	if( !name )
	{
		g.flags.bMasterServer = 1;
	}
	if( ( status = _InitMessageService( FALSE ) ) < 1 )
	{
		if( status == -1 )
			lprintf( WIDE("Initization of %s message service failed (service already exists? communication)."), name );
		if( status == 0 )
			; //lprintf( WIDE("Initization of %s message service failed (service already exists? communication)."), name );
		return FALSE;
	}
	else
	{
		//static int nBaseMsg;
		PCLIENT_SERVICE pService = New( CLIENT_SERVICE );

		pService->service_routes = NULL;

		pService->flags.bRegistered = 0;
		pService->flags.bFailed = 0;
		pService->flags.connected = 0;
		pService->flags.bClosed = 0;
		pService->flags.bWaitingInReceive = 0;

		// setup service message base.
		if( !name )
		{
			pService->flags.bMasterServer = 1;
			pService->name = StrDup( WIDE("Master Server") );
		}
		else
		{
			pService->flags.bMasterServer = 0;
			pService->name = StrDup( name );
		}
		pService->handler_ex = handler_ex;
		pService->handler = handler;
		pService->functions = functions;
		pService->entries = entries?entries:256;
		pService->references = 0;

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

		if( !pService->flags.bMasterServer && !g.flags.bServiceHandlerStarted )
		{
			// pass FALSE (not master service, begin receiving on my_message_id)
			ThreadTo( HandleServiceMessages, (PTRSZVAL)0 );
			g.flags.bServiceHandlerStarted = 1;
		}

		if( pService->flags.bMasterServer && !g.flags.bCoreServiceHandlerStarted )
		{
			// pass FALSE (IS master service, begin receiving on 1)
			ThreadTo( HandleServiceMessages, (PTRSZVAL)1 );
			g.flags.bCoreServiceHandlerStarted = 1;
		}

		if( pService->flags.bMasterServer && !g.flags.bCoreServiceHandlerStarted )
		{
			// pass 1 as message filter ID, begin receiving message 1 on input queue.
			ThreadTo( HandleMessages, 1 );
			g.flags.bCoreServiceInputHandlerStarted = 1;
		}

		if( !pService->flags.bMasterServer )
		{
			RegisterWithMasterService();
			DoRegisterService( pService );
		}
		else
		{
			// should attempt some sort of ping before assuming it's good.
			pService->ServiceID = 0;
			pService->GetFunctionTable = NULL;
			pService->flags.bRegistered = 1;
		}

		while( !pService->flags.bRegistered )
			Relinquish();

		if( pService->flags.bFailed )
		{
			pService->flags.bClosed = 1;
			while( pService->flags.bWaitingInReceive )
			{
				pService->recv->dest.process_id = INVALID_MESSAGE;
				if( pService->thread )
					WakeThread( pService->thread );
				else
					break;
				//Relinquish();
			}
			while( pService->thread )
			{
				Relinquish();
				WakeThread( pService->thread );
			}
			Release( pService->name );
			Release( pService );
			return FALSE;
		}
		if( pService )
		{
			LinkThing( g.services, pService );
			return TRUE;
		}
	}
	return FALSE;
}
	//--------------------------------------------------------------------
#undef RegisterService
CLIENTMSG_PROC( LOGICAL, RegisterService )( TEXTCHAR *name
														  , server_function_table functions
														  , int entries
														)
{
	return RegisterServiceEx( name, functions, entries, NULL );
}


MSGCLIENT_NAMESPACE_END

//-------------------------------------------------------------
