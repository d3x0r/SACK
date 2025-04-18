
/*
 *  Bard Client libarary, handles interfacing to bard service from applications.
 *
 *
 * Creator: Jim Buckeyne
 *
 * (c) 2006
 *
 */

#include <stdhdrs.h>
#include <msgclient.h>
#include <bard.h>
#include "msgid.h"

typedef struct event_dispatch_tag
{
	struct {
		uint32_t bSimple : 1; // simple event...
	} flags;
	uint32_t EventID;
	char *name;
	struct {
		// extra data attached to simple event message from network
		// is passed here (timestamp)
		struct {
			void (CPROC *proc)(uintptr_t,char *extra);
         uintptr_t psv;
		} simple;
	} dispatch;
} EVENT_DISPATCHER, *PEVENT_DISPATCHER;

typedef struct bard_client_local_tag
{
	struct {
		uint32_t connected : 1;
		uint32_t disconnected : 1;
	} flags;
	uint32_t MsgBase;
   PLIST dispatchers;
} LOCAL;
static LOCAL l;

static int CPROC EventHandler( uint32_t EventMsg, uint32_t *data, uint32_t length );

static int ConnectToServer( void )
{
	if( l.flags.disconnected )
      return FALSE;
   if( !l.flags.connected )
   {
      if( InitMessageService() )
      {
         l.MsgBase = LoadService( BARD_SERVICE_NAME, EventHandler );
         Log1( BARD_SERVICE_NAME" message base is %d", l.MsgBase );
         if( l.MsgBase != INVALID_INDEX )
            l.flags.connected = 1;
      }
   }
   if( !l.flags.connected )
      Log( "Failed to connect" );
   return l.flags.connected;
}


uintptr_t CPROC ReconnectThread( PTHREAD thread )
{
	while( 1 )
	{
		lprintf( "attempt connection..." );
		if( ConnectToServer() )
		{
			PEVENT_DISPATCHER event;
			INDEX idx;
			lprintf( "success, re-register events." );
			LIST_FORALL( l.dispatchers, idx, PEVENT_DISPATCHER, event )
			{
				if( event->flags.bSimple )
				{
					uint32_t result[1];
					uint32_t response;
					uint32_t null = 0;
					uint32_t reslen = sizeof( result );
					if( TransactServerMultiMessage( MSG_RegisterSimpleEvent + l.MsgBase, 2
															, &response, result, &reslen
															, &event->EventID, sizeof( event->EventID )
															, event->name?event->name                :(char*)&null
															, event->name?(strlen( event->name ) + 1):(sizeof(null))
															)
						&& ( response == ( (l.MsgBase+MSG_RegisterSimpleEvent) | SERVER_SUCCESS ) )
					  )
					{
                  // goood - nothing to do
					}
					else
					{
						lprintf( "failed to re-register event: %s", event->name  );
					}

				}
			}
			// have to assume that everything went okay...
         lprintf( "finisihed registering, end thread." );
         break;
		}
		else
		{
         lprintf( "Failed to connect to service..." );
			WakeableSleep( 250 );
		}
	}
   return 0;
}


static int CPROC EventHandler( uint32_t EventMsg, uint32_t *data, uint32_t length )
{
	// events for updates are dispatched here, and routed further to the
	// application specified callback.
	switch( EventMsg )
	{
	case MSG_MateEnded:
		l.flags.connected = 0; // not connected anymore :)
		lprintf( "Mate closed... need to re-register when mate comes back up..." );
      ThreadTo( ReconnectThread, 0 );
      break;
	case MSG_DispatchSimpleEvent:
		// data[0] = client event_id;
		// data + 1 = extra information
      lprintf( "dispatch simple event..." );
		{
			PEVENT_DISPATCHER event_dispatch = (PEVENT_DISPATCHER)GetLink( &l.dispatchers, data[0] );
			if( event_dispatch )
			{
				event_dispatch->dispatch.simple.proc( event_dispatch->dispatch.simple.psv
																, (char*)(data + 1) );
			}
		}
		break;
	default:
      lprintf( "Unhandled message..." );
      break;
	}
   return 0;
}

int BARD_RegisterForSimpleEvent ( char *eventname, void (CPROC*eventproc)(uintptr_t,char *extra), uintptr_t psv )
{
	uint32_t result[1];
	uint32_t response;
	uint32_t null = 0;
	uint32_t reslen = sizeof( result );
	PEVENT_DISPATCHER event;
	if( !ConnectToServer() )return 0;
	event = (PEVENT_DISPATCHER)Allocate( sizeof( EVENT_DISPATCHER ) );
	event->name = eventname?StrDup( eventname ):NULL;
	event->dispatch.simple.proc = eventproc;
	event->dispatch.simple.psv = psv;
   event->flags.bSimple = 1;
	AddLink( &l.dispatchers, event );
	event->EventID = FindLink( &l.dispatchers, event );
   // eventID is the ID that this client thinks that this is...
	if( TransactServerMultiMessage( MSG_RegisterSimpleEvent + l.MsgBase, 2
											, &response, result, &reslen
											, &event->EventID, sizeof( event->EventID )
											, event->name?event->name                :(char*)&null
											, event->name?(strlen( event->name ) + 1):(sizeof(null))
											)
		&& ( response == ( (l.MsgBase+MSG_RegisterSimpleEvent) | SERVER_SUCCESS ) )
	  )
	{
      return 1;
	}
   return 0;
}


int BARD_IssueSimpleEvent( char *eventname )
{
   uint32_t null = 0;
	if( !ConnectToServer() )return 0;
	if( !TransactServerMessage( MSG_IssueSimpleEvent + l.MsgBase
									  , eventname?eventname:(char*)&null
									  , eventname?strlen(eventname)+1:sizeof( null )
									  , NULL, NULL, NULL ) )
      return 0;
   return 1;
}




