#include <stdhdrs.h>
#include <msgclient.h>
#include <deadstart.h>
#include <construct.h>
#include <sqlgetoption.h>
#include "summoner.h"

#ifdef __cplusplus
namespace sack { namespace task { namespace construct {
using namespace sack::msg::client;
#endif
#define l summonser_construct_local
typedef struct local_tag
{
	int init_ran;
	PSERVICE_ROUTE MsgBase;
	TEXTCHAR my_name[256];
	// handle a registry of external functions to call
	// for alive checks... otherwise be simple and claim
   // alive (at least the message services are alive)
} LOCAL;

static LOCAL l;

static int CPROC HandleSummonerEvents( PSERVICE_ROUTE SourceID, MSGIDTYPE MsgID, uint32_t *data, size_t len )
{
	switch( MsgID )
	{
	case MSG_RU_ALIVE:
		// echo the data back to the server..
		// this helps mate requests and responces...
		SendRoutedServerMessage( SourceID, MSG_IM_ALIVE, data, len );
		break;
	case MSG_DIE:
		lprintf( "Command to die, therefore I shall..." );
		exit(0);
		break;
	default:
		lprintf( "Received unknown message %" _MsgID_f " from %p", MsgID, SourceID );
		break;
	}
   return TRUE;
}

//#if 0
PRELOAD( Started )
{
#ifndef __NO_OPTIONS__
	if( SACK_GetProfileIntEx( "SACK/Summoner", "Auto register with summoner?", 0, TRUE ) )
#else
   if( 0 )
#endif
	{
		l.init_ran = 1;
		l.MsgBase = LoadServiceEx( SUMMONER_NAME, HandleSummonerEvents );
		lprintf( "Message base for service is %d", l.MsgBase );
		if( l.MsgBase )
		{
			MSGIDTYPE result;
			size_t result_length;
			result_length = sizeof( l.my_name );
			if( !TransactServerMessage( l.MsgBase, MSG_WHOAMI, NULL, 0
											  , &result, l.my_name, &result_length ) )
			{
				// since we JUST loaded it, this shold be nearly impossible to hit.
				lprintf( "Failed to find out who I am from summoner." );
				UnloadService( SUMMONER_NAME );
				l.MsgBase = NULL;
				return;
			}
			else if( result != ((MSG_WHOAMI)|SERVER_SUCCESS ) )
			{
				lprintf( "Server responce was in error... disable support" );
				UnloadService( SUMMONER_NAME );
				l.MsgBase = NULL;
				return;
			}
			else if( !result_length )
			{
				lprintf( "Summoner is not responsible for us, and requires no notifications." );
				//UnloadService( SUMMONER_NAME );
				l.MsgBase = NULL;
				return;
			}
							 //else l.my_name is my task name from sommoner.config
         lprintf( "SAY I AM STARTING with %s", l.my_name );
			if( !TransactServerMessage( l.MsgBase, MSG_IM_STARTING, l.my_name, (uint32_t)strlen( l.my_name ) + 1
											  , NULL, NULL, 0 ) )
			{
				// this should almost be guaranteed to work...
				lprintf( "Failed to send starting to summoner... disable support" );
				UnloadService( SUMMONER_NAME );
				l.MsgBase = NULL;
				return;
			}
			//lprintf( "We're starting, go ahead." );
		}
	}
	else
		l.MsgBase = NULL;

}
//#endif

 void  LoadComplete ( void )
{
	uint32_t result;
   // if we registered with the summoner...
	if( l.MsgBase )
	{
		lprintf( "Sending IM_READY to summoner...\n" );
		result = ((MSG_IM_READY) | SERVER_SUCCESS);
		if( TransactServerMessage( l.MsgBase, MSG_IM_READY, l.my_name, (uint32_t)strlen( l.my_name ) + 1
										 , NULL /*&result*/, NULL, 0 )
		  )
		{
			lprintf( "should be wait on true false: %d", result );
			if( result == ((MSG_IM_READY) | SERVER_SUCCESS) ) //-V547
			{
			}
			else
			{
				lprintf( "Summoner has somehow complained that we're started?!" );
				DebugBreak();
			}
		}
		else
		{
			lprintf( "Summoner has dissappeared.  Disabling support." );
			l.MsgBase = NULL;
		}
	}
	//else
   //   lprintf( "Service has been disabled." );
}

ATEXIT( Ended )
{
	if( l.init_ran && l.MsgBase )
	{
		lprintf( "mark ready in summoner (dispatch as ended?)" );
		LoadComplete();
		UnloadService( SUMMONER_NAME );
	}
}
#undef l
#ifdef __cplusplus
}}} //namespace sack namespace 
#endif
