
#include <stdhdrs.h>
#include <json_emitter.h>

/*
{
   opcode:#
   data: {
         }
}

*/

#define l local_flatland_protocol_data

enum FlatlandMessageOpcodes
{
	Flatland_Request_Worlds,
	Flatland_Reply_Worlds,

}

struct message_out
{
	int opcode;
   POINTER *data;
};

struct message_out_world_reply
{
	int opcode;
   int count;
   POINTER *data;
};

static struct local_protocol_data
{
	struct json_context *context;
   struct json_context_object *request_worlds_message;
   struct json_context_object *reply_worlds_message;
} l;

void InitProtocol( void )
{
	l.context = json_create_context();
	{
		l.request_worlds_message = json_create_object( context, "RequestWorlds" );
		json_add_object_member( context, l.request_worlds_message, "opcode", 0, JSON_Element_Integer );
	}

	{
		l.reply_worlds_message = json_create_object( context, "ReplyWorlds" );
		json_add_object_member( context, l.reply_worlds_message, "opcode", 0, JSON_Element_Integer );
		{
			struct json_context_object *data_object =
				json_add_object_member( context, l.reply_worlds_message, "data", -1, JSON_Element_Object );
			json_add_object_member_array_pointer( context, data_object, "names", 8, JSON_Element_String, 4 );
		}
	}
}

void SendReplyWorlds( CTEXTSTR *world_names )
{
	struct message_out_world_reply msg;
	CTEXTSTR msg_buf;
	int n;
   for( n = 0; world_names[n]; n++ );
	msg.opcode = Flatland_Reply_Worlds;
   msg.names = n;
   msg.data = world_names;
	msg_buf = json_build_message( l.context, l.reply_worlds_message, &msg );
}

void SendRequestWorlds( void )
{
	struct message_out msg;
   CTEXTSTR msg_buf;
	msg.opcode = Flatland_Request_Worlds;
	msg_buf = json_build_message( l.context, l.request_worlds_message, &msg );
}

