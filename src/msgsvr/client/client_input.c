
#include "global.h"

MSGCLIENT_NAMESPACE

PTRANSACTIONHANDLER GetTransactionHandler( PSERVICE_ROUTE route )
{
	PTRANSACTIONHANDLER handler = g.pTransactions;
	while( handler )
	{
		if( handler->route == route )
			break;
		handler = NextThing( handler );
	}
	if( !handler )
	{
		handler = New( TRANSACTIONHANDLER );
		MemSet( handler, 0, sizeof( TRANSACTIONHANDLER ) );
		InitializeCriticalSec( &handler->csMsgTransact );
		handler->route = route;
		LinkThing( g.pTransactions, handler );
	}
	return handler;
}


//--------------------------------------------------------------------

static int GetAMessageEx( MSGQ_TYPE msgq, MSGIDTYPE MsgFilter, CTEXTSTR q, int flags DBG_PASS )
#define GetAMessage(m,x,f) GetAMessageEx(m,x,_WIDE(#m),f DBG_SRC)
{
	//int bLog = 0;
	if( IsThisThread( g.pThread ) )
	{
		int logged = 0;
		PQMSG MessageIn = GetMessageBuffer();
		int MessageLen;
		
		do
		{
			//if( bLog )
#ifdef DEBUG_THREADS
			lprintf( "Attempt to recieve for %08lx %p", MsgFilter, msgq );
#endif
			//lprintf( "vvv" );
			MessageLen = msgrcv( msgq, MSGTYPE MessageIn, 8192, MsgFilter, flags );
#ifdef DEBUG_DATA_XFER
			LogBinary( (uint8_t*)MessageIn, MessageLen + sizeof( MSGIDTYPE ) );
#endif
			//lprintf( "^^^" );
			//lprintf( "Got a receive..." );
			if( ( MessageLen == (-((int32_t)sizeof(MSGIDTYPE))) ) ) // retry
			{
				lprintf( "Recieved -4 message (no data?!) no message, should have been -1, ENOMSG" );
				MessageIn->hdr.msgid = RU_ALIVE;
				continue; // continue on do-while checks while condition - gofigure.
			}
			if( MessageLen == -1 )
			{
#ifdef _WIN32
				int my_errno = GetLastError();
#  ifdef errno
#    undef errno
#  endif
#  define errno my_errno
#endif
				if( errno == ENOMSG )
				{
					lprintf( "No message... nowait was set?" );
					return 0;
				}
				else if( errno == EIDRM )
				{
					lprintf( "No message... Message queue removed" );
					return -1;
				}
				else
				{
					if( errno == EINTR ){
						//bLog = 1;
						//lprintf( "Error Interrupt - that's okay..." );
					}
					else if( errno == EINVAL )
					{
						lprintf( "msgrecv on q %d is invalid! open it. or what is %" _MsgID_f "(%08" _MsgID_f ") or %08d"
								 , msgq, g.my_message_id, g.my_message_id, flags );
					}
					else
					{
						xlprintf( LOG_ALWAYS )( "msgrcv resulted in error: %d", errno );
					}
					MessageIn->hdr.msgid = RU_ALIVE; // loop back around.
				}
			}
			else
			{
				if( MessageIn->dest.process_id == 0 )
				{
					lprintf( "---------- DO NOT BE HERE ----------------" );
					HandleCoreMessage( MessageIn, (MessageLen-(sizeof(QMSG)-sizeof(MSGIDTYPE))) DBG_SRC );
				}
			}
		}
		while( !g.flags.disconnected
				&& MessageIn->hdr.msgid == RU_ALIVE );  // RU_ALIVE receives are no receive.

#ifdef DEBUG_THREADS
		lprintf( "Responce received...%08lX", MessageIn->hdr.msgid );
#endif
		if( !g.flags.disconnected )
		{
			int cnt = 0;
			INDEX idx;
			PSLEEPER sleeper;
#ifdef DEBUG_THREADS
			lprintf( "waking sleepers." );
#endif
			LIST_FORALL( g.pSleepers, idx, PSLEEPER, sleeper )
			{
				if( (MessageIn->dest.process_id == sleeper->handler->route->source.process_id )
					&& ( ( ( MessageIn->hdr.msgid & 0xFFFFFFF ) == MSG_ServiceLoad ) 
					    || (MessageIn->dest.service_id == sleeper->handler->route->source.service_id ) ) )
				{
					sleeper->handler->flags.responce_received = 1; // set to enable application to get it's message...
			
					sleeper->handler->MessageIn = MessageIn;
					sleeper->handler->MessageLen = MessageLen;

					cnt++;
#ifdef DEBUG_THREADS
					lprintf( "Wake thread waiting for responces...%p", sleeper->thread  );
#endif
					WakeThread( sleeper->thread );
				}
			}
			if( !cnt )
			{
			//lprintf( "FATALITY - received responce from service, and noone was waiting for it!" );
			//lprintf( "No Sleepers woken - maybe - they haven't gotten around to sleeping yet?" );
			}
		}
	}
	else
	{
		//lprintf( "Not the message thread... exiting quietly... %d %p %d %Ld"
		//	, g.my_message_id
		//	 , g.pThread
		//	 , getpid()
		//		, g.pThread->ThreadID
		//	);
		return 2;
	}
	return 1;
}


