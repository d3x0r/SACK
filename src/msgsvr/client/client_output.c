
#include "global.h"

MSGCLIENT_NAMESPACE

//--------------------------------------------------------------------

int SendInMultiMessageEx( PSERVICE_ROUTE routeID, uint32_t MsgID, uint32_t parts, BUFFER_LENGTH_PAIR *pairs DBG_PASS)
{
	CPOINTER msg;
	size_t len;
	size_t ofs;
	uint32_t param;
	PQMSG MessageOut;
	// shouldn't use MessageOut probably.. ..
	// protect msgout against multiple people.. ..
	EnterCriticalSec( &g.csMsgTransact );
	MessageOut = GetMessageBuffer();
	MessageOut->dest.process_id              = routeID->dest.process_id;
	MessageOut->dest.service_id     = routeID->dest.service_id;
	MessageOut->hdr.source.process_id = routeID->source.process_id;
	MessageOut->hdr.source.service_id = routeID->source.service_id;
	MessageOut->hdr.msgid = MsgID;

	ofs = 0;
	for( param = 0; param < parts; param++ )
	{
		msg = pairs[param].buffer;
		len = pairs[param].len;
		if( len + ofs > 8192 )
		{
		// wow - this is a BIG message - lets see - what can we do?
#ifdef WIN32
#undef SetLastError
			SetLastError( E2BIG );
#else
			errno = E2BIG;
#endif
			_lprintf(DBG_RELAY)( WIDE("Length of message is too big to transport...%") _size_f WIDE(" (len %") _size_f WIDE(" ofs %") _size_f WIDE(")"), len + ofs, len, ofs );
			LeaveCriticalSec( &g.csMsgTransact );
			return FALSE;
		}
		if( msg && len )
		{
			//Log3( WIDE("Adding %d bytes at %d: %08x "), len, ofs, ((uint32_t*)msg)[0] );
			MemCpy( ((char*)QMSGDATA(MessageOut)) + ofs, msg, len );
			ofs += len;
		}
	}
	{
		int stat;
	// send to application inbound queue..
#ifdef DEBUG_OUTEVENTS
		lprintf( WIDE("Sending result to application...") );
		LogBinary( (uint8_t*)MessageOut, ofs );

#endif
		stat = msgsnd( g.msgq_in, MSGTYPE MessageOut, ofs + sizeof(QMSG) - sizeof( MSGIDTYPE ), 0 );
		DropMessageBuffer( MessageOut );
		LeaveCriticalSec( &g.csMsgTransact );
		return !stat;
	}
}
int SendInMultiMessage( PSERVICE_ROUTE routeID, uint32_t MsgID, uint32_t parts, BUFFER_LENGTH_PAIR *pairs )
#define SendInMultiMessage(r,m,parts,pairs) SendInMultiMessageEx(r,m,parts,pairs DBG_SRC )
{
	return SendInMultiMessage( routeID, MsgID, parts, pairs);
}

//--------------------------------------------------------------------

int SendInMessage( PSERVICE_ROUTE routeID, uint32_t MsgID, POINTER buffer, size_t len )
{
	BUFFER_LENGTH_PAIR pair;
	pair.buffer = buffer;
	pair.len = len;
	return SendInMultiMessage( routeID, MsgID, 1, &pair );
}

//--------------------------------------------------------------------

#ifdef _DEBUG_RECEIVE_DISPATCH_
int metamsgrcv( MSGQ_TYPE q, POINTER p, int len, long id, int opt DBG_PASS )
{
	int stat;
	_xlprintf(1 DBG_RELAY)( WIDE("*** Read Message %d"), id);
	stat = msgrcv( q,MSGTYPE p,len,id,opt );
#undef msgrcv
	#define msgrcv(q,p,l,i,o) metamsgrcv(q,p,l,i,o DBG_SRC)
	_xlprintf(1 DBG_RELAY)( WIDE("*** Got message %d"), stat );
	return stat;
}
#endif

static int PrivateSendTransactionResponseMultiMessageEx( PSERVICE_ROUTE DestID
																, uint32_t MessageID, uint32_t buffers
																, BUFFER_LENGTH_PAIR *pairs
																 DBG_PASS )
