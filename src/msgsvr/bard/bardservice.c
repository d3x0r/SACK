// bardservice.c is the main for bardservice.
#define BARD_SERVICE_NETWORK_PORT 15235


//*******************************************************************
//**                                                               **
//**  BARD Service - Button Application Remote Database Service    **
//**                creates  "bardservice"                         **
//**                                                               **
//**      (C) Copyright 2005 Freedom Collective                    **
//**      All Rights Reserved.                                     **
//**                                                               **
//*******************************************************************

#include <stdhdrs.h>
#include <construct.h>
#include <network.h>
#include <system.h>
//for the msgsvr.  needs msgclient library
#include <msgclient.h>
#include <systray.h>
#include <idle.h>
#include <timers.h>

#include <bard.h>
#include "msgid.h"

typedef struct received_event_tag
{
	char *data;
   _32 tick;
}RECEIVED_EVENT, *PRECEIVED_EVENT;

typedef struct simple_event_tag
{
	_32 source_id;
   _32 event_id;
   char *name;
   SOCKADDR *sa;
} SIMPLE_EVENT, *PSIMPLE_EVENT;

typedef struct simple_remote_event_tag
{
	// a simple event was registered for receiption on a remote
	// system... when this event is dispatched there will be a sequence ID
	// the event will be sent directly to this, but also broadcast to all systems
	// if the event was received directed, then that service is registered..
	// if the even is received on the broadcast, then the service may attempt to
   // renotify of its own interest in this message. (in addition to dispatching to application)
	char *name;
   SOCKADDR *sa;
} SIMPLE_REMOTE_EVENT, *PSIMPLE_REMOTE_EVENT;

typedef struct bard_local_tag
{
	struct {
		_32 bExit : 1;
	} flags;
	_32 MsgBase;  //the BaseID is generated from Registering as a Service.
	LOGICAL bExit;  //The MiscExit sets this to false, which causes fallout from main loop and exit.
	PTHREAD pMainThread; // the main thread pointer.
	PCLIENT pcService;  // a UDP socket to send and receive events to other peer BARD services
   SOCKADDR *saBroadcast; // a generally used address for SendUDP
	SOCKADDR *saMe; // the socket address that I'm sending from
#ifdef WIN32
	HWND hWnd;
#endif
	PLIST simple_events; // list of PSIMPLE_EVENTs
   PLIST received_events; // list of events that have been received (to avoid multiple processsing)
} LOCAL;

static LOCAL l;


static void ExitBardService( void )
{
	xlprintf(LOG_ALWAYS)( WIDE("ExitBardService set bExit from %ld to TRUE"), l.flags.bExit);
	l.flags.bExit=TRUE;
	WakeThread( l.pMainThread );
}


//************************************************************************
//  HandleConnectionToThisService
//
//
//
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//             _32 *params   A pointer to anything, this is Application Defined, too.
//             _32 param_length  The length of the data pointed to.
//             _32 *result   The callback must write to this buffer to acknowledge the event.
//             _32 *result_length The length of the acknowledge.
//
//
//************************************************************************

static int CPROC HandleConnectionToThisService( _32 *params, _32 param_length
															 , _32 *result, _32 *result_length )
{
	xlprintf(LOG_ALWAYS)("MessageHandler:MateStarted" );
	// params[-1] is the source ID of this message
	// this is a unique ID for each client that has a connection to this service
	// it serves as a unique handle.

	// create a local structure to track this client's structures
	// identify each structure with params[-1] such that should the client
   // disconnect, the resources will be freed.


	// then when resulting, return with the maximum number of messages which
	// are expected to be handled by this service (requested by a client).
	result[0] = MSG_LASTMESSAGE;
	// also result with the number of unique event messages this service will generate
	result[1] = MSG_LASTEVENT;
   // result with the size of the result correctly.
	(*result_length) = sizeof( _32[2] );
   // return success code
   return 1;
}


