#include <stdhdrs.h>
#include <msgclient.h>
#include <timers.h>


#define MSG_Test1 (MSG_EventUser)
#define MSG_Test2 (MSG_Test1+1)

PSERVICE_ROUTE BaseID, OtherBaseID;
uint32_t last_tick;

int CPROC EventHandler( MSGIDTYPE Msg, uint32_t *params, size_t paramlen )
{

	uint32_t tick;
	printf( WIDE("Got event %08lx at %ld\n"), Msg, ( tick = GetTickCount() ) - last_tick );
	last_tick = tick;
	LogBinary( (POINTER)params, paramlen );
	return 0;
}

uint32_t nlongmsgs;
uintptr_t CPROC LongWaitTrans( PTHREAD thread )
{
	while( 1 )
	{
		uint32_t buffer[32];
		uint32_t result[32];
		size_t buflen, reslen;
		MSGIDTYPE  responce;
		buffer[0] = 0x12345678;
		buffer[1] = 0xabcdef01;
		buffer[2] = 0xdeafbabe;
		buflen = 12;
		reslen = 12;
		if( !TransactServerMessage( OtherBaseID, MSG_Test2
										  , buffer, buflen
										  , &responce, result, &reslen ) )
		{
			printf( WIDE("Transact message generally failed - suppose that's a timeout.\n") );
		}
		else
		{
			if( responce & 0x80000000 )
				printf( WIDE("Responce was faillure from the server...\n") );
			else
			{
				nlongmsgs++;
			}
		}
	}
   return 0;
}

SaneWinMain( argc, argv )
{
	// dumps to log.
	DumpServiceList();
	if( argc > 1 )
	{
		TEXTCHAR logname[64];
		if( argc > 2 )
			snprintf( logname, 64, WIDE("service_client_%s.log"), argv[2] );
		else
			snprintf( logname, 64, WIDE("service_client_%s.log"), argv[1] );
		SetSystemLog( SYSLOG_FILENAME, logname );
	}
	if( argc > 3 )
	{
		OtherBaseID = LoadService( argv[3], EventHandler );
		ThreadTo( LongWaitTrans, 0 );
	}
	else
	{
		OtherBaseID = LoadService( WIDE("Test Service 1"), EventHandler );
		if( OtherBaseID )
		{
			ThreadTo( LongWaitTrans, 0 );
		}
		else
			lprintf( WIDE("Failed to connect to first service") );
	}
	BaseID = LoadService( (argc < 2)?WIDE("Test Service 1"):argv[1], EventHandler );
	if( BaseID )
	{
		uint32_t msgs = 0;
		uint32_t endat = GetTickCount() + 3000;
		printf( WIDE("Starting 3 second wait... sending messages and validating responses...\n") );
		while( endat > GetTickCount() )
		{
			uint32_t buffer[32];
			uint32_t result[32];
			size_t buflen, reslen;
			MSGIDTYPE responce;
			buffer[0] = 0x12345678;
			buffer[1] = 0xabcdef01;
			buffer[2] = 0xdeafbabe;
			buflen = 12;
			reslen = 12;
			if( !TransactServerMessage( BaseID, MSG_Test1
											  , buffer, buflen
											  , &responce, result, &reslen ) )
			{
				printf( WIDE("Transact message generally failed - suppose that's a timeout.\n") );
			}
			else
			{
				if( responce & 0x80000000 )
					printf( WIDE("Responce was faillure from the server...\n") );
				else
				{
					msgs++;
				}
			}
		}
		printf( WIDE("Did %d short and %d long pause messages in %ld.%03ld seconds (or so)\n"), msgs, nlongmsgs
				, ( GetTickCount() - endat + 3000 ) / 1000
				, ( GetTickCount() - endat + 3000 ) % 1000
				);
		// wait some time for the event test to complete....
		WakeableSleep( 1000 );
		printf( WIDE("Waited 1 second (should get responces in microseconds)\n") );
	}
	else
		printf( WIDE("Failed to connect to second service\n") );
   return 0;
}
EndSaneWinMain()
