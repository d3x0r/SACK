#define GAME_SERVER_MAIN
#include "game_server.h"


PTRSZVAL OnOpen( PCLIENT pc, PTRSZVAL psv )
{
	struct game_client *client = New( struct game_client );
   SetNetworkLong( pc, 0, (PTRSZVAL)client );
}

void OnClose( PCLIENT pc, PTRSZVAL psv )
{
   struct game_client *client = (struct game_client *)GetNetworkLong( pc, 0 );
}

void OnError( PCLIENT pc, PTRSZVAL psv, int error )
{
}

void OnEvent( PCLIENT pc, PTRSZVAL psv, POINTER buffer, int msglen )
{
	struct game_client *client = (struct game_client *)GetNetworkLong( pc, 0 );
   struct game_message *msg;
	if( json_parse_message( gsg.json_context, (CTEXTSTR)buffer, msglen, NULL, &msg ) )
	{
		switch( msg->MsgID )
		{
		case GameMessage_Hello:
         break;
		}

		json_dispose_message( msg );
	}
	else
	{
		// unhandled message received.
      RemoveClient( pc );
	}
}

void InitJSON( void )
{
	gsg.json_context = json_create_context();
	gsg.base_message = json_create_object( gsg.json_context, sizeof( game_message ) );
	json_add_object_member( gsg.base_message, "MsgID", offsetof( struct game_message, ID ), JSON_Element_Integer_32, 0 );
   json_add_object_member( gsg.base_message, NULL, offsetof( struct game_message, extra ), JSON_Element_Raw_Object, 0 );
}

SaneWinMain( argc, argv )
{
	InitJSON();

	NetworkWait( NULL, 1024, 1 );
 
	gsg.server = WebSocketCreate( "0.0.0.0:27843/org.d3x0r/games/directory"
										 , OnOpen
										 , OnEvent
										 , OnClose
										 , Onerror
										 , 0 );
	while( 1 )
      WakeableSleep( 10000 );
}
EndSaneWinMain()