static int CPROC HandleDisconnectionFromThisService( _32 *params, _32 param_length
																	, _32 *result, _32 *result_length )
{
	xlprintf(LOG_ALWAYS)("MessageHandler:MateEnded");
	// params[-1] is the source ID of this message
	// this is a unique ID for each client that has a connection to this service
	// it serves as a unique handle.

   // find local structures identified [owned] by params[-1], and destroy.

	{
		PSIMPLE_EVENT event;
      INDEX idx;
		LIST_FORALL( l.simple_events, idx, PSIMPLE_EVENT, event )
		{
			if( event->source_id == params[-1] )
			{
				lprintf( WIDE("Found a client regisered... deleting %s"), event->name );
				SetLink( &l.simple_events, idx, NULL );
            Release( event );
			}
		}
	}

   if( result_length ) // a close never expects a return, and will pass a NULL for the length.
		(*result_length) = 0; // but if it happens to want a length, set NO length.
	// return success (if failure, probably does not matter, this is disregarded due to the nature
	// what this message is - the disconnect of a client that is no longer (or will shortly be
	// no longer there) )
	return 1;
}

static void SendSimpleMessage( char *type, char *data )
{
	static char msg[512];
	int ofs;
	int n;
	while( msg[0] )
      Relinquish();
	ofs = snprintf( msg, sizeof( msg ), WIDE("%08lx:%s:%s"), GetTickCount(), type, data );
	lprintf( WIDE("msg: %s"), msg );
	// and all near, registered clients also need this message...
	for( n = 0; n < 10; n++ )
	{
		SendUDPEx( l.pcService
					, msg
					, ofs+1
					, l.saBroadcast );
      Relinquish();
	}
   msg[0] = 0;
}


//message server 07062005

static void DispatchSimpleEvent( char *eventname )
{
	INDEX idx;
   PSIMPLE_EVENT event;
	LIST_FORALL( l.simple_events, idx, PSIMPLE_EVENT, event )
	{
		int skip = 0;
		if( event->name )
		{
			// break incoming name at a : following data
			char *p = strchr( eventname, ':' );
			if( p )
				skip = p-eventname;
			else
            skip = strlen( eventname );

         // test exact name match of registered event->name....
			//skip = strlen( event->name );
		}
		lprintf( WIDE("Event: %s  eventname: %s %d %d"), event->name, eventname, skip, strncmp( eventname, event->name, skip ) );
		if( !event->name ||
			( skip
			  ? ( strncmp( eventname, event->name, skip ) == 0 )
			  : ( strcmp( eventname, event->name ) == 0 ) )
		  )
		{
         //lprintf( WIDE("dsipatch...") );
			SendMultiServiceEvent( event->source_id
									  , l.MsgBase + MSG_DispatchSimpleEvent, 2
									  , &event->event_id, sizeof( event->event_id )
										, eventname + skip + 1
										, (strlen( eventname ) - (skip + 1)) + 1
									  );
		}
	}
}


//************************************************************************
//  HandleRegisterSimpleEvent - Register a Simple Event
//
//
//
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//             _32 *params
//                 [0] = client event id
//                  +1 = a character string
//             _32 param_length  The length of the string pointed to.
//             _32 *result
//                        result[0] = a unique handle to the event(not 0)
//             _32 *result_length  sizeof( result[0] )
//
//
//************************************************************************

// event_id is unused?
// source_id is the message service ID that wants this event...
// saSource is the network address of a system that has someone that wants this event...
// name is the event name...
void CreateSimpleEvent( _32 event_id, _32 source_id, SOCKADDR *saSource, char *name )
{
	// identify the client as a wanting recipient of this message
	// (by name)
   PSIMPLE_EVENT simple_event = (PSIMPLE_EVENT)Allocate( sizeof( SIMPLE_EVENT ) );
	simple_event->source_id = source_id;
	simple_event->event_id = event_id;
	simple_event->sa = saSource; // if it comes in from a network registration...
   if( name )
		simple_event->name = StrDup( name );
	else
		simple_event->name = NULL;
	AddLink( &l.simple_events, simple_event );
	// tell everyone else that we want that event...
	if( source_id && !saSource )
	{
      // only when we create a new event that a client desires...
		SendSimpleMessage( "register simple event", name );
	}
}