//--------------------------------------------------------------------

// the work in this routine is basically to
// find a handler to receive into?

int WaitReceiveServerMsg ( PSLEEPER sleeper
					, uint32_t MsgOut
					DBG_PASS )
{
	PTRANSACTIONHANDLER handler = sleeper->handler;
	if( sleeper->thread )
	{
		int received;
		int IsThread = IsThisThread( g.pThread );
		//lprintf( "waiting for cmd result" );
#ifdef DEBUG_THREAD
		lprintf( "This thread? %s", IsThread?"Yes":"No" );
#endif
		do
		{
			// this library is totally serialized for one transaction
			// at a time, from multiple threads.
			received = 0;
			while( ( handler->flags.bCheckedResponce ||
					  !handler->flags.responce_received ) &&
					(
#ifdef DEBUG_DATA_XFER
#ifdef _DEBUG
					 (lprintf( "Compare %" _32f " vs %" _32f " (=%" _32fs ") (positive keep waiting)"
								, handler->wait_for_responce
								, timeGetTime()
								, handler->wait_for_responce - timeGetTime()  ) ),
#endif
#endif
					( handler->wait_for_responce > timeGetTime() )) ) // wait for a responce
			{
				received = 1;
				// check for responces...
				// will return immediate if is not this thread which
				// is supposed to be there...
#ifdef DEBUG_DATA_XFER
				_lprintf(DBG_RELAY)( "getting or waiting for... a message..." );
#endif
				if( IsThread )
				{
					lprintf( "Get message (might be my thread" );
					if( GetAMessage( g.msgq_in, g.my_message_id, IPC_NOWAIT ) == 2 )
					{
						Log( "Okay - won't check for messages anymore - just wait..." );
						IsThread = 0;
					}
				}
				// seperate test, can decide to not be the thread...
				if( !IsThread )
				{
					if( handler->flags.responce_received && !handler->flags.bCheckedResponce )
					{
						DeleteLink( &g.pSleepers, sleeper );
						goto dont_sleep;
					}
					handler->flags.bCheckedResponce = 0;

					//lprintf( "Going to sleep for %" _32fs
					//		 , handler->wait_for_responce - timeGetTime()
					//		 );
					WakeableSleep( handler->wait_for_responce - timeGetTime() );
					//lprintf( "AWAKE! %d", handler->flags.responce_received );

					DeleteLink( &g.pSleepers, sleeper );
				}
				else
				{
					Relinquish();
				}
			}

       		// timeout...?
			if( !handler->flags.bCheckedResponce )
				received = 1;

			//lprintf( "When we finished this loop still was waiting %" _32fs, handler->wait_for_responce - timeGetTime() );
			//else
			{
				//lprintf( "Excellent... the responce is back before I could sleep!" );
			}
			if( !handler->flags.responce_received )
			{
				Log( "Responce timeout!" );
				handler->flags.waiting_for_responce = 0;
				LeaveCriticalSec( &handler->csMsgTransact );
				return FALSE; // DONE - fail! abort!
			}
			else
			{
				//Log( "Result to application... " );
			}
		dont_sleep: ;
			handler->flags.bCheckedResponce = 1;
			//lprintf( "Read message..." );
			// if the message is a response to me.....
		}
		while( received && ReceiveServerMessageEx( handler, handler->MessageIn, handler->MessageLen DBG_RELAY ) );
		if( received )
		{
			handler->flags.waiting_for_responce = 0;

			//Log2( "Got responce: %08x %d long", *MsgIn, LengthIn?*LengthIn:-1 );
			if( ( *handler->MessageID & 0x0FFFFFFF ) != ( (handler->LastMsgID) & 0x0FFFFFFF ) )
			{
				lprintf( "Mismatched server responce to client message: %" _MsgID_f " to %" _MsgID_f
						 , *handler->MessageID & 0x0FFFFFFF
						 , handler->LastMsgID & 0x0FFFFFFF
						  );
			}
			else
			{
				if( handler->route->dest.process_id == 1 
				  && handler->route->dest.service_id == 0 
				  && MsgOut == MSG_ServiceLoad
				  && ( (*handler->MessageID) & SERVER_SUCCESS ) )
				{
					handler->route->dest = handler->MessageIn->hdr.source; //-V595
				}
			}	
			//lprintf( "Clear received response" );
			//handler->flags.responce_received = 0; // allow more responces to be received.
		}
	}
	if( handler )
	{
		//lprintf( "cleanup...%p", handler );
		handler->flags.waiting_for_responce = 0;
		//LeaveCriticalSec( &handler->csMsgTransact );
	}
	//lprintf( "done waiting..." );
	return TRUE;
}

