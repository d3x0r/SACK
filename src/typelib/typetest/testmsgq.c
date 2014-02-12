#include <stdhdrs.h>
#include <sack_types.h>
#define DO_LOGGING
#include <logging.h>
#include <timers.h>

#define MAX_MSGS 500000

#define QUEUE_SIZE 32768

CRITICALSECTION cs;
_32 count = MAX_MSGS;
_32 setsize = 1;
_32 going;
_32 dwNow;
_32 dwSize;
int n;

PTRSZVAL CPROC RunServer( PTHREAD thead )
{
	PMSGHANDLE pmq_in = CreateMsgQueue( WIDE("test in"), QUEUE_SIZE, NULL, 0 );
	PMSGHANDLE pmq_out = CreateMsgQueue( WIDE("test out"), QUEUE_SIZE, NULL, 0 );
	EnterCriticalSec( &cs );
   going = 1;
	if( pmq_in && pmq_out )
	{
		_32 msg[4096];
		_32 msgsize;
      _32 step;
		for( n = 0; n < count; n+=setsize )
		{
         for( step = 0; step < setsize; step++ )
				msgsize = DequeMsg( pmq_in, 0, msg, sizeof( msg ), 0 );
			if( msg[0] != n )
				fprintf( stderr, WIDE("Sequence error: Got %ld expected %d\n"), msg[0], n );
			if( dwNow + 1000 < GetTickCount() )
			{
            dwNow = GetTickCount();
				lprintf( WIDE("client Received %d Messages...%d"), n, msgsize );
			}
         for( step = 0; step < setsize; step++ )
				EnqueMsg( pmq_out, msg, msgsize, 0 );
		}
	}
	LeaveCriticalSec( &cs );
   return 0;
}

PTRSZVAL CPROC RunClient( PTHREAD thead )
{
	PMSGHANDLE pmq_in = CreateMsgQueue( WIDE("test out"), QUEUE_SIZE, NULL, 0 );
	PMSGHANDLE pmq_out = CreateMsgQueue( WIDE("test in"), QUEUE_SIZE, NULL, 0 );
   EnterCriticalSec( &cs );
   going = 1;
	if( pmq_in && pmq_out )
	{
      _32 step;
		_32 msg[4096];
		_32 msgsize = dwSize;
		for( n = 0; n < count; n+=setsize )
		{
			msg[0] = n;
         for( step = 0; step < setsize; step++ )
				EnqueMsg( pmq_out, msg, msgsize, 0 );
         for( step = 0; step < setsize; step++ )
				msgsize = DequeMsg( pmq_in, 0, msg, sizeof( msg ), 0 );
			if( dwNow + 1000 < GetTickCount() )
			{
            dwNow = GetTickCount();
				lprintf( WIDE("server Received %d Messages... %d"), n, msgsize );
			}
		}
	}
	LeaveCriticalSec( &cs );
   return 0;
}

int main( int argc, char **argv )
{
	SetSystemLog( SYSLOG_FILE, stdout );
	SystemLogTime( SYSLOG_TIME_CPU
					 // |SYSLOG_TIME_DELTA
					 );
	if( argc > 1 )
	{
		_32 dwStart;
		if( argc > 2 )
		{
			dwSize = atoi( argv[2] );
			if( dwSize > QUEUE_SIZE/4 )
            dwSize = QUEUE_SIZE/4;
		}
		if( argc > 3 )
		{
			setsize = atoi( argv[3] );
			if( !setsize )
            setsize = 1;
		}
		if( argc > 4 )
		{
			count = atoi( argv[4] );
			if( !count )
				count = 1;
		}
      dwNow = dwStart = GetTickCount();
		if( stricmp( argv[1], WIDE("server") ) == 0 )
		{
         ThreadTo( RunServer, 0 );
		}
		if( stricmp( argv[1], WIDE("client") ) == 0 )
		{
         ThreadTo( RunClient, 0 );
		}
		while( !going )
         Relinquish();
      EnterCriticalSec( &cs );
		{
         _32 dwTime = GetTickCount() - dwStart + 1;
			lprintf( WIDE("Transacted %d messages in %dms = %d/sec, %d bytes/sec")
					 , n
                , dwTime
					 , (n * 1000)/dwTime
					 , (dwSize * n * 1000)/dwTime
					 );
      }
	}
   return 0;
}