static int CPROC HandleRegisterSimpleEvent( _32 *params, _32 param_length
														, _32 *result, _32 *result_length )
{
	// identify the client as a wanting recipient of this message
	// (by name)
	CreateSimpleEvent( params[0], params[-1], NULL, (char*)(params[1]?params+1:NULL) );
   (*result_length) = 0;
   return 1;
}

//************************************************************************
//  HandleIssueSimpleEvent - Issue a Simple Event
//
//
//
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//             _32 *params
//                  params[0] = the handle of the service and
//                  params+1  = a character string
//             _32 param_length  (appropriate length)
//             _32 *result        no result
//                        result[0] = a unique handle to the event(not 0)
//             _32 *result_length  INVALID_INDEX (no data)
//
//
//************************************************************************

static int CPROC HandleIssueSimpleEvent( _32 *params, _32 param_length
													, _32 *result, _32 *result_length )
{
	char msg[256];
	int ofs;
   int n;
	ofs = snprintf( msg, sizeof( msg ), WIDE("%08lx:simple event:%s"), GetTickCount(), params );
	lprintf( WIDE("msg: %s"), msg );
	// and all near, registered clients also need this message...
	{
		INDEX idx;
		PSIMPLE_EVENT event;
		LIST_FORALL( l.simple_events, idx, PSIMPLE_EVENT, event )
		{
			// do not send this event to the client originating
			// the event, it will instead have already been dispatched
         // internally by the client library.
			if( event->source_id!=params[-1] &&
				( (!event->name )
				 || ( strcmp( event->name, (char*)params ) == 0 ) ) )
			{
				if( event->sa )
				{
					for( n = 0; n < 10; n++ )
						SendUDPEx( l.pcService
									, msg
									, ofs+1
									, event->sa );
				}
			}
		}
	}
	for( n = 0; n < 10; n++ )
		SendUDPEx( l.pcService
					, msg
					, ofs+1
					, l.saBroadcast );
	if( result_length )
      (*result_length) = INVALID_INDEX;
   return 1;
}




//************************************************************************
//  registerthis
//
//
//
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//                 parm
//
//
//  RETURNS: either a one or a zero, success or failure.
//
//************************************************************************

int CPROC RegisterThis( int parm)
{
#define NUM_FUNCTIONS (sizeof(functions)/sizeof(server_function))
	static SERVER_FUNCTION functions[]
		= {
			/*[MSG_ServiceLoad] =*/ ServerFunctionEntry( HandleConnectionToThisService )
		  , /*[MSG_ServiceUnload] =*/ ServerFunctionEntry( HandleDisconnectionFromThisService )
			// these NULLS would be better skipped with C99 array init - please do look
			// that up and implement here.
			, NULL
			, NULL
		  , /*[MSG_RegisterSimpleEvent] =*/ ServerFunctionEntry( HandleRegisterSimpleEvent )
		  , /*[MSG_IssueSimpleEvent] =*/ ServerFunctionEntry( HandleIssueSimpleEvent )
		};

   // RegisterServiceHandler takes two parameters...the name of the new service, and a callback function to handle messages passed in.
	if( !(l.MsgBase = RegisterService( BARD_SERVICE_NAME, functions, NUM_FUNCTIONS ) ) )  //yes, this is an assignment, not a comparison.
	{
		LoadFunction( WIDE("sack.msgsvr.service.plugin"), NULL );

		if( !(l.MsgBase = RegisterService( BARD_SERVICE_NAME, functions, NUM_FUNCTIONS ) ) )  //yes, this is an assignment, not a comparison.
		{
			lprintf( WIDE("Sorry, could not register a service.") );
			return 0; // FALSE
		}
	}
   return 1; // TRUE
}