#define PrivateSendTransactionResponseMultiMessage(d,m,bu,p) PrivateSendTransactionResponseMultiMessageEx(d,m,bu,p DBG_SRC )
{
	CPOINTER msg;
	size_t len, ofs;
	uint32_t param;
	int status;
	PQMSG MessageOut;
	if( g.flags.disconnected )
	{
		_lprintf(DBG_RELAY)( WIDE("Have already disconnected from server... no further communication possible.") );
		return TRUE;
	}
	MessageOut = GetMessageBuffer();

	MessageOut->dest.process_id       = DestID->dest.process_id;
	MessageOut->dest.service_id		  = DestID->dest.service_id;
	MessageOut->hdr.source.process_id = DestID->source.process_id;
	MessageOut->hdr.source.service_id = DestID->source.service_id;

	MessageOut->hdr.msgid             = MessageID;
	// don't know len at this point.. .. ..
	//if( len > 8188 )
	//{
		// wow - this is a BIG message - lets see - what can we do?
		//SetLastError( E2BIG );
		//lprintf( WIDE("Lenght of message is too big to transport...") );
		//return FALSE;
	//}
	ofs = 0;
	//Log1( WIDE("Adding %d params"), buffers );
	for( param = 0; param < buffers; param++ )
	{
		msg = pairs[param].buffer;
		len = pairs[param].len;
		if( len + ofs > 8192 )
		{
		// wow - this is a BIG message - lets see - what can we do?
#ifdef WIN32
			SetLastError( E2BIG );
#else
			errno = E2BIG;
#endif
			_lprintf(DBG_RELAY)( WIDE("Length of message is too big to transport...%") _size_f WIDE(" (len %") _size_f WIDE(" ofs %") _size_f WIDE(")"), len + ofs, len, ofs );
			return FALSE;
		}
		if( msg && len )
		{
			//Log3( WIDE("Adding %d bytes at %d: %08x "), len, ofs, ((uint32_t*)msg)[0] );
			MemCpy( ((char*)QMSGDATA( MessageOut )) + ofs, msg, len );
			ofs += len;
		}
	}
	// subtract 4 from the offset (the msg_id is not counted)
	//Log2( WIDE("Sent %d  (%d) bytes"), g.MessageOut[0], ofs - sizeof( MSGIDTYPE ) );
	// 0 success, non zero failure - return notted state
				  //lprintf( WIDE("Send Message. %08lX"), *(uint32_t*)g.MessageOut );
	//_xlprintf( 1 DBG_RELAY )( "blah." );
#ifdef LOG_SENT_MESSAGES
	_lprintf(DBG_RELAY)( "Send is %d", ofs );
#endif
	status = !msgsnd( g.msgq_out, MSGTYPE MessageOut, ofs + sizeof( QMSG )- sizeof( MSGIDTYPE ), 0 );
	DropMessageBuffer( MessageOut );
	return status;
}