// the work in this routine is basically to
// find a handler to receive into?

int QueueWaitReceiveServerMsg ( PSLEEPER sleeper, PTRANSACTIONHANDLER handler
										  , MSGIDTYPE *MsgIn
										  , POINTER BufferIn
										  , size_t *LengthIn
											DBG_PASS )
{
	if( MsgIn )
	{
		//lprintf( "waiting for cmd result" );
#ifdef DEBUG_THREAD
		lprintf( "This thread? %s", IsThread?"Yes":"No" );
#endif
		handler->MessageID = MsgIn;
		handler->msg = BufferIn;
		handler->len = LengthIn;
		handler->flags.bCheckedResponce = 0;

		sleeper->thread = MakeThread();
		sleeper->handler = handler;
		AddLink( &g.pSleepers, sleeper );
	}
	else
	{
		sleeper->handler = handler;
		sleeper->thread = NULL;
	}
	//lprintf( "done waiting..." );
	return TRUE;
}


uintptr_t CPROC HandleMessages( PTHREAD thread )
{
	MSGIDTYPE MsgFilter = (MSGIDTYPE)GetThreadParam( thread );
	g.pThread = thread;
#ifdef DEBUG_THREADS
	lprintf( "threadID: %lx", g.my_message_id );
#endif

	g.flags.message_responce_handler_ready = TRUE;
	while( !g.flags.disconnected )
	{
		int r;
		//Log( "enter read a message..." );
		if( ( r = GetAMessage( g.msgq_in, MsgFilter, 0 ) ) < 0 )
		{
			Log( "thread is exiting..." );
			g.flags.message_responce_handler_ready = FALSE;
			break;
		}
		if( r == 2 )
		{
			Log( "THIS thread is no longer THE thread!?!?!?!?!?!" );
			break;
		}
	}
	g.pThread = NULL;
	return 0;
}



MSGCLIENT_NAMESPACE_END

//-------------------------------------------------------------