int TestMessage( char *text, char *buffer, int size )
{
	int chars;
	char *msg = strchr( buffer, ':' );

	if( !msg )
		return 0;
	msg++;
   lprintf( "massage is %s", msg );
   chars=strlen( text );
	if( strncmp( (char*)msg, text, chars ) == 0 )
	{
		INDEX idx;
      PRECEIVED_EVENT event;
		LIST_FORALL( l.received_events, idx, PRECEIVED_EVENT, event )
		{
         lprintf( "Comparing %s vs %s", event->data, buffer );
			if( strncmp( event->data, buffer, size ) == 0 )
            return 0;
		}
		// otherwise this is a new message, and we need to track it.
      lprintf( "Queuing new event... ");
		{
			PRECEIVED_EVENT event = New( RECEIVED_EVENT );
			event->data = NewArray(TEXTCHAR, size );
			event->tick = GetTickCount();
			MemCpy( event->data, buffer, size );
			AddLink( &l.received_events, event );
         return (msg-buffer) + chars;
		}
	}
   return 0;
}

void CPROC UDPReceive( PCLIENT pc, POINTER buffer, int size, SOCKADDR *source )
{
	if( !buffer )
	{
		buffer = Allocate( 1500 );
		// new feature as of 02/21/2006 network sends the network source address(server)
		// as the source of this... which allows comparison of packets to disregard
		// reception of self-broadcast.
      l.saMe = source;
	}
	else
	{
      int chars = 0;
#define TEST_MESSAGE(text)  TestMessage(text,(char*)buffer,size)

		if( CompareAddress( l.saMe, source ) )
		{
         lprintf( "received event from myself... ignore it?" );
		}

      lprintf( WIDE("Received UDP buffer") );
		LogBinary( (P_8)buffer, size );

		if( ( chars = TEST_MESSAGE( WIDE("register simple event:") ) ) )
		{
			char *event = ((char*)buffer) + chars;
			lprintf( "want these simple events:%s", event );
			// this client wants to receive these events...
      	CreateSimpleEvent( 0, 0, DuplicateAddress( source ), event );

		}
		if( ( chars = TEST_MESSAGE( WIDE("simple event:") ) ) )
		{
			char *event = ((char*)buffer) + chars;
         lprintf( "dispatching event %s", event );
			DispatchSimpleEvent( event ); // strip the simple event: off the message.
		}
		if( ( chars = TEST_MESSAGE( WIDE("positive event:") ) ) )
		{
         //  TO BE IMPLEMENTED!
			//LOGICAL result = DispatchPositiveEvent( );
			//// this protocol should have some sort of sequence ID so that
			//// the actual call is not made, but then we have to remember the
			//// reponses of the positive event handler
			//// plus reception of this event is delayed by the amount of time
         //// it takes to get a positive acknowledgement from the client.
         //SendUDP( pc, &result, sizeof( result ), source );
		}
	}
	ReadUDP( pc, buffer, 1500 );
}




//---------------------------------------------------------------
#ifdef WIN32
/*
 * handle windows messages - espcially those posted from 16 bit applications...
 * provides a 16 bit to 32 bit proxy bard event service
 *
 */
#define UWM_GENERATE_SIMPLE_EVENT     (WM_USER+301)

WINPROC( long, WndProc )( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   lprintf( "default window message handler... %d msg", uMsg );
	switch( uMsg )
	{
	case WM_CREATE:
		break;
	case WM_DESTROY:
		break;
	case WM_TIMER:

	case UWM_GENERATE_SIMPLE_EVENT:
		{
			char buf[257];
			GlobalGetAtomName( (ATOM)lParam, buf, sizeof( buf ) );
         lprintf( "Received dispatch event %s", buf );
			BARD_IssueSimpleEvent( buf );
			GlobalDeleteAtom( lParam );
		}
      break;
	}
   return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


int CPROC MyIdle( PTRSZVAL psv )
{
	MSG msg;

	//lprintf( WIDE("Hit my idle... which should be the main thread...") );
	if( IsThisThread( l.pMainThread  ) )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( msg.message == WM_QUIT )
			{
				exit(1);
				//g.flags.bExit = 1;
				return -1;
			}
         lprintf( WIDE("Dispatch message %d\n"), msg.message );
			DispatchMessage( &msg );
		}
	}
   return 0;
}

