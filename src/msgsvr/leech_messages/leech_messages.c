#include <sack_types.h>
#include <logging.h>
#include <stdio.h>

#ifdef __LINUX__
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#if defined( _WIN32 ) || defined( USE_SACK_MSGQ )
	#define IPC_NOWAIT MSGQUE_NOWAIT
	#define ENOMSG MSGQUE_ERROR_NOMSG
	#define EINTR -1
	#define EIDRM -2
#define EINVAL -3
#define E2BIG -4
#ifdef DEBUG_DATA_XFER
#  define msgsnd( q,msg,len,opt) ( lprintf( WIDE("Send Message...") ), LogBinary( (uint8_t*)msg, (len)+4  ), EnqueMsg( q,(msg),(len),(opt)) )
#else
#  define msgsnd EnqueMsg
#endif
	#define msgrcv(q,m,sz,id,o) DequeMsg((PMSGHANDLE)q,&id,m,sz,o)
	#define MSGQSIZE 32768
   #define IPC_CREAT  1
   #define IPC_EXCL   2
#define msgget(name,n,opts) ( (opts) & IPC_CREAT )      \
	? ( ( (opts) & IPC_EXCL)                             \
	  ? ( OpenMsgQueue( name, NULL, 0 )                  \
       ? MSGFAIL                                        \
       : CreateMsgQueue( name, MSGQSIZE, NULL, 0 ))     \
	  : CreateMsgQueue( name, MSGQSIZE, NULL, 0 ))       \
	: OpenMsgQueue( name, NULL, 0 )
	//#define msgget(name,n,opts ) CreateMsgQueue( name, MSGQSIZE, NULL, 0 )
	#define msgctl(n,o,f) 
	#define MSGFAIL NULL
#else
#ifdef DEBUG_DATA_XFER
   //#define msgsnd( q,msg,len,opt) ( _xlprintf(1 DBG_RELAY)( WIDE("Send Message...") ), LogBinary( (POINTER)msg, (len)+4  ), msgsnd( q,(msg),(len),(opt)) )
   #define msgsnd( q,msg,len,opt) ( lprintf( WIDE("Send Message...") ), LogBinary( (POINTER)msg, (len)+4  ), msgsnd( q,(msg),(len),(opt)) )
#endif
	#define msgget( name,n,opts) msgget( n,opts )
   #define MSGFAIL -1
#endif


#define MSGQ_ID_BASE "Srvr"

int main( void )
{
#ifdef __LINUX64__
#define ssize_tf WIDE("lu")
#else
#define ssize_tf WIDE("u")
#endif
	static char buffer[4096];
	ssize_t len;
	int queue;
#ifdef __LINUX__
	key_t key, key2, key3, key4;
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
#ifdef __cplusplus
#define	MSGBUF_TYPE msgbuf*
#else
#define MSGBUF_TYPE POINTER
#endif
	queue = msgget( "", key, 0 );
	if( queue == MSGFAIL )
		lprintf( "failed to open key1" );
	else
	{
		do
		{
			len = msgrcv( queue, (MSGBUF_TYPE)buffer, sizeof( buffer ) - sizeof(long), 0, IPC_NOWAIT );
			if( len != MSGFAIL )
			{
				lprintf( WIDE("Received %") ssize_tf WIDE(" bytes from queue 1")
						 , len );
            LogBinary( (uint8_t*)buffer, len + sizeof( long ) );
			}
		} while( len != MSGFAIL );

	}

   queue = msgget( "", key2, 0 );
	if( queue == MSGFAIL )
		lprintf( "failed to open key2" );
	else
	{
		do
		{
			len = msgrcv( queue, (MSGBUF_TYPE)buffer, sizeof( buffer ) - sizeof(long), 0, IPC_NOWAIT );
			if( len != MSGFAIL )
			{
				lprintf( WIDE("Received %") ssize_tf WIDE(" bytes from queue 2"), len );
            LogBinary( (uint8_t*)buffer, len + sizeof( long ) );
			}
		} while( len != MSGFAIL );
	}

   queue = msgget( "", key3, 0 );
	if( queue == MSGFAIL )
		lprintf( "failed to open key3" );
	else
	{
		do
		{
			len = msgrcv( queue, (MSGBUF_TYPE)buffer, sizeof( buffer ) - sizeof(long), 0, IPC_NOWAIT );
			if( len != MSGFAIL )
			{
				lprintf( WIDE("Received %") ssize_tf WIDE(" bytes from queue 3"), len );
            LogBinary( (uint8_t*)buffer, len + sizeof( long ) );
			}
		} while( len != MSGFAIL );
	}

   queue = msgget( "", key4, 0 );
	if( queue == MSGFAIL )
		lprintf( "failed to open key4" );
	else
	{
		do
		{
			len = msgrcv( queue, (MSGBUF_TYPE)buffer, sizeof( buffer ) - sizeof(long), 0, IPC_NOWAIT );
			if( len != MSGFAIL )
			{
				lprintf( WIDE("Received %") ssize_tf WIDE(" bytes from queue 4"), len );
            LogBinary( (uint8_t*)buffer, len + sizeof( long ) );
			}
		} while( len != MSGFAIL );
	}
   return 0;
}



#else
int main( void )
{
	lprintf( "No function for windows." );
	printf( "No function for windows.\n" );
   return 0;
}
#endif
