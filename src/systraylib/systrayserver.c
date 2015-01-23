#include <msgclient.h>
#include <systray.h>
#include <timers.h>
#include "msgid.h"


typedef struct
{
	_32 MsgBaseServer;

}LOCAL;
#define l local_server_icon_data
static LOCAL l;


static int CPROC ServiceFunction( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
	// echo the data we got back to the client...
// most other things will do useful functions in functions.

	//call the libary functions (eg. REgisterIcon) here

   xlprintf(LOG_NOISE)("Ok, got ServiceFjunction");
   *result_length = param_length;
   return 3;
}
static int CPROC ServerRegisterIcon( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
	// echo the data we got back to the client...
// most other things will do useful functions in functions.

   xlprintf(LOG_NOISE)("About to RegisterIcon for %s", (char *)params);
	RegisterIcon((char* )params);//since one parameter is known to be passed, the name of the icon

   *result_length = param_length;
   *result_length = INVALID_INDEX; //this generates no responce.

   return 4;
}
static int CPROC ServerUnregisterIcon( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
   xlprintf(LOG_NOISE)("Yeah, right.  Unregister the icon.  Sounds good.");
}
static int CPROC ServerGeneric( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
   xlprintf(LOG_ALWAYS)("Got a generic.  ");

	result[0] = MSG_LASTMESSAGE;
	result[1] = MSG_LASTEVENT;
	(*result_length) = 8;
//     *result_length = param_length;
//     *result_length = INVALID_INDEX; //this generates no responce.

   return 5;
}
static int CPROC ServerMateEnded( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
   xlprintf(LOG_ALWAYS)("Got a MateEnded. params is %lu by the way. ", &params);

   if( result_length ) // a close never expects a return, and will pass a NULL for the length.
		(*result_length) = INVALID_INDEX; // but if it happens to want a length, set NO length.
	// return success (if failure, probably does not matter, this is disregarded due to the nature
	// what this message is - the disconnect of a client that is no longer (or will shortly be
	// no longer there) )

   return 6;
}
static int CPROC ServerMateStarted( _32 *params, _32 param_length
		  , _32 *result, _32 *result_length )
{
	xlprintf(LOG_ALWAYS)("Got a MateStarted.  Returning %d"
							  , INVALID_INDEX
							  );

  	result[0] = MSG_LASTMESSAGE;
  	result[1] = MSG_LASTEVENT;
  	(*result_length) = 8;
   return 7;
}

#define NUM_FUNCTIONS (sizeof(functions)/sizeof( server_function))
SERVER_FUNCTION functions[] =
{
	ServerFunctionEntry( ServerMateEnded )
, ServerFunctionEntry( ServerMateStarted  )
, ServerFunctionEntry( ServerGeneric )
, ServerFunctionEntry( ServerGeneric )
//, [MSG_EventUser]=ServerFunctionEntry( ServiceFunction)
, [MSG_RegisterIcon]= ServerFunctionEntry( ServerRegisterIcon )
, [MSG_UnregisterIcon]= ServerFunctionEntry( ServerUnregisterIcon )
//, [MSG_ChangeIcon]= ServerFunctionEntry( ServerChangeIcon )

};



void OpenServer( void )
{
	l.MsgBaseServer = RegisterService( WIDE("systray"), functions, NUM_FUNCTIONS );
   xlprintf(LOG_ADVISORY)("l.MsgBaseServer is %lu", l.MsgBaseServer );
}

int main(void)
{
   OpenServer();
   xlprintf(LOG_ADVISORY)("Entering main.");
	while(1)
	{
		WakeableSleep(SLEEP_FOREVER);
	}
   return 1;
}

//---end server interface        this is an application.  in the makefile call it systrayserver and an app