CLIENTMSG_PROC( int, SendServerMultiMessage )( PSERVICE_ROUTE RouteID, uint32_t MessageID, uint32_t buffers, ... )
{
	BUFFER_LENGTH_PAIR *pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * buffers );
	uint32_t n;
	va_list args;
	int status;
	va_start( args, buffers );
	for( n = 0; n < buffers; n++ )
	{
		pairs[n].buffer = va_arg( args, POINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
	status = PrivateSendTransactionResponseMultiMessage( RouteID, MessageID, buffers, pairs );
	Release( pairs );
	return status;
}

CLIENTMSG_PROC( int, SendRoutedServerMultiMessage )( PSERVICE_ROUTE RouteID, uint32_t MessageID, uint32_t buffers, ... )
{
	int status;
	BUFFER_LENGTH_PAIR *pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * buffers );
	uint32_t n;
	va_list args;
	va_start( args, buffers );
	for( n = 0; n < buffers; n++ )
	{

		pairs[n].buffer = va_arg( args, POINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
	status = PrivateSendTransactionResponseMultiMessage( RouteID, MessageID, buffers, pairs );
	Release( pairs );
	return status;
}

CLIENTMSG_PROC( int, SendRoutedServerMessage )( PSERVICE_ROUTE RouteID, uint32_t MessageID, POINTER buffer, size_t len )
{
	int status;
	BUFFER_LENGTH_PAIR pair;
	pair.buffer = buffer;
	pair.len = len;
	status = PrivateSendTransactionResponseMultiMessage( RouteID, MessageID, 1, &pair );
	return status;
}

CLIENTMSG_PROC( int, SendServerMessage )( PSERVICE_ROUTE RouteID, uint32_t MessageID, POINTER msg, size_t len )
{
	BUFFER_LENGTH_PAIR pair;
	int status;
	pair.buffer = msg;
	pair.len = len;
	status = PrivateSendTransactionResponseMultiMessage( RouteID, MessageID, 1, &pair );
	return status;
}


// returns FALSE on timeout, else success.
// this is used by msg.core.dll - used for forwarding messages
// to real handlers...
CLIENTMSG_PROC( int, TransactRoutedServerMultiMessageEx )( PSERVICE_ROUTE RouteID
																			, MSGIDTYPE MsgOut, uint32_t buffers
																			, MSGIDTYPE *MsgIn
																			, POINTER BufferIn, size_t *LengthIn
																			, uint32_t timeout
																			// buffer starts arg list, length is
																			// not used, but is here for demonstration
																			, ... )
{
	BUFFER_LENGTH_PAIR *pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * buffers );
	uint32_t n;
	va_list args;
	SLEEPER sleeper;
	int status;
	PTRANSACTIONHANDLER handler = GetTransactionHandler( RouteID );

	// can send the message, but may not get another responce
	// until the first is done...
	if( MsgIn || BufferIn )
	{
		if( !handler )
		{
			lprintf( WIDE("We have no business being here... no loadservice has been made to this service!") );
			return 0;
		}
		//lprintf( WIDE("Enter %p"), handler );
		EnterCriticalSec( &handler->csMsgTransact );
		switch( MsgOut )
		{
		case RU_ALIVE:
			//lprintf( WIDE("Lying about message to expect") );
			handler->LastMsgID = IM_ALIVE;
			break;
		default:
			//lprintf( WIDE("set last msgID %") _MsgID_f, MsgOut );
			handler->LastMsgID = MsgOut;
			break;
		}
		if( ( handler->MessageID = MsgIn ) )
			(*MsgIn) = handler->LastMsgID;
		handler->wait_for_responce = timeGetTime() + (timeout?timeout:DEFAULT_TIMEOUT);
	}
	//lprintf( WIDE("transact message...") );
	va_start( args, timeout );
	for( n = 0; n < buffers; n++ )
	{
		pairs[n].buffer = va_arg( args, POINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
	QueueWaitReceiveServerMsg( &sleeper, handler
											, MsgIn
											, BufferIn
											, LengthIn
											 DBG_SRC );
	if( !( PrivateSendTransactionResponseMultiMessage( RouteID, MsgOut, buffers
															  , pairs ) ) )
	{
		DeleteLink( &g.pSleepers, &sleeper );	
		Release( pairs );
		if( handler )
		{
			handler->flags.waiting_for_responce = 0;
			LeaveCriticalSec( &handler->csMsgTransact );
		}
 		return FALSE;
	}
	Release( pairs );
	//lprintf( WIDE("Entering wait after serving a message...") );
	if( MsgIn || (BufferIn && LengthIn) )
	{
		status = WaitReceiveServerMsg( &sleeper, MsgOut DBG_SRC );
	}
	else
	{
		handler->flags.waiting_for_responce = 0;
		LeaveCriticalSec( &handler->csMsgTransact );
		status = TRUE;
	}
	DeleteLink( &g.pSleepers, &sleeper );	
	return status;
}

struct debug_transact {
	CTEXTSTR pFile;
	int nLine;
}next_transact;

#undef TransactServerMultiMessage
CLIENTMSG_PROC( int, TransactServerMultiMessage )( PSERVICE_ROUTE RouteID, MSGIDTYPE MsgOut, uint32_t buffers
										, MSGIDTYPE *MsgIn, POINTER BufferIn, size_t *LengthIn
										 // buffer starts arg list, length is
										 // not used, but is here for demonstration
										, ... )
{
	SLEEPER sleeper;
	BUFFER_LENGTH_PAIR *pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * buffers );
	PTRANSACTIONHANDLER handler = GetTransactionHandler( RouteID );
	uint32_t n;
	va_list args;
	int stat;
	va_start( args, LengthIn );
	for( n = 0; n < buffers; n++ )
	{
		pairs[n].buffer = va_arg( args, CPOINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
	//if( MsgOut == 0x23f )
	//	DebugBreak();
	QueueWaitReceiveServerMsg( &sleeper, handler
											, MsgIn
											, BufferIn
											, LengthIn
											 DBG_SRC );
#ifdef LOG_SENT_MESSAGES
	lprintf( WIDE( "%s(%d):Sending message..." ), next_transact.pFile, next_transact.nLine );
#endif
	switch( MsgOut )
	{
	case RU_ALIVE:
		lprintf( WIDE("Lying about message to expect") );
		handler->LastMsgID = IM_ALIVE;
		break;
	default:
		//lprintf( WIDE("set last msgID %ld"), MsgOut );
		handler->LastMsgID = MsgOut;
		break;
	}
#if defined( _DEBUG ) || defined( _DEBUG_INFO )
	if( !( PrivateSendTransactionResponseMultiMessageEx( RouteID, MsgOut, buffers
																	, pairs
																	, next_transact.pFile, next_transact.nLine
																	) ) )
#else
	if( !( PrivateSendTransactionResponseMultiMessageEx( RouteID, MsgOut, buffers
																	, pairs
																	
																	) ) )
#endif
	{
		//lprintf( WIDE("Leaving...") );
		//handler->flags.wait_for_responce = 0;
		//LeaveCriticalSec( &handler->csMsgTransact );
		DeleteLink( &g.pSleepers, &sleeper );	
		Release( pairs );
		return FALSE;
	}

	Release( pairs );
	handler->wait_for_responce = timeGetTime() + (DEFAULT_TIMEOUT);
	//lprintf( WIDE("waiting... %p"), handler );
	if( MsgIn || (BufferIn && LengthIn) )
		stat = WaitReceiveServerMsg( &sleeper, MsgOut DBG_SRC );
	else
	{
		handler->flags.waiting_for_responce = 0;
		LeaveCriticalSec( &handler->csMsgTransact );
		stat = TRUE;
	}
	DeleteLink( &g.pSleepers, &sleeper );	
	//lprintf( WIDE("Done %p %d"),handler, stat );
	return stat;

}

// buffer starts arg list, length is
// not used, but is here for demonstration
CLIENTMSG_PROC( TSMMProto, TransactServerMultiMessageExEx )( DBG_VOIDPASS )
{
#ifdef LOG_SENT_MESSAGES
#  ifdef _DEBUG
	next_transact.pFile = pFile;
	next_transact.nLine = nLine;
#  endif
#endif
	return TransactServerMultiMessage;
}


CLIENTMSG_PROC( int, TransactServerMultiMessageEx )( PSERVICE_ROUTE RouteID, MSGIDTYPE MsgOut, uint32_t buffers
																	, MSGIDTYPE *MsgIn, POINTER BufferIn, size_t *LengthIn
																	, uint32_t timeout
																	 // buffer starts arg list, length is
																	 // not used, but is here for demonstration
																	, ... )
{
	SLEEPER sleeper;
	BUFFER_LENGTH_PAIR *pairs = (BUFFER_LENGTH_PAIR*)Allocate( sizeof( BUFFER_LENGTH_PAIR ) * buffers );
	PTRANSACTIONHANDLER handler = GetTransactionHandler( RouteID );
	uint32_t n;
	va_list args;
	int stat;
	va_start( args, timeout );
	for( n = 0; n < buffers; n++ )
	{
		pairs[n].buffer = va_arg( args, POINTER );
		pairs[n].len = va_arg( args, uint32_t );
	}
	QueueWaitReceiveServerMsg( &sleeper, handler
											, MsgIn
											, BufferIn
											, LengthIn
											 DBG_SRC );	
	if( !(PrivateSendTransactionResponseMultiMessage( RouteID, MsgOut, buffers
															 , pairs ) ) )
	{
		DeleteLink( &g.pSleepers, &sleeper );	
		Release( pairs );
		handler->flags.waiting_for_responce = 0;
		LeaveCriticalSec( &handler->csMsgTransact );
		return FALSE;
	}
	Release( pairs );
	handler->wait_for_responce = timeGetTime() + (timeout?timeout:DEFAULT_TIMEOUT);
	if( MsgIn || (BufferIn && LengthIn) )
		stat = WaitReceiveServerMsg( &sleeper, MsgOut DBG_SRC );
	else
	{
		handler->flags.waiting_for_responce = 0;
		LeaveCriticalSec( &handler->csMsgTransact );
		stat = TRUE;
	}
	DeleteLink( &g.pSleepers, &sleeper );	
	return stat;

}

//--------------------------------------------------------------------

CLIENTMSG_PROC( int, TransactServerMessageEx)( PSERVICE_ROUTE RouteID, MSGIDTYPE MsgOut, CPOINTER BufferOut, size_t LengthOut
						  , MSGIDTYPE *MsgIn, POINTER BufferIn, size_t *LengthIn DBG_PASS )
{
	return TransactServerMultiMessageExEx(DBG_VOIDRELAY)( RouteID, MsgOut, 1, MsgIn, BufferIn, LengthIn
												, BufferOut, LengthOut );
}

//--------------------------------------------------------------------

CLIENTMSG_PROC( int, TransactServerMessageExx)( PSERVICE_ROUTE RouteID, MSGIDTYPE MsgOut, CPOINTER BufferOut, size_t LengthOut
															 , MSGIDTYPE *MsgIn, POINTER BufferIn, size_t *LengthIn
															  , uint32_t timeout DBG_PASS )
{
#ifdef LOG_SENT_MESSAGES
#  ifdef _DEBUG
	next_transact.pFile = pFile;
	next_transact.nLine = nLine;
#  endif
#endif
	return TransactServerMultiMessageEx( RouteID, MsgOut, 1, MsgIn, BufferIn, LengthIn, timeout
												  , BufferOut, LengthOut );
}


MSGCLIENT_NAMESPACE_END

