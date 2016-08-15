#include <stdhdrs.h>
#include <sack_types.h>
#define DO_LOGGING
#include <logging.h>
#include <timers.h>

#define MAX_MSGS 500000

#define QUEUE_SIZE 32768

CRITICALSECTION cs;
uint32_t count = MAX_MSGS;
uint32_t setsize = 1;
uint32_t going;
uint32_t dwNow;
uint32_t dwSize;
int n;

uintptr_t CPROC RunServer( PTHREAD thead )
{
	PMSGHANDLE pmq_in = CreateMsgQueue( WIDE("test in"), QUEUE_SIZE, NULL, 0 );
	PMSGHANDLE pmq_out = CreateMsgQueue( WIDE("test out"), QUEUE_SIZE, NULL, 0 );
	EnterCriticalSec( &cs );
   going = 1;
	if( pmq_in && pmq_out )
	{
		uint32_t msg[4096];
		uint32_t msgsize;
      uint32_t step;
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

uintptr_t CPROC RunClient( PTHREAD thead )
{
	PMSGHANDLE pmq_in = CreateMsgQueue( WIDE("test out"), QUEUE_SIZE, NULL, 0 );
	PMSGHANDLE pmq_out = CreateMsgQueue( WIDE("test in"), QUEUE_SIZE, NULL, 0 );
   EnterCriticalSec( &cs );
   going = 1;
	if( pmq_in && pmq_out )
	{
      uint32_t step;
		uint32_t msg[4096];
		uint32_t msgsize = dwSize;
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
		uint32_t dwStart;
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
         uint32_t dwTime = GetTickCount() - dwStart + 1;
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

