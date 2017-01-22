
#include "global.h"

MSGCLIENT_NAMESPACE


//--------------------------------------------------------------------

// this expects route to be destination-correct
CLIENTMSG_PROC(int, SendMultiServiceEventPairsEx)( PSERVICE_ROUTE RouteID, uint32_t event
															  , uint32_t parts
															  , BUFFER_LENGTH_PAIR *pairs
															  DBG_PASS
															  )
#define SendMultiServiceEventPairs(p,ev,par,pair) SendMultiServiceEventPairsEx(p,ev,par,pair DBG_SRC)
{
	static struct {
		QMSG msg;
		uint32_t data[2048-sizeof(QMSG)];
	}msg;
	static LOGICAL initialized;
	static CRITICALSECTION cs;
	uint8_t* msgbuf;
	size_t sendlen = 0;
	if( !initialized )
	{
		InitializeCriticalSec( &cs );
		initialized = 1;
	}
#ifdef DEBUG_MESSAGE_BASE_ID
	//DBG_VARSRC;
#endif
	EnterCriticalSec( &cs );

	if( RouteID )
	{
		msg.msg.dest = RouteID->dest;
		msg.msg.hdr.source = RouteID->source;
	}
	else
	{
		// local event message, should be hard coded?
		msg.msg.dest.process_id = g.my_message_id;
		msg.msg.dest.service_id = 0;
		msg.msg.hdr.source.process_id = g.my_message_id;
		msg.msg.hdr.source.service_id = 0;
	}
	msg.msg.hdr.msgid = event;
	msgbuf = (uint8_t*)msg.data;
	while( parts )
	{
		if( pairs->buffer && pairs->len )
		{
			MemCpy( msgbuf + sendlen, pairs->buffer, pairs->len );
			sendlen += pairs->len;
			pairs++;
		}
		parts--;
	}
						// outgoing que for this handler.
#if defined( DEBUG_EVENTS ) 
	_lprintf(DBG_RELAY)( WIDE("Send Event...") );
#if !defined( DEBUG_DATA_XFER )
	LogBinary( &msg, sendlen + sizeof( MSGHDR ) + sizeof( MSGIDTYPE ) );
#endif
#endif
	if( !msg.msg.dest.process_id )
		msg.msg.dest.process_id = g.my_message_id;

	{
		int status;
		status = msgsnd( RouteID?g.msgq_event:g.msgq_local, MSGTYPE &msg
							, sendlen + ( sizeof( QMSG ) - sizeof( MSGIDTYPE ) ), 0 );
		LeaveCriticalSec( &cs );
		return !status;
	}

}

