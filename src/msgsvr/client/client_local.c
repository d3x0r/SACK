
#include "global.h"

MSGCLIENT_NAMESPACE




//--------------------------------------------------------------------

PTRSZVAL CPROC HandleLocalEventMessages( PTHREAD thread )
{
	g.pLocalEventThread = thread;
	g.flags.local_events_ready = TRUE;
	//g.my_message_id = getpid(); //(_32)( thread->ThreadID & 0xFFFFFFFF );
	while( !g.flags.disconnected )
	{
		int r;
		if( thread == g.pLocalEventThread )
		{
			// thread local storage :)
			static int levels;
			static _32 *pBuffer;
			static _32 MessageEvent[2048]; // 8192 bytes
			if( !levels )
				pBuffer = MessageEvent;
			else
				pBuffer = (_32*)Allocate( sizeof( MessageEvent ) );
			levels++;
			//lprintf( WIDE("---- GET A LOCAL EVENT!") );
			if( ( r = HandleEvents( g.msgq_local, (PQMSG)pBuffer, 0 ) ) < 0 )
			{
				Log( WIDE("EventHandler has reported a fatal error condition.") );
				break;
			}
			levels--;
			if( levels )
				Release( pBuffer );
		}
		else if( r == 2 )
		{
			Log( WIDE("Thread has been restarted.") );
			// don't clear ready or main event flag
			// things.
			return 0;
		}
	}
	g.flags.local_events_ready = FALSE;
	g.pLocalEventThread = NULL;
	return 0;
}

MSGCLIENT_NAMESPACE_END
