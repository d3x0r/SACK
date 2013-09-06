
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

static struct local_protocol_data
{
	struct json_context *context;
   struct json_context_object *request_worlds_message;
   struct json_context_object *reply_worlds_message;
} l;

void InitProtocol( void )
{
   l.context = json_create_context();
	l.request_worlds_message = json_create_object( context, "RequestWorlds" );
	json_add_object_member( context, l.request_worlds_message, "opcode", 0, JSON_Element_Integer );

	l.reply_worlds_message = json_create_object( context, "ReplyWorlds" );
	json_add_object_member( context, l.reply_worlds_message, "opcode", 0, JSON_Element_Integer );
	{
		struct json_context_object *data_object =
			json_add_object_member( context, l.reply_worlds_message, "data", 4, JSON_Element_Object );
      json_add_object_member( context, data_object, "name", 0, JSON_Element_String );

	}
}

void SendReplyWorlds( CTEXTSTR *world_names )
{
	json_build_message( l.context, l.reply_worlds_message, world_names );
}