#if defined( _DEBUG ) || defined( _DEBUG_INFO )
static struct {
	CTEXTSTR pFile;
	int nLine;
}nextsmmse;
#endif
#undef SendMultiServiceEvent
CLIENTMSG_PROC(int, SendMultiServiceEvent)( PSERVICE_ROUTE RouteID, uint32_t event
								 , uint32_t parts
								 , ... )
{
	int status;
	BUFFER_LENGTH_PAIR *pairs = NewArray( BUFFER_LENGTH_PAIR, parts );
	uint32_t n;
	va_list args;
	va_start( args, parts );
	for( n = 0; n < parts; n++ )
	{
		pairs[n].buffer = va_arg( args, POINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
	status = SendMultiServiceEventPairsEx( RouteID, event, parts, pairs, nextsmmse.pFile, nextsmmse.nLine );
#else
	status = SendMultiServiceEventPairsEx( RouteID, event, parts, pairs );
#endif
	Release( pairs );
	return status;
}

CLIENTMSG_PROC(SendMultiServiceEventProto, SendMultiServiceEventEx)( DBG_VOIDPASS )
{
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
	nextsmmse.pFile = pFile;
	nextsmmse.nLine = nLine;
#endif
	return SendMultiServiceEvent;
}

//--------------------------------------------------------------------

uintptr_t CPROC HandleEventMessages( PTHREAD thread )
{
	g.pEventThread = thread;
	g.flags.events_ready = TRUE;
#ifdef DEBUG_THREADS
	lprintf( WIDE("threadID: %Lx %lx"), GetThreadID( thread ), (unsigned long)(GetThreadID( thread ) & 0xFFFFFFFF) );
#endif
	//g.my_message_id = g.my_message_id; //(uint32_t)( thread->ThreadID & 0xFFFFFFFF );
	while( !g.flags.disconnected )
	{
		int r;
		if( thread == g.pEventThread )
		{
			static uint32_t MessageEvent[2048]; // 8192 bytes
#ifdef DEBUG_EVENTS
			lprintf( WIDE("Reading event...") );
#endif
			if( ( r = HandleEvents( g.msgq_event, (PQMSG)MessageEvent, 0 ) ) < 0 )
			{
				Log( WIDE("EventHandler has reported a fatal error condition.") );
				break;
			}
#ifdef DEBUG_EVENTS
			lprintf( WIDE("Read event...") );
#endif
		}
		else if( r == 2 )
		{
			Log( WIDE("Thread has been restarted.") );
			// don't clear ready or main event flag
			// things.
			return 0;
		}
	}
	lprintf( WIDE("Done with this event thread - BAD! ") );
	g.flags.events_ready = FALSE;
	g.pEventThread = NULL;
	return 0;
}

//--------------------------------------------------------------------

int HandleEvents( MSGQ_TYPE msgq, PQMSG MessageEvent, int initial_flags )
{
	int receive_flags = initial_flags;
	int receive_count = 0;
	while( 1 )
	{
		int32_t MessageLen;
#ifdef DEBUG_EVENTS
		lprintf( WIDE("Reading eventqueue... my_message_id = %d"), g.my_message_id );
#endif
			//lprintf( "vvv" );
		MessageLen = msgrcv( msgq
								 , MSGTYPE MessageEvent, 8192
								 , g.my_message_id
								 , receive_flags );
		//lprintf( "^^^" );
#ifdef DEBUG_DATA_XFER
      if( MessageLen >= 0 )
			LogBinary( MessageEvent, MessageLen + sizeof( MSGIDTYPE ) );
#endif
		if( (MessageLen+ sizeof( MSGIDTYPE )) == 0 )
		{
			lprintf( WIDE("Recieved -4 message (no data?!) no message, should have been -1, ENOMSG") );
		}
		else if( MessageLen == -1 )
		{
#ifdef _WIN32
			int my_errno = GetLastError();
#ifdef errno
#undef errno
#endif
#define errno my_errno
#endif
			//Log( WIDE("Failed a message...") );
			if( errno == ENOMSG )
			{
				//Log( WIDE("No message...") );
				if( receive_count )
				{
					PEVENTHANDLER pLastHandler;
					PEVENTHANDLER pHandler = g.pHandlers;
#ifdef DEBUG_EVENTS
					lprintf( WIDE("Dispatch dispatch_pending..") );
#endif
					while( pHandler )
					{
						pHandler->flags.dispatched = 1;
						if( pHandler->flags.notify_if_dispatched )
						{
							//lprintf( WIDE("Okay one had something pending...") );
							if( pHandler->Handler )
								pHandler->Handler( MSG_EventDispatchPending, NULL, 0 );
							else if( pHandler->HandlerEx )
								pHandler->HandlerEx( 0, MSG_EventDispatchPending, NULL, 0 );
							else if( pHandler->HandlerExx )
								pHandler->HandlerExx( pHandler->psv, 0, MSG_EventDispatchPending, NULL, 0 );
							// okay did that... now clear status.
							pHandler->flags.notify_if_dispatched = 0;
						}
						pLastHandler = pHandler;
						pHandler = pHandler->next;
						pLastHandler->flags.dispatched = 0;
						//lprintf( WIDE("next handler please...") );
					}
				}
				receive_flags = 0; // re-enable pause.
				//lprintf( WIDE("Done reading...") );
				break; // done;
			}
			else if( errno == EIDRM )
			{
				Log( WIDE("Queue was removed.") );
				g.flags.events_ready = 0;
				return -1;
			}
			else
			{
				if( errno != EINTR )
					xlprintf( LOG_ALWAYS )( WIDE("msgrcv resulted in error: %d"), errno );
				//else
				//	Log( WIDE("EINTR received.") );
				break;
			}
		}
		else
		{
			PEVENTHANDLER pHandler = g.pHandlers;
			receive_flags = IPC_NOWAIT;
			receive_count++;
#ifdef DEBUG_EVENTS
			lprintf( WIDE("Got an event message...") );
#endif
			if( MessageEvent->hdr.msgid < MSG_EventUser )
			{
				// core server events...
				// MessageEvent->hdr.sourceid == client_id (client_route_id)
				switch( MessageEvent->hdr.msgid )
				{
				case MSG_SERVICE_DATA:
					{
						PREFIX_PACKED struct msg_service_name {
							PLIST *list;
							// so what i there's no reference to this?! OMG!
							// I still need to have it defined!
							SERVICE_ENDPOINT service_id;
							TEXTCHAR newname; // actually is the first element of an array of characters with nul terminator.
						} PACKED *data_msg = (struct msg_service_name*)(MessageEvent+1);
						// ahh - someone requested the list of services...
						// client-application API...
						// add to the list... what list?
						PLIST *list = data_msg->list;
						// MessageEvent[3] for this message is client_id
						// which may be used to directly contact the client.
						//lprintf( WIDE("Adding service %ld called %s to list.."), MessageEvent[4], MessageEvent + 5 );
						// SetLink( data_msg->list, data_msg->dest.service_id, data_msg->newname );
						AddLink( list
								 , StrDup( &data_msg->newname ) );
					}
					break;
				case MSG_SERVICE_NOMORE:
					{
						// ahh - someone requested the list of services...
						// client-application API...
						// this is the END of the list
						PREFIX_PACKED struct msg_service_nomore {
							int *bDone;
							PTHREAD pThread;
						} PACKED *data_msg = (struct msg_service_nomore*)(MessageEvent+1);
						//int *bDone = *(int**)((&MessageEvent->hdr)+1);
						//PTHREAD pThread = *(PTHREAD*)((int*)((&MessageEvent->hdr)+1)+1);
						(*data_msg->bDone) = 1;
						WakeThread( data_msg->pThread );
					}
					break;
				}
			}
			else for( ; pHandler; pHandler = pHandler->next )
			{
				uint32_t Msg;
#ifdef DEBUG_EVENTS
				lprintf( WIDE("Finding handler for %ld-%d %p (from %lx to %lx)")
						 , MessageEvent->hdr.msgid
						 , 0//pHandler->MsgCountEvents
						 , pHandler->Handler
						 , (uint32_t*)((&MessageEvent->hdr)+1)
						 , 0 /*pHandler->ServiceID*/ );
#endif
				//if( !pHandler->ServiceID )
				//	pHandler->ServiceID = g.my_message_id;
				if( ( pHandler->RouteID.source.process_id != MessageEvent->dest.process_id )
               || ( pHandler->RouteID.source.service_id != MessageEvent->dest.service_id ) )
				{
					// if it's not from this handler's server... try the next.
					continue;
				}
				Msg = MessageEvent->hdr.msgid;
				//lprintf( WIDE("Msg now %d base %d %d"), Msg, pHandler->MsgBase, pHandler->MsgCountEvents );
				if(( pHandler->Handler // have a handler
					 || pHandler->HandlerEx // have a fancier handler....
                || pHandler->HandlerExx ) // or an even fancier handler...
				  && !( Msg & 0x80000000 ) // not negative result (msg IS 32 bits)
				  /*&& ( Msg < pHandler->MsgCountEvents )*/ ) // in range of handler
				{
					int result_yesno;
#ifdef DEBUG_EVENTS
					lprintf( WIDE("Dispatch event message to handler...") );
#endif
					pHandler->flags.dispatched = 1;
					if( pHandler->Handler )
					{
						//lprintf( WIDE("small handler") );
						result_yesno = pHandler->Handler( Msg, (uint32_t*)((&MessageEvent->hdr)+1), MessageLen - sizeof( MSGHDR ) );
					}
					else if( pHandler->HandlerEx )
					{
						//lprintf( WIDE("ex handler...%d"), Msg );
						result_yesno = pHandler->HandlerEx( (PSERVICE_ROUTE)MessageEvent
																	 , Msg
																	 , (uint32_t*)((&MessageEvent->hdr)+1)
																	 , MessageLen - sizeof( MSGHDR ) );
					}
					else if( pHandler->HandlerExx )
					{
						//lprintf( WIDE("ex handler...%d"), Msg );
						result_yesno = pHandler->HandlerExx( pHandler->psv
																	  , (PSERVICE_ROUTE)(uintptr_t)MessageEvent->hdr.source.process_id
																	  , Msg
																	  , (uint32_t*)((&MessageEvent->hdr)+1)
																	  , MessageLen - sizeof( MSGHDR ) );
					}
					if( result_yesno & EVENT_WAIT_DISPATCH )
					{
						//lprintf( WIDE("Setting status to send dispatch_events...") );
						pHandler->flags.notify_if_dispatched = 1;
					}
					pHandler->flags.dispatched = 0;
					break;
				}
			}
		}
	}
	if( receive_count )
		return 1;
	return 0;
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( int, ProcessClientMessages )( uintptr_t unused )
{
	static uint32_t MessageBuffer[2048];
	if( IsThisThread( g.pEventThread ) )
	{
		lprintf( WIDE("External handle event messages...") );
		return HandleEvents( g.msgq_event, (PQMSG)MessageBuffer, 0/*IPC_NOWAIT*/ );
	}
	if( g.pLocalEventThread && IsThisThread( g.pLocalEventThread ) )
	{
#ifdef LOG_LOCAL_EVENT
		lprintf( WIDE("External handle local event messages...") );
#endif
		// if this is thE thread... chances are someone can wake it up
		// and it is allowed to go to sleep.  This thread is indeed wakable
		// by normal measures.
		return HandleEvents( g.msgq_local, (PQMSG)MessageBuffer, 0/*IPC_NOWAIT*/ );
	}
	return -1;
}



MSGCLIENT_NAMESPACE_END

//-------------------------------------------------------------