#endif

void CPROC CheckEventTimer( PTRSZVAL psv )
{
	INDEX idx;
	PRECEIVED_EVENT event;
	LIST_FORALL( l.received_events, idx, PRECEIVED_EVENT, event )
	{
		if( ( event->tick + 5000 ) < GetTickCount() )
		{
			SetLink( &l.received_events, idx, NULL );
			Release( event->data );
			Release( event );
		}
	}
}


//************************************************************************
//  Main.
//
//  PARAMETERS:		Name		Description
//					------------------------------------------------------
//                 argc       a count of arguments passed to the function.
//
//                 argv       variable argument data passed.
//
//
//************************************************************************
#ifdef WIN32
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow  )
#else
int main( int argc, char **argv )
#endif
{
	int retval;

	retval = 0xd1e;

   l.flags.bExit=FALSE;


#ifdef WIN32
	RegisterIcon( WIDE("Bard") );
	{
      ATOM aClass;
		WNDCLASS wc;
		memset( &wc, 0, sizeof(WNDCLASS) );
		wc.style = CS_OWNDC | CS_GLOBALCLASS;

		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = 0;
		wc.lpszClassName = "BARDClass";
		aClass = RegisterClass( &wc );
		if( !aClass )
		{
			//MessageBox( NULL, WIDE("Failed to register class to handle SQL Proxy messagses."), WIDE("INIT FAILURE"), MB_OK );
			return 0;
		}
		l.hWnd = CreateWindowEx( 0,
										(char*)aClass,
										"BARD Service",
										0,
										0,
										0,
										0,
										0,
										HWND_MESSAGE, // Parent
										NULL, // Menu
										GetModuleHandle(NULL),
										(void*)1 );
		if( !l.hWnd )
		{
			Log1( WIDE("Failed to create window!?!?!?! %d"), GetLastError() );
			//MessageBox( NULL, WIDE("Failed to create window to handle SQL Proxy Messages"), WIDE("INIT FAILURE"), MB_OK );
			return 0;
		}
	}
#endif

	retval = RegisterThis(1);
	if( !retval )
		return 0;

	NetworkStart();
   l.saBroadcast = CreateRemote( WIDE("255.255.255.255"), BARD_SERVICE_NETWORK_PORT );
	l.pcService = ServeUDP( NULL, BARD_SERVICE_NETWORK_PORT, UDPReceive, NULL );
	if( !l.pcService )
	{
		xlprintf( LOG_ERROR )( WIDE("Service failed to be able to open his broadcast receiption port at UDP:%d"), BARD_SERVICE_NETWORK_PORT );
      exit(0);
	}
   // need to set this option, or broadcast packet may not work.
   UDPEnableBroadcast( l.pcService, TRUE );
	l.pMainThread = MakeThread();
   AddTimer( 2500, CheckEventTimer, 0 );

	{//contruct lib is required
		printf( WIDE("Okay... sending ready\n") );
		LoadComplete();
	}

	while( !l.flags.bExit )
	{
#ifdef WIN32
		MSG msg;
		if( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
		else
			break;
#else
		WakeableSleep( 5000 );
#endif
	}

   return ( retval );
}

//------------------------------------------------------------------------------
//
//$Log: bardservice.c,v $
//Revision 1.3  2005/07/27 23:26:07  christopher
//checkpoint
//
//Revision 1.2  2005/07/16 00:08:22  christopher
//checkpoint.
//
//Revision 1.1  2005/07/15 22:34:31  christopher
//inital commit
//
//




