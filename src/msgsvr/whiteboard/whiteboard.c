
#include <stdhdrs.h>
#include <timers.h>
#include <colordef.h>
#include <msgclient.h>
#include <deadstart.h>

#include "whiteboard.h"

typedef struct global_tag {
   PBOARD_LAYERSET layers;
   PBOARD_CLIENTSET clients;
   PWHITEBOARDSET boards;
} GLOBAL;

GLOBAL g;

static uintptr_t CPROC IsBoard( void *member, uintptr_t psv_name )
{
	TEXTSTR name = (TEXTSTR)psv_name;
	PWHITEBOARD board = (PWHITEBOARD)member;
	if( stricmp( name, board->name ) )
		return (uintptr_t)board;
   return 0;
}

static uintptr_t CPROC IsClient( void *member, uintptr_t psv_name )
{
	struct param_tag {
		TEXTSTR name;
      uint32_t SourceID;
	} *params = (struct param_tag*)psv_name;
	PBOARD_CLIENT client = (PBOARD_CLIENT)member;
	if( stricmp( params->name, client->name ) )
		return (uintptr_t)client;
   return 0;
}

PBOARD_CLIENT CPROC FindClientByName( TEXTSTR name, uint32_t SourceID )
{
	return (PBOARD_CLIENT)ForAllInSet( BOARD_CLIENT, g.clients, IsClient, (uintptr_t)&name );
}

PBOARD_CLIENT FindClient( PWHITEBOARD board, INDEX client_id )
{
	INDEX idx;
   PBOARD_CLIENT client;
	LIST_FORALL( board->clients, idx, PBOARD_CLIENT, client )
	{
		if( client->client_id == client_id )
         break;
	}
   return client;
}

PWHITEBOARD FindBoard( TEXTSTR name )
{
	return (PWHITEBOARD)ForAllInSet( WHITEBOARD, g.boards, IsBoard, (uintptr_t)name );
}

PBOARD_POINT CreateBoardPoint( PBOARD_LAYER layer, int32_t x, int32_t y )
{
	PBOARD_POINT point = GetFromSet( BOARD_POINT, &layer->points );
	point->x = x;
	point->y = y;
	point->c = layer->current_color;
   return point;
}

uintptr_t CPROC SendCreateLayer( POINTER p, uintptr_t psv )
{
	PBOARD_CLIENT client = (PBOARD_CLIENT)p;
	INDEX iLayer = (INDEX)psv;
	//if( !client->flags.bCreating )
	{
		SendMultiServiceEvent( client->client_id, WB_CREATE_LAYER, 1
									, &iLayer, sizeof( iLayer ) );
	}
   return 0;
}

PBOARD_LAYER GetCurrentLayer( PBOARD_CLIENT client )
{
	if( !client->current_layer )
	{
      INDEX iLayer;
		client->current_layer = GetFromSet( BOARD_LAYER, &client->layers );
      iLayer = GetMemberIndex( BOARD_LAYER, &client->layers, client->current_layer );
		client->current_layer->board = client->board;
      ForAllInSet( BOARD_CLIENT, &client->board->clients, SendCreateLayer, iLayer );
	}
   return client->current_layer;
}

uintptr_t CPROC SendSetCurrentLayer( POINTER p, uintptr_t psv )
{
	PBOARD_CLIENT client = (PBOARD_CLIENT)p;
	INDEX iLayer = (INDEX)psv;
	//if( !client->flags.bCreating )
	{
		SendMultiServiceEvent( client->client_id, WB_SET_LAYER, 1
									, &iLayer, sizeof( iLayer ) );
	}
}

PBOARD_LAYER SetCurrentLayer( PBOARD_CLIENT client, INDEX iLayer )
{
	if( !client->current_layer )
	{
		INDEX iLayer;
      uint32_t msg[2];
		client->current_layer = GetFromSet( BOARD_LAYER, &client->layers );
      iLayer = GetMemberIndex( BOARD_LAYER, &client->layers, client->current_layer );
		client->current_layer->board = client->board;
		msg[0] = GetMemberIndex( BOARD_LAYER, &client->layers, client->current_layer );
      msg[1] = client->client_id;
      ForAllInSet( BOARD_CLIENT, &client->board->clients, SendSetCurrentLayer, iLayer );
	}
   return client->current_layer;
}

void CPROC WhiteboardBeginShape( INDEX iClient, INDEX iBoard, enum SHAPE_TYPE type, int32_t x, int32_t y )
{
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD, &g.boards, iBoard );
	if( board )
	{
		PBOARD_CLIENT client = FindClient( board, iClient );
		if( client )
		{
			client->creating = type;
         CreateBoardPoint( GetCurrentLayer( client ), x, y );
		}
	}
}

void CPROC WhiteboardContinueShape( INDEX iClient, INDEX iBoard, int32_t x, int32_t y )
{
	PWHITEBOARD board = GetUsedSetMember( WHITEBOARD, &g.boards, iBoard );
	if( board )
	{
		PBOARD_CLIENT client = FindClient( board, iClient );
		if( client )
		{
			if( !client->current_layer )
			{
				// this is an error condition
            return;
			}
         CreateBoardPoint( client->current_layer, x, y );
		}
	}
}

int CPROC MessageHandler( uint32_t SourceID, uint32_t MsgID
								, uint32_t *params, uint32_t param_len
								, uint32_t *result, uint32_t *resultlen )
{

	switch( MsgID )
	{
	case WB_CONNECT:
		{
			// a new client has connected...
		}
      return TRUE;
	case WB_DISCONNECT:
		return TRUE;
	case WB_LOGIN:
		{
			PBOARD_CLIENT client = FindClientByName( (TEXTSTR)params, SourceID );
			if( !client )
			{
				client = GetFromSet( BOARD_CLIENT, &g.clients );
            client->name = StrDup( (CTEXTSTR)params );
			}
         result[0] = GetMemberIndex( BOARD_CLIENT, &g.clients, client );
			(*resultlen) = 4;
		}
      return TRUE;
	case WB_OPEN_BOARD:
		{
			PWHITEBOARD board = FindBoard( (TEXTSTR)params );
			if( !board )
			{
				board = GetFromSet( WHITEBOARD, &g.boards );
            board->name = StrDup( (TEXTSTR)params );
			}
         result[0] = GetMemberIndex( WHITEBOARD, &g.boards, board );
			(*resultlen) = 4;
		}
		return TRUE;
	case WB_FIRST_VECTOR:
      WhiteboardBeginShape( SourceID, params[0], params[1], params[2], params[3] );
		return TRUE;
	case WB_ADD_VECTOR:
      WhiteboardContinueShape( SourceID, params[0], params[1], params[2] );
      return TRUE;
	case WB_ADD_POINT:
		return TRUE;
	}
   return 0;
}

PRELOAD( RegisterMyService )
{
   RegisterServiceHandler( WIDE("Whiteboard Service"), MessageHandler );
}

int main( void )
{
	WakeableSleep( SLEEP_FOREVER );
   return 0;
}
